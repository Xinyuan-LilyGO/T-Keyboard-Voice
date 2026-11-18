# AudioRecord

[English](README.md) | **简体中文**

ES7210 板载/外接采集、AEC 参考读取，以及录音后扬声器回放。

```bash
pio run -e audio_record -t upload
pio device monitor -b 115200
```

启用 ALDO2，需要正常 PSRAM。ES7210 地址 0x40，DIN15；MIC1/2 为板载，MIC3 为 CN5 外接输入，MIC4 为 AEC 参考。回放还需要 ES8389 与扬声器，PA_EN 为 GPIO45。

| 命令 | 操作 |
| --- | --- |
| `onboard` | 板载双麦，默认模式 |
| `external` | 四通道 packed TDM-I2S，包含 MIC3/MIC4 |
| `loop onboard` | 板载双麦录制 1 秒后混音，经双扬声器播放 |
| `loop aux` | 外接 MIC3 录制 1 秒后经双扬声器播放 |
| `mic` | ES7210 寄存器读回 |
| `stop` | 停止采集并关闭 PA |

每 500 ms 打印一个短采样块的四路 RMS、原始 peak、DC 和 CN5 检测脚；不是持续半秒平均。`onboard` 模式下 MIC3/4 填零。回放后自动静音并恢复之前的采集模式。

对板载麦说话、向 CN5 输入已知信号，观察对应通道变化；录音回放需要听到实际录制内容并检查两只扬声器。不要将空接 ADC 噪声当作有效声音。`external` 仅采集时 MIC4 可能安静；它是参考输入，不是回声消除算法。

CN5 检测脚仅作提示，某些 TRRS 插头不会改变它。初始化/超时失败会打印 FAILED，可用 `onboard` 或 `external` 重试。检查 ALDO2、0x40、I2S 时钟、PSRAM 和插头类型。录放切换、线序和增益见 [音频说明](../../docs/audio.md)。
