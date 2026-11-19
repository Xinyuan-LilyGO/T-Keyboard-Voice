# Dependencies and Resource Provenance

[English](#english) | [简体中文](#简体中文)

## English

| Dependency | Pinned version/commit | Purpose |
| --- | --- | --- |
| pioarduino platform fork | `meshtastic/pioarduino-platform-espressif32`, `f89295f6a617a8ec611f4bd4eb2f998799dc5dc8` | Arduino-ESP32 3.x toolchain |
| SensorLib | `2b9e591f245e447d3d00ec8798c3f49b897882d9` | AXP2101 PMIC |
| LovyanGFX | `1.2.26` | GC9107 displays |
| ButtonSense | `1.0.0` | Key events |
| esptool | `v5.3.0` source archive | Build/flashing tool; avoids incomplete packages containing only a launcher |

Dependencies are fetched by PlatformIO or ESP-IDF Component Manager. The three shared libraries for Arduino IDE/CLI and ESP-IDF can also be downloaded by `scripts/prepare_dependencies.py` into the ignored `.build/deps/` directory. Full third-party library sources are not committed to this project. LovyanGFX is pinned to `3f78b7058fca0fd1fc1ddb32f75a18262e01d440`, and ButtonSense to `b1aeb22a73c523b37ebd11bee54000b77484f76c`. Their `src/` contents were compared file by file with the original PlatformIO dependencies and matched. Each dependency's own license, author, and copyright declarations remain authoritative.

ESP-IDF uses the official `espressif/arduino-esp32` 3.3.11 component with ESP-IDF 5.5.6. Arduino IDE/CLI uses the official esp32 3.3.11 board package. See the [Arduino guide](arduino_ide.md) and [ESP-IDF guide](esp_idf.md).

Shared drivers were extracted from the original project's `src/main.cpp`, `src/voice_display.cpp`, and `include/`. ES8389/ES7210 register sequences retain the original implementation, whose comments cited Linux ES8389, ESP-ADF, and Espressif ES7210 drivers. This migration does not claim those configurations as independently original work.

`assets/66_stereo_48k.wav` and `assets/original/66.wav` come from the original project, with audio contents unchanged. Test documents under `docs/hardware/` have the same provenance; this repository's copy has had entries cleaned up and numbering aligned to the current 18 tests, while the original project files remain unchanged. This reference does not replace the current Factory README or hardware acceptance testing. Redistribution rights for code and resources still require confirmation. The test clip is used only by Factory and AudioPlayback; AudioRecord plays back its own captured data.

---

## 简体中文

**外部依赖与资料来源**

| 依赖 | 固定版本/提交 | 用途 |
| --- | --- | --- |
| pioarduino 平台分支 | `meshtastic/pioarduino-platform-espressif32`，`f89295f6a617a8ec611f4bd4eb2f998799dc5dc8` | Arduino-ESP32 3.x 工具链 |
| SensorLib | `2b9e591f245e447d3d00ec8798c3f49b897882d9` | AXP2101 PMIC |
| LovyanGFX | `1.2.26` | GC9107 显示 |
| ButtonSense | `1.0.0` | 按键事件 |
| esptool | `v5.3.0` 源码归档 | 构建/烧录工具，避免仅含启动器的不完整包 |

依赖由 PlatformIO 或 ESP-IDF Component Manager 获取；Arduino IDE/CLI 和 ESP-IDF 的三项共用库也可通过 `scripts/prepare_dependencies.py` 下载到忽略目录 `.build/deps/`。没有把整套第三方库源码纳入本工程。LovyanGFX 固定提交为 `3f78b7058fca0fd1fc1ddb32f75a18262e01d440`，ButtonSense 固定提交为 `b1aeb22a73c523b37ebd11bee54000b77484f76c`；已与原 PlatformIO 依赖的 `src/` 内容逐文件比对一致。各库许可、作者和版权以其自身声明为准。

ESP-IDF 使用官方 `espressif/arduino-esp32` 3.3.11 组件和 ESP-IDF 5.5.6。Arduino IDE/CLI 使用官方 esp32 3.3.11 开发板包。安装说明见 [Arduino](arduino_ide.md#简体中文) 与 [ESP-IDF](esp_idf.md#简体中文)。

公共驱动由原工程 `src/main.cpp`、`src/voice_display.cpp` 及 `include/` 提取。ES8389、ES7210 寄存器时序沿用原实现，其注释原先注明参考 Linux ES8389、ESP-ADF 和 Espressif ES7210 驱动；本次迁移不声称这些配置为独立原创。

`assets/66_stereo_48k.wav` 与 `assets/original/66.wav` 来自原工程，音频内容未更改。`docs/hardware/` 下的测试文档同样来源于原工程，本仓库副本已按当前硬件清理相关条目并对齐 18 项编号，原工程文件未改写；该参考资料不替代当前 Factory README 和实际验收。代码及资源的再分发许可仍需单独确认。测试音频只用于 Factory 和 AudioPlayback，AudioRecord 回放使用实时录制的数据。
