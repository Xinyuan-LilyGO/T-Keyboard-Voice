#include "VoiceInput.h"

namespace KeyboardVoice {

const int KEY_PINS[5] = {Pins::KEY1, Pins::KEY2, Pins::KEY3, Pins::KEY4, Pins::KEY5};
button::ButtonSense keys[5];
button::ButtonManager keyManager;
uint8_t keySeenMask = 0;
bool powerKeyReleasedSinceBoot = false;

void initializeKeys()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    powerKeyReleasedSinceBoot = digitalRead(Pins::POWER_KEY) == HIGH;
    for (size_t i = 0; i < 5; ++i)
    {
        keys[i].begin(KEY_PINS[i], true, true);
        keys[i].setDebounceThresh(DEBOUNCE_MS);
        keys[i].setHoldThresh(500);
        keyManager.add(keys[i]);
    }
}

}  // namespace KeyboardVoice
