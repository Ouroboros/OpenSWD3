#include "openswd3/resource_io/legacy_dbcs_text_buffer.hpp"

#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>

namespace openswd3::resource_io {
namespace {

constexpr compat::u8 kCp950LeadFirst = 0x81U;
constexpr compat::u8 kCp950LeadLast = 0xFEU;

[[nodiscard]] const compat::u8* cp950_char_next(
    const compat::u8* const current
) noexcept {
    if (*current == 0U) {
        return current;
    }
    if (*current >= kCp950LeadFirst && *current <= kCp950LeadLast &&
        current[1] != 0U) {
        return current + 2;
    }
    return current + 1;
}

[[noreturn]] void terminate_legacy_fault() noexcept {
    std::abort();
}

}  // namespace

compat::i32 legacy_cp950_next_character_offset(
    const compat::u8* const text,
    const compat::i32 current_offset
) noexcept {
    if (text == nullptr || current_offset < 0) {
        terminate_legacy_fault();
    }

    const compat::u8* const current = text + current_offset;
    return static_cast<compat::i32>(cp950_char_next(current) - text);
}

compat::i32 legacy_cp950_previous_character_offset(
    const compat::u8* const text,
    const compat::i32 current_offset
) noexcept {
    if (text == nullptr) {
        terminate_legacy_fault();
    }

    if (current_offset <= 0) {
        return 0;
    }

    compat::i32 previous_offset = 0;
    compat::i32 scan_offset = 0;
    while (scan_offset < current_offset) {
        previous_offset = scan_offset;
        const compat::i32 next_offset =
            legacy_cp950_next_character_offset(text, scan_offset);
        if (next_offset >= current_offset || next_offset == scan_offset) {
            return previous_offset;
        }

        scan_offset = next_offset;
    }

    return previous_offset;
}

compat::i32 legacy_cp950_bounded_length(
    const compat::u8* const text,
    const compat::i32 maximum_bytes
) noexcept {
    if (text == nullptr) {
        terminate_legacy_fault();
    }

    const compat::u8* current = text;
    const compat::u8* next = cp950_char_next(current);
    compat::i32 accepted = 0;
    if (next == current) {
        return accepted;
    }

    for (;;) {
        const auto step = static_cast<compat::u32>(next - current);
        const compat::u32 candidate_bits =
            static_cast<compat::u32>(accepted) + step;
        const compat::i32 candidate =
            std::bit_cast<compat::i32>(candidate_bits);
        if (candidate > maximum_bytes) {
            return accepted;
        }

        accepted = candidate;
        current = next;
        next = cp950_char_next(current);
        if (next == current) {
            return accepted;
        }
    }
}

LegacyDbcsTextBuffer::LegacyDbcsTextBuffer(
    const compat::u8* const initial_text,
    const compat::i32 capacity,
    const compat::i32 x,
    const compat::i32 y
) noexcept
    : x_{x},
      y_{y},
      capacity_{capacity},
      cursor_byte_offset_{0xC0C0},
      result_{0},
      ime_state_{0},
      input_enabled_state_{1} {
    if (capacity_ < 0) {
        terminate_legacy_fault();
    }

    const auto allocation_size = static_cast<std::size_t>(
        static_cast<compat::u32>(capacity_) + 1U
    );
    buffer_ = static_cast<compat::u8*>(
        ::operator new(allocation_size, std::nothrow)
    );
    if (buffer_ == nullptr) {
        terminate_legacy_fault();
    }

    std::memset(buffer_, 0, allocation_size);
    cursor_byte_offset_ = legacy_cp950_bounded_length(
        initial_text,
        capacity_
    );
    if (cursor_byte_offset_ > 0) {
        std::memcpy(
            buffer_,
            initial_text,
            static_cast<std::size_t>(cursor_byte_offset_)
        );
    }
    cursor_byte_offset_ = 0;
}

LegacyDbcsTextBuffer::~LegacyDbcsTextBuffer() {
    ::operator delete(buffer_);
}

compat::i32 LegacyDbcsTextBuffer::result() const noexcept {
    return result_;
}

compat::i32 LegacyDbcsTextBuffer::x() const noexcept {
    return x_;
}

compat::i32 LegacyDbcsTextBuffer::y() const noexcept {
    return y_;
}

compat::i32 LegacyDbcsTextBuffer::cursor_byte_offset() const noexcept {
    return cursor_byte_offset_;
}

compat::i32 LegacyDbcsTextBuffer::copy_to(
    compat::u8* const destination,
    const compat::i32 destination_size
) const noexcept {
    if (destination_size < 0 ||
        (destination == nullptr && destination_size != 0)) {
        terminate_legacy_fault();
    }

    if (destination_size > 0) {
        std::memset(
            destination,
            0,
            static_cast<std::size_t>(destination_size)
        );
    }

    compat::i32 maximum_bytes = destination_size;
    if (maximum_bytes > capacity_) {
        maximum_bytes = capacity_;
    }
    const compat::i32 copied = legacy_cp950_bounded_length(
        buffer_,
        maximum_bytes
    );
    if (copied > 0) {
        std::memcpy(
            destination,
            buffer_,
            static_cast<std::size_t>(copied)
        );
    }
    return 1;
}

LegacyDbcsTextBufferSnapshot LegacyDbcsTextBuffer::snapshot() const noexcept {
    return LegacyDbcsTextBufferSnapshot{
        .x = x_,
        .y = y_,
        .capacity = capacity_,
        .cursor_byte_offset = cursor_byte_offset_,
        .result = result_,
        .ime_state = ime_state_,
        .input_enabled_state = input_enabled_state_,
    };
}

LegacyDbcsTextBufferEditView
LegacyDbcsTextBuffer::borrow_edit_view() noexcept {
    return LegacyDbcsTextBufferEditView{
        .bytes = buffer_,
        .capacity = capacity_,
        .cursor_byte_offset = &cursor_byte_offset_,
        .result = &result_,
        .ime_state = &ime_state_,
        .input_enabled_state = &input_enabled_state_,
    };
}

}  // namespace openswd3::resource_io
