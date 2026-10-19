#include "map/field_view_internal.h"

namespace swd3
{

// swd32.exe 0x004147E0: the animations of a list stepped and drawn at their screen places with their translucency and
// sound cues; finished ones (the script reached its end) are freed
void FieldView::DrawAnimations(std::unique_ptr<ScriptAnimation>& list)
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    std::unique_ptr<ScriptAnimation>* link = &list;

    while (*link != nullptr)
    {
        ScriptAnimation& animation = **link;
        // 0x0041486B, 0x00414898: the place words are zero-extended (xor, mov cx, [esi]).
        const int x = static_cast<u16>(animation.x);
        const int y = static_cast<u16>(animation.y);

        if (this->replaying)
        {
            // Smooth motion: the frame the act showed last, nothing stepped, played or freed.
            const Picture* shown = owner.archive.GetPicture(animation.act.frame, animation.act.picture);

            if (shown != nullptr)
            {
                painter.Options().alpha = animation.act.signal;
                painter.SetSource(shown->data.data());
                painter.DrawPicture(
                    x - animation.act.x,
                    y - animation.act.y,
                    shown->width,
                    shown->height,
                    animation.act.flags
                );
            }

            link = &animation.next;
            continue;
        }

        if (owner.acts.Run(animation.act) == 0)
        {
            DebugLog(
                u8"Act Err(PicPaint:PlayAct) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
                animation.act.id,
                animation.act.action,
                animation.act.direction
            );
        }

        const Picture* picture = owner.archive.GetPicture(animation.act.frame, animation.act.picture);

        if (picture != nullptr)
        {
            painter.Options().alpha = animation.act.signal;
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                x - animation.act.x,
                y - animation.act.y,
                picture->width,
                picture->height,
                animation.act.flags
            );
        }

        if (animation.act.view != 0)
        {
            owner.PlaySoundAt(animation.act.view, owner.field.cameraLeft + x, owner.field.cameraTop + y);
            animation.act.view = 0;
        }

        if (animation.act.waiting == 1)
        {
            *link = std::move(animation.next);
        }
        else
        {
            link = &animation.next;
        }
    }
}

// swd32.exe 0x00414B60: the sprites over the map: stepped, drawn when near the screen, moved by their steps (unless
// the script waits) and freed once within 32 pixels of their target
void FieldView::DrawSprites()
{
    Game& owner = this->game;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    std::unique_ptr<ScriptSprite>* link = &owner.scene.spritesA;

    while (*link != nullptr)
    {
        ScriptSprite& sprite = **link;
        const u64 key = EffectKey(EffectKind::Sprites, &sprite);
        const auto fromX = static_cast<i32>(crt::Ftol(sprite.x));
        const auto fromY = static_cast<i32>(crt::Ftol(sprite.y));
        const std::array<i32, 2> place = this->EffectAt(key, fromX, fromY, false);
        const auto screenX = place[0] - field.cameraLeft;
        const auto screenY = place[1] - field.cameraTop;

        if (this->replaying)
        {
            // Smooth motion: the frame the act showed last where the sprite glides, nothing stepped, moved or freed.
            const Picture* shown = owner.archive.GetPicture(sprite.act.frame, sprite.act.picture);

            if (shown != nullptr && screenX > -kSpriteMargin && screenX < Display::Width + kSpriteMargin &&
                screenY > -kSpriteMargin && screenY < Display::Height + kSpriteMargin)
            {
                painter.Options().alpha = sprite.act.signal;
                painter.SetSource(shown->data.data());
                painter.DrawPicture(
                    screenX - sprite.act.x,
                    screenY - sprite.act.y,
                    shown->width,
                    shown->height,
                    sprite.act.flags,
                    PictureArgument(*shown)
                );
            }

            link = &sprite.next;
            continue;
        }

        if (owner.acts.Run(sprite.act) == 0)
        {
            DebugLog(
                u8"Act Err(PicPaint:DrawTObj) 設定錯誤 \nact[%d] qq[%d] dir[%d]",
                sprite.act.id,
                sprite.act.action,
                sprite.act.direction
            );
        }

        if (screenX > -kSpriteMargin && screenX < Display::Width + kSpriteMargin && screenY > -kSpriteMargin &&
            screenY < Display::Height + kSpriteMargin)
        {
            const Picture* picture = owner.archive.GetPicture(sprite.act.frame, sprite.act.picture);

            if (picture != nullptr)
            {
                painter.Options().alpha = sprite.act.signal;
                painter.SetSource(picture->data.data());
                painter.DrawPicture(
                    screenX - sprite.act.x,
                    screenY - sprite.act.y,
                    picture->width,
                    picture->height,
                    sprite.act.flags,
                    PictureArgument(*picture)
                );
            }
        }

        if (sprite.act.delay != 0)
        {
            link = &sprite.next;
            continue;
        }

        const double nextX = static_cast<double>(sprite.stepX) + static_cast<double>(sprite.x);
        sprite.x = static_cast<float>(nextX);
        const float nextY = sprite.stepY + sprite.y;
        sprite.y = nextY;
        const auto movedX = static_cast<int>(crt::Ftol(nextX));
        const auto movedY = static_cast<int>(crt::Ftol(nextY));

        // The rebuild's continuous motion: the sprite takes its step until it steps again, when its act's next delay
        // runs out (a frame past FieldView::RoleStepMostFrames).
        if (owner.smoothMotion)
        {
            const auto next = static_cast<i32>(owner.acts.Peek(sprite.act).delay) + 1;
            const i32 frames = next > FieldView::RoleStepMostFrames ? 1 : next;
            this->MoveEffect(key, fromX, fromY, movedX, movedY, frames, false, kEffectSnap);
        }

        if (movedX <= sprite.targetX - kSpriteArrival || movedX >= sprite.targetX + kSpriteArrival ||
            movedY <= sprite.targetY - kSpriteArrival || movedY >= sprite.targetY + kSpriteArrival)
        {
            link = &sprite.next;
        }
        else
        {
            *link = std::move(sprite.next);
        }
    }
}

// swd32.exe 0x004308C0: the time left of a timed event (which 0: the timer 0x004CAD20 under flag 0x10, 1: 0x004A9918
// under flag 0x4A) in frames as [h]m:ss with the digits of act 0x232C, unless flag 0x4C hides it
void FieldView::DrawTimer(int x, int y, int which)
{
    Game& owner = this->game;
    const GameState& state = owner.state;

    if (which == 0 && !state.GameFlag(GameState::Timer))
    {
        return;
    }

    if (which == 1 && !state.GameFlag(GameState::SecondTimer))
    {
        return;
    }

    if (state.GameFlag(GameState::HideTimer))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    int seconds = (which != 0 ? view.unk4A9918 : view.unk4CAD20) / kFramesPerSecond;

    if (seconds < 0)
    {
        seconds = 0;
    }

    ActState& digits = view.unk4A9A30;
    digits.id = Scene::TimerAct;
    digits.direction = 0;
    int at = x;

    auto draw = [&owner, &painter, &digits, &at, y](int action)
    {
        digits.action = action;
        owner.acts.Run(digits);
        const Picture* picture = owner.archive.GetPicture(digits.frame, digits.picture);

        if (picture != nullptr)
        {
            painter.SetSource(picture->data.data());
            painter.DrawPicture(at, y, picture->width, picture->height, Painter::Copy);
            at += picture->width;
        }
    };

    if (seconds / (kTensBase * kSecondsPerMinute) != 0)
    {
        draw(seconds / (kTensBase * kSecondsPerMinute));
    }

    draw(seconds / kSecondsPerMinute % kTensBase);
    draw(kTimerColon);
    const int rest = seconds % kSecondsPerMinute;
    draw(rest / kTensBase);
    draw(rest % kTensBase);
}

// swd32.exe 0x00430C60: a ripple over `lines` lines of `pitch` pixels at (left, top) with a gridWidth x gridHeight grid
ScriptObject::ScriptObject(int pitch, int lines, int left, int top, int gridWidth, int gridHeight)
    : pitch(pitch),
      lines(lines),
      left(left),
      top(top),
      gridWidth(gridWidth),
      gridHeight(gridHeight),
      damping(kDefaultDamping),
      heights(
          kPages * static_cast<std::size_t>(std::max(gridWidth, 0)) * static_cast<std::size_t>(std::max(gridHeight, 0)),
          0
      ),
      screen(static_cast<std::size_t>(std::max(pitch, 0)) * static_cast<std::size_t>(std::max(lines, 0)), 0)
{
}

// swd32.exe 0x00430D10: the ripple moved to (x, y), kept one cell inside the right and bottom of the screen copy
void ScriptObject::MoveTo(int x, int y)
{
    this->left = x;
    this->top = y;

    if (this->gridWidth + x > this->pitch)
    {
        this->left = this->pitch - this->gridWidth - 1;
    }

    if (this->gridHeight + y > this->lines)
    {
        this->top = this->lines - this->gridHeight - 1;
    }
}

// swd32.exe 0x00430FF0: a drop of `radius` cells into the current page, deepest (radius x depth) at its centre;
// a negative place is chosen at random inside the grid
void ScriptObject::Splash(int x, int y, int radius, int depth, crt::Random& random)
{
    if (x < 0)
    {
        x = random.Next() % (this->gridWidth - 2 * radius - 1) + radius + 1;
    }

    if (y < 0)
    {
        y = random.Next() % (this->gridHeight - 2 * radius - 1) + radius + 1;
    }

    int fromX = -radius;
    int fromY = -radius;
    int toX = radius;
    int toY = radius;

    if (x - radius < 1)
    {
        fromX = 1 - x;
    }

    if (y - radius < 1)
    {
        fromY = 1 - y;
    }

    if (x + radius > this->gridWidth - 1)
    {
        toX = this->gridWidth - x - 1;
    }

    if (radius + y > this->gridHeight - 1)
    {
        toY = this->gridHeight - y - 1;
    }

    const int page = this->gridWidth * this->gridHeight * this->page;

    for (int dy = fromY; dy < toY; ++dy)
    {
        for (int dx = fromX; dx < toX; ++dx)
        {
            if (dx * dx + dy * dy < radius * radius)
            {
                const int at = page + x + dx + this->gridWidth * (dy + y);
                const auto drop = static_cast<i16>(
                    crt::Ftol((static_cast<double>(radius) - std::sqrt(static_cast<double>(dx * dx + dy * dy))) * depth)
                );

                if (at >= 0 && static_cast<std::size_t>(at) < this->heights.size())
                {
                    this->heights[static_cast<std::size_t>(at)] =
                        static_cast<i16>(this->heights[static_cast<std::size_t>(at)] + drop);
                }
            }
        }
    }
}

// swd32.exe 0x00430CF0: its grid and screen copy freed
ScriptObject::~ScriptObject() = default;

// swd32.exe 0x00430D50: a copy of the screen
void ScriptObject::CopyScreen(std::span<const u16> pixels)
{
    const std::size_t count = std::min(pixels.size(), static_cast<std::size_t>(this->pitch) * this->lines);
    this->screen.assign(pixels.begin(), pixels.begin() + static_cast<std::ptrdiff_t>(count));
}

// swd32.exe 0x00430D80: the screen inside the grid redrawn from the copy, each pixel taken from the place its wave
// slopes point at (the differences to the cells right of and below it, divided by 8); only the first page is read
void ScriptObject::Refract(std::span<u16> pixels) const
{
    const int width = this->gridWidth;
    const int last = width * (this->gridHeight - 1);
    int at = this->left + this->pitch * (this->top + 1);

    auto height = [this](int index) -> int
    {
        return index >= 0 && static_cast<std::size_t>(index) < this->heights.size()
                   ? this->heights[static_cast<std::size_t>(index)]
                   : 0;
    };

    auto copy = [this](int index, u16& out)
    {
        if (index >= 0 && static_cast<std::size_t>(index) < this->screen.size())
        {
            out = this->screen[static_cast<std::size_t>(index)];
        }
    };

    for (int cell = width + 1; cell < last; at += this->pitch - width + kGridBorder)
    {
        const int end = width + cell - kGridBorder;

        while (cell < end)
        {
            for (int pair = 0; pair < kPixelsPerPair; ++pair)
            {
                const int here = height(cell);
                const int source = this->pitch * ((here - height(cell + width)) >> kRefractShift) + at +
                                   ((here - height(cell + 1)) >> kRefractShift);

                if (at >= 0 && static_cast<std::size_t>(at) < pixels.size())
                {
                    copy(source, pixels[static_cast<std::size_t>(at)]);
                }

                ++cell;
                ++at;
            }
        }

        cell += kGridBorder;
    }
}

// swd32.exe 0x00430EB0: one step of the waves into the other page: each cell becomes the average of its neighbours'
// sums less its old value, damped; true once every cell of the new page is 0
bool ScriptObject::Step()
{
    const int width = this->gridWidth;
    const int cells = width * this->gridHeight;
    this->page ^= 1;
    const int target = this->page * cells;
    const int source = (this->page ^ 1) * cells;
    bool settled = true;

    auto read = [this, source](int index) -> int
    {
        const int at = source + index;
        return at >= 0 && static_cast<std::size_t>(at) < this->heights.size()
                   ? this->heights[static_cast<std::size_t>(at)]
                   : 0;
    };

    int cell = width + 1;
    int sum = read(2 * width + kGridBorder) + read(kGridBorder);
    const int last = width * (this->gridHeight - 1);

    while (cell < last)
    {
        int before = sum;
        sum = read(width + cell) + read(cell - width);
        const int end = width + cell - kGridBorder;

        while (cell < end)
        {
            const int current = sum;
            sum = read(cell + width + 1) + read(cell - width + 1);
            const auto at = static_cast<std::size_t>(target + cell);

            if (at < this->heights.size())
            {
                const int change =
                    ((before + current + read(cell - 1) + sum + read(cell + 1)) >> kAverageShift) - this->heights[at];
                const auto value = static_cast<i16>(change - (change >> this->damping));
                this->heights[at] = value;

                if (value != 0)
                {
                    settled = false;
                }
            }

            ++cell;
            before = current;
        }

        cell += kGridBorder;
    }

    return settled;
}

}  // namespace swd3
