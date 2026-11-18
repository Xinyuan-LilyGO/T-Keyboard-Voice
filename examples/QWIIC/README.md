# QWIIC

**English** | [简体中文](README_CN.md)

Scan the shared I2C bus on CN4 and send/receive data on CN3 UART0. USB CDC is the console and does not occupy UART0.

```bash
pio run -e qwiic -t upload
pio device monitor -b 115200
```

- CN4: SDA GPIO3 and SCL GPIO2, sharing `Wire` with the onboard devices.
- This demo enables ALDO2 so the scan can detect ES8389 `0x10`, AXP2101 `0x34`, and ES7210 `0x40`.
- CN3: TX GPIO43 and RX GPIO44, 115200 baud, 8N1, 3.3 V TTL.

Use `scan` to scan I2C and `send` to transmit a test message immediately through UART0. By default, UART0 transmits every 2 seconds. Received data is forwarded to USB with a `[UART0 RX]` prefix.

For I2C testing, connect a device whose address does not conflict with the three onboard addresses. Seeing only onboard addresses does not verify the external CN4 wiring. With no external device connected, the scan still runs normally; this demo does not apply Factory's external-device failure rule.

For UART testing, use a 3.3 V USB-to-TTL adapter with TX/RX crossed and a common ground, or short CN3 TX to RX for a loopback test. Do not connect multiple outputs that drive the same TX/RX line, and do not use RS-232 or 5 V TTL levels.

If no external I2C address appears, check power, pins, and address conflicts. For missing UART data, check voltage levels, baud rate, and crossed wiring. GPIO6/7 are not a second QWIIC I2C bus.
