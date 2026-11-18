# AudioPlayback

**English** | [简体中文](README_CN.md)

ES8389, speaker, headphone, and power amplifier (PA) control demo. Connect the speakers/headphones correctly and keep them away from your ears during the first playback test.

```bash
pio run -e audio_playback -t upload
pio device monitor -b 115200
```

This demo enables ALDO2. I2C uses SDA GPIO3 and SCL GPIO2; the ES8389 address is `0x10`. Audio pins are MCLK GPIO16, BCLK GPIO17, WS GPIO18, DOUT GPIO8, and PA_EN GPIO45. The original firmware's ES7210 capture-stop configuration is retained to avoid interference on the shared clocks.

After initialization, the PA is off and the demo waits for commands:

| Command | Action |
| --- | --- |
| `speaker` | Enable the PA and play the test WAV |
| `headphone` | Disable the PA and play the test WAV; check the headphones manually |
| `play` | Play using the current PA state |
| `pa on` / `pa off` | Enable/disable the speaker amplifier |
| `stop` | Mute and reset the playback chain |

The WAV is 48 kHz, 16-bit stereo. Playback is blocking and lasts for a short clip; commands are handled after playback finishes. A `sent` log only confirms that data was written successfully. Actual sound from both speakers/headphone channels must be checked by listening.

If there is no sound, check address `0x10`, ALDO2, the clocks, PA state, and output wiring. Disabling the PA only controls the speaker amplifier; it does not mean the headphone output is muted. See the [audio guide (Chinese)](../../docs/audio.md) for registers, gain, and format details.
