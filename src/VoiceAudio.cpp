#include "VoiceAudio.h"
#include "VoicePower.h"
#include "VoiceTestAudio.h"

namespace KeyboardVoice {

I2SClass audio;
bool audioReady = false;
bool audioCaptureMode = false;
bool audioPackedFourChannelMode = false;

struct LoopbackDenoiser
{
    int32_t previousInput = 0;
    int32_t previousOutput = 0;
    int32_t envelope = 0;
    size_t hangover = 0;
    bool open = false;

    int16_t process(int32_t input)
    {
        const int32_t filtered = input - previousInput + static_cast<int32_t>(
            (static_cast<int64_t>(previousOutput) * LOOPBACK_HIGHPASS_ALPHA_Q15) >> 15);
        previousInput = input;
        previousOutput = filtered;

        const int32_t magnitude = filtered < 0 ? -filtered : filtered;
        if (magnitude > envelope)
            envelope += (magnitude - envelope + 3) >> 2;
        else
            envelope -= (envelope + 31) >> 5;

        if (!open)
        {
            if (envelope >= LOOPBACK_GATE_OPEN)
            {
                open = true;
                hangover = LOOPBACK_GATE_HANGOVER_FRAMES;
            }
            else
            {
                return 0;
            }
        }
        else if (envelope >= LOOPBACK_GATE_CLOSE)
        {
            hangover = LOOPBACK_GATE_HANGOVER_FRAMES;
        }
        else if (hangover > 0)
        {
            --hangover;
        }
        else
        {
            open = false;
            return 0;
        }

        return static_cast<int16_t>(constrain(filtered, -32768, 32767));
    }
};
bool initializeES8389()
{
    if (!i2cPresent(Devices::ES8389))
        return false;

    const uint8_t init[][2] = {
        {0xF3, 0x00},
        {0x00, 0x7E},
        {0xF3, 0x38},
        {0x24, 0x64},
        {0x25, 0x04},
        {0x45, 0x03},
        {0x60, 0x2A},
        {0x61, 0xC9},
        {0x62, 0x4F},
        {0x63, 0x06},
        {0x6B, 0x00},
        {0x6D, 0x16},
        {0x6E, 0xAA},
        {0x6F, 0x66},
        {0x70, 0x99},
        {0x23, 0x00},
        {0x72, 0x10},
        {0x73, 0x10},
        {0x10, 0xC4},
        {0x01, 0x08},
        {0xF1, 0x00},
        {0x12, 0x01},
        {0x13, 0x01},
        {0x14, 0x01},
        {0x15, 0x01},
        {0x16, 0x35},
        {0x17, 0x09},
        {0x18, 0x91},
        {0x19, 0x28},
        {0x1A, 0x01},
        {0x1B, 0x01},
        {0x1C, 0x11},
        {0xF0, 0x12},
        {0x02, 0x00},
        {0x04, 0x00},
        {0x05, 0x10},
        {0x06, 0x00},
        {0x07, 0xC0},
        {0x08, 0x00},
        {0x09, 0xC0},
        {0x0A, 0x80},
        {0x0B, 0x04},
        {0x0C, 0x01},
        {0x0D, 0x00},
        {0x0F, 0x00},
        {0x21, 0x1F},
        {0x22, 0x7F},
        {0x2F, 0xC0},
        {0x30, 0xF4},
        {0x31, 0x00},
        {0x41, 0x7F},
        {0x42, 0x7F},
        {0x43, 0x10},
        {0x44, 0x00},
        {0x49, 0x0F},
        {0x4C, 0xC0},
        {0x00, 0x00},
        {0x03, 0xC1},
        {0x00, 0x01},
        {0x4D, 0x02},
    };
    for (const auto &entry : init)
    {
        if (!writeRegister(Devices::ES8389, entry[0], entry[1]))
            return false;
    }

    const uint8_t clock48k[][2] = {
        {0x02, 0x00},
        {0x04, 0x01},
        {0x05, 0x41},
        {0x06, 0x04},
        {0x07, 0xD0},
        {0x08, 0x10},
        {0x09, 0xD1},
        {0x0A, 0x80},

        {0x0F, 0x50},
        {0x11, 0x00},
        {0x21, 0x1F},
        {0x22, 0x7F},
        {0x26, 0xBF},
        {0x30, 0xF4},
        {0x41, 0x7F},
        {0x42, 0x7F},
        {0x43, 0x10},
        {0xF0, 0x12},
        {0xF1, 0x00},
        {0x16, 0x35},
        {0x18, 0x91},
        {0x19, 0x28},
    };
    for (const auto &entry : clock48k)
    {
        bool ok = true;
        if (entry[0] == 0x0F)
            ok = updateRegister(Devices::ES8389, entry[0], 0xC0, entry[1] & 0xC0);
        else if (entry[0] == 0x30)
            ok = updateRegister(Devices::ES8389, entry[0], 0xC0, entry[1] & 0xC0);
        else if (entry[0] == 0x43)
            ok = updateRegister(Devices::ES8389, entry[0], 0x81, entry[1] & 0x81);
        else
            ok = writeRegister(Devices::ES8389, entry[0], entry[1]);
        if (!ok)
            return false;
    }

    if (!updateRegister(Devices::ES8389, 0x20, 0x7F, 0x60))
        return false;
    if (!updateRegister(Devices::ES8389, 0x40, 0x7F, 0x60))
        return false;

    if (!writeRegister(Devices::ES8389, 0x27, 0xBF))
        return false;
    if (!writeRegister(Devices::ES8389, 0x28, 0xBF))
        return false;
    if (!writeRegister(Devices::ES8389, 0x46, 0xB0))
        return false;
    if (!writeRegister(Devices::ES8389, 0x47, 0xB0))
        return false;
    if (!writeRegister(Devices::ES8389, 0x48, 0xBE))
        return false;
    if (!updateRegister(Devices::ES8389, 0x69, 0x20, 0x20))
        return false;
    if (!writeRegister(Devices::ES8389, 0x61, 0xD9))
        return false;
    if (!writeRegister(Devices::ES8389, 0x64, 0x8F))
        return false;
    if (!writeRegister(Devices::ES8389, 0x10, 0xE4))
        return false;
    if (!writeRegister(Devices::ES8389, 0x03, 0xC3))
        return false;
    delay(72);
    if (!writeRegister(Devices::ES8389, 0x4D, 0x00))
        return false;

    const uint8_t verify[][2] = {
        {0x02, 0x00},
        {0x04, 0x01},

        {0x0F, 0x40},
        {0x20, 0x60},
        {0x40, 0x60},
        {0x46, 0xB0},
        {0x47, 0xB0},
        {0x4D, 0x00},
    };
    for (const auto &entry : verify)
    {
        uint8_t actual = 0;
        const uint8_t mask = entry[0] == 0x0F ? 0xC0 : 0xFF;
        if (!readRegister(Devices::ES8389, entry[0], actual) ||
            (actual & mask) != (entry[1] & mask))
        {
            Serial.printf("[ES8389] verify reg 0x%02X expected 0x%02X got 0x%02X\n",
                          entry[0], entry[1], actual);
            return false;
        }
    }
    Serial.println("[ES8389] 48 kHz stereo playback configuration verified");
    return true;
}

bool initializeES7210(bool capture, bool tdm)
{
    if (!i2cPresent(Devices::ES7210))
        return false;
    const uint8_t init[][2] = {
        {0x00, 0xFF},
        {0x00, 0x41},
        {0x01, 0x3F},
        {0x09, 0x30},
        {0x0A, 0x30},
        {0x23, 0x2A},
        {0x22, 0x0A},
        {0x20, 0x0A},
        {0x21, 0x2A},
        {0x08, 0x10},
        {0x40, 0x43},
        {0x42, 0x70},
        {0x07, 0x20},
        {0x02, 0xC1},
        {0x11, 0x60},
        {0x12, 0x00},
        {0x45, 0x00},
        {0x46, 0x00},
        {0x06, 0x00},
        {0x49, 0x08},
        {0x4A, 0x08},
        {0x4C, 0xFF},
        {0x40, 0x43},
    };
    for (const auto &entry : init)
    {
        if (!writeRegister(Devices::ES7210, entry[0], entry[1]))
            return false;
    }

    const uint8_t mic12[][2] = {
        {0x16, 0x00},
        {0x1A, 0x00},
        {0x1D, 0xBF},
        {0x1E, 0xBF},
        {0x1B, 0xBF},
        {0x1C, 0xBF},
        {0x41, 0x70},
        {0x43, 0x1A},
        {0x44, 0x1A},
        {0x47, 0x08},
        {0x48, 0x08},
        {0x4B, 0x00},
    };
    for (const auto &entry : mic12)
    {
        if (!writeRegister(Devices::ES7210, entry[0], entry[1]))
            return false;
    }

    const uint8_t mode[][2] = {
        {0x10, 0x00},
        {0x12, static_cast<uint8_t>(tdm ? 0x02 : 0x00)},

        {0x45, static_cast<uint8_t>(tdm ? 0x1A : 0x00)},
        {0x46, static_cast<uint8_t>(tdm ? 0x10 : 0x00)},
        {0x4B, static_cast<uint8_t>(capture ? 0x00 : 0xFF)},
        {0x4C, static_cast<uint8_t>(tdm ? 0x00 : 0xFF)},
        {0x40, static_cast<uint8_t>(capture ? 0x43 : 0xC0)},
        {0x01, static_cast<uint8_t>(capture ? (tdm ? 0x20 : 0x14) : 0x34)},
        {0x06, static_cast<uint8_t>(capture ? 0x00 : 0x07)},
    };
    for (const auto &entry : mode)
    {
        if (!writeRegister(Devices::ES7210, entry[0], entry[1]))
            return false;
    }
    if (capture)
    {
        if (!writeRegister(Devices::ES7210, 0x00, 0x71) ||
            !writeRegister(Devices::ES7210, 0x00, 0x41))
            return false;
        for (const auto &entry : mic12)
        {
            uint8_t actual = 0;
            if (!readRegister(Devices::ES7210, entry[0], actual) || actual != entry[1])
            {
                Serial.printf("[ES7210] verify reg 0x%02X expected 0x%02X got 0x%02X\n",
                              entry[0], entry[1], actual);
                return false;
            }
        }
        for (const auto &entry : mode)
        {
            uint8_t actual = 0;
            if (!readRegister(Devices::ES7210, entry[0], actual) || actual != entry[1])
            {
                Serial.printf("[ES7210] verify reg 0x%02X expected 0x%02X got 0x%02X\n",
                              entry[0], entry[1], actual);
                return false;
            }
        }
        Serial.println(tdm ? "[ES7210] packed TDM-I2S: L=MIC1/MIC3, R=MIC2/MIC4, 48 kHz, 16-bit slots"
                          : "[ES7210] standard I2S: MIC1 / MIC2, 48 kHz, 16 bit");
        Serial.println("[ES7210] MIC3 AUX gain: 30 dB; MIC1/MIC2 gain: 30 dB, ALC off");
    }
    else
    {
        Serial.println("[ES7210] ADC stopped for playback; all microphone paths powered down");
    }
    return true;
}

void discardMicrophoneStartupData()
{
    delay(60);
    uint32_t discarded[AUDIO_BLOCK_FRAMES * 2];

    for (size_t block = 0; block < 32; ++block)
    {
        size_t received = 0;
        const esp_err_t error = i2s_channel_read(audio.rxChan(), discarded, sizeof(discarded),
                                                 &received, 0);
        if (error != ESP_OK || received < sizeof(discarded))
            break;
    }
}

bool initializeAudio(AudioMode mode)
{
    const bool capture = mode != AudioMode::Playback;
    // 四通道仍使用标准 I2S 的 32-bit 左右字，不切换为脉冲帧 TDM。
    const bool packedFourChannel = mode == AudioMode::FourChannel;
    if (audioReady && audioCaptureMode == capture &&
        (!capture || audioPackedFourChannelMode == packedFourChannel))
        return true;
    digitalWrite(Pins::PA_ENABLE, LOW);
    audioReady = false;
    if (!audio.end())
        return false;
    if (!initializeAXP2101(PowerRail::Audio))
        return false;
    if (capture)
    {
        // 采集时保持 DAC 输入静音，避免共享时钟引入扬声器噪声。
        pinMode(Pins::I2S_DOUT, OUTPUT);
        digitalWrite(Pins::I2S_DOUT, LOW);
    }
    audio.setPins(Pins::I2S_BCLK, Pins::I2S_WS, capture ? -1 : Pins::I2S_DOUT,
                  Pins::I2S_DIN, Pins::I2S_MCLK);

    const auto bits = packedFourChannel ? I2S_DATA_BIT_WIDTH_32BIT
                                        : I2S_DATA_BIT_WIDTH_16BIT;
    if (!audio.begin(I2S_MODE_STD, AUDIO_SAMPLE_RATE, bits, I2S_SLOT_MODE_STEREO,
                     I2S_STD_SLOT_BOTH))
    {
        audio.end();
        return false;
    }

    audio.setTimeout(capture ? 100 : 5000);
    const bool codecsReady = (capture || initializeES8389()) &&
                             initializeES7210(capture, packedFourChannel);
    if (!codecsReady)
    {
        audio.end();
        return false;
    }
    if (capture)
        discardMicrophoneStartupData();
    audioCaptureMode = capture;
    audioPackedFourChannelMode = packedFourChannel;
    audioReady = true;
    return true;
}

bool playTestAudio()
{
    const bool speakerEnabled = digitalRead(Pins::PA_ENABLE) == HIGH;
    if (!initializeAudio())
        return false;
    const size_t wavSize = TEST_AUDIO_WAV_SIZE;
    if (wavSize < 44)
    {
        Serial.println("[AUDIO] embedded 66_stereo_48k.wav is invalid");
        return false;
    }

    if (memcmp(TEST_AUDIO_WAV, "RIFF", 4) != 0 ||
        memcmp(TEST_AUDIO_WAV + 8, "WAVE", 4) != 0)
    {
        Serial.println("[AUDIO] embedded WAV has no RIFF/WAVE header");
        return false;
    }
    auto readLe16 = [](const uint8_t *data) -> uint16_t {
        return static_cast<uint16_t>(data[0]) |
               (static_cast<uint16_t>(data[1]) << 8);
    };
    auto readLe32 = [](const uint8_t *data) -> uint32_t {
        return static_cast<uint32_t>(data[0]) |
               (static_cast<uint32_t>(data[1]) << 8) |
               (static_cast<uint32_t>(data[2]) << 16) |
               (static_cast<uint32_t>(data[3]) << 24);
    };

    const uint8_t *fmt = nullptr;
    const uint8_t *pcm = nullptr;
    uint32_t pcmSize = 0;
    size_t cursor = 12;
    while (cursor + 8 <= wavSize)
    {
        const uint8_t *chunk = TEST_AUDIO_WAV + cursor;
        const uint32_t chunkSize = readLe32(chunk + 4);
        const size_t payload = cursor + 8;
        if (payload > wavSize || chunkSize > wavSize - payload)
            break;
        if (memcmp(chunk, "fmt ", 4) == 0 && chunkSize >= 16)
            fmt = chunk + 8;
        else if (memcmp(chunk, "data", 4) == 0)
        {
            pcm = chunk + 8;
            pcmSize = chunkSize;
            break;
        }
        cursor = payload + chunkSize + (chunkSize & 1U);
    }
    if (!fmt || !pcm || readLe16(fmt) != 1 || readLe16(fmt + 2) != 2 ||
        readLe32(fmt + 4) != AUDIO_SAMPLE_RATE || readLe16(fmt + 14) != 16)
    {
        Serial.println("[AUDIO] embedded WAV format is not 48 kHz/16-bit stereo PCM");
        return false;
    }

    Serial.printf("[AUDIO] playing embedded 66_stereo_48k.wav (%u PCM bytes)\n",
                  static_cast<unsigned>(pcmSize));
    digitalWrite(Pins::PA_ENABLE, speakerEnabled ? HIGH : LOW);
    delay(20);
    if (!audio.configureTX(AUDIO_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                           I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH))
        return false;
    const size_t written = audio.write(pcm, pcmSize);
    Serial.printf("[AUDIO] PCM write %u/%u bytes\n", static_cast<unsigned>(written),
                  static_cast<unsigned>(pcmSize));
    return written == pcmSize;
}

bool recordAuxAndPlaySpeaker()
{

    int16_t *recording = static_cast<int16_t *>(ps_malloc(AUX_LOOPBACK_FRAMES * 2 * sizeof(int16_t)));
    if (!recording)
    {
        Serial.println("[AUX LOOPBACK] recording buffer allocation failed");
        return false;
    }

    bool success = initializeAudio(AudioMode::FourChannel);
    size_t capturedFrames = 0;
    int64_t dcSum = 0;
    int32_t rawPeak = 0;
    int32_t channelPeaks[4] = {};
    uint32_t stereoWords[256];
    while (success && capturedFrames < AUX_LOOPBACK_FRAMES)
    {
        const size_t framesToRead = min<size_t>(128, AUX_LOOPBACK_FRAMES - capturedFrames);
        const size_t bytesToRead = framesToRead * 2 * sizeof(uint32_t);
        const size_t received = audio.readBytes(reinterpret_cast<char *>(stereoWords), bytesToRead);
        if (received != bytesToRead)
        {
            Serial.printf("[AUX LOOPBACK] capture timeout at %u/%u frames\n",
                          static_cast<unsigned>(capturedFrames),
                          static_cast<unsigned>(AUX_LOOPBACK_FRAMES));
            success = false;
            break;
        }
        for (size_t frame = 0; frame < framesToRead; ++frame)
        {
            const uint32_t left = stereoWords[frame * 2];
            const uint32_t right = stereoWords[frame * 2 + 1];
            for (size_t channel = 0; channel < 4; ++channel)
            {
                const int16_t channelSample = Es7210::sampleFromStereoWords(left, right, channel);
                channelPeaks[channel] = max(
                    channelPeaks[channel],
                    abs(static_cast<int32_t>(channelSample)));
            }
            const int16_t sample = Es7210::sampleFromStereoWords(
                left, right, Es7210::AUX_LOOPBACK_CHANNEL);
            recording[(capturedFrames + frame) * 2] = sample;
            recording[(capturedFrames + frame) * 2 + 1] = sample;
            dcSum += sample;
            rawPeak = max(rawPeak, abs(static_cast<int32_t>(sample)));
        }
        capturedFrames += framesToRead;
    }

    audioReady = false;
    audio.end();
    digitalWrite(Pins::PA_ENABLE, LOW);
    if (success)
        success = initializeAudio(AudioMode::Playback);
    if (success)
    {
        const int32_t dc = capturedFrames ? static_cast<int32_t>(dcSum / capturedFrames) : 0;

        constexpr int32_t AUX_PLAYBACK_GAIN = 2;
        LoopbackDenoiser denoiser;
        for (size_t frame = 0; frame < capturedFrames; ++frame)
        {
            const int32_t sample = (static_cast<int32_t>(recording[frame * 2]) - dc) *
                                   AUX_PLAYBACK_GAIN;
            const int16_t cleaned = denoiser.process(sample);
            recording[frame * 2] = cleaned;
            recording[frame * 2 + 1] = cleaned;
        }
        digitalWrite(Pins::PA_ENABLE, HIGH);
        delay(20);
        const size_t bytes = capturedFrames * 2 * sizeof(int16_t);
        const size_t written = audio.write(reinterpret_cast<const uint8_t *>(recording), bytes);
        success = written == bytes;
        Serial.printf("[AUX LOOPBACK] captured %u frames; MIC1/2/3/4 peaks=%ld/%ld/%ld/%ld rawPeak=%ld dc=%ld gain=%d; speaker playback %s\n",
                      static_cast<unsigned>(capturedFrames), static_cast<long>(channelPeaks[0]),
                      static_cast<long>(channelPeaks[1]), static_cast<long>(channelPeaks[2]),
                      static_cast<long>(channelPeaks[3]),
                      static_cast<long>(rawPeak),
                      static_cast<long>(dc), static_cast<int>(AUX_PLAYBACK_GAIN),
                      success ? "complete" : "failed");
    }
    free(recording);
    return success;
}

bool recordOnboardMicsAndPlaySpeaker()
{

    int16_t *recording = static_cast<int16_t *>(
        ps_malloc(ONBOARD_MIC_LOOPBACK_FRAMES * 2 * sizeof(int16_t)));
    if (!recording)
    {
        Serial.println("[DUAL MIC LOOPBACK] recording buffer allocation failed");
        return false;
    }

    bool success = initializeAudio(AudioMode::FourChannel);
    size_t capturedFrames = 0;
    int64_t dcSum[2] = {};
    int32_t channelPeaks[2] = {};
    uint32_t stereoWords[256];
    while (success && capturedFrames < ONBOARD_MIC_LOOPBACK_FRAMES)
    {
        const size_t framesToRead = min<size_t>(128, ONBOARD_MIC_LOOPBACK_FRAMES - capturedFrames);
        const size_t bytesToRead = framesToRead * 2 * sizeof(uint32_t);
        const size_t received = audio.readBytes(reinterpret_cast<char *>(stereoWords), bytesToRead);
        if (received != bytesToRead)
        {
            Serial.printf("[DUAL MIC LOOPBACK] capture timeout at %u/%u frames\n",
                          static_cast<unsigned>(capturedFrames),
                          static_cast<unsigned>(ONBOARD_MIC_LOOPBACK_FRAMES));
            success = false;
            break;
        }
        for (size_t frame = 0; frame < framesToRead; ++frame)
        {
            const uint32_t left = stereoWords[frame * 2];
            const uint32_t right = stereoWords[frame * 2 + 1];
            const int16_t mic1 = Es7210::sampleFromStereoWords(
                left, right, Es7210::ONBOARD_MIC1_CHANNEL);
            const int16_t mic2 = Es7210::sampleFromStereoWords(
                left, right, Es7210::ONBOARD_MIC2_CHANNEL);
            const size_t outputFrame = capturedFrames + frame;
            recording[outputFrame * 2] = mic1;
            recording[outputFrame * 2 + 1] = mic2;
            dcSum[0] += mic1;
            dcSum[1] += mic2;
            channelPeaks[0] = max(channelPeaks[0], abs(static_cast<int32_t>(mic1)));
            channelPeaks[1] = max(channelPeaks[1], abs(static_cast<int32_t>(mic2)));
        }
        capturedFrames += framesToRead;
    }

    audioReady = false;
    audio.end();
    digitalWrite(Pins::PA_ENABLE, LOW);
    if (success)
        success = initializeAudio(AudioMode::Playback);
    if (success)
    {
        const int32_t dc1 = capturedFrames ? static_cast<int32_t>(dcSum[0] / capturedFrames) : 0;
        const int32_t dc2 = capturedFrames ? static_cast<int32_t>(dcSum[1] / capturedFrames) : 0;
        LoopbackDenoiser denoiser;
        for (size_t frame = 0; frame < capturedFrames; ++frame)
        {
            const int32_t mic1 = static_cast<int32_t>(recording[frame * 2]) - dc1;
            const int32_t mic2 = static_cast<int32_t>(recording[frame * 2 + 1]) - dc2;
            const int16_t cleaned = denoiser.process((mic1 + mic2) / 2);
            recording[frame * 2] = cleaned;
            recording[frame * 2 + 1] = cleaned;
        }
        digitalWrite(Pins::PA_ENABLE, HIGH);
        delay(20);
        const size_t bytes = capturedFrames * 2 * sizeof(int16_t);
        const size_t written = audio.write(reinterpret_cast<const uint8_t *>(recording), bytes);
        success = written == bytes;
        Serial.printf("[DUAL MIC LOOPBACK] captured %u frames; MIC1/MIC2 peaks=%ld/%ld dc=%ld/%ld; speaker playback %s\n",
                      static_cast<unsigned>(capturedFrames), static_cast<long>(channelPeaks[0]),
                      static_cast<long>(channelPeaks[1]), static_cast<long>(dc1),
                      static_cast<long>(dc2), success ? "complete" : "failed");
    }
    free(recording);
    return success;
}

void suspendAudio()
{
    digitalWrite(Pins::PA_ENABLE, LOW);
    if (i2cPresent(Devices::ES8389))
    {

        updateRegister(Devices::ES8389, 0x40, 0x03, 0x03);
        updateRegister(Devices::ES8389, 0x24, 0x0F, 0x04);
        updateRegister(Devices::ES8389, 0x25, 0x0F, 0x04);
        writeRegister(Devices::ES8389, 0x10, 0xD4);
        delay(70);
        const uint8_t standby[][2] = {{0x61, 0x59}, {0x64, 0x00}, {0x03, 0x00}, {0x00, 0x3E}};
        for (const auto &entry : standby)
            writeRegister(Devices::ES8389, entry[0], entry[1]);
        updateRegister(Devices::ES8389, 0x45, 0x80, 0x80);
        delay(8);
        updateRegister(Devices::ES8389, 0x45, 0x80, 0x00);
    }
    if (i2cPresent(Devices::ES7210))
    {

        const uint8_t standby[][2] = {
            {0x47, 0xFF},
            {0x48, 0xFF},
            {0x49, 0xFF},
            {0x4A, 0xFF},
            {0x4B, 0xFF},
            {0x4C, 0xFF},
            {0x40, 0xC0},
            {0x01, 0x7F},
            {0x06, 0x07},
        };
        for (const auto &entry : standby)
            writeRegister(Devices::ES7210, entry[0], entry[1]);
    }
    audio.end();
    audioReady = false;
}

void stopAudio() {
    digitalWrite(Pins::PA_ENABLE, LOW);
    audio.end();
    audioReady = false;
    audioCaptureMode = false;
    audioPackedFourChannelMode = false;
}

size_t readMicrophoneFrames(int16_t *samples, size_t maxFrames) {
    if (!samples || !audioReady || !audioCaptureMode) return 0;
    maxFrames = min<size_t>(maxFrames, AUDIO_BLOCK_FRAMES);
    const size_t bytesPerFrame = audioPackedFourChannelMode ? 8 : 4;
    uint32_t raw[AUDIO_BLOCK_FRAMES * 2];
    const size_t received = audio.readBytes(reinterpret_cast<char *>(raw), maxFrames * bytesPerFrame);
    if (received % bytesPerFrame != 0) return 0;
    const size_t frames = received / bytesPerFrame;
    for (size_t frame = 0; frame < frames; ++frame) {
        if (audioPackedFourChannelMode) {
            for (size_t mic = 0; mic < 4; ++mic)
                samples[frame * 4 + mic] = Es7210::sampleFromStereoWords(raw[frame * 2], raw[frame * 2 + 1], mic);
        } else {
            const int16_t *stereo = reinterpret_cast<const int16_t *>(raw);
            samples[frame * 4] = stereo[frame * 2];
            samples[frame * 4 + 1] = stereo[frame * 2 + 1];
            samples[frame * 4 + 2] = 0;
            samples[frame * 4 + 3] = 0;
        }
    }
    return frames;
}

void printCodecDiagnostics() {
    const uint8_t regs[] = {0x01, 0x06, 0x08, 0x10, 0x11, 0x12, 0x16, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x22,
                            0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C};
    for (uint8_t reg : regs) {
        uint8_t value = 0;
        if (readRegister(Devices::ES7210, reg, value))
            Serial.printf("[ES7210] 0x%02X=0x%02X\n", reg, value);
        else Serial.printf("[ES7210] 0x%02X READ FAILED\n", reg);
    }
}

}  // namespace KeyboardVoice
