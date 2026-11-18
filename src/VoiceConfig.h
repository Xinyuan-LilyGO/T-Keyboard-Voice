#pragma once
#include "pin_config.h"

namespace KeyboardVoice {

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr size_t SERIAL_POLL_MAX_BYTES = 128;
constexpr uint32_t I2C_FREQUENCY = 400000;
constexpr uint32_t QWIIC_UART_BAUD = 115200;
constexpr uint32_t LCD_FREQUENCY = 4500000;
constexpr uint32_t AUDIO_SAMPLE_RATE = 48000;
constexpr uint32_t DEBOUNCE_MS = 35;
constexpr int SHUTDOWN_KEY = Pins::POWER_KEY;
constexpr uint32_t POWER_KEY_SLEEP_HOLD_MS = 3000;
constexpr uint32_t POWER_KEY_RELEASE_STABLE_MS = 300;
constexpr uint32_t POWER_KEY_RELEASE_TIMEOUT_MS = 1500;
constexpr uint64_t POWER_KEY_WAKE_MASK = 1ULL << Pins::POWER_KEY;
constexpr uint64_t VBUS_IRQ_WAKE_MASK = 1ULL << Pins::PMU_IRQ;
constexpr uint64_t STANDBY_WAKE_MASK = POWER_KEY_WAKE_MASK | VBUS_IRQ_WAKE_MASK;
constexpr size_t AUX_LOOPBACK_FRAMES = AUDIO_SAMPLE_RATE;
constexpr size_t ONBOARD_MIC_LOOPBACK_FRAMES = AUDIO_SAMPLE_RATE;
constexpr size_t MIC_CHANNEL_COUNT = 2;

constexpr int32_t LOOPBACK_HIGHPASS_ALPHA_Q15 = 32604;
constexpr int32_t LOOPBACK_GATE_OPEN = 220;
constexpr int32_t LOOPBACK_GATE_CLOSE = 100;
constexpr size_t LOOPBACK_GATE_HANGOVER_FRAMES = AUDIO_SAMPLE_RATE / 20;
constexpr size_t AUDIO_BLOCK_FRAMES = 128;

}  // namespace KeyboardVoice
