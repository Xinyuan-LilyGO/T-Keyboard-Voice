# V0.3 引脚表

硬件目标：ESP32-S3-WROOM-1 N16R8，16 MB Flash、8 MB Octal PSRAM。唯一代码定义见 `src/pin_config.h`。

| 功能 | GPIO / 参数 |
| --- | --- |
| 板载 I2C / CN4 QWIIC | SDA 3、SCL 2，共用 Wire，400 kHz |
| AXP2101 / ES8389 / ES7210 地址 | 0x34 / 0x10 / 0x40 |
| PMIC IRQ | 21，低有效 |
| KEY1 / KEY2 / KEY3 / KEY4 / KEY5 | 10 / 14 / 13 / 12 / 0，低有效 |
| 电源键 | KEY4 / GPIO12，同时连接 AXP2101 PWRON |
| 五屏 CS1～CS5 | 48 / 38 / 7 / 42 / 11 |
| LCD CLK / MOSI / DC / 共用 RST | 41 / 40 / 39 / 46 |
| 五屏共用背光 | 1，低有效 PWM |
| 屏幕 | 5 × GC9107，128 × 128，共享 SPI，基线 4.5 MHz |
| WS2812 | 47，14 颗，GRB 顺序 |
| I2S MCLK / BCLK / WS | 16 / 17 / 18 |
| I2S MCU DOUT / DIN | 8 → ES8389 / 15 ← ES7210 SDOUT1 |
| ES7210 INT | 9 |
| PA_EN | 45，高有效 |
| CN5 AUX 检测 | 6，板上有 100 kΩ 下拉 |
| CN3 UART0 TX / RX | 43 / 44，115200，8N1，3.3 V TTL |

注意：

- GPIO35～37 属于 Octal PSRAM，不可用作 LCD CS。
- CN4 和板载器件共享总线；不得把 GPIO6/7 配置成第二套 QWIIC I2C。
- 外接 I2C 设备避免使用 0x10、0x34、0x40。
- KEY5/GPIO0 是启动绑带脚，复位时保持释放。
- UART 对测需共地并交叉连接 TX/RX，禁止接 RS-232 电平或 5 V TTL。
