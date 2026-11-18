# WS2812

[English](README.md) | **简体中文**

14 颗 RGB 灯颜色和追逐演示。

```bash
pio run -e ws2812 -t upload
pio device monitor -b 115200
```

数据 GPIO47，DLDO1 为灯带供电，使用 Arduino 3.x RMT，GRB 顺序。

默认逐颗白色追逐。命令：`red`、`green`、`blue`、`white`、`chase`、`off`。亮度使用较低固定值，避免演示时持续满功率点亮。

人工检查 14 颗灯都能点亮、RGB 顺序正确且追逐连续。初始化错误检查 DLDO1 和 RMT；后段灯不亮检查链路、灯珠焊接和供电。软件输出成功不替代逐颗观察。
