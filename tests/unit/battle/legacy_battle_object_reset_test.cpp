#include "openswd3/battle/legacy_battle_object_reset.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::compat::u32;

struct Event {
    u32 kind{};
    u32 token{};

    [[nodiscard]] bool operator==(const Event&) const = default;
};

class TrackingObjectResetPorts final
    : public openswd3::battle::LegacyBattleGlobalResetPort,
      public openswd3::battle::LegacyBattleFixedObjectResetPort,
      public openswd3::battle::LegacyBattleActorObjectResetPort {
public:
    [[nodiscard]] u32 reset_global_state() override {
        events.push_back(Event{.kind = 1U});
        return global_return;
    }

    [[nodiscard]] u32 reset_fixed_object(const u32 object_token) override {
        events.push_back(Event{.kind = 2U, .token = object_token});
        fixed_tokens.push_back(object_token);
        return object_token ^ 0xFFFFFFFFU;
    }

    [[nodiscard]] u32 reset_actor_object(const u32 actor_token) override {
        events.push_back(Event{.kind = 3U, .token = actor_token});
        actor_tokens.push_back(actor_token);
        if (observed_state != nullptr && actor_tokens.size() == 1U) {
            table_was_clear_before_actor_loop =
                std::ranges::all_of(observed_state->table, [](const u32 word) {
                    return word == 0U;
                });
        }
        return actor_token ^ 0xA5A5A5A5U;
    }

    openswd3::battle::LegacyBattleObjectResetState* observed_state{};
    u32 global_return{0x12345678U};
    std::vector<Event> events;
    std::vector<u32> fixed_tokens;
    std::vector<u32> actor_tokens;
    bool table_was_clear_before_actor_loop{};
};

[[nodiscard]] std::vector<u32> expected_actor_tokens() {
    std::vector<u32> tokens;
    for (u32 index = 0U;
         index < openswd3::battle::kLegacyBattleActorGroupBElementCount;
         ++index) {
        tokens.push_back(
            openswd3::battle::kLegacyBattleActorGroupBBaseToken +
            openswd3::battle::kLegacyBattleActorGroupBElementSize * index
        );
    }
    for (u32 index = 0U;
         index < openswd3::battle::kLegacyBattleActorGroupAElementCount;
         ++index) {
        tokens.push_back(
            openswd3::battle::kLegacyBattleActorGroupABaseToken +
            openswd3::battle::kLegacyBattleActorGroupAElementSize * index
        );
    }
    return tokens;
}

}  // namespace

void test_battle_object_reset(openswd3::test::Context& test) {
    openswd3::battle::LegacyBattleObjectResetState state;
    state.table.fill(0xDEADBEEFU);
    TrackingObjectResetPorts ports;
    ports.observed_state = &state;

    const auto result = openswd3::battle::reset_legacy_battle_objects(
        state, ports, ports, ports
    );
    const std::vector<u32> actor_tokens = expected_actor_tokens();
    const u32 last_actor_token = actor_tokens.back();

    test.expect_true(
        result.global_reset_calls == 1U &&
            result.global_reset_return_snapshot == 0x12345678U &&
            result.fixed_object_tokens ==
                openswd3::battle::kLegacyBattleFixedResetObjectTokens &&
            result.fixed_object_reset_calls == 3U &&
            ports.fixed_tokens ==
                std::vector<u32>{
                    0x004B9F00U,
                    0x004ACBA8U,
                    0x004B8A00U,
                } &&
            result.fixed_object_return_snapshots ==
                std::array<u32, 3>{
                    0xFFB460FFU,
                    0xFFB53457U,
                    0xFFB475FFU,
                } &&
            result.table_dword_writes == 0x60U &&
            ports.table_was_clear_before_actor_loop &&
            result.group_b_reset_calls == 8U &&
            result.group_a_reset_calls == 10U &&
            ports.actor_tokens == actor_tokens && ports.events.size() == 22U &&
            ports.events[0].kind == 1U &&
            ports.events[1] == Event{.kind = 2U, .token = 0x004B9F00U} &&
            ports.events[3] == Event{.kind = 2U, .token = 0x004B8A00U} &&
            ports.events[4] ==
                Event{
                    .kind = 3U,
                    .token =
                        openswd3::battle::kLegacyBattleActorGroupBBaseToken,
                } &&
            ports.events[12] ==
                Event{
                    .kind = 3U,
                    .token =
                        openswd3::battle::kLegacyBattleActorGroupABaseToken,
                } &&
            result.return_value == (last_actor_token ^ 0xA5A5A5A5U),
        "battle object reset preserves fixed calls table clear group order and final eax"
    );
}
