# Display

**English** | [简体中文](README_CN.md)

Numbering, color, and shared-backlight demo for the five GC9107 displays. It does not initialize the keys, audio, or RGB LEDs.

```bash
pio run -e display -t upload
pio device monitor -b 115200
```

No external peripherals are required. ALDO3 powers the keyboard/display rail. SPI uses CLK GPIO41, MOSI GPIO40, DC GPIO39, and shared RST GPIO46. The five CS pins are GPIO48/38/7/42/11. The shared backlight is on GPIO1 and is active-low.

After startup, the five displays show their numbers 1–5, colors, and CS pins. Serial commands:

- `numbers`: show display numbers and pins.
- `colors`: fill the five displays with solid colors.
- `off` / `on`: turn the shared backlight off/on.
- `dim` / `bright`: select low/full brightness.
- `fade`: fade the backlight from dark to bright.

Visually check the display order, colors, text, and synchronized backlight changes. If one display fails, check its CS pin and cable. If all displays fail, check ALDO3, the shared SPI bus, and reset. Do not independently reset one panel through the shared reset line, as this also affects the other four. Do not use GPIO35–37, which are reserved for PSRAM.
