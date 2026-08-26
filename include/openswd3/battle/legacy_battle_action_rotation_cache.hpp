#pragma once

#include "openswd3/asset_runtime/legacy_action_record.hpp"
#include "openswd3/battle/legacy_battle_image_rotation.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace openswd3::battle {

struct LegacyBattleActionRotationUpdateSnapshot {
    compat::u32 eax{};
    compat::u32 edx{};
    std::uint64_t domain_token{};
};

class LegacyBattleActionRotationUpdatePort {
public:
    virtual ~LegacyBattleActionRotationUpdatePort() = default;

    [[nodiscard]] virtual LegacyBattleActionRotationUpdateSnapshot
    update_action(asset_runtime::LegacyActionRecord& record) = 0;
};

struct LegacyBattleMutableFrameImage {
    compat::u32 owner_token{};
    bool pointer_valid{};
    std::span<compat::u8> bytes{};
};

class LegacyBattleMutableFrameImagePort {
public:
    virtual ~LegacyBattleMutableFrameImagePort() = default;

    [[nodiscard]] virtual LegacyBattleMutableFrameImage
    query_frame_image(compat::u32 resource_id, compat::u32 frame_index) = 0;
};

struct LegacyBattleActionRotationCacheState {
    asset_runtime::LegacyActionRecord action_record{};
    std::array<compat::u32, 3> frame_owner_tokens{};
    compat::u32 field_b4{};
    compat::u32 field_b8{};
    compat::u32 field_bc{};
    compat::u16 stored_action_id{};
};

enum class LegacyBattleActionRotationCacheStatus : compat::u8 {
    completed,
    initial_action_update_stopped,
    action_update_stopped,
    frame_index_out_of_range,
    division_by_zero,
    frame_image_pointer_invalid,
    rotation_typed_stop,
    action_loop_nonterminating,
};

struct LegacyBattleActionRotationCacheResult {
    LegacyBattleActionRotationCacheStatus status{
        LegacyBattleActionRotationCacheStatus::completed
    };
    compat::u32 action_update_calls{};
    compat::u32 loop_iterations{};
    compat::u32 frame_query_calls{};
    compat::u32 rotation_calls{};
    compat::u32 skipped_cached_frames{};
    compat::u32 field_bc_writes{};
    compat::u32 record_clear_calls{};
    compat::u32 last_resource_id{};
    compat::u32 last_frame_index{};
    compat::i32 rotation_shift{};
    std::array<compat::u16, 3> local_frame_slots{0xFFFFU, 0xFFFFU, 0xFFFFU};
    LegacyBattleImageRotationResult rotation{};
};

// sub_451420: initialize and rotate up to three cached battle action frames.
[[nodiscard]] LegacyBattleActionRotationCacheResult
initialize_legacy_battle_action_rotation_cache(
    LegacyBattleActionRotationCacheState& state,
    LegacyBattleActionRotationUpdatePort& update_port,
    LegacyBattleMutableFrameImagePort& image_port,
    compat::u32 unused_destination_snapshot,
    compat::u32 field_b4,
    compat::u32 field_b8,
    compat::u32 initial_action_id,
    compat::u32 rotation_divisor
);

}  // namespace openswd3::battle
