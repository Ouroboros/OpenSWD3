#include "openswd3/battle/legacy_battle_background_initialization.hpp"

#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"

#include <span>
#include <utility>

namespace openswd3::battle {
namespace {

class ArchiveBackgroundImageLoadPort final
    : public LegacyBattleBackgroundImageLoadPort {
public:
    [[nodiscard]] LegacyBattleBackgroundImageLoadResult load_image(
        const std::filesystem::path& archive_path,
        const compat::u32 one_based_resource,
        const compat::u32 variant_index
    ) override {
        asset_runtime::LegacyTswArchive archive;
        if (archive.open(archive_path) !=
            asset_runtime::LegacyTswOpenStatus::ready) {
            return {};
        }

        asset_runtime::LegacyTswFrameResult loaded =
            archive.read_frame(one_based_resource, variant_index);
        if (loaded.status != asset_runtime::LegacyTswFrameStatus::ready) {
            return {};
        }

        LegacyBattleBackgroundImageLoadResult result{
            .ready = true,
            .has_palette = loaded.frame.has_palette,
            .command_stream = std::move(loaded.frame.command_stream),
        };
        for (std::size_t index = 0U; index < result.palette.size(); ++index) {
            const std::size_t offset = index * 2U;
            result.palette[index] = static_cast<compat::u16>(
                static_cast<compat::u16>(loaded.frame.palette[offset]) |
                static_cast<compat::u16>(
                    static_cast<compat::u16>(loaded.frame.palette[offset + 1U])
                    << 8U
                )
            );
        }
        return result;
    }
};

[[nodiscard]] bool is_image_rotation_typed_stop(
    const LegacyBattleImageRotationStatus status
) noexcept {
    switch (status) {
    case LegacyBattleImageRotationStatus::completed:
    case LegacyBattleImageRotationStatus::shift_not_positive:
    case LegacyBattleImageRotationStatus::magic_mismatch:
    case LegacyBattleImageRotationStatus::first_row_flags_unsupported:
    case LegacyBattleImageRotationStatus::mode_out_of_range:
        return false;
    case LegacyBattleImageRotationStatus::header_read_out_of_range:
    case LegacyBattleImageRotationStatus::first_row_header_read_out_of_range:
    case LegacyBattleImageRotationStatus::image_read_out_of_range:
    case LegacyBattleImageRotationStatus::image_write_out_of_range:
    case LegacyBattleImageRotationStatus::temporary_read_out_of_range:
    case LegacyBattleImageRotationStatus::temporary_write_out_of_range:
        return true;
    }
    return true;
}

[[nodiscard]] bool is_action_rotation_typed_stop(
    const LegacyBattleActionRotationCacheStatus status
) noexcept {
    switch (status) {
    case LegacyBattleActionRotationCacheStatus::completed:
    case LegacyBattleActionRotationCacheStatus::initial_action_update_stopped:
    case LegacyBattleActionRotationCacheStatus::action_update_stopped:
        return false;
    case LegacyBattleActionRotationCacheStatus::frame_index_out_of_range:
    case LegacyBattleActionRotationCacheStatus::division_by_zero:
    case LegacyBattleActionRotationCacheStatus::frame_image_pointer_invalid:
    case LegacyBattleActionRotationCacheStatus::rotation_typed_stop:
    case LegacyBattleActionRotationCacheStatus::action_loop_nonterminating:
        return true;
    }
    return true;
}

}  // namespace

LegacyBattleBackgroundInitializationResult initialize_legacy_battle_background(
    LegacyBattleBackgroundState& background,
    LegacyBattleActionRotationCacheState& rotation_cache,
    LegacyBattleBackgroundImageLoadPort& image_load_port,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const LegacyBattleBackgroundInitializationRequest& request
) {
    LegacyBattleBackgroundInitializationResult result;
    result.archive_path =
        request.data_root / kLegacyBattleBackgroundArchiveName;

    result.cache_release = release_legacy_battle_action_rotation_cache(
        rotation_cache, rotation_release_port
    );

    result.previous_image_released = !background.image.empty();
    background.image.clear();

    LegacyBattleBackgroundImageLoadResult loaded = image_load_port.load_image(
        result.archive_path, request.one_based_resource, 0U
    );
    result.image_load_calls = 1U;
    if (!loaded.ready) {
        result.status =
            LegacyBattleBackgroundInitializationStatus::image_load_failed;
        result.return_value = 0U;
        return result;
    }

    const std::span<const compat::u16> palette = loaded.has_palette
        ? std::span<const compat::u16>{loaded.palette}
        : std::span<const compat::u16>{};
    result.conversion = rendering::convert_legacy_image_command_stream(
        loaded.command_stream, palette, pixel_conversion
    );
    if (result.conversion.status !=
        rendering::LegacyImageCommandStreamStatus::completed) {
        result.status = LegacyBattleBackgroundInitializationStatus::
            image_conversion_typed_stop;
        return result;
    }
    background.image = std::move(result.conversion.bytes);

    if (request.rotation_divisor == 0) {
        result.status = LegacyBattleBackgroundInitializationStatus::
            rotation_division_by_zero;
        return result;
    }
    result.rotation_shift =
        static_cast<compat::i32>(compat::i32{640} / request.rotation_divisor);
    result.image_rotation = rotate_legacy_battle_literal_image(
        background.image,
        LegacyBattleImageRotationMode::pixels_right,
        result.rotation_shift
    );
    if (is_image_rotation_typed_stop(result.image_rotation.status)) {
        result.status = LegacyBattleBackgroundInitializationStatus::
            image_rotation_typed_stop;
        return result;
    }

    if (request.background_action_gate != 0U &&
        static_cast<compat::u16>(request.initial_action_id) != 0U) {
        result.action_rotation_requested = true;
        result.action_rotation = initialize_legacy_battle_action_rotation_cache(
            rotation_cache,
            action_update_port,
            frame_image_port,
            kLegacyBattleBackgroundGeometryOwnerToken,
            request.field_b4,
            request.field_b8,
            request.initial_action_id,
            static_cast<compat::u32>(request.rotation_divisor)
        );
        if (is_action_rotation_typed_stop(result.action_rotation.status)) {
            result.status = LegacyBattleBackgroundInitializationStatus::
                action_rotation_cache_typed_stop;
            return result;
        }
    }

    background.completion_words[2] = 0xFFFFU;
    result.completion_write_order[0] = 2U;
    background.completion_words[1] = 0xFFFFU;
    result.completion_write_order[1] = 1U;
    background.completion_words[0] = 0xFFFFU;
    result.completion_write_order[2] = 0U;
    result.completion_words_published = true;
    result.return_value = 0xFFFFFFFFU;
    return result;
}

LegacyBattleBackgroundInitializationResult initialize_legacy_battle_background(
    LegacyBattleBackgroundState& background,
    LegacyBattleActionRotationCacheState& rotation_cache,
    LegacyBattleActionRotationReleasePort& rotation_release_port,
    LegacyBattleActionRotationUpdatePort& action_update_port,
    LegacyBattleMutableFrameImagePort& frame_image_port,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const LegacyBattleBackgroundInitializationRequest& request
) {
    ArchiveBackgroundImageLoadPort image_load_port;
    return initialize_legacy_battle_background(
        background,
        rotation_cache,
        image_load_port,
        rotation_release_port,
        action_update_port,
        frame_image_port,
        pixel_conversion,
        request
    );
}

}  // namespace openswd3::battle
