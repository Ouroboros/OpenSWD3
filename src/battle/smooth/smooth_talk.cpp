#include "battle/smooth/smooth_talk.h"

#include "battle/battle_services.h"

#include <climits>
#include <span>
#include <string>

#include "core/debug_log.h"
#include "game/direction.h"
#include "game/game.h"
#include "talk/script_text.h"
#include "talk/talk.h"
#include "talk/talk_constants.h"
#include "talk/talk_event.h"
#include "ui/ui_draw.h"

namespace swd3::smooth
{

namespace
{

constexpr u16 kChoiceSound = 0xC0;
constexpr int kAlphaMask = 0xF;
constexpr int kFrameAbove = 4;  // the frame of an opening window reaches past its text
constexpr int kFrameBelow = 8;
constexpr int kFramePadCells = 2;  // cells the frame adds to the width
constexpr int kFrameBorder = 0x10;
constexpr u16 kSkipDelay = 0x8FFF;        // the delay of a window when every talk is skipped
constexpr u16 kFastest = 0xFFFF;          // the speed and delay of a skipped window
constexpr u32 kTenthMilliseconds = 0x64;  // the timeouts count tenths of a second
constexpr u32 kUntimedHold = 0x10018;     // a fading window without a timeout stays this many tenths
constexpr u16 kShownCounter = 0x10;       // the counter of a shown fading window (it fades over kFadeFrames)
constexpr u16 kTypingCounter = 0x24;
constexpr u16 kChoiceCounter = 0x20;
constexpr i32 kMouseRest = 0xC;          // polls the left button is ignored after a blocking window closes
constexpr std::size_t kChosenColor = 2;  // the colour of the chosen choice
constexpr u16 kWhite = 0xFFFF;
constexpr u16 kOffWhite = 0xFFFE;
constexpr int kMarkRight = 4;  // the page mark sits this far inside the frame's corner
constexpr int kMarkBottom = 8;
constexpr int kPixelsPerPair = 2;
constexpr int kPlateLeft = 6;
constexpr int kPlateTop = 0x1B;
constexpr int kPlateRight = 0x10;
constexpr int kPlateBottom = 8;
constexpr int kPlateBorder = 4;
constexpr int kPlateByteWidth = 8;  // pixels per byte of the name
constexpr int kPlateShadeRows = 0xF;
constexpr int kPlateShadeAbove = 0x10;
constexpr int kPlateShadeLeft = 0x10;
constexpr int kPlateShadePad = 0x1C;
constexpr int kPlateTextX = 0xC;
constexpr int kPlateTextY = 0x1A;
constexpr std::size_t kPlateColor = 4;
}  // namespace

TalkFrame::TalkFrame(Talk& talk)
    : talk(talk)
{
}

// What Talk::DrawWindow decides of the frame; the draws into `scene`.
void TalkFrame::DrawWindowAdvance(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance, Scene& scene)
{
    Talk& talk = this->talk;
    Game& owner = talk.game;
    GameState& state = owner.state;
    Painter& painter = owner.painter;
    ScenePainter onScreen(owner.battleServices, scene);
    const Field& field = owner.field;
    std::string& text = window.text;
    const int unit = talk.unit;
    ButtonState& space = owner.controls.At(Button::Space);

    auto word = [&text](std::size_t at) -> u16
    {
        const u16 low = at < text.size() ? static_cast<u8>(text[at]) : u16{0};
        const u16 high = at + 1 < text.size() ? static_cast<u8>(text[at + 1]) : u16{0};
        return static_cast<u16>(low | high << CHAR_BIT);
    };

    auto byte = [&text](std::size_t at) -> char
    {
        return at < text.size() ? text[at] : '\0';
    };

    auto setWord = [&text](std::size_t at, u16 value)
    {
        if (at + 1 < text.size())
        {
            text[at] = static_cast<char>(value);
            text[at + 1] = static_cast<char>(value >> CHAR_BIT);
        }
    };

    std::size_t cursor = window.page;
    int color = window.color;
    int shade = window.shade;
    int style = window.style & TalkWindow::StyleMask;
    int column = 0;
    std::size_t consumed = 0;
    int x = 1;
    int firstLine = 0;
    int line = 0;
    int choiceIndex = -1;
    bool opening = true;
    int alpha = 0;
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
    int edge = 0;

    if ((window.flags & TalkWindow::Fade) != 0)
    {
        opening = false;

        if ((window.flags & TalkWindow::Typed) != 0)
        {
            if (window.counter < kFadeFrames)
            {
                alpha = kFadeStep * window.counter;
            }
        }
        else
        {
            if (++window.openStep > kOpenSteps)
            {
                window.openStep = kOpenSteps;
            }

            alpha = kOpenStep * window.openStep;
        }

        alpha &= kAlphaMask;
        left = window.x;
        top = window.y;
        right = window.x + window.width + kFramePadCells * unit;
        bottom = window.height + window.y;
    }
    else
    {
        int step = 0;
        int x0 = 0;
        int y0 = 0;
        int offsetY = 0;

        if ((window.flags & TalkWindow::FixedPlace) != 0)
        {
            if (++window.openStep > kOpenSteps)
            {
                window.openStep = kOpenSteps;
                opening = false;
            }

            step = window.openStep;
            x0 = window.fromX + step * ((window.x - window.fromX) / kOpenSteps);
            y0 = window.fromY;
            offsetY = step * ((window.y - window.fromY) / kOpenSteps);
        }
        else if (window.role != TalkWindow::NoRole)
        {
            if (++window.openStep > kOpenSteps)
            {
                window.openStep = kOpenSteps;
                opening = false;
            }

            const Role& speaker = state.roles[std::min<std::size_t>(window.role, GameState::Roles - 1)];
            step = window.openStep;
            x0 = speaker.x + step * (field.cameraLeft + window.x - speaker.x) / kOpenSteps - field.cameraLeft;
            y0 = speaker.y;
            offsetY = step * (field.cameraTop + window.y - speaker.y) / kOpenSteps - field.cameraTop;
        }
        else
        {
            if (++window.openStep > kOpenSteps)
            {
                window.openStep = kOpenSteps;
                opening = false;
            }

            step = window.openStep;
            x0 = window.fromX + step * (field.cameraLeft + window.x - window.fromX) / kOpenSteps - field.cameraLeft;
            y0 = window.fromY;
            offsetY = step * (field.cameraTop + window.y - window.fromY) / kOpenSteps - field.cameraTop;
        }

        left = x0;
        top = y0 + offsetY - kFrameAbove;
        right = x0 + step * ((window.width + kFramePadCells * unit) / kOpenSteps);
        bottom = step * (window.height / kOpenSteps) + top + kFrameBelow;
    }

    if ((window.flags & TalkWindow::Overlay) != 0)
    {
        opening = false;
        left = window.x;
        right = window.x + kFramePadCells * unit + window.width;
        top = window.y;
        bottom = window.y + window.height;
        edge = window.x;
    }
    else
    {
        edge = left;
        const int frame = window.frameAct != nullptr ? window.frameAct->frame : 0;
        WindowFrame(painter, owner.archive, frame, left, top, right, bottom, alpha, kFrameBorder, scene);
    }

    onScreen.SetClipClamped(edge, top, edge + right, top + bottom);
    --window.delay;
    talk.advance = heldAdvance;

    if (talk.skipAll != 0)
    {
        window.delay = kSkipDelay;
        talk.advance = 1;
        space.heldFrames = 1;
    }

    if (owner.controls.At(Button::Enter).heldFrames == 1)
    {
        space.heldFrames = 1;
    }

    if ((window.flags & TalkWindow::Skippable) != 0 && space.heldFrames == 1)
    {
        window.flags |=
            (window.flags & TalkWindow::SkipRequested) != 0 ? TalkWindow::Skipping : TalkWindow::SkipRequested;
    }

    if ((window.flags & TalkWindow::Skipping) != 0)
    {
        window.speed = kFastest;

        if ((window.flags & TalkWindow::SpeedSet) == 0)
        {
            window.delay = kFastest;
            window.flags |= TalkWindow::SpeedSet;
        }
    }

    const u32 now = owner.frameStart;

    if ((window.flags & (TalkWindow::PageFull | TalkWindow::Typed)) != 0)
    {
        if ((window.flags & TalkWindow::NoAdvance) != 0 && talk.skipAll == 0)
        {
            talk.advance = 0;
        }

        bool timedOut = false;

        if ((window.flags & TalkWindow::Typed) == 0)
        {
            if (talk.advance != 0)
            {
                window.flags |= TalkWindow::Advanced;
            }

            if (space.heldFrames != 1)
            {
                talk.advance = 0;
                window.flags &= ~TalkWindow::Advanced;
            }

            if (talk.advance != 0)
            {
                window.flags |= TalkWindow::Skipping;
            }

            if (window.timeout == TalkWindow::NoTimeout)
            {
                if ((window.flags & TalkWindow::NoAdvance) != 0)
                {
                    talk.advance = heldAdvance;
                }
            }
            else if (
                (now - window.shownAt) / kTenthMilliseconds > window.timeout && (window.flags & TalkWindow::Timed) != 0
            )
            {
                window.shownAt = 0;
                timedOut = true;
            }
        }
        else
        {
            if (heldAdvance != 0)
            {
                window.flags |= TalkWindow::Read;
            }

            if (talk.advance != 0)
            {
                window.flags |= TalkWindow::Advanced;
            }

            if (space.heldFrames != 1)
            {
                talk.advance = 0;
                window.flags &= ~TalkWindow::Advanced;
            }

            timedOut = window.timeout != TalkWindow::NoTimeout &&
                       (now - window.shownAt) / kTenthMilliseconds > window.timeout &&
                       (window.flags & TalkWindow::Timed) != 0;
        }

        if (timedOut)
        {
            window.flags |= TalkWindow::Advanced;
            talk.advance = 1;
        }

        if ((window.flags & TalkWindow::Fade) != 0)
        {
            talk.advance = 0;
            const u32 elapsed = (now - window.shownAt) / kTenthMilliseconds;

            if (window.timeout == TalkWindow::NoTimeout)
            {
                if (elapsed < kUntimedHold)
                {
                    window.counter = kShownCounter;
                }
            }
            else if (elapsed > window.timeout)
            {
                if (static_cast<i16>(--window.counter) < 0)
                {
                    window.counter = 0;
                }

                if (window.counter == 0)
                {
                    window.flags |= TalkWindow::Advanced;
                    talk.advance = 1;
                }
            }
        }
    }
    else
    {
        window.counter = kTypingCounter;

        if ((window.flags & TalkWindow::Skippable) != 0)
        {
            talk.advance = 0;
        }
        else if (talk.advance != 0)
        {
            window.flags |= TalkWindow::Skipping;
        }
    }

    ClickRegions& regions = state.clickRegions;

    if ((window.flags & TalkWindow::Choices) != 0)
    {
        window.counter = kChoiceCounter;

        if (regions.regions.empty())
        {
            window.counter = 1;
            talk.advance = 1;
            window.flags |= TalkWindow::Advanced;
        }
    }

    std::string shown;

    auto flush = [&]()
    {
        talk.DrawColoredText(pixels, x * unit, Talk::LineCells * line * unit, color, shade, shown, style);
    };

    auto savePage = [&]()
    {
        window.pageColor = static_cast<u16>(color);
        window.pageShade = static_cast<u16>(shade);
        window.style = static_cast<u16>(window.style | style << TalkWindow::SavedStyleShift);
    };

    bool done = opening;
    bool nextPage = false;
    bool typedChar = false;

    while (!done)
    {
        if (talk.skipAll == 1)
        {
            talk.advance = 1;
            typedChar = false;
        }

        const std::size_t codeAt = cursor;
        int count = 0;

        if (talk.advance != 0)
        {
            if ((window.flags & TalkWindow::Skippable) == 0)
            {
                window.delay = kSkipDelay;
            }

            if (regions.regions.empty())
            {
                if ((window.flags & TalkWindow::PageFull) != 0 && talk.advance != Talk::AdvanceHoldsPage)
                {
                    nextPage = true;
                    break;
                }

                if ((window.flags & TalkWindow::Advanced) != 0)
                {
                    if ((window.flags & TalkWindow::FixedPlace) == 0)
                    {
                        if (window.role == TalkWindow::NoRole)
                        {
                            state.leader.talkState = Role::NoTalk;
                        }
                        else if (window.role < GameState::Roles)
                        {
                            Role& speaker = state.roles[window.role];

                            if ((speaker.flags & Role::FacesTalker) != 0 && owner.acts.Run(speaker.act) == 0)
                            {
                                LogActError(u8"Act Err(DlgMessage:Fr)", speaker);
                            }
                        }

                        if ((window.flags & TalkWindow::Blocking) != 0)
                        {
                            owner.controls.mouseSuppressed = kMouseRest;
                            const i32 kept = owner.scene.playerHold & ::swd3::Scene::HoldLocked;
                            const i32 locks = std::max((owner.scene.playerHold & ~::swd3::Scene::HoldLocked) - 1, 0);
                            owner.scene.playerHold = kept | locks;
                        }
                    }

                    window.page = TalkWindow::Closed;
                    space.press = 0;
                    space.heldFrames = 0;
                    break;
                }
            }
        }

        const std::size_t reached = consumed + window.page;

        if (static_cast<i16>(window.delay) >= 0)
        {
            if (reached == window.typed)
            {
                flush();
                break;
            }
        }
        else
        {
            if (reached > static_cast<std::size_t>(count) + window.typed && typedChar)
            {
                window.delay = window.speed;
                window.typed = reached;
                flush();
                break;
            }

            typedChar = false;
        }

        const u16 code = word(cursor);

        if (code == kTextEnd)
        {
            flush();

            if ((window.flags & TalkWindow::Typed) == 0)
            {
                if (window.shownAt == 0)
                {
                    window.shownAt = now;
                }

                if ((window.flags & TalkWindow::Fade) != 0)
                {
                    window.counter = kShownCounter;
                }
            }

            window.flags |= TalkWindow::Typed;

            if (!regions.regions.empty())
            {
                regions.flags |= ClickRegions::TalkChoices;
            }

            break;
        }

        const int after = x + column;

        if (unit * after > window.width || code == kNewLine)
        {
            flush();

            if (Talk::LineCells * unit * (line + 1) >= window.height)
            {
                window.flags |= TalkWindow::PageFull;

                if (!regions.regions.empty())
                {
                    regions.flags |= ClickRegions::TalkChoices;
                }

                if (word(cursor) == kNewLine)
                {
                    window.typed = consumed + kCodeBytes + window.page;
                    consumed += kCodeBytes;
                }

                if (window.shownAt == 0)
                {
                    window.shownAt = now;
                }

                if (talk.skipAll != 0 || (window.flags & TalkWindow::Instant) != 0)
                {
                    window.typed = consumed + window.page;
                }

                savePage();
                break;
            }

            column = 0;
            x = 1;
            shown.clear();
            ++line;

            if (code == kNewLine)
            {
                cursor += kCodeBytes;
                consumed += kCodeBytes;
            }

            if (firstLine == 0)
            {
                firstLine = static_cast<int>(consumed);
            }

            continue;
        }

        switch (code)
        {
            case kLine:
                if (reached >= window.typed)
                {
                    flush();
                    window.page += static_cast<std::size_t>(firstLine);
                    window.typed += kCodeBytes;
                    done = true;
                    break;
                }

                cursor += kCodeBytes;
                consumed += kCodeBytes;
                break;

            case kPage:
                flush();

                if (!regions.regions.empty())
                {
                    regions.flags |= ClickRegions::TalkChoices;
                }

                window.typed = consumed + kCodeBytes + window.page;
                consumed += kCodeBytes;

                if (window.shownAt == 0)
                {
                    window.shownAt = now;
                }

                if (talk.skipAll != 0 || (window.flags & TalkWindow::Instant) != 0)
                {
                    window.typed = consumed + window.page;
                }

                savePage();
                window.flags |= TalkWindow::PageFull;
                done = true;
                break;

            case kSpeed:
            {
                const auto digit = static_cast<u8>(byte(cursor + kCodeBytes) - '0');
                cursor += kCodeDigitBytes;
                consumed += kCodeDigitBytes;
                window.speed = static_cast<u16>(Talk::SpeedScale * (owner.config.textSpeed + digit));

                if ((window.flags & TalkWindow::Skipping) != 0)
                {
                    window.speed = 0;
                }

                break;
            }

            case kColor:
            case kSwappedShadeCode:
            case kStyle:
            {
                cursor += kCodeDigitBytes;
                consumed += kCodeDigitBytes;
                flush();
                const int digit = byte(cursor - 1) - '0';
                x = after;
                column = 0;
                shown.clear();

                if (code == kColor)
                {
                    color = digit;
                    shade = digit;
                }
                else if (code == kSwappedShadeCode)
                {
                    shade = digit;
                }
                else
                {
                    style = digit == 0 ? TextFont::BoldShadow : digit == 1 ? TextFont::BoldOutline : digit;
                }

                break;
            }

            case kChoice:
            case kChoiceSeen:
            {
                window.flags |= TalkWindow::Choices;
                ++choiceIndex;
                flush();
                std::size_t at = cursor + kCodeBytes;
                std::string label;

                while (byte(at) != '.' && at < text.size())
                {
                    label.push_back(text[at]);
                    ++at;
                }

                count = static_cast<int>(label.size());
                consumed += kCodeBytes + label.size() + 1;
                cursor = at + 1;
                const int labelX = after * unit;

                if (talk.choice == choiceIndex)
                {
                    owner.largeFont.DrawText(
                        pixels,
                        labelX,
                        Talk::LineCells * line * unit,
                        label,
                        static_cast<Pixel>(owner.textColors.primary[kChosenColor]),
                        TextFont::GradientUp | TextFont::BoldShadow
                    );
                    talk.marker.Draw(
                        surface,
                        painter,
                        labelX,
                        Talk::LineCells * line * unit,
                        count * unit,
                        Talk::LineCells * unit
                    );
                }
                else
                {
                    talk.DrawColoredText(pixels, labelX, Talk::LineCells * line * unit, color, shade, label, style);
                }

                owner.largeFont.SetBackground(TextFont::NoBackground);
                x = after + count;
                column = 0;
                shown.clear();

                if (word(codeAt) == kChoice)
                {
                    ClickRegion region;
                    region.unk00 = 1;
                    region.left = static_cast<u16>(window.x + unit * (x - count));
                    region.top = static_cast<u16>(window.y + Talk::LineCells * line * unit);
                    region.right = static_cast<u16>(region.left + count * unit);
                    region.bottom = static_cast<u16>(region.top + Talk::LineCells * unit);
                    regions.regions.push_back(region);
                }

                setWord(codeAt, kChoiceSeen);
                break;
            }

            case kSound:
            case kSoundSeen:
            {
                std::size_t at = cursor + kCodeBytes;
                std::string number;

                while (byte(at) != '.' && at < text.size())
                {
                    number.push_back(text[at]);
                    ++at;
                }

                consumed += number.size() + 1;
                cursor = at + 1;
                Talk::ParseNumber(number, count);

                if (word(codeAt) == kSound)
                {
                    owner.audio.PlaySound(kChoiceSound, owner.config.soundVolume);
                }

                setWord(codeAt, kSoundSeen);
                break;
            }

            case kKey:
            case kKeySeen:
                window.speed = 0;
                window.delay = 0;
                cursor += kCodeBytes;
                window.flags |= TalkWindow::Skipping;
                consumed += kCodeBytes;
                break;

            default:
            {
                const char first = byte(cursor);
                shown.push_back(first);
                ++column;
                ++cursor;
                typedChar = true;
                ++consumed;

                if (static_cast<i8>(first) < 0)
                {
                    shown.push_back(byte(cursor));
                    ++column;
                    ++cursor;
                    ++consumed;
                }

                break;
            }
        }
    }

    if (nextPage)
    {
        window.page = window.typed;
        const char next = byte(window.typed);

        if (static_cast<i8>(next) >= 0 && next != '%')
        {
            ++window.typed;
        }

        window.flags &= ~(TalkWindow::PageFull | TalkWindow::Advanced | TalkWindow::Instant);
        window.color = window.pageColor;
        window.shade = window.pageShade;
        window.style = static_cast<u16>(window.style >> TalkWindow::SavedStyleShift);
    }

    if (!opening)
    {
        edge = left;
    }

    onScreen.SetSource(pixels, Talk::SurfaceColumns * unit, Talk::SurfaceRows * unit);

    // The first pixel is the keyed colour of the surface: a white one is made off-white so white stays visible.
    if (pixels != nullptr && pixels[0] == kWhite)
    {
        pixels[0] = kOffWhite;
    }

    onScreen.Options().alpha = alpha;
    onScreen.DrawPicture(
        window.x,
        window.y,
        Talk::SurfaceColumns * unit,
        Talk::SurfaceRows * unit,
        alpha != 0 ? Painter::RawTranslucent : Painter::RawKeyed
    );
    owner.graphics.ClearSurface(surface);

    if ((window.flags & (TalkWindow::PageFull | TalkWindow::Typed)) != 0 &&
        (((window.flags & TalkWindow::ShowMark) != 0 && (window.flags & TalkWindow::Read) == 0) ||
         (window.flags & TalkWindow::NoAdvance) == 0))
    {
        const ActState& mark = (window.flags & TalkWindow::Typed) == 0 ? talk.NextAct() : talk.EndAct();
        const Picture* picture = owner.archive.GetPicture(mark.frame, mark.picture);

        if (picture != nullptr)
        {
            onScreen.SetSource(picture->data.data());
            onScreen.DrawPicture(
                right - mark.x - kMarkRight,
                bottom - mark.y - kMarkBottom,
                picture->width,
                picture->height,
                Painter::Copy
            );
        }
    }

    if (!opening && !window.name.empty() && window.page != TalkWindow::Closed)
    {
        const auto length = static_cast<int>(window.name.size());
        // 0x00417DE0 is given 8 * length + 0x1C pixels and blends half as many pairs.
        const int pairs = (kPlateByteWidth * length + kPlateShadePad) >> 1;

        scene.Add(
            [edge, top, pairs](BattleServices& drawing)
            {
                const int pitch = drawing.painter.ScreenPitch();
                const std::span<u16> screen(drawing.screen, static_cast<std::size_t>(pitch) * Display::Height);

                for (int row = 0; row < kPlateShadeRows; ++row)
                {
                    const int at = edge + pitch * (top + row - kPlateShadeAbove) + kPlateShadeLeft;

                    for (int pair = 0; pair < pairs; ++pair)
                    {
                        const int first = at + kPixelsPerPair * pair;

                        if (first < 0 || static_cast<std::size_t>(first) + 1 >= screen.size())
                        {
                            continue;
                        }

                        constexpr int kPixelBits = CHAR_BIT * sizeof(u16);
                        u32 value = screen[static_cast<std::size_t>(first)] |
                                    static_cast<u32>(screen[static_cast<std::size_t>(first) + 1]) << kPixelBits;
                        drawing.painter.BlendPairsWithColor(std::span<u32>(&value, 1), 0);
                        screen[static_cast<std::size_t>(first)] = static_cast<u16>(value);
                        screen[static_cast<std::size_t>(first) + 1] = static_cast<u16>(value >> kPixelBits);
                    }
                }
            }
        );

        const int frame = window.nameAct != nullptr ? window.nameAct->frame : 0;
        WindowFrame(
            painter,
            owner.archive,
            frame,
            edge + kPlateLeft,
            top - kPlateTop,
            length + edge + kPlateByteWidth * length + kPlateRight,
            top - kPlateBottom,
            alpha,
            kPlateBorder,
            scene
        );
        owner.mediumFont.SetShadow(static_cast<Pixel>(owner.textColors.secondary[kPlateColor]));
        SceneFont(owner.mediumFont, scene)
            .DrawText(
                owner.drawSurface,
                edge + kPlateTextX,
                top - kPlateTextY,
                window.name,
                static_cast<Pixel>(owner.textColors.primary[kPlateColor]),
                TextFont::BoldShadow
            );
        owner.mediumFont.SetShadow(0);
    }
}

// What Talk::Draw decides of the frame; the draws into `scene`.
void TalkFrame::DrawAdvance(Scene& scene)
{
    Talk& talk = this->talk;
    Game& owner = talk.game;
    const Scene::Still still(scene);

    if (owner.scene.talkWindows == nullptr)
    {
        return;
    }

    std::unique_ptr<Surface> surface =
        owner.graphics.CreateOffscreen(Talk::SurfaceColumns * talk.unit, Talk::SurfaceRows * talk.unit);
    owner.graphics.ClearSurface(*surface);
    owner.largeFont.SetTarget(Talk::SurfaceColumns * talk.unit, Talk::SurfaceRows * talk.unit);
    u16* pixels = owner.painter.LockSurface(*surface);
    owner.painter.UnlockSurface(*surface);
    const i32 heldAdvance = talk.advance;

    for (TalkWindow* window = owner.scene.talkWindows.get(); window != nullptr; window = window->next.get())
    {
        this->DrawWindowAdvance(*window, *surface, pixels, heldAdvance, scene);
    }

    surface.reset();
    ScenePainter(owner.battleServices, scene).SetClipClamped(0, 0, Display::Width - 1, Display::Height - 1);
    talk.advance = 0;
    std::unique_ptr<TalkWindow>* link = &owner.scene.talkWindows;

    while (*link != nullptr)
    {
        TalkWindow& window = **link;

        if (window.page != TalkWindow::Closed)
        {
            link = &window.next;
            continue;
        }

        if (window.role != TalkWindow::NoRole && window.role < GameState::Roles)
        {
            owner.state.roles[window.role].talkState = Role::NoTalk;
        }

        *link = std::move(window.next);
    }

    const Role& endButton = owner.scene.pointerButtonRole;

    if (owner.acts.Run(talk.EndAct()) == 0)
    {
        LogActError(u8"Act Err(DlgMessage:EndBtn)", endButton);
    }

    // The original reports the end button's role here too (0x0043012B..0x0043014B push the same globals).
    if (owner.acts.Run(talk.NextAct()) == 0)
    {
        LogActError(u8"Act Err(DlgMessage:NextPage)", endButton);
    }

    i32 pitch = 0;
    i32 height = 0;
    owner.graphics.BackSize(pitch, height);
    owner.largeFont.SetTarget(pitch / static_cast<i32>(sizeof(u16)), height);
}

}  // namespace swd3::smooth
