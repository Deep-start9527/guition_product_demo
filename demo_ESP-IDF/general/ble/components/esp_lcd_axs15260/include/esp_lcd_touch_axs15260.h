/*
 * SPDX-FileCopyrightText: 2025-2026 Guition
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ESP LCD touch: AXS15260/AXS15260D
 */

#pragma once

#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an AXS15260 touch driver on an ESP LCD panel IO.
 *
 * The panel IO must be configured for the controller's raw receive protocol.
 * The driver passes lcd_cmd = -1 to esp_lcd_panel_io_rx_param(), so the I2C
 * panel IO performs a receive-only transaction without a command phase.
 */
esp_err_t esp_lcd_touch_new_i2c_axs15260(const esp_lcd_panel_io_handle_t io,
                                         const esp_lcd_touch_config_t *config,
                                         esp_lcd_touch_handle_t *out_touch);

/** AXS15260/AXS15260D default 7-bit I2C address. */
#define ESP_LCD_TOUCH_IO_I2C_AXS15260_ADDRESS (0x3B)

/** Default panel IO configuration for a raw AXS15260 report read. */
#define ESP_LCD_TOUCH_IO_I2C_AXS15260_CONFIG() \
    { \
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_AXS15260_ADDRESS, \
        .scl_speed_hz = 100000, \
        .control_phase_bytes = 1, \
        .dc_bit_offset = 0, \
        .lcd_cmd_bits = 16, \
        .flags = { \
            .disable_control_phase = 1, \
        }, \
        .transaction_timeout_ms = 50, \
    }

#ifdef __cplusplus
}
#endif
