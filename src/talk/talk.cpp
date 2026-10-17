#include "talk/talk.h"

#include <climits>
#include <span>
#include <string>

#include "core/debug_log.h"
#include "game/direction.h"
#include "game/game.h"
#include "talk/script_text.h"
#include "talk/talk_event.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr u16 kChoiceSound = 0xC0;
constexpr int kDecimalBase = 0xA;
constexpr int kMaxDigits = 9;  // ParseNumber takes at most this many digits

// The talk windows open in kOpenSteps frames, or fade in (kOpenStep a frame) and out (kFadeStep a frame).
constexpr int kOpenSteps = 4;
constexpr int kOpenStep = 4;
constexpr int kFadeStep = 2;
constexpr int kFadeFrames = 8;
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

// The name plate over a window: its frame, the shade under it and its text.
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

constexpr u32 kNoIndex = 0xFFFFFFFF;  // no role under the pointer
constexpr i32 kIdleAction = 0x34;     // the controlled role stands idle (like action 0)

}  // namespace

Talk::Talk(Game& game)
    : game(game)
{
}

// swd32.exe 0x00430350: text in the large font with the colour and the shadow colour of the tables
void Talk::DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style)
{
    TextFont& font = this->game.largeFont;
    const TextColors& colors = this->game.textColors;
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(static_cast<Pixel>(colors.secondary[static_cast<std::size_t>(shade) & (TextColors::Colors - 1)]));
    font.DrawText(
        pixels,
        x,
        y,
        text,
        static_cast<Pixel>(colors.primary[static_cast<std::size_t>(color) & (TextColors::Colors - 1)]),
        style
    );
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(0);
}

// swd32.exe 0x00430BE0: an optionally signed decimal of at most 9 digits; false for anything else. The original reads
// backwards from the text without bound for an empty number; here that gives 0.
bool Talk::ParseNumber(std::string_view text, int& value)
{
    std::size_t at = 0;
    int sign = 1;

    if (!text.empty() && (text[0] == '+' || text[0] == '-'))
    {
        sign = text[0] == '-' ? -1 : 1;
        at = 1;
    }

    int digits = 0;

    for (std::size_t index = at; index < text.size() && text[index] != '\0'; ++index)
    {
        if (static_cast<u8>(text[index] - '0') >= kDecimalBase)
        {
            return false;
        }

        ++digits;
    }

    if (digits > kMaxDigits)
    {
        return false;
    }

    value = 0;
    int scale = sign;

    for (int index = digits - 1; index >= 0; --index)
    {
        value += scale * (text[at + static_cast<std::size_t>(index)] - '0');
        scale *= kDecimalBase;
    }

    return true;
}

// swd32.exe 0x0042ED40: every talk window drawn and advanced through a 40 x 11 cell surface; closed windows are freed
// (their speakers released), then the end and next page marks step
void Talk::Draw()
{
    Game& owner = this->game;

    if (owner.scene.talkWindows == nullptr)
    {
        return;
    }

    std::unique_ptr<Surface> surface =
        owner.graphics.CreateOffscreen(Talk::SurfaceColumns * this->unit, Talk::SurfaceRows * this->unit);
    owner.graphics.ClearSurface(*surface);
    owner.largeFont.SetTarget(Talk::SurfaceColumns * this->unit, Talk::SurfaceRows * this->unit);
    u16* pixels = owner.painter.LockSurface(*surface);
    owner.painter.UnlockSurface(*surface);
    const i32 heldAdvance = this->advance;

    for (TalkWindow* window = owner.scene.talkWindows.get(); window != nullptr; window = window->next.get())
    {
        this->DrawWindow(*window, *surface, pixels, heldAdvance);
    }

    surface.reset();
    owner.painter.SetClipClamped(0, 0, Display::Width - 1, Display::Height - 1);
    this->advance = 0;
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

    if (owner.acts.Run(this->EndAct()) == 0)
    {
        LogActError(u8"Act Err(DlgMessage:EndBtn)", endButton);
    }

    // The original reports the end button's role here too (0x0043012B..0x0043014B push the same globals).
    if (owner.acts.Run(this->NextAct()) == 0)
    {
        LogActError(u8"Act Err(DlgMessage:NextPage)", endButton);
    }

    i32 pitch = 0;
    i32 height = 0;
    owner.graphics.BackSize(pitch, height);
    owner.largeFont.SetTarget(pitch / static_cast<i32>(sizeof(u16)), height);
}

void Talk::DrawWindow(TalkWindow& window, Surface& surface, u16* pixels, i32 heldAdvance)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    std::string& text = window.text;
    const int unit = this->unit;
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
        DrawWindowFrame(painter, owner.archive, frame, left, top, right, bottom, alpha, kFrameBorder);
    }

    painter.SetClipClamped(edge, top, edge + right, top + bottom);
    --window.delay;
    this->advance = heldAdvance;

    if (this->skipAll != 0)
    {
        window.delay = kSkipDelay;
        this->advance = 1;
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
        if ((window.flags & TalkWindow::NoAdvance) != 0 && this->skipAll == 0)
        {
            this->advance = 0;
        }

        bool timedOut = false;

        if ((window.flags & TalkWindow::Typed) == 0)
        {
            if (this->advance != 0)
            {
                window.flags |= TalkWindow::Advanced;
            }

            if (space.heldFrames != 1)
            {
                this->advance = 0;
                window.flags &= ~TalkWindow::Advanced;
            }

            if (this->advance != 0)
            {
                window.flags |= TalkWindow::Skipping;
            }

            if (window.timeout == TalkWindow::NoTimeout)
            {
                if ((window.flags & TalkWindow::NoAdvance) != 0)
                {
                    this->advance = heldAdvance;
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

            if (this->advance != 0)
            {
                window.flags |= TalkWindow::Advanced;
            }

            if (space.heldFrames != 1)
            {
                this->advance = 0;
                window.flags &= ~TalkWindow::Advanced;
            }

            timedOut = window.timeout != TalkWindow::NoTimeout &&
                       (now - window.shownAt) / kTenthMilliseconds > window.timeout &&
                       (window.flags & TalkWindow::Timed) != 0;
        }

        if (timedOut)
        {
            window.flags |= TalkWindow::Advanced;
            this->advance = 1;
        }

        if ((window.flags & TalkWindow::Fade) != 0)
        {
            this->advance = 0;
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
                    this->advance = 1;
                }
            }
        }
    }
    else
    {
        window.counter = kTypingCounter;

        if ((window.flags & TalkWindow::Skippable) != 0)
        {
            this->advance = 0;
        }
        else if (this->advance != 0)
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
            this->advance = 1;
            window.flags |= TalkWindow::Advanced;
        }
    }

    std::string shown;

    auto flush = [&]()
    {
        this->DrawColoredText(pixels, x * unit, Talk::LineCells * line * unit, color, shade, shown, style);
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
        if (this->skipAll == 1)
        {
            this->advance = 1;
            typedChar = false;
        }

        const std::size_t codeAt = cursor;
        int count = 0;

        if (this->advance != 0)
        {
            if ((window.flags & TalkWindow::Skippable) == 0)
            {
                window.delay = kSkipDelay;
            }

            if (regions.regions.empty())
            {
                if ((window.flags & TalkWindow::PageFull) != 0 && this->advance != Talk::AdvanceHoldsPage)
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
                            const i32 kept = owner.scene.playerHold & Scene::HoldLocked;
                            const i32 locks = std::max((owner.scene.playerHold & ~Scene::HoldLocked) - 1, 0);
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

                if (this->skipAll != 0 || (window.flags & TalkWindow::Instant) != 0)
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

                if (this->skipAll != 0 || (window.flags & TalkWindow::Instant) != 0)
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

                if (this->choice == choiceIndex)
                {
                    owner.largeFont.DrawText(
                        pixels,
                        labelX,
                        Talk::LineCells * line * unit,
                        label,
                        static_cast<Pixel>(owner.textColors.primary[kChosenColor]),
                        TextFont::GradientUp | TextFont::BoldShadow
                    );
                    this->marker.Draw(
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
                    this->DrawColoredText(pixels, labelX, Talk::LineCells * line * unit, color, shade, label, style);
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

    painter.SetSource(pixels);

    // The first pixel is the keyed colour of the surface: a white one is made off-white so white stays visible.
    if (pixels != nullptr && pixels[0] == kWhite)
    {
        pixels[0] = kOffWhite;
    }

    painter.Options().alpha = alpha;
    painter.DrawPicture(
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
        const ActState& mark = (window.flags & TalkWindow::Typed) == 0 ? this->NextAct() : this->EndAct();
        const Picture* picture = owner.archive.GetPicture(mark.frame, mark.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
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
        const int pitch = painter.ScreenPitch();
        const std::span<u16> screen(owner.drawSurface, static_cast<std::size_t>(pitch) * Display::Height);

        for (int row = 0; row < kPlateShadeRows; ++row)
        {
            const int at = edge + pitch * (top + row - kPlateShadeAbove) + kPlateShadeLeft;

            for (int pair = 0; pair < kPlateByteWidth * length + kPlateShadePad; ++pair)
            {
                const int first = at + kPixelsPerPair * pair;

                if (first < 0 || static_cast<std::size_t>(first) + 1 >= screen.size())
                {
                    continue;
                }

                constexpr int kPixelBits = CHAR_BIT * sizeof(u16);
                u32 value = screen[static_cast<std::size_t>(first)] |
                            static_cast<u32>(screen[static_cast<std::size_t>(first) + 1]) << kPixelBits;
                painter.BlendPairsWithColor(std::span<u32>(&value, 1), 0);
                screen[static_cast<std::size_t>(first)] = static_cast<u16>(value);
                screen[static_cast<std::size_t>(first) + 1] = static_cast<u16>(value >> kPixelBits);
            }
        }

        const int frame = window.nameAct != nullptr ? window.nameAct->frame : 0;
        DrawWindowFrame(
            painter,
            owner.archive,
            frame,
            edge + kPlateLeft,
            top - kPlateTop,
            length + edge + kPlateByteWidth * length + kPlateRight,
            top - kPlateBottom,
            alpha,
            kPlateBorder
        );
        owner.mediumFont.SetShadow(static_cast<Pixel>(owner.textColors.secondary[kPlateColor]));
        owner.mediumFont.DrawText(
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

// swd32.exe 0x00427300: the pointer over the map. With no event running and the controlled role standing, a visible
// role with an event under the pointer shows pointer 9 (0xA for flag 0x40, which then blocks map events); a left click
// on it turns both to face each other and starts its event. Otherwise a clickable region of the map under the pointer
// shows pointer 0xB and a click starts its event; elsewhere the pointer shows the direction from the controlled role,
// the right button walks that way and a left click stands for the space key while a talk window is open. With choices
// on screen the pointer picks the choice under it and a click ends them.
void Talk::HandlePointer()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    Controls& controls = owner.controls;
    ButtonState& left = controls.At(Button::MouseLeft);
    i32& cursor = owner.scene.pointerRole.act.action;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const int cameraLeft = field.cameraLeft;
    const int cameraTop = field.cameraTop;
    u32 found = kNoIndex;
    bool regionsAllowed = true;

    if (state.leader.id == Role::NoId && (player.act.action == 0 || player.act.action == kIdleAction))
    {
        for (found = 1; found < static_cast<u32>(state.roleCount); ++found)
        {
            const Role& role = state.roles[found];

            if ((role.flags & Role::Visible) == 0 || role.event == 0 || role.talkState != Role::NoTalk)
            {
                continue;
            }

            const Picture* picture = owner.archive.FindPicture(role.act.frame, role.act.picture);
            const auto x = static_cast<u32>(role.x - role.act.x - cameraLeft);
            const auto y = static_cast<u32>(role.y - role.act.y - cameraTop);

            if (picture != nullptr && static_cast<u32>(state.pointerX) > x &&
                static_cast<u32>(state.pointerX) < x + picture->width && static_cast<u32>(state.pointerY) > y &&
                static_cast<u32>(state.pointerY) < y + picture->height)
            {
                break;
            }
        }

        if (found < static_cast<u32>(state.roleCount) && (state.roles[found].flags & Role::NoTalkPointer) == 0)
        {
            cursor = Scene::TalkPointer;

            if ((state.roles[found].flags & Role::Examine) != 0)
            {
                cursor = Scene::ExaminePointer;
                regionsAllowed = false;
            }
        }
    }

    auto faceAway = [](int direction)
    {
        return (direction & ~1) + ((direction - 1) & 1);  // the opposite direction of its pair
    };

    auto standing = [&player]()
    {
        return player.act.action == kIdleAction || player.act.action == 0;
    };

    if (!state.GameFlag(GameState::NoChoicePointer) && !state.clickRegions.regions.empty() && standing())
    {
        ClickRegion* region = nullptr;
        const int index = state.clickRegions.Find(state.pointerX, state.pointerY, region);
        cursor = Scene::ChoicePointer;

        if (region != nullptr)
        {
            this->choice = index;

            if (left.press != 0 && left.heldFrames == 1)
            {
                state.clickRegions.Free();
                left = ButtonState{};
                DebugLog(
                    u8"\n"
                    u8"Button[%d]Pressed",
                    this->choice
                );
            }
        }

        return;
    }

    if (found == kNoIndex || found == static_cast<u32>(state.roleCount) ||
        found == static_cast<u32>(state.playerRole) || state.leader.id != Role::NoId ||
        state.roles[found].talkState != Role::NoTalk || left.press == 0 || left.heldFrames != 1)
    {
        const std::vector<u32>& blocks = state.map.blocks;
        const auto cellX = static_cast<u32>(cameraLeft + state.pointerX) >> MapHeader::TileShift;
        const auto cellY = static_cast<u32>(cameraTop + state.pointerY) >> MapHeader::TileShift;
        const std::size_t cell = cellX + static_cast<std::size_t>(state.map.width) * cellY;
        const u8 regionId = cell < blocks.size() ? static_cast<u8>(blocks[cell] & MapHeader::RegionMask) : u8{0};
        MapRegion* region = nullptr;

        if (state.leader.id == Role::NoId && regionsAllowed && standing() && regionId != 0 &&
            (region = owner.field.FindRegion(regionId)) != nullptr &&
            state.GameFlag(region->flag & MapRegion::ClickFlagMask))
        {
            cursor = Scene::RegionPointer;

            if (left.press == 0 || left.heldFrames != 1)
            {
                return;
            }

            Role& leader = state.leader;
            leader.unk14 = 0;
            leader.unk20 = 0;
            leader.flags = 0;
            leader.event = static_cast<u16>(region->event & MapRegion::EventMask);
            leader.x = cameraLeft + (state.pointerX & ~MapHeader::TileFraction);
            leader.id = TalkWindow::NoRole;
            leader.y = cameraTop + (state.pointerY & ~MapHeader::TileFraction);
            const int direction = state.DirectionToPlayer(cameraLeft + state.pointerX, cameraTop + state.pointerY);
            player.act.savedDirection = player.act.direction;
            player.act.savedAction = player.act.action;
            player.act.action = 0;
            player.act.direction = faceAway(direction);
            player.act.delay = 0;

            if (owner.acts.Run(player.act) == 0)
            {
                LogActError(u8"Act Err(Talk:MouseActiveStory)", player);
            }

            owner.scene.playerHold = Scene::HoldLocked;
            DebugLog(
                u8"\n"
                u8"MapEvent.Talk[%d]",
                leader.event
            );
            return;
        }

        i32 distance = 0;
        const int angle = AngleBetween(
            state.pointerX,
            state.pointerY,
            player.x + MapHeader::TileSize / 2 * player.act.width - cameraLeft,
            player.y + MapHeader::TileSize / 2 * player.act.height - cameraTop,
            distance
        );
        const int direction = AngleToDirection(angle);

        if (cursor == Scene::ChoicePointer)
        {
            cursor = direction;
        }

        const ButtonState& right = controls.At(Button::MouseRight);

        if (((player.x | player.y) & MapHeader::TileFraction) == 0 && owner.scene.playerHold == 0 && right.press != 0 &&
            distance >= MapHeader::TileSize)
        {
            if (direction == ActState::Up || direction == ActState::UpLeft || direction == ActState::UpRight)
            {
                controls.At(Button::Up).press = right.press;
            }

            if (direction == ActState::Down || direction == ActState::DownRight || direction == ActState::DownLeft)
            {
                controls.At(Button::Down).press = right.press;
            }

            if (direction == ActState::Left || direction == ActState::UpLeft || direction == ActState::DownLeft)
            {
                controls.At(Button::Left).press = right.press;
            }

            if (direction == ActState::Right || direction == ActState::DownRight || direction == ActState::UpRight)
            {
                controls.At(Button::Right).press = right.press;
            }
        }

        ButtonState& space = controls.At(Button::Space);

        if (owner.scene.talkWindows != nullptr && space.press == 0)
        {
            space = left;
        }

        return;
    }

    Role& target = state.roles[found];
    const int direction = state.DirectionToPlayer(
        target.x + MapHeader::TileSize / 2 * target.act.width,
        target.y + MapHeader::TileSize / 2 * target.act.height
    );

    if ((target.flags & Role::FacesTalker) != 0)
    {
        target.act.savedAction = target.act.action;
        target.act.savedDirection = target.act.direction;
        target.act.action = 0;
        target.act.delay = 0;
        target.act.direction = direction;

        if (owner.acts.Run(target.act) == 0)
        {
            LogActError(u8"Act Err(Talk:RoleTurn)", target);
        }
    }

    player.act.action = 0;
    player.act.direction = faceAway(direction);
    player.act.delay = 0;

    if (owner.acts.Run(player.act) == 0)
    {
        LogActError(u8"Act Err(Talk:MRoleTFace)", player);
    }

    Role& leader = state.leader;
    leader.unk14 = target.unk14;
    leader.unk20 = target.unk20;
    leader.event = target.event;
    leader.id = target.id;
    leader.flags = target.flags;
    leader.x = target.x;
    leader.y = target.y;
    left = ButtonState{};
    DebugLog(
        u8"\n"
        u8"RoleEvent.GUID[%d].Talk[%d].Ridx[%d].RArgu[%x]",
        target.id,
        target.event,
        state.FindRole(target.id),
        target.flags
    );
}

// The mark of a finished text: the act of the button pointer role (0x004C9708 inside 0x004C96C8).
ActState& Talk::EndAct()
{
    return this->game.scene.pointerButtonRole.act;
}

// The mark of a full page: the act of the page pointer role (0x004CADE0 inside 0x004CADA0).
ActState& Talk::NextAct()
{
    return this->game.scene.pointerPageRole.act;
}

}  // namespace swd3
