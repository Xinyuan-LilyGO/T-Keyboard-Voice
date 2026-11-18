#include "VoicePower.h"
#include "VoiceInput.h"
using namespace KeyboardVoice;

bool ready = false;

void setup() {
    // 按键所在电源域与显示共用，但不初始化屏幕。
    ready = beginBoard(PowerRail::KeyboardDisplay);
    if (!ready) { Serial.println("ERROR: power initialization failed."); return; }
    initializeKeys();
    Serial.println("Keys: press/release/click/hold KEY1..KEY5; KEY4 does not sleep in this demo.");
}

void loop() {
    if (!ready) { delay(10); return; }
    keyManager.update();
    for (size_t i = 0; i < 5; ++i) {
        if (keys[i].wasPressed()) {
            keySeenMask |= 1U << i;
            Serial.printf("KEY%u GPIO%d pressed; seen=0x%02X\n", static_cast<unsigned>(i + 1), KEY_PINS[i], keySeenMask);
        }
        if (keys[i].wasReleased()) Serial.printf("KEY%u released\n", static_cast<unsigned>(i + 1));
        if (keys[i].wasClicked()) Serial.printf("KEY%u clicked\n", static_cast<unsigned>(i + 1));
        if (keys[i].wasHold()) Serial.printf("KEY%u hold\n", static_cast<unsigned>(i + 1));
    }
    delay(2);
}
