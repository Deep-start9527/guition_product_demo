# ESP32-C6 ESP-Hosted 从机

本目录保存与 ESP32-P4 主机匹配的 ESP-Hosted `2.12.13` ESP32-C6 从机工程和发布固件。源码与固件严格分开：

- `slave/`：可独立构建的 ESP32-C6 从机工程。依赖源码、锁文件和 `cmd_system` 组件均已放在该目录内，不引用 BLE 工程或本机 ESP-IDF 示例目录。
- `firmware/`：已经构建好的发布固件。`esp32c6-hosted-2.12.13.bin` 是从地址 `0x0` 烧录的完整合并镜像；其余文件用于调试和核对。

工程使用 ESP-IDF v6.1、SDIO 4-bit，并提供 Wi-Fi、BLE Controller 和由 P4 主机发起的协处理器 OTA RPC。

## 构建和打包

先激活 ESP-IDF v6.1，然后执行：

```bash
cd slave
./build_firmware.sh
```

脚本只构建 ESP32-C6。构建完成后，它调用 `idf.py merge-bin`，将 bootloader、分区表、初始 OTA 数据和应用合并为：

```text
firmware/esp32c6-hosted-2.12.13.bin
```

ESP-IDF v6.1 可以放在任意位置，只要当前终端的 `IDF_PATH` 和 `IDF_PYTHON_ENV_PATH` 来自已激活的 v6.1 环境。工程源码不依赖当前仓库中的其他工程。

## 烧录合并镜像

让 C6 进入下载模式，在已安装 esptool 的 Python 或 ESP-IDF 环境中执行：

```bash
cd firmware
./flash_firmware.sh /dev/ttyACM0
```

Windows 可把端口替换为 `COMx`。脚本只要求选择串口，并将 `esp32c6-hosted-2.12.13.bin` 从 `0x0` 写入；用户不需要选择分区或逐个指定文件。升级板载 ESP-Hosted 2.3.x 时应使用这个完整镜像，然后同时重启 P4 和 C6。

合并镜像包含以下内容：

| 偏移 | 内容 | 原始文件 |
| --- | --- | --- |
| `0x0` | bootloader | `firmware/bootloader/bootloader.bin` |
| `0x8000` | 分区表 | `firmware/partition_table/partition-table.bin` |
| `0xd000` | 初始 OTA 数据 | `firmware/ota_data_initial.bin` |
| `0x10000` | ESP-Hosted 应用 | `firmware/network_adapter.bin` |

## 主机 OTA 支持

此从机固件包含 P4 主机发起的 OTA 协议支持：

- `slave/main/slave_control.c` 实现 `OTA_BEGIN`、`OTA_WRITE`、`OTA_END` 和 `OTA_ACTIVATE` RPC。
- `slave/main/slave_config.h` 在 ESP-IDF v6.1 启用 OTA 镜像头有效性检查。
- `slave/partitions.esp32c6.csv` 包含 `otadata` 以及两个 1920 KiB 的 `ota_0`、`ota_1` 应用分区。
- `slave/sdkconfig.esp32c6-hosted` 启用 `CONFIG_ESP_OTA_WORKAROUND=y`。

C6 因而能够接收 P4 端通过 `esp_hosted_slave_ota_begin/write/end/activate` API 发送的固件。P4 应用仍需提供固件来源并调用这些 API；BLE 示例当前没有 OTA 操作界面。

`firmware/SHA256SUMS` 可用于检查发布文件：

```bash
sha256sum -c firmware/SHA256SUMS
```
