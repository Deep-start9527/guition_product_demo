/*
 * SPDX-FileCopyrightText: 2025-2026 Guition
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AXS15260 RGB display controller over 3-wire SPI and RGB bus.
 */

#pragma once

#include "soc/soc_caps.h"
#include "esp_lcd_panel_vendor.h"

#if SOC_LCD_RGB_SUPPORTED
#include "esp_lcd_panel_rgb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int cmd;
    const void *data;
    size_t data_bytes;
    unsigned int delay_ms;
} axs15260_lcd_init_cmd_t;

typedef struct {
    const esp_lcd_rgb_panel_config_t *rgb_config;
    const axs15260_lcd_init_cmd_t *init_cmds; /*!< NULL selects the built-in 540x540 sequence. */
    size_t init_cmds_size;
} axs15260_vendor_config_t;

/**
 * @brief Create the AXS15260 RGB panel and send its initialization commands.
 *
 * The supplied 3-wire panel IO is consumed and deleted after the commands are
 * sent. This lets boards reuse an SPI GPIO as the touch interrupt pin. The
 * built-in command sequence is for the 540x540 RGB565 test panel.
 */
esp_err_t esp_lcd_new_panel_axs15260(esp_lcd_panel_io_handle_t io,
                                     const esp_lcd_panel_dev_config_t *panel_dev_config,
                                     esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief RGB timing for the 540x540 AXS15260 test panel.
 *
 * @note Matches the current test board configuration: 30 MHz PCLK,
 *       540x540 active pixels, and 160/160 horizontal and 20/80 vertical
 *       porches. Use this macro for esp_lcd_rgb_panel_config_t::timings.
 */
#define AXS15260_540_540_PANEL_RGB_TIMING() { \
    .pclk_hz = 30000000,                    \
    .h_res = 540,                           \
    .v_res = 540,                           \
    .hsync_pulse_width = 40,                \
    .hsync_back_porch = 160,                \
    .hsync_front_porch = 160,               \
    .vsync_pulse_width = 4,                 \
    .vsync_back_porch = 20,                 \
    .vsync_front_porch = 80,                \
    .flags = {                              \
        .hsync_idle_low = false,            \
        .vsync_idle_low = false,            \
        .de_idle_high = false,              \
        .pclk_active_neg = false,           \
        .pclk_idle_high = false,             \
    },                                      \
}

#ifdef __cplusplus
}
#endif
#endif /* SOC_LCD_RGB_SUPPORTED */
