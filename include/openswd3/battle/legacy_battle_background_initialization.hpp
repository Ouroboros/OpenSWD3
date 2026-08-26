#pragma once

#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"
#include "openswd3/battle/legacy_battle_image_rotation.hpp"
#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <array>
#include <filesystem>
#include <vector>

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleBackgroundGeometryOwnerToken =
    0x0053B0B8U;
inline constexpr compat::u32 kLegacyBattleBackgroundRotationCacheToken =
    0x004FDFA8U;
inline constexpr const char* kLegacyBattleBackgroundArchiveName =
    "all_map2.tsw";

struct LegacyBattleBackgroundImageLoadResult {
    bool ready{};
    bool has_palette{};
    std::array<compat::u16, 256> palette{};
    std::vector<compat::u8> command_stream;
};

class LegacyBattleBackgroundImageLoadPort {
public:
    virtual ~LegacyBattleBackgroundImageLoadPort() = default;

    [[nodiscard]] virtual LegacyBattleBackgroundImageLoadResult load_image(
        const std::filesystem::path& archive_path,
        compat::u32 one_based_resource,
        compat::u32 variant_index
    ) = 0;
};

struct LegacyBattleBackgroundInitializationRequest {
    std::filesystem::path data_root;
    compat::u32 one_based_resource{};
    compat::u32 initial_action_id{};
    compat::u32 field_b4{};
    compat::u32 field_b8{};
    compat::i32 rotation_divisor{};
    compat::u16 background_action_gate{};
};

struct LegacyBattleBackgroundState {
    std::vector<compat::u8> image;
    std::array<compat::u16, 3> completion_words{};
};

enum class LegacyBattleBackgroundInitializationStatus : compat::u8 {
    completed,
    image_load_failed,
    image_conversion_typed_stop,
    rotation_division_by_zero,
    image_rotation_typed_stop,
    action_rotation_cache_typed_stop,
};

struct LegacyBattleBackgroundInitializationResult {
    LegacyBattleBackgroundInitializationStatus status{
        LegacyBattleBackgroundInitializationStatus::completed
    };
    std::filesystem::path archive_path;
    LegacyBattleActionRotationReleaseResult cache_release{};
    bool previous_image_released{};
    compat::u32 image_load_calls{};
    rendering::LegacyImageCommandStreamResult conversion{};
    compat::i32 rotation_shift{};
    LegacyBattleImageRotationResult image_rotation{};
    bool action_rotation_requested{};
    LegacyBattleActionRotationCacheResult action_rotation{};
    bool completion_words_published{};
    std::array<compat::u8, 3> completion_write_order{};
    compat::u32 return_value{};
};

// sub_451940: rebuild the battle background image and optional action cache.
[[nodiscard]] LegacyBattleBackgroundInitializationResult
initialize_legacy_battle_background(
    LegacyBattleBackgroundState& background,
    LegacyBattleActionRotationCacheState& rotation_cache,
    LegacyBattleBackgroundImageLoadPort& image_load_port,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const LegacyBattleBackgroundInitializationRequest& request
);

[[nodiscard]] LegacyBattleBackgroundInitializationResult
initialize_legacy_battle_background(
    LegacyBattleBackgroundState& background,
    LegacyBattleActionRotationCacheState& rotation_cache,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const LegacyBattleBackgroundInitializationRequest& request
);

}  // namespace openswd3::battle
