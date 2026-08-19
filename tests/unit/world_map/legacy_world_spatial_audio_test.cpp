#include "test.hpp"

#include "openswd3/world_map/legacy_world_spatial_audio.hpp"

#include <array>
#include <bit>
#include <functional>
#include <limits>
#include <vector>

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
using openswd3::world_map::kLegacyWorldSpatialAudioPlayingBit;
using openswd3::world_map::kLegacyWorldSpatialAudioRoleBit;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldSpatialAudioPorts;
using openswd3::world_map::LegacyWorldSpatialAudioState;
using openswd3::world_map::LegacyWorldSpatialAudioStatus;
using openswd3::world_map::update_legacy_world_spatial_audio;

struct PlayCall {
    u16 sound_id{};
    i32 volume{};
    i32 pan{};
    i32 loop_count{};
};

class RecordingPorts final : public LegacyWorldSpatialAudioPorts {
public:
    void play_sample(
        const u16 sound_id,
        const i32 volume,
        const i32 pan,
        const i32 loop_count
    ) noexcept override {
        plays.push_back({sound_id, volume, pan, loop_count});
        if (after_play) {
            after_play();
        }
    }

    void stop_sample(const u16 sound_id) noexcept override {
        stops.push_back(sound_id);
    }

    void
    set_sample_volume(const u16 sound_id, const i32 volume) noexcept override {
        volumes.push_back({sound_id, volume});
        if (after_volume) {
            after_volume();
        }
    }

    void set_sample_pan(const u16 sound_id, const i32 pan) noexcept override {
        pans.push_back({sound_id, pan});
    }

    std::function<void()> after_play;
    std::function<void()> after_volume;
    std::vector<PlayCall> plays;
    std::vector<u16> stops;
    std::vector<std::pair<u16, i32>> volumes;
    std::vector<std::pair<u16, i32>> pans;
};

struct Fixture {
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    std::array<i16, 3U> distances{};
    std::array<i16, 3U> vertical_offsets{};
    RecordingPorts ports;

    Fixture() {
        roles[0].world_x = 0U;
        roles[0].world_y = 0U;
        roles[1].world_x = 32U;
        roles[1].world_y = 24U;
        roles[1].flags = kLegacyWorldSpatialAudioRoleBit;
        roles[1].guid = 7U;
        roles[1].field_2c = 42U;
    }

    [[nodiscard]] LegacyWorldSpatialAudioState state() {
        return {
            .controlled_role_index = 0U,
            .mix_level = 11,
            .distance_by_role = distances,
            .vertical_offset_by_role = vertical_offsets,
        };
    }
};

void test_periodic_countdown_and_start(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].field_30 = 0x00030002U;
    auto state = fixture.state();

    const auto waiting = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        waiting.countdown_advanced && !waiting.sample_started &&
            fixture.roles[1].field_30 == 0x00030001U,
        "finite scheduler decrements its low word before starting"
    );
    test.expect_true(
        waiting.distance == 40 && waiting.volume == 118 && waiting.pan == 4 &&
            waiting.parameters_updated,
        "waiting samples still receive distance volume and pan updates"
    );

    const auto started = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        started.sample_started && fixture.roles[1].field_30 == 0x00030003U &&
            fixture.ports.plays.size() == 1U,
        "low word one reloads the finite scheduler and starts a sample"
    );
    test.expect_true(
        fixture.ports.plays[0].sound_id == 42U &&
            fixture.ports.plays[0].volume == 0 &&
            fixture.ports.plays[0].pan == 0 &&
            fixture.ports.plays[0].loop_count == 1,
        "0x00413CA0 submits the fixed zero/zero/one play arguments"
    );
    test.expect_true(
        fixture.distances[1] == 10 && fixture.vertical_offsets[1] == 3,
        "start stores scaled distance and vertical offset by GUID role"
    );
}

void test_indefinite_loop_and_stop(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].field_30 = 0xFFFF0000U;
    auto state = fixture.state();

    const auto started = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        started.sample_started && fixture.roles[1].field_30 == 0xFFFFFFFFU &&
            (fixture.roles[1].flags & kLegacyWorldSpatialAudioPlayingBit) != 0U,
        "FFFF scheduler marks an indefinitely active sample"
    );

    fixture.roles[1].world_x = 513U;
    const auto stopped = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        stopped.distance == 513 && stopped.sample_stopped &&
            fixture.ports.stops == std::vector<u16>{42U} &&
            (fixture.roles[1].flags & kLegacyWorldSpatialAudioPlayingBit) == 0U,
        "distance greater than 512 stops and clears an active sample"
    );
}

void test_distance_boundary_and_wrapping(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].world_x = 0U;
    fixture.roles[1].world_y = 512U;
    fixture.roles[1].field_30 = 0x00010001U;
    auto state = fixture.state();

    const auto boundary = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        boundary.distance == 512 && boundary.sample_started &&
            boundary.volume == 0 && boundary.pan == 0,
        "distance exactly 512 remains inside the audio update path"
    );
    test.expect_equal(
        fixture.vertical_offsets[1],
        i16{-64},
        "vertical storage keeps the original low-word shift wrap"
    );

    fixture.roles[1].world_x = 0x40000000U;
    fixture.roles[1].world_y = 0x40000000U;
    fixture.roles[1].field_30 = 0x00010002U;
    const auto overflow = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_equal(
        overflow.distance,
        i32{0},
        "negative wrapped square sum follows x87 indefinite low dword zero"
    );
}

void test_scheduler_wrap_and_active_skip(openswd3::test::Context& test) {
    Fixture wrapping;
    wrapping.roles[1].field_30 = 0x00030000U;
    auto wrapping_state = wrapping.state();
    const auto wrapped = update_legacy_world_spatial_audio(
        wrapping.roles[1], wrapping.roles, wrapping_state, wrapping.ports
    );
    test.expect_true(
        wrapped.countdown_advanced && !wrapped.sample_started &&
            wrapping.roles[1].field_30 == 0x0002FFFFU,
        "zero scheduler low word decrements across the full packed dword"
    );

    Fixture active;
    active.roles[1].field_30 = 0x00010001U;
    active.roles[1].flags |= kLegacyWorldSpatialAudioPlayingBit;
    auto active_state = active.state();
    const auto skipped = update_legacy_world_spatial_audio(
        active.roles[1], active.roles, active_state, active.ports
    );
    test.expect_true(
        !skipped.countdown_advanced && !skipped.sample_started &&
            active.roles[1].field_30 == 0x00010001U &&
            skipped.parameters_updated,
        "an already-active sample skips scheduler reload but updates params"
    );
}

void test_post_audio_reload_order(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].field_30 = 0x00010001U;
    fixture.roles[2].world_x = 100U;
    fixture.roles[2].world_y = 200U;
    auto state = fixture.state();
    fixture.ports.after_play = [&]() {
        fixture.roles[1].world_y = 240U;
        fixture.roles[1].field_2c = 43U;
        state.controlled_role_index = 2U;
        state.mix_level = 22;
    };
    fixture.ports.after_volume = [&]() {
        fixture.roles[1].world_x = 320U;
        fixture.roles[1].field_2c = 44U;
        state.controlled_role_index = 0U;
    };

    const auto result = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        result.distance == 40 && fixture.distances[1] == 10 &&
            fixture.vertical_offsets[1] == 5,
        "distance stays frozen while post-play vertical inputs are reloaded"
    );
    test.expect_true(
        fixture.ports.plays.size() == 1U &&
            fixture.ports.plays[0].sound_id == 42U &&
            fixture.ports.volumes ==
                std::vector<std::pair<u16, i32>>{{43U, 236}} &&
            fixture.ports.pans == std::vector<std::pair<u16, i32>>{{44U, 40}},
        "play, volume and pan reload sound and position fields at LST slots"
    );
    test.expect_true(
        result.volume == 236 && result.pan == 40 && result.parameters_updated,
        "post-play mix and post-volume horizontal mutations remain visible"
    );
}

void test_parameter_wrapping_and_role_gate(openswd3::test::Context& test) {
    Fixture wrapping;
    wrapping.roles[1].world_x = 0x02000000U;
    wrapping.roles[1].world_y = 0U;
    wrapping.roles[1].field_30 = 0x00010002U;
    auto wrapping_state = wrapping.state();
    wrapping_state.mix_level = std::numeric_limits<i32>::max();
    const auto parameters = update_legacy_world_spatial_audio(
        wrapping.roles[1], wrapping.roles, wrapping_state, wrapping.ports
    );
    test.expect_true(
        parameters.distance == 0 && parameters.volume == -11 &&
            parameters.pan == -4194304,
        "volume multiply and pan shift preserve signed 32-bit wrapping"
    );

    Fixture gated;
    gated.roles[1].flags = kLegacyWorldSpatialAudioPlayingBit;
    auto gated_state = gated.state();
    const auto stopped = update_legacy_world_spatial_audio(
        gated.roles[1], gated.roles, gated_state, gated.ports
    );
    test.expect_true(
        stopped.sample_stopped && !stopped.parameters_updated &&
            gated.ports.stops == std::vector<u16>{42U} &&
            (gated.roles[1].flags & kLegacyWorldSpatialAudioPlayingBit) == 0U,
        "a cleared role-audio bit stops and clears an active loop"
    );

    Fixture finite;
    finite.roles[1].field_30 = 0x00010001U;
    auto finite_state = finite.state();
    const auto started = update_legacy_world_spatial_audio(
        finite.roles[1], finite.roles, finite_state, finite.ports
    );
    finite.roles[1].world_x = 513U;
    const auto outside = update_legacy_world_spatial_audio(
        finite.roles[1], finite.roles, finite_state, finite.ports
    );
    test.expect_true(
        started.sample_started && !outside.sample_stopped &&
            finite.ports.stops.empty(),
        "finite periodic samples never acquire the explicit-stop playing bit"
    );
}

void test_checked_invalid_state(openswd3::test::Context& test) {
    Fixture fixture;
    auto state = fixture.state();
    state.controlled_role_index = 3U;
    const auto listener = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_equal(
        listener.status,
        LegacyWorldSpatialAudioStatus::invalid_controlled_role,
        "invalid controlled-role pointer is isolated before access"
    );

    state = fixture.state();
    fixture.roles[1].flags |= kLegacyWorldGuidLookupSkipBit;
    fixture.roles[1].field_30 = 0x00010001U;
    const auto resolved = update_legacy_world_spatial_audio(
        fixture.roles[1], fixture.roles, state, fixture.ports
    );
    test.expect_true(
        resolved.status ==
                LegacyWorldSpatialAudioStatus::invalid_resolved_role &&
            resolved.sample_started,
        "missing GUID target is isolated at the original post-play array write"
    );

    Fixture vertical_listener;
    vertical_listener.roles[1].field_30 = 0x00010001U;
    auto vertical_state = vertical_listener.state();
    vertical_listener.ports.after_play = [&]() {
        vertical_state.controlled_role_index = 3U;
    };
    const auto vertical = update_legacy_world_spatial_audio(
        vertical_listener.roles[1],
        vertical_listener.roles,
        vertical_state,
        vertical_listener.ports
    );
    test.expect_true(
        vertical.status ==
                LegacyWorldSpatialAudioStatus::invalid_controlled_role &&
            vertical.sample_started && vertical_listener.distances[1] == 10 &&
            vertical_listener.vertical_offsets[1] == 0 &&
            vertical_listener.ports.volumes.empty(),
        "post-play listener validation preserves the preceding distance write"
    );

    Fixture pan_listener;
    pan_listener.roles[1].field_30 = 0x00010002U;
    auto pan_state = pan_listener.state();
    pan_listener.ports.after_volume = [&]() {
        pan_state.controlled_role_index = 3U;
    };
    const auto pan = update_legacy_world_spatial_audio(
        pan_listener.roles[1], pan_listener.roles, pan_state, pan_listener.ports
    );
    test.expect_true(
        pan.status == LegacyWorldSpatialAudioStatus::invalid_controlled_role &&
            pan_listener.ports.volumes.size() == 1U &&
            pan_listener.ports.pans.empty() && !pan.parameters_updated,
        "post-volume listener validation stops at the original pan dereference"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_periodic_countdown_and_start(test);
    test_indefinite_loop_and_stop(test);
    test_distance_boundary_and_wrapping(test);
    test_scheduler_wrap_and_active_skip(test);
    test_post_audio_reload_order(test);
    test_parameter_wrapping_and_role_gate(test);
    test_checked_invalid_state(test);
    return test.exit_code();
}
