#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// Decoding of in-memory audio files (WAV, MP3) with FFmpeg into interleaved 16-bit stereo PCM at
// kMixRate Hz, the format of the emulated Miles mixer.
namespace swd3::platform {

constexpr int kMixRate = 44100;

bool decodeAudio(const std::uint8_t* data, std::size_t size, std::vector<std::int16_t>& pcm);

// Converts raw interleaved PCM (8-bit unsigned or 16-bit signed) to the mixer format.
bool convertPcm(const std::uint8_t* data, std::size_t bytes, int channels, int rate, int bits,
                std::vector<std::int16_t>& pcm);

}  // namespace swd3::platform
