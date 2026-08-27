#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"

#include <array>

namespace openswd3::battle {
namespace {

using compat::u16;
using compat::u32;

constexpr u32 kGroupACount = 10U;
constexpr u32 kActorMappingCount = 4U;
constexpr u32 kOptionSourceCount = 4U;
constexpr u32 kPermissionCount = 9U;

inline constexpr std::array<u32, 21> kStaticActionTextTokens{
    0x004A76D0U, 0x004A76C8U, 0x004A76C0U, 0x004A76B8U, 0x004A76B0U,
    0x004A76A8U, 0x004A76A0U, 0x004A7698U, 0x004A7690U, 0x004A7688U,
    0x004A7680U, 0x004A7678U, 0x004A7670U, 0x004A7668U, 0x004A7660U,
    0x004A7658U, 0x004A6BD8U, 0x004A7650U, 0x004A020CU, 0x004A7648U,
    0x004A6BD0U,
};

[[nodiscard]] constexpr u32 group_a_token(const u32 actor_index) noexcept {
    return kLegacyBattleActionGroupABaseToken +
        actor_index * kLegacyBattleActionGroupAStride;
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

[[nodiscard]] bool write_option_code(
    LegacyBattleStartupResetBlocks& reset, const u32 index, const u16 value
) noexcept {
    if (index == 0U) {
        reset.value_4fe5cc =
            (reset.value_4fe5cc & 0xFFFF0000U) | static_cast<u32>(value);
        return true;
    }
    if (index == 1U) {
        reset.value_4fe5cc = (reset.value_4fe5cc & 0x0000FFFFU) |
            (static_cast<u32>(value) << 16U);
        return true;
    }
    if (index == 2U) {
        reset.value_4fe5d0 = value;
        return true;
    }
    return false;
}

[[nodiscard]] bool write_option_token(
    LegacyBattleStartupResetBlocks& reset, const u32 index, const u32 value
) noexcept {
    if (index == 0U) {
        reset.value_4ff0b0 = value;
        return true;
    }
    if (index == 1U) {
        reset.value_4ff0b4 = value;
        return true;
    }
    if (index == 2U) {
        reset.value_4ff0b8 = value;
        return true;
    }
    return false;
}

[[nodiscard]] u32 lookup_action_text_token(
    const LegacyBattleActionModeRefreshBindings& bindings, const u32 code
) noexcept {
    if (code <= 0x29U) {
        return kStaticActionTextTokens[code - 0x15U];
    }
    switch (code) {
    case 0x2AU:
        return bindings.input_dispatch.action_kind;
    case 0x2BU:
        return bindings.final_actor.published_actor_code;
    case 0x2CU:
        return bindings.frame_input.target_cursor;
    case 0x2DU:
        return bindings.input_dispatch.action_lookup_auxiliary;
    case 0x2EU:
        return bindings.frame_input.list_selection;
    case 0x2FU:
        return bindings.frame_input.grid_selection;
    case 0x30U:
        return bindings.frame_input.narrow_list_selection;
    default:
        return bindings.frame_input.selection_actor_code;
    }
}

}  // namespace

LegacyBattleActionModeRefreshResult refresh_legacy_battle_action_mode(
    const LegacyBattleActionModeRefreshBindings bindings,
    LegacyBattleInputDispatchPort& port,
    const LegacyBattleActionModeRefreshRequest& request
) {
    LegacyBattleActionModeRefreshResult result;
    auto& reset = bindings.startup_reset;
    auto& input = bindings.input_dispatch;
    u32 eax = 0x01010101U;
    u32 ecx = 0U;
    u32 edx = request.entry_edx;
    u32 ebx = 1U;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        return result;
    };
    const auto stop = [&](const LegacyBattleActionModeRefreshStatus status) {
        result.status = status;
        return finish();
    };
    const auto invoke = [&](const LegacyBattleInputDispatchCall call) {
        ++result.port_calls;
        const auto reply = port.invoke_input_dispatch({
            .call = call,
            .eax = eax,
            .ecx = ecx,
            .edx = edx,
        });
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
    };

    reset.value_524414 = eax;
    reset.value_524418 = eax;
    eax = bindings.final_actor.queued_actor_code;
    reset.value_4ff0b0 = 0U;
    const u32 actor_index = eax - 8U;
    reset.value_4ff0b4 = 0U;
    edx &= 0xFFFF0000U;
    reset.value_4ff0b8 = 0U;
    reset.value_524418 &= 0x000000FFU;
    reset.value_53bf22 = static_cast<u16>(edx);

    u32 source_index = 0U;
    if (actor_index == 0U && bindings.party_presence[0U] == 1U) {
        if ((bindings.startup_mode_flags & 2U) != 0U) {
            edx = (edx & 0xFFFF0000U) | static_cast<u16>(ebx);
            reset.value_4ff0b0 = kLegacyBattleActionModeFixedTextToken;
            reset.value_4fe5cc = (reset.value_4fe5cc & 0xFFFF0000U) | 6U;
            reset.value_53bf22 = static_cast<u16>(edx);
            write_permission(reset, 6U, ebx);
            ++result.permission_writes;
            ++result.option_code_writes;
            ++result.option_token_writes;
        }
    } else {
        if (actor_index >= kActorMappingCount) {
            return stop(
                LegacyBattleActionModeRefreshStatus::actor_mapping_typed_stop
            );
        }
        source_index = bindings.source_state.actor_label_indices[actor_index];
    }

    if (source_index >= kOptionSourceCount) {
        return stop(
            LegacyBattleActionModeRefreshStatus::option_source_typed_stop
        );
    }

    for (const auto& source :
         bindings.source_state.option_sources[source_index]) {
        ecx = source.object_token;
        ++result.option_pointer_reads;
        if (ecx == 0U) {
            return stop(
                LegacyBattleActionModeRefreshStatus::option_object_typed_stop
            );
        }
        ecx = (ecx & 0xFFFF0000U) | static_cast<u32>(source.action_code);
        ++result.option_object_reads;
        if (source.action_code < 0x15U || source.action_code >= 0x32U) {
            continue;
        }

        eax = static_cast<u16>(edx);
        const u32 permission_index = 6U + eax;
        if (permission_index >= kPermissionCount) {
            return stop(
                LegacyBattleActionModeRefreshStatus::option_workspace_typed_stop
            );
        }
        write_permission(reset, permission_index, ebx);
        ++result.permission_writes;
        ebx = static_cast<u16>(ecx);
        edx =
            (edx & 0xFFFF0000U) | static_cast<u16>(static_cast<u16>(edx) + 1U);
        if (!write_option_code(reset, eax, static_cast<u16>(ecx))) {
            return stop(
                LegacyBattleActionModeRefreshStatus::option_workspace_typed_stop
            );
        }
        ++result.option_code_writes;
        reset.value_53bf22 = static_cast<u16>(edx);
        ebx = lookup_action_text_token(bindings, ebx);
        if (!write_option_token(reset, eax, ebx)) {
            return stop(
                LegacyBattleActionModeRefreshStatus::option_workspace_typed_stop
            );
        }
        ++result.option_token_writes;
        ++result.qualifying_options;
        ebx = 1U;
    }

    eax = actor_index;
    eax <<= 6U;
    eax -= actor_index;
    eax <<= 4U;
    eax -= actor_index;
    ecx = group_a_token(actor_index);
    if (actor_index >= kGroupACount) {
        return stop(
            LegacyBattleActionModeRefreshStatus::group_a_actor_typed_stop
        );
    }
    invoke(LegacyBattleInputDispatchCall::action_mode_query_primary_actor);
    ++result.primary_actor_queries;
    if (eax == 0U) {
        write_permission(reset, 2U, eax);
        ++result.permission_writes;
    }

    edx = bindings.final_actor.queued_actor_code;
    ecx = edx - 8U;
    const u32 second_actor_index = ecx;
    eax = ecx;
    eax <<= 6U;
    eax -= ecx;
    eax <<= 4U;
    eax -= ecx;
    eax = eax + eax * 2U;
    ecx = group_a_token(second_actor_index);
    invoke(LegacyBattleInputDispatchCall::action_mode_query_secondary_actor);
    ++result.secondary_actor_queries;
    if (eax == 0U) {
        write_permission(reset, 4U, eax);
        ++result.permission_writes;
    }

    ecx = bindings.final_actor.queued_actor_code;
    ecx += 0xFFFFFFF8U;
    const u32 third_actor_index = ecx;
    eax = ecx;
    eax <<= 6U;
    eax -= ecx;
    eax <<= 4U;
    eax -= ecx;
    edx = eax + eax * 2U;
    ecx = group_a_token(third_actor_index);
    invoke(LegacyBattleInputDispatchCall::action_mode_query_active_actor);
    ++result.active_actor_queries;
    if (eax != ebx) {
        return finish();
    }

    ecx = input.action_kind;
    eax = 0U;
    reset.value_524414 = eax;
    write_permission(reset, 2U, ebx);
    ++result.permission_writes;
    reset.value_524418 = eax;
    write_permission(reset, 5U, ebx);
    ++result.permission_writes;
    if (ecx >= kPermissionCount) {
        return stop(LegacyBattleActionModeRefreshStatus::permission_typed_stop);
    }
    eax = read_permission(reset, ecx);
    if (eax == 0U) {
        input.action_kind = 2U;
    }
    return finish();
}

}  // namespace openswd3::battle
