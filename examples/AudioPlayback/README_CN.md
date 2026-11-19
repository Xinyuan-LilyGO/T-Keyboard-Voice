# AudioPlayback

[English](README.md) | **简体中文**

ES8389、扬声器、耳机及 PA 控制演示。需要正确连接扬声器/耳机，首次播放请勿贴近耳朵。

```bash
pio run -e audio_playback -t upload
pio device monitor -b 115200
```

启用 ALDO2。I2C SDA3/SCL2，ES8389 0x10；MCLK16、BCLK17、WS18、DOUT8，PA_EN45。保留原固件 ES7210 停采配置以避免共享时钟下的干扰。

初始化后 PA 默认关闭，等待命令：

| 命令 | 操作 |
| --- | --- |
| `speaker` | 开启 PA，播放测试 WAV |
| `headphone` | 关闭 PA，播放测试 WAV，人工检查耳机 |
| `play` | 按当前 PA 状态播放 |
| `pa on` / `pa off` | 控制扬声器功放 |
| `stop` | 静音并重置播放链路 |

WAV 为 48 kHz、16-bit 双声道。播放是阻塞操作，约持续一个短片段；命令在播放结束后处理。日志 `sent` 只说明数据写入成功，左右扬声器/耳机实际声音必须人工确认。

无声时检查 0x10、ALDO2、时钟、PA 和输出接线。PA 关闭仅控制扬声器功放，不代表耳机静音。寄存器、增益及格式说明见 [音频说明](../../docs/audio.md#简体中文)。
