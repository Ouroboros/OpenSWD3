#include "openswd3/battle/legacy_battle_actor_frame_snapshot_clear.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "test.hpp"

#include <array>
#include <cstring>

namespace {

using openswd3::battle::LegacyBattleActorFrameSnapshotClearRequest;
using openswd3::battle::LegacyBattleActorFrameSnapshotClearStatus;
using openswd3::battle::LegacyBattleActorFrameSnapshotClearView;
using openswd3::compat::u32;

using SnapshotWords = std::
    array<u32, openswd3::battle::kLegacyBattleActorFrameSnapshotClearDwords>;

[[nodiscard]] LegacyBattleActorFrameSnapshotClearRequest request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0x11223344U,
        .entry_edx = 0x55667788U,
        .entry_edi = 0x99AABBCCU,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x0045529FU,
        .entry_flags = {
            .carry = true,
            .parity = false,
            .auxiliary_carry = true,
            .auxiliary_carry_defined = true,
            .zero = false,
            .sign = true,
            .overflow = true,
        },
    };
}

[[nodiscard]] SnapshotWords record_words(
    const openswd3::asset_runtime::LegacyActionRecord& record
) noexcept {
    SnapshotWords words{};
    std::memcpy(words.data(), &record, sizeof(record));
    return words;
}

void set_record_words(
    openswd3::asset_runtime::LegacyActionRecord& record,
    const SnapshotWords& words
) noexcept {
    std::memcpy(&record, words.data(), sizeof(record));
}

[[nodiscard]] SnapshotWords initial_words() noexcept {
    SnapshotWords words{};
    for (std::size_t index = 0U; index < words.size(); ++index) {
        words[index] = 0xA5000000U + static_cast<u32>(index);
    }
    return words;
}

[[nodiscard]] bool flags_equal(
    const openswd3::battle::LegacyBattleActorCoordinateFlags& left,
    const openswd3::battle::LegacyBattleActorCoordinateFlags& right
) noexcept {
    return left.carry == right.carry && left.parity == right.parity &&
        left.auxiliary_carry == right.auxiliary_carry &&
        left.auxiliary_carry_defined == right.auxiliary_carry_defined &&
        left.zero == right.zero && left.sign == right.sign &&
        left.overflow == right.overflow;
}

[[nodiscard]] bool xor_zero_flags(
    const openswd3::battle::LegacyBattleActorCoordinateFlags& flags
) noexcept {
    return !flags.carry && flags.parity && !flags.auxiliary_carry_defined &&
        flags.zero && !flags.sign && !flags.overflow;
}

}  // namespace

void test_battle_actor_frame_snapshot_clear(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionDispatchState action;
        auto& expected =
            action.group_a_action_execution[3U].frame_source_action_record;
        const auto token =
            openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken +
            3U * openswd3::battle::kLegacyBattleActorCoordinatesGroupAStride;
        const auto resolved =
            openswd3::battle::resolve_legacy_battle_actor_frame_snapshot_clear(
                {.action = &action}, token
            );
        const auto missing =
            openswd3::battle::resolve_legacy_battle_actor_frame_snapshot_clear(
                {}, token
            );
        test.expect_true(
            resolved.frame_source_action_record == &expected &&
                resolved.reverse_destination_bytes == nullptr &&
                missing.frame_source_action_record == nullptr,
            "frame-snapshot clear resolver aliases the canonical Group-A slot-zero action record"
        );
    }

    {
        openswd3::asset_runtime::LegacyActionRecord record;
        set_record_words(record, initial_words());
        const auto entry = request();
        const auto result =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                {.frame_source_action_record = &record}, entry
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::completed &&
                result.returned && record_words(record) == SnapshotWords{} &&
                result.destination_writes == 0x26U &&
                result.cleared_dwords == 0x26U &&
                result.fault_dword_index == 0x26U && result.return_eax == 0U &&
                result.return_ecx == 0U &&
                result.return_edx == entry.actor_token &&
                result.return_edi == entry.entry_edi &&
                result.return_esp == entry.entry_esp + 4U &&
                result.return_eip == entry.entry_return_address &&
                result.region_start_token == entry.actor_token + 0x02A0U &&
                result.current_destination_token ==
                    entry.actor_token + 0x0338U &&
                result.stack_write_count == 1U &&
                result.stack_writes[0U] == entry.entry_edi &&
                result.stack_read_count == 2U &&
                result.stack_reads[0U] == entry.entry_edi &&
                result.stack_reads[1U] == entry.entry_return_address &&
                result.flags_known && xor_zero_flags(result.flags) &&
                !result.direction_flag,
            "forward REP STOSD clears exactly one action record and preserves the plain-RET stack contract"
        );
    }

    {
        const auto words = initial_words();
        for (std::size_t fault = 0U; fault < words.size(); ++fault) {
            openswd3::asset_runtime::LegacyActionRecord record;
            set_record_words(record, words);
            auto entry = request();
            entry.destination_dword_writable[fault] = false;
            const auto result =
                openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                    {.frame_source_action_record = &record}, entry
                );
            const auto actual = record_words(record);
            bool prefix_matches = true;
            for (std::size_t index = 0U; index < words.size(); ++index) {
                prefix_matches = prefix_matches &&
                    actual[index] == (index < fault ? 0U : words[index]);
            }
            test.expect_true(
                result.status ==
                        LegacyBattleActorFrameSnapshotClearStatus::
                            destination_dword_write_typed_stop &&
                    result.fault_dword_index == fault &&
                    result.destination_writes == fault &&
                    result.cleared_dwords == fault && prefix_matches &&
                    result.return_eax == 0U &&
                    result.return_ecx == words.size() - fault &&
                    result.return_edx == entry.actor_token &&
                    result.return_edi ==
                        entry.actor_token + 0x02A0U + 4U * fault &&
                    result.return_esp == entry.entry_esp - 4U &&
                    result.return_eip == 0x00478700U &&
                    result.stack_write_count == 1U &&
                    result.stack_read_count == 0U &&
                    xor_zero_flags(result.flags),
                "each forward STOSD fault preserves the already-cleared dword prefix"
            );
        }
    }

    {
        const auto words = initial_words();
        alignas(u32) std::array<std::byte, sizeof(SnapshotWords)>
            reverse_storage{};
        std::memcpy(reverse_storage.data(), words.data(), sizeof(words));
        auto entry = request();
        entry.direction_flag = true;
        const auto result =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                {.reverse_destination_bytes = reverse_storage.data()}, entry
            );
        SnapshotWords actual{};
        std::memcpy(actual.data(), reverse_storage.data(), sizeof(actual));
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::completed &&
                actual == SnapshotWords{} && result.returned &&
                result.direction_flag && result.cleared_dwords == 0x26U &&
                result.return_edi == entry.entry_edi &&
                result.current_destination_token ==
                    entry.actor_token + 0x0208U &&
                result.return_ecx == 0U && xor_zero_flags(result.flags),
            "set DF walks from actor plus 0x2A0 down through the preceding physical dword window"
        );

        for (std::size_t fault = 0U; fault < words.size(); ++fault) {
            std::memcpy(reverse_storage.data(), words.data(), sizeof(words));
            auto fault_entry = entry;
            fault_entry.destination_dword_writable[fault] = false;
            const auto stopped =
                openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                    {.reverse_destination_bytes = reverse_storage.data()},
                    fault_entry
                );
            std::memcpy(actual.data(), reverse_storage.data(), sizeof(actual));
            bool suffix_matches = true;
            const std::size_t first_cleared = words.size() - fault;
            for (std::size_t index = 0U; index < words.size(); ++index) {
                suffix_matches = suffix_matches &&
                    actual[index] ==
                        (index >= first_cleared ? 0U : words[index]);
            }
            test.expect_true(
                stopped.status ==
                        LegacyBattleActorFrameSnapshotClearStatus::
                            destination_dword_write_typed_stop &&
                    stopped.fault_dword_index == fault &&
                    stopped.cleared_dwords == fault && suffix_matches &&
                    stopped.return_edi ==
                        fault_entry.actor_token + 0x02A0U - 4U * fault &&
                    stopped.return_ecx == words.size() - fault &&
                    stopped.return_eip == 0x00478700U &&
                    stopped.direction_flag && xor_zero_flags(stopped.flags),
                "each reverse STOSD fault preserves the physically descending cleared suffix"
            );
        }
    }

    {
        openswd3::asset_runtime::LegacyActionRecord record;
        const auto words = initial_words();
        set_record_words(record, words);
        auto push_request = request();
        push_request.stack_access.push_edi_writable = false;
        push_request.entry_flags_known = false;
        const auto push_stop =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                {.frame_source_action_record = &record}, push_request
            );

        set_record_words(record, words);
        auto pop_request = request();
        pop_request.stack_access.pop_edi_readable = false;
        const auto pop_stop =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                {.frame_source_action_record = &record}, pop_request
            );

        set_record_words(record, words);
        auto return_request = request();
        return_request.stack_access.return_address_readable = false;
        const auto return_stop =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                {.frame_source_action_record = &record}, return_request
            );

        test.expect_true(
            push_stop.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::
                        push_edi_write_typed_stop &&
                push_stop.return_eax == push_request.entry_eax &&
                push_stop.return_ecx == push_request.actor_token &&
                push_stop.return_edx == push_request.actor_token &&
                push_stop.return_edi == push_request.entry_edi &&
                push_stop.return_esp == push_request.entry_esp &&
                push_stop.return_eip == 0x004786F2U &&
                push_stop.stack_write_count == 0U && !push_stop.flags_known &&
                flags_equal(push_stop.flags, push_request.entry_flags) &&
                record_words(record) == SnapshotWords{} &&
                pop_stop.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::
                        pop_edi_read_typed_stop &&
                pop_stop.return_eax == 0U && pop_stop.return_ecx == 0U &&
                pop_stop.return_edx == pop_request.actor_token &&
                pop_stop.return_edi == pop_request.actor_token + 0x0338U &&
                pop_stop.return_esp == pop_request.entry_esp - 4U &&
                pop_stop.return_eip == 0x00478702U &&
                pop_stop.stack_write_count == 1U &&
                pop_stop.stack_read_count == 0U &&
                xor_zero_flags(pop_stop.flags) &&
                return_stop.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::
                        return_address_read_typed_stop &&
                return_stop.return_edi == return_request.entry_edi &&
                return_stop.return_esp == return_request.entry_esp &&
                return_stop.return_eip == 0x00478703U &&
                return_stop.stack_write_count == 1U &&
                return_stop.stack_read_count == 1U &&
                return_stop.stack_reads[0U] == return_request.entry_edi &&
                !return_stop.returned && xor_zero_flags(return_stop.flags),
            "PUSH, POP, and RET stops preserve their exact instruction prefixes and machine state"
        );
    }

    {
        auto entry = request();
        const auto forward_missing =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                LegacyBattleActorFrameSnapshotClearView{}, entry
            );
        entry.direction_flag = true;
        const auto reverse_missing =
            openswd3::battle::clear_legacy_battle_actor_frame_snapshot(
                LegacyBattleActorFrameSnapshotClearView{}, entry
            );
        test.expect_true(
            forward_missing.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::
                        destination_dword_write_typed_stop &&
                reverse_missing.status ==
                    LegacyBattleActorFrameSnapshotClearStatus::
                        destination_dword_write_typed_stop &&
                forward_missing.fault_dword_index == 0U &&
                reverse_missing.fault_dword_index == 0U &&
                forward_missing.current_destination_token ==
                    entry.actor_token + 0x02A0U &&
                reverse_missing.current_destination_token ==
                    entry.actor_token + 0x02A0U,
            "missing forward or reverse physical storage stops at the first STOSD without inventing an owner"
        );
    }
}
