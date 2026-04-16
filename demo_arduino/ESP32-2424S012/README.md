## ESP32-2424S012

[English](README_en.md)

### 选择不同的tp
修改`demo/esp_panel_board_custom_conf.h`

电容触摸：

    #defineESP32_2424S012C     (1)
    #defineESP32_2424S012N     (0)

无触摸：

    #defineESP32_2424S012C     (0)
    #defineESP32_2424S012N     (1)

### 屏幕显示颜色异常
将`libraries/lv_conf.h`文件中`#define LV_COLOR_16_SWAP 0`修改为`#define LV_COLOR_16_SWAP 1`

### Board settings
![settings](ESP32C3-board-setting.png)