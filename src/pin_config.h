#pragma once

#include <Arduino.h>

namespace Pins {
constexpr int BOARD_I2C_SCL = 2;
constexpr int BOARD_I2C_SDA = 3;

constexpr int QWIIC_I2C_SCL = BOARD_I2C_SCL;
constexpr int QWIIC_I2C_SDA = BOARD_I2C_SDA;
constexpr int QWIIC_UART_TX = 43;
constexpr int QWIIC_UART_RX = 44;
constexpr int PMU_IRQ = 21;
constexpr int AUX_DETECT = 6;

constexpr int I2S_MCLK = 16;
constexpr int I2S_BCLK = 17;
constexpr int I2S_WS = 18;
constexpr int I2S_DIN = 15;
constexpr int I2S_DOUT = 8;
constexpr int ES7210_INT = 9;
constexpr int PA_ENABLE = 45;

constexpr int KEY1 = 10;
constexpr int KEY2 = 14;
constexpr int KEY3 = 13;
constexpr int KEY4 = 12;
constexpr int KEY5 = 0;
constexpr int POWER_KEY = KEY4;

constexpr int LCD_SCLK = 41;
constexpr int LCD_MOSI = 40;
constexpr int LCD_DC = 39;
constexpr int LCD_BL = 1;
constexpr int LCD_RST = 46;

constexpr int LCD_CS[] = {48, 38, 7, 42, 11};
constexpr size_t LCD_COUNT = sizeof(LCD_CS) / sizeof(LCD_CS[0]);
constexpr int LCD_WIDTH = 128;
constexpr int LCD_HEIGHT = 128;

constexpr int WS2812_DATA = 47;
constexpr size_t WS2812_COUNT = 14;
}

namespace Devices {
constexpr uint8_t AXP2101 = 0x34;
constexpr uint8_t ES8389 = 0x10;
constexpr uint8_t ES7210 = 0x40;

constexpr uint8_t ONBOARD_I2C_ADDRESSES[] = {ES8389, AXP2101, ES7210};
constexpr size_t ONBOARD_I2C_ADDRESS_COUNT =
    sizeof(ONBOARD_I2C_ADDRESSES) / sizeof(ONBOARD_I2C_ADDRESSES[0]);
}
