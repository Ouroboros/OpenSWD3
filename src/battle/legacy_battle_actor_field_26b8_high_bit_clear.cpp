#include "openswd3/battle/legacy_battle_actor_field_26b8_high_bit_clear.hpp"

namespace openswd3::battle {

LegacyBattleActorField26b8HighBitClearResult
clear_legacy_battle_actor_field_26b8_high_bit(
    compat::u32* const field_26b8,
    const LegacyBattleActorField26b8HighBitClearMemoryAccess access
) noexcept {
    using Status = LegacyBattleActorField26b8HighBitClearStatus;
    if (field_26b8 == nullptr || !access.field_readable) {
        return {Status::field_read_typed_stop};
    }

    const compat::u32 previous = *field_26b8;
    if (!access.field_writable) {
        return {Status::field_write_typed_stop};
    }

    *field_26b8 = previous & 0x7FFFFFFFU;
    if (!access.return_address_readable) {
        return {Status::return_address_read_typed_stop};
    }

    return {};
}

}  // namespace openswd3::battle
