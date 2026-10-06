#pragma once

#include "openswd3/battle/legacy_battle_script_dispatch.hpp"

#include "openswd3/battle/legacy_battle_group_b_action_reconfiguration.hpp"
#include "openswd3/battle/legacy_battle_script_curve.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>

namespace openswd3::battle::detail {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kLegacyBattleGroupASecondarySkipQueryToken = 0x0046E0A0U;
constexpr u32 kLegacyBattleScriptCoordinateXToken = 0x0053CCE8U;
constexpr u32 kLegacyBattleScriptCoordinateYToken = 0x0053CCECU;
constexpr u32 kLegacyBattleScriptPositionXToken = 0x0053CE74U;
constexpr u32 kLegacyBattleScriptPairXToken = 0x0053CE78U;
constexpr u32 kLegacyBattleScriptPairYToken = 0x0053CE7AU;
constexpr u32 kLegacyBattleScriptGroupAMirrorBaseToken = 0x004FF558U;
constexpr i32 kLegacyBattleScriptExtendedGroupBCleanupCount = 10;

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value & 0xFFFFU);
}

inline void store_word(
    std::array<compat::u8, kLegacyBattleScriptDynamicCommandSize>& bytes,
    const std::size_t offset,
    const u16 value
) noexcept {
    bytes[offset] = static_cast<compat::u8>(value & 0x00FFU);
    bytes[offset + 1U] = static_cast<compat::u8>(value >> 8U);
}

[[nodiscard]] constexpr u16 high_word(const u32 value) noexcept {
    return static_cast<u16>(value >> 16U);
}

constexpr void set_low_word(u32& destination, const u16 value) noexcept {
    destination = (destination & 0xFFFF0000U) | static_cast<u32>(value);
}

constexpr void set_high_word(u32& destination, const u16 value) noexcept {
    destination =
        (destination & 0x0000FFFFU) | (static_cast<u32>(value) << 16U);
}

[[nodiscard]] constexpr u32
with_low_word(const u32 value, const u16 low) noexcept {
    return (value & 0xFFFF0000U) | static_cast<u32>(low);
}

[[nodiscard]] constexpr i32 signed_word(const u16 value) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(value));
}

[[nodiscard]] constexpr u32
wrapping_add(const u32 left, const u32 right) noexcept {
    return left + right;
}

[[nodiscard]] constexpr bool has_even_parity(u32 value) noexcept {
    value &= 0xFFU;
    value ^= value >> 4U;
    value ^= value >> 2U;
    value ^= value >> 1U;
    return (value & 1U) == 0U;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_flags(const u32 left, const u32 right, const u32 sum) noexcept {
    return {
        .carry = sum < left,
        .parity = has_even_parity(sum),
        .auxiliary_carry = ((left ^ right ^ sum) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = sum == 0U,
        .sign = (sum & 0x80000000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ sum)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
add_word_flags(const u16 left, const u16 right, const u16 sum) noexcept {
    return {
        .carry = sum < left,
        .parity = has_even_parity(sum),
        .auxiliary_carry = ((left ^ right ^ sum) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = sum == 0U,
        .sign = (sum & 0x8000U) != 0U,
        .overflow = ((~(left ^ right) & (left ^ sum)) & 0x8000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
test_word_flags(const u16 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x8000U) != 0U,
        .overflow = false,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 difference = left - right;
    return {
        .carry = left < right,
        .parity = has_even_parity(difference),
        .auxiliary_carry = ((left ^ right ^ difference) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = difference == 0U,
        .sign = (difference & 0x80000000U) != 0U,
        .overflow =
            (((left ^ right) & (left ^ difference)) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
test_flags(const u32 value) noexcept {
    return {
        .carry = false,
        .parity = has_even_parity(value),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = false,
    };
}

struct ScriptActorAddress {
    u32 token{};
    u32 coordinate_eax{};
    u32 selected_call_eax{};
    u32 coordinate_edx{};
    LegacyBattleActorCoordinateFlags coordinate_flags{};
};

[[nodiscard]] constexpr ScriptActorAddress
script_actor_address(const u16 actor) noexcept {
    if (actor > 7U) {
        const u32 index = static_cast<u32>(actor) - 8U;
        const u32 times_sixty_three = (index << 6U) - index;
        const u32 times_one_thousand_eight = times_sixty_three << 4U;
        const u32 times_one_thousand_seven = times_one_thousand_eight - index;
        const u32 times_three_thousand_twenty_one =
            times_one_thousand_seven + times_one_thousand_seven * 2U;
        return {
            .token = kLegacyBattleScriptGroupABaseToken +
                times_three_thousand_twenty_one * 4U,
            .coordinate_eax = times_three_thousand_twenty_one,
            .selected_call_eax = times_one_thousand_seven,
            .coordinate_edx = actor,
            .coordinate_flags = subtract_flags(times_one_thousand_eight, index),
        };
    }

    const u32 index = actor;
    const u32 times_three = index + index * 2U;
    const u32 times_twenty_four = times_three << 3U;
    const u32 times_twenty_three = times_twenty_four - index;
    const u32 times_sixty_nine = times_twenty_three + times_twenty_three * 2U;
    const u32 times_three_hundred_forty_five =
        times_sixty_nine + times_sixty_nine * 4U;
    const u32 times_one_thousand_three_hundred_eighty_one =
        index + times_three_hundred_forty_five * 4U;
    return {
        .token = kLegacyBattleScriptGroupBBaseToken +
            times_one_thousand_three_hundred_eighty_one * 8U,
        .coordinate_eax = times_one_thousand_three_hundred_eighty_one,
        .selected_call_eax = actor,
        .coordinate_edx = times_three_hundred_forty_five,
        .coordinate_flags = subtract_flags(times_twenty_four, index),
    };
}

class ScriptRunner {
public:
    ScriptRunner(
        LegacyBattleScriptWorkspace& workspace,
        LegacyBattleScriptDispatchBindings bindings,
        LegacyBattleScriptDispatchPort& port,
        const LegacyBattleScriptDispatchRequest& request
    )
        : workspace_(workspace), bindings_(bindings), port_(port),
          request_(request),
          current_coordinate_access_(request.current_coordinate_access),
          live_count_control_(request.live_count_control),
          eax_(request.entry_eax), ecx_(request.entry_ecx),
          edx_(request.entry_edx), flags_(request.entry_flags),
          entry_ecx_(request.entry_ecx), entry_esi_(request.entry_esi),
          entry_edi_(request.entry_edi) {
        result_.cursor_before = workspace_.cursor;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult run() {
        u16 raw_opcode{};
        if (!read_u16(workspace_.cursor, raw_opcode)) {
            return finish();
        }
        const i32 opcode = signed_word(raw_opcode);
        result_.opcode = opcode;

        switch (opcode) {
        case -1:
            return case_terminal();
        case 0:
        case 7:
            return finish(1U);
        case 1:
            return case_one();
        case 2:
            return case_two();
        case 3:
            return case_three();
        case 4:
            return case_four();
        case 5:
            return case_five();
        case 6:
            return case_six();
        case 8:
            return case_eight();
        case 9:
            return case_nine();
        case 10:
            return case_ten();
        case 11:
            return case_eleven();
        case 12:
            return case_twelve();
        case 13:
            return case_thirteen();
        case 14:
            return case_fourteen();
        case 15:
            return case_fifteen();
        case 16:
            return case_sixteen();
        case 17:
            workspace_.cursor = wrapping_add(workspace_.cursor, 2U);
            return finish(1U);
        case 18:
            return case_eighteen();
        case 19:
            return case_nineteen();
        case 20:
            return case_twenty();
        case 21:
            return case_twenty_one();
        case 22:
            return case_twenty_two();
        case 23:
            return case_twenty_three();
        case 24:
            return case_twenty_four();
        case 25:
            return case_twenty_five();
        case 26:
            return case_twenty_six();
        case 27:
            return case_twenty_seven();
        case 28:
            return case_twenty_eight();
        case 29:
            return case_twenty_nine();
        case 30:
            return case_thirty();
        case 31:
            return case_thirty_one();
        case 32:
        case 38:
            return finish(1U);
        case 33:
            return case_thirty_three();
        case 34:
            return case_thirty_four();
        case 35:
            return case_thirty_five();
        case 36:
            return case_thirty_six();
        case 37:
            return case_thirty_seven();
        case 39:
            return case_thirty_nine();
        case 40:
            return case_forty();
        case 41:
            return case_forty_one();
        case 42:
            return case_forty_two();
        case 43:
            return case_forty_three();
        case 44:
            return case_forty_four();
        case 45:
            return case_forty_five();
        case 46:
            return case_forty_six();
        case 47:
            return case_forty_seven();
        case 48:
            return case_forty_eight();
        case 49:
            return case_forty_nine();
        case 50:
            return case_fifty();
        case 51:
            return case_fifty_one();
        case 52:
            return case_fifty_two();
        case 53:
            return case_fifty_three();
        case 54:
            return case_fifty_four();
        case 55:
            return case_fifty_five();
        case 56:
            return case_fifty_six();
        case 57:
            return case_fifty_seven();
        case 58:
            return case_fifty_eight();
        case 59:
            return case_fifty_nine();
        case 60:
            return case_sixty();
        case 61:
            return case_sixty_one();
        case 62:
            return case_sixty_two();
        case 63:
            return case_sixty_three();
        case 64:
            return case_sixty_four();
        case 65:
            return case_sixty_five();
        case 66:
            return case_sixty_six();
        case 67:
            return case_sixty_seven();
        case 68:
            return case_sixty_eight();
        case 69:
            return case_sixty_nine();
        case 70:
            return case_seventy();
        case 71:
            return case_seventy_one();
        case 72:
            return case_seventy_two();
        case 73:
            return case_seventy_three();
        case 74:
            return case_seventy_four();
        case 75:
            return case_seventy_five();
        case 76:
            return case_seventy_six();
        case 77:
            return case_seventy_seven();
        case 78:
            return case_seventy_eight();
        case 79:
            return case_seventy_nine();
        case 80:
            return case_eighty();
        case 81:
            return case_eighty_one();
        case 82:
            return case_eighty_two();
        case 83:
            return case_eighty_three();
        default:
            return finish(1U);
        }
    }

private:
    [[nodiscard]] LegacyBattleScriptDispatchResult
    finish(const u32 return_eax = 1U) {
        result_.return_eax = return_eax;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        result_.cursor_after = workspace_.cursor;
        return result_;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult
    stop(const LegacyBattleScriptDispatchStatus status, const u32 offset) {
        result_.status = status;
        result_.stopped_offset = offset;
        return finish(eax_);
    }

    [[nodiscard]] bool read_u8(const u32 offset, u8& value) {
        if (offset >= bindings_.assets.script_capacity ||
            offset >= bindings_.assets.script.size()) {
            result_.status =
                LegacyBattleScriptDispatchStatus::script_typed_stop;
            result_.stopped_offset = offset;
            return false;
        }
        value = bindings_.assets.script[offset];
        return true;
    }

    [[nodiscard]] bool read_u16(const u32 offset, u16& value) {
        u8 low{};
        if (!read_u8(offset, low)) {
            return false;
        }
        u8 high{};
        if (!read_u8(wrapping_add(offset, 1U), high)) {
            return false;
        }
        value = static_cast<u16>(
            static_cast<u16>(low) | (static_cast<u16>(high) << 8U)
        );
        return true;
    }

    [[nodiscard]] std::optional<u32> group_a_token(const i32 code) {
        const i32 index = code - 8;
        if (index < 0 || index >= 10) {
            result_.status =
                LegacyBattleScriptDispatchStatus::group_a_actor_typed_stop;
            result_.stopped_offset = workspace_.cursor;
            return std::nullopt;
        }
        return kLegacyBattleScriptGroupABaseToken +
            static_cast<u32>(index) * kLegacyBattleScriptGroupAElementSize;
    }

    [[nodiscard]] bool set_actor_availability_block(
        const i32 code, const u32 actor_token, const u32 value
    ) {
        const i32 signed_index = code - 8;
        auto* actor = signed_index >= 0 && signed_index < 10
            ? &bindings_.final_actor.group_a_availability_blocks
                   [static_cast<std::size_t>(signed_index)]
            : nullptr;
        ecx_ = actor_token;
        result_.actor_availability_block =
            set_legacy_battle_actor_availability_block(
                actor,
                {
                    .value = value,
                    .actor_token = ecx_,
                    .entry_eax = eax_,
                    .entry_edx = edx_,
                }
            );
        ++result_.actor_availability_block_calls;
        eax_ = result_.actor_availability_block.return_eax;
        ecx_ = result_.actor_availability_block.return_ecx;
        edx_ = result_.actor_availability_block.return_edx;
        if (result_.actor_availability_block.status ==
            LegacyBattleActorAvailabilityBlockStatus::completed) {
            return true;
        }
        result_.status = LegacyBattleScriptDispatchStatus::
            actor_availability_block_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    [[nodiscard]] std::optional<u32> group_b_token(const i32 code) {
        if (code < 0 || code >= 8) {
            result_.status =
                LegacyBattleScriptDispatchStatus::group_b_actor_typed_stop;
            result_.stopped_offset = workspace_.cursor;
            return std::nullopt;
        }
        return kLegacyBattleScriptGroupBBaseToken +
            static_cast<u32>(code) * kLegacyBattleScriptGroupBElementSize;
    }

    [[nodiscard]] std::optional<u32> actor_token(const i32 code) {
        return code > 7 ? group_a_token(code) : group_b_token(code);
    }

    [[nodiscard]] bool query_actor_current_coordinate_words(
        const u32 caller_address,
        const u32 actor_token,
        u16& output_x,
        u16& output_y,
        const u32 output_x_token,
        const u32 output_y_token,
        const u32 entry_eax,
        const u32 entry_edx,
        const LegacyBattleActorCoordinateFlags entry_flags
    ) {
        const u32 query_call = result_.current_coordinate_query_calls + 1U;
        const bool apply_access = current_coordinate_access_.query_call == 0U ||
            current_coordinate_access_.query_call == query_call;
        const LegacyBattleActorCurrentCoordinateQueryRequest request{
            .actor_token = actor_token,
            .output_x_token = output_x_token,
            .output_y_token = output_y_token,
            .entry_eax = entry_eax,
            .entry_edx = entry_edx,
            .entry_flags = entry_flags,
            .first_output_pointer_readable = !apply_access ||
                current_coordinate_access_.first_output_pointer_readable,
            .second_output_pointer_readable = !apply_access ||
                current_coordinate_access_.second_output_pointer_readable,
            .first_output_writable = !apply_access ||
                current_coordinate_access_.first_output_writable,
            .second_output_writable = !apply_access ||
                current_coordinate_access_.second_output_writable,
        };
        result_.current_coordinate_query =
            query_legacy_battle_actor_current_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    {.startup = &bindings_.startup}, actor_token
                ),
                &output_x,
                &output_y,
                request
            );
        ++result_.current_coordinate_query_calls;
        result_.current_coordinate_trace.push_back({
            .caller_address = caller_address,
            .request = request,
            .result = result_.current_coordinate_query,
        });
        eax_ = result_.current_coordinate_query.return_eax;
        ecx_ = result_.current_coordinate_query.return_ecx;
        edx_ = result_.current_coordinate_query.return_edx;
        flags_ = result_.current_coordinate_query.flags;
        if (result_.current_coordinate_query.status ==
            LegacyBattleActorCurrentCoordinateQueryStatus::completed) {
            return true;
        }
        result_.status = LegacyBattleScriptDispatchStatus::
            actor_current_coordinate_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    [[nodiscard]] bool query_actor_current_coordinate_dwords(
        const u32 caller_address,
        const u32 actor_token,
        i32& output_x,
        i32& output_y,
        const u32 entry_eax,
        const u32 entry_edx,
        const LegacyBattleActorCoordinateFlags entry_flags
    ) {
        u32 output_x_bits = std::bit_cast<u32>(output_x);
        u32 output_y_bits = std::bit_cast<u32>(output_y);
        u16 output_x_word = low_word(output_x_bits);
        u16 output_y_word = low_word(output_y_bits);
        const bool completed = query_actor_current_coordinate_words(
            caller_address,
            actor_token,
            output_x_word,
            output_y_word,
            kLegacyBattleScriptCoordinateXToken,
            kLegacyBattleScriptCoordinateYToken,
            entry_eax,
            entry_edx,
            entry_flags
        );
        if (result_.current_coordinate_query.output_writes >= 1U) {
            set_low_word(output_x_bits, output_x_word);
            output_x = std::bit_cast<i32>(output_x_bits);
        }
        if (result_.current_coordinate_query.output_writes >= 2U) {
            set_low_word(output_y_bits, output_y_word);
            output_y = std::bit_cast<i32>(output_y_bits);
        }
        return completed;
    }

    void publish_dynamic_text_coordinates() noexcept {
        auto& bytes = workspace_.dynamic_commands.back().bytes;
        store_word(bytes, 0x1EU, workspace_.pair_x);
        store_word(bytes, 0x20U, workspace_.pair_y);
    }

    [[nodiscard]] bool query_actor_coordinates(const u16 actor) {
        const auto address = script_actor_address(actor);
        eax_ = address.coordinate_eax;
        ecx_ = address.token;
        edx_ = address.coordinate_edx;
        result_.coordinate_query = query_legacy_battle_actor_coordinates(
            resolve_legacy_battle_actor_coordinates(
                {.startup = &bindings_.startup}, address.token
            ),
            &workspace_.pair_x,
            &workspace_.pair_y,
            {
                .actor_token = address.token,
                .output_x_token = kLegacyBattleScriptPairXToken,
                .output_y_token = kLegacyBattleScriptPairYToken,
                .entry_eax = eax_,
                .entry_edx = edx_,
                .entry_flags = address.coordinate_flags,
            }
        );
        ++result_.coordinate_query_calls;
        eax_ = result_.coordinate_query.return_eax;
        ecx_ = result_.coordinate_query.return_ecx;
        edx_ = result_.coordinate_query.return_edx;
        if (result_.coordinate_query.status ==
            LegacyBattleActorCoordinateQueryStatus::completed) {
            return true;
        }
        result_.status =
            LegacyBattleScriptDispatchStatus::actor_coordinate_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    [[nodiscard]] bool publish_actor_coordinates(
        const u32 actor_token,
        const u32 x_argument,
        const u32 y_argument,
        const u32 entry_eax,
        const u32 entry_edx,
        const u32 entry_esi,
        const u32 entry_edi,
        const LegacyBattleActorCoordinateFlags entry_flags
    ) {
        result_.coordinate_publication =
            publish_legacy_battle_actor_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    {.startup = &bindings_.startup}, actor_token
                ),
                x_argument,
                y_argument,
                {
                    .actor_token = actor_token,
                    .entry_eax = entry_eax,
                    .entry_ecx = actor_token,
                    .entry_edx = entry_edx,
                    .entry_esi = entry_esi,
                    .entry_edi = entry_edi,
                    .entry_flags = entry_flags,
                }
            );
        ++result_.coordinate_publication_calls;
        eax_ = result_.coordinate_publication.return_eax;
        ecx_ = result_.coordinate_publication.return_ecx;
        edx_ = result_.coordinate_publication.return_edx;
        if (result_.coordinate_publication.status ==
            LegacyBattleActorCoordinatePublicationStatus::completed) {
            if (live_count_control_.publication_call ==
                result_.coordinate_publication_calls) {
                bindings_.startup.actor_metrics.group_a_count =
                    live_count_control_.party_count_after_publication;
                bindings_.startup.actor_metrics.group_b_count =
                    live_count_control_.enemy_count_after_publication;
            }
            return true;
        }
        result_.status = LegacyBattleScriptDispatchStatus::
            actor_coordinate_publication_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    void prepare_selected_actor_cleanup_call(const u16 actor) noexcept {
        const auto address = script_actor_address(actor);
        eax_ = address.selected_call_eax;
        ecx_ = address.token;
        if (actor <= 7U) {
            edx_ = address.coordinate_eax;
        }
    }

    [[nodiscard]] bool query_actor_base_coordinates(const u16 actor) {
        const auto address = script_actor_address(actor);
        eax_ = address.selected_call_eax;
        ecx_ = address.token;
        if (actor <= 7U) {
            edx_ = address.coordinate_eax;
        }
        result_.base_coordinate_query =
            query_legacy_battle_actor_base_coordinates(
                resolve_legacy_battle_actor_coordinates(
                    {.startup = &bindings_.startup}, address.token
                ),
                &workspace_.position_x,
                &workspace_.pair_y,
                {
                    .actor_token = address.token,
                    .output_x_token = kLegacyBattleScriptPositionXToken,
                    .output_y_token = kLegacyBattleScriptPairYToken,
                    .entry_eax = eax_,
                    .entry_edx = edx_,
                    .entry_flags = address.coordinate_flags,
                }
            );
        ++result_.base_coordinate_query_calls;
        eax_ = result_.base_coordinate_query.return_eax;
        ecx_ = result_.base_coordinate_query.return_ecx;
        edx_ = result_.base_coordinate_query.return_edx;
        if (result_.base_coordinate_query.status ==
            LegacyBattleActorBaseCoordinateQueryStatus::completed) {
            return true;
        }
        result_.status =
            LegacyBattleScriptDispatchStatus::actor_base_coordinate_typed_stop;
        result_.stopped_offset = workspace_.cursor;
        return false;
    }

    [[nodiscard]] bool initialize_dynamic_text_actor_group(
        const u16 actor, const bool group_b_selected_cleanup
    ) {
        const auto address = script_actor_address(actor);
        if (actor > 7U) {
            for (i32 index = 0; index < 10; ++index) {
                const auto current = group_a_token(index + 8);
                if (!current.has_value()) {
                    return false;
                }
                ecx_ = *current;
                if (!invoke(
                        LegacyBattleScriptDispatchCall::pending_47c660,
                        *current,
                        {0U}
                    )) {
                    return false;
                }
            }
            prepare_selected_actor_cleanup_call(actor);
            return invoke(
                LegacyBattleScriptDispatchCall::pending_47d900,
                address.token,
                {0x24U}
            );
        }

        if (group_b_selected_cleanup) {
            prepare_selected_actor_cleanup_call(actor);
            if (!invoke(
                    LegacyBattleScriptDispatchCall::pending_47c660,
                    address.token,
                    {1U}
                )) {
                return false;
            }
        }
        for (i32 index = 0; index < 8; ++index) {
            const auto current = group_b_token(index);
            if (!current.has_value()) {
                return false;
            }
            ecx_ = *current;
            if (!invoke(
                    LegacyBattleScriptDispatchCall::pending_47c660,
                    *current,
                    {0U}
                )) {
                return false;
            }
        }
        return true;
    }

    bool set_actor_action_mode(
        const u32 actor_token,
        const u32 mode,
        const u32 entry_eax,
        const u32 entry_edx,
        const u32 return_address,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        auto request =
            request_
                .actor_action_mode_requests[result_.actor_action_mode_calls];
        request.actor_token = actor_token;
        request.mode = mode;
        request.entry_eax = entry_eax;
        request.entry_edx = entry_edx;
        request.entry_return_address = return_address;
        request.entry_flags = entry_flags;
        request.entry_flags_known = true;
        result_.actor_action_modes[result_.actor_action_mode_calls] =
            set_legacy_battle_actor_action_mode(
                resolve_legacy_battle_actor_action_mode(
                    {
                        .action = &bindings_.action,
                        .startup = &bindings_.startup,
                    },
                    actor_token
                ),
                request
            );
        const auto& mode_result =
            result_.actor_action_modes[result_.actor_action_mode_calls];
        result_.actor_action_mode = mode_result;
        ++result_.actor_action_mode_calls;
        eax_ = mode_result.return_eax;
        ecx_ = mode_result.return_ecx;
        edx_ = mode_result.return_edx;
        flags_ = mode_result.flags;
        if (mode_result.status !=
            LegacyBattleActorActionModeStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::actor_action_mode_typed_stop;
            return false;
        }
        return true;
    }

    void record_nested_actor_action_mode(
        const LegacyBattleActorActionModeResult& nested
    ) {
        result_.actor_action_mode = nested;
        result_.actor_action_modes[result_.actor_action_mode_calls] = nested;
        ++result_.actor_action_mode_calls;
        flags_ = nested.flags;
    }

    bool invoke(
        const LegacyBattleScriptDispatchCall call_kind,
        const u32 object_token = 0U,
        const std::initializer_list<u32> arguments = {}
    ) {
        if (call_kind == LegacyBattleScriptDispatchCall::script_page_load) {
            workspace_.cursor = 0U;
        }
        LegacyBattleScriptDispatchCallRequest request{
            .call = call_kind,
            .object_token = object_token,
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
            .flags = flags_,
            .cursor = workspace_.cursor,
        };
        request.argument_count = static_cast<u32>(arguments.size());
        std::size_t index = 0U;
        for (const u32 argument : arguments) {
            if (index < request.arguments.size()) {
                request.arguments[index] = argument;
            }
            ++index;
        }
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, request);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        flags_ = reply.flags;
        if (reply.typed_stop) {
            if (call_kind == LegacyBattleScriptDispatchCall::frame) {
                result_.status =
                    LegacyBattleScriptDispatchStatus::frame_typed_stop;
                result_.stopped_offset = request.cursor;
            } else {
                result_.status = LegacyBattleScriptDispatchStatus::
                    script_page_load_typed_stop;
            }

            return false;
        }

        return true;
    }

    [[nodiscard]] LegacyBattleGroupBActionCompositionCallReply
    invoke_group_b_action_composition_callee(
        const LegacyBattleGroupBActionCompositionCallRequest& request
    ) {
        switch (request.call) {
        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_resource_definition:

        case LegacyBattleGroupBActionCompositionCall::
            reserved_load_action_profile:
            return {
                .eax = 0U,
                .ecx = 0U,
                .edx = 0U,
                .typed_stop = true,
                .resource_definition = nullptr,
                .profile_buffer = nullptr,
            };

        case LegacyBattleGroupBActionCompositionCall::copy_action_text:
            break;
        }

        constexpr auto call_kind =
            LegacyBattleScriptDispatchCall::legacy_string_copy;

        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = request.ecx,
            .argument_count = 2U,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        call.arguments[0U] = request.arguments[0U];
        call.arguments[1U] = request.arguments[1U];
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.typed_stop,
            .resource_definition = nullptr,
            .profile_buffer = nullptr,
        };
    }

    class ScriptGroupBActionCompositionPort final
        : public LegacyBattleGroupBActionCompositionPort {
    public:
        explicit ScriptGroupBActionCompositionPort(
            ScriptRunner& runner
        ) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattleGroupBActionCompositionCallReply invoke(
            const LegacyBattleGroupBActionCompositionCallRequest& request
        ) override {
            return runner_.invoke_group_b_action_composition_callee(request);
        }

    private:
        ScriptRunner& runner_;
    };

    [[nodiscard]] LegacyBattleMonDefinitionTextReleaseCallReply
    invoke_pending_definition_text_release(
        const LegacyBattleMonDefinitionTextReleaseCallRequest& request
    ) {
        constexpr auto call_kind =
            LegacyBattleScriptDispatchCall::pending_478220;
        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = request.block_token,
            .argument_count = 1U,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        call.arguments[0U] = request.block_token;
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .typed_stop = reply.typed_stop,
        };
    }

    class ScriptGroupBActionReconfigurationPort final
        : public LegacyBattleMonDatabasePort,
          public LegacyBattleGroupBActionReconfigurationReleasePort {
    public:
        explicit ScriptGroupBActionReconfigurationPort(
            ScriptRunner& runner
        ) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattleMonDatabaseState&
        legacy_battle_mon_database_state() noexcept override {
            return runner_.port_.legacy_battle_mon_database_state();
        }

        [[nodiscard]] LegacyBattleMonProfile&
        legacy_battle_mon_profile_scratch() noexcept override {
            return runner_.port_.legacy_battle_mon_profile_scratch();
        }

        [[nodiscard]] std::array<u8, kLegacyBattleMonDefinitionScratchBytes>&
        legacy_battle_mon_definition_scratch() noexcept override {
            return runner_.port_.legacy_battle_mon_definition_scratch();
        }

        [[nodiscard]] LegacyBattleMonText&
        legacy_battle_mon_definition_scratch_description() noexcept override {
            return runner_.port_
                .legacy_battle_mon_definition_scratch_description();
        }

        [[nodiscard]] LegacyBattleMonDatabaseCallReply
        invoke_legacy_battle_mon_database(
            const LegacyBattleMonDatabaseCallRequest& request,
            const std::span<u8> destination
        ) override {
            return runner_.port_.invoke_legacy_battle_mon_database(
                request, destination
            );
        }

        [[nodiscard]] LegacyBattleMonDefinitionTextReleaseResult
        release_group_b_action_resource_text(
            const std::span<u8>,
            LegacyBattleMonText&,
            LegacyBattleMonDatabasePort&,
            const LegacyBattleMonDefinitionTextReleaseRequest& request
        ) override {
            const auto reply = runner_.invoke_pending_definition_text_release({
                .block_token = request.object_token,
                .eax = request.entry_eax,
                .ecx = request.entry_ecx,
                .edx = request.entry_edx,
            });
            return {
                .status = reply.typed_stop
                    ? LegacyBattleMonDefinitionTextReleaseStatus::
                          release_call_typed_stop
                    : LegacyBattleMonDefinitionTextReleaseStatus::completed,
                .stopped_token = reply.typed_stop ? request.object_token : 0U,
                .return_eax = reply.eax,
                .return_ecx = reply.ecx,
                .return_edx = reply.edx,
            };
        }

    private:
        ScriptRunner& runner_;
    };

    [[nodiscard]] LegacyBattlePartyItemDefinitionCallReply
    invoke_party_item_definition_callee(
        const LegacyBattlePartyItemDefinitionCallRequest& request
    ) {
        LegacyBattleScriptDispatchCall call_kind{};
        u32 object_token{};
        std::array<u32, 4U> arguments{};
        u32 argument_count{};
        switch (request.call) {
        case LegacyBattlePartyItemDefinitionCall::report_zero_item:
            call_kind = LegacyBattleScriptDispatchCall::message_box;
            object_token = request.window_token;
            arguments = {
                request.text_token,
                request.flags,
                request.source_file_token,
                request.source_line,
            };
            argument_count = 4U;
            break;

        case LegacyBattlePartyItemDefinitionCall::allocate_item_node:
            call_kind = LegacyBattleScriptDispatchCall::allocate;
            arguments[0U] = request.allocation_size;
            argument_count = 1U;
            break;

        case LegacyBattlePartyItemDefinitionCall::copy_caption:
            call_kind = LegacyBattleScriptDispatchCall::legacy_string_copy;
            object_token = request.destination_token;
            arguments[0U] = request.destination_token;
            arguments[1U] = request.source_token;
            argument_count = 2U;
            break;
        }

        LegacyBattleScriptDispatchCallRequest call{
            .call = call_kind,
            .object_token = object_token,
            .argument_count = argument_count,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
            .cursor = workspace_.cursor,
        };
        std::copy_n(arguments.begin(), argument_count, call.arguments.begin());
        result_.call_trace.push_back(call_kind);
        ++result_.port_calls;
        const auto reply =
            port_.invoke_battle_script(workspace_, bindings_, call);
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
        return {
            .eax = reply.eax,
            .ecx = reply.ecx,
            .edx = reply.edx,
            .allocation_accessible_bytes = world_map::kLegacyWorldItemNodeBytes,
            .typed_stop = reply.typed_stop,
        };
    }

    class ScriptPartyItemDefinitionPort final
        : public LegacyBattlePartyItemDefinitionPort {
    public:
        explicit ScriptPartyItemDefinitionPort(ScriptRunner& runner) noexcept
            : runner_(runner) {}

        [[nodiscard]] LegacyBattlePartyItemDefinitionCallReply invoke(
            const LegacyBattlePartyItemDefinitionCallRequest& request
        ) override {
            return runner_.invoke_party_item_definition_callee(request);
        }

    private:
        ScriptRunner& runner_;
    };

    [[nodiscard]] bool run_frame() {
        return invoke(LegacyBattleScriptDispatchCall::frame);
    }

    [[nodiscard]] bool toggle_actor_binary_state(
        const u32 actor_token,
        const u32 entry_eax,
        const u32 entry_edx,
        const u32 call_address,
        const u32 return_address,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        if (!execute_legacy_battle_actor_binary_state_toggle_call(
                {.startup = &bindings_.startup},
                result_.actor_binary_state_toggle,
                request_.actor_binary_state_toggle_requests,
                actor_token,
                1U,
                entry_eax,
                entry_edx,
                call_address,
                return_address,
                entry_flags,
                true
            )) {
            eax_ = result_.actor_binary_state_toggle.last.return_eax;
            ecx_ = result_.actor_binary_state_toggle.last.return_ecx;
            edx_ = result_.actor_binary_state_toggle.last.return_edx;
            if (result_.actor_binary_state_toggle.last.flags_known) {
                flags_ = result_.actor_binary_state_toggle.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_binary_state_toggle_typed_stop;
            return false;
        }

        eax_ = result_.actor_binary_state_toggle.last.return_eax;
        ecx_ = result_.actor_binary_state_toggle.last.return_ecx;
        edx_ = result_.actor_binary_state_toggle.last.return_edx;
        flags_ = result_.actor_binary_state_toggle.last.flags;
        return true;
    }

    [[nodiscard]] bool activate_actor_presentation(
        const u32 actor_token,
        const u32 entry_eax,
        const u32 entry_edx,
        const u32 call_address,
        const u32 return_address,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        if (!execute_legacy_battle_actor_presentation_activation_call(
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                result_.actor_presentation_activation,
                request_.actor_presentation_activation_requests,
                actor_token,
                1U,
                entry_eax,
                entry_edx,
                call_address,
                return_address,
                entry_flags
            )) {
            eax_ = result_.actor_presentation_activation.last.return_eax;
            ecx_ = result_.actor_presentation_activation.last.return_ecx;
            edx_ = result_.actor_presentation_activation.last.return_edx;
            if (result_.actor_presentation_activation.last.flags_known) {
                flags_ = result_.actor_presentation_activation.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_presentation_activation_typed_stop;
            return false;
        }
        eax_ = result_.actor_presentation_activation.last.return_eax;
        ecx_ = result_.actor_presentation_activation.last.return_ecx;
        edx_ = result_.actor_presentation_activation.last.return_edx;
        flags_ = result_.actor_presentation_activation.last.flags;
        return true;
    }

    [[nodiscard]] bool select_actor_target(
        const u32 actor_token,
        const u16 argument_value,
        const u32 entry_eax,
        const u32 entry_edx,
        const u32 call_address,
        const u32 return_address,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        if (!execute_legacy_battle_actor_target_selection_call(
                result_.actor_target_selection,
                request_.actor_target_selection_requests,
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                call_address,
                return_address,
                actor_token,
                argument_value,
                entry_eax,
                entry_edx,
                entry_flags,
                true
            )) {
            eax_ = result_.actor_target_selection.last.return_eax;
            ecx_ = result_.actor_target_selection.last.return_ecx;
            edx_ = result_.actor_target_selection.last.return_edx;
            if (result_.actor_target_selection.last.flags_known) {
                flags_ = result_.actor_target_selection.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_target_selection_typed_stop;
            return false;
        }

        eax_ = result_.actor_target_selection.last.return_eax;
        ecx_ = result_.actor_target_selection.last.return_ecx;
        edx_ = result_.actor_target_selection.last.return_edx;
        flags_ = result_.actor_target_selection.last.flags;
        return true;
    }

    [[nodiscard]] bool query_actor_target_selection_count(
        const u32 actor_token,
        const u32 entry_eax,
        const u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        if (!execute_legacy_battle_actor_target_selection_count_query_call(
                result_.actor_target_selection_count_query,
                request_.actor_target_selection_count_query_requests,
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                0x0046D429U,
                0x0046D42EU,
                actor_token,
                entry_eax,
                entry_edx,
                entry_flags,
                true
            )) {
            eax_ = result_.actor_target_selection_count_query.last.return_eax;
            ecx_ = result_.actor_target_selection_count_query.last.return_ecx;
            edx_ = result_.actor_target_selection_count_query.last.return_edx;
            if (result_.actor_target_selection_count_query.last.flags_known) {
                flags_ = result_.actor_target_selection_count_query.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_target_selection_count_query_typed_stop;
            return false;
        }

        eax_ = result_.actor_target_selection_count_query.last.return_eax;
        ecx_ = result_.actor_target_selection_count_query.last.return_ecx;
        edx_ = result_.actor_target_selection_count_query.last.return_edx;
        flags_ = result_.actor_target_selection_count_query.last.flags;
        return true;
    }

    [[nodiscard]] bool increment_actor_start_gate(
        const u32 actor_token,
        const u32 entry_eax,
        const u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) {
        if (!execute_legacy_battle_actor_start_gate_increment_call(
                result_.actor_start_gate_increment,
                request_.actor_start_gate_increment_requests,
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                0x0046DD4AU,
                0x0046DD4FU,
                actor_token,
                entry_eax,
                entry_edx,
                entry_flags,
                true
            )) {
            eax_ = result_.actor_start_gate_increment.last.return_eax;
            ecx_ = result_.actor_start_gate_increment.last.return_ecx;
            edx_ = result_.actor_start_gate_increment.last.return_edx;
            if (result_.actor_start_gate_increment.last.flags_known) {
                flags_ = result_.actor_start_gate_increment.last.flags;
            }
            result_.status = LegacyBattleScriptDispatchStatus::
                actor_start_gate_increment_typed_stop;
            return false;
        }

        eax_ = result_.actor_start_gate_increment.last.return_eax;
        ecx_ = result_.actor_start_gate_increment.last.return_ecx;
        edx_ = result_.actor_start_gate_increment.last.return_edx;
        flags_ = result_.actor_start_gate_increment.last.flags;
        return true;
    }

    [[nodiscard]] bool rebuild_actor_order_direct() {
        const auto order = rebuild_legacy_battle_actor_order(
            bindings_.metrics,
            bindings_.startup.actor_metrics.group_b_count,
            bindings_.startup.actor_metrics.group_a_count,
            edx_
        );
        eax_ = order.return_value;
        ecx_ = order.final_ecx;
        edx_ = order.final_edx;
        if (order.status != LegacyBattleActorOrderStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
            return false;
        }
        const auto group_b =
            rebuild_legacy_battle_group_b_order(bindings_.metrics);
        eax_ = group_b.return_value;
        ecx_ = group_b.final_ecx;
        edx_ = group_b.final_edx;
        if (group_b.status != LegacyBattleGroupBOrderStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::closed_callee_typed_stop;
            return false;
        }
        return true;
    }

    [[nodiscard]] bool insert_attack_order_direct(
        const u32 type, const u32 value, const u32 position
    ) {
        auto inserted = insert_legacy_battle_attack_order_entry(
            {
                .records = bindings_.startup.reset.records_524788,
                .party_source_words = bindings_.startup.reset.block_520e90,
                .primary_gate = &bindings_.shared.attack_order_primary_gate,
                .secondary_gate = &bindings_.shared.attack_order_secondary_gate,
            },
            type,
            value,
            position
        );
        eax_ = inserted.return_eax;
        ecx_ = inserted.return_ecx;
        edx_ = inserted.return_edx;
        if (inserted.status != LegacyBattleAttackOrderInsertStatus::completed) {
            result_.status =
                LegacyBattleScriptDispatchStatus::attack_order_typed_stop;
            return false;
        }
        return true;
    }

    void shutdown_script_direct() {
        const bool had_script_allocation =
            bindings_.assets.script_capacity != 0U;
        bindings_.shared.frame_gate = 1U;
        bindings_.shared.script_completion_gate = 1U;
        bindings_.shared.shutdown_values.fill(0U);
        workspace_.value_b = 0;
        workspace_.value_c = 0;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.position_x = 0U;
        workspace_.position_y = 0U;
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        workspace_.packed_actor_state = 0U;
        workspace_.waiting_argument = 0U;
        set_low_word(workspace_.waiting_state, 0U);
        workspace_.packed_value_a = 0U;
        workspace_.packed_value_b = 0U;
        workspace_.word_a = 0U;
        workspace_.word_b = 0U;
        workspace_.word_c = 0U;
        workspace_.word_d = 0U;
        bindings_.shared.frame_value = 0xFFFFU;
        workspace_.list_count = 0U;
        workspace_.dynamic_wait_state = 0U;
        bindings_.assets.figtalk_page_offset = 0U;
        workspace_.shutdown_auxiliary = 0U;
        if (had_script_allocation) {
            bindings_.assets.figtalk_actual_size = 0U;
            bindings_.assets.script_capacity = 0U;
            workspace_.cursor = 0U;
        }
        eax_ = 0U;
    }

    void cleanup_all_actors() {
        i32 index = 0;
        while (index < static_cast<i32>(
                           bindings_.startup.actor_metrics.group_b_count)) {
            const auto token = group_b_token(index);
            if (!token.has_value()) {
                return;
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47d350, *token);
            reset_legacy_battle_actor_effect_resource_slots(
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                *token
            );
            ++index;
        }
        index = 0;
        while (index < static_cast<i32>(
                           bindings_.startup.actor_metrics.group_a_count)) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return;
            }
            invoke(LegacyBattleScriptDispatchCall::pending_47d350, *token);
            reset_legacy_battle_actor_effect_resource_slots(
                {
                    .action = &bindings_.action,
                    .startup = &bindings_.startup,
                },
                *token
            );
            ++index;
        }
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_terminal();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_ten();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eleven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twelve();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fourteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_nineteen();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty();

    [[nodiscard]] LegacyBattleScriptDispatchResult run_action_case(
        const u32 action_code,
        const bool insert_group_a_attack,
        const bool completion_runs_frame
    ) {
        u16 state = high_word(workspace_.packed_actor_state);
        if ((state & 0x8000U) == 0U) {
            u16 actor{};
            u16 argument{};
            if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor) ||
                !read_u16(wrapping_add(workspace_.cursor, 4U), argument)) {
                return finish();
            }
            set_high_word(workspace_.packed_actor_state, actor);
            const i32 code = signed_word(actor);
            const auto token = actor_token(code);
            if (!token.has_value()) {
                return finish(eax_);
            }
            if (code > 7) {
                bindings_.final_actor.queued_actor_code =
                    static_cast<u32>(code);
                const i32 signed_index = code - 8;
                if (signed_index < 0 || signed_index >= 10) {
                    return stop(
                        LegacyBattleScriptDispatchStatus::
                            shared_state_typed_stop,
                        static_cast<u32>(signed_index)
                    );
                }
                const u32 index = static_cast<u32>(signed_index);
                if (action_code == 12U &&
                    !select_actor_target(
                        *token,
                        0U,
                        index * 1007U,
                        index * 3021U,
                        0x0046B850U,
                        0x0046B855U,
                        subtract_flags(index * 1008U, index)
                    )) {
                    return finish(eax_);
                }
                if (!set_actor_action_mode(
                        *token,
                        action_code,
                        index * 3021U,
                        static_cast<u32>(code - 7),
                        action_code == 11U ? 0x0046B6B7U : 0x0046B87EU,
                        subtract_flags(index * 1008U, index)
                    )) {
                    return finish(eax_);
                }
                if (action_code == 11U) {
                    bindings_.startup.reset
                        .block_520e90[static_cast<std::size_t>(index)] = 1U;
                }
            } else {
                const u32 index = static_cast<u32>(code);
                if (!select_actor_target(
                        *token,
                        action_code == 11U ? actor : 0U,
                        index,
                        index * 1381U,
                        action_code == 11U ? 0x0046B733U : 0x0046B8D4U,
                        action_code == 11U ? 0x0046B738U : 0x0046B8D9U,
                        subtract_flags(index * 24U, index)
                    )) {
                    return finish(eax_);
                }
                if (!set_actor_action_mode(
                        *token,
                        action_code,
                        index,
                        index * 1381U,
                        action_code == 11U ? 0x0046B75FU : 0x0046B900U,
                        subtract_flags(index * 24U, index)
                    )) {
                    return finish(eax_);
                }
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47d860,
                *token,
                {argument}
            );
            if (code > 7 && insert_group_a_attack &&
                !insert_attack_order_direct(1U, std::bit_cast<u32>(code), 0U)) {
                return finish(eax_);
            }
            set_high_word(
                workspace_.packed_actor_state, static_cast<u16>(actor | 0x8000U)
            );
            bindings_.action.action_pending_aux = 1U;
            bindings_.shared.action_completion_gate = 0U;
            bindings_.shared.frame_gate = 1U;
            if (!run_frame()) {
                return finish(eax_);
            }

            return finish(1U);
        }

        if (bindings_.shared.action_completion_gate != 1U) {
            bindings_.shared.frame_gate = 1U;
            if (!run_frame()) {
                return finish(eax_);
            }

            return finish(1U);
        }

        workspace_.cursor = wrapping_add(workspace_.cursor, 6U);
        bindings_.shared.frame_gate = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        bindings_.action.action_pending_aux = 0U;
        if (completion_runs_frame && !run_frame()) {
            return finish(eax_);
        }

        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_twenty_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_three();

    [[nodiscard]] LegacyBattleScriptPanelNode* panel_node(const u32 token) {
        for (auto& node : workspace_.panel_nodes) {
            if (node.token == token) {
                return &node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_five();

    [[nodiscard]] LegacyBattleScriptEffectNode* effect_node(const u32 token) {
        for (auto& node : workspace_.effect_nodes) {
            if (node.token == token) {
                return &node;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_thirty_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty();

    [[nodiscard]] bool
    scan_percent_q(const u32 start, const u32 limit, u32& length) {
        length = 0U;
        while (length < limit) {
            u8 first{};
            if (!read_u8(wrapping_add(start, length), first)) {
                return false;
            }
            if (first == 0x25U) {
                u8 second{};
                if (!read_u8(wrapping_add(start, length + 1U), second)) {
                    return false;
                }
                if (second == 0x51U) {
                    return true;
                }
            }
            ++length;
        }
        return true;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult create_dynamic_text(
        const bool anchored_variant, const bool group_b_target_cleanup
    ) {
        u16 actor{};
        if (!read_u16(wrapping_add(workspace_.cursor, 2U), actor)) {
            return finish();
        }
        workspace_.text_offset = wrapping_add(workspace_.cursor, 2U);
        invoke(
            LegacyBattleScriptDispatchCall::allocate,
            0U,
            {kLegacyBattleScriptDynamicCommandSize}
        );
        workspace_.dynamic_command_token = eax_;
        if (eax_ == 0U) {
            return stop(
                LegacyBattleScriptDispatchStatus::allocation_typed_stop, 0U
            );
        }
        workspace_.dynamic_commands.push_back({.token = eax_});

        if (!anchored_variant) {
            invoke(
                LegacyBattleScriptDispatchCall::format_dynamic_text,
                workspace_.dynamic_command_token,
                {workspace_.text_offset, bindings_.shared.frame_value, actor}
            );
            invoke(
                LegacyBattleScriptDispatchCall::finalize_dynamic_text,
                workspace_.dynamic_command_token
            );
        }

        if (!query_actor_coordinates(actor)) {
            return finish(eax_);
        }
        if (anchored_variant && !query_actor_base_coordinates(actor)) {
            return finish(eax_);
        }
        if (!initialize_dynamic_text_actor_group(
                actor, group_b_target_cleanup
            )) {
            return finish(eax_);
        }

        if (anchored_variant) {
            invoke(
                LegacyBattleScriptDispatchCall::format_dynamic_text,
                workspace_.dynamic_command_token,
                {workspace_.text_offset,
                 0x00010000U,
                 std::bit_cast<u32>(signed_word(workspace_.pair_x)),
                 std::bit_cast<u32>(signed_word(workspace_.pair_y))}
            );
            invoke(
                LegacyBattleScriptDispatchCall::finalize_dynamic_text,
                workspace_.dynamic_command_token
            );
        }
        publish_dynamic_text_coordinates();
        bindings_.message_state = 0U;
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult finish_dynamic_text() {
        for (i32 index = 0; index < 10; ++index) {
            const auto token = group_a_token(index + 8);
            if (!token.has_value()) {
                return finish(eax_);
            }
            invoke(
                LegacyBattleScriptDispatchCall::pending_47c660, *token, {0U}
            );
        }
        for (i32 index = 0;
             index < kLegacyBattleScriptExtendedGroupBCleanupCount;
             ++index) {
            const u32 token = kLegacyBattleScriptGroupBBaseToken +
                static_cast<u32>(index) * kLegacyBattleScriptGroupBElementSize;
            invoke(LegacyBattleScriptDispatchCall::pending_47c660, token, {0U});
        }
        u32 length{};
        if (!scan_percent_q(
                workspace_.text_offset, kLegacyBattleScriptTextScanLimit, length
            )) {
            return finish();
        }
        workspace_.cursor = wrapping_add(workspace_.text_offset, length + 2U);
        workspace_.text_offset = workspace_.cursor;
        workspace_.short_text.fill(0U);
        bindings_.shared.frame_gate = 1U;
        bindings_.action.action_pending_aux = 0U;
        workspace_.coordinate_x = 0;
        workspace_.coordinate_y = 0;
        workspace_.pair_x = 0U;
        workspace_.pair_y = 0U;
        set_high_word(workspace_.packed_actor_state, 0U);
        return finish(1U);
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_forty_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_fifty_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_four();

    [[nodiscard]] LegacyBattleScriptPlayerItemQuantity*
    player_item(const u32 token) {
        for (auto& item : bindings_.shared.player_items) {
            if (item.token == token) {
                return &item;
            }
        }
        return nullptr;
    }

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_sixty_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_three();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_four();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_five();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_six();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_seven();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_eight();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_seventy_nine();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_one();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_two();

    [[nodiscard]] LegacyBattleScriptDispatchResult case_eighty_three();

    LegacyBattleScriptWorkspace& workspace_;
    LegacyBattleScriptDispatchBindings bindings_;
    LegacyBattleScriptDispatchPort& port_;
    LegacyBattleScriptDispatchRequest request_{};
    LegacyBattleScriptCurrentCoordinateAccess current_coordinate_access_{};
    LegacyBattleScriptLiveCountControl live_count_control_{};
    LegacyBattleScriptDispatchResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    LegacyBattleActorCoordinateFlags flags_{};
    u32 entry_ecx_{};
    u32 entry_esi_{};
    u32 entry_edi_{};
};

}  // namespace openswd3::battle::detail
