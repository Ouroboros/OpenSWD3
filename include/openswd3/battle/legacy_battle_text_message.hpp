#pragma once

#include "openswd3/compat/types.hpp"

#include <array>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleTextMessageHeadToken = 0x005214F8U;
inline constexpr compat::u32 kLegacyBattleTextMessageAllocationSize = 0x24U;
inline constexpr compat::u32 kLegacyBattleTextMessageAllocateCallToken =
    0x00487C10U;
inline constexpr compat::u32 kLegacyBattleTextMessageLengthCallToken =
    0x00499160U;

struct LegacyBattleTextMessageRecord {
    compat::u32 next_token{};
    compat::u32 value_04{};
    compat::u32 value_08{};
    compat::u32 text_length{};
    compat::u32 value_10{};
    compat::u32 value_14{};
    compat::u32 flags{};
    compat::u16 kind{};
    compat::u16 padding_1e{};
    compat::u32 text_token{};
};

static_assert(sizeof(LegacyBattleTextMessageRecord) == 0x24U);

struct LegacyBattleTextMessageAllocation {
    compat::u32 token{};
    LegacyBattleTextMessageRecord record{};
};

struct LegacyBattleTextMessageState {
    std::vector<LegacyBattleTextMessageAllocation> allocations;
};

struct LegacyBattleTextMessageRegisters {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

enum class LegacyBattleTextMessageCall : compat::u8 {
    allocate,
    measure_text,
};

struct LegacyBattleTextMessageCallRequest {
    LegacyBattleTextMessageCall call{LegacyBattleTextMessageCall::allocate};
    compat::u32 argument{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
};

struct LegacyBattleTextMessageCallReply {
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    bool text_access_failed{};
};

class LegacyBattleTextMessagePort {
public:
    virtual ~LegacyBattleTextMessagePort() = default;

    [[nodiscard]] virtual LegacyBattleTextMessageCallReply
    invoke_text_message(const LegacyBattleTextMessageCallRequest& request) = 0;
};

struct LegacyBattleTextMessageRequest {
    compat::u32 value_04{};
    compat::u32 value_08{};
    compat::u16 kind{};
    compat::u32 text_token{};
    compat::u32 flags{};
    LegacyBattleTextMessageRegisters entry{};
};

enum class LegacyBattleTextMessageStatus : compat::u8 {
    completed,
    allocation_typed_stop,
    text_typed_stop,
    chain_typed_stop,
};

struct LegacyBattleTextMessageResult {
    LegacyBattleTextMessageStatus status{
        LegacyBattleTextMessageStatus::completed
    };
    LegacyBattleTextMessageRegisters return_registers{};
    compat::u32 allocation_calls{};
    compat::u32 measure_calls{};
    compat::u32 traversal_count{};
    compat::u32 allocated_token{};
    compat::u32 stopped_chain_token{};
    bool appended{};
};

// Typed closure of legacy 0x004698E0. Allocates and zeroes one 0x24-byte
// message record, publishes its fields, applies the bit-6 override, and
// appends it to the shared singly linked list.
[[nodiscard]] LegacyBattleTextMessageResult enqueue_legacy_battle_text_message(
    LegacyBattleTextMessageState& state,
    compat::u32& head_token,
    LegacyBattleTextMessagePort& port,
    const LegacyBattleTextMessageRequest& request
);

}  // namespace openswd3::battle
