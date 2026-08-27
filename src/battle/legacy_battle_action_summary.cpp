#include "openswd3/battle/legacy_battle_action_summary.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;
using Call = LegacyBattleActionSummaryCall;
using Status = LegacyBattleActionSummaryStatus;

inline constexpr u32 kFontToken = 0x004AB998U;
inline constexpr u32 kTextSurfaceToken = 0x004CD76CU;
inline constexpr u32 kGroupABaseToken = 0x005029D0U;
inline constexpr u32 kGroupACount = 10U;
inline constexpr u32 kActionCodeBaseToken = 0x004FE5CCU;
inline constexpr u32 kDynamicPermissionBaseToken = 0x00524419U;

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u32 low_word) noexcept {
    return (value & 0xFFFF0000U) | (low_word & 0xFFFFU);
}

[[nodiscard]] u32 read_permission(
    const LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    if (index == 0U) {
        return reset.value_524413;
    }
    const u32 shift = ((index - 1U) & 3U) * 8U;
    const u32 word = index <= 4U ? reset.value_524414 : reset.value_524418;
    return word >> shift & 0xFFU;
}

void write_permission(
    LegacyBattleStartupResetBlocks& reset, const u32 index, const u32 value
) noexcept {
    if (index == 0U) {
        reset.value_524413 = static_cast<compat::u8>(value);
        return;
    }
    u32& word = index <= 4U ? reset.value_524414 : reset.value_524418;
    const u32 shift = ((index - 1U) & 3U) * 8U;
    word = (word & ~(0xFFU << shift)) | ((value & 0xFFU) << shift);
}

[[nodiscard]] u32 read_action_token(
    const LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    if (index == 0U) {
        return reset.value_4ff0b0;
    }
    if (index == 1U) {
        return reset.value_4ff0b4;
    }
    return reset.value_4ff0b8;
}

[[nodiscard]] u16 read_action_code(
    const LegacyBattleStartupResetBlocks& reset, const u32 index
) noexcept {
    if (index == 0U) {
        return static_cast<u16>(reset.value_4fe5cc);
    }
    if (index == 1U) {
        return static_cast<u16>(reset.value_4fe5cc >> 16U);
    }
    return reset.value_4fe5d0;
}

[[nodiscard]] constexpr u32 actor_offset(const u32 index) noexcept {
    u32 value = index;
    value <<= 6U;
    value -= index;
    value <<= 4U;
    value -= index;
    value = value + value * 2U;
    value <<= 2U;
    return value;
}

class ActionModePortAdapter final : public LegacyBattleInputDispatchPort {
public:
    ActionModePortAdapter(
        LegacyBattleActionSummaryPort& port,
        LegacyBattleActionSummaryResult& result
    ) noexcept
        : port_(port), result_(result) {}

    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        Call call = Call::action_mode_query_primary_actor;
        switch (request.call) {
        case LegacyBattleInputDispatchCall::action_mode_query_primary_actor:
            break;
        case LegacyBattleInputDispatchCall::action_mode_query_secondary_actor:
            call = Call::action_mode_query_secondary_actor;
            break;
        case LegacyBattleInputDispatchCall::action_mode_query_active_actor:
            call = Call::action_mode_query_active_actor;
            break;
        default:
            break;
        }
        ++result_.port_calls;
        const auto reply = port_.invoke_action_summary({
            .call = call,
            .object_token = request.ecx,
            .eax = request.eax,
            .ecx = request.ecx,
            .edx = request.edx,
        });
        return {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx};
    }

private:
    LegacyBattleActionSummaryPort& port_;
    LegacyBattleActionSummaryResult& result_;
};

}  // namespace

LegacyBattleActionSummaryResult draw_legacy_battle_action_summary(
    const LegacyBattleActionSummaryBindings bindings,
    LegacyBattleActionSummaryPort& port,
    const LegacyBattleActionSummaryRequest& request
) {
    LegacyBattleActionSummaryResult result;
    auto& startup = bindings.startup;
    auto& reset = startup.reset;
    u32 eax = bindings.final_actor.queued_actor_code;
    u32 ecx = request.entry_ecx;
    u32 edx = request.entry_edx;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto stop = [&](const Status status) {
        result.status = status;
        return finish();
    };
    const auto invoke = [&](const Call call,
                            const u32 object_token,
                            const std::initializer_list<u32> arguments = {}) {
        LegacyBattleActionSummaryCallRequest call_request{
            .call = call,
            .object_token = object_token,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        };
        std::copy(
            arguments.begin(), arguments.end(), call_request.arguments.begin()
        );
        ++result.port_calls;
        const auto reply = port.invoke_action_summary(call_request);
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        if (call == Call::configure_font_reset) {
            ++result.font_reset_calls;
        } else if (call == Call::configure_font_style) {
            ++result.font_style_calls;
        } else if (call == Call::draw_text) {
            ++result.text_draw_calls;
        }
    };

    if (eax == 0U) {
        return finish();
    }

    ecx = kFontToken;
    invoke(Call::configure_font_reset, kFontToken, {0U});
    ecx = kFontToken;
    invoke(Call::configure_font_style, kFontToken, {0xFFFEU});

    eax = bindings.final_actor.queued_actor_code;
    ecx = eax - 8U;
    const u32 actor_index = ecx;
    eax = actor_offset(actor_index);
    if (actor_index >= kGroupACount) {
        return stop(Status::group_a_actor_typed_stop);
    }
    ecx = startup.group_a_profiles.profile_tokens[actor_index];
    if (ecx == 0U) {
        return stop(Status::group_a_profile_typed_stop);
    }
    if (startup.group_a_profiles.profile_kinds[actor_index] == 0x38U) {
        ecx = kGroupABaseToken + eax;
        invoke(Call::query_actor_special_gate, ecx);
        ++result.actor_special_queries;
        if (eax == 0U) {
            write_permission(reset, 4U, 1U);
        }
    }

    ActionModePortAdapter action_mode_port(port, result);
    result.action_mode_refresh = refresh_legacy_battle_action_mode(
        {
            .startup_reset = reset,
            .source_state = startup.action_mode_source,
            .party_presence = startup.party_presence,
            .startup_mode_flags = startup.mode_flags,
            .final_actor = bindings.final_actor,
            .frame_input = bindings.frame_input,
            .input_dispatch = bindings.input_dispatch,
        },
        action_mode_port,
        {.entry_eax = eax, .entry_ecx = ecx, .entry_edx = edx}
    );
    ++result.action_mode_refresh_calls;
    eax = result.action_mode_refresh.return_eax;
    ecx = result.action_mode_refresh.return_ecx;
    edx = result.action_mode_refresh.return_edx;
    if (result.action_mode_refresh.status !=
        LegacyBattleActionModeRefreshStatus::completed) {
        return stop(Status::action_mode_refresh_typed_stop);
    }

    for (u32 index = 0U; index < 4U; ++index) {
        const u32 permission = read_permission(reset, index + 1U);
        edx = (edx & 0xFFFFFF00U) | permission;
        ecx = index + 1U;
        eax = index;
        ecx = replace_low_word(
            ecx,
            permission == 1U ? startup.primary_text_color
                             : startup.secondary_text_color
        );
        const u32 row_y = request.origin_y + index * 24U;
        const u32 label = kLegacyBattleStaticActionTextTokens[16U + index];
        edx = label;
        eax = request.origin_x;
        ecx = kFontToken;
        invoke(
            Call::draw_text,
            kFontToken,
            {
                kTextSurfaceToken,
                eax,
                row_y,
                label,
                replace_low_word(
                    0U,
                    permission == 1U ? startup.primary_text_color
                                     : startup.secondary_text_color
                ),
                4U,
            }
        );
        ++result.fixed_action_rows;

        edx = index + 1U;
        eax = request.action_kind;
        if (edx != eax) {
            continue;
        }
        ecx = kFontToken;
        invoke(Call::configure_font_style, kFontToken, {0xF000U});
        const u32 highlight_color =
            replace_low_word(eax, startup.primary_text_color);
        eax = request.origin_x - 1U;
        ecx = kFontToken;
        edx = 0U;
        invoke(
            Call::draw_text,
            kFontToken,
            {
                kTextSurfaceToken,
                eax,
                row_y - 1U,
                label,
                highlight_color,
                0x10U,
            }
        );
        ecx = kFontToken;
        invoke(Call::configure_font_style, kFontToken, {0xFFFEU});
    }

    const u32 dynamic_offset = 6U - kDynamicPermissionBaseToken;
    edx = request.origin_y;
    eax = dynamic_offset;
    ecx = kActionCodeBaseToken;
    u32 code_pointer = kActionCodeBaseToken;
    u32 row_y = request.origin_y + 24U;
    for (u32 index = 0U; index < 3U; ++index) {
        const u32 token = read_action_token(reset, index);
        if (token != 0U) {
            eax = read_action_code(reset, index);
            ecx = bindings.final_actor.queued_actor_code - 8U;
            const u32 action_code = eax;
            const u32 actor_index_for_call = ecx;
            eax = actor_index_for_call;
            eax <<= 6U;
            eax -= actor_index_for_call;
            eax <<= 4U;
            eax -= actor_index_for_call;
            edx = eax + eax * 2U;
            ecx = kGroupABaseToken + edx * 4U;
            invoke(Call::query_action_available, ecx, {action_code});
            ++result.action_availability_queries;

            const u32 permission_index = 6U + index;
            const bool available =
                eax == 1U && read_permission(reset, permission_index) == 1U;
            u32 color = 0U;
            if (available) {
                color = replace_low_word(eax, startup.primary_text_color);
                ecx = token;
                eax = kTextSurfaceToken;
                edx = request.origin_x + 48U;
            } else {
                color = replace_low_word(ecx, startup.secondary_text_color);
                edx = token;
                write_permission(reset, permission_index, 0U);
                ++result.permission_clears;
                ecx = kFontToken;
                eax = request.origin_x + 48U;
            }
            ecx = kFontToken;
            invoke(
                Call::draw_text,
                kFontToken,
                {
                    kTextSurfaceToken,
                    request.origin_x + 48U,
                    row_y,
                    token,
                    color,
                    4U,
                }
            );
            ++result.dynamic_action_rows;

            edx = dynamic_offset;
            ecx = request.action_kind;
            eax = edx + index + kDynamicPermissionBaseToken;
            if (eax == ecx) {
                ecx = kFontToken;
                invoke(Call::configure_font_style, kFontToken, {0xF000U});
                const u32 highlight_color =
                    replace_low_word(ecx, startup.primary_text_color);
                edx = kTextSurfaceToken;
                eax = row_y - 1U;
                ecx = kFontToken;
                invoke(
                    Call::draw_text,
                    kFontToken,
                    {
                        kTextSurfaceToken,
                        request.origin_x + 47U,
                        eax,
                        token,
                        highlight_color,
                        0x10U,
                    }
                );
                ecx = kFontToken;
                invoke(Call::configure_font_style, kFontToken, {0xFFFEU});
            }
        }
        ++code_pointer;
        ++code_pointer;
        row_y += 24U;
        ecx = code_pointer;
    }

    ecx = kFontToken;
    invoke(Call::configure_font_style, kFontToken, {0xFFFEU});
    return finish();
}

}  // namespace openswd3::battle
