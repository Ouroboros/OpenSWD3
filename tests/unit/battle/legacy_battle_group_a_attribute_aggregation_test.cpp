#include "openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp"

#include "test.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleGroupAAttributeAggregationCallReply;
using openswd3::battle::LegacyBattleGroupAAttributeAggregationCallRequest;
using openswd3::battle::LegacyBattleGroupAAttributeAggregationPort;
using openswd3::battle::LegacyBattleGroupAAttributeSourceTable;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::world_map::LegacyWorldItemNode;

struct AttributePort final : LegacyBattleGroupAAttributeAggregationPort {
    [[nodiscard]] LegacyBattleGroupAAttributeAggregationCallReply
    invoke_group_a_attribute_aggregation(
        const LegacyBattleGroupAAttributeAggregationCallRequest& request
    ) override {
        requests.push_back(request);
        if (on_report) {
            on_report();
        }

        return {
            .eax = 0xA0000000U + static_cast<u32>(requests.size()),
            .ecx = 0xB0000000U + static_cast<u32>(requests.size()),
            .edx = 0xC0000000U + static_cast<u32>(requests.size()),
        };
    }

    openswd3::battle::LegacyBattleFixedObjectState fixed_objects;
    std::function<void()> on_report;
    std::vector<LegacyBattleGroupAAttributeAggregationCallRequest> requests;
};

void set_snapshot_byte(
    LegacyWorldItemNode& node, const std::size_t offset, const u8 value
) {
    node.definition_snapshot[offset] = value;
}

void set_snapshot_word(
    LegacyWorldItemNode& node, const std::size_t offset, const u16 value
) {
    set_snapshot_byte(node, offset, static_cast<u8>(value));
    set_snapshot_byte(node, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] u8 profile_byte(
    const openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) {
    return static_cast<u8>(profile[offset]);
}

[[nodiscard]] u16 profile_word(
    const openswd3::battle::LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) {
    return static_cast<u16>(profile_byte(profile, offset)) |
        static_cast<u16>(
               static_cast<u16>(profile_byte(profile, offset + 1U)) << 8U
        );
}

void set_actor_byte(
    std::array<u32, 14>& actor, const std::size_t offset, const u8 value
) {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    actor[index] =
        (actor[index] & ~(0xFFU << shift)) | (static_cast<u32>(value) << shift);
}

void set_actor_word(
    std::array<u32, 14>& actor, const std::size_t offset, const u16 value
) {
    set_actor_byte(actor, offset, static_cast<u8>(value));
    set_actor_byte(actor, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] u8
actor_byte(const std::array<u32, 14>& actor, const std::size_t offset) {
    return static_cast<u8>(
        actor[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] u16
actor_word(const std::array<u32, 14>& actor, const std::size_t offset) {
    return static_cast<u16>(actor_byte(actor, offset)) |
        static_cast<u16>(
               static_cast<u16>(actor_byte(actor, offset + 1U)) << 8U
        );
}

[[nodiscard]] LegacyBattleGroupAAttributeSourceTable
make_sources(const std::array<LegacyWorldItemNode, 16>& nodes) {
    LegacyBattleGroupAAttributeSourceTable sources{};
    for (u32 index = 0U; index < sources.size(); ++index) {
        sources[index] = {
            .record = &nodes[index],
            .record_token = nodes[index].legacy_token,
        };
    }
    return sources;
}

}  // namespace

void test_battle_group_a_attribute_aggregation(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupAAttributeAggregationCall;
    using openswd3::battle::LegacyBattleGroupAAttributeAggregationState;
    using openswd3::battle::LegacyBattleGroupAAttributeAggregationStatus;
    using openswd3::battle::LegacyBattleGroupAEmbeddedProfileApplicationStatus;
    using openswd3::battle::LegacyBattleGroupAConfigurationState;
    using openswd3::battle::LegacyBattleGroupAWorkspaceState;
    using openswd3::battle::aggregate_legacy_battle_group_a_attributes;

    for (const u32 row : {4U, 0xFFFFFFFFU}) {
        openswd3::world_map::LegacyWorldItemListState items;
        const auto sources =
            openswd3::battle::bind_legacy_battle_group_a_attribute_sources(
                items, row
            );
        LegacyBattleGroupAAttributeAggregationState state;
        state.embedded_profiles[0U].fill(std::byte{0xCC});
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        AttributePort port;
        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U + row * 0x40U,
            0U,
            port.fixed_objects,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::
                        source_record_typed_stop &&
                result.fault_source_index == 0U &&
                result.source_records_visited == 0U &&
                result.embedded_profile_dwords_zeroed == 82U &&
                state.embedded_profiles[0U][0U] == std::byte{},
            "unmapped role rows retain embedded-profile clearing and stop at the first item read"
        );
    }

    for (const u32 root : {0U, 0xDEADBEEFU}) {
        openswd3::world_map::LegacyWorldItemListState items;
        set_snapshot_word(items.role_item_lists[0U]->sentinel, 0x48U, 1U);
        set_snapshot_word(items.role_item_lists[0U]->sentinel, 0x24U, 2U);
        items.role_item_lists[3U]->legacy_head_token = root;
        const auto sources =
            openswd3::battle::bind_legacy_battle_group_a_attribute_sources(
                items, 0U
            );
        LegacyBattleGroupAAttributeAggregationState state;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        AttributePort port;
        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U,
            0U,
            port.fixed_objects,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::
                        source_record_typed_stop &&
                result.fault_source_index == 3U &&
                result.source_records_visited == 3U &&
                actor_word(configuration.actor_record, 0x26U) == 2U &&
                result.embedded_profile_apply_calls == 0U,
            "null or unmapped role roots stop at their slot after retaining earlier attribute additions"
        );
    }

    for (const bool remove_root : {false, true}) {
        openswd3::world_map::LegacyWorldItemListState items;
        auto& owner = items.role_item_lists[32U];
        owner->sentinel.item_id = 7U;
        set_snapshot_word(owner->sentinel, 0x24U, 1U);
        auto& replacement = owner->nodes.emplace_back();
        replacement.legacy_token = 0x73500000U;
        replacement.item_id = 9U;
        set_snapshot_word(replacement, 0x24U, 11U);
        const auto sources =
            openswd3::battle::bind_legacy_battle_group_a_attribute_sources(
                items, 2U
            );
        LegacyBattleGroupAAttributeAggregationState state;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        u16 effect_curve_index{};
        bool observed_prefix{};
        AttributePort port;
        port.on_report = [&] {
            observed_prefix = effect_curve_index == 7U &&
                workspace.tail_words[5U] == 7U &&
                profile_word(state.primary_profile, 0x24U) == 1U;
            effect_curve_index = 99U;
            if (remove_root) {
                owner.reset();
            } else {
                owner->legacy_head_token = replacement.legacy_token;
            }
        };
        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8B50U,
            0U,
            port.fixed_objects,
            port,
            &effect_curve_index
        );
        test.expect_true(
            result.status ==
                    (remove_root
                         ? LegacyBattleGroupAAttributeAggregationStatus::
                               source_record_typed_stop
                         : LegacyBattleGroupAAttributeAggregationStatus::
                               completed) &&
                observed_prefix && effect_curve_index == 99U &&
                actor_word(configuration.actor_record, 0x26U) ==
                    (remove_root ? 0U : 11U) &&
                result.embedded_profile_apply_calls == (remove_root ? 0U : 2U),
            "borrowed role roots are resolved after diagnostics and published action fields are not copied back over callback changes"
        );
        if (!remove_root) {
            port.on_report = {};
            const auto repeated = aggregate_legacy_battle_group_a_attributes(
                &state,
                workspace,
                configuration,
                &sources,
                0x005029D0U,
                0x004C8B50U,
                0U,
                port.fixed_objects,
                port,
                &effect_curve_index
            );
            test.expect_true(
                repeated.status ==
                        LegacyBattleGroupAAttributeAggregationStatus::
                            completed &&
                    profile_word(state.primary_profile, 0x24U) == 11U &&
                    actor_word(configuration.actor_record, 0x26U) == 22U &&
                    workspace.tail_words[5U] == 9U && effect_curve_index == 9U,
                "repeated aggregation resolves the current root without caching an old item record"
            );
        }
    }

    {
        std::array<LegacyWorldItemNode, 16> nodes{};
        nodes[0U].item_id = 7U;
        set_snapshot_word(nodes[0U], 0x24U, 1U);
        auto sources = make_sources(nodes);
        LegacyWorldItemNode replacement;
        replacement.item_id = 0x039DU;
        replacement.legacy_token = 0x73000000U;
        set_snapshot_word(replacement, 0x24U, 9U);
        LegacyBattleGroupAAttributeAggregationState state;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        AttributePort port;
        port.on_report = [&] {
            sources[0U] = {
                .record = &replacement,
                .record_token = replacement.legacy_token,
            };
        };
        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U,
            0U,
            port.fixed_objects,
            port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::completed &&
                result.diagnostic_calls == 1U &&
                profile_word(state.primary_profile, 0x24U) == 1U &&
                workspace.tail_words[5U] == 7U &&
                actor_word(configuration.actor_record, 0x26U) == 9U &&
                workspace.special_item_latch == 1U,
            "attribute diagnostic preserves the copied prefix but subsequent additions read the replacement source"
        );
    }

    {
        std::array<LegacyWorldItemNode, 16> nodes{};
        for (u32 index = 0U; index < nodes.size(); ++index) {
            auto& node = nodes[index];
            node.item_id = static_cast<u16>(index + 1U);
            node.legacy_token = 0x73000000U + index * 0x100U;
            node.legacy_description_token = 0x74000000U + index;
            set_snapshot_word(node, 0x24U, 1U);
            set_snapshot_word(node, 0x26U, 2U);
            set_snapshot_word(node, 0x28U, 7U);
            set_snapshot_word(node, 0x2AU, 8U);
            set_snapshot_word(node, 0x2CU, 3U);
            set_snapshot_word(node, 0x2EU, 4U);
            set_snapshot_word(node, 0x30U, 5U);
            set_snapshot_word(node, 0x32U, 6U);
            set_snapshot_word(node, 0x34U, 9U);
            set_snapshot_word(node, 0x36U, 10U);
            set_snapshot_word(node, 0x38U, 11U);
            for (std::size_t byte_index = 0U; byte_index < 9U; ++byte_index) {
                set_snapshot_byte(node, 0x92U + byte_index, 1U);
            }
        }
        set_snapshot_word(nodes[0U], 0x48U, 1U);
        set_snapshot_word(nodes[6U], 0x3CU, 1U);
        nodes[7U].item_id = 0x0111U;
        nodes[8U].item_id = openswd3::world_map::kLegacyItemSentinelId;
        nodes[15U].item_id = 0x039DU;
        nodes[15U].legacy_token = 0x73AB0F00U;
        set_snapshot_word(nodes[7U], 0x48U, 50U);
        set_snapshot_word(nodes[8U], 0x48U, 53U);
        set_snapshot_word(nodes[7U], 0x50U, 0xAAAAU);
        set_snapshot_word(nodes[8U], 0x50U, 0xBCDEU);

        auto sources = make_sources(nodes);
        LegacyBattleGroupAAttributeAggregationState state;
        for (auto& profile : state.embedded_profiles) {
            profile.fill(std::byte{0xCC});
        }
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        set_actor_word(configuration.actor_record, 0x26U, 0xFFF0U);
        set_actor_word(configuration.actor_record, 0x28U, 0xFFF0U);
        set_actor_word(configuration.actor_record, 0x16U, 300U);
        set_actor_word(configuration.actor_record, 0x14U, 400U);
        set_actor_word(configuration.actor_record, 0x18U, 500U);
        set_actor_word(configuration.actor_record, 0x1EU, 600U);
        set_actor_word(configuration.actor_record, 0x10U, 0xFFF0U);
        set_actor_word(configuration.actor_record, 0x12U, 0xFFF0U);
        for (std::size_t offset = 0x2DU; offset <= 0x35U; ++offset) {
            set_actor_byte(configuration.actor_record, offset, 250U);
        }
        AttributePort port;

        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U,
            0x12345678U,
            port.fixed_objects,
            port
        );

        bool name_bytes_match = true;
        for (std::size_t offset = 0x2DU; offset <= 0x35U; ++offset) {
            name_bytes_match = name_bytes_match &&
                actor_byte(configuration.actor_record, offset) == 6U;
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::completed &&
                result.embedded_profile_dwords_zeroed == 82U &&
                result.primary_profile_dwords_copied == 41U &&
                result.embedded_profile_dwords_copied == 82U &&
                result.source_records_visited == 16U &&
                result.actor_word_additions == 160U &&
                result.actor_byte_additions == 108U &&
                result.early_bonus_additions == 18U &&
                result.embedded_profile_apply_calls == 2U &&
                result.embedded_profile_applications[0U].status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                result.embedded_profile_applications[1U].status ==
                    LegacyBattleGroupAEmbeddedProfileApplicationStatus::
                        completed &&
                state.embedded_profile_application.status_bits == 0x05U &&
                result.diagnostic_calls == 0U &&
                result.special_item_latch_writes == 1U &&
                profile_word(state.embedded_profiles[0U], 0x50U) == 0x0111U &&
                profile_word(state.embedded_profiles[1U], 0x50U) == 0xBCDEU &&
                profile_byte(state.primary_profile, 0xA0U) == 0U &&
                profile_byte(state.primary_profile, 0xA1U) == 0U &&
                profile_byte(state.primary_profile, 0xA2U) == 0U &&
                profile_byte(state.primary_profile, 0xA3U) == 0x74U &&
                actor_word(configuration.actor_record, 0x26U) == 0x0070U &&
                actor_word(configuration.actor_record, 0x28U) == 0x0090U &&
                actor_word(configuration.actor_record, 0x16U) == 348U &&
                actor_word(configuration.actor_record, 0x14U) == 464U &&
                actor_word(configuration.actor_record, 0x18U) == 580U &&
                actor_word(configuration.actor_record, 0x1EU) == 696U &&
                actor_word(configuration.actor_record, 0x10U) == 0x0060U &&
                actor_word(configuration.actor_record, 0x12U) == 0x0070U &&
                name_bytes_match && workspace.tail_words[5U] == 1U &&
                workspace.tail_words[7U] == 54U &&
                workspace.tail_words[8U] == 60U &&
                workspace.tail_words[9U] == 66U &&
                workspace.special_item_latch == 1U && port.requests.empty() &&
                result.return_eax == 0x73AB0F00U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x005029D0U,
            "group-A attribute aggregation preserves all sixteen source classes, low-width additions, embedded records, and final registers"
        );
    }

    {
        LegacyBattleGroupAAttributeAggregationState state;
        state.primary_profile.fill(std::byte{0xAA});
        for (auto& profile : state.embedded_profiles) {
            profile.fill(std::byte{0xBB});
        }
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[5U] = 0x1234U;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x72000000U,
        };
        configuration.actor_record.fill(0xCCCCCCCCU);
        AttributePort port;

        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            nullptr,
            0x005029D0U,
            0U,
            0U,
            port.fixed_objects,
            port
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::completed &&
                result.embedded_profile_dwords_zeroed == 82U &&
                result.source_records_visited == 0U &&
                state.primary_profile[0U] == std::byte{0xAA} &&
                state.embedded_profiles[0U][0U] == std::byte{} &&
                state.embedded_profiles[1U][0U] == std::byte{} &&
                workspace.tail_words[5U] == 0x1234U &&
                configuration.actor_record[0U] == 0xCCCCCCCCU &&
                port.requests.empty() && result.return_eax == 0U &&
                result.return_ecx == 0x005029D0U &&
                result.return_edx == 0x005029D0U,
            "null source table clears only the two embedded profiles and completes sixteen empty loop iterations"
        );
    }

    {
        std::array<LegacyWorldItemNode, 16> nodes{};
        for (u32 index = 0U; index < nodes.size(); ++index) {
            nodes[index].item_id = static_cast<u16>(index + 2U);
            nodes[index].legacy_token = 0x75000000U + index * 0x100U;
        }
        auto sources = make_sources(nodes);
        LegacyBattleGroupAAttributeAggregationState state;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x76000000U,
        };
        AttributePort port;

        const auto result = aggregate_legacy_battle_group_a_attributes(
            &state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U,
            0x12345678U,
            port.fixed_objects,
            port
        );

        test.expect_true(
            result.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::completed &&
                result.diagnostic_calls == 1U &&
                result.embedded_profile_apply_calls == 2U &&
                port.requests.size() == 1U &&
                port.requests[0U].call ==
                    LegacyBattleGroupAAttributeAggregationCall::
                        report_missing_primary_attribute &&
                port.requests[0U].window_token == 0x12345678U &&
                port.requests[0U].item_id == 2U &&
                port.requests[0U].diagnostic_text_token == 0x004A7C94U &&
                port.requests[0U].diagnostic_source_token == 0x004A7C44U &&
                port.requests[0U].diagnostic_source_line == 0x182U,
            "zero primary profile gate reports the fixed manrole diagnostic before attribute additions"
        );
    }

    {
        std::array<LegacyWorldItemNode, 16> nodes{};
        for (u32 index = 0U; index < nodes.size(); ++index) {
            nodes[index].legacy_token = 0x77000000U + index * 0x100U;
            set_snapshot_word(nodes[index], 0x48U, 1U);
        }
        auto sources = make_sources(nodes);
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration{
            .actor_record_token = 0x78000000U,
        };
        AttributePort actor_state_port;
        const auto actor_state_stop =
            aggregate_legacy_battle_group_a_attributes(
                nullptr,
                workspace,
                configuration,
                &sources,
                0x005029D0U,
                0x004C8AD0U,
                0U,
                actor_state_port.fixed_objects,
                actor_state_port
            );

        LegacyBattleGroupAAttributeAggregationState source_state;
        for (auto& profile : source_state.embedded_profiles) {
            profile.fill(std::byte{0xCC});
        }
        sources[3U].record = nullptr;
        AttributePort source_port;
        const auto source_stop = aggregate_legacy_battle_group_a_attributes(
            &source_state,
            workspace,
            configuration,
            &sources,
            0x005029D0U,
            0x004C8AD0U,
            0U,
            source_port.fixed_objects,
            source_port
        );

        auto actor_sources = make_sources(nodes);
        LegacyBattleGroupAAttributeAggregationState actor_record_state;
        LegacyBattleGroupAConfigurationState actor_record_configuration{
            .actor_record_token = 0U,
        };
        actor_record_configuration.actor_record.fill(0xDDDDDDDDU);
        AttributePort actor_record_port;
        const auto actor_record_stop =
            aggregate_legacy_battle_group_a_attributes(
                &actor_record_state,
                workspace,
                actor_record_configuration,
                &actor_sources,
                0x005029D0U,
                0x004C8AD0U,
                0U,
                actor_record_port.fixed_objects,
                actor_record_port
            );

        test.expect_true(
            actor_state_stop.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::
                        actor_state_typed_stop &&
                actor_state_stop.embedded_profile_dwords_zeroed == 0U &&
                source_stop.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::
                        source_record_typed_stop &&
                source_stop.fault_source_index == 3U &&
                source_stop.source_records_visited == 3U &&
                source_state.embedded_profiles[0U][0U] == std::byte{} &&
                actor_record_stop.status ==
                    LegacyBattleGroupAAttributeAggregationStatus::
                        actor_record_typed_stop &&
                actor_record_stop.fault_source_index == 0U &&
                actor_record_stop.primary_profile_dwords_copied == 41U &&
                workspace.tail_words[5U] == nodes[0U].item_id &&
                actor_record_configuration.actor_record[0U] == 0xDDDDDDDDU,
            "group-A attribute aggregation stops at the first missing actor, source record, or actor base access with exact prefixes"
        );
    }
}
