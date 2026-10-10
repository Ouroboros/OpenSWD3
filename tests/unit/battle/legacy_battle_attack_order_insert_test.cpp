#include "openswd3/battle/legacy_battle_attack_order_insert.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace {

using openswd3::battle::LegacyBattleStartupResetRecord;
using openswd3::compat::u32;

[[nodiscard]] bool same_record(
    const LegacyBattleStartupResetRecord& actual,
    const LegacyBattleStartupResetRecord& expected
) {
    return std::ranges::equal(
        std::as_bytes(std::span{&actual, 1U}),
        std::as_bytes(std::span{&expected, 1U})
    );
}

[[nodiscard]] LegacyBattleStartupResetRecord marked_record(const u32 value) {
    return {
        .value_00 = value,
        .value_04 = 0x10000000U | value,
        .value_08 = 0xABCDU,
        .value_0a = 0x9876U,
        .value_0c = 0x20000000U | value,
        .value_10 = 0x30000000U | value,
        .value_14 = 0x40000000U | value,
        .value_18 = 0x50000000U | value,
    };
}

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 18> records{};
    std::array<openswd3::battle::LegacyBattleIntensityEffectRecord, 8>
        adjacent{};
    std::array<u32, 50> sources{};
    u32 primary_gate{9U};
    u32 secondary_gate{8U};

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderInsertBindings
    bindings() {
        return {
            .records = records,
            .adjacent_intensity_records = adjacent,
            .party_source_words = sources,
            .primary_gate = &primary_gate,
            .secondary_gate = &secondary_gate,
        };
    }
};

}  // namespace

void test_battle_attack_order_insert(openswd3::test::Context& test) {
    using openswd3::battle::insert_legacy_battle_attack_order_entry;
    using openswd3::battle::LegacyBattleAttackOrderInsertStatus;

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.sources.size(); ++index) {
            fixture.sources[index] = 0x11110000U + static_cast<u32>(index);
        }

        fixture.records[0U] = marked_record(0xFFFFFFFFU);
        auto expected = fixture.records[0U];
        expected.value_00 = 9U;
        expected.value_04 = fixture.sources[9U];
        expected.value_08 = 1U;
        expected.value_0a = 8U;
        expected.value_0c = fixture.sources[5U];
        expected.value_14 = fixture.sources[6U];
        expected.value_18 = fixture.sources[7U];
        auto expected_sources = fixture.sources;
        std::fill_n(expected_sources.begin() + 5U, 5U, 0U);

        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, 9U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 0U && result.shifted_records == 0U &&
                result.source_words_cleared == 5U &&
                same_record(fixture.records[0U], expected) &&
                fixture.sources == expected_sources &&
                fixture.primary_gate == 0U && fixture.secondary_gate == 0U,
            "party transfer consumes exactly five source DWORDs, retains record +10 and clears the actual gates"
        );
    }

    {
        Fixture fixture;
        fixture.records[0U] = marked_record(10U);
        fixture.records[1U] = marked_record(11U);
        fixture.records[2U] = marked_record(0xFFFFFFFFU);
        const auto before = fixture.records;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, 8U, 1U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 28U && result.shifted_records == 2U &&
                same_record(fixture.records[0U], before[0U]) &&
                same_record(fixture.records[2U], before[1U]) &&
                same_record(fixture.records[3U], before[2U]) &&
                fixture.records[1U].value_00 == 8U &&
                fixture.records[1U].value_10 == before[1U].value_10,
            "positioned insertion moves all seven DWORDs including the marked empty record"
        );
    }

    {
        Fixture fixture;
        fixture.records[0U] = marked_record(10U);
        const auto before = fixture.records;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, 8U, 3U
        );

        test.expect_true(
            result.written_offset == 0U && result.shifted_records == 0U &&
                fixture.records[0U].value_00 == 8U &&
                same_record(fixture.records[1U], before[1U]) &&
                same_record(fixture.records[3U], before[3U]),
            "party position beyond the first empty record retains the original fallback to zero"
        );
    }

    for (const u32 type : {0U, 3U, 0x00010001U, 0xFFFFFFFFU}) {
        Fixture fixture;
        fixture.records[2U] = marked_record(0x1234U);
        auto expected = fixture.records[2U];
        expected.value_00 = 0x80000001U;
        expected.value_08 = static_cast<openswd3::compat::u16>(type);
        fixture.sources.fill(0xA5A5A5A5U);
        const auto before_sources = fixture.sources;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), type, 0x80000001U, 2U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 56U && result.shifted_records == 0U &&
                same_record(fixture.records[2U], expected) &&
                fixture.sources == before_sources &&
                fixture.primary_gate == 9U && fixture.secondary_gate == 8U,
            "only full type one transfers party data; general types write a WORD and retain every other byte"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index] = marked_record(static_cast<u32>(index));
        }

        const auto before = fixture.records;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, 8U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 0U && result.shifted_records == 0U &&
                fixture.records[0U].value_00 == 8U &&
                same_record(fixture.records[1U], before[1U]) &&
                same_record(fixture.records[17U], before[17U]),
            "a full queue still loses its count and party sentinel insertion overwrites zero"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < fixture.records.size(); ++index) {
            fixture.records[index] = marked_record(static_cast<u32>(index));
        }

        const auto before = fixture.records;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 2U, 55U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.shifted_records == 1U &&
                fixture.records[0U].value_00 == 55U &&
                same_record(fixture.records[1U], before[0U]) &&
                same_record(fixture.records[17U], before[17U]),
            "the full-queue zero-count bug shifts only the former first record on general insertion"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < 17U; ++index) {
            fixture.records[index] = marked_record(static_cast<u32>(index));
        }

        fixture.records[17U] = marked_record(0xFFFFFFFFU);
        fixture.adjacent[0U].unknown_1c.fill(0xA5U);
        fixture.adjacent[0U].lookup_key_a = 0x7654U;
        const auto before = fixture.records;
        auto expected_adjacent = fixture.adjacent;
        expected_adjacent[0U].source_value = before[17U].value_00;
        expected_adjacent[0U].value_04 = before[17U].value_04;
        expected_adjacent[0U].secondary_value = 0x9876ABCDU;
        expected_adjacent[0U].value_0c = before[17U].value_0c;
        expected_adjacent[0U].x_offset = before[17U].value_10;
        expected_adjacent[0U].y_offset = before[17U].value_14;
        expected_adjacent[0U].render_flags = before[17U].value_18;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 2U, 55U, 0U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 0U && result.shifted_records == 18U &&
                fixture.records[0U].value_00 == 55U &&
                same_record(fixture.records[1U], before[0U]) &&
                same_record(fixture.records[17U], before[16U]) &&
                std::ranges::equal(
                    std::as_bytes(std::span{fixture.adjacent}),
                    std::as_bytes(std::span{expected_adjacent})
                ),
            "a last empty record writes the real adjacent intensity prefix and preserves its remaining 124 bytes"
        );
    }

    {
        Fixture fixture;
        for (std::size_t index = 0U; index < 17U; ++index) {
            fixture.records[index] = marked_record(static_cast<u32>(index));
        }

        const auto before = fixture.records;
        auto bindings = fixture.bindings();
        bindings.adjacent_intensity_records = {};
        const auto result =
            insert_legacy_battle_attack_order_entry(bindings, 2U, 55U, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        record_shift_destination_typed_stop &&
                !result.written_offset.has_value() &&
                result.shifted_records == 0U &&
                std::ranges::equal(
                    std::as_bytes(std::span{fixture.records}),
                    std::as_bytes(std::span{before})
                ),
            "an unavailable adjacent view retains the existing stop at the first actual destination write"
        );
    }

    {
        Fixture fixture;
        fixture.records[0U] = marked_record(10U);
        fixture.records[1U] = marked_record(0xFFFFFFFFU);
        const auto before = fixture.records;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 2U, 55U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        record_shift_source_typed_stop &&
                result.shifted_records == 2U &&
                !result.written_offset.has_value() &&
                same_record(fixture.records[0U], before[0U]) &&
                same_record(fixture.records[1U], before[0U]) &&
                same_record(fixture.records[2U], before[1U]),
            "negative general position retains completed shifts before the first unavailable source read"
        );
    }

    {
        Fixture fixture;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 2U, 55U, 0x40000000U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 0U && result.shifted_records == 0U &&
                fixture.records[0U].value_00 == 55U &&
                fixture.records[0U].value_08 == 2U,
            "the position byte offset wraps to the actual first record instead of rejecting the logical index"
        );
    }

    {
        Fixture fixture;
        fixture.records[0U] = marked_record(0xFFFFFFFFU);
        auto expected = fixture.records[0U];
        expected.value_0c = 55U;
        expected.value_14 = 0xFFFF1234U;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 0xABCD1234U, 55U, 0x24924925U
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                result.written_offset == 12U &&
                same_record(fixture.records[0U], expected),
            "wrapped position may write inside a record while retaining the original DWORD then WORD stores"
        );
    }

    {
        Fixture fixture;
        const auto before = fixture.records;
        auto bindings = fixture.bindings();
        bindings.adjacent_intensity_records = {};
        const auto result = insert_legacy_battle_attack_order_entry(
            bindings, 2U, 55U, 0x0924925BU
        );

        auto expected = before;
        expected[17U].value_18 = 55U;
        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        record_store_typed_stop &&
                !result.written_offset.has_value() &&
                std::ranges::equal(
                    std::as_bytes(std::span{fixture.records}),
                    std::as_bytes(std::span{expected})
                ),
            "an unavailable type WORD retains the preceding wrapped DWORD store in the last record"
        );
    }

    for (const u32 value : {7U, 0x7333333BU}) {
        Fixture fixture;
        fixture.records[0U] = marked_record(0xFFFFFFFFU);
        auto expected = fixture.records[0U];
        expected.value_00 = value;
        expected.value_08 = 1U;
        fixture.sources.fill(0xA5A5A5A5U);
        const auto before_sources = fixture.sources;
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, value, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        party_source_typed_stop &&
                result.written_offset == 0U &&
                same_record(fixture.records[0U], expected) &&
                fixture.sources == before_sources &&
                fixture.primary_gate == 9U && fixture.secondary_gate == 8U,
            "underflow or a wrapped second source read preserves only the two queue header stores"
        );
    }

    for (std::size_t available = 0U; available < 5U; ++available) {
        Fixture fixture;
        fixture.records[0U] = marked_record(0xFFFFFFFFU);
        fixture.sources = {11U, 22U, 33U, 44U, 55U};
        const auto before_sources = fixture.sources;
        auto expected = fixture.records[0U];
        expected.value_00 = 8U;
        expected.value_08 = 1U;
        if (available >= 2U) {
            expected.value_14 = 22U;
            expected.value_0c = 11U;
        }

        if (available >= 3U) {
            expected.value_18 = 33U;
        }

        auto bindings = fixture.bindings();
        bindings.party_source_words =
            std::span{fixture.sources}.first(available);
        const auto result = insert_legacy_battle_attack_order_entry(
            bindings, 1U, 8U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        party_source_typed_stop &&
                result.written_offset == 0U &&
                result.source_words_cleared == 0U &&
                same_record(fixture.records[0U], expected) &&
                fixture.sources == before_sources &&
                fixture.primary_gate == 9U && fixture.secondary_gate == 8U,
            "each missing source read preserves its exact record prefix and suppresses source and gate clearing"
        );
    }

    {
        Fixture fixture;
        fixture.sources = {11U, 22U, 33U, 44U, 55U};
        const auto result = insert_legacy_battle_attack_order_entry(
            fixture.bindings(), 1U, 0x80000008U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderInsertStatus::completed &&
                fixture.records[0U].value_00 == 0x80000008U &&
                fixture.records[0U].value_0c == 11U &&
                fixture.records[0U].value_04 == 55U &&
                result.source_words_cleared == 5U,
            "full-width party value is retained while its source offset wraps to the real first group"
        );
    }

    for (const bool missing_primary : {true, false}) {
        Fixture fixture;
        fixture.sources = {11U, 22U, 33U, 44U, 55U};
        auto bindings = fixture.bindings();
        if (missing_primary) {
            bindings.primary_gate = nullptr;
        } else {
            bindings.secondary_gate = nullptr;
        }

        const auto result = insert_legacy_battle_attack_order_entry(
            bindings, 1U, 8U, 0xFFFFFFFFU
        );

        test.expect_true(
            result.status ==
                    (missing_primary ? LegacyBattleAttackOrderInsertStatus::
                                           primary_gate_typed_stop
                                     : LegacyBattleAttackOrderInsertStatus::
                                           secondary_gate_typed_stop) &&
                result.written_offset == 0U &&
                result.source_words_cleared == 5U &&
                fixture.records[0U].value_04 == 55U &&
                fixture.sources[0U] == 0U && fixture.sources[4U] == 0U &&
                fixture.primary_gate == (missing_primary ? 9U : 0U) &&
                fixture.secondary_gate == 8U,
            "each unavailable gate retains record and source clearing plus only the preceding gate write"
        );
    }

    for (const bool empty_view : {true, false}) {
        Fixture fixture;
        fixture.records[0U].value_00 = 10U;
        fixture.records[1U].value_00 = 11U;
        const auto before = fixture.records;
        auto bindings = fixture.bindings();
        bindings.records =
            std::span{fixture.records}.first(empty_view ? 0U : 2U);
        const auto result =
            insert_legacy_battle_attack_order_entry(bindings, 3U, 55U, 0U);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderInsertStatus::
                        record_scan_typed_stop &&
                !result.written_offset.has_value() &&
                std::ranges::equal(
                    std::as_bytes(std::span{fixture.records}),
                    std::as_bytes(std::span{before})
                ),
            "short or empty queue stops at the unavailable scan read before any mutation"
        );
    }
}
