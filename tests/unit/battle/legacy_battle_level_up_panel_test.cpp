#include "openswd3/battle/legacy_battle_level_up_panel.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleLevelUpPanelRequest;
using openswd3::battle::LegacyBattleVictoryRewardCall;
using openswd3::battle::LegacyBattleVictoryRewardCallReply;
using openswd3::battle::LegacyBattleVictoryRewardCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool
    ) override {
        action_ids.push_back(action_id);
        variants.push_back(variant_index);
        constexpr std::array<u16, 8> kWords{
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
    std::vector<u32> action_ids;
    std::vector<u32> variants;
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
        piece_indices.push_back(piece_index);
        if (resource_ids.size() > successful_loads ||
            piece_index >= pixels.size()) {
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

    std::array<std::array<u8, 2>, 9> pixels{{
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
    std::vector<u32> piece_indices;
    std::size_t successful_loads{1000U};
};

class Port final : public openswd3::battle::LegacyBattleVictoryRewardPort {
public:
    [[nodiscard]] LegacyBattleVictoryRewardCallReply invoke_victory_reward(
        const LegacyBattleVictoryRewardCallRequest& request
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

    [[nodiscard]] openswd3::battle::LegacyBattleActionCallReply invoke(
        const openswd3::battle::LegacyBattleActionCallRequest& request
    ) override {
        action_calls.push_back(request);
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleInputDispatchCallReply
    invoke_input_dispatch(
        const openswd3::battle::LegacyBattleInputDispatchCallRequest& request
    ) override {
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    void reply(
        const LegacyBattleVictoryRewardCall call,
        const LegacyBattleVictoryRewardCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32 count(const LegacyBattleVictoryRewardCall call) const {
        u32 result = 0U;
        for (const auto& request : calls) {
            if (request.call == call) {
                ++result;
            }
        }
        return result;
    }

    std::vector<LegacyBattleVictoryRewardCallRequest> calls;
    std::map<
        LegacyBattleVictoryRewardCall,
        std::vector<LegacyBattleVictoryRewardCallReply>>
        replies;
    std::map<LegacyBattleVictoryRewardCall, std::size_t> reply_indices;
    std::vector<openswd3::battle::LegacyBattleActionCallRequest> action_calls;
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        target.transition_actor_index = 2U;
        target.transition_stage = 32U;
        startup.action_mode_source.actor_label_indices[2U] = 1U;
        party_resources[1U].field_2c = 7U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleLevelUpPanelBindings
    bindings() {
        return {
            .victory = victory,
            .startup = startup,
            .target_selection = target,
            .party_member_resources = party_resources,
            .framebuffer = framebuffer,
            .raster = raster,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
        };
    }

    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    std::array<openswd3::world_map::LegacyWorldStoryPartyMemberResources, 4>
        party_resources{};
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

[[nodiscard]] openswd3::battle::LegacyBattleLevelUpPanelResult
run(Fixture& fixture, const LegacyBattleLevelUpPanelRequest& request = {}) {
    return openswd3::battle::draw_legacy_battle_level_up_panel(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleVictoryRewardCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

[[nodiscard]] LegacyBattleVictoryRewardCallReply formatted_reply(
    const std::span<const u8> bytes,
    const u32 eax = 9U,
    const u32 ecx = 0x11112222U,
    const u32 edx = 0x33334444U
) {
    LegacyBattleVictoryRewardCallReply result{
        .eax = eax,
        .ecx = ecx,
        .edx = edx,
        .formatted_text_length = static_cast<u32>(bytes.size()),
    };
    std::copy(bytes.begin(), bytes.end(), result.formatted_text.begin());
    return result;
}

}  // namespace

void test_battle_level_up_panel(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleLevelUpPanelStatus;

    {
        Fixture fixture;
        fixture.target.transition_stage = 32U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {
                .eax = 1U,
                .ecx = 0x11110000U,
                .edx = 0x22220000U,
                .publish_transition_actor_index = true,
                .transition_actor_index = 3U,
            }
        );
        fixture.startup.action_mode_source.actor_label_indices[3U] = 1U;
        constexpr std::array<u8, 9> kLevelText{
            'A', 0xA4U, 0xC9U, 0xB2U, 0xC4U, '7', 0xAFU, 0xC5U, '!'
        };
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::format_level_up_text,
            formatted_reply(kLevelText)
        );
        const auto result = run(
            fixture,
            {
                .entry_eax = 0xAAAAAAAAU,
                .entry_ecx = 0xBBBBBBBBU,
                .entry_edx = 0xCCCCCCCCU,
                .rectangle_return = {.eax = 2U, .ecx = 3U, .edx = 0xABCD0000U},
                .title_frame_return =
                    {.eax = 4U, .ecx = 5U, .edx = 0x12340000U},
                .summary_frame_return = {.eax = 6U, .ecx = 7U, .edx = 8U},
                .local_text_token = 0x71000000U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleLevelUpPanelStatus::completed &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.text_draw_calls == 2U && result.port_calls == 3U &&
                result.transition_stage_calls == 1U &&
                fixture.victory.panel_action_record.action_id == 0x233BU &&
                fixture.action_streams.action_ids ==
                    std::vector<u32>{0x233BU} &&
                fixture.target.transition_actor_index == 2U,
            "level-up presentation draws both panels, the fixed title and the live selected actor text"
        );
        const auto& title = fixture.port.calls[0U];
        const auto& format = fixture.port.calls[1U];
        const auto& draw = fixture.port.calls[2U];
        test.expect_true(
            title.call == LegacyBattleVictoryRewardCall::draw_text &&
                title.arguments[1U] == 0x108U && title.arguments[2U] == 0xB4U &&
                title.arguments[3U] == 0x004A7A44U &&
                text_bytes(title) ==
                    std::vector<u8>{0xA4U, 0xC9U, 0xAFU, 0xC5U} &&
                result.transition_stage.return_eax == 1U &&
                result.transition_stage.return_ecx == 1U &&
                fixture.port.count(
                    LegacyBattleVictoryRewardCall::
                        reserved_transition_stage_advance_slot
                ) == 0U,
            "level-up presentation preserves the fixed CP950 title and uses the typed stage gate"
        );
        test.expect_true(
            format.call ==
                    LegacyBattleVictoryRewardCall::format_level_up_text &&
                format.arguments[0U] == 0x71000000U &&
                format.arguments[1U] == 0x004A7A38U &&
                format.arguments[2U] == 0x0049E158U &&
                format.arguments[3U] == 7U && format.eax == 0x71000000U &&
                format.ecx == 7U && format.edx == 7U &&
                draw.call == LegacyBattleVictoryRewardCall::draw_text &&
                draw.arguments[1U] == 0xD0U && draw.arguments[2U] == 0xDCU &&
                draw.eax == 9U && draw.ecx == 0x004C9A28U &&
                draw.edx == 0x004CD76CU &&
                text_bytes(draw) ==
                    std::vector<u8>(kLevelText.begin(), kLevelText.end()),
            "level-up presentation reads the live actor after the stage gate and preserves the format and draw register snapshots"
        );
        test.expect_true(
            !fixture.frame_provider.resource_ids.empty() &&
                (fixture.frame_provider.resource_ids[0U] & 0xFFFF0000U) ==
                    0xABCD0000U &&
                (fixture.frame_provider.resource_ids.back() & 0xFFFF0000U) ==
                    0x12340000U,
            "level-up panel resources preserve the rectangle and title-text high-word chain"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 0xFFU;
        fixture.target.transition_mode = 0U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U, .ecx = 2U, .edx = 3U}
        );
        const auto result =
            run(fixture, {.entry_eax = 7U, .entry_ecx = 8U, .entry_edx = 9U});
        test.expect_true(
            result.status == LegacyBattleLevelUpPanelStatus::completed &&
                result.rectangle_calls == 0U &&
                result.tiled_frame_calls == 0U &&
                result.text_draw_calls == 0U && result.port_calls == 0U &&
                result.transition_stage_calls == 1U &&
                result.return_eax == 0xFFU && result.return_ecx == 1U &&
                result.return_edx == 0U,
            "missing actor outside transition mode one skips the panel but still runs the typed stage gate"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 0xFFU;
        fixture.target.transition_mode = 1U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status == LegacyBattleLevelUpPanelStatus::completed &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.text_draw_calls == 1U && result.port_calls == 1U &&
                result.transition_stage_calls == 1U,
            "transition mode one still draws the level-up panel without a selected actor"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 0x80U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleLevelUpPanelStatus::actor_index_typed_stop &&
                result.return_eax == 0xFFFFFF80U && result.return_edx == 0U &&
                result.port_calls == 1U,
            "negative selected actor stops at the first signed action-label table access"
        );
    }

    {
        Fixture fixture;
        fixture.startup.action_mode_source.actor_label_indices[2U] = 4U;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleLevelUpPanelStatus::
                        party_member_resource_typed_stop &&
                result.return_eax == 64U && result.return_ecx == 28U &&
                result.return_edx == 0U,
            "out-of-range action label stops after the original name and profile offset arithmetic"
        );
    }

    {
        Fixture fixture;
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::
                reserved_transition_stage_advance_slot,
            {.eax = 1U}
        );
        std::array<u8, 64> oversized{};
        oversized.fill('X');
        fixture.port.reply(
            LegacyBattleVictoryRewardCall::format_level_up_text,
            formatted_reply(oversized)
        );
        const auto result = run(fixture, {.local_text_token = 0x72000000U});
        test.expect_true(
            result.status ==
                    LegacyBattleLevelUpPanelStatus::format_buffer_typed_stop &&
                result.text_draw_calls == 1U && result.port_calls == 2U &&
                result.transition_stage_calls == 1U &&
                fixture.port.count(LegacyBattleVictoryRewardCall::draw_text) ==
                    1U,
            "level-up formatting stops after the formatter side effect and before the overflowing draw"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.successful_loads = 0U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleLevelUpPanelStatus::title_frame_typed_stop &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U && result.port_calls == 0U,
            "level-up title-frame failure preserves the rectangle prefix and blocks text and summary work"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.successful_loads = 213U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleLevelUpPanelStatus::summary_frame_typed_stop &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 2U &&
                result.text_draw_calls == 1U && result.port_calls == 1U,
            "level-up summary-frame failure preserves the title prefix and blocks the summary query"
        );
    }
}
