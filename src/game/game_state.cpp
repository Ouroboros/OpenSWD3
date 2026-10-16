#include "game/game_state.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "pic/pixel_format.h"

namespace swd3 {

// swd32.exe 0x0040DB40
int CountClickRegions() {
    int count = 0;
    for (ClickRegion* region = G.clickRegions; region != nullptr; region = region->next) {
        ++count;
    }
    return count;
}

// swd32.exe 0x0040DB60: first region strictly containing the pointer; returns its index (the count when none)
int FindClickRegion(ClickRegion** found) {
    int index = 0;
    ClickRegion* region = G.clickRegions;
    for (; region != nullptr; ++index) {
        const auto px = static_cast<std::uint32_t>(G.pointerX);
        const auto py = static_cast<std::uint32_t>(G.pointerY);
        if (px > region->left && px < region->right && py > region->top && py < region->bottom) {
            break;
        }
        region = region->next;
    }
    *found = region;
    return index;
}

// swd32.exe 0x0040DBC0
int FreeClickRegions() {
    ClickRegion* region = G.clickRegions;
    for (ClickRegion* next = G.clickRegions; next != nullptr; G.clickRegions = next) {
        next = next->next;
        crt::free(region);
        region = next;
    }
    G.clickRegionFlags = 0;
    G.clickRegions = nullptr;
    G.clickRegionState[0] = 0;
    G.clickRegionState[1] = 0;
    G.clickRegionState[2] = 0;
    return 0;
}

// swd32.exe 0x0040DC00: clears the move bookkeeping of an action state (fields +0x1C, +0x20, +0x3C = -1)
std::uint8_t* ResetMoveRecord(std::uint8_t* record) {
    mem::put32(record, 0x1C, 0xFFFFFFFFu);
    mem::put32(record, 0x20, 0xFFFFFFFFu);
    mem::put32(record, 0x3C, 0xFFFFFFFFu);
    mem::put16(record, 0x48, 0);
    mem::put16(record, 0x46, 0);
    mem::put16(record, 0x44, 0);
    mem::put16(record, 0x42, 0);
    mem::put32(record, 0x90, 0);
    return record;
}

// swd32.exe 0x0040DC30
EventNode* FindEvent(int id) {
    for (EventNode* node = G.eventList; node != nullptr; node = node->next) {
        if (node->id == id) {
            return node;
        }
    }
    return nullptr;
}

// swd32.exe 0x0040DC50
bool GameFlag(int flag) { return (G.gameFlags[flag >> 3] & G.bitMasks[flag & 7]) != 0; }

// swd32.exe 0x0040DC80
std::uint8_t SetGameFlag(int flag) {
    const auto value = static_cast<std::uint8_t>(G.bitMasks[flag & 7] | G.gameFlags[flag >> 3]);
    G.gameFlags[flag >> 3] = value;
    return value;
}

// swd32.exe 0x0040DCB0
std::uint8_t ClearGameFlag(int flag) {
    const auto value = static_cast<std::uint8_t>((0xFF - G.bitMasks[flag & 7]) & G.gameFlags[flag >> 3]);
    G.gameFlags[flag >> 3] = value;
    return value;
}

// swd32.exe 0x0040DCE0: only 1 sets the flag
std::uint8_t SetGameFlagTo(int flag, int value) {
    if (value == 1) {
        return SetGameFlag(flag);
    }
    return ClearGameFlag(flag);
}

// swd32.exe 0x0040DD10
int SetWalkSpeed(int speed) {
    G.walkSpeed = speed;
    return speed;
}

// swd32.exe 0x0040DD20
int SetInputLock(int value) {
    G.inputLock = value;
    return 1;
}

// swd32.exe 0x0040DD30
int ClearInputLock() {
    G.inputLock = 0;
    return 1;
}

// swd32.exe 0x0040DD40: fills a 0x21C byte follower record with 0xFF
std::uint32_t ClearFollower(void* record) {
    std::memset(record, 0xFF, 0x21C);
    return 0xFFFFFFFFu;
}

// swd32.exe 0x0040DD60: unpacks a stored role record (fields reordered on load)
std::uint8_t LoadRoleRecord(std::uint8_t* role, const std::uint8_t* record) {
    mem::put32(role, 0, mem::u32(record, 0));
    mem::put16(role, 0xA, mem::u16(record, 4));
    mem::put16(role, 0xC, mem::u16(record, 6));
    mem::put16(role, 0xE, mem::u16(record, 8));
    mem::put16(role, 4, mem::u16(record, 0xA));
    mem::put16(role, 6, mem::u16(record, 0xC));
    mem::put16(role, 8, mem::u16(record, 0xE));
    for (int offset = 0x10; offset <= 0x1E; offset += 2) {
        mem::put16(role, offset, mem::u16(record, offset));
    }
    mem::put16(role, 0x2A, mem::u16(record, 0x20));
    mem::put32(role, 0x20, mem::u16(record, 0x22));
    mem::put16(role, 0x24, mem::u16(record, 0x24));
    mem::put16(role, 0x26, mem::u16(record, 0x26));
    mem::put16(role, 0x28, mem::u16(record, 0x28));
    for (int i = 0; i < 10; ++i) {
        role[0x2C + i] = record[0x2A + i];
    }
    return role[0x35];
}

// swd32.exe 0x0040DE50: grey dashes around the rectangle (top left to right, right down, bottom right to left,
// left up), the shade cycling over 32 steps; skipped when the rectangle leaves the 640x480 screen
void DrawMarchingRect(std::uint16_t* pixels, int x, int y, int width, int height, int pitch) {
    if (x < 0 || y < 0 || width + x >= 0x280 || y + height >= 0x1E0) {
        return;
    }
    std::uint32_t phase = static_cast<std::uint32_t>(G.marchingPhase);
    auto shade = [&phase]() {
        const std::uint32_t v = (0x3F - phase) & 0x1F;
        const std::uint32_t grey = v | (v << 5) | (v << 10);
        std::uint32_t pair = (grey << 16) | grey;
        PixelConvert(reinterpret_cast<std::uint16_t*>(&pair), 2);
        phase = (phase + 1) & 0x1F;
        return static_cast<std::uint16_t>(pair);
    };
    const int topLeft = pitch * y + x;
    std::uint16_t* p = pixels + topLeft;
    for (int i = 0; i < width; ++i) {
        *p++ = shade();
    }
    p = pixels + width + topLeft;
    for (int i = 0; i < height; ++i) {
        *p = shade();
        p += pitch;
    }
    const int bottomLeft = pitch * (y + height) + x;
    p = pixels + width + bottomLeft;
    for (int i = 0; i < width; ++i) {
        *p-- = shade();
    }
    p = pixels + bottomLeft;
    for (int i = 0; i < height; ++i) {
        *p = shade();
        p -= pitch;
    }
    G.marchingPhase = (G.marchingPhase + 1) & 0x1F;
}

// swd32.exe 0x0040E030: direction code from the centre of the controlled role towards (x, y)
int DirectionToPlayer(int x, int y) {
    const Role& role = G.roles[G.playerRole];
    std::int32_t distance;
    const int angle = AngleBetween(role.x + 8 * role.act.width, role.y + 8 * role.act.height, x, y, &distance);
    return AngleToDirection(angle);
}

// swd32.exe 0x0040E080: every fourth pixel of every fourth row of a 640x480 screen into its first 160x120 pixels
std::uint16_t* ShrinkScreenQuarter(std::uint16_t* pixels) {
    std::uint16_t* source = pixels;
    std::uint16_t* dest = pixels;
    for (int row = 0; row < 0x78; ++row) {
        for (int column = 0; column < 0xA0; ++column) {
            const std::uint16_t value = *source;
            source += 4;
            *dest++ = value;
        }
        source += 0x780;
    }
    return source;
}

// swd32.exe 0x00411E20: angle in degrees (multiples of 5, quadrant by quadrant) from (toX, toY) to (fromX, fromY)
// found in the sine table; 0 when the points coincide
int AngleBetween(int fromX, int fromY, int toX, int toY, std::int32_t* distance) {
    const int dx = fromX - toX;
    const int dy = fromY - toY;
    const auto length = static_cast<std::int32_t>(crt::ftol(std::sqrt(static_cast<double>(dy * dy + dx * dx))));
    *distance = length;
    if (length == 0) {
        return 0;
    }
    const int sine = std::abs((dy << 13) / length);
    int base;
    int index;
    int step;
    if (dy >= 0) {
        index = 0;
        base = 0xB4;
        step = 5;
        if (dx >= 0) {
            base = 0x10E;
            index = 0x59;
            step = -5;
        }
    } else {
        base = 0x5A;
        index = 0x59;
        step = -5;
        if (dx >= 0) {
            base = 0;
            index = 0;
            step = 5;
        }
    }
    int best = 0x2328;
    int angle = 0;
    for (int candidate = base; candidate < base + 0x5A; candidate += 5) {
        const int difference = std::abs(sine - G.sineTable[index]);
        if (difference <= best) {
            best = difference;
            angle = candidate;
        }
        index += step;
    }
    return angle;
}

// swd32.exe 0x00411F00: 16 sectors of 22.5 degrees to direction codes
int AngleToDirection(int angle) {
    const int table[17] = {3, 7, 7, 0, 0, 4, 4, 2, 2, 6, 6, 1, 1, 5, 5, 3, 0};
    return table[0x10 * angle / 0x168];
}

}  // namespace swd3
