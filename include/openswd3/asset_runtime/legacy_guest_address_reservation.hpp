#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

namespace openswd3::asset_runtime {

// Dynamic guest identities are never recycled. A live external block must
// register its full byte range before a persistent guest pointer is borrowed.
[[nodiscard]] std::optional<compat::u32>
reserve_legacy_guest_bytes(std::size_t count) noexcept;

class LegacyGuestExternalReservation;

[[nodiscard]] std::shared_ptr<LegacyGuestExternalReservation>
register_legacy_external_guest_bytes(
    compat::u32 guest_base, std::size_t count
) noexcept;

class LegacyGuestExternalReservation final {
public:
    LegacyGuestExternalReservation(const LegacyGuestExternalReservation&) =
        delete;
    LegacyGuestExternalReservation& operator=(
        const LegacyGuestExternalReservation&
    ) = delete;
    ~LegacyGuestExternalReservation();

    [[nodiscard]] compat::u32 guest_base() const noexcept {
        return guest_base_;
    }

private:
    friend std::shared_ptr<LegacyGuestExternalReservation>
    register_legacy_external_guest_bytes(
        compat::u32 guest_base, std::size_t count
    ) noexcept;

    LegacyGuestExternalReservation(
        compat::u32 guest_base, std::uint64_t id
    ) noexcept
        : guest_base_(guest_base), id_(id) {}

    compat::u32 guest_base_{};
    std::uint64_t id_{};
};

}  // namespace openswd3::asset_runtime
