#pragma once

#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/** GSL3680 7-bit I2C address; do not use the 8-bit write address 0x80. */
#define ESP_LCD_TOUCH_IO_I2C_GSL3680_ADDRESS (0x40)

/** Optional address configuration in esp_lcd_touch_config_t.driver_data. */
typedef struct {
    uint8_t dev_addr;
} esp_lcd_touch_io_gsl3680_config_t;

/**
 * Create a GSL3680 driver using an existing I2C panel IO.
 * The caller retains ownership of the IO and I2C bus.
 * This firmware/algorithm supports one driver instance per application.
 * Configure max coordinates and GPIOs for the JC8012P4A1C panel.
 * Delete the touch handle with esp_lcd_touch_del() before deleting the IO.
 */
esp_err_t esp_lcd_touch_new_i2c_gsl3680(esp_lcd_panel_io_handle_t io,
                                      const esp_lcd_touch_config_t *config,
                                      esp_lcd_touch_handle_t *out_touch);

/** Legacy firmware health check; requires an initialized GSL3680 instance. */
esp_err_t esp_while_read(void);

/** Default panel IO settings, matching the existing 100 kHz board setup. */
#define ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG()                 \
    {                                                       \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GSL3680_ADDRESS,     \
        .scl_speed_hz = 100000,                              \
        .control_phase_bytes = 1,                           \
        .dc_bit_offset = 0,                                 \
        .lcd_cmd_bits = 8,                                  \
        .lcd_param_bits = 8,                                \
        .flags = { .disable_control_phase = 1 },             \
    }

#ifdef __cplusplus
}
#endif
