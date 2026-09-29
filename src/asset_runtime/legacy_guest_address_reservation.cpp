#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"

#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <new>

namespace openswd3::asset_runtime {
namespace {

// swd3.exe PE header: fixed ImageBase 0x00400000, SizeOfImage 0x001A7000;
// relocation directory is empty (p4-oracle-runtime-baseline.tsv).
constexpr std::uint64_t kOriginalImageBegin = 0x00400000ULL;
constexpr std::uint64_t kOriginalImageEnd = 0x005A7000ULL;
constexpr std::uint64_t kDynamicBegin = 0x50000000ULL;
constexpr std::uint64_t kDynamicEnd = 0x70000000ULL;
constexpr std::uint64_t kGuestEnd = 0x100000000ULL;

struct ExternalRange {
    std::uint64_t end{};
    std::uint64_t id{};
};

struct GuestRegistry {
    std::mutex mutex;
    std::map<compat::u32, std::uint64_t> dynamic;
    std::map<compat::u32, ExternalRange> external;
    std::uint64_t next_dynamic{kDynamicBegin};
    std::uint64_t next_id{1U};
};

[[nodiscard]] GuestRegistry* registry() noexcept {
    // Leases can outlive other static objects during process teardown.
    static auto* const value = new (std::nothrow) GuestRegistry;
    return value;
}

[[nodiscard]] constexpr std::uint64_t
align_sixteen(const std::uint64_t value) noexcept {
    return (value + 15U) & ~std::uint64_t{15U};
}

}  // namespace

std::optional<compat::u32>
reserve_legacy_guest_bytes(const std::size_t count) noexcept {
    auto* const state = registry();
    const auto bytes = static_cast<std::uint64_t>(count == 0U ? 1U : count);
    if (state == nullptr || bytes > kDynamicEnd - kDynamicBegin) {
        return std::nullopt;
    }

    const std::uint64_t padded = align_sixteen(bytes);
    std::lock_guard guard{state->mutex};
    std::uint64_t candidate = state->next_dynamic;
    while (candidate <= kDynamicEnd && padded <= kDynamicEnd - candidate) {
        const auto next =
            state->external.upper_bound(static_cast<compat::u32>(candidate));
        if (next != state->external.begin()) {
            const auto previous = std::prev(next);
            if (previous->second.end > candidate) {
                candidate = align_sixteen(previous->second.end);
                continue;
            }
        }

        if (next != state->external.end() && next->first < candidate + padded) {
            candidate = align_sixteen(next->second.end);
            continue;
        }

        try {
            state->dynamic.emplace(
                static_cast<compat::u32>(candidate), candidate + padded
            );
        } catch (const std::bad_alloc&) {
            return std::nullopt;
        }

        state->next_dynamic = candidate + padded;
        return static_cast<compat::u32>(candidate);
    }

    return std::nullopt;
}

std::shared_ptr<LegacyGuestExternalReservation>
register_legacy_external_guest_bytes(
    const compat::u32 guest_base, const std::size_t count
) noexcept {
    auto* const state = registry();
    const auto begin = static_cast<std::uint64_t>(guest_base);
    const auto bytes = static_cast<std::uint64_t>(count == 0U ? 1U : count);
    if (state == nullptr || guest_base == 0U || bytes > kGuestEnd - begin) {
        return {};
    }

    const std::uint64_t end = begin + bytes;
    if (begin < kOriginalImageEnd && end > kOriginalImageBegin) {
        return {};
    }

    std::lock_guard guard{state->mutex};
    const auto next_dynamic = state->dynamic.lower_bound(guest_base);
    if (next_dynamic != state->dynamic.end() && next_dynamic->first < end) {
        return {};
    }

    if (next_dynamic != state->dynamic.begin() &&
        std::prev(next_dynamic)->second > begin) {
        return {};
    }

    const auto next = state->external.lower_bound(guest_base);
    if (next != state->external.end() && next->first < end) {
        return {};
    }

    if (next != state->external.begin() &&
        std::prev(next)->second.end > begin) {
        return {};
    }

    if (state->next_id == std::numeric_limits<std::uint64_t>::max()) {
        return {};
    }

    try {
        auto lease = std::shared_ptr<LegacyGuestExternalReservation>(
            new LegacyGuestExternalReservation(guest_base, 0U)
        );
        const std::uint64_t id = state->next_id++;
        state->external.emplace(guest_base, ExternalRange{end, id});
        lease->id_ = id;
        return lease;
    } catch (const std::bad_alloc&) {
        return {};
    }
}

LegacyGuestExternalReservation::~LegacyGuestExternalReservation() {
    if (id_ == 0U) {
        return;
    }

    auto* const state = registry();
    if (state == nullptr) {
        return;
    }

    std::lock_guard guard{state->mutex};
    const auto found = state->external.find(guest_base_);
    if (found != state->external.end() && found->second.id == id_) {
        state->external.erase(found);
    }
}

}  // namespace openswd3::asset_runtime
