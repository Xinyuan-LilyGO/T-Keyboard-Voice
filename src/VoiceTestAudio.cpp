#include "VoiceTestAudio.h"

namespace KeyboardVoice {
alignas(4) const uint8_t TEST_AUDIO_WAV[] = {
#include "audio/test_audio.inc"
};
const size_t TEST_AUDIO_WAV_SIZE = sizeof(TEST_AUDIO_WAV);
}
