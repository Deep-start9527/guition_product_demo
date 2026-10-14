/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch_gsl3680.h"
#include "esp_log.h"

void app_main(void)
{
    /* The existing JC8012P4A1C wiring; this example only initializes touch. */
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_7,
        .scl_io_num = GPIO_NUM_8,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus));

    const esp_lcd_panel_io_i2c_config_t io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
    esp_lcd_panel_io_handle_t io = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus, &io_config, &io));

    const esp_lcd_touch_config_t touch_config = {
        .x_max = 800,
        .y_max = 1280,
        .rst_gpio_num = GPIO_NUM_22,
        .int_gpio_num = GPIO_NUM_21,
        .levels = { .reset = 0, .interrupt = 0 },
        .flags = { .mirror_x = true },
    };
    esp_lcd_touch_handle_t touch = NULL;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(io, &touch_config, &touch));

    while (true) {
        uint16_t x[3], y[3];
        uint8_t points = 0;
        esp_err_t result = esp_lcd_touch_read_data(touch);
        if (result == ESP_OK && esp_lcd_touch_get_coordinates(touch, x, y, NULL, &points, 3)) {
            for (uint8_t i = 0; i < points; ++i) {
                ESP_LOGI("gsl3680", "point %u: x=%u y=%u", i, x[i], y[i]);
            }
        } else if (result != ESP_OK) {
            ESP_LOGW("gsl3680", "read failed: %s", esp_err_to_name(result));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
