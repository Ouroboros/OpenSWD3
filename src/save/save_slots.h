#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/types.h"
#include "party/party.h"
#include "res/act_state.h"
#include "text/big5.h"

namespace swd3
{

class Painter;

// A role in a save preview: its maxima and level (the hit points 0xFFFF when the role was not in the party).
struct SavePreviewRole
{
    u16 maxHitPoints = 0;  // +0x00
    u16 maxSpirit = 0;     // +0x02
    u16 maxStamina = 0;    // +0x04
    u16 level = 0;         // +0x06
};

// The preview of a save slot (0x2A8 bytes in the original, three of them from 0x004CB968).
struct SavePreview
{
    static constexpr u16 Missing = 1;
    static constexpr u16 Loaded = 2;

    std::array<SavePreviewRole, Party::Roles> roles;  // +0x00
    i32 unk20 = 0;                                    // +0x20
    i32 unk24 = 0;                                    // +0x24 2, or 3 with story flag 0x53
    i32 gold = 0;                                     // +0x28 the dword at +4 of the saved data: the gold
    u16 state = 0;                                    // +0x30 Missing, Loaded
    std::vector<u16> thumbnail;                       // +0x294 160 x 120 pixels
    std::string date;                                 // +0x298 "yyyy/mm/dd hh:mm"
    std::string location;                             // +0x29C
    std::array<char, 0x40> names = {};                // +0x2A0
    std::string playTime;                             // +0x2A4
};

// The save slots of the load and save screens: the newest slot, the previews of the page of three slots it is on and
// the animations of their frames.
class SaveSlots
{
public:
    static constexpr int Visible = 3;
    static constexpr int ActsPerSlot = 4;
    static constexpr u32 HeaderBytes = 0xC;
    static constexpr u32 ThumbnailPixels = 0x4B00;
    static constexpr u32 LocationBytes = 0x20;
    static constexpr std::string_view DateTemplate = "____/__/__ __:__";  // swd32.exe 0x0049FAA4
    static constexpr std::u8string_view Hours = u8"%02d時";               // swd32.exe 0x0049FA9C
    static constexpr std::u8string_view Minutes = u8"%02d分";             // swd32.exe 0x0049FA94
    static constexpr std::u8string_view Seconds = u8"%02d秒";             // swd32.exe 0x0049FA8C
    static constexpr std::array<i32, ActsPerSlot> FrameActs = {1, 2, 8, 0x11};

    static void Free(SavePreview& preview);
    static void LoadPreview(SavePreview& preview, int slot, const std::u8string& saveDirectory, const Painter& painter);
    void ResetActs();
    void FindNewest(const std::u8string& saveDirectory, const Painter& painter);
    void LoadPage(const std::u8string& saveDirectory, const Painter& painter);

    std::array<std::array<ActState, ActsPerSlot>, Visible> acts;  // 0x004CB248
    std::array<SavePreview, Visible> previews;                    // 0x004CB968
    i32 newest = 0;                                               // 0x004CC2A0 the slot written last
};

}  // namespace swd3
