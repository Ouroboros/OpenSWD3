#include "test.hpp"

#include "openswd3/audio_video/legacy_stream_commands.hpp"
#include "openswd3/audio_video/legacy_stream_manager.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacyStreamBackend;
using openswd3::audio_video::LegacyStreamCommandState;
using openswd3::audio_video::LegacyStreamHandle;
using openswd3::audio_video::LegacyStreamManager;
using openswd3::audio_video::LegacyStreamManagerInitializeStatus;
using openswd3::audio_video::apply_legacy_stream_transition;
using openswd3::audio_video::configure_legacy_stream_transition;
using openswd3::audio_video::legacy_stream_absent;
using openswd3::audio_video::play_legacy_stream;
using openswd3::audio_video::poll_legacy_stream_transition;
using openswd3::audio_video::set_legacy_stream_volume;
using openswd3::audio_video::stop_legacy_stream;
using openswd3::compat::i32;
using openswd3::compat::u32;

enum class BackendCall {
    open,
    close,
    set_user_data,
    get_user_data,
    set_volume,
    get_volume,
    set_loop_count,
    start,
    status,
    position,
};

struct BackendEvent {
    BackendCall call{};
    u32 handle{};
    i32 value{};
    u32 auxiliary{};

    bool operator==(const BackendEvent&) const = default;
};

class RecordingBackend final : public LegacyStreamBackend {
public:
    LegacyStreamHandle open_stream(
        const u32 driver_token,
        const std::string_view filename,
        const i32 file_offset
    ) override {
        last_driver_token = driver_token;
        last_filename = filename;
        last_file_offset = file_offset;
        const u32 handle = open_fails ? 0U : next_handle++;
        events.push_back({BackendCall::open, handle, file_offset});
        return handle;
    }

    std::string_view last_error() const override {
        return error_text;
    }

    void close_stream(const LegacyStreamHandle handle) override {
        events.push_back({BackendCall::close, handle});
        ++close_counts[handle];
    }

    void set_stream_user_data(
        const LegacyStreamHandle handle,
        const u32 slot,
        const i32 value
    ) override {
        events.push_back({BackendCall::set_user_data, handle, value, slot});
        user_data[handle] = value;
    }

    i32 stream_user_data(
        const LegacyStreamHandle handle,
        const u32 slot
    ) override {
        events.push_back({BackendCall::get_user_data, handle, 0, slot});
        return user_data[handle];
    }

    void set_stream_volume(
        const LegacyStreamHandle handle,
        const i32 volume
    ) override {
        events.push_back({BackendCall::set_volume, handle, volume});
        volumes[handle] = volume;
        volume_history.push_back(volume);
        ++volume_set_counts[handle];
    }

    i32 stream_volume(const LegacyStreamHandle handle) override {
        events.push_back({BackendCall::get_volume, handle});
        return volumes[handle];
    }

    void set_stream_loop_count(
        const LegacyStreamHandle handle,
        const i32 loop_count
    ) override {
        events.push_back({BackendCall::set_loop_count, handle, loop_count});
        loops[handle] = loop_count;
    }

    void start_stream(const LegacyStreamHandle handle) override {
        events.push_back({BackendCall::start, handle});
    }

    u32 stream_status(const LegacyStreamHandle handle) override {
        events.push_back({BackendCall::status, handle});
        return statuses[handle];
    }

    void stream_ms_position(
        const LegacyStreamHandle handle,
        i32& total_milliseconds,
        i32& current_milliseconds
    ) override {
        events.push_back({BackendCall::position, handle});
        total_milliseconds = 9000;
        current_milliseconds = 3000;
    }

    [[nodiscard]] std::size_t call_count(const BackendCall call) const {
        std::size_t count{};
        for (const BackendEvent& event : events) {
            if (event.call == call) {
                ++count;
            }
        }
        return count;
    }

    void clear_events() {
        events.clear();
        volume_history.clear();
        volume_set_counts.fill(0U);
        close_counts.fill(0U);
    }

    bool open_fails{};
    u32 next_handle{1U};
    u32 last_driver_token{};
    std::string last_filename;
    i32 last_file_offset{};
    std::string error_text{"backend open failure"};
    std::array<i32, 16U> user_data{};
    std::array<i32, 16U> volumes{};
    std::array<i32, 16U> loops{};
    std::array<u32, 16U> statuses{};
    std::array<u32, 16U> volume_set_counts{};
    std::array<u32, 16U> close_counts{};
    std::vector<i32> volume_history;
    std::vector<BackendEvent> events;
};

void expect_initialized(
    openswd3::test::Context& test,
    LegacyStreamManager& manager,
    const u32 driver_token = 0x12345678U
) {
    test.expect_equal(
        manager.initialize_pool(driver_token),
        LegacyStreamManagerInitializeStatus::ready,
        "stream pool initialization succeeds"
    );
    test.expect_true(manager.initialized(), "initialized flag is one");
    test.expect_true(manager.stream_enabled(), "enabled flag is one");
    test.expect_equal(manager.driver_token(), driver_token, "driver stored");
}

void test_pool_play_and_volume(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyStreamManager manager(backend);

    test.expect_equal(
        manager.play("Music\\before-init.mp3", 100, 80, 1),
        0,
        "empty pre-initialization free list rejects play"
    );
    test.expect_equal(
        manager.set_volume(100, 80),
        0,
        "pre-initialization volume update returns zero"
    );
    expect_initialized(test, manager);
    test.expect_equal(manager.free_stream_count(), 2U, "two free nodes");

    test.expect_equal(
        manager.play("Music\\Battle.mp3", 100, 200, 3),
        100,
        "successful play returns its stream ID"
    );
    test.expect_equal(backend.last_driver_token, 0x12345678U, "open driver");
    test.expect_equal(
        backend.last_filename,
        std::string{"Music\\Battle.mp3"},
        "open filename"
    );
    test.expect_equal(backend.last_file_offset, 0, "open offset is zero");
    test.expect_equal(backend.user_data[1U], 100, "user data slot zero");
    test.expect_equal(backend.volumes[1U], 127, "play volume clamped");
    test.expect_equal(backend.loops[1U], 3, "loop count forwarded");
    test.expect_equal(manager.active_stream_count(), 1U, "one active node");
    test.expect_equal(manager.free_stream_count(), 1U, "one free node");

    const std::size_t opens_before_duplicate =
        backend.call_count(BackendCall::open);
    test.expect_equal(
        manager.play("Music\\duplicate.mp3", 100, 10, 1),
        0,
        "duplicate stream ID is rejected"
    );
    test.expect_equal(
        backend.call_count(BackendCall::open),
        opens_before_duplicate,
        "duplicate check happens before open"
    );

    test.expect_equal(
        manager.play("Music\\second.mp3", 200, 64, 0),
        200,
        "second free node can play"
    );
    test.expect_equal(
        manager.play("Music\\third.mp3", 300, 64, 1),
        0,
        "two-node pool rejects a third stream"
    );
    test.expect_equal(manager.active_stream_count(), 2U, "two active nodes");

    test.expect_equal(
        manager.set_volume(100, -4),
        0,
        "set-volume returns backend readback"
    );
    test.expect_equal(backend.volumes[1U], 0, "negative volume clamps zero");
    test.expect_equal(
        manager.set_volume(999, 40),
        -1,
        "missing stream returns minus one"
    );

    backend.clear_events();
    test.expect_true(manager.shutdown(), "shutdown returns one");
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::close, 2U},
            {BackendCall::close, 1U},
        },
        "shutdown closes active head order"
    );
}

void test_open_failure_and_free_shutdown(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyStreamManager manager(backend);
    expect_initialized(test, manager, 7U);
    backend.open_fails = true;

    test.expect_equal(
        manager.play("Music\\missing.mp3", 100, 80, 1),
        0,
        "backend open failure returns zero"
    );
    test.expect_equal(
        manager.last_error(),
        std::string_view{"backend open failure"},
        "backend error is copied"
    );
    test.expect_equal(manager.active_stream_count(), 0U, "no active stream");
    test.expect_equal(manager.free_stream_count(), 2U, "node recycled");

    backend.clear_events();
    static_cast<void>(manager.shutdown());
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::close, 0U},
            {BackendCall::close, 0U},
        },
        "original shutdown closes zero handles on free nodes"
    );
}

void test_fade_service(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyStreamManager manager(backend);
    expect_initialized(test, manager);
    static_cast<void>(manager.play("Music\\fade.mp3", 100, 32, 1));
    backend.clear_events();

    test.expect_equal(
        manager.begin_fade(100, 4),
        100,
        "fade returns the matching ID"
    );
    test.expect_equal(
        backend.call_count(BackendCall::position),
        1U,
        "fade queries and ignores millisecond position"
    );
    backend.clear_events();

    test.expect_false(manager.service(), "service return is always zero");
    test.expect_false(manager.service(), "second service return is zero");
    test.expect_false(manager.service(), "third service return is zero");
    test.expect_false(manager.service(), "removal service return is zero");
    test.expect_equal(
        backend.volume_history,
        std::vector<i32>{24, 16, 8, 0},
        "fixed-point fade emits exact per-service volumes"
    );
    test.expect_equal(backend.close_counts[1U], 1U, "fade closes at zero");
    test.expect_equal(manager.active_stream_count(), 0U, "fade removed node");
    test.expect_equal(manager.free_stream_count(), 2U, "fade recycled node");

    manager.set_stream_enabled(false);
    test.expect_equal(
        manager.play("Music\\disabled-fade.mp3", 100, 20, 1),
        100,
        "play does not consult the stream-enabled flag"
    );
    test.expect_equal(
        manager.set_volume(100, 21),
        21,
        "volume update does not consult the stream-enabled flag"
    );
    backend.clear_events();
    test.expect_equal(
        manager.begin_fade(100, 1),
        0,
        "disabled manager rejects fade"
    );
    test.expect_equal(
        backend.call_count(BackendCall::position),
        0U,
        "disabled fade stops before position query"
    );
}

void test_status_switch_and_cascade(openswd3::test::Context& test) {
    {
        RecordingBackend backend;
        LegacyStreamManager manager(backend);
        expect_initialized(test, manager);
        static_cast<void>(manager.play("one", 1, 20, 1));
        static_cast<void>(manager.play("two", 2, 20, 1));
        backend.statuses[2U] = 2U;
        backend.statuses[1U] = 3U;
        backend.clear_events();

        static_cast<void>(manager.service());
        test.expect_equal(
            manager.active_stream_count(),
            0U,
            "status two cascades removal into following default status"
        );
        test.expect_equal(
            backend.volume_set_counts[2U],
            2U,
            "status two writes zero twice"
        );
        test.expect_equal(
            backend.volume_set_counts[1U],
            1U,
            "cascaded default removal writes zero once"
        );
        test.expect_equal(backend.close_counts[2U], 1U, "status two closes");
        test.expect_equal(backend.close_counts[1U], 1U, "cascade closes");
    }

    {
        RecordingBackend backend;
        LegacyStreamManager manager(backend);
        expect_initialized(test, manager);
        expect_initialized(test, manager);
        expect_initialized(test, manager);
        static_cast<void>(manager.play("default", 1, 20, 1));
        static_cast<void>(manager.play("retained-four", 2, 20, 1));
        static_cast<void>(manager.play("retained-eight", 3, 20, 1));
        static_cast<void>(manager.play("retained-sixteen", 4, 20, 1));
        static_cast<void>(manager.play("removed", 5, 20, 1));
        backend.statuses[5U] = 2U;
        backend.statuses[4U] = 16U;
        backend.statuses[3U] = 8U;
        backend.statuses[2U] = 4U;
        backend.statuses[1U] = 3U;
        backend.clear_events();

        static_cast<void>(manager.service());
        test.expect_equal(
            manager.active_stream_count(),
            4U,
            "statuses four, eight and sixteen retain and clear cascade"
        );
        test.expect_equal(backend.close_counts[5U], 1U, "head removed");
        test.expect_equal(backend.close_counts[4U], 0U, "status 16 retained");
        test.expect_equal(backend.close_counts[3U], 0U, "status 8 retained");
        test.expect_equal(backend.close_counts[2U], 0U, "status 4 retained");
        test.expect_equal(backend.close_counts[1U], 0U, "default retained");
    }
}

void test_stream_wrappers(openswd3::test::Context& test) {
    RecordingBackend backend;
    LegacyStreamManager manager(backend);
    expect_initialized(test, manager);

    test.expect_equal(
        play_legacy_stream(manager, "Music\\Map.mp3", 0, 11),
        0,
        "0x004856C0 returns zero when stream gate is zero"
    );
    test.expect_equal(backend.call_count(BackendCall::open), 0U, "gate blocks");
    test.expect_equal(
        play_legacy_stream(manager, "Music\\Map.mp3", 1, 11),
        1,
        "0x004856C0 returns one after submitting play"
    );
    test.expect_equal(backend.user_data[1U], 100, "wrapper stream ID is 100");
    test.expect_equal(backend.volumes[1U], 127, "11*128/11 clamps 127");
    test.expect_equal(backend.loops[1U], 1, "wrapper loops once");
    test.expect_equal(legacy_stream_absent(manager), 0, "active stream present");

    test.expect_equal(
        set_legacy_stream_volume(manager, 5),
        58,
        "0x00485850 scales level and returns backend volume"
    );
    test.expect_equal(backend.volumes[1U], 58, "scaled volume reaches backend");

    LegacyStreamCommandState state{
        .transition_mode = 2,
        .current_fade_divisor = 0,
        .pending_fade_divisor = 15,
        .mix_level = 11,
    };
    test.expect_equal(
        apply_legacy_stream_transition(manager, state),
        100,
        "mode two starts configured fade"
    );
    test.expect_equal(state.current_fade_divisor, 15, "current divisor copied");
    test.expect_equal(
        poll_legacy_stream_transition(manager, state),
        0,
        "mode two remains active while stream exists"
    );

    test.expect_equal(stop_legacy_stream(manager), 100, "stop uses divisor one");
    static_cast<void>(manager.service());
    test.expect_equal(legacy_stream_absent(manager), 1, "service removes stream");
    test.expect_equal(
        poll_legacy_stream_transition(manager, state),
        1,
        "absent mode-two stream completes transition"
    );
    test.expect_equal(state.transition_mode, 0, "poll clears transition mode");
    test.expect_equal(state.current_fade_divisor, 0, "poll clears current value");

    state.pending_fade_divisor = 7;
    test.expect_equal(
        configure_legacy_stream_transition(state, 2, 30),
        30,
        "mode two configuration returns and stores value"
    );
    test.expect_equal(state.pending_fade_divisor, 30, "pending value stored");
    test.expect_equal(
        configure_legacy_stream_transition(state, 16, 25),
        16,
        "non-two configuration returns mode"
    );
    test.expect_equal(
        state.pending_fade_divisor,
        30,
        "non-two configuration leaves pending value"
    );

    state.transition_mode = 0;
    test.expect_equal(
        apply_legacy_stream_transition(manager, state),
        -2,
        "mode zero returns mode minus two"
    );
    state.transition_mode = 1;
    state.current_fade_divisor = 9;
    test.expect_equal(
        apply_legacy_stream_transition(manager, state),
        0,
        "mode one clears state even without a stream"
    );
    test.expect_equal(state.transition_mode, 0, "mode one clears mode");
    test.expect_equal(state.current_fade_divisor, 0, "mode one clears current");

    state.transition_mode = 1;
    test.expect_equal(
        poll_legacy_stream_transition(manager, state),
        -1,
        "poll mode one returns minus one"
    );
    state.transition_mode = 3;
    test.expect_equal(
        poll_legacy_stream_transition(manager, state),
        1,
        "poll mode three returns one without changing state"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_pool_play_and_volume(test);
    test_open_failure_and_free_shutdown(test);
    test_fade_service(test);
    test_status_switch_and_cascade(test);
    test_stream_wrappers(test);
    return test.exit_code();
}
