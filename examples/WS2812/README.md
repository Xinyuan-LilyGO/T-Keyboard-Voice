# WS2812

**English** | [简体中文](README_CN.md)

Color and chase demo for the 14 RGB LEDs.

```bash
pio run -e ws2812 -t upload
pio device monitor -b 115200
```

The data pin is GPIO47, and DLDO1 supplies the LED chain. The driver uses Arduino 3.x RMT with GRB color order.

The default effect is a single white LED chasing along the chain. Commands: `red`, `green`, `blue`, `white`, `chase`, and `off`. The demo uses a low, fixed brightness to avoid sustained full-power operation.

Visually check that all 14 LEDs light up, the RGB colors are correct, and the chase moves continuously. For initialization errors, check DLDO1 and RMT. If LEDs near the end do not light up, check the data chain, solder joints, and power supply. Successful software output does not replace inspecting every LED.
