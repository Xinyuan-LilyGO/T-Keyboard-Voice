#include "VoicePower.h"
#include "power_key_standby.h"
#include <pmic/xpowers/axp2101/AXP2101Regs.hpp>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>
#include <esp_system.h>

namespace KeyboardVoice {

PmicAXP2101 pmic;
bool pmicReady = false;
BatteryTelemetry batteryTelemetry;
PowerKeyConfig powerKeyConfig;
bool sleepRequested = false;
static uint8_t enabledRailMask = 0;
static SleepHooks sleepHooks;
PowerKeyStandby shutdownKeyState(DEBOUNCE_MS);
RTC_DATA_ATTR uint32_t spuriousWakeCount = 0;

void setSleepHooks(const SleepHooks &hooks) { sleepHooks = hooks; }

bool enablePeripheralRails(uint8_t mask) {
    if (!pmicReady) return false;
    auto *rails = pmic.getChannel();
    if (!rails) return false;
    const uint8_t channels[] = {AXP2101Channel::CH_ALDO2, AXP2101Channel::CH_ALDO3,
                                AXP2101Channel::CH_DLDO1};
    bool ready = true;
    for (size_t i = 0; i < 3; ++i) {
        if (!(mask & (1U << i))) continue;
        const bool ok = rails->setVoltage(channels[i], 3300) &&
                        rails->enable(channels[i], true) &&
                        rails->isEnabled(channels[i]) && rails->getVoltage(channels[i]) == 3300;
        if (ok) enabledRailMask |= 1U << i;
        ready = ok && ready;
    }
    return ready;
}

struct StandbyPin
{
    int pin;
    gpio_mode_t mode;
    gpio_pull_mode_t pull;
    int level;
};

constexpr StandbyPin STANDBY_PINS[] = {
    {Pins::KEY5, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::LCD_BL, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::BOARD_I2C_SCL, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::BOARD_I2C_SDA, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::AUX_DETECT, GPIO_MODE_INPUT, GPIO_PULLDOWN_ONLY, LOW},
    {Pins::LCD_CS[2], GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::I2S_DOUT, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::ES7210_INT, GPIO_MODE_INPUT, GPIO_PULLDOWN_ONLY, LOW},
    {Pins::KEY1, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::LCD_CS[4], GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::KEY3, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::KEY2, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::I2S_DIN, GPIO_MODE_INPUT, GPIO_PULLDOWN_ONLY, LOW},
    {Pins::I2S_MCLK, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::I2S_BCLK, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::I2S_WS, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_CS[1], GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_DC, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_MOSI, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_SCLK, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_CS[3], GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::QWIIC_UART_TX, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::QWIIC_UART_RX, GPIO_MODE_INPUT, GPIO_PULLUP_ONLY, LOW},
    {Pins::PA_ENABLE, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_RST, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::WS2812_DATA, GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
    {Pins::LCD_CS[0], GPIO_MODE_OUTPUT, GPIO_FLOATING, LOW},
};

struct PeripheralRail
{
    uint8_t channel;
    const char *name;
};

// 休眠保留 MCU 的 DCDC1，关闭其余外设及未使用输出。
constexpr PeripheralRail POWER_OFF_RAILS[] = {
    {AXP2101Channel::CH_DCDC2, "DCDC2/unused"},
    {AXP2101Channel::CH_DCDC3, "DCDC3/unused"},
    {AXP2101Channel::CH_DCDC4, "DCDC4/unused"},
    {AXP2101Channel::CH_DCDC5, "DCDC5/unused"},
    {AXP2101Channel::CH_ALDO1, "ALDO1/unused"},
    {AXP2101Channel::CH_ALDO2, "ALDO2/audio"},
    {AXP2101Channel::CH_ALDO3, "ALDO3/keyboard-display"},
    {AXP2101Channel::CH_ALDO4, "ALDO4/unused"},
    {AXP2101Channel::CH_BLDO1, "BLDO1/unused"},
    {AXP2101Channel::CH_BLDO2, "BLDO2/unused"},
    {AXP2101Channel::CH_CPUSLDO, "CPUSLDO/unused"},
    {AXP2101Channel::CH_DLDO1, "DLDO1/WS2812"},
    {AXP2101Channel::CH_DLDO2, "DLDO2/unused"},
};
bool i2cPresent(TwoWire &bus, uint8_t address)
{
    bus.beginTransmission(address);
    return bus.endTransmission() == 0;
}

bool i2cPresent(uint8_t address)
{
    return i2cPresent(Wire, address);
}

bool initializeAXP2101(uint8_t railMask)
{
    if (pmicReady)
        return enablePeripheralRails(railMask);

    // SensorLib 会改写 TS 配置，先保存并在初始化后恢复、读回。
    static bool savedTsConfig = false;
    static uint8_t tsControl = 0;
    static uint8_t adcControl = 0;
    static uint8_t moduleControl = 0;
    static bool chargingSuspended = false;
    if (!savedTsConfig)
    {
        if (!readRegister(Devices::AXP2101, axp2101_regs::ts::TS_PIN_CTRL, tsControl) ||
            !readRegister(Devices::AXP2101, axp2101_regs::adc::CHANNEL_CTRL, adcControl) ||
            !readRegister(Devices::AXP2101, axp2101_regs::ctrl::MODULE_EN, moduleControl))
        {
            Serial.println("[AXP2101] could not read TS configuration");
            return false;
        }
        savedTsConfig = true;
    }
    const bool found = pmic.begin(Wire, Devices::AXP2101,
                                  Pins::BOARD_I2C_SDA, Pins::BOARD_I2C_SCL);
    Wire.setClock(I2C_FREQUENCY);
    if (!found)
    {
        Serial.println("[AXP2101] SensorLib initialization failed");
        return false;
    }
    bool tsRestored = false;
    for (int attempt = 0; attempt < 3 && !tsRestored; ++attempt)
    {
        const bool tsWritten = writeRegister(Devices::AXP2101, axp2101_regs::ts::TS_PIN_CTRL,
                                             tsControl);
        const bool adcWritten = writeRegister(Devices::AXP2101, axp2101_regs::adc::CHANNEL_CTRL,
                                              adcControl);
        uint8_t actualTs = 0;
        uint8_t actualAdc = 0;
        tsRestored = tsWritten && adcWritten &&
                     readRegister(Devices::AXP2101, axp2101_regs::ts::TS_PIN_CTRL, actualTs) &&
                     readRegister(Devices::AXP2101, axp2101_regs::adc::CHANNEL_CTRL, actualAdc) &&
                     actualTs == tsControl && actualAdc == adcControl;
        if (!tsRestored)
            delay(5);
    }
    if (!tsRestored)
    {
        chargingSuspended = true;
        const bool stopped = pmic.enableModule(PmicAXP2101::Module::CELL_CHARGE, false);
        Serial.printf("[AXP2101] TS restore failed; charging disable=%s\n",
                      stopped ? "OK" : "FAIL");
        if (!stopped)
            pmic.power().softShutdown();
        return false;
    }
    if (chargingSuspended)
    {
        if (!pmic.enableModule(PmicAXP2101::Module::CELL_CHARGE, (moduleControl & 0x02) != 0))
        {
            Serial.println("[AXP2101] could not restore charger enable state");
            return false;
        }
        chargingSuspended = false;
    }

    auto *rails = pmic.getChannel();
    bool railsReady = true;
    for (uint8_t channel : {AXP2101Channel::CH_DCDC1})
    {
        const bool configured = rails->setVoltage(channel, 3300) && rails->enable(channel, true);
        railsReady = configured && railsReady;
        railsReady = rails->isEnabled(channel) && rails->getVoltage(channel) == 3300 && railsReady;
    }

    const bool watchdogDisabled = pmic.enableModule(PmicAXP2101::Module::WATCHDOG, false);
    const bool gaugeReady = pmic.enableModule(PmicAXP2101::Module::GAUGE, true);
    const bool batteryDetectReady = pmic.enableModule(PmicAXP2101::Module::BAT_DETECT, true);
    const auto channels = PmicAdcBase::Channel::BAT_VOLTAGE |
                          PmicAdcBase::Channel::VBUS_VOLTAGE |
                          PmicAdcBase::Channel::VSYS_VOLTAGE;
    const bool adcReady = pmic.adc().enableChannels(channels);
    delay(20);
    pmicReady = railsReady && gaugeReady && batteryDetectReady && adcReady;
    Serial.printf("[AXP2101] rails=%s watchdog=%s gauge=%s battery-detect=%s adc=%s\n",
                  railsReady ? "OK" : "FAIL", watchdogDisabled ? "off" : "FAIL",
                  gaugeReady ? "OK" : "FAIL", batteryDetectReady ? "OK" : "FAIL",
                  adcReady ? "OK" : "FAIL");
    return pmicReady && enablePeripheralRails(railMask);
}

const char *chargingStatusText(const BatteryTelemetry &telemetry)
{
    if (telemetry.fault)
        return "FAULT";
    if (!telemetry.batteryPresent)
        return "NO BAT";
    if (!telemetry.vbusPresent)
        return "NO VBUS";
    switch (telemetry.chargingStatusCode)
    {
    case 0:
        return "TRICKLE";
    case 1:
        return "PRECHARGE";
    case 2:
        return "CC";
    case 3:
        return "CV";
    case 4:
        return "DONE";
    case 5:
        return "IDLE";
    default:
        return "UNKNOWN";
    }
}

bool sampleBatteryTelemetry()
{
    batteryTelemetry = {};
    if (!initializeAXP2101())
        return false;

    auto *charger = pmic.getCharger();
    if (!charger)
        return false;

    const auto status = charger->getStatus();
    batteryTelemetry.available = status.online;
    batteryTelemetry.batteryPresent = status.batteryPresent;
    batteryTelemetry.vbusPresent = status.vbusPresent;
    batteryTelemetry.charging = status.charging;
    batteryTelemetry.chargeDone = status.chargeDone;
    batteryTelemetry.fault = status.fault;
    const int chargingStatus = pmic.core().readReg(axp2101_regs::bmu::STATUS2);
    if (chargingStatus >= 0)
    {
        batteryTelemetry.chargingStatusCode = static_cast<uint8_t>(chargingStatus) & 0x07;
        batteryTelemetry.charging = batteryTelemetry.batteryPresent &&
                                    batteryTelemetry.vbusPresent &&
                                    batteryTelemetry.chargingStatusCode <= 3;
        batteryTelemetry.chargeDone = batteryTelemetry.chargingStatusCode == 4;
    }
    batteryTelemetry.chargeVoltageMv = charger->getChargeVoltage();
    batteryTelemetry.fastChargeCurrentMa = charger->getFastChargeCurrent();

    auto &adc = pmic.adc();
    batteryTelemetry.capacityValid =
        adc.read(PmicAdcBase::Channel::BAT_PERCENTAGE, batteryTelemetry.capacityPercent);
    batteryTelemetry.voltageValid =
        adc.read(PmicAdcBase::Channel::BAT_VOLTAGE, batteryTelemetry.batteryVoltageMv);
    batteryTelemetry.vbusVoltageValid =
        adc.read(PmicAdcBase::Channel::VBUS_VOLTAGE, batteryTelemetry.vbusVoltageMv);
    batteryTelemetry.systemVoltageValid =
        adc.read(PmicAdcBase::Channel::VSYS_VOLTAGE, batteryTelemetry.systemVoltageMv);

    Serial.printf(
        "[AXP2101] battery=%s capacity=%.0f%% VBAT=%.0fmV VSYS=%.0fmV "
        "VBUS=%s %.0fmV IBAT/IBUS=N/A status=%s CV=%umV ICC=%umA fault=%s\n",
        batteryTelemetry.batteryPresent ? "CONNECTED" : "ABSENT",
        batteryTelemetry.capacityPercent, batteryTelemetry.batteryVoltageMv,
        batteryTelemetry.systemVoltageMv,
        batteryTelemetry.vbusPresent ? "PRESENT" : "ABSENT",
        batteryTelemetry.vbusVoltageMv,
        chargingStatusText(batteryTelemetry), batteryTelemetry.chargeVoltageMv,
        batteryTelemetry.fastChargeCurrentMa, batteryTelemetry.fault ? "YES" : "NO");
    return batteryTelemetry.available;
}

bool batteryTelemetryPasses()
{
    return batteryTelemetry.available && batteryTelemetry.batteryPresent &&
           batteryTelemetry.capacityValid && batteryTelemetry.capacityPercent >= 0.0f &&
           batteryTelemetry.capacityPercent <= 100.0f && batteryTelemetry.voltageValid &&
           batteryTelemetry.batteryVoltageMv >= 2500.0f &&
           batteryTelemetry.batteryVoltageMv <= 5000.0f &&
           batteryTelemetry.vbusVoltageValid && batteryTelemetry.systemVoltageValid &&
           batteryTelemetry.systemVoltageMv >= 2500.0f &&
           batteryTelemetry.systemVoltageMv <= 5500.0f &&
           batteryTelemetry.chargingStatusCode <= 5 &&
           batteryTelemetry.chargeVoltageMv != 0 && !batteryTelemetry.fault;
}

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool readRegister(uint8_t address, uint8_t reg, uint8_t &value)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
        return false;
    if (Wire.requestFrom(address, static_cast<uint8_t>(1)) != 1)
        return false;
    value = Wire.read();
    return true;
}

bool updateRegister(uint8_t address, uint8_t reg, uint8_t mask, uint8_t value)
{
    uint8_t current = 0;
    return readRegister(address, reg, current) &&
           writeRegister(address, reg, (current & ~mask) | (value & mask));
}

bool readPowerKeyConfig()
{
    powerKeyConfig = {};
    if (!initializeAXP2101())
        return false;

    // 禁止 PMIC 长按断电；软件休眠必须保持按键及 USB 唤醒路径。
    const bool powerOffWritten = writeRegister(Devices::AXP2101,
                                               axp2101_regs::pmu::PWROFF_EN, 0x04);

    uint8_t commonValue = 0;
    const bool commonRead = readRegister(Devices::AXP2101, axp2101_regs::ctrl::COMMON_CFG,
                                         commonValue);
    const bool commonWritten = commonRead &&
                               writeRegister(Devices::AXP2101, axp2101_regs::ctrl::COMMON_CFG,
                                             static_cast<uint8_t>(commonValue & 0xF0));

    const bool wakeWritten = writeRegister(Devices::AXP2101,
                                           axp2101_regs::pmu::SLEEP_WAKEUP_CTRL, 0x00);

    const bool onTimingWritten = pmic.pwron().setOnDurationMs(512);

    const bool offTimingWritten = pmic.pwron().setOffDurationMs(6000);
    const int offConfig = pmic.core().readReg(axp2101_regs::pmu::PWROFF_EN);
    const int commonConfig = pmic.core().readReg(axp2101_regs::ctrl::COMMON_CFG);
    const int wakeConfig = pmic.core().readReg(axp2101_regs::pmu::SLEEP_WAKEUP_CTRL);
    const int timing = pmic.core().readReg(axp2101_regs::pmu::IRQ_OFF_ON_LEVEL);
    if (!powerOffWritten || !commonWritten || !wakeWritten || !onTimingWritten ||
        !offTimingWritten ||
        offConfig < 0 || commonConfig < 0 || wakeConfig < 0 || timing < 0 ||
        !pmic.pwron().getOnDurationMs(powerKeyConfig.onDurationMs) ||
        !pmic.pwron().getOffDurationMs(powerKeyConfig.offDurationMs))
    {
        Serial.println("[POWER] failed to configure AXP2101 power-key registers");
        return false;
    }

    const bool longPressDisabled = (offConfig & 0x03) == 0x00;
    powerKeyConfig.registerValue = static_cast<uint8_t>(timing);
    constexpr uint64_t keyIrqs = AXP2101Irq::IRQ_PEKEY_SHORT_PRESS |
                                 AXP2101Irq::IRQ_PEKEY_LONG_PRESS |
                                 AXP2101Irq::IRQ_PEKEY_POSITIVE_EDGE |
                                 AXP2101Irq::IRQ_PEKEY_NEGATIVE_EDGE;

    const bool irqReady = pmic.irq().disable(AXP2101Irq::IRQ_ALL_MASK) &&
                          pmic.irq().clearStatus() && pmic.irq().enable(keyIrqs);
    powerKeyConfig.available = longPressDisabled;
    Serial.printf("[POWER] KEY4 GPIO%d hold %lums = deep sleep; PMIC long press "
                  "disabled; ON=%ums IRQ GPIO%d=%s; "
                  "REG10=0x%02X REG22=0x%02X REG26=0x%02X\n",
                  SHUTDOWN_KEY, static_cast<unsigned long>(POWER_KEY_SLEEP_HOLD_MS),
                  powerKeyConfig.onDurationMs, Pins::PMU_IRQ,
                  irqReady ? "OK" : "FAIL",
                  commonConfig, offConfig, wakeConfig);
    return powerKeyConfig.available;
}

void pollPmicIrq()
{
    if (!pmicReady || digitalRead(Pins::PMU_IRQ) != LOW)
        return;
    const uint64_t status = pmic.irq().readStatus();
    if (status == 0)
        return;
    Serial.printf("[AXP2101 IRQ] 0x%06lX KEY4=%s\n", static_cast<unsigned long>(status),
                  digitalRead(Pins::POWER_KEY) == LOW ? "DOWN" : "UP");
    if (AXP2101Irq::isPekeyLongPress(status))
    {
        Serial.println("[POWER] KEY4/PWRON long-press IRQ (PMIC power-off disabled)");
    }
}

bool configureStandbyPin(const StandbyPin &config)
{
    const gpio_num_t pin = static_cast<gpio_num_t>(config.pin);
    if (gpio_reset_pin(pin) != ESP_OK)
        return false;
    if (config.mode == GPIO_MODE_OUTPUT && gpio_set_level(pin, config.level) != ESP_OK)
        return false;
    return gpio_set_direction(pin, config.mode) == ESP_OK &&
           gpio_set_pull_mode(pin, config.pull) == ESP_OK;
}

void restoreStandbyPins()
{
    for (const auto &config : STANDBY_PINS)
    {

        configureStandbyPin(config);
        gpio_hold_dis(static_cast<gpio_num_t>(config.pin));
    }
    gpio_deep_sleep_hold_dis();
    rtc_gpio_deinit(static_cast<gpio_num_t>(Pins::POWER_KEY));
    rtc_gpio_deinit(static_cast<gpio_num_t>(Pins::PMU_IRQ));
}

esp_err_t configureStandbyWakePins()
{
    for (int pin : {Pins::POWER_KEY, Pins::PMU_IRQ})
    {
        const gpio_num_t wakePin = static_cast<gpio_num_t>(pin);
        esp_err_t error = rtc_gpio_init(wakePin);
        if (error == ESP_OK)
            error = rtc_gpio_set_direction(wakePin, RTC_GPIO_MODE_INPUT_ONLY);
        if (error == ESP_OK)
            error = rtc_gpio_pullup_en(wakePin);
        if (error == ESP_OK)
            error = rtc_gpio_pulldown_dis(wakePin);
        if (error != ESP_OK)
            return error;
    }
    return esp_sleep_enable_ext1_wakeup_io(STANDBY_WAKE_MASK,
                                           ESP_EXT1_WAKEUP_ANY_LOW);
}

void preparePeripheralSignalsForPowerOff()
{

    constexpr int outputsLow[] = {
        Pins::PA_ENABLE, Pins::WS2812_DATA, Pins::I2S_DOUT,
        Pins::I2S_MCLK, Pins::I2S_BCLK, Pins::I2S_WS,
        Pins::LCD_SCLK, Pins::LCD_MOSI, Pins::LCD_DC, Pins::LCD_RST,
        Pins::LCD_CS[0], Pins::LCD_CS[1], Pins::LCD_CS[2],
        Pins::LCD_CS[3], Pins::LCD_CS[4],
    };
    for (int pin : outputsLow)
    {
        gpio_set_level(static_cast<gpio_num_t>(pin), LOW);
        gpio_set_direction(static_cast<gpio_num_t>(pin), GPIO_MODE_OUTPUT);
        gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_FLOATING);
    }
    gpio_set_direction(static_cast<gpio_num_t>(Pins::I2S_DIN), GPIO_MODE_INPUT);
    gpio_set_pull_mode(static_cast<gpio_num_t>(Pins::I2S_DIN), GPIO_PULLDOWN_ONLY);
    gpio_set_direction(static_cast<gpio_num_t>(Pins::ES7210_INT), GPIO_MODE_INPUT);
    gpio_set_pull_mode(static_cast<gpio_num_t>(Pins::ES7210_INT), GPIO_PULLDOWN_ONLY);
}

bool powerDownPeripheralRails()
{
    auto *rails = pmic.getChannel();
    if (!pmicReady || !rails)
        return false;

    bool ready = true;
    for (const auto &rail : POWER_OFF_RAILS)
    {
        const bool disabled = rails->enable(rail.channel, false) &&
                              !rails->isEnabled(rail.channel);
        Serial.printf("[POWER] %s off=%s\n", rail.name, disabled ? "OK" : "FAIL");
        ready = disabled && ready;
    }

    const auto adcChannels = PmicAdcBase::Channel::BAT_VOLTAGE |
                             PmicAdcBase::Channel::VBUS_VOLTAGE |
                             PmicAdcBase::Channel::VSYS_VOLTAGE;
    bool adcDisabled = pmic.adc().disableChannels(adcChannels);
    uint8_t adcControl = 0xFF;
    adcDisabled = adcDisabled &&
                  readRegister(Devices::AXP2101, axp2101_regs::adc::CHANNEL_CTRL, adcControl) &&
                  (adcControl & 0x0D) == 0;
    Serial.printf("[POWER] VBAT/VBUS/VSYS ADC off=%s REG30=0x%02X\n",
                  adcDisabled ? "OK" : "FAIL", adcControl);
    return ready && adcDisabled;
}

bool isBoardI2cPin(int pin)
{
    return pin == Pins::BOARD_I2C_SCL || pin == Pins::BOARD_I2C_SDA;
}

bool holdStandbyPins(bool includeBoardI2c)
{
    for (const auto &config : STANDBY_PINS)
    {

        if (!includeBoardI2c && isBoardI2cPin(config.pin))
            continue;
        if (!configureStandbyPin(config))
        {
            Serial.printf("[POWER] GPIO%d standby configuration failed\n", config.pin);
            return false;
        }
        const esp_err_t error = gpio_hold_en(static_cast<gpio_num_t>(config.pin));
        if (error != ESP_OK)
        {
            Serial.printf("[POWER] GPIO%d hold failed: %s\n",
                          config.pin, esp_err_to_name(error));
            return false;
        }
    }
    gpio_deep_sleep_hold_en();
    return true;
}

bool waitForPowerKeyReleased()
{
    const uint32_t startedAt = millis();
    uint32_t releasedAt = 0;
    bool releaseTiming = false;

    while (millis() - startedAt < POWER_KEY_RELEASE_TIMEOUT_MS)
    {
        if (digitalRead(Pins::POWER_KEY) == LOW)
        {
            releaseTiming = false;
        }
        else if (!releaseTiming)
        {
            releaseTiming = true;
            releasedAt = millis();
        }
        else if (millis() - releasedAt >= POWER_KEY_RELEASE_STABLE_MS)
        {
            return true;
        }
        delay(5);
    }
    return false;
}

void releaseStandbyHolds()
{
    for (const auto &config : STANDBY_PINS)
        gpio_hold_dis(static_cast<gpio_num_t>(config.pin));
    gpio_deep_sleep_hold_dis();
    rtc_gpio_deinit(static_cast<gpio_num_t>(Pins::POWER_KEY));
    rtc_gpio_deinit(static_cast<gpio_num_t>(Pins::PMU_IRQ));
}

void cancelSleepRequest(const char *reason) {
    Serial.printf("[POWER] sleep cancelled: %s\n", reason);
    sleepRequested = false;
    shutdownKeyState.reset();
    if (sleepHooks.pending) sleepHooks.pending(false);
}

void recoverAfterFailedSleep(const char *reason) {
    const uint8_t restoreMask = enabledRailMask;
    releaseStandbyHolds();
    esp_sleep_disable_ext1_wakeup_io(0);
    Serial.begin(SERIAL_BAUD);
    Serial.setTxTimeoutMs(0);
    Wire.begin(Pins::BOARD_I2C_SDA, Pins::BOARD_I2C_SCL, I2C_FREQUENCY);
    pmicReady = false;
    if (!initializeAXP2101(restoreMask) || !readPowerKeyConfig())
        Serial.println("[POWER] recovery failed: PMIC/rails unavailable");
    pinMode(Pins::PA_ENABLE, OUTPUT);
    digitalWrite(Pins::PA_ENABLE, LOW);
    pinMode(Pins::POWER_KEY, INPUT_PULLUP);
    pinMode(Pins::PMU_IRQ, INPUT_PULLUP);
    pinMode(Pins::ES7210_INT, INPUT_PULLUP);
    if (sleepHooks.recover) sleepHooks.recover();
    Serial.printf("[POWER] deep-sleep aborted: %s\n", reason);
    sleepRequested = false;
    shutdownKeyState.reset();
    if (sleepHooks.pending) sleepHooks.pending(false);
}

void enterPowerKeyDeepSleep()
{

    if (digitalRead(SHUTDOWN_KEY) == LOW)
    {
        cancelSleepRequest("KEY4 is still low");
        return;
    }
    if (!waitForPowerKeyReleased())
    {
        cancelSleepRequest("KEY4 did not stay high");
        return;
    }

    // 先释放应用占用的外设，再保持 GPIO 并切断外围电源。
    if (sleepHooks.prepare) sleepHooks.prepare();
    preparePeripheralSignalsForPowerOff();

    if (!powerDownPeripheralRails())
        Serial.println("[POWER] some peripheral rails stayed on; continuing to sleep");

    const bool vbusWakeIrqReady = pmicReady && pmic.irq().disable(AXP2101Irq::IRQ_ALL_MASK) &&
                                  pmic.irq().clearStatus() &&
                                  pmic.irq().enable(AXP2101Irq::IRQ_VBUS_INSERT);
    Serial.printf("[POWER] PMIC VBUS-insert wake IRQ=%s\n",
                  vbusWakeIrqReady ? "ON" : "FAIL");

    Wire.end();

    esp_sleep_disable_ext1_wakeup_io(0);

    const esp_err_t wakeError = configureStandbyWakePins();
    if (wakeError == ESP_OK && holdStandbyPins(true))
    {
        Serial.printf("[POWER] deep sleep: DCDC1 stays on, other rails off; "
                      "press KEY4 GPIO%d or insert USB (PMU IRQ GPIO%d) to wake\n",
                      Pins::POWER_KEY, Pins::PMU_IRQ);
        Serial.flush();
        Serial.end();
        esp_deep_sleep_start();
        return;
    }

    Serial.printf("[POWER] deep-sleep setup failed (%s); restoring UI\n",
                  wakeError == ESP_OK ? "GPIO hold" : esp_err_to_name(wakeError));
    recoverAfterFailedSleep("deep-sleep setup failed");
}

void pollShutdownKey()
{

    const auto event = shutdownKeyState.update(digitalRead(SHUTDOWN_KEY) == LOW, millis(),
                                                POWER_KEY_SLEEP_HOLD_MS);
    if (event == PowerKeyStandby::Event::HOLD_REACHED)
    {
        sleepRequested = true;
        digitalWrite(Pins::PA_ENABLE, LOW);
        if (sleepHooks.pending) sleepHooks.pending(true);
        Serial.println("[POWER] KEY4 hold reached; release KEY4 to enter deep sleep");
    }
    else if (event == PowerKeyStandby::Event::SLEEP_READY)
    {
        enterPowerKeyDeepSleep();
    }
}

void printPowerDiagnostics()
{
    Serial.printf("[POWER] KEY4 GPIO%d=%d PMU_IRQ GPIO%d=%d wake-cause=%d key-config=%s\n",
                  Pins::POWER_KEY, digitalRead(Pins::POWER_KEY), Pins::PMU_IRQ,
                  digitalRead(Pins::PMU_IRQ), static_cast<int>(esp_sleep_get_wakeup_cause()),
                  powerKeyConfig.available ? "ready" : "unavailable");
    const uint8_t registers[] = {0x00, 0x01, 0x10, 0x18, 0x19, 0x20, 0x21, 0x22, 0x26, 0x27, 0x80, 0x90};
    for (uint8_t reg : registers)
    {
        uint8_t value = 0;
        if (readRegister(Devices::AXP2101, reg, value))
        {
            Serial.printf("[POWER] AXP2101 REG%02X=0x%02X\n", reg, value);
        }
        else
        {
            Serial.printf("[POWER] AXP2101 REG%02X read failed\n", reg);
        }
    }
}

String scanI2C(TwoWire &bus)
{
    String found;
    for (uint8_t address = 1; address < 0x7F; ++address)
    {
        if (!i2cPresent(bus, address))
            continue;
        char text[8];
        snprintf(text, sizeof(text), "0x%02X ", address);
        found += text;
    }
    return found;
}

bool isOnboardI2CAddress(uint8_t address)
{
    for (size_t i = 0; i < Devices::ONBOARD_I2C_ADDRESS_COUNT; ++i)
    {
        if (Devices::ONBOARD_I2C_ADDRESSES[i] == address)
            return true;
    }
    return false;
}

bool hasExternalI2CDevice(TwoWire &bus)
{
    for (uint8_t address = 1; address < 0x7F; ++address)
    {
        if (!isOnboardI2CAddress(address) && i2cPresent(bus, address))
            return true;
    }
    return false;
}

void printI2CScan()
{
    Serial.printf("[QWIIC I2C] CN4 shares onboard bus: SDA GPIO%d / SCL GPIO%d\n",
                  Pins::QWIIC_I2C_SDA, Pins::QWIIC_I2C_SCL);
    String found = scanI2C(Wire);
    Serial.printf("[QWIIC I2C] devices: %s\n", found.length() ? found.c_str() : "none");
    Serial.println("[QWIIC I2C] onboard addresses: 0x10 (ES8389), 0x34 (AXP2101), "
                   "0x40 (ES7210)");
}

const char *resetReasonText(esp_reset_reason_t reason)
{
    switch (reason)
    {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_EXT: return "external";
    case ESP_RST_SW: return "software";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "int-wdt";
    case ESP_RST_TASK_WDT: return "task-wdt";
    case ESP_RST_WDT: return "wdt";
    case ESP_RST_DEEPSLEEP: return "deep-sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
    }
}

bool powerKeyHeldForWake()
{

    const uint32_t startedAt = millis();
    while (millis() - startedAt < 120)
    {
        if (digitalRead(Pins::POWER_KEY) != LOW)
            return false;
        delay(5);
    }
    return true;
}

void reenterPowerKeyDeepSleep()
{
    esp_sleep_disable_ext1_wakeup_io(0);
    const esp_err_t error = configureStandbyWakePins();
    if (error != ESP_OK)
        return;
    holdStandbyPins(true);
    esp_deep_sleep_start();
}

bool beginBoard(uint8_t railMask) {
    restoreStandbyPins();
    const esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();
    const uint64_t ext1Status = wakeCause == ESP_SLEEP_WAKEUP_EXT1
                                    ? esp_sleep_get_ext1_wakeup_status()
                                    : 0;
    const bool wokeFromKey = (ext1Status & POWER_KEY_WAKE_MASK) != 0;
    const bool wokeFromVbus = (ext1Status & VBUS_IRQ_WAKE_MASK) != 0;
    esp_sleep_disable_ext1_wakeup_io(0);

    pinMode(Pins::POWER_KEY, INPUT_PULLUP);
    pinMode(Pins::PMU_IRQ, INPUT_PULLUP);
    if (wokeFromKey && !wokeFromVbus && !powerKeyHeldForWake())
    {
        ++spuriousWakeCount;
        reenterPowerKeyDeepSleep();
    }
    Serial.begin(SERIAL_BAUD);

    Serial.setTxTimeoutMs(0);
    delay(800);
    Serial.println("\nT-Keyboard Voice V0.3 board support");
    const char *bootSource = wokeFromVbus ? "USB/VBUS wake"
                             : wokeFromKey ? "KEY4 wake"
                                           : "cold/power-on";
    Serial.printf("[POWER] boot=%s reset=%s wake-cause=%d EXT1=0x%08lX\n",
                  bootSource,
                  resetReasonText(esp_reset_reason()),
                  static_cast<int>(wakeCause),
                  static_cast<unsigned long>(ext1Status));
    if (spuriousWakeCount != 0)
    {
        Serial.printf("[POWER] ignored %lu spurious KEY4 EXT1 wake(s) while asleep\n",
                      static_cast<unsigned long>(spuriousWakeCount));
        spuriousWakeCount = 0;
    }
    Serial.printf("[MEMORY] PSRAM: %lu bytes\n",
                  static_cast<unsigned long>(ESP.getPsramSize()));

    pinMode(Pins::PA_ENABLE, OUTPUT);
    digitalWrite(Pins::PA_ENABLE, LOW);
    pinMode(Pins::PMU_IRQ, INPUT_PULLUP);
    pinMode(Pins::AUX_DETECT, INPUT);
    pinMode(Pins::ES7210_INT, INPUT_PULLUP);
    Wire.begin(Pins::BOARD_I2C_SDA, Pins::BOARD_I2C_SCL, I2C_FREQUENCY);
    const bool powerReady = initializeAXP2101(railMask);

    uint8_t recoveredOffConfig = 0;
    uint8_t recoveredTiming = 0;
    if (readRegister(Devices::AXP2101, axp2101_regs::pmu::PWROFF_EN, recoveredOffConfig) &&
        readRegister(Devices::AXP2101, axp2101_regs::pmu::IRQ_OFF_ON_LEVEL, recoveredTiming))
    {
        Serial.printf("[POWER] PMIC before reconfig REG22=0x%02X REG27=0x%02X\n",
                      recoveredOffConfig, recoveredTiming);
    }
    const bool batteryConnected = pmicReady && pmic.isBatteryConnect();
    Serial.printf("[POWER] battery=%s\n", batteryConnected ? "connected" : "ABSENT");
    if (!batteryConnected)
    {
        Serial.println("[POWER] WARNING: USB-only test. Deep sleep keeps DCDC1 on so "
                       "KEY4 wake works, but the PMIC power path is not exercised; "
                       "connect a battery for full power validation.");
    }
    // 即使某条外设电源轨异常，仍尽量完成 PMIC 电源键安全配置。
    const bool keyReady = pmicReady && readPowerKeyConfig();
    return powerReady && keyReady;
}

}  // namespace KeyboardVoice
