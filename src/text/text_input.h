#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/types.h"

namespace swd3
{

class AudioManager;

// The window messages a text input handles (their Windows numbers).
enum class TextMessage : u32
{
    InputLanguageChange = 0x51,
    KeyDown = 0x100,
    Char = 0x102,
    ImeChar = 0x286,
};

// The keys a text input handles (Windows virtual-key codes).
enum class TextKey : u32
{
    Back = 0x08,
    Return = 0x0D,
    Escape = 0x1B,
    End = 0x23,
    Home = 0x24,
    Left = 0x25,
    Right = 0x27,
    Insert = 0x2D,
    Delete = 0x2E,
};

// Shared by every text input: a Big5 lead byte typed as a character of its own waits for its trail byte
// (0x004FB7E8).
struct TextEntryState
{
    i32 leadPending = 0;
};

// A one-line Big5 editor fed with window messages (0x20 bytes in the original).
class TextInput
{
public:
    static constexpr int kEnter = 1;
    static constexpr int kEscape = 2;
    static constexpr u16 kRejectedSound = 0x8C;

    TextInput(std::string_view text, int capacity, int x, int y);
    ~TextInput();

    int HandleMessage(
        TextMessage message,
        u32 parameter,
        bool imeLanguage,
        TextEntryState& entry,
        AudioManager& audio,
        int soundVolume
    );
    int GetResult() const;
    int GetX() const;
    int GetY() const;
    int GetCursor() const;
    std::string CopyText(int size) const;
    int SetInsertMode(int insert);
    int GetInsertMode() const;
    bool CursorLeft();
    int CursorRight();
    int InsertText(const char* text);
    int DeleteAtCursor();

private:
    std::vector<char> text;  // capacity + 1 bytes
    i32 x = 0;
    i32 y = 0;
    i32 capacity = 0;
    i32 cursor = 0;     // byte offset
    i32 result = 0;     // kEnter, kEscape
    i32 imeActive = 0;  // the input language is an IME
    i32 insert = 1;     // always 1
};

// The bytes of the whole characters of `text` within `maxBytes`.
u32 TextFitLength(const char* text, int maxBytes);

}  // namespace swd3
