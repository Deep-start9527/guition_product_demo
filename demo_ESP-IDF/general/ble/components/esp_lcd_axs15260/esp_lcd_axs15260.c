/*
 * SPDX-FileCopyrightText: 2025-2026 Guition
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_axs15260.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if SOC_LCD_RGB_SUPPORTED

static const char *TAG = "AXS15260_LCD";

typedef struct {
    esp_lcd_panel_io_handle_t io;
    int reset_gpio;
    bool reset_high;
    bool reset_configured;
    esp_err_t (*rgb_init)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_reset)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_del)(esp_lcd_panel_t *panel);
    esp_err_t (*rgb_display)(esp_lcd_panel_t *panel, bool on);
} axs15260_panel_t;

#include "axs15260_init_commands.h"

static void panel_delay(unsigned ms)
{
    if (ms) {
        vTaskDelay(pdMS_TO_TICKS(ms) + 2);
    }
}

static esp_err_t panel_send_init(axs15260_panel_t *axs,
                                 const axs15260_lcd_init_cmd_t *commands, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        const axs15260_lcd_init_cmd_t *cmd = &commands[i];
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(axs->io, cmd->cmd, cmd->data, cmd->data_bytes),
                            TAG, "Display init command 0x%02x failed", cmd->cmd);
        panel_delay(cmd->delay_ms);
    }
    return ESP_OK;
}

static esp_err_t axs15260_panel_init(esp_lcd_panel_t *panel)
{
    axs15260_panel_t *axs = panel->user_data;
    return axs->rgb_init(panel);
}

static esp_err_t axs15260_panel_reset(esp_lcd_panel_t *panel)
{
    axs15260_panel_t *axs = panel->user_data;
    /* The controller reset and register sequence run during construction so
     * the 3-wire IO can be released before a shared touch IRQ is configured. */
    return axs->rgb_reset(panel);
}

static esp_err_t axs15260_panel_del(esp_lcd_panel_t *panel)
{
    axs15260_panel_t *axs = panel->user_data;
    ESP_RETURN_ON_ERROR(axs->rgb_del(panel), TAG, "RGB panel deletion failed");
    if (axs->reset_configured) {
        gpio_reset_pin(axs->reset_gpio);
    }
    free(axs);
    return ESP_OK;
}

static esp_err_t axs15260_panel_display(esp_lcd_panel_t *panel, bool on)
{
    axs15260_panel_t *axs = panel->user_data;
    return axs->rgb_display(panel, on);
}

static esp_err_t axs15260_controller_reset(axs15260_panel_t *axs)
{
    if (axs->reset_configured) {
        ESP_RETURN_ON_ERROR(gpio_set_level(axs->reset_gpio, axs->reset_high), TAG, "Assert reset failed");
        panel_delay(10);
        ESP_RETURN_ON_ERROR(gpio_set_level(axs->reset_gpio, !axs->reset_high), TAG, "Release reset failed");
    } else {
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(axs->io, 0x01, NULL, 0), TAG, "Software reset failed");
    }
    panel_delay(120);
    return ESP_OK;
}

esp_err_t esp_lcd_new_panel_axs15260(esp_lcd_panel_io_handle_t io,
                                     const esp_lcd_panel_dev_config_t *config,
                                     esp_lcd_panel_handle_t *out)
{
    ESP_RETURN_ON_FALSE(out, ESP_ERR_INVALID_ARG, TAG, "NULL output");
    *out = NULL;
    ESP_RETURN_ON_FALSE(io && config && config->vendor_config, ESP_ERR_INVALID_ARG, TAG,
                        "Missing IO/config");
    const axs15260_vendor_config_t *vendor = config->vendor_config;
    ESP_RETURN_ON_FALSE(vendor->rgb_config, ESP_ERR_INVALID_ARG, TAG, "Missing RGB config");
    ESP_RETURN_ON_FALSE((vendor->init_cmds != NULL) == (vendor->init_cmds_size != 0),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid custom command table");
    ESP_RETURN_ON_FALSE(config->bits_per_pixel == 16 && vendor->rgb_config->data_width == 16,
                        ESP_ERR_NOT_SUPPORTED, TAG, "Only RGB565 is supported");
    ESP_RETURN_ON_FALSE(config->reset_gpio_num == GPIO_NUM_NC ||
                        GPIO_IS_VALID_OUTPUT_GPIO(config->reset_gpio_num),
                        ESP_ERR_INVALID_ARG, TAG, "Invalid reset GPIO");

    const axs15260_lcd_init_cmd_t *commands = vendor->init_cmds ? vendor->init_cmds :
                                                   axs15260_default_init_cmds;
    const size_t command_count = vendor->init_cmds ? vendor->init_cmds_size :
        sizeof(axs15260_default_init_cmds) / sizeof(axs15260_default_init_cmds[0]);
    for (size_t i = 0; i < command_count; ++i) {
        ESP_RETURN_ON_FALSE(commands[i].cmd >= 0 && commands[i].cmd <= 0xFF &&
                            (!commands[i].data_bytes || commands[i].data),
                            ESP_ERR_INVALID_ARG, TAG, "Invalid init command");
    }

    axs15260_panel_t *axs = calloc(1, sizeof(*axs));
    ESP_RETURN_ON_FALSE(axs, ESP_ERR_NO_MEM, TAG, "No memory for display");
    axs->io = io;
    axs->reset_gpio = config->reset_gpio_num;
    axs->reset_high = config->flags.reset_active_high;

    esp_err_t ret;
    if (axs->reset_gpio != GPIO_NUM_NC) {
        const gpio_config_t gpio = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << axs->reset_gpio,
        };
        ret = gpio_config(&gpio);
        if (ret != ESP_OK) {
            goto fail;
        }
        axs->reset_configured = true;
    }

    esp_lcd_panel_handle_t panel = NULL;
    ret = esp_lcd_new_rgb_panel(vendor->rgb_config, &panel);
    if (ret != ESP_OK) {
        goto fail;
    }
    axs->rgb_init = panel->init;
    axs->rgb_reset = panel->reset;
    axs->rgb_del = panel->del;
    axs->rgb_display = panel->disp_on_off;
    panel->user_data = axs;
    panel->init = axs15260_panel_init;
    panel->reset = axs15260_panel_reset;
    panel->del = axs15260_panel_del;
    panel->disp_on_off = axs15260_panel_display;

    ret = axs15260_controller_reset(axs);
    if (ret == ESP_OK) {
        ret = panel_send_init(axs, commands, command_count);
    }
    if (ret != ESP_OK) {
        axs->rgb_del(panel);
        if (axs->reset_configured) {
            gpio_reset_pin(axs->reset_gpio);
        }
        free(axs);
        return ret;
    }

    /* The board manager deletes its generic IO as soon as this factory
     * returns. Delete it here too for standalone callers and shared GPIOs. */
    ret = esp_lcd_panel_io_del(io);
    if (ret != ESP_OK) {
        axs->rgb_del(panel);
        if (axs->reset_configured) {
            gpio_reset_pin(axs->reset_gpio);
        }
        free(axs);
        return ret;
    }
    axs->io = NULL;
    *out = panel;
    return ESP_OK;

fail:
    if (axs->reset_configured) {
        gpio_reset_pin(axs->reset_gpio);
    }
    free(axs);
    return ret;
}

#endif /* SOC_LCD_RGB_SUPPORTED */
