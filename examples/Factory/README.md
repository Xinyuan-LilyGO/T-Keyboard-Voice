# Factory

**English** | [简体中文](README_CN.md)

Complete board test firmware for T-Keyboard Voice V0.3, with **18 tests, a five-display interface, key navigation, and USB serial control**. `Factory.ino` is the sketch entry, `Factory.cpp` contains the application, and the hardware drivers are shared from the repository's `src/`.

> The expected behavior below is a checklist derived from the current code, not a record of completed hardware acceptance testing. An automatic `PASS` only means that the test's software conditions were met. Sound, displays, interfaces, and low-power operation still require observation or measurement.

## 1. Equipment and Safety

- A T-Keyboard Voice V0.3 board (ESP32-S3 N16R8), a USB data cable, and a computer.
- A compatible test battery with the correct polarity. Some tests work without a battery, but test 2 reports `FAIL / Battery not connected`.
- Two correctly connected speakers and headphones. Keep them away from your ears during the first playback test.
- A compatible external microphone/audio source and matching plug for CN5. Follow the hardware input specifications; do not connect signals of unknown voltage levels.
- A 3.3 V logic I2C peripheral whose address does not conflict with onboard `0x10`, `0x34`, or `0x40`.
- A 3.3 V USB-to-TTL adapter for UART testing, or a jumper for CN3 TX/RX loopback.

**Disconnect USB and the battery before changing wiring; deep sleep is not power-off.** Do not connect CN3 to RS-232 or 5 V TTL, or directly connect two driven outputs. See the [pinout (Chinese)](../../docs/pinmap.md).

## 2. Build, Upload, and Connect

Run the following commands from the **repository root**.

### PlatformIO

```bash
pio run -e factory
pio run -e factory -t upload
pio device monitor -b 115200
```

To select a port, append `--upload-port COMx` when uploading and `-p COMx` for the monitor. Replace `COMx` with the actual device port.

In VS Code, the project's **Ctrl+Shift+B** task lets you select `factory` without using the extension's cached task list. See [VS Code troubleshooting (Chinese)](../../docs/vscode_troubleshooting.md) for the `reading 'id'` exception and project-local status-bar buttons.

### Arduino IDE

Install the official **esp32 3.3.11** board package and required libraries. Select **ESP32S3 Dev Module, 16 MB Flash, OPI PSRAM, Hardware CDC and JTAG, and USB CDC On Boot Enabled**. Open `Factory.ino`; keep `Factory.cpp` and `build_opt.h` in the same directory. See the [Arduino guide (Chinese)](../../docs/arduino_ide.md) for the complete partition and menu settings.

### ESP-IDF

Use an exported **ESP-IDF 5.5.6** environment. This build uses the official Arduino component; it is not a pure ESP-IDF driver port.

```bash
python scripts/prepare_dependencies.py
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory -p COMx build flash monitor
```

Replace `COMx` with the actual port. Exit the ESP-IDF monitor with `Ctrl+]`. Explicitly select Factory rather than accidentally flashing a previously selected demo. See the [ESP-IDF guide (Chinese)](../../docs/esp_idf.md).

All three builds use the **115200-baud USB CDC console**. Commands must end with a newline; select LF or CRLF. CN3 UART0 is not the console. Reselect the port if USB re-enumerates. Partition layouts differ between build systems; do not mix an old bootloader/partition table with a new application.

During normal reset and startup, release KEY5 so GPIO0 does not select download mode.

## 3. Expected Startup Behavior

1. The console prints `T-Keyboard Voice V0.3 board support`, `[POWER] boot=...`, and `[MEMORY] PSRAM: ... bytes`, followed by `T-Keyboard Voice V0.3 factory hardware test`, help, and an I2C scan. N16R8 should report approximately 8 MB of PSRAM, not zero.
2. **Only test 1 runs automatically at startup, not all 18 tests.** Successful PMIC and required rail initialization gives test 1 a `PASS`. Other tests remain `WAIT` until executed; this is normal.
3. On a regular test page, displays 1/5 show KEY1/KEY5 navigation arrows, display 2 shows the test number/name, display 3 shows the result, and display 4 shows the guide and a short detail message. Battery, microphone, and display tests use dedicated views.
4. All five displays and the backlight should work without persistent black screens, obvious corruption, or repeated unexpected resets. On-screen details are truncated; read the serial log for the complete explanation.

For an initial check, send these commands one line at a time through USB:

```text
help
scan
power
status
```

Factory requests all peripheral power rails. With healthy onboard devices, the scan should show ES8389 `0x10`, AXP2101 `0x34`, and ES7210 `0x40`. Address acknowledgments do not establish that the complete audio, power, or interface functions work.

## 4. Controls and Result Recording

### Keys

| Key | Action during regular tests |
| --- | --- |
| KEY1 / KEY5 | Previous/next test, wrapping at either end |
| KEY2 | Rerun the current test; in test 7, toggle the PA and play audio |
| KEY3 click | Manually mark the current test `PASS` |
| KEY4 short press | Manually mark the current test `FAIL` |
| Hold KEY4 for 3 seconds, then release | Request and enter deep sleep, not complete power-off |

**Navigating to a test immediately executes it; it does not merely select a test to run later.** In test 12, KEY2–KEY4 are test inputs rather than rerun/PASS/FAIL controls. KEY1/KEY5 navigation remains active, and holding KEY4 long enough can still request sleep.

### Serial Commands

| Command | Action |
| --- | --- |
| `help` / `?` | Print help |
| `run N` | Immediately execute test N, from 1 to 18; for example, `run 8` |
| `next` / `prev` | Execute the adjacent test |
| `pass` / `fail` | Record an operator decision for the current test |
| `status` | List all 18 results and details for the current boot |
| `scan` | Scan the shared I2C bus; this does not replace running test 14 |
| `power` | Show power-key, PMIC register, and boot/wake diagnostics |
| `mic` | Show ADC registers and current live capture diagnostics; enter test 8 or 16 first |
| `run` | Execute all tests sequentially; **it does not stop at each test for manual confirmation** |

Playback, recording, LED effects, and display sequences include short blocking operations, so regular key/serial responses may be delayed. Wait for the sequence to finish before operating the controls again. Factory has no separate serial `sleep` command.

### Result States

| State | Meaning and action |
| --- | --- |
| `WAIT` | Not tested; neither a failure nor a pass |
| `RUN` | In progress |
| `PASS` | Software conditions met or manually confirmed; interpret it within that test's limits |
| `FAIL` | Software check failed or the operator rejected the result; inspect the detail |
| `CHECK` | Initialization/data flow completed but listening, observation, or measurement is still required; not an automatic fault |

Regular result pages use dark green for `PASS`, red for `FAIL`, orange for `CHECK`, and blue for `WAIT`/`RUN`. A manual `pass` can overwrite an automatic failure and must not be used to hide faults. Automatically refreshed tests, such as battery telemetry, may update the result again.

**Recommended workflow:** send `run N`, observe or measure, send `pass` only after confirming correct behavior (otherwise `fail`), then review `status`. Do not blindly send a sequence of `pass` commands. Finish and record ordinary functional tests before performing test 3's sleep/wake checks.

## 5. All 18 Tests: Actions and Correct Behavior

| Number / test | Action and expected behavior | Current firmware decision |
| --- | --- | --- |
| 1 · AXP2101 / `run 1` | Address `0x34` responds, PMIC initialization and required rail readbacks succeed, and the display interface is established | Automatic `PASS` / `FAIL`; does not replace voltage/current measurements |
| 2 · Battery telemetry / `run 2` | Connect the battery; connection, charge level, VBAT/VSYS, VBUS, and charge status should match the actual supply conditions as USB is connected/disconnected | Automatic; no battery gives `FAIL`, which is not synonymous with PMIC communication failure |
| 3 · Power key/sleep / `run 3` | Hold KEY4 for 3 seconds, then release; displays/backlight and RGB turn off and USB CDC disconnects. Restart using KEY4 or a valid USB-insertion wake event | `CHECK`; requires operator and instrument checks described below |
| 4 · ES8389 / `run 4` | The log reports the codec at `0x10` | Address acknowledgment automatically gives `PASS`; speaker/headphone operation is not verified |
| 5 · ES7210 / `run 5` | The log reports the ADC at `0x40` | Address acknowledgment automatically gives `PASS`; microphone capture is not verified |
| 6 · Speakers / `run 6` | The built-in clip plays immediately; both speakers should produce recognizable audio without obvious distortion, noise, or a missing output | `CHECK` after data is sent; listening is required |
| 7 · PA / `run 7` | Starts at `PA: OFF`. Click KEY2 to turn it ON and play; click again to turn it OFF and play. Compare audible/silent speaker output | `CHECK`; compare speakers only, because PA does not mute headphones |
| 8 · Onboard microphones / `run 8` | Displays 3/4 show `MIC1 BOARD` / `MIC2 BOARD`. Speech should affect both waveforms and RMS; inspect `mic` diagnostics | `CHECK`; no fixed RMS threshold automatically certifies sensitivity |
| 9 · Headphones / `run 9` | With PA off, hear the test audio through both headphone channels | `CHECK`; manually check headphones, not speakers |
| 10 · Five displays / `run 10` | Displays show red/green/blue/white/yellow and numbers 1–5 in the configured order, then return to the test UI after about 1.8 seconds; no missing panels, repeated numbers, or obvious corruption | `CHECK`; rerun if the sequence was missed |
| 11 · Backlight / `run 11` | All displays have a white background; the shared backlight fades up, turns off, then reaches full brightness. Brightness changes should be synchronized | `CHECK`; shared active-low GPIO1 |
| 12 · Five keys / `run 12` | Each key produces press/release logs; the `seen mask` reaches `0x1F` after all five have been detected | Automatic `PASS` at the complete mask; the mask accumulates during the current boot |
| 13 · WS2812 / `run 13` | All 14 LEDs complete solid RGBW, a single-LED chase, and position patterns without missing LEDs, wrong colors, or a broken chain | `CHECK`; inspect each LED |
| 14 · CN4 I2C / `run 14` | Connect an external peripheral and find an address other than `0x10/0x34/0x40`; also verify that onboard devices remain present | Automatic `PASS` only when an external address is found; `FAIL` without an external device is expected under the current rule |
| 15 · CN3 UART / `run 15` | The peer receives `T-KeyboardVoice QWIIC UART0 test`; data sent back appears on USB with `[QWIIC UART0 RX]` | `CHECK`; a sent log alone proves neither peer reception nor RX operation |
| 16 · External mic/AEC reference / `run 16` | Displays 3/4 show `MIC3 AUX` / `MIC4 AEC`. Feed known audio into CN5 and check MIC3 response; assess MIC4 against an actual reference signal | `CHECK`; playback is muted, so a quiet MIC4 alone does not establish a fault |
| 17 · MIC3 record/playback / `run 17` | Selecting the test starts approximately 1 second of recording, then both speakers play the audio just supplied to CN5 | `CHECK`; confirm the actual recording, not merely audible noise |
| 18 · Onboard pair record/playback / `run 18` | Speak into the onboard microphones for approximately 1 second immediately after selecting the test, then hear the mixed recording through both speakers | `CHECK`; MIC1/MIC2 are not assigned exclusively to separate speakers |

The actual test resource is `assets/66_stereo_48k.wav`, 48 kHz, 16-bit stereo. Some retained UI prompts call it `66.wav`; they refer to this built-in clip. No separate audio file upload is required.

## 6. Checks That Are Easy to Misinterpret

### Battery, Current, and Charging

Test 2 refreshes about once per second. Current software checks include a 0–100% charge level, VBAT of 2500–5000 mV, VSYS of 2500–5500 mV, valid ADC reads, charge status/settings, and PMIC faults. These are coarse software bounds, **not battery or charger specification acceptance limits**. With no battery, `Battery not connected` and `FAIL` are expected; this does not constitute a complete power-test pass.

`IBAT/IBUS: N/A` is an expected capability limit: the AXP2101 does not provide the real-time current measurements required here. `ICC` is a configured charge-current value, not measured current. Actual charging also depends on battery, VBUS, and charge phase.

### Five Keys

In test 12, first short-press KEY2, KEY3, and KEY4. Press KEY1 to move to test 11, wait for its backlight sequence to finish, then press KEY5 to return to test 12. Once all five keys have been seen during the same boot, expect `0x1F` and `PASS`. Earlier key presses may already have set some bits. Do not hold KEY4 long enough to request sleep.


### Microphones, AEC, and Recording

- Run `run 8` or `run 16`, wait for live capture, then send `mic`. `No live samples` during another test does not by itself indicate a damaged ADC.
- Compare onboard microphones using a steady source at equal distance and repeated readings. Waveforms are automatically scaled; short-window RMS or a single waveform height is not a production sensitivity metric.
- MIC3 is the external input; MIC4 is the AEC reference. Reading a reference does not implement echo cancellation. Playback is muted during test 16, so the reference may have no useful excitation and need not resemble MIC3.
- CN5/GPIO6 insertion detection is only a hint. Some TRRS plugs do not change it; `AUX UNPLUGGED` alone is not enough to reject external capture.
- Tests 17/18 **record for 1 second and then play back**; they are not real-time monitoring. Recording setup starts when the test is selected. To return to live waveforms afterward, run `run 8` or `run 16` again.
- Investigate `capture timeout`, `recording buffer allocation failed`, `READ FAILED`, or sustained clipping by checking clocks, power, PSRAM, wiring, and input levels. Do not simply override these failures with a manual pass.

### Sleep and Wake-up

1. Save serial `status` and observation records, then select `run 3`.
2. Hold KEY4 for at least 3 seconds and release it after the prompt. Holding without releasing does not complete the sleep gesture.
3. Check that display/backlight, RGB, and audio stop and USB CDC disconnects. Measure low-power operation with instruments; a black screen is not proof of acceptable sleep current.
4. Wake with KEY4, holding it until startup begins if necessary, then release it. Reconnect the USB console and send `power`; check the corresponding KEY4 wake source.
5. For USB-insertion wake testing, use a battery to maintain sleep with VBUS initially absent, then insert USB and check `USB/VBUS wake`. A cable that stays connected generates no new insertion event. **Removing USB from a USB-only board and reconnecting it is a power cycle, not a battery-backed USB-insertion wake test.**

Deep sleep retains DCDC1; it is not PMIC power-off. Wake-up restarts the application at test 1. Results are stored in RAM and do not survive a reset/deep-sleep wake. Do not mistake the new boot's test 1 PASS for a test 3 pass. To record test 3 again, select `run 3` and manually confirm it against the actual wake observations. See the [power/sleep guide (Chinese)](../../docs/power_and_sleep.md) for supply conditions and failed-sleep recovery.

## 7. Troubleshooting and Final Records

| Symptom | Check first |
| --- | --- |
| No USB serial output or commands do not execute | Data cable, runtime port, 115200 baud, newline setting, and USB CDC options; do not connect to CN3 instead |
| Zero PSRAM or recording-buffer allocation failure | N16R8 board and OPI PSRAM settings; do not treat recording results as valid until resolved |
| Test 1 fails or all displays stay blank | PMIC `0x34`, power and rail readbacks, then ALDO3, shared SPI/reset, and each CS; do not use GPIO35–37 reserved for PSRAM |
| Tests 4/5 pass but audio/capture fails | I2C acknowledgment is only a prerequisite; check audio power, I2S clocks, PA, and speaker/headphone/microphone wiring |
| Test 14 fails | Confirm an external device with a non-onboard address is connected, then check power, SDA/SCL, and address conflicts |
| No UART return data in test 15 | CN3 TX GPIO43 / RX GPIO44, 3.3 V TTL, 115200 8N1, common ground, and crossed TX/RX. A local TX/RX jumper can test loopback; do not join multiple driven outputs |
| Sleep is aborted or wake-up fails | Inspect reasons such as `[POWER] deep-sleep aborted`, KEY4 release, GPIO12/21, PMIC IRQ, and supply conditions; USB reconnection alone is insufficient evidence |

Finish with `status` and review all 18 tests. Any `WAIT` or `CHECK` means the complete set has not been confirmed. Even all-`PASS` results must be reviewed against original logs, observations, and measurements because manual overrides can hide faults. The firmware does not automatically export or persist an acceptance report. Record the board/hardware revision, firmware/build system, date, operator, observations, and measured voltages/currents.

Further references: [Factory test guide (Chinese)](../../docs/factory_test_guide_cn.md), [audio guide (Chinese)](../../docs/audio.md), and [software validation and hardware limits (Chinese)](../../docs/validation.md).
