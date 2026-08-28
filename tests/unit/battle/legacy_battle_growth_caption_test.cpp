#include "openswd3/battle/legacy_battle_growth_completion_caption.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <map>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleGrowthCaptionCall;
using openswd3::battle::LegacyBattleGrowthCaptionCallReply;
using openswd3::battle::LegacyBattleGrowthCaptionCallRequest;
using openswd3::battle::LegacyBattleGrowthCaptionRequest;
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
    std::size_t successful_loads{1000U};
};

class Port final : public openswd3::battle::LegacyBattleGrowthCaptionPort {
public:
    [[nodiscard]] LegacyBattleGrowthCaptionCallReply invoke_growth_caption(
        const LegacyBattleGrowthCaptionCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            return found->second[index++];
        }
        return LegacyBattleGrowthCaptionPort::invoke_growth_caption(request);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGrowthCaptionRegisters
    play_growth_completion_sample(
        const u32 eax,
        const u32 ecx,
        const u32 edx,
        const u32 sound_id,
        const i32 mix_level
    ) override {
        sample_calls.push_back({
            eax,
            ecx,
            edx,
            sound_id,
            std::bit_cast<u32>(mix_level),
        });
        return sample_reply;
    }

    void reply(
        const LegacyBattleGrowthCaptionCall call,
        const LegacyBattleGrowthCaptionCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32
    count(const LegacyBattleGrowthCaptionCall call) const noexcept {
        return static_cast<u32>(std::count_if(
            calls.begin(), calls.end(), [call](const auto& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleGrowthCaptionCallRequest> calls;
    std::map<
        LegacyBattleGrowthCaptionCall,
        std::vector<LegacyBattleGrowthCaptionCallReply>>
        replies;
    std::map<LegacyBattleGrowthCaptionCall, std::size_t> reply_indices;
    std::vector<std::array<u32, 5U>> sample_calls;
    openswd3::battle::LegacyBattleGrowthCaptionRegisters sample_reply{
        .eax = 0x10203040U,
        .ecx = 0x50607080U,
        .edx = 0x90A0B0C0U,
    };
};

struct Fixture {
    Fixture() : action_updater(action_streams), raster(framebuffer.geometry()) {
        target.transition_mode = 1U;
        target.transition_actor_index = 2U;
        target.transition_stage = 56U;
        startup.action_mode_source.actor_label_indices[2U] = 1U;
        victory.panel_action_record.field_4a = 0x77U;
        advancement.growth_caption_text = {0xA6U, 0xA8U, 0xAAU, 0xACU, 0U};
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGrowthCaptionBindings
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

[[nodiscard]] openswd3::battle::LegacyBattleGrowthCaptionResult
run(Fixture& fixture,
    const LegacyBattleGrowthCaptionRequest& request = {
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0x55556666U,
        .rectangle_return =
            {
                .eax = 0x77778888U,
                .ecx = 0x9999AAAAU,
                .edx = 0xABCD5555U,
            },
        .frame_return =
            {
                .eax = 0xBBBBCCCCU,
                .ecx = 0xDDDDEEEEU,
                .edx = 0xFFFF0000U,
            },
        .local_text_token = 0x70002000U,
    }) {
    return openswd3::battle::advance_legacy_battle_growth_caption(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] openswd3::battle::LegacyBattleGrowthCaptionResult run_completion(
    Fixture& fixture,
    LegacyBattleGrowthCaptionRequest request = {
        .initial_text_byte = 0x7FU,
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0x55556666U,
        .rectangle_return =
            {
                .eax = 0x77778888U,
                .ecx = 0x9999AAAAU,
                .edx = 0xABCD5555U,
            },
        .frame_return =
            {
                .eax = 0xBBBBCCCCU,
                .ecx = 0xDDDDEEEEU,
                .edx = 0xFFFF0000U,
            },
        .local_text_token = 0x70002000U,
    }
) {
    return openswd3::battle::advance_legacy_battle_growth_completion_caption(
        fixture.bindings(), fixture.port, request
    );
}

void show_panel(Port& port) {
    port.reply(
        LegacyBattleGrowthCaptionCall::reserved_transition_stage_advance_slot,
        {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
    );
}

[[nodiscard]] std::vector<u8>
text_bytes(const LegacyBattleGrowthCaptionCallRequest& request) {
    return {
        request.text.begin(),
        request.text.begin() + static_cast<std::ptrdiff_t>(request.text_length),
    };
}

}  // namespace

void test_battle_growth_caption(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGrowthCaptionStatus;

    {
        Fixture fixture;
        fixture.target.transition_mode = 2U;

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.port_calls == 0U && result.rectangle_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x55556666U,
            "growth caption requires transition mode exactly one"
        );
    }

    {
        Fixture fixture;
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.name_length == 4U && result.detail_length == 6U &&
                result.rectangle_width == 48 && result.detail_x == 250 &&
                result.format_calls == 2U && result.length_calls == 2U &&
                result.text_draw_calls == 2U &&
                result.transition_stage_calls == 1U &&
                result.transition_stage.return_eax == 1U &&
                fixture.port.count(
                    LegacyBattleGrowthCaptionCall::
                        reserved_transition_stage_advance_slot
                ) == 0U &&
                fixture.victory.panel_action_record.action_id == 0x233BU &&
                !fixture.frame_provider.resource_ids.empty() &&
                fixture.frame_provider.resource_ids.front() == 0xABCD0077U,
            "growth caption sizes the frame from the four-byte actor name and centers the bracketed detail"
        );
        const auto name = std::find_if(
            fixture.port.calls.begin(),
            fixture.port.calls.end(),
            [](const auto& request) {
                return request.call ==
                    LegacyBattleGrowthCaptionCall::format_name;
            }
        );
        const auto detail = std::find_if(
            fixture.port.calls.begin(),
            fixture.port.calls.end(),
            [](const auto& request) {
                return request.call ==
                    LegacyBattleGrowthCaptionCall::format_detail;
            }
        );
        test.expect_true(
            name != fixture.port.calls.end() &&
                text_bytes(*name) ==
                    std::vector<u8>{0xA9U, 0x67U, 0xA5U, 0x69U} &&
                detail != fixture.port.calls.end() &&
                text_bytes(*detail) ==
                    std::vector<u8>{0x5BU, 0xA6U, 0xA8U, 0xAAU, 0xACU, 0x5DU},
            "growth caption preserves the CP950 name and brackets the shared caption bytes"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_actor_index = 0xFFU;

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthCaptionStatus::actor_index_typed_stop &&
                result.port_calls == 0U && result.rectangle_calls == 0U &&
                result.return_ecx == 0xFFFFFFFFU,
            "growth caption sign-extends actor FF and stops at the first real label access"
        );
    }

    {
        Fixture fixture;
        LegacyBattleGrowthCaptionCallReply overflow{
            .eax = 64U,
            .ecx = 0x11110000U,
            .edx = 0x22220000U,
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        overflow.formatted_text.fill(0x41U);
        fixture.port.reply(
            LegacyBattleGrowthCaptionCall::format_name, overflow
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthCaptionStatus::
                        name_format_buffer_typed_stop &&
                result.format_calls == 1U && result.length_calls == 0U &&
                result.rectangle_calls == 0U && result.return_eax == 64U,
            "growth caption stops after an overflowing name format and before lstrlen or drawing"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_stage = 20U;

        const auto result = run(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.format_calls == 1U && result.length_calls == 1U &&
                result.text_draw_calls == 0U && result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                result.transition_stage_calls == 1U &&
                fixture.target.transition_stage == 32U,
            "growth caption keeps the frame but skips both text rows while the stage quotient is nonzero"
        );
    }

    {
        Fixture fixture;
        fixture.advancement.growth_caption_text.fill(0x41U);
        show_panel(fixture.port);

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthCaptionStatus::
                        caption_source_typed_stop &&
                result.format_calls == 2U && result.length_calls == 1U &&
                result.text_draw_calls == 1U,
            "growth caption preserves the name row and partial detail formatting before the missing source terminator"
        );
    }

    {
        Fixture fixture;
        show_panel(fixture.port);
        LegacyBattleGrowthCaptionCallReply overflow{
            .eax = 64U,
            .ecx = 0x11110000U,
            .edx = 0x22220000U,
            .publish_formatted_text = true,
            .formatted_text_length = 64U,
        };
        overflow.formatted_text.fill(0x42U);
        fixture.port.reply(
            LegacyBattleGrowthCaptionCall::format_detail, overflow
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthCaptionStatus::
                        detail_format_buffer_typed_stop &&
                result.format_calls == 2U && result.length_calls == 1U &&
                result.text_draw_calls == 1U && result.return_eax == 64U,
            "growth caption stops after detail formatting but before the second lstrlen and draw"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_mode = 2U;

        const auto result = run_completion(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.sample_calls == 0U && result.port_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x55556666U,
            "growth completion caption keeps the seeded local buffer private when transition mode is not exactly one"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_stage = 0U;
        fixture.input.sample_mix_level = -4;
        show_panel(fixture.port);

        const auto result = run_completion(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.sample_calls == 1U &&
                fixture.port.sample_calls ==
                    std::vector<std::array<u32, 5U>>{{
                        0U,
                        std::bit_cast<u32>(-4),
                        0x55556666U,
                        0x160U,
                        std::bit_cast<u32>(-4),
                    }} &&
                result.transition_stage_calls == 1U &&
                fixture.target.transition_stage == 18U &&
                result.text_draw_calls == 0U &&
                fixture.port.count(
                    LegacyBattleGrowthCaptionCall::
                        reserved_transition_stage_advance_slot
                ) == 0U,
            "growth completion caption plays the zero-stage sample then waits for the typed stage to settle"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_stage = 9U;
        fixture.port.reply(
            LegacyBattleGrowthCaptionCall::
                reserved_transition_stage_advance_slot,
            {.eax = 0U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );

        const auto result = run_completion(fixture);

        test.expect_true(
            result.status == LegacyBattleGrowthCaptionStatus::completed &&
                result.sample_calls == 0U &&
                fixture.port.sample_calls.empty() &&
                result.text_draw_calls == 0U,
            "growth completion caption skips its sample for every nonzero live stage"
        );
    }

    {
        Fixture fixture;
        fixture.target.transition_stage = 0U;
        fixture.target.transition_actor_index = 0xFFU;

        const auto result = run_completion(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthCaptionStatus::actor_index_typed_stop &&
                result.sample_calls == 1U && result.format_calls == 0U &&
                result.return_edx == 0xFFFFFFFFU,
            "growth completion caption plays before the signed actor minus-one label access stops"
        );
    }
}
