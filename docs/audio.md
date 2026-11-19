# Audio Signal Paths

[English](#english) | [简体中文](#简体中文)

## English

### Devices and Sample Format

- ES8389: playback codec at I2C address 0x10.
- ES7210: four-channel ADC at I2C address 0x40; only SDOUT1 is connected to the MCU.
- Samples are 48 kHz, 16-bit. Playback uses standard stereo I2S with a 12.288 MHz MCLK (256fs).
- `assets/66_stereo_48k.wav` is converted by `scripts/generate_audio.py` into `src/audio/test_audio.inc`, with `VoiceTestAudio.cpp` providing the shared read-only resource. None of the three build systems depends on linker-generated embedding symbols. Only AudioPlayback and Factory use this test clip; unused data is removed by the linker.
- `assets/original/66.wav` is retained only as an archive and is not part of the default build.

| Channel | Physical purpose |
| --- | --- |
| MIC1 | Onboard microphone 1 |
| MIC2 | Onboard microphone 2 |
| MIC3 | CN5 external input |
| MIC4 | AEC reference from the left output |

MIC4 provides a reference signal; the firmware does not implement acoustic echo cancellation. A quiet MIC4 when nothing is playing is normal.

### Three Explicit Modes

`initializeAudio(AudioMode::Playback)`, `AudioMode::Onboard`, and `AudioMode::FourChannel` select playback, onboard dual-microphone capture, and four-channel capture, respectively. Shared drivers do not inspect Factory test numbers.

The onboard pair uses standard 16-bit stereo I2S. Four-channel capture preserves the ES7210 packed TDM-I2S format:

- High/low 16 bits of the left 32-bit word: MIC1 / MIC3.
- High/low 16 bits of the right 32-bit word: MIC2 / MIC4.
- ESP32 reception still uses standard 32-bit stereo I2S. Do not replace it with pulse-frame `I2S_MODE_TDM`.

`readMicrophoneFrames()` returns interleaved MIC1–MIC4 samples. In dual-microphone mode, MIC3/MIC4 are filled with zeros; this does not indicate an external-channel fault. Each read returns at most 128 frames.

### Gain and Mode Switching

MIC1/2 retain matching 30 dB analog gain, 0 dB digital gain, and disabled ALC, with key registers read back for verification. MIC3 uses 30 dB gain, while MIC4 uses 0 dB reference gain. Compare microphone sensitivity with a steady source at equal distance; a single short-window speech RMS reading is not sufficient evidence.

Capture disables the PA and keeps DAC input data muted. Recording and playback share clocks and I2S resources, so switching stops the previous mode first. The PA controls only the speaker amplifier and does not verify the headphone path.

CN5 GPIO6 is expected to be HIGH when idle and LOW when inserted, but detection is only a hint. Some TRRS plugs do not change this pin, so it must not be used to prohibit MIC3 capture.

### Record-then-Playback

AudioRecord and Factory support two paths:

1. Record MIC3 for 1 second → DC removal, high-pass filter, noise gate, and limited 2× gain → both speakers.
2. Record MIC1/MIC2 for 1 second → per-channel DC removal, mixing, high-pass filter, and noise gate → both speakers.

This is not real-time, low-latency monitoring. Recording buffers use PSRAM; allocation, capture, or playback failures must be reported as failures. Successfully sending PCM only confirms a software write; actual audio still requires listening.

---

## 简体中文

**音频链路**

### 芯片与采样参数

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

### 三种显式模式

`initializeAudio(AudioMode::Playback)`、`AudioMode::Onboard`、`AudioMode::FourChannel` 分别表示播放、板载双麦、四通道采集。公共驱动不读取 Factory 的测试编号。

板载双麦使用 16-bit stereo 标准 I2S。四通道采集保留 ES7210 的 packed TDM-I2S：

- 左 32-bit 字的高/低 16 位：MIC1 / MIC3。
- 右 32-bit 字的高/低 16 位：MIC2 / MIC4。
- ESP32 仍使用标准 I2S 32-bit stereo 接收，不能替换成脉冲帧 `I2S_MODE_TDM`。

`readMicrophoneFrames()` 输出 MIC1～MIC4 四通道交错样本；双麦模式下 MIC3/MIC4 为零，不能据此判断外接通道故障。每次最多读取 128 帧。

### 增益与切换

MIC1/2 保留相同的 30 dB 模拟增益、0 dB 数字增益、ALC 关闭，并读回关键寄存器；MIC3 为 30 dB、MIC4 参考为 0 dB。比较麦克风灵敏度时，使用等距稳定声源，不能用单次短时语音 RMS 作结论。

采集时关闭 PA 并让 DAC 数据保持静音；录音/播放共享时钟与 I2S 资源，切换会先停止原模式。PA 只控制扬声器功放，不能代替耳机链路检查。

CN5 GPIO6 预期空闲 HIGH、插入 LOW，但它仅作提示：某些 TRRS 插头不改变该引脚，不能因此禁止 MIC3 采集。

### 录音回放

AudioRecord 和 Factory 支持两条链路：

1. MIC3 录制 1 秒 → 去直流、高通、噪声门、受限 2 倍增益 → 双扬声器。
2. MIC1/MIC2 录制 1 秒 → 各自去直流、混音、高通与噪声门 → 双扬声器。

这不是实时低延迟监听。录音缓冲使用 PSRAM；分配、采集或播放失败必须报告失败。发送完 PCM 只能说明软件写入成功，实际声音仍需人工确认。
