#include <math.h>
#include "VoicePower.h"
#include "VoiceAudio.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;
bool capturing = false;
AudioMode captureMode = AudioMode::Onboard;
uint32_t lastReport = 0;
uint8_t failures = 0;

void setup() {
    ready = beginBoard(PowerRail::Audio);
    if (ready) capturing = initializeAudio(captureMode);
    Serial.println("AudioRecord: onboard / external / loop onboard / loop aux / mic / stop");
    Serial.println("MIC1/2=onboard, MIC3=AUX, MIC4=AEC reference; not echo cancellation.");
    if (!ready || !capturing) Serial.println("ERROR: power/ADC/I2S initialization failed.");
}

void loop() {
    if (!ready) { delay(10); return; }
    String command;
    if (console.read(command)) {
        if (command == "onboard" || command == "external") {
            captureMode = command == "onboard" ? AudioMode::Onboard : AudioMode::FourChannel;
            capturing = initializeAudio(captureMode);
            failures = 0;
            if (!capturing) Serial.println("Capture initialization FAILED.");
        } else if (command == "loop onboard" || command == "loop aux") {
            capturing = false;
            const bool success = command == "loop aux" ? recordAuxAndPlaySpeaker() : recordOnboardMicsAndPlaySpeaker();
            Serial.printf("Record/playback %s; listen to both speakers.\n", success ? "sent" : "FAILED");
            // 回放结束后重新静音并恢复之前选择的采集模式。
            capturing = initializeAudio(captureMode);
            failures = 0;
            if (!capturing) Serial.println("Capture restore FAILED.");
        } else if (command == "mic") printCodecDiagnostics();
        else if (command == "stop") { capturing = false; stopAudio(); }
        else Serial.println("Commands: onboard / external / loop onboard / loop aux / mic / stop");
    }
    if (!capturing) { delay(2); return; }
    int16_t samples[AUDIO_BLOCK_FRAMES * 4];
    const size_t frames = readMicrophoneFrames(samples, AUDIO_BLOCK_FRAMES);
    if (!frames) {
        if (++failures >= 3) {
            stopAudio();
            capturing = false;
            Serial.println("Capture FAILED: timeout/partial frames. Use onboard/external to retry.");
        }
        return;
    }
    failures = 0;
    if (millis() - lastReport < 500) return;
    lastReport = millis();
    Serial.printf("[MIC] %u frames CN5 GPIO%d=%d", static_cast<unsigned>(frames), Pins::AUX_DETECT, digitalRead(Pins::AUX_DETECT));
    for (size_t mic = 0; mic < 4; ++mic) {
        int64_t sum = 0;
        int32_t peak = 0;
        for (size_t frame = 0; frame < frames; ++frame) {
            const int32_t value = samples[frame * 4 + mic];
            sum += value;
            peak = max(peak, abs(value));
        }
        const int32_t dc = static_cast<int32_t>(sum / frames);
        uint64_t squares = 0;
        for (size_t frame = 0; frame < frames; ++frame) {
            const int32_t value = samples[frame * 4 + mic] - dc;
            squares += static_cast<uint64_t>(static_cast<int64_t>(value) * value);
        }
        Serial.printf(" MIC%u rms=%.1f peak=%ld dc=%ld", static_cast<unsigned>(mic + 1), sqrt(static_cast<double>(squares) / frames), static_cast<long>(peak), static_cast<long>(dc));
    }
    Serial.println();
}
