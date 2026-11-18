#include <Arduino.h>
#include <math.h>
#include "VoicePower.h"
#include "VoiceDisplay.h"
#include "VoiceInput.h"
#include "VoiceRGB.h"
#include "VoiceAudio.h"
using namespace KeyboardVoice;

// 界面布局只属于 Factory，不进入公共硬件配置。
constexpr size_t MIC_WAVEFORM_SAMPLES = Pins::LCD_WIDTH;
constexpr int MIC_PLOT_TOP = 19;
constexpr int MIC_PLOT_BOTTOM = 105;
constexpr int MIC_PLOT_MIDDLE = (MIC_PLOT_TOP + MIC_PLOT_BOTTOM) / 2;
constexpr uint32_t MIC_WAVEFORM_INTERVAL_MS = 40;
static_assert(MIC_WAVEFORM_SAMPLES <= AUDIO_BLOCK_FRAMES, "Capture block must cover the waveform");

constexpr size_t TEST_COUNT = 18;
constexpr size_t AXP2101_TEST = 0;
constexpr size_t BATTERY_TEST = 1;
constexpr size_t PA_TEST = 6;
constexpr size_t MICROPHONE_TEST = 7;
constexpr size_t KEY_INPUT_TEST = 11;
constexpr size_t WS2812_TEST = 12;
constexpr size_t QWIIC_I2C_TEST = 13;
constexpr size_t QWIIC_UART_TEST = 14;
constexpr size_t EXTERNAL_MIC_TEST = 15;
constexpr size_t AUX_LOOPBACK_TEST = 16;
constexpr size_t ONBOARD_MIC_LOOPBACK_TEST = 17;
constexpr uint16_t BLACK = 0x0000;
constexpr uint16_t BLUE = 0x001F;
constexpr uint16_t RED = 0xF800;
constexpr uint16_t GREEN = 0x07E0;
constexpr uint16_t DARKGREEN = 0x03E0;
constexpr uint16_t ORANGE = 0xFD20;
constexpr uint16_t YELLOW = 0xFFE0;
constexpr uint16_t WHITE = 0xFFFF;

enum class Result : uint8_t
{
    UNTESTED,
    RUNNING,
    PASS,
    FAIL,
    MANUAL
};

struct TestState
{
    Result result = Result::UNTESTED;
    String detail;
};
struct MicrophoneWaveformState
{
    bool active = false;
    uint8_t readFailures = 0;
    uint32_t lastDrawMs = 0;
    uint32_t lastCaptureMs = 0;
    size_t frameCount = 0;
    int32_t displayPeak = 1024;
    float rms[MIC_CHANNEL_COUNT] = {};
    float blockRms[MIC_CHANNEL_COUNT] = {};
    float slotRms[4] = {};
    int32_t dcOffset[MIC_CHANNEL_COUNT] = {};
    int32_t rawPeak[MIC_CHANNEL_COUNT] = {};
    int32_t slotRawPeak[4] = {};
    size_t railSamples[MIC_CHANNEL_COUNT] = {};
    int16_t samples[MIC_CHANNEL_COUNT][MIC_WAVEFORM_SAMPLES] = {};
    int16_t previousY[MIC_CHANNEL_COUNT][MIC_WAVEFORM_SAMPLES - 1] = {};
    int16_t previousHeight[MIC_CHANNEL_COUNT][MIC_WAVEFORM_SAMPLES - 1] = {};
};
const char *const TEST_NAMES[] = {
    "AXP2101 scan",
    "Battery telemetry",
    "Power key/sleep",
    "ES8389 scan",
    "ES7210 scan",
    "Speaker output",
    "PA enable",
    "Microphone capture",
    "Headphone output",
    "LCD CS/display",
    "LCD backlight",
    "Five key inputs",
    "14x WS2812",
    "QWIIC I2C scan",
    "QWIIC serial log",
    "External mic / AEC",
    "MIC3 AUX loopback",
    "Dual onboard mic loopback",
};

const char *const TEST_GUIDES[] = {
    "Scan PMIC at I2C 0x34",
    "Read battery and charger data",
    "KEY4 hold 3s = sleep",
    "Scan codec at I2C 0x10",
    "Scan ADC at I2C 0x40",
    "Listen to 66.wav on speaker",
    "KEY2 toggles PA and plays",
    "Speak close to MIC1/MIC2",
    "Listen to 66.wav in headphones",
    "Check five colors and numbers",
    "Check fade, off and full",
    "Press KEY1 through KEY5",
    "Check all 14 RGB LEDs",
    "Scan shared bus; require external I2C address",
    "Check UART0 TX or send RX data",
    "Connect an external input to CN5",
    "Record MIC3 AUX for 1 s, then listen on speaker",
    "Record MIC1/MIC2 for 1 s, then listen on speaker",
};

static_assert(sizeof(TEST_NAMES) / sizeof(TEST_NAMES[0]) == TEST_COUNT, "Test name count must match TEST_COUNT");
static_assert(sizeof(TEST_GUIDES) / sizeof(TEST_GUIDES[0]) == TEST_COUNT, "Test guide count must match TEST_COUNT");

TestState states[TEST_COUNT];
size_t currentTest = 0;
int auxDetectRaw = HIGH;
int auxDetectLevel = HIGH;
uint32_t auxDetectChangedMs = 0;
uint32_t lastBatteryRefresh = 0;
MicrophoneWaveformState microphoneWaveform;
void setResult(size_t index, Result result, const String &detail);
void renderCurrentTest();
void runTest(size_t index);
void navigateTest(int direction);
bool isMicrophoneTest(size_t index)
{
    return index == MICROPHONE_TEST || index == EXTERNAL_MIC_TEST;
}

const char *resultText(Result result)
{
    switch (result)
    {
    case Result::RUNNING:
        return "RUN";
    case Result::PASS:
        return "PASS";
    case Result::FAIL:
        return "FAIL";
    case Result::MANUAL:
        return "CHECK";
    default:
        return "WAIT";
    }
}

void prepareDisplay(VoiceDisplay &display, uint16_t color)
{
    display.fillScreen(color);
    display.setTextWrap(true);
    display.setTextSize(1);
    display.setTextColor(BLACK, color);
}

void drawNavigationArrow(VoiceDisplay &display, bool left)
{
    prepareDisplay(display, WHITE);
    display.fillRect(34, 56, 60, 17, BLUE);
    if (left)
    {
        display.fillTriangle(18, 64, 48, 36, 48, 92, BLUE);
    }
    else
    {
        display.fillTriangle(110, 64, 80, 36, 80, 92, BLUE);
    }
    display.setTextColor(BLACK, WHITE);
    display.drawCenterString(left ? "KEY1" : "KEY5", Pins::LCD_WIDTH / 2, 106);
}

uint16_t resultColor(Result result)
{
    switch (result)
    {
    case Result::PASS:
        return DARKGREEN;
    case Result::FAIL:
        return RED;
    case Result::MANUAL:
        return ORANGE;
    default:
        return BLUE;
    }
}

void renderBatteryTelemetry()
{
    if (!displaysReady)
        return;
    const uint16_t statusColor = resultColor(states[BATTERY_TEST].result);

    prepareDisplay(display2, WHITE);
    display2.fillRect(0, 0, Pins::LCD_WIDTH, 22, statusColor);
    display2.setTextColor(states[BATTERY_TEST].result == Result::MANUAL ? BLACK : WHITE,
                          statusColor);
    display2.drawCenterString(resultText(states[BATTERY_TEST].result), Pins::LCD_WIDTH / 2, 6);
    display2.setTextColor(BLACK, WHITE);
    display2.drawCenterString("BATTERY", Pins::LCD_WIDTH / 2, 29);
    display2.setTextSize(batteryTelemetry.batteryPresent ? 2 : 1);
    display2.drawCenterString(batteryTelemetry.batteryPresent ? "CONNECTED" : "NOT CONNECTED",
                              Pins::LCD_WIDTH / 2, 48);
    display2.setTextSize(3);
    char capacity[16];
    if (batteryTelemetry.capacityValid)
    {
        snprintf(capacity, sizeof(capacity), "%.0f%%", batteryTelemetry.capacityPercent);
    }
    else
    {
        snprintf(capacity, sizeof(capacity), "--%%");
    }
    display2.drawCenterString(capacity, Pins::LCD_WIDTH / 2, 76);
    display2.setTextSize(1);
    display2.drawCenterString("KEY2: RERUN", Pins::LCD_WIDTH / 2, 114);

    prepareDisplay(display3, WHITE);
    display3.fillRect(0, 0, Pins::LCD_WIDTH, 22, BLUE);
    display3.setTextColor(WHITE, BLUE);
    display3.drawCenterString("BATTERY ADC", Pins::LCD_WIDTH / 2, 6);
    display3.setTextColor(BLACK, WHITE);
    display3.setTextSize(2);
    char value[24];
    snprintf(value, sizeof(value), "V %.0fmV",
             batteryTelemetry.voltageValid ? batteryTelemetry.batteryVoltageMv : 0.0f);
    display3.drawCenterString(value, Pins::LCD_WIDTH / 2, 37);
    snprintf(value, sizeof(value), "S %.0fmV",
             batteryTelemetry.systemVoltageValid ? batteryTelemetry.systemVoltageMv : 0.0f);
    display3.drawCenterString(value, Pins::LCD_WIDTH / 2, 70);
    display3.setTextSize(1);
    display3.drawCenterString("KEY3: PASS", Pins::LCD_WIDTH / 2, 114);

    prepareDisplay(display4, WHITE);
    display4.fillRect(0, 0, Pins::LCD_WIDTH, 22, BLUE);
    display4.setTextColor(WHITE, BLUE);
    display4.drawCenterString("CHARGER", Pins::LCD_WIDTH / 2, 6);
    display4.setTextColor(resultColor(states[BATTERY_TEST].result), WHITE);
    display4.setTextSize(2);
    display4.drawCenterString(chargingStatusText(batteryTelemetry), Pins::LCD_WIDTH / 2, 29);
    display4.setTextColor(BLACK, WHITE);
    display4.setTextSize(1);
    display4.setCursor(3, 58);
    display4.printf("VBUS: %s\n", batteryTelemetry.vbusPresent ? "PRESENT" : "ABSENT");
    display4.printf("IN: %.0fmV\n", batteryTelemetry.vbusVoltageMv);
    display4.print("IBAT/IBUS: N/A\n");
    display4.printf("CV:%umV ICC:%umA", batteryTelemetry.chargeVoltageMv,
                    batteryTelemetry.fastChargeCurrentMa);
    display4.setTextColor(RED, WHITE);
    display4.drawRightString("KEY4: FAIL", Pins::LCD_WIDTH - 3, 114);
}

void initializeMicrophoneWaveformPanel(VoiceDisplay &display, const char *title,
                                       uint16_t headerColor, const char *keyLabel)
{
    display.fillScreen(BLACK);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.fillRect(0, 0, Pins::LCD_WIDTH, MIC_PLOT_TOP, headerColor);
    display.setTextColor(headerColor == ORANGE ? BLACK : WHITE, headerColor);
    display.drawCenterString(title, Pins::LCD_WIDTH / 2, 5);
    display.fillRect(0, MIC_PLOT_BOTTOM + 1, Pins::LCD_WIDTH,
                     Pins::LCD_HEIGHT - MIC_PLOT_BOTTOM - 1, WHITE);
    display.setTextColor(BLACK, WHITE);
    display.drawCenterString("RMS 0", Pins::LCD_WIDTH / 2, MIC_PLOT_BOTTOM + 3);
    display.drawCenterString(keyLabel, Pins::LCD_WIDTH / 2, MIC_PLOT_BOTTOM + 13);
}

void renderAuxStatus()
{
    display2.fillRect(0, 56, Pins::LCD_WIDTH, 23, WHITE);
    display2.setTextSize(1);
    display2.setTextColor(BLACK, WHITE);
    display2.drawCenterString(auxDetectLevel == LOW ? "AUX INSERTED" : "AUX UNPLUGGED",
                              Pins::LCD_WIDTH / 2, 57);
    char text[24];
    snprintf(text, sizeof(text), "GPIO%d: %s", Pins::AUX_DETECT,
             auxDetectRaw == LOW ? "LOW" : "HIGH");
    display2.drawCenterString(text, Pins::LCD_WIDTH / 2, 68);
}

void renderMicrophoneTest()
{
    const bool external = currentTest == EXTERNAL_MIC_TEST;
    prepareDisplay(display2, WHITE);
    display2.fillRect(0, 0, Pins::LCD_WIDTH, 24, BLUE);
    display2.setTextColor(WHITE, BLUE);
    display2.setCursor(5, 7);
    display2.printf("TEST %02u/%02u", static_cast<unsigned>(currentTest + 1),
                    static_cast<unsigned>(TEST_COUNT));
    display2.setTextColor(BLACK, WHITE);
    display2.setTextSize(2);
    display2.drawCenterString(external ? "AUX / REF" : "DUAL MIC", Pins::LCD_WIDTH / 2, 34);
    display2.setTextSize(1);
    if (external)
    {
        renderAuxStatus();
    }
    else
    {
        display2.drawCenterString(microphoneWaveform.active ? "LIVE CAPTURE" : "CAPTURE STOPPED",
                                  Pins::LCD_WIDTH / 2, 61);
    }
    const uint16_t statusColor = resultColor(states[currentTest].result);
    display2.fillRect(18, 81, Pins::LCD_WIDTH - 36, 23, statusColor);
    display2.setTextColor(states[currentTest].result == Result::MANUAL ? BLACK : WHITE,
                          statusColor);
    display2.drawCenterString(resultText(states[currentTest].result),
                              Pins::LCD_WIDTH / 2, 89);
    display2.setTextColor(BLACK, WHITE);
    display2.drawCenterString("KEY2: RESTART", Pins::LCD_WIDTH / 2, 114);

    initializeMicrophoneWaveformPanel(display3, external ? "MIC3 AUX" : "MIC1 BOARD",
                                      BLUE, "KEY3: PASS");
    initializeMicrophoneWaveformPanel(display4, external ? "MIC4 AEC" : "MIC2 BOARD",
                                      ORANGE, "KEY4: FAIL");
    for (size_t channel = 0; channel < MIC_CHANNEL_COUNT; ++channel)
    {
        for (size_t x = 0; x < MIC_WAVEFORM_SAMPLES - 1; ++x)
        {
            microphoneWaveform.previousY[channel][x] = MIC_PLOT_MIDDLE;
            microphoneWaveform.previousHeight[channel][x] = 1;
        }
    }
}

void renderCurrentTest()
{
    if (!displaysReady)
        return;
    String detail = states[currentTest].detail;
    if (detail.length() > 44)
        detail = detail.substring(0, 44);
    String guide = TEST_GUIDES[currentTest];
    if (guide.length() > 44)
        guide = guide.substring(0, 44);

    drawNavigationArrow(display1, true);

    if (currentTest == BATTERY_TEST && batteryTelemetry.available)
    {
        renderBatteryTelemetry();
        drawNavigationArrow(display5, false);
        return;
    }

    if (isMicrophoneTest(currentTest))
    {
        renderMicrophoneTest();
        drawNavigationArrow(display5, false);
        return;
    }

    prepareDisplay(display2, WHITE);
    display2.fillRect(0, 0, Pins::LCD_WIDTH, 24, BLUE);
    display2.setTextColor(WHITE, BLUE);
    display2.setCursor(5, 7);
    display2.printf("TEST %02u/%02u", static_cast<unsigned>(currentTest + 1),
                    static_cast<unsigned>(TEST_COUNT));
    display2.setTextColor(BLACK, WHITE);
    display2.setTextSize(2);
    display2.setCursor(4, 34);
    display2.println(TEST_NAMES[currentTest]);
    if (currentTest == PA_TEST)
    {
        const bool enabled = digitalRead(Pins::PA_ENABLE) == HIGH;
        const uint16_t paColor = enabled ? DARKGREEN : RED;
        display2.fillRect(4, 70, Pins::LCD_WIDTH - 8, 32, paColor);
        display2.setTextColor(WHITE, paColor);
        display2.drawCenterString(enabled ? "PA: ON" : "PA: OFF", Pins::LCD_WIDTH / 2, 77);
    }
    display2.setTextSize(1);
    display2.setTextColor(BLACK, WHITE);
    display2.drawCenterString(currentTest == PA_TEST ? "KEY2: TOGGLE PA" : "KEY2: RERUN",
                              Pins::LCD_WIDTH / 2, 112);

    const uint16_t statusColor = resultColor(states[currentTest].result);
    prepareDisplay(display3, statusColor);
    const uint16_t statusTextColor = states[currentTest].result == Result::MANUAL ? BLACK : WHITE;
    display3.setTextColor(statusTextColor, statusColor);
    display3.drawCenterString("RESULT", Pins::LCD_WIDTH / 2, 18);
    display3.setTextSize(3);
    display3.drawCenterString(resultText(states[currentTest].result), Pins::LCD_WIDTH / 2, 48);
    display3.setTextSize(1);
    display3.drawCenterString("KEY3: PASS", Pins::LCD_WIDTH / 2, 108);

    prepareDisplay(display4, WHITE);
    display4.setTextColor(BLUE, WHITE);
    display4.setCursor(3, 4);
    display4.println("TEST GUIDE");
    display4.setTextColor(BLACK, WHITE);
    display4.setCursor(3, 20);
    display4.println(guide);
    display4.drawFastHLine(3, 65, Pins::LCD_WIDTH - 6, BLACK);
    display4.setTextColor(resultColor(states[currentTest].result), WHITE);
    display4.setCursor(3, 72);
    display4.println("DETAIL");
    display4.setTextColor(BLACK, WHITE);
    display4.setCursor(3, 87);
    display4.println(detail.length() ? detail : "Not tested");
    display4.setTextColor(RED, WHITE);
    display4.drawRightString(currentTest == 2 ? "HOLD KEY4: SLEEP" : "KEY4: FAIL",
                             Pins::LCD_WIDTH - 3, 115);

    drawNavigationArrow(display5, false);
}

void sendQwiicUartTestMessage()
{
    Serial0.println("T-KeyboardVoice QWIIC UART0 test");
    Serial0.flush();
    Serial.printf("[QWIIC UART0] sent test log at %lu baud on TX GPIO%d; RX GPIO%d\n",
                  static_cast<unsigned long>(QWIIC_UART_BAUD),
                  Pins::QWIIC_UART_TX, Pins::QWIIC_UART_RX);
}

int waveformY(int16_t sample, int32_t peak)
{
    const int32_t halfHeight = (MIC_PLOT_BOTTOM - MIC_PLOT_TOP) / 2 - 2;
    const int32_t scaled = static_cast<int32_t>(
        static_cast<int64_t>(sample) * halfHeight / (peak > 0 ? peak : 1));
    int y = MIC_PLOT_MIDDLE + scaled;
    if (y < MIC_PLOT_TOP)
        y = MIC_PLOT_TOP;
    if (y > MIC_PLOT_BOTTOM)
        y = MIC_PLOT_BOTTOM;
    return y;
}

void drawMicrophoneWaveform(VoiceDisplay &display, size_t channel, uint16_t color,
                            const char *keyLabel)
{
    display.startWrite();
    for (size_t x = 0; x < MIC_WAVEFORM_SAMPLES - 1; ++x)
    {
        display.writeFastVLine(static_cast<int32_t>(x),
                               microphoneWaveform.previousY[channel][x],
                               microphoneWaveform.previousHeight[channel][x], BLACK);
        display.writePixel(static_cast<int32_t>(x), MIC_PLOT_MIDDLE, 0x2945);

        int y1 = waveformY(microphoneWaveform.samples[channel][x],
                           microphoneWaveform.displayPeak);
        int y2 = waveformY(microphoneWaveform.samples[channel][x + 1],
                           microphoneWaveform.displayPeak);
        if (y1 > y2)
        {
            const int swap = y1;
            y1 = y2;
            y2 = swap;
        }
        const int height = y2 - y1 + 1;
        microphoneWaveform.previousY[channel][x] = y1;
        microphoneWaveform.previousHeight[channel][x] = height;
        display.writeFastVLine(static_cast<int32_t>(x), y1, height, color);
    }
    display.endWrite();

    display.fillRect(0, MIC_PLOT_BOTTOM + 1, Pins::LCD_WIDTH,
                     Pins::LCD_HEIGHT - MIC_PLOT_BOTTOM - 1, WHITE);
    display.setTextSize(1);
    display.setTextColor(BLACK, WHITE);
    char rmsText[20];
    snprintf(rmsText, sizeof(rmsText), "RMS %.0f", microphoneWaveform.rms[channel]);
    display.drawCenterString(rmsText, Pins::LCD_WIDTH / 2, MIC_PLOT_BOTTOM + 3);
    display.drawCenterString(keyLabel, Pins::LCD_WIDTH / 2, MIC_PLOT_BOTTOM + 13);
}

void pollAuxDetection()
{
    const uint32_t now = millis();
    const int raw = digitalRead(Pins::AUX_DETECT);
    bool changed = false;
    if (raw != auxDetectRaw)
    {
        auxDetectRaw = raw;
        auxDetectChangedMs = now;
        changed = true;
    }
    if (auxDetectLevel != auxDetectRaw && now - auxDetectChangedMs >= DEBOUNCE_MS)
    {
        auxDetectLevel = auxDetectRaw;
        changed = true;
        Serial.printf("[AUX] CN5 %s; GPIO%d=%s\n",
                      auxDetectLevel == LOW ? "inserted" : "unplugged",
                      Pins::AUX_DETECT, auxDetectLevel == LOW ? "LOW" : "HIGH");
    }
    if (changed && currentTest == EXTERNAL_MIC_TEST && displaysReady)
        renderAuxStatus();
}

void pollMicrophoneWaveforms()
{
    if (!isMicrophoneTest(currentTest))
        return;
    pollAuxDetection();
    if (!microphoneWaveform.active)
        return;
    const uint32_t now = millis();
    if (now - microphoneWaveform.lastDrawMs < MIC_WAVEFORM_INTERVAL_MS)
        return;
    microphoneWaveform.lastDrawMs = now;

    int16_t logicalSamples[MIC_WAVEFORM_SAMPLES * 4] = {};
    const size_t frameCount = readMicrophoneFrames(logicalSamples, MIC_WAVEFORM_SAMPLES);
    if (frameCount < 2) {
        if (++microphoneWaveform.readFailures >= 3) {
            microphoneWaveform.active = false;
            setResult(currentTest, Result::FAIL, "ES7210 capture timed out or returned partial frames");
        }
        return;
    }
    microphoneWaveform.readFailures = 0;
    microphoneWaveform.lastCaptureMs = millis();
    microphoneWaveform.frameCount = frameCount;

    for (size_t mic = 0; mic < 4; ++mic)
    {
        int64_t sum = 0;
        for (size_t frame = 0; frame < frameCount; ++frame)
            sum += logicalSamples[frame * 4 + mic];
        const int32_t mean = static_cast<int32_t>(sum / frameCount);
        uint64_t sumSquares = 0;
        int32_t peak = 0;
        for (size_t frame = 0; frame < frameCount; ++frame)
        {
            const int32_t raw = logicalSamples[frame * 4 + mic];
            const int32_t magnitude = raw < 0 ? -raw : raw;
            if (magnitude > peak)
                peak = magnitude;
            const int32_t centered = raw - mean;
            sumSquares += static_cast<uint64_t>(static_cast<int64_t>(centered) * centered);
        }
        microphoneWaveform.slotRms[mic] = sqrt(static_cast<double>(sumSquares) / frameCount);
        microphoneWaveform.slotRawPeak[mic] = peak;
    }
    int16_t interleaved[MIC_WAVEFORM_SAMPLES * MIC_CHANNEL_COUNT];
    const size_t channels[MIC_CHANNEL_COUNT] =
        {currentTest == EXTERNAL_MIC_TEST ? Es7210::MIC3 : Es7210::MIC1,
         currentTest == EXTERNAL_MIC_TEST ? Es7210::MIC4 : Es7210::MIC2};
    for (size_t frame = 0; frame < frameCount; ++frame)
    {
        for (size_t channel = 0; channel < MIC_CHANNEL_COUNT; ++channel)
        {
            const size_t mic = channels[channel];
            interleaved[frame * MIC_CHANNEL_COUNT + channel] =
                logicalSamples[frame * 4 + mic];
        }
    }

    int64_t sums[MIC_CHANNEL_COUNT] = {};
    for (size_t frame = 0; frame < frameCount; ++frame)
    {
        for (size_t channel = 0; channel < MIC_CHANNEL_COUNT; ++channel)
        {
            sums[channel] += interleaved[frame * MIC_CHANNEL_COUNT + channel];
        }
    }

    int32_t blockPeak = 1;
    for (size_t channel = 0; channel < MIC_CHANNEL_COUNT; ++channel)
    {
        const int32_t mean = static_cast<int32_t>(sums[channel] / frameCount);
        microphoneWaveform.dcOffset[channel] = mean;
        microphoneWaveform.rawPeak[channel] = 0;
        microphoneWaveform.railSamples[channel] = 0;
        uint64_t sumSquares = 0;
        for (size_t frame = 0; frame < frameCount; ++frame)
        {
            const int32_t raw = interleaved[frame * MIC_CHANNEL_COUNT + channel];
            const int32_t rawMagnitude = raw < 0 ? -raw : raw;
            if (rawMagnitude > microphoneWaveform.rawPeak[channel])
                microphoneWaveform.rawPeak[channel] = rawMagnitude;
            if (raw == INT16_MIN || raw == INT16_MAX)
                ++microphoneWaveform.railSamples[channel];
            const int32_t centered = raw - mean;
            sumSquares += static_cast<uint64_t>(static_cast<int64_t>(centered) * centered);
            const int32_t magnitude = centered < 0 ? -centered : centered;
            if (magnitude > blockPeak)
                blockPeak = magnitude;
        }

        const float blockRms = sqrt(static_cast<double>(sumSquares) / frameCount);
        microphoneWaveform.blockRms[channel] = blockRms;
        microphoneWaveform.rms[channel] = microphoneWaveform.rms[channel] == 0.0f
                                              ? blockRms
                                              : microphoneWaveform.rms[channel] * 0.75f +
                                                    blockRms * 0.25f;

        for (size_t x = 0; x < MIC_WAVEFORM_SAMPLES; ++x)
        {
            const size_t frame = x * (frameCount - 1) / (MIC_WAVEFORM_SAMPLES - 1);
            int32_t centered = interleaved[frame * MIC_CHANNEL_COUNT + channel] - mean;
            if (centered < INT16_MIN)
                centered = INT16_MIN;
            if (centered > INT16_MAX)
                centered = INT16_MAX;
            microphoneWaveform.samples[channel][x] = static_cast<int16_t>(centered);
        }
    }

    if (blockPeak > microphoneWaveform.displayPeak)
    {
        microphoneWaveform.displayPeak = blockPeak;
    }
    else
    {
        microphoneWaveform.displayPeak =
            (microphoneWaveform.displayPeak * 15 + blockPeak) / 16;
        if (microphoneWaveform.displayPeak < 256)
            microphoneWaveform.displayPeak = 256;
    }

    if (displaysReady)
    {
        drawMicrophoneWaveform(display3, 0, GREEN, "KEY3: PASS");
        drawMicrophoneWaveform(display4, 1, YELLOW, "KEY4: FAIL");
    }
}

void printMicrophoneDiagnostics()
{
    const uint8_t registers[] = {
        0x01, 0x06, 0x08, 0x10, 0x11, 0x12, 0x16, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x22,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C,
    };
    Serial.println("[MIC] ES7210 register readback:");
    for (const uint8_t reg : registers)
    {
        uint8_t actual = 0;
        if (readRegister(Devices::ES7210, reg, actual))
            Serial.printf("  0x%02X=0x%02X\n", reg, actual);
        else
            Serial.printf("  0x%02X=READ FAILED\n", reg);
    }
    if (!isMicrophoneTest(currentTest) || !microphoneWaveform.active ||
        microphoneWaveform.frameCount == 0)
    {
        Serial.printf("[MIC] No live samples; use 'run %u' or 'run %u', then 'mic'.\n",
                      static_cast<unsigned>(MICROPHONE_TEST + 1),
                      static_cast<unsigned>(EXTERNAL_MIC_TEST + 1));
        return;
    }

    const unsigned firstMic = currentTest == EXTERNAL_MIC_TEST ? 3 : 1;
    Serial.printf("[MIC] Last block: %u frames (%.2f ms), read %lu ms ago\n",
                  static_cast<unsigned>(microphoneWaveform.frameCount),
                  1000.0 * microphoneWaveform.frameCount / AUDIO_SAMPLE_RATE,
                  static_cast<unsigned long>(millis() - microphoneWaveform.lastCaptureMs));
    Serial.printf("[MIC] logical MIC1/2/3/4 RMS=%.1f/%.1f/%.1f/%.1f rawPeak=%ld/%ld/%ld/%ld\n",
                  microphoneWaveform.slotRms[0], microphoneWaveform.slotRms[1],
                  microphoneWaveform.slotRms[2], microphoneWaveform.slotRms[3],
                  static_cast<long>(microphoneWaveform.slotRawPeak[0]),
                  static_cast<long>(microphoneWaveform.slotRawPeak[1]),
                  static_cast<long>(microphoneWaveform.slotRawPeak[2]),
                  static_cast<long>(microphoneWaveform.slotRawPeak[3]));
    for (size_t channel = 0; channel < MIC_CHANNEL_COUNT; ++channel)
    {
        Serial.printf("[MIC%u] RMS=%.1f displayRMS=%.1f DC=%ld rawPeak=%ld rails=%u/%u\n",
                      firstMic + static_cast<unsigned>(channel),
                      microphoneWaveform.blockRms[channel], microphoneWaveform.rms[channel],
                      static_cast<long>(microphoneWaveform.dcOffset[channel]),
                      static_cast<long>(microphoneWaveform.rawPeak[channel]),
                      static_cast<unsigned>(microphoneWaveform.railSamples[channel]),
                      static_cast<unsigned>(microphoneWaveform.frameCount));
    }
    if (microphoneWaveform.blockRms[0] > 0.0f && microphoneWaveform.blockRms[1] > 0.0f)
    {
        const double ratio = microphoneWaveform.blockRms[1] / microphoneWaveform.blockRms[0];
        Serial.printf("[MIC] MIC%u/MIC%u=%.3f (%+.2f dB)\n",
                      firstMic + 1, firstMic, ratio, 20.0 * log10(ratio));
    }
    else
    {
        Serial.println("[MIC] Level difference: N/A (zero RMS)");
    }
    Serial.println("[MIC] Short sampled window; compare repeated readings with an equal-distance steady source.");
}

void logTest(size_t index)
{
    Serial.printf("[TEST %02u] %-20s : %-5s %s\n", static_cast<unsigned>(index + 1),
                  TEST_NAMES[index], resultText(states[index].result), states[index].detail.c_str());
}

void setResult(size_t index, Result result, const String &detail)
{
    states[index].result = result;
    states[index].detail = detail;
    logTest(index);
    if (index == currentTest)
        renderCurrentTest();
}

void updateBatteryTest(bool forceLog)
{
    const Result previous = states[BATTERY_TEST].result;
    Result result = Result::FAIL;
    String detail;

    if (!sampleBatteryTelemetry())
    {
        detail = "AXP2101 telemetry unavailable";
    }
    else if (!batteryTelemetry.batteryPresent)
    {
        detail = "Battery not connected";
    }
    else if (!batteryTelemetryPasses())
    {
        detail = "Battery telemetry invalid or PMIC fault";
    }
    else
    {
        result = Result::PASS;
        char text[64];
        snprintf(text, sizeof(text), "BAT %.0f%% %.0fmV SYS %.0fmV %s",
                 batteryTelemetry.capacityPercent, batteryTelemetry.batteryVoltageMv,
                 batteryTelemetry.systemVoltageMv, chargingStatusText(batteryTelemetry));
        detail = text;
    }

    states[BATTERY_TEST].result = result;
    states[BATTERY_TEST].detail = detail;
    if (forceLog || result != previous)
        logTest(BATTERY_TEST);
    if (currentTest == BATTERY_TEST)
        renderCurrentTest();
}

void togglePaAndPlayAudio()
{
    const bool enable = digitalRead(Pins::PA_ENABLE) == LOW;
    digitalWrite(Pins::PA_ENABLE, enable ? HIGH : LOW);
    delay(20);

    const bool played = playTestAudio();
    char detail[52];
    snprintf(detail, sizeof(detail), "PA %s; audio played; confirm speaker",
             enable ? "ON" : "OFF");
    Serial.printf("[PA] GPIO%d %s; playback=%s\n", Pins::PA_ENABLE,
                  enable ? "ON" : "OFF", played ? "OK" : "FAIL");
    setResult(PA_TEST, played ? Result::MANUAL : Result::FAIL,
              played ? detail : "PA toggled; audio playback failed");
}

void runTest(size_t index)
{
    if (sleepRequested || index >= TEST_COUNT)
        return;
    if (isMicrophoneTest(currentTest))
    {
        microphoneWaveform.active = false;
        if (audioCaptureMode)
        {
            audioReady = false;
            audio.end();
        }
    }
    if (currentTest == PA_TEST && index != PA_TEST)
    {
        digitalWrite(Pins::PA_ENABLE, LOW);
    }
    currentTest = index;
    setResult(index, Result::RUNNING, "Automatic test running...");

    switch (index)
    {
    case AXP2101_TEST:
    {
        const bool ready = initializeAXP2101(PowerRail::All);
        if (ready)
        {
            readPowerKeyConfig();
            if (!displaysReady)
                initializeDisplays();
        }
        setResult(index, ready ? Result::PASS : Result::FAIL,
                  ready ? "SensorLib initialized AXP2101 at 0x34"
                        : "SensorLib could not initialize AXP2101");
        break;
    }
    case BATTERY_TEST:
        updateBatteryTest(true);
        break;
    case 2:
        if (readPowerKeyConfig())
        {
            char detail[80];
            snprintf(detail, sizeof(detail), "KEY4 GPIO%d %s; %lums=sleep; ON=%ums; IRQ=%d",
                     Pins::POWER_KEY, digitalRead(Pins::POWER_KEY) ? "UP" : "DOWN",
                     static_cast<unsigned long>(POWER_KEY_SLEEP_HOLD_MS),
                     powerKeyConfig.onDurationMs, digitalRead(Pins::PMU_IRQ));
            setResult(index, Result::MANUAL, detail);
        }
        else
        {
            setResult(index, Result::FAIL, "AXP2101 PWRON timing unavailable");
        }
        break;
    case 3:
        setResult(index, i2cPresent(Devices::ES8389) ? Result::PASS : Result::FAIL,
                  i2cPresent(Devices::ES8389) ? "ES8389 found at 0x10" : "ES8389 0x10 not found");
        break;
    case 4:
        setResult(index, i2cPresent(Devices::ES7210) ? Result::PASS : Result::FAIL,
                  i2cPresent(Devices::ES7210) ? "ES7210 found at 0x40" : "ES7210 0x40 not found");
        break;
    case 5:
    {
        digitalWrite(Pins::PA_ENABLE, HIGH);
        const bool played = playTestAudio();
        setResult(index, played ? Result::MANUAL : Result::FAIL,
                  played ? "66.wav played; confirm speaker" : "audio playback failed");
        break;
    }
    case PA_TEST:
    {
        digitalWrite(Pins::PA_ENABLE, LOW);
        const bool ready = initializeAudio();
        setResult(index, ready ? Result::MANUAL : Result::FAIL,
                  ready ? "PA OFF; click KEY2 to toggle and play"
                        : "audio initialization failed");
        break;
    }
    case MICROPHONE_TEST:
    case EXTERNAL_MIC_TEST:
    {
        digitalWrite(Pins::PA_ENABLE, LOW);
        const bool ready = initializeAudio(index == MICROPHONE_TEST ? AudioMode::Onboard : AudioMode::FourChannel);
        microphoneWaveform = {};
        microphoneWaveform.active = ready;
        microphoneWaveform.lastDrawMs = millis() - MIC_WAVEFORM_INTERVAL_MS;
        microphoneWaveform.displayPeak = 1024;
        auxDetectRaw = digitalRead(Pins::AUX_DETECT);
        auxDetectLevel = auxDetectRaw;
        auxDetectChangedMs = millis();
        if (index == EXTERNAL_MIC_TEST)
        {
            Serial.printf("[AUX] CN5 %s; GPIO%d=%s; MIC3 input, MIC4 AEC reference; playback muted\n",
                          auxDetectLevel == LOW ? "inserted" : "unplugged", Pins::AUX_DETECT,
                          auxDetectRaw == LOW ? "LOW" : "HIGH");
        }
        setResult(index, ready ? Result::MANUAL : Result::FAIL,
                  ready ? (index == EXTERNAL_MIC_TEST ? "MIC3 AUX / MIC4 AEC; playback muted"
                        : "MIC1/MIC2 onboard live capture")
                        : "audio initialization failed");
        break;
    }
    case AUX_LOOPBACK_TEST:
    {

        Serial.printf("[AUX LOOPBACK] CN5 detect GPIO%d=%s; capturing MIC3\n",
                      Pins::AUX_DETECT,
                      digitalRead(Pins::AUX_DETECT) == LOW ? "LOW" : "HIGH");
        const bool played = recordAuxAndPlaySpeaker();
        setResult(index, played ? Result::MANUAL : Result::FAIL,
                  played ? "MIC3 AUX recorded and played; confirm speaker"
                         : "MIC3 AUX recording or speaker playback failed");
        break;
    }
    case ONBOARD_MIC_LOOPBACK_TEST:
    {
        const bool played = recordOnboardMicsAndPlaySpeaker();
        setResult(index, played ? Result::MANUAL : Result::FAIL,
                  played ? "MIC1/MIC2 recorded and played; confirm speaker"
                         : "MIC1/MIC2 recording or speaker playback failed");
        break;
    }
    case 8:
    {
        digitalWrite(Pins::PA_ENABLE, LOW);
        const bool played = playTestAudio();
        setResult(index, played ? Result::MANUAL : Result::FAIL,
                  played ? "PA off; confirm 66.wav in headphones"
                         : "audio playback failed");
        break;
    }
    case 9:
    {
        if (!displaysReady)
        {
            setResult(index, Result::FAIL, "Keyboard power/display initialization failed");
            break;
        }
        const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, YELLOW};
        for (size_t display = 0; display < Pins::LCD_COUNT; ++display)
        {
            displays[display].fillScreen(colors[display % 5]);
            displays[display].setTextColor(BLACK);
            displays[display].setTextSize(3);
            displays[display].setCursor(45, 48);
            displays[display].printf("%u", static_cast<unsigned>(display + 1));
        }
        delay(1800);
        setResult(index, Result::MANUAL, "5 CS pins colored/numbered; inspect LCDs");
        break;
    }
    case 10:
        if (!displaysReady)
        {
            setResult(index, Result::FAIL, "Keyboard power/display initialization failed");
            break;
        }
        for (auto &display : displays)
            display.fillScreen(WHITE);
        setBacklightBrightness(0);
        for (int brightness = 0; brightness <= 255; brightness += 5)
        {
            setBacklightBrightness(brightness);
            delay(10);
        }
        delay(500);
        setBacklightBrightness(0);
        delay(800);
        setBacklightBrightness(255);
        setResult(index, Result::MANUAL, "GPIO1 fade -> off -> full; inspect backlight");
        break;
    case KEY_INPUT_TEST:
    {
        char text[52];
        snprintf(text, sizeof(text), "Five-key seen mask 0x%02X", keySeenMask);
        setResult(index, keySeenMask == 0x1F ? Result::PASS : Result::MANUAL, text);
        break;
    }
    case WS2812_TEST:
        testWs2812();
        setResult(index, Result::MANUAL, "Check solid RGBW, 14-LED chase and RGB pattern");
        break;
    case QWIIC_I2C_TEST:
    {
        String devices = scanI2C(Wire);
        const bool externalFound = hasExternalI2CDevice(Wire);
        String missing;
        for (size_t i = 0; i < Devices::ONBOARD_I2C_ADDRESS_COUNT; ++i)
        {
            const uint8_t address = Devices::ONBOARD_I2C_ADDRESSES[i];
            if (i2cPresent(address))
                continue;
            if (missing.length())
                missing += ", ";
            char text[7];
            snprintf(text, sizeof(text), "0x%02X", address);
            missing += text;
        }
        String detail = devices.length() ? "Shared CN4/onboard bus " + devices
                                         : "No devices on shared CN4/onboard bus";
        if (missing.length())
            detail += "; missing onboard " + missing;
        else
            detail += "; onboard 0x10/0x34/0x40 present";
        detail += externalFound ? "; external address found" : "; no external address";
        setResult(index, externalFound ? Result::PASS : Result::FAIL, detail);
        break;
    }
    case QWIIC_UART_TEST:
        sendQwiicUartTestMessage();
        setResult(index, Result::MANUAL, "UART0 test log sent; RX mirrors to USB");
        break;
    default:
        break;
    }
}

void navigateTest(int direction)
{
    const int next = (static_cast<int>(currentTest) + direction + static_cast<int>(TEST_COUNT)) %
                     static_cast<int>(TEST_COUNT);
    runTest(static_cast<size_t>(next));
}

void printStatus()
{
    Serial.println("\nT-KeyboardVoice hardware test status");
    for (size_t i = 0; i < TEST_COUNT; ++i)
        logTest(i);
}

void printHelp()
{
    Serial.println("\nCommands:");
    Serial.printf("  run [1..%u]  run one test (no number: all tests)\n",
                  static_cast<unsigned>(TEST_COUNT));
    Serial.println("  next / prev  run adjacent test");
    Serial.println("  pass / fail  record manual result");
    Serial.println("  status       print all results");
    Serial.println("  scan         scan the QWIIC I2C connector");
    Serial.println("  power        print KEY4 levels and PMIC power registers");
    Serial.println("  mic          print microphone registers, RMS, level difference and clipping");
    Serial.println("  help         print this help");
}

void runAllTests()
{
    for (size_t i = 0; i < TEST_COUNT; ++i)
    {
        pollShutdownKey();
        if (sleepRequested)
            break;
        runTest(i);
        for (int pause = 0; pause < 25 && !sleepRequested; ++pause)
        {
            pollShutdownKey();
            delay(10);
        }
    }
    printStatus();
}

void handleCommand(String command)
{
    command.trim();
    command.toLowerCase();
    if (!command.length())
        return;
    if (command == "help" || command == "?")
    {
        printHelp();
    }
    else if (command == "status")
    {
        printStatus();
    }
    else if (command == "power")
    {
        printPowerDiagnostics();
    }
    else if (command == "mic")
    {
        printMicrophoneDiagnostics();
    }
    else if (command == "scan")
    {
        printI2CScan();
    }
    else if (command == "next")
    {
        navigateTest(1);
    }
    else if (command == "prev")
    {
        navigateTest(-1);
    }
    else if (command == "pass")
    {
        setResult(currentTest, Result::PASS, "operator confirmed");
    }
    else if (command == "fail")
    {
        setResult(currentTest, Result::FAIL, "operator rejected");
    }
    else if (command == "run")
    {
        runAllTests();
    }
    else if (command.startsWith("run "))
    {
        const int number = command.substring(4).toInt();
        if (number >= 1 && number <= static_cast<int>(TEST_COUNT))
            runTest(number - 1);
        else
            Serial.printf("Test number must be 1..%u\n", static_cast<unsigned>(TEST_COUNT));
    }
    else
    {
        Serial.println("Unknown command; enter 'help'");
    }
}

void pollSerial()
{
    static String command;
    if (sleepRequested)
        return;
    for (size_t count = 0; count < SERIAL_POLL_MAX_BYTES && Serial.available(); ++count)
    {
        const char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            if (command.length())
            {
                handleCommand(command);
                command = "";
                return;
            }
        }
        else if (command.length() < 80)
        {
            command += c;
        }
    }
}

void pollQwiicUart()
{
    static bool lineStart = true;
    for (size_t count = 0; count < SERIAL_POLL_MAX_BYTES && Serial0.available(); ++count)
    {
        const int value = Serial0.read();
        if (value < 0)
            break;
        const uint8_t byte = static_cast<uint8_t>(value);
        if (lineStart)
        {
            Serial.print("[QWIIC UART0 RX] ");
            lineStart = false;
        }
        if (byte == '\n')
        {
            Serial.println();
            lineStart = true;
        }
        else if (byte != '\r' && byte >= 0x20 && byte <= 0x7E)
        {
            Serial.write(byte);
        }
        else if (byte != '\r')
        {
            Serial.printf("\\x%02X", byte);
        }
    }
}

void pollKeys()
{
    keyManager.update();

    if (!powerKeyReleasedSinceBoot)
    {
        static bool releaseTiming = false;
        static uint32_t releasedAt = 0;
        if (digitalRead(Pins::POWER_KEY) == LOW || keys[3].isPressed())
        {
            releaseTiming = false;
        }
        else if (!releaseTiming)
        {
            releaseTiming = true;
            releasedAt = millis();
        }
        else if (millis() - releasedAt >= DEBOUNCE_MS)
        {
            powerKeyReleasedSinceBoot = true;
        }
    }

    for (size_t i = 0; i < 5; ++i)
    {
        auto &key = keys[i];
        const bool ignorePowerOnPress = i == 3 && !powerKeyReleasedSinceBoot;

        if (key.wasPressed())
        {
            Serial.printf("[KEY] K%u GPIO%d pressed\n", static_cast<unsigned>(i + 1), KEY_PINS[i]);
            keySeenMask |= 1U << i;

            if (currentTest == KEY_INPUT_TEST)
            {
                char detail[52];
                snprintf(detail, sizeof(detail), "Five-key seen mask 0x%02X", keySeenMask);
                setResult(currentTest, keySeenMask == 0x1F ? Result::PASS : Result::MANUAL, detail);
            }

            if (i == 0)
            {
                navigateTest(-1);
            }
            else if (i == 4)
            {
                navigateTest(1);
            }
            else if (currentTest != KEY_INPUT_TEST && currentTest != PA_TEST && i == 1)
            {
                runTest(currentTest);
            }
        }

        if (key.wasReleased())
        {
            Serial.printf("[KEY] K%u released\n", static_cast<unsigned>(i + 1));
        }
        if (key.wasClicked())
        {
            Serial.printf("[KEY] K%u clicked\n", static_cast<unsigned>(i + 1));
            if (currentTest == PA_TEST && i == 1)
            {
                togglePaAndPlayAudio();
            }
            if (i == 2 && currentTest != KEY_INPUT_TEST)
            {
                setResult(currentTest, Result::PASS, "Operator confirmed with KEY3 click");
            }
            if (i == 3 && !ignorePowerOnPress && currentTest != KEY_INPUT_TEST)
            {
                setResult(currentTest, Result::FAIL, "Operator rejected with KEY4 click");
            }
        }
        if (key.wasHold())
        {
            Serial.printf("[KEY] K%u hold\n", static_cast<unsigned>(i + 1));
        }
        if (i == 3 && key.wasReleased())
            powerKeyReleasedSinceBoot = true;
    }
}

void pollBatteryTelemetry()
{
    if (currentTest != BATTERY_TEST || millis() - lastBatteryRefresh < 1000)
        return;
    lastBatteryRefresh = millis();
    updateBatteryTest(false);
}

void prepareFactorySleep() {
    microphoneWaveform.active = false;
    suspendAudio();
    suspendDisplays();
    if (ws2812Ready) clearWs2812();
    deinitWs2812();
    Serial0.flush();
    Serial0.end();
}
void recoverFactory() {
    Serial0.begin(QWIIC_UART_BAUD, SERIAL_8N1, Pins::QWIIC_UART_RX, Pins::QWIIC_UART_TX);
    initializeDisplays();
    renderCurrentTest();
}
void dimFactory(bool pending) {
    if (displaysReady) setBacklightBrightness(pending ? 0 : 255);
}
void setup() {
    setSleepHooks({prepareFactorySleep, recoverFactory, dimFactory});
    beginBoard(PowerRail::All);
    Serial.println("T-Keyboard Voice V0.3 factory hardware test");
    Serial0.begin(QWIIC_UART_BAUD, SERIAL_8N1, Pins::QWIIC_UART_RX, Pins::QWIIC_UART_TX);
    initializeKeys();
    runTest(0);
    printPowerDiagnostics();
    printHelp();
    printI2CScan();
}

void loop()
{
    pollShutdownKey();
    if (sleepRequested)
    {
        delay(2);
        return;
    }
    pollPmicIrq();
    pollSerial();
    if (sleepRequested)
        return;
    pollQwiicUart();
    pollKeys();
    pollBatteryTelemetry();
    pollMicrophoneWaveforms();
    delay(2);
}
