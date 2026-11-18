# QWIIC

[English](README.md) | **简体中文**

CN4 共享 I2C 扫描与 CN3 UART0 收发。USB CDC 是控制台，不占用 UART0。

```bash
pio run -e qwiic -t upload
pio device monitor -b 115200
```

- CN4：SDA3、SCL2，与板载器件共用 Wire。
- 本示例启用 ALDO2，便于看到 ES8389 0x10、AXP2101 0x34、ES7210 0x40。
- CN3：TX43、RX44，115200、8N1、3.3 V TTL。

命令 `scan` 扫描 I2C，`send` 立即从 UART0 发送测试文本。UART0 默认每 2 秒发送一次；收到的数据带 `[UART0 RX]` 前缀转发到 USB。

I2C 测试接入一个不占用三个板载地址的设备；扫描中只有板载地址不代表 CN4 的外接接线已验证。无外设时扫描仍正常演示，不按 Factory 的外接设备测试规则报失败。

UART 测试可使用 3.3 V USB-TTL 对测，TX/RX 交叉并共地；或仅短接 CN3 TX/RX 做回环。不要同时连接相互驱动的多个 TX，不要接 RS-232/5 V TTL。

无外接地址检查供电、引脚、地址冲突；UART 无数据检查对接电平、波特率和交叉接线。GPIO6/7 不是另一套 QWIIC I2C。
