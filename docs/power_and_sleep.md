# Power and Sleep

[English](#english) | [简体中文](#简体中文)

## English

### Power Rails

| Output | Purpose | Firmware target voltage |
| --- | --- | --- |
| DCDC1 | MCU and always-powered domain | 3.3 V, retained during operation and sleep |
| ALDO2 | Audio | 3.3 V |
| ALDO3 | Keyboard/displays | 3.3 V |
| DLDO1 | WS2812 | 3.3 V |

`beginBoard()` establishes GPIO, USB CDC, the shared I2C bus, and the PMIC baseline. Peripherals request rails as needed. Keys requires ALDO3 but does not initialize the LCDs; Factory requests all required peripheral rails.

SensorLib initialization changes TS-related settings. The original protection sequence is retained: save TS/ADC configuration, initialize, then restore and verify it by readback. If restoration fails, charging is paused; the original enable state is restored afterward. Do not remove this protection path. Demos do not arbitrarily change charge voltage or fast-charge current settings. AXP2101 does not provide the battery/VBUS current measurements used here, so logs show N/A. ICC is a charge-current setting, not a live measurement.

### KEY4 and Wake-up

- KEY4/GPIO12 is directly connected to AXP2101 PWRON.
- Baseline startup retains the original PMIC key configuration: long-press PMIC power-off is disabled to avoid leaving a USB-only board unable to restart through the key.
- Power and Factory poll the software sleep gesture: hold for 3 seconds, wait for the prompt, then release to enter deep sleep.
- Other demos do not poll this gesture. KEY4 remains a normal input or unused, and their software does not enter sleep because of that gesture.
- Deep sleep retains DCDC1, the PMIC, and key/IRQ wake paths. It is not complete power-off.
- A low level on the GPIO12 key or the PMIC VBUS-insertion IRQ on GPIO21 wakes the MCU. USB insertion is an event; an already connected cable does not generate another insertion event.

### Shutdown Preparation and Recovery

Factory registers peripheral cleanup, failed-sleep recovery, and backlight indication through `SleepHooks`. The power module does not depend on Factory UI or test numbers.

Before sleeping, the firmware disables the PA, stops audio/displays/RMT/LEDC/UART, places peripheral GPIOs at safe levels, disables switchable rails and related ADCs, and configures pin holds and wake sources. USB CDC disconnects during sleep; reconnect the serial monitor after wake-up.

The original policy is retained: some rail-disable failures are logged while sleep is still attempted. If wake pins or GPIO holds cannot be configured, the firmware releases holds and restores power/application state rather than repeatedly rebooting. A failure log must not be treated as a successful low-power acceptance test.

### Required Hardware Verification

Check KEY4 hold/release, KEY4 wake, USB-insertion wake, startup-key suppression, failed-sleep recovery, and sleep current under USB-only, battery-connected, and no-VBUS conditions. VSYS supplies the speaker amplifier, so PA_EN must remain low; hardware should provide a PA_EN pull-down. QWIIC may remain powered from an always-powered domain.

---

## 简体中文

**供电与休眠**

### 电源轨

| 输出 | 用途 | 固件目标电压 |
| --- | --- | --- |
| DCDC1 | MCU 及常供电域 | 3.3 V，运行/休眠都保留 |
| ALDO2 | 音频 | 3.3 V |
| ALDO3 | 键盘/显示 | 3.3 V |
| DLDO1 | WS2812 | 3.3 V |

`beginBoard()` 建立 GPIO、USB CDC、共享 I2C 与 PMIC 基础状态。外设按需请求电源轨；Keys 需要 ALDO3，但不会初始化 LCD。Factory 请求全部必要电源轨。

SensorLib 初始化会改变 TS 相关设置，因此沿用原实现：先保存 TS/ADC 配置，初始化后恢复并读回。恢复失败时暂停充电，后续恢复原使能状态；不得删除这条保护路径。充电电压和快充电流设置不因演示而随意改写。AXP2101 不提供电池/VBUS 电流测量，日志显示 N/A；ICC 是充电设置值，不是实时电流。

### KEY4 与唤醒

- KEY4/GPIO12 直接连接 AXP2101 PWRON。
- 基础启动保留原 PMIC 电源键配置：关闭 PMIC 长按关机行为，避免 USB-only 情况下无法通过按键重新启动。
- Power、Factory 轮询软件休眠手势：长按 3 秒，收到提示后释放，才执行 deep sleep。
- 其他 Demo 不轮询该手势，KEY4 只作为普通输入或闲置，不会因 Demo 的软件逻辑进入休眠。
- deep sleep 保留 DCDC1、PMIC 和按键/IRQ 唤醒通路，不是真正断电。
- GPIO12 按键低电平或 GPIO21 的 PMIC VBUS 插入 IRQ 唤醒 MCU；仅插入 USB 有唤醒含义，持续插着 USB 并不产生新的插入事件。

### 收尾与恢复

Factory 通过 `SleepHooks` 注册外设收尾、失败恢复和背光提示，供电模块不包含 Factory 的 UI 或测试编号。

休眠前先关闭 PA，停止音频/显示/RMT/LEDC/UART，再将外设 GPIO 置为安全电平，关闭可切电源轨和相关 ADC，配置 GPIO 保持与唤醒源。休眠会使 USB CDC 断开，唤醒后需要重新连接串口监视器。

保留原固件策略：部分电源轨关闭失败会记录日志并继续尝试休眠；唤醒脚或 GPIO 保持无法配置时，撤销保持、恢复电源与应用，而不是反复重启。日志失败不能当成低功耗验收通过。

### 必须实机验证

分别在 USB-only、带电池以及无 VBUS 的条件下检查：KEY4 长按/释放、KEY4 唤醒、USB 插入唤醒、启动按键屏蔽、失败恢复和休眠电流。VSYS 给功放供电，PA_EN 必须保持低；硬件应提供 PA_EN 下拉。QWIIC 可能随常供电域保持供电。
