#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <utility>

namespace openswd3::asset_runtime {
namespace {

constexpr compat::u16 kCommandEa = 0x4145U;
constexpr compat::u16 kCommandHa = 0x4148U;
constexpr compat::u16 kCommandMa = 0x414DU;
constexpr compat::u16 kCommandNa = 0x414EU;
constexpr compat::u16 kCommandTa = 0x4154U;
constexpr compat::u16 kCommandXa = 0x4158U;
constexpr compat::u16 kCommandYa = 0x4159U;
constexpr compat::u16 kCommandBc = 0x4342U;
constexpr compat::u16 kCommandGc = 0x4347U;
constexpr compat::u16 kCommandLc = 0x434CU;
constexpr compat::u16 kCommandRc = 0x4352U;
constexpr compat::u16 kCommandDe = 0x4544U;
constexpr compat::u16 kCommandLf = 0x464CU;
constexpr compat::u16 kCommandSg = 0x4753U;
constexpr compat::u16 kCommandDl = 0x4C44U;
constexpr compat::u16 kCommandOn = 0x4E4FU;
constexpr compat::u16 kCommand2O = 0x4F32U;
constexpr compat::u16 kCommandAo = 0x4F41U;
constexpr compat::u16 kCommandVo = 0x4F56U;
constexpr compat::u16 kCommandXo = 0x4F58U;
constexpr compat::u16 kCommandYo = 0x4F59U;
constexpr compat::u16 kCommandAp = 0x5041U;
constexpr compat::u16 kCommandEq = 0x5145U;
constexpr compat::u16 kCommandFr = 0x5246U;
constexpr compat::u16 kCommandOr = 0x524FU;
constexpr compat::u16 kCommandDs = 0x5344U;
constexpr compat::u16 kCommandMs = 0x534DU;
constexpr compat::u16 kCommandNt = 0x544EU;
constexpr compat::u16 kCommandWt = 0x5457U;
constexpr compat::u16 kCommandIv = 0x5649U;
constexpr compat::u16 kCommandHw = 0x5748U;
constexpr compat::u16 kCommandVw = 0x5756U;
constexpr compat::u16 kCommandYx = 0x5859U;

[[nodiscard]] bool read_word(
    const std::span<const compat::u8> stream,
    const std::size_t word_index,
    compat::u16& value
) noexcept {
    const std::size_t offset = static_cast<std::size_t>(word_index) * 2U;
    if (offset + 2U > stream.size()) {
        return false;
    }
    value = static_cast<compat::u16>(
        static_cast<compat::u16>(stream[offset]) |
        static_cast<compat::u16>(
            static_cast<compat::u16>(stream[offset + 1U]) << 8U
        )
    );
    return true;
}

[[nodiscard]] bool consume_word(
    LegacyActionRecord& record,
    const std::span<const compat::u8> stream,
    compat::u16& value
) noexcept {
    if (!read_word(stream, record.command_cursor, value)) {
        return false;
    }
    record.command_cursor =
        static_cast<compat::u16>(record.command_cursor + 1U);
    return true;
}

[[nodiscard]] bool consume_byte_operand(
    LegacyActionRecord& record,
    const std::span<const compat::u8> stream,
    compat::u8& value
) noexcept {
    const std::size_t offset =
        static_cast<std::size_t>(record.command_cursor) * 2U;
    if (offset >= stream.size()) {
        return false;
    }

    value = stream[offset];
    record.command_cursor =
        static_cast<compat::u16>(record.command_cursor + 1U);
    return true;
}

void reset_changed_key(
    LegacyActionRecord& record, const compat::u32 mode_mask
) noexcept {
    record.wait_remaining = 0U;
    record.wait_default = 0U;
    record.command_cursor = 0U;
    record.field_24 = 0U;
    record.field_28 = 0U;
    record.draw_offset_x = 0U;
    record.draw_offset_y = 0U;
    record.field_2c = 0U;
    record.field_30 = 0U;
    record.field_58 = 0U;
    record.field_5a = 0U;
    record.field_76 = 0U;
    record.field_78 = 0U;
    record.field_8a = 0U;
    record.field_88 = 0U;
    record.field_89 = 0U;
    record.field_62 = 0U;
    record.field_64 = 0U;
    record.field_66 = 0U;
    record.field_68 = 0U;
    record.field_7a = 0U;
    record.field_7c = 0U;
    record.field_7e = 0U;
    record.field_80 = 0U;
    record.field_82 = 0U;
    record.field_84 = 0U;
    record.field_86 = 0U;
    record.field_8c = 0U;
    record.external_mode = 0U;
    record.field_94 = 0U;
    record.mode_flags &= mode_mask;
}

void write_dx(
    std::optional<compat::u32>& edx, const compat::u16 value
) noexcept {
    if (edx.has_value()) {
        *edx = (*edx & 0xFFFF0000U) | value;
    }
}

void publish_command_edx(
    std::optional<compat::u32>& edx,
    const compat::u16 command,
    const compat::u16 operand_index,
    const LegacyActionRecord& record
) noexcept {
    // 4324FD..432505 also clears the high word for table-default entries.
    constexpr std::array<compat::u8, 21> dispatch{
        0, 7, 7, 1, 7, 7, 7, 7, 2, 3, 7, 7, 7, 7, 7, 4, 7, 7, 7, 5, 6,
    };
    if (command >= kCommandEa && command <= kCommandYa) {
        edx = dispatch[command - kCommandEa];
    }

    switch (command) {
    case kCommandEa:

    case kCommandTa:

    case kCommandXa:

    case kCommandYa:

    case kCommandSg:
        edx = operand_index;
        break;

    case kCommandMa:
        edx = record.mode_flags;
        break;

    case kCommandBc:
        edx = record.field_68;
        break;

    case kCommandGc:
        edx = record.field_66;
        break;

    case kCommandRc:
        edx = record.field_64;
        break;

    case kCommandLf:
        write_dx(edx, operand_index);
        if (edx.has_value()) {
            *edx += 7U;
        }

        break;

    case kCommandDl:
        edx = record.field_62;
        break;

    case kCommandAo:
        edx = record.field_50;
        break;

    case kCommandXo:
        edx = record.field_5e;
        break;

    case kCommandYo:
        edx = record.field_60;
        break;

    case kCommandAp:

    case kCommandNt:
        edx = record.packed_ap_state;
        break;

    case kCommandEq:
        edx = record.field_28;
        break;

    case kCommandFr:
        edx = record.field_4a;
        break;

    case kCommandOr:
        edx = record.field_4e;
        break;

    case kCommandDs:
        if ((record.wait_override & 0x8000U) != 0U) {
            edx = record.wait_override & 0x7FFFU;
        }

        break;

    case kCommandWt:
        edx = (operand_index & 0xFF00U) | record.field_88;
        break;

    case kCommandHw:
        edx = record.field_30;
        break;

    case kCommandVw:
        write_dx(edx, record.field_58);
        break;

    case kCommandYx:
        edx = record.draw_offset_y;
        break;

    default:
        break;
    }
}

[[nodiscard]] LegacyActionUpdateResult
malformed_result(const LegacyActionUpdateResult& base) noexcept {
    LegacyActionUpdateResult result = base;
    result.status = LegacyActionUpdateStatus::malformed_stream;
    result.return_value = 0U;
    result.return_edx.reset();
    return result;
}

}  // namespace

void initialize_legacy_action_record(LegacyActionRecord& record) noexcept {
    record.field_1c = 0xFFFFFFFFU;
    record.one_shot_base_variant = 0xFFFFFFFFU;
    record.one_shot_variant_delta = 0xFFFFFFFFU;
    record.wait_override = 0U;
    record.wait_default = 0U;
    record.wait_remaining = 0U;
    record.command_cursor = 0U;
    record.external_mode = 0U;
}

LegacyActActionStreamProvider::LegacyActActionStreamProvider(
    LegacyActRuntime& runtime
) noexcept
    : runtime_(runtime) {}

LegacyActionStreamLoadResult LegacyActActionStreamProvider::load_action_stream(
    const compat::u32 action_id,
    const compat::u32 variant_index,
    const bool cached
) {
    LegacyActionStreamLoadResult result;
    if (cached) {
        const LegacyActQueryResult loaded =
            runtime_.query_cached(action_id, variant_index);
        if (loaded.status != LegacyActRuntimeStatus::ready) {
            result.status = LegacyActionStreamStatus::load_stopped;
            result.stop = {loaded.status, loaded.physical_status};
            return result;
        }
        result.status = LegacyActionStreamStatus::ready;
        result.stream = loaded.stream;
        result.cache_hit = loaded.cache_hit;
        return result;
    }

    LegacyActDirectResult loaded =
        runtime_.load_direct(action_id, variant_index);
    if (loaded.status != LegacyActRuntimeStatus::ready) {
        result.status = LegacyActionStreamStatus::load_stopped;
        result.stop = {loaded.status, loaded.physical_status};
        return result;
    }
    direct_stream_ = std::move(loaded.stream);
    result.status = LegacyActionStreamStatus::ready;
    result.stream = direct_stream_;
    return result;
}

LegacyActionUpdater::LegacyActionUpdater(
    LegacyActionStreamProvider& provider
) noexcept
    : provider_(provider) {}

void LegacyActionUpdater::set_stream_cache_mode(
    const compat::u32 value
) noexcept {
    stream_cache_mode_ = value;
}

compat::u32 LegacyActionUpdater::stream_cache_mode() const noexcept {
    return stream_cache_mode_;
}

LegacyActionUpdateResult
LegacyActionUpdater::update(
    LegacyActionRecord& record, const std::optional<compat::u32> entry_edx
) {
    LegacyActionUpdateResult result;
    result.return_edx = entry_edx;
    if (record.external_mode == 1U && record.command_cursor != 0U) {
        return result;
    }
    if (record.action_id == 0U) {
        return result;
    }

    if (record.variant_delta != record.cached_variant_delta) {
        record.cached_variant_delta = record.variant_delta;
        reset_changed_key(record, 0x80000000U);
        result.key_changed = true;
    }
    if (record.base_variant != record.cached_base_variant) {
        record.cached_base_variant = record.base_variant;
        reset_changed_key(record, 0x80000003U);
        result.key_changed = true;
    }
    if (record.action_id != record.cached_action_id) {
        record.cached_action_id = record.action_id;
        reset_changed_key(record, 0x80000003U);
        result.key_changed = true;
    }

    const compat::u32 selected_variant =
        record.base_variant + record.variant_delta;
    const LegacyActionStreamLoadResult loaded = provider_.load_action_stream(
        record.action_id, selected_variant, stream_cache_mode_ == 1U
    );
    if (loaded.status == LegacyActionStreamStatus::load_stopped) {
        // 43243E stores +54 only after the loader has returned normally.
        // Preserve the key-reset prefix and the previous pointer on a stop.
        result.status = LegacyActionUpdateStatus::stream_load_stopped;
        result.return_value = 0U;
        result.stream_stop = loaded.stop;
        result.return_edx.reset();
        return result;
    }

    result.return_edx = loaded.return_edx;
    record.stream_pointer_32 =
        loaded.status == LegacyActionStreamStatus::ready ? 1U : 0U;
    result.cache_hit = loaded.cache_hit;
    if (loaded.status != LegacyActionStreamStatus::ready) {
        result.status = LegacyActionUpdateStatus::stream_load_failed;
        result.return_value = 0U;
        return result;
    }

    if (record.wait_remaining != 0U) {
        record.wait_remaining =
            static_cast<compat::u16>(record.wait_remaining - 1U);
        return result;
    }

    record.field_50 = 0U;
    std::size_t dispatch_count = 0U;
    const std::size_t safe_dispatch_limit = loaded.stream.size() + 1U;
    while (dispatch_count < safe_dispatch_limit) {
        ++dispatch_count;
        compat::u16 command{};
        write_dx(result.return_edx, record.command_cursor);
        if (result.return_edx.has_value()) {
            ++*result.return_edx;
        }

        if (!consume_word(record, loaded.stream, command)) {
            return malformed_result(result);
        }

        const bool rewind = command == kCommand2O ||
            ((command == kCommandDe || command == kCommandVo) &&
             record.external_mode == 1U);
        if (rewind && result.return_edx.has_value()) {
            --*result.return_edx;
        }

        if (command == kCommandDe) {
            if (record.external_mode == 1U) {
                record.command_cursor =
                    static_cast<compat::u16>(record.command_cursor - 1U);
            }
            return result;
        }
        if (command == kCommandVo) {
            if (record.external_mode == 1U) {
                record.command_cursor =
                    static_cast<compat::u16>(record.command_cursor - 1U);
            } else {
                record.command_cursor = 0U;
            }
            return result;
        }
        if (command == kCommand2O) {
            record.command_cursor =
                static_cast<compat::u16>(record.command_cursor - 1U);
            if (record.wait_remaining == 0U) {
                record.field_8c = 1U;
            }
            return result;
        }

        record.wait_remaining = record.wait_default;
        const bool has_wait_override = (record.wait_override & 0x8000U) != 0U;
        if (has_wait_override) {
            record.wait_remaining =
                static_cast<compat::u16>(record.wait_override & 0x7FFFU);
        }

        write_dx(result.return_edx, record.wait_override);
        const compat::u16 operand_index = record.command_cursor;
        compat::u16 first{};
        compat::u16 second{};
        compat::u8 byte_operand{};
        switch (command) {
        case kCommandEa:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }
            record.field_24 = first;
            break;
        case kCommandHa:
            record.mode_flags = (record.mode_flags & 0x8000000BU) | 0x08U;
            break;
        case kCommandMa:
            record.mode_flags = (record.mode_flags & 0x80000007U) | 0x04U;
            break;
        case kCommandNa:
            record.mode_flags = (record.mode_flags & 0x8000002FU) | 0x2CU;
            break;
        case kCommandTa:
            if (!consume_word(record, loaded.stream, record.field_5a)) {
                return malformed_result(result);
            }
            break;
        case kCommandXa:
            if (!consume_word(record, loaded.stream, record.field_76)) {
                return malformed_result(result);
            }
            break;
        case kCommandYa:
            if (!consume_word(record, loaded.stream, record.field_78)) {
                return malformed_result(result);
            }
            break;

        case kCommandBc:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }

            record.field_68 = first;
            if (!read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }

            record.field_74 = second;
            break;

        case kCommandGc:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }

            record.field_66 = first;
            if (!read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }

            record.field_72 = second;
            break;

        case kCommandLc:
            record.field_70 = 0U;
            record.field_72 = 0U;
            record.field_74 = 0U;
            break;

        case kCommandRc:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }

            record.field_64 = first;
            if (!read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }

            record.field_70 = second;
            break;

        case kCommandLf: {
            std::array<compat::u16*, 7> fields{
                &record.field_7a,
                &record.field_7c,
                &record.field_7e,
                &record.field_80,
                &record.field_82,
                &record.field_84,
                &record.field_86,
            };

            // 432662 masks the base once. The seven reads are physically
            // contiguous; only the final cursor store at 4326B7 truncates.
            const std::size_t first_word = record.command_cursor;
            for (std::size_t index = 0U; index < fields.size(); ++index) {
                if (!read_word(
                        loaded.stream, first_word + index, *fields[index]
                    )) {
                    return malformed_result(result);
                }
            }

            record.command_cursor =
                static_cast<compat::u16>(first_word + fields.size());
            break;
        }

        case kCommandSg:
            record.mode_flags = (record.mode_flags & 0x80000017U) | 0x14U;
            if (!consume_byte_operand(record, loaded.stream, byte_operand)) {
                return malformed_result(result);
            }

            record.field_8a = byte_operand;
            break;

        case kCommandDl:
            record.mode_flags = (record.mode_flags & 0x80000013U) | 0x10U;
            if (!consume_byte_operand(record, loaded.stream, byte_operand)) {
                return malformed_result(result);
            }

            record.field_62 = byte_operand;
            break;

        case kCommandOn:
            record.mode_flags &= 0xFFFFFFFEU;
            break;
        case kCommandAo:
            if (!consume_word(record, loaded.stream, record.field_50)) {
                return malformed_result(result);
            }
            break;
        case kCommandXo:
            if (!consume_word(record, loaded.stream, record.field_5e)) {
                return malformed_result(result);
            }
            break;
        case kCommandYo:
            if (!consume_word(record, loaded.stream, record.field_60)) {
                return malformed_result(result);
            }
            break;
        case kCommandAp: {
            if (!consume_word(record, loaded.stream, record.field_4c)) {
                return malformed_result(result);
            }
            const compat::u16 count =
                static_cast<compat::u16>(record.packed_ap_state & 0x00FFU);
            compat::u16 current =
                static_cast<compat::u16>((record.packed_ap_state >> 8U) + 1U);
            if (current > count) {
                current = 1U;
            }
            record.packed_ap_state = static_cast<compat::u16>(
                count | static_cast<compat::u16>(current << 8U)
            );
            break;
        }
        case kCommandEq:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }
            record.field_28 = first;
            break;
        case kCommandFr:
            if (!consume_word(record, loaded.stream, record.field_4a)) {
                return malformed_result(result);
            }
            break;
        case kCommandOr:
            if (!consume_word(record, loaded.stream, record.field_4e)) {
                return malformed_result(result);
            }
            break;
        case kCommandDs:
            if (!consume_word(record, loaded.stream, record.wait_default)) {
                return malformed_result(result);
            }
            record.wait_remaining = record.wait_default;
            if (has_wait_override) {
                record.wait_remaining =
                    static_cast<compat::u16>(record.wait_override & 0x7FFFU);
            }
            break;
        case kCommandMs:
            record.field_94 = 1U;
            break;
        case kCommandNt:
            record.packed_ap_state = 0U;
            if (!consume_word(record, loaded.stream, record.packed_ap_state)) {
                return malformed_result(result);
            }
            break;

        case kCommandWt:
            if (!consume_byte_operand(record, loaded.stream, byte_operand)) {
                return malformed_result(result);
            }

            record.field_88 = byte_operand;
            break;

        case kCommandIv:
            record.mode_flags |= 1U;
            break;

        case kCommandHw:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }

            record.field_2c = first;
            if (!read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }

            record.field_30 = second;
            break;

        case kCommandVw:
            if (!consume_word(record, loaded.stream, record.field_58)) {
                return malformed_result(result);
            }
            break;

        case kCommandYx:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }

            record.draw_offset_x = first;
            if (!read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }

            record.draw_offset_y = second;
            break;

        default:
            break;
        }

        publish_command_edx(
            result.return_edx, command, operand_index, record
        );
    }

    return malformed_result(result);
}

}  // namespace openswd3::asset_runtime
