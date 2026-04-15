## JC8012P4A1C-I-W-Y

[中文](README.md)

### JC8012P4A1 Compilation Notes
The ESP32_Display_Panel library currently does not include the gsl3680 driver, it needs to be added manually.

Copy the contents of the touch folder to the libraries/ESP32_Display_Panel/src/touch/ folder.

Note: The libraries path can be found under File -> Preferences -> Sketchbook location.

### Board setting
![setting](ESP32P4-board-setting.png)