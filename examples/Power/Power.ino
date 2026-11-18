#include "VoicePower.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;
uint32_t lastReport = 0;

void setup() {
    ready = beginBoard();
    Serial.println("Power: battery / power / scan / sleep / help");
    Serial.println("Hold KEY4 for 3 seconds, then release to sleep.");
    if (!ready) Serial.println("ERROR: PMIC initialization failed; inspect power and I2C.");
    printPowerDiagnostics();
}

void loop() {
    pollShutdownKey();
    if (sleepRequested) { delay(2); return; }
    pollPmicIrq();
    String command;
    if (console.read(command)) {
        if (command == "battery" && ready) sampleBatteryTelemetry();
        else if (command == "power") printPowerDiagnostics();
        else if (command == "scan") printI2CScan();
        else if (command == "sleep" && ready) enterPowerKeyDeepSleep();
        else Serial.println("Commands: battery / power / scan / sleep / help");
    }
    if (ready && millis() - lastReport >= 2000) {
        lastReport = millis();
        sampleBatteryTelemetry();
    }
    delay(2);
}
