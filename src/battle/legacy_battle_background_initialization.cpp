#include "openswd3/battle/legacy_battle_background_initialization.hpp"

#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/asset_runtime/legacy_tsw_archive.hpp"
#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <span>
#include <utility>

namespace openswd3::battle {

LegacyBattleBackgroundInitializationRequest
make_legacy_battle_startup_background_request(
    const LegacyBattleDefinition& definition,
    const std::filesystem::path& data_root,
    const compat::u32 random_below_four
) {
    return {
        .data_root = data_root,
        .one_based_resource = definition.background_resource,
        .initial_action_id = definition.background_action_id,
        .field_b4 = definition.background_field_b4,
        .field_b8 = definition.background_field_b8,
        .rotation_divisor = static_cast<compat::u16>(random_below_four + 1U),
        .background_action_gate = definition.secondary_count,
    };
}

LegacyBattleBackgroundImageLoadResult
LegacyBattleArchiveBackgroundImageLoadPort::load_image(
    const std::filesystem::path& archive_path,
    const compat::u32 one_based_resource,
    const compat::u32 variant_index
) {
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
        .width = loaded.frame.descriptor.width,
        .height = loaded.frame.descriptor.height,
        .image_size = loaded.frame.descriptor.primary_decompressed_size,
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

namespace {

[[nodiscard]] compat::u16 read_image_word(
    const std::span<const compat::u8> bytes, const std::size_t offset
) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(bytes[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(bytes[offset + 1U]) << 8U
        )
    );
}

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
    case LegacyBattleActionRotationCacheStatus::frame_query_typed_stop:
    case LegacyBattleActionRotationCacheStatus::action_update_typed_stop:
    case LegacyBattleActionRotationCacheStatus::action_update_edx_unavailable:
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

    const compat::u32 previous_image_token = background.image_record[0U];
    if (previous_image_token != 0U) {
        result.previous_image_release_token = previous_image_token;
        if (background.image_allocation_token != previous_image_token) {
            result.status = LegacyBattleBackgroundInitializationStatus::
                image_release_typed_stop;
            return result;
        }

        std::vector<compat::u8>{}.swap(background.image);
        background.image_allocation_token = 0U;
        result.previous_image_released = true;
        background.image_record[0U] = 0U;
    }

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

    const auto image_token =
        asset_runtime::reserve_legacy_guest_bytes(loaded.command_stream.size());
    std::optional<compat::u32> palette_token;
    if (loaded.has_palette) {
        palette_token = asset_runtime::reserve_legacy_guest_bytes(
            loaded.palette.size() * sizeof(compat::u16)
        );
    }

    if (!image_token.has_value() ||
        (loaded.has_palette && !palette_token.has_value())) {
        result.status = LegacyBattleBackgroundInitializationStatus::
            image_identity_typed_stop;
        return result;
    }

    background.image_allocation_token = *image_token;
    background.image = std::move(loaded.command_stream);
    if (loaded.has_palette) {
        background.palette_allocation_token = *palette_token;
        background.image_palette = loaded.palette;
    }

    background.image_record[0U] = *image_token;
    background.image_record[3U] =
        (background.image_record[3U] & 0xFFFF0000U) | loaded.width;
    background.image_record[3U] = (background.image_record[3U] & 0x0000FFFFU) |
        (static_cast<compat::u32>(loaded.height) << 16U);
    background.image_record[1U] = 0U;
    background.image_record[2U] = loaded.has_palette ? *palette_token : 0U;
    background.image_record[4U] = loaded.image_size;

    using ConversionStatus = rendering::LegacyImageCommandStreamStatus;
    if (background.image.size() < sizeof(compat::u16)) {
        result.conversion.status = ConversionStatus::source_exhausted;
    } else if (
        read_image_word(background.image, 0U) !=
        rendering::kLegacyImageCommandStreamMagic
    ) {
        result.conversion.status = ConversionStatus::invalid_magic;
    } else if (background.image.size() < 8U) {
        result.conversion.status = ConversionStatus::source_exhausted;
    } else if ((read_image_word(background.image, 6U) & 0x7FFFU) != 8U) {
        background.image_record[3U] =
            (background.image_record[3U] & 0xFFFF0000U) |
            read_image_word(background.image, 2U);
        background.image_record[3U] =
            (background.image_record[3U] & 0x0000FFFFU) |
            (static_cast<compat::u32>(read_image_word(background.image, 4U))
             << 16U);
        result.conversion.status =
            rendering::convert_legacy_image_command_stream_literals_in_place(
                background.image, pixel_conversion, &result.conversion.header
            );
    } else {
        const compat::u32 allocation_size =
            static_cast<compat::u32>(read_image_word(background.image, 2U)) *
                static_cast<compat::u32>(
                    read_image_word(background.image, 4U)
                ) *
                4U +
            0x800U;
        const auto temporary_token =
            asset_runtime::reserve_legacy_guest_bytes(allocation_size);
        if (!temporary_token.has_value()) {
            result.status = LegacyBattleBackgroundInitializationStatus::
                image_identity_typed_stop;
            return result;
        }

        const std::span<const compat::u16> palette = loaded.has_palette
            ? std::span<const compat::u16>{background.image_palette}
            : std::span<const compat::u16>{};
        result.conversion = rendering::convert_legacy_image_command_stream(
            background.image, palette, pixel_conversion
        );
        if (result.conversion.status == ConversionStatus::completed) {
            if (result.conversion.bytes.size() > allocation_size) {
                result.conversion.status = ConversionStatus::size_overflow;
            } else {
                background.palette_allocation_token = 0U;
                std::vector<compat::u8>{}.swap(background.image);
                background.image_allocation_token = 0U;
                const auto final_image_token =
                    asset_runtime::reserve_legacy_guest_bytes(
                        result.conversion.bytes.size()
                    );
                if (!final_image_token.has_value()) {
                    result.status = LegacyBattleBackgroundInitializationStatus::
                        image_identity_typed_stop;
                    return result;
                }

                background.image = std::move(result.conversion.bytes);
                background.image_allocation_token = *final_image_token;
                background.image_record[4U] =
                    static_cast<compat::u32>(background.image.size());
                background.image_record[0U] = *final_image_token;
                background.image_record[2U] = 0U;
            }
        }
    }

    if (result.conversion.status != ConversionStatus::completed &&
        result.conversion.status != ConversionStatus::invalid_magic &&
        result.conversion.status != ConversionStatus::unsupported_depth) {
        result.status = LegacyBattleBackgroundInitializationStatus::
            image_conversion_typed_stop;
        return result;
    }

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
    LegacyBattleArchiveBackgroundImageLoadPort image_load_port;
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
