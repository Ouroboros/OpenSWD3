#include "openswd3/battle/legacy_battle_attack_order_dequeue.hpp"

#include <array>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleAttackOrderDequeueActorReply;
using openswd3::battle::LegacyBattleAttackOrderDequeueActorRequest;
using openswd3::battle::LegacyBattleAttackOrderDequeueBindings;
using openswd3::battle::LegacyBattleAttackOrderDequeueOutput;
using openswd3::battle::LegacyBattleAttackOrderDequeuePort;
using openswd3::battle::LegacyBattleAttackOrderDequeueRequest;
using openswd3::battle::LegacyBattleAttackOrderDequeueStatus;
using openswd3::battle::LegacyBattleIntensityEffectRecord;
using openswd3::battle::LegacyBattleStartupResetRecord;
using openswd3::compat::u8;
using openswd3::compat::u32;

class ActorPort final : public LegacyBattleAttackOrderDequeuePort {
public:
    [[nodiscard]] LegacyBattleAttackOrderDequeueActorReply query_actor(
        const LegacyBattleAttackOrderDequeueActorRequest& request
    ) override {
        calls.push_back(request);
        if (next_reply < replies.size()) {
            return replies[next_reply++];
        }
        return {
            .eax = default_eax,
            .ecx = request.actor_token,
            .edx = request.stale_edx,
        };
    }

    std::vector<LegacyBattleAttackOrderDequeueActorRequest> calls;
    std::vector<LegacyBattleAttackOrderDequeueActorReply> replies;
    std::size_t next_reply{};
    u32 default_eax{};
};

struct Fixture {
    std::array<LegacyBattleStartupResetRecord, 18> records;
    std::array<LegacyBattleIntensityEffectRecord, 8> intensity;
    u32 output_first{0xAAAAAAAAU};
    std::array<u32, 6> output_tail{
        0xBBBBBBBBU,
        0xCCCCCCCCU,
        0xDDDDDDDDU,
        0xEEEEEEEEU,
        0x11111111U,
        0x22222222U,
    };
    ActorPort port;

    Fixture() {
        for (auto& record : records) {
            clear_record(record);
        }
    }

    static void clear_record(LegacyBattleStartupResetRecord& record) {
        record = {};
        record.value_00 = 0xFFFFFFFFU;
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
            .output = LegacyBattleAttackOrderDequeueOutput{
                .value_00 = &output_first,
                .tail_dwords = output_tail,
            },
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

void write_u32(std::array<u8, 0x2E>& bytes, const u32 offset, const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

}  // namespace

void test_battle_attack_order_dequeue(openswd3::test::Context& test) {

    {
        Fixture fixture;
        const auto before = fixture.records;
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(),
                fixture.port,
                {.entry_eax = 0x11111111U,
                 .entry_ecx = 0x22222222U,
                 .entry_edx = 0x33333333U}
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 0U && result.output_dwords == 7U &&
                result.actor_query_calls == 0U &&
                fixture.output_first == 0xFFFFFFFFU &&
                same_records(fixture.records, before) &&
                result.return_eax == 0x00524788U && result.return_ecx == 0U &&
                result.return_edx == 0x33333333U,
            "an empty first record is copied without mutating the attack order"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 3U, 0x100U);
        Fixture::seed_record(fixture.records[1], 9U, 0x200U);
        const auto expected = fixture.records[1];
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                fixture.output_first == 3U &&
                same_record(fixture.records[0], expected) &&
                fixture.records[1].value_00 == 0xFFFFFFFFU &&
                fixture.records[1].value_04 == 0U &&
                result.shifted_records == 17U &&
                result.cleared_records == 17U && result.return_eax == 0U &&
                result.return_ecx == 0U && result.return_edx == 0x00524980U,
            "a group-B record is copied, removed, and followed by an all-one zero tail"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 8U, 0x100U);
        Fixture::seed_record(fixture.records[1], 9U, 0x200U);
        fixture.port.replies = {
            {.eax = 1U, .ecx = 0x11111111U, .edx = 0x22222222U},
            {.eax = 0U, .ecx = 0x33333333U, .edx = 0x44444444U},
        };
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            );

        test.expect_true(
            result.selected_index == 1U && fixture.output_first == 9U &&
                result.actor_query_calls == 2U &&
                fixture.port.calls[0].actor_token == 0x005029D0U &&
                fixture.port.calls[0].stale_eax == 0U &&
                fixture.port.calls[1].actor_token == 0x00505904U &&
                fixture.port.calls[1].stale_eax == 0xBCDU,
            "leading group-A records use the exact actor token arithmetic and stop on the first non-one reply"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 7U, 0x100U);
        fixture.port.replies = {
            {.eax = 0U, .ecx = 0x12345678U, .edx = 0x87654321U},
        };
        static_cast<void>(
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            )
        );

        test.expect_true(
            fixture.port.calls.size() == 1U &&
                fixture.port.calls[0].actor_index == 0xFFFFFFFFU &&
                fixture.port.calls[0].stale_eax == 0xFFFFF433U &&
                fixture.port.calls[0].actor_token == 0x004FFA9CU,
            "record value seven preserves the one-before group-A token underflow"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 0x80000007U, 0x100U);
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            );

        test.expect_true(
            result.actor_query_calls == 0U &&
                fixture.output_first == 0x80000007U,
            "the initial record comparison remains signed"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 3U, 0x12340000U);
        std::array<u32, 2> short_tail{0xAAAAAAAAU, 0xBBBBBBBBU};
        auto bindings = fixture.bindings();
        bindings.output.tail_dwords = short_tail;
        const auto before = fixture.records;
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                bindings, fixture.port
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        output_destination_typed_stop &&
                result.output_dwords == 3U && fixture.output_first == 3U &&
                short_tail[0] == 0x12340001U && short_tail[1] == 0x00030002U &&
                result.return_eax == 0x00524788U && result.return_ecx == 4U &&
                same_records(fixture.records, before),
            "a short caller output stops on the first real movsd destination and preserves its prefix"
        );
    }

    {
        Fixture fixture;
        Fixture::seed_record(fixture.records[0], 8U, 0x100U);
        Fixture::seed_record(fixture.records[1], 8U, 0x200U);
        fixture.port.default_eax = 1U;
        auto bindings = fixture.bindings();
        bindings.records = std::span{fixture.records}.first(2U);
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                bindings, fixture.port, {.entry_edx = 0x13572468U}
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        record_scan_typed_stop &&
                result.actor_query_calls == 2U && result.return_eax == 1U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x13572468U,
            "a short attack-order owner stops at the next physical record read after preserving completed queries"
        );
    }

    {
        Fixture fixture;
        for (auto& record : fixture.records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }
        fixture.port.default_eax = 1U;
        fixture.intensity[0].source_value = 3U;
        fixture.intensity[0].value_04 = 0x11111111U;
        fixture.intensity[0].secondary_value = 0x22222222U;
        fixture.intensity[0].value_0c = 0x33333333U;
        fixture.intensity[0].x_offset = 0x44444444U;
        fixture.intensity[0].y_offset = 0x55555555U;
        fixture.intensity[0].render_flags = 0x66666666U;
        const auto before = fixture.records;
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port, {.entry_edx = 0x77777777U}
            );

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                result.selected_index == 18U &&
                result.selected_from_adjacent_intensity &&
                result.actor_query_calls == 18U &&
                same_records(fixture.records, before) &&
                fixture.output_first == 3U &&
                fixture.output_tail ==
                    std::array<u32, 6>{
                        0x11111111U,
                        0x22222222U,
                        0x33333333U,
                        0x44444444U,
                        0x55555555U,
                        0x66666666U,
                    } &&
                result.return_eax == 0x00524980U && result.return_ecx == 18U &&
                result.return_edx == 0x77777777U,
            "a full attack order continues into the exact adjacent intensity-record prefix"
        );
    }

    {
        Fixture fixture;
        for (auto& record : fixture.records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }
        fixture.port.default_eax = 1U;
        fixture.intensity[0].source_value = 8U;
        write_u32(fixture.intensity[0].unknown_1c, 0U, 2U);
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            );

        test.expect_true(
            result.selected_index == 19U && result.actor_query_calls == 19U &&
                fixture.output_first == 2U,
            "the unbounded 28-byte scan preserves its second physical intensity-record position"
        );
    }

    {
        Fixture fixture;
        for (auto& record : fixture.records) {
            Fixture::seed_record(record, 8U, 0x100U);
        }
        fixture.port.default_eax = 1U;
        auto bindings = fixture.bindings();
        bindings.adjacent_intensity_records = {};
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                bindings, fixture.port, {.entry_edx = 0xCAFEBABEU}
            );

        test.expect_true(
            result.status ==
                    LegacyBattleAttackOrderDequeueStatus::
                        record_scan_typed_stop &&
                result.actor_query_calls == 18U && result.return_eax == 1U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0xCAFEBABEU,
            "the first unavailable adjacent dword is the typed stop after eighteen successful actor queries"
        );
    }

    {
        Fixture fixture;
        for (u32 index = 0U; index < fixture.records.size(); ++index) {
            Fixture::seed_record(fixture.records[index], 2U, index * 0x100U);
        }
        const auto result =
            openswd3::battle::dequeue_legacy_battle_attack_order_entry(
                fixture.bindings(), fixture.port
            );
        bool all_empty = true;
        for (const auto& record : fixture.records) {
            all_empty = all_empty && record.value_00 == 0xFFFFFFFFU &&
                record.value_04 == 0U && record.value_08 == 0U &&
                record.value_0a == 0U && record.value_0c == 0U &&
                record.value_10 == 0U && record.value_14 == 0U &&
                record.value_18 == 0U;
        }

        test.expect_true(
            result.status == LegacyBattleAttackOrderDequeueStatus::completed &&
                all_empty && result.cleared_records == 18U,
            "a full nonempty table preserves the original fallback that clears from the selected slot"
        );
    }
}
