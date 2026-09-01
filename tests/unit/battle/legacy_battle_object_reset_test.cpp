#include "openswd3/battle/legacy_battle_object_reset.hpp"

#include <algorithm>
#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorObjectResetRequest;
using openswd3::battle::LegacyBattleFixedObjectResetStatus;
using openswd3::battle::LegacyBattleObjectResetCallReply;
using openswd3::compat::u32;

struct Event {
    u32 kind{};
    u32 token{};

    [[nodiscard]] bool operator==(const Event&) const = default;
};

class TrackingObjectResetPorts final
    : public openswd3::battle::LegacyBattleGlobalResetPort,
      public openswd3::battle::LegacyBattleFixedObjectStatePort,
      public openswd3::battle::LegacyBattleActorObjectResetPort {
public:
    [[nodiscard]] LegacyBattleObjectResetCallReply
    reset_global_state() override {
        events.push_back(Event{.kind = 1U});
        return global_reply;
    }

    [[nodiscard]] LegacyBattleObjectResetCallReply reset_actor_object(
        const LegacyBattleActorObjectResetRequest& request
    ) override {
        events.push_back(Event{.kind = 3U, .token = request.actor_token});
        actor_requests.push_back(request);
        if (observed_state != nullptr && actor_requests.size() == 1U) {
            table_was_clear_before_actor_loop =
                std::ranges::all_of(observed_state->table, [](const u32 word) {
                    return word == 0U;
                });
            fixed_objects_were_clear_before_actor_loop = std::ranges::all_of(
                legacy_battle_fixed_object_state().object_words,
                [](const auto& words) {
                    return std::ranges::all_of(words, [](const u32 word) {
                        return word == 0U;
                    });
                }
            );
        }
        return {
            .eax = request.actor_token ^ 0xA5A5A5A5U,
            .ecx = request.actor_token ^ 0x5A5A5A5AU,
            .edx = request.edx + 1U,
        };
    }

    openswd3::battle::LegacyBattleObjectResetState* observed_state{};
    LegacyBattleObjectResetCallReply global_reply{
        .eax = 0x12345678U,
        .ecx = 0x23456789U,
        .edx = 0x3456789AU,
    };
    std::vector<Event> events;
    std::vector<LegacyBattleActorObjectResetRequest> actor_requests;
    bool table_was_clear_before_actor_loop{};
    bool fixed_objects_were_clear_before_actor_loop{};
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
    for (auto& words : ports.legacy_battle_fixed_object_state().object_words) {
        words.fill(0xC0DEC0DEU);
    }

    const auto result = openswd3::battle::reset_legacy_battle_objects(
        state, ports, ports, ports
    );
    const std::vector<u32> actor_tokens = expected_actor_tokens();
    const u32 last_actor_token = actor_tokens.back();

    bool fixed_resets_match = true;
    for (std::size_t index = 0U; index < result.fixed_object_resets.size();
         ++index) {
        const auto& reset = result.fixed_object_resets[index];
        fixed_resets_match = fixed_resets_match &&
            reset.status == LegacyBattleFixedObjectResetStatus::completed &&
            reset.object_token ==
                openswd3::battle::kLegacyBattleFixedResetObjectTokens[index] &&
            reset.dword_writes == 5U && reset.return_eax == 0U &&
            reset.return_ecx ==
                openswd3::battle::kLegacyBattleFixedResetObjectTokens[index] &&
            reset.return_edx == ports.global_reply.edx;
    }

    bool actor_registers_threaded =
        ports.actor_requests.size() == actor_tokens.size();
    u32 expected_eax = 0U;
    u32 expected_edx = ports.global_reply.edx;
    for (std::size_t index = 0U;
         actor_registers_threaded && index < actor_tokens.size();
         ++index) {
        const auto& request = ports.actor_requests[index];
        const u32 token = actor_tokens[index];
        actor_registers_threaded = request.actor_token == token &&
            request.eax == expected_eax && request.ecx == token &&
            request.edx == expected_edx;
        expected_eax = token ^ 0xA5A5A5A5U;
        ++expected_edx;
    }

    test.expect_true(
        result.global_reset_calls == 1U &&
            result.global_reset_reply.eax == 0x12345678U &&
            result.global_reset_reply.ecx == 0x23456789U &&
            result.global_reset_reply.edx == 0x3456789AU &&
            result.fixed_object_tokens ==
                openswd3::battle::kLegacyBattleFixedResetObjectTokens &&
            result.fixed_object_reset_calls == 3U && fixed_resets_match &&
            result.table_dword_writes == 0x60U &&
            ports.fixed_objects_were_clear_before_actor_loop &&
            ports.table_was_clear_before_actor_loop &&
            result.group_b_reset_calls == 8U &&
            result.group_a_reset_calls == 10U && actor_registers_threaded &&
            ports.events.size() == 19U && ports.events[0].kind == 1U &&
            ports.events[1] ==
                Event{
                    .kind = 3U,
                    .token =
                        openswd3::battle::kLegacyBattleActorGroupBBaseToken,
                } &&
            ports.events[9] ==
                Event{
                    .kind = 3U,
                    .token =
                        openswd3::battle::kLegacyBattleActorGroupABaseToken,
                } &&
            result.return_value == (last_actor_token ^ 0xA5A5A5A5U) &&
            result.return_ecx == (last_actor_token ^ 0x5A5A5A5AU) &&
            result.return_edx ==
                ports.global_reply.edx + static_cast<u32>(actor_tokens.size()),
        "battle object reset directly clears three fixed owners, clears the table, preserves group order, and threads registers"
    );
}
