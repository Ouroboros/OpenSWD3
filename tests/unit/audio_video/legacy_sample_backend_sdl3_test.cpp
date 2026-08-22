#include "test.hpp"

#include "legacy_sample_backend_sdl3.hpp"

#include "openswd3/audio_video/legacy_audio_output.hpp"
#include "openswd3/audio_video/legacy_snd_archive.hpp"
#include "openswd3/audio_video/legacy_stream_commands.hpp"
#include "openswd3/audio_video/legacy_stream_manager.hpp"
#include "openswd3/audio_video/legacy_world_music.hpp"
#include "openswd3/media_ffmpeg/legacy_ffmpeg_backends.hpp"

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::platform_sdl3::LegacyPcmDecodeStatus;
using openswd3::platform_sdl3::SdlLegacySampleBackend;

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u16
read_u16(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[nodiscard]] std::vector<u8> make_legacy_pcm(
    const std::size_t frame_count, const u16 bits_per_sample = 16U
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
    write_u32(bytes, 28U, 22'050U * static_cast<u32>(bytes_per_frame));
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
    const auto decoded =
        openswd3::platform_sdl3::decode_legacy_riff_pcm(pcm, 22'050);
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
        decoded.stereo_frames[0] < 0.0F && decoded.stereo_frames[2] > 0.0F,
        "signed PCM polarity survives conversion"
    );

    std::vector<u8> malformed = pcm;
    malformed[36] = 0U;
    malformed[37] = 0U;
    test.expect_equal(
        openswd3::platform_sdl3::decode_legacy_riff_pcm(malformed, 22'050)
            .status,
        LegacyPcmDecodeStatus::ready,
        "ID 506/507 malformed data tag remains accepted"
    );

    malformed[0] = 0U;
    test.expect_equal(
        openswd3::platform_sdl3::decode_legacy_riff_pcm(malformed, 22'050)
            .status,
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
            handle, std::string_view{".mp3"}, pcm, 0U
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
        backend.driver_token(), 0U, "closed output clears the driver token"
    );
}

class RealWorldMusicPorts final
    : public openswd3::audio_video::LegacyWorldMusicPorts {
public:
    RealWorldMusicPorts(
        openswd3::audio_video::LegacyStreamManager& manager,
        const std::span<const u8> maps_payload
    ) noexcept
        : manager_(manager), maps_payload_(maps_payload) {}

    void poll_stream_transition() override {}

    bool music_stream_absent() override {
        return openswd3::audio_video::legacy_stream_absent(manager_) != 0;
    }

    void configure_stream_transition(
        openswd3::compat::i32, openswd3::compat::i32
    ) override {}

    void apply_stream_transition() override {}

    std::string_view music_source_filename(const u32 music_id) override {
        const auto filename =
            openswd3::audio_video::legacy_music_source_filename_from_maps(
                maps_payload_, music_id
            );
        return filename.value_or(std::string_view{});
    }

    void play_music_stream(const std::string_view filename) override {
        requested_filename = filename;
        static_cast<void>(
            openswd3::audio_video::play_legacy_stream(manager_, filename, 1, 6)
        );
    }

    void
    set_music_stream_volume(const openswd3::compat::i32 mix_level) override {
        static_cast<void>(
            openswd3::audio_video::set_legacy_stream_volume(manager_, mix_level)
        );
    }

    openswd3::audio_video::LegacyStreamManager& manager_;
    std::span<const u8> maps_payload_;
    std::string requested_filename;
};

class VideoFramePorts final
    : public openswd3::audio_video::LegacyVideoFramePorts {
public:
    std::span<u16> video_destination_pixels() override {
        return pixels;
    }

    openswd3::compat::i32 video_destination_pitch_bytes() override {
        return 640 * static_cast<openswd3::compat::i32>(sizeof(u16));
    }

    openswd3::audio_video::LegacyVideoPixelFormat
    video_pixel_format() override {
        return openswd3::audio_video::LegacyVideoPixelFormat::rgb565;
    }

    void report_video_copy_failure() override {
        copy_failed = true;
    }

    bool present_video_frame() override {
        presented = true;
        return true;
    }

    std::vector<u16> pixels = std::vector<u16>(640U * 480U);
    bool copy_failed{};
    bool presented{};
};

void test_real_ffmpeg_media(
    openswd3::test::Context& test, const std::filesystem::path& data_root
) {
    test.expect_true(
        openswd3::media_ffmpeg::linked_ffmpeg_version().starts_with("n9.0"),
        "the media shim reports its linked FFmpeg 9.0 version"
    );

    auto stream_backend =
        openswd3::media_ffmpeg::make_legacy_stream_backend(data_root);
    const auto stream =
        stream_backend->open_stream(1U, "Music\\Map_Ca12.mp3", 0);
    test.expect_true(stream != 0U, "FFmpeg opens a real legacy MP3 stream");
    if (stream != 0U) {
        stream_backend->set_stream_user_data(stream, 0U, 100);
        stream_backend->set_stream_volume(stream, 96);
        stream_backend->set_stream_loop_count(stream, 0);
        stream_backend->start_stream(stream);
        openswd3::compat::i32 total_milliseconds{};
        openswd3::compat::i32 current_milliseconds{};
        stream_backend->stream_ms_position(
            stream, total_milliseconds, current_milliseconds
        );
        test.expect_true(
            stream_backend->stream_user_data(stream, 0U) == 100 &&
                stream_backend->stream_volume(stream) == 96 &&
                stream_backend->stream_status(stream) == 4U &&
                total_milliseconds >= 4'000 && total_milliseconds <= 4'050 &&
                current_milliseconds >= 0,
            "real MP3 playback exposes Miles-compatible state and duration"
        );
        stream_backend->close_stream(stream);
    }

    auto world_music_backend =
        openswd3::media_ffmpeg::make_legacy_stream_backend(data_root);
    openswd3::audio_video::LegacyStreamManager world_music_manager{
        *world_music_backend
    };
    test.expect_equal(
        world_music_manager.initialize_pool(1U),
        openswd3::audio_video::LegacyStreamManagerInitializeStatus::ready,
        "the real world-music stream pool initializes"
    );
    std::ifstream maps_file{data_root / "MAPS.DAT", std::ios::binary};
    maps_file.seekg(0, std::ios::end);
    const auto maps_size = maps_file.tellg();
    std::vector<u8> maps_payload;
    if (maps_size > std::streamoff{0x200}) {
        maps_payload.resize(static_cast<std::size_t>(maps_size) - 0x200U);
        maps_file.seekg(0x200, std::ios::beg);
        maps_file.read(
            reinterpret_cast<char*>(maps_payload.data()),
            static_cast<std::streamsize>(maps_payload.size())
        );
    }
    openswd3::audio_video::LegacyWorldMusicState world_music{
        .mix_level = 6,
    };
    RealWorldMusicPorts world_music_ports{world_music_manager, maps_payload};
    const auto map_music =
        openswd3::audio_video::update_legacy_world_music_request_from_maps(
            world_music, maps_payload, 214U, world_music_ports
        );
    static_cast<void>(openswd3::audio_video::service_legacy_world_music(
        world_music, "", world_music_ports
    ));
    test.expect_true(
        map_music == openswd3::audio_video::LegacyWorldMusicMapsStatus::ready &&
            world_music_ports.requested_filename == "Music\\Map_Ca12.mp3" &&
            world_music_manager.active_stream_count() == 1U &&
            !world_music_manager.stream_absent(100),
        "real MAPS map 214 resolves Map_Ca12 and starts FFmpeg BGM stream 100"
    );
    static_cast<void>(world_music_manager.shutdown());

    const auto verify_bink = [&](const std::string_view filename,
                                 const u32 expected_frame_count,
                                 const bool require_nonzero_pixels,
                                 const bool decode_to_eof,
                                 const std::string_view message) {
        auto video_backend =
            openswd3::media_ffmpeg::make_legacy_video_backend();
        const auto opened = video_backend->open_video(
            (data_root / "Video" / filename).string()
        );
        test.expect_true(
            opened.disposition ==
                    openswd3::audio_video::LegacyVideoOpenDisposition::opened &&
                opened.handle != 0U && opened.summary.width == 640 &&
                opened.summary.height == 480,
            message
        );
        if (opened.handle == 0U) {
            return;
        }

        test.expect_false(
            video_backend->wait_for_video_frame(opened.handle),
            "the first Bink frame is immediately due"
        );
        const auto first_decode =
            video_backend->decode_video_frame(opened.handle);
        VideoFramePorts ports;
        const openswd3::audio_video::LegacyVideoCopyRequest request{
            .destination = ports.video_destination_pixels(),
            .pitch_bytes = ports.video_destination_pitch_bytes(),
            .destination_height = 480,
            .destination_x = 0,
            .destination_y = 0,
            .pixel_format = ports.video_pixel_format(),
        };
        const auto copied =
            video_backend->copy_video_frame(opened.handle, request);
        const bool pixels_valid = !require_nonzero_pixels ||
            std::ranges::any_of(ports.pixels, [](const u16 pixel) {
                return pixel != 0U;
            });
        test.expect_true(
            first_decode ==
                    openswd3::audio_video::LegacyVideoDecodeStatus::
                        frame_ready &&
                copied == 1 &&
                video_backend->video_frame_number(opened.handle) == 1U &&
                video_backend->video_frame_count(opened.handle) ==
                    expected_frame_count &&
                pixels_valid,
            "the first real Bink frame decodes and copies to RGB565"
        );
        video_backend->service_video(opened.handle);
        video_backend->advance_video_frame(opened.handle);
        if (video_backend->wait_for_video_frame(opened.handle)) {
            SDL_Delay(40U);
        }
        const auto second_decode =
            video_backend->decode_video_frame(opened.handle);
        video_backend->service_video(opened.handle);
        test.expect_true(
            second_decode ==
                    openswd3::audio_video::LegacyVideoDecodeStatus::
                        frame_ready &&
                video_backend->video_frame_number(opened.handle) == 2U,
            "the second Bink frame demuxes with embedded audio packets"
        );

        if (decode_to_eof) {
            video_backend->advance_video_frame(opened.handle);
            auto terminal_status =
                openswd3::audio_video::LegacyVideoDecodeStatus::frame_ready;
            while (video_backend->video_frame_number(opened.handle) <=
                   expected_frame_count) {
                terminal_status =
                    video_backend->decode_video_frame(opened.handle);
                if (terminal_status !=
                    openswd3::audio_video::LegacyVideoDecodeStatus::
                        frame_ready) {
                    break;
                }
                video_backend->advance_video_frame(opened.handle);
            }
            test.expect_true(
                terminal_status ==
                        openswd3::audio_video::LegacyVideoDecodeStatus::
                            completed &&
                    video_backend->video_frame_number(opened.handle) ==
                        expected_frame_count &&
                    video_backend->video_frame_count(opened.handle) ==
                        expected_frame_count,
                "the complete short Bink stream reaches explicit EOF without " "a repeated black frame"
            );
        }

        video_backend->close_video(opened.handle);
    };

    verify_bink(
        "firegod.bik",
        176U,
        true,
        true,
        "FFmpeg opens the short real Bink movie"
    );
    verify_bink(
        "opening.bik",
        7'369U,
        false,
        true,
        "FFmpeg opens the real OP Bink movie"
    );
}

void test_real_archive_formats(
    openswd3::test::Context& test, const std::filesystem::path& archive_path
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
        if (entry == nullptr || entry->file_offset == 0U || sound_id == 270U ||
            sound_id == 277U) {
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
                loaded.bytes, 44'100
            )
                .status,
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
    if (argument_count >= 2) {
        test_real_archive_formats(test, arguments[1]);
    }
    if (argument_count == 3) {
        test_real_ffmpeg_media(test, arguments[2]);
    }
    return test.exit_code();
}
