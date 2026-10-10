#include "openswd3/battle/legacy_battle_attack_order_remove.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>

namespace {

using openswd3::battle::LegacyBattleIntensityEffectRecord;
using openswd3::battle::LegacyBattleStartupResetRecord;
using openswd3::compat::u16;
using openswd3::compat::u32;

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 19> records{};
    std::array<LegacyBattleIntensityEffectRecord, 8> adjacent{};

    Fixture() {
        for (u32 index = 0U; index < records.size(); ++index) {
            records[index] = {
                .value_00 = 100U + index,
                .value_04 = 0x11220000U + index,
                .value_08 = static_cast<u16>(0x3300U + index),
                .value_0a = static_cast<u16>(0x4400U + index),
                .value_0c = 0x55660000U + index,
                .value_10 = 0x77880000U + index,
                .value_14 = 0x99AA0000U + index,
                .value_18 = 0xBBCC0000U + index,
            };
        }

        for (u32 index = 0U; index < adjacent.size(); ++index) {
            adjacent[index].source_value = 0xAABB0000U + index;
            adjacent[index].value_04 = 0xCCDDEEFFU;
            adjacent[index].secondary_value = 0x12345678U;
            adjacent[index].value_0c = 0x87654321U;
            adjacent[index].x_offset = 0x10203040U;
            adjacent[index].y_offset = 0x50607080U;
            adjacent[index].render_flags = 0x90A0B0C0U;
            adjacent[index].unknown_1c.fill(0xA5U);
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleAttackOrderRemoveBindings
    bindings() {
        return {
            .records = std::span{records}.first(18U),
            .adjacent_intensity_records = adjacent,
        };
    }
};

template <typename Storage>
[[nodiscard]] bool equal_bytes(const Storage& actual, const Storage& expected) {
    return std::ranges::equal(
        std::as_bytes(std::span{&actual, 1U}),
        std::as_bytes(std::span{&expected, 1U})
    );
}

void fill_all_one(LegacyBattleStartupResetRecord& record) {
    std::ranges::fill(
        std::as_writable_bytes(std::span{&record, 1U}), std::byte{0xFFU}
    );
}

}  // namespace

void test_battle_attack_order_remove(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleAttackOrderRemoveStatus;
    using openswd3::battle::remove_legacy_battle_attack_order_entry;

    {
        Fixture fixture;
        fixture.records[18U].value_00 = 99U;
        const auto before = fixture.records;
        const auto adjacent_before = fixture.adjacent;
        auto bindings = fixture.bindings();
        bindings.records = fixture.records;
        bindings.adjacent_intensity_records = {};

        const auto result =
            remove_legacy_battle_attack_order_entry(bindings, 99U);

        test.expect_true(
            result.status == LegacyBattleAttackOrderRemoveStatus::completed &&
                !result.removed_index.has_value() &&
                result.shifted_records == 0U &&
                result.tail_dwords_written == 0U &&
                equal_bytes(fixture.records, before) &&
                equal_bytes(fixture.adjacent, adjacent_before),
            "no match scans only eighteen slots and preserves every queue and intensity byte even with an extra supplied slot"
        );
    }

    for (const u32 match : {0U, 2U, 17U}) {
        Fixture fixture;
        const auto adjacent_before = fixture.adjacent;
        auto expected = fixture.records;
        for (u32 destination = match; destination < 17U; ++destination) {
            expected[destination] = fixture.records[destination + 1U];
        }

        fill_all_one(expected[17U]);
        const auto result = remove_legacy_battle_attack_order_entry(
            fixture.bindings(), 100U + match
        );

        test.expect_true(
            result.status == LegacyBattleAttackOrderRemoveStatus::completed &&
                result.removed_index == match &&
                result.shifted_records == 18U - match &&
                result.tail_dwords_written == 14U &&
                equal_bytes(fixture.records, expected) &&
                equal_bytes(fixture.adjacent, adjacent_before),
            "first middle and last matches move all seven DWORD fields through the actual adjacent source and leave the fixed tail all FF"
        );
    }

    {
        Fixture fixture;
        fixture.records[0U].value_00 = 7U;
        fixture.records[1U].value_00 = 7U;
        fixture.records[2U].value_00 = 9U;
        auto expected = fixture.records;
        for (u32 destination = 0U; destination < 17U; ++destination) {
            expected[destination] = fixture.records[destination + 1U];
        }

        fill_all_one(expected[17U]);
        const auto result =
            remove_legacy_battle_attack_order_entry(fixture.bindings(), 7U);

        test.expect_true(
            result.removed_index == 0U && fixture.records[0U].value_00 == 7U &&
                equal_bytes(fixture.records, expected),
            "only the first equal value is removed and the later duplicate retains its complete payload"
        );
    }

    for (const u32 value : {0U, 0xFFFFFFFFU, 0x80000000U, 0x10000007U}) {
        Fixture fixture;
        fixture.records[0U].value_00 = 7U;
        fixture.records[4U].value_00 = value;
        fixture.records[4U].value_08 = 0xCAFEU;
        auto expected = fixture.records;
        for (u32 destination = 4U; destination < 17U; ++destination) {
            expected[destination] = fixture.records[destination + 1U];
        }

        fill_all_one(expected[17U]);
        const auto result =
            remove_legacy_battle_attack_order_entry(fixture.bindings(), value);

        test.expect_true(
            result.status == LegacyBattleAttackOrderRemoveStatus::completed &&
                result.removed_index == 4U && result.shifted_records == 14U &&
                equal_bytes(fixture.records, expected),
            "zero all-one high-bit and high-word values compare as complete DWORDs independently of the stored type"
        );
    }

    for (const u32 match : {0U, 17U}) {
        Fixture fixture;
        const auto adjacent_before = fixture.adjacent;
        auto expected = fixture.records;
        for (u32 destination = match; destination < 17U; ++destination) {
            expected[destination] = fixture.records[destination + 1U];
        }

        auto bindings = fixture.bindings();
        bindings.adjacent_intensity_records = {};
        const auto result =
            remove_legacy_battle_attack_order_entry(bindings, 100U + match);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        adjacent_record_typed_stop &&
                result.removed_index == match &&
                result.shifted_records == 17U - match &&
                result.tail_dwords_written == 0U &&
                equal_bytes(fixture.records, expected) &&
                equal_bytes(fixture.adjacent, adjacent_before),
            "the missing adjacent source preserves each earlier move and blocks both tail fills at the first one-past read"
        );
    }

    {
        Fixture fixture;
        auto expected = fixture.records;
        expected[0U] = fixture.records[1U];
        expected[1U] = fixture.records[2U];
        auto bindings = fixture.bindings();
        bindings.records = bindings.records.first(3U);
        const auto result =
            remove_legacy_battle_attack_order_entry(bindings, 100U);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        record_shift_source_typed_stop &&
                result.removed_index == 0U && result.shifted_records == 2U &&
                result.tail_dwords_written == 0U &&
                equal_bytes(fixture.records, expected),
            "a short owner retains two complete moved records and stops at the next source before the tail fills"
        );
    }

    for (const std::size_t count : {0U, 3U}) {
        Fixture fixture;
        const auto before = fixture.records;
        auto bindings = fixture.bindings();
        bindings.records = bindings.records.first(count);
        const auto result =
            remove_legacy_battle_attack_order_entry(bindings, 99U);

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderRemoveStatus::
                        record_scan_typed_stop &&
                !result.removed_index.has_value() &&
                result.shifted_records == 0U &&
                result.tail_dwords_written == 0U &&
                equal_bytes(fixture.records, before),
            "empty and short scan views stop at the first unavailable value without changing any record"
        );
    }
}
