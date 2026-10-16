#include "talk/talk.h"

#include <span>
#include <string>

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

Talk::Talk(Game& game) : game(game)
{
}

// swd32.exe 0x00430350: text in the large font with the colour and the shadow colour of the tables
void Talk::DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style)
{
    TextFont& font = this->game.largeFont;
    const TextColors& colors = this->game.textColors;
    font.SetBackground(TextFont::kNoBackground);
    font.SetShadow(static_cast<Pixel>(colors.secondary[static_cast<std::size_t>(shade) & 0xF]));
    font.DrawText(pixels, x, y, text, static_cast<Pixel>(colors.primary[static_cast<std::size_t>(color) & 0xF]), style);
    font.SetBackground(TextFont::kNoBackground);
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

        if (window.page != TalkWindow::kClosed)
        {
            link = &window.next;
            continue;
        }

        if (window.role != TalkWindow::kNoRole && window.role < GameState::kRoles)
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

    if ((window.flags & 0x40) != 0)
    {
        opening = false;

        if ((window.flags & 0x8000) != 0)
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

        if ((window.flags & 0x800) != 0)
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
        else if (window.role != TalkWindow::kNoRole)
        {
            if (++window.openStep > 4)
            {
                window.openStep = 4;
                opening = false;
            }

            const Role& speaker = state.roles[std::min<std::size_t>(window.role, GameState::kRoles - 1)];
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

    if ((window.flags & 0x80) != 0)
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
        this->unk4B7CCC = 1;
    }

    if (this->unk4B7D7C == 1)
    {
        this->unk4B7CCC = 1;
    }

    if ((window.flags & 0x20) == 0x20 && this->unk4B7CCC == 1)
    {
        window.flags |= static_cast<i32>(window.flags) < 0 ? 0x40000000u : 0x80000000u;
    }

    if ((window.flags & 0x40000000) == 0x40000000)
    {
        window.speed = 0xFFFF;

        if ((window.flags & 0x20000000) == 0)
        {
            window.delay = 0xFFFF;
            window.flags |= 0x20000000;
        }
    }

    const u32 now = owner.frameStart;

    if ((window.flags & 0xC000) != 0)
    {
        if ((window.flags & 0x400) != 0 && this->skipAll == 0)
        {
            this->advance = 0;
        }

        bool timedOut = false;

        if ((window.flags & 0x8000) == 0)
        {
            if (this->advance != 0)
            {
                window.flags |= 0x200;
            }

            if (this->unk4B7CCC != 1)
            {
                this->advance = 0;
                window.flags &= ~0x200u;
            }

            if (this->advance != 0)
            {
                window.flags |= 0x40000000;
            }

            if (window.timeout == 0xFFFF)
            {
                if ((window.flags & 0x400) != 0)
                {
                    this->advance = heldAdvance;
                }
            }
            else if ((now - window.shownAt) / 0x64 > window.timeout && (window.flags & 8) != 0)
            {
                window.shownAt = 0;
                timedOut = true;
            }
        }
        else
        {
            if (heldAdvance != 0)
            {
                window.flags |= 1;
            }

            if (this->advance != 0)
            {
                window.flags |= 0x200;
            }

            if (this->unk4B7CCC != 1)
            {
                this->advance = 0;
                window.flags &= ~0x200u;
            }

            timedOut =
                window.timeout != 0xFFFF && (now - window.shownAt) / 0x64 > window.timeout && (window.flags & 8) != 0;
        }

        if (timedOut)
        {
            window.flags |= 0x200;
            this->advance = 1;
        }

        if ((window.flags & 0x40) == 0x40)
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
                    window.flags |= 0x200;
                    this->advance = 1;
                }
            }
        }
    }
    else
    {
        window.counter = 0x24;

        if ((window.flags & 0x20) == 0x20)
        {
            this->advance = 0;
        }
        else if (this->advance != 0)
        {
            window.flags |= 0x40000000;
        }
    }

    ClickRegions& regions = state.clickRegions;

    if ((window.flags & 0x2000) != 0)
    {
        window.counter = 0x20;

        if (regions.regions.empty())
        {
            window.counter = 1;
            this->advance = 1;
            window.flags |= 0x200;
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
            if ((window.flags & 0x20) == 0)
            {
                window.delay = 0x8FFF;
            }

            if (regions.regions.empty())
            {
                if ((window.flags & 0x4000) != 0 && this->advance != 2)
                {
                    nextPage = true;
                    break;
                }

                if ((window.flags & 0x200) != 0)
                {
                    if ((window.flags & 0x800) == 0)
                    {
                        if (window.role == TalkWindow::kNoRole)
                        {
                            state.leader.unk26 = 0;
                        }
                        else if (window.role < GameState::kRoles)
                        {
                            Role& speaker = state.roles[window.role];

                            if ((speaker.flags & 0x800) != 0)
                            {
                                owner.acts.Run(speaker.act);
                            }
                        }

                        if ((window.flags & 0x10) != 0)
                        {
                            this->unk4CAE94 = 0xC;
                            const i32 kept = owner.scene.unk4A9920 & 0x8000;
                            const i32 locks = std::max((owner.scene.unk4A9920 & ~0x8000) - 1, 0);
                            owner.scene.unk4A9920 = kept | locks;
                        }
                    }

                    window.page = TalkWindow::kClosed;
                    this->unk4B7CC0 = 0;
                    this->unk4B7CCC = 0;
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

            if ((window.flags & 0x8000) == 0)
            {
                if (window.shownAt == 0)
                {
                    window.shownAt = now;
                }

                if ((window.flags & 0x40) != 0)
                {
                    window.counter = 0x10;
                }
            }

            window.flags |= 0x8000;

            if (!regions.regions.empty())
            {
                regions.flags |= 0x1000;
            }

            break;
        }

        const int after = x + column;

        if (unit * after > window.width || code == kNewLine)
        {
            flush();

            if (2 * unit * (line + 1) >= window.height)
            {
                window.flags |= 0x4000;

                if (!regions.regions.empty())
                {
                    regions.flags |= 0x1000;
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

                if (this->skipAll != 0 || (window.flags & 0x100) != 0)
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
                    regions.flags |= 0x1000;
                }

                window.typed = consumed + 2 + window.page;
                consumed += 2;

                if (window.shownAt == 0)
                {
                    window.shownAt = now;
                }

                if (this->skipAll != 0 || (window.flags & 0x100) != 0)
                {
                    window.typed = consumed + window.page;
                }

                savePage();
                window.flags |= 0x4000;
                done = true;
                break;

            case kSpeed:
            {
                const auto digit = static_cast<u8>(byte(cursor + 2) - '0');
                cursor += 3;
                consumed += 3;
                window.speed = static_cast<u16>(2 * (owner.config.textSpeed + digit));

                if ((window.flags & 0x40000000) == 0x40000000)
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
                window.flags |= 0x2000;
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

                owner.largeFont.SetBackground(TextFont::kNoBackground);
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
                window.flags |= 0x40000000;
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

        window.flags &= ~0x4300u;
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

    if ((window.flags & 0xC000) != 0 &&
        (((window.flags & 2) != 0 && (window.flags & 1) == 0) || (window.flags & 0x400) == 0))
    {
        const ActState& mark = (window.flags & 0x8000) == 0 ? this->nextAct : this->endAct;
        const Picture* picture = owner.archive.GetPicture(mark.frame, mark.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(right - mark.x - 4, bottom - mark.y - 8, picture->width, picture->height, 0);
        }
    }

    if (!opening && !window.name.empty() && window.page != TalkWindow::kClosed)
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

}  // namespace swd3
