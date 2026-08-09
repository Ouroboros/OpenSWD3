#include "test.hpp"

#include "openswd3/audio_video/legacy_sample_manager.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::audio_video::LegacySampleBackend;
using openswd3::audio_video::LegacySampleHandle;
using openswd3::audio_video::LegacySampleManager;
using openswd3::audio_video::LegacySampleManagerInitializeStatus;
using openswd3::audio_video::LegacySamplePlayRequest;
using openswd3::audio_video::LegacySndArchive;
using openswd3::audio_video::LegacySndOpenStatus;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u32;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kDiskRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kPayloadOffset =
    kIndexOffset + kSlotCount * kDiskRecordSize;
constexpr std::size_t kPayloadSize = 48U;

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

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-sample-manager-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(
        const char* name,
        const std::span<const u8> bytes
    ) const {
        std::ofstream output{
            root_ / name,
            std::ios::binary | std::ios::trunc
        };
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

[[nodiscard]] std::vector<u8> synthetic_archive() {
    std::vector<u8> archive(kPayloadOffset + 3U * kPayloadSize, 0U);
    for (u32 sound_id = 1U; sound_id <= 3U; ++sound_id) {
        const std::size_t record =
            kIndexOffset + (sound_id - 1U) * kDiskRecordSize;
        const u32 payload_offset = static_cast<u32>(
            kPayloadOffset + (sound_id - 1U) * kPayloadSize
        );
        write_u32(archive, record + 0x14U, static_cast<u32>(kPayloadSize));
        write_u32(archive, record + 0x18U, payload_offset);
        write_u32(archive, record + 0x20U, sound_id == 2U ? 1U : 0U);

        for (std::size_t index = 0U; index < kPayloadSize; ++index) {
            archive[payload_offset + index] = static_cast<u8>(
                sound_id * 0x20U + static_cast<u32>(index)
            );
        }
        if (sound_id == 2U) {
            archive[payload_offset] = 'I';
            archive[payload_offset + 1U] = 'D';
            archive[payload_offset + 2U] = '3';
        }
    }
    return archive;
}

enum class BackendCall {
    allocate,
    initialize,
    release,
    set_file,
    set_named_file,
    set_user_data,
    get_user_data,
    set_volume,
    set_pan,
    set_loop_count,
    start,
    end,
    status,
    close_output,
};

struct BackendEvent {
    BackendCall call{};
    u32 handle{};
    i32 value{};
    u32 auxiliary{};
    std::size_t byte_count{};

    bool operator==(const BackendEvent&) const = default;
};

class RecordingBackend final : public LegacySampleBackend {
public:
    u32 driver_token() const override {
        return 0x12345678U;
    }

    LegacySampleHandle allocate_sample_handle() override {
        if (allocated_count >= allocation_limit) {
            events.push_back({BackendCall::allocate, 0U});
            return 0U;
        }
        const u32 handle = ++allocated_count;
        events.push_back({BackendCall::allocate, handle});
        return handle;
    }

    void initialize_sample(const LegacySampleHandle handle) override {
        events.push_back({BackendCall::initialize, handle});
    }

    void release_sample_handle(const LegacySampleHandle handle) override {
        events.push_back({BackendCall::release, handle});
    }

    bool set_sample_file(
        const LegacySampleHandle handle,
        const std::span<const u8> bytes
    ) override {
        events.push_back(
            {BackendCall::set_file, handle, 0, 0U, bytes.size()}
        );
        return set_file_result;
    }

    bool set_named_sample_file(
        const LegacySampleHandle handle,
        const std::string_view extension,
        const std::span<const u8> bytes,
        const u32 auxiliary
    ) override {
        named_extension = extension;
        events.push_back(
            {BackendCall::set_named_file, handle, 0, auxiliary, bytes.size()}
        );
        return set_named_file_result;
    }

    void set_sample_user_data(
        const LegacySampleHandle handle,
        const u32 slot,
        const u32 value
    ) override {
        events.push_back(
            {BackendCall::set_user_data, handle, static_cast<i32>(value), slot}
        );
        user_data[handle] = value;
    }

    u32 sample_user_data(
        const LegacySampleHandle handle,
        const u32 slot
    ) override {
        events.push_back({BackendCall::get_user_data, handle, 0, slot});
        return user_data[handle];
    }

    void set_sample_volume(
        const LegacySampleHandle handle,
        const i32 volume
    ) override {
        events.push_back({BackendCall::set_volume, handle, volume});
    }

    void set_sample_pan(
        const LegacySampleHandle handle,
        const i32 pan
    ) override {
        events.push_back({BackendCall::set_pan, handle, pan});
    }

    void set_sample_loop_count(
        const LegacySampleHandle handle,
        const i32 loop_count
    ) override {
        events.push_back({BackendCall::set_loop_count, handle, loop_count});
    }

    void start_sample(const LegacySampleHandle handle) override {
        events.push_back({BackendCall::start, handle});
    }

    void end_sample(const LegacySampleHandle handle) override {
        events.push_back({BackendCall::end, handle});
    }

    u32 sample_status(const LegacySampleHandle handle) override {
        events.push_back({BackendCall::status, handle});
        return statuses[handle];
    }

    void close_output() override {
        events.push_back({BackendCall::close_output, 0U});
    }

    void clear_events() {
        events.clear();
    }

    u32 allocation_limit{64U};
    u32 allocated_count{};
    bool set_file_result{true};
    bool set_named_file_result{true};
    std::array<u32, 65> user_data{};
    std::array<u32, 65> statuses{};
    std::string_view named_extension;
    std::vector<BackendEvent> events;
};

struct Fixture {
    Fixture() : manager(backend, archive) {
        tree.write("all.snd", synthetic_archive());
        open_status = archive.open(tree.root() / "all.snd");
    }

    TestTree tree;
    LegacySndArchive archive;
    RecordingBackend backend;
    LegacySampleManager manager;
    LegacySndOpenStatus open_status{};
};

[[nodiscard]] LegacySamplePlayRequest request(
    const u32 sound_id,
    const i32 volume = 100,
    const i32 pan = 0,
    const i32 loop_count = 1,
    const u32 auxiliary = 0U
) {
    return LegacySamplePlayRequest{
        .existing_buffer = std::nullopt,
        .sound_id = sound_id,
        .volume = volume,
        .pan = pan,
        .loop_count = loop_count,
        .named_file_auxiliary = auxiliary,
    };
}

void expect_ready_fixture(
    openswd3::test::Context& test,
    const Fixture& fixture
) {
    test.expect_equal(
        fixture.open_status,
        LegacySndOpenStatus::ready,
        "synthetic SND fixture opens"
    );
}

void test_pool_initialization(openswd3::test::Context& test) {
    RecordingBackend closed_backend;
    LegacySndArchive closed_archive;
    LegacySampleManager closed_manager(closed_backend, closed_archive);
    test.expect_equal(
        closed_manager.initialize_pool(4),
        LegacySampleManagerInitializeStatus::archive_not_open,
        "pool requires the SND archive"
    );
    test.expect_true(
        closed_backend.events.empty(),
        "closed archive returns before backend allocation"
    );

    Fixture fixture;
    expect_ready_fixture(test, fixture);
    test.expect_equal(
        fixture.manager.initialize_pool(20),
        LegacySampleManagerInitializeStatus::ready,
        "pool initialization succeeds"
    );
    test.expect_true(fixture.manager.initialized(), "initialized flag is one");
    test.expect_true(
        fixture.manager.sample_enabled(),
        "sample-enabled flag starts one"
    );
    test.expect_equal(
        fixture.manager.configured_handle_count(),
        16,
        "handle count is capped at sixteen"
    );
    test.expect_equal(
        fixture.manager.free_sample_count(),
        std::size_t{16U},
        "all allocated handles start on the free list"
    );
    test.expect_equal(
        fixture.manager.driver_token(),
        0x12345678U,
        "driver getter forwards the backend token"
    );
    test.expect_equal(
        fixture.backend.events.size(),
        std::size_t{32U},
        "each handle is allocated then initialized"
    );
    test.expect_equal(
        fixture.backend.events[0],
        BackendEvent{BackendCall::allocate, 1U},
        "first pool event allocates handle one"
    );
    test.expect_equal(
        fixture.backend.events[1],
        BackendEvent{BackendCall::initialize, 1U},
        "allocated handle is initialized immediately"
    );
    test.expect_equal(
        fixture.backend.events.back(),
        BackendEvent{BackendCall::initialize, 16U},
        "sixteenth handle completes the capped pool"
    );

    fixture.backend.clear_events();
    test.expect_equal(
        fixture.manager.initialize_pool(3),
        LegacySampleManagerInitializeStatus::ready,
        "second initialization returns one"
    );
    test.expect_true(
        fixture.backend.events.empty(),
        "second initialization does not rebuild the pool"
    );
}

void test_partial_and_negative_pool(openswd3::test::Context& test) {
    {
        Fixture fixture;
        fixture.backend.allocation_limit = 2U;
        test.expect_equal(
            fixture.manager.initialize_pool(5),
            LegacySampleManagerInitializeStatus::ready,
            "handle allocation failure still returns ready"
        );
        test.expect_equal(
            fixture.manager.configured_handle_count(),
            5,
            "configured count is not reduced after partial allocation"
        );
        test.expect_equal(
            fixture.manager.free_sample_count(),
            std::size_t{2U},
            "previously allocated handles survive first failure"
        );
        test.expect_equal(
            fixture.backend.events.back(),
            BackendEvent{BackendCall::allocate, 0U},
            "allocation loop stops on the first null handle"
        );
    }

    {
        Fixture fixture;
        test.expect_equal(
            fixture.manager.initialize_pool(-3),
            LegacySampleManagerInitializeStatus::ready,
            "negative configured count skips the pool loop"
        );
        test.expect_equal(
            fixture.manager.configured_handle_count(),
            -3,
            "negative configured count remains observable"
        );
        test.expect_true(
            fixture.backend.events.empty(),
            "negative count allocates no handles"
        );
    }
}

void test_play_riff_and_named(openswd3::test::Context& test) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(3));
    fixture.backend.clear_events();

    test.expect_equal(
        fixture.manager.play(request(1U, 200, 0, 3, 9U)),
        0,
        "play command keeps the original zero return"
    );
    const std::vector<BackendEvent> riff_events{
        {BackendCall::initialize, 3U},
        {BackendCall::set_file, 3U, 0, 0U, 72U},
        {BackendCall::set_user_data, 3U, 1, 0U},
        {BackendCall::set_volume, 3U, 127},
        {BackendCall::set_pan, 3U, 63},
        {BackendCall::set_loop_count, 3U, 3},
        {BackendCall::start, 3U},
    };
    test.expect_equal(
        fixture.backend.events,
        riff_events,
        "RIFF playback backend event order"
    );
    test.expect_equal(
        fixture.manager.active_sample_count(),
        std::size_t{1U},
        "played handle moves to active head"
    );
    test.expect_equal(
        fixture.archive.entries()[0].reference_count,
        1U,
        "successful play increments the SND slot reference"
    );
    test.expect_true(
        fixture.manager.buffer_is_live(
            fixture.archive.entries()[0].buffer_token
        ),
        "successful play records a live buffer token"
    );

    fixture.backend.clear_events();
    std::vector<u8> named_bytes(kPayloadSize, 0x55U);
    named_bytes[0] = 'I';
    named_bytes[1] = 'D';
    named_bytes[2] = '3';
    LegacySamplePlayRequest named = request(2U, -1, 65, 0, 77U);
    named.existing_buffer = std::move(named_bytes);
    static_cast<void>(fixture.manager.play(std::move(named)));
    const std::vector<BackendEvent> named_events{
        {BackendCall::initialize, 2U},
        {BackendCall::set_named_file, 2U, 0, 77U, kPayloadSize},
        {BackendCall::set_user_data, 2U, 2, 0U},
        {BackendCall::set_volume, 2U, 0},
        {BackendCall::set_pan, 2U, 127},
        {BackendCall::set_loop_count, 2U, 0},
        {BackendCall::start, 2U},
    };
    test.expect_equal(
        fixture.backend.events,
        named_events,
        "named playback backend event order"
    );
    test.expect_equal(
        fixture.backend.named_extension,
        std::string_view{".mp3"},
        "non-RIFF extension is fixed to mp3"
    );
}

void test_setup_failure_and_empty_pool_leak(
    openswd3::test::Context& test
) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(1));
    fixture.backend.clear_events();
    fixture.backend.set_file_result = false;

    static_cast<void>(fixture.manager.play(request(1U)));
    const std::vector<BackendEvent> failure_events{
        {BackendCall::initialize, 1U},
        {BackendCall::set_file, 1U, 0, 0U, 72U},
    };
    test.expect_equal(
        fixture.backend.events,
        failure_events,
        "file setup failure stops before user data and start"
    );
    test.expect_equal(
        fixture.manager.free_sample_count(),
        std::size_t{1U},
        "failed setup returns the handle to free head"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{0U},
        "failed setup frees the attempted buffer"
    );

    fixture.backend.set_file_result = true;
    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.play(request(1U)));
    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.play(request(2U)));
    test.expect_true(
        fixture.backend.events.empty(),
        "empty free list returns before reinitializing a backend handle"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{2U},
        "freshly loaded buffer leaks when no handle is free"
    );
    test.expect_equal(
        fixture.archive.entries()[1].reference_count,
        0U,
        "empty-pool leak does not increment the target SND reference"
    );
}

void test_overlapping_reference_bug(openswd3::test::Context& test) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(2));
    static_cast<void>(fixture.manager.play(request(1U)));
    const u32 first_token = fixture.archive.entries()[0].buffer_token;
    static_cast<void>(fixture.manager.play(request(1U)));
    const u32 second_token = fixture.archive.entries()[0].buffer_token;

    test.expect_equal(
        fixture.archive.entries()[0].reference_count,
        2U,
        "overlapping sound increments the single slot twice"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{2U},
        "both overlapping allocations remain live"
    );
    test.expect_true(
        first_token != second_token,
        "last buffer token overwrites rather than shares the first token"
    );

    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.stop(1U));
    test.expect_equal(
        fixture.archive.entries()[0].reference_count,
        1U,
        "first stop decrements without freeing"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{2U},
        "nonzero reference preserves both allocations"
    );
    test.expect_equal(
        fixture.backend.events[0],
        BackendEvent{BackendCall::get_user_data, 1U},
        "stop finds the most recently activated matching handle"
    );

    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.stop(1U));
    test.expect_equal(
        fixture.archive.entries()[0].reference_count,
        0U,
        "second stop reaches zero"
    );
    test.expect_false(
        fixture.manager.buffer_is_live(second_token),
        "zero reference frees only the last recorded buffer"
    );
    test.expect_true(
        fixture.manager.buffer_is_live(first_token),
        "first overlapping buffer retains the original leak"
    );
}

void test_invalid_id_boundary_and_reference_underflow(
    openswd3::test::Context& test
) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(1));
    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.play(request(0U)));
    static_cast<void>(fixture.manager.play(request(3001U)));
    test.expect_true(
        fixture.backend.events.empty(),
        "unsafe original sound IDs are isolated before backend side effects"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{0U},
        "invalid ID boundary retains no buffer"
    );

    static_cast<void>(fixture.manager.play(request(1U)));
    fixture.backend.user_data[1U] = 2U;
    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.stop(2U));
    test.expect_equal(
        fixture.archive.entries()[1].reference_count,
        std::numeric_limits<u32>::max(),
        "mismatched user data preserves unsigned reference underflow"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{1U},
        "underflow does not free the unrelated live buffer"
    );
}

void test_stop_all_and_parameter_updates(openswd3::test::Context& test) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(3));
    static_cast<void>(fixture.manager.play(request(1U)));
    static_cast<void>(fixture.manager.play(request(2U)));

    fixture.backend.clear_events();
    test.expect_equal(
        fixture.manager.set_volume(2U, 500),
        0,
        "volume update returns zero"
    );
    test.expect_equal(
        fixture.backend.events,
        std::vector<BackendEvent>{
            {BackendCall::get_user_data, 2U},
            {BackendCall::set_volume, 2U, 127},
        },
        "volume updates the first active matching sound"
    );

    fixture.backend.clear_events();
    test.expect_equal(
        fixture.manager.set_pan(3U, -62),
        1,
        "pan returns its converted value even without a matching sound"
    );
    test.expect_equal(
        fixture.backend.events,
        std::vector<BackendEvent>{
            {BackendCall::get_user_data, 2U},
            {BackendCall::get_user_data, 3U},
        },
        "missing pan update only scans the active list"
    );

    fixture.backend.clear_events();
    test.expect_true(fixture.manager.stop_all(), "stop-all returns one");
    const std::vector<BackendEvent> stop_events{
        {BackendCall::end, 2U},
        {BackendCall::get_user_data, 2U},
        {BackendCall::end, 3U},
        {BackendCall::get_user_data, 3U},
    };
    test.expect_equal(
        fixture.backend.events,
        stop_events,
        "stop-all walks active head order and releases references"
    );
    test.expect_equal(
        fixture.manager.active_sample_count(),
        std::size_t{0U},
        "stop-all clears active head"
    );
    test.expect_equal(
        fixture.manager.free_sample_count(),
        std::size_t{3U},
        "stop-all returns every node to free list"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{0U},
        "non-overlapping stop-all frees all buffers"
    );
}

void test_completed_sample_service(openswd3::test::Context& test) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(3));
    static_cast<void>(fixture.manager.play(request(1U)));
    static_cast<void>(fixture.manager.play(request(2U)));
    fixture.backend.statuses[2U] = 1U;
    fixture.backend.statuses[3U] = 3U;
    fixture.backend.clear_events();

    test.expect_true(
        fixture.manager.service_completed_samples(),
        "sample service returns one when enabled"
    );
    const std::vector<BackendEvent> expected{
        {BackendCall::status, 2U},
        {BackendCall::set_volume, 2U, 0},
        {BackendCall::end, 2U},
        {BackendCall::get_user_data, 2U},
        {BackendCall::status, 3U},
    };
    test.expect_equal(
        fixture.backend.events,
        expected,
        "status one reclaims while status three stays active"
    );
    test.expect_equal(
        fixture.manager.active_sample_count(),
        std::size_t{1U},
        "one nonterminal sample remains active"
    );

    fixture.backend.statuses[3U] = 2U;
    fixture.backend.clear_events();
    static_cast<void>(fixture.manager.service_completed_samples());
    test.expect_equal(
        fixture.manager.active_sample_count(),
        std::size_t{0U},
        "status two is also terminal"
    );
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{0U},
        "terminal services free non-overlapping buffers"
    );
}

void test_disabled_and_shutdown(openswd3::test::Context& test) {
    Fixture fixture;
    static_cast<void>(fixture.manager.initialize_pool(2));
    static_cast<void>(fixture.manager.play(request(1U)));
    fixture.manager.set_sample_enabled(false);
    fixture.backend.clear_events();
    test.expect_equal(fixture.manager.play(request(2U)), 0, "disabled play");
    test.expect_equal(fixture.manager.stop(1U), 0, "disabled stop");
    test.expect_false(fixture.manager.stop_all(), "disabled stop-all");
    test.expect_equal(
        fixture.manager.set_volume(1U, 50),
        0,
        "disabled volume"
    );
    test.expect_equal(fixture.manager.set_pan(1U, 50), 0, "disabled pan");
    test.expect_false(
        fixture.manager.service_completed_samples(),
        "disabled service"
    );
    test.expect_true(
        fixture.backend.events.empty(),
        "disabled commands have no backend side effects"
    );

    fixture.manager.set_sample_enabled(true);
    fixture.backend.clear_events();
    test.expect_true(fixture.manager.shutdown(), "shutdown returns one");
    const std::vector<BackendEvent> expected{
        {BackendCall::end, 2U},
        {BackendCall::release, 2U},
        {BackendCall::end, 1U},
        {BackendCall::release, 1U},
        {BackendCall::close_output, 0U},
    };
    test.expect_equal(
        fixture.backend.events,
        expected,
        "shutdown releases active list, free list, then output"
    );
    test.expect_false(fixture.manager.initialized(), "shutdown clears state");
    test.expect_false(fixture.archive.is_open(), "shutdown closes SND archive");
    test.expect_equal(
        fixture.manager.live_buffer_count(),
        std::size_t{0U},
        "shutdown releases owned buffer storage"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_pool_initialization(test);
    test_partial_and_negative_pool(test);
    test_play_riff_and_named(test);
    test_setup_failure_and_empty_pool_leak(test);
    test_overlapping_reference_bug(test);
    test_invalid_id_boundary_and_reference_underflow(test);
    test_stop_all_and_parameter_updates(test);
    test_completed_sample_service(test);
    test_disabled_and_shutdown(test);
    return test.exit_code();
}
