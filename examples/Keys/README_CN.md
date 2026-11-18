# Keys

[English](README.md) | **简体中文**

独立读取五个按键，不带 Factory 的导航和 PASS/FAIL 行为。

```bash
pio run -e keys -t upload
pio device monitor -b 115200
```

KEY1～5 为 GPIO10/14/13/12/0，低有效；启用 ALDO3，但不初始化显示。

依次短按和长按每个键，串口应打印按下、释放/单击和长按事件，并更新 `seen` 位掩码。五键都识别后掩码为 0x1F。KEY4 在此 Demo 中不触发软件休眠。

KEY5/GPIO0 是启动绑带脚，复位时请释放。输入没有变化时先核对引脚、排线和 ALDO3。
