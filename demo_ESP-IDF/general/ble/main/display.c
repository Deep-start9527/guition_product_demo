#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_board_manager.h"
#include "esp_board_manager_includes.h"
#include "esp_lv_adapter.h"
#include "guition_touch.h"
#include "display.h"

#ifndef CONFIG_EXAMPLE_TOUCH_ENABLED
#define CONFIG_EXAMPLE_TOUCH_ENABLED 0
#endif

static const char *TAG = "Display";
static lv_display_t *s_disp;
static bool s_touch_registered;

esp_err_t display_init(void)
{
    if (s_disp) {
        return ESP_ERR_INVALID_STATE;
    }
    bool lcd_ready = false, touch_ready = false, brightness_ready = false;
    bool adapter_ready = false;
    dev_display_lcd_handles_t *lcd = NULL;
    dev_display_lcd_config_t *lcd_cfg = NULL;
    esp_lcd_touch_handle_t touch = NULL;
    esp_err_t ret = esp_board_manager_init_device_by_name("display_lcd");
    if (ret != ESP_OK) {
        return ret;
    }
    lcd_ready = true;
    ret = esp_board_manager_get_device_handle("display_lcd", (void **)&lcd);
    if (ret != ESP_OK || !lcd || !lcd->panel_handle) {
        if (ret == ESP_OK) ret = ESP_ERR_INVALID_STATE;
        goto fail;
    }
    ret = esp_board_manager_get_device_config("display_lcd", (void **)&lcd_cfg);
    if (ret != ESP_OK || !lcd_cfg) {
        if (ret == ESP_OK) ret = ESP_ERR_INVALID_STATE;
        goto fail;
    }

    // Initialize only selected display devices. BMGR acquires their buses lazily.
    // In particular, CAP/NONE must not initialize a conflicting resistive bus.
    if (CONFIG_EXAMPLE_TOUCH_ENABLED && esp_board_manager_check_name("lcd_touch")) {
        ret = esp_board_manager_init_device_by_name("lcd_touch");
        if (ret != ESP_OK) goto fail;
        touch_ready = true;
#if CONFIG_GUITION_BOARD_TOUCH_ADAPTER
        guition_touch_device_t *device = NULL;
        ret = esp_board_manager_get_device_handle("lcd_touch", (void **)&device);
        if (ret != ESP_OK) goto fail;
        touch = guition_touch_get_handle(device);
#elif CONFIG_ESP_BOARD_DEV_LCD_TOUCH_SUPPORT
        dev_lcd_touch_handles_t *device = NULL;
        ret = esp_board_manager_get_device_handle("lcd_touch", (void **)&device);
        if (ret != ESP_OK) goto fail;
        if (device) touch = device->touch_handle;
#endif
    }

    esp_lv_adapter_display_config_t display_config;
    if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_DSI) == 0) {
        display_config = ESP_LV_ADAPTER_DISPLAY_MIPI_DEFAULT_CONFIG(
            lcd->panel_handle, lcd->io_handle, lcd_cfg->lcd_width, lcd_cfg->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
    } else if (strcmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_SPI) == 0) {
#if CONFIG_SPIRAM
        if (strcmp(lcd_cfg->chip, "axs15231b") == 0) {
            // AXS QSPI video mode cannot address dirty rectangles. Submit one
            // complete frame; IDF splits this transfer while keeping CS active.
            display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_TE_HIGH_PERFORMANCE_CONFIG(
                lcd->panel_handle, lcd->io_handle, lcd_cfg->lcd_width, lcd_cfg->lcd_height,
                ESP_LV_ADAPTER_ROTATE_90, 38, 40000000, 4, 16);
            // ASYNC_FULL rotates into a persistent full-frame staging buffer.
            // Disable adapter row submissions: each would restart AXS RAMWR.
            display_config.te_sync.bounce_buffer_enabled = false;
            display_config.te_sync.bounce_buffer_rows = 0;
            display_config.te_sync.time_tvdl_ms = 13;
            display_config.te_sync.time_tvdh_ms = 3;
            display_config.te_sync.intr_type = GPIO_INTR_NEGEDGE;
        } else {
            display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
                lcd->panel_handle, lcd->io_handle, lcd_cfg->lcd_width, lcd_cfg->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
        }
#else
        // Adapter owns a 10-line DMA buffer until the SPI completion callback.
        display_config = ESP_LV_ADAPTER_DISPLAY_SPI_WITHOUT_PSRAM_DEFAULT_CONFIG(
            lcd->panel_handle, lcd->io_handle, lcd_cfg->lcd_width, lcd_cfg->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
#endif
    } else if (strncmp(lcd_cfg->sub_type, ESP_BOARD_DEVICE_LCD_SUB_TYPE_RGB, 3) == 0) {
        display_config = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
            lcd->panel_handle, lcd->io_handle, lcd_cfg->lcd_width, lcd_cfg->lcd_height, ESP_LV_ADAPTER_ROTATE_0);
        // S3 board configs allocate two panel frame buffers, matching Arduino.
#if CONFIG_IDF_TARGET_ESP32S3
        display_config.tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT;
#endif
        display_config.profile.use_psram = true;
        display_config.profile.buffer_height = lcd_cfg->lcd_height;
    } else {
        ret = ESP_ERR_NOT_SUPPORTED;
        goto fail;
    }

    esp_lv_adapter_config_t adapter_config = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    adapter_config.stack_in_psram = false;
    ret = esp_lv_adapter_init(&adapter_config);
    if (ret != ESP_OK) goto fail;
    adapter_ready = true;
    s_disp = esp_lv_adapter_register_display(&display_config);
    if (!s_disp) {
        ret = ESP_ERR_NO_MEM;
        goto fail;
    }
    if (touch) {
        esp_lv_adapter_touch_config_t touch_config = {
            .disp = s_disp, .handle = touch, .scale = { .x = 1.0f, .y = 1.0f },
        };
        if (!esp_lv_adapter_register_touch(&touch_config)) {
            ret = ESP_ERR_NO_MEM;
            goto fail;
        }
        s_touch_registered = true;
    }
    ret = esp_lv_adapter_start();
    if (ret != ESP_OK) goto fail;
    if (esp_board_manager_check_name("lcd_brightness")) {
        ret = esp_board_manager_init_device_by_name("lcd_brightness");
        if (ret != ESP_OK) goto fail;
        brightness_ready = true;
    }
    return ESP_OK;

fail:
    ESP_LOGE(TAG, "Display initialization failed: %s", esp_err_to_name(ret));
    // Keep hardware alive if the adapter cannot release outstanding transfers.
    if (adapter_ready) {
        esp_err_t cleanup = esp_lv_adapter_deinit();
        if (cleanup != ESP_OK) {
            ESP_LOGE(TAG, "Adapter cleanup failed: %s", esp_err_to_name(cleanup));
            return ret;
        }
    }
    s_disp = NULL;
    s_touch_registered = false;
#if CONFIG_GUITION_BOARD_TOUCH_ADAPTER
    esp_err_t pending_cleanup = guition_touch_bmgr_recover();
    if (pending_cleanup != ESP_OK) {
        ESP_LOGE(TAG, "Pending touch cleanup failed: %s", esp_err_to_name(pending_cleanup));
    }
#endif
    const char *names[] = {"lcd_brightness", "lcd_touch", "display_lcd"};
    const bool ready[] = {brightness_ready, touch_ready, lcd_ready};
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (ready[i]) {
            esp_err_t cleanup = esp_board_manager_deinit_device_by_name(names[i]);
            if (cleanup != ESP_OK) {
                ESP_LOGE(TAG, "Cleanup of %s failed: %s", names[i], esp_err_to_name(cleanup));
            }
        }
    }
    return ret;
}

bool lvgl_display_lock(int timeout_ms)
{
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

void lvgl_display_unlock(void)
{
    esp_lv_adapter_unlock();
}

bool display_has_touch(void)
{
    return s_touch_registered;
}
