#include "battle/smooth/smooth_messages.h"

#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_message_constants.h"
#include "battle/battle_state.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "platform/display.h"
#include "res/act_archive.h"
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

    auto step = [&view](MessageStep::Kind kind, int x, int y, std::array<int, 2> motion)
    {
        view.steps.push_back(MessageStep{.kind = kind, .x = x, .y = y, .motion = motion});
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
            step(MessageStep::Kind::Bar, 0, 0, {})->frame = owner.panelAct.frame;
        }

        if ((message->flags & BattleMessage::BlackBar) != 0)
        {
            step(MessageStep::Kind::BlackBar, 0, 0, {});
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

        // The rebuild's smooth battle: a sliding message moves on by the same halving next frame (on its last frame
        // the slide out takes over).
        const bool slidesNext = !paused && message->frames > 1;
        std::array<int, 2> slide = {0, 0};

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
            slide = {slidesNext ? ((half + kSlideStopX + x) >> 1) - x : 0, 0};
            step(MessageStep::Kind::Box, x - half, kBarTop, slide);
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
            slide = {slidesNext ? ((x + half + kSlideStopX) >> 1) - x : 0, 0};
            step(MessageStep::Kind::Box, x - half, kBarTop, slide);
        }

        if ((message->flags & BattleMessage::Rise) != 0)
        {
            x = MessageHalfWidth(*message) + kSlideStopX;
            y = (message->slideY + kRiseTarget) / 2;
            message->slideY = y;
            message->slideX = x;
            slide = {0, slidesNext ? (y + kRiseTarget) / 2 - y : 0};
            step(MessageStep::Kind::Box, kSlideStopX, y - kMessageTextDown, slide);
        }

        step(MessageStep::Kind::Text, x, y, slide)->text = message->Text();
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

            const std::array<int, 2> moving = {paused ? 0 : kSlideOutStep, 0};
            step(MessageStep::Kind::Box, message.slideX - half, kBarTop, moving);
            step(MessageStep::Kind::Text, message.slideX, kSlideTextY, moving)->text = message.Text();
            keep = message.slideX < Display::Width;
        }

        if (!keep && (message.flags & BattleMessage::FromRight) != 0)
        {
            const int half = MessageHalfWidth(message);

            if (!paused)
            {
                message.slideX -= kSlideOutStep;
            }

            const std::array<int, 2> moving = {paused ? 0 : -kSlideOutStep, 0};
            step(MessageStep::Kind::Box, message.slideX - half, kBarTop, moving);
            step(MessageStep::Kind::Text, message.slideX, kSlideTextY, moving)->text = message.Text();
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
            const std::array<int, 2> moving = {0, paused ? 0 : -message.y};
            step(MessageStep::Kind::Box, message.slideX - half, message.slideY - kMessageTextDown, moving);
            step(MessageStep::Kind::Text, message.slideX, message.slideY, moving)->text = message.Text();
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
        const PaintRecord::Motion moving(step.motion);

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

}  // namespace swd3::smooth
