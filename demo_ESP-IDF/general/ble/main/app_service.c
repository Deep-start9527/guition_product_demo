#include "app_service.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "host/ble_sm.h"
#include "host/ble_store.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#if CONFIG_IDF_TARGET_ESP32P4
#include "esp_hosted.h"
#include "esp_hosted_misc.h"
#endif

#ifndef CONFIG_EXAMPLE_SECURITY_PASSKEY
#define CONFIG_EXAMPLE_SECURITY_PASSKEY 0
#endif
#ifndef CONFIG_EXAMPLE_PASSKEY
#define CONFIG_EXAMPLE_PASSKEY 123456
#endif
#ifndef CONFIG_EXAMPLE_SEND_PERIOD_SECONDS
#define CONFIG_EXAMPLE_SEND_PERIOD_SECONDS 5
#endif

static const uint16_t NO_CONN = BLE_HS_CONN_HANDLE_NONE;
static const ble_uuid128_t s_nus_service = BLE_UUID128_INIT(
    0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x01,0x00,0x40,0x6e);
static const ble_uuid128_t s_nus_rx = BLE_UUID128_INIT(
    0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x02,0x00,0x40,0x6e);
static const ble_uuid128_t s_nus_tx = BLE_UUID128_INIT(
    0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x03,0x00,0x40,0x6e);

typedef enum { DATA_ECHO, DATA_ACTIVE } data_kind_t;
typedef struct { data_kind_t kind; uint32_t generation; uint16_t length; uint8_t data[BLE_DATA_MAX]; } data_req_t;
typedef enum { CTRL_ADVERTISE, CTRL_RETRY, CTRL_CLEAR_BONDS } ctrl_kind_t;
typedef struct { ctrl_kind_t kind; uint32_t operation; } ctrl_req_t;

static SemaphoreHandle_t s_lock;
static QueueHandle_t s_data_queue, s_ctrl_queue;
static app_snapshot_t s_state;
static uint16_t s_conn = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_tx_handle;
static uint8_t s_addr_type;
static bool s_stack_ready;
static uint32_t s_next_operation;
static uint32_t s_record_sequence;
void ble_store_config_init(void);

static void lock(void) { xSemaphoreTake(s_lock, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_lock); }
static uint64_t now_ms(void *unused) { (void)unused; return esp_timer_get_time() / 1000ULL; }
static void pause_ms(void *unused, unsigned ms) { (void)unused; vTaskDelay(pdMS_TO_TICKS(ms)); }

static void addr_text(char out[18], const uint8_t addr[6])
{
    snprintf(out, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

static void set_error(int error, bool retryable, const char *message)
{
    lock();
    s_state.phase = APP_ERROR;
    s_state.error = error;
    s_state.retryable = retryable;
    snprintf(s_state.message, sizeof(s_state.message), "%s (%d)", message, error);
    unlock();
}

static void record_data(bool tx, const uint8_t *data, size_t length)
{
    lock();
    if (s_state.history_count == BLE_HISTORY_MAX) {
        memmove(&s_state.history[0], &s_state.history[1],
                sizeof(s_state.history[0]) * (BLE_HISTORY_MAX - 1));
        s_state.history_evicted++;
        s_state.history_count--;
    }
    app_record_t *r = &s_state.history[s_state.history_count++];
    memset(r, 0, sizeof(*r));
    r->sequence = ++s_record_sequence;
    r->tx = tx;
    r->length = length;
    r->preview_len = length < BLE_PREVIEW_MAX ? length : BLE_PREVIEW_MAX;
    memcpy(r->data, data, r->preview_len);
    if (length > r->preview_len) s_state.preview_truncated++;
    if (tx) { s_state.tx_packets++; s_state.tx_bytes += length; }
    else { s_state.rx_packets++; s_state.rx_bytes += length; }
    unlock();
}

static ble_link_t link_copy(uint16_t *conn)
{
    ble_link_t link;
    lock(); link = s_state.link; *conn = s_conn; unlock();
    return link;
}

static int send_check(void *unused, uint32_t generation, uint16_t *mtu)
{
    (void)unused;
    uint16_t conn;
    ble_link_t link = link_copy(&conn);
    int rc = ble_policy_tx(&link, CONFIG_EXAMPLE_SECURITY_PASSKEY, generation);
    if (rc == BLE_POLICY_STALE) { lock(); s_state.stale_requests++; unlock(); }
    *mtu = link.mtu;
    return rc;
}

static int send_submit(void *unused, const uint8_t *data, size_t length, uint32_t generation)
{
    (void)unused; (void)generation;
    uint16_t conn;
    (void)link_copy(&conn);
    struct os_mbuf *om = ble_hs_mbuf_from_flat(data, length);
    if (!om) return 1;
    int rc = ble_gatts_notify_custom(conn, s_tx_handle, om); /* consumes om */
    if (rc == BLE_HS_ENOMEM || rc == BLE_HS_EBUSY || rc == BLE_HS_EAGAIN) return 1;
    return rc ? -1 : 0;
}

static void handle_data(const data_req_t *request)
{
    static const ble_send_hooks_t hooks = {
        .now_ms = now_ms, .pause_ms = pause_ms, .submit = send_submit, .check = send_check,
    };
    size_t accepted = 0;
    int rc = ble_policy_send(&hooks, request->data, request->length,
                             request->generation, &accepted);
    if (rc == BLE_POLICY_OK) {
        record_data(true, request->data, request->length);
    } else {
        lock();
        s_state.notify_failures++;
        if (request->kind == DATA_ECHO) {
            s_state.no_echo_packets++;
            s_state.no_echo_bytes += request->length;
        }
        snprintf(s_state.message, sizeof(s_state.message), "Notify failed: policy %d", rc);
        unlock();
    }
}

static int nus_access(uint16_t conn_handle, uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    (void)attr_handle; (void)arg;
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) return BLE_ATT_ERR_WRITE_NOT_PERMITTED;
    size_t length = OS_MBUF_PKTLEN(ctxt->om);
    ble_link_t link;
    lock(); link = s_state.link; unlock();
    int policy = ble_policy_rx(&link, CONFIG_EXAMPLE_SECURITY_PASSKEY, length);
    if (policy == BLE_POLICY_SECURITY) return BLE_ATT_ERR_INSUFFICIENT_AUTHEN;
    if (policy == BLE_POLICY_LENGTH) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    uint16_t active_conn;
    (void)link_copy(&active_conn);
    if (policy != BLE_POLICY_OK || conn_handle != active_conn) return BLE_ATT_ERR_UNLIKELY;

    data_req_t request = { .kind = DATA_ECHO, .generation = link.generation, .length = length };
    if (os_mbuf_copydata(ctxt->om, 0, length, request.data) != 0) return BLE_ATT_ERR_UNLIKELY;
    if (xQueueSend(s_data_queue, &request, 0) != pdTRUE) {
        lock(); s_state.drop_packets++; s_state.drop_bytes += length; unlock();
        return BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    record_data(false, request.data, request.length);
    return 0;
}

static const struct ble_gatt_svc_def s_services[] = { {
    .type = BLE_GATT_SVC_TYPE_PRIMARY, .uuid = &s_nus_service.u,
    .characteristics = (struct ble_gatt_chr_def[]) {
        { .uuid = &s_nus_rx.u, .access_cb = nus_access,
          .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP |
                   (CONFIG_EXAMPLE_SECURITY_PASSKEY ? (BLE_GATT_CHR_F_WRITE_ENC | BLE_GATT_CHR_F_WRITE_AUTHEN) : 0),
          .min_key_size = CONFIG_EXAMPLE_SECURITY_PASSKEY ? 16 : 0 },
        { .uuid = &s_nus_tx.u, .access_cb = nus_access, .flags = BLE_GATT_CHR_F_NOTIFY,
          .val_handle = &s_tx_handle }, { 0 }
    }
}, { 0 } };

static void enqueue_advertise(void)
{
    ctrl_req_t request = { .kind = CTRL_ADVERTISE };
    if (xQueueSend(s_ctrl_queue, &request, 0) != pdTRUE) {
        set_error(BLE_HS_ENOMEM, true, "Advertising request queue full");
    }
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    struct ble_gap_conn_desc desc;
    switch (event->type) {
#if defined(BLE_GAP_EVENT_LINK_ESTAB)
    case BLE_GAP_EVENT_LINK_ESTAB:
        if (event->link_estab.status != 0) { enqueue_advertise(); return 0; }
        if (ble_gap_conn_find(event->link_estab.conn_handle, &desc) != 0) return 0;
#else
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status != 0) { enqueue_advertise(); return 0; }
        if (ble_gap_conn_find(event->connect.conn_handle, &desc) != 0) return 0;
#endif
        lock();
        s_conn = desc.conn_handle;
        s_state.phase = APP_CONNECTED;
        s_state.link.connected = true;
        s_state.link.subscribed = false;
        s_state.link.encrypted = desc.sec_state.encrypted;
        s_state.link.authenticated = desc.sec_state.authenticated;
        s_state.link.bonded = desc.sec_state.bonded;
        s_state.link.key_size = desc.sec_state.key_size;
        s_state.link.mtu = 23;
        s_state.link.generation++;
        addr_text(s_state.peer, desc.peer_ota_addr.val);
        snprintf(s_state.message, sizeof(s_state.message), "Connected");
        unlock();
        if (CONFIG_EXAMPLE_SECURITY_PASSKEY) (void)ble_gap_security_initiate(s_conn);
        return 0;
    case BLE_GAP_EVENT_DISCONNECT:
        lock();
        s_conn = NO_CONN;
        s_state.link.connected = false;
        s_state.link.subscribed = false;
        s_state.link.encrypted = s_state.link.authenticated = s_state.link.bonded = false;
        s_state.link.key_size = 0;
        s_state.link.mtu = 23;
        s_state.link.generation++;
        s_state.peer[0] = 0;
        unlock();
        enqueue_advertise();
        return 0;
    case BLE_GAP_EVENT_SUBSCRIBE:
        if (event->subscribe.attr_handle == s_tx_handle) {
            lock(); s_state.link.subscribed = event->subscribe.cur_notify; unlock();
        }
        return 0;
    case BLE_GAP_EVENT_MTU:
        lock(); s_state.link.mtu = event->mtu.value; unlock();
        return 0;
    case BLE_GAP_EVENT_ENC_CHANGE:
        if (ble_gap_conn_find(event->enc_change.conn_handle, &desc) == 0) {
            lock();
            s_state.link.encrypted = desc.sec_state.encrypted;
            s_state.link.authenticated = desc.sec_state.authenticated;
            s_state.link.bonded = desc.sec_state.bonded;
            s_state.link.key_size = desc.sec_state.key_size;
            unlock();
        }
        return 0;
    case BLE_GAP_EVENT_PASSKEY_ACTION: {
        if (event->passkey.params.action != BLE_SM_IOACT_DISP) return BLE_HS_EINVAL;
        struct ble_sm_io io = { .action = event->passkey.params.action,
                                .passkey = CONFIG_EXAMPLE_PASSKEY };
        return ble_sm_inject_io(event->passkey.conn_handle, &io);
    }
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0)
            (void)ble_store_util_delete_peer(&desc.peer_id_addr);
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        enqueue_advertise();
        return 0;
    default: return 0;
    }
}

static int advertise_once(void)
{
    struct ble_hs_adv_fields fields = {0};
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = &s_nus_service;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc) return rc;
    struct ble_hs_adv_fields response = {0};
    response.name = (uint8_t *)s_state.name;
    response.name_len = strlen(s_state.name);
    response.name_is_complete = 1;
    rc = ble_gap_adv_rsp_set_fields(&response);
    if (rc) return rc;
    struct ble_gap_adv_params params = { .conn_mode = BLE_GAP_CONN_MODE_UND,
                                         .disc_mode = BLE_GAP_DISC_MODE_GEN };
    return ble_gap_adv_start(s_addr_type, NULL, BLE_HS_FOREVER, &params, gap_event, NULL);
}

static void advertise_with_retry(uint32_t operation)
{
    int rc = BLE_HS_EBUSY;
    for (unsigned attempt = 1; attempt <= 3; ++attempt) {
        rc = advertise_once();
        if (rc == 0 || rc == BLE_HS_EALREADY) {
            lock();
            s_state.phase = APP_ADVERTISING; s_state.error = 0; s_state.retryable = false;
            s_state.completed_operation = operation;
            snprintf(s_state.message, sizeof(s_state.message), "Advertising");
            unlock(); return;
        }
        if (attempt < 3) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    set_error(rc, true, "Advertising failed after 3 attempts");
}

static void clear_bonds(uint32_t operation)
{
    uint16_t conn; (void)link_copy(&conn);
    if (conn != NO_CONN) {
        (void)ble_gap_terminate(conn, BLE_ERR_REM_USER_CONN_TERM);
        uint64_t deadline = now_ms(NULL) + 2000;
        do { vTaskDelay(pdMS_TO_TICKS(20)); (void)link_copy(&conn); } while (conn != NO_CONN && now_ms(NULL) < deadline);
        if (conn != NO_CONN) { set_error(BLE_HS_ETIMEOUT, true, "Disconnect before bond clear timed out"); return; }
    }
    int rc = ble_store_clear();
    if (rc) { set_error(rc, true, "Bond clear failed"); return; }
    advertise_with_retry(operation);
}

static void host_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    if (!rc) rc = ble_hs_id_infer_auto(0, &s_addr_type);
    uint8_t addr[6] = {0};
    if (!rc) rc = ble_hs_id_copy_addr(s_addr_type, addr, NULL);
    if (rc) { set_error(rc, false, "BLE identity address failed"); return; }
    lock(); addr_text(s_state.address, addr); unlock();
    enqueue_advertise();
}

static void host_reset(int reason) { set_error(reason, true, "NimBLE host reset"); }
static void host_task(void *arg)
{
    (void)arg; nimble_port_run(); nimble_port_freertos_deinit();
}

static esp_err_t stack_init(void)
{
    esp_err_t rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        rc = nvs_flash_erase();
        if (rc == ESP_OK) rc = nvs_flash_init();
    }
    if (rc != ESP_OK) return rc;
#if CONFIG_IDF_TARGET_ESP32P4
#if CONFIG_EXAMPLE_COMPILE_ONLY
    return ESP_ERR_NOT_SUPPORTED;
#endif
    rc = esp_hosted_connect_to_slave();
    if (rc != ESP_OK) return rc;
    size_t mac_len = esp_hosted_iface_mac_addr_len_get(ESP_MAC_BT);
    uint8_t hosted_mac[6] = {0};
    if (mac_len != sizeof(hosted_mac)) return ESP_ERR_NOT_SUPPORTED;
    rc = esp_hosted_iface_mac_addr_get(hosted_mac, mac_len, ESP_MAC_BT);
    if (rc != ESP_OK) return rc;
    lock();
    snprintf(s_state.name, sizeof(s_state.name), "Guition-BLE-%02X%02X",
             hosted_mac[4], hosted_mac[5]);
    unlock();
    rc = esp_hosted_bt_controller_init();
    if (rc != ESP_OK) return rc;
    rc = esp_hosted_bt_controller_enable();
    if (rc != ESP_OK) return rc;
#endif
    rc = nimble_port_init();
    if (rc != ESP_OK) return rc;
    ble_hs_cfg.reset_cb = host_reset;
    ble_hs_cfg.sync_cb = host_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    if (CONFIG_EXAMPLE_SECURITY_PASSKEY) {
        ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
        ble_hs_cfg.sm_bonding = 1;
        ble_hs_cfg.sm_mitm = 1;
        ble_hs_cfg.sm_sc = 1;
        ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
        ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    } else {
        ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
        ble_hs_cfg.sm_bonding = 0;
        ble_hs_cfg.sm_mitm = 0;
    }
    ble_svc_gap_init();
    ble_svc_gatt_init();
    int hrc = ble_gatts_count_cfg(s_services);
    if (!hrc) hrc = ble_gatts_add_svcs(s_services);
    if (hrc) return ESP_FAIL;
    hrc = ble_svc_gap_device_name_set(s_state.name);
    if (hrc) return ESP_FAIL;
    ble_store_config_init();
    s_stack_ready = true;
    nimble_port_freertos_init(host_task);
    return ESP_OK;
}

static void service_task(void *arg)
{
    (void)arg;
    esp_err_t rc = stack_init();
    if (rc != ESP_OK) {
#if CONFIG_IDF_TARGET_ESP32P4 && CONFIG_EXAMPLE_COMPILE_ONLY
        set_error(rc, false, "Compile-only P4 config: BLE runtime intentionally disabled");
#else
        set_error(rc, false, "BLE initialization failed; reboot after fixing configuration");
#endif
    }
    uint64_t next_periodic = now_ms(NULL) + (uint64_t)CONFIG_EXAMPLE_SEND_PERIOD_SECONDS * 1000;
    while (true) {
        ctrl_req_t ctrl;
        if (xQueueReceive(s_ctrl_queue, &ctrl, pdMS_TO_TICKS(20)) == pdTRUE) {
            if (!s_stack_ready) continue;
            if (ctrl.kind == CTRL_CLEAR_BONDS) clear_bonds(ctrl.operation);
            else advertise_with_retry(ctrl.operation);
        }
        data_req_t data;
        if (xQueueReceive(s_data_queue, &data, 0) == pdTRUE) handle_data(&data);
#if CONFIG_EXAMPLE_PERIODIC_SEND
        if (now_ms(NULL) >= next_periodic) {
            const char *text = CONFIG_EXAMPLE_SEND_TEXT;
            (void)app_service_send((const uint8_t *)text, strnlen(text, BLE_DATA_MAX + 1));
            next_periodic = now_ms(NULL) + (uint64_t)CONFIG_EXAMPLE_SEND_PERIOD_SECONDS * 1000;
        }
#else
        (void)next_periodic;
#endif
    }
}

esp_err_t app_service_init(void)
{
    if (s_lock) return ESP_ERR_INVALID_STATE;
    s_lock = xSemaphoreCreateMutex();
    s_data_queue = xQueueCreate(BLE_QUEUE_DEPTH, sizeof(data_req_t));
    s_ctrl_queue = xQueueCreate(BLE_QUEUE_DEPTH, sizeof(ctrl_req_t));
    if (!s_lock || !s_data_queue || !s_ctrl_queue) return ESP_ERR_NO_MEM;
    memset(&s_state, 0, sizeof(s_state));
    s_state.phase = APP_STARTING;
    s_state.link.mtu = 23;
    s_state.secure_mode = CONFIG_EXAMPLE_SECURITY_PASSKEY;
    snprintf(s_state.message, sizeof(s_state.message), "Starting BLE");
#if CONFIG_IDF_TARGET_ESP32P4
    /* The BT MAC belongs to the C6 and is read through ESP-Hosted after the
     * constructor-created transport connects in stack_init(). */
    snprintf(s_state.name, sizeof(s_state.name), "Guition-BLE-P4");
#else
    uint8_t mac[6] = {0};
    esp_err_t mac_rc = esp_read_mac(mac, ESP_MAC_BT);
    if (mac_rc != ESP_OK) return mac_rc;
    snprintf(s_state.name, sizeof(s_state.name), "Guition-BLE-%02X%02X", mac[1], mac[0]);
#endif
    if (xTaskCreate(service_task, "ble_service", 6144, NULL, 5, NULL) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

esp_err_t app_service_send(const uint8_t *data, size_t length)
{
    if (!data || length > BLE_DATA_MAX) return ESP_ERR_INVALID_ARG;
    data_req_t request = { .kind = DATA_ACTIVE, .length = length };
    lock(); request.generation = s_state.link.generation; unlock();
    memcpy(request.data, data, length);
    if (xQueueSend(s_data_queue, &request, 0) != pdTRUE) return ESP_ERR_TIMEOUT;
    return ESP_OK;
}

static esp_err_t submit_control(ctrl_kind_t kind)
{
    ctrl_req_t request = { .kind = kind };
    lock(); request.operation = ++s_next_operation; s_state.operation = request.operation; unlock();
    return xQueueSend(s_ctrl_queue, &request, 0) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}
esp_err_t app_service_retry(void) { return submit_control(CTRL_RETRY); }
esp_err_t app_service_clear_bonds(void) { return submit_control(CTRL_CLEAR_BONDS); }
void app_service_snapshot(app_snapshot_t *out)
{
    if (!out) return;
    if (!s_lock) { memset(out, 0, sizeof(*out)); return; }
    lock(); *out = s_state; unlock();
}
