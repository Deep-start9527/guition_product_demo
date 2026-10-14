# JC1060P470（不带外壳）

BMGR 板型 `jc1060p470` 对应文档中的 `JC1060P470C_I_W`。
带外壳、带 IP101 的版本请选择 `jc1060p470_i_w_y`。

两版本共用 JD9165 / GT911 屏幕配置与初始化命令；在
`idf.py menuconfig → Guition JC1060P470 display → LCD variant` 选择屏幕。
当前仅 LCD V1 可用；LCD V2 的 Kconfig 符号和实现分支已预留，待补充命令后启用，
不会将未配置的第二种屏幕当成 V1 使用。

| 信号 | 不带外壳 | 带外壳 |
| --- | --- | --- |
| LCD reset / backlight | GPIO27 / GPIO23 | GPIO27 / GPIO23 |
| I2C SDA / SCL | GPIO7 / GPIO8 | GPIO7 / GPIO8 |
| I2S 输入（麦克风） | GPIO11 | GPIO48 |
| 功放使能 | GPIO20 | GPIO11 |
| 电池 ADC | GPIO52 | GPIO53 |
| IP101 Ethernet | 无 | 有 |

触摸 reset / interrupt 未设置，均为 -1。引脚依据用户维护的 `asset/chip/chip-version.md`，
各板目录的 `board_pins.h` 提供该型号的完整参考宏，不自动初始化额外外设。
现有 SDMMC 电源参数保持原配置：文档标注 SD LDO3，与已用 2.5 V 的 MIPI LDO3
存在电压冲突，未将原 SDMMC LDO4 擅自改为 LDO3。原 `uart_1` 的 RX=GPIO27
也与 LCD reset 重叠；显示工作期间不能同时启用这一路 UART。
