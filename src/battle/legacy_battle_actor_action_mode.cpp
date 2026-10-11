#include "openswd3/battle/legacy_battle_actor_action_mode.hpp"

namespace openswd3::battle {

LegacyBattleActorActionModeResult set_legacy_battle_actor_action_mode(
    const LegacyBattleActorActionModeView actor,
    const compat::u32 mode,
    const LegacyBattleActorActionModeMemoryAccess access
) noexcept {
    using Status = LegacyBattleActorActionModeStatus;
    if (!access.argument_readable) {
        return {Status::argument_read_typed_stop};
    }

    const bool special = mode == 2U || mode == 13U || mode == 3U ||
        mode == 14U || mode == 6U || mode == 15U;
    if (special) {
        const compat::u8 mask = mode == 6U ? 0x08U
            : mode == 14U || mode == 15U   ? 0x40U
                                           : 0U;
        if (mask != 0U) {
            if (actor.mode_flags == nullptr || !access.mode_flags_readable) {
                return {Status::mode_flags_read_typed_stop};
            }

            const auto previous_flags = *actor.mode_flags;
            if (!access.mode_flags_writable) {
                return {Status::mode_flags_write_typed_stop};
            }

            *actor.mode_flags = static_cast<compat::u8>(previous_flags | mask);
        }

        if (actor.display_kind == nullptr || !access.display_kind_writable) {
            return {Status::display_kind_write_typed_stop};
        }

        *actor.display_kind = static_cast<compat::u16>(mode);
    }

    if (actor.action_kind == nullptr || !access.action_kind_writable) {
        return {Status::action_kind_write_typed_stop};
    }

    *actor.action_kind = special ? 0U : static_cast<compat::u16>(mode);
    if (!access.return_address_readable) {
        return {Status::return_address_read_typed_stop};
    }

    return {};
}

}  // namespace openswd3::battle
