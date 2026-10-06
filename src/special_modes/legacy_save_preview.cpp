#include "openswd3/special_modes/legacy_save_preview.hpp"

#include <algorithm>
#include <charconv>
#include <new>

namespace openswd3::special_modes {
namespace {

compat::u16 read_u16(const compat::u8* source) noexcept {
    return static_cast<compat::u16>(
        static_cast<compat::u16>(source[0]) |
        (static_cast<compat::u16>(source[1]) << 8U)
    );
}

void write_u16(compat::u8* destination, const compat::u16 value) noexcept {
    destination[0] = static_cast<compat::u8>(value);
    destination[1] = static_cast<compat::u8>(value >> 8U);
}

void write_u32(compat::u8* destination, const compat::u32 value) noexcept {
    write_u16(destination, static_cast<compat::u16>(value));
    write_u16(destination + 2U, static_cast<compat::u16>(value >> 16U));
}

void append_time_part(
    std::vector<compat::u8>& output,
    std::size_t& offset,
    const compat::u32 value,
    const compat::u8 suffix_first,
    const compat::u8 suffix_second
) {
    std::array<char, 10U> digits{};
    const auto converted =
        std::to_chars(digits.data(), digits.data() + digits.size(), value);
    if (value < 10U) {
        output[offset++] = '0';
    }

    for (auto cursor = digits.data(); cursor != converted.ptr; ++cursor) {
        output[offset++] = static_cast<compat::u8>(*cursor);
    }

    output[offset++] = suffix_first;
    output[offset++] = suffix_second;
}

}  // namespace

void reset_legacy_save_preview(LegacySavePreviewRecord& preview) noexcept {
    std::vector<compat::u16>{}.swap(preview.pixels);
    std::vector<compat::u8>{}.swap(preview.timestamp);
    std::vector<compat::u8>{}.swap(preview.map_name);
    std::vector<compat::u8>{}.swap(preview.role_names);
    std::vector<compat::u8>{}.swap(preview.play_time);
    preview.bytes.fill(0U);
}

void refresh_legacy_save_preview_actions(
    LegacyInputMenuSavePreviewResetState& state
) noexcept {
    constexpr std::array<compat::u32, 4U> action_ids{1U, 2U, 8U, 17U};
    for (compat::i32 column = 0; column < 3; ++column) {
        for (std::size_t index = 0U; index < action_ids.size(); ++index) {
            auto& action = state.preview_actions
                               [static_cast<std::size_t>(column) * 4U + index];
            asset_runtime::initialize_legacy_action_record(action);
            action.action_id = action_ids[index];
            const auto selected_column = state.selected_save_slot % 3;
            action.base_variant = 0U;
            action.variant_delta = 6U;
            if (selected_column == column) {
                action.base_variant = 8U;
            }
        }
    }
}

LegacySavePreviewPopulateStatus populate_legacy_save_preview(
    LegacySavePreviewRecord& preview,
    const resource_io::LegacySavePreviewPayload& payload,
    const rendering::LegacyPixelConversionState& pixel_conversion,
    const resource_io::LegacySavePreviewReadStage next_read
) {
    using Stage = resource_io::LegacySavePreviewReadStage;
    try {
        std::array<compat::u8, 17U> date{
            '_',
            '_',
            '_',
            '_',
            '/',
            '_',
            '_',
            '/',
            '_',
            '_',
            ' ',
            '_',
            '_',
            ':',
            '_',
            '_',
            0U,
        };
        preview.timestamp.resize(date.size());
        if (next_read == Stage::timestamp) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        std::copy_n(payload.timestamp.data(), 4U, date.data());
        for (std::size_t index = 0U; index < 4U; ++index) {
            std::copy_n(
                payload.timestamp.data() + 4U + index * 2U,
                2U,
                date.data() + 5U + index * 3U
            );
        }

        const auto date_end = std::find(date.begin(), date.end(), 0U);
        std::copy(date.begin(), date_end + 1, preview.timestamp.begin());
        preview.pixels.resize(0x4B00U);
        const auto pixel_count = next_read == Stage::pixels
            ? payload.preview_bytes_read / 2U
            : preview.pixels.size();
        for (std::size_t index = 0U; index < pixel_count; ++index) {
            preview.pixels[index] =
                read_u16(payload.preview.data() + index * 2U);
        }

        if (next_read == Stage::pixels) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        rendering::legacy_convert_pixels_forward(
            pixel_conversion, preview.pixels.data(), 0x4B00
        );
        if (next_read == Stage::label) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        const auto label_end = std::find(
            payload.nul_terminated_label.begin(),
            payload.nul_terminated_label.end(),
            0U
        );
        if (label_end == payload.nul_terminated_label.end()) {
            return LegacySavePreviewPopulateStatus::label_unterminated;
        }

        preview.map_name.assign(
            payload.nul_terminated_label.begin(), label_end + 1
        );
        if (next_read == Stage::flags) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        constexpr std::array<std::size_t, 4U> flag_offsets{3U, 3U, 4U, 4U};
        constexpr std::array<compat::u8, 4U> flag_masks{0x40U, 0x80U, 1U, 2U};
        for (std::size_t index = 0U; index < flag_offsets.size(); ++index) {
            if (payload.flags.bytes.size() <= flag_offsets[index]) {
                return LegacySavePreviewPopulateStatus::flags_truncated;
            }

            if ((payload.flags.bytes[flag_offsets[index]] &
                 flag_masks[index]) == 0U) {
                write_u16(preview.bytes.data() + index * 8U, 0xFFFFU);
            }
        }

        if (payload.flags.bytes.size() <= 10U) {
            return LegacySavePreviewPopulateStatus::flags_truncated;
        }

        write_u32(
            preview.bytes.data() + 0x24U,
            (payload.flags.bytes[10U] & 8U) != 0U ? 3U : 2U
        );
        if (next_read == Stage::primary) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        std::copy_n(
            payload.raw_after_primary.data() + 4U,
            4U,
            preview.bytes.data() + 0x20U
        );
        if (next_read == Stage::party) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        if (payload.party.bytes.size() < 8U) {
            return LegacySavePreviewPopulateStatus::party_truncated;
        }

        std::copy_n(
            payload.party.bytes.data() + 4U, 4U, preview.bytes.data() + 0x28U
        );
        // 0040982C allocates the separate 0xE0-byte role snapshot before
        // the source copy. Its failure retains the preceding +0x28 write.
        std::vector<compat::u8> role_snapshot(0xE0U);
        if (payload.party.bytes.size() < 0x1E4U) {
            return LegacySavePreviewPopulateStatus::party_truncated;
        }

        std::copy_n(
            payload.party.bytes.data() + 0x104U,
            role_snapshot.size(),
            role_snapshot.data()
        );
        for (std::size_t index = 0U; index < 4U; ++index) {
            auto* destination = preview.bytes.data() + index * 8U;
            if (read_u16(destination) == 0xFFFFU) {
                continue;
            }

            const auto* role = role_snapshot.data() + index * 0x38U;
            std::copy_n(role + 0x0AU, 6U, destination);
            write_u16(destination + 6U, role[0x2CU]);
        }

        preview.role_names.resize(0x40U);
        std::copy_n(
            payload.role_names.data(),
            next_read == Stage::role_names ? payload.role_name_bytes_read
                                           : payload.role_names.size(),
            preview.role_names.data()
        );
        if (next_read == Stage::role_names ||
            next_read == Stage::elapsed_seconds) {
            return LegacySavePreviewPopulateStatus::payload_unavailable;
        }

        preview.play_time.assign(0x40U, 0U);
        std::size_t offset{};
        append_time_part(
            preview.play_time,
            offset,
            payload.elapsed_seconds / 3600U,
            0xAEU,
            0xC9U
        );
        append_time_part(
            preview.play_time,
            offset,
            (payload.elapsed_seconds % 3600U) / 60U,
            0xA4U,
            0xC0U
        );
        append_time_part(
            preview.play_time,
            offset,
            payload.elapsed_seconds % 60U,
            0xACU,
            0xEDU
        );
        std::vector<compat::u8>{}.swap(role_snapshot);
        write_u16(preview.bytes.data() + 0x30U, 2U);
        return LegacySavePreviewPopulateStatus::completed;
    } catch (const std::bad_alloc&) {
        return LegacySavePreviewPopulateStatus::allocation_failed;
    }
}

}  // namespace openswd3::special_modes
