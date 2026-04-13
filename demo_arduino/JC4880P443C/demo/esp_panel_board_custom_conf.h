/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file   BOARD_ESP32_P4_FUNCTION_EV_BOARD.h
 * @brief  Configuration file for ESP32-P4-Function-EV-Board
 * @author @Deep-start9527
 * @link   
 */

#pragma once

// *INDENT-OFF*

/**
 * @brief Flag to enable custom board configuration (0/1)
 *
 * Set to `1` to enable custom board configuration, `0` to disable
 */
#define ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM  (1)
#if ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure general panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Board name
 */
#define ESP_PANEL_BOARD_NAME                "custom:JC4880P443C_I_W_Y"

/**
 * @brief Panel resolution configuration in pixels
 */
#define ESP_PANEL_BOARD_WIDTH               (480)  // Panel width (horizontal, in pixels)
#define ESP_PANEL_BOARD_HEIGHT              (800)   // Panel height (vertical, in pixels)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the LCD panel /////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief LCD panel configuration flag (0/1)
 *
 * Set to `1` to enable LCD panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_LCD             (1)

#if ESP_PANEL_BOARD_USE_LCD
/**
 * @brief LCD controller selection
 */
#define ESP_PANEL_BOARD_LCD_CONTROLLER      JD9165

/**
 * @brief LCD bus type selection
 */
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_MIPI_DSI)

/**
 * @brief LCD bus parameters configuration
 *
 * Configure parameters based on the selected bus type. Parameters for other bus types will be ignored.
 * For detailed parameter explanations, see:
 * https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/lcd/index.html
 * https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/index.html
 */
#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_MIPI_DSI

    /**
     * @brief MIPI DSI bus
     */
    /* For host */
    #define ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_NUM           (2)     // ESP32-P4 supports 1 or 2 lanes
    #define ESP_PANEL_BOARD_LCD_MIPI_DSI_LANE_RATE_MBPS     (550)  // Single lane bit rate, should check the LCD drive IC
                                                                    // datasheet for the supported lane rate. Different
                                                                    // color format (RGB565/RGB888) may have different
                                                                    // lane bit rate requirements.
                                                                    // ESP32-P4 supports max 1500Mbps
    /* For refresh panel (DPI) */
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_CLK_MHZ            (28)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_PIXEL_BITS         (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                                    // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HPW                (12)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HBP                (42)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_HFP                (42)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VPW                (2)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VBP                (8)
    #define ESP_PANEL_BOARD_LCD_MIPI_DPI_VFP                (166)
    /* For DSI power PHY */
    #define ESP_PANEL_BOARD_LCD_MIPI_PHY_LDO_ID             (3)     // -1 if not used.

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE

/**
 * @brief LCD vendor initialization commands
 *
 * Vendor specific initialization can be different between manufacturers, should consult the LCD supplier for
 * initialization sequence code. Please uncomment and change the following macro definitions. Otherwise, the LCD driver
 * will use the default initialization sequence code.
 *
 * The initialization sequence can be specified in two formats:
 * 1. Raw format:
 *    {command, (uint8_t []){data0, data1, ...}, data_size, delay_ms}
 * 2. Helper macros:
 *    - ESP_PANEL_LCD_CMD_WITH_8BIT_PARAM(delay_ms, command, {data0, data1, ...})
 *    - ESP_PANEL_LCD_CMD_WITH_NONE_PARAM(delay_ms, command)
 */

#define ESP_PANEL_BOARD_LCD_VENDOR_INIT_CMD()                       \
    {                                                               \
        {0xFF, (uint8_t []){0x77,0x01,0x00,0x00,0x13},5,0},         \
        {0xEF, (uint8_t []){0x08}, 1, 0},                           \   
        {0xFF, (uint8_t []){0x77,0x01,0x00,0x00,0x10},5,0},         \
        {0xC0, (uint8_t []){0x63, 0x00}, 2, 0},                     \
        {0xC1, (uint8_t []){0x0D, 0x02}, 2, 0},                     \
        {0xC2, (uint8_t []){0x10, 0x08}, 2, 0},                     \
        {0xCC, (uint8_t []){0x10}, 1, 0},                           \
        {0xB0, (uint8_t []){0x80, 0x09, 0x53, 0x0C, 0xD0, 0x07, 0x0C, 0x09, 0x09, 0x28, 0x06, 0xD4, 0x13, 0x69, 0x2B, 0x71}, 16, 0},    \
        {0xB1, (uint8_t []){0x80, 0x94, 0x5A, 0x10, 0xD3, 0x06, 0x0A, 0x08, 0x08, 0x25, 0x03, 0xD3, 0x12, 0x66, 0x6A, 0x0D}, 16, 0},    \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},   \
        {0xB0, (uint8_t []){0x5D}, 1, 0},                           \
        {0xB1, (uint8_t []){0x58}, 1, 0},                           \
        {0xB2, (uint8_t []){0x87}, 1, 0},                           \
        {0xB3, (uint8_t []){0x80}, 1, 0},                           \ 
        {0xB5, (uint8_t []){0x4E}, 1, 0},                           \
        {0xB7, (uint8_t []){0x85}, 1, 0},                           \
        {0xB8, (uint8_t []){0x21}, 1, 0},                           \   
        {0xB9, (uint8_t []){0x10, 0x1F}, 2, 0},                     \
        {0xBB, (uint8_t []){0x03}, 1,0},                            \
        {0xBC, (uint8_t []){0x00}, 1,0},                            \
        {0xC1, (uint8_t []){0x78}, 1, 0},                           \
        {0xC2, (uint8_t []){0x78}, 1, 0},                           \
        {0xD0, (uint8_t []){0x88}, 1, 0},                           \
        {0xE0, (uint8_t []){0x00, 0x3A, 0x02}, 3, 0},               \
        {0xE1, (uint8_t []){0x04, 0xA0, 0x00, 0xA0, 0x05,0xA0, 0x00, 0xA0, 0x00, 0x40, 0x40}, 11, 0},   \
        {0xE2, (uint8_t []){0x30, 0x00, 0x40, 0x40, 0x32, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00}, 13, 0},  \
        {0xE3, (uint8_t []){0x00, 0x00, 0x33, 0x33}, 4, 0},         \   
        {0xE4, (uint8_t []){0x44, 0x44}, 2, 0},                     \
        {0xE5, (uint8_t []){0x09, 0x2E, 0xA0, 0xA0, 0x0B, 0x30, 0xA0, 0xA0, 0x05, 0x2A, 0xA0, 0xA0, 0x07, 0x2C, 0xA0, 0xA0}, 16, 0},    \
        {0xE6, (uint8_t []){0x00, 0x00, 0x33, 0x33}, 4, 0},         \
        {0xE7, (uint8_t []){0x44, 0x44}, 2, 0},                     \
        {0xE8, (uint8_t []){0x08, 0x2D, 0xA0, 0xA0, 0x0A, 0x2F, 0xA0, 0xA0, 0x04, 0x29, 0xA0, 0xA0, 0x06, 0x2B, 0xA0, 0xA0}, 16, 0},    \
        {0xEB, (uint8_t []){0x00, 0x00, 0x4E, 0x4E, 0x00, 0x00, 0x00}, 7, 0},   \
        {0xEC, (uint8_t []){0x08, 0x01}, 2, 0},                     \
        {0xED, (uint8_t []){0xB0, 0x2B, 0x98, 0xA4, 0x56, 0x7F, 0xFF, 0xFF, 0xFF, 0xFF, 0xF7, 0x65, 0x4A, 0x89, 0xB2, 0x0B}, 16, 0},    \
        {0xEF, (uint8_t []){0x08, 0x08, 0x08, 0x45, 0x3F, 0x54}, 6, 0},     \
        {0xFF, (uint8_t []){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},       \
        {0x11, (uint8_t []){0x00}, 1, 120},                         \
        {0x29, (uint8_t []){0x00}, 1, 20},                          \
    }


/**
 * @brief LCD color configuration
 */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                        // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (0)     // 0: RGB, 1: BGR
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (0)     // 0/1

/**
 * @brief LCD transformation configuration
 */
#define ESP_PANEL_BOARD_LCD_SWAP_XY             (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_X            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_Y            (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_GAP_X               (0)     // [0, ESP_PANEL_BOARD_WIDTH]
#define ESP_PANEL_BOARD_LCD_GAP_Y               (0)     // [0, ESP_PANEL_BOARD_HEIGHT]

/**
 * @brief LCD reset pin configuration
 */
#define ESP_PANEL_BOARD_LCD_RST_IO              (0)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_LCD_RST_LEVEL           (0)     // Reset active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_LCD

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Please update the following macros to configure the touch panel ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Touch panel configuration flag (0/1)
 *
 * Set to `1` to enable touch panel support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_TOUCH               (1)

#if ESP_PANEL_BOARD_USE_TOUCH
/**
 * @brief Touch controller selection
 */
#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        GT911

/**
 * @brief Touch bus type selection
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)

#if (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C) || \
    (ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST        (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief Touch bus parameters configuration
 */
#if ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_I2C

    /**
     * @brief I2C bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_TOUCH_I2C_HOST_ID           (0)     // Typically set to 0
#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_TOUCH_I2C_CLK_HZ            (400 * 1000)
                                                                // Typically set to 400K
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (0)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (0)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (8)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (7)
#endif
    /* For panel */
    #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS           (0)     // Typically set to 0 to use the default address.
                                                                // - For touchs with only one address, set to 0
                                                                // - For touchs with multiple addresses, set to 0 or
                                                                //   the address. Like GT911, there are two addresses:
                                                                //   0x5D(default) and 0x14

#endif // ESP_PANEL_BOARD_TOUCH_BUS_TYPE

/**
 * @brief Touch panel transformation flags
 */
#define ESP_PANEL_BOARD_TOUCH_SWAP_XY           (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_X          (0)     // 0/1
#define ESP_PANEL_BOARD_TOUCH_MIRROR_Y          (0)     // 0/1

/**
 * @brief Touch panel control pins
 */
#define ESP_PANEL_BOARD_TOUCH_RST_IO            (-1)    // Reset pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_RST_LEVEL         (0)     // Reset active level, 0: low, 1: high
#define ESP_PANEL_BOARD_TOUCH_INT_IO            (-1)    // Interrupt pin, -1 if not used
#define ESP_PANEL_BOARD_TOUCH_INT_LEVEL         (0)     // Interrupt active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_USE_TOUCH

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the backlight ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Backlight configuration flag (0/1)
 *
 * Set to `1` to enable backlight support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_BACKLIGHT           (1)

#if ESP_PANEL_BOARD_USE_BACKLIGHT
/**
 * @brief Backlight control type selection
 */
#define ESP_PANEL_BOARD_BACKLIGHT_TYPE          (ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO)

#if (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

    /**
     * @brief Backlight control pin configuration
     */
    #define ESP_PANEL_BOARD_BACKLIGHT_IO        (23)    // Output GPIO pin number
    #define ESP_PANEL_BOARD_BACKLIGHT_ON_LEVEL  (1)     // Active level, 0: low, 1: high

#endif // ESP_PANEL_BOARD_BACKLIGHT_TYPE

/**
 * @brief Backlight idle state configuration (0/1)
 *
 * Set to 1 if want to turn off the backlight after initializing. Otherwise, the backlight will be on.
 */
#define ESP_PANEL_BOARD_BACKLIGHT_IDLE_OFF      (0)

#endif // ESP_PANEL_BOARD_USE_BACKLIGHT

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// Please update the following macros to configure the IO expander //////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief IO expander configuration flag (0/1)
 *
 * Set to `1` to enable IO expander support, `0` to disable
 */
#define ESP_PANEL_BOARD_USE_EXPANDER            (0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////// Please utilize the following macros to execute any additional code if required /////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// File Version ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Do not change the following versions. These version numbers are used to check compatibility between this
 * configuration file and the library. Rules for version numbers:
 * 1. Major version mismatch: Configurations are incompatible, must use library version
 * 2. Minor version mismatch: May be missing new configurations, recommended to update
 * 3. Patch version mismatch: No impact on functionality
 */
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MAJOR 1
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_MINOR 0
#define ESP_PANEL_BOARD_CUSTOM_FILE_VERSION_PATCH 0

#endif // ESP_PANEL_BOARD_DEFAULT_USE_CUSTOM

// *INDENT-ON*
