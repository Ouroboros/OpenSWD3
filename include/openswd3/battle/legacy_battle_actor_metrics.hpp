#pragma once

#include "openswd3/compat/types.hpp"

#include <array>

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
class LegacyBattleFrameCoordinatorPort;
class LegacyBattleStartupPort;

struct LegacyBattleActorMetricState {
    std::array<compat::i32, 18> values{};
    std::array<compat::u32, 18> actor_order{};
    std::array<compat::u32, 18> selected_mask{};

    compat::u32 group_b_count{};
    compat::u32 group_a_count{};
    compat::u32 local_word_token{};
    compat::u32 local_byte_token{};
    compat::u16 local_word{};
    compat::u8 local_byte{};

    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
};

class LegacyBattleActorMetricStatePort {
public:
    [[nodiscard]] LegacyBattleActorMetricState& actor_metric_state() noexcept {
        return actor_metric_state_;
    }

    [[nodiscard]] const LegacyBattleActorMetricState&
    actor_metric_state() const noexcept {
        return actor_metric_state_;
    }

protected:
    LegacyBattleActorMetricStatePort() = default;
    ~LegacyBattleActorMetricStatePort() = default;

private:
    LegacyBattleActorMetricState actor_metric_state_{};
};

enum class LegacyBattleActorMetricStatus : compat::u8 {
    completed,
    value_store_typed_stop,
};

enum class LegacyBattleActorOrderStatus : compat::u8 {
    completed,
    metric_read_typed_stop,
    mask_access_typed_stop,
    order_store_typed_stop,
};

struct LegacyBattleActorMetricResult {
    LegacyBattleActorMetricStatus status{
        LegacyBattleActorMetricStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 port_calls{};
    compat::u32 group_b_iterations{};
    compat::u32 group_a_iterations{};
};

struct LegacyBattleActorOrderResult {
    LegacyBattleActorOrderStatus status{
        LegacyBattleActorOrderStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    compat::u32 selections{};
    compat::u32 metric_reads{};
    compat::u32 mask_reads{};
    compat::u32 mask_writes{};
};

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActionDispatchPort& port,
    compat::u32 group_b_count,
    compat::u32 group_a_count
);

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleStartupPort& port,
    compat::u32 group_b_count,
    compat::u32 group_a_count
);

[[nodiscard]] LegacyBattleActorMetricResult
rebuild_legacy_battle_actor_metrics(LegacyBattleFrameCoordinatorPort& port);

[[nodiscard]] LegacyBattleActorOrderResult rebuild_legacy_battle_actor_order(
    LegacyBattleActorMetricState& state,
    compat::u32 group_b_count,
    compat::u32 group_a_count,
    compat::u32 caller_edx = 0U
);

}  // namespace openswd3::battle
