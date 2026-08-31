#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_group_b_action_item_selection.hpp"
#include "test.hpp"

#include <array>
#include <deque>
#include <memory>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class RandomPort final
    : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(const u32 bound) override {
        bounds.push_back(bound);
        const u32 value = values.front();
        values.pop_front();
        if (bounds.size() == mutate_on_call && actor_to_mutate != nullptr) {
            actor_to_mutate->resource_token = replacement_resource_token;
            actor_to_mutate->resource_bytes = replacement_resource_bytes;
        }

        return value;
    }

    void push(const u32 value) {
        values.push_back(value);
    }

    std::deque<u32> values;
    std::vector<u32> bounds;
    openswd3::battle::LegacyBattleActorGroupBElementState* actor_to_mutate{};
    std::size_t mutate_on_call{};
    u32 replacement_resource_token{};
    std::array<u8, 0xA4> replacement_resource_bytes{};
};

class DefinitionPort final
    : public openswd3::battle::LegacyBattleGroupBActionItemSelectionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const openswd3::battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        if (force_allocation_stop &&
            request.call ==
                openswd3::battle::LegacyBattleMonDatabaseCall::
                    allocate_stream) {
            allocation_succeeds = false;
        }
        return openswd3::test::LegacyBattleMonDatabaseFixture::
            invoke_legacy_battle_mon_database(request, destination);
    }

protected:
    [[nodiscard]] std::optional<bool> prepare_definition_record(
        const std::span<u8> destination, const u32
    ) noexcept override {
        if (prepared_definition == nullptr) {
            return false;
        }
        std::copy(
            prepared_definition->cbegin(),
            prepared_definition->cend(),
            destination.begin()
        );
        return true;
    }

public:
    bool force_allocation_stop{};
    std::shared_ptr<const std::array<u8, 0xA4>> prepared_definition;
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_dword(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] openswd3::battle::LegacyBattleActorGroupBElementState
prepared_actor() {
    openswd3::battle::LegacyBattleActorGroupBElementState actor;
    actor.resource_token = 0x73001234U;
    actor.action_execution.retreat_ready_flags = 0x00A0U;
    write_word(actor.resource_bytes, 0x54U, 0x0080U);
    write_word(actor.resource_bytes, 0x66U, 0x1111U);
    write_word(actor.resource_bytes, 0x6AU, 0x2222U);
    write_word(actor.resource_bytes, 0x6EU, 0x3333U);
    return actor;
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
definition(const u32 flags, const u16 item_id) {
    auto bytes = std::make_shared<std::array<u8, 0xA4>>();
    write_dword(*bytes, 0x20U, flags);
    write_word(*bytes, 0x48U, item_id);
    return bytes;
}

}  // namespace

void test_battle_group_b_action_item_selection(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionItemSelectionStatus;

    {
        RandomPort random;
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                nullptr,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .entry_eax = 0xAAAAAAAAU,
                    .entry_ecx = 0xBBBBBBBBU,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 1U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0xCCCCCCCCU && result.random_calls == 0U,
            "action item selection stops at the first actor flag access"
        );
    }

    {
        auto actor = prepared_actor();
        actor.action_execution.retreat_ready_flags = 0x0080U;
        RandomPort random;
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {.actor_token = 0x00525508U, .entry_edx = 0xCCCCCCCCU}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0xCCCCCCCCU && result.random_calls == 0U,
            "cleared action-item gate returns zero without consuming random"
        );
    }

    {
        constexpr std::array<std::pair<u32, u32>, 7> kCases{{
            {0x0031U, 1U},
            {0x0032U, 2U},
            {0x004FU, 2U},
            {0x0050U, 3U},
            {0x0064U, 3U},
            {0x0065U, 1U},
            {0x10028U, 1U},
        }};
        bool matched = true;
        for (const auto [value, bound] : kCases) {
            auto actor = prepared_actor();
            RandomPort random;
            random.push(3U);
            DefinitionPort port;
            const auto result =
                openswd3::battle::select_legacy_battle_group_b_action_item(
                    &actor,
                    random,
                    port,
                    {.actor_token = 0x00525508U, .resource_value = value}
                );
            matched = matched && result.initial_random_bound == bound &&
                result.return_eax == 0U &&
                random.bounds == std::vector<u32>{bound};
        }
        test.expect_true(
            matched,
            "resource-value ranges select bounds one two and three by unsigned low word"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(0xABCDFFFFU);
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor, random, port, {.actor_token = 0x00525508U}
            );
        test.expect_true(
            result.selection_value == 0xFFFFFFFFU &&
                result.return_eax == 0xFFFF0000U &&
                result.return_edx == 0xABCDFFFFU && !result.return_ecx_known,
            "low-word all-ones preserves the increment-mask-decrement underflow"
        );
    }

    {
        auto actor = prepared_actor();
        actor.resource_token = 0U;
        RandomPort random;
        random.push(0U);
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor, random, port, {.actor_token = 0x00525508U}
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::
                        resource_read_typed_stop &&
                result.return_eax == 0U && result.return_edx == 0U &&
                result.random_calls == 1U,
            "first resource read happens after the selection draw"
        );
    }

    {
        auto actor = prepared_actor();
        write_word(actor.resource_bytes, 0x66U, 0U);
        RandomPort random;
        random.push(0xAAAA0000U);
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor, random, port, {.actor_token = 0x00525508U}
            );
        test.expect_true(
            result.return_eax == 0x73000000U && result.random_calls == 1U &&
                actor.action_execution.retreat_ready_flags == 0x00A0U,
            "zero selected definition preserves the resource-token high word"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(0U);
        random.push(0xABCD003CU);
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .profile_argument = 0x7FU,
                }
            );
        test.expect_true(
            result.decision_threshold == 60U &&
                result.return_eax == 0xABCD0000U && result.random_calls == 2U &&
                actor.action_execution.retreat_ready_flags == 0x00A0U,
            "profile below the resource word uses sixty and rejects equality"
        );
    }

    {
        auto actor = prepared_actor();
        write_word(actor.resource_bytes, 0x54U, 0x007AU);
        RandomPort random;
        random.push(0U);
        random.push(0xABCD0059U);
        DefinitionPort port;
        port.prepared_definition = definition(0x89ABCDEFU, 0x4567U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .profile_argument = 0xFFFF007AU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::completed &&
                result.decision_threshold == 90U && result.item_id == 0x4567U &&
                result.return_eax == 0x89AB4567U &&
                actor.action_execution.retreat_ready_flags == 0x0080U,
            "selection zero loads its definition clears bit five and returns the item"
        );
        test.expect_true(
            port.open_calls == 1U && port.seek_calls == 3U &&
                port.read_calls == 3U && port.release_calls == 1U,
            "selection zero preserves the low-word definition loader ABI"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(1U);
        random.push(0xDCBA003BU);
        DefinitionPort port;
        port.prepared_definition = definition(0x08000000U, 0x5678U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .resource_value = 0x0032U,
                    .profile_argument = 0x7AU,
                }
            );
        test.expect_true(
            result.item_id == 0x5678U && result.return_eax == 0x08005678U &&
                result.definition_argument == 0x2222U &&
                result.definition_destination_token == 0x00525518U &&
                port.requested_definition_ids == std::vector<u32>{0x2222U} &&
                result.return_ecx_known,
            "selection one preserves its distinct resource and destination registers"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(0U);
        random.push(0xABCD0000U);
        random.actor_to_mutate = &actor;
        random.mutate_on_call = 2U;
        random.replacement_resource_token = 0x74005678U;
        write_word(random.replacement_resource_bytes, 0x66U, 0x4444U);
        DefinitionPort port;
        port.prepared_definition = definition(0x08000000U, 0x4567U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .profile_argument = 0x7AU,
                }
            );
        test.expect_true(
            result.item_id == 0x4567U &&
                result.selected_definition == 0x4444U &&
                result.definition_argument == 0xABCD4444U &&
                port.requested_definition_ids == std::vector<u32>{0x4444U},
            "selection zero re-reads the resource token and definition after its decision draw"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(2U);
        random.push(0U);
        DefinitionPort port;
        port.force_allocation_stop = true;
        port.prepared_definition = definition(0x08000000U, 0x6789U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .resource_value = 0x0050U,
                    .profile_argument = 0x7AU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::
                        definition_load_typed_stop &&
                result.definition_argument == 0x00003333U &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == 0x11223344U &&
                actor.action_composition.resource_definition[0x48U] == 0U &&
                actor.action_execution.retreat_ready_flags == 0x00A0U,
            "definition-loader allocation stop preserves the cleared output and blocks the flag clear"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(1U);
        random.push(0U);
        random.actor_to_mutate = &actor;
        random.mutate_on_call = 2U;
        random.replacement_resource_token = 0U;
        DefinitionPort port;
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor,
                random,
                port,
                {
                    .actor_token = 0x00525508U,
                    .resource_value = 0x0032U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSelectionStatus::
                        resource_reread_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0U &&
                result.return_edx == 0x00525518U && !result.return_ecx_known,
            "selection one stops at its post-random resource re-read"
        );
    }

    {
        auto actor = prepared_actor();
        RandomPort random;
        random.push(0U);
        random.push(0U);
        DefinitionPort port;
        port.prepared_definition = definition(0x12345678U, 0x9999U);
        const auto result =
            openswd3::battle::select_legacy_battle_group_b_action_item(
                &actor, random, port, {.actor_token = 0x00525508U}
            );
        test.expect_true(
            result.item_id == 0U && result.return_eax == 0x12340000U &&
                actor.action_execution.retreat_ready_flags == 0x0080U,
            "definition without bit twenty-seven clears the low return word"
        );
    }
}
