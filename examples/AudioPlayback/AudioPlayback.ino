#include "VoicePower.h"
#include "VoiceAudio.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;

void playClip() {
    const bool success = playTestAudio();
    if (!success) digitalWrite(Pins::PA_ENABLE, LOW);
    Serial.printf("Playback %s; verify sound manually.\n", success ? "sent" : "FAILED");
}

void setup() {
    ready = beginBoard(PowerRail::Audio) && initializeAudio();
    Serial.println("AudioPlayback: speaker / headphone / play / pa on / pa off / stop");
    if (!ready) Serial.println("ERROR: power/codec/I2S initialization failed.");
}

void loop() {
    String command;
    if (console.read(command) && ready) {
        if (command == "speaker") { digitalWrite(Pins::PA_ENABLE, HIGH); playClip(); }
        else if (command == "headphone") { digitalWrite(Pins::PA_ENABLE, LOW); playClip(); }
        else if (command == "play") playClip();
        else if (command == "pa on") { digitalWrite(Pins::PA_ENABLE, HIGH); Serial.println("PA ON"); }
        else if (command == "pa off") { digitalWrite(Pins::PA_ENABLE, LOW); Serial.println("PA OFF"); }
        else if (command == "stop") {
            stopAudio();
            ready = initializeAudio();
            Serial.println("Stopped; PA muted.");
        } else Serial.println("Commands: speaker / headphone / play / pa on / pa off / stop");
    }
    delay(2);
}
