#include "openswd3/battle/legacy_battle_actor_action_presentation.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "test.hpp"

#include <array>
#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActionCallReply;
using openswd3::battle::LegacyBattleActionCallRequest;
using openswd3::battle::LegacyBattleActorActionPresentationOwners;
using openswd3::battle::LegacyBattleActorActionPresentationRequest;
using openswd3::battle::LegacyBattleActorActionPresentationStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class DispatchPort final
    : public openswd3::battle::LegacyBattleActionDispatchPort {
public:
    [[nodiscard]] LegacyBattleActionCallReply
    invoke(const LegacyBattleActionCallRequest& request) override {
        calls.push_back(request);
        const auto found = replies.find(request.callee_token);
        if (found != replies.end() && !found->second.empty()) {
            const auto reply = found->second.front();
            found->second.pop_front();
            return reply;
        }
        return default_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageCallReply
    invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest&
    ) override {
        return {};
    }

    void push(const u32 callee, const LegacyBattleActionCallReply& reply) {
        replies[callee].push_back(reply);
    }

    LegacyBattleActionCallReply default_reply{
        .eax = 0x71000000U,
        .edx = 0xA5A55A5AU,
        .outputs = {0x72000000U, 32U, 48U},
    };
    std::unordered_map<u32, std::deque<LegacyBattleActionCallReply>> replies;
    std::vector<LegacyBattleActionCallRequest> calls;
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    void prepare() {
        bytes.assign(32U * 32U * sizeof(u16), 0U);
        available = true;
    }

    [[nodiscard]] bool load_frame_piece(
        u32, u32, openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        ++calls;
        if (!available) {
            piece = {};
            return false;
        }

        piece = {
            .source =
                {
                    .bytes = bytes,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .legacy_source_token = 0x73000000U,
            .width = 32U,
            .height = 32U,
        };
        return true;
    }

    std::vector<u8> bytes;
    bool available{};
    u32 calls{};
};

struct Fixture {
    openswd3::rendering::LegacyFramebuffer framebuffer;
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState shared_effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    FrameProvider frame_provider;
    DispatchPort port;
    std::unique_ptr<openswd3::battle::LegacyBattleStartupState> startup{
        std::make_unique<openswd3::battle::LegacyBattleStartupState>()
    };
    std::unique_ptr<openswd3::battle::LegacyBattleActionDispatchState> action{
        std::make_unique<openswd3::battle::LegacyBattleActionDispatchState>()
    };

    Fixture() {
        static_cast<void>(
            openswd3::rendering::initialize_legacy_raster_geometry(
                raster, framebuffer.geometry().surface
            )
        );
        startup->group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            openswd3::battle::kLegacyBattleActorGroupBElementCount>>();
    }

    [[nodiscard]] LegacyBattleActorActionPresentationOwners owners() noexcept {
        return {.action = action.get(), .startup = startup.get()};
    }

    [[nodiscard]]
    openswd3::battle::LegacyBattleActorActionPresentationPlatform
    platform() noexcept {
        return {
            .port = &port,
            .framebuffer = &framebuffer,
            .raster = &raster,
            .shared_request = &shared_request,
            .shared_effects = &shared_effects,
            .jitter = &jitter,
            .frame_provider = &frame_provider,
        };
    }
};

[[nodiscard]] LegacyBattleActorActionPresentationRequest
request(const u32 actor_token) noexcept {
    return {
        .actor_token = actor_token,
        .effect_argument = 0xAABBCCDDU,
        .entry_eax = 0x11223344U,
        .entry_edx = 0x55667788U,
        .entry_ebx = 0x99AABBCCU,
        .entry_ebp = 0x12345678U,
        .entry_esi = 0x76543210U,
        .entry_edi = 0x0BADF00DU,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x004566E0U,
        .entry_flags =
            {
                .carry = true,
                .parity = false,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = true,
                .zero = false,
                .sign = true,
                .overflow = true,
            },
        .entry_flags_known = true,
    };
}

}  // namespace

void test_battle_actor_action_presentation(openswd3::test::Context& test) {
    constexpr u32 group_a_token =
        openswd3::battle::kLegacyBattleActionGroupABaseToken;
    constexpr u32 group_b_token =
        openswd3::battle::kLegacyBattleActionGroupBBaseToken;

    {
        Fixture fixture;
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_action_presentation(
                fixture.owners(), group_a_token
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_action_presentation(
                fixture.owners(), group_b_token
            );
        const auto invalid =
            openswd3::battle::resolve_legacy_battle_actor_action_presentation(
                fixture.owners(), group_a_token + 1U
            );
        test.expect_true(
            group_a.actor.residual ==
                    &(*fixture.startup->group_a_runtime_reset)[0U] &&
                group_a.actor.action_execution ==
                    &fixture.action->group_a_action_execution[0U] &&
                group_a.shared == &fixture.action->group_a_action_shared &&
                group_b.actor.residual ==
                    &(*fixture.startup->group_b_lifecycle)[0U].runtime_reset &&
                group_b.actor.action_execution ==
                    &(*fixture.startup->group_b_lifecycle)[0U]
                         .action_execution &&
                group_b.shared == &fixture.action->group_a_action_shared &&
                invalid.actor.residual == nullptr,
            "action presentation resolves only canonical Group-A and Group-B actor owners"
        );
    }

    {
        Fixture fixture;
        const auto entry = request(group_a_token);
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::completed &&
                result.returned && result.exit_instruction == 0x00479849U &&
                result.return_eax == entry.entry_eax &&
                result.return_ecx == group_a_token &&
                result.return_edx == entry.entry_edx &&
                result.return_ebx == entry.entry_ebx &&
                result.return_ebp == entry.entry_ebp &&
                result.return_esi == entry.entry_esi &&
                result.return_edi == entry.entry_edi &&
                result.return_esp == entry.entry_esp + 8U &&
                result.return_eip == entry.entry_return_address &&
                (*fixture.startup->group_a_runtime_reset)[0U].field_2af4 ==
                    entry.effect_argument &&
                result.physical_call_count == 0U && result.port_calls == 0U,
            "zero-ready actor commits the entry effect argument and returns through the common RETN 4 path"
        );
    }

    {
        Fixture fixture;
        auto entry = request(group_a_token);
        entry.stop_before_actor_access = 0U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        actor_write_typed_stop &&
                result.stopped_access_ordinal == 0U &&
                result.stopped_instruction == 0x00478B71U &&
                result.stopped_token == group_a_token + 0x2AF4U &&
                result.return_eip == 0x00478B71U &&
                result.return_esp == entry.entry_esp - 16U &&
                (*fixture.startup->group_a_runtime_reset)[0U].field_2af4 == 0U,
            "first actor write fault stops at 00478B71 without committing the effect argument"
        );
    }

    {
        Fixture fixture;
        auto entry = request(group_a_token);
        entry.return_address_readable = false;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        return_address_read_typed_stop &&
                !result.returned && result.exit_instruction == 0x00479849U &&
                result.return_eip == 0x00479849U &&
                result.return_esp == entry.entry_esp &&
                (*fixture.startup->group_a_runtime_reset)[0U].field_2af4 ==
                    entry.effect_argument,
            "RETN 4 stack fault preserves actor writes and does not advance ESP"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.field_26b8 = 0x80000005U;
        fixture.port.push(
            0x0047BA80U, {.eax = 1U, .ecx = group_a_token, .edx = 0xA5A55A5AU}
        );
        const auto entry = request(group_a_token);
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::completed &&
                result.returned && result.exit_instruction == 0x00478CD1U &&
                actor.field_26b8 == 5U && result.port_calls == 1U &&
                result.physical_call_count == 2U &&
                result.physical_calls[0U].call_address == 0x00478CB3U &&
                result.physical_calls[0U].callee_token == 0x0047BA80U &&
                result.physical_calls[1U].call_address == 0x00478CC8U &&
                result.physical_calls[1U].callee_token ==
                    openswd3::battle::
                        kLegacyBattleActorField26b8HighBitClearAddress &&
                result.physical_calls[1U].return_address == 0x00478CCDU,
            "high-bit path records the query and typed clear CALL before returning at 00478CD1"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.field_26b8 = 0x80000005U;
        fixture.port.push(0x0047BA80U, {.eax = 1U});
        auto entry = request(group_a_token);
        entry.high_bit_clear_request.access.return_address_readable = false;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        high_bit_clear_typed_stop &&
                !result.returned && actor.field_26b8 == 5U &&
                result.port_calls == 1U && result.physical_call_count == 2U &&
                result.high_bit_clear.status ==
                    openswd3::battle::
                        LegacyBattleActorField26b8HighBitClearStatus::
                            return_address_read_typed_stop,
            "nested clear RET fault keeps the committed dword and both physical CALL identities"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U].progress.special_ready = 1U;
        const auto entry = request(group_a_token);
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        constexpr std::array<u32, 7U> call_addresses{
            0x00478FD1U,
            0x00478FF1U,
            0x004792A9U,
            0x00479444U,
            0x00479801U,
            0x00479808U,
            0x00479840U,
        };
        bool calls_match = result.physical_call_count == call_addresses.size();
        for (std::size_t index = 0U;
             calls_match && index < call_addresses.size();
             ++index) {
            calls_match = result.physical_calls[index].call_address ==
                call_addresses[index];
        }
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::completed &&
                result.returned && result.exit_instruction == 0x00479849U &&
                result.port_calls == call_addresses.size() && calls_match &&
                fixture.action->group_a_action_execution[0U]
                        .render_source_token == 0x71000000U &&
                fixture.action->group_a_action_execution[0U]
                        .resource.value_0c == 32U &&
                fixture.action->group_a_action_execution[0U]
                        .resource.value_0e == 48U &&
                fixture.action->group_a_action_execution[0U]
                        .resource.value_00 == 0x72000000U &&
                fixture.action->group_a_action_execution[0U]
                    .resource.value_00_known &&
                fixture.action->group_a_action_execution[0U]
                    .resource.value_0c_known &&
                fixture.action->group_a_action_execution[0U]
                    .resource.value_0e_known &&
                fixture.action->group_a_action_shared.turn_frame_source_token ==
                    0x72000000U &&
                fixture.action->group_a_action_shared.draw_height_third ==
                    16U &&
                fixture.action->group_a_action_shared.draw_height_quarter ==
                    12U,
            "ready actor reaches the common tail with the exact seven-call physical prefix and canonical resource writeback"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U].progress.special_ready = 1U;
        auto entry = request(group_a_token);
        entry.stop_before_call = 2U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        call_typed_stop &&
                result.stopped_call_ordinal == 2U &&
                result.return_eip == 0x004792A9U &&
                result.physical_call_count == 2U && result.port_calls == 2U &&
                result.physical_calls[0U].call_address == 0x00478FD1U &&
                result.physical_calls[1U].call_address == 0x00478FF1U &&
                fixture.action->group_a_action_execution[0U]
                        .render_source_token == 0x71000000U,
            "third physical CALL stop preserves the two completed calls and resource publication prefix"
        );
    }

    {
        for (const u32 random_value : {0U, 1U}) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& coordinates = fixture.startup->party[0U];
            actor.completion_word = 2U;
            coordinates.coordinate_mode_gate = 1U;
            coordinates.position_x = 10U;
            fixture.port.push(0x00439070U, {.eax = random_value});
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    request(group_a_token)
                );
            test.expect_true(
                result.returned && result.physical_call_count == 1U &&
                    result.physical_calls[0U].call_address == 0x00478C07U &&
                    result.physical_calls[0U].callee_token == 0x00439070U &&
                    actor.completion_word == 1U &&
                    coordinates.position_x == (random_value == 1U ? 9U : 10U),
                "mode 1 uses random AX exactly and decrements the completion word with 16-bit coordinates"
            );
        }
    }

    {
        struct ModeOnePolarityCase {
            u32 source_runtime;
            u32 mirror;
            u16 expected_x;
        };
        constexpr std::array<ModeOnePolarityCase, 4U> cases{
            ModeOnePolarityCase{0U, 0U, 9U},
            ModeOnePolarityCase{0U, 1U, 11U},
            ModeOnePolarityCase{1U, 0U, 11U},
            ModeOnePolarityCase{1U, 1U, 9U},
        };
        bool polarity_matches = true;
        for (const auto& item : cases) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& party = fixture.startup->party[0U];
            actor.completion_word = 1U;
            party.coordinate_mode_gate = 1U;
            party.position_x = 10U;
            party.configuration.source_runtime_value = item.source_runtime;
            party.progress.post_action_value = item.mirror;
            fixture.port.push(0x00439070U, {.eax = 1U});
            auto entry = request(group_a_token);
            if (item.source_runtime == 1U) {
                entry.stop_before_call = 1U;
            }
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            polarity_matches = polarity_matches &&
                party.position_x == item.expected_x &&
                actor.completion_word == 0U &&
                (item.source_runtime == 1U ? result.status ==
                         LegacyBattleActorActionPresentationStatus::
                             call_typed_stop
                                           : result.returned);
        }
        test.expect_true(
            polarity_matches,
            "mode 1 preserves all source-runtime and mirror direction polarities"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        auto& coordinates = fixture.startup->party[0U];
        actor.completion_word = 2U;
        coordinates.coordinate_mode_gate = 2U;
        coordinates.position_y = 5U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && coordinates.position_y == 0U &&
                actor.completion_word == 0U &&
                coordinates.coordinate_mode_gate == 0U,
            "mode 2 clamps a signed nonpositive post-subtraction coordinate and clears both mode words"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        auto& coordinates = fixture.startup->party[0U];
        actor.completion_word = 2U;
        coordinates.coordinate_mode_gate = 2U;
        coordinates.position_y = 0x8000U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && coordinates.position_y == 0x7FF6U &&
                actor.completion_word == 2U &&
                coordinates.coordinate_mode_gate == 2U,
            "mode 2 preserves 16-bit wrap before the signed threshold comparison"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        auto& coordinates = fixture.startup->party[0U];
        actor.completion_word = 2U;
        coordinates.coordinate_mode_gate = 3U;
        coordinates.position_y = 0xFFF0U;
        coordinates.alternate_position_y = 20U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && coordinates.position_y == 14U &&
                actor.completion_word == 2U &&
                coordinates.coordinate_mode_gate == 3U &&
                result.physical_call_count == 0U,
            "mode 3 keeps the below-threshold signed coordinate without invoking the high-bit child"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.field_26b8 = 0x80000005U;
        fixture.port.push(0x0047BA80U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && result.exit_instruction == 0x00479849U &&
                result.physical_call_count == 1U &&
                result.physical_calls[0U].call_address == 0x00478CB3U &&
                actor.field_26b8 == 0x80000005U,
            "high-bit query result other than one returns through the common exit without clearing the field"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.turn_completion_latch = 1U;
        const auto exact_one =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );

        Fixture other_fixture;
        auto& other_actor = other_fixture.action->group_a_action_execution[0U];
        other_fixture.startup->party[0U].progress.special_ready = 1U;
        other_actor.turn_completion_latch = 0x00010001U;
        auto other_entry = request(group_a_token);
        other_entry.stop_before_call = 0U;
        const auto other =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        other_fixture.owners(), group_a_token
                    ),
                other_fixture.platform(),
                other_entry
            );
        test.expect_true(
            exact_one.returned && exact_one.physical_call_count == 0U &&
                other.status ==
                    LegacyBattleActorActionPresentationStatus::
                        call_typed_stop &&
                other.return_eip == 0x00478FD1U,
            "turn-completion latch returns only for the exact dword value one"
        );
    }

    {
        struct EarlyGateCase {
            u32 source_runtime;
            u32 motion_mode;
            u32 scene_identity;
            u32 script_state;
            bool continues;
        };
        constexpr std::array<EarlyGateCase, 6U> cases{
            EarlyGateCase{0U, 1U, 1U, 0U, false},
            EarlyGateCase{0U, 1U, 1U, 1U, true},
            EarlyGateCase{1U, 0U, 1U, 0U, true},
            EarlyGateCase{1U, 1U, 0U, 0U, true},
            EarlyGateCase{1U, 1U, 1U, 0U, false},
            EarlyGateCase{1U, 1U, 1U, 1U, true},
        };
        constexpr std::array<const char*, 6U> labels{
            "non-runtime actor with zero script state returns",
            "non-runtime actor with nonzero script state continues",
            "runtime actor with non-one motion mode continues",
            "runtime actor with non-one scene identity continues",
            "runtime actor with matching motion and scene returns for zero script state",
            "runtime actor with matching motion and scene continues for nonzero script state",
        };
        for (std::size_t scenario = 0U; scenario < cases.size(); ++scenario) {
            const auto& item = cases[scenario];
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& party = fixture.startup->party[0U];
            party.configuration.source_runtime_value = item.source_runtime;
            actor.action_twenty_seven_motion_mode = item.motion_mode;
            party.progress.scene_identity = item.scene_identity;
            party.progress.script_binary_state = item.script_state;
            auto entry = request(group_a_token);
            entry.stop_before_call = item.source_runtime == 1U ? 1U : 0U;
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            const std::size_t expected_prefix =
                item.source_runtime == 1U ? 1U : 0U;
            bool scenario_matches = item.continues
                ? result.status ==
                        LegacyBattleActorActionPresentationStatus::
                            call_typed_stop &&
                    result.physical_call_count == expected_prefix
                : result.returned && result.physical_call_count == 0U;
            if (item.continues && expected_prefix == 1U) {
                scenario_matches = scenario_matches &&
                    result.physical_calls[0U].call_address == 0x00478D13U;
            }
            test.expect_true(scenario_matches, labels[scenario]);
        }
    }

    {
        constexpr std::array<u32, 11U> expected_actions{
            0x24U,
            0x26U,
            0x2CU,
            0x2DU,
            0x31U,
            0x33U,
            0x48U,
            0x49U,
            0x4AU,
            0x4BU,
            0xBEEFU,
        };
        constexpr std::array<const char*, 11U> labels{
            "default action selects 24",
            "live record threshold selects 26",
            "override reset selects 2C",
            "override high-byte bit6 selects 2D",
            "override high-byte bit3 selects 31",
            "runtime-ready actor selects 33",
            "presentation kind 1 selects 48",
            "presentation kind 2 selects 49",
            "presentation kind 4 selects 4A",
            "presentation kind 8 selects 4B",
            "full-word variant override replaces the selected action",
        };
        for (std::size_t scenario = 0U; scenario < expected_actions.size();
             ++scenario) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& party = fixture.startup->party[0U];
            party.progress.special_ready = 1U;
            auto entry = request(group_a_token);
            entry.stop_before_call = 0U;
            switch (scenario) {
            case 1U:
                party.progress.special_ready = 0U;
                party.configuration.source_runtime_value = 1U;
                party.configuration.actor_record[1U] = 1U;
                party.configuration.actor_record[2U] = 8U << 16U;
                entry.stop_before_call = 1U;
                break;

            case 2U:
                actor.action_override_flags = 0x0001U;
                break;

            case 3U:
                actor.action_override_flags = 0x4001U;
                break;

            case 4U:
                actor.action_override_flags = 0x0801U;
                break;

            case 5U:
                party.configuration.source_runtime_value = 1U;
                entry.stop_before_call = 1U;
                break;

            case 6U:
                actor.presentation_kind = 1U;
                break;

            case 7U:
                actor.presentation_kind = 2U;
                break;

            case 8U:
                actor.presentation_kind = 4U;
                break;

            case 9U:
                actor.presentation_kind = 8U;
                break;

            case 10U:
                actor.profile_variant_override = 0xBEEFU;
                break;

            default:
                break;
            }
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            const std::size_t expected_prefix =
                scenario == 1U || scenario == 5U ? 1U : 0U;
            bool scenario_matches = result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        call_typed_stop &&
                result.stopped_call_ordinal == expected_prefix &&
                actor.frame_source_action_record.base_variant ==
                    expected_actions[scenario];
            if (expected_prefix == 1U) {
                scenario_matches = scenario_matches &&
                    result.physical_calls[0U].call_address == 0x00478D13U &&
                    result.physical_calls[0U].callee_token == 0x0044A240U;
            }
            test.expect_true(scenario_matches, labels[scenario]);
        }
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        auto& party = fixture.startup->party[0U];
        party.progress.special_ready = 1U;
        party.item_effect_application.display_kind = 6U;
        actor.frame_source_action_record.field_5a = 8U;
        actor.special_particle_coordinate_suppression = 4U;
        auto entry = request(group_a_token);
        entry.stop_before_call = 0U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        call_typed_stop &&
                actor.action_kind == 6U &&
                actor.frame_source_action_record.field_5a == 0U &&
                actor.turn_action_record.action_id ==
                    actor.frame_source_action_record.action_id &&
                (party.progress.mode_gate & 8U) != 0U,
            "snapshot bit3 selects the display kind while actor bit2 copies the full action record and sets mode bit3"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U].progress.special_ready = 1U;
        fixture.port.push(0x004321E0U, {.eax = 0U});
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && result.exit_instruction == 0x00479849U &&
                result.physical_call_count == 1U &&
                result.physical_calls[0U].call_address == 0x00478FD1U &&
                result.physical_calls[0U].return_eax == 0U,
            "zero first-record update reply returns through the common tail before frame lookup"
        );
    }

    {
        struct ImmediateExitCase {
            u16 mode_gate;
            u16 action_kind;
            u32 call_address;
            u32 callee;
            u32 exit_instruction;
        };
        constexpr std::array<ImmediateExitCase, 4U> cases{
            ImmediateExitCase{8U, 0U, 0x00479143U, 0x0047F3C0U, 0x0047914CU},
            ImmediateExitCase{
                0x0200U, 0U, 0x00479156U, 0x0047F580U, 0x0047915FU
            },
            ImmediateExitCase{
                0x0080U, 0U, 0x0047917BU, 0x0047F710U, 0x00479184U
            },
            ImmediateExitCase{
                0x0400U, 0x1CU, 0x0047918FU, 0x0047F710U, 0x00479198U
            },
        };
        bool exits_match = true;
        for (const auto& item : cases) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& progress = fixture.startup->party[0U].progress;
            progress.special_ready = 1U;
            progress.mode_gate = item.mode_gate;
            actor.action_kind = item.action_kind;
            const auto entry = request(group_a_token);
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            exits_match = exits_match && result.returned &&
                result.exit_instruction == item.exit_instruction &&
                result.return_ebx == entry.entry_ebx &&
                result.return_ebp == entry.entry_ebp &&
                result.return_esi == entry.entry_esi &&
                result.return_edi == entry.entry_edi &&
                result.return_esp == entry.entry_esp + 8U &&
                result.physical_call_count == 3U &&
                result.physical_calls[2U].call_address == item.call_address &&
                result.physical_calls[2U].callee_token == item.callee;

            Fixture stopped_fixture;
            auto& stopped_actor =
                stopped_fixture.action->group_a_action_execution[0U];
            auto& stopped_progress =
                stopped_fixture.startup->party[0U].progress;
            stopped_progress.special_ready = 1U;
            stopped_progress.mode_gate = item.mode_gate;
            stopped_actor.action_kind = item.action_kind;
            auto stopped_entry = request(group_a_token);
            stopped_entry.return_address_readable = false;
            const auto stopped = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            stopped_fixture.owners(), group_a_token
                        ),
                    stopped_fixture.platform(),
                    stopped_entry
                );
            exits_match = exits_match &&
                stopped.status ==
                    LegacyBattleActorActionPresentationStatus::
                        return_address_read_typed_stop &&
                !stopped.returned &&
                stopped.exit_instruction == item.exit_instruction &&
                stopped.return_esp == stopped_entry.entry_esp;
        }
        test.expect_true(
            exits_match,
            "all four call-then-return exits preserve physical calls, callee identity, saved registers and RET faults"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.field_26b8 = 0x80000005U;
        fixture.port.push(0x0047BA80U, {.eax = 1U});
        auto entry = request(group_a_token);
        entry.return_address_readable = false;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        return_address_read_typed_stop &&
                !result.returned && result.exit_instruction == 0x00478CD1U &&
                result.return_eip == 0x00478CD1U &&
                result.return_esp == entry.entry_esp &&
                actor.field_26b8 == 5U && result.physical_call_count == 2U,
            "high-bit-clear RETN 4 fault preserves the nested clear and stops at the dedicated exit"
        );
    }

    {
        struct DrawCase {
            u32 field_26c0;
            u16 mode_gate;
            u32 overlay;
            u16 action_kind;
            u32 expected_call;
        };
        constexpr std::array<DrawCase, 6U> cases{
            DrawCase{0x80000000U, 0U, 0U, 0U, 0x004794C2U},
            DrawCase{0x02000000U, 0U, 0U, 0U, 0x0047931FU},
            DrawCase{0x04000000U, 0U, 0U, 0U, 0x00479461U},
            DrawCase{0U, 4U, 0U, 0U, 0x0047952DU},
            DrawCase{0U, 0U, 1U, 0U, 0x0047938FU},
            DrawCase{0U, 0U, 0U, 2U, 0x004795ABU},
        };
        bool draw_cases_match = true;
        for (const auto& item : cases) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& progress = fixture.startup->party[0U].progress;
            progress.special_ready = 1U;
            progress.mode_gate = item.mode_gate;
            actor.field_26c0 = item.field_26c0;
            actor.overlay_render_enabled = item.overlay;
            actor.action_kind = item.action_kind;
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    request(group_a_token)
                );
            bool found = false;
            for (std::size_t index = 0U; index < result.physical_call_count;
                 ++index) {
                found = found ||
                    result.physical_calls[index].call_address ==
                        item.expected_call;
            }
            draw_cases_match = draw_cases_match && result.returned && found;
        }
        test.expect_true(
            draw_cases_match,
            "bit31, bit25, bit26, mode bit2, overlay and action-kind-2 drawing branches retain their physical CALLs"
        );
    }

    {
        bool ordered = true;
        for (const bool bit31 : {true, false}) {
            const u32 read_address = bit31 ? 0x00479479U : 0x004794E4U;
            const u32 write_address = bit31 ? 0x0047947FU : 0x004794EAU;
            const u32 resource_address = bit31 ? 0x0047948DU : 0x004794F8U;
            Fixture baseline_fixture;
            baseline_fixture.startup->party[0U].progress.special_ready = 1U;
            baseline_fixture.startup->party[0U].progress.mode_gate =
                bit31 ? 0U : 4U;
            baseline_fixture.action->group_a_action_execution[0U].field_26c0 =
                bit31 ? 0x80000000U : 0U;
            const auto baseline = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            baseline_fixture.owners(), group_a_token
                        ),
                    baseline_fixture.platform(),
                    request(group_a_token)
                );
            std::size_t read_ordinal = baseline.actor_accesses_completed;
            std::size_t write_ordinal = baseline.actor_accesses_completed;
            std::size_t resource_ordinal = baseline.actor_accesses_completed;
            bool committed_before_resource = false;
            for (std::size_t ordinal = 0U;
                 ordinal < baseline.actor_accesses_completed;
                 ++ordinal) {
                Fixture fixture;
                fixture.startup->party[0U].progress.special_ready = 1U;
                fixture.startup->party[0U].progress.mode_gate = bit31 ? 0U : 4U;
                auto& actor = fixture.action->group_a_action_execution[0U];
                actor.field_26c0 = bit31 ? 0x80000000U : 0U;
                auto entry = request(group_a_token);
                entry.stop_before_actor_access = ordinal;
                const auto stopped = openswd3::battle::
                    advance_legacy_battle_actor_action_presentation(
                        openswd3::battle::
                            resolve_legacy_battle_actor_action_presentation(
                                fixture.owners(), group_a_token
                            ),
                        fixture.platform(),
                        entry
                    );
                if (stopped.stopped_instruction == read_address &&
                    stopped.status ==
                        LegacyBattleActorActionPresentationStatus::
                            actor_read_typed_stop) {
                    read_ordinal = ordinal;
                }
                if (stopped.stopped_instruction == write_address &&
                    stopped.status ==
                        LegacyBattleActorActionPresentationStatus::
                            actor_write_typed_stop) {
                    write_ordinal = ordinal;
                }
                if (stopped.stopped_instruction == resource_address &&
                    stopped.status ==
                        LegacyBattleActorActionPresentationStatus::
                            resource_record_read_typed_stop) {
                    resource_ordinal = ordinal;
                    committed_before_resource = actor.render_flags ==
                        (actor.presentation_render_flags & 0x80000003U);
                }
            }
            ordered = ordered && baseline.returned &&
                read_ordinal < baseline.actor_accesses_completed &&
                write_ordinal == read_ordinal + 1U &&
                resource_ordinal == write_ordinal + 1U &&
                committed_before_resource;
        }
        test.expect_true(
            ordered,
            "bit31 and mode-bit2 paths read y adjustment, commit actor flags, then read resource in LST order"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U].progress.special_ready = 1U;
        fixture.startup->party[0U].workspace.special_item_latch = 1U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        bool recorded_decimal_call = false;
        for (std::size_t index = 0U; index < result.physical_call_count;
             ++index) {
            recorded_decimal_call = recorded_decimal_call ||
                (result.physical_calls[index].call_address == 0x004795EFU &&
                 result.physical_calls[index].callee_token == 0x004507A0U);
        }
        test.expect_true(
            result.status ==
                    LegacyBattleActorActionPresentationStatus::
                        decimal_draw_typed_stop &&
                !result.returned && result.decimal_draw_calls == 1U &&
                result.decimal_draw.status ==
                    openswd3::battle::LegacyBattleTenPlaceDecimalStatus::
                        place_typed_stop &&
                recorded_decimal_call,
            "ten-place decimal child typed stop preserves the physical 004795EF CALL and suppresses the suffix"
        );
    }

    {
        Fixture fixture;
        fixture.startup->party[0U].progress.special_ready = 1U;
        fixture.startup->party[0U].workspace.special_item_latch = 1U;
        fixture.frame_provider.prepare();
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        test.expect_true(
            result.returned && result.decimal_draw_calls == 1U &&
                result.decimal_draw.status ==
                    openswd3::battle::LegacyBattleTenPlaceDecimalStatus::
                        completed &&
                result.decimal_draw.call_count != 0U &&
                fixture.frame_provider.calls != 0U,
            "ten-place decimal child completes normally with an available frame provider"
        );
    }

    {
        constexpr std::array<u16, 2U> positions{319U, 320U};
        bool pan_cases_match = true;
        for (const u16 position_x : positions) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            fixture.startup->party[0U].progress.special_ready = 1U;
            fixture.startup->party[0U].position_x = position_x;
            actor.frame_source_action_record.field_58 = 0x1234U;
            const auto result = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    request(group_a_token)
                );
            bool pan_matches = false;
            for (const auto& call : fixture.port.calls) {
                pan_matches = pan_matches ||
                    (call.callee_token == 0x00485650U &&
                     call.arguments[0U] == 0x1234U &&
                     call.arguments[1U] ==
                         (position_x < 320U ? 0xFFFFFFF0U : 0x10U));
            }
            pan_cases_match = pan_cases_match && result.returned &&
                pan_matches && actor.frame_source_action_record.field_58 == 0U;
        }
        test.expect_true(
            pan_cases_match,
            "sample pan uses signed x threshold sides and clears the sample word"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        auto& party = fixture.startup->party[0U];
        party.progress.special_ready = 1U;
        party.configuration.source_runtime_value = 1U;
        actor.action_kind = 6U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        constexpr std::array<u32, 4U> expected_calls{
            0x00479675U,
            0x00479695U,
            0x00479706U,
            0x0047971AU,
        };
        bool calls_match = result.returned;
        for (const u32 expected_call : expected_calls) {
            bool found = false;
            for (std::size_t index = 0U; index < result.physical_call_count;
                 ++index) {
                found = found ||
                    result.physical_calls[index].call_address == expected_call;
            }
            calls_match = calls_match && found;
        }
        test.expect_true(
            calls_match && actor.turn_frame_token == 0x71000000U,
            "action kind 6 runtime path preserves second-record update, lookup, draw and sample CALL order"
        );
    }

    {
        Fixture fixture;
        auto& actor = fixture.action->group_a_action_execution[0U];
        fixture.startup->party[0U].progress.special_ready = 1U;
        actor.frame_source_action_record.field_4e = 1U;
        actor.frame_source_action_record.field_50 = 1U;
        const auto result =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        fixture.owners(), group_a_token
                    ),
                fixture.platform(),
                request(group_a_token)
            );
        bool lookup_found = false;
        bool draw_found = false;
        for (std::size_t index = 0U; index < result.physical_call_count;
             ++index) {
            lookup_found = lookup_found ||
                result.physical_calls[index].call_address == 0x00479785U;
            draw_found = draw_found ||
                result.physical_calls[index].call_address == 0x004797F7U;
        }
        test.expect_true(
            result.returned && lookup_found && draw_found &&
                actor.additional_render_source_token == 0x71000000U,
            "additional record path retains lookup, draw and canonical source-token writeback"
        );
    }

    {
        Fixture baseline_fixture;
        baseline_fixture.startup->party[0U].progress.special_ready = 1U;
        const auto baseline =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        baseline_fixture.owners(), group_a_token
                    ),
                baseline_fixture.platform(),
                request(group_a_token)
            );
        bool all_actor_stops_match =
            baseline.returned && baseline.actor_accesses_completed != 0U;
        std::size_t resource_stops{};
        bool write_479276_before_resource = false;
        bool resource_47927c_after_write = false;
        std::size_t read_47902a_ordinal = baseline.actor_accesses_completed;
        std::size_t write_479035_ordinal = baseline.actor_accesses_completed;
        std::size_t write_479276_ordinal{};
        for (std::size_t ordinal = 0U;
             ordinal < baseline.actor_accesses_completed;
             ++ordinal) {
            Fixture fixture;
            fixture.startup->party[0U].progress.special_ready = 1U;
            auto entry = request(group_a_token);
            entry.stop_before_actor_access = ordinal;
            const auto stopped = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            all_actor_stops_match = all_actor_stops_match &&
                !stopped.returned &&
                stopped.actor_accesses_completed == ordinal &&
                stopped.stopped_access_ordinal == ordinal &&
                stopped.status !=
                    LegacyBattleActorActionPresentationStatus::completed;
            if (stopped.status ==
                LegacyBattleActorActionPresentationStatus::
                    resource_record_read_typed_stop) {
                ++resource_stops;
            }
            if (stopped.stopped_instruction == 0x0047902AU &&
                stopped.status ==
                    LegacyBattleActorActionPresentationStatus::
                        actor_read_typed_stop) {
                read_47902a_ordinal = ordinal;
            }
            if (stopped.stopped_instruction == 0x00479035U &&
                stopped.status ==
                    LegacyBattleActorActionPresentationStatus::
                        actor_write_typed_stop) {
                write_479035_ordinal = ordinal;
            }
            if (stopped.stopped_instruction == 0x00479276U) {
                write_479276_before_resource = stopped.status ==
                        LegacyBattleActorActionPresentationStatus::
                            actor_write_typed_stop &&
                    fixture.action->group_a_action_execution[0U].render_flags ==
                        0U;
                write_479276_ordinal = ordinal;
            }
            if (stopped.stopped_instruction == 0x0047927CU) {
                resource_47927c_after_write = stopped.status ==
                        LegacyBattleActorActionPresentationStatus::
                            resource_record_read_typed_stop &&
                    ordinal == write_479276_ordinal + 1U &&
                    fixture.action->group_a_action_execution[0U].render_flags ==
                        0x0CU;
            }
        }
        test.expect_true(
            read_47902a_ordinal < baseline.actor_accesses_completed &&
                write_479035_ordinal == read_47902a_ordinal + 1U &&
                write_479276_before_resource && resource_47927c_after_write,
            "LST 0x47902A read precedes 0x479035 write; 0x479276 write precedes 0x47927C resource read"
        );
        test.expect_true(
            all_actor_stops_match,
            "every normal-path actor and resource access ordinal stops before the faulting access with its committed prefix"
        );
        test.expect_true(
            resource_stops != 0U &&
                resource_stops == baseline.resource_record_reads,
            "normal-path resource typed stops cover every successful baseline resource read"
        );
    }

    {
        Fixture baseline_fixture;
        auto& baseline_actor =
            baseline_fixture.action->group_a_action_execution[0U];
        auto& baseline_party = baseline_fixture.startup->party[0U];
        baseline_party.configuration.source_runtime_value = 1U;
        baseline_actor.action_twenty_seven_motion_mode = 0U;
        auto baseline_entry = request(group_a_token);
        baseline_entry.stop_before_call = 1U;
        const auto baseline =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        baseline_fixture.owners(), group_a_token
                    ),
                baseline_fixture.platform(),
                baseline_entry
            );
        bool all_live_stops_match = baseline.status ==
            LegacyBattleActorActionPresentationStatus::call_typed_stop;
        std::size_t nested_stops{};
        for (std::size_t ordinal = 0U;
             ordinal < baseline.actor_accesses_completed;
             ++ordinal) {
            Fixture fixture;
            auto& actor = fixture.action->group_a_action_execution[0U];
            auto& party = fixture.startup->party[0U];
            party.configuration.source_runtime_value = 1U;
            actor.action_twenty_seven_motion_mode = 0U;
            auto entry = request(group_a_token);
            entry.stop_before_call = 1U;
            entry.stop_before_actor_access = ordinal;
            const auto stopped = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            all_live_stops_match = all_live_stops_match && !stopped.returned &&
                stopped.actor_accesses_completed == ordinal &&
                stopped.stopped_access_ordinal == ordinal;
            if (stopped.status ==
                LegacyBattleActorActionPresentationStatus::
                    nested_record_read_typed_stop) {
                ++nested_stops;
            }
        }
        test.expect_true(
            all_live_stops_match && nested_stops == 2U,
            "live-record path exposes both nested word reads as independent prefix-preserving stops"
        );
    }

    {
        Fixture baseline_fixture;
        baseline_fixture.startup->party[0U].progress.special_ready = 1U;
        const auto baseline =
            openswd3::battle::advance_legacy_battle_actor_action_presentation(
                openswd3::battle::
                    resolve_legacy_battle_actor_action_presentation(
                        baseline_fixture.owners(), group_a_token
                    ),
                baseline_fixture.platform(),
                request(group_a_token)
            );
        bool all_call_stops_match =
            baseline.returned && baseline.physical_call_count != 0U;
        for (std::size_t ordinal = 0U; ordinal < baseline.physical_call_count;
             ++ordinal) {
            Fixture fixture;
            fixture.startup->party[0U].progress.special_ready = 1U;
            auto entry = request(group_a_token);
            entry.stop_before_call = ordinal;
            const auto stopped = openswd3::battle::
                advance_legacy_battle_actor_action_presentation(
                    openswd3::battle::
                        resolve_legacy_battle_actor_action_presentation(
                            fixture.owners(), group_a_token
                        ),
                    fixture.platform(),
                    entry
                );
            all_call_stops_match = all_call_stops_match &&
                stopped.status ==
                    LegacyBattleActorActionPresentationStatus::
                        call_typed_stop &&
                stopped.stopped_call_ordinal == ordinal &&
                stopped.physical_call_count == ordinal &&
                stopped.port_calls == ordinal &&
                stopped.return_eip ==
                    baseline.physical_calls[ordinal].call_address;
        }
        test.expect_true(
            all_call_stops_match,
            "every normal-path pending CALL ordinal stops before invocation and preserves the exact completed prefix"
        );
    }

    {
        Fixture fixture;
        openswd3::battle::LegacyBattleActorActionPresentationCallTrace trace{};
        openswd3::battle::LegacyBattleActorActionPresentationCallRequests
            requests{};
        requests.count = 1U;
        requests.requests[0U].return_address_readable = false;
        const bool returned = openswd3::battle::
            execute_legacy_battle_actor_action_presentation_call(
                fixture.owners(),
                fixture.platform(),
                trace,
                requests,
                0x004566DBU,
                0x004566E0U,
                group_a_token,
                1U,
                0x12345678U,
                0x9ABCDEF0U
            );
        test.expect_true(
            !returned && trace.calls == 1U &&
                trace.call_addresses[0U] == 0x004566DBU &&
                trace.return_addresses[0U] == 0x004566E0U &&
                trace.actor_tokens[0U] == group_a_token &&
                trace.effect_arguments[0U] == 1U &&
                trace.last.status ==
                    LegacyBattleActorActionPresentationStatus::
                        return_address_read_typed_stop,
            "caller wrapper retains physical identity and a typed-stop result"
        );
    }
}
