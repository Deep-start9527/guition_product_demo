#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default I2C device address of the ST7102 touch controller.
 */
#define ESP_LCD_TOUCH_IO_I2C_ST7102_ADDRESS (0x55)

/**
 * @brief Default I2C panel IO configuration for ST7102 touch.
 *
 * @note The caller creates and owns the panel IO and I2C bus.
 */
#define ESP_LCD_TOUCH_IO_I2C_ST7102_CONFIG()              \
    {                                                  \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_ST7102_ADDRESS, \
        .scl_speed_hz = 100000,                         \
        .control_phase_bytes = 1,                      \
        .dc_bit_offset = 0,                            \
        .lcd_cmd_bits = 16,                            \
        .lcd_param_bits = 8,                           \
        .flags = { .disable_control_phase = 1 },        \
    }

/**
 * @brief Create an ST7102 touch controller with installed firmware.
 *
 * @note The IO must send 16-bit, MSB-first register addresses without a control byte.
 * @note Use GPIO_NUM_NC for unused pins and for a reset pin shared with the display.
 * @note The caller retains IO ownership and should stop touch access before deletion.
 *
 * @param[in] io Touch panel IO handle
 * @param[in] config Touch configuration
 * @param[out] out_touch Created touch handle; NULL on failure
 * @return ESP_OK on success; an error code otherwise
 */
esp_err_t esp_lcd_touch_new_i2c_st7102(esp_lcd_panel_io_handle_t io,
                                       const esp_lcd_touch_config_t *config,
                                       esp_lcd_touch_handle_t *out_touch);

#ifdef __cplusplus
}
#endif
