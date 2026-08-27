#include "openswd3/battle/legacy_battle_context_prompt.hpp"

#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;

constexpr std::array<u32, 7> kGenericCursorMessages{
    1U,
    2U,
    4U,
    5U,
    8U,
    27U,
    30U,
};

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr bool
is_generic_cursor_message(const u32 message_state) noexcept {
    for (const u32 value : kGenericCursorMessages) {
        if (message_state == value) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] constexpr bool
is_typed_stop(const LegacyBattleOffsetActionFrameDrawStatus status) noexcept {
    return status ==
        LegacyBattleOffsetActionFrameDrawStatus::frame_unavailable ||
        status == LegacyBattleOffsetActionFrameDrawStatus::blit_typed_stop;
}

}  // namespace

LegacyBattleContextPromptResult draw_legacy_battle_context_prompt(
    const LegacyBattleContextPromptBindings bindings,
    LegacyBattleContextPromptPort& port,
    const LegacyBattleContextPromptRequest& request
) {
    LegacyBattleContextPromptResult result;
    const u32 message_gate = bindings.action.message_gate;
    bindings.prompt.frame_counter += 1U;
    result.return_value = bindings.prompt.frame_counter;
    if (signed_bits(bindings.prompt.frame_counter) >= 300 &&
        (message_gate & 0x80000000U) == 0U) {
        return result;
    }

    const auto draw = [&](const LegacyBattleContextPromptBranch branch,
                          const u32 action_id,
                          const u32 base_variant,
                          const i32 x,
                          const i32 y,
                          const u32 offset_mode) {
        result.branch = branch;
        result.action_id = action_id;
        result.base_variant = base_variant;
        result.x = x;
        result.y = y;
        result.offset_mode = offset_mode;
        result.draw = draw_legacy_battle_offset_action_frame(
            port.battle_offset_action_frame_draw_state(),
            bindings.framebuffer,
            bindings.clip,
            bindings.shared_request,
            bindings.shared_effects,
            bindings.jitter,
            bindings.action_updater,
            bindings.frame_provider,
            action_id,
            base_variant,
            x,
            y,
            offset_mode,
            request.action_update_edx_snapshot
        );
        ++result.draw_calls;
        result.return_value = result.draw.return_value;
        if (is_typed_stop(result.draw.status)) {
            result.status =
                LegacyBattleContextPromptStatus::offset_action_frame_typed_stop;
        }
    };

    const u32 message_state = bindings.message_state;
    if (is_generic_cursor_message(message_state)) {
        draw(
            LegacyBattleContextPromptBranch::generic_cursor,
            0x238EU,
            0U,
            request.mouse_x,
            request.mouse_y,
            0U
        );
        return result;
    }

    if (message_state == 3U) {
        if (bindings.final_actor.pre_frame_gate_b != 1U) {
            result.return_value = message_state - 1U;
            return result;
        }
        draw(
            LegacyBattleContextPromptBranch::case_three,
            bindings.prompt.case_three_resource_selector == 0U ? 0x2393U
                                                               : 0x238FU,
            0U,
            request.mouse_x,
            request.mouse_y,
            0U
        );
        return result;
    }

    if ((message_gate & 0x80000000U) == 0U) {
        draw(
            LegacyBattleContextPromptBranch::actor_cursor,
            bindings.final_actor.active_actor_code != 0U ? 0x238DU : 0x238CU,
            0U,
            request.mouse_x,
            request.mouse_y,
            0U
        );
        return result;
    }

    draw(
        LegacyBattleContextPromptBranch::message_actor,
        0x23A0U,
        bindings.action.message_aux,
        static_cast<i32>(
            std::bit_cast<compat::i16>(bindings.action.selection_word)
        ),
        static_cast<i32>(
            std::bit_cast<compat::i16>(bindings.action.selection_high_word)
        ),
        bindings.startup.mirror_mode == 0U ? 0U : 1U
    );
    if (result.status == LegacyBattleContextPromptStatus::completed &&
        result.return_value == 1U) {
        bindings.action.message_aux = 0U;
    }
    return result;
}

}  // namespace openswd3::battle
