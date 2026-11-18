#include "VoiceRGB.h"
#include "VoicePower.h"
#include <esp32-hal-rmt.h>

namespace KeyboardVoice {

bool ws2812Ready = false;
void writeWs2812(const uint8_t colors[Pins::WS2812_COUNT][3])
{
    if (!initializeAXP2101(PowerRail::Rgb)) return;
    if (!ws2812Ready)
    {
        ws2812Ready = rmtInit(Pins::WS2812_DATA, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
    }
    if (!ws2812Ready)
        return;

    rmt_data_t data[Pins::WS2812_COUNT * 24];
    size_t symbol = 0;
    for (size_t led = 0; led < Pins::WS2812_COUNT; ++led)
    {
        const uint8_t grb[3] = {colors[led][1], colors[led][0], colors[led][2]};
        for (uint8_t component : grb)
        {
            for (int bit = 7; bit >= 0; --bit)
            {
                const bool one = component & (1U << bit);
                data[symbol].level0 = 1;
                data[symbol].duration0 = one ? 8 : 4;
                data[symbol].level1 = 0;
                data[symbol].duration1 = one ? 4 : 8;
                ++symbol;
            }
        }
    }
    rmtWrite(Pins::WS2812_DATA, data, symbol, RMT_WAIT_FOR_EVER);
    delayMicroseconds(80);
}

void clearWs2812()
{
    uint8_t colors[Pins::WS2812_COUNT][3] = {};
    writeWs2812(colors);
}

void deinitWs2812()
{
    if (!ws2812Ready)
        return;
    rmtDeinit(Pins::WS2812_DATA);
    ws2812Ready = false;
}

void testWs2812()
{
    uint8_t colors[Pins::WS2812_COUNT][3] = {};
    struct SolidColorStage
    {
        const char *name;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };
    const SolidColorStage stages[] = {
        {"RED", 24, 0, 0},
        {"GREEN", 0, 24, 0},
        {"BLUE", 0, 0, 24},
        {"WHITE", 12, 12, 12},
    };

    clearWs2812();
    for (const auto &stage : stages)
    {
        Serial.printf("[WS2812] all 14 LEDs: %s\n", stage.name);
        for (size_t led = 0; led < Pins::WS2812_COUNT; ++led)
        {
            colors[led][0] = stage.red;
            colors[led][1] = stage.green;
            colors[led][2] = stage.blue;
        }
        writeWs2812(colors);
        delay(1200);
        clearWs2812();
        delay(200);
    }

    Serial.println("[WS2812] individual LED chase: WHITE");
    for (size_t led = 0; led < Pins::WS2812_COUNT; ++led)
    {
        memset(colors, 0, sizeof(colors));
        colors[led][0] = 18;
        colors[led][1] = 18;
        colors[led][2] = 18;
        Serial.printf("[WS2812] LED %u/%u: WHITE\n",
                      static_cast<unsigned>(led + 1),
                      static_cast<unsigned>(Pins::WS2812_COUNT));
        writeWs2812(colors);
        delay(150);
    }

    Serial.println("[WS2812] final RGB position pattern");
    const uint8_t palette[3][3] = {{24, 0, 0}, {0, 24, 0}, {0, 0, 24}};
    for (size_t led = 0; led < Pins::WS2812_COUNT; ++led)
    {
        colors[led][0] = palette[led % 3][0];
        colors[led][1] = palette[led % 3][1];
        colors[led][2] = palette[led % 3][2];
    }
    writeWs2812(colors);
    delay(1500);
    clearWs2812();
}

}  // namespace KeyboardVoice
