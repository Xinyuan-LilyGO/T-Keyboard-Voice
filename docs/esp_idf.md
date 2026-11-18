# ESP-IDF 支持

## 架构与版本

这是**真正通过 `idf.py`、CMake 和 ESP-IDF 工具链构建的工程**，但驱动复用 **Arduino-ESP32 组件**，不是不依赖 Arduino 的纯 ESP-IDF 驱动移植。`Wire`、`ESP_I2S`、ButtonSense 等现有接口继续使用，不维护另一份功能实现。

- ESP-IDF：**5.5.6**，工程限制为 5.5.x。
- Arduino-ESP32 组件：**3.3.11**，由官方 Component Manager 自动获取。
- 其余三项库与 PlatformIO 使用同一版本源码，由 `scripts/prepare_dependencies.py` 固定提交下载。
- 目标：ESP32-S3 N16R8，16 MB Flash、8 MB OPI PSRAM。

请安装并导出上述 ESP-IDF 环境；当前终端如果默认指向 6.x，先切换至 5.5.x，不要通过跳过版本检查强行构建。

## 构建 Factory

在已导出 ESP-IDF 5.5.x 的终端中，从仓库根目录执行：

```bash
python scripts/prepare_dependencies.py
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory build
```

也可在终端进入 `idf/` 后执行：

```bash
idf.py -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory build
```

首次运行需要联网下载组件。不要把 Arduino IDE 或 PlatformIO 的预编译 SDK 当作 ESP-IDF 安装目录。

## 选择示例

`VOICE_EXAMPLE` 可取：`Power`、`Display`、`Keys`、`WS2812`、`AudioPlayback`、`AudioRecord`、`QWIIC`、`Factory`。

```bash
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=AudioRecord build
```

CMake 直接编译当前 `examples/` 的 `.ino` 或 `Factory.cpp`，只选择该示例需要的公共驱动；不会复制源码或包含其他 Demo 的应用入口。切换示例时必须显式传入 `VOICE_EXAMPLE`，CMake 会记住上一次选择。

## 烧录与监视

```bash
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory -p COMx build flash monitor
```

Linux/macOS 将 `COMx` 换为实际设备路径。监视器退出使用 `Ctrl+]`。本命令会重新确认 Factory 的选择，避免误烧录上一次构建的其他 Demo。

## 批量构建

```bash
python scripts/build_idf.py
python scripts/build_idf.py Factory AudioRecord
```

脚本使用当前 `IDF_PATH` 与 `IDF_PYTHON_ENV_PATH`，不下载或修改系统 SDK，不烧录。在 Windows 上调用原生 Python 和工具链。建议使用 ESP-IDF 原生终端；不依赖 Git Bash 来激活 SDK。

为了复用 SDK 编译缓存，批量脚本共用 `.build/idf/work/`。每个成功的示例将 ELF、应用镜像、bootloader、分区表和烧录参数复制到 `.build/idf/<示例名>/`；日志在 `.build/logs/idf/`。共用工作目录最终保留的是**最后一个示例**，不要直接对它运行 `flash` 并假定是 Factory。

## 文件和配置

```text
idf/
├── CMakeLists.txt               # 版本、示例和依赖入口
├── sdkconfig.defaults           # N16R8、PSRAM、1000 Hz tick、Arduino 自动启动
├── partitions.csv               # 16 MB Flash，6 MB 单应用分区
├── main/
│   ├── CMakeLists.txt           # 选择原示例与公共模块
│   ├── Kconfig.projbuild        # 补齐 Arduino 3.3.11 的 ESP_I2S 选择开关
│   └── idf_component.yml        # 固定官方 Arduino 组件
└── components/                 # 三项库的薄 CMake 适配层，不复制驱动
```

- Arduino 核心自动调用原示例的 `setup()` / `loop()`；不再额外定义另一份 `app_main()`。
- Arduino 3.3.11 的 CMake 支持选择 `ESP_I2S`，但其 Kconfig 尚缺对应声明，本工程通过 `main/Kconfig.projbuild` 补齐，不改写下载的核心源码。只开启 Wire、SPI 和 ESP_I2S，其他 Arduino 库显式关闭；Component Manager 仍可能下载官方清单中的传递依赖。
- USB CDC 与板载 UART0 的用途保持不变。
- sdkconfig 生成后，修改 `sdkconfig.defaults` 不一定覆盖已有值；需要在 `menuconfig` 中核对，或对可丢弃的构建目录重新配置。
- 不同入口使用的分区表并不完全相同。切换构建系统应由各自工具完整烧录 bootloader、分区表与应用，不要只把应用 `.bin` 写到 0 地址，也不要混用旧分区表。这里不提供 OTA 升级保证。
- 编译通过不代表显示、音频、充电或休眠已经实机验证。验收状态见 [validation.md](validation.md)。
