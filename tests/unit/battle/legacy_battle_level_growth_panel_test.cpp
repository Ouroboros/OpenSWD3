#include "openswd3/battle/legacy_battle_level_growth_panel.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <map>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleLevelGrowthPanelCall;
using openswd3::battle::LegacyBattleLevelGrowthPanelCallReply;
using openswd3::battle::LegacyBattleLevelGrowthPanelCallRequest;
using openswd3::battle::LegacyBattleLevelGrowthPanelRequest;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(const u32, const u32, const bool) override {
        constexpr std::array<u16, 8U> kWords{
            0x5246U, 0x0077U, 0x5041U, 0U, 0x5859U, 2U, 3U, 0x4544U
        };
        bytes.clear();
        for (const u16 word : kWords) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
        return {
            openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            bytes,
            false,
        };
    }

    std::vector<u8> bytes;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (resource_ids.size() > successful_loads || piece_index >= 9U) {
            piece = {};
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = pixels[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::array<u8, 2U>, 9U> pixels{{
        {1U, 0U},
        {2U, 0U},
        {3U, 0U},
        {4U, 0U},
        {5U, 0U},
        {6U, 0U},
        {7U, 0U},
        {8U, 0U},
        {9U, 0U},
    }};
    std::vector<u32> resource_ids;
    std::size_t successful_loads{2000U};
};

class Port final : public openswd3::battle::LegacyBattleLevelGrowthPanelPort {
public:
    [[nodiscard]] LegacyBattleLevelGrowthPanelCallReply
    invoke_level_growth_panel(
        const LegacyBattleLevelGrowthPanelCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            return found->second[index++];
        }
        return LegacyBattleLevelGrowthPanelPort::invoke_level_growth_panel(
            request
        );
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelGrowthPanelRegisters
    play_level_growth_sample(
        const u32 eax,
        const u32 ecx,
        const u32 edx,
        const u32 sound_id,
        const i32 mix_level
    ) override {
        samples.push_back(
            {eax, ecx, edx, sound_id, std::bit_cast<u32>(mix_level)}
        );
        return {.eax = 0xFACE1200U, .ecx = ecx, .edx = edx};
    }

    void reply(
        const LegacyBattleLevelGrowthPanelCall call,
        const LegacyBattleLevelGrowthPanelCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32
    count(const LegacyBattleLevelGrowthPanelCall call) const noexcept {
        return static_cast<u32>(std::count_if(
            calls.begin(), calls.end(), [call](const auto& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleLevelGrowthPanelCallRequest> calls;
    std::map<
        LegacyBattleLevelGrowthPanelCall,
        std::vector<LegacyBattleLevelGrowthPanelCallReply>>
        replies;
    std::map<LegacyBattleLevelGrowthPanelCall, std::size_t> reply_indices;
    std::vector<std::array<u32, 5U>> samples;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        target.transition_actor_index = 2U;
        startup.action_mode_source.actor_label_indices[2U] = 1U;
        victory.panel_action_record.field_4a = 0x77U;
        input.sample_mix_level = -6;
        std::array<u16*, 3U> baseline_limits{
            &advancement.profile_copy_scratch.limit_first,
            &advancement.profile_copy_scratch.limit_second,
            &advancement.profile_copy_scratch.limit_third,
        };
        std::array<u16*, 3U> current_limits{
            &party_resources[1U].limit_first,
            &party_resources[1U].limit_second,
            &party_resources[1U].limit_third,
        };
        for (std::size_t index = 0U; index < 3U; ++index) {
            *baseline_limits[index] = static_cast<u16>(10U + index);
            *current_limits[index] = static_cast<u16>(12U + index * 2U);
        }
        for (std::size_t index = 0U; index < 8U; ++index) {
            advancement.profile_copy_scratch.fields_10_to_1e[index] =
                static_cast<u16>(20U + index);
            party_resources[1U].fields_10_to_1e[index] =
                static_cast<u16>(25U + index * 2U);
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelGrowthPanelBindings
    bindings() {
        return {
            .advancement = advancement,
            .victory = {
                .state = victory,
                .startup = startup,
                .metrics = metrics,
                .input_dispatch = input,
                .target_selection = target,
                .party_member_resources = party_resources,
                .script_variables = script_variables,
                .framebuffer = framebuffer,
                .raster = raster,
                .shared_effects = effects,
                .jitter = jitter,
                .action_updater = action_updater,
                .frame_provider = frame_provider,
            },
        };
    }

    openswd3::battle::LegacyBattleLevelAdvancementState advancement;
    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleInputDispatchState input;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    std::array<openswd3::world_map::LegacyWorldStoryPartyMemberResources, 4U>
        party_resources{};
    std::array<u32, 8U> script_variables{};
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    Port port;
};

[[nodiscard]] openswd3::battle::LegacyBattleLevelGrowthPanelResult
run(Fixture& fixture,
    const LegacyBattleLevelGrowthPanelRequest& request = {
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0xABCD5555U,
        .rectangle_return =
            {
                .eax = 0x01020304U,
                .ecx = 0x11112222U,
                .edx = 0xABCD5555U,
            },
        .title_frame_return =
            {
                .eax = 0x11110000U,
                .ecx = 0x22220000U,
                .edx = 0x33330000U,
            },
        .summary_frame_return =
            {
                .eax = 0x44440000U,
                .ecx = 0x55550000U,
                .edx = 0x66660000U,
            },
        .local_text_token = 0x70001000U,
    }) {
    return openswd3::battle::advance_legacy_battle_level_growth_panel(
        fixture.bindings(), fixture.port, request
    );
}

void show_panel(Port& port) {
    port.reply(
        LegacyBattleLevelGrowthPanelCall::query_panel,
        {.eax = 1U, .ecx = 0xAAAA0000U, .edx = 0xBBBB0000U}
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleLevelGrowthPanelCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

}  // namespace

void test_battle_level_growth_panel(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleLevelGrowthPanelStatus;

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 0xFFU;

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleLevelGrowthPanelStatus::completed &&
                result.port_calls == 0U && result.rectangle_calls == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0xABCD5555U,
            "growth panel skips every side effect when no transition actor exists"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 0U;
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleLevelGrowthPanelStatus::completed &&
                fixture.input.target_transition_word == 1U &&
                result.format_calls == 7U && result.text_draw_calls == 8U &&
                result.sample_calls == 0U &&
                fixture.port.count(
                    LegacyBattleLevelGrowthPanelCall::query_panel
                ) == 1U &&
                fixture.port.count(
                    LegacyBattleLevelGrowthPanelCall::format_integer
                ) == 7U &&
                fixture.victory.panel_action_record.action_id == 0x233BU &&
                fixture.victory.panel_action_record.base_variant == 0U,
            "growth panel draws the name and seven baseline values before the stage-30 gate"
        );
        const auto format = std::find_if(
            fixture.port.calls.begin(),
            fixture.port.calls.end(),
            [](const auto& request) {
                return request.call ==
                    LegacyBattleLevelGrowthPanelCall::format_integer;
            }
        );
        const std::array<u8, 11U> expected{
            0xA5U,
            0xCDU,
            0xA9U,
            0x52U,
            0xA4U,
            0x4FU,
            0x3AU,
            0x20U,
            0x20U,
            0x31U,
            0x30U,
        };
        test.expect_true(
            format != fixture.port.calls.end() &&
                text_bytes(*format) ==
                    std::vector<u8>(expected.begin(), expected.end()),
            "growth panel formats the CP950 life label with wsprintf-style width four"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 29U;
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleLevelGrowthPanelStatus::completed &&
                fixture.input.target_transition_word == 30U &&
                fixture.advancement.growth_delta_primary ==
                    std::array<u16, 3U>{1U, 3U, 4U} &&
                fixture.advancement.growth_delta_secondary ==
                    std::array<u16, 6U>{4U, 6U, 7U, 8U, 9U, 10U} &&
                result.displayed_growth_values == 7U &&
                result.decremented_growth_values == 2U &&
                result.sample_calls == 2U &&
                fixture.port.samples[0U][2U] ==
                    std::bit_cast<u32>(fixture.input.sample_mix_level) &&
                fixture.port.samples[0U][3U] == 0x125U,
            "stage 29 snapshots nine wrapping deltas and starts both seven-field animation chains"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 30U;
        fixture.advancement.growth_delta_primary = {0U, 2U, 3U};
        fixture.advancement.growth_delta_secondary = {0U, 2U, 3U, 4U, 5U, 6U};
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleLevelGrowthPanelStatus::completed &&
                fixture.advancement.growth_delta_primary ==
                    std::array<u16, 3U>{0U, 1U, 3U} &&
                fixture.advancement.growth_delta_secondary ==
                    std::array<u16, 6U>{0U, 1U, 3U, 4U, 5U, 6U} &&
                result.sample_calls == 2U,
            "growth animation advances only the first pending value in each serial chain"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 100U;
        fixture.advancement.growth_delta_primary = {1U, 2U, 3U};
        fixture.advancement.growth_delta_secondary = {4U, 5U, 6U, 7U, 8U, 9U};
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleLevelGrowthPanelStatus::completed &&
                fixture.advancement.growth_delta_primary ==
                    std::array<u16, 3U>{0U, 0U, 0U} &&
                fixture.advancement.growth_delta_secondary ==
                    std::array<u16, 6U>{0U, 0U, 0U, 0U, 0U, 0U} &&
                result.displayed_growth_values == 7U &&
                result.sample_calls == 0U,
            "stage 100 clears all nine deltas before rendering final current values"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 100U;
        fixture.advancement.profile_copy_scratch.limit_first = 0U;
        fixture.party_resources[1U].limit_first = 0x8000U;
        for (std::size_t index = 1U; index < 7U; ++index) {
            if (index == 1U) {
                fixture.party_resources[1U].limit_second =
                    fixture.advancement.profile_copy_scratch.limit_second;
            } else if (index == 2U) {
                fixture.party_resources[1U].limit_third =
                    fixture.advancement.profile_copy_scratch.limit_third;
            } else {
                fixture.party_resources[1U].fields_10_to_1e[index - 3U] =
                    fixture.advancement.profile_copy_scratch
                        .fields_10_to_1e[index - 3U];
            }
        }
        fixture.advancement.profile_copy_scratch.fields_10_to_1e[0U] = 0U;
        fixture.party_resources[1U].fields_10_to_1e[0U] = 0x8000U;
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.displayed_growth_values == 1U,
            "growth panel compares the first three values signed and the remaining values unsigned"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 29U;
        fixture.startup.action_mode_source.actor_label_indices[2U] = 4U;
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleLevelGrowthPanelStatus::
                        party_member_resource_typed_stop &&
                fixture.input.target_transition_word == 29U &&
                result.format_calls == 7U && result.text_draw_calls == 8U &&
                fixture.advancement.growth_delta_primary ==
                    std::array<u16, 3U>{0U, 0U, 0U},
            "growth panel preserves the fully drawn baseline prefix before the first missing profile access"
        );
    }

    {
        Fixture fixture;
        fixture.input.target_transition_word = 0U;
        show_panel(fixture.port);
        LegacyBattleLevelGrowthPanelCallReply overflow{
            .eax = 64U,
            .ecx = 0x11110000U,
            .edx = 0x22220000U,
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        overflow.formatted_text.fill(0x41U);
        fixture.port.reply(
            LegacyBattleLevelGrowthPanelCall::format_integer, overflow
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleLevelGrowthPanelStatus::
                        format_buffer_typed_stop &&
                result.format_calls == 1U && result.text_draw_calls == 1U &&
                fixture.input.target_transition_word == 0U,
            "growth panel stops after unbounded formatting but before the overflowing text draw and stage write"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 10U;

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleLevelGrowthPanelStatus::
                        actor_index_typed_stop &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.port_calls == 0U,
            "growth panel stops on the eleventh actor at the first real label access after the frame prefix"
        );
    }
}
