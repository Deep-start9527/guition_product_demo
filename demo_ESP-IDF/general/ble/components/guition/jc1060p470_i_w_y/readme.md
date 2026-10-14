# JC1060P470（带外壳）

BMGR 板型 `jc1060p470_i_w_y` 对应 `JC1060P470C_I_W_Y`，与不带外壳版本
使用相同 JD9165 / GT911 屏幕。屏幕选择、GPIO 差异和既有 SD/UART 限制见
[JC1060 说明](../jc1060p470/readme.md)。

运行 `idf.py bmgr -b jc1060p470_i_w_y` 选择板型，然后使用 menuconfig 选择屏幕。
当前仅 LCD V1 可用；第二种屏幕先预留，待补充初始化命令。

带外壳版本具有 IP101；当前 BMGR 0.7.3~1 没有原生 IP101 设备支持，
按用户要求不添加以太网设备、驱动依赖或初始化代码。
`board_pins.h` 仅保留用户文档中已有的以太网引脚，供后续参考：

| 信号 | GPIO |
| --- | --- |
| REF_CLK | 50 |
| TX_EN / TXD0 / TXD1 | 49 / 34 / 35 |
| CRS_DV / RXD0 / RXD1 | 28 / 29 / 30 |
| PHY reset | 51 |
| MDC / MDIO | 31 / 52 |

这些参考宏不会初始化以太网硬件。
