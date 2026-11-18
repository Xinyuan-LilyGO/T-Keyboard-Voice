# Power

**English** | [简体中文](README_CN.md)

AXP2101, battery telemetry, and deep sleep demo. It does not initialize the displays, audio, or RGB LEDs.

## Run

```bash
pio run -e power -t upload
pio device monitor -b 115200
```

No external peripherals are required. Connect a test battery for complete power testing. The MCU is powered by DCDC1. I2C uses SDA GPIO3 and SCL GPIO2; KEY4 is GPIO12, and the PMIC IRQ is GPIO21.

## Controls and Expected Behavior

Every 2 seconds, the console reports battery presence, charge level, VBAT/VSYS/VBUS, charging status, and charge settings. Send newline-terminated commands:

| Command | Action |
| --- | --- |
| `battery` | Read battery telemetry immediately |
| `power` | Show KEY4/IRQ levels, power registers, and the wake-up reason |
| `scan` | Scan the shared I2C bus with the currently enabled power rails |
| `sleep` | Enter deep sleep when the power key has been released |

Holding KEY4 for 3 seconds and then releasing it also enters deep sleep. USB CDC disconnects during sleep. Press KEY4 or reconnect USB, then reopen the serial console to inspect the wake-up reason. An already connected USB cable does not generate another insertion event.

With no battery connected, `ABSENT` is expected and does not indicate a PMIC communication failure. Battery and VBUS current readings are `N/A`. This demo does not request the audio power rail, so an I2C scan may not show all three onboard device addresses. Use the QWIIC demo for a complete scan.

## Troubleshooting

If initialization fails, check address `0x34`, SDA/SCL, and the power supply. For wake-up problems, check the battery/USB conditions and GPIO12/21. Sleep current must be measured with suitable instruments. See the [power guide (Chinese)](../../docs/power_and_sleep.md).
