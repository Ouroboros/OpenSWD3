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

[[nodiscard]] bool read_word(const std::span<const compat::u8> stream,
                             const compat::u16 word_index,
                             compat::u16& value) noexcept {
    const std::size_t offset = static_cast<std::size_t>(word_index) * 2U;
    if (offset + 2U > stream.size()) {
        return false;
    }
    value = static_cast<compat::u16>(
        static_cast<compat::u16>(stream[offset]) |
        static_cast<compat::u16>(static_cast<compat::u16>(stream[offset + 1U])
                                 << 8U));
    return true;
}

[[nodiscard]] bool consume_word(LegacyActionRecord& record,
                                const std::span<const compat::u8> stream,
                                compat::u16& value) noexcept {
    if (!read_word(stream, record.command_cursor, value)) {
        return false;
    }
    record.command_cursor =
        static_cast<compat::u16>(record.command_cursor + 1U);
    return true;
}

void reset_changed_key(LegacyActionRecord& record,
                       const compat::u32 mode_mask) noexcept {
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

[[nodiscard]] LegacyActionUpdateResult
malformed_result(const LegacyActionUpdateResult& base) noexcept {
    LegacyActionUpdateResult result = base;
    result.status = LegacyActionUpdateStatus::malformed_stream;
    result.return_value = 0U;
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
    LegacyActRuntime& runtime) noexcept
    : runtime_(runtime) {}

LegacyActionStreamLoadResult LegacyActActionStreamProvider::load_action_stream(
    const compat::u32 action_id, const compat::u32 variant_index,
    const bool cached) {
    LegacyActionStreamLoadResult result;
    if (cached) {
        const LegacyActQueryResult loaded =
            runtime_.query_cached(action_id, variant_index);
        if (loaded.status != LegacyActRuntimeStatus::ready) {
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
        direct_stream_.clear();
        return result;
    }
    direct_stream_ = std::move(loaded.stream);
    result.status = LegacyActionStreamStatus::ready;
    result.stream = direct_stream_;
    return result;
}

LegacyActionUpdater::LegacyActionUpdater(
    LegacyActionStreamProvider& provider) noexcept
    : provider_(provider) {}

void LegacyActionUpdater::set_stream_cache_mode(
    const compat::u32 value) noexcept {
    stream_cache_mode_ = value;
}

compat::u32 LegacyActionUpdater::stream_cache_mode() const noexcept {
    return stream_cache_mode_;
}

LegacyActionUpdateResult
LegacyActionUpdater::update(LegacyActionRecord& record) {
    LegacyActionUpdateResult result;
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
        record.action_id, selected_variant, stream_cache_mode_ == 1U);
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
        if (!consume_word(record, loaded.stream, command)) {
            return malformed_result(result);
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

        compat::u16 first{};
        compat::u16 second{};
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
            if (!consume_word(record, loaded.stream, first) ||
                !read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }
            record.field_68 = first;
            record.field_74 = second;
            break;
        case kCommandGc:
            if (!consume_word(record, loaded.stream, first) ||
                !read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }
            record.field_66 = first;
            record.field_72 = second;
            break;
        case kCommandLc:
            record.field_70 = 0U;
            record.field_72 = 0U;
            record.field_74 = 0U;
            break;
        case kCommandRc:
            if (!consume_word(record, loaded.stream, first) ||
                !read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }
            record.field_64 = first;
            record.field_70 = second;
            break;
        case kCommandLf: {
            std::array<compat::u16*, 7> fields{
                &record.field_7a, &record.field_7c, &record.field_7e,
                &record.field_80, &record.field_82, &record.field_84,
                &record.field_86,
            };
            for (compat::u16* const field : fields) {
                if (!consume_word(record, loaded.stream, *field)) {
                    return malformed_result(result);
                }
            }
            break;
        }
        case kCommandSg:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }
            record.mode_flags = (record.mode_flags & 0x80000017U) | 0x14U;
            record.field_8a = static_cast<compat::u8>(first);
            break;
        case kCommandDl:
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }
            record.mode_flags = (record.mode_flags & 0x80000013U) | 0x10U;
            record.field_62 = static_cast<compat::u8>(first);
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
                count | static_cast<compat::u16>(current << 8U));
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
            if (!consume_word(record, loaded.stream, first)) {
                return malformed_result(result);
            }
            record.field_88 = static_cast<compat::u8>(first);
            break;
        case kCommandIv:
            record.mode_flags |= 1U;
            break;
        case kCommandHw:
            if (!consume_word(record, loaded.stream, first) ||
                !read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }
            record.field_2c = first;
            record.field_30 = second;
            break;
        case kCommandVw:
            if (!consume_word(record, loaded.stream, record.field_58)) {
                return malformed_result(result);
            }
            break;
        case kCommandYx:
            if (!consume_word(record, loaded.stream, first) ||
                !read_word(loaded.stream, record.command_cursor, second)) {
                return malformed_result(result);
            }
            record.draw_offset_x = first;
            record.draw_offset_y = second;
            break;
        default:
            break;
        }
    }
    return malformed_result(result);
}

}  // namespace openswd3::asset_runtime
