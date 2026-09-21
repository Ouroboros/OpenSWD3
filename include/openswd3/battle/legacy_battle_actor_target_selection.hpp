#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <utility>

namespace openswd3::battle {

struct LegacyBattleActionDispatchState;
struct LegacyBattleStartupState;

inline constexpr compat::u32 kLegacyBattleActorTargetSelectionAddress =
    0x00478A70U;
inline constexpr compat::u32 kLegacyBattleActorTargetSelectionEndAddress =
    0x00478A91U;
inline constexpr std::size_t kLegacyBattleActorTargetSelectionMaxCalls = 32U;
inline constexpr std::size_t kLegacyBattleActorTargetSelectionAccessCount = 5U;

inline constexpr std::array<compat::u32, 20>
    kLegacyBattleActorTargetSelectionCallAddresses{
        0x00454B8DU, 0x00456B59U, 0x00456CECU, 0x00456D80U, 0x00456E5AU,
        0x00456FC1U, 0x00457903U, 0x00457925U, 0x00457C18U, 0x00457D72U,
        0x00457DFBU, 0x00457E26U, 0x0045AF7DU, 0x004671C9U, 0x0046B522U,
        0x0046B5B9U, 0x0046B733U, 0x0046B850U, 0x0046B8D4U, 0x0046DC9CU,
    };

inline constexpr std::array<compat::u32, 20>
    kLegacyBattleActorTargetSelectionReturnAddresses{
        0x00454B92U, 0x00456B5EU, 0x00456CF1U, 0x00456D85U, 0x00456E5FU,
        0x00456FC6U, 0x00457908U, 0x0045792AU, 0x00457C1DU, 0x00457D77U,
        0x00457E00U, 0x00457E2BU, 0x0045AF82U, 0x004671CEU, 0x0046B527U,
        0x0046B5BEU, 0x0046B738U, 0x0046B855U, 0x0046B8D9U, 0x0046DCA1U,
    };

enum class LegacyBattleActorTargetSelectionAccessKind : compat::u8 {
    argument_read,
    idle_state_write,
    action_target_write,
    progress_write,
    return_address_read,
};

enum class LegacyBattleActorTargetSelectionStatus : compat::u8 {
    completed,
    argument_read_typed_stop,
    idle_state_write_typed_stop,
    action_target_write_typed_stop,
    progress_write_typed_stop,
    return_address_read_typed_stop,
};

struct LegacyBattleActorTargetSelectionAccess {
    LegacyBattleActorTargetSelectionAccessKind kind{};
    compat::u32 instruction_address{};
    compat::u32 memory_address{};
    compat::u32 value{};
    compat::u8 width{};
    bool write{};
};

struct LegacyBattleActorTargetSelectionView {
    compat::u32* idle_state_latch{};
    compat::u16* action_target{};
    compat::u32* progress{};
};

struct LegacyBattleActorTargetSelectionOwners {
    LegacyBattleActionDispatchState* action{};
    LegacyBattleStartupState* startup{};
};

struct LegacyBattleActorTargetSelectionRequest {
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u16 argument_value{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_esp{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{};
    bool argument_read_accessible{true};
    bool idle_state_write_accessible{true};
    bool action_target_write_accessible{true};
    bool progress_write_accessible{true};
    bool return_address_read_accessible{true};
};

template <typename Value> class LegacyBattleActorTargetSelectionLazyArray {
public:
    LegacyBattleActorTargetSelectionLazyArray() = default;

    LegacyBattleActorTargetSelectionLazyArray(
        const LegacyBattleActorTargetSelectionLazyArray& other
    ) {
        if (other.values_ != nullptr) {
            values_ = std::make_unique<Storage>(*other.values_);
        }
    }

    LegacyBattleActorTargetSelectionLazyArray&
    operator=(const LegacyBattleActorTargetSelectionLazyArray& other) {
        if (this == &other) {
            return *this;
        }
        if (other.values_ == nullptr) {
            values_.reset();
            return *this;
        }
        values_ = std::make_unique<Storage>(*other.values_);
        return *this;
    }

    LegacyBattleActorTargetSelectionLazyArray(
        LegacyBattleActorTargetSelectionLazyArray&&
    ) noexcept = default;

    LegacyBattleActorTargetSelectionLazyArray&
    operator=(LegacyBattleActorTargetSelectionLazyArray&&) noexcept = default;

    [[nodiscard]] Value& operator[](const std::size_t index) {
        if (values_ == nullptr) {
            values_ = std::make_unique<Storage>();
        }
        return (*values_)[index];
    }

    [[nodiscard]] const Value&
    operator[](const std::size_t index) const noexcept {
        if (values_ == nullptr) {
            static constexpr Value empty{};
            return empty;
        }
        return (*values_)[index];
    }

    [[nodiscard]] static constexpr std::size_t size() noexcept {
        return kLegacyBattleActorTargetSelectionMaxCalls;
    }

private:
    using Storage =
        std::array<Value, kLegacyBattleActorTargetSelectionMaxCalls>;

    std::unique_ptr<Storage> values_;
};

struct LegacyBattleActorTargetSelectionRequestList {
    LegacyBattleActorTargetSelectionLazyArray<
        LegacyBattleActorTargetSelectionRequest>
        calls;
    std::size_t count{};
};

struct LegacyBattleActorTargetSelectionResult {
    LegacyBattleActorTargetSelectionStatus status{
        LegacyBattleActorTargetSelectionStatus::completed
    };
    compat::u32 stop_instruction{};
    compat::u32 call_address{};
    compat::u32 return_address{};
    compat::u32 actor_token{};
    compat::u16 argument_value{};
    compat::u32 return_eax{};
    compat::u32 return_ecx{};
    compat::u32 return_edx{};
    compat::u32 return_esp{};
    compat::u32 return_eip{kLegacyBattleActorTargetSelectionAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool returned{};
    std::array<
        LegacyBattleActorTargetSelectionAccess,
        kLegacyBattleActorTargetSelectionAccessCount>
        accesses{};
    std::size_t access_count{};
};

struct LegacyBattleActorTargetSelectionTrace {
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> call_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> return_addresses;
    LegacyBattleActorTargetSelectionLazyArray<compat::u32> actor_tokens;
    LegacyBattleActorTargetSelectionLazyArray<compat::u16> argument_values;
    std::size_t calls{};

private:
    std::unique_ptr<LegacyBattleActorTargetSelectionResult> last_storage_;

public:
    LegacyBattleActorTargetSelectionResult& last;

    LegacyBattleActorTargetSelectionTrace()
        : last_storage_(
              std::make_unique<LegacyBattleActorTargetSelectionResult>()
          ),
          last(*last_storage_) {}

    LegacyBattleActorTargetSelectionTrace(
        const LegacyBattleActorTargetSelectionTrace& other
    )
        : call_addresses(other.call_addresses),
          return_addresses(other.return_addresses),
          actor_tokens(other.actor_tokens),
          argument_values(other.argument_values), calls(other.calls),
          last_storage_(
              std::make_unique<LegacyBattleActorTargetSelectionResult>(
                  other.last
              )
          ),
          last(*last_storage_) {}

    LegacyBattleActorTargetSelectionTrace&
    operator=(const LegacyBattleActorTargetSelectionTrace& other) {
        if (this == &other) {
            return *this;
        }
        call_addresses = other.call_addresses;
        return_addresses = other.return_addresses;
        actor_tokens = other.actor_tokens;
        argument_values = other.argument_values;
        calls = other.calls;
        last = other.last;
        return *this;
    }

    LegacyBattleActorTargetSelectionTrace(
        LegacyBattleActorTargetSelectionTrace&& other
    ) noexcept
        : call_addresses(std::move(other.call_addresses)),
          return_addresses(std::move(other.return_addresses)),
          actor_tokens(std::move(other.actor_tokens)),
          argument_values(std::move(other.argument_values)), calls(other.calls),
          last_storage_(
              std::make_unique<LegacyBattleActorTargetSelectionResult>(
                  std::move(other.last)
              )
          ),
          last(*last_storage_) {}

    LegacyBattleActorTargetSelectionTrace&
    operator=(LegacyBattleActorTargetSelectionTrace&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        call_addresses = std::move(other.call_addresses);
        return_addresses = std::move(other.return_addresses);
        actor_tokens = std::move(other.actor_tokens);
        argument_values = std::move(other.argument_values);
        calls = other.calls;
        last = std::move(other.last);
        return *this;
    }
};

[[nodiscard]] LegacyBattleActorTargetSelectionView
resolve_legacy_battle_actor_target_selection(
    const LegacyBattleActorTargetSelectionOwners& owners,
    compat::u32 actor_token
) noexcept;

[[nodiscard]] LegacyBattleActorTargetSelectionResult
apply_legacy_battle_actor_target_selection(
    const LegacyBattleActorTargetSelectionView& actor,
    const LegacyBattleActorTargetSelectionRequest& request
) noexcept;

[[nodiscard]] bool execute_legacy_battle_actor_target_selection_call(
    LegacyBattleActorTargetSelectionTrace& trace,
    const LegacyBattleActorTargetSelectionRequestList& requests,
    const LegacyBattleActorTargetSelectionOwners& owners,
    compat::u32 call_address,
    compat::u32 return_address,
    compat::u32 actor_token,
    compat::u16 argument_value,
    compat::u32 entry_eax = 0U,
    compat::u32 entry_edx = 0U,
    const LegacyBattleActorCoordinateFlags& entry_flags = {},
    bool entry_flags_known = false,
    std::size_t request_offset = 0U
) noexcept;

}  // namespace openswd3::battle
