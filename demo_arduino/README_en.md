## Arduino Library Installation

[中文](README.md)

#### This document was translated by AI

### Install Libraries
    1. Go to the Board Manager and download the ESP32 board support package.
![Local Image](../asset/image/arduino_esp32_boards_install.png)

    2. Go to the Library Manager and download the following libraries:
        - ESP32_Display_Panel
        - ESP32_IO_Expander
        - esp-lib-utils
![Local Image](../asset/image/arduino_ESP32_Display_Panel_install.png)

    3. Go to the Library Manager and download the following library:
        - lvgl
![Local Image](../asset/image/arduino_lvgl_install.png)

### Configure LVGL
1. Copy the [lv_conf.h](file:///home/jczn/guition_product_demo/demo_arduino/lv_conf.h) file to the `Arduino/libraries/` directory.
2. Copy the `Arduino/libraries/lvgl/demos` folder to the `Arduino/libraries/lvgl/src` directory.
- Note: The paths above are for Linux systems. Please adjust them for other operating systems. You can find your libraries path by opening `File` -> `Preferences` and checking the `Sketchbook location`.

### Configure Board
Please configure according to the images provided in the folder corresponding to your product model.

### Screen Drift
RGB screens may experience screen drift. For solutions, please refer to the [FAQ and Answers in ESP32_Display_Panel](https://github.com/esp-arduino-libs/ESP32_Display_Panel/blob/master/docs/envs/use_with_arduino_cn.md#%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98%E5%8F%8A%E8%A7%A3%E7%AD%94).