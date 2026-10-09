#include "test.hpp"

#include "openswd3/audio_video/legacy_world_music.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace {

using namespace openswd3::audio_video;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class MusicBackend final : public LegacyStreamBackend {
public:
    u32 open_stream(u32, std::string_view filename, i32) override {
        paths.emplace_back(filename);
        operations.push_back('o');
        return open_fails ? 0U : 1U;
    }

    std::string_view last_error() const override {
        return "open failed";
    }

    void close_stream(u32) override {
        operations.push_back('c');
        playing = false;
    }

    void set_stream_user_data(u32, u32, i32 value) override {
        stream_id = value;
    }

    i32 stream_user_data(u32, u32) override {
        return stream_id;
    }

    void set_stream_volume(u32, i32 value) override {
        volume = value;
        volumes.push_back(value);
        operations.push_back('v');
    }

    i32 stream_volume(u32) override {
        return volume;
    }

    void set_stream_loop_count(u32, i32 value) override {
        loop_count = value;
    }

    void start_stream(u32) override {
        playing = true;
        operations.push_back('s');
        if (after_start) {
            after_start();
        }
    }

    u32 stream_status(u32) override {
        return playing ? 4U : 2U;
    }

    void stream_ms_position(u32, i32& total, i32& current) override {
        total = 1000;
        current = 0;
    }

    std::vector<std::string> paths;
    std::vector<i32> volumes;
    std::vector<char> operations;
    std::function<void()> after_start;
    i32 stream_id{};
    i32 volume{};
    i32 loop_count{};
    bool playing{};
    bool open_fails{};
};

void write_u16(std::span<u8> bytes, std::size_t offset, u16 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(std::span<u8> bytes, std::size_t offset, u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

struct Fixture {
    LegacyWorldMusicState state;
    MusicBackend backend;
    LegacyStreamManager streams{backend};
    std::array<u8, 2048> maps{};

    Fixture() {
        static_cast<void>(streams.initialize_pool(1U));
    }

    LegacyWorldMusicBindings bindings() {
        return {
            state.request_flags,
            state.selected_mode,
            {state.music_slots[0],
             state.music_slots[1],
             state.music_slots[2],
             state.music_slots[3],
             state.music_slots[4],
             state.music_slots[5]},
            state.mix_level,
            state.current_fade_divisor,
            state.pending_fade_divisor,
        };
    }

    void source(u32 id, std::string_view filename) {
        maps.fill(0U);
        write_u32(maps, 8U, 0x20U);
        write_u32(maps, 0x20U, 0x40U);
        write_u32(maps, 0x40U + id * 4U, 0x600U);
        std::ranges::copy(filename, maps.begin() + 0x604U);
    }
};

void test_maps_music_directory(openswd3::test::Context& test) {
    std::array<u8, 0xA0U> maps{};
    write_u32(maps, 0x08U, 0x20U);
    write_u32(maps, 0x20U, 0x40U);
    write_u32(maps, 0x24U, 0x30U);
    write_u32(maps, 0x30U, 0x50U);
    write_u16(maps, 0x50U, 7U);
    write_u16(maps, 0x52U, 1U);
    write_u16(maps, 0x54U, 2U);
    write_u16(maps, 0x56U, 0x2000U);
    write_u32(maps, 0x44U, 0x70U);
    write_u32(maps, 0x48U, 0x84U);
    constexpr std::string_view first_name{"Map_Ca12.wav"};
    constexpr std::string_view second_name{"Map_Eu01.wav"};
    std::ranges::copy(first_name, maps.begin() + 0x74U);
    std::ranges::copy(second_name, maps.begin() + 0x88U);
    Fixture fixture;
    test.expect_equal(
        update_legacy_world_music_request_from_maps(
            fixture.bindings(), maps, 7U, fixture.streams
        ),
        LegacyWorldMusicMapsStatus::ready,
        "MAPS directories resolve the map record"
    );
    test.expect_true(
        fixture.state.music_slots[1] == 1U &&
            fixture.state.music_slots[2] == 2U &&
            fixture.state.request_flags == 0x00080000U,
        "the eight-byte record publishes both IDs and restart flags"
    );
    test.expect_equal(
        legacy_music_source_filename_from_maps(maps, 1U),
        std::optional<std::string_view>{first_name},
        "the first filename resolves through MAPS"
    );
    test.expect_equal(
        legacy_music_source_filename_from_maps(maps, 2U),
        std::optional<std::string_view>{second_name},
        "the second filename skips its record prefix"
    );
    test.expect_equal(
        update_legacy_world_music_request_from_maps(
            fixture.bindings(), maps, 8U, fixture.streams
        ),
        LegacyWorldMusicMapsStatus::map_not_found,
        "the zero map ID terminates lookup"
    );
    test.expect_equal(
        update_legacy_world_music_request_from_maps(
            fixture.bindings(),
            std::span<const u8>{maps}.first(12U),
            7U,
            fixture.streams
        ),
        LegacyWorldMusicMapsStatus::payload_out_of_range,
        "a truncated directory remains an explicit data error"
    );
}

void test_path_construction(openswd3::test::Context& test) {
    test.expect_equal(
        build_legacy_music_path("D:\\swd3\\", "Map_Ca00.wav"),
        std::optional<std::string>{"D:\\swd3\\Music\\Map_Ca00.mp3"},
        "the base prefix is preserved"
    );
    test.expect_equal(
        build_legacy_music_path("", "Story.11.wave"),
        std::optional<std::string>{"Music\\Story.mp3"},
        "the first period ends the basename"
    );
    test.expect_equal(
        build_legacy_music_path("", "MissingExtension"),
        std::optional<std::string>{},
        "a missing period does not scan beyond host memory"
    );
}

void test_map_request_update(openswd3::test::Context& test) {
    constexpr std::array table{
        LegacyWorldMusicTableEntry{7U, 101U, 102U, 0x6000U},
        LegacyWorldMusicTableEntry{},
        LegacyWorldMusicTableEntry{8U, 201U, 202U, 0U},
    };
    {
        Fixture fixture;
        static_cast<void>(fixture.streams.play("existing.mp3", 100, 64, 1));
        fixture.state.request_flags = 0x001FFFFFU;
        update_legacy_world_music_request(
            fixture.bindings(), table, 7U, fixture.streams
        );
        test.expect_true(
            fixture.state.music_slots[1] == 101U &&
                fixture.state.music_slots[2] == 102U &&
                fixture.state.request_flags == 0x000C0000U &&
                fixture.state.selected_mode == 2U &&
                fixture.state.pending_fade_divisor == 15U &&
                fixture.state.current_fade_divisor == 15U,
            "a normal-group change publishes IDs and the actual shared fade state"
        );
        static_cast<void>(fixture.streams.service());
        test.expect_equal(
            fixture.backend.volume,
            59,
            "the real stream begins fading with divisor fifteen"
        );
    }

    {
        Fixture fixture;
        fixture.state.request_flags = 0x008C1234U;
        update_legacy_world_music_request(
            fixture.bindings(), table, 7U, fixture.streams
        );
        test.expect_true(
            fixture.state.request_flags == 0x008C1234U &&
                fixture.state.selected_mode == 0U &&
                fixture.state.pending_fade_divisor == 0U,
            "the alternate group retains flags without starting a fade"
        );
    }

    {
        Fixture fixture;
        fixture.state.request_flags = 0x000C0055U;
        fixture.state.music_slots[1] = 9U;
        fixture.state.music_slots[2] = 10U;
        update_legacy_world_music_request(
            fixture.bindings(), table, 99U, fixture.streams
        );
        test.expect_true(
            fixture.state.music_slots[1] == 0U &&
                fixture.state.music_slots[2] == 0U &&
                fixture.state.request_flags == 0x55U,
            "a missing map clears only its IDs and paired flags"
        );
    }

    {
        Fixture fixture;
        fixture.state.music_slots[1] = 101U;
        fixture.state.music_slots[2] = 102U;
        fixture.state.selected_mode = 7U;
        fixture.state.request_flags = 0x12345678U;
        update_legacy_world_music_request(
            fixture.bindings(), table, 7U, fixture.streams
        );
        test.expect_true(
            fixture.state.selected_mode == 7U &&
                fixture.state.request_flags == 0x12345678U,
            "unchanged IDs return without clearing flags or configuring a fade"
        );
    }
}

void test_world_music_service(openswd3::test::Context& test) {
    {
        Fixture fixture;
        static_cast<void>(fixture.streams.play("existing.mp3", 100, 64, 1));
        fixture.state.request_flags = 2U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", fixture.maps, fixture.streams
        );
        test.expect_true(
            !result.requested_path && fixture.state.request_flags == 2U &&
                fixture.streams.active_stream_count() == 1U,
            "an existing stream prevents request consumption"
        );
    }

    {
        Fixture fixture;
        fixture.source(42U, "Map_Ca00.wav");
        fixture.state.mix_level = 9;
        fixture.state.music_slots[0] = 0x80000002U;
        fixture.state.music_slots[1] = 42U;
        fixture.state.music_slots[3] = 0x80000001U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "R:\\", fixture.maps, fixture.streams
        );
        test.expect_true(
            fixture.state.selected_mode == 2U &&
                fixture.state.music_slots[0] == 2U &&
                fixture.state.music_slots[3] == 1U &&
                fixture.state.request_flags == 1U,
            "slot zero consumes its pending bit after slot three and overwrites the selected mode"
        );
        test.expect_true(
            result.requested_path ==
                    std::optional<std::string>{"R:\\Music\\Map_Ca00.mp3"} &&
                result.playing &&
                fixture.backend.paths ==
                    std::vector<std::string>{*result.requested_path} &&
                fixture.backend.operations ==
                    std::vector<char>{'o', 'v', 's', 'v'} &&
                fixture.backend.volumes == std::vector<i32>{104, 104},
            "normal slot one resolves from actual MAPS data and plays before the independent volume write"
        );
    }

    {
        Fixture fixture;
        fixture.state.request_flags = 2U;
        fixture.state.music_slots[3] = 88U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", fixture.maps, fixture.streams
        );
        test.expect_true(
            fixture.state.request_flags == 3U && !result.requested_path &&
                !result.missing_source_id,
            "mode above two without restart returns before filename lookup"
        );
    }

    for (const u32 flags : {0x00AA0002U, 0x00820002U}) {
        Fixture fixture;
        fixture.source(77U, "Story_50.mid");
        fixture.state.request_flags = flags;
        fixture.state.music_slots[5] = 77U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", fixture.maps, fixture.streams
        );
        test.expect_true(
            result.requested_path ==
                    std::optional<std::string>{"Music\\Story_50.mp3"} &&
                result.playing &&
                fixture.state.request_flags == (flags & ~0x00200000U),
            "alternate restart selects its second slot and clears 0x200000 while retaining 0x20000"
        );
    }

    {
        Fixture fixture;
        fixture.state.music_slots[1] = 0x8001U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", fixture.maps, fixture.streams
        );
        test.expect_true(
            !result.requested_path && !result.missing_source_id &&
                fixture.state.request_flags == 1U,
            "suppressed music IDs skip MAPS lookup while advancing the mode"
        );
    }

    {
        Fixture fixture;
        fixture.state.selected_mode = 2U;
        fixture.state.current_fade_divisor = 15U;
        fixture.state.pending_fade_divisor = 15U;
        fixture.state.music_slots[1] = 42U;
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", {}, fixture.streams
        );
        test.expect_true(
            fixture.state.selected_mode == 0U &&
                fixture.state.current_fade_divisor == 0U &&
                fixture.state.pending_fade_divisor == 15U &&
                result.missing_source_id == 42U && !result.playing &&
                fixture.state.request_flags == 1U,
            "polling clears a completed fade before an invalid source reports its actual missing ID"
        );
    }

    for (const bool fail_open : {false, true}) {
        Fixture fixture;
        fixture.source(42U, "Map_Ca00.wav");
        fixture.state.request_flags = 0x00200000U;
        fixture.state.mix_level = 6;
        fixture.state.music_slots[1] = 42U;
        fixture.backend.open_fails = fail_open;
        fixture.backend.after_start = [&] {
            fixture.state.mix_level = -7;
            fixture.state.request_flags = 0x00AA4400U;
        };
        const auto result = service_legacy_world_music(
            fixture.bindings(), "", fixture.maps, fixture.streams
        );
        test.expect_true(
            result.requested_path.has_value() && result.playing == !fail_open &&
                fixture.streams.active_stream_count() ==
                    (fail_open ? 0U : 1U) &&
                fixture.streams.free_stream_count() == (fail_open ? 2U : 1U) &&
                fixture.state.request_flags == (fail_open ? 1U : 0x008A4401U),
            "playback failure retains the original flag updates without inventing an active stream"
        );
        test.expect_true(
            fixture.backend.volumes ==
                (fail_open ? std::vector<i32>{} : std::vector<i32>{69, 0}),
            "volume and request flags are reread from the actual shared data after playback"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_maps_music_directory(test);
    test_path_construction(test);
    test_map_request_update(test);
    test_world_music_service(test);
    return test.exit_code();
}
