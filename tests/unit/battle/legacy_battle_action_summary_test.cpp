#include "openswd3/battle/legacy_battle_action_summary.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActionSummaryCall;
using openswd3::battle::LegacyBattleActionSummaryCallReply;
using openswd3::battle::LegacyBattleActionSummaryCallRequest;
using openswd3::compat::u32;

class SummaryPort final
    : public openswd3::battle::LegacyBattleActionSummaryPort {
public:
    [[nodiscard]] LegacyBattleActionSummaryCallReply invoke_action_summary(
        const LegacyBattleActionSummaryCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = reply_indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        return found->second[index++];
    }

    void reply(
        const LegacyBattleActionSummaryCall call,
        const LegacyBattleActionSummaryCallReply value
    ) {
        replies[call].push_back(value);
    }

    std::vector<LegacyBattleActionSummaryCallRequest> calls;
    std::map<
        LegacyBattleActionSummaryCall,
        std::vector<LegacyBattleActionSummaryCallReply>>
        replies;
    std::map<LegacyBattleActionSummaryCall, std::size_t> reply_indices;
};

struct Fixture {
    Fixture() {
        startup.primary_text_color = 0x1234U;
        startup.secondary_text_color = 0x5678U;
        startup.group_a_profiles.profile_tokens.fill(1U);
        for (auto& source : startup.action_mode_source.option_sources[0U]) {
            source.object_token = 1U;
        }
        final_actor.queued_actor_code = 8U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionSummaryBindings
    bindings() {
        return {
            .startup = startup,
            .final_actor = final_actor,
            .frame_input = frame_input,
            .input_dispatch = input_dispatch,
        };
    }

    void complete_action_mode() {
        port.reply(
            LegacyBattleActionSummaryCall::action_mode_query_primary_actor,
            {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );
        port.reply(
            LegacyBattleActionSummaryCall::action_mode_query_secondary_actor,
            {.eax = 1U, .ecx = 0x33330000U, .edx = 0x44440000U}
        );
        port.reply(
            LegacyBattleActionSummaryCall::action_mode_query_active_actor,
            {.eax = 0U, .ecx = 0xAAAA0000U, .edx = 0xBBBBCCDDU}
        );
    }

    openswd3::battle::LegacyBattleStartupState startup;
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame_input;
    openswd3::battle::LegacyBattleInputDispatchState input_dispatch;
    SummaryPort port;
};

[[nodiscard]] std::size_t
count_call(const SummaryPort& port, const LegacyBattleActionSummaryCall call) {
    return static_cast<std::size_t>(std::ranges::count_if(
        port.calls, [call](const auto& request) { return request.call == call; }
    ));
}

[[nodiscard]] std::vector<LegacyBattleActionSummaryCallRequest>
text_calls(const SummaryPort& port, const u32 style) {
    std::vector<LegacyBattleActionSummaryCallRequest> result;
    std::ranges::copy_if(
        port.calls, std::back_inserter(result), [style](const auto& request) {
            return request.call == LegacyBattleActionSummaryCall::draw_text &&
                request.arguments[5U] == style;
        }
    );
    return result;
}

}  // namespace

void test_battle_action_summary(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionSummaryStatus;
    using openswd3::battle::draw_legacy_battle_action_summary;

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 0U;
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(),
            fixture.port,
            {
                .entry_eax = 0x11111111U,
                .entry_ecx = 0x22222222U,
                .entry_edx = 0x33333333U,
            }
        );
        test.expect_true(
            result.status == LegacyBattleActionSummaryStatus::completed &&
                result.port_calls == 0U && result.return_eax == 0U &&
                result.return_ecx == 0x22222222U &&
                result.return_edx == 0x33333333U,
            "zero queued actor returns before font or profile access while preserving ECX and EDX"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 7U;
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(), fixture.port, {.entry_edx = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionSummaryStatus::group_a_actor_typed_stop &&
                result.port_calls == 2U && result.font_reset_calls == 1U &&
                result.font_style_calls == 1U &&
                result.return_eax == 0xFFFFD0CCU &&
                result.return_ecx == 0xFFFFFFFFU &&
                result.return_edx == 0x12345678U,
            "queued actor seven stops at the first wrapped group-A profile pointer read after both font calls"
        );
    }

    {
        Fixture fixture;
        fixture.startup.group_a_profiles.profile_tokens[0U] = 0U;
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(), fixture.port, {.entry_edx = 0x87654321U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionSummaryStatus::
                        group_a_profile_typed_stop &&
                result.port_calls == 2U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0x87654321U,
            "zero profile token stops at the original kind dereference after the physical actor slot read"
        );
    }

    {
        Fixture fixture;
        fixture.complete_action_mode();
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(),
            fixture.port,
            {.origin_x = 100U, .origin_y = 200U, .action_kind = 2U}
        );
        const auto rows = text_calls(fixture.port, 4U);
        const auto highlights = text_calls(fixture.port, 0x10U);
        test.expect_true(
            result.status == LegacyBattleActionSummaryStatus::completed &&
                result.action_mode_refresh_calls == 1U &&
                result.fixed_action_rows == 4U &&
                result.dynamic_action_rows == 0U &&
                result.text_draw_calls == 5U && result.font_reset_calls == 1U &&
                result.font_style_calls == 4U && result.port_calls == 13U,
            "fixed action summary completes four rows, one highlight, and all font calls"
        );
        test.expect_true(
            rows.size() == 4U && rows[0U].arguments[1U] == 100U &&
                rows[0U].arguments[2U] == 200U &&
                rows[0U].arguments[3U] == 0x004A6BD8U && rows[0U].eax == 100U &&
                rows[0U].ecx == 0x004AB998U && rows[0U].edx == 0x004A6BD8U &&
                rows[1U].arguments[2U] == 224U &&
                rows[1U].arguments[3U] == 0x004A7650U &&
                rows[2U].arguments[3U] == 0x004A020CU &&
                rows[3U].arguments[3U] == 0x004A7648U &&
                std::ranges::all_of(
                    rows,
                    [](const auto& row) { return row.arguments[4U] == 0x1234U; }
                ),
            "fixed action rows preserve static tokens, geometry, and primary color"
        );
        test.expect_true(
            highlights.size() == 1U && highlights[0U].arguments[1U] == 99U &&
                highlights[0U].arguments[2U] == 223U &&
                highlights[0U].arguments[3U] == 0x004A7650U &&
                highlights[0U].arguments[4U] == 0x1234U &&
                highlights[0U].eax == 99U &&
                highlights[0U].ecx == 0x004AB998U && highlights[0U].edx == 0U,
            "selected fixed action draws its one-pixel primary overlay"
        );
        test.expect_true(
            count_call(
                fixture.port,
                LegacyBattleActionSummaryCall::action_mode_query_primary_actor
            ) == 1U,
            "fixed action summary directly executes the closed action-mode refresh"
        );
        test.expect_equal(
            result.return_eax,
            6U - 0x00524419U,
            "empty dynamic action tail preserves its final stale EAX"
        );
        test.expect_equal(
            result.return_ecx,
            0x004AB998U,
            "final font restore publishes the font owner in ECX"
        );
        test.expect_equal(
            result.return_edx,
            200U,
            "empty dynamic action tail preserves its original Y in EDX"
        );
    }

    {
        Fixture fixture;
        fixture.startup.group_a_profiles.profile_kinds[0U] = 0x38U;
        fixture.startup.action_mode_source.option_sources[0U][0U].action_code =
            0x25U;
        fixture.startup.action_mode_source.option_sources[0U][1U].action_code =
            0x26U;
        fixture.port.reply(
            LegacyBattleActionSummaryCall::query_actor_special_gate,
            {.eax = 0U, .ecx = 0x01020304U, .edx = 0x05060708U}
        );
        fixture.complete_action_mode();
        fixture.port.reply(
            LegacyBattleActionSummaryCall::query_action_available,
            {.eax = 1U, .ecx = 0x11110000U, .edx = 0x22220000U}
        );
        fixture.port.reply(
            LegacyBattleActionSummaryCall::query_action_available,
            {.eax = 0U, .ecx = 0xABCD0000U, .edx = 0x44440000U}
        );
        fixture.port.reply(
            LegacyBattleActionSummaryCall::configure_font_style,
            {.eax = 0U, .ecx = 0U, .edx = 0U}
        );
        fixture.port.reply(
            LegacyBattleActionSummaryCall::configure_font_style,
            {.eax = 0x11110000U, .ecx = 0xBEEF0000U, .edx = 0x22220000U}
        );
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(),
            fixture.port,
            {.origin_x = 100U, .origin_y = 200U, .action_kind = 7U}
        );
        const auto rows = text_calls(fixture.port, 4U);
        const auto highlights = text_calls(fixture.port, 0x10U);
        test.expect_true(
            result.status == LegacyBattleActionSummaryStatus::completed &&
                result.actor_special_queries == 1U &&
                result.action_availability_queries == 2U &&
                result.fixed_action_rows == 4U &&
                result.dynamic_action_rows == 2U &&
                result.permission_clears == 1U && result.port_calls == 18U &&
                rows.size() == 6U && rows[4U].arguments[1U] == 148U &&
                rows[4U].arguments[2U] == 224U &&
                rows[4U].arguments[3U] == 0x004A6BD8U &&
                rows[4U].arguments[4U] == 0x1234U &&
                rows[4U].eax == 0x004CD76CU && rows[4U].ecx == 0x004AB998U &&
                rows[4U].edx == 148U && rows[5U].arguments[2U] == 248U &&
                rows[5U].arguments[3U] == 0x004A7650U &&
                rows[5U].arguments[4U] == 0xABCD5678U && rows[5U].eax == 148U &&
                rows[5U].ecx == 0x004AB998U && rows[5U].edx == 0x004A7650U &&
                highlights.size() == 1U &&
                highlights[0U].arguments[1U] == 147U &&
                highlights[0U].arguments[2U] == 247U &&
                highlights[0U].arguments[3U] == 0x004A7650U &&
                highlights[0U].arguments[4U] == 0xBEEF1234U &&
                highlights[0U].eax == 247U &&
                highlights[0U].ecx == 0x004AB998U &&
                highlights[0U].edx == 0x004CD76CU &&
                ((fixture.startup.reset.value_524418 >> 16U) & 0xFFU) == 0U,
            "dynamic actions keep available primary color, disabled stale high word, permission clear, and selected overlay"
        );
    }

    {
        Fixture fixture;
        fixture.startup.action_mode_source.option_sources[0U][0U].object_token =
            0U;
        const auto result = draw_legacy_battle_action_summary(
            fixture.bindings(), fixture.port, {.entry_edx = 0x12345678U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionSummaryStatus::
                        action_mode_refresh_typed_stop &&
                result.action_mode_refresh.status ==
                    openswd3::battle::LegacyBattleActionModeRefreshStatus::
                        option_object_typed_stop &&
                result.action_mode_refresh_calls == 1U &&
                result.port_calls == 2U &&
                fixture.startup.reset.value_524414 == 0x01010101U &&
                fixture.startup.reset.value_4ff0b0 == 0U &&
                result.return_eax == 8U && result.return_ecx == 0U &&
                result.return_edx == 0x12340000U,
            "nested action-mode stop preserves its reset prefix and blocks every summary row"
        );
    }
}
