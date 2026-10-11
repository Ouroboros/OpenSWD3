#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::battle {

struct LegacyBattleActorActionModeView {
    compat::u16* action_kind{};
    compat::u16* display_kind{};
    compat::u8* mode_flags{};
};

struct LegacyBattleActorActionModeMemoryAccess {
    bool argument_readable{true};
    bool mode_flags_readable{true};
    bool mode_flags_writable{true};
    bool display_kind_writable{true};
    bool action_kind_writable{true};
    bool return_address_readable{true};
};

enum class LegacyBattleActorActionModeStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    mode_flags_read_typed_stop,
    mode_flags_write_typed_stop,
    display_kind_write_typed_stop,
    action_kind_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorActionModeResult {
    LegacyBattleActorActionModeStatus status{
        LegacyBattleActorActionModeStatus::completed
    };
};

[[nodiscard]] LegacyBattleActorActionModeResult
set_legacy_battle_actor_action_mode(
    LegacyBattleActorActionModeView actor,
    compat::u32 mode,
    LegacyBattleActorActionModeMemoryAccess access = {}
) noexcept;

}  // namespace openswd3::battle
