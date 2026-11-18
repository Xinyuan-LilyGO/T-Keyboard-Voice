#include "VoicePower.h"
#include "VoiceRGB.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;
bool chase = true;
uint8_t color[3] = {18, 18, 18};
size_t position = 0;
uint32_t lastFrame = 0;

void showColor() {
    uint8_t colors[Pins::WS2812_COUNT][3] = {};
    for (size_t i = 0; i < Pins::WS2812_COUNT; ++i) {
        if (chase && i != position) continue;
        for (size_t channel = 0; channel < 3; ++channel) colors[i][channel] = color[channel];
    }
    writeWs2812(colors);
}

void setup() {
    ready = beginBoard(PowerRail::Rgb);
    if (!ready) { Serial.println("ERROR: power initialization failed."); return; }
    showColor();
    ready = ws2812Ready;
    Serial.println("WS2812: red / green / blue / white / chase / off");
    if (!ready) Serial.println("ERROR: RMT initialization failed.");
}

void loop() {
    if (!ready) { delay(10); return; }
    String command;
    if (console.read(command)) {
        chase = command == "chase";
        memset(color, 0, sizeof(color));
        if (command == "red") color[0] = 24;
        else if (command == "green") color[1] = 24;
        else if (command == "blue") color[2] = 24;
        else if (command == "white" || command == "chase") color[0] = color[1] = color[2] = 12;
        else if (command != "off") Serial.println("Commands: red / green / blue / white / chase / off");
        showColor();
    }
    if (chase && millis() - lastFrame >= 120) {
        lastFrame = millis();
        position = (position + 1) % Pins::WS2812_COUNT;
        showColor();
    }
    delay(2);
}
