#include "talk/talk.h"

#include <span>
#include <string>

#include "core/debug_log.h"
#include "game/direction.h"
#include "game/game.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr u16 kEnd = 0x5125;      // "%Q"
constexpr u16 kNewLine = 0x4E25;  // "%N"
constexpr u16 kLine = 0x4C25;     // "%L"
constexpr u16 kPage = 0x5025;     // "%P"
constexpr u16 kSpeed = 0x5325;    // "%S" and a digit
constexpr u16 kColor = 0x4325;    // "%C" and a digit
constexpr u16 kShade = 0x2544;    // "D%" and a digit
constexpr u16 kStyle = 0x4725;    // "%G" and a digit
constexpr u16 kChoice = 0x4225;   // "%B" text "." (made "%b" once its click area is added)
constexpr u16 kChoiceSeen = 0x6225;
constexpr u16 kSound = 0x4125;  // "%A" number "." (made "%a" once its sound played)
constexpr u16 kSoundSeen = 0x6125;
constexpr u16 kKey = 0x4B25;  // "%K"
constexpr u16 kKeySeen = 0x6B25;
constexpr u16 kChoiceSound = 0xC0;

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
    font.SetShadow(static_cast<Pixel>(colors.secondary[static_cast<std::size_t>(shade) & 0xF]));
    font.DrawText(pixels, x, y, text, static_cast<Pixel>(colors.primary[static_cast<std::size_t>(color) & 0xF]), style);
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
        if (static_cast<u8>(text[index] - '0') > 9)
        {
            return false;
        }

        ++digits;
    }

    if (digits > 9)
    {
        return false;
    }

    value = 0;
    int scale = sign;

    for (int index = digits - 1; index >= 0; --index)
    {
        value += scale * (text[at + static_cast<std::size_t>(index)] - '0');
        scale *= 0xA;
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

    std::unique_ptr<Surface> surface = owner.graphics.CreateOffscreen(0x28 * this->unit, 0xB * this->unit);
    owner.graphics.ClearSurface(*surface);
    owner.largeFont.SetTarget(0x28 * this->unit, 0xB * this->unit);
    u16* pixels = owner.painter.LockSurface(*surface);
    owner.painter.UnlockSurface(*surface);
    const i32 heldAdvance = this->advance;

    for (TalkWindow* window = owner.scene.talkWindows.get(); window != nullptr; window = window->next.get())
    {
        this->DrawWindow(*window, *surface, pixels, heldAdvance);
    }

    surface.reset();
    owner.painter.SetClipClamped(0, 0, 0x27F, 0x1DF);
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
            owner.state.roles[window.role].unk26 = 0;
        }

        *link = std::move(window.next);
    }

    owner.acts.Run(this->endAct);
    owner.acts.Run(this->nextAct);
    i32 pitch = 0;
    i32 height = 0;
    owner.graphics.BackSize(pitch, height);
    owner.largeFont.SetTarget(pitch / 2, height);
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
        return static_cast<u16>(low | high << 8);
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
            text[at + 1] = static_cast<char>(value >> 8);
        }
    };

    std::size_t cursor = window.page;
    int color = window.color;
    int shade = window.shade;
    int style = window.style & 0xFF;
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
            if (window.counter < 8)
            {
                alpha = 2 * window.counter;
            }
        }
        else
        {
            if (++window.openStep > 4)
            {
                window.openStep = 4;
            }

            alpha = 4 * window.openStep;
        }

        alpha &= 0xF;
        left = window.x;
        top = window.y;
        right = window.x + window.width + 2 * unit;
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
            if (++window.openStep > 4)
            {
                window.openStep = 4;
                opening = false;
            }

            step = window.openStep;
            x0 = window.fromX + step * ((window.x - window.fromX) / 4);
            y0 = window.fromY;
            offsetY = step * ((window.y - window.fromY) / 4);
        }
        else if (window.role != TalkWindow::NoRole)
        {
            if (++window.openStep > 4)
            {
                window.openStep = 4;
                opening = false;
            }

            const Role& speaker = state.roles[std::min<std::size_t>(window.role, GameState::Roles - 1)];
            step = window.openStep;
            x0 = speaker.x + step * (field.cameraLeft + window.x - speaker.x) / 4 - field.cameraLeft;
            y0 = speaker.y;
            offsetY = step * (field.cameraTop + window.y - speaker.y) / 4 - field.cameraTop;
        }
        else
        {
            if (++window.openStep > 4)
            {
                window.openStep = 4;
                opening = false;
            }

            step = window.openStep;
            x0 = window.fromX + step * (field.cameraLeft + window.x - window.fromX) / 4 - field.cameraLeft;
            y0 = window.fromY;
            offsetY = step * (field.cameraTop + window.y - window.fromY) / 4 - field.cameraTop;
        }

        left = x0;
        top = y0 + offsetY - 4;
        right = x0 + step * ((window.width + 2 * unit) / 4);
        bottom = step * (window.height >> 2) + top + 8;
    }

    if ((window.flags & TalkWindow::Overlay) != 0)
    {
        opening = false;
        left = window.x;
        right = window.x + 2 * unit + window.width;
        top = window.y;
        bottom = window.y + window.height;
        edge = window.x;
    }
    else
    {
        edge = left;
        const int frame = window.frameAct != nullptr ? window.frameAct->frame : 0;
        DrawWindowFrame(painter, owner.archive, frame, left, top, right, bottom, alpha, 0x10);
    }

    painter.SetClipClamped(edge, top, edge + right, top + bottom);
    --window.delay;
    this->advance = heldAdvance;

    if (this->skipAll != 0)
    {
        window.delay = 0x8FFF;
        this->advance = 1;
        space.heldFrames = 1;
    }

    if (owner.controls.At(Button::Enter).heldFrames == 1)
    {
        space.heldFrames = 1;
    }

    if ((window.flags & TalkWindow::Skippable) != 0 && space.heldFrames == 1)
    {
        window.flags |= static_cast<i32>(window.flags) < 0 ? TalkWindow::Skipping : TalkWindow::SkipRequested;
    }

    if ((window.flags & TalkWindow::Skipping) != 0)
    {
        window.speed = 0xFFFF;

        if ((window.flags & TalkWindow::SpeedSet) == 0)
        {
            window.delay = 0xFFFF;
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

            if (window.timeout == 0xFFFF)
            {
                if ((window.flags & TalkWindow::NoAdvance) != 0)
                {
                    this->advance = heldAdvance;
                }
            }
            else if ((now - window.shownAt) / 0x64 > window.timeout && (window.flags & TalkWindow::Timed) != 0)
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

            timedOut = window.timeout != 0xFFFF && (now - window.shownAt) / 0x64 > window.timeout &&
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
            const u32 elapsed = (now - window.shownAt) / 0x64;

            if (window.timeout == 0xFFFF)
            {
                if (elapsed < 0x10018)
                {
                    window.counter = 0x10;
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
        window.counter = 0x24;

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
        window.counter = 0x20;

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
        this->DrawColoredText(pixels, x * unit, 2 * line * unit, color, shade, shown, style);
    };

    auto savePage = [&]()
    {
        window.pageColor = static_cast<u16>(color);
        window.pageShade = static_cast<u16>(shade);
        window.style = static_cast<u16>(window.style | style << 8);
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
                window.delay = 0x8FFF;
            }

            if (regions.regions.empty())
            {
                if ((window.flags & TalkWindow::PageFull) != 0 && this->advance != 2)
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
                            state.leader.unk26 = 0;
                        }
                        else if (window.role < GameState::Roles)
                        {
                            Role& speaker = state.roles[window.role];

                            if ((speaker.flags & Role::FacesTalker) != 0)
                            {
                                owner.acts.Run(speaker.act);
                            }
                        }

                        if ((window.flags & TalkWindow::Blocking) != 0)
                        {
                            owner.controls.mouseSuppressed = 0xC;
                            const i32 kept = owner.scene.unk4A9920 & 0x8000;
                            const i32 locks = std::max((owner.scene.unk4A9920 & ~0x8000) - 1, 0);
                            owner.scene.unk4A9920 = kept | locks;
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

        if (code == kEnd)
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
                    window.counter = 0x10;
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

            if (2 * unit * (line + 1) >= window.height)
            {
                window.flags |= TalkWindow::PageFull;

                if (!regions.regions.empty())
                {
                    regions.flags |= ClickRegions::TalkChoices;
                }

                if (word(cursor) == kNewLine)
                {
                    window.typed = consumed + 2 + window.page;
                    consumed += 2;
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
                cursor += 2;
                consumed += 2;
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
                    window.typed += 2;
                    done = true;
                    break;
                }

                cursor += 2;
                consumed += 2;
                break;

            case kPage:
                flush();

                if (!regions.regions.empty())
                {
                    regions.flags |= ClickRegions::TalkChoices;
                }

                window.typed = consumed + 2 + window.page;
                consumed += 2;

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
                const auto digit = static_cast<u8>(byte(cursor + 2) - '0');
                cursor += 3;
                consumed += 3;
                window.speed = static_cast<u16>(2 * (owner.config.textSpeed + digit));

                if ((window.flags & TalkWindow::Skipping) != 0)
                {
                    window.speed = 0;
                }

                break;
            }

            case kColor:
            case kShade:
            case kStyle:
            {
                cursor += 3;
                consumed += 3;
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
                else if (code == kShade)
                {
                    shade = digit;
                }
                else
                {
                    style = digit == 0 ? 4 : digit == 1 ? 0x10 : digit;
                }

                break;
            }

            case kChoice:
            case kChoiceSeen:
            {
                window.flags |= TalkWindow::Choices;
                ++choiceIndex;
                flush();
                std::size_t at = cursor + 2;
                std::string label;

                while (byte(at) != '.' && at < text.size())
                {
                    label.push_back(text[at]);
                    ++at;
                }

                count = static_cast<int>(label.size());
                consumed += 2 + label.size() + 1;
                cursor = at + 1;
                const int labelX = after * unit;

                if (this->choice == choiceIndex)
                {
                    owner.largeFont.DrawText(
                        pixels,
                        labelX,
                        2 * line * unit,
                        label,
                        static_cast<Pixel>(owner.textColors.primary[2]),
                        0x84
                    );
                    this->marker.Draw(surface, painter, labelX, 2 * line * unit, count * unit, 2 * unit);
                }
                else
                {
                    this->DrawColoredText(pixels, labelX, 2 * line * unit, color, shade, label, style);
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
                    region.top = static_cast<u16>(window.y + 2 * line * unit);
                    region.right = static_cast<u16>(region.left + count * unit);
                    region.bottom = static_cast<u16>(region.top + 2 * unit);
                    regions.regions.push_back(region);
                }

                setWord(codeAt, kChoiceSeen);
                break;
            }

            case kSound:
            case kSoundSeen:
            {
                std::size_t at = cursor + 2;
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
                cursor += 2;
                window.flags |= TalkWindow::Skipping;
                consumed += 2;
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
        window.style = static_cast<u16>(window.style >> 8);
    }

    if (!opening)
    {
        edge = left;
    }

    painter.SetSource(pixels);

    if (pixels != nullptr && pixels[0] == 0xFFFF)
    {
        pixels[0] = 0xFFFE;
    }

    painter.Options().alpha = alpha;
    painter.DrawPicture(window.x, window.y, 0x28 * unit, 0xB * unit, alpha != 0 ? 0x14 : 4);
    owner.graphics.ClearSurface(surface);

    if ((window.flags & (TalkWindow::PageFull | TalkWindow::Typed)) != 0 &&
        (((window.flags & TalkWindow::ShowMark) != 0 && (window.flags & TalkWindow::Read) == 0) ||
         (window.flags & TalkWindow::NoAdvance) == 0))
    {
        const ActState& mark = (window.flags & TalkWindow::Typed) == 0 ? this->nextAct : this->endAct;
        const Picture* picture = owner.archive.GetPicture(mark.frame, mark.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(right - mark.x - 4, bottom - mark.y - 8, picture->width, picture->height, 0);
        }
    }

    if (!opening && !window.name.empty() && window.page != TalkWindow::Closed)
    {
        const auto length = static_cast<int>(window.name.size());
        const int pitch = painter.ScreenPitch();
        const std::span<u16> screen(owner.drawSurface, static_cast<std::size_t>(pitch) * 0x1E0);

        for (int row = 0; row < 0xF; ++row)
        {
            const int at = edge + pitch * (top + row - 0x10) + 0x10;

            for (int pair = 0; pair < 8 * length + 0x1C; ++pair)
            {
                const int first = at + 2 * pair;

                if (first < 0 || static_cast<std::size_t>(first) + 1 >= screen.size())
                {
                    continue;
                }

                u32 value = screen[static_cast<std::size_t>(first)] |
                            static_cast<u32>(screen[static_cast<std::size_t>(first) + 1]) << 16;
                painter.BlendPairsWithColor(std::span<u32>(&value, 1), 0);
                screen[static_cast<std::size_t>(first)] = static_cast<u16>(value);
                screen[static_cast<std::size_t>(first) + 1] = static_cast<u16>(value >> 16);
            }
        }

        const int frame = window.nameAct != nullptr ? window.nameAct->frame : 0;
        DrawWindowFrame(
            painter,
            owner.archive,
            frame,
            edge + 6,
            top - 0x1B,
            length + edge + 8 * length + 0x10,
            top - 8,
            alpha,
            4
        );
        owner.mediumFont.SetShadow(static_cast<Pixel>(owner.textColors.secondary[4]));
        owner.mediumFont.DrawText(
            owner.drawSurface,
            edge + 0xC,
            top - 0x1A,
            window.name,
            static_cast<Pixel>(owner.textColors.primary[4]),
            4
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
    u32 found = 0xFFFFFFFF;
    bool regionsAllowed = true;

    if (state.leader.id == 0xFFFF && (player.act.action == 0 || player.act.action == 0x34))
    {
        for (found = 1; found < static_cast<u32>(state.roleCount); ++found)
        {
            const Role& role = state.roles[found];

            if ((role.flags & Role::Visible) == 0 || role.event == 0 || role.unk26 != 0)
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
            cursor = 9;

            if ((state.roles[found].flags & Role::Examine) != 0)
            {
                cursor = 0xA;
                regionsAllowed = false;
            }
        }
    }

    auto faceAway = [](int direction)
    {
        return (direction & 0xFFFE) + ((direction - 1) & 1);
    };

    auto standing = [&player]()
    {
        return player.act.action == 0x34 || player.act.action == 0;
    };

    if (!state.GameFlag(GameState::NoChoicePointer) && !state.clickRegions.regions.empty() && standing())
    {
        ClickRegion* region = nullptr;
        const int index = state.clickRegions.Find(state.pointerX, state.pointerY, region);
        cursor = 0xD;

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

    if (found == 0xFFFFFFFF || found == static_cast<u32>(state.roleCount) ||
        found == static_cast<u32>(state.playerRole) || state.leader.id != 0xFFFF || state.roles[found].unk26 != 0 ||
        left.press == 0 || left.heldFrames != 1)
    {
        const std::vector<u32>& blocks = state.map.blocks;
        const auto cellX = static_cast<u32>(cameraLeft + state.pointerX) >> 4;
        const auto cellY = static_cast<u32>(cameraTop + state.pointerY) >> 4;
        const std::size_t cell = cellX + static_cast<std::size_t>(state.map.width) * cellY;
        const u8 regionId = cell < blocks.size() ? static_cast<u8>(blocks[cell]) : u8{0};
        MapRegion* region = nullptr;

        if (state.leader.id == 0xFFFF && regionsAllowed && standing() && regionId != 0 &&
            (region = owner.field.FindRegion(regionId)) != nullptr && state.GameFlag(region->flag & 0xFFFF))
        {
            cursor = 0xB;

            if (left.press == 0 || left.heldFrames != 1)
            {
                return;
            }

            Role& leader = state.leader;
            leader.unk14 = 0;
            leader.unk20 = 0;
            leader.flags = 0;
            leader.event = static_cast<u16>(region->event & 0x7FFF);
            leader.x = cameraLeft + (state.pointerX & ~0xF);
            leader.id = 0xFFFD;
            leader.y = cameraTop + (state.pointerY & ~0xF);
            const int direction = state.DirectionToPlayer(cameraLeft + state.pointerX, cameraTop + state.pointerY);
            player.act.savedDirection = player.act.direction;
            player.act.savedAction = player.act.action;
            player.act.action = 0;
            player.act.direction = faceAway(direction);
            player.act.delay = 0;
            owner.acts.Run(player.act);
            owner.scene.unk4A9920 = 0x8000;
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
            player.x + 8 * player.act.width - cameraLeft,
            player.y + 8 * player.act.height - cameraTop,
            distance
        );
        const int direction = AngleToDirection(angle);

        if (cursor == 0xD)
        {
            cursor = direction;
        }

        const ButtonState& right = controls.At(Button::MouseRight);

        if (((player.x | player.y) & 0xF) == 0 && owner.scene.unk4A9920 == 0 && right.press != 0 && distance >= 0x10)
        {
            if (direction == 0 || direction == 4 || direction == 7)
            {
                controls.At(Button::Up).press = right.press;
            }

            if (direction == 1 || direction == 5 || direction == 6)
            {
                controls.At(Button::Down).press = right.press;
            }

            if (direction == 2 || direction == 4 || direction == 6)
            {
                controls.At(Button::Left).press = right.press;
            }

            if (direction == 3 || direction == 5 || direction == 7)
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
    const int direction = state.DirectionToPlayer(target.x + 8 * target.act.width, target.y + 8 * target.act.height);

    if ((target.flags & Role::FacesTalker) != 0)
    {
        target.act.savedAction = target.act.action;
        target.act.savedDirection = target.act.direction;
        target.act.action = 0;
        target.act.delay = 0;
        target.act.direction = direction;
        owner.acts.Run(target.act);
    }

    player.act.action = 0;
    player.act.direction = faceAway(direction);
    player.act.delay = 0;
    owner.acts.Run(player.act);
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

}  // namespace swd3
