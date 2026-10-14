# JC8012P4A1

JC8012P4A1C_I_W_Y 支持两种 JD9365 屏幕，默认 LCD V2。

选择板型 `jc8012p4a1` 后，运行 `idf.py menuconfig`，在
`Guition JC8012P4A1 display → LCD variant` 中选择屏幕，无需修改 C 或 YAML 文件。

| 配置 | Kconfig | DPI 时钟 | 垂直后沿 / 同步宽度 / 前沿 |
| --- | --- | --- | --- |
| LCD V1 | `CONFIG_GUITION_JC8012_LCD_V1` | 60 MHz | 8 / 4 / 20 |
| LCD V2（默认） | `CONFIG_GUITION_JC8012_LCD_V2` | 70 MHz | 4 / 10 / 30 |

两者使用各自已有的完整初始化命令表；分辨率均为 800×1280，水平后沿 / 同步宽度 /
前沿均为 20 / 20 / 40，DSI lane 速率保持 1500 Mbps。V1 的 DPI 参数在工厂中按
Kconfig 覆盖，`board_devices.yaml` 保存默认 V2 参数。
保存后重新编译、烧录。若显示异常，先断电并检查日志与所选屏幕是否匹配。

芯片版本低于 3.x 时，仍需在 menuconfig 中设置 `ESP32P4_SELECTS_REV_LESS_V3`
并选择匹配的最低芯片版本；屏幕选项不改变芯片版本选项。
