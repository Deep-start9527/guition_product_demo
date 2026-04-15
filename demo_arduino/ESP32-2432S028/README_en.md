## ESP32-2432S028

[中文](README.md)

### Select Different TP (Touch Panel)
Modify `demo/esp_panel_board_custom_conf.h`

Capacitive Touch:

    #define ESP32_2432S028C     (1)
    #define ESP32_2432S028R     (0)
    #define ESP32_2432S028N     (0)

Resistive Touch:

    #define ESP32_2432S028C     (0)
    #define ESP32_2432S028R     (1)
    #define ESP32_2432S028N     (0)

No Touch:

    #define ESP32_2432S028C     (0)
    #define ESP32_2432S028R     (0)
    #define ESP32_2432S028N     (1)

### Abnormal Screen Display Colors
Change `#define LV_COLOR_16_SWAP 0` to `#define LV_COLOR_16_SWAP 1` in the `libraries/lv_conf.h` file.

### Board Settings
![settings](ESP32-board-setting.png)