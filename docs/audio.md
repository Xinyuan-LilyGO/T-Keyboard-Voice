# 音频链路

## 芯片与采样参数

- ES8389：播放 Codec，I2C 地址 0x10。
- ES7210：四路 ADC，I2C 地址 0x40，MCU 只接 SDOUT1。
- 48 kHz、16-bit 样本；播放为双声道标准 I2S，MCLK 为 12.288 MHz（256fs）。
- 测试 WAV 为 `assets/66_stereo_48k.wav`，经 `scripts/generate_audio.py` 转为 `src/audio/test_audio.inc`，由 `VoiceTestAudio.cpp` 提供统一只读资源。三种构建入口不再依赖 linker 嵌入符号；只有 AudioPlayback 和 Factory 使用这份测试音频，未引用时由链接器剔除。
- 原始 `assets/original/66.wav` 仅归档，不参与默认构建。

| 通道 | 物理用途 |
| --- | --- |
| MIC1 | 板载麦克风 1 |
| MIC2 | 板载麦克风 2 |
| MIC3 | CN5 外接输入 |
| MIC4 | 左输出的 AEC 参考 |

MIC4 是参考信号读取，不表示固件实现了声学回声消除。没有播放时 MIC4 安静是正常现象。

## 三种显式模式

`initializeAudio(AudioMode::Playback)`、`AudioMode::Onboard`、`AudioMode::FourChannel` 分别表示播放、板载双麦、四通道采集。公共驱动不读取 Factory 的测试编号。

板载双麦使用 16-bit stereo 标准 I2S。四通道采集保留 ES7210 的 packed TDM-I2S：

- 左 32-bit 字的高/低 16 位：MIC1 / MIC3。
- 右 32-bit 字的高/低 16 位：MIC2 / MIC4。
- ESP32 仍使用标准 I2S 32-bit stereo 接收，不能替换成脉冲帧 `I2S_MODE_TDM`。

`readMicrophoneFrames()` 输出 MIC1～MIC4 四通道交错样本；双麦模式下 MIC3/MIC4 为零，不能据此判断外接通道故障。每次最多读取 128 帧。

## 增益与切换

MIC1/2 保留相同的 30 dB 模拟增益、0 dB 数字增益、ALC 关闭，并读回关键寄存器；MIC3 为 30 dB、MIC4 参考为 0 dB。比较麦克风灵敏度时，使用等距稳定声源，不能用单次短时语音 RMS 作结论。

采集时关闭 PA 并让 DAC 数据保持静音；录音/播放共享时钟与 I2S 资源，切换会先停止原模式。PA 只控制扬声器功放，不能代替耳机链路检查。

CN5 GPIO6 预期空闲 HIGH、插入 LOW，但它仅作提示：某些 TRRS 插头不改变该引脚，不能因此禁止 MIC3 采集。

## 录音回放

AudioRecord 和 Factory 支持两条链路：

1. MIC3 录制 1 秒 → 去直流、高通、噪声门、受限 2 倍增益 → 双扬声器。
2. MIC1/MIC2 录制 1 秒 → 各自去直流、混音、高通与噪声门 → 双扬声器。

这不是实时低延迟监听。录音缓冲使用 PSRAM；分配、采集或播放失败必须报告失败。发送完 PCM 只能说明软件写入成功，实际声音仍需人工确认。
