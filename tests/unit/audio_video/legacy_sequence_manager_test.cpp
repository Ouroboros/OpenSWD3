#include "test.hpp"

#include "openswd3/audio_video/legacy_sequence_manager.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacyMidiDriverHandle;
using openswd3::audio_video::LegacySequenceBackend;
using openswd3::audio_video::LegacySequenceHandle;
using openswd3::audio_video::LegacySequenceManager;
using openswd3::audio_video::LegacySequenceManagerInitializeStatus;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u32;

enum class BackendCall {
    open_output,
    close_output,
    allocate,
    release,
    initialize,
    set_user_data,
    get_user_data,
    set_volume,
    set_loop_count,
    start,
    status,
    end,
};

struct BackendEvent {
    BackendCall call{};
    u32 handle{};
    i32 value{};
    u32 auxiliary{};

    bool operator==(const BackendEvent&) const = default;
};

class RecordingBackend final : public LegacySequenceBackend {
public:
    bool open_midi_output(
        const i32 device_id, LegacyMidiDriverHandle& driver
    ) override {
        const bool success = open_attempt < open_results.size()
            ? open_results[open_attempt]
            : true;
        error_text = open_attempt < open_errors.size()
            ? open_errors[open_attempt]
            : std::string{};
        ++open_attempt;
        driver = success ? output_driver : 0U;
        events.push_back({BackendCall::open_output, driver, device_id});
        return success;
    }

    std::string_view last_error() const override {
        return error_text;
    }

    void close_midi_output(const LegacyMidiDriverHandle driver) override {
        events.push_back({BackendCall::close_output, driver});
    }

    LegacySequenceHandle
    allocate_sequence_handle(const LegacyMidiDriverHandle driver) override {
        events.push_back({BackendCall::allocate, driver});
        return sequence_handle;
    }

    void release_sequence_handle(const LegacySequenceHandle handle) override {
        events.push_back({BackendCall::release, handle});
    }

    i32 initialize_sequence(
        const LegacySequenceHandle handle,
        const std::span<const u8> bytes,
        const u32 start_offset
    ) override {
        initialized_bytes.assign(bytes.begin(), bytes.end());
        events.push_back({
            BackendCall::initialize,
            handle,
            initialize_result,
            start_offset,
        });
        error_text = initialize_error;
        return initialize_result;
    }

    void set_sequence_user_data(
        const LegacySequenceHandle handle, const u32 slot, const i32 value
    ) override {
        user_data = value;
        events.push_back({BackendCall::set_user_data, handle, value, slot});
    }

    i32 sequence_user_data(
        const LegacySequenceHandle handle, const u32 slot
    ) override {
        events.push_back({BackendCall::get_user_data, handle, user_data, slot});
        return user_data;
    }

    void set_sequence_volume(
        const LegacySequenceHandle handle,
        const i32 volume,
        const i32 milliseconds
    ) override {
        events.push_back({
            BackendCall::set_volume,
            handle,
            volume,
            static_cast<u32>(milliseconds),
        });
    }

    void set_sequence_loop_count(
        const LegacySequenceHandle handle, const i32 loop_count
    ) override {
        events.push_back({BackendCall::set_loop_count, handle, loop_count});
    }

    void start_sequence(const LegacySequenceHandle handle) override {
        events.push_back({BackendCall::start, handle});
    }

    u32 sequence_status(const LegacySequenceHandle handle) override {
        events.push_back(
            {BackendCall::status, handle, static_cast<i32>(status)}
        );
        return status;
    }

    void end_sequence(const LegacySequenceHandle handle) override {
        events.push_back({BackendCall::end, handle});
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
    }

    std::vector<bool> open_results{true};
    std::vector<std::string> open_errors;
    std::size_t open_attempt{};
    u32 output_driver{0x11223344U};
    u32 sequence_handle{7U};
    i32 initialize_result{1};
    std::string initialize_error{"sequence initialize failure"};
    std::string error_text;
    i32 user_data{};
    u32 status{4U};
    std::vector<u8> initialized_bytes;
    std::vector<BackendEvent> events;
};

[[nodiscard]] std::filesystem::path
artifact_path(const std::string_view filename) {
    const std::filesystem::path root{OPENSWD3_TEST_ARTIFACT_ROOT};
    std::filesystem::create_directories(root);
    return root / filename;
}

void write_file(
    const std::filesystem::path& path, const std::span<const u8> bytes
) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    stream.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size())
    );
}

void test_output_open_paths(openswd3::test::Context& test) {
    RecordingBackend direct_backend;
    LegacySequenceManager direct_manager(direct_backend);
    test.expect_equal(
        direct_manager.initialize_output(0xDEADBEEFU),
        LegacySequenceManagerInitializeStatus::ready,
        "default MIDI output initializes"
    );
    test.expect_true(direct_manager.initialized(), "initialized flag is one");
    test.expect_true(
        direct_manager.sequence_enabled(), "sequence enabled flag is one"
    );
    test.expect_equal(
        direct_manager.midi_driver(),
        0x11223344U,
        "backend output driver is stored"
    );
    test.expect_equal(
        direct_manager.free_sequence_count(),
        1U,
        "exactly one sequence node is allocated"
    );
    test.expect_equal(
        direct_backend.events,
        std::vector<BackendEvent>{
            {BackendCall::open_output, 0x11223344U, -1},
            {BackendCall::allocate, 0x11223344U},
        },
        "default output is tried before handle allocation"
    );

    RecordingBackend fallback_backend;
    fallback_backend.open_results = {false, true};
    fallback_backend.open_errors = {"default failed", ""};
    LegacySequenceManager fallback_manager(fallback_backend);
    test.expect_equal(
        fallback_manager.initialize_output(5U),
        LegacySequenceManagerInitializeStatus::ready,
        "device zero fallback initializes"
    );
    test.expect_equal(
        fallback_manager.last_error(),
        std::string_view{"default failed"},
        "successful fallback retains the first error text"
    );
    test.expect_equal(
        fallback_backend.events,
        std::vector<BackendEvent>{
            {BackendCall::open_output, 0U, -1},
            {BackendCall::open_output, 0x11223344U, 0},
            {BackendCall::allocate, 0x11223344U},
        },
        "fallback uses device zero"
    );

    RecordingBackend failed_backend;
    failed_backend.open_results = {false, false};
    failed_backend.open_errors = {"first", "second"};
    LegacySequenceManager failed_manager(failed_backend);
    test.expect_equal(
        failed_manager.initialize_output(9U),
        LegacySequenceManagerInitializeStatus::midi_output_open_failed,
        "two output failures reject initialization"
    );
    test.expect_equal(
        failed_manager.last_error(),
        std::string_view{"first //second"},
        "two Miles errors retain the original separator"
    );
    test.expect_false(failed_manager.initialized(), "failed output stays off");
}

void test_play_and_absent_query(openswd3::test::Context& test) {
    const std::array<u8, 5U> bytes{1U, 2U, 3U, 4U, 5U};
    const std::filesystem::path path = artifact_path("sequence-test.xmi");
    write_file(path, bytes);

    RecordingBackend backend;
    LegacySequenceManager manager(backend);
    static_cast<void>(manager.initialize_output(0U));
    backend.clear_events();

    test.expect_equal(
        manager.play(path.string(), 44, 200, 3),
        44,
        "sequence play returns user-data ID"
    );
    test.expect_equal(
        backend.initialized_bytes,
        std::vector<u8>(bytes.begin(), bytes.end()),
        "entire file is retained for the sequence lifetime"
    );
    test.expect_equal(
        backend.events,
        std::vector<BackendEvent>{
            {BackendCall::initialize, 7U, 1, 0U},
            {BackendCall::set_user_data, 7U, 44, 0U},
            {BackendCall::set_volume, 7U, 127, 0U},
            {BackendCall::set_loop_count, 7U, 3},
            {BackendCall::start, 7U},
        },
        "play preserves init, ID, volume, loop and start order"
    );
    test.expect_equal(manager.active_sequence_count(), 1U, "one active node");
    test.expect_equal(manager.free_sequence_count(), 0U, "pool is exhausted");
    test.expect_equal(
        manager.play(path.string(), 55, 30, 1),
        0,
        "single-node pool rejects a second sequence"
    );

    backend.clear_events();
    test.expect_false(manager.sequence_absent(44), "active ID is present");
    test.expect_equal(
        backend.call_count(BackendCall::get_user_data),
        2U,
        "ID lookup preserves the duplicate Miles user-data query"
    );
    backend.clear_events();
    test.expect_false(
        manager.sequence_absent(0), "zero queries the active head"
    );
    test.expect_equal(
        backend.call_count(BackendCall::get_user_data),
        0U,
        "zero-ID head query does not call the backend"
    );
}

void test_initialize_results_and_service(openswd3::test::Context& test) {
    const std::array<u8, 2U> bytes{9U, 8U};
    const std::filesystem::path path = artifact_path("sequence-status.xmi");
    write_file(path, bytes);

    RecordingBackend rejected_backend;
    rejected_backend.initialize_result = 0;
    LegacySequenceManager rejected_manager(rejected_backend);
    static_cast<void>(rejected_manager.initialize_output(0U));
    rejected_backend.clear_events();
    test.expect_equal(
        rejected_manager.play(path.string(), 70, 50, 1),
        0,
        "zero init result recycles the node"
    );
    test.expect_equal(
        rejected_manager.last_error(),
        std::string_view{"sequence initialize failure"},
        "zero init result copies the backend error"
    );
    test.expect_equal(rejected_manager.free_sequence_count(), 1U, "recycled");

    RecordingBackend minus_one_backend;
    minus_one_backend.initialize_result = -1;
    LegacySequenceManager minus_one_manager(minus_one_backend);
    static_cast<void>(minus_one_manager.initialize_output(0U));
    minus_one_backend.clear_events();
    test.expect_equal(
        minus_one_manager.play(path.string(), 71, 50, 1),
        71,
        "minus-one init result records an error but still starts"
    );
    test.expect_equal(
        minus_one_manager.last_error(),
        std::string_view{"sequence initialize failure"},
        "minus-one init error is retained"
    );

    minus_one_backend.status = 4U;
    minus_one_backend.clear_events();
    test.expect_false(
        minus_one_manager.service(), "service always returns zero"
    );
    test.expect_equal(
        minus_one_manager.active_sequence_count(),
        1U,
        "status four retains the sequence"
    );
    test.expect_equal(
        minus_one_backend.events,
        std::vector<BackendEvent>{{BackendCall::status, 7U, 4}},
        "retained status only polls Miles"
    );

    minus_one_backend.status = 2U;
    minus_one_backend.clear_events();
    test.expect_false(minus_one_manager.service(), "completed service is zero");
    test.expect_equal(
        minus_one_backend.events,
        std::vector<BackendEvent>{
            {BackendCall::status, 7U, 2},
            {BackendCall::end, 7U},
        },
        "status two ends without releasing the reusable handle"
    );
    test.expect_equal(minus_one_manager.active_sequence_count(), 0U, "ended");
    test.expect_equal(minus_one_manager.free_sequence_count(), 1U, "recycled");
}

void test_shutdown_order(openswd3::test::Context& test) {
    const std::array<u8, 1U> bytes{0x7FU};
    const std::filesystem::path path = artifact_path("sequence-shutdown.xmi");
    write_file(path, bytes);

    RecordingBackend free_backend;
    LegacySequenceManager free_manager(free_backend);
    static_cast<void>(free_manager.initialize_output(0U));
    free_backend.clear_events();
    test.expect_true(free_manager.shutdown(), "free manager shutdown succeeds");
    test.expect_equal(
        free_backend.events,
        std::vector<BackendEvent>{
            {BackendCall::release, 7U},
            {BackendCall::close_output, 0x11223344U},
        },
        "free handle is released before output close"
    );

    RecordingBackend active_backend;
    LegacySequenceManager active_manager(active_backend);
    static_cast<void>(active_manager.initialize_output(0U));
    static_cast<void>(active_manager.play(path.string(), 1, 1, 1));
    active_backend.clear_events();
    test.expect_true(active_manager.shutdown(), "active shutdown succeeds");
    test.expect_equal(
        active_backend.events,
        std::vector<BackendEvent>{
            {BackendCall::end, 7U},
            {BackendCall::release, 7U},
            {BackendCall::close_output, 0x11223344U},
        },
        "active handle ends and releases before output close"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_output_open_paths(test);
    test_play_and_absent_query(test);
    test_initialize_results_and_service(test);
    test_shutdown_order(test);
    return test.exit_code();
}
