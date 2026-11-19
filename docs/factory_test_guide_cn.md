# Factory Test Guide

[English](#english) | [简体中文](#简体中文)

## English

For T-Keyboard Voice V0.3. This is the code-migration baseline; hardware verification is required before formal production testing.

### Preparation

Prepare a USB data cable, test battery, two speakers, headphones, a CN5 external audio source/microphone, an I2C peripheral that does not conflict with onboard addresses, and 3.3 V UART test equipment. See the [pinout](pinmap.md).

Flash `factory` and open the 115200-baud USB CDC console. Start with `help`, `scan`, and `power` to check PSRAM, PMIC, and power logs. The test clip may be loud; keep speakers/headphones away from your ears on the first playback.

### 18 Tests

| Number | Test | Action/observation | Current firmware decision |
| --- | --- | --- | --- |
| 1 | AXP2101 | Check 0x34, PMIC initialization, and rail readbacks | Automatic |
| 2 | Battery telemetry | Battery connection, capacity/voltage, charge state, and faults | Automatic; no battery gives FAIL (Battery not connected) |
| 3 | Power key/sleep | Hold KEY4 for 3 seconds and release; check key wake and USB-insertion wake | Manual |
| 4 | ES8389 address | Acknowledgment at 0x10 | Automatic; address acknowledgment only |
| 5 | ES7210 address | Acknowledgment at 0x40 | Automatic; address acknowledgment only |
| 6 | Speakers | Play the test WAV and check both speakers | Manual |
| 7 | PA | Use KEY2 to toggle the PA and play; compare sound | Manual |
| 8 | Onboard microphone pair | Speak from equal distance; observe MIC1/MIC2 waveforms and `mic` logs | Manual |
| 9 | Headphones | With PA off, check headphone audio | Manual |
| 10 | Five displays | Verify panel order, colors, and numbers | Manual |
| 11 | Backlight | Verify synchronized fade, off, and full brightness | Manual |
| 12 | Five keys | Check all inputs and a seen mask of 0x1F | Automatic when all five have been seen; otherwise manual confirmation is pending |
| 13 | WS2812 | Check all 14 LEDs: RGBW, chase, and position patterns | Manual |
| 14 | CN4 I2C | Connect a peripheral and find an address other than 0x10/0x34/0x40 | Automatic |
| 15 | CN3 UART | The peer receives text; returned data appears on USB | Manual |
| 16 | External mic/AEC reference | Select `run 16`, feed MIC3, and observe MIC3/MIC4 | Manual |
| 17 | AUX record/playback | Record MIC3 for 1 second, then play through both speakers | Manual |
| 18 | Onboard pair record/playback | Record MIC1/MIC2, process and mix, then play through both speakers | Manual |

Test 2 retains the original coarse limits: VBAT 2500–5000 mV, VSYS 2500–5500 mV, and capacity 0–100%, together with valid reads, charge status/settings, and fault checks. These software limits are not battery specification acceptance bounds; formal limits must follow the actual hardware/battery specifications.

### Controls

- `run N`: select and execute a test; `next`/`prev`: move to adjacent tests.
- `run`: execute all tests sequentially; it does not complete manual listening/inspection automatically.
- `pass`/`fail`: record an operator decision for the current test.
- `status`: print all results.
- `power`: power-register and wake diagnostics.
- `mic`: ADC configuration, recent-block RMS, clipping, and channel differences.

KEY1/KEY5 navigate; KEY2 reruns; KEY3/KEY4 short clicks mark PASS/FAIL. KEY2 toggles the PA in test 7. In test 12, KEY2–KEY4 are test inputs. The original behavior is retained: KEY1/KEY5 navigation remains active during test 12, and the seen mask accumulates during the current boot. A manual PASS can override the current result, so review the final report together with operator observations.

### Decision Limits

- `WAIT`, `RUN`, `PASS`, `FAIL`, and `CHECK` mean untested, running, passed, failed, and awaiting manual confirmation.
- I2C acknowledgment does not prove complete device functionality. A successful DMA write does not prove that a speaker or headphone actually produced sound.
- To preserve the original rule, test 14 passes based only on finding an external address. Missing onboard addresses are also logged and must not be ignored.
- Test 16 insertion detection is only a hint. MIC4 is a reference input, not an implemented AEC algorithm.
- Record-then-playback is not real-time monitoring; noise without a valid input is not a valid recording.
- Sleep is ESP32 deep sleep with DCDC1 retained. Verify USB-only, battery-connected, and no-VBUS conditions separately; see [power and sleep](power_and_sleep.md).

Before production release, record actual results, hardware revision, firmware checksum, and operator details for every test. Source documents under `hardware/` have had entries cleaned up and numbering aligned to the current 18 tests; they are reference material. The current Factory README defines operating steps, and documentation alone is not evidence of hardware acceptance.

---

## 简体中文

**Factory 出厂测试指南**

适用 T-Keyboard Voice V0.3。当前为代码迁移基线，正式产测前必须完成实机验证。

### 准备

USB 数据线、测试电池、两只扬声器、耳机、CN5 外接音源/麦克风、一个不与板载地址冲突的 I2C 设备，以及 3.3 V UART 对测设备。接线见 [引脚表](pinmap.md#简体中文)。

烧录 `factory`，打开 115200 USB CDC 串口。先执行 `help`、`scan`、`power`，确认 PSRAM、PMIC 和供电日志。测试音频音量较大，首次播放不要把扬声器或耳机贴近耳朵。

### 18 项测试

| 编号 | 测试 | 操作/观察 | 当前固件判定方式 |
| --- | --- | --- | --- |
| 1 | AXP2101 | 检查 0x34、PMIC 初始化及电源轨读回 | 自动 |
| 2 | 电池信息 | 电池连接、电量/电压、充电状态与故障 | 自动；无电池为 FAIL（Battery not connected） |
| 3 | 电源键/休眠 | 长按 KEY4 3 秒再释放，测试按键及 USB 插入唤醒 | 人工 |
| 4 | ES8389 地址 | 0x10 应答 | 自动，仅证明地址应答 |
| 5 | ES7210 地址 | 0x40 应答 | 自动，仅证明地址应答 |
| 6 | 扬声器 | 播放测试 WAV，检查左右扬声器 | 人工 |
| 7 | PA | KEY2 切换 PA 并播放，比较声音 | 人工 |
| 8 | 板载双麦 | 对等距声源讲话，观察 MIC1/MIC2 波形和 `mic` 日志 | 人工 |
| 9 | 耳机 | PA 关闭，检查耳机声音 | 人工 |
| 10 | 五屏显示 | 屏序、颜色和编号正确 | 人工 |
| 11 | 背光 | 渐变、熄灭、全亮同步正常 | 人工 |
| 12 | 五键 | 确认各键输入，seen 掩码 0x1F | 五键已见时自动，否则待人工 |
| 13 | WS2812 | 14 灯 RGBW、逐颗追逐及位置图案 | 人工 |
| 14 | CN4 I2C | 接入外设并扫描到非 0x10/0x34/0x40 的地址 | 自动 |
| 15 | CN3 UART | 对端收到文本，发送数据回 USB 控制台 | 人工 |
| 16 | 外接麦/AEC 参考 | 选择 `run 16`，向 MIC3 输入信号并观察 MIC3/MIC4 | 人工 |
| 17 | AUX 回放 | MIC3 录制 1 秒后双扬声器输出 | 人工 |
| 18 | 板载双麦回放 | MIC1/MIC2 录制、处理、混音后双扬声器输出 | 人工 |

第 2 项保留原电压范围：VBAT 2500～5000 mV、VSYS 2500～5500 mV、电量 0～100%，并检查读取有效、充电状态码/设定和故障。软件阈值不是电池规格书合格区间，正式验收仍需按硬件/电池规格制定边界。

### 操作

- `run N`：选择指定项目；`next`/`prev`：相邻项目。
- `run`：依次执行全部项目，不会自动完成人工听音/观察。
- `pass`/`fail`：对当前项目记人工结果。
- `status`：输出全部测试状态。
- `power`：电源寄存器与唤醒诊断。
- `mic`：ADC 配置、最近采样块 RMS、削波和通道差异。

KEY1/KEY5 为导航，KEY2 重测，KEY3/KEY4 短按为人工 PASS/FAIL；第 7 项 KEY2 切换 PA，第 12 项 KEY2～4 作为被测输入。沿用原行为：第 12 项 KEY1/KEY5 的导航仍有效，seen 掩码在本次运行中累计。人工 PASS 可以覆盖当前结果，因此最终报告应与操作者观察记录结合审查。

### 判定边界

- 状态 `WAIT`、`RUN`、`PASS`、`FAIL`、`CHECK` 分别为未测、运行中、通过、失败、待人工确认。
- 有 I2C 应答不等于芯片功能完整；DMA 写入成功不等于扬声器/耳机实际发声。
- 第 14 项为保持原规则，仅依据外接地址自动判定；日志同时列出缺失的板载地址，不能忽略其他失败项目。
- 第 16 项的检测脚仅提示插入；MIC4 是参考读取，不是已实现 AEC 算法。
- 录音回放不是实时监听；没有接入信号时噪声不算有效录音。
- 休眠是 ESP32 deep sleep，DCDC1 保留，必须分别验证 USB-only、带电池和无 VBUS 情况；详见 [供电与休眠](power_and_sleep.md#简体中文)。

正式出厂前逐项补齐实测结果、硬件版本、固件校验值和操作者记录。`hardware/` 中的来源文档已按当前硬件清理条目并对齐 18 项编号，供参考对照；具体操作以当前 Factory README 为准，文档本身不代表已完成硬件验收。
