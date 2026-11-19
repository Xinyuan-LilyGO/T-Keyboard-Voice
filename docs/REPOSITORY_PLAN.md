# T-Keyboard Voice Repository Plan (Compact)

[English](#english) | [简体中文](#简体中文)

## English

### Goal

Following the T-Glass-V3 layout, group related functions into **7 standalone demos + 1 Factory** instead of creating a separate demo for every test item.

The new local directory is `D:\Code2\T-Keyboard-Voice`. The original project is retained without overwriting its firmware. The structure and code have been implemented; see [validation.md](validation.md) for validation status.

### Demo Groups

| Demo | Features |
| --- | --- |
| Power | AXP2101, battery telemetry, KEY4 sleep, and KEY4/USB wake |
| Display | Five-display numbering/colors and shared backlight control |
| Keys | Press, release, click, and long-press events for all five keys |
| WS2812 | Colors, individual lighting, and chase effects for 14 RGB LEDs |
| AudioPlayback | ES8389 initialization, speaker/headphone playback, and PA control |
| AudioRecord | ES7210 initialization, onboard/external capture, AEC reference reading, and record-then-playback |
| QWIIC | CN4 I2C scanning and CN3 UART send/receive |
| Factory | The current hardware's 18 board tests and interactions |

HelloWorld, chip scans, backlight, PA, headphones, individual microphone modes, and playback variants are not separate demos. They are selected through simple key/serial controls within the corresponding group. Grouping demos does not remove Factory test coverage.

### File Structure

```text
T-Keyboard-Voice/
├── README.md                   # Quick start, demo list, and hardware overview
├── platformio.ini              # Board, dependencies, and example selection
├── library.json
├── library.properties
├── boards/                     # Existing board definition
├── src/                        # Shared drivers, no application entry
│   ├── pin_config.h
│   ├── VoicePower.h/.cpp       # PMIC, battery, sleep/wake
│   ├── VoiceDisplay.h/.cpp     # Five panels, shared SPI, backlight
│   ├── VoiceInput.h/.cpp       # Five keys
│   ├── VoiceRGB.h/.cpp         # WS2812
│   ├── VoiceAudio.h/.cpp       # ES8389, ES7210, I2S, audio processing
│   ├── es7210_samples.h        # Retained sample unpacking logic
│   └── power_key_standby.h     # Retained power-key state machine
├── examples/
│   ├── Power/
│   ├── Display/
│   ├── Keys/
│   ├── WS2812/
│   ├── AudioPlayback/
│   ├── AudioRecord/
│   ├── QWIIC/
│   └── Factory/
├── assets/                     # Shared test audio and resources
├── tests/                      # Two retained host regression tests
└── docs/                       # Factory instructions and hardware references
```

Each regular demo contains its matching `.ino`, `README.md`, and Arduino IDE `build_opt.h` compiler options. Factory keeps its application and required helpers without splitting the application into many speculative modules.

### Separation Rules

- Each demo initializes only its required peripherals; record-then-playback requests the playback path when needed.
- Demos and Factory share one driver implementation, without duplicate drivers or including each other's `.ino` files.
- Factory uses 18 consecutive test numbers, serial commands, decision rules, and key controls.
- Do not introduce extra abstraction layers for a few lines of UART or I2C scan code.
- Do not add a UI framework or features absent from the original project.

### Implementation Order

1. **Migrate Factory:** establish a complete build in the new directory and confirm toolchain/dependencies.
2. **Extract drivers:** power → displays → input/RGB → audio, keeping Factory usable.
3. **Implement 7 demos:** each group must build, upload, and operate independently.
4. **Validate and document:** build all examples independently, retain host regressions, and perform hardware acceptance item by item.

### Behavior to Preserve

- PMIC charging/TS protection, rail sequencing, and KEY4/USB wake. Deep sleep is not PMIC power-off.
- Shared SPI/reset for the five displays and active-low backlight.
- ES7210 dual/four-channel formats and recording/playback switching protection. AEC reference reading is not an echo-cancellation algorithm.
- CN4 shares onboard I2C; CN3 UART0 remains separate from USB CDC.

PlatformIO was the initial validation baseline. [Arduino IDE/CLI](arduino_ide.md) and an [ESP-IDF Arduino-component entry](esp_idf.md) were added later, sharing read-only audio and drivers. See [validation.md](validation.md) for actual build results.

**Deliverable: 7 functional demos + 1 complete Factory, covering all features of the current hardware.**

---

## 简体中文

**T-Keyboard Voice 仓库整理方案（精简版）**

### 目标

参考 T-Glass-V3，按功能分组整理为 **7 个独立 Demo + 1 个 Factory**。相关功能合并演示，不再把每个测试项都单独建一个 Demo。

实际新目录：`D:\Code2\T-Keyboard-Voice`。原工程保留，不覆盖原固件。结构和代码已按本方案建立，验证状态见 [validation.md](validation.md#简体中文)。

### Demo 分组

| Demo | 包含功能 |
| --- | --- |
| Power | AXP2101、电池信息、KEY4 休眠、KEY4/USB 唤醒 |
| Display | 五屏编号/颜色显示、共享背光调节 |
| Keys | 五个按键的按下、释放、单击、长按 |
| WS2812 | 14 颗 RGB 灯的颜色、逐颗点亮和追逐效果 |
| AudioPlayback | ES8389 初始化、扬声器/耳机播放、PA 控制 |
| AudioRecord | ES7210 初始化、板载/外接麦克风采集、AEC 参考读取、录音后回放 |
| QWIIC | CN4 I2C 扫描、CN3 UART 收发 |
| Factory | 覆盖当前硬件的 18 项整机测试及交互 |

不再单设 HelloWorld、芯片扫描、背光、PA、耳机、不同麦克风及回放 Demo；这些内容放入对应分组，用简单的按键或串口命令选择。Demo 合并不代表删减 Factory 的测试项。

### 文件结构

```text
T-Keyboard-Voice/
├── README.md                   # 快速开始、Demo 列表、引脚说明
├── platformio.ini              # 板型、依赖、示例选择
├── library.json
├── library.properties
├── boards/                     # 保留现有板型定义
├── src/                        # 共用驱动，不放整机入口
│   ├── pin_config.h
│   ├── VoicePower.h/.cpp       # PMIC、电池、休眠唤醒
│   ├── VoiceDisplay.h/.cpp     # 五屏、共享 SPI、背光
│   ├── VoiceInput.h/.cpp       # 五键
│   ├── VoiceRGB.h/.cpp         # WS2812
│   ├── VoiceAudio.h/.cpp       # ES8389、ES7210、I2S、音频处理
│   ├── es7210_samples.h        # 保留音频解包逻辑
│   └── power_key_standby.h     # 保留电源键状态机
├── examples/
│   ├── Power/
│   ├── Display/
│   ├── Keys/
│   ├── WS2812/
│   ├── AudioPlayback/
│   ├── AudioRecord/
│   ├── QWIIC/
│   └── Factory/
├── assets/                     # 统一测试音频等资源
├── tests/                      # 保留现有两个主机回归测试
└── docs/                       # 出厂测试说明、硬件资料
```

每个普通 Demo 放 `示例名.ino`、`README.md` 和供 Arduino IDE 读取的 `build_opt.h` 编译参数。Factory 保留主程序和必要辅助文件，不预先拆成大量应用模块。

### 拆分原则

- 每个 Demo 仅初始化本组需要的外设；录音后回放按需使用播放链路。
- Demo 和 Factory 使用同一份公共驱动，不复制多份，不互相包含 `.ino`。
- Factory 使用连续的 18 项编号、串口命令、判定规则和按键操作。
- UART 收发、I2C 扫描等简单功能不为几行代码额外建立封装层。
- 不引入新 UI 框架，不添加原工程没有的功能。

### 制作顺序

1. **迁移 Factory**：先在新目录建立能编译的完整固件，确认工具链和依赖。
2. **提取驱动**：电源 → 显示 → 输入/RGB → 音频，保持 Factory 可用。
3. **制作 7 个 Demo**：按上表分组，每组可以独立编译、烧录、操作。
4. **验证和补文档**：所有示例独立编译，保留主机回归测试，逐项实机验收。

### 必须保留

- PMIC 充电/TS 保护、电源轨时序、KEY4 与 USB 唤醒行为；deep sleep 不等于 PMIC 关机。
- 五屏共享 SPI/复位、背光低电平有效。
- ES7210 双通道与四通道数据格式、录放切换保护；AEC 参考读取不等于实现回声消除算法。
- CN4 与板载 I2C 共用总线，CN3 UART0 与 USB CDC 分开。

首轮以 PlatformIO 为验证基线。后续增加了 [Arduino IDE/CLI 入口](arduino_ide.md#简体中文) 和 [ESP-IDF Arduino 组件入口](esp_idf.md#简体中文)，统一只读音频资源与驱动；各入口的实际编译结果见 [validation.md](validation.md#简体中文)。

**最终交付：7 个功能 Demo + 1 个完整 Factory，覆盖当前硬件的全部功能。**
