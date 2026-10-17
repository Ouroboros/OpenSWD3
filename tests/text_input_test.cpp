#include <climits>
#include <string>

#include "audio/audio_manager.h"
#include "config/config.h"
#include "platform/audio_device.h"
#include "test.h"
#include "text/big5.h"
#include "text/dbcs.h"
#include "text/text_input.h"

using swd3::TextInput;
using swd3::TextKey;
using swd3::TextMessage;

namespace
{

constexpr std::size_t kBig5Bytes = 2;  // a Big5 character: a lead byte and a trail byte
constexpr std::size_t kCopyBytes = 64;
constexpr int kCapacity = 6;
constexpr int kInputX = 30;
constexpr int kInputY = 40;
constexpr int kWideCapacity = 8;
constexpr swd3::u32 kOtherLead = 0xB0;  // a second lead byte before the trail byte is dropped
constexpr swd3::u32 kControlCharacter = 0x10;

int Key(TextInput& input, TextKey key, swd3::TextEntryState& entry, swd3::AudioManager& audio)
{
    return input
        .HandleMessage(TextMessage::KeyDown, static_cast<swd3::u32>(key), false, entry, audio, swd3::Config::MaxVolume);
}

int Char(TextInput& input, swd3::u32 character, swd3::TextEntryState& entry, swd3::AudioManager& audio)
{
    return input.HandleMessage(TextMessage::Char, character, false, entry, audio, swd3::Config::MaxVolume);
}

swd3::u8 ByteOf(const std::string& text, std::size_t at)
{
    return static_cast<swd3::u8>(text[at]);
}

}  // namespace

TEST_CASE("text input: Big5 character stepping")
{
    // "a一b" and a lone lead byte at the end.
    const std::string text = swd3::Big5(u8"a一b") + swd3::Big5(u8"一").substr(0, 1);
    constexpr std::size_t kAfterA = 1;
    constexpr std::size_t kAfterBig5 = kAfterA + kBig5Bytes;
    constexpr std::size_t kLoneLead = kAfterBig5 + 1;
    CHECK_EQ(swd3::CharNext(text, 0), kAfterA);
    CHECK_EQ(swd3::CharNext(text, kAfterA), kAfterBig5);
    CHECK_EQ(swd3::CharNext(text, kLoneLead), text.size());
    CHECK_EQ(swd3::CharNext(text, text.size()), text.size());
    CHECK_EQ(swd3::CharPrev(text, kAfterBig5), kAfterA);
    CHECK_EQ(swd3::CharPrev(text, 0), 0u);
    CHECK_EQ(swd3::TextFitLength(text, kBig5Bytes), kAfterA);
    CHECK_EQ(swd3::TextFitLength(text, kAfterBig5), kAfterBig5);
}

TEST_CASE("text input: editing keeps whole characters within the capacity")
{
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    swd3::TextEntryState entry;
    TextInput input(u8"一乙xyz", kCapacity, kInputX, kInputY);
    CHECK_EQ(input.GetX(), kInputX);
    CHECK_EQ(input.GetY(), kInputY);
    CHECK_EQ(input.GetCursor(), 0);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"一乙xy"));
    CHECK(input.CopyText(kBig5Bytes + 1) == swd3::Big5(u8"一"));
    Key(input, TextKey::End, entry, audio);
    CHECK_EQ(input.GetCursor(), kCapacity);
    Key(input, TextKey::Left, entry, audio);
    CHECK_EQ(input.GetCursor(), kCapacity - 1);
    Key(input, TextKey::Back, entry, audio);
    CHECK_EQ(input.GetCursor(), kCapacity - kBig5Bytes);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"一乙y"));
    Key(input, TextKey::Home, entry, audio);
    CHECK_EQ(input.GetCursor(), 0);
    Key(input, TextKey::Back, entry, audio);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"乙y"));
    const std::string ding = swd3::Big5(u8"丁");
    const swd3::u32 imeChar = static_cast<swd3::u32>(ByteOf(ding, 0)) << CHAR_BIT | ByteOf(ding, 1);
    CHECK_EQ(input.HandleMessage(TextMessage::ImeChar, imeChar, false, entry, audio, swd3::Config::MaxVolume), 0);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"丁乙y"));
    CHECK_EQ(input.GetCursor(), static_cast<int>(kBig5Bytes));
    Char(input, 'q', entry, audio);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"丁q乙y"));
    Char(input, 'r', entry, audio);
    CHECK(input.CopyText(kCopyBytes) == swd3::Big5(u8"丁qr乙"));
    Key(input, TextKey::Return, entry, audio);
    CHECK_EQ(input.GetResult(), TextInput::Enter);
    Key(input, TextKey::Escape, entry, audio);
    CHECK_EQ(input.GetResult(), TextInput::Escape);
}

TEST_CASE("text input: lead bytes typed one by one, refused characters")
{
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    swd3::TextEntryState entry;
    TextInput input(u8"", kWideCapacity, 0, 0);
    const std::string one = swd3::Big5(u8"一");
    Char(input, ByteOf(one, 0), entry, audio);
    CHECK_EQ(entry.leadPending, 1);
    Char(input, kOtherLead, entry, audio);
    CHECK_EQ(entry.leadPending, 1);
    Char(input, ByteOf(one, 1), entry, audio);
    CHECK_EQ(entry.leadPending, 0);
    CHECK(input.CopyText(kCopyBytes) == one);
    Char(input, '%', entry, audio);
    Char(input, kControlCharacter, entry, audio);
    CHECK(input.CopyText(kCopyBytes) == one);
    CHECK_EQ(input.HandleMessage(TextMessage::InputLanguageChange, 0, true, entry, audio, swd3::Config::MaxVolume), 1);
    Char(input, 'z', entry, audio);
    CHECK(input.CopyText(kCopyBytes) == one);
    CHECK_EQ(input.SetInsertMode(0), 1);
    CHECK_EQ(input.GetInsertMode(), 1);
}
