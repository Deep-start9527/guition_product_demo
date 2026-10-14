#include "esp_log.h"
#include "display.h"
#include "app_ui.h"
#include "app_service.h"

void app_main(void)
{
    esp_err_t rc = display_init();
    if (rc != ESP_OK) { ESP_LOGE("ble", "Display: %s", esp_err_to_name(rc)); return; }
    if (!lvgl_display_lock(1000)) { ESP_LOGE("ble", "Display lock timed out"); return; }
    app_ui_init(display_has_touch());
    lvgl_display_unlock();
    rc = app_service_init();
    if (rc != ESP_OK) ESP_LOGE("ble", "Service: %s", esp_err_to_name(rc));
}
