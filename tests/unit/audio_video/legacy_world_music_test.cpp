#include "test.hpp"

#include "openswd3/audio_video/legacy_world_music.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using openswd3::audio_video::LegacyWorldMusicPorts;
using openswd3::audio_video::LegacyWorldMusicState;
using openswd3::audio_video::LegacyWorldMusicTableEntry;
using openswd3::audio_video::build_legacy_music_path;
using openswd3::audio_video::service_legacy_world_music;
using openswd3::audio_video::update_legacy_world_music_request;
using openswd3::compat::i32;
using openswd3::compat::u32;

enum class PortCall {
    poll_transition,
    stream_absent,
    configure_transition,
    apply_transition,
    source_filename,
    play_stream,
    set_volume,
};

struct PortEvent {
    PortCall call{};
    i32 first{};
    i32 second{};
    std::string text;

    bool operator==(const PortEvent&) const = default;
};

[[nodiscard]] PortEvent event(
    const PortCall call,
    const i32 first = 0,
    const i32 second = 0,
    std::string text = {}
) {
    return {call, first, second, std::move(text)};
}

class RecordingPorts final : public LegacyWorldMusicPorts {
public:
    void poll_stream_transition() override {
        events.push_back(event(PortCall::poll_transition));
    }

    bool music_stream_absent() override {
        events.push_back(event(PortCall::stream_absent));
        return stream_absent;
    }

    void configure_stream_transition(const i32 mode, const i32 value) override {
        events.push_back(event(PortCall::configure_transition, mode, value));
    }

    void apply_stream_transition() override {
        events.push_back(event(PortCall::apply_transition));
    }

    std::string_view music_source_filename(const u32 music_id) override {
        events.push_back(
            event(PortCall::source_filename, static_cast<i32>(music_id))
        );
        last_resolved_id = music_id;
        return source_filename;
    }

    void play_music_stream(const std::string_view filename) override {
        events.push_back(
            event(PortCall::play_stream, 0, 0, std::string{filename})
        );
    }

    void set_music_stream_volume(const i32 mix_level) override {
        events.push_back(event(PortCall::set_volume, mix_level));
    }

    bool stream_absent{true};
    u32 last_resolved_id{};
    std::string source_filename{"Map_Ca00.wav"};
    std::vector<PortEvent> events;
};

void test_path_construction(openswd3::test::Context& test) {
    test.expect_equal(
        build_legacy_music_path("D:\\swd3\\", "Map_Ca00.wav"),
        std::optional<std::string>{"D:\\swd3\\Music\\Map_Ca00.mp3"},
        "0x0040EB60 preserves prefix and replaces extension with mp3"
    );
    test.expect_equal(
        build_legacy_music_path("", "Story.11.wave"),
        std::optional<std::string>{"Music\\Story.mp3"},
        "the first period terminates the copied legacy basename"
    );
    test.expect_equal(
        build_legacy_music_path("", "MissingExtension"),
        std::optional<std::string>{},
        "invalid host data is isolated instead of scanning beyond its view"
    );
}

void test_map_request_update(openswd3::test::Context& test) {
    constexpr std::array table{
        LegacyWorldMusicTableEntry{7U, 101U, 102U, 0x6000U},
        LegacyWorldMusicTableEntry{},
        LegacyWorldMusicTableEntry{8U, 201U, 202U, 0U},
    };

    {
        RecordingPorts ports;
        ports.stream_absent = false;
        LegacyWorldMusicState state;
        state.request_flags = 0x001FFFFFU;
        state.music_slots[1U] = 1U;
        state.music_slots[2U] = 2U;

        update_legacy_world_music_request(state, table, 7U, ports);
        test.expect_equal(state.music_slots[1U], 101U, "first table value");
        test.expect_equal(state.music_slots[2U], 102U, "second table value");
        test.expect_equal(
            state.request_flags,
            0x000C0000U,
            "normal-group change clears flags then applies entry bits"
        );
        test.expect_equal(
            ports.events,
            std::vector<PortEvent>{
                event(PortCall::stream_absent),
                event(PortCall::configure_transition, 2, 15),
                event(PortCall::apply_transition),
            },
            "active music receives the exact fade setup call order"
        );
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.request_flags = 0x008C1234U;
        state.music_slots[1U] = 1U;
        state.music_slots[2U] = 2U;

        update_legacy_world_music_request(state, table, 7U, ports);
        test.expect_equal(
            state.request_flags,
            0x008C1234U,
            "alternate group preserves its high group bit and request pair"
        );
        test.expect_true(
            ports.events.empty(),
            "alternate group does not query or fade the current stream"
        );
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.request_flags = 0x000C0055U;
        state.music_slots[1U] = 9U;
        state.music_slots[2U] = 10U;

        update_legacy_world_music_request(state, table, 99U, ports);
        test.expect_equal(state.music_slots[1U], 0U, "missing first slot zero");
        test.expect_equal(
            state.music_slots[2U], 0U, "missing second slot zero"
        );
        test.expect_equal(
            state.request_flags,
            0x00000055U,
            "missing entry clears only the paired request flags"
        );
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.music_slots[1U] = 101U;
        state.music_slots[2U] = 102U;

        update_legacy_world_music_request(state, table, 7U, ports);
        test.expect_true(
            ports.events.empty(),
            "unchanged table values return before stream coordination"
        );
    }
}

void test_world_music_service(openswd3::test::Context& test) {
    {
        RecordingPorts ports;
        ports.stream_absent = false;
        LegacyWorldMusicState state;
        state.request_flags = 2U;

        test.expect_true(
            service_legacy_world_music(state, "", ports),
            "all machine-level exits report one"
        );
        test.expect_equal(
            ports.events,
            std::vector<PortEvent>{
                event(PortCall::poll_transition),
                event(PortCall::stream_absent),
            },
            "an existing stream gates request consumption"
        );
        test.expect_equal(state.request_flags, 2U, "gated flags unchanged");
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.request_flags = 0U;
        state.mix_level = 9;
        state.music_slots[0U] = 0x80000002U;
        state.music_slots[1U] = 42U;
        state.music_slots[3U] = 0x80000001U;

        static_cast<void>(service_legacy_world_music(state, "R:\\", ports));
        test.expect_equal(
            state.selected_mode,
            2U,
            "slot zero pending mode overwrites slot three pending mode"
        );
        test.expect_equal(
            state.music_slots[0U], 2U, "slot zero pending bit is consumed"
        );
        test.expect_equal(
            state.music_slots[3U],
            1U,
            "slot three pending bit is consumed first"
        );
        test.expect_equal(state.request_flags, 1U, "mode advances to one");
        test.expect_equal(
            ports.last_resolved_id, 42U, "normal slot one chosen"
        );
        test.expect_equal(
            ports.events,
            std::vector<PortEvent>{
                event(PortCall::poll_transition),
                event(PortCall::stream_absent),
                event(PortCall::source_filename, 42),
                event(PortCall::play_stream, 0, 0, "R:\\Music\\Map_Ca00.mp3"),
                event(PortCall::set_volume, 9),
            },
            "resolve, play and volume order follows 0x0040CECD-0x0040CEE5"
        );
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.request_flags = 2U;
        state.music_slots[3U] = 88U;

        static_cast<void>(service_legacy_world_music(state, "", ports));
        test.expect_equal(
            state.request_flags,
            3U,
            "mode above two without restart flag collapses to three"
        );
        test.expect_equal(
            ports.events.size(),
            std::size_t{2U},
            "collapse returns before catalog lookup"
        );
    }

    {
        RecordingPorts ports;
        ports.source_filename = "Story_50.mid";
        LegacyWorldMusicState state;
        state.request_flags = 0x008A0002U;
        state.mix_level = 11;
        state.music_slots[5U] = 77U;

        static_cast<void>(service_legacy_world_music(state, "", ports));
        test.expect_equal(
            ports.last_resolved_id,
            77U,
            "alternate restart without first-slot flag selects group slot two"
        );
        test.expect_equal(
            state.request_flags,
            0x00880002U,
            "successful play clears 0x20000 and writes mode two"
        );
    }

    {
        RecordingPorts ports;
        LegacyWorldMusicState state;
        state.music_slots[1U] = 0x8001U;

        static_cast<void>(service_legacy_world_music(state, "", ports));
        test.expect_equal(
            ports.events.size(),
            std::size_t{2U},
            "music IDs with bit 0x8000 do not reach the catalog"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_path_construction(test);
    test_map_request_update(test);
    test_world_music_service(test);
    return test.exit_code();
}
