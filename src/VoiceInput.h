#pragma once
#include <ButtonManager.h>
#include <ButtonSense.h>
#include "VoiceConfig.h"

namespace KeyboardVoice {

extern button::ButtonSense keys[5];
extern button::ButtonManager keyManager;
extern const int KEY_PINS[5];
extern uint8_t keySeenMask;
extern bool powerKeyReleasedSinceBoot;
void initializeKeys();

}  // namespace KeyboardVoice
