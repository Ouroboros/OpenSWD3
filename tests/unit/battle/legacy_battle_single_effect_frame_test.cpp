#include "openswd3/battle/legacy_battle_single_effect_frame.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <algorithm>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleEffectCallReply;
using openswd3::battle::LegacyBattleEffectCallRequest;
using openswd3::compat::u32;

class SingleEffectPort final
    : public openswd3::battle::LegacyBattleEffectCallPort {
public:
    [[nodiscard]] LegacyBattleEffectCallReply
    invoke(const LegacyBattleEffectCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        return default_reply;
    }

    void push(const u32 callee, const LegacyBattleEffectCallReply& reply) {
        replies[callee].push_back(reply);
    }

    [[nodiscard]] std::size_t count(const u32 callee) const {
        return static_cast<std::size_t>(std::ranges::count_if(
            calls, [callee](const LegacyBattleEffectCallRequest& request) {
                return request.callee_token == callee;
            }
        ));
    }

    LegacyBattleEffectCallReply default_reply{};
    std::unordered_map<u32, std::deque<LegacyBattleEffectCallReply>> replies;
    std::vector<LegacyBattleEffectCallRequest> calls;
};

[[nodiscard]] LegacyBattleEffectCallReply resource_reply(
    const u32 owner,
    const u32 value,
    const u32 width,
    const u32 height,
    const u32 data
) {
    LegacyBattleEffectCallReply reply{.eax = owner};
    reply.outputs[0] = value;
    reply.outputs[1] = width;
    reply.outputs[2] = height;
    reply.outputs[3] = data;
    return reply;
}

[[nodiscard]] LegacyBattleEffectCallReply
pair_reply(const u32 first, const u32 second, const u32 edx = 0U) {
    LegacyBattleEffectCallReply reply{.edx = edx};
    reply.outputs[0] = first;
    reply.outputs[1] = second;
    return reply;
}

[[nodiscard]] bool has_argument(
    const SingleEffectPort& port,
    const u32 callee,
    const std::size_t argument,
    const u32 value
) {
    return std::ranges::any_of(
        port.calls, [=](const LegacyBattleEffectCallRequest& request) {
            return request.callee_token == callee &&
                request.arguments[argument] == value;
        }
    );
}

}  // namespace

void test_battle_single_effect_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleSingleEffectFrameState;
    using openswd3::battle::LegacyBattleSingleEffectFrameStatus;

    {
        LegacyBattleSingleEffectFrameState state;
        SingleEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state, port, 0U, 0U, 8U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleSingleEffectFrameStatus::
                        slot_index_typed_stop &&
                result.port_calls == 0U,
            "single effect frame stops at first invalid record access"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        state.primary[0].complete = 1U;
        state.primary[0].status_flags = 0x8000U;
        state.primary[0].source_value = 7U;
        state.battle_gate = 9U;
        SingleEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state, port, 0U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 1U && state.battle_gate == 0U &&
                port.battle_message_state() == 1U &&
                state.primary[0].source_value == 0U && port.calls.empty(),
            "signed status side effects precede complete record clear"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        state.alternate[1].source_value = 8U;
        state.alternate_active[1] = 1U;
        SingleEffectPort port;
        port.push(0x004321E0U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state, port, 0x1000U, 0x66U, 1U
            );
        test.expect_true(
            result.return_value == 1U &&
                state.primary[1].source_value == 0x66U &&
                state.alternate[1].source_value == 0U &&
                state.alternate_active[1] == 0U,
            "initialization failure clears alternate record and returns one"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        SingleEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state, port, 0x1000U, 0U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleSingleEffectFrameStatus::
                        resource_owner_typed_stop &&
                port.count(0x00478400U) == 0U &&
                state.released_owner_value_clears == 0U,
            "null owner stops before current resource and coordinate queries"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        auto& record = state.primary[0];
        record.base_offset = 20U;
        record.base_y_offset = 10U;
        record.render_flags = 2U;
        record.pan_value = 0x44U;
        state.global_flip_mode = 1U;
        state.sample_handle_value = 0x88U;
        state.coordinate_output_x_token = 0xAAAA1111U;
        state.coordinate_output_y_token = 0xBBBB2222U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        auto& actor = (*startup->group_b_lifecycle)[0].action_execution;
        actor.position_x = 100U;
        actor.position_y = 200U;
        SingleEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(
            0x00431760U, resource_reply(0x1111U, 0x2222U, 200U, 30U, 0x3333U)
        );
        port.push(0x00478400U, pair_reply(9U, 0U, 0xA5A50000U));
        port.push(0x00485610U, {.ecx = 0xAAAA0000U, .edx = 0xBBBB0000U});
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken,
                0U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.return_value == 0U && record.pan_value == 0U &&
                state.current_resource_value_token == 0x2222U &&
                state.released_owner_value_clears == 1U &&
                has_argument(port, 0x00485610U, 0U, 0xFFFF0044U) &&
                has_argument(port, 0x00485650U, 0U, 0xAAAA0044U) &&
                has_argument(port, 0x00485650U, 1U, 0xFFFFFFF0U) &&
                has_argument(port, 0x004170E0U, 0U, 0xFFFFFFB0U) &&
                has_argument(port, 0x004170E0U, 1U, 190U) &&
                has_argument(port, 0x004170E0U, 4U, 3U) &&
                has_argument(port, 0x004170E0U, 5U, 0x3333U) &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.return_eax == 0xAAAA1111U &&
                result.coordinate_query.return_ecx == 0x005200C8U &&
                result.coordinate_query.return_edx == 0xBBBB2222U &&
                result.coordinate_query.flags.zero &&
                port.count(0x004783B0U) == 0U && port.count(0x004885A0U) == 2U,
            "left edge preserves coordinate and play-ECX high words with data render"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        auto& record = state.primary[0];
        record.pan_value = 0x55U;
        SingleEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(0x00431760U, resource_reply(0x1000U, 0U, 10U, 11U, 0U));
        port.push(0x00478400U, pair_reply(2U, 3U));
        port.push(0x00478470U, pair_reply(400U, 20U));
        port.push(0x00485610U, {.ecx = 0xAAAA0000U, .edx = 0xBBBB0000U});
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state, port, 0x1000U, 0U, 0U
            );
        test.expect_true(
            result.return_value == 0U && port.count(0x00478470U) == 1U &&
                has_argument(port, 0x00485650U, 0U, 0xBBBB0055U) &&
                has_argument(port, 0x00485650U, 1U, 16U) &&
                port.count(0x004885A0U) == 1U &&
                state.released_owner_value_clears == 1U,
            "right edge uses play-EDX high word and skips zero nested value release"
        );
    }

    {
        LegacyBattleSingleEffectFrameState state;
        state.primary[0].pan_value = 0x44U;
        state.coordinate_output_x_token = 0xDDDD1111U;
        state.coordinate_output_y_token = 0xEEEE2222U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        auto& actor = (*startup->group_b_lifecycle)[0].action_execution;
        actor.position_x = 0x1234U;
        actor.position_y_read_accessible = false;
        SingleEffectPort port;
        port.push(0x004321E0U, {.eax = 1U});
        port.push(
            0x00431760U, resource_reply(0x1111U, 0x2222U, 10U, 11U, 0x3333U)
        );
        port.push(
            0x00478400U, pair_reply(0xAAAA0000U, 0xBBBB0000U, 0xCCCC0000U)
        );
        const auto result =
            openswd3::battle::advance_legacy_battle_single_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken,
                0x66U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleSingleEffectFrameStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                result.coordinate_query.output_writes == 1U &&
                result.coordinate_query.output_x == 0x1234U &&
                result.coordinate_query.return_eax == 0xDDDD1111U &&
                result.coordinate_query.return_ecx ==
                    openswd3::battle::
                        kLegacyBattleActorCoordinatesGroupBBaseToken &&
                result.coordinate_query.return_edx == 0xEEEE2222U &&
                result.coordinate_query.flags.zero &&
                state.current_resource_value_token == 0x2222U &&
                state.primary[0].pan_value == 0x44U &&
                state.released_owner_value_clears == 0U &&
                port.count(0x00485610U) == 0U &&
                port.count(0x004170E0U) == 0U &&
                port.count(0x004885A0U) == 0U && port.count(0x004783B0U) == 0U,
            "single-effect coordinate Y stop keeps the first low-word write and suppresses sample render and releases"
        );
    }
}
