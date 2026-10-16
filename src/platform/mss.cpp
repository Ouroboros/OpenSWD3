#include "swd3/mss.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "platform/audio_decode.h"
#include "swd3/w32.h"

namespace mss {

// One playing sound of the mixer (sample or stream).
struct Voice {
    std::shared_ptr<const std::vector<std::int16_t>> pcm;  // interleaved stereo at kMixRate
    std::size_t frame = 0;
    S32 loopCount = 1;  // 0: forever
    S32 volume = 127;
    S32 pan = 64;
    U32 status = SMP_DONE;
    S32 userData[8] = {};
};

struct DigitalDriver {
    SDL_AudioStream* device = nullptr;
};

struct Sample : Voice {
    DigitalDriver* driver = nullptr;
};

struct Stream : Voice {
    DigitalDriver* driver = nullptr;
};

struct MidiDriver {};
struct Sequence {};

namespace {

std::recursive_mutex g_mutex;
std::vector<Voice*> g_voices;
std::string g_lastError;
std::string g_redistDirectory;
S32 g_preferences[64] = {0, 64};
std::vector<std::int32_t> g_mix;

void setError(const char* text) { g_lastError = text; }

void attach(Voice* voice) {
    if (std::find(g_voices.begin(), g_voices.end(), voice) == g_voices.end()) {
        g_voices.push_back(voice);
    }
}

void detach(Voice* voice) { g_voices.erase(std::remove(g_voices.begin(), g_voices.end(), voice), g_voices.end()); }

void mixVoice(Voice& voice, std::int32_t* out, int frames) {
    const auto& pcm = *voice.pcm;
    const std::size_t total = pcm.size() / 2;
    const std::int32_t left = std::min<std::int32_t>(256, (127 - voice.pan) * 256 / 63);
    const std::int32_t right = std::min<std::int32_t>(256, voice.pan * 256 / 63);
    const std::int32_t gain = voice.volume * 256 / 127;
    for (int i = 0; i < frames; ++i) {
        if (voice.frame >= total) {
            if (voice.loopCount == 1 || total == 0) {
                voice.status = SMP_DONE;
                return;
            }
            if (voice.loopCount > 1) {
                --voice.loopCount;
            }
            voice.frame = 0;
        }
        const std::int32_t l = pcm[voice.frame * 2];
        const std::int32_t r = pcm[voice.frame * 2 + 1];
        out[i * 2] += l * gain / 256 * left / 256;
        out[i * 2 + 1] += r * gain / 256 * right / 256;
        ++voice.frame;
    }
}

void SDLCALL feed(void*, SDL_AudioStream* stream, int additional, int) {
    if (additional <= 0) {
        return;
    }
    const int frames = additional / 4;
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    g_mix.assign(static_cast<std::size_t>(frames) * 2, 0);
    for (Voice* voice : g_voices) {
        if (voice->status == SMP_PLAYING && voice->pcm) {
            mixVoice(*voice, g_mix.data(), frames);
        }
    }
    std::vector<std::int16_t> out(static_cast<std::size_t>(frames) * 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::int16_t>(std::clamp<std::int32_t>(g_mix[i], -32768, 32767));
    }
    SDL_PutAudioStreamData(stream, out.data(), frames * 4);
}

bool loadRiff(const std::uint8_t* image, std::vector<std::int16_t>& pcm) {
    if (std::memcmp(image, "RIFF", 4) != 0 || std::memcmp(image + 8, "WAVE", 4) != 0) {
        return false;
    }
    std::uint32_t riffSize;
    std::memcpy(&riffSize, image + 4, 4);
    const std::uint8_t* chunk = image + 12;
    const std::uint8_t* end = image + 8 + riffSize;
    int channels = 0;
    int rate = 0;
    int bits = 0;
    while (chunk + 8 <= end) {
        std::uint32_t size;
        std::memcpy(&size, chunk + 4, 4);
        if (std::memcmp(chunk, "fmt ", 4) == 0) {
            std::uint16_t tag;
            std::uint16_t channelCount;
            std::uint32_t samplesPerSecond;
            std::uint16_t bitsPerSample;
            std::memcpy(&tag, chunk + 8, 2);
            std::memcpy(&channelCount, chunk + 10, 2);
            std::memcpy(&samplesPerSecond, chunk + 12, 4);
            std::memcpy(&bitsPerSample, chunk + 22, 2);
            if (tag != 1) {
                return false;
            }
            channels = channelCount;
            rate = static_cast<int>(samplesPerSecond);
            bits = bitsPerSample;
        } else if (std::memcmp(chunk, "data", 4) == 0) {
            return swd3::platform::convertPcm(chunk + 8, size, channels, rate, bits, pcm);
        }
        chunk += 8 + ((size + 1) & ~1u);
    }
    return false;
}

bool readFile(const char* name, std::vector<std::uint8_t>& data) {
    w32::HANDLE file = w32::CreateFileA(name, w32::GENERIC_READ, 1, nullptr, w32::OPEN_EXISTING,
                                        w32::FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == w32::INVALID_HANDLE_VALUE) {
        return false;
    }
    const w32::DWORD size = w32::GetFileSize(file, nullptr);
    data.resize(size);
    w32::DWORD read = 0;
    const bool ok = w32::ReadFile(file, data.data(), size, &read, nullptr) && read == size;
    w32::CloseHandle(file);
    return ok;
}

}  // namespace

void AIL_serve() {}

S32 AIL_startup() { return 1; }

void AIL_shutdown() {}

char* AIL_set_redist_directory(const char* directory) {
    g_redistDirectory = directory != nullptr ? directory : "";
    return g_redistDirectory.data();
}

char* AIL_last_error() { return g_lastError.data(); }

void* AIL_mem_alloc_lock(U32 size) { return std::malloc(size); }

void AIL_mem_free_lock(void* block) { std::free(block); }

S32 AIL_set_preference(U32 number, S32 value) {
    const S32 previous = g_preferences[number & 63];
    g_preferences[number & 63] = value;
    return previous;
}

S32 AIL_get_preference(U32 number) { return g_preferences[number & 63]; }

S32 AIL_waveOutOpen(HDIGDRIVER* driver, void*, S32, const PCMWAVEFORMAT*) {
    *driver = nullptr;
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        setError(SDL_GetError());
        return -1;
    }
    const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, swd3::platform::kMixRate};
    SDL_AudioStream* device = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &feed, nullptr);
    if (device == nullptr) {
        setError(SDL_GetError());
        return -1;
    }
    SDL_ResumeAudioStreamDevice(device);
    *driver = new DigitalDriver{device};
    return 0;
}

void AIL_waveOutClose(HDIGDRIVER driver) {
    if (driver == nullptr) {
        return;
    }
    SDL_DestroyAudioStream(driver->device);
    delete driver;
}

S32 AIL_digital_configuration(HDIGDRIVER, S32* rate, S32* format, char* string) {
    if (rate != nullptr) *rate = swd3::platform::kMixRate;
    if (format != nullptr) *format = 3;
    if (string != nullptr) std::strcpy(string, "SDL3 mixer");
    return 0;
}

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER driver) {
    auto* sample = new Sample();
    sample->driver = driver;
    return sample;
}

void AIL_release_sample_handle(HSAMPLE sample) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    detach(sample);
    delete sample;
}

void AIL_init_sample(HSAMPLE sample) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    detach(sample);
    sample->pcm.reset();
    sample->frame = 0;
    sample->loopCount = 1;
    sample->volume = 127;
    sample->pan = 64;
    sample->status = SMP_DONE;
}

S32 AIL_set_sample_file(HSAMPLE sample, const void* fileImage, S32) {
    auto pcm = std::make_shared<std::vector<std::int16_t>>();
    if (!loadRiff(static_cast<const std::uint8_t*>(fileImage), *pcm)) {
        setError("Unsupported sample file");
        return 0;
    }
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->pcm = pcm;
    sample->frame = 0;
    return 1;
}

S32 AIL_set_named_sample_file(HSAMPLE sample, const char*, const void* fileImage, S32 fileSize, S32) {
    auto pcm = std::make_shared<std::vector<std::int16_t>>();
    if (!swd3::platform::decodeAudio(static_cast<const std::uint8_t*>(fileImage), static_cast<std::size_t>(fileSize),
                                     *pcm)) {
        setError("Cannot decode sample file");
        return 0;
    }
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->pcm = pcm;
    sample->frame = 0;
    return 1;
}

void AIL_set_sample_user_data(HSAMPLE sample, U32 index, S32 value) { sample->userData[index & 7] = value; }

S32 AIL_sample_user_data(HSAMPLE sample, U32 index) { return sample->userData[index & 7]; }

void AIL_set_sample_volume(HSAMPLE sample, S32 volume) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->volume = std::clamp<S32>(volume, 0, 127);
}

void AIL_set_sample_pan(HSAMPLE sample, S32 pan) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->pan = std::clamp<S32>(pan, 0, 127);
}

void AIL_set_sample_loop_count(HSAMPLE sample, S32 count) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->loopCount = count;
}

void AIL_start_sample(HSAMPLE sample) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    if (!sample->pcm) {
        return;
    }
    sample->frame = 0;
    sample->status = SMP_PLAYING;
    attach(sample);
}

void AIL_end_sample(HSAMPLE sample) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    sample->status = SMP_DONE;
    detach(sample);
}

U32 AIL_sample_status(HSAMPLE sample) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return sample->status;
}

HSTREAM AIL_open_stream(HDIGDRIVER driver, const char* fileName, S32) {
    std::vector<std::uint8_t> data;
    if (!readFile(fileName, data)) {
        setError("Cannot open stream file");
        return nullptr;
    }
    auto pcm = std::make_shared<std::vector<std::int16_t>>();
    if (!swd3::platform::decodeAudio(data.data(), data.size(), *pcm)) {
        setError("Cannot decode stream file");
        return nullptr;
    }
    auto* stream = new Stream();
    stream->driver = driver;
    stream->pcm = pcm;
    return stream;
}

void AIL_close_stream(HSTREAM stream) {
    if (stream == nullptr) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    detach(stream);
    delete stream;
}

void AIL_set_stream_user_data(HSTREAM stream, U32 index, S32 value) { stream->userData[index & 7] = value; }

S32 AIL_stream_user_data(HSTREAM stream, U32 index) { return stream->userData[index & 7]; }

void AIL_set_stream_volume(HSTREAM stream, S32 volume) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    stream->volume = std::clamp<S32>(volume, 0, 127);
}

S32 AIL_stream_volume(HSTREAM stream) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return stream->volume;
}

void AIL_set_stream_loop_count(HSTREAM stream, S32 count) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    stream->loopCount = count;
}

void AIL_start_stream(HSTREAM stream) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    stream->frame = 0;
    stream->status = SMP_PLAYING;
    attach(stream);
}

S32 AIL_stream_status(HSTREAM stream) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    return static_cast<S32>(stream->status);
}

void AIL_stream_ms_position(HSTREAM stream, S32* totalMilliseconds, S32* currentMilliseconds) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    const std::size_t frames = stream->pcm ? stream->pcm->size() / 2 : 0;
    if (totalMilliseconds != nullptr) {
        *totalMilliseconds = static_cast<S32>(frames * 1000 / swd3::platform::kMixRate);
    }
    if (currentMilliseconds != nullptr) {
        *currentMilliseconds = static_cast<S32>(stream->frame * 1000 / swd3::platform::kMixRate);
    }
}

S32 AIL_midiOutOpen(HMDIDRIVER* driver, void*, S32) {
    *driver = nullptr;
    setError("No MIDI output device");
    return -1;
}

void AIL_midiOutClose(HMDIDRIVER) {}

HSEQUENCE AIL_allocate_sequence_handle(HMDIDRIVER) { return nullptr; }

void AIL_release_sequence_handle(HSEQUENCE) {}

S32 AIL_init_sequence(HSEQUENCE, const void*, S32) {
    setError("No MIDI output device");
    return 0;
}

void AIL_set_sequence_user_data(HSEQUENCE, U32, S32) {}

S32 AIL_sequence_user_data(HSEQUENCE, U32) { return 0; }

void AIL_set_sequence_volume(HSEQUENCE, S32, S32) {}

void AIL_set_sequence_loop_count(HSEQUENCE, S32) {}

void AIL_start_sequence(HSEQUENCE) {}

void AIL_end_sequence(HSEQUENCE) {}

U32 AIL_sequence_status(HSEQUENCE) { return SMP_DONE; }

}  // namespace mss
