#include <string>

#include "audio/audio_manager.h"
#include "platform/audio_device.h"
#include "test.h"
#include "text/dbcs.h"
#include "text/text_input.h"

using swd3::TextInput;
using swd3::TextKey;
using swd3::TextMessage;

namespace
{

int Key(TextInput& input, TextKey key, swd3::TextEntryState& entry, swd3::AudioManager& audio)
{
    return input.HandleMessage(TextMessage::KeyDown, static_cast<swd3::u32>(key), false, entry, audio, 11);
}

int Char(TextInput& input, swd3::u32 character, swd3::TextEntryState& entry, swd3::AudioManager& audio)
{
    return input.HandleMessage(TextMessage::Char, character, false, entry, audio, 11);
}

}  // namespace

TEST_CASE("text input: Big5 character stepping")
{
    const char text[] = "a\xA4\x40"
                        "b\xA4";
    CHECK(swd3::CharNext(text) == text + 1);
    CHECK(swd3::CharNext(text + 1) == text + 3);
    CHECK(swd3::CharNext(text + 4) == text + 5);
    CHECK(swd3::CharNext(text + 5) == text + 5);
    CHECK(swd3::CharPrev(text, text + 3) == text + 1);
    CHECK(swd3::CharPrev(text, text) == text);
    CHECK_EQ(swd3::TextFitLength(text, 2), 1u);
    CHECK_EQ(swd3::TextFitLength(text, 3), 3u);
}

TEST_CASE("text input: editing keeps whole characters within the capacity")
{
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    swd3::TextEntryState entry;
    TextInput input("\xA4\x40\xA4\x41xyz", 6, 30, 40);
    CHECK_EQ(input.GetX(), 30);
    CHECK_EQ(input.GetY(), 40);
    CHECK_EQ(input.GetCursor(), 0);
    CHECK(input.CopyText(64) == "\xA4\x40\xA4\x41xy");
    CHECK(input.CopyText(3) == "\xA4\x40");
    Key(input, TextKey::End, entry, audio);
    CHECK_EQ(input.GetCursor(), 6);
    Key(input, TextKey::Left, entry, audio);
    CHECK_EQ(input.GetCursor(), 5);
    Key(input, TextKey::Back, entry, audio);
    CHECK_EQ(input.GetCursor(), 4);
    CHECK(input.CopyText(64) == "\xA4\x40\xA4\x41y");
    Key(input, TextKey::Home, entry, audio);
    CHECK_EQ(input.GetCursor(), 0);
    Key(input, TextKey::Back, entry, audio);
    CHECK(input.CopyText(64) == "\xA4\x41y");
    CHECK_EQ(input.HandleMessage(TextMessage::ImeChar, 0xA442, false, entry, audio, 11), 0);
    CHECK(input.CopyText(64) == "\xA4\x42\xA4\x41y");
    CHECK_EQ(input.GetCursor(), 2);
    Char(input, 'q', entry, audio);
    CHECK(input.CopyText(64) == "\xA4\x42q\xA4\x41y");
    Char(input, 'r', entry, audio);
    CHECK(input.CopyText(64) == "\xA4\x42qr\xA4\x41");
    Key(input, TextKey::Return, entry, audio);
    CHECK_EQ(input.GetResult(), TextInput::kEnter);
    Key(input, TextKey::Escape, entry, audio);
    CHECK_EQ(input.GetResult(), TextInput::kEscape);
}

TEST_CASE("text input: lead bytes typed one by one, refused characters")
{
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    swd3::TextEntryState entry;
    TextInput input("", 8, 0, 0);
    Char(input, 0xA4, entry, audio);
    CHECK_EQ(entry.leadPending, 1);
    Char(input, 0xB0, entry, audio);
    CHECK_EQ(entry.leadPending, 1);
    Char(input, 0x40, entry, audio);
    CHECK_EQ(entry.leadPending, 0);
    CHECK(input.CopyText(64) == "\xA4\x40");
    Char(input, '%', entry, audio);
    Char(input, 0x10, entry, audio);
    CHECK(input.CopyText(64) == "\xA4\x40");
    CHECK_EQ(input.HandleMessage(TextMessage::InputLanguageChange, 0, true, entry, audio, 11), 1);
    Char(input, 'z', entry, audio);
    CHECK(input.CopyText(64) == "\xA4\x40");
    CHECK_EQ(input.SetInsertMode(0), 1);
    CHECK_EQ(input.GetInsertMode(), 1);
}
