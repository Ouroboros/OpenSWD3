#include "save/save_slots.h"

#include <span>

#include "codec/lzo.h"
#include "core/format.h"
#include "draw/painter.h"
#include "file/file.h"
#include "save/save_games.h"

namespace swd3
{

namespace
{

constexpr u32 kSecondsPerHour = 0xE10;
constexpr u32 kSecondsPerMinute = 0x3C;
constexpr std::size_t kRoleRecords = 0x104;
constexpr std::size_t kRoleRecordBytes = 0x38;

u32 Dword(std::span<const u8> bytes, std::size_t at)
{
    if (at + 4 > bytes.size())
    {
        return 0;
    }

    return static_cast<u32>(
        bytes[at] | bytes[at + 1] << 8 | bytes[at + 2] << 16 | static_cast<u32>(bytes[at + 3]) << 24
    );
}

u16 Word(std::span<const u8> bytes, std::size_t at)
{
    return at + 2 <= bytes.size() ? static_cast<u16>(bytes[at] | bytes[at + 1] << 8) : u16{0};
}

// A packed block (its packed size, its size in words, the 14-bit LZO data) at `at`.
std::vector<u8> Unpack(std::span<const u8> view, std::size_t at)
{
    const u32 packed = Dword(view, at);
    const u32 words = Dword(view, at + 4);
    std::vector<u8> output(2 * static_cast<std::size_t>(words), 0);

    if (at + 8 + packed <= view.size())
    {
        LzoDecoder(view.subspan(at + 8, packed), output).Run();
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
void SaveSlots::LoadPreview(SavePreview& preview, int slot, const std::string& installDirectory, const Painter& painter)
{
    File file;

    if (!file.Open(SaveGames::SlotPath(installDirectory, slot), File::Disposition::OpenExisting, File::Access::Read))
    {
        preview.state = SavePreview::kMissing;
        return;
    }

    file.CreateMapping();
    const u8* mapped = file.MapView(0, 0);
    const std::span<const u8> view(mapped, mapped != nullptr ? file.Size() : 0);
    std::string date(SaveSlots::kDateTemplate);

    if (view.size() >= SaveSlots::kHeaderBytes)
    {
        for (const auto& [from, to] :
             {std::pair{0, 0},
              {1, 1},
              {2, 2},
              {3, 3},
              {4, 5},
              {5, 6},
              {6, 8},
              {7, 9},
              {8, 0xB},
              {9, 0xC},
              {0xA, 0xE},
              {0xB, 0xF}})
        {
            date[static_cast<std::size_t>(to)] = static_cast<char>(view[static_cast<std::size_t>(from)]);
        }
    }

    preview.date = date;
    std::size_t at = SaveSlots::kHeaderBytes;
    preview.thumbnail.assign(SaveSlots::kThumbnailPixels, 0);

    for (std::size_t pixel = 0; pixel < preview.thumbnail.size(); ++pixel)
    {
        preview.thumbnail[pixel] = Word(view, at + 2 * pixel);
    }

    painter.ConvertPixels(preview.thumbnail);
    at += 2 * SaveSlots::kThumbnailPixels;

    for (std::size_t index = at; index < view.size() && view[index] != 0; ++index)
    {
        preview.location.push_back(static_cast<char>(view[index]));
    }

    at += SaveSlots::kLocationBytes;
    const std::vector<u8> flags = Unpack(view, at);

    auto flag = [&flags](std::size_t byte, u8 bit)
    {
        return byte < flags.size() && (flags[byte] & bit) != 0;
    };

    if (!flag(3, 0x40))
    {
        preview.roles[0].maxHitPoints = 0xFFFF;
    }

    if (!flag(3, 0x80))
    {
        preview.roles[1].maxHitPoints = 0xFFFF;
    }

    if (!flag(4, 1))
    {
        preview.roles[2].maxHitPoints = 0xFFFF;
    }

    if (!flag(4, 2))
    {
        preview.roles[3].maxHitPoints = 0xFFFF;
    }

    preview.unk24 = (flag(0xA, 8) ? 1 : 0) + 2;
    const std::size_t table = at + Dword(view, at) + 8;
    const std::size_t after = table + Dword(view, table);
    preview.unk20 = static_cast<i32>(Dword(view, after + 0xC));
    const std::size_t block = after + 0x24;
    const std::vector<u8> data = Unpack(view, block);
    preview.unk28 = static_cast<i32>(Dword(data, 4));

    for (std::size_t role = 0; role < preview.roles.size(); ++role)
    {
        SavePreviewRole& entry = preview.roles[role];

        if (entry.maxHitPoints != 0xFFFF)
        {
            const std::size_t record = kRoleRecords + kRoleRecordBytes * role;
            entry.maxHitPoints = Word(data, record + 0xA);
            entry.maxSpirit = Word(data, record + 0xC);
            entry.maxStamina = Word(data, record + 0xE);
            entry.level = record + 0x2C < data.size() ? data[record + 0x2C] : u16{0};
        }
    }

    const std::size_t names = block + Dword(view, block) + 8;

    for (std::size_t index = 0; index < preview.names.size() && names + index < view.size(); ++index)
    {
        preview.names[index] = static_cast<char>(view[names + index]);
    }

    u32 seconds = Dword(view, names + 0x40);
    preview.playTime = WinFormat(SaveSlots::kHours, {static_cast<i32>(seconds / kSecondsPerHour)});
    seconds %= kSecondsPerHour;
    preview.playTime += WinFormat(SaveSlots::kMinutes, {static_cast<i32>(seconds / kSecondsPerMinute)});
    preview.playTime += WinFormat(SaveSlots::kSeconds, {static_cast<i32>(seconds % kSecondsPerMinute)});
    file.UnmapView(mapped);
    file.CloseMapping();
    file.Close();
    preview.state = SavePreview::kLoaded;
}

// swd32.exe 0x00409B80: the frames of the three previews (acts 1, 2, 8 and 0x11), lit (action 8) for the newest slot
void SaveSlots::ResetActs()
{
    for (int slot = 0; slot < SaveSlots::kVisible; ++slot)
    {
        for (std::size_t index = 0; index < SaveSlots::kActsPerSlot; ++index)
        {
            ActState& act = this->acts[static_cast<std::size_t>(slot)][index];
            ResetPlayback(act);
            act.id = SaveSlots::kFrameActs[index];
            act.action = this->newest % SaveSlots::kVisible == slot ? 8 : 0;
            act.direction = 6;
        }
    }
}

// The previews of the page of three slots holding the newest slot.
void SaveSlots::LoadPage(const std::string& installDirectory, const Painter& painter)
{
    for (SavePreview& preview : this->previews)
    {
        SaveSlots::Free(preview);
    }

    const int first = SaveSlots::kVisible * (this->newest / SaveSlots::kVisible);

    for (int index = 0; index < SaveSlots::kVisible; ++index)
    {
        SaveSlots::LoadPreview(
            this->previews[static_cast<std::size_t>(index)],
            first + index,
            installDirectory,
            painter
        );
    }

    this->ResetActs();
}

// swd32.exe 0x00409A10: the slot written last (comparing the high dword of the write times only; a tie goes to the
// later slot), then its page of previews
void SaveSlots::FindNewest(const std::string& installDirectory, const Painter& painter)
{
    i32 newestHigh = 0;

    for (int slot = 0; slot <= SaveGames::kLastSlot; ++slot)
    {
        File file;

        if (file.Open(
                SaveGames::SlotPath(installDirectory, slot),
                File::Disposition::OpenExisting,
                File::Access::ReadWrite
            ))
        {
            u64 written = 0;
            file.LastWriteTime(written);
            const auto high = static_cast<i32>(written >> 32);

            if (high >= newestHigh)
            {
                this->newest = slot;
                newestHigh = high;
            }
        }

        file.Close();
    }

    this->LoadPage(installDirectory, painter);
}

}  // namespace swd3
