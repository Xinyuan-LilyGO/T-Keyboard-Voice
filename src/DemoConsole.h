#pragma once
#include <Arduino.h>

namespace KeyboardVoice {

// 有界、非阻塞的串口行读取器；超长行整行丢弃，避免误执行截断命令。
class DemoConsole {
 public:
    bool read(String &command) {
        for (size_t count = 0; count < 128 && Serial.available(); ++count) {
            const char ch = static_cast<char>(Serial.read());
            if (ch == '\r' || ch == '\n') {
                if (overflow_) {
                    overflow_ = false;
                    line_ = "";
                    Serial.println("Command too long; discarded.");
                    continue;
                }
                if (!line_.length()) continue;
                command = line_;
                line_ = "";
                command.trim();
                command.toLowerCase();
                if (command.length()) return true;
            } else if (!overflow_) {
                if (line_.length() < 80) line_ += ch;
                else overflow_ = true;
            }
        }
        return false;
    }

 private:
    String line_;
    bool overflow_ = false;
};

}  // namespace KeyboardVoice
