#include "openswd3/battle/legacy_battle_effect_frame.hpp"
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

class IntensityEffectPort final
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
    const u32 data,
    const u32 edx = 0U
) {
    LegacyBattleEffectCallReply reply{.eax = owner, .edx = edx};
    reply.outputs[0] = value;
    reply.outputs[1] = width;
    reply.outputs[2] = height;
    reply.outputs[3] = data;
    return reply;
}

[[nodiscard]] bool has_argument(
    const IntensityEffectPort& port,
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

void test_battle_intensity_effect_frame(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleEffectFrameState;
    using openswd3::battle::LegacyBattleIntensityEffectFrameStatus;

    {
        LegacyBattleEffectFrameState state;
        IntensityEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state, port, 0U, 0U, 0xAABBCCDDU, 99U
            );
        test.expect_true(
            result.return_value == 1U &&
                result.status ==
                    LegacyBattleIntensityEffectFrameStatus::completed &&
                result.final_edx == 0xAABBCCDDU && port.calls.empty(),
            "zero source returns before invalid slot access and preserves caller EDX"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        IntensityEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state, port, 0U, 1U, 2U, 8U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleIntensityEffectFrameStatus::
                        slot_index_typed_stop &&
                result.port_calls == 0U,
            "nonzero source stops at first invalid intensity slot access"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.intensity_values[2] = -32;
        state.intensity_records[2].source_value = 0x77U;
        IntensityEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state, port, 0U, 1U, 0x12345678U, 2U
            );
        test.expect_true(
            result.return_value == 1U && state.intensity_values[2] == 0 &&
                state.intensity_records[2].source_value == 0x77U &&
                result.final_edx == 0x12345678U && port.calls.empty(),
            "signed minus-thirty-two threshold clears intensity only"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.global_mode = 1U;
        state.intensity_values[1] = -31;
        state.intensity_coordinate_output_x_token = 0xAAAA1111U;
        state.intensity_coordinate_output_y_token = 0xBBBB2222U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->party[0].position_x = 0x1111U;
        startup->party[0].position_y = 0x2222U;
        IntensityEffectPort port;
        port.push(0x004321E0U, {.eax = 0U, .edx = 0xAABBCCDDU});
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken,
                0x66U,
                0x77U,
                1U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.return_value == 0U && result.final_edx == 0xAABBCCDDU &&
                state.intensity_records[1].source_value == 0x2222U &&
                state.intensity_records[1].secondary_value == 0x77U &&
                state.intensity_records[1].mode_snapshot == 1U &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.return_eax == 0xAAAA1111U &&
                result.coordinate_query.return_ecx == 0x00502222U &&
                result.coordinate_query.return_edx == 0xBBBB2222U &&
                result.coordinate_query.flags.zero &&
                has_argument(port, 0x004321E0U, 0U, 0x00524A18U) &&
                port.count(0x004783B0U) == 0U && port.count(0x004315D0U) == 0U,
            "coordinate query precedes record initialization and zero EAX returns intact"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.intensity_values[0] = -31;
        state.intensity_records[0].lookup_key_a = 0x3344U;
        state.intensity_records[0].lookup_key_b = 0x5566U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        IntensityEffectPort port;
        port.push(0x004321E0U, {.eax = 0x12340001U, .edx = 0xABCD0002U});
        port.push(0x004315D0U, {.eax = 0U, .edx = 0xDEADBEEFU});
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken,
                1U,
                2U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleIntensityEffectFrameStatus::
                        resource_owner_typed_stop &&
                result.final_edx == 0xDEADBEEFU &&
                has_argument(port, 0x004315D0U, 0U, 0x12343344U) &&
                has_argument(port, 0x004315D0U, 1U, 0xABCD5566U) &&
                state.current_resource_value_token == 0U &&
                port.count(0x004170E0U) == 0U,
            "lookup preserves initializer EAX and EDX high words before owner stop"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        auto& record = state.intensity_records[0];
        record.x_offset = 0x10U;
        record.y_offset = 0xFFFFFFF0U;
        record.render_flags = 0x55U;
        record.lookup_key_a = 0x1122U;
        record.lookup_key_b = 0x3344U;
        state.intensity_values[0] = -31;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->party[0].coordinate_mode_gate = 1U;
        startup->party[0].alternate_position_x = 0xFFF0U;
        startup->party[0].alternate_position_y = 0x0008U;
        IntensityEffectPort port;
        port.push(0x004321E0U, {.eax = 0xAAAA0001U, .edx = 0xBBBB0002U});
        port.push(
            0x004315D0U,
            resource_reply(0x1000U, 0x2000U, 0x12340020U, 0x56780030U, 0x3000U)
        );
        port.push(0x004170E0U, {.edx = 0xCAFEBABEU});
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken,
                0x56785000U,
                0x6000U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.return_value == 0U && result.final_edx == 0xCAFEBABEU &&
                state.current_resource_value_token == 0x2000U &&
                state.intensity_records[0].source_value == 0x56780008U &&
                state.render_intensity_a == -31 &&
                state.render_intensity_b == -31 &&
                state.render_intensity_c == -31 &&
                state.intensity_values[0] == -35 &&
                has_argument(port, 0x004315D0U, 0U, 0xAAAA1122U) &&
                has_argument(port, 0x004315D0U, 1U, 0xBBBB3344U) &&
                has_argument(port, 0x004170E0U, 0U, 0xFFFFFFE0U) &&
                has_argument(port, 0x004170E0U, 1U, 24U) &&
                has_argument(port, 0x004170E0U, 2U, 0x20U) &&
                has_argument(port, 0x004170E0U, 3U, 0x30U) &&
                has_argument(port, 0x004170E0U, 4U, 0x55U) &&
                has_argument(port, 0x004170E0U, 5U, 0x3000U) &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.alternate_coordinates &&
                port.count(0x004783B0U) == 0U && port.calls.size() == 3U,
            "render uses signed coordinate lows, full offsets, low dimensions and no release"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.intensity_values[0] = -31;
        state.intensity_coordinate_output_x_token = 0xAAAA1111U;
        state.intensity_coordinate_output_y_token = 0xBBBB2222U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->party[0].coordinate_mode_gate_read_accessible = false;
        IntensityEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken,
                0xCCCC0066U,
                0xDDDD7777U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleIntensityEffectFrameStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query_calls == 1U &&
                result.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        actor_gate_read_typed_stop &&
                result.coordinate_query.return_eax == 0xBBBB2222U &&
                result.coordinate_query.return_ecx ==
                    openswd3::battle::
                        kLegacyBattleActorCoordinatesGroupABaseToken &&
                result.coordinate_query.return_edx == 0xDDDD7777U &&
                !result.coordinate_query.flags.zero &&
                !result.coordinate_query.flags.parity &&
                result.final_edx == 0xDDDD7777U &&
                state.intensity_values[0] == -31 &&
                state.intensity_records[0].source_value == 0U &&
                port.calls.empty(),
            "intensity coordinate gate stop preserves the signed-threshold flags and caller EDX before every opaque callee"
        );
    }

    {
        LegacyBattleEffectFrameState state;
        state.intensity_values[0] = -31;
        state.intensity_coordinate_output_x_token = 0xAAAA1111U;
        state.intensity_coordinate_output_y_token = 0xBBBB2222U;
        auto startup =
            std::make_unique<openswd3::battle::LegacyBattleStartupState>();
        startup->party[0].position_x = 0x1234U;
        startup->party[0].position_y_read_accessible = false;
        IntensityEffectPort port;
        const auto result =
            openswd3::battle::advance_legacy_battle_intensity_effect_frame(
                state,
                port,
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken,
                0xCCCC0066U,
                0xDDDD7777U,
                0U,
                {.startup = startup.get()}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleIntensityEffectFrameStatus::
                        actor_coordinate_typed_stop &&
                result.coordinate_query.status ==
                    openswd3::battle::LegacyBattleActorCoordinateQueryStatus::
                        primary_y_read_typed_stop &&
                result.coordinate_query.output_writes == 1U &&
                result.coordinate_query.output_x == 0x1234U &&
                result.coordinate_query.return_eax == 0xAAAA1111U &&
                result.coordinate_query.return_ecx ==
                    openswd3::battle::
                        kLegacyBattleActorCoordinatesGroupABaseToken &&
                result.coordinate_query.return_edx == 0xBBBB2222U &&
                result.final_edx == 0xBBBB2222U &&
                state.intensity_values[0] == -31 &&
                state.intensity_records[0].source_value == 0U &&
                port.calls.empty(),
            "intensity coordinate Y stop preserves one aliased low-word write and suppresses initialization lookup render and decrement"
        );
    }
}
