#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_growth_item_result_selection.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleGrowthItemResultSelectionCall;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionCallReply;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionCallRequest;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class Port final
    : public openswd3::battle::LegacyBattleGrowthItemResultSelectionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] LegacyBattleGrowthItemResultSelectionCallReply
    invoke_growth_item_result_selection(
        const LegacyBattleGrowthItemResultSelectionCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found != replies.end() && index < found->second.size()) {
            return found->second[index++];
        }

        auto reply = LegacyBattleGrowthItemResultSelectionPort::
            invoke_growth_item_result_selection(request);
        if (request.call ==
                LegacyBattleGrowthItemResultSelectionCall::
                    query_actor_completion ||
            request.call ==
                LegacyBattleGrowthItemResultSelectionCall::
                    reserved_select_growth_item) {
            reply.eax = 0U;
        }
        return reply;
    }

    void reply(
        const LegacyBattleGrowthItemResultSelectionCall call,
        const LegacyBattleGrowthItemResultSelectionCallReply& reply
    ) {
        replies[call].push_back(reply);
    }

    [[nodiscard]] u32
    count(const LegacyBattleGrowthItemResultSelectionCall call) const noexcept {
        return static_cast<u32>(std::ranges::count_if(
            calls,
            [call](
                const LegacyBattleGrowthItemResultSelectionCallRequest& request
            ) { return request.call == call; }
        ));
    }

    std::vector<LegacyBattleGrowthItemResultSelectionCallRequest> calls;
    std::map<
        LegacyBattleGrowthItemResultSelectionCall,
        std::vector<LegacyBattleGrowthItemResultSelectionCallReply>>
        replies;
    std::map<LegacyBattleGrowthItemResultSelectionCall, std::size_t>
        reply_indices;

protected:
    [[nodiscard]] std::optional<bool> prepare_definition_record(
        const std::span<u8> destination, const u32
    ) noexcept override {
        const auto call =
            LegacyBattleGrowthItemResultSelectionCall::load_item_definition;
        auto& index = reply_indices[call];
        const auto found = replies.find(call);
        if (found == replies.end() || index >= found->second.size()) {
            return false;
        }
        const auto& reply = found->second[index++];
        if (!reply.publish_definition) {
            return false;
        }
        std::copy(
            reply.definition.cbegin(),
            reply.definition.cend(),
            destination.begin()
        );
        definition_description.assign(
            reply.description.cbegin(),
            reply.description.cbegin() + reply.description_length
        );
        return true;
    }
};

struct Fixture {
    Fixture() {
        target.transition_actor_index = 0xFFU;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleGrowthItemResultSelectionBindings bindings() {
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

[[nodiscard]] openswd3::battle::LegacyBattleGrowthItemResultSelectionResult
run(Fixture& fixture,
    const LegacyBattleGrowthItemResultSelectionRequest& request = {
        .entry_eax = 0x11112222U,
        .entry_ecx = 0x33334444U,
        .entry_edx = 0x55556666U,
    }) {
    return openswd3::battle::advance_legacy_battle_growth_item_result_selection(
        fixture.bindings(), fixture.port, request
    );
}

[[nodiscard]] std::array<u8, 160U>
title_definition(const std::initializer_list<u8> title) {
    std::array<u8, 160U> definition{};
    std::copy(title.begin(), title.end(), definition.begin());
    return definition;
}

void set_profile_word(
    openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) {
    profile[offset] = static_cast<std::byte>(static_cast<u8>(value));
    profile[offset + 1U] = static_cast<std::byte>(static_cast<u8>(value >> 8U));
}

void seed_growth_reward(
    Fixture& fixture,
    const std::size_t actor_index,
    const u16 item_id,
    const u16 maximum
) {
    auto& profile = fixture.startup.party[actor_index]
                        .attribute_aggregation.embedded_profiles[0U];
    set_profile_word(profile, 0x04U, maximum);
    set_profile_word(profile, 0x10U, item_id);
    auto& head = fixture.port.group_a_reward_profile_state().head;
    head.item_id = item_id;
    head.quantity = maximum;
}

}  // namespace

void test_battle_growth_item_result_selection(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleGrowthItemResultCaptionToken;
    using openswd3::battle::kLegacyBattleGrowthItemScratchToken;
    using openswd3::battle::kLegacyBattleVictoryGroupABaseToken;
    using openswd3::battle::kLegacyBattleVictoryGroupAElementSize;
    using openswd3::battle::LegacyBattleGrowthItemResultSelectionStatus;

    {
        Fixture zero;
        zero.metrics.group_a_count = 0U;
        const auto zero_result = run(zero);
        test.expect_true(
            zero_result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::completed &&
                zero_result.port_calls == 0U && zero_result.return_eax == 0U &&
                zero_result.return_ecx == 0x33334444U &&
                zero_result.return_edx == 0x55556666U,
            "growth item result selection returns the live zero group count without touching an actor"
        );

        Fixture negative;
        negative.metrics.group_a_count = 0xFFFFFFFFU;
        const auto negative_result = run(negative);
        test.expect_true(
            negative_result.port_calls == 0U &&
                negative_result.return_eax == 0xFFFFFFFFU &&
                negative.target.transition_actor_index == 0xFFU,
            "growth item result selection treats the initial group count as signed"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 4U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.victory.group_a_skip_secondary[1U] = 1U;
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::query_actor_completion,
            {
                .eax = 1U,
                .publish_group_a_count = true,
                .group_a_count = 3U,
            }
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::completed &&
                result.completion_query_calls == 1U &&
                result.item_selection_calls == 0U && result.return_eax == 3U &&
                fixture.metrics.group_a_count == 3U &&
                fixture.port.calls[0U].actor_token ==
                    kLegacyBattleVictoryGroupABaseToken +
                        2U * kLegacyBattleVictoryGroupAElementSize,
            "growth item result selection applies both exact-one skip fields and reloads the live count after an exact-one completion query"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        const auto result = run(fixture);
        test.expect_true(
            result.selected_actor_count == 0U && result.item_load_calls == 0U &&
                result.return_eax == 1U &&
                fixture.target.transition_actor_index == 0xFFU &&
                result.growth_reward.return_eax == 0U &&
                fixture.port.count(
                    LegacyBattleGrowthItemResultSelectionCall::
                        reserved_select_growth_item
                ) == 0U,
            "growth item result selection directly skips an actor whose two embedded reward profiles are empty"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        seed_growth_reward(fixture, 1U, 0x0665U, 10U);
        auto definition = title_definition({0x46U, 0x41U, 0x00U});
        LegacyBattleGrowthItemResultSelectionCallReply load_reply{
            .eax = 0x31313131U,
            .ecx = 0x32323232U,
            .edx = 0x33333333U,
            .publish_definition = true,
            .definition = definition,
            .description_length = 2U,
        };
        load_reply.description[0U] = 0x61U;
        load_reply.description[1U] = 0x62U;
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::load_item_definition,
            load_reply
        );
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::release_item_description,
            {.eax = 0x41414141U, .ecx = 0x42424242U, .edx = 0x43434343U}
        );
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::copy_caption,
            {.eax = 0x51515151U, .ecx = 0x52525252U, .edx = 0x53535353U}
        );

        const auto result = run(fixture);
        const auto& scratch =
            fixture.port.battle_growth_actor_selection_state().scratch;
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::completed &&
                result.port_calls == 4U &&
                result.completion_query_calls == 1U &&
                result.item_selection_calls == 1U &&
                result.item_load_calls == 1U &&
                result.item_release_calls == 1U &&
                result.caption_copy_calls == 1U &&
                result.selected_actor_count == 1U &&
                result.selected_item_code == 0x0665U &&
                fixture.target.transition_mode == 1U &&
                fixture.target.transition_actor_index == 1U &&
                fixture.advancement.growth_caption_text[0U] == 0x46U &&
                fixture.advancement.growth_caption_text[1U] == 0x41U &&
                fixture.advancement.growth_caption_text[2U] == 0U &&
                scratch.description.empty() &&
                result.return_eax == 0x51515151U &&
                result.return_ecx == 0x52525252U &&
                result.return_edx == 0x53535353U,
            "growth item result selection commits the first successful actor after loading, releasing and copying its item title"
        );
        test.expect_true(
            fixture.port.calls[0U].actor_token ==
                    kLegacyBattleVictoryGroupABaseToken +
                        kLegacyBattleVictoryGroupAElementSize &&
                fixture.port.calls[0U].eax == 3U &&
                fixture.port.calls[0U].ecx ==
                    kLegacyBattleVictoryGroupABaseToken +
                        kLegacyBattleVictoryGroupAElementSize &&
                result.growth_reward.return_eax == 0x0665U &&
                fixture.port.group_a_reward_profile_state()
                        .head.blocking_flag == 1U &&
                fixture.port.requested_definition_ids ==
                    std::vector<u32>{0x0665U} &&
                fixture.port.calls[1U].source_token ==
                    kLegacyBattleGrowthItemScratchToken &&
                fixture.port.calls[2U].destination_token ==
                    kLegacyBattleGrowthItemResultCaptionToken &&
                fixture.port.calls[2U].source_token ==
                    kLegacyBattleGrowthItemScratchToken &&
                fixture.port.calls[2U].eax == 0x41414141U &&
                fixture.port.calls[2U].text_length == 2U &&
                fixture.port.count(
                    LegacyBattleGrowthItemResultSelectionCall::
                        reserved_select_growth_item
                ) == 0U,
            "growth item result selection preserves the four remaining callsite arguments after direct reward selection"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        seed_growth_reward(fixture, 0U, 0x0669U, 10U);
        LegacyBattleGrowthItemResultSelectionCallReply load_reply{
            .publish_definition = true,
        };
        load_reply.definition.fill(0x58U);
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::load_item_definition,
            load_reply
        );
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::release_item_description,
            {.eax = 0x61616161U, .ecx = 0x62626262U, .edx = 0x63636363U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::
                        caption_destination_typed_stop &&
                result.port_calls == 3U && result.caption_copy_calls == 1U &&
                result.call_trace.back() ==
                    LegacyBattleGrowthItemResultSelectionCall::copy_caption &&
                result.stopped_caption_index == 24U &&
                fixture.target.transition_mode == 1U &&
                fixture.target.transition_actor_index == 0xFFU &&
                std::ranges::all_of(
                    fixture.advancement.growth_caption_text,
                    [](const u8 value) { return value == 0x58U; }
                ) &&
                result.return_eax == 0x61616161U &&
                result.return_ecx == 0x62626262U &&
                result.return_edx == 0x63636363U,
            "growth item result selection preserves mode and the 24-byte copy prefix before the destination stop while withholding the actor"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        auto& profile = fixture.startup.party[0U]
                            .attribute_aggregation.embedded_profiles[0U];
        set_profile_word(profile, 0x04U, 10U);
        set_profile_word(profile, 0x10U, 7U);
        fixture.port.group_a_reward_profile_state().head.item_id = 1U;
        fixture.port.group_a_reward_profile_state().head.legacy_next_token =
            0x00DEAD00U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::
                        growth_reward_typed_stop &&
                result.item_selection_calls == 1U &&
                result.growth_reward.status ==
                    openswd3::battle::
                        LegacyBattleGroupAGrowthRewardSelectionStatus::
                            profile_node_typed_stop &&
                result.item_load_calls == 0U &&
                fixture.target.transition_actor_index == 0xFFU &&
                fixture.port.count(
                    LegacyBattleGrowthItemResultSelectionCall::
                        reserved_select_growth_item
                ) == 0U,
            "growth item caller propagates a direct compact-chain stop before loading item text"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 11U;
        const auto result = run(fixture);
        test.expect_true(
            result.status ==
                    LegacyBattleGrowthItemResultSelectionStatus::
                        group_a_actor_typed_stop &&
                result.stopped_actor_index == 10U &&
                result.completion_query_calls == 10U &&
                result.item_selection_calls == 10U &&
                fixture.target.transition_actor_index == 0xFFU,
            "growth item result selection stops on the first physical group-A actor access beyond ten"
        );
    }
}
