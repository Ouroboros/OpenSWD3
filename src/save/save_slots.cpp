#include "save/save_slots.h"

#include <array>
#include <climits>
#include <cstddef>
#include <cstring>
#include <span>

#include "codec/lzo.h"
#include "core/byte_reader.h"
#include "core/format.h"
#include "draw/painter.h"
#include "file/file.h"
#include "game/game_state.h"
#include "party/party.h"
#include "save/save_games.h"

namespace swd3
{

namespace
{

constexpr u32 kSecondsPerHour = 0xE10;
constexpr u32 kSecondsPerMinute = 0x3C;
constexpr std::size_t kRoleRecords = 0x104;
constexpr u16 kAbsentRole = 0xFFFF;  // maxHitPoints of a role not in the party
constexpr int kUnk24Flag = 0x53;
constexpr i32 kUnk24Base = 2;
constexpr char kDigitPlace = '_';                           // a place of DateTemplate the next header digit fills
constexpr std::size_t kBlockHeaderBytes = 2 * sizeof(u32);  // a packed block: its packed size and its word count
constexpr std::size_t kUnk20At = 0xC;                       // after the second table
constexpr std::size_t kDataBlockAt = 0x24;
constexpr std::size_t kUnk28At = 4;  // in the unpacked data
constexpr i32 kLitAction = 8;        // the preview frame of the newest slot
constexpr i32 kPreviewDirection = 6;

u32 Dword(std::span<const u8> bytes, std::size_t at)
{
    return ByteReader::Word32At(bytes, at);
}

u16 Word(std::span<const u8> bytes, std::size_t at)
{
    return ByteReader::Word16At(bytes, at);
}

// A packed block (its packed size, its size in words, the 14-bit LZO data) at `at`.
std::vector<u8> Unpack(std::span<const u8> view, std::size_t at)
{
    const u32 packed = Dword(view, at);
    const u32 words = Dword(view, at + sizeof(u32));
    std::vector<u8> output(sizeof(u16) * static_cast<std::size_t>(words), 0);

    if (at + kBlockHeaderBytes + packed <= view.size())
    {
        LzoDecoder(view.subspan(at + kBlockHeaderBytes, packed), output).Run();
    }

    return output;
}

}  // namespace

// swd32.exe 0x004099C0
void SaveSlots::Free(SavePreview& preview)
{
    preview = SavePreview{};
}

// swd32.exe 0x00409600: the preview of save `slot`: the date of its header, the thumbnail, the location, the roles of
// the party (from the story flags) with their maxima and level, the names and the play time
// swd32.exe 0x00498050 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498058 (EH handler of 0x00409600)
void SaveSlots::LoadPreview(SavePreview& preview, int slot, const std::u8string& saveDirectory, const Painter& painter)
{
    File file;

    if (!file.Open(SaveGames::SlotPath(saveDirectory, slot), File::Disposition::OpenExisting, File::Access::Read))
    {
        preview.state = SavePreview::Missing;
        return;
    }

    file.CreateMapping();
    const u8* mapped = file.MapView(0, 0);
    const std::span<const u8> view(mapped, mapped != nullptr ? file.Size() : 0);
    std::string date(SaveSlots::DateTemplate);

    if (view.size() >= SaveSlots::HeaderBytes)
    {
        std::size_t digit = 0;

        for (char& place : date)
        {
            if (place == kDigitPlace)
            {
                place = static_cast<char>(view[digit++]);
            }
        }
    }

    preview.date = date.substr(0, strnlen(date.c_str(), date.size()));
    std::size_t at = SaveSlots::HeaderBytes;
    preview.thumbnail.assign(SaveSlots::ThumbnailPixels, 0);

    for (std::size_t pixel = 0; pixel < preview.thumbnail.size(); ++pixel)
    {
        preview.thumbnail[pixel] = Word(view, at + sizeof(u16) * pixel);
    }

    painter.ConvertPixels(preview.thumbnail);
    at += sizeof(u16) * SaveSlots::ThumbnailPixels;

    for (std::size_t index = at; index < view.size() && view[index] != 0; ++index)
    {
        preview.location.push_back(static_cast<char>(view[index]));
    }

    at += SaveSlots::LocationBytes;
    const std::vector<u8> flags = Unpack(view, at);

    auto flag = [&flags](int number)
    {
        const auto byte = static_cast<std::size_t>(number >> GameState::FlagByteShift);
        return byte < flags.size() && (flags[byte] & GameState::BitMasks[number & GameState::FlagBitMask]) != 0;
    };

    for (std::size_t role = 0; role < preview.roles.size(); ++role)
    {
        if (!flag(Party::FirstRoleFlag + static_cast<int>(role)))
        {
            preview.roles[role].maxHitPoints = kAbsentRole;
        }
    }

    preview.unk24 = (flag(kUnk24Flag) ? 1 : 0) + kUnk24Base;
    const std::size_t table = at + Dword(view, at) + kBlockHeaderBytes;
    const std::size_t after = table + Dword(view, table);
    preview.unk20 = static_cast<i32>(Dword(view, after + kUnk20At));
    const std::size_t block = after + kDataBlockAt;
    const std::vector<u8> data = Unpack(view, block);
    preview.unk28 = static_cast<i32>(Dword(data, kUnk28At));

    for (std::size_t role = 0; role < preview.roles.size(); ++role)
    {
        SavePreviewRole& entry = preview.roles[role];

        if (entry.maxHitPoints != kAbsentRole)
        {
            const std::size_t record = kRoleRecords + sizeof(RoleRecord) * role;
            entry.maxHitPoints = Word(data, record + offsetof(RoleRecord, maxHitPoints));
            entry.maxSpirit = Word(data, record + offsetof(RoleRecord, maxSpirit));
            entry.maxStamina = Word(data, record + offsetof(RoleRecord, maxStamina));
            const std::size_t level = record + offsetof(RoleRecord, level);
            entry.level = level < data.size() ? data[level] : u16{0};
        }
    }

    const std::size_t names = block + Dword(view, block) + kBlockHeaderBytes;

    for (std::size_t index = 0; index < preview.names.size() && names + index < view.size(); ++index)
    {
        preview.names[index] = static_cast<char>(view[names + index]);
    }

    u32 seconds = Dword(view, names + preview.names.size());
    preview.playTime = WinFormat(SaveSlots::Hours, {static_cast<i32>(seconds / kSecondsPerHour)});
    seconds %= kSecondsPerHour;
    preview.playTime += WinFormat(SaveSlots::Minutes, {static_cast<i32>(seconds / kSecondsPerMinute)});
    preview.playTime += WinFormat(SaveSlots::Seconds, {static_cast<i32>(seconds % kSecondsPerMinute)});
    file.UnmapView(mapped);
    file.CloseMapping();
    file.Close();
    preview.state = SavePreview::Loaded;
}

// swd32.exe 0x00409B80: the frames of the three previews (acts 1, 2, 8 and 0x11), lit (action 8) for the newest slot
void SaveSlots::ResetActs()
{
    for (int slot = 0; slot < SaveSlots::Visible; ++slot)
    {
        for (std::size_t index = 0; index < SaveSlots::ActsPerSlot; ++index)
        {
            ActState& act = this->acts[static_cast<std::size_t>(slot)][index];
            ResetPlayback(act);
            act.id = SaveSlots::FrameActs[index];
            act.action = this->newest % SaveSlots::Visible == slot ? kLitAction : 0;
            act.direction = kPreviewDirection;
        }
    }
}

// The previews of the page of three slots holding the newest slot.
void SaveSlots::LoadPage(const std::u8string& saveDirectory, const Painter& painter)
{
    for (SavePreview& preview : this->previews)
    {
        SaveSlots::Free(preview);
    }

    const int first = SaveSlots::Visible * (this->newest / SaveSlots::Visible);

    for (int index = 0; index < SaveSlots::Visible; ++index)
    {
        SaveSlots::LoadPreview(this->previews[static_cast<std::size_t>(index)], first + index, saveDirectory, painter);
    }

    this->ResetActs();
}

// swd32.exe 0x00409A10: the slot written last (comparing the high dword of the write times only; a tie goes to the
// later slot), then its page of previews
// swd32.exe 0x00498070 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x00498078 (EH handler of 0x00409A10)
void SaveSlots::FindNewest(const std::u8string& saveDirectory, const Painter& painter)
{
    i32 newestHigh = 0;

    for (int slot = 0; slot <= SaveGames::LastSlot; ++slot)
    {
        File file;

        if (file.Open(
                SaveGames::SlotPath(saveDirectory, slot),
                File::Disposition::OpenExisting,
                File::Access::ReadWrite
            ))
        {
            u64 written = 0;
            file.LastWriteTime(written);
            const auto high = static_cast<i32>(written >> (CHAR_BIT * sizeof(u32)));

            if (high >= newestHigh)
            {
                this->newest = slot;
                newestHigh = high;
            }
        }

        file.Close();
    }

    this->LoadPage(saveDirectory, painter);
}

}  // namespace swd3
