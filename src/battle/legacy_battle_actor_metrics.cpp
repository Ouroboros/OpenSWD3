#include "openswd3/battle/legacy_battle_actor_metrics.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_frame_coordinator.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u16;
using compat::u32;
using compat::u8;

constexpr u32 kCallQueryActorMetric = 0x004783B0U;
constexpr u32 kGroupBBaseToken = 0x00525508U;
constexpr u32 kGroupBStride = 0x00002B28U;
constexpr u32 kGroupABaseToken = 0x005029D0U;
constexpr u32 kGroupAStride = 0x00002F34U;

struct Registers {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
};

struct InvocationReply {
    Registers registers{};
    bool publish_metric_byte{};
    u8 metric_byte{};
    bool publish_metric_word{};
    u16 metric_word{};
    bool publish_group_b_count{};
    u32 group_b_count{};
    bool publish_group_a_count{};
    u32 group_a_count{};
};

[[nodiscard]] constexpr u32 sign_extended_word(const u16 value) noexcept {
    return std::bit_cast<u32>(static_cast<i32>(std::bit_cast<i16>(value)));
}

void apply_reply(
    LegacyBattleActorMetricState& state,
    Registers& registers,
    const InvocationReply& reply
) noexcept {
    registers = reply.registers;
    if (reply.publish_metric_byte) {
        state.local_byte = reply.metric_byte;
    }
    if (reply.publish_metric_word) {
        state.local_word = reply.metric_word;
    }
    if (reply.publish_group_b_count) {
        state.group_b_count = reply.group_b_count;
    }
    if (reply.publish_group_a_count) {
        state.group_a_count = reply.group_a_count;
    }
}

template <typename Call>
[[nodiscard]] LegacyBattleActorMetricResult
rebuild_impl(LegacyBattleActorMetricState& state, Call&& call) {
    LegacyBattleActorMetricResult result;
    std::ranges::fill(state.values, 0);
    std::ranges::fill(state.secondary_values, 0U);
    state.local_word = static_cast<u16>(state.entry_ecx);
    state.local_byte = static_cast<u8>(state.entry_ecx >> 16U);

    Registers registers{
        .eax = state.group_b_count,
        .ecx = 0U,
        .edx = state.entry_edx,
    };
    u32 index = 0U;
    if (state.group_b_count != 0U) {
        while (true) {
            registers.eax = state.local_word_token;
            registers.ecx = kGroupBBaseToken + index * kGroupBStride;
            ++result.port_calls;
            apply_reply(
                state,
                registers,
                call(
                    registers.ecx,
                    state.local_byte_token,
                    state.local_word_token,
                    registers
                )
            );
            registers.edx = sign_extended_word(state.local_word);
            registers.eax = state.group_b_count;
            if (index >= state.values.size()) {
                result.status =
                    LegacyBattleActorMetricStatus::value_store_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = state.entry_ecx;
                result.final_edx = registers.edx;
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
                return result;
            }
            state.values[index] = std::bit_cast<i32>(registers.edx);
            ++index;
            ++result.group_b_iterations;
            if (index >= state.group_b_count) {
                break;
            }
        }
    }

    index = 8U;
    registers.eax = state.group_a_count + index;
    if (registers.eax > index) {
        while (true) {
            registers.edx = state.local_byte_token;
            registers.ecx = kGroupABaseToken + (index - 8U) * kGroupAStride;
            ++result.port_calls;
            apply_reply(
                state,
                registers,
                call(
                    registers.ecx,
                    state.local_byte_token,
                    state.local_word_token,
                    registers
                )
            );
            registers.eax = sign_extended_word(state.local_word);
            registers.ecx = state.group_a_count;
            if (index >= state.values.size()) {
                result.status =
                    LegacyBattleActorMetricStatus::value_store_typed_stop;
                result.return_value = registers.eax;
                result.final_ecx = state.entry_ecx;
                result.final_edx = registers.edx;
                state.entry_eax = result.return_value;
                state.entry_edx = result.final_edx;
                return result;
            }
            state.values[index] = std::bit_cast<i32>(registers.eax);
            ++index;
            ++result.group_a_iterations;
            registers.ecx = state.group_a_count + 8U;
            if (index >= registers.ecx) {
                break;
            }
        }
    }

    result.return_value = registers.eax;
    result.final_ecx = state.entry_ecx;
    result.final_edx = registers.edx;
    state.entry_eax = result.return_value;
    state.entry_edx = result.final_edx;
    return result;
}

[[nodiscard]] std::array<u32, 8>
action_arguments(const u32 byte_token, const u32 word_token) noexcept {
    std::array<u32, 8> result{};
    result[0] = byte_token;
    result[1] = word_token;
    return result;
}

}  // namespace

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActionDispatchPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&port](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        const auto reply = port.invoke({
            .callee_token = kCallQueryActorMetric,
            .arguments = action_arguments(byte_token, word_token),
            .eax = registers.eax,
            .ecx = actor_token,
            .edx = registers.edx,
        });
        return InvocationReply{
            .registers = {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx},
            .publish_metric_byte = reply.publish_metric_byte,
            .metric_byte = reply.metric_byte,
            .publish_metric_word = reply.publish_metric_word,
            .metric_word = reply.metric_word,
            .publish_group_b_count = reply.publish_group_b_count,
            .group_b_count = reply.group_b_count,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
        };
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleStartupPort& port,
    const compat::u32 group_b_count,
    const compat::u32 group_a_count
) {
    auto& state = port.actor_metric_state();
    state.group_b_count = group_b_count;
    state.group_a_count = group_a_count;
    auto call = [&port](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        const auto reply = port.invoke({
            .call = LegacyBattleStartupCall::query_actor_metric,
            .arguments = {byte_token, word_token, 0U, 0U},
            .eax = registers.eax,
            .ecx = actor_token,
            .edx = registers.edx,
        });
        return InvocationReply{
            .registers =
                {.eax = reply.return_value,
                 .ecx = reply.ecx_snapshot,
                 .edx = reply.edx_snapshot},
            .publish_metric_byte = reply.publish_metric_byte,
            .metric_byte = reply.metric_byte,
            .publish_metric_word = reply.publish_metric_word,
            .metric_word = reply.metric_word,
            .publish_group_b_count = reply.publish_group_b_count,
            .group_b_count = reply.group_b_count,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
        };
    };
    return rebuild_impl(state, call);
}

LegacyBattleActorMetricResult
rebuild_legacy_battle_actor_metrics(LegacyBattleFrameCoordinatorPort& port) {
    auto& state = port.actor_metric_state();
    auto call = [&port](
                    const u32 actor_token,
                    const u32 byte_token,
                    const u32 word_token,
                    const Registers registers
                ) {
        const auto reply = port.invoke({
            .call = LegacyBattleFrameCoordinatorCall::query_actor_metric,
            .arguments = {byte_token, word_token, 0U, 0U, 0U, 0U, 0U, 0U},
            .eax = registers.eax,
            .ecx = actor_token,
            .edx = registers.edx,
        });
        return InvocationReply{
            .registers = {.eax = reply.eax, .ecx = reply.ecx, .edx = reply.edx},
            .publish_metric_byte = reply.publish_metric_byte,
            .metric_byte = reply.metric_byte,
            .publish_metric_word = reply.publish_metric_word,
            .metric_word = reply.metric_word,
            .publish_group_b_count = reply.publish_group_b_count,
            .group_b_count = reply.group_b_count,
            .publish_group_a_count = reply.publish_group_a_count,
            .group_a_count = reply.group_a_count,
        };
    };
    return rebuild_impl(state, call);
}

}  // namespace openswd3::battle
