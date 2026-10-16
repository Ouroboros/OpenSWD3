#pragma once

#include <cstdint>

// Miles Sound System 6 (mss32.dll) entry points used by swd32.exe, emulated over SDL3 audio
// (src/platform/mss.cpp): one software mixer at 44100 Hz 16-bit stereo; samples from in-memory RIFF PCM or
// MP3, streams from MP3 files (FFmpeg). No MIDI output device exists, so AIL_midiOutOpen fails.
namespace mss {

using S32 = std::int32_t;
using U32 = std::uint32_t;

struct DigitalDriver;
struct Sample;
struct Stream;
struct MidiDriver;
struct Sequence;
using HDIGDRIVER = DigitalDriver*;
using HSAMPLE = Sample*;
using HSTREAM = Stream*;
using HMDIDRIVER = MidiDriver*;
using HSEQUENCE = Sequence*;

// Sample / stream / sequence status values.
constexpr U32 SMP_FREE = 1;
constexpr U32 SMP_DONE = 2;
constexpr U32 SMP_PLAYING = 4;
constexpr U32 SMP_STOPPED = 8;
constexpr U32 SMP_PLAYINGBUTRELEASED = 16;

// Preferences.
constexpr U32 DIG_MIXER_CHANNELS = 1;
constexpr U32 DIG_USE_WAVEOUT = 15;

#pragma pack(push, 1)
struct PCMWAVEFORMAT {
    std::uint16_t wFormatTag;
    std::uint16_t nChannels;
    std::uint32_t nSamplesPerSec;
    std::uint32_t nAvgBytesPerSec;
    std::uint16_t nBlockAlign;
    std::uint16_t wBitsPerSample;
};
#pragma pack(pop)

void AIL_serve();
S32 AIL_startup();
void AIL_shutdown();
char* AIL_set_redist_directory(const char* directory);
char* AIL_last_error();
void* AIL_mem_alloc_lock(U32 size);
void AIL_mem_free_lock(void* block);
S32 AIL_set_preference(U32 number, S32 value);
S32 AIL_get_preference(U32 number);

S32 AIL_waveOutOpen(HDIGDRIVER* driver, void* waveOut, S32 deviceId, const PCMWAVEFORMAT* format);
void AIL_waveOutClose(HDIGDRIVER driver);
S32 AIL_digital_configuration(HDIGDRIVER driver, S32* rate, S32* format, char* string);

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER driver);
void AIL_release_sample_handle(HSAMPLE sample);
void AIL_init_sample(HSAMPLE sample);
S32 AIL_set_sample_file(HSAMPLE sample, const void* fileImage, S32 block);
S32 AIL_set_named_sample_file(HSAMPLE sample, const char* suffix, const void* fileImage, S32 fileSize, S32 block);
void AIL_set_sample_user_data(HSAMPLE sample, U32 index, S32 value);
S32 AIL_sample_user_data(HSAMPLE sample, U32 index);
void AIL_set_sample_volume(HSAMPLE sample, S32 volume);
void AIL_set_sample_pan(HSAMPLE sample, S32 pan);
void AIL_set_sample_loop_count(HSAMPLE sample, S32 count);
void AIL_start_sample(HSAMPLE sample);
void AIL_end_sample(HSAMPLE sample);
U32 AIL_sample_status(HSAMPLE sample);

HSTREAM AIL_open_stream(HDIGDRIVER driver, const char* fileName, S32 streamMemory);
void AIL_close_stream(HSTREAM stream);
void AIL_set_stream_user_data(HSTREAM stream, U32 index, S32 value);
S32 AIL_stream_user_data(HSTREAM stream, U32 index);
void AIL_set_stream_volume(HSTREAM stream, S32 volume);
S32 AIL_stream_volume(HSTREAM stream);
void AIL_set_stream_loop_count(HSTREAM stream, S32 count);
void AIL_start_stream(HSTREAM stream);
S32 AIL_stream_status(HSTREAM stream);
void AIL_stream_ms_position(HSTREAM stream, S32* totalMilliseconds, S32* currentMilliseconds);

S32 AIL_midiOutOpen(HMDIDRIVER* driver, void* midiOut, S32 deviceId);
void AIL_midiOutClose(HMDIDRIVER driver);
HSEQUENCE AIL_allocate_sequence_handle(HMDIDRIVER driver);
void AIL_release_sequence_handle(HSEQUENCE sequence);
S32 AIL_init_sequence(HSEQUENCE sequence, const void* start, S32 sequenceNumber);
void AIL_set_sequence_user_data(HSEQUENCE sequence, U32 index, S32 value);
S32 AIL_sequence_user_data(HSEQUENCE sequence, U32 index);
void AIL_set_sequence_volume(HSEQUENCE sequence, S32 volume, S32 milliseconds);
void AIL_set_sequence_loop_count(HSEQUENCE sequence, S32 count);
void AIL_start_sequence(HSEQUENCE sequence);
void AIL_end_sequence(HSEQUENCE sequence);
U32 AIL_sequence_status(HSEQUENCE sequence);

}  // namespace mss
