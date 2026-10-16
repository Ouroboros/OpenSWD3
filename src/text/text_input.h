#pragma once

#include <cstdint>

#include "swd3/w32.h"

namespace swd3 {

// Result of a TextInput after Enter / Escape.
constexpr int kTextInputEnter = 1;
constexpr int kTextInputEscape = 2;

// Big5 line editor fed with window messages (size 0x20).
class TextInput {
public:
    TextInput(const char* text, int capacity, int x, int y);
    ~TextInput();
    int GetResult();
    int GetX();
    int GetY();
    int GetCursor();
    int CopyText(char* out, int size);
    int SetInsertMode(int insert);
    int GetInsertMode();
    w32::BOOL CursorLeft();
    int CursorRight();
    int InsertText(const char* text);
    int DeleteAtCursor();

    char* m_text;          // +0x00 capacity + 1 bytes
    std::int32_t m_x;      // +0x04
    std::int32_t m_y;      // +0x08
    std::int32_t m_capacity;  // +0x0C bytes
    std::int32_t m_cursor;    // +0x10 byte offset
    std::int32_t m_result;    // +0x14 kTextInputEnter / kTextInputEscape
    std::int32_t m_imeActive; // +0x18
    std::int32_t m_insert;    // +0x1C always 1
};

static_assert(sizeof(TextInput) == 0x20);

std::uint32_t TextFitLength(const char* text, int maxBytes);

}  // namespace swd3
