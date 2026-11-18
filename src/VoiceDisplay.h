#pragma once

#include <LovyanGFX.hpp>

class VoiceDisplay : public lgfx::LGFX_Device {
 public:
  bool begin(lgfx::Bus_SPI &bus, int csPin);
  bool isInitialized() const { return initialized_; }

 private:
  lgfx::Panel_GC9107 panel_;
  bool initialized_ = false;
};

#include "VoiceConfig.h"
namespace KeyboardVoice {
extern lgfx::Bus_SPI lcdBus;
extern VoiceDisplay displays[Pins::LCD_COUNT];
extern VoiceDisplay &display1;
extern VoiceDisplay &display2;
extern VoiceDisplay &display3;
extern VoiceDisplay &display4;
extern VoiceDisplay &display5;
extern bool displaysReady;
extern bool backlightAttached;
void initializeDisplays();
void setBacklightBrightness(uint8_t brightness);
void suspendDisplays();
}
