#include "openswd3/battle/legacy_battle_group_b_target_cycle.hpp"

#include <bit>

namespace openswd3::battle {
namespace {

using compat::i32;
using compat::u32;
using Call = LegacyBattleTargetSelectionRuntimeCall;
using Status = LegacyBattleGroupBTargetCycleStatus;

inline constexpr u32 kGroupBBaseToken = 0x00525508U;
inline constexpr u32 kGroupBStride = 0x2B28U;
inline constexpr u32 kGroupBCount = 8U;

[[nodiscard]] constexpr i32 signed_bits(const u32 value) noexcept {
    return std::bit_cast<i32>(value);
}

[[nodiscard]] constexpr u32 group_b_token(const u32 index) noexcept {
    return kGroupBBaseToken + index * kGroupBStride;
}

class GroupBTargetCycleMachine {
public:
    GroupBTargetCycleMachine(
        const LegacyBattleGroupBTargetCycleBindings bindings,
        LegacyBattleTargetSelectionRuntimePort& port,
        const LegacyBattleGroupBTargetCycleRequest& request
    )
        : bindings_(bindings), port_(port), eax_(request.entry_eax),
          ecx_(request.entry_ecx), edx_(request.entry_edx) {}

    [[nodiscard]] LegacyBattleGroupBTargetCycleResult run() {
        ecx_ = bindings_.frame_input.target_actor_index;
        eax_ = bindings_.metrics.group_b_count;
        scanned_ = 0U;
        if (signed_bits(ecx_) >= signed_bits(eax_)) {
            ecx_ = 0U;
            bindings_.frame_input.target_actor_index = ecx_;
        }

        const u32 initial_index = ecx_;
        eax_ = initial_index * 0x159U;
        ecx_ = group_b_token(initial_index);
        if (initial_index >= kGroupBCount) {
            stop(Status::group_b_actor_typed_stop);
            return finish();
        }
        invoke_completion(initial_index);
        if (eax_ == 1U) {
            while (true) {
                eax_ = bindings_.frame_input.target_cursor;
                ecx_ = bindings_.metrics.group_b_count;
                ++eax_;
                bindings_.frame_input.target_cursor = eax_;
                if (signed_bits(eax_) > signed_bits(ecx_)) {
                    eax_ = 1U;
                    bindings_.frame_input.target_cursor = eax_;
                }

                if (eax_ >=
                    bindings_.target_runtime.target_actor_indices.size()) {
                    stop(Status::target_order_typed_stop);
                    return finish();
                }
                eax_ = bindings_.target_runtime.target_actor_indices[eax_];
                ++result_.target_order_reads;
                ++scanned_;
                result_.skipped_targets = scanned_;
                bindings_.frame_input.target_actor_index = eax_;
                if (signed_bits(scanned_) >= signed_bits(ecx_)) {
                    bindings_.message_state = 1U;
                    break;
                }

                const u32 candidate = eax_;
                edx_ = candidate * 0x159U;
                eax_ = candidate * 0x565U;
                ecx_ = group_b_token(candidate);
                if (candidate >= kGroupBCount) {
                    stop(Status::group_b_actor_typed_stop);
                    return finish();
                }
                invoke_completion(candidate);
                if (eax_ != 1U) {
                    break;
                }
            }
        }

        const u32 reset_index = bindings_.frame_input.target_actor_index;
        eax_ = reset_index;
        edx_ = reset_index * 0x565U;
        ecx_ = group_b_token(reset_index);
        if (reset_index >= kGroupBCount) {
            stop(Status::group_b_actor_typed_stop);
            return finish();
        }
        const auto reset = port_.invoke_target_selection_runtime({
            .call = Call::reset_actor_selection,
            .actor_token = ecx_,
            .arguments = {1U},
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        ++result_.reset_calls;
        eax_ = reset.eax;
        ecx_ = reset.ecx;
        edx_ = reset.edx;

        eax_ = bindings_.frame_input.target_actor_index;
        bindings_.target_runtime.selection_input_gate = 1U;
        ++eax_;
        bindings_.final_actor.published_actor_code = eax_;
        return finish();
    }

private:
    void invoke_completion(const u32 actor_index) {
        const auto reply = port_.invoke_target_selection_runtime({
            .call = Call::query_group_b_completion,
            .actor_token = group_b_token(actor_index),
            .eax = eax_,
            .ecx = ecx_,
            .edx = edx_,
        });
        ++result_.port_calls;
        ++result_.completion_queries;
        eax_ = reply.eax;
        ecx_ = reply.ecx;
        edx_ = reply.edx;
    }

    void stop(const Status status) noexcept {
        result_.status = status;
    }

    [[nodiscard]] LegacyBattleGroupBTargetCycleResult finish() {
        result_.return_eax = eax_;
        result_.return_ecx = ecx_;
        result_.return_edx = edx_;
        return result_;
    }

    LegacyBattleGroupBTargetCycleBindings bindings_;
    LegacyBattleTargetSelectionRuntimePort& port_;
    LegacyBattleGroupBTargetCycleResult result_{};
    u32 eax_{};
    u32 ecx_{};
    u32 edx_{};
    u32 scanned_{};
};

}  // namespace

LegacyBattleGroupBTargetCycleResult cycle_legacy_battle_group_b_target(
    const LegacyBattleGroupBTargetCycleBindings bindings,
    LegacyBattleTargetSelectionRuntimePort& port,
    const LegacyBattleGroupBTargetCycleRequest& request
) {
    return GroupBTargetCycleMachine(bindings, port, request).run();
}

}  // namespace openswd3::battle
