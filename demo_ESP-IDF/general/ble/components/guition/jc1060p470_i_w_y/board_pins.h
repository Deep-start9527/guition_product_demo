// SPDX-License-Identifier: MIT
#pragma once
#include "driver/gpio.h"

// GPIO reference from asset/chip/chip-version.md; does not initialize hardware.
#define LCD_DSI_LANE_NUM            2
#define LCD_DSI_PHY_LDO_ID          3
#define LCD_RST_PIN                 GPIO_NUM_27
#define LCD_BL_PIN                  GPIO_NUM_23
#define LCD_BL_ON_LEVEL             1
#define TP_C_I2C_HOST               0
#define TP_C_SCL_PIN                GPIO_NUM_8
#define TP_C_SDA_PIN                GPIO_NUM_7
#define TP_C_RST_PIN                GPIO_NUM_NC
#define TP_C_INT_PIN                GPIO_NUM_NC
#define WIFI_SDIO_CLK_PIN           GPIO_NUM_18
#define WIFI_SDIO_CMD_PIN           GPIO_NUM_19
#define WIFI_SDIO_D0_PIN            GPIO_NUM_14
#define WIFI_SDIO_D1_PIN            GPIO_NUM_15
#define WIFI_SDIO_D2_PIN            GPIO_NUM_16
#define WIFI_SDIO_D3_PIN            GPIO_NUM_17
#define WIFI_ESP32C6_RESET_PIN      GPIO_NUM_54         // ESP32-C6芯片复位
#define WIFI_ESP32C6_IO2_PIN        GPIO_NUM_6          // 连接至ESP32-C6 的GPIO2，可用于ESP32-C6进入休眠时唤醒源
#define SDMMC_LDO_ID                3
#define SD_SDMMC_SCK_PIN            GPIO_NUM_43
#define SD_SDMMC_CMD_PIN            GPIO_NUM_44
#define SD_SDMMC_D0_PIN             GPIO_NUM_39
#define SD_SDMMC_D1_PIN             GPIO_NUM_40
#define SD_SDMMC_D2_PIN             GPIO_NUM_41
#define SD_SDMMC_D3_PIN             GPIO_NUM_42
#define CODEC_I2S_MCLK_PIN          GPIO_NUM_13
#define CODEC_I2S_SCLK_PIN          GPIO_NUM_12
#define CODEC_I2S_LRCK_PIN          GPIO_NUM_10
#define CODEC_I2S_DOUT_PIN          GPIO_NUM_48     // 麦克风
#define CODEC_I2S_DIN_PIN           GPIO_NUM_9      // 喇叭
#define AUDIO_PA_CTRL_PIN           GPIO_NUM_11
#define RMII_CLK_PIN                GPIO_NUM_50
#define RMII_TXEN_PIN               GPIO_NUM_49
#define RMII_TXD0_PIN               GPIO_NUM_34
#define RMII_TXD1_PIN               GPIO_NUM_35
#define RMII_RXDV_PIN               GPIO_NUM_28
#define RMII_RXD0_PIN               GPIO_NUM_29
#define RMII_RXD1_PIN               GPIO_NUM_30
#define PHY_RSTN_PIN                GPIO_NUM_51
#define MDC_PIN                     GPIO_NUM_31
#define MDIO_PIN                    GPIO_NUM_52
#define BAT_ADC_PIN                 GPIO_NUM_53
