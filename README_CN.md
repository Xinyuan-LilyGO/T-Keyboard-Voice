# T-Keyboard Voice V0.3

[English](README.md) | **简体中文**

面向 ESP32-S3 N16R8 的板级驱动、**7 个独立外设 Demo** 和 **1 个 Factory 整机测试固件**。

## 构建入口

| 入口 | 说明 |
| --- | --- |
| PlatformIO | 下方快速开始，8 个独立构建环境 |
| [Arduino IDE / CLI](docs/arduino_ide.md) | Arduino-ESP32 3.3.11，统一库与同一套示例 |
| [ESP-IDF](docs/esp_idf.md) | ESP-IDF 5.5.6 + 官方 Arduino 3.3.11 组件，CMake 选择示例 |

三种入口复用相同驱动、示例和测试音频。ESP-IDF 入口仍依赖 Arduino 组件，不是纯 ESP-IDF 驱动移植。

## 快速开始

以下三种方式任选其一，命令均在**仓库根目录**执行。首次构建需要联网下载工具或固定版本依赖。目标板为 ESP32-S3 N16R8，USB CDC 控制台统一使用 **115200 波特率**，命令以换行结束。

### 1. PlatformIO（推荐）

```bash
pio run -e factory
pio run -e factory -t upload
pio device monitor -b 115200
```

上述命令依次安装工具、编译 Factory、上传固件、打开串口。更换示例只需替换环境名，例如 `pio run -e display -t upload`，无须修改源码或 `src_dir`。上传指定端口使用 `--upload-port COMx`；监视器使用 `-p COMx`。

VS Code 中也可按 **Ctrl+Shift+B** 并选择环境，使用项目原生编译任务。项目状态栏按钮同样绑定这些任务；

### 2. Arduino IDE（不推荐）

1. 安装 Arduino IDE 2.x，在「文件 → 首选项 → 附加开发板管理器网址」中加入：
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. 在“工具 → 开发板 → 开发板管理器”中安装 Espressif 的 **esp32 3.3.11**。
3. 在仓库根目录执行以下命令，准备依赖并生成库安装包：

```bash
python scripts/prepare_dependencies.py
python scripts/package_arduino.py
```

4. 在 IDE 中使用「项目 → 加载库 → 添加 .ZIP 库」，从 `.build/arduino/zips/` 依次安装 **SensorLib.zip、LovyanGFX.zip、ButtonSense.zip、TKeyboardVoice.zip**。避免保留同名旧版库。
5. 打开 `examples/Factory/Factory.ino`；同目录的 `Factory.cpp` 和 `build_opt.h` 必须保留。其他示例打开各自的同名 `.ino`。
6. 选择实际 USB 端口，并设置开发板菜单：

| 菜单 | 设置 |
| --- | --- |
| Board | ESP32S3 Dev Module |
| CPU Frequency | 240 MHz |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |

7. 点击「验证」编译，再点击「上传」烧录；打开串口监视器，设为 **115200**，行结束符选择 LF 或 CRLF。

测试音频已内置，无须额外上传文件系统。完整安装、下载模式和 Arduino CLI 批量构建说明见 [Arduino 使用说明](docs/arduino_ide.md)。

8. 或者使用Arduino CLI 一般下载新版Arduino IDE时自带，然后交给AI agent 说：请根据[Arduino 使用说明](docs/arduino_ide.md)帮我配置项目环境，若中国境内遇到网络问题请打开VPN

### 3. ESP-IDF

安装并进入已导出的 **ESP-IDF 5.5.6** 环境或大于该环境版本；

准备公共依赖并编译 Factory：

```bash
python scripts/prepare_dependencies.py
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory build
```

连接板卡后，指定实际端口编译、烧录并打开监视器：

```bash
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory -p COMx build flash monitor
```

将 `COMx` 换为实际端口，Linux/macOS 使用对应设备路径；监视器用 `Ctrl+]` 退出。更换示例时显式替换 `VOICE_EXAMPLE`，例如 `AudioRecord`，不要误烧录上次选择的其他 Demo。组件管理器会下载官方组件，Flash/PSRAM 默认配置位于 `idf/sdkconfig.defaults`。详见 [ESP-IDF 使用说明](docs/esp_idf.md)。

**注意：**三种入口的分区方案不同，切换时应使用对应工具完整烧录，不要混用 bootloader、分区表和应用镜像。

## 示例列表

| 示例 | PlatformIO 环境名 | 内容 |
| --- | --- | --- |
| [Power](examples/Power/README_CN.md) | `power` | AXP2101、电池、休眠和唤醒 |
| [Display](examples/Display/README_CN.md) | `display` | 五屏显示、背光 |
| [Keys](examples/Keys/README_CN.md) | `keys` | 五键事件 |
| [WS2812](examples/WS2812/README_CN.md) | `ws2812` | 14 颗 RGB 灯 |
| [AudioPlayback](examples/AudioPlayback/README_CN.md) | `audio_playback` | ES8389、扬声器、耳机、PA |
| [AudioRecord](examples/AudioRecord/README_CN.md) | `audio_record` | ES7210、板载/外接麦克风、录音回放 |
| [QWIIC](examples/QWIIC/README_CN.md) | `qwiic` | CN4 I2C、CN3 UART |
| [Factory](examples/Factory/README_CN.md) | `factory` | 全部 18 项整机测试 |

## 文件组织

```text
src/         共用驱动，无应用入口
examples/    每个目录一个可独立运行的示例
boards/      V0.3 ESP32-S3 N16R8 板型定义
idf/         ESP-IDF CMake 入口与 Arduino 组件适配
assets/      原始测试音频
scripts/     示例选择、批量构建和主机测试
tests/       音频解包与电源键回归测试
docs/        引脚、硬件资料、产测和验证记录
```

小 Demo 和 Factory 共用驱动；Factory 不包含各 Demo 的 `.ino`。小 Demo 不初始化无关外设；必要的共享电源轨由 PMIC 模块使能。示例目录中的 `.ino` 是 Arduino 入口；Factory 的主逻辑在同目录 `Factory.cpp` 中。

## 硬件与构建要求

- ESP32-S3-WROOM-1 N16R8：16 MB Flash、8 MB Octal PSRAM，`qio_opi`。
- 使用配置中固定的 Arduino-ESP32 3.x 平台；不要换成仍基于 Arduino 2.x 的平台。
- USB CDC 是控制台；CN3 UART0 独立用于接口演示。
- KEY4 长按休眠仅在 Power 与 Factory 中启用；其他 Demo 不将 KEY4 转换成休眠操作。
- 首次播放请远离耳朵、先确认接线；采集和播放会重新配置同一个 I2S 资源。
- Arduino IDE 的板型、依赖及菜单设置见 [Arduino 使用说明](docs/arduino_ide.md)；ESP-IDF 的版本和分区配置见 [ESP-IDF 使用说明](docs/esp_idf.md)。测试音频已内置为只读 C++ 资源，不需要额外上传文件系统。

## 验证

```bash
python scripts/build_examples.py
python scripts/test_host.py
python scripts/check_repository.py
python scripts/generate_audio.py --check
```

Arduino CLI 与 ESP-IDF 的批量编译分别使用 `scripts/build_arduino.py`、`scripts/build_idf.py`，先按各自文档准备工具链与固定依赖。

主机测试需要原生 C++ 编译器，可通过 `CXX` 指定。批量构建不会上传固件。当前验证结果与尚需实机完成的项目见 [validation.md](docs/validation.md)。

- [引脚表](docs/pinmap.md)
- [供电与休眠](docs/power_and_sleep.md)
- [音频说明](docs/audio.md)
- [Factory 测试指南](docs/factory_test_guide_cn.md)
- [迁移对照](docs/migration_map.md)
- [依赖和资源来源](docs/third_party.md)

## 许可

本仓库保留 GitHub 远程仓库已有的 [MIT License](LICENSE)。第三方依赖以及引用的音频、图片、硬件资料仍须遵守各自的版权、许可证和授权要求，来源说明见 [第三方依赖与资源](docs/third_party.md)。
