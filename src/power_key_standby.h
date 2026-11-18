#pragma once

#include <stdint.h>

class PowerKeyStandby {
 public:
  enum class Event { NONE, HOLD_REACHED, SLEEP_READY };

  explicit PowerKeyStandby(uint32_t debounceMs = 35) : debounceMs_(debounceMs) {}

  Event update(bool pressed, uint32_t now, uint32_t holdMs) {
    if (holdMs == 0) {
      reset();
      return Event::NONE;
    }

    if (stage_ == Stage::BOOT_RELEASE || stage_ == Stage::SLEEP_RELEASE) {
      if (pressed) {
        releaseTiming_ = false;
        return Event::NONE;
      }
      if (!releaseTiming_) {
        releaseTiming_ = true;
        transitionAt_ = now;
      }
      if (static_cast<uint32_t>(now - transitionAt_) < debounceMs_) return Event::NONE;
      if (stage_ == Stage::SLEEP_RELEASE) {
        reset();
        return Event::SLEEP_READY;
      }
      stage_ = Stage::IDLE;
      releaseTiming_ = false;
      return Event::NONE;
    }

    if (stage_ == Stage::IDLE) {
      if (pressed) {
        stage_ = Stage::PRESS;
        transitionAt_ = now;
      }
      return Event::NONE;
    }

    if (!pressed) {
      stage_ = Stage::IDLE;
      return Event::NONE;
    }

    const uint32_t elapsed = now - transitionAt_;
    if (elapsed >= debounceMs_ && elapsed - debounceMs_ >= holdMs) {
      stage_ = Stage::SLEEP_RELEASE;
      releaseTiming_ = false;
      return Event::HOLD_REACHED;
    }
    return Event::NONE;
  }

  void reset() {
    stage_ = Stage::BOOT_RELEASE;
    releaseTiming_ = false;
    transitionAt_ = 0;
  }

  bool pending() const { return stage_ == Stage::SLEEP_RELEASE; }

 private:
  enum class Stage { BOOT_RELEASE, IDLE, PRESS, SLEEP_RELEASE };

  uint32_t debounceMs_;
  uint32_t transitionAt_ = 0;
  Stage stage_ = Stage::BOOT_RELEASE;
  bool releaseTiming_ = false;
};
