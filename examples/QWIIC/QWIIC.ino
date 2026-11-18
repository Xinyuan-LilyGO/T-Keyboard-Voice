#include "VoicePower.h"
#include "DemoConsole.h"
using namespace KeyboardVoice;

DemoConsole console;
bool ready = false;
uint32_t lastSend = 0;

void setup() {
    // 使板载音频器件上电，以便扫描时同时观察三个板载地址。
    ready = beginBoard(PowerRail::Audio);
    if (!ready) { Serial.println("ERROR: power initialization failed."); return; }
    Serial0.begin(QWIIC_UART_BAUD, SERIAL_8N1, Pins::QWIIC_UART_RX, Pins::QWIIC_UART_TX);
    Serial.println("QWIIC: scan / send / help; UART0 TX=43 RX=44 115200 8N1, 3.3V TTL.");
    printI2CScan();
}

void loop() {
    if (!ready) { delay(10); return; }
    String command;
    if (console.read(command)) {
        if (command == "scan") printI2CScan();
        else if (command == "send") Serial0.println("T-KeyboardVoice QWIIC UART0 test");
        else Serial.println("Commands: scan / send / help");
    }
    if (millis() - lastSend >= 2000) {
        lastSend = millis();
        Serial0.println("T-KeyboardVoice QWIIC UART0 test");
    }
    // 有界读取，不因 UART 持续流量阻塞 USB 控制台。
    static bool lineStart = true;
    for (size_t count = 0; count < 128 && Serial0.available(); ++count) {
        const int value = Serial0.read();
        if (value < 0) break;
        if (lineStart) { Serial.print("[UART0 RX] "); lineStart = false; }
        if (value == '\n') { Serial.println(); lineStart = true; }
        else if (value >= 32 && value <= 126) Serial.write(static_cast<uint8_t>(value));
        else if (value != '\r') Serial.printf("\\x%02X", value);
    }
    delay(2);
}
