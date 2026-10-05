#include "openswd3/world_map/legacy_world_save_restore.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <type_traits>

namespace openswd3::world_map {
namespace {

[[nodiscard]] compat::u16 read_half_word(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        (static_cast<compat::u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] compat::u32 read_word(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u32>(bytes[offset]) |
        (static_cast<compat::u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<compat::u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<compat::u32>(bytes[offset + 3U]) << 24U);
}

}  // namespace

LegacySaveU16PrefixResult
read_legacy_save_u16_prefix(const resource_io::LegacySaveContainer& save) {
    constexpr std::size_t kMonsterIdsOffset = 0x1E4U;
    constexpr std::size_t kListsOffset = kMonsterIdsOffset + 64U * 2U;
    const auto& source = save.blocks[2U].bytes;
    LegacySaveU16PrefixResult result;
    if (source.size() < kListsOffset) {
        return result;
    }
    for (std::size_t index = 0U; index < result.prefix.monster_ids.size();
         ++index) {
        result.prefix.monster_ids[index] =
            read_half_word(source, kMonsterIdsOffset + index * 2U);
    }

    std::size_t offset = kListsOffset;
    const auto next_word = [&source, &offset](compat::u16& value) {
        if (offset > source.size() || 2U > source.size() - offset) {
            return false;
        }
        value = read_half_word(source, offset);
        offset += 2U;
        return true;
    };
    for (auto& list : result.prefix.party_item_ids) {
        for (;;) {
            compat::u16 item_id{};
            if (!next_word(item_id)) {
                return result;
            }
            if (item_id == 0U) {
                break;
            }
            list.push_back(item_id);
        }
    }
    for (;;) {
        compat::u16 item_id{};
        if (!next_word(item_id)) {
            return result;
        }
        if (item_id == 0U) {
            break;
        }
        compat::u16 quantity{};
        if (!next_word(quantity)) {
            return result;
        }
        result.prefix.player_inventory.push_back({item_id, quantity});
    }
    result.prefix.consumed_bytes = offset;
    result.complete = true;
    return result;
}

LegacySaveWorldEntry read_legacy_save_world_entry(
    const resource_io::LegacySaveContainer& save
) noexcept {
    const auto& source = save.raw_after_primary;
    LegacySaveWorldEntry result;
    result.selected_guid = read_word(source, 0U);
    result.logical_map_id = read_word(source, 4U);
    for (std::size_t index = 0U; index < result.selected_role_words.size();
         ++index) {
        result.selected_role_words[index] = read_word(source, 8U + index * 4U);
    }
    return result;
}

void restore_legacy_save_controlled_role_words(
    const LegacySaveWorldEntry& source, LegacyWorldRoleRecord& destination
) noexcept {
    destination.world_x = source.selected_role_words[0U];
    destination.world_y = source.selected_role_words[1U];
    destination.action.action_id = source.selected_role_words[2U];
    destination.action.base_variant = source.selected_role_words[3U];
    destination.action.variant_delta = source.selected_role_words[4U];
}

LegacySaveWorldExtensionA read_legacy_save_world_extension_a(
    const resource_io::LegacySaveContainer& save
) noexcept {
    const auto& source = save.extension_a;
    LegacySaveWorldExtensionA result;
    std::copy_n(
        source.begin(), result.role_names.size(), result.role_names.begin()
    );
    result.elapsed_ticks = read_word(source, 0x40U);
    result.deferred_tile_x =
        std::bit_cast<compat::i16>(read_half_word(source, 0x44U));
    result.deferred_tile_y =
        std::bit_cast<compat::i16>(read_half_word(source, 0x46U));
    result.deferred_map_id =
        std::bit_cast<compat::i16>(read_half_word(source, 0x48U));
    result.map_22_role_field_40 =
        std::bit_cast<compat::i16>(read_half_word(source, 0x4AU));
    result.primary_countdown_ticks = read_word(source, 0x4CU);
    result.primary_transition_value = read_word(source, 0x50U);
    std::copy_n(
        source.begin() + 0x54U,
        result.uninterpreted_tail.size(),
        result.uninterpreted_tail.begin()
    );
    return result;
}

void restore_legacy_save_world_transition_and_countdown(
    const LegacySaveWorldExtensionA& source,
    LegacyWorldStoryVmState& story,
    rendering::LegacyCountdownState& countdown
) noexcept {
    story.deferred_map_tile_x = source.deferred_tile_x;
    story.deferred_map_tile_y = source.deferred_tile_y;
    story.deferred_map_id = source.deferred_map_id;
    story.guid_one_action_override = std::bit_cast<compat::u32>(
        static_cast<compat::i32>(source.map_22_role_field_40)
    );
    countdown.primary_ticks = source.primary_countdown_ticks;
    countdown.primary_transition_value = source.primary_transition_value;
}

void restore_legacy_save_selection_extension(
    const resource_io::LegacySaveContainer& save,
    std::array<compat::i16, kLegacyWorldSelectionWordCount>& selection_words,
    LegacyWorldSelectionScrollState& selection_scroll
) noexcept {
    static_assert(4U + kLegacyWorldSelectionWordCount * 2U == 0x84U);
    const auto& source = save.extension_c;
    selection_scroll.frame_interval =
        std::bit_cast<compat::i16>(read_half_word(source, 0U));
    selection_scroll.frames_remaining =
        std::bit_cast<compat::i16>(read_half_word(source, 2U));
    for (std::size_t index = 0U; index < selection_words.size(); ++index) {
        selection_words[index] =
            std::bit_cast<compat::i16>(read_half_word(source, 4U + index * 2U));
    }
}

LegacySaveTailTextStatus restore_legacy_save_tail_mode_texts(
    const resource_io::LegacySaveContainer& save, LegacyWorldStoryVmState& story
) noexcept {
    const auto& source = save.blocks[4U].bytes;
    constexpr std::size_t kTextSize = kLegacyWorldStoryModeTextSize;
    constexpr std::size_t kRequired = 2U * kTextSize;
    if (source.size() < kRequired) {
        return LegacySaveTailTextStatus::missing_block_bytes;
    }
    for (std::size_t index = 0U; index < story.mode_texts.size(); ++index) {
        auto& text = story.mode_texts[index];
        text = {};
        const std::size_t offset = index * kTextSize;
        if (source[offset] != 0U) {
            std::copy_n(source.begin() + offset, kTextSize, text.bytes.begin());
            text.allocated = true;
        }
    }
    return LegacySaveTailTextStatus::ready;
}

LegacySaveRoleSourcesResult restore_legacy_save_role_sources(
    const resource_io::LegacySaveContainer& save,
    const std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& database
) noexcept {
    constexpr std::size_t kRecordSize = kLegacyMapsRoleSourceRecordSize;
    const auto& source = save.blocks[1U].bytes;
    LegacySaveRoleSourcesResult result;
    for (std::size_t index = 0U; index < source.size() / kRecordSize; ++index) {
        const std::size_t offset = index * kRecordSize;
        const LegacyMapsRolePatchRequest request{
            .guid = read_half_word(source, offset + 2U),
            .action_id = read_half_word(source, offset + 4U),
            .base_variant = read_half_word(source, offset + 6U),
            .variant_delta = read_half_word(source, offset + 8U),
            .tile_x = read_half_word(source, offset + 10U),
            .tile_y = read_half_word(source, offset + 12U),
            .talk_script_id = read_half_word(source, offset + 14U),
            .path_data_id = read_half_word(source, offset + 16U),
            .flags_or_mask = read_half_word(source, offset + 20U),
            .flags_and_mask = 0U,
            .logical_map_id = read_half_word(source, offset),
        };
        result.status = patch_legacy_maps_role_source_record(
            maps_payload, database, request
        );
        ++result.records_consumed;
        if (result.status == LegacyMapsRolePatchStatus::guid_not_found) {
            result.status = LegacyMapsRolePatchStatus::ready;
            continue;
        }
        if (result.status != LegacyMapsRolePatchStatus::ready) {
            return result;
        }
        ++result.records_matched;
    }
    return result;
}

LegacySaveStoryPrefixStatus restore_legacy_save_story_prefix(
    const resource_io::LegacySaveContainer& save, LegacyWorldStoryVmState& story
) noexcept {
    constexpr std::size_t kVariablesOffset = 4U;
    constexpr std::size_t kPartyResourcesOffset = 0x104U;
    constexpr std::size_t kPartyResourcesSize =
        sizeof(LegacyWorldStoryPartyMemberResources) *
        kLegacyWorldStoryPartyMemberResourceCount;
    static_assert(kPartyResourcesSize == 0xE0U);
    static_assert(
        std::is_trivially_copyable_v<LegacyWorldStoryPartyMemberResources>
    );

    const auto& flags = save.blocks[0U].bytes;
    const auto& state = save.blocks[2U].bytes;
    if (flags.size() < story.flags.size() ||
        state.size() < kPartyResourcesOffset + kPartyResourcesSize) {
        return LegacySaveStoryPrefixStatus::missing_block_bytes;
    }

    std::copy_n(flags.begin(), story.flags.size(), story.flags.begin());
    story.script_clock = read_word(state, 0U);
    for (std::size_t index = 0U; index < story.script_variables.size();
         ++index) {
        story.script_variables[index] =
            read_word(state, kVariablesOffset + index * 4U);
    }
    // rep movsd at 0x00408586 copies the entire 0xE0-byte physical record
    // group, including otherwise unobserved padding inside each record.
    std::memcpy(
        story.party_member_resources.data(),
        state.data() + kPartyResourcesOffset,
        kPartyResourcesSize
    );
    return LegacySaveStoryPrefixStatus::ready;
}

}  // namespace openswd3::world_map
