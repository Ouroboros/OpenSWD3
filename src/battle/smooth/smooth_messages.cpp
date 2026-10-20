#include "battle/smooth/smooth_messages.h"

#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_message_constants.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/smooth/smooth_hud.h"
#include "core/format.h"
#include "crt/msvc.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "platform/display.h"
#include "res/act_archive.h"
#include "text/big5.h"
#include "ui/ui_draw.h"

namespace swd3::smooth
{

namespace
{

constexpr u32 kBlack = 0xFF;

// The message bar at the top of the battle screen.
constexpr int kBarLeft = 0xA;
constexpr int kBarTop = 0xA;
constexpr int kBarWidth = 0x26C;
constexpr int kMessageBarHeight = 0x20;
constexpr int kBarTintRed = -8;
constexpr int kBarTintGreen = -8;
constexpr int kBarTintBlue = 0xA;
constexpr int kBarFrameLeft = kBarLeft + kShadeMargin;
constexpr int kBarFrameTop = kBarTop + kShadeMargin;
constexpr int kBarFrameRight = 0x272;
constexpr int kBarFrameBottom = 0x26;
constexpr int kMessageTextRight = 0x10;  // a message is drawn this far right of and kMessageTextDown below its x, y
constexpr int kMessageTextDown = 6;
constexpr int kLeftTextX = 0x12;
constexpr int kCenterX = 0x13B;
constexpr int kCenteredGlyph = 0x14;  // the glyph width used to centre a message

// Sliding and rising messages stop at kSlideStopX (plus half their width) on a black box kSlideBoxWidth wide.
constexpr int kSlideTextY = 0x10;
constexpr int kSlideStopX = 0xE3;
constexpr int kSlideBoxWidth = 0xB0;
constexpr int kSlideOutStep = 0x50;  // pixels an expired message slides out a frame
constexpr int kSlideGlyph = 0x16;
constexpr int kSlideGlyphs = 8;
constexpr int kRiseTarget = 0x20;     // it drops halfway towards kRiseTarget a frame
constexpr i32 kRiseAcceleration = 2;  // an expired rising message doubles its speed every frame

int MessageHalfWidth(const BattleMessage& message)
{
    return (kSlideGlyph * (kSlideGlyphs - (message.length >> 1))) >> 1;
}

constexpr i8 kAbsorbBelow = -10;  // a resistance below -10 tenths absorbs: green bar of the excess
constexpr int kBarHeight = 3;
constexpr double kBarScale = 120.0;  // pixels of a full resistance
constexpr int kBarX = 0x11C;
constexpr int kBoxTextX = 2;
constexpr int kBoxTextY = 4;
constexpr int kBrightLevel = 0x1C;
constexpr int kCraftBodyBottom = 0xFC;
constexpr int kCraftLineX = 0xD0;
constexpr int kCraftLineY = 0xDE;
constexpr int kCraftTitleX = 0x100;
constexpr int kDarkLevel = 2;
constexpr int kDefeatBodyBottom = 0xF4;
constexpr int kDefeatTitleX = 0x104;
constexpr int kElementNameRise = 8;  // the element name sits this far above its bar
constexpr int kElementNameX = 0xFC;
constexpr int kElements = 9;
constexpr int kGrayLevel = 0x10;
constexpr int kGrowDivisor = 3;
constexpr int kLootAdvance = 0x12;
constexpr int kLootRowHeight = 0x14;
constexpr int kLootTitleX = 0x108;
constexpr int kLootX = 0xD2;
constexpr int kMasteryCharWidth = 0x11;  // the advance of its text, and the width of two bytes
constexpr int kMasteryShadeExtra = 0xC;
constexpr int kMasteryShadeMargin = 8;
constexpr int kMasteryShadeY = 0xD0;
constexpr int kMasteryTextExtra = 0x20;
constexpr int kMasteryTextX = 0xD8;
constexpr int kMasteryTextY = 0xDA;
constexpr Pixel kMessageTextColor = 0xFFFF;
constexpr int kMiddleLevel = 0xD;
constexpr float kNeutralBarLength = 2.0F;
constexpr int kPanelAdvance = 0x10;  // the usual glyph advance of the panel font
constexpr int kPanelShadeWidth = 0xB8;
constexpr Pixel kPanelTextColor = 0xFFC0;
constexpr int kResistBodyBottom = 0x12E;
constexpr int kResistBodyTop = 0x7A;
constexpr int kResistFirstBar = 0x82;
constexpr int kResistShadeX = 0xEC;
constexpr int kResistLeft = kResistShadeX + kShadeMargin;
constexpr int kResistNameX = 0x130;
constexpr int kResistRight = 0x1A0;
constexpr int kResistRowHeight = 0x14;
constexpr int kResistTitleBottom = 0x6A;
constexpr int kResistTitleTop = kResistShadeY + kShadeMargin;
constexpr int kResultBodyTop = 0xD4;
constexpr int kResultShadeX = 0xC4;
constexpr int kResultLeft = kResultShadeX + kShadeMargin;
constexpr int kResultRight = 0x178;
constexpr int kResultTitleBottom = 0xC4;
constexpr int kResultTitleTop = kResultShadeY + kShadeMargin;
constexpr double kTenth = 0.1;  // resistances are in tenths
constexpr int kTextStyle = TextFont::BoldOutline;
constexpr int kWhiteLevel = 0x1F;
constexpr int kWipedAdvance = 0x11;
constexpr int kWipedX = 0xFE;
constexpr int kWipedY = 0xD8;
}  // namespace

MessagesFrame::MessagesFrame(BattleMessages& messages)
    : messages(messages)
{
}

// What BattleMessages::Draw decides of the frame: the panel act stepped, the slides moved, the frames counted down,
// the messages gone off the screen freed; the draws in their order into the view.
void MessagesFrame::Advance(MessagesView& view)
{
    BattleMessages& owner = this->messages;

    if (owner.messages == nullptr)
    {
        return;
    }

    auto step = [&view](MessageStep::Kind kind, int x, int y)
    {
        view.steps.push_back(MessageStep{.kind = kind, .x = x, .y = y});
        return &view.steps.back();
    };

    const bool paused = owner.state.paused != 0;

    for (BattleMessage* message = owner.messages.get(); message != nullptr; message = message->next.get())
    {
        int y = message->y + kMessageTextDown;
        int x = message->x + kMessageTextRight;

        if (message->flags < 0)
        {
            owner.panelAct.id = BattleMessages::PanelAct;
            owner.panelAct.action = 0;
            owner.acts.Run(owner.panelAct);
            step(MessageStep::Kind::Bar, 0, 0)->frame = owner.panelAct.frame;
        }

        if ((message->flags & BattleMessage::BlackBar) != 0)
        {
            step(MessageStep::Kind::BlackBar, 0, 0);
        }

        const int flags = message->flags;

        if ((flags & BattleMessage::Left) != 0)
        {
            x = kLeftTextX;
        }

        if ((flags & BattleMessage::Centered) != 0)
        {
            x = kCenterX - ((kCenteredGlyph * (message->length >> 1)) >> 1);
        }

        if (message->frames <= 0)
        {
            continue;
        }

        if ((flags & BattleMessage::FromRight) != 0)
        {
            y = kSlideTextY;
            const int half = MessageHalfWidth(*message);

            if (message->slideX == 0)
            {
                message->slideX = Display::Width;
            }

            if (!paused)
            {
                message->slideX = (half + kSlideStopX + message->slideX) >> 1;
            }

            x = message->slideX;
            step(MessageStep::Kind::Box, x - half, kBarTop);
        }

        if ((message->flags & BattleMessage::FromLeft) != 0)
        {
            y = kSlideTextY;
            const int half = MessageHalfWidth(*message);

            if (!paused)
            {
                message->slideX = (message->slideX + half + kSlideStopX) >> 1;
            }

            x = message->slideX;
            step(MessageStep::Kind::Box, x - half, kBarTop);
        }

        if ((message->flags & BattleMessage::Rise) != 0)
        {
            x = MessageHalfWidth(*message) + kSlideStopX;
            y = (message->slideY + kRiseTarget) / 2;
            message->slideY = y;
            message->slideX = x;
            step(MessageStep::Kind::Box, kSlideStopX, y - kMessageTextDown);
        }

        step(MessageStep::Kind::Text, x, y)->text = message->Text();
        --message->frames;
    }

    std::unique_ptr<BattleMessage>* link = &owner.messages;

    while (*link != nullptr)
    {
        BattleMessage& message = **link;
        bool keep = message.frames > 0;

        if (!keep && (message.flags & BattleMessage::FromLeft) != 0)
        {
            const int half = MessageHalfWidth(message);

            if (!paused)
            {
                message.slideX += kSlideOutStep;
            }

            step(MessageStep::Kind::Box, message.slideX - half, kBarTop);
            step(MessageStep::Kind::Text, message.slideX, kSlideTextY)->text = message.Text();
            keep = message.slideX < Display::Width;
        }

        if (!keep && (message.flags & BattleMessage::FromRight) != 0)
        {
            const int half = MessageHalfWidth(message);

            if (!paused)
            {
                message.slideX -= kSlideOutStep;
            }

            step(MessageStep::Kind::Box, message.slideX - half, kBarTop);
            step(MessageStep::Kind::Text, message.slideX, kSlideTextY)->text = message.Text();
            keep = message.slideX > 0;
        }

        if (!keep && (message.flags & BattleMessage::Rise) != 0)
        {
            const int half = MessageHalfWidth(message);

            if (!paused)
            {
                message.slideY -= message.y;
            }

            message.y *= kRiseAcceleration;
            step(MessageStep::Kind::Box, message.slideX - half, message.slideY - kMessageTextDown);
            step(MessageStep::Kind::Text, message.slideX, message.slideY)->text = message.Text();
            keep = message.slideY > kRiseStart;
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

// What BattleMessages::Draw draws, as Advance decided it.
void MessagesFrame::Draw(const MessagesView& view)
{
    BattleMessages& owner = this->messages;

    for (const MessageStep& step : view.steps)
    {
        switch (step.kind)
        {
            case MessageStep::Kind::Bar:
                FillBox(
                    owner.painter,
                    kBarLeft,
                    kBarTop,
                    kBarWidth,
                    kMessageBarHeight,
                    kBarTintRed,
                    kBarTintGreen,
                    kBarTintBlue,
                    FillMode::Tint
                );
                DrawWindowFrame(
                    owner.painter,
                    owner.archive,
                    step.frame,
                    kBarFrameLeft,
                    kBarFrameTop,
                    kBarFrameRight,
                    kBarFrameBottom,
                    0,
                    kFrameBorder
                );
                break;

            case MessageStep::Kind::BlackBar:
                owner.hud.FillColor(kBarLeft, kBarTop, kBarWidth, kMessageBarHeight, kBlack);
                break;

            case MessageStep::Kind::Box:
                owner.hud.FillColor(step.x, step.y, kSlideBoxWidth, kMessageBarHeight, kBlack);
                break;

            case MessageStep::Kind::Text:
                owner.DrawMessageText(step.x, step.y, step.text);
                break;
        }
    }
}

void MessagesFrame::Frame(Scene& scene)
{
    MessagesView view;
    this->Advance(view);
    scene.Add(
        [view = std::move(view)](BattleServices& drawing)
        {
            MessagesFrame(drawing.messages).Draw(view);
        }
    );
}

// What BattleMessages::DrawPanelText decides of the frame; the draws into `scene`.
void MessagesFrame::DrawPanelTextAdvance(int x, int y, std::string_view text, Scene& scene)
{
    BattleMessages& messages = this->messages;
    SceneFont(messages.panelFont, scene).DrawText(messages.painter.Target(), x, y, text, kPanelTextColor, kTextStyle);
}

// What BattleMessages::DrawMessageText decides of the frame; the draws into `scene`.
void MessagesFrame::DrawMessageTextAdvance(int x, int y, std::string_view text, Scene& scene)
{
    BattleMessages& messages = this->messages;
    SceneFont(messages.messageFont, scene)
        .DrawText(messages.painter.Target(), x, y, text, kMessageTextColor, kTextStyle);
}

// What BattleMessages::DrawMessageBox decides of the frame; the draws into `scene`.
void MessagesFrame::DrawMessageBoxAdvance(
    int x,
    int y,
    int width,
    int height,
    int textX,
    int textY,
    std::string_view text,
    Scene& scene
)
{
    BattleMessages& messages = this->messages;
    messages.panelAct.id = BattleMessages::PanelAct;
    messages.panelAct.action = 0;
    messages.acts.Run(messages.panelAct);
    Box(messages.painter, x, y, width, height, kShadeRed, kShadeGreen, kShadeBlue, FillMode::Blend, scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        x + kShadeMargin,
        y + kShadeMargin,
        x + width - kShadeMargin,
        y + height - kShadeMargin,
        0,
        kFrameBorder,
        scene
    );

    if (textX != 0 || textY != 0)
    {
        this->DrawPanelTextAdvance(textX, textY, text, scene);
    }
    else
    {
        this->DrawPanelTextAdvance(x + kBoxTextX, y + kBoxTextY, text, scene);
    }
}

// What BattleMessages::DrawLootPanel decides of the frame; the draws into `scene`.
void MessagesFrame::DrawLootPanelAdvance(Scene& scene)
{
    BattleMessages& messages = this->messages;
    const int bottom = kLootRowHeight * messages.lootCount + kResultBodyTop;
    messages.panelFont.SetAdvance(kLootAdvance);
    const int frame = messages.PanelFrame();
    Box(messages.painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        messages.panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend,
        scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder,
        scene
    );
    this->DrawPanelTextAdvance(kLootTitleX, kResultTitleTop, BattleMessages::LootTitle, scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        messages.panelHeight + kResultBodyTop,
        0,
        kFrameBorder,
        scene
    );

    if (messages.GrowPanel(kResultBodyTop, bottom, kGrowDivisor) == 1 && messages.lootCount != 0)
    {
        int y = kResultBodyTop;

        for (int item = 0; item < messages.lootCount; ++item)
        {
            const std::string line = WinFormat(
                BattleMessages::LootLine,
                {messages.lootNames[static_cast<std::size_t>(item)],
                 messages.lootCounts[static_cast<std::size_t>(item)]}
            );
            this->DrawPanelTextAdvance(kLootX, y, line, scene);
            y += kLootRowHeight;
        }
    }

    messages.panelFont.SetAdvance(kPanelAdvance);
}

// What BattleMessages::DrawDefeatPanel decides of the frame; the draws into `scene`.
void MessagesFrame::DrawDefeatPanelAdvance(Scene& scene)
{
    BattleMessages& messages = this->messages;
    const int frame = messages.PanelFrame();
    Box(messages.painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        messages.panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend,
        scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder,
        scene
    );
    this->DrawPanelTextAdvance(kDefeatTitleX, kResultTitleTop, BattleMessages::DefeatTitle, scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        messages.panelHeight + kResultBodyTop,
        0,
        kFrameBorder,
        scene
    );

    if (messages.GrowPanel(kResultBodyTop, kDefeatBodyBottom, kGrowDivisor) == 1)
    {
        messages.panelFont.SetAdvance(kWipedAdvance);
        this->DrawPanelTextAdvance(kWipedX, kWipedY, BattleMessages::PartyWiped, scene);
        messages.panelFont.SetAdvance(kPanelAdvance);
    }
}

// What BattleMessages::DrawCraftResultPanel decides of the frame; the draws into `scene`.
void MessagesFrame::DrawCraftResultPanelAdvance(Scene& scene)
{
    BattleMessages& messages = this->messages;
    const int frame = messages.PanelFrame();
    Box(messages.painter,
        kResultShadeX,
        kResultShadeY,
        kPanelShadeWidth,
        messages.panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend,
        scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        frame,
        kResultLeft,
        kResultTitleTop,
        kResultRight,
        kResultTitleBottom,
        0,
        kFrameBorder,
        scene
    );
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        kResultRight,
        messages.panelHeight + kResultBodyTop,
        0,
        kFrameBorder,
        scene
    );

    if (messages.GrowPanel(kResultBodyTop, kCraftBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    if (messages.craftSucceeded == 1)
    {
        this->DrawPanelTextAdvance(kCraftTitleX, kResultTitleTop, BattleMessages::CraftSuccess, scene);
        const std::string line = WinFormat(BattleMessages::CraftGot, {messages.lootNames[0]});
        this->DrawPanelTextAdvance(kCraftLineX, kCraftLineY, line, scene);
    }
    else
    {
        this->DrawPanelTextAdvance(kCraftTitleX, kResultTitleTop, BattleMessages::CraftFailed, scene);
        this->DrawPanelTextAdvance(kCraftLineX, kCraftLineY, BattleMessages::NothingGained, scene);
    }
}

// What BattleMessages::DrawResistancePanel decides of the frame; the draws into `scene`.
void MessagesFrame::DrawResistancePanelAdvance(Scene& scene)
{
    BattleMessages& messages = this->messages;
    u32 color = 0;
    float length = 0.0F;

    if ((messages.state.panelFlags & BattleState::ResistancePanel) == 0)
    {
        return;
    }

    const int frame = messages.PanelFrame();
    Box(messages.painter,
        kResistShadeX,
        kResistShadeY,
        kPanelShadeWidth,
        messages.panelHeight + kShadeExtra,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend,
        scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        frame,
        kResistLeft,
        kResistTitleTop,
        kResistRight,
        kResistTitleBottom,
        0,
        kFrameBorder,
        scene
    );
    this->DrawPanelTextAdvance(kResistNameX, kResistTitleTop, messages.panelUnit->name.data(), scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        kResistLeft,
        kResistBodyTop,
        kResistRight,
        messages.panelHeight + kResistBodyTop,
        0,
        kFrameBorder,
        scene
    );

    if (messages.GrowPanel(kResistBodyTop, kResistBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    int y = kResistFirstBar;

    for (int element = 0; element < kElements; ++element)
    {
        const auto resistance = static_cast<i8>(messages.panelUnit->resistances[static_cast<std::size_t>(element)]);

        if (resistance == 0)
        {
            color = messages.painter.MakeColor(kGrayLevel, kGrayLevel, kGrayLevel);
            length = kNeutralBarLength;
        }

        if (resistance > 0)
        {
            color = messages.painter.MakeColor(kBrightLevel, kDarkLevel, kDarkLevel);
            length = static_cast<float>(static_cast<double>(resistance) * kTenth * kBarScale);
        }

        if (resistance < 0)
        {
            color = messages.painter.MakeColor(kDarkLevel, kMiddleLevel, kBrightLevel);
            length = static_cast<float>(static_cast<double>(resistance) * -kTenth * kBarScale);
        }

        if (resistance < kAbsorbBelow)
        {
            color = messages.painter.MakeColor(kDarkLevel, kBrightLevel, kMiddleLevel);
            length = static_cast<float>(static_cast<double>(resistance - kAbsorbBelow) * -kTenth * kBarScale);
        }

        const u32 white = messages.painter.MakeColor(kWhiteLevel, kWhiteLevel, kWhiteLevel);
        SceneFont(messages.panelFont, scene)
            .DrawText(
                messages.painter.Target(),
                kElementNameX,
                y - kElementNameRise,
                BattleMessages::ElementNames[element],
                static_cast<Pixel>(white),
                kTextStyle
            );
        HudFrame(messages.hud).FillColor(kBarX, y, static_cast<i32>(crt::Ftol(length)), kBarHeight, color, scene);
        y += kResistRowHeight;
    }
}

// What BattleMessages::DrawMastery decides of the frame; the draws into `scene`.
void MessagesFrame::DrawMasteryAdvance(Scene& scene)
{
    BattleMessages& messages = this->messages;

    if (messages.state.learnedPending != 1)
    {
        return;
    }

    const std::string text = WinFormat(BattleMessages::MasteryLine, {messages.state.lastLearned.name});
    const int halves = static_cast<int>(text.size()) / 2;
    const int width = kMasteryCharWidth * halves + kMasteryTextExtra;
    Box(messages.painter,
        kResultShadeX,
        kMasteryShadeY,
        width + kMasteryShadeExtra,
        messages.panelHeight + kMasteryShadeMargin,
        kShadeRed,
        kShadeGreen,
        kShadeBlue,
        FillMode::Blend,
        scene);
    WindowFrame(
        messages.painter,
        messages.archive,
        messages.panelAct.frame,
        kResultLeft,
        kResultBodyTop,
        width + kResultLeft,
        messages.panelHeight + kResultBodyTop,
        0,
        kFrameBorder,
        scene
    );

    if (messages.GrowPanel(kResultBodyTop, kDefeatBodyBottom, kGrowDivisor) != 1)
    {
        return;
    }

    messages.panelFont.SetAdvance(kMasteryCharWidth);
    this->DrawPanelTextAdvance(kMasteryTextX, kMasteryTextY, text, scene);
    messages.panelFont.SetAdvance(kPanelAdvance);
}

void MessagesFrame::DrawPanelTextAdvance(int x, int y, std::u8string_view text, Scene& scene)
{
    this->DrawPanelTextAdvance(x, y, Big5(text), scene);
}

void MessagesFrame::DrawMessageBoxAdvance(
    int x,
    int y,
    int width,
    int height,
    int textX,
    int textY,
    std::u8string_view text,
    Scene& scene
)
{
    this->DrawMessageBoxAdvance(x, y, width, height, textX, textY, Big5(text), scene);
}

}  // namespace swd3::smooth
