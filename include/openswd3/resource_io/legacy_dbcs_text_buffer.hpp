#pragma once

#include "openswd3/compat/types.hpp"

namespace openswd3::resource_io {

struct LegacyDbcsTextBufferSnapshot {
    compat::i32 x{};
    compat::i32 y{};
    compat::i32 capacity{};
    compat::i32 cursor_byte_offset{};
    compat::i32 result{};
    compat::i32 ime_state{};
    compat::i32 input_enabled_state{};
};

struct LegacyDbcsTextBufferEditView {
    compat::u8* bytes{};
    compat::i32 capacity{};
    compat::i32* cursor_byte_offset{};
    compat::i32* result{};
    compat::i32* ime_state{};
    compat::i32* input_enabled_state{};
};

[[nodiscard]] compat::i32 legacy_cp950_next_character_offset(
    const compat::u8* text,
    compat::i32 current_offset
) noexcept;

[[nodiscard]] compat::i32 legacy_cp950_previous_character_offset(
    const compat::u8* text,
    compat::i32 current_offset
) noexcept;

[[nodiscard]] compat::i32 legacy_cp950_bounded_length(
    const compat::u8* text,
    compat::i32 maximum_bytes
) noexcept;

class LegacyDbcsTextBuffer final {
public:
    LegacyDbcsTextBuffer(
        const compat::u8* initial_text,
        compat::i32 capacity,
        compat::i32 x,
        compat::i32 y
    ) noexcept;
    ~LegacyDbcsTextBuffer();

    LegacyDbcsTextBuffer(const LegacyDbcsTextBuffer&) = delete;
    LegacyDbcsTextBuffer& operator=(const LegacyDbcsTextBuffer&) = delete;
    LegacyDbcsTextBuffer(LegacyDbcsTextBuffer&&) = delete;
    LegacyDbcsTextBuffer& operator=(LegacyDbcsTextBuffer&&) = delete;

    [[nodiscard]] compat::i32 result() const noexcept;
    [[nodiscard]] compat::i32 x() const noexcept;
    [[nodiscard]] compat::i32 y() const noexcept;
    [[nodiscard]] compat::i32 cursor_byte_offset() const noexcept;
    [[nodiscard]] compat::i32 copy_to(
        compat::u8* destination,
        compat::i32 destination_size
    ) const noexcept;
    [[nodiscard]] LegacyDbcsTextBufferSnapshot snapshot() const noexcept;
    [[nodiscard]] LegacyDbcsTextBufferEditView borrow_edit_view() noexcept;

private:
    compat::u8* buffer_{};
    compat::i32 x_{};
    compat::i32 y_{};
    compat::i32 capacity_{};
    compat::i32 cursor_byte_offset_{};
    compat::i32 result_{};
    compat::i32 ime_state_{};
    compat::i32 input_enabled_state_{};
};

}  // namespace openswd3::resource_io
