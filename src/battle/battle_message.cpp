#include "battle/battle_message.h"

#include <cstring>

#include "battle/battle_draw.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "font/text_font.h"
#include "pic/pixel_format.h"
#include "res/act_anim.h"
#include "swd3/w32.h"
#include "ui/ui_draw.h"

namespace swd3 {

namespace {

std::uint16_t* surface() { return reinterpret_cast<std::uint16_t*>(G.drawSurface); }

// frame picture of the panels: act 0x233B, action 0
int panelFrame() {
    G.panelAct.id = 0x233B;
    G.panelAct.action = 0;
    ActRun(&G.panelAct);
    return G.panelAct.fr;
}

void drawPanelText(int x, int y, const char* text) {
    G.textFont_4C9A28.DrawText(surface(), x, y, text, 0xFFC0, 0x10);
}

void drawMessageText(int x, int y, const char* text) {
    G.textFont_4AB998.DrawText(surface(), x, y, text, 0xFFFF, 0x10);
}

int halfWidth(const BattleMessage* message) { return (0x16 * (8 - (message->length >> 1))) >> 1; }

}  // namespace

// swd32.exe 0x004694E0: takes the first non-zero entry of the 4 x 10 queue unless one is pending
void TakePendingAction() {
    if (G.pendingAction != 0) {
        return;
    }
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 10; ++column) {
            std::int16_t& entry = G.actionQueue[row * 10 + column];
            if (entry != 0) {
                G.pendingActionRow = static_cast<std::int16_t>(row);
                const std::int16_t action = entry;
                entry = 0;
                G.pendingAction = action;
                return;
            }
        }
    }
}

// swd32.exe 0x00469080: "戰利品" panel with one "%-12s X %2d" line per item; the return value is unused
void DrawLootPanel() {
    char line[64] = {};
    line[0] = G.ValueName;
    const int bottom = 0x14 * static_cast<std::uint16_t>(G.lootCount) + 0xD4;
    G.textFont_4C9A28.SetAdvance(0x12);
    const int frame = panelFrame();
    FillBoxMode(0xC4, 0xB0, 0xB8, G.panelHeight + 0x28, 0, 4, 4, kFillBlend);
    DrawWindowFrame(frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kMessageFrame | 8);
    drawPanelText(0x108, 0xB4, G.lootTitleText);
    DrawWindowFrame(G.panelAct.fr, 0xC8, 0xD4, 0x178, G.panelHeight + 0xD4, 0, kMessageFrame | 8);
    if (GrowPanel(0xD4, bottom, 3) == 1 && G.lootCount != 0) {
        int y = 0xD4;
        int item = 0;
        do {
            w32::wsprintfA(line, G.a12sX2d, G.lootNames[item], G.lootCounts[item]);
            drawPanelText(0xD2, y, line);
            ++item;
            y += 0x14;
        } while (item < static_cast<std::uint16_t>(G.lootCount));
    }
    G.textFont_4C9A28.SetAdvance(0x10);
}

// swd32.exe 0x00469220: "戰鬥失敗" panel with "隊伍全滅!!"
void DrawDefeatPanel() {
    const int frame = panelFrame();
    FillBoxMode(0xC4, 0xB0, 0xB8, G.panelHeight + 0x28, 0, 4, 4, kFillBlend);
    DrawWindowFrame(frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kMessageFrame | 8);
    drawPanelText(0x104, 0xB4, G.defeatTitleText);
    DrawWindowFrame(G.panelAct.fr, 0xC8, 0xD4, 0x178, G.panelHeight + 0xD4, 0, kMessageFrame | 8);
    if (GrowPanel(0xD4, 0xF4, 3) == 1) {
        G.textFont_4C9A28.SetAdvance(0x11);
        drawPanelText(0xFE, 0xD8, G.partyWipedText);
        G.textFont_4C9A28.SetAdvance(0x10);
    }
}

// swd32.exe 0x00469340: "煉符成功" with "得到符咒:%s" of the first loot name, or "煉符失敗" / "沒有得到東西"
void DrawCraftResultPanel() {
    char line[64] = {};
    line[0] = G.ValueName;
    const int frame = panelFrame();
    FillBoxMode(0xC4, 0xB0, 0xB8, G.panelHeight + 0x28, 0, 4, 4, kFillBlend);
    DrawWindowFrame(frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kMessageFrame | 8);
    DrawWindowFrame(G.panelAct.fr, 0xC8, 0xD4, 0x178, G.panelHeight + 0xD4, 0, kMessageFrame | 8);
    if (GrowPanel(0xD4, 0xFC, 3) != 1) {
        return;
    }
    if (G.craftSucceeded == 1) {
        drawPanelText(0x100, 0xB4, G.craftSuccessText);
        w32::wsprintfA(line, G.craftGotText, G.lootNames[0]);
        drawPanelText(0xD0, 0xDE, line);
    } else {
        drawPanelText(0x100, 0xB4, G.craftFailedText);
        drawPanelText(0xD0, 0xDE, G.nothingGainedText);
    }
}

// swd32.exe 0x00469550: shaded framed box; the text goes to (textX, textY) unless both are 0
void DrawMessageBox(int x, int y, int width, int height, int textX, int textY, const char* text) {
    G.panelAct.id = 0x233B;
    G.panelAct.action = 0;
    ActRun(&G.panelAct);
    FillBoxMode(x, y, width, height, 0, 4, 4, kFillBlend);
    DrawWindowFrame(G.panelAct.fr, x + 4, y + 4, x + width - 4, y + height - 4, 0, kMessageFrame | 8);
    if (textX != 0 || textY != 0) {
        drawPanelText(textX, textY, text);
    } else {
        drawPanelText(x + 2, y + 4, text);
    }
}

// swd32.exe 0x00469620: grows G.panelHeight by 1 / divisor of the rest; 1 once it stopped
int GrowPanel(int top, int bottom, int divisor) {
    const int step = (bottom - G.panelHeight - top) / divisor;
    G.panelHeight += step;
    return step == 0;
}

// swd32.exe 0x00469650: name and the nine element resistances (signed tenths at +0x92) of G.panelUnit as bars,
// when bit 5 of G.battleUiFlags is set. The percentage text is formatted but never drawn.
void DrawResistancePanel() {
    char text[8] = {};
    text[0] = G.ValueName;
    std::uint32_t color = 0;
    float length = 0.0f;
    if ((G.battleUiFlags & 0x20) == 0) {
        return;
    }
    const int frame = panelFrame();
    FillBoxMode(0xEC, 0x56, 0xB8, G.panelHeight + 0x28, 0, 4, 4, kFillBlend);
    DrawWindowFrame(frame, 0xF0, 0x5A, 0x1A0, 0x6A, 0, kMessageFrame | 8);
    drawPanelText(0x130, 0x5A, reinterpret_cast<const char*>(G.panelUnit));
    DrawWindowFrame(G.panelAct.fr, 0xF0, 0x7A, 0x1A0, G.panelHeight + 0x7A, 0, kMessageFrame | 8);
    const auto* resistance = reinterpret_cast<const std::int8_t*>(G.panelUnit + 0x92);
    if (GrowPanel(0x7A, 0x12E, 3) != 1) {
        return;
    }
    int y = 0x82;
    for (int element = 0; element < 9; ++element) {
        if (*resistance == 0) {
            color = MakeColor(0x10, 0x10, 0x10);
            w32::lstrcpyA(text, "--");
            length = 2.0f;
        }
        if (*resistance > 0) {
            color = MakeColor(0x1C, 2, 2);
            w32::wsprintfA(text, "%2d0%%", *resistance);
            length = static_cast<float>(static_cast<double>(*resistance) * 0.1 * 120.0);
        }
        if (*resistance < 0) {
            color = MakeColor(2, 0xD, 0x1C);
            w32::wsprintfA(text, "%2d0%%", -*resistance);
            length = static_cast<float>(static_cast<double>(*resistance) * -0.1 * 120.0);
        }
        if (*resistance < -10) {
            color = MakeColor(2, 0x1C, 0xD);
            w32::wsprintfA(text, "%2d0%%", -*resistance);
            length = static_cast<float>(static_cast<double>(*resistance + 10) * -0.1 * 120.0);
        }
        const std::uint32_t white = MakeColor(0x1F, 0x1F, 0x1F);
        G.textFont_4C9A28.DrawText(surface(), 0xFC, y - 8, G.elementNames[element], static_cast<std::uint16_t>(white),
                                   0x10);
        FillBox(0x11C, y, static_cast<std::int32_t>(crt::ftol(length)), 3, color);
        ++resistance;
        y += 0x14;
    }
}

// swd32.exe 0x004698E0: appends a message; kMessageRise starts with speed 1 at y -32
void AddBattleMessage(int x, int y, std::int16_t frames, const char* text, int flags) {
    auto* message = static_cast<BattleMessage*>(crt::malloc(sizeof(BattleMessage)));
    std::memset(message, 0, sizeof(BattleMessage));
    message->x = x;
    message->y = y;
    message->frames = frames;
    message->text = text;
    message->length = w32::lstrlenA(text);
    message->flags |= flags;
    if ((flags & kMessageRise) != 0) {
        message->y = 1;
        message->slideY = -0x20;
    }
    BattleMessage** link = &G.battleMessages;
    while (*link != nullptr) {
        link = &(*link)->next;
    }
    *link = message;
    message->next = nullptr;
}

// swd32.exe 0x00469960: draws the messages and counts their frames down; expired ones slide out (unless
// G.battlePaused) and are freed once off screen
void DrawBattleMessages() {
    if (G.battleMessages == nullptr) {
        return;
    }
    for (BattleMessage* message = G.battleMessages; message != nullptr; message = message->next) {
        int y = message->y + 6;
        int x = message->x + 0x10;
        if (message->flags < 0) {
            G.panelAct.id = 0x233B;
            G.panelAct.action = 0;
            ActRun(&G.panelAct);
            FillBoxMode(0xA, 0xA, 0x26C, 0x20, -8, -8, 0xA, kFillTint);
            DrawWindowFrame(G.panelAct.fr, 0xE, 0xE, 0x272, 0x26, 0, kMessageFrame | 8);
        }
        if ((message->flags & kMessageBlackBar) != 0) {
            FillBox(0xA, 0xA, 0x26C, 0x20, 0xFF);
        }
        const int flags = message->flags;
        if ((flags & kMessageLeft) != 0) {
            x = 0x12;
        }
        if ((flags & kMessageCentered) != 0) {
            x = 0x13B - ((0x14 * (message->length >> 1)) >> 1);
        }
        if (message->frames <= 0) {
            continue;
        }
        if ((flags & kMessageFromRight) != 0) {
            y = 0x10;
            const int half = halfWidth(message);
            if (message->slideX == 0) {
                message->slideX = 0x280;
            }
            if (G.battlePaused == 0) {
                message->slideX = (half + 0xE3 + message->slideX) >> 1;
            }
            x = message->slideX;
            FillBox(x - half, 0xA, 0xB0, 0x20, 0xFF);
        }
        if ((message->flags & kMessageFromLeft) != 0) {
            y = 0x10;
            const int half = halfWidth(message);
            if (G.battlePaused == 0) {
                message->slideX = (message->slideX + half + 0xE3) >> 1;
            }
            x = message->slideX;
            FillBox(x - half, 0xA, 0xB0, 0x20, 0xFF);
        }
        if ((message->flags & kMessageRise) != 0) {
            x = halfWidth(message) + 0xE3;
            y = (message->slideY + 0x20) / 2;
            message->slideY = y;
            message->slideX = x;
            FillBox(0xE3, y - 6, 0xB0, 0x20, 0xFF);
        }
        drawMessageText(x, y, message->text);
        --message->frames;
    }
    BattleMessage** link = &G.battleMessages;
    BattleMessage* message = G.battleMessages;
    while (message != nullptr) {
        bool keep = message->frames > 0;
        if (!keep) {
            if ((message->flags & kMessageFromLeft) != 0) {
                const int half = halfWidth(message);
                if (G.battlePaused == 0) {
                    message->slideX += 0x50;
                }
                const int x = message->slideX;
                FillBox(x - half, 0xA, 0xB0, 0x20, 0xFF);
                drawMessageText(x, 0x10, message->text);
                keep = message->slideX < 0x280;
            }
            if (!keep && (message->flags & kMessageFromRight) != 0) {
                const int half = halfWidth(message);
                if (G.battlePaused == 0) {
                    message->slideX -= 0x50;
                }
                const int x = message->slideX;
                FillBox(x - half, 0xA, 0xB0, 0x20, 0xFF);
                drawMessageText(x, 0x10, message->text);
                keep = message->slideX > 0;
            }
            if (!keep && (message->flags & kMessageRise) != 0) {
                const int half = halfWidth(message);
                if (G.battlePaused == 0) {
                    message->slideY -= message->y;
                }
                const int x = message->slideX;
                const int y = message->slideY;
                message->y *= 2;
                FillBox(x - half, y - 6, 0xB0, 0x20, 0xFF);
                drawMessageText(x, y, message->text);
                keep = message->slideY > -0x20;
            }
        }
        if (keep) {
            link = &message->next;
            message = message->next;
        } else {
            *link = message->next;
            crt::free(message);
            message = *link;
        }
    }
}

}  // namespace swd3
