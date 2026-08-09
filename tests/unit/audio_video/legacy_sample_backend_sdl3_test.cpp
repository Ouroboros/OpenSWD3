#include "test.hpp"

#include "legacy_sample_backend_sdl3.hpp"

#include "openswd3/audio_video/legacy_audio_output.hpp"
#include "openswd3/audio_video/legacy_snd_archive.hpp"

#include <SDL3/SDL_hints.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <set>
#include <span>
#include <string_view>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::platform_sdl3::LegacyPcmDecodeStatus;
using openswd3::platform_sdl3::SdlLegacySampleBackend;

void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u16 read_u16(
    const std::span<const u8> bytes,
    const std::size_t offset
) {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] u32 read_u32(
    const std::span<const u8> bytes,
    const std::size_t offset
) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::vector<u8> make_legacy_pcm(
    const std::size_t frame_count,
    const u16 bits_per_sample = 16U
) {
    const std::size_t bytes_per_frame = bits_per_sample / 8U;
    const std::size_t pcm_size = frame_count * bytes_per_frame;
    std::vector<u8> bytes(44U + pcm_size, 0U);
    bytes[0] = 'R';
    bytes[1] = 'I';
    bytes[2] = 'F';
    bytes[3] = 'F';
    bytes[8] = 'W';
    bytes[9] = 'A';
    bytes[10] = 'V';
    bytes[11] = 'E';
    bytes[12] = 'f';
    bytes[13] = 'm';
    bytes[14] = 't';
    bytes[15] = ' ';
    bytes[36] = 'd';
    bytes[37] = 'a';
    bytes[38] = 't';
    bytes[39] = 'a';
    write_u32(bytes, 4U, static_cast<u32>(bytes.size() - 8U));
    write_u32(bytes, 16U, 16U);
    write_u16(bytes, 20U, 1U);
    write_u16(bytes, 22U, 1U);
    write_u32(bytes, 24U, 22'050U);
    write_u32(
        bytes,
        28U,
        22'050U * static_cast<u32>(bytes_per_frame)
    );
    write_u16(bytes, 32U, static_cast<u16>(bytes_per_frame));
    write_u16(bytes, 34U, bits_per_sample);
    write_u32(bytes, 40U, static_cast<u32>(pcm_size));
    if (bits_per_sample == 16U) {
        for (std::size_t frame = 0U; frame < frame_count; ++frame) {
            const u16 value = frame % 2U == 0U ? 0xC000U : 0x4000U;
            write_u16(bytes, 44U + frame * 2U, value);
        }
    } else {
        for (std::size_t frame = 0U; frame < frame_count; ++frame) {
            bytes[44U + frame] = frame % 2U == 0U ? 32U : 224U;
        }
    }
    return bytes;
}

void test_decode(openswd3::test::Context& test) {
    const std::vector<u8> pcm = make_legacy_pcm(4U);
    const auto decoded = openswd3::platform_sdl3::decode_legacy_riff_pcm(
        pcm,
        22'050
    );
    test.expect_equal(
        decoded.status,
        LegacyPcmDecodeStatus::ready,
        "fixed legacy PCM header decodes"
    );
    test.expect_equal(
        decoded.stereo_frames.size(),
        std::size_t{8U},
        "four mono frames become four stereo frames"
    );
    test.expect_true(
        std::fabs(decoded.stereo_frames[0] - decoded.stereo_frames[1]) <
            0.000001F,
        "mono conversion duplicates the left channel"
    );
    test.expect_true(
        decoded.stereo_frames[0] < 0.0F &&
            decoded.stereo_frames[2] > 0.0F,
        "signed PCM polarity survives conversion"
    );

    std::vector<u8> malformed = pcm;
    malformed[36] = 0U;
    malformed[37] = 0U;
    test.expect_equal(
        openswd3::platform_sdl3::decode_legacy_riff_pcm(
            malformed,
            22'050
        ).status,
        LegacyPcmDecodeStatus::ready,
        "ID 506/507 malformed data tag remains accepted"
    );

    malformed[0] = 0U;
    test.expect_equal(
        openswd3::platform_sdl3::decode_legacy_riff_pcm(
            malformed,
            22'050
        ).status,
        LegacyPcmDecodeStatus::invalid_riff,
        "non-RIFF data is rejected"
    );
}

void test_dummy_device_backend(openswd3::test::Context& test) {
    static_cast<void>(SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy"));
    SdlLegacySampleBackend backend;
    const auto output =
        openswd3::audio_video::initialize_legacy_audio_output(backend);
    test.expect_equal(
        output.status,
        openswd3::audio_video::LegacyAudioOutputStatus::ready,
        "SDL dummy output opens through the legacy negotiation"
    );
    test.expect_true(
        backend.driver_token() != 0U,
        "opened SDL logical device is the driver token"
    );
    test.expect_equal(
        output.sample_handle_count,
        16,
        "SDL backend exposes sixteen sample handles"
    );

    const auto handle = backend.allocate_sample_handle();
    test.expect_equal(handle, 1U, "first SDL sample handle is one-based");
    backend.initialize_sample(handle);
    const std::vector<u8> pcm = make_legacy_pcm(44'100U);
    test.expect_true(
        backend.set_sample_file(handle, pcm),
        "legacy PCM is converted for the mixer"
    );
    test.expect_false(
        backend.set_named_sample_file(
            handle,
            std::string_view{".mp3"},
            pcm,
            0U
        ),
        "named MP3 setup is unavailable without a decoder"
    );
    backend.set_sample_user_data(handle, 0U, 77U);
    test.expect_equal(
        backend.sample_user_data(handle, 0U),
        77U,
        "sample user data round-trips"
    );
    backend.set_sample_loop_count(handle, 0);
    backend.start_sample(handle);
    test.expect_equal(
        backend.sample_status(handle),
        4U,
        "zero-loop-count sample remains playing"
    );
    backend.end_sample(handle);
    test.expect_equal(
        backend.sample_status(handle),
        2U,
        "ended sample reports terminal status two"
    );
    backend.release_sample_handle(handle);
    backend.close_output();
    test.expect_equal(
        backend.driver_token(),
        0U,
        "closed output clears the driver token"
    );
}

void test_real_archive_formats(
    openswd3::test::Context& test,
    const std::filesystem::path& archive_path
) {
    openswd3::audio_video::LegacySndArchive archive;
    test.expect_equal(
        archive.open(archive_path),
        openswd3::audio_video::LegacySndOpenStatus::ready,
        "real SND archive opens for SDL decode coverage"
    );
    if (!archive.is_open()) {
        return;
    }

    std::set<std::array<u32, 3U>> decoded_formats;
    for (u32 sound_id = 1U;
         sound_id <= openswd3::audio_video::kLegacySndSlotCount;
         ++sound_id) {
        const auto* const entry = archive.entry(sound_id);
        if (entry == nullptr || entry->file_offset == 0U ||
            sound_id == 270U || sound_id == 277U) {
            continue;
        }

        const auto loaded = archive.load_sample(sound_id);
        test.expect_equal(
            loaded.status,
            openswd3::audio_video::LegacySndSampleStatus::ready,
            "real SND view loads"
        );
        if (loaded.status !=
                openswd3::audio_video::LegacySndSampleStatus::ready ||
            loaded.bytes.size() < 44U) {
            continue;
        }

        const std::array<u32, 3U> key{
            read_u16(loaded.bytes, 0x16U),
            read_u32(loaded.bytes, 0x18U),
            read_u16(loaded.bytes, 0x22U),
        };
        if (!decoded_formats.insert(key).second) {
            continue;
        }
        test.expect_equal(
            openswd3::platform_sdl3::decode_legacy_riff_pcm(
                loaded.bytes,
                44'100
            ).status,
            LegacyPcmDecodeStatus::ready,
            "each real PCM format converts through SDL3"
        );
    }
    test.expect_equal(
        decoded_formats.size(),
        std::size_t{13U},
        "all thirteen real PCM format combinations are covered"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_decode(test);
    test_dummy_device_backend(test);
    if (argument_count == 2) {
        test_real_archive_formats(test, arguments[1]);
    }
    return test.exit_code();
}
