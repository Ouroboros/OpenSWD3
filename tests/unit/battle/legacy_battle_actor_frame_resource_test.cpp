#include "openswd3/battle/legacy_battle_actor_frame_resource.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <cstring>
#include <memory>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleActorFrameResourceRequest;
using openswd3::battle::LegacyBattleActorFrameResourceResult;
using openswd3::battle::LegacyBattleActorFrameResourceStackAccess;
using openswd3::battle::LegacyBattleActorFrameResourceStatus;
using openswd3::battle::LegacyBattleActorFrameResourceView;
using openswd3::compat::u32;

class StreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        ++calls;
        return result;
    }

    openswd3::asset_runtime::LegacyActionStreamLoadResult result{};
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
        piece.width = 0x1234U;
        piece.height = 0x5678U;
        return available;
    }

    bool available{true};
    std::vector<std::array<u32, 2>> requests;
};

[[nodiscard]] std::
    array<u32, openswd3::battle::kLegacyBattleActorFrameResourceDwords>
    record_words(const openswd3::asset_runtime::LegacyActionRecord& record) {
    std::array<u32, openswd3::battle::kLegacyBattleActorFrameResourceDwords>
        words{};
    std::memcpy(words.data(), &record, sizeof(record));
    return words;
}

void set_record_words(
    openswd3::asset_runtime::LegacyActionRecord& record,
    const std::array<
        u32,
        openswd3::battle::kLegacyBattleActorFrameResourceDwords>& words
) {
    std::memcpy(&record, words.data(), sizeof(record));
}

struct Fixture {
    openswd3::asset_runtime::LegacyActionRecord source{};
    openswd3::asset_runtime::LegacyActionRecord prepared{};
    u32 frame_token{0xCCCCCCCCU};
    bool frame_token_writable{true};
    StreamProvider stream_provider;
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        stream_provider
    };
    FrameProvider frame_provider;

    [[nodiscard]] LegacyBattleActorFrameResourceView view() noexcept {
        return {
            .source_action = &source,
            .prepared_action = &prepared,
            .frame_token = &frame_token,
            .frame_token_write_accessible = &frame_token_writable,
        };
    }

    [[nodiscard]] LegacyBattleActorFrameResourceResult
    run(const LegacyBattleActorFrameResourceRequest& request = {}) {
        return openswd3::battle::prepare_legacy_battle_actor_frame_resource(
            view(), action_updater, frame_provider, request
        );
    }
};

[[nodiscard]] LegacyBattleActorFrameResourceRequest success_request() {
    return {
        .actor_token = 0x005029D0U,
        .entry_eax = 0x01020304U,
        .entry_ecx = 0x005029D0U,
        .entry_edx = 0x11112222U,
        .entry_ebx = 0x33334444U,
        .entry_esi = 0x55556666U,
        .entry_edi = 0x77778888U,
        .entry_esp = 0x80001000U,
        .entry_return_address = 0x004605DEU,
        .entry_flags =
            {
                .carry = true,
                .parity = true,
                .auxiliary_carry = true,
                .auxiliary_carry_defined = true,
                .zero = true,
                .sign = false,
                .overflow = true,
            },
        .action_updater_return_eax = 0xCAFE0001U,
        .action_updater_return_ecx = 0xABCD7654U,
        .action_updater_return_edx = 0xDEADBEEFU,
        .override_action_updater_return_eax = true,
        .frame_provider_return_eax = 0x0BADF00DU,
        .frame_provider_return_ecx = 0x12345678U,
        .frame_provider_return_edx = 0x89ABCDEFU,
    };
}

}  // namespace

void test_battle_actor_frame_resource(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleStartupState startup;
        openswd3::battle::LegacyBattleActionDispatchState action;
        startup.group_b_lifecycle = std::make_shared<std::array<
            openswd3::battle::LegacyBattleActorGroupBElementState,
            8>>();
        const auto group_a =
            openswd3::battle::resolve_legacy_battle_actor_frame_resource(
                {.action = &action, .startup = &startup},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupABaseToken
            );
        const auto group_b =
            openswd3::battle::resolve_legacy_battle_actor_frame_resource(
                {.action = &action, .startup = &startup},
                openswd3::battle::kLegacyBattleActorCoordinatesGroupBBaseToken
            );
        test.expect_true(
            group_a.source_action ==
                    &action.group_a_action_execution[0U]
                         .frame_source_action_record &&
                group_a.prepared_action ==
                    &action.group_a_action_execution[0U]
                         .frame_prepared_action_record &&
                group_b.source_action ==
                    &(*startup.group_b_lifecycle)[0U]
                         .action_execution.frame_source_action_record &&
                group_b.frame_token ==
                    &(*startup.group_b_lifecycle)[0U]
                         .action_execution.turn_frame_token,
            "frame resource resolves Group-A action and Group-B lifecycle owners"
        );
    }

    {
        Fixture fixture;
        std::array<u32, openswd3::battle::kLegacyBattleActorFrameResourceDwords>
            words{};
        for (std::size_t index = 0U; index < words.size(); ++index) {
            words[index] = 0x11000000U + static_cast<u32>(index);
        }
        set_record_words(fixture.source, words);
        fixture.source.action_id = 0U;
        fixture.source.field_4a = 0x1357U;
        fixture.source.field_4c = 0x2468U;
        const auto expected = record_words(fixture.source);
        const auto request = success_request();
        const auto result = fixture.run(request);
        test.expect_true(
            result.status == LegacyBattleActorFrameResourceStatus::completed &&
                result.returned && !result.returned_early &&
                record_words(fixture.prepared) == expected &&
                result.copied_dwords == 0x26U && result.source_reads == 0x28U &&
                result.destination_writes == 0x26U &&
                result.action_update_calls == 1U &&
                result.frame_lookup_calls == 1U &&
                fixture.frame_provider.requests ==
                    std::vector<std::array<u32, 2>>{{0x1357U, 0x2468U}} &&
                result.frame_id == 0x2468U && result.resource_id == 0x1357U &&
                result.action_updater_entry_eax ==
                    request.actor_token + 0x0CB8U &&
                result.action_updater_entry_ecx == 0U &&
                result.action_updater_entry_edx == request.entry_edx &&
                result.frame_provider_entry_eax == 0xCAFE2468U &&
                result.frame_provider_entry_ecx == 0xABCD1357U &&
                result.frame_provider_entry_edx == 0xDEADBEEFU &&
                fixture.frame_token == 0x0BADF00DU &&
                result.frame_token_committed && result.actor_writes == 1U &&
                result.return_eax == 0x0BADF00DU &&
                result.return_ecx == 0x12345678U &&
                result.return_edx == 0x89ABCDEFU &&
                result.return_ebx == request.entry_ebx &&
                result.return_esi == request.entry_esi &&
                result.return_edi == request.entry_edi &&
                result.return_esp == request.entry_esp + 4U &&
                result.stack_writes ==
                    std::array<u32, 8>{
                        request.entry_ebx,
                        request.entry_esi,
                        request.entry_edi,
                        request.actor_token + 0x0CB8U,
                        0x00478640U,
                        0xCAFE2468U,
                        0xABCD1357U,
                        0x00478660U,
                    } &&
                result.stack_write_count == 8U &&
                result.stack_reads ==
                    std::array<u32, 6>{
                        0x00478640U,
                        0x00478660U,
                        request.entry_edi,
                        request.entry_esi,
                        request.entry_ebx,
                        request.entry_return_address,
                    } &&
                result.stack_read_count == 6U &&
                result.return_eip == request.entry_return_address &&
                result.flags_known && !result.flags.carry &&
                !result.flags.parity && result.flags.auxiliary_carry_defined &&
                result.flags.auxiliary_carry && !result.flags.zero &&
                result.flags.sign && !result.flags.overflow,
            "normal frame resource preparation preserves copy order, caller stack, callee residues, and ADD flags"
        );
    }

    {
        Fixture fixture;
        fixture.source.action_id = 1U;
        const auto request = success_request();
        auto early_request = request;
        early_request.override_action_updater_return_eax = false;
        const auto before = fixture.prepared;
        const auto result = fixture.run(early_request);
        test.expect_true(
            result.status == LegacyBattleActorFrameResourceStatus::completed &&
                result.returned && result.returned_early &&
                result.action_update.status ==
                    openswd3::asset_runtime::LegacyActionUpdateStatus::
                        stream_load_failed &&
                result.return_eax == 0U &&
                result.return_ecx == early_request.action_updater_return_ecx &&
                result.return_edx == early_request.action_updater_return_edx &&
                result.frame_lookup_calls == 0U &&
                fixture.frame_provider.requests.empty() &&
                fixture.frame_token == 0xCCCCCCCCU &&
                !result.frame_token_committed && result.flags_known &&
                result.flags.zero && result.flags.parity &&
                !result.flags.auxiliary_carry_defined &&
                result.return_esp == early_request.entry_esp + 4U &&
                std::memcmp(
                    &fixture.prepared, &before, sizeof(fixture.prepared)
                ) != 0,
            "zero updater return keeps its mutated destination and takes the TEST-governed early return"
        );
    }

    {
        std::array<u32, openswd3::battle::kLegacyBattleActorFrameResourceDwords>
            source_words{};
        std::array<u32, openswd3::battle::kLegacyBattleActorFrameResourceDwords>
            initial_words{};
        for (std::size_t index = 0U; index < source_words.size(); ++index) {
            source_words[index] = 0xA0000000U + static_cast<u32>(index);
            initial_words[index] = 0xB0000000U + static_cast<u32>(index);
        }
        for (std::size_t fault = 0U; fault < source_words.size(); ++fault) {
            Fixture fixture;
            set_record_words(fixture.source, source_words);
            set_record_words(fixture.prepared, initial_words);
            auto request = success_request();
            request.source_dword_readable[fault] = false;
            const auto result = fixture.run(request);
            const auto actual = record_words(fixture.prepared);
            bool prefix_matches = true;
            for (std::size_t index = 0U; index < source_words.size(); ++index) {
                const u32 expected =
                    index < fault ? source_words[index] : initial_words[index];
                prefix_matches = prefix_matches && actual[index] == expected;
            }
            test.expect_true(
                result.status ==
                        LegacyBattleActorFrameResourceStatus::
                            source_dword_read_typed_stop &&
                    result.fault_dword_index == fault &&
                    result.return_eip == 0x00478638U &&
                    result.source_reads == fault &&
                    result.destination_writes == fault &&
                    result.copied_dwords == fault && prefix_matches &&
                    result.return_esi ==
                        request.actor_token + 0x02A0U + 4U * fault &&
                    result.return_edi ==
                        request.actor_token + 0x0CB8U + 4U * fault &&
                    result.return_ecx == source_words.size() - fault &&
                    result.flags.carry,
                "each REP source fault preserves the previously copied dword prefix"
            );
        }

        for (std::size_t fault = 0U; fault < source_words.size(); ++fault) {
            Fixture fixture;
            set_record_words(fixture.source, source_words);
            set_record_words(fixture.prepared, initial_words);
            auto request = success_request();
            request.destination_dword_writable[fault] = false;
            const auto result = fixture.run(request);
            const auto actual = record_words(fixture.prepared);
            bool prefix_matches = true;
            for (std::size_t index = 0U; index < source_words.size(); ++index) {
                const u32 expected =
                    index < fault ? source_words[index] : initial_words[index];
                prefix_matches = prefix_matches && actual[index] == expected;
            }
            test.expect_true(
                result.status ==
                        LegacyBattleActorFrameResourceStatus::
                            destination_dword_write_typed_stop &&
                    result.fault_dword_index == fault &&
                    result.return_eip == 0x00478638U &&
                    result.source_reads == fault + 1U &&
                    result.destination_writes == fault &&
                    result.copied_dwords == fault && prefix_matches &&
                    result.return_esi ==
                        request.actor_token + 0x02A0U + 4U * fault &&
                    result.return_edi ==
                        request.actor_token + 0x0CB8U + 4U * fault &&
                    result.return_ecx == source_words.size() - fault,
                "each REP destination fault leaves the current dword uncommitted"
            );
        }
    }

    {
        alignas(u32) std::array<
            std::byte,
            openswd3::asset_runtime::kLegacyActionRecordSize + sizeof(u32)>
            storage{};
        const u32 first = 0x11223344U;
        std::memcpy(storage.data(), &first, sizeof(first));
        for (std::size_t index = 1U; index <
             openswd3::battle::kLegacyBattleActorFrameResourceDwords + 1U;
             ++index) {
            const u32 value = 0xA0000000U + static_cast<u32>(index);
            std::memcpy(
                storage.data() + index * sizeof(u32), &value, sizeof(value)
            );
        }
        StreamProvider stream_provider;
        openswd3::asset_runtime::LegacyActionUpdater updater{stream_provider};
        FrameProvider frame_provider;
        auto request = success_request();
        request.source_dword_readable.back() = false;
        const auto result =
            openswd3::battle::prepare_legacy_battle_actor_frame_resource(
                {
                    .source_action_bytes = storage.data(),
                    .prepared_action_bytes = storage.data() + sizeof(u32),
                },
                updater,
                frame_provider,
                request
            );
        bool propagated = true;
        for (std::size_t index = 1U;
             index < openswd3::battle::kLegacyBattleActorFrameResourceDwords;
             ++index) {
            u32 value{};
            std::memcpy(
                &value, storage.data() + index * sizeof(u32), sizeof(value)
            );
            propagated = propagated && value == first;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameResourceStatus::
                        source_dword_read_typed_stop &&
                result.fault_dword_index ==
                    openswd3::battle::kLegacyBattleActorFrameResourceDwords -
                        1U &&
                result.return_eip == 0x00478638U &&
                result.copied_dwords ==
                    openswd3::battle::kLegacyBattleActorFrameResourceDwords -
                        1U &&
                propagated,
            "forward overlapping REP MOVSD observes each preceding destination write instead of snapshotting the source"
        );
    }

    {
        Fixture fixture;
        fixture.source.field_4a = 0x1357U;
        fixture.source.field_4c = 0x2468U;
        auto request = success_request();
        request.prepared_frame_word_readable = false;
        const auto frame_stop = fixture.run(request);

        Fixture resource_fixture;
        resource_fixture.source.field_4a = 0x1357U;
        resource_fixture.source.field_4c = 0x2468U;
        request.prepared_frame_word_readable = true;
        request.prepared_resource_word_readable = false;
        const auto resource_stop = resource_fixture.run(request);
        test.expect_true(
            frame_stop.status ==
                    LegacyBattleActorFrameResourceStatus::
                        prepared_frame_word_read_typed_stop &&
                frame_stop.return_eax == 0xCAFE0001U &&
                frame_stop.return_ecx == 0xABCD7654U &&
                frame_stop.return_edx == 0xDEADBEEFU &&
                frame_stop.return_eip == 0x0047864BU &&
                resource_stop.status ==
                    LegacyBattleActorFrameResourceStatus::
                        prepared_resource_word_read_typed_stop &&
                resource_stop.return_eax == 0xCAFE2468U &&
                resource_stop.return_ecx == 0xABCD7654U &&
                resource_stop.return_edx == 0xDEADBEEFU &&
                resource_stop.return_eip == 0x00478652U &&
                frame_stop.flags_known && !frame_stop.flags.zero &&
                resource_stop.flags_known && !resource_stop.flags.zero,
            "word faults preserve updater high words and only completed low-word replacements"
        );
    }

    {
        struct StackCase {
            bool LegacyBattleActorFrameResourceStackAccess::* field;
            LegacyBattleActorFrameResourceStatus status;
        };
        constexpr std::array<StackCase, 12> cases{
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::push_ebx_writable,
                LegacyBattleActorFrameResourceStatus::push_ebx_write_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::push_esi_writable,
                LegacyBattleActorFrameResourceStatus::push_esi_write_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::push_edi_writable,
                LegacyBattleActorFrameResourceStatus::push_edi_write_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    action_argument_push_writable,
                LegacyBattleActorFrameResourceStatus::
                    action_argument_push_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    action_call_return_push_writable,
                LegacyBattleActorFrameResourceStatus::
                    action_call_return_push_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    frame_argument_push_writable,
                LegacyBattleActorFrameResourceStatus::
                    frame_argument_push_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    resource_argument_push_writable,
                LegacyBattleActorFrameResourceStatus::
                    resource_argument_push_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    frame_call_return_push_writable,
                LegacyBattleActorFrameResourceStatus::
                    frame_call_return_push_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_edi_readable,
                LegacyBattleActorFrameResourceStatus::pop_edi_read_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_esi_readable,
                LegacyBattleActorFrameResourceStatus::pop_esi_read_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_ebx_readable,
                LegacyBattleActorFrameResourceStatus::pop_ebx_read_typed_stop,
            },
            StackCase{
                &LegacyBattleActorFrameResourceStackAccess::
                    success_return_address_readable,
                LegacyBattleActorFrameResourceStatus::
                    success_return_address_read_typed_stop,
            },
        };
        constexpr std::array<u32, 12> expected_stack_writes{
            0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 8U, 8U, 8U
        };
        constexpr std::array<u32, 12> expected_stack_reads{
            0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 2U, 3U, 4U, 5U
        };
        constexpr std::array<u32, 12> expected_stack_pointers{
            0x80001000U,
            0x80000FFCU,
            0x80000FF8U,
            0x80000FF4U,
            0x80000FF0U,
            0x80000FF4U,
            0x80000FF0U,
            0x80000FECU,
            0x80000FF4U,
            0x80000FF8U,
            0x80000FFCU,
            0x80001000U,
        };
        constexpr std::array<u32, 12> expected_fault_eips{
            0x00478620U,
            0x00478623U,
            0x00478624U,
            0x0047863AU,
            0x0047863BU,
            0x00478659U,
            0x0047865AU,
            0x0047865BU,
            0x00478669U,
            0x0047866AU,
            0x0047866BU,
            0x0047866CU,
        };
        for (std::size_t index = 0U; index < cases.size(); ++index) {
            const auto& current = cases[index];
            Fixture fixture;
            auto request = success_request();
            request.stack_access.*(current.field) = false;
            const auto result = fixture.run(request);
            const std::array<u32, 8> expected_values{
                request.entry_ebx,
                request.entry_esi,
                request.entry_edi,
                request.actor_token + 0x0CB8U,
                0x00478640U,
                0xCAFE0000U,
                0xABCD0000U,
                0x00478660U,
            };
            bool stack_prefix_matches = true;
            for (std::size_t stack_index = 0U;
                 stack_index < result.stack_write_count;
                 ++stack_index) {
                stack_prefix_matches = stack_prefix_matches &&
                    result.stack_writes[stack_index] ==
                        expected_values[stack_index];
            }

            u32 expected_eax = request.entry_eax;
            u32 expected_ecx = request.entry_ecx;
            u32 expected_edx = request.entry_edx;
            u32 expected_ebx = request.entry_ebx;
            u32 expected_esi = request.entry_esi;
            u32 expected_edi = request.entry_edi;
            if (index >= 1U) {
                expected_ebx = request.entry_ecx;
            }
            if (index >= 3U) {
                expected_eax = request.actor_token + 0x0CB8U;
                expected_ecx = 0U;
                expected_esi = request.actor_token + 0x02A0U + 0x98U;
                expected_edi = request.actor_token + 0x0CB8U + 0x98U;
            }
            if (index >= 5U) {
                expected_eax = 0xCAFE0000U;
                expected_ecx = 0xABCD0000U;
                expected_edx = 0xDEADBEEFU;
            }
            if (index >= 8U) {
                expected_eax = request.frame_provider_return_eax;
                expected_ecx = request.frame_provider_return_ecx;
                expected_edx = request.frame_provider_return_edx;
            }
            if (index >= 9U) {
                expected_edi = request.entry_edi;
            }
            if (index >= 10U) {
                expected_esi = request.entry_esi;
            }
            if (index >= 11U) {
                expected_ebx = request.entry_ebx;
            }

            bool flags_match{};
            if (index <= 4U) {
                flags_match = result.flags.carry == request.entry_flags.carry &&
                    result.flags.parity == request.entry_flags.parity &&
                    result.flags.auxiliary_carry ==
                        request.entry_flags.auxiliary_carry &&
                    result.flags.auxiliary_carry_defined ==
                        request.entry_flags.auxiliary_carry_defined &&
                    result.flags.zero == request.entry_flags.zero &&
                    result.flags.sign == request.entry_flags.sign &&
                    result.flags.overflow == request.entry_flags.overflow;
            } else if (index <= 7U) {
                flags_match = !result.flags.carry && !result.flags.parity &&
                    !result.flags.auxiliary_carry_defined &&
                    !result.flags.zero && result.flags.sign &&
                    !result.flags.overflow;
            } else {
                flags_match = !result.flags.carry && !result.flags.parity &&
                    result.flags.auxiliary_carry_defined &&
                    result.flags.auxiliary_carry && !result.flags.zero &&
                    result.flags.sign && !result.flags.overflow;
            }
            test.expect_true(
                result.status == current.status && !result.returned &&
                    result.stack_write_count == expected_stack_writes[index] &&
                    result.stack_read_count == expected_stack_reads[index] &&
                    result.return_esp == expected_stack_pointers[index] &&
                    result.return_eip == expected_fault_eips[index] &&
                    result.action_update_calls == (index >= 5U ? 1U : 0U) &&
                    result.frame_lookup_calls == (index >= 8U ? 1U : 0U) &&
                    result.frame_token_committed == (index >= 8U) &&
                    result.copied_dwords == (index >= 3U ? 0x26U : 0U) &&
                    stack_prefix_matches && result.flags_known && flags_match &&
                    result.return_eax == expected_eax &&
                    result.return_ecx == expected_ecx &&
                    result.return_edx == expected_edx &&
                    result.return_ebx == expected_ebx &&
                    result.return_esi == expected_esi &&
                    result.return_edi == expected_edi,
                "every normal-path stack typed stop preserves its exact fault EIP, stack, registers, flags, callee, actor, and copy prefix"
            );
        }

        struct EarlyPopCase {
            bool LegacyBattleActorFrameResourceStackAccess::* field;
            LegacyBattleActorFrameResourceStatus status;
            u32 reads;
            u32 esp;
            u32 fault_eip;
        };
        constexpr std::array<EarlyPopCase, 3> early_pop_cases{
            EarlyPopCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_edi_readable,
                LegacyBattleActorFrameResourceStatus::pop_edi_read_typed_stop,
                1U,
                0x80000FF4U,
                0x00478647U,
            },
            EarlyPopCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_esi_readable,
                LegacyBattleActorFrameResourceStatus::pop_esi_read_typed_stop,
                2U,
                0x80000FF8U,
                0x00478648U,
            },
            EarlyPopCase{
                &LegacyBattleActorFrameResourceStackAccess::pop_ebx_readable,
                LegacyBattleActorFrameResourceStatus::pop_ebx_read_typed_stop,
                3U,
                0x80000FFCU,
                0x00478649U,
            },
        };
        for (std::size_t index = 0U; index < early_pop_cases.size(); ++index) {
            Fixture fixture;
            fixture.source.action_id = 1U;
            auto request = success_request();
            request.override_action_updater_return_eax = false;
            request.stack_access.*(early_pop_cases[index].field) = false;
            const auto result = fixture.run(request);
            test.expect_true(
                result.status == early_pop_cases[index].status &&
                    result.returned_early && !result.returned &&
                    result.return_eax == 0U &&
                    result.return_ecx == request.action_updater_return_ecx &&
                    result.return_edx == request.action_updater_return_edx &&
                    result.return_esp == early_pop_cases[index].esp &&
                    result.return_eip == early_pop_cases[index].fault_eip &&
                    result.stack_write_count == 5U &&
                    result.stack_read_count == early_pop_cases[index].reads &&
                    result.stack_writes[0U] == request.entry_ebx &&
                    result.stack_writes[1U] == request.entry_esi &&
                    result.stack_writes[2U] == request.entry_edi &&
                    result.stack_writes[3U] == request.actor_token + 0x0CB8U &&
                    result.stack_writes[4U] == 0x00478640U &&
                    result.stack_reads[0U] == 0x00478640U &&
                    (index < 1U ||
                     result.stack_reads[1U] == request.entry_edi) &&
                    (index < 2U ||
                     result.stack_reads[2U] == request.entry_esi) &&
                    result.copied_dwords == 0x26U &&
                    result.action_update_calls == 1U &&
                    result.frame_lookup_calls == 0U &&
                    !result.frame_token_committed && result.flags_known &&
                    !result.flags.carry && result.flags.parity &&
                    !result.flags.auxiliary_carry_defined &&
                    result.flags.zero && !result.flags.sign &&
                    !result.flags.overflow &&
                    result.return_edi ==
                        (index >= 1U ? request.entry_edi
                                     : request.actor_token + 0x0CB8U + 0x98U) &&
                    result.return_esi ==
                        (index >= 2U ? request.entry_esi
                                     : request.actor_token + 0x02A0U + 0x98U) &&
                    result.return_ebx == request.entry_ecx,
                "the updater-zero path exposes each POP typed stop with exact fault EIP, TEST flags, and stack prefix"
            );
        }

        Fixture early_fixture;
        early_fixture.source.action_id = 1U;
        auto early_request = success_request();
        early_request.override_action_updater_return_eax = false;
        early_request.stack_access.early_return_address_readable = false;
        const auto early = early_fixture.run(early_request);
        test.expect_true(
            early.status ==
                    LegacyBattleActorFrameResourceStatus::
                        early_return_address_read_typed_stop &&
                early.returned_early && !early.returned &&
                early.return_esp == early_request.entry_esp &&
                early.stack_read_count == 4U &&
                early.return_eip == 0x0047864AU &&
                early.stack_reads[0U] == 0x00478640U &&
                early.stack_reads[1U] == early_request.entry_edi &&
                early.stack_reads[2U] == early_request.entry_esi &&
                early.stack_reads[3U] == early_request.entry_ebx,
            "the zero-return RET reads its own caller return address"
        );
    }

    {
        Fixture fixture;
        auto request = success_request();
        request.direction_flag = true;
        const auto result = fixture.run(request);
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameResourceStatus::
                        direction_flag_contract_typed_stop &&
                result.copied_dwords == 0U &&
                result.return_esi == request.actor_token + 0x02A0U &&
                result.return_edi == request.actor_token + 0x0CB8U &&
                result.return_ecx == 0x26U &&
                result.return_eip == 0x00478638U && result.direction_flag &&
                result.flags.carry,
            "set DF stops at the documented platform contract before REP MOVSD"
        );
    }

    {
        Fixture fixture;
        fixture.frame_provider.available = false;
        auto request = success_request();
        request.frame_provider_return_eax = 0U;
        const auto result = fixture.run(request);
        test.expect_true(
            result.status == LegacyBattleActorFrameResourceStatus::completed &&
                !result.frame_available && result.frame_token_committed &&
                fixture.frame_token == 0U && result.return_eax == 0U,
            "a null provider token is still committed before the normal return"
        );

        Fixture stopped_fixture;
        stopped_fixture.frame_token_writable = false;
        const auto stopped = stopped_fixture.run(success_request());
        test.expect_true(
            stopped.status ==
                    LegacyBattleActorFrameResourceStatus::
                        frame_token_write_typed_stop &&
                stopped.frame_lookup_calls == 1U &&
                stopped.return_eip == 0x00478663U &&
                !stopped.frame_token_committed &&
                stopped_fixture.frame_token == 0xCCCCCCCCU,
            "frame-token write fault preserves provider side effects and suppresses pops"
        );
    }
}
