#include "openswd3/battle/legacy_battle_actor_frame_snapshot.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorFrameSnapshotRequest;
using openswd3::battle::LegacyBattleActorFrameSnapshotStatus;
using openswd3::battle::LegacyBattleActorFrameSnapshotView;
using openswd3::compat::u32;

class StreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        ++calls;
        return {};
    }

    u32 calls{};
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        requests.push_back({resource_id, piece_index});
        piece.width = width;
        piece.height = height;
        return available;
    }

    bool available{true};
    openswd3::compat::u16 width{20U};
    openswd3::compat::u16 height{30U};
    std::vector<std::array<u32, 2>> requests;
};

struct LeafFixture {
    u32 special_ready{};
    u32 source_runtime{};
    openswd3::compat::u16 profile{2U};
    u32 anchor{};
    u32 mirror{};
    u32 frame_token{0xCCCCCCCCU};
    openswd3::compat::u16 position_x{100U};
    openswd3::compat::u16 position_y{200U};
    bool special_ready_readable{true};
    bool source_runtime_readable{true};
    bool profile_readable{true};
    bool anchor_readable{true};
    bool mirror_readable{true};
    bool frame_token_readable{true};
    bool frame_token_writable{true};
    bool position_x_readable{true};
    bool position_y_readable{true};
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater updater{stream_provider};
    FrameProvider frame_provider;

    [[nodiscard]] LegacyBattleActorFrameSnapshotView view() noexcept {
        return {
            .special_ready = &special_ready,
            .source_runtime_value = &source_runtime,
            .profile_value = &profile,
            .frame_anchor_x = &anchor,
            .mirror_mode = &mirror,
            .frame_token = &frame_token,
            .position_x = &position_x,
            .position_y = &position_y,
            .special_ready_read_accessible = &special_ready_readable,
            .source_runtime_value_read_accessible = &source_runtime_readable,
            .profile_value_read_accessible = &profile_readable,
            .frame_anchor_x_read_accessible = &anchor_readable,
            .mirror_mode_read_accessible = &mirror_readable,
            .frame_token_read_accessible = &frame_token_readable,
            .frame_token_write_accessible = &frame_token_writable,
            .position_x_read_accessible = &position_x_readable,
            .position_y_read_accessible = &position_y_readable,
        };
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActorFrameSnapshotResult
    run(const LegacyBattleActorFrameSnapshotRequest& request = {}) {
        return openswd3::battle::query_legacy_battle_actor_frame_snapshot(
            view(), updater, frame_provider, request
        );
    }
};

}  // namespace

void test_battle_actor_frame_snapshot(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleStartupState startup;
        openswd3::battle::LegacyBattleActionDispatchState action;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_frame_snapshot(
                {.action = &action, .startup = &startup},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_frame_snapshot(
                {.action = &action, .startup = &startup},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken
            );
        test.expect_true(
            group_a.position_x == &startup.party[0U].position_x &&
                group_a.special_ready ==
                    &startup.party[0U].progress.special_ready &&
                group_a.profile_value ==
                    &action.group_a_action_execution[0U].profile_value &&
                group_b.source_runtime_value ==
                    &(*startup.group_b_lifecycle)[0U]
                         .action_configuration.source_runtime_value &&
                group_b.frame_token ==
                    &(*startup.group_b_lifecycle)[0U]
                         .action_execution.turn_frame_token,
            "frame snapshot resolves split Group-A and lifecycle Group-B owners"
        );
    }

    {
        LeafFixture fixture;
        fixture.position_x = 10U;
        fixture.position_y = 0xFFFEU;
        const auto result = fixture.run({
            .actor_token = 0x005029D0U,
            .output_token = 0x70001000U,
            .entry_edx = 0xAABBCCDDU,
            .frame_provider_return_eax = 0x0BADF00DU,
        });
        test.expect_true(
            result.status == LegacyBattleActorFrameSnapshotStatus::completed &&
                result.output ==
                    std::array<u32, 4>{0xFFFFFFE6U, 0xFFFFFFFEU, 20U, 30U} &&
                result.action_record.base_variant == 2U &&
                result.action_record.draw_offset_x == 0x24U &&
                result.action_update_calls == 1U &&
                fixture.stream_provider.calls == 0U &&
                result.frame_lookup_calls == 1U &&
                result.overlapping_resource_dword == 0U &&
                result.overlapping_frame_dword == 0U &&
                fixture.frame_provider.requests ==
                    std::vector<std::array<u32, 2>>{{0U, 0U}} &&
                fixture.frame_token == 0x0BADF00DU &&
                result.return_eax == 0x70001000U && result.return_ecx == 30U &&
                result.return_edx == 0x0BADF00DU && result.flags_known &&
                !result.flags.carry && result.flags.parity &&
                !result.flags.auxiliary_carry_defined && result.flags.zero &&
                !result.flags.sign && !result.flags.overflow &&
                result.actor_reads == 8U && result.actor_writes == 1U &&
                result.local_reads == 2U && result.frame_reads == 2U &&
                result.output_pointer_reads == 1U && result.output_writes == 4U,
            "frame snapshot preserves signed coordinates, full dword outputs, calls, registers, and final XOR flags"
        );
    }

    {
        LeafFixture fixture;
        fixture.special_ready = 1U;
        fixture.source_runtime = 7U;
        fixture.anchor = 9U;
        fixture.mirror = 1U;
        fixture.position_x = 100U;
        const auto result = fixture.run({.actor_token = 0x005029D0U});
        test.expect_true(
            result.status == LegacyBattleActorFrameSnapshotStatus::completed &&
                result.action_record.draw_offset_x == 0x33U &&
                result.output[0U] == 131U && result.mirrored &&
                result.frame_reads == 3U,
            "mode one forces the 0x33 anchor after the actor override and mirrors through frame width"
        );
    }

    {
        LeafFixture fixture;
        fixture.special_ready = 1U;
        fixture.source_runtime = 0U;
        const auto result = fixture.run({
            .entry_edx = 0x12345678U,
            .initial_output = {1U, 2U, 3U, 4U},
        });
        test.expect_true(
            result.status == LegacyBattleActorFrameSnapshotStatus::completed &&
                result.returned_early && result.return_eax == 0U &&
                result.return_ecx == 1U && result.return_edx == 0x12345678U &&
                result.flags.zero &&
                result.output == std::array<u32, 4>{1U, 2U, 3U, 4U} &&
                result.action_update_calls == 0U &&
                result.frame_lookup_calls == 0U,
            "mode-one missing source returns after TEST EAX without touching the output block"
        );
    }

    {
        LeafFixture fixture;
        fixture.special_ready = 1U;
        fixture.source_runtime = 0xABCD0001U;
        fixture.profile = 0U;
        const auto result = fixture.run({.entry_edx = 0xCAFEBABEU});
        test.expect_true(
            result.status == LegacyBattleActorFrameSnapshotStatus::completed &&
                result.returned_early && result.return_eax == 0xABCD0000U &&
                result.return_ecx == 1U && result.return_edx == 0xCAFEBABEU &&
                result.flags.zero && result.flags.parity && !result.flags.sign,
            "zero profile preserves the source high word on the mode-one early return"
        );
    }

    {
        LeafFixture fixture;
        fixture.frame_provider.available = false;
        const auto result = fixture.run({
            .actor_token = 0x005029D0U,
            .frame_provider_return_eax = 0U,
            .initial_output = {9U, 8U, 7U, 6U},
        });
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_width_read_typed_stop &&
                fixture.frame_token == 0U &&
                result.output == std::array<u32, 4>{64U, 200U, 7U, 6U} &&
                result.output_writes == 2U && result.frame_reads == 0U,
            "missing frame commits the null token and preserves X/Y before the first width dereference stop"
        );
    }

    {
        LeafFixture fixture;
        const u32 actor_token = 0x005029D0U;
        const auto result = fixture.run({
            .actor_token = actor_token,
            .output_token = actor_token + 0x254CU,
            .initial_output = {9U, 8U, 7U, 6U},
        });
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_width_read_typed_stop &&
                fixture.frame_token == 64U &&
                result.output == std::array<u32, 4>{64U, 200U, 7U, 6U} &&
                result.output_writes == 2U,
            "X output alias overwrites actor frame token before the first physical reload"
        );
    }

    {
        LeafFixture fixture;
        const u32 actor_token = 0x005029D0U;
        const auto result = fixture.run({
            .actor_token = actor_token,
            .output_token = actor_token + 0x2544U,
            .initial_output = {9U, 8U, 7U, 6U},
        });
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_height_read_typed_stop &&
                fixture.frame_token == 20U &&
                result.output == std::array<u32, 4>{64U, 200U, 20U, 6U} &&
                result.output_writes == 3U,
            "width output alias is observed by the second frame-token reload and preserves three stores"
        );
    }

    {
        LeafFixture fixture;
        fixture.position_y_readable = false;
        const auto result = fixture.run({
            .actor_token = 0x005029D0U,
            .output_token = 0x80000000U,
            .initial_output = {1U, 2U, 3U, 4U},
        });
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        position_y_read_typed_stop &&
                result.output == std::array<u32, 4>{64U, 2U, 3U, 4U} &&
                result.output_writes == 1U &&
                result.return_eax == 0x80000000U && result.return_edx == 0U,
            "Y source stop preserves the committed X dword and the local-Y EDX residue"
        );
    }

    {
        LeafFixture fixture;
        auto request = LegacyBattleActorFrameSnapshotRequest{
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
        };
        request.output_writable[1U] = false;
        const auto result = fixture.run(request);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        output_y_write_typed_stop &&
                result.output == std::array<u32, 4>{64U, 2U, 3U, 4U} &&
                result.output_writes == 1U && result.return_ecx == 200U,
            "Y output stop preserves the X prefix and final Y SUB result"
        );
    }

    {
        LeafFixture fixture;
        fixture.mirror = 1U;
        fixture.frame_provider.available = false;
        const auto result = fixture.run({
            .actor_token = 0x005029D0U,
            .frame_provider_return_eax = 0U,
            .frame_provider_return_edx = 0x11223344U,
        });
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        mirror_frame_width_read_typed_stop &&
                fixture.frame_token == 0U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0x11223344U &&
                result.flags.zero && result.flags.parity &&
                !result.flags.auxiliary_carry_defined,
            "mirrored null frame stops at the pre-load XOR with provider EDX residue"
        );
    }

    {
        LeafFixture frame_parameter;
        const auto frame_parameter_stop = frame_parameter.run({
            .action_updater_return_ecx = 0x11111111U,
            .action_updater_return_edx = 0x22222222U,
            .action_updater_flags = {.carry = true},
            .action_updater_flags_known = true,
            .overlapping_frame_dword_readable = false,
        });
        LeafFixture resource_parameter;
        const auto resource_parameter_stop = resource_parameter.run({
            .action_updater_return_ecx = 0x33333333U,
            .action_updater_return_edx = 0x44444444U,
            .action_updater_flags = {.zero = true},
            .action_updater_flags_known = true,
            .overlapping_resource_dword_readable = false,
        });
        test.expect_true(
            frame_parameter_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        overlapping_frame_dword_read_typed_stop &&
                frame_parameter_stop.action_update_calls == 1U &&
                frame_parameter_stop.frame_lookup_calls == 0U &&
                frame_parameter_stop.local_reads == 0U &&
                frame_parameter_stop.return_eax == 1U &&
                frame_parameter_stop.return_ecx == 0x11111111U &&
                frame_parameter_stop.return_edx == 0x22222222U &&
                frame_parameter_stop.flags_known &&
                frame_parameter_stop.flags.carry &&
                resource_parameter_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        overlapping_resource_dword_read_typed_stop &&
                resource_parameter_stop.action_update_calls == 1U &&
                resource_parameter_stop.frame_lookup_calls == 0U &&
                resource_parameter_stop.local_reads == 1U &&
                resource_parameter_stop.return_eax == 1U &&
                resource_parameter_stop.return_ecx == 0U &&
                resource_parameter_stop.return_edx == 0x44444444U &&
                resource_parameter_stop.flags_known &&
                resource_parameter_stop.flags.zero,
            "overlapping local dword reads stop independently with updater register and flag residues"
        );
    }

    {
        LeafFixture source;
        source.special_ready = 1U;
        source.source_runtime_readable = false;
        const auto source_stop = source.run({.entry_edx = 0x12345678U});
        LeafFixture anchor;
        anchor.anchor_readable = false;
        const auto anchor_stop = anchor.run();
        LeafFixture mirror;
        mirror.mirror_readable = false;
        const auto mirror_stop = mirror.run({
            .actor_token = 0x005029D0U,
            .frame_provider_return_eax = 0xDEADC0DEU,
            .frame_provider_return_ecx = 0xAAAAAAAAU,
            .frame_provider_return_edx = 0xBBBBBBBBU,
        });
        test.expect_true(
            source_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        source_runtime_value_read_typed_stop &&
                source_stop.actor_reads == 1U && source_stop.return_eax == 0U &&
                source_stop.return_ecx == 1U &&
                source_stop.return_edx == 0x12345678U &&
                anchor_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_anchor_x_read_typed_stop &&
                anchor_stop.actor_reads == 2U &&
                mirror_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        mirror_mode_read_typed_stop &&
                mirror_stop.action_update_calls == 1U &&
                mirror_stop.frame_lookup_calls == 1U &&
                mirror_stop.return_eax == 0xDEADC0DEU &&
                mirror_stop.return_ecx == 0xAAAAAAAAU &&
                mirror_stop.return_edx == 0xBBBBBBBBU &&
                !mirror_stop.flags_known && mirror.frame_token == 0xCCCCCCCCU,
            "source, anchor, and mirror faults stop at their physical reads without later commits"
        );
    }

    {
        LeafFixture position;
        position.position_x_readable = false;
        const auto position_stop = position.run({
            .actor_token = 0x005029D0U,
            .frame_provider_return_edx = 0x11112222U,
            .initial_output = {1U, 2U, 3U, 4U},
        });
        LeafFixture pointer;
        const auto pointer_stop = pointer.run({
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
            .output_pointer_readable = false,
        });
        LeafFixture output;
        auto output_request = LegacyBattleActorFrameSnapshotRequest{
            .actor_token = 0x005029D0U,
            .output_token = 0x70000000U,
            .initial_output = {1U, 2U, 3U, 4U},
        };
        output_request.output_writable[0U] = false;
        const auto output_stop = output.run(output_request);
        test.expect_true(
            position_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        position_x_read_typed_stop &&
                position_stop.return_eax == 0x24U &&
                position_stop.return_ecx == 0U &&
                position_stop.return_edx == 0x11112222U &&
                position_stop.output_writes == 0U &&
                pointer_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        output_pointer_read_typed_stop &&
                pointer_stop.return_eax == 0x24U &&
                pointer_stop.return_edx == 64U &&
                pointer_stop.output == std::array<u32, 4>{1U, 2U, 3U, 4U} &&
                output_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        output_x_write_typed_stop &&
                output_stop.return_eax == 0x70000000U &&
                output_stop.return_edx == 64U &&
                output_stop.output_writes == 0U,
            "X source, output-pointer, and X store stops preserve the exact pre-fault residues"
        );
    }

    {
        LeafFixture first_token;
        const auto first_token_stop = first_token.run({
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
            .first_frame_token_readable = false,
        });
        LeafFixture frame_width;
        const auto frame_width_stop = frame_width.run({
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
            .frame_width_readable = false,
        });
        LeafFixture output_width;
        auto output_width_request = LegacyBattleActorFrameSnapshotRequest{
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
        };
        output_width_request.output_writable[2U] = false;
        const auto output_width_stop = output_width.run(output_width_request);
        LeafFixture second_token;
        const auto second_token_stop = second_token.run({
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
            .second_frame_token_readable = false,
        });
        test.expect_true(
            first_token_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        first_frame_token_read_typed_stop &&
                first_token_stop.output ==
                    std::array<u32, 4>{64U, 200U, 3U, 4U} &&
                first_token_stop.return_edx == 0U &&
                frame_width_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_width_read_typed_stop &&
                frame_width_stop.return_ecx == 0U &&
                frame_width_stop.output_writes == 2U &&
                output_width_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        output_width_write_typed_stop &&
                output_width_stop.return_ecx == 20U &&
                output_width_stop.output_writes == 2U &&
                second_token_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        second_frame_token_read_typed_stop &&
                second_token_stop.return_ecx == 20U &&
                second_token_stop.return_edx == 1U &&
                second_token_stop.output ==
                    std::array<u32, 4>{64U, 200U, 20U, 4U},
            "width-phase stops retain the X/Y or X/Y/width physical store prefix"
        );
    }

    {
        LeafFixture frame_height;
        const auto frame_height_stop = frame_height.run({
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
            .frame_height_readable = false,
        });
        LeafFixture output_height;
        auto output_height_request = LegacyBattleActorFrameSnapshotRequest{
            .actor_token = 0x005029D0U,
            .initial_output = {1U, 2U, 3U, 4U},
        };
        output_height_request.output_writable[3U] = false;
        const auto output_height_stop =
            output_height.run(output_height_request);
        LeafFixture nonbinary_mirror;
        nonbinary_mirror.anchor = 9U;
        nonbinary_mirror.mirror = 2U;
        const auto nonbinary = nonbinary_mirror.run({
            .actor_token = 0x005029D0U,
        });
        test.expect_true(
            frame_height_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_height_read_typed_stop &&
                frame_height_stop.return_ecx == 0U &&
                frame_height_stop.output ==
                    std::array<u32, 4>{64U, 200U, 20U, 4U} &&
                output_height_stop.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        output_height_write_typed_stop &&
                output_height_stop.return_ecx == 30U &&
                output_height_stop.output_writes == 3U &&
                nonbinary.status ==
                    LegacyBattleActorFrameSnapshotStatus::completed &&
                !nonbinary.mirrored && nonbinary.output[0U] == 91U,
            "height stops preserve three stores and mirror values other than one stay unmirrored"
        );
    }

    {
        LeafFixture fixture;
        fixture.special_ready_readable = false;
        const auto first = fixture.run({.entry_edx = 0x12345678U});
        fixture.special_ready_readable = true;
        fixture.profile_readable = false;
        const auto profile = fixture.run({.entry_edx = 0x12345678U});
        fixture.profile_readable = true;
        fixture.frame_token_writable = false;
        const auto commit = fixture.run({
            .actor_token = 0x005029D0U,
            .frame_provider_return_ecx = 0xAAAAAAAAU,
            .frame_provider_return_edx = 0xBBBBBBBBU,
        });
        test.expect_true(
            first.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        special_ready_read_typed_stop &&
                first.return_eax == 0U && first.return_ecx == 0U &&
                first.return_edx == 0x12345678U && first.flags.zero &&
                profile.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        profile_value_read_typed_stop &&
                profile.return_ecx == 0U &&
                commit.status ==
                    LegacyBattleActorFrameSnapshotStatus::
                        frame_token_write_typed_stop &&
                commit.return_eax == 1U && commit.return_ecx == 0U &&
                commit.return_edx == 0xBBBBBBBBU,
            "typed views stop at the first inaccessible actor read or frame-token store with current residues"
        );
    }
}
