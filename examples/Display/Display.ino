#include "VoicePower.h"
#include "VoiceDisplay.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;
uint8_t brightness = 255;

void showPanels(bool solid) {
    const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0xFFE0};
    for (size_t i = 0; i < Pins::LCD_COUNT; ++i) {
        displays[i].fillScreen(colors[i]);
        if (!solid) {
            displays[i].setTextColor(i == 2 ? 0xFFFF : 0x0000);
            displays[i].setTextSize(3);
            displays[i].setCursor(45, 45);
            displays[i].printf("%u", static_cast<unsigned>(i + 1));
            displays[i].setTextSize(1);
            displays[i].setCursor(8, 100);
            displays[i].printf("GC9107 CS=%d", Pins::LCD_CS[i]);
        }
    }
}

void setup() {
    if (!beginBoard(PowerRail::KeyboardDisplay)) {
        Serial.println("ERROR: power initialization failed.");
        return;
    }
    initializeDisplays();
    ready = displaysReady;
    if (ready) showPanels(false);
    Serial.println("Display: numbers / colors / off / on / dim / bright / fade");
    if (!ready) Serial.println("ERROR: display initialization failed.");
}

void loop() {
    String command;
    if (console.read(command) && ready) {
        if (command == "numbers") showPanels(false);
        else if (command == "colors") showPanels(true);
        else if (command == "off") brightness = 0;
        else if (command == "on") brightness = 255;
        else if (command == "dim") brightness = 32;
        else if (command == "bright") brightness = 255;
        else if (command == "fade") {
            for (int level = 0; level <= 255; level += 5) {
                setBacklightBrightness(level);
                delay(10);
            }
            brightness = 255;
        } else Serial.println("Commands: numbers / colors / off / on / dim / bright / fade");
        setBacklightBrightness(brightness);
    }
    delay(2);
}
