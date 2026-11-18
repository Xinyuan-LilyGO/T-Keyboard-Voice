#pragma once
#include <ESP_I2S.h>
#include "VoiceConfig.h"
#include "es7210_samples.h"

namespace KeyboardVoice {

enum class AudioMode { Playback, Onboard, FourChannel };
extern I2SClass audio;
extern bool audioReady;
extern bool audioCaptureMode;
extern bool audioPackedFourChannelMode;
bool initializeAudio(AudioMode mode = AudioMode::Playback);
bool initializeES8389();
bool initializeES7210(bool capture, bool tdm);
void suspendAudio();
void stopAudio();
bool playTestAudio();
bool recordAuxAndPlaySpeaker();
bool recordOnboardMicsAndPlaySpeaker();

// 输出 MIC1～4 交错样本，缓冲至少为 maxFrames * 4 个 int16_t；最多读取 AUDIO_BLOCK_FRAMES 帧。
// 双通道模式将 MIC3/4 填零，错误返回 0。
size_t readMicrophoneFrames(int16_t *samples, size_t maxFrames);
void printCodecDiagnostics();

}  // namespace KeyboardVoice
