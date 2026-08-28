#include "openswd3/battle/legacy_battle_growth_actor_selection.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleGrowthActorSelectionCall;
using openswd3::battle::LegacyBattleGrowthActorSelectionCallReply;
using openswd3::battle::LegacyBattleGrowthActorSelectionCallRequest;
using openswd3::battle::LegacyBattleGrowthActorSelectionRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_u16(
    std::array<u8, 160U>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    std::array<u8, 160U>& bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] std::array<u8, 160U>
definition(const std::vector<u8>& name, const u16 code, const u32 limit) {
    std::array<u8, 160U> bytes{};
    std::copy(name.begin(), name.end(), bytes.begin());
    if (name.size() < bytes.size()) {
        bytes[name.size()] = 0U;
    }
    write_u16(bytes, openswd3::battle::kLegacyBattleGrowthItemCodeOffset, code);
    write_u32(
        bytes, openswd3::battle::kLegacyBattleGrowthItemLimitOffset, limit
    );
    return bytes;
}

class Port final
    : public openswd3::battle::LegacyBattleGrowthActorSelectionPort {
public:
    [[nodiscard]] LegacyBattleGrowthActorSelectionCallReply
    invoke_growth_actor_selection(
        const LegacyBattleGrowthActorSelectionCallRequest& request
    ) override {
        calls.push_back(request);
        auto reply =
            LegacyBattleGrowthActorSelectionPort::invoke_growth_actor_selection(
                request
            );
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            reply = found->second[index++];
        } else if (
            request.call ==
            LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block
        ) {
            reply.eax = 0U;
        }
        if (request.call ==
            LegacyBattleGrowthActorSelectionCall::load_item_definition) {
            const auto record = definitions.find(request.item_id);
            if (record != definitions.end()) {
                reply.publish_definition = true;
                reply.definition = record->second;
            }
        }
        return reply;
    }

    void reply(
        const LegacyBattleGrowthActorSelectionCall call,
        const LegacyBattleGrowthActorSelectionCallReply& value
    ) {
        replies[call].push_back(value);
    }

    [[nodiscard]] u32
    count(const LegacyBattleGrowthActorSelectionCall call) const noexcept {
        return static_cast<u32>(std::count_if(
            calls.begin(), calls.end(), [call](const auto& request) {
                return request.call == call;
            }
        ));
    }

    std::vector<LegacyBattleGrowthActorSelectionCallRequest> calls;
    std::map<
        LegacyBattleGrowthActorSelectionCall,
        std::vector<LegacyBattleGrowthActorSelectionCallReply>>
        replies;
    std::map<LegacyBattleGrowthActorSelectionCall, std::size_t> reply_indices;
    std::map<u16, std::array<u8, 160U>> definitions;
};

struct Fixture {
    Fixture() {
        metrics.group_a_count = 1U;
        startup.action_mode_source.actor_label_indices[0U] = 0U;
        victory.party_growth_item_codes[0U] = 0U;
        target.transition_actor_index = 0xFFU;
        advancement.growth_caption_text.fill(0xCCU);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGrowthActorSelectionBindings
    bindings() {
        return {
            .victory = victory,
            .startup = startup,
            .metrics = metrics,
            .target_selection = target,
            .level_advancement = advancement,
        };
    }

    openswd3::battle::LegacyBattleVictoryRewardState victory;
    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleActorMetricState metrics;
    openswd3::battle::LegacyBattleTargetSelectionRuntimeState target;
    openswd3::battle::LegacyBattleLevelAdvancementState advancement;
    Port port;
};

openswd3::world_map::LegacyWorldItemNode& add_item(
    Fixture& fixture,
    const std::size_t list_index,
    const u16 item_id,
    const u16 item_type = openswd3::battle::kLegacyBattleGrowthItemType
) {
    auto& optional =
        fixture.port.world_item_list_state().party_item_lists[list_index];
    if (!optional.has_value()) {
        optional.emplace();
    }
    auto& list = *optional;
    list.nodes.emplace_back();
    auto& node = list.nodes.back();
    node.item_id = item_id;
    node.legacy_token = 0x71000000U + static_cast<u32>(list_index) * 0x1000U +
        static_cast<u32>(list.nodes.size()) * 0xB0U;
    write_u16(
        node.definition_snapshot,
        openswd3::battle::kLegacyBattleGrowthItemTypeOffset,
        item_type
    );
    if (list.nodes.size() == 1U) {
        list.sentinel.legacy_next_token = node.legacy_token;
    }
    return node;
}

[[nodiscard]] openswd3::battle::LegacyBattleGrowthActorSelectionResult
run(Fixture& fixture,
    const LegacyBattleGrowthActorSelectionRequest& request = {
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0x55556666U,
    }) {
    return openswd3::battle::advance_legacy_battle_growth_actor_selection(
        fixture.bindings(), fixture.port, request
    );
}

}  // namespace

void test_battle_growth_actor_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGrowthActorSelectionStatus;

    {
        const openswd3::battle::LegacyBattleVictoryRewardState initial;
        test.expect_true(
            std::ranges::all_of(
                initial.party_growth_item_codes,
                [](const u32 value) { return value == 0U; }
            ),
            "growth actor profiles begin at the authoritative zero-initialized PE state"
        );
    }

    {
        Fixture fixture;
        add_item(fixture, 0U, 0x0700U);
        fixture.port.definitions[0x0700U] =
            definition({0xA4U, 0x40U}, 0x1234U, 2U);
        fixture.port.definitions[0x1234U] =
            definition({0xA4U, 0x40U}, 0x1234U, 2U);
        fixture.victory.party_reward_counters[0U] = 2U;

        const auto result = run(fixture);
        const auto& list =
            *fixture.port.world_item_list_state().party_item_lists[0U];
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::completed &&
                result.port_calls == 6U && result.actor_query_calls == 1U &&
                result.item_load_calls == 3U &&
                result.item_release_calls == 2U &&
                result.item_presence_calls == 1U &&
                result.allocation_calls == 1U &&
                result.matching_item_count == 1U &&
                result.selected_actor_count == 1U && result.return_eax == 1U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x80001234U &&
                fixture.target.transition_mode == 1U &&
                fixture.target.transition_actor_index == 0U &&
                fixture.victory.party_reward_counters[0U] == 0U &&
                fixture.victory.party_growth_limits[0U] == 2U &&
                fixture.victory.party_growth_item_codes[0U] == 0x80001234U &&
                fixture.advancement.growth_caption_text[0U] == 0xA4U &&
                fixture.advancement.growth_caption_text[1U] == 0x40U &&
                fixture.advancement.growth_caption_text[2U] == 0U &&
                fixture.advancement.growth_caption_text[3U] == 0xCCU &&
                list.nodes.size() == 2U &&
                list.nodes.back().item_id == 0x1234U &&
                list.nodes.back().legacy_token == 0x70010000U,
            "growth actor selection appends a zeroed typed item, copies its caption and publishes the transition"
        );
        test.expect_true(
            fixture.port.calls.size() == 6U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleGrowthActorSelectionCall::
                        query_group_a_reward_block &&
                fixture.port.calls[0U].actor_token == 0x005029D0U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleGrowthActorSelectionCall::
                        load_item_definition &&
                fixture.port.calls[1U].destination_token == 0x0053BC28U &&
                fixture.port.calls[1U].item_id == 0x0700U &&
                fixture.port.calls[1U].eax == 0U &&
                fixture.port.calls[1U].ecx == 0x00500700U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleGrowthActorSelectionCall::query_item_presence &&
                fixture.port.calls[2U].arguments[0U] == 0x1BB0U &&
                fixture.port.calls[2U].eax == 0U &&
                fixture.port.calls[2U].ecx == 2U &&
                fixture.port.calls[2U].edx == 2U &&
                fixture.port.calls[5U].call ==
                    LegacyBattleGrowthActorSelectionCall::
                        load_item_definition &&
                fixture.port.calls[5U].destination_token == 0x7001000CU &&
                fixture.port.calls[5U].item_id == 0x1234U &&
                fixture.port.calls[5U].eax == 0x70010000U &&
                fixture.port.calls[5U].ecx == 0x1234U,
            "growth actor selection preserves the actor query, scratch load, presence query and allocated-record register layouts"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.startup.action_mode_source.actor_label_indices[1U] = 1U;
        fixture.victory.party_growth_item_codes[1U] = 0U;
        fixture.victory.party_reward_counters[0U] = 1U;
        fixture.victory.party_reward_counters[1U] = 1U;
        add_item(fixture, 0U, 0x0701U);
        add_item(fixture, 1U, 0x0702U);
        fixture.port.definitions[0x0701U] = definition({0x41U}, 0x1111U, 1U);
        fixture.port.definitions[0x0702U] = definition({0x42U}, 0x2222U, 1U);
        fixture.port.definitions[0x1111U] = definition({0x41U}, 0x1111U, 1U);
        fixture.port.definitions[0x2222U] = definition({0x42U}, 0x2222U, 1U);

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::completed &&
                result.selected_actor_count == 2U &&
                result.allocation_calls == 2U &&
                fixture.target.transition_actor_index == 1U &&
                fixture.advancement.growth_caption_text[0U] == 0x42U &&
                fixture.victory.party_growth_item_codes[0U] == 0x80001111U &&
                fixture.victory.party_growth_item_codes[1U] == 0x80002222U,
            "growth actor selection continues after a successful actor and leaves the last eligible actor published"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 4U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.victory.group_a_skip_secondary[1U] = 1U;
        fixture.port.reply(
            LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block,
            {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::completed &&
                result.actor_query_calls == 2U && result.port_calls == 2U &&
                result.selected_actor_count == 0U,
            "growth actor selection preserves the two exact-one skip fields, actor query gate and minus-one profile gate"
        );
    }

    {
        Fixture fixture;
        add_item(fixture, 0U, 0x0703U);
        fixture.port.definitions[0x0703U] = definition({0x43U}, 0x3333U, 1U);
        fixture.victory.party_reward_counters[0U] = 0x80000000U;

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::completed &&
                result.matching_item_count == 1U &&
                result.item_presence_calls == 0U &&
                result.selected_actor_count == 0U,
            "growth actor selection compares progress and limit as signed dwords"
        );
    }

    {
        Fixture blocked;
        add_item(blocked, 0U, 0x0665U);
        blocked.port.definitions[0x0665U] = definition({0x44U}, 0x0665U, 0U);
        blocked.port.reply(
            LegacyBattleGrowthActorSelectionCall::query_item_presence,
            {.eax = 1U}
        );
        const auto exact_one = run(blocked);

        Fixture allowed;
        add_item(allowed, 0U, 0x0665U);
        allowed.port.definitions[0x0665U] = definition({0x45U}, 0x0665U, 0U);
        allowed.port.reply(
            LegacyBattleGrowthActorSelectionCall::query_item_presence,
            {.eax = 2U}
        );
        const auto other_nonzero = run(allowed);

        test.expect_true(
            exact_one.selected_actor_count == 0U &&
                other_nonzero.selected_actor_count == 1U,
            "growth actor selection applies the two item blacklist values only when presence returns exactly one"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.startup.action_mode_source.actor_label_indices[1U] = 3U;
        fixture.victory.party_growth_item_codes[3U] = 0U;
        add_item(fixture, 1U, 0x0704U);
        fixture.port.definitions[0x0704U] = definition({0x46U}, 0x4444U, 0U);
        fixture.port.definitions[0x4444U] = definition({0x46U}, 0x4444U, 0U);

        const auto result = run(fixture);

        test.expect_true(
            result.selected_actor_count == 1U &&
                fixture.target.transition_actor_index == 1U &&
                fixture.victory.party_growth_item_codes[3U] == 0x80004444U,
            "growth actor selection preserves the original compact actor item-list index while using the mapped profile slot"
        );
    }

    {
        Fixture fixture;
        add_item(fixture, 0U, 0x0705U);
        fixture.port.definitions[0x0705U] = definition({0x47U}, 0x5555U, 0U);
        fixture.port.reply(
            LegacyBattleGrowthActorSelectionCall::allocate_item_node,
            {.eax = 0U,
             .ecx = 0xABCDEF01U,
             .edx = 0x12345678U,
             .allocation_failed = true}
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::
                        allocation_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x2CU &&
                result.return_edx == 0x12345678U &&
                fixture.port.world_item_list_state()
                        .party_item_lists[0U]
                        ->nodes.size() == 1U &&
                fixture.target.transition_mode == 0U,
            "growth actor selection stops at the first zero-allocation memset access before appending or publishing"
        );
    }

    {
        Fixture fixture;
        add_item(fixture, 0U, 0x0706U);
        std::vector<u8> long_name(24U, 0x58U);
        fixture.port.definitions[0x0706U] = definition(long_name, 0x6666U, 0U);
        fixture.port.definitions[0x6666U] = definition(long_name, 0x6666U, 0U);

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::
                        caption_destination_typed_stop &&
                result.stopped_title_index == 24U &&
                result.selected_actor_count == 0U &&
                fixture.target.transition_mode == 1U &&
                fixture.target.transition_actor_index == 0xFFU &&
                std::ranges::all_of(
                    fixture.advancement.growth_caption_text,
                    [](const u8 value) { return value == 0x58U; }
                ) &&
                fixture.port.world_item_list_state()
                        .party_item_lists[0U]
                        ->nodes.size() == 2U,
            "growth actor selection preserves allocation, record load, mode publish and 24 copied bytes before caption overflow stops"
        );
    }

    {
        Fixture fixture;
        fixture.startup.action_mode_source.actor_label_indices[0U] = 4U;
        const auto invalid_profile = run(fixture);

        Fixture missing_list;
        missing_list.port.world_item_list_state().party_item_lists[0U].reset();
        const auto missing = run(missing_list);

        test.expect_true(
            invalid_profile.status ==
                    LegacyBattleGrowthActorSelectionStatus::
                        growth_profile_typed_stop &&
                missing.status ==
                    LegacyBattleGrowthActorSelectionStatus::
                        missing_party_item_sentinel_typed_stop,
            "growth actor selection stops at the first invalid growth profile or required sentinel access"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 2U;
        fixture.port.reply(
            LegacyBattleGrowthActorSelectionCall::query_group_a_reward_block,
            {
                .eax = 0U,
                .publish_group_a_count = true,
                .group_a_count = 0U,
            }
        );

        const auto result = run(fixture);

        test.expect_true(
            result.status ==
                    LegacyBattleGrowthActorSelectionStatus::completed &&
                result.actor_query_calls == 1U && result.return_eax == 0U,
            "growth actor selection reloads the live group-A count at the loop tail"
        );
    }
}
