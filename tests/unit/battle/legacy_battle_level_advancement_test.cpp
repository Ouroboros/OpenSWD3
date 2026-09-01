#include "legacy_battle_level_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_level_advancement.hpp"

#include <array>
#include <bit>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleLevelAdvancementCall;
using openswd3::battle::LegacyBattleLevelAdvancementCallReply;
using openswd3::battle::LegacyBattleLevelAdvancementCallRequest;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldStoryPartyMemberResources;

class Port final : public openswd3::battle::LegacyBattleLevelAdvancementPort,
                   public openswd3::test::LegacyBattleLevelDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleLevelAdvancementCallReply
    invoke_level_advancement(
        const LegacyBattleLevelAdvancementCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        return found->second[index++];
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelAdvancementRegisters
    stop_level_sample(
        const u32 eax, const u32 ecx, const u32 edx, const u32 sound_id
    ) override {
        stop_calls.push_back({sound_id, eax, ecx, edx});
        return {.eax = 1U, .ecx = ecx, .edx = edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelAdvancementRegisters
    play_level_sample(
        const u32 eax,
        const u32 ecx,
        const u32 edx,
        const u32 sound_id,
        const i32 mix_level
    ) override {
        play_calls.push_back(
            {sound_id, std::bit_cast<u32>(mix_level), eax, ecx, edx}
        );
        return {.eax = 0xABCD1200U, .ecx = ecx, .edx = edx};
    }

    void reply(
        const LegacyBattleLevelAdvancementCall call,
        const LegacyBattleLevelAdvancementCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32 count(const LegacyBattleLevelAdvancementCall call) const {
        u32 result = 0U;
        for (const auto& request : calls) {
            if (request.call == call) {
                ++result;
            }
        }
        return result;
    }

    std::vector<LegacyBattleLevelAdvancementCallRequest> calls;
    std::map<
        LegacyBattleLevelAdvancementCall,
        std::vector<LegacyBattleLevelAdvancementCallReply>>
        replies;
    std::map<LegacyBattleLevelAdvancementCall, std::size_t> reply_indices;
    std::vector<std::array<u32, 4>> stop_calls;
    std::vector<std::array<u32, 5>> play_calls;
};

struct Fixture {
    Fixture() {
        input.sample_mix_level = -4;
        victory.party_profile_threshold = 10U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelAdvancementBindings
    bindings() {
        return {
            .state = state,
            .victory = victory,
            .startup = startup,
            .metrics = metrics,
            .input_dispatch = input,
            .target_selection = target,
            .party_member_resources = party_resources,
        };
    }

    openswd3::battle::LegacyBattleLevelAdvancementState state;
    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    std::array<LegacyWorldStoryPartyMemberResources, 4> party_resources{};
    Port port;
};

[[nodiscard]] openswd3::battle::LegacyBattleLevelAdvancementResult
run(Fixture& fixture,
    const openswd3::battle::LegacyBattleLevelAdvancementRequest& request = {}) {
    return openswd3::battle::advance_legacy_battle_actor_level(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] LegacyWorldStoryPartyMemberResources
make_profile(const u16 base, const u8 level, const u32 field_20) {
    LegacyWorldStoryPartyMemberResources profile{
        .field_00 = 100U,
        .current_first = static_cast<u16>(base + 1U),
        .current_second = static_cast<u16>(base + 2U),
        .current_third = static_cast<u16>(base + 3U),
        .limit_first = static_cast<u16>(base + 1U),
        .limit_second = static_cast<u16>(base + 2U),
        .limit_third = static_cast<u16>(base + 3U),
        .field_20 = field_20,
        .transient_value = 0x7788U,
        .field_28 = 0x99AABBCCU,
        .field_2c = level,
    };
    for (std::size_t index = 0U; index < profile.fields_10_to_1e.size();
         ++index) {
        profile.fields_10_to_1e[index] = static_cast<u16>(base + 10U + index);
    }
    for (std::size_t index = 0U; index < profile.tail_2d_to_37.size();
         ++index) {
        profile.tail_2d_to_37[index] = static_cast<u8>(0xA0U + index);
    }
    return profile;
}

[[nodiscard]] std::vector<u8> make_requirement_stream(const u32 value) {
    std::vector<u8> stream;
    openswd3::test::LegacyBattleLevelDatabaseFixture::append_word(stream, 0U);
    stream.resize(stream.size() + 0x16U, 0U);
    openswd3::test::LegacyBattleLevelDatabaseFixture::append_dword(
        stream, value
    );
    openswd3::test::LegacyBattleLevelDatabaseFixture::append_word(stream, 5U);
    return stream;
}

[[nodiscard]] std::vector<u8>
make_profile_stream(const LegacyWorldStoryPartyMemberResources& profile) {
    const auto* const bytes = reinterpret_cast<const u8*>(&profile);
    std::vector<u8> stream;
    openswd3::test::LegacyBattleLevelDatabaseFixture::append_word(stream, 0U);
    stream.insert(stream.end(), bytes + 0x0AU, bytes + 0x24U);
    openswd3::test::LegacyBattleLevelDatabaseFixture::append_word(stream, 5U);
    return stream;
}

}  // namespace

void test_battle_level_advancement(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleLevelAdvancementStatus;

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 0U;
        const auto result =
            run(fixture, {.entry_eax = 7U, .entry_ecx = 8U, .entry_edx = 9U});
        test.expect_true(
            result.status == LegacyBattleLevelAdvancementStatus::completed &&
                result.visited_actors == 0U && result.port_calls == 0U &&
                fixture.state.completion_gate == 1U &&
                result.return_eax == 0U && result.return_ecx == 8U &&
                result.return_edx == 9U,
            "level advancement completes immediately for a non-positive live party count"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.victory.group_a_skip_secondary[1U] = 1U;
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleLevelAdvancementStatus::completed &&
                result.visited_actors == 2U && result.port_calls == 0U &&
                fixture.state.completion_gate == 1U &&
                result.return_eax == 2U && result.return_ecx == 2U,
            "level advancement skips exact-one actor fields and re-reads the live count at each tail"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 0U;
        fixture.party_resources[0U].field_2c = 10U;
        fixture.target.transition_actor_index = 3U;
        const auto result = run(fixture, {.entry_edx = 0xABCD1234U});
        test.expect_true(
            result.status == LegacyBattleLevelAdvancementStatus::completed &&
                result.requirement_calls == 0U &&
                fixture.target.transition_actor_index == 0xFFU &&
                fixture.state.completion_gate == 1U &&
                result.return_edx == 0xABCD000AU,
            "level advancement rejects a level at the unsigned threshold after preserving the EDX high word"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.party_resources[0U].field_00 = 49U;
        fixture.party_resources[0U].field_2c = 2U;
        fixture.port.record_available = true;
        fixture.port.level_value = 50U;
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleLevelAdvancementStatus::completed &&
                result.requirement_calls == 1U &&
                result.profile_build_calls == 0U &&
                fixture.target.transition_actor_index == 0xFFU &&
                fixture.state.completion_gate == 1U &&
                result.return_eax == 1U && result.return_ecx == 1U,
            "level advancement rejects signed-insufficient experience and continues through the normal actor tail"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.startup.action_mode_source.actor_label_indices[1U] = 1U;
        auto original = make_profile(100U, 2U, 0x01020304U);
        fixture.party_resources[1U] = original;

        const auto baseline = make_profile(10U, 2U, 500U);
        const auto advanced = make_profile(12U, 3U, 700U);
        fixture.port.custom_streams.push_back(make_requirement_stream(50U));
        fixture.port.custom_streams.push_back(make_profile_stream(baseline));
        fixture.port.custom_streams.push_back(make_profile_stream(advanced));
        const auto result =
            run(fixture,
                {
                    .entry_edx = 0x12345678U,
                    .requirement_output_token = 0x71000000U,
                });
        const auto& profile = fixture.party_resources[1U];
        test.expect_true(
            result.status == LegacyBattleLevelAdvancementStatus::completed &&
                result.requirement_calls == 1U &&
                result.profile_build_calls == 2U &&
                result.stop_sample_calls == 1U &&
                result.play_sample_calls == 1U &&
                result.selected_actor_index == 1U &&
                fixture.target.transition_actor_index == 1U &&
                fixture.state.completion_gate == 1U,
            "level advancement commits the first eligible actor and exits after both profile builds and audio calls"
        );
        test.expect_true(
            profile.field_00 == original.field_00 && profile.field_2c == 3U &&
                profile.limit_first == original.limit_first + 2U &&
                profile.limit_second == original.limit_second + 2U &&
                profile.limit_third == original.limit_third + 2U &&
                profile.current_first == profile.limit_first &&
                profile.current_second == profile.limit_second &&
                profile.current_third == profile.limit_third &&
                profile.field_20 == 700U &&
                profile.transient_value == original.transient_value &&
                profile.field_28 == original.field_28 &&
                profile.tail_2d_to_37 == original.tail_2d_to_37,
            "level advancement preserves untouched profile fields while replacing level and applying wrapping deltas"
        );
        bool attribute_deltas_match = true;
        for (std::size_t index = 0U; index < profile.fields_10_to_1e.size();
             ++index) {
            attribute_deltas_match = attribute_deltas_match &&
                profile.fields_10_to_1e[index] ==
                    static_cast<u16>(original.fields_10_to_1e[index] + 2U);
        }
        test.expect_true(
            attribute_deltas_match &&
                fixture.state.profile_copy_scratch.field_00 ==
                    original.field_00 &&
                fixture.state.profile_copy_scratch.tail_2d_to_37 ==
                    original.tail_2d_to_37,
            "level advancement copies all 56 profile bytes and applies each derived u16 delta independently"
        );
        test.expect_true(
            result.level_load.group == 2U && result.level_load.level == 3U &&
                result.level_load.output_value == 50U &&
                result.level_load.record_found &&
                result.baseline_profile_load.party_number_one_based == 2U &&
                result.baseline_profile_load.level == 2U &&
                result.baseline_profile_load.record_found &&
                result.advanced_profile_load.party_number_one_based == 2U &&
                result.advanced_profile_load.level == 3U &&
                result.advanced_profile_load.record_found &&
                fixture.port.requested_entries ==
                    std::vector<std::pair<u32, u32>>{
                        {2U, 3U},
                        {2U, 2U},
                        {2U, 3U},
                    } &&
                fixture.port.open_calls == 1U &&
                fixture.port.release_calls == 3U && fixture.port.calls.empty(),
            "level advancement shares one LEVEL session across the requirement and both direct profile loads"
        );
        test.expect_true(
            fixture.port.stop_calls ==
                    std::vector<std::array<u32, 4>>{{
                        0x12CU,
                        56U,
                        2U,
                        0x00020001U,
                    }} &&
                fixture.port.play_calls ==
                    std::vector<std::array<u32, 5>>{{
                        0x12BU,
                        0xFFFFFFFCU,
                        0xFFFFFFFCU,
                        2U,
                        0x00020001U,
                    }} &&
                result.return_eax == 0xABCD1201U,
            "level advancement stops the old cue and plays the level cue with the stale delta registers"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.party_resources[0U].field_00 = 100U;
        fixture.party_resources[0U].field_2c = 2U;
        fixture.port.allocation_succeeds = false;
        const auto result =
            run(fixture,
                {
                    .requirement_output_token = 0x71000000U,
                    .requirement_number_of_bytes_read_token = 0x72000000U,
                });
        test.expect_true(
            result.status ==
                    LegacyBattleLevelAdvancementStatus::
                        level_requirement_typed_stop &&
                result.requirement_calls == 1U &&
                result.profile_build_calls == 0U &&
                result.level_load.return_eax == 0U &&
                result.level_load.return_ecx == 0x100U &&
                result.level_load.return_edx == 0x72000000U &&
                fixture.state.completion_gate == 0U &&
                fixture.port.release_calls == 0U,
            "level advancement propagates the LEVEL stream-clear fault before the experience comparison and completion tail"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.party_resources[0U].field_00 = 100U;
        fixture.party_resources[0U].field_2c = 2U;
        fixture.port.custom_streams.push_back(make_requirement_stream(50U));
        fixture.port.custom_streams.push_back(
            make_profile_stream(make_profile(10U, 2U, 500U))
        );
        const auto result =
            run(fixture,
                {
                    .requirement_output_token = 0x71000000U,
                    .baseline_output_accessible_bytes = 0x12U,
                });
        test.expect_true(
            result.status ==
                    LegacyBattleLevelAdvancementStatus::
                        level_profile_typed_stop &&
                result.requirement_calls == 1U &&
                result.profile_build_calls == 1U &&
                result.baseline_profile_load.status ==
                    openswd3::battle::LegacyBattleLevelProfileLoadStatus::
                        output_access_typed_stop &&
                result.baseline_profile_load.output_bytes_copied == 8U &&
                result.advanced_profile_load.read_calls == 0U &&
                fixture.state.completion_gate == 0U &&
                fixture.target.transition_actor_index == 0U,
            "level advancement propagates the first direct profile fault before the second profile and completion tail"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 4U;
        const auto result = run(fixture, {.entry_edx = 0xABCD1234U});
        test.expect_true(
            result.status ==
                    LegacyBattleLevelAdvancementStatus::
                        party_member_resource_typed_stop &&
                fixture.state.completion_gate == 0U &&
                result.return_eax == 4U && result.return_ecx == 28U &&
                result.return_edx == 0xABCD0000U,
            "level advancement stops at the first out-of-range profile level read after original address arithmetic"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 11U;
        fixture.victory.group_a_skip_primary.fill(1U);
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleLevelAdvancementStatus::
                        group_a_actor_typed_stop &&
                result.visited_actors == 11U &&
                fixture.state.completion_gate == 0U &&
                result.return_eax == 10U && result.return_ecx == 11U,
            "level advancement stops on the eleventh physical actor without a modern scan cap"
        );
    }
}
