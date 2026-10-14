/*
 * SPDX-FileCopyrightText: 2025-2026 Guition
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_axs15260.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AXS15260";

#define AXS15260_REG_VERSION              (0x0C)
#define AXS15260_MAX_TOUCH_NUMBER         (5)
#define AXS15260_TOUCH_HEAD_PACKET        (2)
#define AXS15260_TOUCH_POINT_SIZE         (6)
#define AXS15260_TOUCH_BUF_SIZE           ((AXS15260_MAX_TOUCH_NUMBER * AXS15260_TOUCH_POINT_SIZE) + \
                                           AXS15260_TOUCH_HEAD_PACKET)

/* AXS15260D report layout. Each point is six bytes long. */
#define AXS15260_GESTURE_POS              (0)
#define AXS15260_POINT_NUM_POS            (1)
#define AXS15260_POINT_EVENT_X_HIGH_OFFSET    (0)
#define AXS15260_POINT_X_LOW_OFFSET           (1)
#define AXS15260_POINT_TRACK_ID_Y_HIGH_OFFSET (2)
#define AXS15260_POINT_Y_LOW_OFFSET           (3)
#define AXS15260_POINT_WEIGHT_OFFSET          (4)

/* The high nibble of the first byte in each point is the event field. */
#define AXS15260_EVENT_DOWN               (0x00)
#define AXS15260_EVENT_CONTACT            (0x08)

static esp_err_t esp_lcd_touch_axs15260_read_data(esp_lcd_touch_handle_t tp);
static bool esp_lcd_touch_axs15260_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                                          uint16_t *strength, uint8_t *point_num, uint8_t max_point_num);
static esp_err_t esp_lcd_touch_axs15260_get_track_id(esp_lcd_touch_handle_t tp, uint8_t *track_id,
                                                      uint8_t point_num);
static esp_err_t esp_lcd_touch_axs15260_del(esp_lcd_touch_handle_t tp);
static esp_err_t esp_lcd_touch_axs15260_enter_sleep(esp_lcd_touch_handle_t tp);
static esp_err_t esp_lcd_touch_axs15260_exit_sleep(esp_lcd_touch_handle_t tp);
static esp_err_t esp_lcd_touch_axs15260_reset(esp_lcd_touch_handle_t tp);

static void axs15260_clear_points(esp_lcd_touch_handle_t tp)
{
    portENTER_CRITICAL(&tp->data.lock);
    tp->data.points = 0;
    portEXIT_CRITICAL(&tp->data.lock);
}

static esp_err_t esp_lcd_touch_axs15260_read_version(esp_lcd_touch_handle_t tp)
{
    uint8_t version[2] = {0};
    esp_err_t ret = esp_lcd_panel_io_rx_param(tp->io, AXS15260_REG_VERSION,
                                               version, sizeof(version));
    ESP_RETURN_ON_ERROR(ret, TAG, "Failed to read AXS15260 firmware version");

    ESP_LOGI(TAG, "Firmware version number is 0x%02x%02x, INT GPIO%d level=%d",
             version[0], version[1], tp->config.int_gpio_num,
             tp->config.int_gpio_num == GPIO_NUM_NC ? -1 : gpio_get_level(tp->config.int_gpio_num));
    return ESP_OK;
}

static esp_err_t esp_lcd_touch_axs15260_configure_gpio(esp_lcd_touch_handle_t tp)
{
    esp_err_t ret;

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t rst_gpio_config = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = BIT64(tp->config.rst_gpio_num),
        };
        ret = gpio_config(&rst_gpio_config);
        ESP_RETURN_ON_ERROR(ret, TAG, "Reset GPIO config failed");

        ret = esp_lcd_touch_axs15260_reset(tp);
        ESP_RETURN_ON_ERROR(ret, TAG, "AXS15260 reset failed");
    }

    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        const gpio_config_t int_gpio_config = {
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = tp->config.levels.interrupt ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE,
            .pull_down_en = tp->config.levels.interrupt ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
            .intr_type = tp->config.levels.interrupt ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE,
            .pin_bit_mask = BIT64(tp->config.int_gpio_num),
        };
        ret = gpio_config(&int_gpio_config);
        ESP_RETURN_ON_ERROR(ret, TAG, "Interrupt GPIO config failed");

        if (tp->config.interrupt_callback != NULL) {
            ret = esp_lcd_touch_register_interrupt_callback(tp, tp->config.interrupt_callback);
            ESP_RETURN_ON_ERROR(ret, TAG, "Interrupt callback registration failed");
        }
    }

    return ESP_OK;
}

esp_err_t esp_lcd_touch_new_i2c_axs15260(const esp_lcd_panel_io_handle_t io,
                                         const esp_lcd_touch_config_t *config,
                                         esp_lcd_touch_handle_t *out_touch)
{
    ESP_RETURN_ON_FALSE(io != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller IO handle can't be NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the touch controller configuration can't be NULL");
    ESP_RETURN_ON_FALSE(out_touch != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the touch controller handle can't be NULL");

    *out_touch = NULL;

    esp_lcd_touch_handle_t tp = heap_caps_calloc(1, sizeof(*tp), MALLOC_CAP_DEFAULT);
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_NO_MEM, TAG, "No memory for AXS15260 controller");

    tp->io = io;
    tp->read_data = esp_lcd_touch_axs15260_read_data;
    tp->get_xy = esp_lcd_touch_axs15260_get_xy;
    tp->get_track_id = esp_lcd_touch_axs15260_get_track_id;
    tp->del = esp_lcd_touch_axs15260_del;
    tp->enter_sleep = esp_lcd_touch_axs15260_enter_sleep;
    tp->exit_sleep = esp_lcd_touch_axs15260_exit_sleep;
    tp->data.lock.owner = portMUX_FREE_VAL;
    memcpy(&tp->config, config, sizeof(tp->config));

    esp_err_t ret = esp_lcd_touch_axs15260_configure_gpio(tp);
    if (ret == ESP_OK) {
        ret = esp_lcd_touch_axs15260_read_version(tp);
    }
    if (ret != ESP_OK) {
        tp->config.interrupt_callback = NULL;
        esp_lcd_touch_axs15260_del(tp);
        return ret;
    }

    *out_touch = tp;
    return ESP_OK;
}

static esp_err_t esp_lcd_touch_axs15260_read_data(esp_lcd_touch_handle_t tp)
{
    uint8_t frame[AXS15260_TOUCH_BUF_SIZE] = {0};
    esp_lcd_touch_point_data_t points[CONFIG_ESP_LCD_TOUCH_MAX_POINTS] = {0};
    uint8_t point_num;
    uint8_t cached_points = 0;

    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    esp_err_t ret = esp_lcd_panel_io_rx_param(tp->io, -1, frame, sizeof(frame));
    if (ret != ESP_OK) {
        axs15260_clear_points(tp);
        ESP_LOGW(TAG, "Touch report read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (frame[AXS15260_GESTURE_POS] > 0x0F || frame[AXS15260_POINT_NUM_POS] == 0xFF) {
        axs15260_clear_points(tp);
        return ESP_OK;
    }

    const uint8_t status = frame[AXS15260_POINT_NUM_POS] >> 4;
    if (status != 0 && status != 0x04 && status != 0x08) {
        axs15260_clear_points(tp);
        return ESP_OK;
    }

    point_num = frame[AXS15260_POINT_NUM_POS] & 0x0F;
    if (point_num == 0 || point_num > AXS15260_MAX_TOUCH_NUMBER ||
            point_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        axs15260_clear_points(tp);
        return ESP_OK;
    }

    uint8_t seen_ids = 0;
    for (uint8_t i = 0; i < point_num; i++) {
        const size_t offset = AXS15260_TOUCH_HEAD_PACKET + ((size_t)i * AXS15260_TOUCH_POINT_SIZE);
        const uint8_t event = frame[offset + AXS15260_POINT_EVENT_X_HIGH_OFFSET] >> 4;
        const uint8_t track_id = frame[offset + AXS15260_POINT_TRACK_ID_Y_HIGH_OFFSET] >> 4;

        if (event != AXS15260_EVENT_DOWN && event != AXS15260_EVENT_CONTACT) {
            continue;
        }
        if (track_id >= AXS15260_MAX_TOUCH_NUMBER || (seen_ids & BIT(track_id)) != 0) {
            continue;
        }
        if (cached_points >= CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
            break;
        }

        const uint16_t point_x = ((frame[offset + AXS15260_POINT_EVENT_X_HIGH_OFFSET] & 0x0F) << 8) |
                                 frame[offset + AXS15260_POINT_X_LOW_OFFSET];
        const uint16_t point_y = ((frame[offset + AXS15260_POINT_TRACK_ID_Y_HIGH_OFFSET] & 0x0F) << 8) |
                                 frame[offset + AXS15260_POINT_Y_LOW_OFFSET];

        if ((tp->config.x_max != 0 && point_x >= tp->config.x_max) ||
                (tp->config.y_max != 0 && point_y >= tp->config.y_max)) {
            continue;
        }

        seen_ids |= BIT(track_id);
        points[cached_points].track_id = track_id;
        points[cached_points].x = point_x;
        points[cached_points].y = point_y;
        points[cached_points].strength = frame[offset + AXS15260_POINT_WEIGHT_OFFSET];
        cached_points++;
    }

    portENTER_CRITICAL(&tp->data.lock);
    tp->data.points = cached_points;
    if (cached_points > 0) {
        memcpy(tp->data.coords, points, sizeof(points[0]) * cached_points);
    }
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static bool esp_lcd_touch_axs15260_get_xy(esp_lcd_touch_handle_t tp, uint16_t *x, uint16_t *y,
                                          uint16_t *strength, uint8_t *point_num, uint8_t max_point_num)
{
    ESP_RETURN_ON_FALSE(tp != NULL, false, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(x != NULL, false, TAG, "Pointer to the x coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(y != NULL, false, TAG, "Pointer to the y coordinates array can't be NULL");
    ESP_RETURN_ON_FALSE(point_num != NULL, false, TAG, "Pointer to number of touch points can't be NULL");
    ESP_RETURN_ON_FALSE(max_point_num > 0, false, TAG, "Array size must be equal or larger than 1");

    portENTER_CRITICAL(&tp->data.lock);
    *point_num = tp->data.points > max_point_num ? max_point_num : tp->data.points;
    for (uint8_t i = 0; i < *point_num; i++) {
        x[i] = tp->data.coords[i].x;
        y[i] = tp->data.coords[i].y;
        if (strength != NULL) {
            strength[i] = tp->data.coords[i].strength;
        }
    }
    portEXIT_CRITICAL(&tp->data.lock);

    return *point_num > 0;
}

static esp_err_t esp_lcd_touch_axs15260_get_track_id(esp_lcd_touch_handle_t tp, uint8_t *track_id,
                                                      uint8_t point_num)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(track_id != NULL, ESP_ERR_INVALID_ARG, TAG,
                        "Pointer to the track ID array can't be NULL");
    ESP_RETURN_ON_FALSE(point_num > 0, ESP_ERR_INVALID_ARG, TAG,
                        "Number of touch points must be larger than 0");

    if (point_num > CONFIG_ESP_LCD_TOUCH_MAX_POINTS) {
        point_num = CONFIG_ESP_LCD_TOUCH_MAX_POINTS;
    }

    portENTER_CRITICAL(&tp->data.lock);
    for (uint8_t i = 0; i < point_num; i++) {
        track_id[i] = tp->data.coords[i].track_id;
    }
    portEXIT_CRITICAL(&tp->data.lock);

    return ESP_OK;
}

static esp_err_t esp_lcd_touch_axs15260_enter_sleep(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(tp->config.rst_gpio_num != GPIO_NUM_NC, ESP_ERR_NOT_SUPPORTED,
                        TAG, "AXS15260 sleep requires a reset GPIO");

    ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset),
                        TAG, "Reset GPIO set level failed");
    vTaskDelay(pdMS_TO_TICKS(20));
    return ESP_OK;
}

static esp_err_t esp_lcd_touch_axs15260_exit_sleep(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");
    ESP_RETURN_ON_FALSE(tp->config.rst_gpio_num != GPIO_NUM_NC, ESP_ERR_NOT_SUPPORTED,
                        TAG, "AXS15260 wake requires a reset GPIO");

    return esp_lcd_touch_axs15260_reset(tp);
}

static esp_err_t esp_lcd_touch_axs15260_reset(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, tp->config.levels.reset),
                            TAG, "Reset GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
        ESP_RETURN_ON_ERROR(gpio_set_level(tp->config.rst_gpio_num, !tp->config.levels.reset),
                            TAG, "Reset GPIO set level failed");
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return ESP_OK;
}

static esp_err_t esp_lcd_touch_axs15260_del(esp_lcd_touch_handle_t tp)
{
    ESP_RETURN_ON_FALSE(tp != NULL, ESP_ERR_INVALID_ARG, TAG, "Touch controller handle can't be NULL");

    if (tp->config.int_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.int_gpio_num);
        if (tp->config.interrupt_callback != NULL) {
            gpio_isr_handler_remove(tp->config.int_gpio_num);
        }
    }
    if (tp->config.rst_gpio_num != GPIO_NUM_NC) {
        gpio_reset_pin(tp->config.rst_gpio_num);
    }

    free(tp);
    return ESP_OK;
}
