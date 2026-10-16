#include "menu/menu_util.h"

#include <cstring>

#include "core/debug_log.h"
#include "core/random.h"
#include "crt/crt.h"
#include "data/code_ptr.h"
#include "data/image.h"
#include "game/game_state.h"
#include "core/mem.h"
#include "data/globals.h"
#include "draw/draw.h"
#include "font/text_font.h"
#include "pic/pixel_format.h"
#include "res/act_anim.h"
#include "res/tsw.h"
#include "swd3/w32.h"
#include "ui/ui_draw.h"

namespace swd3 {

using std::uint8_t;

// swd32.exe 0x0043B080: one step of `state` drawn translucent (alpha `alpha`) at (x, y); a failed step is logged
// through the empty function at 0x0044A240 and returns 0
int DrawActGhost(ActState* state, int x, int y, int alpha) {
    if (ActRun(state) == 0) {
        DebugLog("Act Err(Menu::DrawActGhost)", reinterpret_cast<const char*>(&G.unk_49FD0C), state->id,
                 state->action, state->direction);
        return 0;
    }
    Picture* picture = TswGetPicture(state->fr, state->ap);
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    G.drawAlpha = alpha;
    return DrawPicture(x - state->x, y - state->y, picture->width, picture->height,
                       static_cast<int>((state->flags & 0x80000003) | 0x14), 0);
}

// swd32.exe 0x0043B980
int CountList(uint8_t** list) {
    int count = 0;
    for (uint8_t* node = *list; node != nullptr; node = mem::ptr<uint8_t>(node, 0)) {
        ++count;
    }
    return count;
}

// swd32.exe 0x0043B9A0: *node becomes the node after `index` steps from the head
void ListNodeAtInto(int index, uint8_t** list, uint8_t** node) {
    *node = *list;
    for (int step = index; step > 0; --step) {
        *node = mem::ptr<uint8_t>(*node, 0);
    }
}

// swd32.exe 0x0043B9C0
uint8_t* ListNodeAt(int index, uint8_t** list) {
    uint8_t* node = *list;
    for (int step = index; step > 0; --step) {
        node = mem::ptr<uint8_t>(node, 0);
    }
    return node;
}

// swd32.exe 0x0043B9E0: the name of `item` (empty items get the empty name) into G.itemNameBuffer
void LoadItemName(const uint8_t* item) {
    if (mem::u16(item, 4) == 0xFFDC) {
        w32::wsprintfA(G.itemNameBuffer, G.emptyItemName);
        return;
    }
    const uint8_t* text = G.itemText;
    const uint8_t* name = text + mem::u32(text, 4 * mem::u16(item, 4) + mem::u32(text, 0x4C));
    int length = 0;
    for (; mem::u16(name, 0) != 0x5125; ++name) {
        G.itemNameBuffer[length++] = static_cast<char>(*name);
    }
    G.itemNameBuffer[length] = 0;
}

// swd32.exe 0x0043BA40: mouse button bits of the menus from the key states
void ReadMenuMouseButtons() {
    G.menuMouseButtons = 0;
    if (G.dword_4B7DAC == 1) {
        if (G.dword_4B7DA0 == 1) {
            G.menuMouseButtons = 1;
        } else if (G.dword_4B7DA0 > 1) {
            G.menuMouseButtons = 2;
        }
    }
    if (G.dword_4B7D9C == 1) {
        if (G.dword_4B7D90 == 1) {
            G.menuMouseButtons |= 4;
        } else if (G.dword_4B7D90 > 1) {
            G.menuMouseButtons |= 8;
        }
    }
}

// swd32.exe 0x0043BAB0: a filled box 8 pixels larger on every side under the panel frame (act 0x233B). The return
// value is unused by callers.
void DrawMenuBox(int x, int y, int width, int height, int mode, int red, int green, int blue) {
    FillBoxMode(x - 8, y - 8, width + 0x10, height + 0x10, red, green, blue, mode);
    G.panelAct.id = 0x233B;
    G.panelAct.action = 0;
    ActRun(&G.panelAct);
    DrawWindowFrame(G.panelAct.fr, x, y, x + width, y + height, 0, static_cast<int>(0x80000008u));
}

// swd32.exe 0x0043BB40: a cursor past the rows goes to the last row and scrolls one down when possible
void ClampMenuCursor(const int* total, int* first, int* cursor, const int* rows) {
    if (*cursor < *rows) {
        return;
    }
    *cursor = 0;
    if (*rows >= 1) {
        *cursor = *rows - 1;
    }
    if (*first + *rows < *total) {
        ++*first;
    }
}

// swd32.exe 0x0043BB80
void MenuCursorDown(const int* total, int* first, int* cursor, const int* rows) {
    ++*cursor;
    ClampMenuCursor(total, first, cursor, rows);
}

// swd32.exe 0x0043BBC0
void MenuCursorUp(int unused, int* first, int* cursor) {
    static_cast<void>(unused);
    if (--*cursor < 0) {
        *cursor = 0;
        if (*first > 0) {
            --*first;
        }
    }
}

// swd32.exe 0x0043BBE0: from the last row a page down; elsewhere the cursor goes to the last row
void MenuPageDown(const int* total, int* first, int* cursor, int* rows, int page) {
    if (*cursor != *rows - 1) {
        *cursor = 0;
        if (*rows >= 1) {
            *cursor = *rows - 1;
        }
        return;
    }
    *first += page;
    if (*first + page < *total) {
        const int last = *total - *first - 1;
        if (*cursor > last) {
            *cursor = last;
        }
        return;
    }
    *first = *total - page;
    if (*first < 0) {
        *first = 0;
    }
    *rows = *total - *first;
    *cursor = *rows - 1;
}

// swd32.exe 0x0043BC60: from the first row a page up; elsewhere the cursor goes to the first row
void MenuPageUp(int unused, int* first, int* cursor, int unused2, int page) {
    static_cast<void>(unused);
    static_cast<void>(unused2);
    if (*cursor != 0) {
        *cursor = 0;
        return;
    }
    *first -= page;
    if (*first < 0) {
        *cursor = 0;
        *first = 0;
    }
}

// swd32.exe 0x0043BC90: counts the nodes, at most `limit`
void CountListUpTo(uint8_t* list, int* count, int limit) {
    *count = 0;
    for (uint8_t* node = list; node != nullptr && *count < limit; node = mem::ptr<uint8_t>(node, 0)) {
        ++*count;
    }
}

namespace {

const std::uint16_t* textTable(std::uint32_t slot) {
    return reinterpret_cast<const std::uint16_t*>(G.itemText + mem::u32(G.itemText, slot));
}

}  // namespace

// swd32.exe 0x0043BE40: the record of the table at +0x58 of the item text whose word list (after its three header
// words, 0xFFFF terminated) holds `id`; null when none
const std::uint16_t* FindTextRecord(int id) {
    const std::uint16_t* record = textTable(0x58);
    if (*record == 0xFFFF) {
        return nullptr;
    }
    for (;;) {
        const std::uint16_t* entry = record + 3;
        while (*entry != 0xFFFF) {
            if (*entry == static_cast<std::uint32_t>(id)) {
                return record;
            }
            ++entry;
        }
        if (entry[1] == 0xFFFF) {
            return nullptr;
        }
        record = entry + 1;
    }
}

// swd32.exe 0x0043BE90: rebuilds G.unlockedEntries from the table at +0x5C of the item text: a name (bytes until
// the word 0x5125), six bytes and a 0xFFFF terminated list of game flags (+0x1388); entries with a set flag are
// kept as (four bytes, two bytes, name). The return value is unused by callers.
void BuildUnlockedEntries() {
    crt::free(G.unlockedEntries);
    G.unlockedEntries = static_cast<uint8_t**>(crt::malloc(0x800));
    G.unlockedCount = 0;
    auto* record = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(textTable(0x5C)));
    if (mem::u16(record, 0) == 0xFFFF) {
        return;
    }
    for (;;) {
        std::memset(G.menuText, 0, sizeof(G.menuText));
        bool unlocked = false;
        int length = 0;
        while (mem::u16(record, 0) != 0x5125) {
            G.menuText[length++] = static_cast<char>(*record);
            ++record;
        }
        uint8_t* data = record + 2;
        const uint8_t* flag = data + 6;
        while (mem::u16(flag, 0) != 0xFFFF) {
            if (GameFlag(mem::u16(flag, 0) + 0x1388)) {
                unlocked = true;
            }
            flag += 2;
        }
        if (unlocked) {
            auto* entry = static_cast<uint8_t*>(
                crt::malloc(static_cast<std::uint32_t>(w32::lstrlenA(G.menuText) + 7)));
            G.unlockedEntries[G.unlockedCount] = entry;
            mem::put32(entry, 0, mem::u32(data, 0));
            mem::put16(entry, 4, mem::u16(data, 4));
            w32::lstrcpyA(reinterpret_cast<char*>(G.unlockedEntries[G.unlockedCount] + 6), G.menuText);
            ++G.unlockedCount;
        }
        if (mem::u16(flag, 2) == 0xFFFF) {
            return;
        }
        record = const_cast<uint8_t*>(flag + 2);
    }
}

// swd32.exe 0x0043C090: key `key` (16 bytes of state each) just pressed, or held more than 10 frames on an even
// frame
bool KeyPressedOrRepeated(int key) {
    const std::int32_t down = image::at<std::int32_t>(0x004B7CB0u + 16u * static_cast<std::uint32_t>(key));
    const std::int32_t frames = image::at<std::int32_t>(0x004B7CBCu + 16u * static_cast<std::uint32_t>(key));
    if (down == 0) {
        return false;
    }
    return frames == 1 || (frames > 0xA && frames % 2 == 0);
}

// swd32.exe 0x0043C2F0: frees the menu buffers and points the menu act at 0x232A / 0x43. The return value is
// unused by callers.
void FreeMenuBuffers() {
    uint8_t* item = reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FC918));
    if (mem::ptr<uint8_t>(item, 0xAC) != nullptr) {
        crt::free(mem::ptr<uint8_t>(item, 0xAC));
        mem::putPtr(reinterpret_cast<uint8_t*>(static_cast<std::uintptr_t>(G.dword_4FC918)), 0xAC,
                    static_cast<uint8_t*>(nullptr));
    }
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FC918)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCA78)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FCA7C)));
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FC930)));
    for (G.dword_4FC974 = 0; G.dword_4FC974 < 0x10; ++G.dword_4FC974) {
        crt::free(G.menuBuffers[G.dword_4FC974]);
    }
    for (G.dword_4FC974 = 0; G.dword_4FC974 < 0x40; ++G.dword_4FC974) {
        crt::free(G.menuStrings[G.dword_4FC974]);
    }
    crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.dword_4FC91C)));
    G.dword_4FB970 = 0x232A;
    G.dword_4FB978 = 0x43;
}

// swd32.exe 0x0043CBD0: counts the non-null pointers at G.dword_4FC920 (at most 15) into G.dword_4FC914. The
// return value is unused by callers.
void CountMenuPointers() {
    const auto* entry = reinterpret_cast<const std::uint32_t*>(static_cast<std::uintptr_t>(G.dword_4FC920));
    int count = 0;
    G.dword_4FC914 = 0;
    while (*entry != 0 && count < 0xF) {
        ++count;
        ++entry;
        G.dword_4FC914 = count;
    }
}

// swd32.exe 0x0043CC00
std::uint32_t* AdvanceByDwords(int count, std::uint32_t base, std::uint32_t* out) {
    *out = base;
    for (int i = count; i > 0; --i) {
        *out += 4;
    }
    return out;
}

// swd32.exe 0x0043D370: "name value" for a stat the viewer may misjudge: exact without a level gap (`level` -
// `known` <= 0), "大概是" / "似乎是" with a random error of a tenth / a hundredth / a thousandth range divided by
// (3 - gap) for gaps 1 / 2, " ???" from 3 (with '?' at 7 above 1000). The return value is unused by callers.
void FormatEstimate(char* out, const char* name, int known, int level, int value, int maximum) {
    int range = 0xA;
    if (value > 0x64) {
        range = 0x64;
    }
    if (value > 0x3E8) {
        range = 0x3E8;
    }
    const int gap = level - known;
    w32::wsprintfA(out, G.nameColumnText, name);
    if (gap <= 0) {
        w32::wsprintfA(out, G.exactValueText, name, value);
        return;
    }
    if (gap >= 3) {
        w32::lstrcatA(out, " ???");
        if (maximum > 0x3E8) {
            out[7] = '?';
        }
        return;
    }
    const int spread = range / (3 - gap);
    int shown = static_cast<int>(R250Below(static_cast<std::uint32_t>(spread))) - spread / 2 + value;
    if (shown < 0) {
        shown = 0;
    }
    if (shown > maximum) {
        shown = maximum;
    }
    if (gap == 1) {
        w32::wsprintfA(out, G.probablyText, name, shown);
    } else if (gap == 2) {
        w32::wsprintfA(out, G.seemsText, name, shown);
    }
}

// swd32.exe 0x0043D470: the menu tabs (picture 0x2439, 40 pixels apart) around the current tab G.dword_4FC910, which
// is drawn from picture 0x243A. The return value is unused by callers.
void DrawMenuTabs() {
    SetClipRectClamped(0xA, 1, 0xCE, 0x1DE);
    int current = G.dword_4FC910;
    int x = 6;
    for (int tab = current - 2; tab <= current + 2; ++tab) {
        if (static_cast<std::uint32_t>(tab) < 0xC && tab != current) {
            Picture* picture = TswGetPicture(0x2439, static_cast<std::uint32_t>(tab));
            G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
            DrawPicture(x, 0x3D, picture->width, picture->height, 0, 0);
            current = G.dword_4FC910;
        }
        x += 0x28;
    }
    Picture* selected = TswGetPicture(0x243A, static_cast<std::uint32_t>(current));
    G.drawSource = reinterpret_cast<uint8_t*>(selected->pixels);
    DrawPicture(0x56, 0x3A, selected->width, selected->height, 0, 0);
    SetClipRectClamped(0, 1, 0x280, 0x1DE);
}

// swd32.exe 0x00449C30: one step of `state` drawn darkened by -25 - `level`, then from `level` up to -13 echoes
// moving out to both sides (offset -12 - `level` falling by one per echo, the left one at half the distance), each
// lighter by one. A failed step is logged through the empty function at 0x0044A240.
int DrawActAlpha(ActState* state, int x, int y, int level) {
    if (ActRun(state) == 0) {
        return DebugLog("Act Err(Menu::DrawActAlpha)", reinterpret_cast<const char*>(&G.unk_49FD0C), state->id,
                        state->action, state->direction);
    }
    Picture* picture = TswGetPicture(state->fr, state->ap);
    const int dark = -0x19 - level;
    G.drawOption304 = dark;
    G.drawOption30C = dark;
    G.drawOption71C = dark;
    G.drawSource = reinterpret_cast<uint8_t*>(picture->pixels);
    int result = DrawPicture(x - state->x, y - state->y, picture->width, picture->height, 4, 0);
    int offset = -0xC - level;
    for (int shade = level; shade < -0xC; ++shade, --offset) {
        G.drawOption304 = shade;
        G.drawOption30C = shade;
        G.drawOption71C = shade;
        DrawPicture(offset - state->x + x, y - state->y, picture->width, picture->height, 4, 0);
        G.drawOption304 = shade;
        G.drawOption30C = shade;
        G.drawOption71C = shade;
        result = DrawPicture(x - offset / 2 - state->x, y - state->y, picture->width, picture->height, 4, 0);
    }
    return result;
}

// swd32.exe 0x0044AE70: a signed change ("%c%-3d") placed after the digits of `value`: white, dark red when
// negative; nothing for 0
void DrawSignedChange(int change, int x, int y, int value) {
    if (change == 0) {
        return;
    }
    std::uint32_t color = MakeColor(0x1F, 0x1F, 0x1F);
    if (change < 0) {
        color = MakeColor(0x1A, 0, 0);
    }
    int shift = 0;
    if (value >= 0x3E8) {
        shift = 0x2C;
    } else if (value >= 0x64) {
        shift = 0x21;
    } else if (value >= 0xA) {
        shift = 0x16;
    } else if (value >= 1) {
        shift = 0xB;
    }
    w32::wsprintfA(G.menuText, "%c%-3d", change <= 0 ? '-' : '+', change < 0 ? -change : change);
    G.textFont_4A9ED0.DrawText(reinterpret_cast<std::uint16_t*>(G.drawSurface), shift + x + 4, y, G.menuText,
                               static_cast<std::uint16_t>(color), 4);
}

// swd32.exe 0x0043C800: frees the menu buffers once the close counter G.word_4FC900 reached 500 (then 2)
void CloseMenuWhenDone() {
    if (G.word_4FC900 == 0x1F4) {
        G.word_4FC900 = 2;
        FreeMenuBuffers();
    }
}

// swd32.exe 0x00445420: runs the handler of page G.dword_4FC3C4 from the table at 0x004FBF54 (pages 11.. start at
// 0x004FBF80)
void RunPageHandler() {
    codeAs<void()>(image::at<code_ptr>(0x004FBF54u + 4u * static_cast<std::uint16_t>(G.dword_4FC3C4)))();
}

}  // namespace swd3
