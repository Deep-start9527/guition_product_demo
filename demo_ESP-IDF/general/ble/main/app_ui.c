#include "app_ui.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "lvgl.h"
#include "app_service.h"

#ifndef CONFIG_EXAMPLE_SECURITY_PASSKEY
#define CONFIG_EXAMPLE_SECURITY_PASSKEY 0
#endif
#ifndef CONFIG_EXAMPLE_PASSKEY
#define CONFIG_EXAMPLE_PASSKEY 123456
#endif
#ifndef CONFIG_EXAMPLE_PERIODIC_SEND
#define CONFIG_EXAMPLE_PERIODIC_SEND 0
#endif
#ifndef CONFIG_EXAMPLE_HEX_PREFIX
#define CONFIG_EXAMPLE_HEX_PREFIX 0
#endif

enum {
    UI_PAGE_PADDING = 12,
    UI_KEYBOARD_MAX_WIDTH = 640,
    UI_KEYBOARD_MIN_HEIGHT = 160,
    UI_KEYBOARD_MAX_HEIGHT = 256,
};

typedef struct {
    lv_obj_t *home, *send_page;
    lv_obj_t *title, *phase, *identity, *link, *security, *stats, *history;
    lv_obj_t *retry, *clear, *send_open, *textarea, *send_result;
    app_snapshot_t snapshot;
    char history_text[8192];
    bool touch, hex_prefix;
} ui_state_t;
static ui_state_t s_ui;

static lv_color_t C_BG(void) { return lv_color_hex(0xF5F7FA); }
static lv_color_t C_CARD(void) { return lv_color_hex(0xFFFFFF); }
static lv_color_t C_TEXT(void) { return lv_color_hex(0x172033); }
static lv_color_t C_MUTED(void) { return lv_color_hex(0x526071); }
static lv_color_t C_PRIMARY(void) { return lv_color_hex(0x1457D9); }
static lv_color_t C_DANGER(void) { return lv_color_hex(0xB42318); }

static void style_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, C_BG(), 0);
    lv_obj_set_style_text_color(screen, C_TEXT(), 0);
    lv_obj_set_style_text_font(screen, LV_FONT_DEFAULT, 0);
}

static lv_obj_t *make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, C_CARD(), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xDDE3EA), 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 8, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_scrollable(card, false);
    return card;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text, bool primary)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_set_height(button, 48);
    lv_obj_set_flex_grow(button, 1);
    lv_obj_set_style_radius(button, 8, 0);
    lv_obj_set_style_bg_color(button, primary ? C_PRIMARY() : lv_color_hex(0xE7EDF8), 0);
    lv_obj_set_scrollable(button, false);
    lv_obj_t *label = lv_label_create(button);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_color(label, primary ? lv_color_hex(0xFFFFFF) : C_TEXT(), 0);
    lv_obj_center(label);
    return button;
}

static void format_history(const app_snapshot_t *s)
{
    size_t used = 0;
    s_ui.history_text[0] = 0;
    /* Keep complete recent records inside the fixed display buffer, including
     * the extra two characters per byte when prefixes are enabled. */
    unsigned first = s->history_count;
    size_t budget = 64;
    while (first > 0) {
        size_t record_size = 48 + s->history[first - 1].preview_len * (s_ui.hex_prefix ? 6 : 4);
        if (budget + record_size >= sizeof(s_ui.history_text)) break;
        budget += record_size;
        --first;
    }
    if (first) {
        used = (size_t)snprintf(s_ui.history_text, sizeof(s_ui.history_text),
                              "Showing latest %u of %u records\n\n", s->history_count - first, s->history_count);
    }
    for (unsigned i = first; i < s->history_count && used + 40 < sizeof(s_ui.history_text); ++i) {
        const app_record_t *r = &s->history[i];
        int n = snprintf(s_ui.history_text + used, sizeof(s_ui.history_text) - used,
                         "#%" PRIu32 " %s %uB%s\nT: ", r->sequence,
                         r->tx ? "TX" : "RX", r->length,
                         r->preview_len < r->length ? " (preview)" : "");
        if (n < 0) break;
        used += (size_t)n;
        for (unsigned j = 0; j < r->preview_len && used + 2 < sizeof(s_ui.history_text); ++j)
            s_ui.history_text[used++] = isprint(r->data[j]) ? (char)r->data[j] : '.';
        if (used + 5 >= sizeof(s_ui.history_text)) break;
        memcpy(s_ui.history_text + used, "\nH: ", 4);
        used += 4;
        for (unsigned j = 0; j < r->preview_len && used + 6 < sizeof(s_ui.history_text); ++j) {
            n = snprintf(s_ui.history_text + used, sizeof(s_ui.history_text) - used,
                         "%s%02X%s", s_ui.hex_prefix ? "0x" : "", r->data[j],
                         j + 1 == r->preview_len ? "" : " ");
            if (n < 0) break;
            used += (size_t)n;
        }
        if (used + 2 >= sizeof(s_ui.history_text)) break;
        s_ui.history_text[used++] = '\n'; s_ui.history_text[used++] = '\n';
        s_ui.history_text[used] = 0;
    }
    s_ui.history_text[used] = 0;
    if (!used) snprintf(s_ui.history_text, sizeof(s_ui.history_text), "No traffic yet");
}

static const char *phase_name(app_phase_t phase)
{
    switch (phase) {
    case APP_STARTING: return "STARTING";
    case APP_ADVERTISING: return "ADVERTISING";
    case APP_CONNECTED: return "CONNECTED";
    case APP_ERROR: return "ERROR";
    default: return "UNKNOWN";
    }
}

static const char *send_block_reason(const app_snapshot_t *s)
{
    switch (ble_policy_tx(&s->link, s->secure_mode, s->link.generation)) {
    case BLE_POLICY_OK: return NULL;
    case BLE_POLICY_LINK: return "Connect a phone or PC before sending";
    case BLE_POLICY_SUBSCRIPTION: return "Enable TX notifications on the phone or PC";
    case BLE_POLICY_SECURITY: return "Complete secure pairing before sending";
    default: return "Connection changed; reconnect and try again";
    }
}

static void refresh(lv_timer_t *timer)
{
    (void)timer;
    app_service_snapshot(&s_ui.snapshot);
    app_snapshot_t *s = &s_ui.snapshot;
    lv_label_set_text_fmt(s_ui.phase, "%s  |  %s", phase_name(s->phase), s->message);
    lv_obj_set_style_text_color(s_ui.phase, s->phase == APP_ERROR ? C_DANGER() : C_PRIMARY(), 0);
    lv_label_set_text_fmt(s_ui.identity, "%s\n%s", s->name[0] ? s->name : "Guition BLE", s->address[0] ? s->address : "Address pending");
    lv_label_set_text_fmt(s_ui.link, "Peer: %s\nMTU: %u   Notify: %s", s->peer[0] ? s->peer : "—", s->link.mtu,
                          s->link.subscribed ? "subscribed" : "off");
    if (s->secure_mode) {
        lv_label_set_text_fmt(s_ui.security, "Passkey: %06d\nEncrypted %s  Auth %s  Bonded %s",
                              CONFIG_EXAMPLE_PASSKEY, s->link.encrypted ? "yes" : "no",
                              s->link.authenticated ? "yes" : "no", s->link.bonded ? "yes" : "no");
    } else {
        lv_label_set_text(s_ui.security, "Security: open (pairing not required)");
    }
    lv_label_set_text_fmt(s_ui.stats,
        "RX %" PRIu32 " / %" PRIu32 "B   TX %" PRIu32 " / %" PRIu32 "B\n"
        "Dropped %" PRIu32 " / %" PRIu32 "B   No echo %" PRIu32 "B\n"
        "History evicted %" PRIu32 "   Preview cuts %" PRIu32 "   Notify errors %" PRIu32,
        s->rx_packets, s->rx_bytes, s->tx_packets, s->tx_bytes,
        s->drop_packets, s->drop_bytes, s->no_echo_bytes,
        s->history_evicted, s->preview_truncated, s->notify_failures);
    format_history(s);
    lv_label_set_text(s_ui.history, s_ui.history_text);
    lv_obj_set_hidden(s_ui.retry, !(s_ui.touch && s->retryable));
}

static void retry_cb(lv_event_t *event) { (void)event; (void)app_service_retry(); }
static void clear_cb(lv_event_t *event) { (void)event; (void)app_service_clear_bonds(); }
static void hex_prefix_cb(lv_event_t *event)
{
    s_ui.hex_prefix = lv_obj_is_checked(lv_event_get_target_obj(event));
    app_service_snapshot(&s_ui.snapshot);
    format_history(&s_ui.snapshot);
    lv_label_set_text(s_ui.history, s_ui.history_text);
}
static void open_send_cb(lv_event_t *event)
{
    (void)event;
    app_service_snapshot(&s_ui.snapshot);
    const char *reason = send_block_reason(&s_ui.snapshot);
    lv_label_set_text(s_ui.send_result, reason ? reason : "Maximum 512 bytes (UTF-8)");
    lv_obj_set_style_text_color(s_ui.send_result, C_MUTED(), 0);
    lv_screen_load(s_ui.send_page);
}
static void back_cb(lv_event_t *event) { (void)event; lv_screen_load(s_ui.home); }
static void send_cb(lv_event_t *event)
{
    (void)event;
    const char *message = lv_textarea_get_text(s_ui.textarea);
    size_t length = strlen(message);
    app_service_snapshot(&s_ui.snapshot);
    const char *reason = send_block_reason(&s_ui.snapshot);
    if (!length) reason = "Enter text before sending";
    else if (length > BLE_DATA_MAX) reason = "Text exceeds the 512-byte UTF-8 limit";
    if (reason) {
        lv_label_set_text(s_ui.send_result, reason);
        lv_obj_set_style_text_color(s_ui.send_result, C_DANGER(), 0);
        return;
    }
    esp_err_t rc = app_service_send((const uint8_t *)message, length);
    lv_label_set_text(s_ui.send_result, rc == ESP_OK ? "Queued" : "Send queue busy; try again");
    lv_obj_set_style_text_color(s_ui.send_result, rc == ESP_OK ? C_PRIMARY() : C_DANGER(), 0);
}

static void build_home(void)
{
    s_ui.home = lv_obj_create(NULL);
    style_screen(s_ui.home);
    lv_obj_set_flex_flow(s_ui.home, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ui.home, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s_ui.home, UI_PAGE_PADDING, 0);
    lv_obj_set_style_pad_row(s_ui.home, 8, 0);
    s_ui.title = make_label(s_ui.home, "Guition BLE Monitor", C_TEXT());
    s_ui.phase = make_label(s_ui.home, "STARTING", C_PRIMARY());
    lv_obj_t *identity_card = make_card(s_ui.home);
    s_ui.identity = make_label(identity_card, "Guition BLE\nAddress pending", C_TEXT());
    lv_obj_t *link_card = make_card(s_ui.home);
    s_ui.link = make_label(link_card, "Peer: —\nMTU: 23   Notify: off", C_TEXT());
    s_ui.security = make_label(link_card, "Security pending", C_MUTED());
    lv_obj_t *stats_card = make_card(s_ui.home);
    s_ui.stats = make_label(stats_card, "RX 0 / 0B   TX 0 / 0B", C_MUTED());
    if (s_ui.touch) {
        lv_obj_t *actions = lv_obj_create(s_ui.home);
        lv_obj_remove_style_all(actions);
        lv_obj_set_size(actions, lv_pct(100), 48);
        lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
        lv_obj_set_style_pad_column(actions, 8, 0);
        lv_obj_set_scrollable(actions, false);
        s_ui.send_open = make_button(actions, "Send", true);
        s_ui.clear = make_button(actions, "Clear bonds", false);
        lv_obj_add_event_cb(s_ui.send_open, open_send_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(s_ui.clear, clear_cb, LV_EVENT_CLICKED, NULL);
        s_ui.retry = make_button(s_ui.home, "Retry advertising", false);
        lv_obj_set_width(s_ui.retry, lv_pct(100));
        lv_obj_add_event_cb(s_ui.retry, retry_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_hidden(s_ui.retry, true);
    } else {
        s_ui.send_open = lv_obj_create(s_ui.home);
        s_ui.retry = lv_obj_create(s_ui.home);
        lv_obj_set_hidden(s_ui.send_open, true);
        lv_obj_set_hidden(s_ui.retry, true);
        make_label(s_ui.home, CONFIG_EXAMPLE_PERIODIC_SEND ? "No touch: periodic send enabled" : "No touch: receive/echo only", C_MUTED());
    }
    make_label(s_ui.home, "Traffic (text and hex preview)", C_TEXT());
    if (s_ui.touch) {
        lv_obj_t *hex_prefix = lv_checkbox_create(s_ui.home);
        lv_checkbox_set_text(hex_prefix, "HEX 0x prefix");
        lv_obj_set_width(hex_prefix, lv_pct(100));
        lv_obj_set_style_min_height(hex_prefix, 48, 0);
        lv_obj_set_style_pad_all(hex_prefix, 8, 0);
        lv_obj_set_checked(hex_prefix, s_ui.hex_prefix);
        lv_obj_add_event_cb(hex_prefix, hex_prefix_cb, LV_EVENT_VALUE_CHANGED, NULL);
    }
    lv_obj_t *history_card = make_card(s_ui.home);
    s_ui.history = make_label(history_card, "No traffic yet", C_TEXT());
}

static void build_send_page(void)
{
    s_ui.send_page = lv_obj_create(NULL);
    style_screen(s_ui.send_page);
    lv_obj_set_flex_flow(s_ui.send_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_ui.send_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(s_ui.send_page, UI_PAGE_PADDING, 0);
    lv_obj_set_style_pad_row(s_ui.send_page, 8, 0);
    lv_obj_t *top = lv_obj_create(s_ui.send_page);
    lv_obj_remove_style_all(top);
    lv_obj_set_size(top, lv_pct(100), 48);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(top, 8, 0);
    lv_obj_set_scrollable(top, false);
    lv_obj_t *back = make_button(top, "Back", false);
    lv_obj_t *send = make_button(top, "Send", true);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(send, send_cb, LV_EVENT_CLICKED, NULL);
    s_ui.textarea = lv_textarea_create(s_ui.send_page);
    lv_obj_set_size(s_ui.textarea, lv_pct(100), 56);
    lv_textarea_set_one_line(s_ui.textarea, true);
    lv_textarea_set_max_length(s_ui.textarea, BLE_DATA_MAX);
    lv_textarea_set_placeholder_text(s_ui.textarea, "Bytes to notify (UTF-8)");
    /* The built-in keyboard forwards OK/Enter and Close to its textarea. */
    lv_obj_add_event_cb(s_ui.textarea, send_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(s_ui.textarea, back_cb, LV_EVENT_CANCEL, NULL);
    s_ui.send_result = make_label(s_ui.send_page, "Maximum 512 bytes", C_MUTED());
    lv_obj_t *keyboard = lv_keyboard_create(s_ui.send_page);
    int32_t keyboard_width = lv_display_get_horizontal_resolution(lv_obj_get_display(s_ui.send_page)) - 2 * UI_PAGE_PADDING;
    if (keyboard_width > UI_KEYBOARD_MAX_WIDTH) keyboard_width = UI_KEYBOARD_MAX_WIDTH;
    int32_t keyboard_height = keyboard_width * 2 / 5;
    if (keyboard_height < UI_KEYBOARD_MIN_HEIGHT) keyboard_height = UI_KEYBOARD_MIN_HEIGHT;
    if (keyboard_height > UI_KEYBOARD_MAX_HEIGHT) keyboard_height = UI_KEYBOARD_MAX_HEIGHT;
    lv_obj_set_width(keyboard, lv_pct(100));
    lv_obj_set_style_max_width(keyboard, UI_KEYBOARD_MAX_WIDTH, 0);
    lv_obj_set_height(keyboard, keyboard_height);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    lv_keyboard_set_popovers(keyboard, true);
    lv_keyboard_set_textarea(keyboard, s_ui.textarea);
}

void app_ui_init(bool touch_available)
{
    memset(&s_ui, 0, sizeof(s_ui));
    s_ui.touch = touch_available;
    s_ui.hex_prefix = CONFIG_EXAMPLE_HEX_PREFIX;
    build_home();
    if (s_ui.touch) build_send_page();
    lv_screen_load(s_ui.home);
    lv_timer_create(refresh, 100, NULL);
}
