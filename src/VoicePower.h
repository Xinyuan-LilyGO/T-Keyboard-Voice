#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <PmicDrv.hpp>
#include "VoiceConfig.h"

namespace KeyboardVoice {

struct BatteryTelemetry
{
    bool available = false;
    bool batteryPresent = false;
    bool vbusPresent = false;
    bool charging = false;
    bool chargeDone = false;
    bool fault = false;
    uint8_t chargingStatusCode = 0xFF;
    bool capacityValid = false;
    bool voltageValid = false;
    bool vbusVoltageValid = false;
    bool systemVoltageValid = false;
    float capacityPercent = 0.0f;
    float batteryVoltageMv = 0.0f;
    float vbusVoltageMv = 0.0f;
    float systemVoltageMv = 0.0f;
    uint16_t chargeVoltageMv = 0;
    uint16_t fastChargeCurrentMa = 0;
};

struct PowerKeyConfig
{
    bool available = false;
    uint16_t onDurationMs = 0;
    uint16_t offDurationMs = 0;
    uint8_t registerValue = 0;
};

namespace PowerRail {
constexpr uint8_t Audio = 1;
constexpr uint8_t KeyboardDisplay = 2;
constexpr uint8_t Rgb = 4;
constexpr uint8_t All = Audio | KeyboardDisplay | Rgb;
}

struct SleepHooks {
    void (*prepare)() = nullptr;
    void (*recover)() = nullptr;
    void (*pending)(bool) = nullptr;
};
extern PmicAXP2101 pmic;
extern bool pmicReady;
extern BatteryTelemetry batteryTelemetry;
extern PowerKeyConfig powerKeyConfig;
extern bool sleepRequested;

bool beginBoard(uint8_t railMask = 0);
bool initializeAXP2101(uint8_t railMask = 0);
bool enablePeripheralRails(uint8_t railMask);
bool sampleBatteryTelemetry();
bool batteryTelemetryPasses();
const char *chargingStatusText(const BatteryTelemetry &telemetry);
bool readPowerKeyConfig();
void pollPmicIrq();
void setSleepHooks(const SleepHooks &hooks);
void pollShutdownKey();
void enterPowerKeyDeepSleep();
void restoreStandbyPins();
void printPowerDiagnostics();
bool i2cPresent(TwoWire &bus, uint8_t address);
bool i2cPresent(uint8_t address);
bool readRegister(uint8_t address, uint8_t reg, uint8_t &value);
bool writeRegister(uint8_t address, uint8_t reg, uint8_t value);
bool updateRegister(uint8_t address, uint8_t reg, uint8_t mask, uint8_t value);
String scanI2C(TwoWire &bus);
bool isOnboardI2CAddress(uint8_t address);
bool hasExternalI2CDevice(TwoWire &bus);
void printI2CScan();

}  // namespace KeyboardVoice
