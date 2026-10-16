#include "battle/battle_message.h"

#include <cstring>
#include <string>

#include "battle/battle_hud.h"
#include "battle/battle_state.h"
#include "battle/battle_text.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "org/object_record.h"
#include "res/act_archive.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr Pixel kPanelTextColor = 0xFFC0;
constexpr Pixel kMessageTextColor = 0xFFFF;
constexpr int kTextStyle = 0x10;
constexpr int kFrameBorder = BattleMessage::Frame | 8;
constexpr u32 kBlack = 0xFF;
constexpr int kElements = 9;

int HalfWidth(const BattleMessage& message)
{
    return (0x16 * (8 - (message.length >> 1))) >> 1;
}

}  // namespace

BattleMessages::BattleMessages(
    Painter& painter,
    TswArchive& archive,
    ActArchive& acts,
    BattleHud& hud,
    BattleState& state,
    TextFont& panelFont,
    TextFont& messageFont
)
    : painter(painter),
      archive(archive),
      acts(acts),
      hud(hud),
      state(state),
      panelFont(panelFont),
      messageFont(messageFont)
{
}

// The frame picture of the panels: act 0x233B, action 0.
int BattleMessages::PanelFrame()
{
    this->panelAct.id = BattleMessages::PanelAct;
    this->panelAct.action = 0;
    this->acts.Run(this->panelAct);
    return this->panelAct.frame;
}

void BattleMessages::DrawPanelText(int x, int y, std::string_view text)
{
    this->panelFont.DrawText(this->painter.Target(), x, y, text, kPanelTextColor, kTextStyle);
}

void BattleMessages::DrawPanelText(int x, int y, std::u8string_view text)
{
    this->DrawPanelText(x, y, Big5(text));
}

void BattleMessages::DrawMessageText(int x, int y, std::string_view text)
{
    this->messageFont.DrawText(this->painter.Target(), x, y, text, kMessageTextColor, kTextStyle);
}

std::string_view BattleMessage::Text() const
{
    const std::string_view all(this->source.data(), this->source.size());
    return all.substr(0, all.find('\0'));
}

// swd32.exe 0x00469080: the "戰利品" panel with one "%-12s X %2d" line per item
void BattleMessages::DrawLootPanel()
{
    const int bottom = 0x14 * this->lootCount + 0xD4;
    this->panelFont.SetAdvance(0x12);
    const int frame = this->PanelFrame();
    FillBox(this->painter, 0xC4, 0xB0, 0xB8, this->panelHeight + 0x28, 0, 4, 4, FillMode::Blend);
    DrawWindowFrame(this->painter, this->archive, frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kFrameBorder);
    this->DrawPanelText(0x108, 0xB4, battle_text::kLootTitle);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        0xC8,
        0xD4,
        0x178,
        this->panelHeight + 0xD4,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(0xD4, bottom, 3) == 1 && this->lootCount != 0)
    {
        int y = 0xD4;

        for (int item = 0; item < this->lootCount; ++item)
        {
            const std::string line = WinFormat(
                battle_text::kLootLine,
                {this->lootNames[static_cast<std::size_t>(item)], this->lootCounts[static_cast<std::size_t>(item)]}
            );
            this->DrawPanelText(0xD2, y, line);
            y += 0x14;
        }
    }

    this->panelFont.SetAdvance(0x10);
}

// swd32.exe 0x00469220: the "戰鬥失敗" panel with "隊伍全滅!!"
void BattleMessages::DrawDefeatPanel()
{
    const int frame = this->PanelFrame();
    FillBox(this->painter, 0xC4, 0xB0, 0xB8, this->panelHeight + 0x28, 0, 4, 4, FillMode::Blend);
    DrawWindowFrame(this->painter, this->archive, frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kFrameBorder);
    this->DrawPanelText(0x104, 0xB4, battle_text::kDefeatTitle);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        0xC8,
        0xD4,
        0x178,
        this->panelHeight + 0xD4,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(0xD4, 0xF4, 3) == 1)
    {
        this->panelFont.SetAdvance(0x11);
        this->DrawPanelText(0xFE, 0xD8, battle_text::kPartyWiped);
        this->panelFont.SetAdvance(0x10);
    }
}

// swd32.exe 0x00469340: "煉符成功" with "得到符咒:%s" of the first loot name, or "煉符失敗" with "沒有得到東西"
void BattleMessages::DrawCraftResultPanel()
{
    const int frame = this->PanelFrame();
    FillBox(this->painter, 0xC4, 0xB0, 0xB8, this->panelHeight + 0x28, 0, 4, 4, FillMode::Blend);
    DrawWindowFrame(this->painter, this->archive, frame, 0xC8, 0xB4, 0x178, 0xC4, 0, kFrameBorder);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        0xC8,
        0xD4,
        0x178,
        this->panelHeight + 0xD4,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(0xD4, 0xFC, 3) != 1)
    {
        return;
    }

    if (this->craftSucceeded == 1)
    {
        this->DrawPanelText(0x100, 0xB4, battle_text::kCraftSuccess);
        const std::string line = WinFormat(battle_text::kCraftGot, {this->lootNames[0]});
        this->DrawPanelText(0xD0, 0xDE, line);
    }
    else
    {
        this->DrawPanelText(0x100, 0xB4, battle_text::kCraftFailed);
        this->DrawPanelText(0xD0, 0xDE, battle_text::kNothingGained);
    }
}

// swd32.exe 0x00469550: a shaded framed box; the text goes to (textX, textY) unless both are 0
void BattleMessages::DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::string_view text)
{
    this->panelAct.id = BattleMessages::PanelAct;
    this->panelAct.action = 0;
    this->acts.Run(this->panelAct);
    FillBox(this->painter, x, y, width, height, 0, 4, 4, FillMode::Blend);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        x + 4,
        y + 4,
        x + width - 4,
        y + height - 4,
        0,
        kFrameBorder
    );

    if (textX != 0 || textY != 0)
    {
        this->DrawPanelText(textX, textY, text);
    }
    else
    {
        this->DrawPanelText(x + 2, y + 4, text);
    }
}

// swd32.exe 0x00469620: grows the panel by 1 / divisor of the rest; 1 once it stopped
int BattleMessages::GrowPanel(int top, int bottom, int divisor)
{
    const int step = (bottom - this->panelHeight - top) / divisor;
    this->panelHeight += step;
    return step == 0 ? 1 : 0;
}

// swd32.exe 0x00469650: the name and the nine element resistances (signed tenths) of the panel object as bars, while
// bit 5 of the battle UI flags is set. The original also formats the percentage of each bar, never drawn.
void BattleMessages::DrawResistancePanel()
{
    u32 color = 0;
    float length = 0.0F;

    if ((this->state.panelFlags & 0x20) == 0)
    {
        return;
    }

    const int frame = this->PanelFrame();
    FillBox(this->painter, 0xEC, 0x56, 0xB8, this->panelHeight + 0x28, 0, 4, 4, FillMode::Blend);
    DrawWindowFrame(this->painter, this->archive, frame, 0xF0, 0x5A, 0x1A0, 0x6A, 0, kFrameBorder);
    this->DrawPanelText(0x130, 0x5A, this->panelUnit->name.data());
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        0xF0,
        0x7A,
        0x1A0,
        this->panelHeight + 0x7A,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(0x7A, 0x12E, 3) != 1)
    {
        return;
    }

    int y = 0x82;

    for (int element = 0; element < kElements; ++element)
    {
        const auto resistance = static_cast<i8>(this->panelUnit->resistances[static_cast<std::size_t>(element)]);

        if (resistance == 0)
        {
            color = this->painter.MakeColor(0x10, 0x10, 0x10);
            length = 2.0F;
        }

        if (resistance > 0)
        {
            color = this->painter.MakeColor(0x1C, 2, 2);
            length = static_cast<float>(static_cast<double>(resistance) * 0.1 * 120.0);
        }

        if (resistance < 0)
        {
            color = this->painter.MakeColor(2, 0xD, 0x1C);
            length = static_cast<float>(static_cast<double>(resistance) * -0.1 * 120.0);
        }

        if (resistance < -10)
        {
            color = this->painter.MakeColor(2, 0x1C, 0xD);
            length = static_cast<float>(static_cast<double>(resistance + 10) * -0.1 * 120.0);
        }

        const u32 white = this->painter.MakeColor(0x1F, 0x1F, 0x1F);
        this->panelFont.DrawText(
            this->painter.Target(),
            0xFC,
            y - 8,
            battle_text::kElementNames[element],
            static_cast<Pixel>(white),
            kTextStyle
        );
        this->hud.FillColor(0x11C, y, static_cast<i32>(crt::Ftol(length)), 3, color);
        y += 0x14;
    }
}

// swd32.exe 0x004698E0: appends a message; kRise starts with speed 1 at y -32
void BattleMessages::Add(int x, int y, i16 frames, std::span<const char> text, int flags)
{
    auto message = std::make_unique<BattleMessage>();
    message->source = text;
    this->Append(std::move(message), x, y, frames, flags);
}

// A constant text (the original passes the address of the string in the executable).
void BattleMessages::Add(int x, int y, i16 frames, std::u8string_view text, int flags)
{
    auto message = std::make_unique<BattleMessage>();
    message->constant = Big5(text);
    message->source = message->constant;
    this->Append(std::move(message), x, y, frames, flags);
}

void BattleMessages::Append(std::unique_ptr<BattleMessage> message, int x, int y, i16 frames, int flags)
{
    message->x = x;
    message->y = y;
    message->frames = frames;
    message->length = static_cast<i32>(message->Text().size());
    message->flags |= flags;

    if ((flags & BattleMessage::Rise) != 0)
    {
        message->y = 1;
        message->slideY = -0x20;
    }

    std::unique_ptr<BattleMessage>* link = &this->messages;

    while (*link != nullptr)
    {
        link = &(*link)->next;
    }

    *link = std::move(message);
}

// swd32.exe 0x00469960: draws the messages and counts their frames down; expired ones slide out (unless the battle is
// paused) and are freed once off screen. The boxes of expired messages are still drawn.
void BattleMessages::Draw()
{
    if (this->messages == nullptr)
    {
        return;
    }

    const bool paused = this->state.paused != 0;

    for (BattleMessage* message = this->messages.get(); message != nullptr; message = message->next.get())
    {
        int y = message->y + 6;
        int x = message->x + 0x10;

        if (message->flags < 0)
        {
            this->panelAct.id = BattleMessages::PanelAct;
            this->panelAct.action = 0;
            this->acts.Run(this->panelAct);
            FillBox(this->painter, 0xA, 0xA, 0x26C, 0x20, -8, -8, 0xA, FillMode::Tint);
            DrawWindowFrame(this->painter, this->archive, this->panelAct.frame, 0xE, 0xE, 0x272, 0x26, 0, kFrameBorder);
        }

        if ((message->flags & BattleMessage::BlackBar) != 0)
        {
            this->hud.FillColor(0xA, 0xA, 0x26C, 0x20, kBlack);
        }

        const int flags = message->flags;

        if ((flags & BattleMessage::Left) != 0)
        {
            x = 0x12;
        }

        if ((flags & BattleMessage::Centered) != 0)
        {
            x = 0x13B - ((0x14 * (message->length >> 1)) >> 1);
        }

        if (message->frames <= 0)
        {
            continue;
        }

        if ((flags & BattleMessage::FromRight) != 0)
        {
            y = 0x10;
            const int half = HalfWidth(*message);

            if (message->slideX == 0)
            {
                message->slideX = 0x280;
            }

            if (!paused)
            {
                message->slideX = (half + 0xE3 + message->slideX) >> 1;
            }

            x = message->slideX;
            this->hud.FillColor(x - half, 0xA, 0xB0, 0x20, kBlack);
        }

        if ((message->flags & BattleMessage::FromLeft) != 0)
        {
            y = 0x10;
            const int half = HalfWidth(*message);

            if (!paused)
            {
                message->slideX = (message->slideX + half + 0xE3) >> 1;
            }

            x = message->slideX;
            this->hud.FillColor(x - half, 0xA, 0xB0, 0x20, kBlack);
        }

        if ((message->flags & BattleMessage::Rise) != 0)
        {
            x = HalfWidth(*message) + 0xE3;
            y = (message->slideY + 0x20) / 2;
            message->slideY = y;
            message->slideX = x;
            this->hud.FillColor(0xE3, y - 6, 0xB0, 0x20, kBlack);
        }

        this->DrawMessageText(x, y, message->Text());
        --message->frames;
    }

    std::unique_ptr<BattleMessage>* link = &this->messages;

    while (*link != nullptr)
    {
        BattleMessage& message = **link;
        bool keep = message.frames > 0;

        if (!keep && (message.flags & BattleMessage::FromLeft) != 0)
        {
            const int half = HalfWidth(message);

            if (!paused)
            {
                message.slideX += 0x50;
            }

            this->hud.FillColor(message.slideX - half, 0xA, 0xB0, 0x20, kBlack);
            this->DrawMessageText(message.slideX, 0x10, message.Text());
            keep = message.slideX < 0x280;
        }

        if (!keep && (message.flags & BattleMessage::FromRight) != 0)
        {
            const int half = HalfWidth(message);

            if (!paused)
            {
                message.slideX -= 0x50;
            }

            this->hud.FillColor(message.slideX - half, 0xA, 0xB0, 0x20, kBlack);
            this->DrawMessageText(message.slideX, 0x10, message.Text());
            keep = message.slideX > 0;
        }

        if (!keep && (message.flags & BattleMessage::Rise) != 0)
        {
            const int half = HalfWidth(message);

            if (!paused)
            {
                message.slideY -= message.y;
            }

            message.y *= 2;
            this->hud.FillColor(message.slideX - half, message.slideY - 6, 0xB0, 0x20, kBlack);
            this->DrawMessageText(message.slideX, message.slideY, message.Text());
            keep = message.slideY > -0x20;
        }

        if (keep)
        {
            link = &message.next;
        }
        else
        {
            *link = std::move(message.next);
        }
    }
}

}  // namespace swd3
