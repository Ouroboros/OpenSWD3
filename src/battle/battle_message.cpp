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

// The shade behind every panel: a blend towards (kShadeRed, kShadeGreen, kShadeBlue), kShadeExtra taller than the
// grown body; the frames sit kShadeMargin inside it.

// The result panels (loot, defeat, craft): a title frame and a body frame growing below it.

// DrawMastery.

// DrawMessageBox: the text without a position starts this far inside the box.

// The resistance panel: the unit name over one bar per element.

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

void BattleMessages::Attach(BattleServices& battle)
{
    this->services = &battle;
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
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawPanelTextAdvance(x, y, text, scene);
    scene.Show(*this->services);
}

void BattleMessages::DrawPanelText(int x, int y, std::u8string_view text)
{
    this->DrawPanelText(x, y, Big5(text));
}

void BattleMessages::DrawMessageText(int x, int y, std::string_view text)
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawMessageTextAdvance(x, y, text, scene);
    scene.Show(*this->services);
}

std::string_view BattleMessage::Text() const
{
    const std::string_view all(this->source.data(), this->source.size());
    return all.substr(0, all.find('\0'));
}

// swd32.exe 0x00469080: the "戰利品" panel with one "%-12s X %2d" line per item
void BattleMessages::DrawLootPanel()
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawLootPanelAdvance(scene);
    scene.Show(*this->services);
}

// swd32.exe 0x00469220: the "戰鬥失敗" panel with "隊伍全滅!!"
void BattleMessages::DrawDefeatPanel()
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawDefeatPanelAdvance(scene);
    scene.Show(*this->services);
}

// swd32.exe 0x00469340: "煉符成功" with "得到符咒:%s" of the first loot name, or "煉符失敗" with "沒有得到東西"
void BattleMessages::DrawCraftResultPanel()
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawCraftResultPanelAdvance(scene);
    scene.Show(*this->services);
}

// swd32.exe 0x00469550: a shaded framed box; the text goes to (textX, textY) unless both are 0
void BattleMessages::DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::string_view text)
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawMessageBoxAdvance(x, y, width, height, textX, textY, text, scene);
    scene.Show(*this->services);
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
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawResistancePanelAdvance(scene);
    scene.Show(*this->services);
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

// swd32.exe 0x00468ED0: while a proficiency has just filled (learnedPending), the box "法寶%s已完全成長!!" with the
// accessory's name: a shade and a frame as wide as the text (17 pixels per two bytes), grown open (GrowPanel), then
// the text at a 17-pixel advance.
void BattleMessages::DrawMastery()
{
    smooth::Scene scene;
    smooth::MessagesFrame(*this).DrawMasteryAdvance(scene);
    scene.Show(*this->services);
}
}  // namespace swd3
