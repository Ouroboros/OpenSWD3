#pragma once

#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <unordered_map>

namespace openswd3::battle {

// Owns the independent records returned by 431760. Cache spans borrow these
// images, never the immutable frames of the ordinary TSW cache.
class LegacyBattleActionRotationResources final
    : public LegacyBattleMutableFrameImagePort,
      public LegacyBattleActionRotationReleasePort {
public:
    explicit LegacyBattleActionRotationResources(
        asset_runtime::LegacyTswRuntime& runtime
    ) noexcept;

    [[nodiscard]] LegacyBattleMutableFrameImage query_frame_image(
        compat::u32 resource_id, compat::u32 frame_index
    ) override;

    // Tokens must name live allocations returned by this port. The caller
    // releases image then record, as 451730 does; misuse is not a no-op.
    void release_image(compat::u32 image_token) noexcept override;
    void release_owner(compat::u32 owner_token) noexcept override;

    [[nodiscard]] std::size_t live_owner_count() const noexcept;
    [[nodiscard]] std::size_t live_image_count() const noexcept;
    [[nodiscard]] std::optional<asset_runtime::LegacyTswRuntimeStatus>
    last_load_status() const noexcept;

private:
    asset_runtime::LegacyTswRuntime& runtime_;
    std::unordered_map<
        compat::u32,
        std::unique_ptr<asset_runtime::LegacyTswRuntimeFrame>>
        owners_;
    std::optional<asset_runtime::LegacyTswRuntimeStatus> last_load_status_;
};

}  // namespace openswd3::battle
