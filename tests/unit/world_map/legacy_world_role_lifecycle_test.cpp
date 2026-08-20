#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/world_map/legacy_world_role_lifecycle.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace {

using openswd3::compat::i32;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;
using openswd3::world_map::clear_legacy_world_role_table;
using openswd3::world_map::kLegacyWorldActiveObjectSlotCount;
using openswd3::world_map::kLegacyWorldRoleCapacity;
using openswd3::world_map::LegacyWorldMapRolePathPorts;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldRoleSurfaceContext;
using openswd3::world_map::LegacyWorldRoleTableResetStatus;
using openswd3::world_map::LegacyWorldRoleTransitionStatus;
using openswd3::world_map::release_legacy_world_role_transition;
using openswd3::world_map::reset_legacy_world_role_table;

enum class TransitionCall {
    complete_path,
    update_action,
};

class RecordingPathPorts final : public LegacyWorldMapRolePathPorts {
public:
    explicit RecordingPathPorts(std::vector<TransitionCall>& calls_in) noexcept
        : calls(calls_in) {}

    [[nodiscard]] bool
    complete_role_path(const u32 role_index) noexcept override {
        calls.push_back(TransitionCall::complete_path);
        completed_role_indices.push_back(role_index);
        return succeeds;
    }

    std::vector<TransitionCall>& calls;
    std::vector<u32> completed_role_indices;
    bool succeeds{true};
};

class RecordingActionPorts final
    : public openswd3::asset_runtime::LegacyActionDrawPorts {
public:
    using LegacyActionRecord = openswd3::asset_runtime::LegacyActionRecord;
    using LegacyActionUpdateStatus =
        openswd3::asset_runtime::LegacyActionUpdateStatus;

    explicit RecordingActionPorts(
        std::vector<TransitionCall>& calls_in
    ) noexcept
        : calls(calls_in) {}

    [[nodiscard]] LegacyActionUpdateStatus
    update_action_record(LegacyActionRecord& record) override {
        calls.push_back(TransitionCall::update_action);
        updated_records.push_back(record);
        return status;
    }

    [[nodiscard]] bool load_frame_piece(
        u16, u16, openswd3::rendering::LegacyFramePiece&
    ) override {
        return false;
    }

    [[nodiscard]] openswd3::rendering::LegacyBlitExecutionStatus
    draw_frame_piece(
        const openswd3::rendering::LegacyFramePiece&, i32, i32, u32, i32
    ) noexcept override {
        return openswd3::rendering::LegacyBlitExecutionStatus::completed;
    }

    std::vector<TransitionCall>& calls;
    std::vector<LegacyActionRecord> updated_records;
    LegacyActionUpdateStatus status{LegacyActionUpdateStatus::completed};
};

void write_u16(
    LegacyWorldObjectSlot& slot, const std::size_t offset, const u16 value
) noexcept {
    slot.bytes[offset] = static_cast<u8>(value);
    slot.bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_cell(
    const std::span<u8> surface, const std::size_t index, const u32 value
) noexcept {
    const std::size_t offset = index * sizeof(u32);
    surface[offset] = static_cast<u8>(value);
    surface[offset + 1U] = static_cast<u8>(value >> 8U);
    surface[offset + 2U] = static_cast<u8>(value >> 16U);
    surface[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u32
read_cell(const std::span<const u8> surface, const std::size_t index) noexcept {
    const std::size_t offset = index * sizeof(u32);
    return static_cast<u32>(surface[offset]) |
        (static_cast<u32>(surface[offset + 1U]) << 8U) |
        (static_cast<u32>(surface[offset + 2U]) << 16U) |
        (static_cast<u32>(surface[offset + 3U]) << 24U);
}

void fill_role_bytes(
    const std::span<LegacyWorldRoleRecord> roles, const std::byte value
) {
    std::ranges::fill(std::as_writable_bytes(roles), value);
}

[[nodiscard]] bool
is_initialized_empty_role(const LegacyWorldRoleRecord& role) {
    LegacyWorldRoleRecord expected{};
    openswd3::asset_runtime::initialize_legacy_action_record(expected.action);
    return std::ranges::equal(
        std::as_bytes(std::span{&role, 1U}),
        std::as_bytes(std::span{&expected, 1U})
    );
}

void test_full_physical_table_clear_without_action_reinit(
    openswd3::test::Context& test
) {
    std::array<LegacyWorldRoleRecord, kLegacyWorldRoleCapacity> roles{};
    fill_role_bytes(roles, std::byte{0xA5U});
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    for (std::size_t index = 0U; index < payloads.size(); ++index) {
        payloads[index] = {static_cast<u8>(index), 0U};
        roles[index].path_payload_pointer_32 =
            index % 2U == 0U ? static_cast<u32>(index + 1U) : 0U;
    }

    const auto result = clear_legacy_world_role_table(roles, payloads);

    const LegacyWorldRoleRecord zero_role{};
    test.expect_true(
        result.status == LegacyWorldRoleTableResetStatus::ready &&
            result.payload_slots_scanned == kLegacyWorldRoleCapacity &&
            result.payload_owners_released == kLegacyWorldRoleCapacity / 2U &&
            result.roles_zeroed == kLegacyWorldRoleCapacity,
        "sub_425B50 scans and zeros all 256 physical role records"
    );
    test.expect_true(
        std::ranges::all_of(
            roles,
            [&zero_role](const LegacyWorldRoleRecord& role) {
                return std::ranges::equal(
                    std::as_bytes(std::span{&role, 1U}),
                    std::as_bytes(std::span{&zero_role, 1U})
                );
            }
        ) && payloads[0U].empty() &&
            payloads[0U].capacity() == 0U && !payloads[1U].empty() &&
            payloads[254U].empty() && !payloads[255U].empty(),
        "map clear releases only non-null +38 owners and leaves actions zero"
    );
}

void test_full_physical_table_reset(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, kLegacyWorldRoleCapacity> roles{};
    fill_role_bytes(roles, std::byte{0xA5U});
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    for (std::size_t index = 0U; index < payloads.size(); ++index) {
        payloads[index] = {static_cast<u8>(index), 0U};
    }
    roles[0U].path_payload_pointer_32 = 1U;
    roles[1U].path_payload_pointer_32 = 0U;
    roles[2U].path_payload_pointer_32 = 3U;

    const auto result = reset_legacy_world_role_table(roles, payloads, 2);
    test.expect_true(
        result.status == LegacyWorldRoleTableResetStatus::ready &&
            result.payload_slots_scanned == 3U &&
            result.payload_owners_released == 2U &&
            result.roles_zeroed == kLegacyWorldRoleCapacity &&
            result.action_records_initialized == kLegacyWorldRoleCapacity,
        "sub_40F3B0 scans the inclusive live range before both table passes"
    );
    test.expect_true(
        payloads[0U].empty() && payloads[0U].capacity() == 0U &&
            !payloads[1U].empty() && payloads[2U].empty() &&
            payloads[2U].capacity() == 0U && !payloads[3U].empty(),
        "only non-null +38 owners inside the inclusive range are freed"
    );
    test.expect_true(
        std::ranges::all_of(roles, is_initialized_empty_role),
        "all 256 role records are zeroed before sub_40DC00 initialization"
    );
}

void test_negative_highest_skips_only_release(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    fill_role_bytes(roles, std::byte{0x5AU});
    roles[0U].path_payload_pointer_32 = 1U;
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    payloads[0U] = {1U, 2U, 3U};

    const auto result =
        reset_legacy_world_role_table(roles, payloads, i32{-17});
    test.expect_true(
        result.status == LegacyWorldRoleTableResetStatus::ready &&
            result.payload_slots_scanned == 0U &&
            result.payload_owners_released == 0U && result.roles_zeroed == 2U &&
            result.action_records_initialized == 2U && !payloads[0U].empty() &&
            std::ranges::all_of(roles, is_initialized_empty_role),
        "every negative highest index skips release but still resets roles"
    );
}

void test_modern_bounds_are_transactional(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[0U].guid = 0x1234U;
    std::array<std::vector<u8>, kLegacyWorldRoleCapacity> payloads;
    payloads[0U] = {1U};

    const auto missing_role = reset_legacy_world_role_table(roles, payloads, 2);
    test.expect_true(
        missing_role.status ==
                LegacyWorldRoleTableResetStatus::
                    active_role_range_out_of_bounds &&
            roles[0U].guid == 0x1234U && !payloads[0U].empty(),
        "a truncated modern owner is rejected before release or reset"
    );

    const auto excessive_highest = reset_legacy_world_role_table(
        roles, payloads, static_cast<i32>(kLegacyWorldRoleCapacity)
    );
    test.expect_true(
        excessive_highest.status ==
                LegacyWorldRoleTableResetStatus::
                    highest_role_index_out_of_range &&
            roles[0U].guid == 0x1234U && !payloads[0U].empty(),
        "a highest index beyond the physical 256-role table is isolated"
    );

    std::array<LegacyWorldRoleRecord, kLegacyWorldRoleCapacity + 1U>
        oversized{};
    oversized[0U].guid = 0x5678U;
    const auto excessive_clear =
        clear_legacy_world_role_table(oversized, payloads);
    test.expect_true(
        excessive_clear.status ==
                LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity &&
            oversized[0U].guid == 0x5678U,
        "map clear rejects an oversized modern owner before mutation"
    );

    const auto excessive_span =
        reset_legacy_world_role_table(oversized, payloads, -1);
    test.expect_equal(
        excessive_span.status,
        LegacyWorldRoleTableResetStatus::role_span_exceeds_capacity,
        "a modern role span cannot exceed the physical table"
    );
}

void test_transition_bit31_early_return(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1U].world_x = 0x24U;
    roles[1U].world_y = 0x2CU;
    roles[1U].interaction_gate = 7U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    std::array<u8, sizeof(u32)> surface{};
    std::vector<TransitionCall> calls;
    RecordingPathPorts path_ports{calls};
    RecordingActionPorts action_ports{calls};

    const auto result = release_legacy_world_role_transition(
        roles,
        slots,
        1U,
        0U,
        LegacyWorldRoleSurfaceContext{
            .map_width = 1U,
            .selected_guid = 0U,
            .surface_grid = surface,
        },
        path_ports,
        action_ports
    );
    test.expect_true(
        result.status == LegacyWorldRoleTransitionStatus::ready &&
            !result.ownership_flag_set && result.slots_scanned == 0U &&
            result.path_completion_calls == 0U &&
            result.action_update_calls == 0U && calls.empty(),
        "bit 31 clear returns from sub_40F6D0 before every helper"
    );
    test.expect_true(
        roles[1U].world_x == 0x24U && roles[1U].world_y == 0x2CU &&
            roles[1U].interaction_gate == 0U,
        "the sole caller still clears role+0x26 after the early return"
    );
}

void test_transition_full_matching_slot_path(openswd3::test::Context& test) {
    std::array<LegacyWorldRoleRecord, 3U> roles{};
    LegacyWorldRoleRecord& role = roles[1U];
    role.world_x = 0x24U;
    role.world_y = 0x2CU;
    role.map_cell_pointer_32 = 0U;
    role.flags = 0xC4000800U;
    role.guid = 9U;
    role.interaction_gate = 3U;
    role.action.base_variant = 1U;
    role.action.variant_delta = 2U;
    role.action.one_shot_base_variant = 7U;
    role.action.one_shot_variant_delta = 11U;
    role.action.wait_remaining = 13U;
    role.action.wait_override = 0x8005U;
    role.action.field_2c = 1U;
    role.action.field_30 = 1U;

    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    write_u16(slots[0U], 0x00U, 1U);
    write_u16(slots[0U], 0x02U, 0U);
    slots[0U].bytes[0x1CU] = 2U;
    write_u16(slots[1U], 0x00U, 1U);
    write_u16(slots[1U], 0x02U, 0U);
    slots[1U].bytes[0x1CU] = 2U;

    std::array<u8, sizeof(u32)> surface{};
    write_cell(surface, 0U, 0xFFFFFFFFU);
    std::vector<TransitionCall> calls;
    RecordingPathPorts path_ports{calls};
    RecordingActionPorts action_ports{calls};

    const auto result = release_legacy_world_role_transition(
        roles,
        slots,
        1U,
        2U,
        LegacyWorldRoleSurfaceContext{
            .map_width = 1U,
            .selected_guid = 3U,
            .surface_grid = surface,
        },
        path_ports,
        action_ports
    );
    test.expect_true(
        result.status == LegacyWorldRoleTransitionStatus::ready &&
            result.ownership_flag_set &&
            result.slots_scanned == kLegacyWorldActiveObjectSlotCount &&
            result.matching_slots == 2U && result.surface_clear_calls == 1U &&
            result.coordinate_alignment_slots == 1U &&
            result.path_completion_calls == 1U &&
            result.action_update_calls == 1U,
        "sub_40F6D0 scans 72 slots and only the first unaligned match moves"
    );
    test.expect_true(
        role.world_x == 0x30U && role.world_y == 0x20U &&
            read_cell(surface, 0U) == 0xCF7FFFFFU,
        "direction two subtracts minus four on x and plus four on y"
    );
    test.expect_equal(
        calls,
        std::vector<TransitionCall>{
            TransitionCall::complete_path,
            TransitionCall::update_action,
        },
        "path completion precedes action refresh"
    );
    test.expect_true(
        path_ports.completed_role_indices == std::vector<u32>{1U} &&
            action_ports.updated_records.size() == 1U &&
            action_ports.updated_records[0U].base_variant == 7U &&
            action_ports.updated_records[0U].variant_delta == 11U &&
            action_ports.updated_records[0U].one_shot_base_variant ==
                0xFFFFFFFFU &&
            action_ports.updated_records[0U].one_shot_variant_delta ==
                0xFFFFFFFFU &&
            action_ports.updated_records[0U].wait_override == 0U,
        "bit 11 restores pending action fields before sub_4321E0"
    );
    test.expect_true(
        role.flags == 0x00000800U && role.action.wait_remaining == 0U &&
            role.action.one_shot_base_variant == 0xFFFFFFFFU &&
            role.action.one_shot_variant_delta == 0xFFFFFFFFU &&
            role.interaction_gate == 0U,
        "the final writes clear bit 31, waits, pending overrides and gate"
    );
}

void test_transition_selected_role_skips_direction_alignment(
    openswd3::test::Context& test
) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    LegacyWorldRoleRecord& role = roles[1U];
    role.world_x = 0x24U;
    role.world_y = 0x2CU;
    role.flags = 0xC4000000U;
    role.guid = 9U;
    role.action.one_shot_base_variant = 0xFFFFFFFFU;
    role.action.one_shot_variant_delta = 0xFFFFFFFFU;

    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    write_u16(slots[0U], 0x00U, 1U);
    write_u16(slots[0U], 0x02U, 0x7FFFU);
    write_u16(slots[1U], 0x00U, 1U);
    write_u16(slots[1U], 0x02U, 0x7FFFU);

    std::array<u8, sizeof(u32)> surface{};
    write_cell(surface, 0U, 0xFFFFFFFFU);
    std::vector<TransitionCall> calls;
    RecordingPathPorts path_ports{calls};
    RecordingActionPorts action_ports{calls};

    const auto result = release_legacy_world_role_transition(
        roles,
        slots,
        1U,
        1U,
        LegacyWorldRoleSurfaceContext{
            .map_width = 1U,
            .selected_guid = 3U,
            .surface_grid = surface,
        },
        path_ports,
        action_ports
    );
    test.expect_true(
        result.status == LegacyWorldRoleTransitionStatus::ready &&
            result.matching_slots == 2U && result.surface_clear_calls == 2U &&
            result.coordinate_alignment_slots == 0U,
        "the selected role clears each unaligned match without reading its direction"
    );
    test.expect_true(
        role.world_x == 0x24U && role.world_y == 0x2CU && role.flags == 0U &&
            role.action.wait_override == 0U &&
            read_cell(surface, 0U) == 0xCF7FFFFFU,
        "the selected-role branch preserves coordinates and retains common slot writes"
    );
}

void test_transition_failures_do_not_skip_final_writes(
    openswd3::test::Context& test
) {
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    LegacyWorldRoleRecord& role = roles[1U];
    role.flags = 0x80000000U;
    role.interaction_gate = 1U;
    role.action.wait_remaining = 9U;
    role.action.one_shot_base_variant = 4U;
    role.action.one_shot_variant_delta = 5U;
    std::array<LegacyWorldObjectSlot, kLegacyWorldActiveObjectSlotCount> slots;
    std::array<u8, sizeof(u32)> surface{};
    std::vector<TransitionCall> calls;
    RecordingPathPorts path_ports{calls};
    path_ports.succeeds = false;
    RecordingActionPorts action_ports{calls};
    action_ports.status =
        openswd3::asset_runtime::LegacyActionUpdateStatus::stream_load_failed;

    const auto result = release_legacy_world_role_transition(
        roles,
        slots,
        1U,
        0U,
        LegacyWorldRoleSurfaceContext{
            .map_width = 1U,
            .selected_guid = 0U,
            .surface_grid = surface,
        },
        path_ports,
        action_ports
    );
    test.expect_true(
        result.status ==
                LegacyWorldRoleTransitionStatus::path_completion_failed &&
            result.action_update_status ==
                openswd3::asset_runtime::LegacyActionUpdateStatus::
                    stream_load_failed &&
            calls ==
                std::vector<TransitionCall>{
                    TransitionCall::complete_path,
                    TransitionCall::update_action,
                },
        "ignored helper failures retain the original call sequence"
    );
    test.expect_true(
        role.flags == 0U && role.action.wait_remaining == 0U &&
            role.action.one_shot_base_variant == 0xFFFFFFFFU &&
            role.action.one_shot_variant_delta == 0xFFFFFFFFU &&
            role.interaction_gate == 0U,
        "diagnostic helper failures do not bypass the assembly's final writes"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_full_physical_table_clear_without_action_reinit(test);
    test_full_physical_table_reset(test);
    test_negative_highest_skips_only_release(test);
    test_modern_bounds_are_transactional(test);
    test_transition_bit31_early_return(test);
    test_transition_full_matching_slot_path(test);
    test_transition_selected_role_skips_direction_alignment(test);
    test_transition_failures_do_not_skip_final_writes(test);
    return test.exit_code();
}
