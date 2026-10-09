#include "openswd3/battle/legacy_battle_dialog_text.hpp"

#include "openswd3/compat/legacy_decimal.hpp"
#include "openswd3/story_scene/legacy_dialog_text.hpp"

#include <algorithm>
#include <bit>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;
using Status = LegacyBattleDialogTextStatus;

constexpr std::array<u8, 5U> kFirstPattern{0xC1U, 0xC9U, 0xAFU, 0x53U, 0U};
constexpr std::array<u8, 5U> kSecondPattern{0xA9U, 0x67U, 0xA5U, 0x69U, 0U};

[[nodiscard]] std::optional<u32>
string_length(const std::span<const u8> bytes) {
    const auto end = std::ranges::find(bytes, u8{});
    if (end == bytes.end()) {
        return std::nullopt;
    }

    return static_cast<u32>(end - bytes.begin());
}

[[nodiscard]] bool release_description(
    LegacyBattleMonDefinitionOwner& definition,
    LegacyBattleMonDatabasePort& mon,
    LegacyBattleDialogTextResult& result
) {
    const auto& bytes = definition.bytes;
    const u32 token = static_cast<u32>(bytes[0xA0U]) |
        (static_cast<u32>(bytes[0xA1U]) << 8U) |
        (static_cast<u32>(bytes[0xA2U]) << 16U) |
        (static_cast<u32>(bytes[0xA3U]) << 24U);
    ++result.release_calls;
    try {
        mon.release_mon_text(token);
    } catch (const std::invalid_argument&) {
        result.status = Status::mon_release_typed_stop;
        return false;
    }

    definition.description.clear();
    std::fill(definition.bytes.begin() + 0xA0U, definition.bytes.end(), 0U);
    return true;
}

}  // namespace

LegacyBattleDialogTextResult prepare_legacy_battle_dialog_text(
    const std::span<u8, 512U> text,
    const std::span<const u8> first_name,
    const std::span<const u8> second_name,
    LegacyBattleDialogTextState& state,
    LegacyBattleMonDatabasePort& mon
) {
    using story_scene::LegacyDialogTextReplaceStatus;
    using story_scene::replace_legacy_dialog_text_prefix;
    LegacyBattleDialogTextResult result;
    LegacyBattleMonDefinitionOwner definition;
    auto& cursor = result.cursor;
    for (;;) {
        if (cursor >= text.size() - 1U) {
            result.status = Status::memory_access_typed_stop;
            return result;
        }

        if ((text[cursor] == '%' && text[cursor + 1U] == 'Q') ||
            text[cursor] == 0U) {
            break;
        }

        if (text[cursor] == '%' && text[cursor + 1U] == 'T') {
            auto& scratch = state.reference_name;
            scratch[0U] = '%';
            scratch[1U] = 'T';
            u32 length{2U};
            for (;;) {
                if (cursor + length >= text.size() ||
                    length >= scratch.size()) {
                    result.status = Status::memory_access_typed_stop;
                    return result;
                }

                if (text[cursor + length] == '.') {
                    break;
                }

                scratch[length] = text[cursor + length];
                ++length;
            }

            scratch[length] = 0U;
            if (length >= 7U) {
                scratch[length] = '.';
                if (length + 1U >= scratch.size()) {
                    result.status = Status::memory_access_typed_stop;
                    return result;
                }

                scratch[length + 1U] = 0U;
                ++result.diagnostics;
            }

            const auto number_size =
                string_length(std::span<const u8>{scratch}.subspan(2U));
            if (!number_size.has_value()) {
                result.status = Status::memory_access_typed_stop;
                return result;
            }

            compat::i32 number{};
            const auto parsed = compat::parse_legacy_decimal_contract(
                std::string_view{
                    reinterpret_cast<const char*>(scratch.data() + 2U),
                    *number_size
                },
                number
            );
            if (parsed ==
                compat::LegacyDecimalParseResult::legacy_fault_no_digits) {
                result.status = Status::decimal_typed_stop;
                return result;
            }

            if (parsed != compat::LegacyDecimalParseResult::success) {
                scratch[length] = '.';
                scratch[length + 1U] = 0U;
                ++result.diagnostics;
                cursor += 4U;
                continue;
            }

            if (!release_description(definition, mon, result)) {
                return result;
            }

            const auto loaded = load_legacy_battle_mon_definition(
                definition.bytes,
                definition.description,
                mon,
                {.path = "mon.dat", .definition_id = std::bit_cast<u32>(number)}
            );
            ++result.mon_load_calls;
            if (legacy_battle_mon_definition_load_stopped(loaded.status)) {
                result.status = Status::mon_load_typed_stop;
                return result;
            }

            if (!loaded.definition_found) {
                ++result.diagnostics;
                if (!release_description(definition, mon, result)) {
                    return result;
                }

                cursor += 4U;
                continue;
            }

            scratch[length] = '.';
            scratch[length + 1U] = 0U;
            const auto replaced = replace_legacy_dialog_text_prefix(
                text.subspan(cursor), scratch, definition.bytes, 510U - cursor
            );
            if (replaced ==
                LegacyDialogTextReplaceStatus::memory_access_typed_stop) {
                result.status = Status::memory_access_typed_stop;
                return result;
            }

            const auto replacement_size = string_length(definition.bytes);
            if (!replacement_size.has_value()) {
                result.status = Status::memory_access_typed_stop;
                return result;
            }

            cursor += *replacement_size;
            continue;
        }

        auto replaced = replace_legacy_dialog_text_prefix(
            text.subspan(cursor), kFirstPattern, first_name, 510U - cursor
        );
        if (replaced == LegacyDialogTextReplaceStatus::no_match) {
            replaced = replace_legacy_dialog_text_prefix(
                text.subspan(cursor), kSecondPattern, second_name, 510U - cursor
            );
        }

        if (replaced ==
            LegacyDialogTextReplaceStatus::memory_access_typed_stop) {
            result.status = Status::memory_access_typed_stop;
            return result;
        }

        if (replaced == LegacyDialogTextReplaceStatus::replaced) {
            // 40B9CC uses the FIRST name even after the second pattern matched.
            const auto advance = string_length(first_name);
            if (!advance.has_value()) {
                result.status = Status::memory_access_typed_stop;
                return result;
            }

            cursor += *advance;
        } else {
            ++cursor;
        }
    }

    if (!release_description(definition, mon, result)) {
        return result;
    }

    const auto metrics = story_scene::measure_legacy_dialog_prepared_text(text);
    if (!metrics.has_value()) {
        result.status = Status::memory_access_typed_stop;
        return result;
    }

    result.metrics = *metrics;
    return result;
}

LegacyBattleDialogFormatResult format_legacy_battle_dialog(
    story_scene::LegacyDialogMessage& message,
    const LegacyBattleDialogFormatRequest& request,
    const LegacyBattleDialogFormatBindings bindings,
    LegacyBattleDialogFormatPort& port
) {
    using FormatStatus = LegacyBattleDialogFormatStatus;
    using compat::u16;
    LegacyBattleDialogFormatResult result;
    auto& record = message.record;
    if (record.flags != 0x800U ||
        (request.mode != 1U && request.mode != 0x10000U)) {
        result.status = FormatStatus::unsupported_call;
        return result;
    }

    const auto word = [&](const std::size_t offset) -> std::optional<u16> {
        if (offset >= request.payload.size() ||
            request.payload.size() - offset < 2U) {
            result.status = FormatStatus::memory_access_typed_stop;
            return std::nullopt;
        }

        return static_cast<u16>(request.payload[offset]) |
            static_cast<u16>(
                   static_cast<u16>(request.payload[offset + 1U]) << 8U
            );
    };
    const std::size_t text_start = request.mode == 1U ? 12U : 4U;
    std::size_t end = text_start;
    for (;;) {
        const auto marker = word(end);
        if (!marker.has_value()) {
            return result;
        }

        if (*marker == 0x5125U) {
            break;
        }

        ++end;
    }

    const u32 text_token = port.allocate_battle_dialog_storage(512U);
    ++result.allocations;
    record.text_allocation_pointer_32 = text_token;
    record.text_cursor_pointer_32 = text_token;
    record.page_stop_pointer_32 = text_token;
    if (text_token == 0U) {
        result.status = FormatStatus::allocation_typed_stop;
        return result;
    }

    message.text.assign(512U, 0U);
    const auto copied = end - text_start + 2U;
    if (copied > message.text.size()) {
        result.status = FormatStatus::memory_access_typed_stop;
        return result;
    }

    std::copy_n(
        request.payload.data() + text_start, copied, message.text.data()
    );
    result.preparation = prepare_legacy_battle_dialog_text(
        std::span<u8, 512U>{message.text.data(), 512U},
        bindings.first_name,
        bindings.second_name,
        bindings.text_state,
        port
    );
    if (result.preparation.status != LegacyBattleDialogTextStatus::completed) {
        result.status = FormatStatus::text_preparation_typed_stop;
        return result;
    }

    if (request.mode == 0x10000U) {
        record.width = static_cast<u16>(
            bindings.scale * (result.preparation.metrics & 0xFFFFU)
        );
        record.height = static_cast<u16>(bindings.scale + bindings.scale);
    } else {
        const auto width = word(8U);
        if (!width.has_value()) {
            return result;
        }

        record.width = static_cast<u16>(*width * bindings.scale);
        const auto height = word(10U);
        if (!height.has_value()) {
            return result;
        }

        record.height = static_cast<u16>(*height * bindings.scale);
    }

    if (!word(0U).has_value()) {
        return result;
    }

    const auto frame_id = word(2U);
    if (!frame_id.has_value()) {
        return result;
    }

    std::size_t slot{};
    for (; slot < bindings.frame_actions.size(); ++slot) {
        if (bindings.frame_actions[slot].action_id == *frame_id) {
            break;
        }
    }

    if (slot == bindings.frame_actions.size()) {
        ++result.diagnostics;
        slot = 0U;
    }

    message.frame_action = &bindings.frame_actions[slot];
    record.frame_action_pointer_32 =
        0x004CAA78U + static_cast<u32>(slot) * 0x98U;
    ++result.action_updates;
    if (!port.update_battle_dialog_action(*message.frame_action)) {
        ++result.diagnostics;
    }

    if (request.caption.empty()) {
        result.status = FormatStatus::memory_access_typed_stop;
        return result;
    }

    if (request.caption[0U] != 0U) {
        message.caption_action = &bindings.caption_actions[slot];
        record.caption_action_pointer_32 =
            0x004A9120U + static_cast<u32>(slot) * 0x98U;
        const u32 caption_token = port.allocate_battle_dialog_storage(32U);
        ++result.allocations;
        record.caption_pointer_32 = caption_token;
        if (caption_token == 0U) {
            result.status = FormatStatus::allocation_typed_stop;
            return result;
        }

        const auto length = string_length(request.caption);
        if (!length.has_value() || *length >= 32U) {
            result.status = FormatStatus::memory_access_typed_stop;
            return result;
        }

        message.caption.assign(
            request.caption.begin(), request.caption.begin() + *length + 1U
        );
        ++result.action_updates;
        if (!port.update_battle_dialog_action(*message.caption_action)) {
            ++result.diagnostics;
        }
    }

    record.role_index = 0xFFFDU;
    record.transition_step = 0U;
    record.field_26 = 0U;
    record.character_countdown = 0U;
    record.character_delay = static_cast<u16>(
        bindings.character_delay_base + bindings.character_delay_base
    );
    record.foreground_index = 4U;
    record.secondary_index = 4U;
    record.text_style = 4U;
    record.saved_text_style = 0U;
    record.flags |= 0x800U;
    record.next_pointer_32 = 0U;
    if ((record.flags & 0x80U) != 0U) {
        record.foreground_index = 5U;
        record.secondary_index = 5U;
    }

    const auto left = word(4U);
    if (!left.has_value()) {
        return result;
    }

    record.left = *left;
    const auto top = word(6U);
    if (!top.has_value()) {
        return result;
    }

    record.top = *top;
    if (request.mode == 0x10000U) {
        // 40B472..40B4F3 uses Y for LEFT and the inverted upper-edge tests.
        const u16 height = record.height;
        record.left = static_cast<u16>(request.y - height - 16U);
        if (record.left < 16U) {
            record.left = 16U;
        }

        if (record.top < 16U) {
            record.top = 16U;
        }

        if (static_cast<u32>(record.left) + record.width < 608U) {
            record.left = static_cast<u16>(608U - record.width);
        }

        if (static_cast<u32>(record.top) + height < 464U) {
            record.top = static_cast<u16>(464U - height);
        }
    }

    return result;
}

}  // namespace openswd3::battle
