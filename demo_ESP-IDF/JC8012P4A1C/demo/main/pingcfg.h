#ifndef __PINGCFG_H
#define __PINGCFG_H

/*==========屏幕引脚=========*/

#define LCD_RST_PIN                 GPIO_NUM_27
#define LCD_BACKLIGHT_PIN           GPIO_NUM_23

#define TP_RST_PIN                  GPIO_NUM_22
#define TP_INT_PIN                  GPIO_NUM_21

#define I2C_SDA_PIN                 GPIO_NUM_7
#define I2C_SCL_PIN                 GPIO_NUM_8

/*==========音频引脚=========*/

#define AUDIO_PA_CTRL_PIN             GPIO_NUM_20

#define I2S_MCLK_PIN                 GPIO_NUM_13
#define I2S_SCLK_PIN                 GPIO_NUM_12
#define I2S_LRCK_PIN                 GPIO_NUM_10
#define I2S_DOUT_PIN                 GPIO_NUM_9        //麦克风输入
#define I2S_DIN_PIN                  GPIO_NUM_11         //音频输出

/*==========SD卡引脚=========*/

#define SDMMC_CLK_PIN                GPIO_NUM_43
#define SDMMC_CMD_PIN                GPIO_NUM_44
#define SDMMC_D0_PIN                 GPIO_NUM_39
#define SDMMC_D1_PIN                 GPIO_NUM_40
#define SDMMC_D2_PIN                 GPIO_NUM_41
#define SDMMC_D3_PIN                 GPIO_NUM_42

/*==========网口引脚=========*/

#define RMII_TXD0_PIN                GPIO_NUM_34
#define RMII_TXD1_PIN                GPIO_NUM_35
#define RMII_RXD0_PIN                GPIO_NUM_29
#define RMII_RXD1_PIN                GPIO_NUM_30
#define RMII_RXDV_PIN                GPIO_NUM_28
#define RMII_CLK_PIN                 GPIO_NUM_50
#define RMII_TXEN_PIN                GPIO_NUM_49

#define MDIO_PIN                     GPIO_NUM_52
#define PHY_RST_PIN                  GPIO_NUM_51
#define MDC_PIN                      GPIO_NUM_31

/*==========WIFI引脚=========*/

#define WIFI_ESP32C6_RST_PIN                 GPIO_NUM_54
#define WIFI_ESP32C6_IO2_PIN                 GPIO_NUM_5

#define WIFI_SDIO_CLK_PIN                GPIO_NUM_18
#define WIFI_SDIO_CMD_PIN                GPIO_NUM_19
#define WIFI_SDIO_D0_PIN                 GPIO_NUM_14
#define WIFI_SDIO_D1_PIN                 GPIO_NUM_15
#define WIFI_SDIO_D2_PIN                 GPIO_NUM_16
#define WIFI_SDIO_D3_PIN                 GPIO_NUM_17

/*==========USB引脚=========*/

// #define USB_FULL_P_PIN                 GPIO_NUM_27
// #define USB_FULL_N_PIN                 GPIO_NUM_26

// #define USB_HIGH_P_PIN                 GPIO_NUM_25
// #define USB_HIGH_N_PIN                 GPIO_NUM_24

/*==========UART引脚=========*/

#define UART0_TX_PIN                 GPIO_NUM_37
#define UART0_RX_PIN                 GPIO_NUM_38

#define UART1_TX_PIN                 GPIO_NUM_26
#define UART1_RX_PIN                 GPIO_NUM_27

#define RS485_TX_PIN                 UART1_TX_PIN
#define RS485_RX_PIN                 UART1_RX_PIN

/*==========电池引脚=========*/

#define BAT_VOLTAGE_ADC_PIN              GPIO_NUM_53


#endif