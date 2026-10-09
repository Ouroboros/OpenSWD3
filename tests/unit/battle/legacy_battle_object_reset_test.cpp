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

class TrackingObjectResetPorts final
    : public openswd3::battle::LegacyBattleFixedObjectState,
      public openswd3::battle::LegacyBattleActorObjectResetPort {
public:
    [[nodiscard]] LegacyBattleObjectResetCallReply reset_actor_object(
        const LegacyBattleActorObjectResetRequest& request
    ) override {
        actor_requests.push_back(request);
        if (observed_state != nullptr && actor_requests.size() == 1U) {
            table_was_clear_before_actor_loop =
                std::ranges::all_of(observed_state->table, [](const u32 word) {
                    return word == 0U;
                });
            fixed_objects_were_clear_before_actor_loop =
                std::ranges::all_of(this->object_words, [](const auto& words) {
                    return std::ranges::all_of(words, [](const u32 word) {
                        return word == 0U;
                    });
                });
        }
        return {
            .eax = request.actor_token ^ 0xA5A5A5A5U,
            .ecx = request.actor_token ^ 0x5A5A5A5AU,
            .edx = request.edx + 1U,
        };
    }

    openswd3::battle::LegacyBattleObjectResetState* observed_state{};
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
    u32 node_token = 0x73000000U;
    for (auto& words : ports.object_words) {
        words.fill(0xC0DEC0DEU);
        words[0U] = node_token++;
        ports.fixed_count_nodes.push_back({.legacy_token = words[0U]});
    }

    const auto result =
        openswd3::battle::reset_legacy_battle_objects(state, ports, ports);
    const std::vector<u32> actor_tokens = expected_actor_tokens();
    const u32 last_actor_token = actor_tokens.back();

    bool fixed_resets_match = true;
    for (std::size_t index = 0U; index < result.fixed_object_resets.size();
         ++index) {
        const auto& reset = result.fixed_object_resets[index];
        fixed_resets_match = fixed_resets_match &&
            reset.status == LegacyBattleFixedObjectResetStatus::completed &&
            std::ranges::all_of(
                ports.object_words[index],
                [](const u32 word) { return word == 0U; }
            );
    }

    bool actor_registers_threaded =
        ports.actor_requests.size() == actor_tokens.size();
    u32 expected_eax = 0U;
    u32 expected_edx = 0U;
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
        result.fixed_chain_release.status ==
                openswd3::battle::LegacyBattleFixedChainReleaseStatus::
                    completed &&
            result.fixed_chain_release.nodes_released == 3U &&
            ports.fixed_count_nodes.empty() && fixed_resets_match &&
            result.table_dword_writes == 0x60U &&
            ports.fixed_objects_were_clear_before_actor_loop &&
            ports.table_was_clear_before_actor_loop &&
            result.group_b_reset_calls == 8U &&
            result.group_a_reset_calls == 10U && actor_registers_threaded &&
            result.return_value == (last_actor_token ^ 0xA5A5A5A5U) &&
            result.return_ecx == (last_actor_token ^ 0x5A5A5A5AU) &&
            result.return_edx == static_cast<u32>(actor_tokens.size()),
        "battle object reset releases fixed chains, clears the table, and preserves actor order"
    );

    openswd3::battle::LegacyBattleObjectResetState failed_state;
    failed_state.table.fill(0x12345678U);
    TrackingObjectResetPorts failed_ports;
    failed_ports.fixed_count_nodes.push_back({.legacy_token = 100U});
    failed_ports.object_words[1U] = {100U, 1U, 2U, 3U, 4U};
    failed_ports.object_words[0U] = {200U, 5U, 6U, 7U, 8U};
    failed_ports.object_words[2U] = {0U, 9U, 10U, 11U, 12U};
    const auto before = failed_ports.object_words;
    const auto failed = openswd3::battle::reset_legacy_battle_objects(
        failed_state, failed_ports, failed_ports
    );
    test.expect_true(
        failed.fixed_chain_release.status ==
                openswd3::battle::LegacyBattleFixedChainReleaseStatus::
                    invalid_chain &&
            failed.fixed_chain_release.stopped_node == 200U &&
            failed_ports.fixed_count_nodes.empty() &&
            failed_ports.object_words[1U] == std::array<u32, 5U>{} &&
            failed_ports.object_words[0U] == before[0U] &&
            failed_ports.object_words[2U] == before[2U] &&
            failed_ports.actor_requests.empty() &&
            std::ranges::all_of(
                failed_state.table,
                [](const u32 word) { return word == 0x12345678U; }
            ),
        "a failed chain release preserves its prefix and blocks table and actor reset"
    );
}
