#pragma once

#include <array>
#include <memory>

#include "core/types.h"
#include "item/item_list.h"
#include "platform/surface.h"

namespace swd3
{

// The state of the battle and of the shop and item lists it shares with the menus.
class BattleState
{
public:
    static constexpr i32 kInitialGaugeFull = 900;

    i32 listBCount = 0;                           // 0x004FD3B0 entries of listB, at most 13
    ItemLink listB;                               // 0x004FD4C0
    i32 mode = 0;                                 // 0x004FD578 bits 0..1: the shop and sell modes of the item lists
    ItemLink items;                               // 0x004FD580 the sorted item list
    u16 itemCursor = 0;                           // 0x004FD588
    i32 gaugeFull = kInitialGaugeFull;            // 0x004A74CC the full action gauge
    std::array<i16, 3> unk4A7632 = {-1, -1, -1};  // 0x004A7632
    i32 evadeDisabled = 0;                        // 0x0053CEB8
    u8 evadeStreak = 0;                           // 0x0053D028 failed evasion rolls in a row
    u8 evadeJustSucceeded = 0;                    // 0x0053D029
    std::array<std::unique_ptr<Surface>, 2> surfaces;  // 0x005244C4 screen sized off-screen surfaces
};

}  // namespace swd3
