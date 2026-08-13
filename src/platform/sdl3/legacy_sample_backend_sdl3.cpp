#include "legacy_sample_backend_sdl3.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <new>

namespace openswd3::platform_sdl3 {
namespace {

constexpr std::size_t kLegacyWaveHeaderSize = 44U;
constexpr std::size_t kFloatStereoFrameSize = 2U * sizeof(float);

[[nodiscard]] compat::u16 read_u16(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        )
    );
}

[[nodiscard]] compat::u32 read_u32(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] bool has_signature(
    const std::span<const compat::u8> bytes,
    const std::size_t offset,
    const char (&signature)[5]
) noexcept {
    return bytes.size() >= offset + 4U &&
        bytes[offset] == static_cast<compat::u8>(signature[0]) &&
        bytes[offset + 1U] == static_cast<compat::u8>(signature[1]) &&
        bytes[offset + 2U] == static_cast<compat::u8>(signature[2]) &&
        bytes[offset + 3U] == static_cast<compat::u8>(signature[3]);
}

[[nodiscard]] float volume_gain(const compat::i32 volume) noexcept {
    return static_cast<float>(std::clamp(volume, 0, 127)) / 127.0F;
}

void pan_gains(const compat::i32 pan, float& left, float& right) noexcept {
    const compat::i32 clamped = std::clamp(pan, 0, 127);
    if (clamped <= 63) {
        left = 1.0F;
        right = static_cast<float>(clamped) / 63.0F;
        return;
    }
    left = static_cast<float>(127 - clamped) / 64.0F;
    right = 1.0F;
}

}  // namespace

LegacyDecodedPcm decode_legacy_riff_pcm(
    const std::span<const compat::u8> bytes,
    const compat::i32 target_sample_rate
) {
    LegacyDecodedPcm result;
    if (bytes.size() < kLegacyWaveHeaderSize ||
        !has_signature(bytes, 0U, "RIFF") ||
        !has_signature(bytes, 8U, "WAVE") ||
        !has_signature(bytes, 12U, "fmt ") || target_sample_rate <= 0) {
        return result;
    }

    const compat::u16 format_tag = read_u16(bytes, 0x14U);
    const compat::u16 channels = read_u16(bytes, 0x16U);
    const compat::u32 sample_rate = read_u32(bytes, 0x18U);
    const compat::u16 block_align = read_u16(bytes, 0x20U);
    const compat::u16 bits_per_sample = read_u16(bytes, 0x22U);
    if (format_tag != 1U || (channels != 1U && channels != 2U) ||
        sample_rate == 0U ||
        (bits_per_sample != 8U && bits_per_sample != 16U)) {
        result.status = LegacyPcmDecodeStatus::unsupported_format;
        return result;
    }

    const compat::u16 expected_block_align = static_cast<compat::u16>(
        channels * static_cast<compat::u16>(bits_per_sample / 8U)
    );
    if (block_align == 0U || block_align != expected_block_align) {
        result.status = LegacyPcmDecodeStatus::unsupported_format;
        return result;
    }

    const std::size_t declared_size = read_u32(bytes, 0x28U);
    const std::size_t available_size = bytes.size() - kLegacyWaveHeaderSize;
    std::size_t pcm_size = std::min(declared_size, available_size);
    pcm_size -= pcm_size % block_align;
    if (pcm_size == 0U ||
        pcm_size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        result.status = LegacyPcmDecodeStatus::unsupported_format;
        return result;
    }

    const SDL_AudioSpec source_spec{
        bits_per_sample == 8U ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE,
        static_cast<int>(channels),
        static_cast<int>(sample_rate),
    };
    const SDL_AudioSpec target_spec{
        SDL_AUDIO_F32,
        2,
        target_sample_rate,
    };
    Uint8* converted{};
    int converted_size{};
    if (!SDL_ConvertAudioSamples(
            &source_spec,
            bytes.data() + kLegacyWaveHeaderSize,
            static_cast<int>(pcm_size),
            &target_spec,
            &converted,
            &converted_size
        )) {
        result.status = LegacyPcmDecodeStatus::conversion_failed;
        return result;
    }

    if (converted_size <= 0 ||
        static_cast<std::size_t>(converted_size) % kFloatStereoFrameSize !=
            0U) {
        SDL_free(converted);
        result.status = LegacyPcmDecodeStatus::conversion_failed;
        return result;
    }

    const std::size_t float_count =
        static_cast<std::size_t>(converted_size) / sizeof(float);
    const float* const converted_floats =
        reinterpret_cast<const float*>(converted);
    try {
        result.stereo_frames.assign(
            converted_floats, converted_floats + float_count
        );
    } catch (const std::bad_alloc&) {
        SDL_free(converted);
        result.status = LegacyPcmDecodeStatus::conversion_failed;
        return result;
    }
    SDL_free(converted);
    result.status = LegacyPcmDecodeStatus::ready;
    return result;
}

SdlLegacySampleBackend::SdlLegacySampleBackend() {
    preferences_[audio_video::kLegacySampleCountPreference] = 24;
}

SdlLegacySampleBackend::~SdlLegacySampleBackend() {
    close_output();
}

void SdlLegacySampleBackend::set_preference(
    const compat::i32 index, const compat::i32 value
) {
    if (index < 0 || static_cast<std::size_t>(index) >= preferences_.size()) {
        return;
    }
    preferences_[static_cast<std::size_t>(index)] = value;
}

compat::i32 SdlLegacySampleBackend::preference(const compat::i32 index) {
    if (index < 0 || static_cast<std::size_t>(index) >= preferences_.size()) {
        return 0;
    }
    return preferences_[static_cast<std::size_t>(index)];
}

bool SdlLegacySampleBackend::open_output(
    const audio_video::LegacyPcmOutputFormat& format
) {
    if (device_ != 0U || mixer_stream_ != nullptr) {
        close_output();
    }
    if (format.format_tag != 1U || format.channels == 0U ||
        format.sample_rate == 0U ||
        (format.bits_per_sample != 8U && format.bits_per_sample != 16U)) {
        last_error_ = "unsupported legacy PCM output format";
        return false;
    }
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        set_sdl_error("SDL_InitSubSystem(SDL_INIT_AUDIO)");
        return false;
    }

    const SDL_AudioSpec requested{
        format.bits_per_sample == 8U ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE,
        static_cast<int>(format.channels),
        static_cast<int>(format.sample_rate),
    };
    const SDL_AudioDeviceID device =
        SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &requested);
    if (device == 0U) {
        set_sdl_error("SDL_OpenAudioDevice");
        return false;
    }

    const SDL_AudioSpec mixer_spec{
        SDL_AUDIO_F32,
        2,
        static_cast<int>(format.sample_rate),
    };
    SDL_AudioStream* const stream = SDL_CreateAudioStream(&mixer_spec, nullptr);
    if (stream == nullptr) {
        set_sdl_error("SDL_CreateAudioStream");
        SDL_CloseAudioDevice(device);
        return false;
    }

    output_sample_rate_ = static_cast<compat::i32>(format.sample_rate);
    if (!SDL_SetAudioStreamGetCallback(stream, audio_callback, this)) {
        set_sdl_error("SDL_SetAudioStreamGetCallback");
        output_sample_rate_ = 0;
        SDL_DestroyAudioStream(stream);
        SDL_CloseAudioDevice(device);
        return false;
    }
    if (!SDL_BindAudioStream(device, stream)) {
        set_sdl_error("SDL_BindAudioStream");
        output_sample_rate_ = 0;
        SDL_DestroyAudioStream(stream);
        SDL_CloseAudioDevice(device);
        return false;
    }

    device_ = device;
    mixer_stream_ = stream;
    const char* const name = SDL_GetAudioDeviceName(device);
    configuration_ = name == nullptr ? "SDL3 audio output" : name;
    last_error_.clear();
    return true;
}

std::string_view SdlLegacySampleBackend::output_configuration() {
    return configuration_;
}

std::string_view SdlLegacySampleBackend::last_error() const {
    return last_error_;
}

compat::u32 SdlLegacySampleBackend::driver_token() const {
    return static_cast<compat::u32>(device_);
}

audio_video::LegacySampleHandle
SdlLegacySampleBackend::allocate_sample_handle() {
    std::lock_guard lock{mutex_};
    if (device_ == 0U) {
        return 0U;
    }
    for (std::size_t index = 0U; index < samples_.size(); ++index) {
        if (!samples_[index].allocated) {
            reset_sample(samples_[index], true);
            return static_cast<audio_video::LegacySampleHandle>(index + 1U);
        }
    }
    return 0U;
}

void SdlLegacySampleBackend::initialize_sample(
    const audio_video::LegacySampleHandle handle
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr && target->allocated) {
        reset_sample(*target, true);
    }
}

void SdlLegacySampleBackend::release_sample_handle(
    const audio_video::LegacySampleHandle handle
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        reset_sample(*target, false);
    }
}

bool SdlLegacySampleBackend::set_sample_file(
    const audio_video::LegacySampleHandle handle,
    const std::span<const compat::u8> bytes
) {
    const compat::i32 target_rate = output_sample_rate_;
    LegacyDecodedPcm decoded = decode_legacy_riff_pcm(bytes, target_rate);
    if (decoded.status != LegacyPcmDecodeStatus::ready) {
        last_error_ = "legacy RIFF PCM decode failed";
        return false;
    }

    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target == nullptr || !target->allocated) {
        return false;
    }
    target->stereo_frames = std::move(decoded.stereo_frames);
    target->configured = true;
    target->status = kStatusDone;
    return true;
}

bool SdlLegacySampleBackend::set_named_sample_file(
    const audio_video::LegacySampleHandle,
    const std::string_view,
    const std::span<const compat::u8>,
    const compat::u32
) {
    last_error_ = "SDL3 sample backend has no MP3 decoder";
    return false;
}

void SdlLegacySampleBackend::set_sample_user_data(
    const audio_video::LegacySampleHandle handle,
    const compat::u32 slot,
    const compat::u32 value
) {
    if (slot != 0U) {
        return;
    }
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        target->user_data = value;
    }
}

compat::u32 SdlLegacySampleBackend::sample_user_data(
    const audio_video::LegacySampleHandle handle, const compat::u32 slot
) {
    if (slot != 0U) {
        return 0U;
    }
    std::lock_guard lock{mutex_};
    const Sample* const target = sample(handle);
    return target == nullptr ? 0U : target->user_data;
}

void SdlLegacySampleBackend::set_sample_volume(
    const audio_video::LegacySampleHandle handle, const compat::i32 volume
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        target->volume = volume;
    }
}

void SdlLegacySampleBackend::set_sample_pan(
    const audio_video::LegacySampleHandle handle, const compat::i32 pan
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        target->pan = pan;
    }
}

void SdlLegacySampleBackend::set_sample_loop_count(
    const audio_video::LegacySampleHandle handle, const compat::i32 loop_count
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        target->loop_count = loop_count;
    }
}

void SdlLegacySampleBackend::start_sample(
    const audio_video::LegacySampleHandle handle
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target == nullptr || !target->allocated || !target->configured ||
        target->stereo_frames.empty()) {
        return;
    }
    target->frame_cursor = 0U;
    target->loops_remaining = target->loop_count;
    target->playing = true;
    target->status = kStatusPlaying;
}

void SdlLegacySampleBackend::end_sample(
    const audio_video::LegacySampleHandle handle
) {
    std::lock_guard lock{mutex_};
    Sample* const target = sample(handle);
    if (target != nullptr) {
        target->playing = false;
        target->status = kStatusDone;
    }
}

compat::u32 SdlLegacySampleBackend::sample_status(
    const audio_video::LegacySampleHandle handle
) {
    std::lock_guard lock{mutex_};
    const Sample* const target = sample(handle);
    return target == nullptr ? kStatusDone : target->status;
}

void SdlLegacySampleBackend::close_output() {
    SDL_AudioStream* const stream = mixer_stream_;
    const SDL_AudioDeviceID device = device_;
    mixer_stream_ = nullptr;
    device_ = 0U;
    output_sample_rate_ = 0;

    if (stream != nullptr && SDL_WasInit(SDL_INIT_AUDIO) != 0U) {
        static_cast<void>(
            SDL_SetAudioStreamGetCallback(stream, nullptr, nullptr)
        );
        SDL_DestroyAudioStream(stream);
    }
    if (device != 0U && SDL_WasInit(SDL_INIT_AUDIO) != 0U) {
        SDL_CloseAudioDevice(device);
    }

    std::lock_guard lock{mutex_};
    for (Sample& target : samples_) {
        reset_sample(target, false);
    }
    configuration_.clear();
}

SdlLegacySampleBackend::Sample* SdlLegacySampleBackend::sample(
    const audio_video::LegacySampleHandle handle
) noexcept {
    if (handle == 0U || handle > samples_.size()) {
        return nullptr;
    }
    return &samples_[handle - 1U];
}

const SdlLegacySampleBackend::Sample* SdlLegacySampleBackend::sample(
    const audio_video::LegacySampleHandle handle
) const noexcept {
    if (handle == 0U || handle > samples_.size()) {
        return nullptr;
    }
    return &samples_[handle - 1U];
}

void SdlLegacySampleBackend::reset_sample(
    Sample& target, const bool allocated
) {
    target.stereo_frames.clear();
    target.frame_cursor = 0U;
    target.user_data = 0U;
    target.status = kStatusDone;
    target.volume = 127;
    target.pan = 63;
    target.loop_count = 1;
    target.loops_remaining = 1;
    target.allocated = allocated;
    target.configured = false;
    target.playing = false;
}

void SdlLegacySampleBackend::set_sdl_error(const std::string_view operation) {
    last_error_.assign(operation);
    last_error_.append(": ");
    last_error_.append(SDL_GetError());
}

void SDLCALL SdlLegacySampleBackend::audio_callback(
    void* const userdata,
    SDL_AudioStream* const stream,
    const int additional_amount,
    const int
) {
    if (userdata == nullptr || stream == nullptr || additional_amount <= 0) {
        return;
    }
    try {
        static_cast<SdlLegacySampleBackend*>(userdata)->mix_and_queue(
            *stream, additional_amount
        );
    } catch (...) {
        // The audio callback cannot propagate allocation failures through SDL.
    }
}

void SdlLegacySampleBackend::mix_and_queue(
    SDL_AudioStream& stream, const int additional_amount
) {
    const std::size_t requested_bytes =
        static_cast<std::size_t>(additional_amount);
    const std::size_t frame_count =
        (requested_bytes + kFloatStereoFrameSize - 1U) / kFloatStereoFrameSize;
    std::vector<float> mixed(frame_count * 2U, 0.0F);

    {
        std::lock_guard lock{mutex_};
        for (Sample& source : samples_) {
            if (!source.allocated || !source.configured || !source.playing) {
                continue;
            }

            const std::size_t source_frame_count =
                source.stereo_frames.size() / 2U;
            const float gain = volume_gain(source.volume);
            float left_pan{};
            float right_pan{};
            pan_gains(source.pan, left_pan, right_pan);

            for (std::size_t frame = 0U; frame < frame_count; ++frame) {
                if (source.frame_cursor >= source_frame_count) {
                    if (source.loop_count == 0 || source.loops_remaining > 1) {
                        if (source.loop_count != 0) {
                            --source.loops_remaining;
                        }
                        source.frame_cursor = 0U;
                    } else {
                        source.playing = false;
                        source.status = kStatusDone;
                        break;
                    }
                }

                const std::size_t source_offset = source.frame_cursor * 2U;
                const std::size_t target_offset = frame * 2U;
                mixed[target_offset] +=
                    source.stereo_frames[source_offset] * gain * left_pan;
                mixed[target_offset + 1U] +=
                    source.stereo_frames[source_offset + 1U] * gain * right_pan;
                ++source.frame_cursor;
            }
        }
    }

    static_cast<void>(SDL_PutAudioStreamData(
        &stream, mixed.data(), static_cast<int>(mixed.size() * sizeof(float))
    ));
}

}  // namespace openswd3::platform_sdl3
