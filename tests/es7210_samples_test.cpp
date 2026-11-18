#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "es7210_samples.h"

struct Frame {
  uint32_t left;
  uint32_t right;
  int16_t microphones[4];
};


constexpr Frame frames[] = {
    {0x12345678, 0x23456789, {0x1234, 0x2345, 0x5678, 0x6789}},
    {0x7FFF8000, 0xFFFF0000, {32767, -1, -32768, 0}},
    {0x8000FFFF, 0x00017FFF, {-32768, 1, -1, 32767}},
    {0x00010000, 0x00000000, {1, 0, 0, 0}},
    {0x00000000, 0x00010000, {0, 1, 0, 0}},
    {0x00000001, 0x00000000, {0, 0, 1, 0}},
    {0x00000000, 0x00000001, {0, 0, 0, 1}},
};

static_assert(Es7210::sampleFromStereoWords(0x7FFF8000, 0xFFFF0000, 0) == 32767,
              "MIC1 must occupy the first left slot");
static_assert(Es7210::sampleFromStereoWords(0x7FFF8000, 0xFFFF0000, 2) == -32768,
              "MIC3 must occupy the second left slot with signed decoding");
static_assert(Es7210::MIC1 == 0 && Es7210::MIC2 == 1 && Es7210::MIC3 == 2 &&
                  Es7210::MIC4 == 3,
              "logical microphone channels must follow the ES7210 wire decoder");
static_assert(Es7210::AUX_LOOPBACK_CHANNEL == Es7210::MIC3 &&
                  Es7210::ONBOARD_MIC1_CHANNEL == Es7210::MIC1 &&
                  Es7210::ONBOARD_MIC2_CHANNEL == Es7210::MIC2,
              "loopback inputs must use logical microphones from packed stereo words");

int main() {
  for (const auto &frame : frames) {
    for (size_t mic = 0; mic < 4; ++mic) {
      assert(Es7210::sampleFromStereoWords(frame.left, frame.right, mic) ==
             frame.microphones[mic]);
    }
  }


  constexpr uint32_t onlyMic3Left = 0x00001234;
  assert(Es7210::sampleFromStereoWords(onlyMic3Left, 0, Es7210::MIC3) ==
         0x1234);
  assert(Es7210::sampleFromStereoWords(onlyMic3Left, 0, Es7210::ONBOARD_MIC1_CHANNEL) == 0);
  assert(Es7210::sampleFromStereoWords(onlyMic3Left, 0, Es7210::MIC2) == 0);


  constexpr uint32_t onlyMic2Right = 0x56780000;
  assert(Es7210::sampleFromStereoWords(0, onlyMic2Right, Es7210::MIC3) == 0);
  assert(Es7210::sampleFromStereoWords(0, onlyMic2Right, Es7210::MIC2) ==
         0x5678);
  assert(Es7210::sampleFromStereoWords(UINT32_MAX, UINT32_MAX, 4) == 0);
  assert(Es7210::sampleFromStereoWords(UINT32_MAX, UINT32_MAX, SIZE_MAX) == 0);
  puts("ES7210 wire order, logical loopback channels, signed limits and invalid channels: PASS");
}
