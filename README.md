## 这是一个GUITION公司ESP32产品点亮屏幕的demo

### 如果存在README.md文件，请先阅读该文件！！！
### 如果存在README.md文件，请先阅读该文件！！！
### 如果存在README.md文件，请先阅读该文件！！！

### 产品文件夹内如果存在图片，请按照图片配置进行配置

## 如果缺失某一款产品，请在issue中提交，会尽快上传

### 根据产品型号选择对应文件夹
JC1060P470C存在后缀`_I_W`与`_I_W_Y`是因为存在两种产品，并且两者存在硬件差异，才会专门添加后缀。其他产品不要根据这个后缀来选择JC1060P470C_I_W_Y,比如:JC8012P4A1C-I-W-Y，请选择JC8012P4A1C,而不是JC1060P470C_I_W_Y。

### 请注意使用的版本
#### 1. Arduino IDE
    - Arduino Library Manager：
        lvgl by kisvegbor
            version : 8.4.0
        ESP32_Display_Panel by esp-arduino-libs
            version : 1.0.4
        ESP32_IO_Expander by esp-arduino-libs
            version : 1.1.1
        esp-lib-utils by esp-arduino-libs
            version : 0.3.0
    - Arduino BOARDS Manager：
        ESP32 by Espressif Systems
            version : 3.3.7        
            version : 3.3.8-cn  
        
#### 2. ESP-IDF
    - ESP-IDF version : 5.5.4

### 编译失败解决方法
    1. 首先检查是否和上方版本一致
    2. 优先使用搜索引擎或询问AI尝试解决
    3. 如果上述两种方法都无法解决，可以在issue中提问，提问时请详细描述问题并且说明产品型号，并附上尽可能完整的错误信息

### 其他样例建议前往乐鑫的仓库查看
#### ESP-IDF比较常用的仓库样例
1. ESP-IDF examples : [https://github.com/espressif/esp-idf/tree/master/examples](https://github.com/espressif/esp-idf/tree/master/examples)
2. ESP-Iot-Solution examples ：[https://github.com/espressif/esp-iot-solution/tree/master/examples](https://github.com/espressif/esp-iot-solution/tree/master/examples)
3. ESP-GMF examples : [https://github.com/espressif/esp-gmf/tree/main/gmf_examples](https://github.com/espressif/esp-gmf/tree/main/gmf_examples)

#### Arduino

Arduino样例主要在 `Arduino IDE` -> `File` -> `Examples` -> `Examples for ESP32` 菜单下