#include "openswd3/battle/legacy_battle_group_a_attribute_aggregation.hpp"

#include <cstddef>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u16;
using compat::u32;
using world_map::LegacyWorldItemNode;

[[nodiscard]] constexpr u8 profile_byte(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u8>(profile[offset]);
}

[[nodiscard]] constexpr u16 profile_word(
    const LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset
) noexcept {
    return static_cast<u16>(profile_byte(profile, offset)) |
        static_cast<u16>(
               static_cast<u16>(profile_byte(profile, offset + 1U)) << 8U
        );
}

void set_profile_byte(
    LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u8 value
) noexcept {
    profile[offset] = static_cast<std::byte>(value);
}

void set_profile_word(
    LegacyBattleGroupASummonProfileRecord& profile,
    const std::size_t offset,
    const u16 value
) noexcept {
    set_profile_byte(profile, offset, static_cast<u8>(value));
    set_profile_byte(profile, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] LegacyBattleGroupASummonProfileRecord
pack_profile(const LegacyWorldItemNode& source) noexcept {
    LegacyBattleGroupASummonProfileRecord profile{};
    for (std::size_t index = 0U; index < source.definition_snapshot.size();
         ++index) {
        set_profile_byte(profile, index, source.definition_snapshot[index]);
    }
    for (std::size_t index = 0U;
         index < sizeof(source.legacy_description_token);
         ++index) {
        set_profile_byte(
            profile,
            source.definition_snapshot.size() + index,
            static_cast<u8>(source.legacy_description_token >> (index * 8U))
        );
    }
    return profile;
}

[[nodiscard]] constexpr u8 actor_byte(
    const std::array<u32, 14>& actor, const std::size_t offset
) noexcept {
    return static_cast<u8>(
        actor[offset / 4U] >> static_cast<u32>((offset & 3U) * 8U)
    );
}

[[nodiscard]] constexpr u16 actor_word(
    const std::array<u32, 14>& actor, const std::size_t offset
) noexcept {
    return static_cast<u16>(actor_byte(actor, offset)) |
        static_cast<u16>(
               static_cast<u16>(actor_byte(actor, offset + 1U)) << 8U
        );
}

void set_actor_byte(
    std::array<u32, 14>& actor, const std::size_t offset, const u8 value
) noexcept {
    const std::size_t index = offset / 4U;
    const u32 shift = static_cast<u32>((offset & 3U) * 8U);
    actor[index] =
        (actor[index] & ~(0xFFU << shift)) | (static_cast<u32>(value) << shift);
}

void set_actor_word(
    std::array<u32, 14>& actor, const std::size_t offset, const u16 value
) noexcept {
    set_actor_byte(actor, offset, static_cast<u8>(value));
    set_actor_byte(actor, offset + 1U, static_cast<u8>(value >> 8U));
}

[[nodiscard]] bool add_actor_word(
    LegacyBattleGroupAConfigurationState& configuration,
    LegacyBattleGroupAAttributeAggregationResult& result,
    const std::size_t offset,
    const u16 value
) noexcept {
    if (configuration.actor_record_token == 0U) {
        result.status = LegacyBattleGroupAAttributeAggregationStatus::
            actor_record_typed_stop;
        return false;
    }
    set_actor_word(
        configuration.actor_record,
        offset,
        static_cast<u16>(actor_word(configuration.actor_record, offset) + value)
    );
    ++result.actor_word_additions;
    return true;
}

[[nodiscard]] bool add_actor_byte(
    LegacyBattleGroupAConfigurationState& configuration,
    LegacyBattleGroupAAttributeAggregationResult& result,
    const std::size_t offset,
    const u8 value
) noexcept {
    if (configuration.actor_record_token == 0U) {
        result.status = LegacyBattleGroupAAttributeAggregationStatus::
            actor_record_typed_stop;
        return false;
    }
    set_actor_byte(
        configuration.actor_record,
        offset,
        static_cast<u8>(actor_byte(configuration.actor_record, offset) + value)
    );
    ++result.actor_byte_additions;
    return true;
}

void add_early_bonus(
    u16& destination,
    const u16 value,
    LegacyBattleGroupAAttributeAggregationResult& result
) noexcept {
    if (value == 0U) {
        return;
    }
    destination = static_cast<u16>(destination + value);
    ++result.early_bonus_additions;
}

}  // namespace

LegacyBattleGroupAAttributeAggregationResult
aggregate_legacy_battle_group_a_attributes(
    LegacyBattleGroupAAttributeAggregationState* state,
    LegacyBattleGroupAWorkspaceState& workspace,
    LegacyBattleGroupAConfigurationState& configuration,
    const LegacyBattleGroupAAttributeSourceTable* sources,
    const u32 actor_token,
    const u32 source_table_token,
    const u32 window_token,
    LegacyBattleGroupAAttributeAggregationPort& port
) {
    LegacyBattleGroupAAttributeAggregationResult result;
    result.return_edx = actor_token;
    static_cast<void>(source_table_token);

    if (state == nullptr) {
        result.status = LegacyBattleGroupAAttributeAggregationStatus::
            actor_state_typed_stop;
        return result;
    }

    for (auto& profile : state->embedded_profiles) {
        profile.fill(std::byte{});
        result.embedded_profile_dwords_zeroed +=
            static_cast<u32>(profile.size() / sizeof(u32));
    }

    if (sources == nullptr) {
        return result;
    }

    for (u32 source_index = 0U;
         source_index < kLegacyBattleGroupAAttributeSourceCount;
         ++source_index) {
        const auto& source = (*sources)[source_index];
        result.fault_source_index = source_index;
        if (source.record == nullptr) {
            result.status = LegacyBattleGroupAAttributeAggregationStatus::
                source_record_typed_stop;
            return result;
        }
        ++result.source_records_visited;

        const auto profile = pack_profile(*source.record);
        const u32 source_token = source.record_token;
        if (source_index == 0U) {
            state->primary_profile = profile;
            result.primary_profile_dwords_copied =
                static_cast<u32>(profile.size() / sizeof(u32));
            workspace.tail_words[5U] = source.record->item_id;
            if (profile_word(state->primary_profile, 0x48U) == 0U) {
                static_cast<void>(port.invoke_group_a_attribute_aggregation({
                    .call = LegacyBattleGroupAAttributeAggregationCall::
                        report_missing_primary_attribute,
                    .actor_token = actor_token,
                    .source_record_token = source_token,
                    .item_id = source.record->item_id,
                    .window_token = window_token,
                    .diagnostic_text_token =
                        kLegacyBattleGroupAAttributeDiagnosticTextToken,
                    .diagnostic_source_token =
                        kLegacyBattleGroupAAttributeDiagnosticSourceToken,
                    .diagnostic_source_line =
                        kLegacyBattleGroupAAttributeDiagnosticSourceLine,
                }));
                ++result.port_calls;
                ++result.diagnostic_calls;
            }
        }

        const u16 value_30 = profile_word(profile, 0x24U);
        result.return_eax = configuration.actor_record_token;
        result.return_ecx = (source_token & 0xFFFF0000U) | value_30;
        if (!add_actor_word(configuration, result, 0x26U, value_30)) {
            return result;
        }
        if (!add_actor_word(
                configuration, result, 0x28U, profile_word(profile, 0x26U)
            ) ||
            !add_actor_word(
                configuration, result, 0x16U, profile_word(profile, 0x2CU)
            ) ||
            !add_actor_word(
                configuration, result, 0x14U, profile_word(profile, 0x2EU)
            ) ||
            !add_actor_word(
                configuration, result, 0x18U, profile_word(profile, 0x30U)
            ) ||
            !add_actor_word(
                configuration, result, 0x1EU, profile_word(profile, 0x32U)
            )) {
            return result;
        }

        const u16 value_34 = profile_word(profile, 0x28U);
        if (value_34 != 0U &&
            (!add_actor_word(configuration, result, 0x10U, value_34) ||
             !add_actor_word(configuration, result, 0x26U, value_34))) {
            return result;
        }

        const u16 value_36 = profile_word(profile, 0x2AU);
        result.return_ecx = (source_token & 0xFFFF0000U) | value_36;
        if (value_36 != 0U &&
            (!add_actor_word(configuration, result, 0x12U, value_36) ||
             !add_actor_word(configuration, result, 0x28U, value_36))) {
            return result;
        }

        if (source_index == 7U || source_index == 8U) {
            const u32 embedded_index = source_index - 7U;
            auto& embedded = state->embedded_profiles[embedded_index];
            embedded = profile;
            result.embedded_profile_dwords_copied +=
                static_cast<u32>(embedded.size() / sizeof(u32));
            if (source.record->item_id != world_map::kLegacyItemSentinelId) {
                set_profile_word(embedded, 0x50U, source.record->item_id);
            }
            const auto reply = port.invoke_group_a_attribute_aggregation({
                .call = LegacyBattleGroupAAttributeAggregationCall::
                    apply_embedded_profile,
                .actor_token = actor_token,
                .source_record_token = source_token,
                .embedded_profile_token =
                    actor_token + (embedded_index == 0U ? 0x158U : 0x1FCU),
                .embedded_profile_index = embedded_index,
                .item_id = source.record->item_id,
                .embedded_profile = embedded,
            });
            ++result.port_calls;
            ++result.embedded_profile_apply_calls;
            result.return_eax = reply.eax;
            result.return_ecx = reply.ecx;
            result.return_edx = actor_token;
            continue;
        }

        if (source_index == 9U || source_index == 10U) {
            result.return_eax = source_token;
            result.return_edx = actor_token;
            continue;
        }

        for (std::size_t byte_index = 0U; byte_index < 9U; ++byte_index) {
            if (!add_actor_byte(
                    configuration,
                    result,
                    0x2DU + byte_index,
                    profile_byte(profile, 0x92U + byte_index)
                )) {
                return result;
            }
        }
        result.return_ecx = (source_token & 0xFFFF0000U) |
            (static_cast<u32>(value_36) & 0x0000FF00U) |
            profile_byte(profile, 0x9AU);

        if (source_index < 7U && profile_word(profile, 0x3CU) == 0U) {
            add_early_bonus(
                workspace.tail_words[7U], profile_word(profile, 0x34U), result
            );
            add_early_bonus(
                workspace.tail_words[8U], profile_word(profile, 0x36U), result
            );
            add_early_bonus(
                workspace.tail_words[9U], profile_word(profile, 0x38U), result
            );
        }

        result.return_eax = source_token;
        if (source.record->item_id == 0x039DU) {
            workspace.special_item_latch = 1U;
            ++result.special_item_latch_writes;
        }
        result.return_edx = actor_token;
    }

    result.fault_source_index =
        static_cast<u32>(kLegacyBattleGroupAAttributeSourceCount);
    return result;
}

}  // namespace openswd3::battle
