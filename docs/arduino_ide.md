# Arduino IDE / Arduino CLI

[English](#english) | [简体中文](#简体中文)

## English

### Support Model

The 7 demos and Factory use the existing `examples/` directly; no separate examples or driver changes are required. Use **Arduino-ESP32 3.3.11** and select **ESP32S3 Dev Module**. Do not use Arduino-ESP32 2.x.

The test WAV is a read-only C++ resource inside the library. Arduino IDE no longer requires linker embedding options, LittleFS, an SD card, or a separate audio upload.

### Generate Arduino IDE Installation ZIPs

With Python available, run from the repository root:

```bash
python scripts/prepare_dependencies.py
python scripts/package_arduino.py
```

The output directory `.build/arduino/zips/` contains `TKeyboardVoice.zip`, `SensorLib.zip`, `LovyanGFX.zip`, and `ButtonSense.zip`. Install them through **Sketch → Include Library → Add .ZIP Library**. The packages exclude `.pio/`, SDKs, and build caches. These library packages are for Arduino IDE; use the original repository for other build systems and maintenance scripts.

### Arduino IDE 2.x Installation

1. Add this URL under **Preferences → Additional Boards Manager URLs**:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Install Espressif's **esp32 3.3.11** through Boards Manager.
3. Place this repository in your Sketchbook at `libraries/TKeyboardVoice/`, with `library.properties` and `src/` directly inside that directory, not one level deeper. Restart the IDE.
4. Install LovyanGFX **1.2.26** and ButtonSense **1.0.0**. SensorLib must be **0.4.1 from the pinned commit below**; do not substitute an older `XPowersLib`:
   - [Pinned SensorLib ZIP](https://github.com/lewisxhe/SensorLib/archive/2b9e591f245e447d3d00ec8798c3f49b897882d9.zip)
   - [LovyanGFX 1.2.26 ZIP](https://github.com/lovyan03/LovyanGFX/archive/3f78b7058fca0fd1fc1ddb32f75a18262e01d440.zip)
   - [ButtonSense 1.0.0 ZIP](https://github.com/lbuque/ButtonSense/archive/b1aeb22a73c523b37ebd11bee54000b77484f76c.zip)
   Use **Sketch → Include Library → Add .ZIP Library**. Remove older libraries with the same names through Library Manager or move them out of the Sketchbook so the IDE does not select the wrong version.
5. Open an example through **File → Examples → LilyGO T-Keyboard Voice**, or open its `.ino` directly. Do not copy only the `.ino`: keep `build_opt.h` in the same directory, and also retain `Factory.cpp` for Factory.

`build_opt.h` is a compiler-option response file read by the official Arduino-ESP32 build recipe, not a C++ header to `#include`. It limits SensorLib to this board's AXP2101 support, matching PlatformIO/ESP-IDF and avoiding unnecessary sensor-driver compilation.

### Board Menus

| Menu | Setting |
| --- | --- |
| Board | ESP32S3 Dev Module |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240 MHz |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS), `app3M_fat9M_16MB` |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600; lower to 460800 or 115200 if unreliable |
| Port | The board's actual USB serial port |

Set Serial Monitor to **115200 baud** and terminate commands with a newline. CN3 UART0 is not the USB console. See each example README for wiring and power precautions.

If automatic flashing fails, hold BOOT/KEY5, briefly press reset, then release BOOT. Reselect the download-mode port. After flashing, you may need to select the runtime USB port again.

### Arduino CLI Batch Validation

After installing Arduino CLI, run from the repository root:

```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
python scripts/prepare_dependencies.py
python scripts/build_arduino.py
```

Build selected examples:

```bash
python scripts/build_arduino.py Factory
python scripts/build_arduino.py AudioPlayback AudioRecord
```

Use `--cli`, `--config-file`, and `--deps-dir` to select the tool and isolated configuration. The script does not flash hardware or modify global library directories. To avoid rebuilding libraries when sketch paths or dependency-discovery order change, it generates a temporary copy of the selected example under `.build/arduino/sketch/VoiceExample/` and includes a fixed set of board headers first. It does not modify `examples/` or maintain a second application implementation; drivers are read directly from repository `src/`.

The shared `.build/arduino/work/` directory reuses core/library compilation caches. Per-example outputs are copied to `.build/arduino/build/<ExampleName>/`, with logs in `.build/logs/arduino/`. Batch output filenames use the common `VoiceExample.ino` prefix; the containing directory identifies the example. Do not manually compile into the same work directory while the script is running.

Arduino CLI uses the same platform, build recipes, library resolution, and `.ino` preprocessing as Arduino IDE. A CLI build pass does not mean that the IDE GUI was manually operated or that hardware acceptance is complete.

### Updating Test Audio

```bash
python scripts/generate_audio.py
python scripts/generate_audio.py --check
```

After changing `assets/66_stereo_48k.wav`, regenerate `src/audio/test_audio.inc` and version both files together. The resource stays in Flash rather than being copied entirely into RAM at startup. AudioRecord does not call test-clip playback, so the linker removes the unused WAV.

---

## 简体中文

**Arduino IDE / Arduino CLI**

### 支持方式

7 个 Demo 和 Factory 直接使用现有 `examples/`，不需要另一套示例或修改驱动。使用 **Arduino-ESP32 3.3.11**，板型选择 **ESP32S3 Dev Module**。不要使用 Arduino-ESP32 2.x。

测试 WAV 已转换为库内只读 C++ 资源，Arduino IDE 不再需要 linker 嵌入参数、LittleFS、SD 卡或额外上传音频文件。

### 生成 Arduino IDE 安装包

已有 Python 时，可在仓库根目录执行：

```bash
python scripts/prepare_dependencies.py
python scripts/package_arduino.py
```

`.build/arduino/zips/` 会生成 `TKeyboardVoice.zip`、`SensorLib.zip`、`LovyanGFX.zip`、`ButtonSense.zip`。在 IDE 中依次使用「项目 → 加载库 → 添加 .ZIP 库」安装即可；四个包不包含 `.pio/`、SDK 和构建缓存。生成的库包用于 Arduino IDE；其他构建入口和维护脚本请使用原仓库。

### Arduino IDE 2.x 安装

1. 在「首选项 → 附加开发板管理器网址」中加入：
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. 在开发板管理器安装 Espressif 的 **esp32 3.3.11**。
3. 将本仓库放到 Sketchbook 的 `libraries/TKeyboardVoice/` 中，确保 `library.properties` 和 `src/` 直接位于该目录下，不要多套一层目录。重启 IDE。
4. 安装 LovyanGFX **1.2.26** 和 ButtonSense **1.0.0**。SensorLib 必须使用下面固定提交对应的 **0.4.1**，不要混用旧版 `XPowersLib`：
   - [SensorLib 固定提交 ZIP](https://github.com/lewisxhe/SensorLib/archive/2b9e591f245e447d3d00ec8798c3f49b897882d9.zip)
   - [LovyanGFX 1.2.26 ZIP](https://github.com/lovyan03/LovyanGFX/archive/3f78b7058fca0fd1fc1ddb32f75a18262e01d440.zip)
   - [ButtonSense 1.0.0 ZIP](https://github.com/lbuque/ButtonSense/archive/b1aeb22a73c523b37ebd11bee54000b77484f76c.zip)
   可以使用「项目 → 加载库 → 添加 .ZIP 库」安装。若已有同名旧库，请先通过库管理器移除或移出 Sketchbook，避免 IDE 选错版本。
5. 从「文件 → 示例 → LilyGO T-Keyboard Voice」打开示例，或直接打开仓库中对应的 `.ino`。不要单独复制 `.ino`：同目录的 `build_opt.h` 必须保留，Factory 还需保留 `Factory.cpp`。

`build_opt.h` 是 Arduino-ESP32 官方构建配方读取的编译参数文件，不是要 `#include` 的 C++ 头文件。它使 SensorLib 仅保留本板需要的 AXP2101，与 PlatformIO/ESP-IDF 的配置一致，避免编译大量无关传感器驱动。

### 开发板菜单

| 菜单 | 设置 |
| --- | --- |
| Board | ESP32S3 Dev Module |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| CPU Frequency | 240 MHz |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS)，对应 `app3M_fat9M_16MB` |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600；不稳定时降至 460800 或 115200 |
| Port | 设备实际 USB 串口 |

串口监视器设为 **115200**，命令以换行结束。CN3 的 UART0 不是 USB 控制台；接口与供电注意事项仍见各示例 README。

如果自动下载失败，按住 BOOT/KEY5，短按复位键后再松开 BOOT；重新选择下载模式下的端口。上传成功后可能需要重新选择运行态 USB 端口。

### Arduino CLI 批量验证

安装 Arduino CLI 后，在仓库根目录执行：

```bash
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@3.3.11 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
python scripts/prepare_dependencies.py
python scripts/build_arduino.py
```

只编译一个示例：

```bash
python scripts/build_arduino.py Factory
python scripts/build_arduino.py AudioPlayback AudioRecord
```

可使用 `--cli`、`--config-file`、`--deps-dir` 指定工具与隔离配置。脚本不烧录，也不修改全局库目录。为了避免 Arduino CLI 因 Sketch 路径和依赖发现顺序变化而重复编译所有库，脚本在 `.build/arduino/sketch/VoiceExample/` 生成同一示例的临时编译副本，并预先包含固定的板级头文件；不会改写 `examples/`，也不维护另一份应用源码，驱动直接来自仓库 `src/`。

脚本共用 `.build/arduino/work/` 以复用核心和库编译缓存，每个示例的输出另存到 `.build/arduino/build/<示例名>/`；日志在 `.build/logs/arduino/`。批量产物的文件名前缀统一为 `VoiceExample.ino`，以所在目录区分实际示例。脚本运行时不要同时手动使用同一工作目录编译。

Arduino CLI 使用 Arduino IDE 同一套平台、编译配方、库解析与 `.ino` 预处理器；CLI 编译通过不等于 IDE GUI 已人工操作或硬件已经验收。

### 更新音频

```bash
python scripts/generate_audio.py
python scripts/generate_audio.py --check
```

修改 `assets/66_stereo_48k.wav` 后重新生成 `src/audio/test_audio.inc`，并将生成结果与原 WAV 一起纳入版本管理。资源保存在 Flash 中，不在启动时复制整个 WAV 到 RAM。AudioRecord 不调用测试音频播放，链接器会剔除未使用的测试 WAV。
