#include "openswd3/battle/legacy_battle_context_prompt.hpp"

#include <array>
#include <bit>
#include <span>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    explicit ActionStreamProvider(const bool ready) : ready_(ready) {
        constexpr std::array<u16, 8> words{
            0x5246U,
            0x0066U,
            0x5041U,
            0U,
            0x5859U,
            2U,
            3U,
            0x4544U,
        };
        for (const u16 word : words) {
            bytes_.push_back(static_cast<u8>(word));
            bytes_.push_back(static_cast<u8>(word >> 8U));
        }
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        action_ids.push_back(action_id);
        variant_indices.push_back(variant_index);
        cached_values.push_back(cached);
        if (!ready_) {
            return {};
        }
        return {
            .status = openswd3::asset_runtime::LegacyActionStreamStatus::ready,
            .stream = bytes_,
        };
    }

    std::vector<u32> action_ids;
    std::vector<u32> variant_indices;
    std::vector<bool> cached_values;

private:
    bool ready_{};
    std::vector<u8> bytes_;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        requests.push_back({resource_id, piece_index});
        if (!available) {
            return false;
        }
        piece = {
            .source =
                openswd3::rendering::LegacyBlitSource{
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    bool available{true};
    std::array<u8, 2> bytes{0x34U, 0x12U};
    std::vector<std::array<u32, 2>> requests;
};

struct Environment {
    openswd3::battle::LegacyBattleContextPromptState prompt;
    openswd3::battle::LegacyBattleActionDispatchState action;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleStartupState startup;
    u32 message_state{};
    openswd3::rendering::LegacyFramebuffer framebuffer{
        {.pitch_bytes = 128, .width = 64, .height = 64}
    };
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 64, 64};
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;

    [[nodiscard]] openswd3::battle::LegacyBattleContextPromptBindings bindings(
        openswd3::asset_runtime::LegacyActionUpdater& updater,
        openswd3::rendering::LegacyFramePieceProvider& provider
    ) {
        return {
            .prompt = prompt,
            .action = action,
            .final_actor = final_actor,
            .startup = startup,
            .message_state = message_state,
            .framebuffer = framebuffer,
            .clip = clip,
            .shared_request = request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = updater,
            .frame_provider = provider,
        };
    }
};

}  // namespace

void test_battle_context_prompt(openswd3::test::Context& test) {
    {
        Environment environment;
        environment.prompt.frame_counter = 299U;
        environment.action.message_gate = 0U;
        ActionStreamProvider stream{false};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames), port, {}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleContextPromptStatus::
                        completed &&
                environment.prompt.frame_counter == 300U &&
                result.return_value == 300U && result.draw_calls == 0U &&
                stream.action_ids.empty(),
            "context prompt returns the incremented counter at the signed three-hundred gate"
        );
    }

    {
        Environment environment;
        environment.prompt.frame_counter = 0xFFFFFFFFU;
        environment.message_state = 1U;
        ActionStreamProvider stream{false};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames),
            port,
            {.mouse_x = 12, .mouse_y = 34}
        );
        test.expect_true(
            environment.prompt.frame_counter == 0U &&
                result.branch ==
                    openswd3::battle::LegacyBattleContextPromptBranch::
                        generic_cursor &&
                result.action_id == 0x238EU && result.x == 12 &&
                result.y == 34 && result.draw_calls == 1U &&
                result.draw.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        action_update_failed,
            "context prompt counter wraps and signed comparison continues into the switch"
        );
    }

    {
        constexpr std::array<u32, 7> generic{1U, 2U, 4U, 5U, 8U, 27U, 30U};
        for (const u32 message : generic) {
            Environment environment;
            environment.message_state = message;
            ActionStreamProvider stream{false};
            openswd3::asset_runtime::LegacyActionUpdater updater{stream};
            FrameProvider frames;
            openswd3::battle::LegacyBattleContextPromptPort port;
            const auto result =
                openswd3::battle::draw_legacy_battle_context_prompt(
                    environment.bindings(updater, frames),
                    port,
                    {.mouse_x = -5, .mouse_y = 9}
                );
            test.expect_true(
                result.branch ==
                        openswd3::battle::LegacyBattleContextPromptBranch::
                            generic_cursor &&
                    result.action_id == 0x238EU && result.base_variant == 0U &&
                    result.x == -5 && result.y == 9 && result.offset_mode == 0U,
                "context prompt switch routes every generic case to the common cursor action"
            );
        }
    }

    {
        Environment environment;
        environment.message_state = 3U;
        environment.final_actor.pre_frame_gate_b = 0U;
        ActionStreamProvider stream{false};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        const auto gated = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames), port, {}
        );
        test.expect_true(
            gated.branch ==
                    openswd3::battle::LegacyBattleContextPromptBranch::none &&
                gated.return_value == 2U && gated.draw_calls == 0U,
            "context prompt case three returns the stale switch index when its gate is not one"
        );

        environment.final_actor.pre_frame_gate_b = 1U;
        environment.prompt.case_three_resource_selector = 0U;
        const auto zero_selector =
            openswd3::battle::draw_legacy_battle_context_prompt(
                environment.bindings(updater, frames), port, {}
            );
        environment.prompt.case_three_resource_selector = 2U;
        const auto nonzero_selector =
            openswd3::battle::draw_legacy_battle_context_prompt(
                environment.bindings(updater, frames), port, {}
            );
        test.expect_true(
            zero_selector.action_id == 0x2393U &&
                nonzero_selector.action_id == 0x238FU &&
                nonzero_selector.branch ==
                    openswd3::battle::LegacyBattleContextPromptBranch::
                        case_three,
            "context prompt case three distinguishes only zero from nonzero resource selection"
        );
    }

    {
        constexpr std::array<u32, 23> defaults{
            0U,  6U,  7U,  9U,  10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U,
            18U, 19U, 20U, 21U, 22U, 23U, 24U, 25U, 26U, 28U, 29U,
        };
        for (const u32 message : defaults) {
            Environment environment;
            environment.message_state = message;
            environment.final_actor.active_actor_code = 0U;
            ActionStreamProvider stream{false};
            openswd3::asset_runtime::LegacyActionUpdater updater{stream};
            FrameProvider frames;
            openswd3::battle::LegacyBattleContextPromptPort port;
            const auto result =
                openswd3::battle::draw_legacy_battle_context_prompt(
                    environment.bindings(updater, frames), port, {}
                );
            test.expect_true(
                result.branch ==
                        openswd3::battle::LegacyBattleContextPromptBranch::
                            actor_cursor &&
                    result.action_id == 0x238CU,
                "context prompt indirect switch routes every default case to actor-cursor selection"
            );
        }

        Environment out_of_range;
        out_of_range.message_state = 31U;
        out_of_range.final_actor.active_actor_code = 8U;
        ActionStreamProvider stream{false};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            out_of_range.bindings(updater, frames), port, {}
        );
        test.expect_true(
            result.action_id == 0x238DU,
            "context prompt out-of-range switch values use the live actor-presence action"
        );
    }

    {
        Environment environment;
        environment.prompt.frame_counter = 299U;
        environment.action.message_gate = 0x80000000U;
        environment.action.message_aux = 3U;
        environment.action.selection_word = 0xFFF0U;
        environment.action.selection_high_word = 0x0009U;
        environment.startup.mirror_mode = 7U;
        ActionStreamProvider stream{false};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames), port, {}
        );
        test.expect_true(
            environment.prompt.frame_counter == 300U &&
                result.branch ==
                    openswd3::battle::LegacyBattleContextPromptBranch::
                        message_actor &&
                result.action_id == 0x23A0U && result.base_variant == 3U &&
                result.x == -16 && result.y == 9 && result.offset_mode == 1U &&
                environment.action.message_aux == 3U,
            "message mode bypasses the counter gate and sign-extends both actor coordinates"
        );
    }

    {
        Environment environment;
        environment.action.message_gate = 0x80000000U;
        environment.action.message_aux = 2U;
        environment.action.selection_word = 10U;
        environment.action.selection_high_word = 11U;
        environment.startup.mirror_mode = 0U;
        ActionStreamProvider stream{true};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        openswd3::battle::LegacyBattleContextPromptPort port;
        port.battle_offset_action_frame_draw_state().result_latch = 1U;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames), port, {}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleContextPromptStatus::
                        completed &&
                result.return_value == 1U && result.offset_mode == 0U &&
                environment.action.message_aux == 0U &&
                stream.action_ids == std::vector<u32>{0x23A0U} &&
                stream.variant_indices == std::vector<u32>{2U},
            "message actor draw clears its shared auxiliary only after an exact one return"
        );
    }

    {
        Environment environment;
        environment.action.message_gate = 0x80000000U;
        environment.action.message_aux = 5U;
        environment.action.selection_word = 10U;
        environment.action.selection_high_word = 11U;
        ActionStreamProvider stream{true};
        openswd3::asset_runtime::LegacyActionUpdater updater{stream};
        FrameProvider frames;
        frames.available = false;
        openswd3::battle::LegacyBattleContextPromptPort port;
        port.battle_offset_action_frame_draw_state().result_latch = 1U;
        const auto result = openswd3::battle::draw_legacy_battle_context_prompt(
            environment.bindings(updater, frames),
            port,
            {.action_update_edx_snapshot = 0xABCD1234U}
        );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleContextPromptStatus::
                        offset_action_frame_typed_stop &&
                result.draw.status ==
                    openswd3::battle::LegacyBattleOffsetActionFrameDrawStatus::
                        frame_unavailable &&
                environment.action.message_aux == 5U &&
                port.battle_offset_action_frame_draw_state().frame_index ==
                    0xABCD0000U,
            "context prompt preserves its draw prefix and auxiliary at the first unavailable-frame access"
        );
    }
}
