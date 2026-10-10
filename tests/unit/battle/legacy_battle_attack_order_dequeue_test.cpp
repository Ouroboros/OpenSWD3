#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"

#include <array>
#include <memory>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleAttackOrderDequeueBindings;
using openswd3::battle::LegacyBattleAttackOrderDequeueStatus;
using openswd3::battle::LegacyBattleGroupAActionExecutionState;
using openswd3::battle::LegacyBattleIntensityEffectRecord;
using openswd3::battle::LegacyBattlePartyStartupRecord;
using openswd3::battle::LegacyBattleStartupResetRecord;
using openswd3::battle::dequeue_legacy_battle_attack_order_entry;
using openswd3::compat::u8;
using openswd3::compat::u32;

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 18> records{};
    std::array<LegacyBattleIntensityEffectRecord, 8> intensity{};
    std::array<LegacyBattlePartyStartupRecord, 10> party{};
    std::array<LegacyBattleGroupAActionExecutionState, 10> party_actions{};
    u32 output_first{0xAAAAAAAAU};
    std::array<u32, 6> output_tail{
        0xBBBBBBBBU,
        0xCCCCCCCCU,
        0xDDDDDDDDU,
        0xEEEEEEEEU,
        0x11111111U,
        0x22222222U,
    };

    Fixture() {
        for (auto& record : records) {
            record.value_00 = 0xFFFFFFFFU;
        }
    }

    static void seed_record(
        LegacyBattleStartupResetRecord& record, const u32 first, const u32 seed
    ) {
        record.value_00 = first;
        record.value_04 = seed + 1U;
        record.value_08 = static_cast<openswd3::compat::u16>(seed + 2U);
        record.value_0a = static_cast<openswd3::compat::u16>(seed + 3U);
        record.value_0c = seed + 4U;
        record.value_10 = seed + 5U;
        record.value_14 = seed + 6U;
        record.value_18 = seed + 7U;
    }

    [[nodiscard]] LegacyBattleAttackOrderDequeueBindings bindings() {
        return {
            .records = records,
            .adjacent_intensity_records = intensity,
            .output = {.value_00 = &output_first, .tail_dwords = output_tail},
            .party = party,
            .party_actions = party_actions,
        };
    }
};

[[nodiscard]] bool same_record(
    const LegacyBattleStartupResetRecord& left,
    const LegacyBattleStartupResetRecord& right
) {
    return left.value_00 == right.value_00 && left.value_04 == right.value_04 &&
        left.value_08 == right.value_08 && left.value_0a == right.value_0a &&
        left.value_0c == right.value_0c && left.value_10 == right.value_10 &&
        left.value_14 == right.value_14 && left.value_18 == right.value_18;
}

[[nodiscard]] bool same_records(
    const std::array<LegacyBattleStartupResetRecord, 18>& left,
    const std::array<LegacyBattleStartupResetRecord, 18>& right
) {
    for (u32 index = 0U; index < left.size(); ++index) {
        if (!same_record(left[index], right[index])) {
            return false;
        }
    }

    return true;
}

void write_u32(std::span<u8> bytes, const u32 offset, const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

}  // namespace

void test_battle_attack_order_dequeue(openswd3::test::Context& test) {
    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 0xFFFFFFFFU, 0x100U);
        const auto before = fixture->records;
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 0U && result.output_dwords == 7U &&
                fixture->output_first == 0xFFFFFFFFU &&
                fixture->output_tail ==
                    std::array<u32, 6>{
                        0x101U, 0x01030102U, 0x104U, 0x105U, 0x106U, 0x107U
                    } &&
                same_records(fixture->records, before) &&
                result.shifted_records == 0U && result.cleared_records == 0U,
            "an empty record copies all seven DWORDs without changing the queue"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 3U, 0x100U);
        Fixture::seed_record(fixture->records[1], 9U, 0x200U);
        const auto expected = fixture->records[1];
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                fixture->output_first == 3U &&
                same_record(fixture->records[0], expected) &&
                fixture->records[1].value_00 == 0xFFFFFFFFU &&
                fixture->records[1].value_04 == 0U &&
                result.shifted_records == 17U && result.cleared_records == 17U,
            "a group-B record is removed before the all-one zero tail is restored"
        );
    }

    for (const u32 special : {0U, 1U, 2U, 0xFFFFFFFFU}) {
        for (const u32 flags : {0U, 0x40U, 0x4000U}) {
            auto fixture = std::make_unique<Fixture>();
            Fixture::seed_record(fixture->records[0], 8U, 0x100U);
            Fixture::seed_record(fixture->records[1], 9U, 0x200U);
            fixture->party[0].progress.mode_gate = flags;
            fixture->party_actions[0].special_mode = special;
            fixture->party_actions[1].special_mode = 2U;
            const auto result =
                dequeue_legacy_battle_attack_order_entry(fixture->bindings());
            const bool skipped = (flags & 0x40U) != 0U || special == 1U;
            test.expect_true(
                result.status ==
                        LegacyBattleAttackOrderDequeueStatus::completed &&
                    result.selected_index == (skipped ? 1U : 0U) &&
                    fixture->output_first == (skipped ? 9U : 8U) &&
                    fixture->party[0].progress.mode_gate == flags &&
                    fixture->party_actions[0].special_mode == special,
                "group-A selection reads the real byte bit six and exact special value"
            );
        }
    }

    for (const u32 code : {7U, 18U}) {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], code, 0x100U);
        const auto before = fixture->records;
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        actor_query_typed_stop &&
                result.output_dwords == 0U &&
                fixture->output_first == 0xAAAAAAAAU &&
                same_records(fixture->records, before),
            "actor underflow and one-past the physical ten slots stop before output"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 8U;
        fixture->records[1].value_00 = 17U;
        fixture->records[2].value_00 = 3U;
        fixture->party_actions[0].special_mode = 1U;
        fixture->party[9].progress.mode_gate = 0x40U;
        auto bindings = fixture->bindings();
        bindings.party_actions = std::span{fixture->party_actions}.first(1U);
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 2U && fixture->output_first == 3U,
            "bit six skips the last physical actor without reading special state"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 8U;
        fixture->records[1].value_00 = 9U;
        fixture->party_actions[0].special_mode = 1U;
        const auto before = fixture->records;
        auto bindings = fixture->bindings();
        bindings.party_actions = std::span{fixture->party_actions}.first(1U);
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        actor_query_typed_stop &&
                result.output_dwords == 0U &&
                fixture->output_first == 0xAAAAAAAAU &&
                same_records(fixture->records, before),
            "missing special state stops only after the preceding actor was skipped"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 0x40000008U;
        fixture->records[1].value_00 = 3U;
        fixture->party_actions[0].special_mode = 1U;
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 1U && fixture->output_first == 3U,
            "actor multiplication wraps to the real first physical slot"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 0x80000007U, 0x100U);
        auto bindings = fixture->bindings();
        bindings.party = {};
        bindings.party_actions = {};
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                fixture->output_first == 0x80000007U,
            "signed negative queue values do not read an actor"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 3U, 0x12340000U);
        std::array<u32, 2> short_tail{0xAAAAAAAAU, 0xBBBBBBBBU};
        auto bindings = fixture->bindings();
        bindings.output.tail_dwords = short_tail;
        const auto before = fixture->records;
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        output_destination_typed_stop &&
                result.output_dwords == 3U && fixture->output_first == 3U &&
                short_tail[0] == 0x12340001U && short_tail[1] == 0x00030002U &&
                same_records(fixture->records, before),
            "a short output keeps the exact copied prefix and leaves the queue intact"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        fixture->records[0].value_00 = 3U;
        auto bindings = fixture->bindings();
        bindings.output.value_00 = nullptr;
        const auto before = fixture->records;
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        output_destination_typed_stop &&
                result.output_dwords == 0U &&
                same_records(fixture->records, before),
            "an unavailable first output field stops at the first store"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 3U, 0x100U);
        Fixture::seed_record(fixture->records[1], 2U, 0x200U);
        const auto expected = fixture->records[1];
        auto bindings = fixture->bindings();
        bindings.records = std::span{fixture->records}.first(2U);
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        shift_source_typed_stop &&
                result.output_dwords == 7U && result.shifted_records == 1U &&
                result.cleared_records == 0U && fixture->output_first == 3U &&
                same_record(fixture->records[0], expected) &&
                same_record(fixture->records[1], expected),
            "a missing shift source preserves output and the completed left shift"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        Fixture::seed_record(fixture->records[0], 8U, 0x100U);
        Fixture::seed_record(fixture->records[1], 8U, 0x200U);
        fixture->party_actions[0].special_mode = 1U;
        auto bindings = fixture->bindings();
        bindings.records = std::span{fixture->records}.first(2U);
        const auto before = fixture->records;
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        record_scan_typed_stop &&
                result.output_dwords == 0U &&
                fixture->output_first == 0xAAAAAAAAU &&
                same_records(fixture->records, before),
            "a short queue stops on the next physical scan read before copying"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        for (auto& record : fixture->records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }

        fixture->party_actions[0].special_mode = 1U;
        fixture->intensity[0].source_value = 3U;
        fixture->intensity[0].value_04 = 0x11111111U;
        fixture->intensity[0].secondary_value = 0x22222222U;
        fixture->intensity[0].value_0c = 0x33333333U;
        fixture->intensity[0].x_offset = 0x44444444U;
        fixture->intensity[0].y_offset = 0x55555555U;
        fixture->intensity[0].render_flags = 0x66666666U;
        const auto before = fixture->records;
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 18U &&
                result.selected_from_adjacent_intensity &&
                same_records(fixture->records, before) &&
                fixture->output_first == 3U &&
                fixture->output_tail ==
                    std::array<u32, 6>{
                        0x11111111U,
                        0x22222222U,
                        0x33333333U,
                        0x44444444U,
                        0x55555555U,
                        0x66666666U,
                    },
            "a full queue continues into the adjacent intensity prefix"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        for (auto& record : fixture->records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }

        fixture->party_actions[0].special_mode = 1U;
        fixture->intensity[0].source_value = 8U;
        write_u32(fixture->intensity[0].unknown_1c, 0U, 2U);
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        test.expect_true(
            result.selected_index == 19U && fixture->output_first == 2U,
            "the adjacent scan keeps its 28-byte stride within an intensity record"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        for (auto& record : fixture->records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }

        fixture->party_actions[0].special_mode = 1U;
        auto bindings = fixture->bindings();
        bindings.adjacent_intensity_records = {};
        const auto before = fixture->records;
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        record_scan_typed_stop &&
                result.output_dwords == 0U &&
                fixture->output_first == 0xAAAAAAAAU &&
                same_records(fixture->records, before),
            "missing adjacent storage stops after the eighteen skipped records"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        for (auto& record : fixture->records) {
            record.value_00 = 8U;
        }

        fixture->party_actions[0].special_mode = 1U;
        fixture->intensity[0].source_value = 8U;
        write_u32(fixture->intensity[0].unknown_1c, 0U, 8U);
        write_u32(fixture->intensity[0].unknown_1c, 28U, 8U);
        write_u32(fixture->intensity[0].unknown_4e, 6U, 8U);
        write_u32(fixture->intensity[0].unknown_4e, 34U, 8U);
        write_u32(fixture->intensity[0].unknown_4e, 62U, 3U);
        fixture->intensity[0].mode_snapshot = 0x12345678U;
        fixture->intensity[0].value_94 = 0x87654321U;
        auto bindings = fixture->bindings();
        bindings.adjacent_intensity_records =
            std::span{fixture->intensity}.first(1U);
        const auto before = fixture->records;
        const auto result = dequeue_legacy_battle_attack_order_entry(bindings);
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        output_source_typed_stop &&
                result.selected_index == 23U && result.output_dwords == 3U &&
                fixture->output_first == 3U &&
                fixture->output_tail[0] == 0x12345678U &&
                fixture->output_tail[1] == 0x87654321U &&
                fixture->output_tail[2] == 0xDDDDDDDDU &&
                same_records(fixture->records, before),
            "a short adjacent source preserves three copied DWORDs before stopping"
        );
    }

    for (const u32 selected_index : {1U, 17U}) {
        auto fixture = std::make_unique<Fixture>();
        for (u32 index = 0U; index < fixture->records.size(); ++index) {
            Fixture::seed_record(
                fixture->records[index],
                index < selected_index ? 8U : 2U,
                index * 0x100U
            );
        }

        fixture->party_actions[0].special_mode = 1U;
        const auto before = fixture->records;
        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        bool retained_prefix = true;
        bool cleared_tail = true;
        for (u32 index = 0U; index < selected_index; ++index) {
            retained_prefix = retained_prefix &&
                same_record(fixture->records[index], before[index]);
        }

        for (u32 index = selected_index; index < fixture->records.size();
             ++index) {
            cleared_tail = cleared_tail &&
                fixture->records[index].value_00 == 0xFFFFFFFFU &&
                fixture->records[index].value_04 == 0U;
        }

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == selected_index &&
                result.shifted_records == 17U - selected_index &&
                result.cleared_records == 18U - selected_index &&
                retained_prefix && cleared_tail,
            "a full table clears from the selected slot and retains skipped records"
        );
    }

    {
        auto fixture = std::make_unique<Fixture>();
        for (u32 index = 0U; index < fixture->records.size(); ++index) {
            Fixture::seed_record(fixture->records[index], 2U, index * 0x100U);
        }

        const auto result =
            dequeue_legacy_battle_attack_order_entry(fixture->bindings());
        bool all_empty = true;
        for (const auto& record : fixture->records) {
            all_empty = all_empty && record.value_00 == 0xFFFFFFFFU &&
                record.value_04 == 0U && record.value_08 == 0U &&
                record.value_0a == 0U && record.value_0c == 0U &&
                record.value_10 == 0U && record.value_14 == 0U &&
                record.value_18 == 0U;
        }

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                all_empty && result.cleared_records == 18U,
            "a full table preserves clearing from the original selected slot"
        );
    }
}
