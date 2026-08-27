#include "openswd3/battle/legacy_battle_action_mode_refresh.hpp"

#include "test.hpp"

#include <optional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleInputDispatchCall;
using openswd3::battle::LegacyBattleInputDispatchCallReply;
using openswd3::battle::LegacyBattleInputDispatchCallRequest;
using openswd3::compat::u32;

class ActionModeRefreshPort final
    : public openswd3::battle::LegacyBattleInputDispatchPort {
public:
    [[nodiscard]] LegacyBattleInputDispatchCallReply invoke_input_dispatch(
        const LegacyBattleInputDispatchCallRequest& request
    ) override {
        calls.push_back(request);
        const std::size_t index = calls.size() - 1U;
        if (index < replies.size() && replies[index].has_value()) {
            return *replies[index];
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::vector<LegacyBattleInputDispatchCallRequest> calls;
    std::vector<std::optional<LegacyBattleInputDispatchCallReply>> replies;
};

struct Fixture {
    Fixture() {
        u32 token = 0x12340000U;
        for (auto& pair : source.option_sources) {
            for (auto& record : pair) {
                record.object_token = token;
                token += 0x10000U;
            }
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionModeRefreshBindings
    bindings() {
        return {
            .startup_reset = reset,
            .source_state = source,
            .party_presence = presence,
            .startup_mode_flags = mode_flags,
            .final_actor = final_actor,
            .frame_input = frame,
            .input_dispatch = input,
        };
    }

    openswd3::battle::LegacyBattleStartupResetBlocks reset;
    openswd3::battle::LegacyBattleActionModeSourceState source;
    std::array<openswd3::compat::u8, 4> presence{};
    u32 mode_flags{};
    openswd3::battle::LegacyBattleFinalActorStepState final_actor;
    openswd3::battle::LegacyBattleFrameInputResolutionState frame;
    openswd3::battle::LegacyBattleInputDispatchState input;
    ActionModeRefreshPort port;
};

}  // namespace

void test_battle_action_mode_refresh(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActionModeRefreshStatus;
    using openswd3::battle::refresh_legacy_battle_action_mode;

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.source.actor_label_indices[1U] = 2U;
        fixture.source.option_sources[2U][0U].action_code = 0x14U;
        fixture.source.option_sources[2U][1U].action_code = 0x32U;
        fixture.port.replies = {
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0x11U, .edx = 0x22U
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 0U, .ecx = 0x33U, .edx = 0x44U
            },
            LegacyBattleInputDispatchCallReply{
                .eax = 2U, .ecx = 0x55U, .edx = 0x66U
            },
        };
        const auto result = refresh_legacy_battle_action_mode(
            fixture.bindings(), fixture.port, {.entry_edx = 0xABCD1234U}
        );
        test.expect_true(
            result.status == LegacyBattleActionModeRefreshStatus::completed &&
                result.option_pointer_reads == 2U &&
                result.option_object_reads == 2U &&
                result.qualifying_options == 0U && result.port_calls == 3U &&
                fixture.reset.value_524414 == 0x00010001U &&
                fixture.reset.value_524418 == 1U &&
                fixture.reset.value_53bf22 == 0U &&
                fixture.port.calls[0U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_primary_actor &&
                fixture.port.calls[0U].eax == 0x3EFU &&
                fixture.port.calls[0U].ecx == 0x00505904U &&
                fixture.port.calls[0U].edx == 0xABCD0000U &&
                fixture.port.calls[1U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_secondary_actor &&
                fixture.port.calls[1U].eax == 0xBCDU &&
                fixture.port.calls[1U].ecx == 0x00505904U &&
                fixture.port.calls[1U].edx == 9U &&
                fixture.port.calls[2U].call ==
                    LegacyBattleInputDispatchCall::
                        action_mode_query_active_actor &&
                fixture.port.calls[2U].eax == 0x3EFU &&
                fixture.port.calls[2U].ecx == 0x00505904U &&
                fixture.port.calls[2U].edx == 0xBCDU &&
                result.return_eax == 2U && result.return_ecx == 0x55U &&
                result.return_edx == 0x66U,
            "out-of-range option codes are skipped before three actor queries preserve their distinct register shapes"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 10U;
        fixture.source.actor_label_indices[2U] = 3U;
        fixture.source.option_sources[3U][0U].action_code = 0x15U;
        fixture.source.option_sources[3U][1U].action_code = 0x31U;
        fixture.frame.selection_actor_code = 0xDEADBEEFU;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[2U] =
            LegacyBattleInputDispatchCallReply{.eax = 0U};
        const auto result = refresh_legacy_battle_action_mode(
            fixture.bindings(), fixture.port, {.entry_edx = 0xCAFE5678U}
        );
        test.expect_true(
            result.status == LegacyBattleActionModeRefreshStatus::completed &&
                result.qualifying_options == 2U &&
                fixture.reset.value_53bf22 == 2U &&
                fixture.reset.value_4fe5cc == 0x00310015U &&
                fixture.reset.value_4ff0b0 == 0x004A76D0U &&
                fixture.reset.value_4ff0b4 == 0xDEADBEEFU &&
                fixture.reset.value_524418 == 0x00010101U &&
                fixture.port.calls[0U].edx == 0xCAFE0002U,
            "two qualifying options publish codes text tokens and permissions including the live adjacent selection value"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.presence[0U] = 1U;
        fixture.mode_flags = 2U;
        fixture.source.option_sources[0U][0U].action_code = 0x25U;
        fixture.source.option_sources[0U][1U].action_code = 0x2AU;
        fixture.input.action_kind = 7U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[2U] =
            LegacyBattleInputDispatchCallReply{.eax = 0U};
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status == LegacyBattleActionModeRefreshStatus::completed &&
                result.qualifying_options == 2U &&
                fixture.reset.value_53bf22 == 3U &&
                fixture.reset.value_4fe5cc == 0x00250006U &&
                fixture.reset.value_4fe5d0 == 0x2AU &&
                fixture.reset.value_4ff0b0 == 0x004A79A0U &&
                fixture.reset.value_4ff0b4 == 0x004A6BD8U &&
                fixture.reset.value_4ff0b8 == 7U &&
                fixture.reset.value_524418 == 0x01010101U,
            "first actor special mode prepends fixed option six then reads the live action-kind dword through the adjacent lookup"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.input.action_kind = 7U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[2U] = LegacyBattleInputDispatchCallReply{
            .eax = 1U, .ecx = 0x88U, .edx = 0x99U
        };
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status == LegacyBattleActionModeRefreshStatus::completed &&
                fixture.reset.value_524414 == 0x00000100U &&
                fixture.reset.value_524418 == 1U &&
                fixture.input.action_kind == 2U && result.return_eax == 0U &&
                result.return_ecx == 7U && result.return_edx == 0x99U,
            "active actor result one rebuilds the sparse permission pair and falls back disallowed action kind to two"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.input.action_kind = 2U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[2U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status == LegacyBattleActionModeRefreshStatus::completed &&
                fixture.input.action_kind == 2U && result.return_eax == 1U &&
                result.return_ecx == 2U,
            "sparse permission index two remains selected and returns its loaded byte"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.input.action_kind = 9U;
        fixture.port.replies.resize(3U);
        fixture.port.replies[0U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[1U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        fixture.port.replies[2U] =
            LegacyBattleInputDispatchCallReply{.eax = 1U};
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status ==
                    LegacyBattleActionModeRefreshStatus::
                        permission_typed_stop &&
                fixture.reset.value_524414 == 0x00000100U &&
                fixture.reset.value_524418 == 1U &&
                fixture.input.action_kind == 9U && result.return_eax == 0U &&
                result.return_ecx == 9U,
            "invalid action kind stops only at the final physical permission read after sparse reset writes"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 8U;
        fixture.source.option_sources[0U][0U].object_token = 0U;
        const auto result = refresh_legacy_battle_action_mode(
            fixture.bindings(), fixture.port, {.entry_edx = 0xABCD1234U}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActionModeRefreshStatus::
                        option_object_typed_stop &&
                result.port_calls == 0U && result.option_pointer_reads == 1U &&
                result.option_object_reads == 0U && result.return_eax == 8U &&
                result.return_ecx == 0U && result.return_edx == 0xABCD0000U &&
                fixture.reset.value_524414 == 0x01010101U &&
                fixture.reset.value_524418 == 1U,
            "missing option object stops at its first field dereference after permission and count initialization"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 12U;
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status ==
                    LegacyBattleActionModeRefreshStatus::
                        actor_mapping_typed_stop &&
                result.port_calls == 0U && result.return_eax == 12U &&
                result.return_ecx == 0U && result.return_edx == 0U,
            "actor code twelve stops at the first four-entry physical actor mapping access"
        );
    }

    {
        Fixture fixture;
        fixture.final_actor.queued_actor_code = 9U;
        fixture.source.actor_label_indices[1U] = 4U;
        const auto result =
            refresh_legacy_battle_action_mode(fixture.bindings(), fixture.port);
        test.expect_true(
            result.status ==
                    LegacyBattleActionModeRefreshStatus::
                        option_source_typed_stop &&
                result.port_calls == 0U && result.return_eax == 9U &&
                result.return_ecx == 0U && result.return_edx == 0U,
            "mapped source four stops at the first option-pointer table access"
        );
    }
}
