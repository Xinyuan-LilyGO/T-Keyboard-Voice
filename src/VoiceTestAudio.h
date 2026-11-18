#pragma once
#include <stddef.h>
#include <stdint.h>

namespace KeyboardVoice {
// 保存在 Flash 中，统一供 PlatformIO、Arduino IDE 和 ESP-IDF 使用。
extern const uint8_t TEST_AUDIO_WAV[];
extern const size_t TEST_AUDIO_WAV_SIZE;
}
