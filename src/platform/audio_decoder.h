#pragma once

#include <cstddef>
#include <vector>

#include "core/types.h"

namespace swd3
{

// The mixer format: interleaved 16-bit stereo at this rate.
constexpr int kMixRate = 44100;
constexpr int kMixChannels = 2;

// An in-memory audio file (MP3, WAV) decoded with FFmpeg into the mixer format.
bool DecodeAudio(const u8* data, std::size_t size, std::vector<i16>& pcm);

// Raw interleaved PCM (8-bit unsigned or 16-bit signed, one or two channels) converted to the mixer format.
bool ConvertPcm(const u8* data, std::size_t bytes, int channels, int rate, int bits, std::vector<i16>& pcm);

}  // namespace swd3
