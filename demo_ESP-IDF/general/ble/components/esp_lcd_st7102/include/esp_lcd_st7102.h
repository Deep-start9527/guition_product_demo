#pragma once

#include "soc/soc_caps.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_touch_st7102.h"

#if SOC_LCD_RGB_SUPPORTED
#include "esp_lcd_panel_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ST7102 display initialization command.
 */
typedef struct {
    int cmd;                /*!< Display command */
    const void *data;       /*!< Command parameters */
    size_t data_bytes;      /*!< Parameter length in bytes */
    unsigned int delay_ms;  /*!< Delay after the command, in milliseconds */
} st7102_lcd_init_cmd_t;

/**
 * @brief ST7102 RGB panel configuration.
 *
 * @note Pass this structure through esp_lcd_panel_dev_config_t::vendor_config.
 */
typedef struct {
    const esp_lcd_rgb_panel_config_t *rgb_config; /*!< RGB panel configuration */
    const st7102_lcd_init_cmd_t *init_cmds;       /*!< Optional commands; NULL selects the default sequence */
    size_t init_cmds_size;                        /*!< Number of initialization commands */
} st7102_vendor_config_t;

/**
 * @brief Create an ST7102 RGB panel.
 *
 * @note The default initialization sequence supports a 480x480 panel with a 16-bit RGB bus.
 * @note Keep the display panel IO and any custom initialization commands valid until the panel is deleted.
 * @note Resetting the display controller also affects touch operation.
 *
 * @param[in] io Display panel IO handle for 8-bit commands and parameters
 * @param[in] panel_dev_config Panel configuration with st7102_vendor_config_t in vendor_config
 * @param[out] ret_panel Created RGB panel handle
 * @return ESP_OK on success; an error code otherwise
 */
esp_err_t esp_lcd_new_panel_st7102(esp_lcd_panel_io_handle_t io,
                                   const esp_lcd_panel_dev_config_t *panel_dev_config,
                                   esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief RGB timing for the 480x480 ST7102 panel.
 *
 * @note Matches the current JC4848F540 configuration. The nominal refresh rate is
 *       approximately 52.7 Hz: 18 MHz / (480 + 2 + 40 + 40) / (480 + 2 + 14 + 112).
 * @note Use this macro to initialize esp_lcd_rgb_panel_config_t::timings.
 */
#define ST7102_480_480_PANEL_RGB_TIMING() { \
    .pclk_hz = 18000000,                    \
    .h_res = 480,                           \
    .v_res = 480,                           \
    .hsync_pulse_width = 2,                 \
    .hsync_back_porch = 40,                 \
    .hsync_front_porch = 40,                \
    .vsync_pulse_width = 2,                 \
    .vsync_back_porch = 14,                 \
    .vsync_front_porch = 112,               \
    .flags = {                              \
        .hsync_idle_low = false,            \
        .vsync_idle_low = false,            \
        .de_idle_high = false,              \
        .pclk_active_neg = true,            \
        .pclk_idle_high = false,            \
    },                                      \
}

#ifdef __cplusplus
}
#endif
#endif /* SOC_LCD_RGB_SUPPORTED */
