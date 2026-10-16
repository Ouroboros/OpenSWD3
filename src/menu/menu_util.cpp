#include "menu/menu_util.h"

#include "core/debug_log.h"
#include "core/mem.h"
#include "data/globals.h"
#include "draw/draw.h"
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

}  // namespace swd3
