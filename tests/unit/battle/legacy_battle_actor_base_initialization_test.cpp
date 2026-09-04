#include "openswd3/battle/legacy_battle_actor_base_initialization.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <span>

namespace {

using openswd3::battle::LegacyBattleActorBaseInitializationOwner;
using openswd3::battle::LegacyBattleActorBaseInitializationStatus;
using openswd3::compat::u8;
using openswd3::compat::u32;

[[nodiscard]] LegacyBattleActorBaseInitializationOwner seeded_owner() {
    LegacyBattleActorBaseInitializationOwner owner;
    owner.fields.linked_action_head_token = 0x11111111U;
    owner.fields.field_266c = 0x22222222U;
    owner.fields.field_26bc = 0x33333333U;
    owner.fields.field_29a2 = 0x4444U;
    owner.fields.field_2a0a = 0x5555U;
    owner.fields.field_2a68 = 0x6666U;
    owner.fields.field_2a6a = 0x7777U;
    owner.fields.field_2a94 = 0x88U;
    owner.resource_definition.fill(0xA5U);
    owner.resource_definition_description = {0xB1U, 0xB2U, 0xB3U};
    owner.action_text.fill(0xC5U);
    owner.action_execution.target_indices.fill(0xD5D5D5D5U);
    owner.action_execution.turn_countdown = -7;
    owner.action_execution.turn_threshold = 0x1111U;
    owner.action_execution.motion_word = 0x2222U;
    owner.action_execution.motion_aux_word = 0x3333U;
    owner.action_execution.summon_phase = 0x4444U;
    owner.action_execution.profile_variant_override = 0x5555U;
    owner.action_kind = 0x6666U;
    return owner;
}

[[nodiscard]] bool
is_initialized(const LegacyBattleActorBaseInitializationOwner& owner) {
    return std::ranges::all_of(
               owner.action_execution.target_indices,
               [](const u32 value) { return value == 0xFFFFFFFFU; }
           ) &&
        owner.fields.field_29a2 == 0xFFFFU &&
        owner.action_execution.turn_countdown == 15 &&
        owner.fields.field_266c == 1U &&
        owner.action_execution.turn_threshold == 0U &&
        owner.fields.field_2a0a == 4U && owner.fields.field_2a68 == 2U &&
        owner.fields.field_2a6a == 0x18U &&
        owner.action_execution.motion_word == 0U &&
        owner.action_execution.motion_aux_word == 0U &&
        owner.action_execution.summon_phase == 0U &&
        owner.action_execution.profile_variant_override == 0U &&
        owner.action_kind == 0U && owner.fields.field_2a94 == 0U &&
        owner.fields.field_26bc == 0x062B062BU &&
        owner.fields.linked_action_head_token == 0U &&
        std::ranges::all_of(
               owner.action_text, [](const u8 value) { return value == 0U; }
        ) &&
        std::ranges::all_of(
               owner.resource_definition,
               [](const u8 value) { return value == 0U; }
        ) &&
        owner.resource_definition_description.empty();
}

void test_complete_initialization(openswd3::test::Context& test) {
    auto owner = seeded_owner();
    const auto result = openswd3::battle::initialize_legacy_battle_actor_base(
        owner,
        {
            .object_token = 0x00521598U,
            .writable_bytes = 0x2B28U,
        }
    );

    test.expect_true(
        is_initialized(owner) &&
            result.status ==
                LegacyBattleActorBaseInitializationStatus::completed &&
            result.dword_writes == 53U && result.word_writes == 10U &&
            result.byte_writes == 1U && result.stopped_object_offset == 0U &&
            result.return_eax == 0x00521598U && result.return_ecx == 0U &&
            result.return_edx == 0x00521598U,
        "actor base initialization preserves all 64 physical writes and returns the actor token"
    );
}

void test_target_index_write_stops(openswd3::test::Context& test) {
    for (std::size_t accessible_indices = 0U; accessible_indices < 4U;
         ++accessible_indices) {
        auto owner = seeded_owner();
        const auto original = owner.action_execution.target_indices;
        const u32 writable_bytes =
            0x2A56U + static_cast<u32>(accessible_indices * sizeof(u32));
        const auto result =
            openswd3::battle::initialize_legacy_battle_actor_base(
                owner,
                {
                    .object_token = 0x10000000U,
                    .writable_bytes = writable_bytes,
                }
            );

        bool prefix_matches = true;
        for (std::size_t index = 0U;
             index < owner.action_execution.target_indices.size();
             ++index) {
            const u32 expected =
                index < accessible_indices ? 0xFFFFFFFFU : original[index];
            prefix_matches = prefix_matches &&
                owner.action_execution.target_indices[index] == expected;
        }
        test.expect_true(
            prefix_matches &&
                result.status ==
                    LegacyBattleActorBaseInitializationStatus::
                        object_write_typed_stop &&
                result.dword_writes == accessible_indices &&
                result.word_writes == 0U && result.byte_writes == 0U &&
                result.stopped_object_offset == writable_bytes &&
                result.return_eax == 0xFFFFFFFFU &&
                result.return_ecx == 0x10002A56U &&
                result.return_edx == 0x10000000U,
            "actor base initialization stops at each target-index write with the completed prefix"
        );
    }

    auto null_owner = seeded_owner();
    const auto null_result =
        openswd3::battle::initialize_legacy_battle_actor_base(
            null_owner,
            {
                .object_token = 0U,
                .writable_bytes = 0x2B28U,
            }
        );
    test.expect_true(
        null_result.status ==
                LegacyBattleActorBaseInitializationStatus::
                    object_write_typed_stop &&
            null_result.stopped_object_offset == 0x2A56U &&
            null_result.return_eax == 0xFFFFFFFFU &&
            null_result.return_ecx == 0x2A56U && null_result.return_edx == 0U,
        "a null actor stops at the first original target-index write"
    );
}

void test_intermediate_high_write_stops(openswd3::test::Context& test) {
    struct StopCase {
        u32 writable_bytes;
        u32 stopped_offset;
        u32 expected_word_writes;
    };
    constexpr std::array cases{
        StopCase{
            .writable_bytes = 0x2A66U,
            .stopped_offset = 0x2A68U,
            .expected_word_writes = 3U
        },
        StopCase{
            .writable_bytes = 0x2A6AU,
            .stopped_offset = 0x2A6AU,
            .expected_word_writes = 4U
        },
        StopCase{
            .writable_bytes = 0x2A6CU,
            .stopped_offset = 0x2A6CU,
            .expected_word_writes = 9U
        },
    };

    for (const auto stop_case : cases) {
        auto owner = seeded_owner();
        const auto result =
            openswd3::battle::initialize_legacy_battle_actor_base(
                owner,
                {
                    .object_token = 0x18000000U,
                    .writable_bytes = stop_case.writable_bytes,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleActorBaseInitializationStatus::
                        object_write_typed_stop &&
                result.dword_writes == 6U &&
                result.word_writes == stop_case.expected_word_writes &&
                result.byte_writes == 0U &&
                result.stopped_object_offset == stop_case.stopped_offset &&
                result.return_eax == 0U && result.return_ecx == 0x18002630U &&
                result.return_edx == 0x18000000U &&
                owner.fields.field_26bc == 0x33333333U &&
                owner.fields.linked_action_head_token == 0x11111111U,
            "actor base initialization stops at each reachable intermediate high-field write"
        );
    }
}

void test_late_direct_write_stop(openswd3::test::Context& test) {
    auto owner = seeded_owner();
    const auto result = openswd3::battle::initialize_legacy_battle_actor_base(
        owner,
        {
            .object_token = 0x20000000U,
            .writable_bytes = 0x2A94U,
        }
    );

    test.expect_true(
        result.status ==
                LegacyBattleActorBaseInitializationStatus::
                    object_write_typed_stop &&
            result.dword_writes == 6U && result.word_writes == 10U &&
            result.byte_writes == 0U &&
            result.stopped_object_offset == 0x2A94U &&
            result.return_eax == 0U && result.return_ecx == 0x20002630U &&
            result.return_edx == 0x20000000U &&
            owner.fields.field_2a94 == 0x88U &&
            owner.fields.field_26bc == 0x33333333U &&
            owner.fields.linked_action_head_token == 0x11111111U &&
            std::ranges::all_of(
                owner.action_text, [](const u8 value) { return value == 0xC5U; }
            ),
        "the 0x2A94 byte write stops after every preceding high actor-field write"
    );
}

void test_action_text_write_stops(openswd3::test::Context& test) {
    for (std::size_t accessible_dwords = 0U; accessible_dwords < 4U;
         ++accessible_dwords) {
        auto owner = seeded_owner();
        const auto original = owner.action_text;
        const auto result =
            openswd3::battle::initialize_legacy_battle_actor_base(
                owner.fields,
                owner.action_execution,
                owner.resource_definition,
                owner.resource_definition_description,
                std::span<u8>{owner.action_text}.first(
                    accessible_dwords * sizeof(u32)
                ),
                owner.action_kind,
                {
                    .object_token = 0x30000000U,
                    .writable_bytes = 0x2B28U,
                }
            );

        bool prefix_matches = true;
        for (std::size_t index = 0U; index < owner.action_text.size();
             ++index) {
            const u8 expected =
                index < accessible_dwords * sizeof(u32) ? 0U : original[index];
            prefix_matches =
                prefix_matches && owner.action_text[index] == expected;
        }
        test.expect_true(
            prefix_matches &&
                result.status ==
                    LegacyBattleActorBaseInitializationStatus::
                        object_write_typed_stop &&
                result.dword_writes == 8U + accessible_dwords &&
                result.word_writes == 10U && result.byte_writes == 1U &&
                result.stopped_object_offset ==
                    0x2630U + accessible_dwords * sizeof(u32) &&
                result.return_eax == 0U && result.return_ecx == 0x30002630U &&
                result.return_edx == 0x30000000U,
            "actor base initialization preserves each completed action-text dword before a typed stop"
        );
    }
}

void test_definition_write_stops(openswd3::test::Context& test) {
    for (std::size_t accessible_dwords = 0U; accessible_dwords < 0x29U;
         ++accessible_dwords) {
        auto owner = seeded_owner();
        const auto original = owner.resource_definition;
        const auto result =
            openswd3::battle::initialize_legacy_battle_actor_base(
                owner.fields,
                owner.action_execution,
                std::span<u8>{owner.resource_definition}.first(
                    accessible_dwords * sizeof(u32)
                ),
                owner.resource_definition_description,
                owner.action_text,
                owner.action_kind,
                {
                    .object_token = 0x40000000U,
                    .writable_bytes = 0x2B28U,
                }
            );

        bool prefix_matches = true;
        for (std::size_t index = 0U; index < owner.resource_definition.size();
             ++index) {
            const u8 expected =
                index < accessible_dwords * sizeof(u32) ? 0U : original[index];
            prefix_matches =
                prefix_matches && owner.resource_definition[index] == expected;
        }
        const u32 accessible = static_cast<u32>(accessible_dwords);
        test.expect_true(
            prefix_matches && !owner.resource_definition_description.empty() &&
                result.status ==
                    LegacyBattleActorBaseInitializationStatus::
                        object_write_typed_stop &&
                result.dword_writes == 12U + accessible &&
                result.word_writes == 10U && result.byte_writes == 1U &&
                result.stopped_object_offset == 0x10U + accessible * 4U &&
                result.return_eax == 0U &&
                result.return_ecx == 0x29U - accessible &&
                result.return_edx == 0x40000000U,
            "actor base initialization preserves every rep-stos dword prefix and the unreached description owner"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_complete_initialization(test);
    test_target_index_write_stops(test);
    test_intermediate_high_write_stops(test);
    test_late_direct_write_stop(test);
    test_action_text_write_stops(test);
    test_definition_write_stops(test);
    return test.exit_code();
}
