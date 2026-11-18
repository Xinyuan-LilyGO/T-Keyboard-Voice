#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "power_key_standby.h"

using Event = PowerKeyStandby::Event;

void expect(PowerKeyStandby &key, bool pressed, uint32_t now, uint32_t holdMs,
            Event event = Event::NONE) {
  assert(key.update(pressed, now, holdMs) == event);
}

void bootPressMustBeReleased() {
  PowerKeyStandby key;
  expect(key, true, 0, 1000);
  expect(key, true, 10000, 1000);
  assert(!key.pending());
  expect(key, false, 10100, 1000);
  expect(key, true, 10120, 1000);
  expect(key, false, 10130, 1000);
  expect(key, false, 10164, 1000);
  expect(key, false, 10165, 1000);
  expect(key, true, 10200, 1000);
  expect(key, true, 11234, 1000);
  expect(key, true, 11235, 1000, Event::HOLD_REACHED);
}

void alreadyReleasedBootAcceptsFirstHold() {
  PowerKeyStandby key;
  expect(key, false, 100, 1000);
  expect(key, false, 135, 1000);
  expect(key, true, 200, 1000);
  expect(key, true, 234, 1000);
  expect(key, true, 235, 1000);
  expect(key, true, 1234, 1000);
  expect(key, true, 1235, 1000, Event::HOLD_REACHED);
  assert(key.pending());
  expect(key, false, 1300, 1000);
  expect(key, false, 1334, 1000);
  expect(key, false, 1335, 1000, Event::SLEEP_READY);
  assert(!key.pending());
}

void shortPressAndPressBounceDoNotAccumulate() {
  PowerKeyStandby key;
  expect(key, false, 0, 1000);
  expect(key, false, 35, 1000);
  expect(key, true, 100, 1000);
  expect(key, false, 130, 1000);
  expect(key, true, 140, 1000);
  expect(key, true, 1135, 1000);
  expect(key, false, 1174, 1000);
  expect(key, false, 2000, 1000);
  assert(!key.pending());
  expect(key, true, 2100, 1000);
  expect(key, true, 3134, 1000);
  expect(key, true, 3135, 1000, Event::HOLD_REACHED);
}

void sparsePollingHonorsDebouncedThreshold() {
  PowerKeyStandby key;
  expect(key, false, 0, 1000);
  expect(key, false, 35, 1000);
  expect(key, true, 50, 1000);
  expect(key, true, 1084, 1000);
  expect(key, true, 1085, 1000, Event::HOLD_REACHED);
}

void repressRestartsReleaseDebounce() {
  PowerKeyStandby key;
  expect(key, false, 0, 100);
  expect(key, false, 35, 100);
  expect(key, true, 50, 100);
  expect(key, true, 185, 100, Event::HOLD_REACHED);
  expect(key, false, 200, 100);
  expect(key, false, 234, 100);
  expect(key, true, 235, 100);
  assert(key.pending());
  expect(key, false, 250, 100);
  expect(key, false, 284, 100);
  expect(key, false, 285, 100, Event::SLEEP_READY);
}

void eventsAreOneShotAndSleepResets() {
  PowerKeyStandby key;
  expect(key, false, 0, 100);
  expect(key, false, 35, 100);
  expect(key, true, 50, 100);
  expect(key, true, 185, 100, Event::HOLD_REACHED);
  expect(key, true, 10000, 100);
  expect(key, false, 10001, 100);
  expect(key, false, 10036, 100, Event::SLEEP_READY);
  expect(key, true, 10040, 100);
  expect(key, true, 11000, 100);
  expect(key, false, 11010, 100);
  expect(key, false, 11045, 100);
  expect(key, true, 11100, 100);
  expect(key, true, 11235, 100, Event::HOLD_REACHED);
  key.reset();
  assert(!key.pending());
  expect(key, true, 12000, 100);
  expect(key, false, 12010, 100);
  expect(key, false, 12045, 100);
}

void zeroHoldDisablesAndCancelsPendingSleep() {
  PowerKeyStandby key;
  expect(key, false, 0, 0);
  expect(key, false, 35, 0);
  expect(key, true, 50, 0);
  expect(key, true, 10000, 0);
  expect(key, false, 11000, 100);
  expect(key, false, 11035, 100);
  expect(key, true, 11050, 100);
  expect(key, true, 11185, 100, Event::HOLD_REACHED);
  expect(key, false, 11200, 0);
  assert(!key.pending());
  expect(key, false, 11235, 0);
}

void timersSurviveMillisWrap() {
  PowerKeyStandby key;
  const uint32_t base = UINT32_MAX - 200;
  expect(key, false, base, 100);
  expect(key, false, base + 35, 100);
  expect(key, true, base + 50, 100);
  expect(key, true, base + 184, 100);
  expect(key, true, base + 185, 100, Event::HOLD_REACHED);
  expect(key, false, UINT32_MAX - 10, 100);
  expect(key, false, 23, 100);
  expect(key, false, 24, 100, Event::SLEEP_READY);

  key.reset();
  const uint32_t pressBase = UINT32_MAX - 150;
  expect(key, false, pressBase, 100);
  expect(key, false, pressBase + 35, 100);
  expect(key, true, pressBase + 50, 100);
  expect(key, true, pressBase + 184, 100);
  expect(key, true, pressBase + 185, 100, Event::HOLD_REACHED);

  key.reset();
  expect(key, false, UINT32_MAX - 10, 100);
  expect(key, false, 24, 100);
  expect(key, true, 50, 100);
  expect(key, true, 185, 100, Event::HOLD_REACHED);
}

void customDebounceAndLargeHoldDoNotOverflow() {
  PowerKeyStandby key(0);
  expect(key, false, 0, 1);
  expect(key, true, 10, 1);
  expect(key, true, 11, 1, Event::HOLD_REACHED);
  expect(key, false, 12, 1, Event::SLEEP_READY);

  PowerKeyStandby longHold;
  expect(longHold, false, 0, UINT32_MAX);
  expect(longHold, false, 35, UINT32_MAX);
  expect(longHold, true, 50, UINT32_MAX);
  expect(longHold, true, 85, UINT32_MAX);
  assert(!longHold.pending());
}

int main() {
  bootPressMustBeReleased();
  alreadyReleasedBootAcceptsFirstHold();
  shortPressAndPressBounceDoNotAccumulate();
  sparsePollingHonorsDebouncedThreshold();
  repressRestartsReleaseDebounce();
  eventsAreOneShotAndSleepResets();
  zeroHoldDisablesAndCancelsPendingSleep();
  timersSurviveMillisWrap();
  customDebounceAndLargeHoldDoNotOverflow();
  puts("Power key boot release, debounce, hold, one-shot and millis wrap: PASS");
}
