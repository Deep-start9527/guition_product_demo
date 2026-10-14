/* SPDX-License-Identifier: MIT */
#include <stdlib.h>
#include "sdkconfig.h"
#include "esp_board_periph.h"
#include "esp_log.h"
#include "guition_touch.h"
#ifdef CONFIG_ESP_BOARD_PERIPH_SPI_SUPPORT
#include "periph_spi.h"
#endif

static const char *TAG = "guition_touch";

struct guition_touch_device {
    const char *bus_name;
    esp_lcd_panel_io_handle_t io;
    esp_lcd_touch_handle_t touch;
    bool bus_acquired;
};

static guition_touch_device_t *s_pending_cleanup;

guition_touch_mode_t guition_touch_selected_mode(void)
{
#if defined(CONFIG_GUITION_TOUCH_CAPACITIVE)
    return GUITION_TOUCH_MODE_CAPACITIVE;
#elif defined(CONFIG_GUITION_TOUCH_RESISTIVE)
    return GUITION_TOUCH_MODE_RESISTIVE;
#else
    return GUITION_TOUCH_MODE_NONE;
#endif
}

esp_lcd_touch_handle_t guition_touch_get_handle(const guition_touch_device_t *device)
{
    return device ? device->touch : NULL;
}

esp_err_t guition_touch_destroy(guition_touch_device_t *device)
{
    if (!device) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    if (device->touch) {
        ret = esp_lcd_touch_del(device->touch);
        if (ret != ESP_OK) {
            return ret;
        }
        device->touch = NULL;
    }
    if (device->io) {
        ret = esp_lcd_panel_io_del(device->io);
        if (ret != ESP_OK) {
            return ret;
        }
        device->io = NULL;
    }
    if (device->bus_acquired) {
        ret = esp_board_periph_unref_handle(device->bus_name);
        if (ret != ESP_OK) {
            return ret;
        }
        device->bus_acquired = false;
    }
    free(device);
    return ESP_OK;
}

static esp_err_t create_io(const guition_touch_config_t *config,
                           guition_touch_device_t *device)
{
    void *bus = NULL;
    esp_err_t ret = esp_board_periph_ref_handle(config->bus_name, &bus);
    if (ret != ESP_OK) {
        return ret;
    }
    device->bus_acquired = true;
    if (!bus) {
        return ESP_ERR_INVALID_STATE;
    }
    if (config->bus == GUITION_TOUCH_BUS_I2C) {
#ifdef CONFIG_ESP_BOARD_PERIPH_I2C_SUPPORT
        return esp_lcd_new_panel_io_i2c(bus, &config->io.i2c, &device->io);
#else
        return ESP_ERR_NOT_SUPPORTED;
#endif
    }
#ifdef CONFIG_ESP_BOARD_PERIPH_SPI_SUPPORT
    const periph_spi_handle_t *spi = bus;
    return esp_lcd_new_panel_io_spi(spi->spi_port, &config->io.spi, &device->io);
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t guition_touch_create(const guition_touch_config_t *config,
                              guition_touch_device_t **out_device)
{
    if (!out_device) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_device = NULL;
    const guition_touch_mode_t selected = guition_touch_selected_mode();
    if (selected != GUITION_TOUCH_MODE_NONE) {
        if (!config || !config->bus_name || !config->bus_name[0] || !config->factory ||
            config->mode != selected || config->touch.x_max == 0 || config->touch.y_max == 0 ||
            (config->bus != GUITION_TOUCH_BUS_I2C && config->bus != GUITION_TOUCH_BUS_SPI)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (config->bus == GUITION_TOUCH_BUS_SPI &&
            (config->io.spi.cs_gpio_num < 0 || config->io.spi.pclk_hz <= 0)) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    guition_touch_device_t *device = calloc(1, sizeof(*device));
    if (!device) {
        return ESP_ERR_NO_MEM;
    }
    if (selected == GUITION_TOUCH_MODE_NONE) {
        *out_device = device;
        return ESP_OK;
    }
    device->bus_name = config->bus_name;
    esp_err_t ret = create_io(config, device);
    if (ret == ESP_OK && !device->io) {
        ret = ESP_ERR_INVALID_STATE;
    }
    if (ret == ESP_OK) {
        ret = config->factory(device->io, &config->touch, &device->touch);
        if (ret == ESP_OK && !device->touch) {
            ret = ESP_ERR_INVALID_STATE;
        }
    }
    if (ret != ESP_OK) {
        const esp_err_t cleanup = guition_touch_destroy(device);
        if (cleanup != ESP_OK) {
            // Return the retained object so the caller can retry cleanup. BMGR
            // setup must not discard it when propagating an init failure.
            *out_device = device;
            ESP_LOGE(TAG, "Touch init failed (%s), cleanup needs retry (%s)",
                     esp_err_to_name(ret), esp_err_to_name(cleanup));
        }
        return ret;
    }
    *out_device = device;
    return ESP_OK;
}

esp_err_t guition_touch_bmgr_recover(void)
{
    if (!s_pending_cleanup) {
        return ESP_OK;
    }
    esp_err_t ret = guition_touch_destroy(s_pending_cleanup);
    if (ret == ESP_OK) {
        s_pending_cleanup = NULL;
    }
    return ret;
}

esp_err_t guition_touch_bmgr_create(const guition_touch_config_t *config, void **out_device)
{
    if (!out_device) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_device = NULL;
    esp_err_t ret = guition_touch_bmgr_recover();
    if (ret != ESP_OK) {
        return ret;
    }
    guition_touch_device_t *device = NULL;
    ret = guition_touch_create(config, &device);
    if (ret != ESP_OK) {
        s_pending_cleanup = device;
        return ret;
    }
    *out_device = device;
    return ESP_OK;
}

esp_err_t guition_touch_bmgr_destroy(void *device)
{
    return guition_touch_destroy(device);
}
