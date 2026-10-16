#include "menu/menu_bestiary.h"

#include <cstring>

#include "battle/battle_enemy.h"
#include "core/debug_log.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "data/image.h"
#include "menu/menu_util.h"
#include "swd3/w32.h"

namespace swd3 {

// swd32.exe 0x0043C9C0: lists the monsters 1..500 whose category byte (G.dword_4FCA78) is the category of tab
// `category` (0..9, 13, 14) into the id array G.dword_4FC91C (zero terminated), and for every monster with a seen
// count (G.dword_4FCA7C) its MON.DAT name into G.menuStrings and the count into G.dword_4FC930. A failed load is
// logged through the empty function at 0x0044A240; the overflow message after the slot loop is never reached.
void BuildBestiaryList(int unused, int category) {
    static_cast<void>(unused);
    const int categories[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xD, 0xE, 0, 0, 0};
    auto* monster = static_cast<std::uint8_t*>(crt::malloc(0xB0));
    std::memset(monster, 0, 0xB0);
    auto* ids = reinterpret_cast<std::int32_t*>(static_cast<std::uintptr_t>(G.dword_4FC91C));
    std::memset(ids, 0, 0x100);
    for (char* text : G.menuStrings) {
        *text = 0;
    }
    const int wanted = categories[category];
    auto* counts = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FC930));
    std::memset(counts, 0, 0x40);
    G.dword_4FC974 = 0;
    const auto* kinds = reinterpret_cast<const std::int8_t*>(static_cast<std::uintptr_t>(G.dword_4FCA78));
    for (int id = 1; id <= 0x1F4; ++id) {
        if (kinds[id] == wanted) {
            ids[G.dword_4FC974] = id;
            *G.menuStrings[G.dword_4FC974] = 0;
            counts[G.dword_4FC974] = 0;
            ++G.dword_4FC974;
        }
    }
    ids[G.dword_4FC974] = 0;
    const auto* seen = reinterpret_cast<const std::uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FCA7C));
    for (int id = 1; id <= 0x1F4; ++id) {
        if (seen[id] == 0) {
            continue;
        }
        for (int slot = 0; slot < 0x40; ++slot) {
            if (ids[slot] != id) {
                continue;
            }
            if (LoadMonster(monster + 0xC, static_cast<std::uint16_t>(id))) {
                w32::lstrcpyA(G.menuStrings[slot], reinterpret_cast<const char*>(monster + 0xC));
                counts[slot] = seen[id];
            } else {
                DebugLog(reinterpret_cast<const char*>(&image::at<std::uint8_t>(0x004A6B48u)),
                         reinterpret_cast<const char*>(&image::at<std::uint8_t>(0x004A6B54u)), id);
            }
            crt::free(*reinterpret_cast<void**>(monster + 0xAC));
            *reinterpret_cast<std::uint32_t*>(monster + 0xAC) = 0;
        }
    }
    crt::free(monster);
    G.dword_4FC90C = 0;
    G.dword_4FC928 = 0;
    G.dword_4FC920 = G.dword_4FC91C;
    CountMenuPointers();
}

}  // namespace swd3
