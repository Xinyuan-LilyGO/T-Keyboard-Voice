# Power

[English](README.md) | **简体中文**

AXP2101、电池信息和 deep sleep 示例，不初始化 LCD、音频或 RGB。

## 运行

```bash
pio run -e power -t upload
pio device monitor -b 115200
```

无需外接设备；完整电源测试请接测试电池。MCU 使用 DCDC1，I2C 为 SDA3/SCL2，KEY4 为 GPIO12，PMIC IRQ 为 GPIO21。

## 操作与预期现象

串口每 2 秒输出电池存在、电量、VBAT/VSYS/VBUS、充电状态和充电设置。发送以换行结尾的命令：

| 命令 | 操作 |
| --- | --- |
| `battery` | 立即读取电池信息 |
| `power` | 查看 KEY4/IRQ、电源寄存器、唤醒原因 |
| `scan` | 扫描当前供电条件下的共享 I2C |
| `sleep` | 按键已释放时进入 deep sleep |

长按 KEY4 3 秒后释放也可休眠。USB CDC 随后断开，按 KEY4 或重新插入 USB 后重新连接串口并查看启动来源。已经插着的 USB 不会再次触发插入事件。

未接电池会报告 ABSENT，不代表 PMIC 通信失败。电池/VBUS 电流为 N/A。未请求音频电源轨，因此扫描不保证三个板载地址都出现；完整扫描用 QWIIC Demo。

## 排查

初始化失败先检查 0x34、SDA/SCL 和供电。唤醒失败检查电池/USB 条件及 GPIO12/21；休眠电流需要仪器测量。详情见 [供电说明](../../docs/power_and_sleep.md)。
