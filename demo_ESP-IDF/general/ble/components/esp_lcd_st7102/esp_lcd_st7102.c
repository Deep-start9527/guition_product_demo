#include <stdlib.h>
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_st7102.h"

#if SOC_LCD_RGB_SUPPORTED
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"

static const char *TAG = "ST7102_LCD";

typedef struct {
    esp_lcd_panel_io_handle_t io;
    const st7102_lcd_init_cmd_t *commands;
    size_t command_count;
    int reset_gpio;
    bool reset_high;
    bool reset_configured;
    bool display_by_gpio;
    bool default_commands;
    bool bgr;
    esp_err_t (*rgb_init)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_reset)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_del)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_display)(esp_lcd_panel_t *panel, bool on);
} st7102_panel_t;

#include "st7102_init_commands.h"

static void panel_delay(unsigned ms)
{
    if (ms) {
        vTaskDelay(pdMS_TO_TICKS(ms) + 2);
    }
}

static esp_err_t st7102_panel_init(esp_lcd_panel_t *panel)
{
    st7102_panel_t *st = panel->user_data;
    for (size_t i = 0; i < st->command_count; ++i) {
        const st7102_lcd_init_cmd_t *cmd = &st->commands[i];
        const void *data = cmd->data;
        uint8_t madctl;
        if (st->default_commands && cmd->cmd == 0x36 && cmd->data_bytes == 1) {
            madctl = (*(const uint8_t *)data & (uint8_t)~0x08) | (st->bgr ? 0x08 : 0);
            data = &madctl;
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(st->io, cmd->cmd, data, cmd->data_bytes),
                            TAG, "Display init command 0x%02x failed", cmd->cmd);
        panel_delay(cmd->delay_ms);
    }
    return st->rgb_init(panel);
}

static esp_err_t st7102_panel_reset(esp_lcd_panel_t *panel)
{
    st7102_panel_t *st = panel->user_data;
    if (st->reset_configured) {
        ESP_RETURN_ON_ERROR(gpio_set_level(st->reset_gpio, st->reset_high), TAG, "Assert reset failed");
        panel_delay(10);
        ESP_RETURN_ON_ERROR(gpio_set_level(st->reset_gpio, !st->reset_high), TAG, "Release reset failed");
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(st->io, 0x01, NULL, 0), TAG, "Software reset failed");
    }
    panel_delay(120);
    return st->rgb_reset(panel);
}

static esp_err_t st7102_panel_del(esp_lcd_panel_t *panel)
{
    st7102_panel_t *st = panel->user_data;
    ESP_RETURN_ON_ERROR(st->rgb_del(panel), TAG, "RGB panel deletion failed");
    if (st->reset_configured) {
        gpio_reset_pin(st->reset_gpio);
    }
    free(st);
    return ESP_OK;
}

static esp_err_t st7102_panel_display(esp_lcd_panel_t *panel, bool on)
{
    st7102_panel_t *st = panel->user_data;
    if (st->display_by_gpio) {
        return st->rgb_display(panel, on);
    }
    return esp_lcd_panel_io_tx_param(st->io, on ? 0x29 : 0x28, NULL, 0);
}

static esp_err_t st7102_panel_invert(esp_lcd_panel_t *panel, bool invert)
{
    st7102_panel_t *st = panel->user_data;
    return esp_lcd_panel_io_tx_param(st->io, invert ? 0x21 : 0x20, NULL, 0);
}

esp_err_t esp_lcd_new_panel_st7102(esp_lcd_panel_io_handle_t io,
                                  const esp_lcd_panel_dev_config_t *config,
                                  esp_lcd_panel_handle_t *out)
{
    ESP_RETURN_ON_FALSE(out, ESP_ERR_INVALID_ARG, TAG, "NULL output");
    *out = NULL;
    ESP_RETURN_ON_FALSE(io && config && config->vendor_config, ESP_ERR_INVALID_ARG, TAG, "Missing IO/config");
    const st7102_vendor_config_t *vendor = config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor->rgb_config, ESP_ERR_INVALID_ARG, TAG, "Missing RGB config");
    ESP_RETURN_ON_FALSE(config->reset_gpio_num == GPIO_NUM_NC || GPIO_IS_VALID_OUTPUT_GPIO(config->reset_gpio_num),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid reset GPIO");
    ESP_RETURN_ON_FALSE(config->bits_per_pixel == 16 && vendor->rgb_config->data_width == 16,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Only 16-bit RGB supported");
    ESP_RETURN_ON_FALSE(config->rgb_ele_order == LCD_RGB_ELEMENT_ORDER_RGB ||
                        config->rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Unsupported element order");
    ESP_RETURN_ON_FALSE((vendor->init_cmds != NULL) == (vendor->init_cmds_size != 0),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid custom command table");
    if (!vendor->init_cmds) {
        ESP_RETURN_ON_FALSE(vendor->rgb_config->timings.h_res == 480 && vendor->rgb_config->timings.v_res == 480,
                            ESP_ERR_NOT_SUPPORTED, TAG, "Default sequence requires 480x480");
    }
    for (size_t i = 0; i < vendor->init_cmds_size; ++i) {
        ESP_RETURN_ON_FALSE(vendor->init_cmds[i].cmd >= 0 && vendor->init_cmds[i].cmd <= 0xFF &&
                            (!vendor->init_cmds[i].data_bytes || vendor->init_cmds[i].data),
                            ESP_ERR_INVALID_ARG, TAG, "Invalid custom command");
    }
    st7102_panel_t *st = calloc(1, sizeof(*st));
    ESP_RETURN_ON_FALSE(st, ESP_ERR_NO_MEM, TAG, "No memory for display");
    st->io = io;
    st->commands = vendor->init_cmds ? vendor->init_cmds : st7102_default_init_cmds;
    st->command_count = vendor->init_cmds ? vendor->init_cmds_size :
                        sizeof(st7102_default_init_cmds) / sizeof(st7102_default_init_cmds[0]);
    st->default_commands = vendor->init_cmds == NULL;
    st->bgr = config->rgb_ele_order == LCD_RGB_ELEMENT_ORDER_BGR;
    st->reset_gpio = config->reset_gpio_num;
    st->reset_high = config->flags.reset_active_high;
    st->display_by_gpio = vendor->rgb_config->disp_gpio_num >= 0;
    esp_err_t ret;
    if (st->reset_gpio != GPIO_NUM_NC) {
        const gpio_config_t gpio = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << st->reset_gpio,
        };
        ret = gpio_config(&gpio);
        if (ret != ESP_OK) {
            goto fail;
        }
        st->reset_configured = true;
    }
    esp_lcd_panel_handle_t panel = NULL;
    ret = esp_lcd_new_rgb_panel(vendor->rgb_config, &panel);
    if (ret != ESP_OK) {
        goto fail;
    }
    /* Keep the native RGB handle, as required by RGB frame-buffer/event APIs. */
    st->rgb_init = panel->init;
    st->rgb_reset = panel->reset;
    st->rgb_del = panel->del;
    st->rgb_display = panel->disp_on_off;
    panel->user_data = st;
    panel->init = st7102_panel_init;
    panel->reset = st7102_panel_reset;
    panel->del = st7102_panel_del;
    panel->disp_on_off = st7102_panel_display;
    panel->invert_color = st7102_panel_invert;
    *out = panel;
    return ESP_OK;
fail:
    if (st->reset_configured) {
        gpio_reset_pin(st->reset_gpio);
    }
    free(st);
    return ret;
}
#endif /* SOC_LCD_RGB_SUPPORTED */
