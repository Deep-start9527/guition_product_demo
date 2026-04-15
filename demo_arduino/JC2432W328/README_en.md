## JC2432W328

[中文](README.md)

### Select Different TP (Touch Panel)
Modify `demo/esp_panel_board_custom_conf.h`

Capacitive Touch:

    #define JC2432W328C     (1)
    #define JC2432W328R     (0)
    #define JC2432W328N     (0)

Resistive Touch:

    #define JC2432W328C     (0)
    #define JC2432W328R     (1)
    #define JC2432W328N     (0)

No Touch:

    #define JC2432W328C     (0)
    #define JC2432W328R     (0)
    #define JC2432W328N     (1)

### Abnormal Screen Display Colors
Change `#define LV_COLOR_16_SWAP 0` to `#define LV_COLOR_16_SWAP 1` in the `libraries/lv_conf.h` file.

### Board Settings
![settings](ESP32-board-setting.png)