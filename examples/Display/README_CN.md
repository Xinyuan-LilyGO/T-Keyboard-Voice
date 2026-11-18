# Display

[English](README.md) | **简体中文**

五块 GC9107 的编号、颜色与共用背光演示，不初始化按键、音频或 RGB。

```bash
pio run -e display -t upload
pio device monitor -b 115200
```

无需外接设备。ALDO3 为键盘/显示供电。SPI CLK41、MOSI40、DC39、RST46；CS 为 48/38/7/42/11，背光 GPIO1 低有效。

上电后五屏分别显示编号 1～5、颜色和 CS 引脚。串口命令：

- `numbers`：编号及引脚。
- `colors`：五屏纯色。
- `off` / `on`：关闭/打开共用背光。
- `dim` / `bright`：低亮度/全亮度。
- `fade`：从暗到亮渐变。

人工检查屏序、颜色、字符是否正确，背光是否同步变化。单屏异常检查 CS 和排线；全部异常检查 ALDO3、共享 SPI 和复位。不要独立复位某块屏而影响其他四块，也不要占用 PSRAM 的 GPIO35～37。
