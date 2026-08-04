#include "openswd3/input_time_rng/legacy_text_input.hpp"

#include "openswd3/resource_io/legacy_dbcs_text_buffer.hpp"

#include <cstdlib>
#include <cstring>
#include <vector>

namespace openswd3::input_time_rng {

namespace {

constexpr compat::u32 kLegacyKeyBackspace = 0x08U;
constexpr compat::u32 kLegacyKeyEnter = 0x0DU;
constexpr compat::u32 kLegacyKeyEscape = 0x1BU;
constexpr compat::u32 kLegacyKeyEnd = 0x23U;
constexpr compat::u32 kLegacyKeyHome = 0x24U;
constexpr compat::u32 kLegacyKeyLeft = 0x25U;
constexpr compat::u32 kLegacyKeyRight = 0x27U;
constexpr compat::u32 kLegacyKeyInsert = 0x2DU;
constexpr compat::u32 kLegacyKeyDelete = 0x2EU;

[[noreturn]] void terminate_legacy_fault() noexcept {
    std::abort();
}

[[nodiscard]] resource_io::LegacyDbcsTextBufferEditView edit_view(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept {
    return buffer.borrow_edit_view();
}

[[nodiscard]] compat::u32 insert_single_byte(
    resource_io::LegacyDbcsTextBuffer& buffer,
    const compat::u8 byte
) {
    const compat::u8 text[2]{byte, 0U};
    return legacy_insert_text_bytes(buffer, text);
}

[[nodiscard]] compat::u32 handle_key_down(
    resource_io::LegacyDbcsTextBuffer& buffer,
    const compat::u32 key
) {
    auto view = edit_view(buffer);

    switch (key) {
    case kLegacyKeyHome:
        while (legacy_move_text_cursor_previous(buffer) == 1U) {
        }
        return 1U;

    case kLegacyKeyEnd:
        while (legacy_move_text_cursor_next(buffer) == 1U) {
        }
        return 1U;

    case kLegacyKeyInsert: {
        const compat::i32 requested_state =
            legacy_text_input_enabled(buffer) == 0 ? 1 : 0;
        static_cast<void>(
            legacy_set_text_input_enabled(buffer, requested_state)
        );
        return 1U;
    }

    case kLegacyKeyLeft:
        static_cast<void>(legacy_move_text_cursor_previous(buffer));
        return 1U;

    case kLegacyKeyRight:
        static_cast<void>(legacy_move_text_cursor_next(buffer));
        return 1U;

    case kLegacyKeyBackspace:
        static_cast<void>(legacy_move_text_cursor_previous(buffer));
        [[fallthrough]];

    case kLegacyKeyDelete:
        static_cast<void>(legacy_delete_text_at_cursor(buffer));
        return 1U;

    case kLegacyKeyEnter:
        *view.result = 1;
        return 1U;

    case kLegacyKeyEscape:
        *view.result = 2;
        return 1U;

    default:
        return 1U;
    }
}

[[nodiscard]] compat::u32 handle_character(
    resource_io::LegacyDbcsTextBuffer& buffer,
    LegacyTextInputDriverState& state,
    const compat::u32 first_parameter,
    LegacyTextInputPorts& ports
) {
    auto view = edit_view(buffer);
    if (*view.ime_state != 0) {
        return 1U;
    }

    const auto character = static_cast<compat::u8>(first_parameter);
    const bool negative_signed_byte = (character & 0x80U) != 0U;
    if (state.dbcs_lead_byte_latch == 1U) {
        if (negative_signed_byte) {
            return 1U;
        }

        static_cast<void>(insert_single_byte(buffer, character));
        state.dbcs_lead_byte_latch = 0U;
        return 1U;
    }

    if (negative_signed_byte) {
        static_cast<void>(insert_single_byte(buffer, character));
        state.dbcs_lead_byte_latch = 1U;
        return 1U;
    }

    if (character < 0x1FU) {
        return 1U;
    }

    if (character == 0x25U) {
        ports.play_sound_effect(kLegacyPercentInputSoundId);
        return 1U;
    }

    static_cast<void>(insert_single_byte(buffer, character));
    state.dbcs_lead_byte_latch = 0U;
    return 1U;
}

}  // namespace

compat::i32 legacy_text_input_enabled(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept {
    return *edit_view(buffer).input_enabled_state;
}

compat::i32 legacy_set_text_input_enabled(
    resource_io::LegacyDbcsTextBuffer& buffer,
    const compat::i32 requested_state
) noexcept {
    static_cast<void>(requested_state);
    auto view = edit_view(buffer);
    *view.input_enabled_state = 1;
    return 1;
}

compat::u32 legacy_move_text_cursor_previous(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept {
    auto view = edit_view(buffer);
    *view.cursor_byte_offset =
        resource_io::legacy_cp950_previous_character_offset(
            view.bytes,
            *view.cursor_byte_offset
        );
    return *view.cursor_byte_offset != 0 ? 1U : 0U;
}

compat::u32 legacy_move_text_cursor_next(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept {
    auto view = edit_view(buffer);
    const compat::i32 previous_offset = *view.cursor_byte_offset;
    const compat::i32 next_offset =
        resource_io::legacy_cp950_next_character_offset(
            view.bytes,
            previous_offset
        );
    if (next_offset == previous_offset) {
        return 0U;
    }

    *view.cursor_byte_offset = next_offset;
    return 1U;
}

compat::u32 legacy_insert_text_bytes(
    resource_io::LegacyDbcsTextBuffer& buffer,
    const compat::u8* const text
) {
    auto view = edit_view(buffer);
    if (*view.input_enabled_state != 1) {
        terminate_legacy_fault();
    }

    const compat::i32 source_length =
        resource_io::legacy_cp950_bounded_length(text, 0x400);
    const auto allocation_size = static_cast<std::size_t>(
        static_cast<compat::u32>(source_length) +
        static_cast<compat::u32>(view.capacity) + 1U
    );
    std::vector<compat::u8> temporary(allocation_size, 0U);

    const compat::i32 old_cursor = *view.cursor_byte_offset;
    if (old_cursor > 0) {
        std::memcpy(
            temporary.data(),
            view.bytes,
            static_cast<std::size_t>(old_cursor)
        );
    }

    const compat::i32 inserted_length =
        resource_io::legacy_cp950_bounded_length(
            text,
            view.capacity - old_cursor
        );
    const compat::i32 new_cursor = old_cursor + inserted_length;
    if (inserted_length > 0) {
        std::memcpy(
            temporary.data() + old_cursor,
            text,
            static_cast<std::size_t>(inserted_length)
        );
    }

    const compat::i32 suffix_length =
        resource_io::legacy_cp950_bounded_length(
            view.bytes + old_cursor,
            view.capacity - new_cursor
        );
    if (suffix_length > 0) {
        std::memcpy(
            temporary.data() + new_cursor,
            view.bytes + old_cursor,
            static_cast<std::size_t>(suffix_length)
        );
    }

    *view.cursor_byte_offset = new_cursor;
    std::memset(
        view.bytes,
        0,
        static_cast<std::size_t>(view.capacity) + 1U
    );
    const compat::i32 copied_length = new_cursor + suffix_length;
    if (copied_length > 0) {
        std::memcpy(
            view.bytes,
            temporary.data(),
            static_cast<std::size_t>(copied_length)
        );
    }

    return 1U;
}

compat::u32 legacy_delete_text_at_cursor(
    resource_io::LegacyDbcsTextBuffer& buffer
) noexcept {
    auto view = edit_view(buffer);
    const compat::i32 cursor = *view.cursor_byte_offset;
    const compat::i32 next =
        resource_io::legacy_cp950_next_character_offset(view.bytes, cursor);
    static_cast<void>(resource_io::legacy_cp950_bounded_length(
        view.bytes + next,
        view.capacity
    ));

    const compat::i32 moved_size = view.capacity - next + 1;
    std::memmove(
        view.bytes + cursor,
        view.bytes + next,
        static_cast<std::size_t>(moved_size)
    );
    return 1U;
}

compat::u32 filter_legacy_text_input_message(
    resource_io::LegacyDbcsTextBuffer& buffer,
    LegacyTextInputDriverState& state,
    const compat::u32 message,
    const compat::u32 first_parameter,
    const compat::u32 second_parameter,
    LegacyTextInputPorts& ports
) {
    if (message == kLegacyKeyDownMessage) {
        return handle_key_down(buffer, first_parameter);
    }

    if (message == kLegacyInputLanguageChangeMessage) {
        auto view = edit_view(buffer);
        *view.ime_state = 0;
        if (ports.is_ime_keyboard_layout(second_parameter)) {
            *view.ime_state = 1;
        }
        return 1U;
    }

    if (message == kLegacyCharacterMessage) {
        return handle_character(buffer, state, first_parameter, ports);
    }

    if (message == kLegacyImeCharacterMessage) {
        const compat::u8 text[3]{
            static_cast<compat::u8>(first_parameter >> 8U),
            static_cast<compat::u8>(first_parameter),
            0U,
        };
        static_cast<void>(legacy_insert_text_bytes(buffer, text));
        return 0U;
    }

    return 1U;
}

}  // namespace openswd3::input_time_rng
