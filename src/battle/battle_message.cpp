#include "battle/battle_message.h"
#include "text/big5.h"

#include <array>
#include <cstring>
#include <string>

#include "battle/battle_hud.h"
#include "battle/battle_message_constants.h"
#include "battle/battle_state.h"
#include "battle/smooth/smooth_messages.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "org/object_record.h"
#include "platform/display.h"
#include "res/act_archive.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr Pixel kPanelTextColor = 0xFFC0;
constexpr Pixel kMessageTextColor = 0xFFFF;
constexpr int kTextStyle = TextFont::BoldOutline;
constexpr int kElements = 9;

// The shade behind every panel: a blend towards (kShadeRed, kShadeGreen, kShadeBlue), kShadeExtra taller than the
// grown body; the frames sit kShadeMargin inside it.
constexpr int kShadeRed = 0;
constexpr int kShadeGreen = 4;
constexpr int kShadeBlue = 4;
constexpr int kShadeExtra = 0x28;
constexpr int kPanelShadeWidth = 0xB8;
constexpr int kGrowDivisor = 3;
constexpr int kPanelAdvance = 0x10;  // the usual glyph advance of the panel font

// The result panels (loot, defeat, craft): a title frame and a body frame growing below it.
constexpr int kResultShadeX = 0xC4;
constexpr int kResultShadeY = 0xB0;
constexpr int kResultLeft = kResultShadeX + kShadeMargin;
constexpr int kResultTitleTop = kResultShadeY + kShadeMargin;
constexpr int kResultRight = 0x178;
constexpr int kResultTitleBottom = 0xC4;
constexpr int kResultBodyTop = 0xD4;
constexpr int kLootTitleX = 0x108;
constexpr int kLootX = 0xD2;
constexpr int kLootRowHeight = 0x14;
constexpr int kLootAdvance = 0x12;
constexpr int kDefeatTitleX = 0x104;
constexpr int kDefeatBodyBottom = 0xF4;
constexpr int kWipedX = 0xFE;
constexpr int kWipedY = 0xD8;
constexpr int kWipedAdvance = 0x11;
constexpr int kCraftTitleX = 0x100;
constexpr int kCraftBodyBottom = 0xFC;
constexpr int kCraftLineX = 0xD0;
constexpr int kCraftLineY = 0xDE;

// DrawMastery.
constexpr int kMasteryCharWidth = 0x11;  // the advance of its text, and the width of two bytes
constexpr int kMasteryTextExtra = 0x20;
constexpr int kMasteryShadeY = 0xD0;
constexpr int kMasteryShadeExtra = 0xC;
constexpr int kMasteryShadeMargin = 8;
constexpr int kMasteryTextX = 0xD8;
constexpr int kMasteryTextY = 0xDA;

// DrawMessageBox: the text without a position starts this far inside the box.
constexpr int kBoxTextX = 2;
constexpr int kBoxTextY = 4;

// The resistance panel: the unit name over one bar per element.
constexpr int kResistShadeX = 0xEC;
constexpr int kResistShadeY = 0x56;
constexpr int kResistLeft = kResistShadeX + kShadeMargin;
constexpr int kResistTitleTop = kResistShadeY + kShadeMargin;
constexpr int kResistRight = 0x1A0;
constexpr int kResistTitleBottom = 0x6A;
constexpr int kResistNameX = 0x130;
constexpr int kResistBodyTop = 0x7A;
constexpr int kResistBodyBottom = 0x12E;
constexpr int kResistFirstBar = 0x82;
constexpr int kResistRowHeight = 0x14;
constexpr int kElementNameX = 0xFC;
constexpr int kElementNameRise = 8;  // the element name sits this far above its bar
constexpr int kBarX = 0x11C;
constexpr int kBarHeight = 3;
constexpr float kNeutralBarLength = 2.0F;
constexpr double kTenth = 0.1;       // resistances are in tenths
constexpr double kBarScale = 120.0;  // pixels of a full resistance
constexpr i8 kAbsorbBelow = -10;     // a resistance below -10 tenths absorbs: green bar of the excess
constexpr int kGrayLevel = 0x10;
constexpr int kBrightLevel = 0x1C;
constexpr int kMiddleLevel = 0xD;
constexpr int kDarkLevel = 2;
constexpr int kWhiteLevel = 0x1F;

constexpr i32 kRiseSpeed = 1;  // the speed a rising message starts with

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
    const int bottom = kLootRowHeight * this->lootCount + kResultBodyTop;
    this->panelFont.SetAdvance(kLootAdvance);
    const int frame = this->PanelFrame();
    FillBox(
        this->painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        this->panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder
    );
    this->DrawPanelText(kLootTitleX, kResultTitleTop, BattleMessages::LootTitle);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        this->panelHeight + kResultBodyTop,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(kResultBodyTop, bottom, kGrowDivisor) == 1 && this->lootCount != 0)
    {
        int y = kResultBodyTop;

        for (int item = 0; item < this->lootCount; ++item)
        {
            const std::string line = WinFormat(
                BattleMessages::LootLine,
                {this->lootNames[static_cast<std::size_t>(item)], this->lootCounts[static_cast<std::size_t>(item)]}
            );
            this->DrawPanelText(kLootX, y, line);
            y += kLootRowHeight;
        }
    }

    this->panelFont.SetAdvance(kPanelAdvance);
}

// swd32.exe 0x00469220: the "戰鬥失敗" panel with "隊伍全滅!!"
void BattleMessages::DrawDefeatPanel()
{
    const int frame = this->PanelFrame();
    FillBox(
        this->painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        this->panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder
    );
    this->DrawPanelText(kDefeatTitleX, kResultTitleTop, BattleMessages::DefeatTitle);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        this->panelHeight + kResultBodyTop,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(kResultBodyTop, kDefeatBodyBottom, kGrowDivisor) == 1)
    {
        this->panelFont.SetAdvance(kWipedAdvance);
        this->DrawPanelText(kWipedX, kWipedY, BattleMessages::PartyWiped);
        this->panelFont.SetAdvance(kPanelAdvance);
    }
}

// swd32.exe 0x00469340: "煉符成功" with "得到符咒:%s" of the first loot name, or "煉符失敗" with "沒有得到東西"
void BattleMessages::DrawCraftResultPanel()
{
    const int frame = this->PanelFrame();
    FillBox(
        this->painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        this->panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        this->panelHeight + kResultBodyTop,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(kResultBodyTop, kCraftBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    if (this->craftSucceeded == 1)
    {
        this->DrawPanelText(kCraftTitleX, kResultTitleTop, BattleMessages::CraftSuccess);
        const std::string line = WinFormat(BattleMessages::CraftGot, {this->lootNames[0]});
        this->DrawPanelText(kCraftLineX, kCraftLineY, line);
    }
    else
    {
        this->DrawPanelText(kCraftTitleX, kResultTitleTop, BattleMessages::CraftFailed);
        this->DrawPanelText(kCraftLineX, kCraftLineY, BattleMessages::NothingGained);
    }
}

// swd32.exe 0x00469550: a shaded framed box; the text goes to (textX, textY) unless both are 0
void BattleMessages::DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::string_view text)
{
    this->panelAct.id = BattleMessages::PanelAct;
    this->panelAct.action = 0;
    this->acts.Run(this->panelAct);
    FillBox(this->painter, x, y, width, height, kShadeRed, kShadeGreen, kShadeBlue, FillMode::Blend);
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        x + kShadeMargin,
        y + kShadeMargin,
        x + width - kShadeMargin,
        y + height - kShadeMargin,
        0,
        kFrameBorder
    );

    if (textX != 0 || textY != 0)
    {
        this->DrawPanelText(textX, textY, text);
    }
    else
    {
        this->DrawPanelText(x + kBoxTextX, y + kBoxTextY, text);
    }
}

// DrawMessageBox with a UTF-8 text.
void BattleMessages::DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::u8string_view text)
{
    this->DrawMessageBox(x, y, width, height, textX, textY, Big5(text));
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

    if ((this->state.panelFlags & BattleState::ResistancePanel) == 0)
    {
        return;
    }

    const int frame = this->PanelFrame();
    FillBox(
        this->painter,
        kResistShadeX,
        kResistShadeY,
        kPanelShadeWidth,
        this->panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        frame,
        kResistLeft,
        kResistTitleTop,
        kResistRight,
        kResistTitleBottom,
        0,
        kFrameBorder
    );
    this->DrawPanelText(kResistNameX, kResistTitleTop, this->panelUnit->name.data());
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        kResistLeft,
        kResistBodyTop,
        kResistRight,
        this->panelHeight + kResistBodyTop,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(kResistBodyTop, kResistBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    int y = kResistFirstBar;

    for (int element = 0; element < kElements; ++element)
    {
        const auto resistance = static_cast<i8>(this->panelUnit->resistances[static_cast<std::size_t>(element)]);

        if (resistance == 0)
        {
            color = this->painter.MakeColor(kGrayLevel, kGrayLevel, kGrayLevel);
            length = kNeutralBarLength;
        }

        if (resistance > 0)
        {
            color = this->painter.MakeColor(kBrightLevel, kDarkLevel, kDarkLevel);
            length = static_cast<float>(static_cast<double>(resistance) * kTenth * kBarScale);
        }

        if (resistance < 0)
        {
            color = this->painter.MakeColor(kDarkLevel, kMiddleLevel, kBrightLevel);
            length = static_cast<float>(static_cast<double>(resistance) * -kTenth * kBarScale);
        }

        if (resistance < kAbsorbBelow)
        {
            color = this->painter.MakeColor(kDarkLevel, kBrightLevel, kMiddleLevel);
            length = static_cast<float>(static_cast<double>(resistance - kAbsorbBelow) * -kTenth * kBarScale);
        }

        const u32 white = this->painter.MakeColor(kWhiteLevel, kWhiteLevel, kWhiteLevel);
        this->panelFont.DrawText(
            this->painter.Target(),
            kElementNameX,
            y - kElementNameRise,
            BattleMessages::ElementNames[element],
            static_cast<Pixel>(white),
            kTextStyle
        );
        this->hud.FillColor(kBarX, y, static_cast<i32>(crt::Ftol(length)), kBarHeight, color);
        y += kResistRowHeight;
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
        message->y = kRiseSpeed;
        message->slideY = kRiseStart;
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
    smooth::MessagesView view;
    smooth::MessagesFrame(*this).Advance(view);
    smooth::MessagesFrame(*this).Draw(view);
}

// swd32.exe 0x00468ED0: while a proficiency has just filled (unk53BFFC), the box "法寶%s已完全成長!!" with the
// accessory's name: a shade and a frame as wide as the text (17 pixels per two bytes), grown open (GrowPanel), then
// the text at a 17-pixel advance.
void BattleMessages::DrawMastery()
{
    if (this->state.unk53BFFC != 1)
    {
        return;
    }

    const std::string text = WinFormat(BattleMessages::MasteryLine, {this->state.lastLearned.name});
    const int halves = static_cast<int>(text.size()) / 2;
    const int width = kMasteryCharWidth * halves + kMasteryTextExtra;
    FillBox(
        this->painter,
        kResultShadeX,
        kMasteryShadeY,
        width + kMasteryShadeExtra,
        this->panelHeight + kMasteryShadeMargin,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend
    );
    DrawWindowFrame(
        this->painter,
        this->archive,
        this->panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        width + kResultLeft,
        this->panelHeight + kResultBodyTop,
        0,
        kFrameBorder
    );

    if (this->GrowPanel(kResultBodyTop, kDefeatBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    this->panelFont.SetAdvance(kMasteryCharWidth);
    this->DrawPanelText(kMasteryTextX, kMasteryTextY, text);
    this->panelFont.SetAdvance(kPanelAdvance);
}
}  // namespace swd3
