#pragma once
#include "esp_err.h"
#include "ble_policy.h"
typedef enum { APP_STARTING, APP_ADVERTISING, APP_CONNECTED, APP_ERROR } app_phase_t;
typedef struct {
    uint32_t sequence;
    uint16_t length, preview_len;
    bool tx;
    uint8_t data[BLE_PREVIEW_MAX];
} app_record_t;
typedef struct {
    app_phase_t phase;
    ble_link_t link;
    char name[32], address[18], peer[18], message[96];
    uint32_t operation, completed_operation;
    int error, progress;
    bool retryable, secure_mode;
    uint32_t rx_bytes, tx_bytes, rx_packets, tx_packets;
    uint32_t drop_bytes, drop_packets, no_echo_bytes, no_echo_packets;
    uint32_t history_evicted, preview_truncated;
    uint32_t notify_failures, stale_requests;
    unsigned history_count;
    app_record_t history[BLE_HISTORY_MAX];
} app_snapshot_t;
esp_err_t app_service_init(void);
esp_err_t app_service_send(const uint8_t *data, size_t length);
esp_err_t app_service_retry(void);
esp_err_t app_service_clear_bonds(void);
void app_service_snapshot(app_snapshot_t *out);
