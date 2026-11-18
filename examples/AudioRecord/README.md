# AudioRecord

**English** | [简体中文](README_CN.md)

ES7210 onboard/external capture, AEC reference reading, and record-then-playback through the speakers.

```bash
pio run -e audio_record -t upload
pio device monitor -b 115200
```

This demo enables ALDO2 and requires working PSRAM. The ES7210 address is `0x40`, and its data reaches the MCU on DIN GPIO15. MIC1/2 are the onboard microphones, MIC3 is the CN5 external input, and MIC4 is the AEC reference. Playback also requires the ES8389 and speakers; PA_EN is GPIO45.

| Command | Action |
| --- | --- |
| `onboard` | Capture the onboard microphone pair; this is the default mode |
| `external` | Capture four-channel packed TDM-I2S, including MIC3/MIC4 |
| `loop onboard` | Record the onboard pair for 1 second, mix, then play through both speakers |
| `loop aux` | Record external MIC3 for 1 second, then play through both speakers |
| `mic` | Read back ES7210 registers |
| `stop` | Stop capture and disable the PA |

Every 500 ms, the console reports four-channel RMS, raw peak, DC, and the CN5 detection pin from a short sample block. These are not continuous half-second averages. In `onboard` mode, MIC3/4 are filled with zeros. After playback, the demo automatically mutes the output and restores the previous capture mode.

Speak into the onboard microphones or feed a known signal into CN5 and observe the corresponding channels. For record-then-playback, listen for the actual recording and check both speakers. Do not mistake noise from an unconnected ADC input for a valid signal. MIC4 may be quiet during capture-only `external` mode; it is a reference input, not an echo-cancellation algorithm.

The CN5 detection pin is only a hint; some TRRS plugs do not change its state. Initialization or timeout failures print `FAILED`; retry with `onboard` or `external`. Check ALDO2, address `0x40`, I2S clocks, PSRAM, and plug type. See the [audio guide (Chinese)](../../docs/audio.md) for capture/playback switching, wire order, and gain details.
