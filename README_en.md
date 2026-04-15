## This is a demo for lighting up the screen on GUITION company's ESP32 products

[[中文]](README.md)

#### This document was translated by AI
### If a README.md file exists, please read it first!!!
### If a README.md file exists, please read it first!!!
### If a README.md file exists, please read it first!!!

### If there are images in the product folder, please configure according to the images.

## If a specific product model is missing, please submit an issue, and it will be uploaded as soon as possible.

### Select the corresponding folder based on the product model
The suffixes `_I_W` and `_I_W_Y` for `JC1060P470C` exist because there are two different products with hardware differences, hence the specific suffixes. Do not choose `JC1060P470C_I_W_Y` for other products based on this suffix. For example, for `JC8012P4A1C-I-W-Y`, please select `JC8012P4A1C`, not `JC1060P470C_I_W_Y`.

### Please note the versions used
#### 1. Arduino IDE
    - Arduino Library Manager:
        - lvgl by kisvegbor
            - version: 8.4.0
        - ESP32_Display_Panel by esp-arduino-libs
            - version: 1.0.4
        - ESP32_IO_Expander by esp-arduino-libs
            - version: 1.1.1
        - esp-lib-utils by esp-arduino-libs
            - version: 0.3.0
    - Arduino BOARDS Manager:
        - ESP32 by Espressif Systems
            - version: 3.3.7        
            - version: 3.3.8-cn  
        
#### 2. ESP-IDF
    - ESP-IDF version: 5.5.4

### Solutions for Compilation Failures
    1. First, check if your versions match those listed above.
    2. Prioritize using search engines or asking AI for solutions.
    3. If neither of the above methods resolves the issue, you can ask a question in the issues section. When asking, please describe the problem in detail, specify the product model, and attach as complete error information as possible.

### For other examples, it is recommended to visit Espressif's repositories
#### Commonly used ESP-IDF example repositories
1. ESP-IDF examples: [https://github.com/espressif/esp-idf/tree/master/examples](https://github.com/espressif/esp-idf/tree/master/examples)
2. ESP-Iot-Solution examples: [https://github.com/espressif/esp-iot-solution/tree/master/examples](https://github.com/espressif/esp-iot-solution/tree/master/examples)
3. ESP-GMF examples: [https://github.com/espressif/esp-gmf/tree/main/gmf_examples](https://github.com/espressif/esp-gmf/tree/main/gmf_examples)

#### Arduino
Arduino examples are mainly located under `Arduino IDE` -> `File` -> `Examples` -> `Examples for ESP32` menu.