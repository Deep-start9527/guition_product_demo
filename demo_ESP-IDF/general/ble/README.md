# Guition BLE 通信示例

本工程使用 ESP-IDF v6.1、NimBLE 和 LVGL 实现单连接 BLE Peripheral/GATT Server。屏幕显示广播、连接、MTU、通知订阅、安全状态、收发计数和固定容量的文本/HEX 历史；触摸板可主动发送及清除绑定，无触摸配置可接收和回显，或通过 Kconfig 开启定时发送。

06-ble 计划已于 2026-10-12 完成，用户确认硬件测试功能基本完成。计划状态见 [06-ble.md](../../../docs/general-examples/06-ble.md)；本地驱动、C6 从机与主机 OTA 固件及后续会话操作约定见 [本地交接记录](../../../docs/general-examples/local-drivers-c6-handoff.md)。

Guition 板库已复制到工程内的 `components/guition`，相关的 AXS15260、ST7102 显示/触摸驱动及 GSL3680 触摸驱动也已放入本工程的 `components/`。因此在本工程执行 `idf.py bmgr -l` 可以直接列出 Guition 开发板，构建不再依赖 `../../demo/components` 中的这些组件。

## NUS 接口与数据规则

设备名为 `Guition-BLE-XXXX`，后缀取蓝牙 MAC。服务使用 Nordic UART Service UUID：

| 项目 | UUID | 属性 |
| --- | --- | --- |
| Service | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | Primary service |
| RX | `6E400002-B5A3-F393-E0A9-E50E24DCCA9E` | Write、Write Without Response |
| TX | `6E400003-B5A3-F393-E0A9-E50E24DCCA9E` | Notify |

单次 RX Write 最大为 `min(MTU-3, 512)` 字节。通知按 `MTU-3` 分段，NUS 被定义为字节流，通知边界不表示应用消息边界。数据队列深度为 8；队列满时有响应写返回 ATT 资源错误，无响应写只能计入丢弃统计。通知没有订阅、安全条件不满足、连接已更换或两秒内持续缺少协议栈资源时，本次发送失败且不会留到下一连接。

默认是开放模式。固定密码模式使用六位 Passkey `123456`，要求加密、MITM、绑定和 16 字节密钥；密码可在 `menuconfig > Guition BLE example` 中修改。手机必须订阅 TX 后才会收到回显。

触摸屏主页的 Send 始终可以打开输入页；提交时若尚未连接、未订阅 TX 通知或安全配对条件不足，页面会提示所需操作。接收文本并不代表手机已订阅 TX；请在手机或 PC 上启用 TX 的 Notify 后再主动发送。Peer、MTU/Notify 和安全信息在卡片内纵向排列。

发送页使用 LVGL 内置 `lv_keyboard`，直接绑定 `lv_textarea`，使用默认字母、大小写及数字/符号布局。键盘居中，最大宽度 640 像素，高度按宽度的 40% 计算并限制在 160–256 像素，不填满剩余屏幕；800×1280 屏幕为 640×256，480×800 屏幕约为 456×182。确认键和单行输入的回车键执行与 Send 按钮相同的发送校验；键盘关闭键返回主页。按键弹出提示使用 LVGL 内置 popover。

HEX 前缀默认开启，按每个字节显示，例如 `0x48 0x65 0x6C 0x6C 0x6F`。触摸屏主页的 `HEX 0x prefix` 复选框可以立即切换历史预览；关闭后显示 `48 65 6C 6C 6F`。在 `menuconfig > Guition BLE example > Show 0x before every HEX preview byte by default` 中可设置开机默认值，无触摸型号使用该配置。运行时开关不写入 NVS，重启后恢复配置默认值。屏幕保留 8 KiB 固定格式化缓冲；记录较长时显示最新的完整记录，并提示当前显示条数。

## 构建

脚本会选择 ESP-IDF v6.1、生成工程自己的 Board Manager 配置、保存完整日志到 `docs/logs/`，并为各用例隔离 sdkconfig 与构建目录。一次只运行一条构建命令。

查看包含 Guition 在内的板型列表：

```bash
idf.py bmgr -l
```

```bash
cd demo_ESP-IDF/general/ble

# 每种芯片只选一个代表型号
bash tools/build_case.sh esp32_2432s024 esp32 open
bash tools/build_case.sh esp32_2424s012 esp32c3 open
bash tools/build_case.sh jc3248w535 esp32s3 open
bash tools/build_case.sh jc8012p4a1 esp32p4 open

# 加密模式或关闭触摸，仍一次执行一条
bash tools/build_case.sh esp32_2432s024 esp32 secure
bash tools/build_case.sh esp32_2432s024 esp32 open touch-off
```

P4 代表板配置现在生成可运行的 Hosted BLE 固件，不再自动追加 `configs/p4-compile-only.defaults`。该文件仅保留给没有 C6 或尚未接线时的显式编译检查。

独立的 ESP32-C6 协处理器工程位于相邻的 `../ESP32-C6-SLAVE/slave`，不依赖本 BLE 工程。构建锁定固件：

```bash
bash tools/build_c6_firmware.sh
```

脚本转入独立工程构建，并使用 `idf.py merge-bin` 输出 `../ESP32-C6-SLAVE/firmware/esp32c6-hosted-2.12.13.bin`。该单文件从 `0x0` 烧录，已包含 bootloader、分区表、OTA 初始数据和应用，用户不需要选择分区。固件包含 Wi-Fi、供主机使用的 BLE controller 和主机 OTA RPC；分区表包含两个 1920 KiB OTA 应用分区。Network Split 和主机休眠保持关闭。

P4 主机与 C6 协处理器必须使用同代 ESP-Hosted 协议。若串口出现 `Host [2.12.x] > Co-proc [2.3.x]`，进入 `../ESP32-C6-SLAVE/firmware` 执行 `./flash_firmware.sh PORT` 完整烧录合并镜像；跨版本只更新应用可能保留不匹配的分区或 OTA 状态。

## ESP32-P4 + ESP32-C6 默认引脚

本工程所有 ESP32-P4 配置均使用 ESP-Hosted 连接 ESP32-C6 时的默认引脚：

| 端 | CMD | CLK | D0 | D1 | D2 | D3 | RESET/EN | 总线 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |
| P4 Host 当前解析值 | GPIO19 | GPIO18 | GPIO14 | GPIO15 | GPIO16 | GPIO17 | GPIO54，配置为 active high | SDIO 4-bit，40 MHz |
| C6 Coprocessor 当前解析值 | GPIO18 | GPIO19 | GPIO20 | GPIO21 | GPIO22 | GPIO23 | 由外部 EN/RST 接线决定 | SDIO 4-bit，high speed |

构建不再要求额外的接线确认选项。正常 P4 构建默认关闭 `CONFIG_EXAMPLE_COMPILE_ONLY`，直接启用 Hosted 运行路径；只有显式使用 `configs/p4-compile-only.defaults` 时才跳过运行时连接。

锁定的 `esp_hosted 2.12.13` 通过组件构造函数在 `app_main` 前初始化 Hosted；本工程现在允许这一启动顺序。若 C6 未连接或固件不匹配，传输层最多可能等待约 20 秒，因此串口中的 Hosted 日志应出现在 `main_task: Calling app_main()` 之前。应用连接 C6 后通过 `esp_hosted_iface_mac_addr_get()` 获取远端蓝牙 MAC，再初始化和启用 C6 蓝牙控制器。

## Python BLE 上位机

桌面上位机位于 `tools/ble_host.py`，提供：

- 按名称或地址过滤并扫描 BLE 设备，显示地址与 RSSI。
- 连接 Guition NUS、可选请求系统配对、自动订阅 TX 通知。
- 文本 UTF-8 与 HEX 输入，最大 512 字节，按协商的 `MTU-3` 自动分包。
- 同时显示收发数据的文本、HEX、时间、分包数量和累计计数；“HEX 每字节显示 0x”复选框可立即切换已有及后续记录的前缀，默认开启。
- “回显自检”发送 32 字节随机数据，并在 6 秒内逐字节核对通知流。
- BLE 扫描失败时提示检查电脑 BLE 能力、蓝牙开关和系统权限。窗口通过后台 asyncio 线程通信，不会因扫描阻塞。

Bleak 被锁定为 `3.0.2`。依赖只安装到工程根目录的 `.venv-ble-host`；该目录已经写入 `.gitignore`，不会进入 Git，也不会修改系统 Python 或 ESP-IDF Python 环境。Bleak 官方支持 Windows 11、BlueZ 5.55 及以上的 Linux，以及 macOS 10.15 及以上，详见 [Bleak 项目页](https://pypi.org/project/bleak/)。

### 自动创建虚拟环境

在 `demo_ESP-IDF/general/ble` 目录执行：

```bash
python3 tools/setup_ble_host.py
python3 tools/run_ble_host.py --check
python3 tools/run_ble_host.py
```

Windows PowerShell 中可使用 `py -3`：

```powershell
py -3 tools/setup_ble_host.py
py -3 tools/run_ble_host.py --check
py -3 tools/run_ble_host.py
```

`setup_ble_host.py` 使用 Python 标准库 `venv` 创建或更新 `.venv-ble-host`，再在该环境中执行 `pip install -r tools/requirements-ble-host.txt`。`run_ble_host.py` 会自动选择这个环境中的 Python，因此平时无需手动激活。

Tkinter 随多数 Windows/macOS Python 安装提供。在精简 Linux 发行版中如果启动时报 `No module named tkinter`，需要先用系统包管理器安装与当前 Python 对应的 Tk 支持（例如 Debian/Ubuntu 的 `python3-tk`）；它属于系统 GUI 绑定，不通过项目虚拟环境安装。

### 手动创建、激活和安装

Linux/macOS：

```bash
python3 -m venv .venv-ble-host
source .venv-ble-host/bin/activate
python -m pip install -r tools/requirements-ble-host.txt
python tools/ble_host.py --check
python tools/ble_host.py
deactivate
```

Windows PowerShell：

```powershell
py -3 -m venv .venv-ble-host
.\.venv-ble-host\Scripts\Activate.ps1
python -m pip install -r tools\requirements-ble-host.txt
python tools\ble_host.py --check
python tools\ble_host.py
deactivate
```

以后需要额外 Python 包时，先激活 `.venv-ble-host`，确认命令行开头出现虚拟环境名称，再运行 `python -m pip install 包名`。需要纳入工程复现的包还应写入 `tools/requirements-ble-host.txt` 并锁定版本。

### 使用步骤

1. 烧录可运行的 BLE 固件并确认设备正在广播。P4 需同时烧录匹配的 C6 Hosted 固件。
2. 打开电脑蓝牙，启动上位机。默认过滤 `Guition-BLE-`，按 F5 或点击“扫描”。
3. 选择设备并连接；固定密码固件勾选“连接时请求配对”，在操作系统提示中输入屏幕密码 `123456`。
4. 选择文本或 HEX，输入数据后发送。通信记录应显示相同的 TX 字节流和 RX 回显。
5. 点击“回显自检”完成自动核对。自检期间不要从其他 Central 连接同一设备。

如果没有发现设备，可清空名称过滤后重新扫描，并确认设备没有被手机占用。Linux 需要 BlueZ 服务运行且当前账户具有蓝牙访问权限；固定密码配对还需要系统蓝牙代理能够显示输入框。

## 手机和命令行 PC 验证

手机使用 nRF Connect 等通用 BLE 工具：扫描 `Guition-BLE-XXXX`，连接，找到 NUS，订阅 TX，然后向 RX 写入数据。开放模式无需配对；加密模式输入屏幕显示的六位密码。清除绑定后，还要在手机系统中删除旧配对记录再连接。

命令行工具使用同一个虚拟环境中的 `bleak`，按已知总长度把多条通知重组为一个字节流并逐字节比较：

```bash
.venv-ble-host/bin/python tests/nus_client.py --name Guition-BLE- --text 'hello'
.venv-ble-host/bin/python tests/nus_client.py --address AA:BB:CC:DD:EE:FF --hex 00010203ff
```

Windows 将上面解释器路径替换为 `.venv-ble-host\Scripts\python.exe`。

无触摸配置清除绑定需要擦除 NVS。`idf.py erase-flash` 会清除芯片上的全部 Flash 内容，确认端口与目标板后再手动执行并重新烧录。

## 内存与固定容量

应用数据队列的静态项大小为 524 字节，8 项约 4.1 KiB；控制队列不足 0.1 KiB；服务任务栈 6 KiB；NimBLE Host 栈配置为 4 KiB。服务历史为 32×76 字节，服务快照与 UI 副本合计约 4.8 KiB，UI 格式化缓冲为 8 KiB，LVGL 堆为 64 KiB。无 PSRAM SPI 显示由适配器再分配约 10 行的 DMA 绘制缓冲，具体大小随分辨率变化。

安全模式链接结果显示：ESP32-2432S024 静态 DRAM 使用 108,043/124,580 字节，ESP32-2424S012 使用 186,641/321,296 字节。最小剩余堆、任务栈余量和持续运行稳定性仍需实机测量，编译结果不能替代这些运行指标。

依赖版本为 ESP-IDF `v6.1-dirty`（提交 `fff9895c82d744c7237be8847347bdd1b07c6643`）、Board Manager `0.7.3~1`、LVGL `9.6.0`、LVGL Adapter `0.7.1`、ESP-Hosted `2.12.13` 和 ESP Wi-Fi Remote `1.6.5`。来源与固件哈希见 [docs/source-hashes.sha256](docs/source-hashes.sha256)，软件/硬件验收状态见 [docs/verification.md](docs/verification.md)。
