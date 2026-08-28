#include "openswd3/battle/legacy_battle_growth_item_result_selection.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleGrowthItemResultSelectionCall;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionCallReply;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionCallRequest;
using openswd3::battle::LegacyBattleGrowthItemResultSelectionRequest;
using openswd3::compat::u8;
using openswd3::compat::u32;

class Port final
    : public openswd3::battle::LegacyBattleGrowthItemResultSelectionPort {
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
                LegacyBattleGrowthItemResultSelectionCall::select_growth_item) {
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
};

struct Fixture {
    Fixture() {
        target.transition_actor_index = 0xFFU;
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleGrowthItemResultSelectionBindings bindings() {
        return {
            .victory = victory,
            .metrics = metrics,
            .target_selection = target,
            .level_advancement = advancement,
        };
    }

    openswd3::battle::LegacyBattleVictoryRewardState victory;
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

}  // namespace

void test_battle_growth_item_result_selection(openswd3::test::Context& test) {
    using openswd3::battle::kLegacyBattleGrowthItemResultCaptionToken;
    using openswd3::battle::kLegacyBattleGrowthItemResultProfileToken;
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
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::select_growth_item,
            {.eax = 0xABCD0000U, .ecx = 0x10203040U, .edx = 0x50607080U}
        );
        const auto result = run(fixture);
        test.expect_true(
            result.selected_actor_count == 0U && result.item_load_calls == 0U &&
                result.return_eax == 1U &&
                fixture.target.transition_actor_index == 0xFFU &&
                fixture.port.count(
                    LegacyBattleGrowthItemResultSelectionCall::
                        select_growth_item
                ) == 1U,
            "growth item result selection tests only AX and skips a nonzero high-word return"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 3U;
        fixture.victory.group_a_skip_primary[0U] = 1U;
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::select_growth_item,
            {.eax = 0xABCD0665U, .ecx = 0x11111111U, .edx = 0x22222222U}
        );
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
                result.port_calls == 5U &&
                result.completion_query_calls == 1U &&
                result.item_selection_calls == 1U &&
                result.item_load_calls == 1U &&
                result.item_release_calls == 1U &&
                result.caption_copy_calls == 1U &&
                result.selected_actor_count == 1U &&
                result.selected_item_code == 0xABCD0665U &&
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
                fixture.port.calls[1U].profile_token ==
                    kLegacyBattleGrowthItemResultProfileToken &&
                fixture.port.calls[1U].arguments[0U] ==
                    kLegacyBattleGrowthItemResultProfileToken &&
                fixture.port.calls[2U].destination_token ==
                    kLegacyBattleGrowthItemScratchToken &&
                fixture.port.calls[2U].item_code == 0xABCD0665U &&
                fixture.port.calls[2U].eax == 0xABCD0665U &&
                fixture.port.calls[3U].source_token ==
                    kLegacyBattleGrowthItemScratchToken &&
                fixture.port.calls[3U].eax == 0x31313131U &&
                fixture.port.calls[4U].destination_token ==
                    kLegacyBattleGrowthItemResultCaptionToken &&
                fixture.port.calls[4U].source_token ==
                    kLegacyBattleGrowthItemScratchToken &&
                fixture.port.calls[4U].eax == 0x41414141U &&
                fixture.port.calls[4U].text_length == 2U,
            "growth item result selection preserves the five callsite arguments and register chain"
        );
    }

    {
        Fixture fixture;
        fixture.metrics.group_a_count = 1U;
        fixture.port.reply(
            LegacyBattleGrowthItemResultSelectionCall::select_growth_item,
            {.eax = 0x0669U}
        );
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
                result.port_calls == 4U && result.caption_copy_calls == 1U &&
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
