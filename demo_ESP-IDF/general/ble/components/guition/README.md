# Guition boards（迁移中）

该目录正在整理为可发布的 `guition_boards` 板库。当前完成独立组件骨架、
触摸公共适配、六个 ESP32/C3 板型及十一个 ESP32-S3 板型；现有 P4 移植已核查，
保留用户已实机验证的 IDF 参数。六类代表编译通过；发布包正在进行隔离验收。
本次迁移没有执行实机验证；ESP32/C3/S3 配置仍待上板验证。

## 组件与版本

- 本地目录：`components/guition`；组件中心名称：`guition/guition_boards`。
- ESP-IDF：6.1.x；固定 `esp_board_manager ==0.7.3~1`、`esp_lcd_touch ==1.2.1`。
- 公共触摸适配不依赖 Arduino、LVGL、本机绝对路径或兄弟组件。
- 组件 CMake 只编译公共源码，不能递归编译全部板型的 `setup_device.c`。
  BMGR 的 `gen_bmgr_codes` 选择并编译当前板型的工厂函数。
- `board_catalog.json` 列出 21 个 Arduino 型号、目标、板型标识、触摸能力和默认值；
  21 个型号对应 21 个 BMGR 板型；JC1060 不带外壳为 `jc1060p470`，
  带外壳的 I_W_Y 为 `jc1060p470_i_w_y`，按用户引脚文档分别配置。
- JC8048F550/JC1060F570C 为已有额外板型，保留原配置；未列入本次 6.1 编译验收。

本地工程可直接放置本目录到 `components/guition`。
发布后，消费工程在 `main/idf_component.yml` 添加板库依赖，BMGR 会扫描名称
包含 `boards` 的下载组件。命名空间需使用实际拥有的组件中心账号，本次不上传。

## Kconfig 与板型迁移接入

公共 `Kconfig` 提供 `GUITION_TOUCH_CAPACITIVE`、`GUITION_TOUCH_RESISTIVE` 和
`GUITION_TOUCH_NONE`。每块板的默认配置和可选类型来自 Arduino 的有效 C/R/N 分支。

`profiles/<board>.Kconfig` 保存板型能力定义。迁移某块板时，将其内容并入该板的
`Kconfig.projbuild`。BMGR 生成代码时会将**当前板型**的配置并入生成组件。
未接入该能力文件的旧板型不会出现新触摸菜单，也不会改变其原有初始化路径。
目标芯片不匹配时适配器关闭。

这样无需在公共 Kconfig 中引用消费工程未定义的其他型号符号。
同一芯片上的板型切换使用 `idf.py bmgr -b <board>` 重新生成；切换芯片时使用
隔离工程/构建目录，不能仅切换触摸选项代替板型生成。

## 已迁移的 ESP32/C3 板型

| board | 屏幕 | 默认触摸 | 可选触摸 |
| --- | --- | --- | --- |
| esp32_2424s012 | GC9A01，240×240 | CST816S | 电容 / 无 |
| esp32_2432s024 | ST7789，240×320 | XPT2046 | 电容 CST820 / 电阻 / 无 |
| esp32_2432s028 | ST7789，240×320 | 无 | 电容 CST820 / 电阻 / 无 |
| esp32_2432s032 | ST7796，240×320 | 无 | 电容 GT911 / 电阻 / 无 |
| esp32_3248s035 | ST7796，320×480 | 无 | 电容 GT911 / 电阻 / 无 |
| jc2432w328 | ST7789，240×320 | 无 | 电容 CST820 / 电阻 / 无 |

这些 ESP32/C3 板型均没有 PSRAM。`GUITION_BOARD_NO_PSRAM` 自动启用；
误启用 `CONFIG_SPIRAM` 时组件 CMake 会终止构建并提示关闭。
示例使用内部 RAM 的 10 行 DMA 缓冲及任务栈，由 esp_lvgl_adapter 等待传输完成。
ST7796 240×320 参数沿用 ESP32-2432S032 的 Arduino 配置，需实机确认。

板型 YAML 的驱动依赖由 BMGR 写入生成组件清单，电容/电阻驱动通过
`$CONFIG{GUITION_TOUCH_*}` 条件按需解析；NONE 不下载对应触摸驱动。
通用 esp_lcd_touch 基础组件仍保留作为公共 API 依赖。

**初始化时按设备获取资源**：使用 `esp_board_manager_init_device_by_name()`
初始化 display_lcd、lcd_touch、lcd_brightness；示例 `display.c` 已提供此路径。
不要在这些多触摸变体板型调用 `esp_board_manager_init()` 批量初始化全部外设。
未选择的总线仅保留配置，不应初始化。S028 的 C/R 变体复用 GPIO32，
原生 BMGR 元数据报告此潜在冲突；互斥选择和按设备初始化避免同时占用。

XPT2046 使用原生驱动建议的 1 MHz（Arduino 宏写 40 MHz）；没有复位脚，
配置为 -1，避免驱动复位动作与 S028 的 SCK GPIO25 冲突。触摸坐标镜像沿用
每个 Arduino 变体；压力阈值 400，开启 ADC 到屏幕坐标转换，校准仍需实机。

## 已迁移的 ESP32-S3 板型

| board | 接口 / 屏幕 | 默认触摸 | 可选触摸 |
| --- | --- | --- | --- |
| esp32_4827s043 | RGB / ST7262，480×272 | XPT2046 | GT911 / XPT2046 / 无 |
| esp32_4848s040 | 三线 SPI+RGB / ST7701，480×480 | GT911 | GT911 / 无 |
| esp32_8048s043 | RGB / ST7262，800×480 | GT911 | GT911 / 无 |
| esp32_8048s050 | RGB / ST7262，800×480 | GT911 | GT911 / 无 |
| esp32_8048s070 | RGB / ST7262，800×480 | GT911 | GT911 / 无 |
| jc3248w535 | QSPI / AXS15231B，320×480 | AXS15231B | AXS15231B / 无 |
| jc3636k518 | QSPI / ST77916，360×360 | CST816S | CST816S / 无 |
| jc3636k718 | QSPI / ST77916，360×360 | CST816S | CST816S / 无 |
| jc3636w518 | QSPI / ST77916，360×360 | CST816S | CST816S / 无 |
| jc4827w543 | QSPI / ST77916，480×272 | GT911 | GT911 / XPT2046 / 无 |
| jc8048w550 | RGB / ST7262，800×480 | GT911 | GT911 / 无 |

S3 默认启用 OPI PSRAM 80 MHz、CPU 240 MHz、QIO Flash 80 MHz。
JC4827W543 为 4 MB Flash，其余板型为 16 MB，依据 Arduino 板卡设置截图。
JC3248W535 和三个 JC3636 原截图使用 Flash 120 MHz，本移植先采用 80 MHz。
不沿用 Arduino 文件系统分区，显示示例使用通用单应用分区。

RGB 采用 RGB565、16 位数据总线、两个 PSRAM 帧缓冲及 10 行 bounce buffer。
消费应用使用 adapter 的 `ESP_LV_ADAPTER_TEAR_AVOID_MODE_DOUBLE_DIRECT` 与之匹配。
ST7701 控制器的 `bits_per_pixel=18` 与 RGB 数据流的 RGB565 分别配置，
保留 33 条原初始化命令。其三线 SPI IO 由 BMGR 持有和释放，驱动不能自动删除。
ESP32-8048S070 的 RGB 数据线及 DE/VSYNC 引脚与 S043/S050 不同，已分别配置。
ST7262 背光使用 GPIO 开关；其余 S3 背光使用 LEDC。

**AXS15231B 应用必须提交整帧**：原生 QSPI 驱动不发送局部地址窗口，
不能直接使用普通 SPI PARTIAL 刷新。仓库 `main/display.c` 使用 adapter
`ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_TE_HIGH_PERFORMANCE_CONFIG`：
原默认旋转 90°、TE GPIO38 下降沿、Tvdl/Tvdh 为 13/3 ms、QSPI 40 MHz/4 线，
采用 ASYNC_FULL staging。必须将 `te_sync.bounce_buffer_enabled=false`、
`bounce_buffer_rows=0`，让每次 panel draw 都写一个完整物理帧；IDF panel IO
内部按 SPI 最大传输长度分段并保持 CS，不在每行重新发送 RAMWR。
该板触摸为 I2C SCL=8/SDA=4，QSPI DC=-1。
AXS 的屏幕和触摸属于同一组件，选择 NONE 仍保留 LCD 驱动依赖。
TE 时序、旋转、颜色、触摸坐标和带宽需实机确认。

S3 固定屏幕驱动版本：ST77916 2.0.2、ST7701 2.0.2~2、AXS15231B 2.1.0。
未逐板构建固件；按接口选定三个代表板，其他板型通过原生代码生成和
真实 S3 工具链的工厂函数语法检查，不能视为各板运行验证。

## 现有 ESP32-P4 配置核查

P4 保留现有 IDF 移植，硬件参数优先于 Arduino 的不同配置。

| board | 实际 LCD / 触摸工厂 | 分辨率 | DPI 时钟 / DSI lane 速率 |
| --- | --- | --- | --- |
| jc1060p470 / jc1060p470_i_w_y | JD9165 / GT911 | 1024×600 | 52 MHz / 750 Mbps |
| jc4880p443 | ST7701 / GT911 | 480×800 | 34 MHz / 550 Mbps |
| jc8012p4a1 | JD9365 / GSL3680 | 800×1280 | V1 60 MHz / V2 70 MHz；1500 Mbps |

均为 DSI 2 lane、LDO3 2.5 V、三个帧缓冲，与示例 MIPI TRIPLE_PARTIAL 模式匹配。
JC1060 两版本共用显示工厂，音频引脚分别配置；LCD V2 暂预留。
IP101 引脚仅保留参考，BMGR 无原生支持，未添加以太网驱动或设备。
JC8012 在 menuconfig 的 `Guition JC8012P4A1 display` 中选择 LCD V1 / V2，
初始化表、DPI 时钟及垂直时序一起切换，默认 V2，无需修改源代码或 YAML。
屏幕选择与板型差异见 [JC8012](jc8012p4a1/readme.md)、
[JC1060](jc1060p470/readme.md)、[带外壳 JC1060](jc1060p470_i_w_y/readme.md)。
原阶段核查结果见下方记录；本次新增配置的编译检查不等同于实机验收。

需要区分以下现状：

- P4 使用原生 BMGR `lcd_touch`，尚未接入本组件新的 C/R/N 触摸菜单。
- JC4880 的 touch chip 元数据写 gls3680，实际工厂调用 GT911；触摸范围为
  800×1280，与屏幕 480×800 不同。本阶段保留已验证参数，需结合触摸坐标确认。
- JC8012 的 GSL3680 依赖 `guition/esp_lcd_touch_gsl3680 ==0.1.0`，不再使用
  兄弟目录。应先发布该驱动，再发布本板库。上传前使用消费工程 main 清单的
  `override_path` 指向解包后的驱动；该覆盖不进入本板库的发布清单。
- JC8012 默认文件的 REV_MIN_100 在 IDF v6.1 中需要
  ESP32P4_SELECTS_REV_LESS_V3 配套。当前默认实际生成最低版本 3.1，
  不能据该默认文件假定固件支持 1.x；旧芯片按原板说明选择配套芯片选项。
- 现有 P4 默认依赖原工程 `partitions.csv`；隔离检查工具会复制该文件。
  独立例子 `examples/board_init` 自带适合最小 4 MB Flash 的分区表，
  应用可根据实际 Flash 和固件大小替换自己的分区表。

详细记录保存在迁移工程 `docs/guition-stage5-verification.md`。

## 触摸公共接口

`include/guition_touch.h` 提供：

- `guition_touch_selected_mode()`：读取编译期选择。
- `guition_touch_create(config, &device)`：获取 BMGR 总线引用、创建 IO、调用板型驱动工厂。
- `guition_touch_get_handle(device)`：取得标准 `esp_lcd_touch_handle_t`，用于 LVGL 输入适配。
- `guition_touch_destroy(device)`：先删除驱动，再删除 IO，最后归还 BMGR 总线引用。

板型以 BMGR `type: custom` 注册初始化/释放回调，将对应 C/R 变体转换成
`guition_touch_config_t`，按选中的模式提供 IO 配置和原生触摸驱动工厂函数。
初始化/释放与 BMGR 一致，从同一任务顺序调用。

BMGR custom 回调使用 `guition_touch_bmgr_create(config, void **handle)` 和
`guition_touch_bmgr_destroy(void *handle)` 桥接，而不是直接调用 create。
BMGR 会丢弃失败 custom 初始化的输出；桥接层会内部保留清理失败的对象，
在下一次初始化前重试清理。若初始化失败后不再重试，应在错误/关闭路径调用
`guition_touch_bmgr_recover()`。该桥接按一块板一个触摸设备设计。

无触摸或未迁移的配置可使用以下最小生命周期，不访问硬件：

```c
guition_touch_device_t *device = NULL;
if (guition_touch_selected_mode() == GUITION_TOUCH_MODE_NONE) {
    ESP_ERROR_CHECK(guition_touch_create(NULL, &device));
    assert(guition_touch_get_handle(device) == NULL);
    ESP_ERROR_CHECK(guition_touch_destroy(device));
}
```

无触摸仍返回一个非空生命周期对象，使 BMGR 的 custom 释放路径能正确查找设备；
其内部没有驱动、IO 或总线引用。已迁移板型包含触摸驱动依赖及工厂函数。

## SPI 复用与失败处理

共享 LCD/触摸的板型只声明一个 SPI peripheral，两个设备使用完全相同的
`bus_name`。适配器调用 `esp_board_periph_ref_handle()`，不会自行
`spi_bus_initialize()` 或 `spi_bus_free()`；专用触摸 SPI/I2C 总线也通过 BMGR 管理。
设备的 CS、频率和 mode 分别来自自己的 panel IO 配置。

共享型号：ESP32-2432S024、ESP32-2432S032、ESP32-3248S035、JC2432W328 的 R 变体。
ESP32-2432S028 的 R 变体使用另一个 host，不能合并；其他拓扑见项目配置核对文档。
无触摸不获取总线引用，因此不会影响仍在使用的 LCD。

驱动工厂返回错误时必须让输出句柄保持 NULL，或返回能够删除的部分初始化句柄。
适配器按驱动、IO、总线的顺序回滚。若释放本身也失败，create 会在返回错误的同时
返回一个非空 device；调用方必须保留它并重试 destroy，不能丢弃后直接再次初始化。
destroy 失败时也保留对象及未释放资源，允许重试。只有 destroy 成功后指针才失效。

`bus_name` 是借用的不可变字符串，必须活到 destroy 成功；板级静态配置满足该要求。
共享 host 的 GPIO 一致性和 CS 冲突检查由板级生成/验证工具负责，不以
“spi_bus_initialize 已初始化”错误代替验证。

## 验证与继续执行

仓库根目录执行：

```bash
python3 tools/guition/generate_touch_kconfig.py --check
PYTHONDONTWRITEBYTECODE=1 /home/peng/.espressif/tools/python/v6.1/venv/bin/python \
  -m unittest discover -s tools/guition/tests -v
```

绝对 Python 路径仅为本机测试命令，可换为已激活 v6.1 的 Python，不进入组件运行依赖。
测试使用 IDF 自带 Kconfig/manifest 解析器及本机下载的 BMGR 扫描器。
C 生命周期测试编译本组件和真实 BMGR 外设引用源码，替换硬件调用；这不等同于
ESP-IDF 固件编译或实机验证。

新公共源码为 MIT；原有文件保留各自 SPDX 许可证，见 `NOTICE` 和 `LICENSES/`。
完整阶段进度在仓库 `docs/esp-idf-v6.1-migration-plan.md`。

## 本仓库的隔离编译

以下工具属于迁移工程，不是组件的运行依赖。仓库根目录执行（激活的 IDF Python
可替代本机绝对路径）：

```bash
/home/peng/.espressif/tools/python/v6.1/venv/bin/python tools/guition/prepare_build.py \
  esp32_2432s028 --touch resistive --destination /tmp/guition-esp32-example
bash tools/guition/build_idf61.sh /tmp/guition-esp32-example --reset-target
```

工具先 source deactivate_idf_v6.1.sh 清理环境，再 source activate_idf_v6.1.sh；
同一 Bash 进程完成后续操作。首次/跨 IDF 路径/跨芯片构建自动清理该隔离工程的
build 和 BMGR 生成文件，执行 `idf.py set-target`，随后重新生成板型并 build。
`--reset-target` 强制该流程。相同版本/目标后续构建保留增量目录。
清理工具只处理有 `guition-build-case.json` 标记的消费工程，并拒绝原 demo。
完整日志保存在 /tmp/guition-idf61-build.*.log；终端使用 esp-idf-build 技能的过滤器。
原 demo 的 sdkconfig/build 和 sdkconfig.defaults 不会被覆盖。

## 组件发布包

`examples/board_init` 是不依赖仓库父目录的最小消费工程。它仅初始化显示相关
设备；完整 LVGL 界面仍见本仓库 demo。发布后通过 main 清单依赖
`guition/guition_boards ==0.1.0`，BMGR 会发现下载组件中的板型。

使用已激活 IDF 的 Python，在本目录执行：

```bash
compote component pack --name guition_boards --dest-dir dist
```

打包不等于上传；必须先发布 GSL3680 依赖。本次不会自动上传或设置账号令牌。
GSL3680 原算法保留 GPL-2.0-or-later；其余原驱动和固件的许可说明待发布者确认，
请查看外部驱动组件的 LICENSE/NOTICE，不将它们误认为本板库的 MIT 文件。
