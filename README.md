# T-Keyboard Voice V0.3

**English** | [简体中文](README_CN.md)

Board support for ESP32-S3 N16R8, with **7 standalone peripheral demos** and **1 Factory firmware containing all 18 board tests**.

## Build Options

| Build system | Details |
| --- | --- |
| PlatformIO | Quick start below; 8 independent build environments |
| [Arduino IDE / CLI](docs/arduino_ide.md) | Arduino-ESP32 3.3.11; the same library and examples |
| [ESP-IDF](docs/esp_idf.md) | ESP-IDF 5.5.6 with the official Arduino 3.3.11 component; select examples through CMake |

All three build systems share the same drivers, examples, and test audio. The ESP-IDF project uses the Arduino component; it is not a pure ESP-IDF driver port. The linked detailed guides are currently in Chinese.

## Quick Start

Choose one of the three options below. Run commands from the **repository root**. The first build requires internet access to download tools or pinned dependencies. The target is ESP32-S3 N16R8; all builds use the **115200-baud USB CDC console** with newline-terminated commands.

### 1. PlatformIO (Recommended)

```bash
pio run -e factory
pio run -e factory -t upload
pio device monitor -b 115200
```

These commands build Factory, upload the firmware, and open the serial monitor. To select another example, replace the environment name; for example, `pio run -e display -t upload`. No source or `src_dir` changes are needed. Use `--upload-port COMx` to specify an upload port and `-p COMx` for the monitor.

In VS Code, you can also press **Ctrl+Shift+B** and choose an environment to use the project's native build task. Project-local status-bar buttons use the same tasks.

### 2. Arduino IDE (Not Recommended)

1. Install Arduino IDE 2.x and add this URL under **File → Preferences → Additional Boards Manager URLs**:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
2. Under **Tools → Board → Boards Manager**, install Espressif's **esp32 3.3.11** package.
3. Run these commands from the repository root to prepare dependencies and generate library ZIPs:

```bash
python scripts/prepare_dependencies.py
python scripts/package_arduino.py
```

4. Use **Sketch → Include Library → Add .ZIP Library** to install **SensorLib.zip, LovyanGFX.zip, ButtonSense.zip, and TKeyboardVoice.zip**, in that order, from `.build/arduino/zips/`. Avoid keeping conflicting older copies of the libraries.
5. Open `examples/Factory/Factory.ino`; keep `Factory.cpp` and `build_opt.h` in the same directory. For another example, open its matching `.ino`.
6. Select the actual USB port and configure the board menus:

| Menu | Setting |
| --- | --- |
| Board | ESP32S3 Dev Module |
| CPU Frequency | 240 MHz |
| USB Mode | Hardware CDC and JTAG |
| USB CDC On Boot | Enabled |
| Flash Mode | QIO 80MHz |
| Flash Size | 16MB (128Mb) |
| Partition Scheme | 16M Flash (3MB APP/9.9MB FATFS) |
| PSRAM | OPI PSRAM |

7. Click **Verify** to compile, then **Upload** to flash. Open Serial Monitor at **115200 baud**, with LF or CRLF line endings.

Test audio is built in; no separate filesystem upload is required. See the [Arduino guide (Chinese)](docs/arduino_ide.md) for full installation, download-mode recovery, and Arduino CLI batch build instructions.

8. Alternatively, use **Arduino CLI**, which is usually bundled with recent Arduino IDE releases, and give an AI agent the following request: “Please configure the project environment according to the [Arduino guide (Chinese)](docs/arduino_ide.md). If network issues occur in mainland China, please enable a VPN.”

### 3. ESP-IDF

Install and export an **ESP-IDF 5.5.6** environment or a later compatible version.

**Version constraint:** The current project manifest accepts ESP-IDF **5.5.x** (`>=5.5,<5.6`). Versions outside this range require configuration changes and separate validation; updating this README does not enable them.

Prepare the shared dependencies and build Factory:

```bash
python scripts/prepare_dependencies.py
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory build
```

Connect the board, then specify its actual port to build, flash, and open the monitor:

```bash
idf.py -C idf -D IDF_TARGET=esp32s3 -D VOICE_EXAMPLE=Factory -p COMx build flash monitor
```

Replace `COMx` with the actual port, using the appropriate device path on Linux/macOS. Exit the monitor with `Ctrl+]`. To switch examples, explicitly change `VOICE_EXAMPLE`, such as to `AudioRecord`, rather than accidentally flashing a previously selected demo. Component Manager downloads the official components; Flash/PSRAM defaults are in `idf/sdkconfig.defaults`. See the [ESP-IDF guide (Chinese)](docs/esp_idf.md).

**Important:** Partition layouts differ between the three build systems. Use the corresponding tool's complete flashing process when switching; do not mix bootloaders, partition tables, and application images.

## Examples

| Example | PlatformIO environment | Features |
| --- | --- | --- |
| [Power](examples/Power/README.md) | `power` | AXP2101, battery telemetry, sleep, and wake-up |
| [Display](examples/Display/README.md) | `display` | Five displays and backlight control |
| [Keys](examples/Keys/README.md) | `keys` | Events from all five keys |
| [WS2812](examples/WS2812/README.md) | `ws2812` | 14 RGB LEDs |
| [AudioPlayback](examples/AudioPlayback/README.md) | `audio_playback` | ES8389, speakers, headphones, and power amplifier control |
| [AudioRecord](examples/AudioRecord/README.md) | `audio_record` | ES7210, onboard/external microphones, and record-then-playback |
| [QWIIC](examples/QWIIC/README.md) | `qwiic` | CN4 I2C and CN3 UART |
| [Factory](examples/Factory/README.md) | `factory` | All 18 board tests |

## Repository Layout

```text
src/         Shared drivers; no application entry point
examples/    One standalone example per directory
boards/      V0.3 ESP32-S3 N16R8 board definition
idf/         ESP-IDF CMake project and Arduino component adapters
assets/      Original test audio
scripts/     Example selection, batch builds, and host tests
tests/       Audio unpacking and power-key regression tests
docs/        Pinout, hardware references, factory testing, and validation records
```

The standalone demos and Factory reuse the same drivers. Factory does not include the demos' `.ino` files. Each demo initializes only the peripherals it needs; the PMIC module enables the required shared power rails. Each example's `.ino` is its Arduino sketch entry, while Factory keeps its main application logic in `Factory.cpp` in the same directory.

## Hardware and Build Requirements

- ESP32-S3-WROOM-1 N16R8: 16 MB Flash and 8 MB Octal PSRAM, using `qio_opi`.
- Use the pinned Arduino-ESP32 3.x platform. Do not replace it with an Arduino 2.x-based platform.
- USB CDC is the console; CN3 UART0 is used separately for the interface demo.
- The KEY4 long-press sleep gesture is enabled only in Power and Factory. Other demos do not turn a KEY4 long press into a software sleep request.
- Check the wiring and keep speakers/headphones away from your ears during the first playback test. Capture and playback reconfigure the same I2S resource.
- See the [Arduino guide](docs/arduino_ide.md) for board selection, dependencies, and IDE menu settings, and the [ESP-IDF guide](docs/esp_idf.md) for SDK versions and partition settings. Test audio is included as a read-only C++ resource; no separate filesystem upload is needed.

## Validation

```bash
python scripts/build_examples.py
python scripts/test_host.py
python scripts/check_repository.py
python scripts/generate_audio.py --check
```

For Arduino CLI and ESP-IDF batch builds, use `scripts/build_arduino.py` and `scripts/build_idf.py`, respectively. Prepare the toolchain and pinned dependencies as described in the corresponding guide first.

Host tests require a native C++ compiler, which can be selected through `CXX`. Batch build scripts do not flash the board. See [validation.md](docs/validation.md) for verified build results and the remaining hardware acceptance checklist.

- [Pinout (Chinese)](docs/pinmap.md)
- [Power and sleep (Chinese)](docs/power_and_sleep.md)
- [Audio (Chinese)](docs/audio.md)
- [Factory test guide (Chinese)](docs/factory_test_guide_cn.md)
- [Migration map (Chinese)](docs/migration_map.md)
- [Dependencies and resource provenance (Chinese)](docs/third_party.md)

## Licensing

This repository retains the [MIT License](LICENSE) already established in the GitHub repository. Third-party dependencies and referenced audio, images, and hardware materials remain subject to their respective copyrights, licenses, and authorization requirements. See [dependencies and resource provenance (Chinese)](docs/third_party.md).
