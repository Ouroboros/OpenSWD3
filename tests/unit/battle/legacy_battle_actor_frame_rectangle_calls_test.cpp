#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "test.hpp"

#include <array>
#include <cstddef>
#include <string>

void test_battle_actor_frame_rectangle_calls(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleActorFrameEntryAccessKind;
    using openswd3::battle::LegacyBattleActorFrameEntryRequest;
    using openswd3::battle::LegacyBattleActorFrameEntryResult;
    using Status = openswd3::battle::LegacyBattleActorFrameEntryStatus;
    using openswd3::compat::u32;

    struct Site {
        u32 call_ip;
        Status ready;
        Status child;
        Status after_return;
    };
    // Each CALL is E8 rel32 in the executable LST: its return slot is IP+5.
    // The four arguments below are a controlled callee fixture, not a claim
    // that all 21 parent branches push identical values.
    constexpr std::array<Site, 21U> sites{{
        {0x00479D39U,
         Status::case_three_rectangle_call_ready,
         Status::case_three_rectangle_child_typed_stop,
         Status::case_three_rectangle_return_ready},
        {0x00479E07U,
         Status::case_three_second_rectangle_call_ready,
         Status::case_three_second_rectangle_child_typed_stop,
         Status::case_three_second_rectangle_return_ready},
        {0x00479E98U,
         Status::case_three_four_shared_rectangle_call_ready,
         Status::case_three_four_shared_rectangle_child_typed_stop,
         Status::case_three_four_shared_rectangle_return_ready},
        {0x00479F37U,
         Status::case_four_rectangle_call_ready,
         Status::case_four_rectangle_child_typed_stop,
         Status::case_four_rectangle_return_ready},
        {0x0047A009U,
         Status::case_four_second_rectangle_call_ready,
         Status::case_four_second_rectangle_child_typed_stop,
         Status::case_four_second_rectangle_return_ready},
        {0x0047A186U,
         Status::case_five_clip_call_ready,
         Status::case_five_clip_child_typed_stop,
         Status::case_five_ten_clip_return_ready},
        {0x0047A2F0U,
         Status::case_seven_rectangle_call_ready,
         Status::case_seven_rectangle_child_typed_stop,
         Status::case_seven_rectangle_return_ready},
        {0x0047A3CBU,
         Status::case_seven_second_rectangle_call_ready,
         Status::case_seven_second_rectangle_child_typed_stop,
         Status::case_seven_second_rectangle_return_ready},
        {0x0047A49BU,
         Status::case_seven_third_rectangle_call_ready,
         Status::case_seven_third_rectangle_child_typed_stop,
         Status::case_seven_third_rectangle_return_ready},
        {0x0047A572U,
         Status::case_seven_fourth_rectangle_call_ready,
         Status::case_seven_fourth_rectangle_child_typed_stop,
         Status::case_seven_fourth_rectangle_return_ready},
        {0x0047A91BU,
         Status::case_ten_clip_call_ready,
         Status::case_ten_clip_child_typed_stop,
         Status::case_five_ten_clip_return_ready},
        {0x0047AC2FU,
         Status::case_thirteen_first_rectangle_call_ready,
         Status::case_thirteen_first_rectangle_child_typed_stop,
         Status::case_thirteen_first_rectangle_return_ready},
        {0x0047ACE6U,
         Status::case_thirteen_second_rectangle_call_ready,
         Status::case_thirteen_second_rectangle_child_typed_stop,
         Status::case_thirteen_second_rectangle_return_ready},
        {0x0047ADB8U,
         Status::case_thirteen_third_rectangle_call_ready,
         Status::case_thirteen_third_rectangle_child_typed_stop,
         Status::case_thirteen_third_rectangle_return_ready},
        {0x0047AE7EU,
         Status::case_thirteen_fourth_rectangle_call_ready,
         Status::case_thirteen_fourth_rectangle_child_typed_stop,
         Status::case_thirteen_fourth_rectangle_return_ready},
        {0x0047AF12U,
         Status::case_seven_shared_rectangle_call_ready,
         Status::case_seven_shared_rectangle_child_typed_stop,
         Status::case_seven_shared_rectangle_return_ready},
        {0x0047AFBEU,
         Status::case_fourteen_early_first_rectangle_call_ready,
         Status::case_fourteen_early_first_rectangle_child_typed_stop,
         Status::case_fourteen_early_first_rectangle_return_ready},
        {0x0047B083U,
         Status::case_fourteen_early_second_rectangle_call_ready,
         Status::case_fourteen_early_second_rectangle_child_typed_stop,
         Status::case_fourteen_early_second_rectangle_return_ready},
        {0x0047B174U,
         Status::case_fourteen_late_first_rectangle_call_ready,
         Status::case_fourteen_late_first_rectangle_child_typed_stop,
         Status::case_fourteen_late_first_rectangle_return_ready},
        {0x0047B246U,
         Status::case_fourteen_late_second_rectangle_call_ready,
         Status::case_fourteen_late_second_rectangle_child_typed_stop,
         Status::case_fourteen_late_second_rectangle_return_ready},
        {0x0047B2D6U,
         Status::case_fourteen_shared_rectangle_call_ready,
         Status::case_fourteen_shared_rectangle_child_typed_stop,
         Status::case_fourteen_shared_rectangle_return_ready},
    }};
    constexpr std::array<u32, 15U> ips{
        0x00416FF0U,
        0x00416FF1U,
        0x00416FF7U,
        0x00416FFCU,
        0x00417006U,
        0x0041700AU,
        0x00417015U,
        0x00417019U,
        0x00417029U,
        0x0041702FU,
        0x00417034U,
        0x00417035U,
        0x0041703BU,
        0x00417046U,
        0x00417047U,
    };
    constexpr std::array<LegacyBattleActorFrameEntryAccessKind, 15U> kinds{
        LegacyBattleActorFrameEntryAccessKind::stack_write,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::stack_write,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::global_read,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::global_read,
        LegacyBattleActorFrameEntryAccessKind::global_write,
        LegacyBattleActorFrameEntryAccessKind::global_write,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::global_write,
        LegacyBattleActorFrameEntryAccessKind::global_write,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
        LegacyBattleActorFrameEntryAccessKind::stack_read,
    };
    for (std::size_t site_index = 0U; site_index < sites.size(); ++site_index) {
        const auto& site = sites[site_index];
        const std::string name =
            "rectangle CALL #" + std::to_string(site_index);
        LegacyBattleActorFrameEntryRequest request{};
        LegacyBattleActorFrameEntryResult parent{};
        parent.status = site.ready;
        parent.eip = site.call_ip;
        parent.esp = 0x0012FED0U;
        parent.esi = 0x12345678U;
        parent.edi = 0x87654321U;
        parent.rectangle_argument_pushes = {480U, 640U, 24U, 43U};
        parent.rectangle_argument_count = 4U;
        parent.direction_flag = (site_index & 1U) != 0U;
        parent.flags_known = true;
        parent.flags = {.carry = true, .sign = true};

        auto call_fault_request = request;
        call_fault_request.stop_before_access = parent.accesses_completed;
        const auto at_call_fault = site_index == 5U || site_index == 10U
            ? openswd3::battle::
                  continue_legacy_battle_actor_frame_case_five_clip_entry(
                      call_fault_request, parent
                  )
            : openswd3::battle::
                  continue_legacy_battle_actor_frame_case_three_rectangle_entry(
                      call_fault_request, parent
                  );
        test.expect_true(
            at_call_fault.eip == site.call_ip &&
                at_call_fault.esp == parent.esp &&
                at_call_fault.stopped_access_kind ==
                    LegacyBattleActorFrameEntryAccessKind::stack_write &&
                at_call_fault.stopped_token == parent.esp - 4U &&
                at_call_fault.accesses_completed == parent.accesses_completed,
            name + " fault before CALL return-slot write"
        );
        const auto child = site_index == 5U || site_index == 10U
            ? openswd3::battle::
                  continue_legacy_battle_actor_frame_case_five_clip_entry(
                      request, parent
                  )
            : openswd3::battle::
                  continue_legacy_battle_actor_frame_case_three_rectangle_entry(
                      request, parent
                  );
        test.expect_true(
            child.status == site.child && child.eip == 0x00416FF0U &&
                child.esp == parent.esp - 4U &&
                child.last_pushed_value == site.call_ip + 5U &&
                child.accesses_completed == parent.accesses_completed + 1U &&
                child.direction_flag == parent.direction_flag &&
                !child.returned,
            name + " enters sub_416FF0 after five-byte CALL"
        );
        const u32 c = child.esp;
        const std::array<u32, 15U> tokens{
            c - 4U,
            c + 4U,
            c - 8U,
            c + 8U,
            c + 12U,
            0x004A0E78U,
            c + 16U,
            0x004A0E7CU,
            0x004CD2F8U,
            0x004CD720U,
            c - 8U,
            0x004CD734U,
            0x004CD310U,
            c - 4U,
            c,
        };
        for (std::size_t ordinal = 0U; ordinal < ips.size(); ++ordinal) {
            auto fault_request = request;
            fault_request.stop_before_access =
                child.accesses_completed + ordinal;
            openswd3::rendering::LegacyRasterGeometryState raster{};
            raster.surface.width = 640;
            raster.surface.height = 480;
            raster.clip_left = 111;
            raster.clip_height = 222;
            raster.clip_top = 333;
            raster.clip_width = 444;
            const auto stopped = openswd3::battle::
                continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                    raster, fault_request, child
                );
            const u32 expected_esp = ordinal == 0U || ordinal == 14U ? c
                : ordinal <= 2U || ordinal >= 11U                    ? c - 4U
                                                                     : c - 8U;
            test.expect_true(
                stopped.eip == ips[ordinal] &&
                    stopped.stopped_token == tokens[ordinal] &&
                    stopped.stopped_access_kind == kinds[ordinal] &&
                    stopped.esp == expected_esp &&
                    stopped.accesses_completed ==
                        fault_request.stop_before_access &&
                    stopped.direction_flag == parent.direction_flag &&
                    (ordinal >= 2U ||
                     (stopped.flags_known && stopped.flags.carry &&
                      stopped.flags.sign)) &&
                    (ordinal < 8U ||
                     (stopped.eax == (ordinal >= 13U ? 1U : 456U) &&
                      stopped.ecx == 597U && stopped.edx == 24U &&
                      stopped.esi == (ordinal == 14U ? parent.esi : 43U) &&
                      stopped.edi == (ordinal <= 10U ? 480U : parent.edi) &&
                      stopped.flags_known && !stopped.flags.carry &&
                      !stopped.flags.zero && !stopped.flags.sign &&
                      !stopped.flags.overflow && stopped.flags.parity &&
                      stopped.flags.auxiliary_carry)) &&
                    raster.clip_left == (ordinal < 9U ? 111 : 43) &&
                    raster.clip_height == (ordinal < 10U ? 222 : 456) &&
                    raster.clip_top == (ordinal < 12U ? 333 : 24) &&
                    raster.clip_width == (ordinal < 13U ? 444 : 597),
                name + " child fault #" + std::to_string(ordinal)
            );
        }
        openswd3::rendering::LegacyRasterGeometryState raster{};
        raster.surface.width = 640;
        raster.surface.height = 480;
        const auto returned = openswd3::battle::
            continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
                raster, request, child
            );
        test.expect_true(
            returned.status == site.after_return &&
                returned.eip == site.call_ip + 5U &&
                returned.esp == parent.esp && returned.eax == 1U &&
                returned.ecx == 597U && returned.edx == 24U &&
                returned.esi == parent.esi && returned.edi == parent.edi &&
                returned.flags_known && !returned.flags.carry &&
                !returned.flags.zero && !returned.flags.sign &&
                !returned.flags.overflow && returned.flags.parity &&
                returned.flags.auxiliary_carry &&
                returned.direction_flag == parent.direction_flag &&
                raster.clip_left == 43 && raster.clip_top == 24 &&
                raster.clip_width == 597 && raster.clip_height == 456,
            name + " child returns with four ordered clip writes"
        );
    }
}
