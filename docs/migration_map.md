# Migration Map

[English](#english) | [简体中文](#简体中文)

## English

The original project remains in the `T-Keyboard Voice` directory, whose name contains a space. The new project is `T-Keyboard-Voice`.

| Original location/function | New location |
| --- | --- |
| `include/pin_config.h` | `src/pin_config.h` |
| `include/es7210_samples.h` | `src/es7210_samples.h` |
| `include/power_key_standby.h` | `src/power_key_standby.h` |
| `src/voice_display.cpp`, `include/voice_display.h` | `src/VoiceDisplay.cpp/.h` |
| AXP2101, battery, I2C register access, sleep/wake functions | `src/VoicePower.cpp/.h` |
| Five-display initialization and backlight | `src/VoiceDisplay.cpp/.h` |
| Five-key initialization and event management | `src/VoiceInput.cpp/.h` |
| RMT and WS2812 output | `src/VoiceRGB.cpp/.h` |
| ES8389/ES7210, I2S lifecycle, recording/playback, and sample unpacking calls | `src/VoiceAudio.cpp/.h` |
| Test catalog, PASS/FAIL, five-display pages, waveforms, key navigation, and serial commands | `examples/Factory/Factory.cpp` |
| Test audio | `assets/66_stereo_48k.wav`, original bytes unchanged |
| Other original audio | `assets/original/66.wav`, archive only |
| Original factory-test DOCX | `docs/hardware/`, entries cleaned up and numbering aligned to the current 18 tests; the original project copy is retained |
| Two host regression tests | `tests/`, with an updated build entry and the core cases retained |

Shared functions use the `KeyboardVoice` namespace. Pin definitions `Pins`, device addresses `Devices`, and the single-panel `VoiceDisplay` class retain their original names. Rail masks use `PowerRail::Audio`, `KeyboardDisplay`, `Rgb`, and `All` to avoid Arduino macro conflicts.

Required changes:

- `initializeAudio(bool)` is replaced by explicit `AudioMode` selection instead of inferring the format from the current Factory test number.
- Waveform UI stays in Factory; low-level capture goes through `readMicrophoneFrames()`, so the standalone recording demo does not depend on LCDs.
- The power layer invokes application cleanup/recovery through `SleepHooks` and does not reference Factory state.
- PMIC baseline initialization preserves MCU power and enables other required rails on demand; Factory still requests all peripheral rails.
- GPIO/register timing, manual decisions, and external I2C pass conditions needed by the current hardware are retained. Current test numbering is defined by Factory's 18-item catalog.

The migration did not copy `.pio`, editor caches, `tmp`, or the unused `lv_esp_idf` directory. Dependencies are obtained again through pinned build configurations.

---

## 简体中文

**迁移对照**

原工程保留在带空格的 `T-Keyboard Voice` 目录，新工程为 `T-Keyboard-Voice`。

| 原位置/函数 | 新位置 |
| --- | --- |
| `include/pin_config.h` | `src/pin_config.h` |
| `include/es7210_samples.h` | `src/es7210_samples.h` |
| `include/power_key_standby.h` | `src/power_key_standby.h` |
| `src/voice_display.cpp`、`include/voice_display.h` | `src/VoiceDisplay.cpp/.h` |
| AXP2101、电池、I2C 寄存器、休眠/唤醒函数 | `src/VoicePower.cpp/.h` |
| 五屏初始化、背光 | `src/VoiceDisplay.cpp/.h` |
| 五键初始化与事件管理 | `src/VoiceInput.cpp/.h` |
| RMT 与 WS2812 输出 | `src/VoiceRGB.cpp/.h` |
| ES8389/ES7210、I2S 生命周期、录放与解包调用 | `src/VoiceAudio.cpp/.h` |
| 测试表、PASS/FAIL、五屏页面、波形、按键导航、串口命令 | `examples/Factory/Factory.cpp` |
| 测试音频 | `assets/66_stereo_48k.wav`，原文件字节不变 |
| 另一份原始音频 | `assets/original/66.wav`，仅归档 |
| 原出厂测试 docx | `docs/hardware/`，按当前硬件清理条目并对齐 18 项编号；原工程副本保留 |
| 两个主机回归测试 | `tests/`，更新构建入口，核心用例保留 |

共享函数位于 `KeyboardVoice` namespace。引脚 `Pins`、设备地址 `Devices`、单屏类 `VoiceDisplay` 保留原命名。电源轨掩码使用 `PowerRail::Audio`、`KeyboardDisplay`、`Rgb`、`All`，避免与 Arduino 宏冲突。

必要改动：

- `initializeAudio(bool)` 改为显式 `AudioMode`，不再根据当前 Factory 测试编号选格式。
- 波形 UI 留在 Factory，底层读样本通过 `readMicrophoneFrames()`，独立采集 Demo 不依赖 LCD。
- 供电层通过 `SleepHooks` 调用应用收尾/恢复，不引用 Factory 状态。
- 公共 PMIC 基础初始化保留 MCU 供电，其他必要电源轨按需使能；Factory 仍请求全部外设电源轨。
- 保留当前硬件所需的 GPIO/寄存器时序、人工判定及外接 I2C 通过条件；测试编号以 Factory 当前 18 项为准。

没有复制 `.pio`、编辑器缓存、`tmp` 或未被现有源码引用的 `lv_esp_idf` 目录。依赖重新由固定版本配置获取。
