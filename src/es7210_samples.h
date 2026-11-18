#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Es7210 {

constexpr size_t MIC1 = 0;
constexpr size_t MIC2 = 1;
constexpr size_t MIC3 = 2;
constexpr size_t MIC4 = 3;

constexpr size_t AUX_LOOPBACK_CHANNEL = MIC3;
constexpr size_t ONBOARD_MIC1_CHANNEL = MIC1;
constexpr size_t ONBOARD_MIC2_CHANNEL = MIC2;

constexpr int16_t sampleFromStereoWords(uint32_t left, uint32_t right, size_t channel) {
  if (channel >= 4) return 0;
  const uint32_t word = (channel & 1U) ? right : left;
  const uint32_t bits = (channel < 2 ? word >> 16 : word) & 0xFFFFU;
  return static_cast<int16_t>(bits < 0x8000U ? static_cast<int32_t>(bits)
                                           : static_cast<int32_t>(bits) - 0x10000);
}
}
