#pragma once

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/compat/types.hpp"

#include <bit>
#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>

namespace openswd3::battle {

static_assert(std::endian::native == std::endian::little);

// A single allocation has both byte-addressable legacy heap storage and
// live 16-bit pixel objects. The Win32 wrapper rounds requested bytes up
// to 16; the last word only exists to cover an odd injected byte count.
class LegacyBattleActorFrameRawBlock final {
public:
    explicit LegacyBattleActorFrameRawBlock(const std::size_t bytes)
        : words_(std::make_unique_for_overwrite<compat::u16[]>(
              bytes / sizeof(compat::u16) + bytes % sizeof(compat::u16)
          )),
          byte_count_(bytes) {}

    LegacyBattleActorFrameRawBlock(const LegacyBattleActorFrameRawBlock&) =
        delete;
    LegacyBattleActorFrameRawBlock& operator=(
        const LegacyBattleActorFrameRawBlock&
    ) = delete;

    [[nodiscard]] std::size_t size_bytes() const noexcept {
        return byte_count_;
    }

    [[nodiscard]] std::span<compat::u8> bytes() noexcept {
        static_assert(std::is_same_v<compat::u8, unsigned char>);
        return {reinterpret_cast<compat::u8*>(words_.get()), byte_count_};
    }

    [[nodiscard]] std::span<compat::u16> words() noexcept {
        return {words_.get(), byte_count_ / sizeof(compat::u16)};
    }

    [[nodiscard]] bool claim_external_guest_base(
        const compat::u32 guest_base
    ) noexcept {
        if (external_lease_ != nullptr) {
            return external_lease_->guest_base() == guest_base;
        }

        external_lease_ =
            asset_runtime::register_legacy_external_guest_bytes(
                guest_base, byte_count_
            );
        return external_lease_ != nullptr;
    }

private:
    std::unique_ptr<compat::u16[]> words_{};
    std::size_t byte_count_{};
    std::shared_ptr<asset_runtime::LegacyGuestExternalReservation>
        external_lease_{};
};

}  // namespace openswd3::battle
