#pragma once

#include "openswd3/battle/legacy_battle_actor_coordinates.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <functional>
#include <utility>
#include <variant>

namespace openswd3::battle {

class LegacyBattleActionDispatchPort;
class LegacyBattleFrameCoordinatorPort;
class LegacyBattleStartupPort;
struct LegacyBattleStartupState;

struct LegacyBattleActorMetricState {
    std::array<compat::i32, 18> values{};
    std::array<compat::u32, 18> actor_order{};
    std::array<compat::u32, 18> selected_mask{};
    std::array<compat::u32, 8> group_b_order{};

    compat::u32 group_b_count{};
    compat::u32 group_a_count{};
    compat::u32 local_word_token{};
    compat::u32 local_byte_token{};
    compat::u16 local_word{};
    compat::u16 local_byte{};
    // Diagnostic knowledge only; neither bit changes game control flow.
    bool local_words_known{true};
    bool entry_registers_known{true};

    compat::u32 entry_eax{};
    compat::u32 entry_ecx{};
    compat::u32 entry_edx{};
    LegacyBattleActorCoordinateFlags entry_flags{};

    compat::u8 priority_update_gate{};
    compat::u32 group_a_mode{};
    compat::u32 group_b_mode{};
    compat::u32 priority_actor_index{0xFFFFFFFFU};
    std::array<compat::u32, 6> priority_actor_record_tail{};
    compat::u32 priority_order_ready{};
    compat::u32 pending_action_activation_latch{};
};

struct LegacyBattleActorPublicationState {
    LegacyBattleActorPublicationState() {
        slots.fill(0xFFFFFFFFU);
    }

    std::array<compat::u32, 18> slots{};
};

class LegacyBattleActorPublicationStatePort {
public:
    [[nodiscard]] virtual LegacyBattleActorPublicationState&
    actor_publication_state() noexcept {
        return actor_publication_state_;
    }

    [[nodiscard]] virtual const LegacyBattleActorPublicationState&
    actor_publication_state() const noexcept {
        return actor_publication_state_;
    }

protected:
    LegacyBattleActorPublicationStatePort() = default;
    ~LegacyBattleActorPublicationStatePort() = default;

private:
    LegacyBattleActorPublicationState actor_publication_state_{};
};

class LegacyBattleActorMetricStatePort {
public:
    [[nodiscard]] virtual LegacyBattleActorMetricState&
    actor_metric_state() noexcept {
        if (auto* state = std::get_if<LegacyBattleActorMetricState>(
                &actor_metric_state_)) {
            return *state;
        }

        return std::get<std::reference_wrapper<LegacyBattleActorMetricState>>(
                   actor_metric_state_)
            .get();
    }

    [[nodiscard]] virtual const LegacyBattleActorMetricState&
    actor_metric_state() const noexcept {
        if (const auto* state = std::get_if<LegacyBattleActorMetricState>(
                &actor_metric_state_)) {
            return *state;
        }

        return std::get<std::reference_wrapper<LegacyBattleActorMetricState>>(
                   actor_metric_state_)
            .get();
    }

    // Bind before retaining state references. The borrowed object must
    // outlive subsequent port calls; binding never copies game fields.
    void borrow_actor_metric_state(LegacyBattleActorMetricState& state) noexcept {
        if (std::get_if<LegacyBattleActorMetricState>(&actor_metric_state_) ==
            &state) {
            return;
        }

        actor_metric_state_.emplace<1U>(std::ref(state));
    }

protected:
    LegacyBattleActorMetricStatePort() = default;

    explicit LegacyBattleActorMetricStatePort(
        LegacyBattleActorMetricState& state
    ) noexcept
        : actor_metric_state_(std::in_place_index<1U>, std::ref(state)) {}

    ~LegacyBattleActorMetricStatePort() = default;

private:
    std::variant<
        LegacyBattleActorMetricState,
        std::reference_wrapper<LegacyBattleActorMetricState>>
        actor_metric_state_{};
};

enum class LegacyBattleActorMetricStatus : compat::u8 {
    completed,
    actor_coordinate_typed_stop,
    value_store_typed_stop,
};

enum class LegacyBattleActorOrderStatus : compat::u8 {
    completed,
    metric_read_typed_stop,
    mask_access_typed_stop,
    order_store_typed_stop,
};

struct LegacyBattleActorMetricResult {
    // False means the tuple is not determined by the supplied model inputs.
    // True is not evidence of an original-run CPU capture. Game table writes
    // can complete after both local outputs are replaced.
    bool final_registers_known{true};
    LegacyBattleActorMetricStatus status{
        LegacyBattleActorMetricStatus::completed
    };
    compat::u32 return_value{};
    compat::u32 final_ecx{};
    compat::u32 final_edx{};
    LegacyBattleActorCoordinateFlags final_flags{};
    LegacyBattleActorCoordinateQueryResult coordinate_query{};
    compat::u32 coordinate_query_calls{};
    compat::u32 port_calls{};
    compat::u32 group_b_iterations{};
    compat::u32 group_a_iterations{};
};

struct LegacyBattleActorOrderResult {
    // Diagnostic validity, with the same meaning as the metric result.
    bool final_registers_known{true};
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

// 0x0045B0E4..0x0045B0FE: two forward 18-dword clears, before the
// first group-B count read. The counts and adjacent state are untouched.
void clear_legacy_battle_actor_metric_tables(
    LegacyBattleActorMetricState& state
) noexcept;

enum class LegacyBattleMetricFirstCountStatus : compat::u8 {
    read_group_a_count,
    query_first_group_b_actor,
};

// 0x0045B0FE..0x0045B11F: read the live startup count, never the metric
// port's unbound default copy. Both branches stop before their next read/CALL.
[[nodiscard]] LegacyBattleMetricFirstCountStatus
probe_legacy_battle_metric_first_count(
    const compat::u32& group_b_count
) noexcept;

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActionDispatchPort& port,
    compat::u32 group_b_count,
    compat::u32 group_a_count,
    const LegacyBattleActorCoordinateOwners& owners
);

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleActorMetricState& state,
    const LegacyBattleActorCoordinateOwners& owners
);

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleStartupPort& port,
    const LegacyBattleActorCoordinateOwners& owners
);

[[nodiscard]] LegacyBattleActorMetricResult rebuild_legacy_battle_actor_metrics(
    LegacyBattleFrameCoordinatorPort& port,
    const LegacyBattleActorCoordinateOwners& owners
);

[[nodiscard]] LegacyBattleActorOrderResult rebuild_legacy_battle_actor_order(
    LegacyBattleActorMetricState& state,
    compat::u32 group_b_count,
    compat::u32 group_a_count,
    compat::u32 caller_edx = 0U,
    bool caller_edx_known = true
);

}  // namespace openswd3::battle
