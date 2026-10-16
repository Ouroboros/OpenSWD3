#include "text/text_input.h"

#include <algorithm>
#include <cstring>

#include "audio/audio_manager.h"
#include "text/dbcs.h"

namespace swd3
{

namespace
{

constexpr u32 kHighBit = 0x80;
constexpr u8 kFirstPrintable = 0x1F;
constexpr char kRejected = '%';

// A character from the bytes of a message: the bytes before the first NUL.
std::string CharacterText(char first, char second)
{
    std::string character;

    if (first != '\0')
    {
        character.push_back(first);

        if (second != '\0')
        {
            character.push_back(second);
        }
    }

    return character;
}

}  // namespace

// swd32.exe 0x004389D0: copies the whole characters of `text` that fit; the cursor starts at 0
TextInput::TextInput(std::string_view text, int capacity, int x, int y)
    : text(static_cast<std::size_t>(capacity) + 1, '\0'), x(x), y(y), capacity(capacity)
{
    const std::string_view source = text.substr(0, text.find('\0'));
    const u32 length = TextFitLength(source, this->capacity);
    std::copy_n(source.begin(), length, this->text.begin());
}

// swd32.exe 0x00438B50: Enter and Escape set the result; Backspace moves left and deletes (also at the start, where it
// deletes the first character). A character byte of 0x80 or more is inserted alone and waits for its trail byte; a
// second high byte while waiting is dropped. '%' is refused with a sound. Characters come only through WM_IME_CHAR
// while the input language is an IME.
int TextInput::HandleMessage(
    TextMessage message,
    u32 parameter,
    bool imeLanguage,
    TextEntryState& entry,
    AudioManager& audio,
    int soundVolume
)
{
    switch (message)
    {
        case TextMessage::ImeChar:
        {
            this->InsertText(CharacterText(static_cast<char>(parameter >> 8), static_cast<char>(parameter)));
            return 0;
        }

        case TextMessage::Char:
        {
            if (this->imeActive != 0)
            {
                return 1;
            }

            const std::string character = CharacterText(static_cast<char>(parameter), '\0');

            if (entry.leadPending == 1)
            {
                if ((parameter & kHighBit) == 0)
                {
                    this->InsertText(character);
                    entry.leadPending = 0;
                }

                return 1;
            }

            if ((parameter & kHighBit) != 0)
            {
                this->InsertText(character);
                entry.leadPending = 1;
                return 1;
            }

            if (static_cast<i8>(parameter) >= static_cast<i8>(kFirstPrintable))
            {
                if (static_cast<char>(parameter) == kRejected)
                {
                    audio.PlaySound(TextInput::kRejectedSound, soundVolume);
                    return 1;
                }

                this->InsertText(character);
                entry.leadPending = 0;
            }

            return 1;
        }

        case TextMessage::InputLanguageChange:
            this->imeActive = imeLanguage ? 1 : 0;
            return 1;

        case TextMessage::KeyDown:
            break;

        default:
            return 1;
    }

    switch (static_cast<TextKey>(parameter))
    {
        case TextKey::Back:
            this->CursorLeft();
            return this->DeleteAtCursor();

        case TextKey::Return:
            this->result = TextInput::kEnter;
            return 1;

        case TextKey::Escape:
            this->result = TextInput::kEscape;
            return 1;

        case TextKey::End:
            if (this->CursorRight() == 1)
            {
                while (this->CursorRight() == 1)
                {
                }
            }

            return 1;

        case TextKey::Home:
            if (this->CursorLeft())
            {
                while (this->CursorLeft())
                {
                }
            }

            return 1;

        case TextKey::Left:
            this->CursorLeft();
            return 1;

        case TextKey::Right:
            this->CursorRight();
            return 1;

        case TextKey::Insert:
            this->SetInsertMode(this->GetInsertMode() == 0 ? 1 : 0);
            return 1;

        case TextKey::Delete:
            return this->DeleteAtCursor();

        default:
            return 1;
    }
}

// swd32.exe 0x00438A70
TextInput::~TextInput() = default;

// swd32.exe 0x00438A60
int TextInput::GetResult() const
{
    return this->result;
}

// swd32.exe 0x00438A80
u32 TextFitLength(std::string_view text, int maxBytes)
{
    std::size_t current = 0;
    i32 length = 0;
    std::size_t next = CharNext(text, 0);

    while (next != current)
    {
        if (length + static_cast<i32>(next - current) > maxBytes)
        {
            break;
        }

        length += static_cast<i32>(next - current);
        current = next;
        next = CharNext(text, next);
    }

    return static_cast<u32>(length);
}

// swd32.exe 0x00438AC0
int TextInput::GetX() const
{
    return this->x;
}

// swd32.exe 0x00438AD0
int TextInput::GetY() const
{
    return this->y;
}

// swd32.exe 0x00438AE0
int TextInput::GetCursor() const
{
    return this->cursor;
}

// swd32.exe 0x00438AF0: the whole characters within `size` bytes (and the capacity)
std::string TextInput::CopyText(int size) const
{
    const int limit = std::min(size, this->capacity);
    const std::string_view contents = this->Contents();
    return std::string(contents.substr(0, TextFitLength(contents, limit)));
}

// swd32.exe 0x00438DC0: the argument is ignored, insert mode stays on
int TextInput::SetInsertMode(int)
{
    this->insert = 1;
    return 1;
}

// swd32.exe 0x00438DD0
int TextInput::GetInsertMode() const
{
    return this->insert;
}

// swd32.exe 0x00438DE0
bool TextInput::CursorLeft()
{
    this->cursor = static_cast<i32>(CharPrev(this->Contents(), static_cast<std::size_t>(this->cursor)));
    return this->cursor != 0;
}

// swd32.exe 0x00438E10
int TextInput::CursorRight()
{
    const auto at = static_cast<std::size_t>(this->cursor);
    const std::size_t next = CharNext(this->Contents(), at);

    if (next == at)
    {
        return 0;
    }

    this->cursor = static_cast<i32>(next);
    return 1;
}

// swd32.exe 0x00438E50: inserts what fits of `text` at the cursor and keeps what then fits of the tail. Outside insert
// mode (never set) the original frees the stack slot holding `this`; that path is not reproduced.
int TextInput::InsertText(std::string_view text)
{
    if (this->insert != 1)
    {
        return 1;
    }

    const std::string contents(this->Contents());
    const auto start = static_cast<std::size_t>(this->cursor);
    const u32 inserted = TextFitLength(text, this->capacity - this->cursor);
    const std::size_t end = start + inserted;
    const std::string_view tail = std::string_view(contents).substr(std::min(start, contents.size()));
    const u32 kept = TextFitLength(tail, this->capacity - static_cast<i32>(end));
    std::string result = contents.substr(0, start);
    result.resize(start, '\0');
    result.append(text.substr(0, inserted));
    result.append(tail.substr(0, kept));
    this->cursor = static_cast<i32>(end);
    std::fill(this->text.begin(), this->text.end(), '\0');
    std::copy(result.begin(), result.end(), this->text.begin());
    return 1;
}

// swd32.exe 0x00438F60: removes the character at the cursor
int TextInput::DeleteAtCursor()
{
    const auto at = static_cast<std::size_t>(this->cursor);
    const std::size_t next = CharNext(this->Contents(), at);
    const auto moved = static_cast<std::size_t>(this->capacity) - next + 1;
    std::copy_n(
        this->text.begin() + static_cast<std::ptrdiff_t>(next),
        moved,
        this->text.begin() + static_cast<std::ptrdiff_t>(at)
    );
    return 1;
}

// The text up to its NUL.
std::string_view TextInput::Contents() const
{
    const std::string_view all(this->text.data(), this->text.size());
    return all.substr(0, all.find('\0'));
}

}  // namespace swd3
