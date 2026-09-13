#pragma once

#include "openswd3/compat/types.hpp"

#include <memory>

namespace openswd3::battle {

// Canonical aliased backing for the full actor+0x26C0 dword. Legacy users
// observe both the complete delay/mode word and its low direction byte. Copy
// construction shares the physical-value owner; assignment preserves the
// destination backing so established actor aliases remain attached.
class LegacyBattleActorField26c0 final {
public:
    LegacyBattleActorField26c0() : backing_(std::make_shared<compat::u32>()) {}

    LegacyBattleActorField26c0(const compat::u32 value)
        : backing_(std::make_shared<compat::u32>(value)) {}

    LegacyBattleActorField26c0(
        const LegacyBattleActorField26c0& other
    ) noexcept = default;
    LegacyBattleActorField26c0(LegacyBattleActorField26c0&& other) noexcept
        : backing_(other.backing_) {}
    LegacyBattleActorField26c0&
    operator=(const LegacyBattleActorField26c0& other) noexcept {
        *backing_ = *other.backing_;
        return *this;
    }
    LegacyBattleActorField26c0&
    operator=(LegacyBattleActorField26c0&& other) noexcept {
        *backing_ = *other.backing_;
        return *this;
    }

    LegacyBattleActorField26c0& operator=(const compat::u32 value) noexcept {
        *backing_ = value;
        return *this;
    }

    [[nodiscard]] operator compat::u32() const noexcept {
        return *backing_;
    }

    [[nodiscard]] compat::u32* data() noexcept {
        return backing_.get();
    }
    [[nodiscard]] const compat::u32* data() const noexcept {
        return backing_.get();
    }

    void alias(const LegacyBattleActorField26c0& canonical) noexcept {
        backing_ = canonical.backing_;
    }

private:
    std::shared_ptr<compat::u32> backing_;
};

}  // namespace openswd3::battle
