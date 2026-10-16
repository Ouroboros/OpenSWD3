#include "text/text_input.h"

#include <cstring>

#include "crt/crt.h"

namespace swd3 {

using namespace w32;

// swd32.exe 0x004389D0: copies the whole characters of `text` that fit; the cursor starts at 0
TextInput::TextInput(const char* text, int capacity, int x, int y) {
    m_x = x;
    m_capacity = capacity;
    m_y = y;
    m_imeActive = 0;
    m_insert = 1;
    m_result = 0;
    m_cursor = 0xC0C0;
    m_text = static_cast<char*>(crt::operator_new(static_cast<std::uint32_t>(capacity + 1)));
    std::memset(m_text, 0, static_cast<std::size_t>(m_capacity + 1));
    const std::uint32_t length = TextFitLength(text, m_capacity);
    m_cursor = static_cast<std::int32_t>(length);
    std::memcpy(m_text, text, length);
    m_cursor = 0;
}

// swd32.exe 0x00438A60
int TextInput::GetResult() { return m_result; }

// swd32.exe 0x00438A70
TextInput::~TextInput() { crt::operator_delete(m_text); }

// swd32.exe 0x00438A80: bytes of the whole characters of `text` within `maxBytes`
std::uint32_t TextFitLength(const char* text, int maxBytes) {
    const char* current = text;
    std::int32_t length = 0;
    const char* next = CharNextA(text);
    while (next != current) {
        if (length + (next - current) > maxBytes) {
            break;
        }
        length += static_cast<std::int32_t>(next - current);
        current = next;
        next = CharNextA(next);
    }
    return static_cast<std::uint32_t>(length);
}

// swd32.exe 0x00438AC0
int TextInput::GetX() { return m_x; }

// swd32.exe 0x00438AD0
int TextInput::GetY() { return m_y; }

// swd32.exe 0x00438AE0
int TextInput::GetCursor() { return m_cursor; }

// swd32.exe 0x00438AF0
int TextInput::CopyText(char* out, int size) {
    std::memset(out, 0, static_cast<std::size_t>(size));
    int limit = size;
    if (size > m_capacity) {
        limit = m_capacity;
    }
    const std::uint32_t length = TextFitLength(m_text, limit);
    std::memcpy(out, m_text, length);
    return 1;
}

// swd32.exe 0x00438DC0: the argument is ignored, insert mode stays on
int TextInput::SetInsertMode(int) {
    m_insert = 1;
    return 1;
}

// swd32.exe 0x00438DD0
int TextInput::GetInsertMode() { return m_insert; }

// swd32.exe 0x00438DE0
BOOL TextInput::CursorLeft() {
    m_cursor = static_cast<std::int32_t>(CharPrevA(m_text, m_text + m_cursor) - m_text);
    return m_cursor != 0;
}

// swd32.exe 0x00438E10
int TextInput::CursorRight() {
    const char* next = CharNextA(m_text + m_cursor);
    if (next == m_text + m_cursor) {
        return 0;
    }
    m_cursor = static_cast<std::int32_t>(next - m_text);
    return 1;
}

// swd32.exe 0x00438E50: inserts what fits of `text` at the cursor, keeping what fits of the tail. Outside insert
// mode (never set) the original frees the stack slot that still holds `this`.
int TextInput::InsertText(const char* text) {
    if (m_insert != 1) {
        crt::operator_delete(this);
        return 1;
    }
    const std::uint32_t size = TextFitLength(text, 0x400) + static_cast<std::uint32_t>(m_capacity) + 1;
    auto* buffer = static_cast<char*>(crt::operator_new(size));
    std::memset(buffer, 0, size);
    std::memcpy(buffer, m_text, static_cast<std::size_t>(m_cursor));
    const std::int32_t cursor = m_cursor;
    const std::uint32_t inserted = TextFitLength(text, m_capacity - cursor);
    const std::int32_t end = cursor + static_cast<std::int32_t>(inserted);
    std::memcpy(buffer + cursor, text, inserted);
    const std::uint32_t tail = TextFitLength(m_text + m_cursor, m_capacity - end);
    std::memcpy(buffer + end, m_text + m_cursor, tail);
    m_cursor = end;
    std::memset(m_text, 0, static_cast<std::size_t>(m_capacity + 1));
    std::memcpy(m_text, buffer, static_cast<std::size_t>(end) + tail);
    crt::operator_delete(buffer);
    return 1;
}

// swd32.exe 0x00438F60: removes the character at the cursor (moves capacity - next + 1 bytes)
int TextInput::DeleteAtCursor() {
    char* at = m_text + m_cursor;
    char* next = CharNextA(at);
    TextFitLength(next, m_capacity);
    std::memmove(at, next, static_cast<std::size_t>(m_capacity - (next - m_text) + 1));
    return 1;
}

}  // namespace swd3
