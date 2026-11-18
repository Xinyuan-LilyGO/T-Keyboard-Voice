#include "VoiceDisplay.h"
#include "VoicePower.h"

bool VoiceDisplay::begin(lgfx::Bus_SPI &bus, int csPin) {
  panel_.setBus(&bus);

  auto config = panel_.config();
  config.pin_cs = csPin;
  config.pin_rst = -1;
  config.pin_busy = -1;
  config.memory_width = 128;
  config.memory_height = 128;
  config.panel_width = 128;
  config.panel_height = 128;
  config.offset_x = 0;
  config.offset_y = 32;
  config.offset_rotation = 0;
  config.dummy_read_pixel = 8;
  config.dummy_read_bits = 1;
  config.readable = false;
  config.invert = false;
  config.rgb_order = false;
  config.dlen_16bit = false;
  config.bus_shared = true;
  panel_.config(config);

  setPanel(&panel_);
  if (!init()) return false;

  setRotation(0);
  fillScreen(0x0000);
  initialized_ = true;
  return true;
}

namespace KeyboardVoice {
lgfx::Bus_SPI lcdBus;
VoiceDisplay displays[Pins::LCD_COUNT];
VoiceDisplay &display1 = displays[0];
VoiceDisplay &display2 = displays[1];
VoiceDisplay &display3 = displays[2];
VoiceDisplay &display4 = displays[3];
VoiceDisplay &display5 = displays[4];
bool displaysReady = false;
bool backlightAttached = false;
void setBacklightBrightness(uint8_t brightness)
{

    // 背光低电平有效，逻辑亮度需要反相。
    ledcWrite(Pins::LCD_BL, 255 - brightness);
}

void initializeDisplays()
{
    if (!initializeAXP2101(PowerRail::KeyboardDisplay)) return;
    if (!backlightAttached)
        backlightAttached = ledcAttach(Pins::LCD_BL, 10000, 8);
    bool ready = backlightAttached;
    setBacklightBrightness(0);

    auto busConfig = lcdBus.config();
    busConfig.spi_host = SPI2_HOST;
    busConfig.spi_mode = 0;
    busConfig.freq_write = LCD_FREQUENCY;
    busConfig.freq_read = LCD_FREQUENCY;
    busConfig.spi_3wire = false;
    busConfig.use_lock = true;
    busConfig.dma_channel = 0;
    busConfig.pin_sclk = Pins::LCD_SCLK;
    busConfig.pin_mosi = Pins::LCD_MOSI;
    busConfig.pin_miso = -1;
    busConfig.pin_dc = Pins::LCD_DC;
    lcdBus.config(busConfig);

    for (int cs : Pins::LCD_CS)
    {
        pinMode(cs, OUTPUT);
        digitalWrite(cs, HIGH);
    }

    // 五屏共享硬件复位，只由管理层统一操作。
    pinMode(Pins::LCD_RST, OUTPUT);
    digitalWrite(Pins::LCD_RST, LOW);
    delay(20);
    digitalWrite(Pins::LCD_RST, HIGH);
    delay(120);

    for (size_t i = 0; i < Pins::LCD_COUNT; ++i)
    {
        if (!displays[i].begin(lcdBus, Pins::LCD_CS[i]))
        {
            ready = false;
            Serial.printf("[LCD] panel %u init failed on GPIO%d\n",
                          static_cast<unsigned>(i + 1), Pins::LCD_CS[i]);
        }
    }
    displaysReady = ready;
    setBacklightBrightness(ready ? 255 : 0);
}

void suspendDisplays() {
    for (auto &display : displays) {
        if (display.isInitialized()) display.sleep();
    }
    if (backlightAttached) ledcDetach(Pins::LCD_BL);
    backlightAttached = false;
    displaysReady = false;
}

}
