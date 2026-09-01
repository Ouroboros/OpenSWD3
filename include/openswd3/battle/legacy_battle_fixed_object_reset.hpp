#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <span>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleFixedObjectDwordCount = 5U;
inline constexpr std::array<compat::u32, 3> kLegacyBattleFixedResetObjectTokens{
    0x004B9F00U,
    0x004ACBA8U,
    0x004B8A00U,
};

struct LegacyBattleFixedObjectState {
    std::array<
        std::array<compat::u32, kLegacyBattleFixedObjectDwordCount>,
        kLegacyBattleFixedResetObjectTokens.size()>
        object_words{};
};

class LegacyBattleFixedObjectStatePort {
public:
    virtual ~LegacyBattleFixedObjectStatePort() = default;

    [[nodiscard]] virtual LegacyBattleFixedObjectState&
    legacy_battle_fixed_object_state() noexcept {
        return state_;
    }

    [[nodiscard]] virtual const LegacyBattleFixedObjectState&
    legacy_battle_fixed_object_state() const noexcept {
        return state_;
    }

private:
    LegacyBattleFixedObjectState state_{};
};

enum class LegacyBattleFixedObjectResetStatus : compat::u8 {
    completed,
    object_write_typed_stop,
};

struct LegacyBattleFixedObjectResetResult {
    LegacyBattleFixedObjectResetStatus status{
        LegacyBattleFixedObjectResetStatus::completed
    };
    compat::u32 object_token{};
    compat::u32 dword_writes{};
    compat::u32 stopped_object_offset{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
};

// Typed closure of legacy 0x004776F0. The accessible span models the
// consecutive dword writes without treating a legacy token as a host pointer.
[[nodiscard]] LegacyBattleFixedObjectResetResult
reset_legacy_battle_fixed_object(
    std::span<compat::u32> object_words,
    compat::u32 object_token,
    compat::u32 entry_edx
) noexcept;

}  // namespace openswd3::battle
