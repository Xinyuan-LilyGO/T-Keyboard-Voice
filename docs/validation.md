# Validation Record and Hardware Acceptance Checklist

[English](#english) | [简体中文](#简体中文)

## English

Date: 2026-11-18.

### Current Hardware Configuration

- 7 standalone functional demos + 1 Factory, for 8 examples in total.
- Factory uses 18 consecutive tests: 13 WS2812, 14 CN4 I2C, 15 CN3 UART, 16 external MIC/AEC, 17 AUX record/playback, and 18 onboard pair record/playback.
- KEY1–KEY5 are retained; KEY5 remains GPIO0. GPIO4/5 are no longer configured or polled by this project.
- Sources, all three build systems, CI, bilingual instructions, and source test documents are aligned with the current hardware.

### Software Validation for This Revision

The table was revalidated after this revision's code changes, rather than reusing old configuration results or firmware checksums. Build scripts do not automatically flash hardware.

- PlatformIO, Arduino CLI, and ESP-IDF each built all 8 current examples successfully: 8/8 PASS per system, 24 build combinations in total.
- Host C++ regressions passed 2/2. Documentation/metadata, audio synchronization, and Factory's 18-case dispatch consistency checks also passed.
- Factory ELFs from all three builds contain 18-entry name and instruction arrays. Retired input API symbols and outdated hints were confirmed absent.

| Example | PlatformIO | Arduino IDE/CLI | ESP-IDF |
| --- | --- | --- | --- |
| Power | PASS | PASS | PASS |
| Display | PASS | PASS | PASS |
| Keys | PASS | PASS | PASS |
| WS2812 | PASS | PASS | PASS |
| AudioPlayback | PASS | PASS | PASS |
| AudioRecord | PASS | PASS | PASS |
| QWIIC | PASS | PASS | PASS |
| Factory | PASS | PASS | PASS |

Additional checks:

- Native C++ regressions cover audio unpacking and the power-key state machine; Windows uses MSVC with `/W4 /WX`.
- `scripts/check_repository.py` checks 8 examples, bilingual README naming and command consistency, local links, metadata, shared-layer isolation, and Factory's 18-item catalog/document numbering.
- `scripts/generate_audio.py --check` verifies the embedded C++ resource against the original WAV.
- Both original WAV files are unchanged. The source Word document has had entries cleaned up and numbering aligned to 18 tests; it is no longer byte-identical to the original. Original project files remain unchanged.
- All 24 ELF/BIN outputs were checked. Image sizes and SHA-256 values are recorded in `.build/hardware_refresh_manifest.json`; old images were not presented as current firmware.
- Concurrent PlatformIO build/IDE metadata testing passed, and the stable C++ entry survived cleanup. All 8 environments expose valid metadata targets.
- Native VS Code tasks, the 8-environment picker, and toolbar bindings were checked. The Factory command expanded from the new task configuration built successfully. The `reading 'id'` exception was traced to an absent task in PlatformIO 3.3.4; local entries bypass it. GUI buttons still require manual confirmation after a window reload, and installed extension files were not modified.

| Factory build | Current application BIN size (bytes) |
| --- | --- |
| PlatformIO | 940432 |
| Arduino CLI | 912864 |
| ESP-IDF | 997584 |

### Tool and Dependency Versions

| Tool/dependency | Version |
| --- | --- |
| PlatformIO Core | 6.1.19 |
| Espressif 32 platform | 55.3.311, commit `f89295f6a617a8ec611f4bd4eb2f998799dc5dc8` |
| Arduino-ESP32 | 3.3.11 |
| ESP-IDF framework libraries used by PlatformIO | 5.5.5+sha.b774170ff46 |
| Xtensa GCC | 14.2.0+20260121 |
| PlatformIO esptool | 5.3.0 |
| Arduino CLI | 1.6.0, official esp32 3.3.11, esptool 5.3.1 |
| ESP-IDF SDK | 5.5.6 + official Arduino 3.3.11 component; dependencies locked in `idf/dependencies.lock` |
| SensorLib | 0.4.1, commit `2b9e591f245e447d3d00ec8798c3f49b897882d9` |
| LovyanGFX | 1.2.26 |
| ButtonSense | 1.0.0 |
| ESP_I2S | Built into framework 3.3.11 |

### Reproduction and Outputs

```bash
python scripts/build_examples.py
python scripts/test_host.py
python scripts/check_repository.py
python scripts/generate_audio.py --check
```

After preparing each toolchain as described in its guide, run:

```bash
python scripts/prepare_dependencies.py
python scripts/build_arduino.py
# Run the following in a terminal with ESP-IDF 5.5.6 exported.
python scripts/build_idf.py
```

| Build system | Logs | Firmware outputs |
| --- | --- | --- |
| PlatformIO | `.build/logs/<environment>.log` | `.pio/build/<environment>/` |
| Arduino CLI | `.build/logs/arduino/<ExampleName>.log` | `.build/arduino/build/<ExampleName>/` |
| ESP-IDF | `.build/logs/idf/<ExampleName>.log` | `.build/idf/<ExampleName>/` |

Final Arduino CLI batch validation uses a fixed temporary sketch and official build recipes, with drivers read directly from repository sources. ESP-IDF uses the Arduino component rather than a pure ESP-IDF driver port. Different partition layouts and build recipes mean image sizes need not match; do not mix flashing files from different systems.

CI covers all three build systems, but local results do not by themselves establish that GitHub Actions ran. `.pio/`, `.build/`, exported example `build/` directories, and temporary `*.ino.cpp` files are excluded from version control.

### Hardware Acceptance Still Required

Compilation is not treated as a hardware pass. Check these items on the current physical board:

- [ ] Five-display numbers, colors, order, and shared backlight.
- [ ] All five key inputs and navigation.
- [ ] Colors, count, and chase order of all 14 WS2812 LEDs.
- [ ] Onboard microphones, external MIC3, MIC4 reference, and CN5 detection.
- [ ] Both speakers, headphones, PA, and both record/playback paths.
- [ ] External I2C on CN4 and CN3 UART peer/loopback tests.
- [ ] Sleep under USB-only and battery/no-VBUS conditions, KEY4 wake, and USB-insertion wake.
- [ ] Actual rail voltages, charging protection, and sleep current.
- [ ] All 18 Factory tests, key navigation, and manual confirmation.

Builds and native tests verify buildability and the corresponding pure logic, not audio electrical paths or other hardware acceptance. Arduino IDE compatibility is tested through the official CLI's matching build chain, not manual IDE GUI operation.

### Support Boundaries

- [Arduino IDE/CLI guide](arduino_ide.md) and [ESP-IDF guide](esp_idf.md).
- No hardware was flashed and no official production firmware was released. Hardware acceptance and resource licensing must be confirmed before a formal release.
- GitHub commits and remote CI status are authoritative on the [repository](https://github.com/Xinyuan-LilyGO/T-Keyboard-Voice) Commits/Actions pages; the local results above do not replace remote run results.

---

## 简体中文

**验证记录与实机验收清单**

日期：2026年11月18日。

### 当前硬件配置

- 7 个独立功能 Demo + 1 个 Factory，共 8 个示例。
- Factory 使用连续的 18 项测试：第 13 项 WS2812、第 14 项 CN4 I2C、第 15 项 CN3 UART、第 16 项外接 MIC/AEC、第 17 项 AUX 录放、第 18 项板载双麦录放。
- 保留 KEY1～KEY5，KEY5 仍为 GPIO0；GPIO4/5 不再由本项目配置或轮询。
- 源码、三种构建入口、CI、中英文说明及来源测试文档均按当前硬件同步。

### 本轮软件验证

本轮代码变更后重新验证下表，不沿用旧配置的构建结论或固件校验值。固件构建不会自动烧录设备。

- PlatformIO、Arduino CLI、ESP-IDF 均已完成当前 8 个示例的实际构建，分别为 8/8 PASS，合计 24 个构建组合。
- 主机 C++ 回归 2/2 PASS；文档/元数据、音频同步及 Factory 18 项分发一致性检查均通过。
- 三个入口的 Factory ELF 中，测试名称和操作说明数组均为 18 项，已确认旧输入接口符号及过期提示不再保留。

| 示例 | PlatformIO | Arduino IDE/CLI | ESP-IDF |
| --- | --- | --- | --- |
| Power | PASS | PASS | PASS |
| Display | PASS | PASS | PASS |
| Keys | PASS | PASS | PASS |
| WS2812 | PASS | PASS | PASS |
| AudioPlayback | PASS | PASS | PASS |
| AudioRecord | PASS | PASS | PASS |
| QWIIC | PASS | PASS | PASS |
| Factory | PASS | PASS | PASS |

其他检查：

- 主机 C++ 回归覆盖音频解包和电源键状态机，Windows 使用 MSVC、`/W4 /WX`。
- `scripts/check_repository.py` 检查 8 个示例、双语文档规范命名和命令一致性、文档链接、元数据、共享层隔离，以及 Factory 18 项目录和文档编号。
- `scripts/generate_audio.py --check` 检查内置 C++ 音频资源与原 WAV 一致。
- 两份原始 WAV 不变；来源 Word 文档已清理相关条目并对齐当前 18 项，不能继续视为原文件的字节等价副本。原工程文件保持不变。
- 已核验本轮 24 份固件 ELF/BIN，并将大小与 SHA-256 记录在 `.build/hardware_refresh_manifest.json`，未引用旧镜像作为当前固件。
- PlatformIO 的编译与 IDE 元数据并发复验通过，稳定 C++ 入口未被清理；8 个环境的元数据均有有效任务目标。
- VS Code 项目原生任务、8 环境选择器和工具栏绑定已校验，按新任务配置展开的 Factory 编译命令已执行通过。扩展 `reading 'id'` 异常已定位为 PlatformIO 3.3.4 的空任务访问；项目入口绕开该路径，GUI 按钮仍需重载窗口后人工确认，未修改扩展安装文件。

| Factory 入口 | 当前应用 BIN 大小（字节） |
| --- | --- |
| PlatformIO | 940432 |
| Arduino CLI | 912864 |
| ESP-IDF | 997584 |

### 工具与依赖版本

| 工具/依赖 | 版本 |
| --- | --- |
| PlatformIO Core | 6.1.19 |
| Espressif 32 平台 | 55.3.311，固定提交 `f89295f6a617a8ec611f4bd4eb2f998799dc5dc8` |
| Arduino-ESP32 | 3.3.11 |
| PlatformIO 使用的 ESP-IDF 框架库 | 5.5.5+sha.b774170ff46 |
| Xtensa GCC | 14.2.0+20260121 |
| PlatformIO esptool | 5.3.0 |
| Arduino CLI | 1.6.0，官方 esp32 3.3.11，esptool 5.3.1 |
| ESP-IDF SDK | 5.5.6 + 官方 Arduino 3.3.11 组件，依赖锁定在 `idf/dependencies.lock` |
| SensorLib | 0.4.1，固定提交 `2b9e591f245e447d3d00ec8798c3f49b897882d9` |
| LovyanGFX | 1.2.26 |
| ButtonSense | 1.0.0 |
| ESP_I2S | 框架内置 3.3.11 |

### 复现与产物

```bash
python scripts/build_examples.py
python scripts/test_host.py
python scripts/check_repository.py
python scripts/generate_audio.py --check
```

按各入口文档准备工具链后，分别运行：

```bash
python scripts/prepare_dependencies.py
python scripts/build_arduino.py
# 下面的命令需在已导出 ESP-IDF 5.5.6 的终端执行。
python scripts/build_idf.py
```

| 入口 | 日志 | 固件产物 |
| --- | --- | --- |
| PlatformIO | `.build/logs/<环境名>.log` | `.pio/build/<环境名>/` |
| Arduino CLI | `.build/logs/arduino/<示例名>.log` | `.build/arduino/build/<示例名>/` |
| ESP-IDF | `.build/logs/idf/<示例名>.log` | `.build/idf/<示例名>/` |

Arduino CLI 最终批量复验使用固定临时 Sketch 和官方编译配方，驱动直接使用仓库源码。ESP-IDF 使用 Arduino 组件，不是纯 ESP-IDF 驱动移植。不同入口的分区与编译配方不同，镜像大小不要求一致，烧录时不得混用配套文件。

CI 已覆盖三种入口，但本地结果不等于已在 GitHub Actions 执行。`.pio/`、`.build/`、示例导出的 `build/` 以及临时 `*.ino.cpp` 均不纳入版本管理。

### 尚未进行的实机验收

未将编译结果当作硬件通过，以下项目仍需连接当前实际板卡检查：

- [ ] 五屏编号、颜色、显示顺序和共享背光。
- [ ] 五个按键输入和导航。
- [ ] 14 颗 WS2812 的颜色、灯数和追逐顺序。
- [ ] 板载双麦、外接 MIC3、MIC4 参考及 CN5 检测。
- [ ] 两只扬声器、耳机、PA 和两种录音回放。
- [ ] CN4 外接 I2C、CN3 UART 对测或回环。
- [ ] USB-only、带电池/无 VBUS 的休眠、KEY4 唤醒和 USB 插入唤醒。
- [ ] 电源轨实际电压、充电保护和休眠电流。
- [ ] Factory 18 项逐项运行、按键导航和人工确认。

编译与主机测试只验证可构建性及相应纯逻辑，不证明音频电气链路或其他硬件功能合格。Arduino IDE 兼容性通过官方 CLI 的同一编译链验证，未人工操作 IDE GUI。

### 支持边界

- [Arduino IDE/CLI 使用说明](arduino_ide.md#简体中文) 与 [ESP-IDF 使用说明](esp_idf.md#简体中文)。
- 没有烧录或发布正式出厂固件；正式发布前需完成实机验收及资源许可确认。
- GitHub 提交记录与远程 CI 状态以[仓库](https://github.com/Xinyuan-LilyGO/T-Keyboard-Voice)的 Commits 和 Actions 页面为准；本节中的本机结果不代替远程运行结果。
