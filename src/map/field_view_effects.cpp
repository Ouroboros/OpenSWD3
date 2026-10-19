#include "map/field_view_internal.h"

namespace swd3
{

// swd32.exe 0x004146F0: while a fade step is set the screen is tinted by the levels reached, which grow by the steps
// for its frames (counted with `step`); once they run out the steps are restored from their start values
void FieldView::FadeColors(int step)
{
    ColorFade& fade = this->game.scene.fade;

    if (fade.stepRed == 0.0F && fade.stepGreen == 0.0F && fade.stepBlue == 0.0F)
    {
        return;
    }

    bool restore = false;

    if (step != 0)
    {
        if (fade.frames < 0)
        {
            restore = true;
        }
        else
        {
            --fade.frames;
        }
    }

    if (!restore && fade.frames >= 0)
    {
        fade.red = fade.red + fade.stepRed;
        fade.green = fade.green + fade.stepGreen;
        fade.blue = fade.blue + fade.stepBlue;
    }
    else
    {
        fade.stepRed = fade.startRed;
        fade.stepGreen = fade.startGreen;
        fade.stepBlue = fade.startBlue;
    }

    this->DrawScreenTint(
        static_cast<int>(crt::Ftol(fade.red)),
        static_cast<int>(crt::Ftol(fade.green)),
        static_cast<int>(crt::Ftol(fade.blue))
    );
}

void FieldView::DrawScreenTint(int red, int green, int blue)
{
    this->game.painter.TintPixels(std::span<u16>(this->game.drawSurface, Display::Pixels), red, green, blue);
}

// swd32.exe 0x00414E50: the lines of the screen blended with their colour: still ones whole, the others growing or
// shrinking column by column at random speeds; a closed line becomes still, an opened one is freed
void FieldView::DrawLines()
{
    Game& owner = this->game;
    const Painter& painter = owner.painter;
    Random& random = owner.random;
    const int pitch = painter.ScreenPitch();
    const std::span<u16> screen(owner.drawSurface, static_cast<std::size_t>(pitch) * Display::Height);

    // 0x00417DE0 is given the pixels of the run and blends half as many pairs (none below one pair, deviations.md).
    auto blend = [&painter, screen](int at, u32 color, int pixels)
    {
        const u32 pairs = static_cast<u32>(pixels) >> 1;

        for (u32 pair = 0; pair < pairs; ++pair)
        {
            const i64 place = at + static_cast<i64>(kPixelsPerPair) * pair;
            const auto first = static_cast<std::size_t>(place);

            if (place < 0)
            {
                continue;
            }

            if (first + 1 >= screen.size())
            {
                break;
            }

            u32 value = MakeWords(screen[first + 1], screen[first]);
            painter.BlendPairsWithColor(std::span<u32>(&value, 1), color);
            screen[first] = LowWord(value);
            screen[first + 1] = HighWord(value);
        }
    };

    std::unique_ptr<ScriptEffect>* link = &owner.scene.effects;

    while (*link != nullptr)
    {
        ScriptEffect& line = **link;
        const u32 color = owner.textColors.primary[static_cast<std::size_t>(line.color) & (TextColors::Colors - 1)];

        const auto column = [](int index) -> std::size_t
        {
            return static_cast<std::size_t>(index);
        };

        auto at = [&line, pitch](int index)
        {
            return line.x + line.offsets[static_cast<std::size_t>(index)] + pitch * (index + line.y);
        };

        if (line.offsets.size() < static_cast<std::size_t>(std::max<int>(line.count, 0)) ||
            line.lengths.size() < static_cast<std::size_t>(std::max<int>(line.count, 0)))
        {
            link = &line.next;
            continue;
        }

        const int kind = line.mode & ScriptEffect::KindMask;

        if (kind == ScriptEffect::Still)
        {
            for (int row = 0; row < line.count; ++row)
            {
                blend(line.x + pitch * (line.y + row), color, line.limit);
            }

            link = &line.next;
            continue;
        }

        if (kind == ScriptEffect::Falling)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length =
                    static_cast<i16>(length + kJitterStep * static_cast<int>(random.Below(kFallJitter)) + kLineStep);

                if (length >= line.limit)
                {
                    ++done;
                    length = line.limit;
                }

                blend(at(index), color, length);
            }

            if (done == line.count)
            {
                line.mode = static_cast<u16>((line.mode & ScriptEffect::NumberMask) | ScriptEffect::Still);
            }
        }

        if ((line.mode & ScriptEffect::KindMask) == ScriptEffect::Rising)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset =
                    static_cast<i16>(offset - kLineStep - kJitterStep * static_cast<int>(random.Below(kRiseJitter)));

                if (offset <= 0)
                {
                    ++done;
                    offset = 0;
                }

                line.lengths[column(index)] = static_cast<i16>(line.limit - offset);
                blend(at(index), color, line.lengths[column(index)]);
            }

            if (done == line.count)
            {
                line.mode = static_cast<u16>((line.mode & ScriptEffect::NumberMask) | ScriptEffect::Still);
            }
        }

        bool finished = false;

        if ((line.mode & ScriptEffect::KindMask) == ScriptEffect::Opening)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& offset = line.offsets[column(index)];
                offset =
                    static_cast<i16>(offset + kJitterStep * static_cast<int>(random.Below(kOpenJitter)) + kLineStep);

                if (offset >= line.limit)
                {
                    ++done;
                    offset = line.limit;
                }

                i16& length = line.lengths[column(index)];
                length = static_cast<i16>(line.limit - offset);

                if (length < kMinLineLength)
                {
                    length = kMinLineLength;
                }

                blend(at(index), color, length);
            }

            finished = done == line.count;
        }

        if (!finished && (line.mode & ScriptEffect::KindMask) == ScriptEffect::Closing)
        {
            int done = 0;

            for (int index = line.count - 1; index >= 0; --index)
            {
                i16& length = line.lengths[column(index)];
                length =
                    static_cast<i16>(length - kLineStep - kJitterStep * static_cast<int>(random.Below(kOpenJitter)));

                if (length <= kMinLineLength)
                {
                    ++done;
                    length = kMinLineLength;
                }

                blend(at(index), color, length);
            }

            finished = done == line.count;
        }

        if (finished)
        {
            *link = std::move(line.next);
        }
        else
        {
            link = &line.next;
        }
    }
}

// swd32.exe 0x00415B70: the weather (story flag 5): the first two particles drift with the camera at their own pace and
// shade; one off the map is placed again at an edge by the map's weather kind, now and then
void FieldView::DrawWeather()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (!state.GameFlag(GameState::Weather))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;

    for (std::size_t particle = 0; particle < kWeatherParticles; ++particle)
    {
        const auto roll = static_cast<int>(random.Below(kPerMille));
        const int width = state.map.width;
        MapParticle& drop = view.weather[particle];
        WeatherShade& shade = view.weatherShades[particle];
        WeatherMotion& motion = view.weatherMotion[particle];

        if (drop.x <= -kWeatherMarginX || drop.x >= MapHeader::TileSize * width + kWeatherMarginX ||
            drop.y <= -kWeatherMarginY || drop.y >= MapHeader::TileSize * state.map.height + kWeatherMarginY)
        {
            if (roll < kWeatherReturnRoll)
            {
                continue;
            }

            shade.tintTarget = 0;
            motion.speedTarget = roll / kWeatherSpeedStep + 1;
            motion.action = view.weatherAction + roll * view.weatherActions / kPerMille;

            switch (view.unk4B7EB4)
            {
                case kWeatherFromBelow:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = MapHeader::TileSize * state.map.height + kWeatherBelow;
                    drop.stepX = static_cast<i32>(random.Below(1)) - kWeatherDrift;
                    drop.stepY = static_cast<i32>(random.Below(1)) - kWeatherDrift;
                    break;

                case kWeatherFromBelowDrifting:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = MapHeader::TileSize * state.map.height + kWeatherBelow;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift));
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift));
                    break;

                case kWeatherFromAbove:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = -kWeatherAbove;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift)) - kWeatherDrift;
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift)) - kWeatherDrift;
                    break;

                case kWeatherFromAboveDrifting:
                    drop.x = static_cast<i32>(random.Below(static_cast<u32>(MapHeader::TileSize * width)));
                    drop.y = -kWeatherAbove;
                    drop.stepX = static_cast<i32>(random.Below(kWeatherDrift));
                    drop.stepY = static_cast<i32>(random.Below(kWeatherDrift));
                    break;

                default:
                    break;
            }

            continue;
        }

        const int speed = motion.speed;

        if (++motion.counter > speed)
        {
            const i32 fromX = drop.x;
            const i32 fromY = drop.y;
            motion.counter = 0;
            drop.x += drop.stepX + ((owner.unk4B7518 * owner.state.walkSpeed) >> kWalkQuarter);
            drop.y += drop.stepY + ((owner.state.walkSpeed * owner.unk4B7920) >> kWalkQuarter);

            if (roll < kWeatherSpeedChance)
            {
                motion.speedTarget = static_cast<i32>(random.Below(kWeatherSpeeds)) + 1;
            }

            if (motion.speed != motion.speedTarget)
            {
                motion.speed += motion.speed <= motion.speedTarget ? 1 : -1;
            }

            if (roll < kWeatherTintChance)
            {
                shade.tintTarget = static_cast<i32>(random.Below(kWeatherTints)) - kWeatherDarkest;
            }

            if (shade.tint != shade.tintTarget)
            {
                shade.tint += shade.tint <= shade.tintTarget ? 1 : -1;
            }

            // The rebuild's continuous motion: the particle moves on until it moves again, speed plus one frames on.
            this->MoveEffect(
                EffectKey(EffectKind::Weather, particle),
                fromX,
                fromY,
                drop.x,
                drop.y,
                std::max(motion.speed, 0) + 1,
                false,
                kEffectSnap
            );
        }

        view.unk4ACAF8.id = Scene::EffectAct;
        view.unk4ACAF8.action = motion.action;
        owner.acts.Run(view.unk4ACAF8);
        const Picture* picture = owner.archive.GetPicture(view.unk4ACAF8.frame, view.unk4ACAF8.picture);

        if (picture == nullptr)
        {
            continue;
        }

        DrawOptions& options = painter.Options();
        options.tintBlue = shade.tint;
        options.tintGreen = shade.tint;
        options.tintRed = shade.tint;
        options.alpha = view.unk4ACAF8.signal;
        const std::array<i32, 2> place =
            this->EffectAt(EffectKey(EffectKind::Weather, particle), drop.x, drop.y, false);
        painter.SetSource(picture->data.data());
        painter.DrawPicture(
            place[0] - view.unk4ACAF8.x - field.cameraLeft,
            place[1] - view.unk4ACAF8.y - field.cameraTop,
            picture->width,
            picture->height,
            view.weatherActions == Scene::GlowWeatherActions ? Painter::Additive : Painter::Subtractive
        );
    }
}

// swd32.exe 0x004161C0: the leaves (story flag 6): four leaves blow across the map, entering again from a side once
// they leave it and now and then changing speed; the action follows the vertical speed
void FieldView::DrawLeaves()
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (!state.GameFlag(GameState::Leaves))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;
    Painter& painter = owner.painter;
    const Field& field = owner.field;
    const int width = MapHeader::TileSize * state.map.width;
    const int height = MapHeader::TileSize * state.map.height;

    for (std::size_t index = 0; index < view.leaves.size(); ++index)
    {
        MapParticle& leaf = view.leaves[index];
        ActState& act = view.unk4AD0D8[index];

        if (leaf.x <= -kLeafMargin || leaf.x >= width + kLeafMargin || leaf.y <= -kLeafMargin ||
            leaf.y >= height + kLeafMargin)
        {
            leaf.x = MapParticle::Unplaced;
        }

        if (leaf.x == MapParticle::Unplaced)
        {
            leaf.x = -kLeafEntry;
            leaf.y = static_cast<i32>(random.Below(static_cast<u32>(height)));
            leaf.stepY = static_cast<i32>(random.Below(2 * kLeafStartDown + 1)) - kLeafStartDown;
            leaf.stepX = static_cast<i32>(random.Below(2 * kLeafStartSide + 1)) - kLeafStartSide;

            if (leaf.stepX == 0)
            {
                leaf.stepX = kLeafZeroSide;
            }

            if (leaf.stepX < 0)
            {
                leaf.x = width + kLeafEntry;
            }
        }
        else if (random.Below(kPerMille) > kQuarterChance)
        {
            const int turn = static_cast<int>(random.Below(kTurnChoices)) - 1;
            leaf.stepY =
                std::clamp(static_cast<int>(random.Below(kTurnChoices)) - 1 + leaf.stepY, -kLeafMaxDown, kLeafMaxDown);
            leaf.stepX = std::clamp(turn + leaf.stepX, -kLeafMaxSide, kLeafMaxSide);

            if (leaf.stepX == 0)
            {
                leaf.stepX = -turn;
            }
        }

        act.action = kLeafActions[static_cast<std::size_t>(
            std::clamp(leaf.stepY + kLeafMaxDown, 0, static_cast<int>(kLeafActions.size()) - 1)
        )];

        if (leaf.stepX > 0)
        {
            ++act.action;
        }

        owner.acts.Run(act);
        const Picture* picture = owner.archive.GetPicture(act.frame, act.picture);

        if (picture != nullptr)
        {
            const std::array<i32, 2> place =
                this->EffectAt(EffectKey(EffectKind::Leaves, index), leaf.x, leaf.y, false);
            painter.SetSource(picture->data.data());
            painter.DrawPicture(
                place[0] - act.x - field.cameraLeft,
                place[1] - act.y - field.cameraTop,
                picture->width,
                picture->height,
                act.flags
            );
        }

        // The rebuild's continuous motion: the leaf takes its step over the next frame.
        this->MoveEffect(
            EffectKey(EffectKind::Leaves, index),
            leaf.x,
            leaf.y,
            leaf.x + leaf.stepX,
            leaf.y + leaf.stepY,
            1,
            false,
            kEffectSnap
        );
        leaf.x += leaf.stepX;
        leaf.y += leaf.stepY;
    }
}

// swd32.exe 0x004163C0: the heat haze (story flag 7): 48 strips of the screen copied up a line each frame; every
// fourth frame one of their lengths, places or line counts is chosen again (all three on the first frame)
void FieldView::DrawHeat()
{
    Game& owner = this->game;

    if (!owner.state.GameFlag(GameState::HeatHaze))
    {
        return;
    }

    Scene& view = owner.scene;
    Random& random = owner.random;

    auto newLength = [&random]()
    {
        return static_cast<i16>(kJitterStep * random.Below(kHeatLengthJitter) + kHeatLengthMin);
    };

    auto newOffset = [&random](int column, int line)
    {
        return line + static_cast<i32>(random.Below(kHeatSway)) + column + Display::Width - 1;
    };

    if (view.heatFrame != 0)
    {
        if (view.heatFrame % kHeatPeriod == 0)
        {
            switch (view.heatFrame / kHeatPeriod % kHeatPeriod)
            {
                case kHeatNewLengths:
                    for (i16& length : view.heatLengths)
                    {
                        length = newLength();
                    }

                    break;

                case kHeatNewOffsets:
                {
                    std::size_t index = 0;

                    for (int column = 0; column < Display::Width; column += kHeatStrip)
                    {
                        for (int line = 0; line < Display::Pixels; line += kHeatStrip * Display::Width)
                        {
                            view.heatOffsets[index++] = newOffset(column, line);
                        }
                    }

                    break;
                }

                case kHeatNewCounts:
                    for (i16& count : view.heatCounts)
                    {
                        count = static_cast<i16>(random.Below(kHeatLines) + 1);
                    }

                    break;

                default:
                    break;
            }
        }
    }
    else
    {
        std::size_t index = 0;

        for (int column = 0; column < Display::Width; column += kHeatStrip)
        {
            for (int line = 0; line < Display::Pixels; line += kHeatStrip * Display::Width)
            {
                view.heatLengths[index] = newLength();
                view.heatOffsets[index] = newOffset(column, line);
                view.heatCounts[index] = static_cast<i16>(random.Below(kHeatLines) + 1);
                ++index;
            }
        }
    }

    ++view.heatFrame;
    auto* bytes = reinterpret_cast<u8*>(owner.drawSurface);
    constexpr std::size_t kScreenBytes = sizeof(Pixel) * Display::Pixels;
    constexpr int kLineBytes = static_cast<int>(sizeof(Pixel)) * Display::Width;

    for (std::size_t index = 0; index < view.heatOffsets.size(); ++index)
    {
        const int count = view.heatCounts[index];
        const auto length = static_cast<u16>(view.heatLengths[index]);
        int at = static_cast<int>(sizeof(Pixel)) * view.heatOffsets[index];

        for (int line = 0; line < count; ++line, at += kLineBytes)
        {
            if (at < 0 || static_cast<std::size_t>(at) + kLineBytes + length > kScreenBytes)
            {
                continue;
            }

            std::memmove(bytes + at, bytes + at + kLineBytes, length);
        }
    }
}

// swd32.exe 0x00416590: lightning (story flag 8): the bolts wanted grow to 8 while the flag is set and shrink without
// it, changing only on one frame in ten; each bolt runs down the screen as a slanted line of brightening pixels
void FieldView::DrawLightning()
{
    Game& owner = this->game;
    Scene& view = owner.scene;
    Random& random = owner.random;
    bool draw = false;

    if (random.Below(kPerMille) > kTenthChance)
    {
        if (owner.state.GameFlag(GameState::Lightning))
        {
            if (++view.lightningCount > kMaxBolts)
            {
                view.lightningCount = kMaxBolts;
                draw = true;
            }
        }
        else if (--view.lightningCount < 0)
        {
            view.lightningCount = 0;
            draw = view.lightningShown != 0;

            if (!draw)
            {
                return;
            }
        }
    }

    if (!draw && view.lightningCount == 0 && view.lightningShown == 0)
    {
        return;
    }

    const int pitch = owner.painter.ScreenPitch();
    // 0x00416610: the end of the screen is fixed at 0x96000 bytes, whatever the pitch.
    constexpr auto pixels = static_cast<std::ptrdiff_t>(Display::Pixels);
    view.lightningShown = 0;
    int started = 0;

    for (Streak& bolt : view.bolts)
    {
        if ((bolt.active & 1) == 0)
        {
            if (started < view.lightningCount)
            {
                bolt.x = static_cast<i16>(kSubpixels * random.Below(Display::Width));
                bolt.y = 0;
                const auto slope = static_cast<i16>(random.Below(kSlopes));
                bolt.slope = bolt.x <= Display::Width / 2 * kSubpixels ? slope : static_cast<i16>(-slope);
                bolt.speed = static_cast<i16>(random.Below(kBoltSpeedJitter) + kBoltSpeedMin);
                bolt.length = static_cast<i16>(random.Below(kBoltLengthJitter) + kBoltLengthMin);
                bolt.life = bolt.speed < kBoltSpeedMin ? kShortLife : kBrightLife;
                bolt.phase = 0;
                bolt.active = 1;
                ++started;
            }

            continue;
        }

        int level = 0;
        const u64 key = EffectKey(EffectKind::Lightning, static_cast<u64>(&bolt - view.bolts.data()));
        const std::array<i32, 2> place = this->EffectAt(key, bolt.x / kSubpixels, bolt.y, true);
        int x = bolt.x + (place[0] - bolt.x / kSubpixels) * kSubpixels;
        std::ptrdiff_t row = static_cast<std::ptrdiff_t>(pitch) * place[1];

        for (int segment = 0; bolt.length >= 0;)
        {
            if (row > 0)
            {
                if (row >= pixels)
                {
                    bolt.active = 0;
                }
                else
                {
                    const std::ptrdiff_t at = row + x / kSubpixels;

                    if (at >= 0 && at < pixels)
                    {
                        owner.painter.TintPixels(std::span<u16>(owner.drawSurface + at, 1), level, level, level);
                    }
                }
            }
            else if (row >= pixels)
            {
                bolt.active = 0;
            }

            x += bolt.slope;
            row += pitch;
            ++level;

            if (bolt.life == kBrightLife)
            {
                ++level;
            }

            if (++segment > bolt.length)
            {
                break;
            }
        }

        const i32 fromX = bolt.x / kSubpixels;
        const i32 fromY = bolt.y;
        bolt.x = static_cast<i16>(bolt.x + bolt.speed * bolt.slope);
        bolt.y = static_cast<i16>(bolt.y + bolt.speed);
        // The rebuild's continuous motion: the bolt takes its step over the next frame.
        this->MoveEffect(key, fromX, fromY, bolt.x / kSubpixels, bolt.y, 1, true, kEffectSnap);

        if (--bolt.life != 0)
        {
            ++view.lightningShown;
        }
        else
        {
            bolt.active = 0;
        }
    }
}

// swd32.exe 0x004167B0: fireflies (story flag 0x16): at most one at a time drifts down the screen, a glowing dot with
// a dimmer cross around it that sways with its phase and fades as its life runs out
void FieldView::DrawFireflies()
{
    Game& owner = this->game;
    Scene& view = owner.scene;
    Random& random = owner.random;
    bool draw = false;

    if (random.Below(kPerMille) > kTenthChance)
    {
        if (owner.state.GameFlag(GameState::Fireflies))
        {
            if (++view.fireflyCount > kMaxFireflies)
            {
                view.fireflyCount = kMaxFireflies;
                draw = true;
            }
        }
        else if (--view.fireflyCount < 0)
        {
            view.fireflyCount = 0;
            draw = view.firefliesShown != 0;

            if (!draw)
            {
                return;
            }
        }
    }

    if (!draw && view.fireflyCount == 0 && view.firefliesShown == 0)
    {
        return;
    }

    Painter& painter = owner.painter;
    const int pitch = painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * Display::Height;
    const std::ptrdiff_t firstLine = Display::Width;
    const std::ptrdiff_t lastLine = Display::Pixels - Display::Width;
    view.firefliesShown = 0;
    int started = 0;

    auto tint = [&painter, &owner, pixels](std::ptrdiff_t at, int level)
    {
        if (at >= 0 && at < pixels)
        {
            painter.TintPixels(std::span<u16>(owner.drawSurface + at, 1), level, level, level);
        }
    };

    for (Streak& fly : view.fireflies)
    {
        if ((fly.active & 1) == 0)
        {
            if (started < view.fireflyCount)
            {
                fly.x = static_cast<i16>(kSubpixels * random.Below(Display::Width));
                fly.y = 0;
                const auto slope = static_cast<i16>(random.Below(kSlopes));
                fly.slope = fly.x <= Display::Width / 2 * kSubpixels ? slope : static_cast<i16>(-slope);
                fly.speed = static_cast<i16>(random.Below(kFireflySpeeds) + 1);
                fly.length = 1;
                fly.life = static_cast<i16>(kFireflyLife * fly.speed - static_cast<int>(random.Below(kFireflyLife)));
                fly.phase = 0;
                fly.active = 1;
                ++started;
            }

            continue;
        }

        auto level = static_cast<u32>(kFireflyGlow - (kFireflySpeeds * kFireflyLife - fly.life) / kFireflyFade);

        if (level > kFireflyDark)
        {
            level = 0;
        }

        const u64 key = EffectKey(EffectKind::Fireflies, static_cast<u64>(&fly - view.fireflies.data()));
        const std::array<i32, 2> place = this->EffectAt(key, fly.x / kSubpixels, fly.y, true);
        int x = fly.x + (place[0] - fly.x / kSubpixels) * kSubpixels;
        const int sway = kFireflySway[static_cast<std::size_t>(fly.phase / kSwayFrames) % kFireflySway.size()];
        std::ptrdiff_t row = static_cast<std::ptrdiff_t>(pitch) * place[1];

        for (int segment = 0; fly.length > 0;)
        {
            if (row > firstLine)
            {
                if (row >= lastLine)
                {
                    fly.active = 0;
                }
                else
                {
                    const auto full = static_cast<int>(level);
                    const int half = full >> 1;
                    const int quarter = full >> 2;
                    const std::ptrdiff_t center = row + x / kSubpixels + sway;
                    tint(center, full);
                    tint(center + 1, half);
                    tint(center - 1, half);
                    tint(center + pitch, half);
                    tint(center - pitch, half);
                    tint(center + pitch + 1, quarter);
                    tint(center + pitch - 1, quarter);
                    tint(center - pitch + 1, quarter);
                    tint(center - pitch - 1, quarter);
                }
            }
            else if (row >= lastLine)
            {
                fly.active = 0;
            }

            x += fly.slope;
            row += pitch;
            ++level;

            if (fly.life == kBrightLife)
            {
                ++level;
            }

            if (++segment >= fly.length)
            {
                break;
            }
        }

        const i32 fromX = fly.x / kSubpixels;
        const i32 fromY = fly.y;
        fly.x = static_cast<i16>(fly.x + fly.speed * fly.slope);
        fly.y = static_cast<i16>(fly.y + fly.speed);
        // The rebuild's continuous motion: the firefly takes its step over the next frame.
        this->MoveEffect(key, fromX, fromY, fly.x / kSubpixels, fly.y, 1, true, kEffectSnap);

        if (++fly.phase > kSwayFrames * (static_cast<int>(kFireflySway.size()) - 1))
        {
            fly.phase = 0;
        }

        fly.life = static_cast<i16>(fly.life - fly.speed);

        if (fly.life > 0)
        {
            ++view.firefliesShown;
        }
        else
        {
            fly.active = 0;
        }
    }
}

// swd32.exe 0x00416B30: the spotlight (story flag 0x13): its rim (action 0x4E, clipped by mixing its width and height)
// and the dark around it (action 0x4F, subtracted within 192 pixels); the light then moves towards its target
void FieldView::DrawSpotlight()
{
    Game& owner = this->game;

    if (!owner.state.GameFlag(GameState::Spotlight))
    {
        return;
    }

    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    ActState& act = view.unk4AD3F0;
    act.id = Scene::EffectAct;
    act.action = kSpotlightRimAction;
    owner.acts.Run(act);
    const Picture* rim = owner.archive.GetPicture(act.frame, act.picture);

    if (rim != nullptr)
    {
        painter.SetClipClamped(
            view.spotlightX - (rim->width >> 1),
            view.spotlightY - (rim->width >> 1),
            view.spotlightX + (rim->height >> 1),
            (rim->height >> 1) + view.spotlightY
        );
        painter.SetSource(rim->data.data());
        painter.DrawPicture(
            view.spotlightX - (rim->width >> 1),
            view.spotlightY - (rim->height >> 1),
            rim->width,
            rim->height,
            0
        );
    }

    act.action = kSpotlightDarkAction;
    owner.acts.Run(act);
    const Picture* dark = owner.archive.GetPicture(act.frame, act.picture);
    painter.SetClipClamped(
        view.spotlightX - Scene::SpotlightRadius,
        view.spotlightY - Scene::SpotlightRadius,
        view.spotlightX + Scene::SpotlightRadius,
        view.spotlightY + Scene::SpotlightRadius
    );

    if (dark != nullptr)
    {
        painter.SetSource(dark->data.data());
        painter.DrawPicture(
            view.spotlightX - (dark->width >> 1),
            view.spotlightY - (dark->height >> 1),
            dark->width,
            dark->height,
            Painter::Subtractive
        );
    }

    if (view.spotlightX != view.spotlightTargetX || view.spotlightY != view.spotlightTargetY)
    {
        const i32 fromX = view.spotlightX;
        const i32 fromY = view.spotlightY;
        view.spotlightX += view.spotlightStepX;
        view.spotlightY += view.spotlightStepY;
        // The rebuild's continuous motion: the spotlight takes its step over the next frame.
        this->MoveEffect(
            EffectKey(EffectKind::Spotlight, u64{0}),
            fromX,
            fromY,
            view.spotlightX,
            view.spotlightY,
            1,
            true,
            kEffectSnap
        );

        if (view.spotlightX == view.spotlightTargetX)
        {
            view.spotlightStepX = 0;
        }

        if (view.spotlightY == view.spotlightTargetY)
        {
            view.spotlightStepY = 0;
        }
    }
}

// swd32.exe 0x00416CC0: the ripples: each copies the screen and refracts it through its waves; settled ones are freed
void FieldView::DrawRipples()
{
    Game& owner = this->game;
    const std::span<u16> screen(
        owner.drawSurface,
        static_cast<std::size_t>(owner.painter.ScreenPitch()) * Display::Height
    );
    std::unique_ptr<ScriptObject>* link = &owner.scene.objects;

    while (*link != nullptr)
    {
        ScriptObject& ripple = **link;
        ripple.CopyScreen(screen);
        ripple.Refract(screen);

        if (this->replaying)
        {
            link = &ripple.next;
        }
        else if (ripple.Step())
        {
            *link = std::move(ripple.next);
        }
        else
        {
            link = &ripple.next;
        }
    }
}

}  // namespace swd3
