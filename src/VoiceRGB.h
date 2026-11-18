#pragma once
#include "VoiceConfig.h"

namespace KeyboardVoice {

extern bool ws2812Ready;
void writeWs2812(const uint8_t colors[Pins::WS2812_COUNT][3]);
void clearWs2812();
void deinitWs2812();
void testWs2812();

}  // namespace KeyboardVoice
