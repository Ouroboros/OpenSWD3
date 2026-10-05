#pragma once

#include "openswd3/rendering/legacy_countdown.hpp"
#include "openswd3/resource_io/legacy_save_container.hpp"
#include "openswd3/world_map/legacy_maps_world_database.hpp"
#include "openswd3/world_map/legacy_world_item_lifecycle.hpp"
#include "openswd3/world_map/legacy_world_role_record.hpp"
#include "openswd3/world_map/legacy_world_selection_scroll.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"

#include <array>
#include <vector>

namespace openswd3::world_map {

// Restores only the byte ranges identified at 0x00408366 and
// 0x0040855F–0x00408586. Other save owners must be restored before gameplay.
enum class LegacySaveStoryPrefixStatus {
    ready,
    missing_block_bytes,
};

struct LegacySaveRoleSourcesResult {
    LegacyMapsRolePatchStatus status{LegacyMapsRolePatchStatus::ready};
    std::size_t records_consumed{};
    std::size_t records_matched{};
};

struct LegacySaveInventoryEntry {
    compat::u16 raw_item_id{};
    compat::u16 quantity{};
};

struct LegacySaveU16Prefix {
    std::array<compat::u16, 64U> monster_ids{};
    std::array<std::vector<compat::u16>, 4U> party_item_ids;
    std::vector<LegacySaveInventoryEntry> player_inventory;
    std::size_t consumed_bytes{};
};

struct LegacySaveU16PrefixResult {
    bool complete{};
    LegacySaveU16Prefix prefix;
};

struct LegacySaveMapOverride {
    compat::u16 key{};
    compat::u16 upper_word{};
    compat::u32 value{};
};

struct LegacySaveMapOverridesResult {
    bool complete{};
    std::vector<LegacySaveMapOverride> records;
    std::size_t consumed_bytes{};
};

enum class LegacySaveMapOverrideApplyStatus {
    ready,
    directory_out_of_range,
    record_out_of_range,
};

struct LegacySaveMapOverrideApplyResult {
    LegacySaveMapOverrideApplyStatus status{
        LegacySaveMapOverrideApplyStatus::ready
    };
    std::size_t records_seen{};
    std::size_t records_matched{};
    std::size_t records_written{};
    std::size_t records_skipped_flag{};
};

// 0x0040890C–0x0040897A; these are MAPS payload record updates, not MON IDs.
[[nodiscard]] LegacySaveMapOverridesResult read_legacy_save_map_overrides(
    const resource_io::LegacySaveContainer& save, std::size_t offset
);
[[nodiscard]] LegacySaveMapOverrideApplyResult apply_legacy_save_map_overrides(
    std::span<compat::u8> maps_payload,
    std::span<const LegacySaveMapOverride> records
) noexcept;

struct LegacySaveWorldEntry {
    compat::u32 selected_guid{};
    compat::u32 logical_map_id{};
    // Original role base 0x004BABA8: dwords at +4, +8, +0x40,
    // +0x48 and +0x74 (position, action id, base variant, variant delta).
    std::array<compat::u32, 5U> selected_role_words{};
};

// 0x004084B3–0x00408501. The original writes all seven words before
// sub_40C130; that map load uses the saved coordinates and action values.
[[nodiscard]] LegacySaveWorldEntry read_legacy_save_world_entry(
    const resource_io::LegacySaveContainer& save
) noexcept;
void restore_legacy_save_controlled_role_words(
    const LegacySaveWorldEntry& source, LegacyWorldRoleRecord& destination
) noexcept;

enum class LegacySaveWorldLoadStatus {
    ready,
    role_source_failed,
};

struct LegacySaveWorldLoadResult {
    LegacySaveWorldLoadStatus status{LegacySaveWorldLoadStatus::ready};
    LegacyWorldLoadRequest load;
    LegacySaveRoleSourcesResult role_sources;
};

// 0x0040844E–0x004084C7 patches role sources before the 0x00408BDE load.
// Saved role words are inputs to that load, not a post-load position patch.
[[nodiscard]] LegacySaveWorldLoadResult prepare_legacy_save_world_load(
    const resource_io::LegacySaveContainer& save,
    std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& database
);

struct LegacySaveWorldExtensionA {
    std::array<compat::u8, 0x40U> role_names{};
    compat::u32 elapsed_ticks{};
    compat::i16 deferred_tile_x{};
    compat::i16 deferred_tile_y{};
    compat::i16 deferred_map_id{};
    compat::i16 map_22_role_field_40{};
    compat::u32 primary_countdown_ticks{};
    compat::u32 primary_transition_value{};
    std::array<compat::u8, 0x30U> uninterpreted_tail{};
};

// 0x00408997–0x00408A4B, before the 0x180-byte party extension.
[[nodiscard]] LegacySaveWorldExtensionA read_legacy_save_world_extension_a(
    const resource_io::LegacySaveContainer& save
) noexcept;

// Publish the existing VM and countdown owners after world initialization.
// Role names, elapsed play time and the uninterpreted tail have other owners.
void restore_legacy_save_world_transition_and_countdown(
    const LegacySaveWorldExtensionA& source,
    LegacyWorldStoryVmState& story,
    rendering::LegacyCountdownState& countdown
) noexcept;

// 0x00408A9B–0x00408AE7, after Fame. A complete current container owns
// the 0x84-byte extension; older saves need their separate EOF branch.
void restore_legacy_save_selection_extension(
    const resource_io::LegacySaveContainer& save,
    std::array<compat::i16, kLegacyWorldSelectionWordCount>& selection_words,
    LegacyWorldSelectionScrollState& selection_scroll
) noexcept;

enum class LegacySaveTailTextStatus {
    ready,
    missing_block_bytes,
};

// 0x00408B18–0x00408B65: each 0x34-byte slot is nullable by first byte.
[[nodiscard]] LegacySaveTailTextStatus restore_legacy_save_tail_mode_texts(
    const resource_io::LegacySaveContainer& save, LegacyWorldStoryVmState& story
) noexcept;

// 0x00408593–0x0040890C; parsing alone does not allocate MON definitions
// or publish any live item lists.
[[nodiscard]] LegacySaveU16PrefixResult
read_legacy_save_u16_prefix(const resource_io::LegacySaveContainer& save);

struct LegacySaveItemDefinitionResult {
    bool loaded{};
    compat::u32 description_token{};
};

class LegacySaveItemDefinitionPort {
public:
    virtual ~LegacySaveItemDefinitionPort() = default;
    [[nodiscard]] virtual LegacySaveItemDefinitionResult load_definition(
        compat::u16 item_id,
        std::span<compat::u8, kLegacyItemDefinitionSnapshotBytes> snapshot,
        std::vector<compat::u8>& description
    ) = 0;
};

enum class LegacySaveRoleDefinitionsStatus {
    ready,
    allocation_failed,
};

struct LegacySaveRoleDefinitionsResult {
    LegacySaveRoleDefinitionsStatus status{
        LegacySaveRoleDefinitionsStatus::ready
    };
    std::size_t definitions_loaded{};
    std::size_t definitions_missing{};
    std::size_t sentinel_ids{};
};

// 0x004085D5–0x00408773; the 64 role roots precede the four party lists.
[[nodiscard]] LegacySaveRoleDefinitionsResult
materialize_legacy_save_role_definitions(
    const LegacySaveU16Prefix& source,
    LegacyWorldItemListState& destination,
    LegacySaveItemDefinitionPort& definitions
) noexcept;

enum class LegacySaveItemListStatus {
    ready,
    missing_party_sentinel,
    allocation_failed,
};

struct LegacySaveItemListResult {
    LegacySaveItemListStatus status{LegacySaveItemListStatus::ready};
    std::size_t party_nodes{};
    std::size_t player_nodes{};
    std::size_t definition_failures{};
};

// 0x00408779–0x0040890C. The host token allocator is the same typed owner
// subsequently consumed by battle item traversal; it is not a Win32 pointer.
[[nodiscard]] LegacySaveItemListResult materialize_legacy_save_item_lists(
    const LegacySaveU16Prefix& source,
    LegacyWorldItemListState& destination,
    LegacySaveItemDefinitionPort& definitions
) noexcept;

// 0x0040844E–0x004084A6: one sub_40D460 call for each complete 22-byte
// record. Missing GUIDs are diagnosed by the original callee, then ignored.
[[nodiscard]] LegacySaveRoleSourcesResult restore_legacy_save_role_sources(
    const resource_io::LegacySaveContainer& save,
    std::span<compat::u8> maps_payload,
    LegacyMapsWorldDatabase& database
) noexcept;

// 0x00408366 precedes the block-2 EOF gate and also applies to older saves.
[[nodiscard]] LegacySaveStoryPrefixStatus restore_legacy_save_story_flags(
    const resource_io::LegacySaveContainer& save, LegacyWorldStoryVmState& story
) noexcept;

[[nodiscard]] LegacySaveStoryPrefixStatus restore_legacy_save_story_prefix(
    const resource_io::LegacySaveContainer& save, LegacyWorldStoryVmState& story
) noexcept;

// 0x00408B91–0x00408B9E: fill the entire 0xD8-byte Talk context with FF
// before loading the saved map. Script zero is not an idle context.
void reset_legacy_save_talk_context(LegacyWorldTalkContext& context) noexcept;

}  // namespace openswd3::world_map
