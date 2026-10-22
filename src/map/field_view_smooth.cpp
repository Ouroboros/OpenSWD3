#include "map/field_view_internal.h"

namespace swd3
{

// The game frame about to be painted for the effect elements (the camera and the roles move continuously, see
// field_view_motion.cpp): the moves of the elements neither drawn nor moved in the frame before are dropped.
void FieldView::CapturePlaces()
{
    Game& owner = this->game;

    if (!owner.smoothMotion)
    {
        return;
    }

    const Field& field = owner.field;
    const u32 frame = owner.gameFrames;
    const bool continuous = this->placesValid && frame == this->placesFrame + 1 && owner.currentMap == this->placesMap;

    if (!continuous)
    {
        this->effectMotions.clear();
    }
    else
    {
        std::erase_if(
            this->effectMotions,
            [frame](const auto& item)
            {
                return item.second.seen + 1 < frame;
            }
        );
    }

    this->placesValid = true;
    this->placesFrame = frame;
    this->placesMap = owner.currentMap;
    this->cameraWidth = field.cameraRight - field.cameraLeft;
    this->cameraHeight = field.cameraBottom - field.cameraTop;
}

std::array<i32, 2> FieldView::EffectAt(u64 key, i32 x, i32 y, bool screen)
{
    if (!this->game.smoothMotion)
    {
        return {x, y};
    }

    const auto found = this->effectMotions.find(key);

    if (this->replaying)
    {
        if (found == this->effectMotions.end() || found->second.seen != this->placesFrame)
        {
            return {x, y};
        }

        return this->EffectPlace(found->second, this->paintColumn, this->paintRow);
    }

    if (found == this->effectMotions.end() || found->second.toX != x || found->second.toY != y ||
        found->second.screen != screen)
    {
        this->effectMotions[key] = EffectMotion{
            static_cast<double>(x),
            static_cast<double>(y),
            x,
            y,
            this->motionClock,
            1,
            screen,
            this->placesFrame,
        };

        return {x, y};
    }

    EffectMotion& effect = found->second;
    effect.seen = this->placesFrame;
    const std::array<double, 2> shown = this->EffectShown(effect);
    return {static_cast<i32>(std::round(shown[0])), static_cast<i32>(std::round(shown[1]))};
}

void FieldView::MoveEffect(u64 key, i32 fromX, i32 fromY, i32 toX, i32 toY, i32 frames, bool screen, i32 snap)
{
    if (!this->game.smoothMotion || this->replaying)
    {
        return;
    }

    std::array<double, 2> from = {static_cast<double>(fromX), static_cast<double>(fromY)};
    const auto found = this->effectMotions.find(key);

    if (found != this->effectMotions.end() && found->second.toX == fromX && found->second.toY == fromY &&
        found->second.screen == screen)
    {
        from = this->EffectShown(found->second);
    }

    if (std::abs(toX - from[0]) > snap || std::abs(toY - from[1]) > snap)
    {
        from = {static_cast<double>(toX), static_cast<double>(toY)};
    }

    const auto interval = std::max(static_cast<i32>(this->game.state.frameInterval), 1);
    this->effectMotions[key] = EffectMotion{
        from[0],
        from[1],
        toX,
        toY,
        this->motionClock,
        std::max(frames, 1) * interval,
        screen,
        this->placesFrame,
    };
}

std::array<double, 2> FieldView::EffectShown(const EffectMotion& effect) const
{
    const double progress =
        std::clamp(static_cast<double>(static_cast<i32>(this->motionClock - effect.start)) / effect.length, 0.0, 1.0);
    return {
        effect.fromX + (effect.toX - effect.fromX) * progress,
        effect.fromY + (effect.toY - effect.fromY) * progress,
    };
}

std::array<i32, 2> FieldView::EffectPlace(const EffectMotion& effect, int column, int row) const
{
    const int scale = this->paintScale;

    if (effect.screen)
    {
        return {CeilDivide(effect.fineX - column, scale), CeilDivide(effect.fineY - row, scale)};
    }

    return {
        FloorDivide(this->paintCameraX + column, scale) + CeilDivide(effect.fineX - this->paintCameraX - column, scale),
        FloorDivide(this->paintCameraY + row, scale) + CeilDivide(effect.fineY - this->paintCameraY - row, scale),
    };
}

std::span<Pixel> FieldView::Canvas() const
{
    Surface* canvas = this->game.mapCanvas.get();

    if (canvas == nullptr)
    {
        return {};
    }

    return {canvas->Pixels(), static_cast<std::size_t>(canvas->Width()) * static_cast<std::size_t>(canvas->Height())};
}

bool FieldView::FillsWindow() const
{
    const Field& field = this->game.field;
    return field.viewWidth != Display::Width || field.viewHeight != Display::Height;
}

int FieldView::ViewRows() const
{
    return (this->game.field.viewHeight + MapHeader::TileSize - 1) / MapHeader::TileSize;
}

std::size_t FieldView::ScreenAt() const
{
    const Field& field = this->game.field;
    return static_cast<std::size_t>(field.screenY) * static_cast<std::size_t>(field.viewWidth) +
           static_cast<std::size_t>(field.screenX);
}

void FieldView::CaptureLayer(std::vector<Pixel>& layer) const
{
    const std::span<Pixel> canvas = this->Canvas();
    layer.assign(canvas.begin(), canvas.end());
}

void FieldView::PaintBetween(Surface& out)
{
    Game& owner = this->game;
    // The map's canvas (the 640x480 screen unless the map fills the window).
    const int width = owner.field.viewWidth;
    const int height = owner.field.viewHeight;
    const auto canvasPixels = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const int scale = std::max(1, out.Width() / width);
    const std::span<Pixel> pixels(out.Pixels(), static_cast<std::size_t>(out.Width()) * out.Height());
    const bool layered =
        this->finalLayer.size() == canvasPixels && out.Width() == width * scale && out.Height() == height * scale;

    if (!layered)
    {
        return;
    }

    if (this->movie.active != 0 || !this->placesValid)
    {
        FieldView::ScaleLayer(this->finalLayer, scale, width, pixels);
        this->paintedValid = false;
        return;
    }

    GameState& state = owner.state;
    Field& field = owner.field;
    Scene& view = owner.scene;
    Painter& painter = owner.painter;

    // The places in 1/scale pixels where the continuous motion has the camera (shaken as the frame shown shook it)
    // and every role.
    const std::array<i32, 2> cameraFine = this->CameraFine(scale);
    const i32 fineCameraX = cameraFine[0] + this->shakeShift[0] * scale;
    const i32 fineCameraY = cameraFine[1] + this->shakeShift[1] * scale;
    std::array<i32, GameState::Roles> fineX = {};
    std::array<i32, GameState::Roles> fineY = {};
    // The pointer is drawn where the mouse is now (the frame read it where it was).
    int mouseX = 0;
    int mouseY = 0;
    owner.input.PeekMouse(mouseX, mouseY);
    this->placesKey.clear();
    this->placesKey.push_back(scale);
    this->placesKey.push_back(fineCameraX);
    this->placesKey.push_back(fineCameraY);
    this->placesKey.push_back(mouseX);
    this->placesKey.push_back(mouseY);

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        const std::array<i32, 2> place = this->RoleFine(index, scale);
        fineX[index] = place[0];
        fineY[index] = place[1];
        this->placesKey.push_back(fineX[index]);
        this->placesKey.push_back(fineY[index]);
    }

    // The effect elements the last game frame drew, placed like the roles (on the map) or on the screen.
    this->paintScale = scale;
    this->paintCameraX = fineCameraX;
    this->paintCameraY = fineCameraY;

    for (auto& [key, effect] : this->effectMotions)
    {
        if (effect.seen != this->placesFrame)
        {
            continue;
        }

        const std::array<double, 2> place = this->EffectShown(effect);
        effect.fineX = static_cast<i32>(std::round(place[0] * scale));
        effect.fineY = static_cast<i32>(std::round(place[1] * scale));
        this->placesKey.push_back(effect.fineX);
        this->placesKey.push_back(effect.fineY);
    }

    // Within a game frame the picture depends on these places alone (the rest is put back the same every time):
    // with every place as last painted, the last picture stands.
    if (this->paintedValid && this->paintedFrame == this->placesFrame && this->paintedKey == this->placesKey &&
        this->paintedTarget == out.Pixels())
    {
        return;
    }

    std::swap(this->paintedKey, this->placesKey);
    this->paintedFrame = this->placesFrame;
    this->paintedTarget = out.Pixels();
    this->paintedValid = true;

    // What the map part changes is kept and put back after each painting: the camera, the roles' places and the rest
    // (PaintKept).
    const i32 cameraLeft = field.cameraLeft;
    const i32 cameraTop = field.cameraTop;
    const i32 cameraRight = field.cameraRight;
    const i32 cameraBottom = field.cameraBottom;
    std::array<i32, GameState::Roles> roleX = {};
    std::array<i32, GameState::Roles> roleY = {};

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        roleX[index] = state.roles[index].x;
        roleY[index] = state.roles[index].y;
    }

    // The painter draws on screens the canvas's size meanwhile (the 640x480 screen again after).
    owner.SetDrawSize(width, height);
    const PaintKept kept(*this);

    auto putBack = [&kept]()
    {
        kept.PutBack();
    };

    // At a render scale N the screen is N x N logical screens woven together: the one of column a and row b of the
    // blocks shows a place p (in 1/N pixels) at the pixel ceil((p - a) / N), so it is the map painted at the camera
    // floor((camera + a) / N) with each role at that camera plus ceil((its place - camera - a) / N). Columns that
    // place the camera and every role alike (and rows alike) show the same screen, which is painted once. The
    // interface, the fade and what follows it lie on the pixels of the logical screen, so they are laid on each
    // screen before the weaving.
    const auto blocks = static_cast<std::size_t>(scale);
    std::array<std::vector<i32>, Game::MaxRenderScale> columnPlaces;
    std::array<std::vector<i32>, Game::MaxRenderScale> rowPlaces;
    std::array<std::size_t, Game::MaxRenderScale> columnShown = {};
    std::array<std::size_t, Game::MaxRenderScale> rowShown = {};

    auto placesOf = [this, &fineX, &fineY, scale](std::vector<i32>& places, i32 fineCamera, int offset, bool across)
    {
        const i32 cameraPlace = FloorDivide(fineCamera + offset, scale);
        places.resize(1 + GameState::Roles);
        places[0] = cameraPlace;

        for (std::size_t index = 0; index < GameState::Roles; ++index)
        {
            const i32 fine = across ? fineX[index] : fineY[index];
            places[1 + index] = cameraPlace + CeilDivide(fine - fineCamera - offset, scale);
        }

        for (const auto& [key, effect] : this->effectMotions)
        {
            if (effect.seen == this->placesFrame)
            {
                const std::array<i32, 2> spot = this->EffectPlace(effect, across ? offset : 0, across ? 0 : offset);
                places.push_back(across ? spot[0] : spot[1]);
            }
        }
    };

    for (std::size_t offset = 0; offset < blocks; ++offset)
    {
        placesOf(columnPlaces[offset], fineCameraX, static_cast<int>(offset), true);
        placesOf(rowPlaces[offset], fineCameraY, static_cast<int>(offset), false);
        columnShown[offset] = offset;
        rowShown[offset] = offset;

        for (std::size_t earlier = 0; earlier < offset; ++earlier)
        {
            if (columnShown[offset] == offset && columnPlaces[earlier] == columnPlaces[offset])
            {
                columnShown[offset] = earlier;
            }

            if (rowShown[offset] == offset && rowPlaces[earlier] == rowPlaces[offset])
            {
                rowShown[offset] = earlier;
            }
        }
    }

    if (scale > 1)
    {
        this->parts.resize(blocks * blocks);
    }

    auto spotlightMotion = this->effectMotions.find(EffectKey(EffectKind::Spotlight, u64{0}));

    if (spotlightMotion != this->effectMotions.end() && spotlightMotion->second.seen != this->placesFrame)
    {
        spotlightMotion = this->effectMotions.end();
    }

    this->replaying = true;

    for (std::size_t row = 0; row < blocks; ++row)
    {
        for (std::size_t column = 0; column < blocks; ++column)
        {
            if (rowShown[row] != row || columnShown[column] != column)
            {
                continue;
            }

            Surface& part = scale > 1 ? this->parts[row * blocks + column] : out;
            this->paintColumn = static_cast<int>(column);
            this->paintRow = static_cast<int>(row);

            if (part.Width() != width || part.Height() != height)
            {
                part.Resize(width, height);
            }

            field.cameraLeft = columnPlaces[column][0];
            field.cameraTop = rowPlaces[row][0];
            field.cameraRight = field.cameraLeft + this->cameraWidth;
            field.cameraBottom = field.cameraTop + this->cameraHeight;

            for (std::size_t index = 0; index < GameState::Roles; ++index)
            {
                state.roles[index].x = columnPlaces[column][1 + index];
                state.roles[index].y = rowPlaces[row][1 + index];
            }

            if (spotlightMotion != this->effectMotions.end())
            {
                const std::array<i32, 2> light =
                    this->EffectPlace(spotlightMotion->second, static_cast<int>(column), static_cast<int>(row));
                view.spotlightX = light[0];
                view.spotlightY = light[1];
            }

            owner.drawSurface = painter.LockSurface(part);
            painter.SetTarget(owner.drawSurface);
            painter.UnlockSurface(part);
            painter.SetClipClamped(0, 0, width, height);

            if ((view.screenFlags & Scene::SpotlightClip) != 0)
            {
                this->ClipToSpotlight();
            }

            this->PaintWorld();

            // The interface of the last game frame drawn again over this map, as the frame drew it: the lines and talk
            // windows, the role heads where they glide, the gold, the pointer, the fade, then what it drew after the
            // fade. It lies on the 640x480 screen in the canvas.
            const std::span<Pixel> screen(part.Pixels(), canvasPixels);

            auto onScreen = [&]()
            {
                owner.drawSurface = screen.data() + this->ScreenAt();
                painter.SetTarget(owner.drawSurface);
                painter.SetClipClamped(0, 0, Display::Width, Display::Height);
            };

            onScreen();
            this->BlendLines(screen.subspan(this->ScreenAt()));
            owner.talk.DrawAgain();
            onScreen();
            this->DrawRoleHeads();
            putBack();
            onScreen();
            this->DrawGold();

            if (owner.menuButtonShown)
            {
                owner.DrawMenuButton(true);
            }

            putBack();

            if (this->pointerShown)
            {
                const ActState& act = view.pointerRole.act;
                const std::array<i32, 2> sway = this->EffectAt(
                    EffectKey(EffectKind::Pointer, u64{0}),
                    this->pointerSway[0],
                    this->pointerSway[1],
                    true
                );
                painter.SetTarget(screen.data());
                painter.SetClipClamped(0, 0, width, height);
                owner.DrawPointer(mouseX + field.screenX + sway[0] + act.x, mouseY + field.screenY + sway[1] + act.y);
                putBack();
            }

            if (this->fadeTinted)
            {
                painter.TintPixelsFast(screen, this->fadeLevels.red, this->fadeLevels.green, this->fadeLevels.blue);
            }

            onScreen();
            this->DrawFinalAgain();
            putBack();
        }
    }

    this->replaying = false;
    field.cameraLeft = cameraLeft;
    field.cameraTop = cameraTop;
    field.cameraRight = cameraRight;
    field.cameraBottom = cameraBottom;

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        state.roles[index].x = roleX[index];
        state.roles[index].y = roleY[index];
    }

    owner.SetDrawSize(Display::Width, Display::Height);

    if (scale > 1)
    {
        std::array<const Surface*, Game::MaxRenderScale * Game::MaxRenderScale> shown = {};

        for (std::size_t row = 0; row < blocks; ++row)
        {
            for (std::size_t column = 0; column < blocks; ++column)
            {
                shown[row * blocks + column] = &this->parts[rowShown[row] * blocks + columnShown[column]];
            }
        }

        Weave(std::span<const Surface* const>(shown.data(), blocks * blocks), scale, pixels);
    }
}

void FieldView::CopySpans(
    const std::vector<Span>& spans,
    const std::vector<Pixel>& from,
    int scale,
    int width,
    std::span<Pixel> to
)
{
    if (scale == 1)
    {
        for (const Span& run : spans)
        {
            std::copy_n(from.begin() + run.start, run.length, to.begin() + run.start);
        }

        return;
    }

    const auto columns = static_cast<u32>(width);
    const auto blocks = static_cast<std::size_t>(scale);
    const std::size_t pitch = columns * blocks;

    for (const Span& run : spans)
    {
        const u32 end = run.start + run.length;

        for (u32 at = run.start; at < end;)
        {
            const u32 y = at / columns;
            const u32 piece = std::min(end, (y + 1) * columns) - at;
            Pixel* first = to.data() + y * blocks * pitch + (at % columns) * blocks;

            for (u32 pixel = 0; pixel < piece; ++pixel)
            {
                std::fill_n(first + pixel * blocks, blocks, from[at + pixel]);
            }

            for (std::size_t row = 1; row < blocks; ++row)
            {
                std::copy_n(first, piece * blocks, first + row * pitch);
            }

            at += piece;
        }
    }
}

void FieldView::ScaleLayer(const std::vector<Pixel>& from, int scale, int width, std::span<Pixel> to)
{
    FieldView::CopySpans({Span{0, static_cast<u32>(from.size())}}, from, scale, width, to);
}

}  // namespace swd3
