## ESP32-4827S043

[English](README_en.md)

### 选择不同的tp
修改`demo/esp_panel_board_custom_conf.h`

#### 电容触摸
    #define ESP32_4827S043C     (1)
    #define ESP32_4827S043R     (0)
    #define ESP32_4827S043N     (0)

#### 电阻触摸
    #define ESP32_4827S043C     (0)
    #define ESP32_4827S043R     (1)
    #define ESP32_4827S043N     (0)

#### 无触摸
    #define ESP32_4827S043C     (0)
    #define ESP32_4827S043R     (0)
    #define ESP32_4827S043N     (1)

### Board setting
![setting](ESP32S3-board-setting.png)