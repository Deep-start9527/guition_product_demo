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

#define JC4827W543C     (1)
#define JC4827W543R     (0)
#define JC4827W543N     (0)

/**
 * @brief Board name
 */

#if JC4827W543C
#define ESP_PANEL_BOARD_NAME                "custom:JC4827W543C"
#elif JC4827W543R
#define ESP_PANEL_BOARD_NAME                "custom:JC4827W543R"
#elif JC4827W543N
#define ESP_PANEL_BOARD_NAME                "custom:JC4827W543N"
#endif




/**
 * @brief Panel resolution configuration in pixels
 */
#define ESP_PANEL_BOARD_WIDTH               (480)   // Panel width (horizontal, in pixels)
#define ESP_PANEL_BOARD_HEIGHT              (272)   // Panel height (vertical, in pixels)

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
#define ESP_PANEL_BOARD_LCD_CONTROLLER      ST77916

/**
 * @brief LCD bus type selection
 */
#define ESP_PANEL_BOARD_LCD_BUS_TYPE        (ESP_PANEL_BUS_TYPE_QSPI)

#if (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI) || \
    (ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI)
/**
 * If set to 1, the bus will skip to initialize the corresponding host. Users need to initialize the host in advance.
 *
 * For drivers which created by this library, even if they use the same host, the host will be initialized only once.
 * So it is not necessary to set the macro to `1`. For other drivers (like `Wire`), please set the macro to `1`
 * ensure that the host is initialized only once.
 */
#define ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST      (0)     // 0/1. Typically set to 0
#endif

/**
 * @brief LCD bus parameters configuration
 *
 * Configure parameters based on the selected bus type. Parameters for other bus types will be ignored.
 * For detailed parameter explanations, see:
 * https://docs.espressif.com/projects/esp-idf/en/v5.3.1/esp32s3/api-reference/peripherals/lcd/index.html
 * https://docs.espressif.com/projects/esp-iot-solution/en/latest/display/lcd/index.html
 */
#if ESP_PANEL_BOARD_LCD_BUS_TYPE == ESP_PANEL_BUS_TYPE_QSPI

    /**
     * @brief QSPI bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_LCD_QSPI_HOST_ID        (1)     // Typically set to 1
#if !ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_SCK         (47)
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA0       (21)
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA1       (48)
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA2       (40)
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_DATA3       (39)
#endif // ESP_PANEL_BOARD_LCD_BUS_SKIP_INIT_HOST
    /* For panel */
    #define ESP_PANEL_BOARD_LCD_QSPI_IO_CS          (45)    // -1 if not used
    #define ESP_PANEL_BOARD_LCD_QSPI_MODE           (0)     // 0-3, typically set to 0
    #define ESP_PANEL_BOARD_LCD_QSPI_CLK_HZ         (32 * 1000 * 1000)
                                                            // Should be an integer divisor of 80M, typically set to 40M
    #define ESP_PANEL_BOARD_LCD_QSPI_CMD_BITS       (32)    // Typically set to 32
    #define ESP_PANEL_BOARD_LCD_QSPI_PARAM_BITS     (8)     // Typically set to 8

#endif // ESP_PANEL_BOARD_LCD_BUS_TYPE
#define ESP_PANEL_BOARD_LCD_VENDOR_INIT_CMD() \
{       \
    {0xff, (uint8_t []){0xa5}, 1, 0},   \
    {0xE7, (uint8_t []){0x10}, 1, 0},   \
    {0x35, (uint8_t []){0x00}, 1, 0},   \
    {0x36, (uint8_t []){0xc0}, 1, 0},   \
    {0x3A, (uint8_t []){0x01}, 1, 0},   \
    {0x40, (uint8_t []){0x01}, 1, 0},   \
    {0x41, (uint8_t []){0x03}, 1, 0},   \
    {0x44, (uint8_t []){0x15}, 1, 0},   \
    {0x45, (uint8_t []){0x15}, 1, 0},   \
    {0x7d, (uint8_t []){0x03}, 1, 0},   \
    {0xc1, (uint8_t []){0xbb}, 1, 0},   \
    {0xc2, (uint8_t []){0x05}, 1, 0},   \
    {0xc3, (uint8_t []){0x10}, 1, 0},   \
    {0xc6, (uint8_t []){0x3e}, 1, 0},   \
    {0xc7, (uint8_t []){0x25}, 1, 0},   \
    {0xc8, (uint8_t []){0x11}, 1, 0},   \
    {0x7a, (uint8_t []){0x5f}, 1, 0},   \
    {0x6f, (uint8_t []){0x44}, 1, 0},   \
    {0x78, (uint8_t []){0x70}, 1, 0},   \
    {0xc9, (uint8_t []){0x00}, 1, 0},   \
    {0x67, (uint8_t []){0x21}, 1, 0},   \
    {0x51, (uint8_t []){0x0a}, 1, 0},   \
    {0x52, (uint8_t []){0x76}, 1, 0},   \
    {0x53, (uint8_t []){0x0a}, 1, 0},   \
    {0x54, (uint8_t []){0x76}, 1, 0},   \
    {0x46, (uint8_t []){0x0a}, 1, 0},   \
    {0x47, (uint8_t []){0x2a}, 1, 0},   \
    {0x48, (uint8_t []){0x0a}, 1, 0},   \
    {0x49, (uint8_t []){0x1a}, 1, 0},   \
    {0x56, (uint8_t []){0x43}, 1, 0},   \
    {0x57, (uint8_t []){0x42}, 1, 0},   \
    {0x58, (uint8_t []){0x3c}, 1, 0},   \
    {0x59, (uint8_t []){0x64}, 1, 0},   \
    {0x5a, (uint8_t []){0x41}, 1, 0},   \
    {0x5b, (uint8_t []){0x3c}, 1, 0},   \
    {0x5c, (uint8_t []){0x02}, 1, 0},   \
    {0x5d, (uint8_t []){0x3c}, 1, 0},   \
    {0x5e, (uint8_t []){0x1f}, 1, 0},   \
    {0x60, (uint8_t []){0x80}, 1, 0},   \
    {0x61, (uint8_t []){0x3f}, 1, 0},   \
    {0x62, (uint8_t []){0x21}, 1, 0},   \
    {0x63, (uint8_t []){0x07}, 1, 0},   \
    {0x64, (uint8_t []){0xe0}, 1, 0},   \
    {0x65, (uint8_t []){0x02}, 1, 0},   \
    {0xca, (uint8_t []){0x20}, 1, 0},   \
    {0xcb, (uint8_t []){0x52}, 1, 0},   \
    {0xcc, (uint8_t []){0x10}, 1, 0},   \
    {0xcD, (uint8_t []){0x42}, 1, 0},   \
    {0xD0, (uint8_t []){0x20}, 1, 0},   \
    {0xD1, (uint8_t []){0x52}, 1, 0},   \
    {0xD2, (uint8_t []){0x10}, 1, 0},   \
    {0xD3, (uint8_t []){0x42}, 1, 0},   \
    {0xD4, (uint8_t []){0x0a}, 1, 0},   \
    {0xD5, (uint8_t []){0x32}, 1, 0},   \
    {0xf8, (uint8_t []){0x03}, 1, 0},   \
    {0xf9, (uint8_t []){0x20}, 1, 0},   \
    {0x80, (uint8_t []){0x00}, 1, 0},   \
    {0xA0, (uint8_t []){0x00}, 1, 0},   \
    {0x81, (uint8_t []){0x07}, 1, 0},   \
    {0xA1, (uint8_t []){0x06}, 1, 0},   \
    {0x82, (uint8_t []){0x02}, 1, 0},   \
    {0xA2, (uint8_t []){0x01}, 1, 0},   \
    {0x86, (uint8_t []){0x11}, 1, 0},   \
    {0xA6, (uint8_t []){0x10}, 1, 0},   \
    {0x87, (uint8_t []){0x27}, 1, 0},   \
    {0xA7, (uint8_t []){0x27}, 1, 0},   \
    {0x83, (uint8_t []){0x37}, 1, 0},   \
    {0xA3, (uint8_t []){0x37}, 1, 0},   \
    {0x84, (uint8_t []){0x35}, 1, 0},   \
    {0xA4, (uint8_t []){0x35}, 1, 0},   \
    {0x85, (uint8_t []){0x3f}, 1, 0},   \
    {0xA5, (uint8_t []){0x3f}, 1, 0},   \
    {0x88, (uint8_t []){0x0b}, 1, 0},   \
    {0xA8, (uint8_t []){0x0b}, 1, 0},   \
    {0x89, (uint8_t []){0x14}, 1, 0},   \
    {0xA9, (uint8_t []){0x14}, 1, 0},   \
    {0x8a, (uint8_t []){0x1a}, 1, 0},   \
    {0xAa, (uint8_t []){0x1a}, 1, 0},   \
    {0x8b, (uint8_t []){0x0a}, 1, 0},   \
    {0xAb, (uint8_t []){0x0a}, 1, 0},   \
    {0x8c, (uint8_t []){0x14}, 1, 0},   \
    {0xAc, (uint8_t []){0x08}, 1, 0},   \
    {0x8d, (uint8_t []){0x17}, 1, 0},   \
    {0xAd, (uint8_t []){0x07}, 1, 0},   \
    {0x8e, (uint8_t []){0x16}, 1, 0},   \
    {0xAe, (uint8_t []){0x06}, 1, 0},   \
    {0x8f, (uint8_t []){0x1B}, 1, 0},   \
    {0xAf, (uint8_t []){0x07}, 1, 0},   \
    {0x90, (uint8_t []){0x04}, 1, 0},   \
    {0xB0, (uint8_t []){0x04}, 1, 0},   \
    {0x91, (uint8_t []){0x0A}, 1, 0},   \
    {0xB1, (uint8_t []){0x0A}, 1, 0},   \
    {0x92, (uint8_t []){0x16}, 1, 0},   \
    {0xB2, (uint8_t []){0x15}, 1, 0},   \
    {0xff, (uint8_t []){0x00}, 1, 0},   \
    {0x11, (uint8_t []){0x00}, 1, 700}, \
    {0x29, (uint8_t []){0x00}, 1, 100}, \
}

/**
 * @brief LCD color configuration
 */
#define ESP_PANEL_BOARD_LCD_COLOR_BITS          (ESP_PANEL_LCD_COLOR_BITS_RGB565)
                                                        // ESP_PANEL_LCD_COLOR_BITS_RGB565/RGB666/RGB888
#define ESP_PANEL_BOARD_LCD_COLOR_BGR_ORDER     (1)     // 0: RGB, 1: BGR
#define ESP_PANEL_BOARD_LCD_COLOR_INEVRT_BIT    (0)     // 0/1

/**
 * @brief LCD transformation configuration
 */
#define ESP_PANEL_BOARD_LCD_SWAP_XY             (0)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_X            (1)     // 0/1
#define ESP_PANEL_BOARD_LCD_MIRROR_Y            (1)     // 0/1
#define ESP_PANEL_BOARD_LCD_GAP_X               (0)     // [0, ESP_PANEL_BOARD_WIDTH]
#define ESP_PANEL_BOARD_LCD_GAP_Y               (0)     // [0, ESP_PANEL_BOARD_HEIGHT]

/**
 * @brief LCD reset pin configuration
 */
#define ESP_PANEL_BOARD_LCD_RST_IO              (-1)    // Reset pin, -1 if not used
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
#if JC4827W543C || JC4827W543R
#define ESP_PANEL_BOARD_USE_TOUCH               (1)
#elif JC4827W543N
#define ESP_PANEL_BOARD_USE_TOUCH               (0)
#endif


#if ESP_PANEL_BOARD_USE_TOUCH
#if JC4827W543C
/**
 * @brief Touch controller selection
 */

#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        GT911
/**
 * @brief Touch bus type selection
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_I2C)

#elif JC4827W543R
/**
 * @brief Touch controller selection
 */

#define ESP_PANEL_BOARD_TOUCH_CONTROLLER        XPT2046
/**
 * @brief Touch bus type selection
 */
#define ESP_PANEL_BOARD_TOUCH_BUS_TYPE          (ESP_PANEL_BUS_TYPE_SPI)

#endif



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
    #define ESP_PANEL_BOARD_TOUCH_I2C_SCL_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_SDA_PULLUP        (1)     // 0/1. Typically set to 1
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SCL            (4)
    #define ESP_PANEL_BOARD_TOUCH_I2C_IO_SDA            (8)
#endif
    /* For panel */
    #define ESP_PANEL_BOARD_TOUCH_I2C_ADDRESS           (0)     // Typically set to 0 to use the default address.
                                                                // - For touchs with only one address, set to 0
                                                                // - For touchs with multiple addresses, set to 0 or
                                                                //   the address. Like GT911, there are two addresses:
                                                                //   0x5D(default) and 0x14
#elif ESP_PANEL_BOARD_TOUCH_BUS_TYPE == ESP_PANEL_BUS_TYPE_SPI
    /**
     * @brief SPI bus
     */
    /* For general */
    #define ESP_PANEL_BOARD_TOUCH_SPI_HOST_ID           (2)     // Typically set to 1
#if !ESP_PANEL_BOARD_TOUCH_BUS_SKIP_INIT_HOST
    /* For host */
    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_SCK            (12)
    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_MOSI           (11)
    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_MISO           (13)
#endif
    /* For panel */
    #define ESP_PANEL_BOARD_TOUCH_SPI_IO_CS             (38)
    #define ESP_PANEL_BOARD_TOUCH_SPI_CLK_HZ            (40 * 1000 * 1000)  // Should be integer divisor of 80M

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
#define ESP_PANEL_BOARD_BACKLIGHT_TYPE          (ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

#if (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_GPIO) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER) || \
    (ESP_PANEL_BOARD_BACKLIGHT_TYPE == ESP_PANEL_BACKLIGHT_TYPE_PWM_LEDC)

    /**
     * @brief Backlight control pin configuration
     */
    #define ESP_PANEL_BOARD_BACKLIGHT_IO        (1)    // Output GPIO pin number
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

#endif

// *INDENT-ON*
