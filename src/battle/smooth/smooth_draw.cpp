#include "battle/smooth/smooth_draw.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include "battle/battle_services.h"
#include "core/log.h"
#include "draw/draw_placer.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "game/scene.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "res/tsw_archive.h"
#include "ui/ui_draw.h"

namespace swd3::smooth
{

const Picture* PictureRef::Get(TswArchive& archive) const
{
    if (!this->shown)
    {
        return nullptr;
    }

    if (this->owned != nullptr)
    {
        return this->owned.get();
    }

    return this->kept ? archive.FindKept(this->id, this->frame) : archive.Peek(this->id, this->frame);
}

PictureRef RefOf(TswArchive& archive, const Picture* picture)
{
    PictureRef reference;
    reference.shown = picture != nullptr && archive.Locate(picture, reference.id, reference.frame);
    return reference;
}

void DrawSteps(BattleServices& services, const PictureSteps& steps)
{
    Painter& painter = services.painter;

    for (const PictureStep& step : steps)
    {
        const Picture* picture = step.picture.Get(services.archive);

        if (picture == nullptr)
        {
            continue;
        }

        std::optional<DrawPlacer::Holding> holding;

        if (step.held)
        {
            holding.emplace(step.stand[0], step.stand[1], step.who);
        }

        painter.SetSource(picture->data.data());

        if (step.tinted)
        {
            DrawOptions& options = painter.Options();
            options.tintRed = step.tintRed;
            options.tintGreen = step.tintGreen;
            options.tintBlue = step.tintBlue;
        }

        if (step.blended)
        {
            painter.Options().alpha = step.alpha;
        }

        painter.DrawPicture(
            step.x,
            step.y,
            picture->width,
            picture->height,
            step.mode,
            step.palette ? PaletteArgument(*picture) : DrawArgument{}
        );
    }
}

void Scene::Add(Draw draw, Where where)
{
    this->steps.push_back(Step{std::move(draw), where.file_name(), where.line(), this->stillDepth > 0});
}

void Scene::Add(PictureSteps steps, Where where)
{
    if (steps.empty())
    {
        return;
    }

    this->Add(
        [steps = std::move(steps)](BattleServices& services)
        {
            DrawSteps(services, steps);
        },
        where
    );
}

void Scene::Add(const PictureStep& step, Where where)
{
    this->Add(PictureSteps{step}, where);
}

void Scene::Add(Scene&& scene)
{
    if (this->steps.empty())
    {
        this->steps = std::move(scene.steps);
    }
    else
    {
        this->steps.insert(
            this->steps.end(),
            std::make_move_iterator(scene.steps.begin()),
            std::make_move_iterator(scene.steps.end())
        );
    }

    scene.steps.clear();
}

void Scene::DrawAll(BattleServices& services) const
{
    for (const Step& step : this->steps)
    {
        step.draw(services);
    }
}

void Scene::Show(BattleServices& services)
{
    if (services.frame != nullptr && services.frame != this)
    {
        services.frame->Add(std::move(*this));
        return;
    }

    this->DrawAll(services);
    this->steps.clear();
}

bool Scene::Empty() const
{
    return this->steps.empty();
}

Scene::Still::Still(Scene& scene)
    : scene(scene)
{
    ++this->scene.stillDepth;
}

Scene::Still::~Still()
{
    --this->scene.stillDepth;
}

ScenePainter::ScenePainter(BattleServices& services, Scene& scene, Scene::Where where)
    : services(services),
      scene(scene),
      where(where)
{
}

// A colour of the screen's format: it draws nothing.
u32 ScenePainter::MakeColor(u32 red, u32 green, u32 blue) const
{
    return this->services.painter.MakeColor(red, green, blue);
}

// The source is the painter's own: the parts deciding a frame set it in the original's order, so what it holds when
// a draw is decided is what the original draws then.
void ScenePainter::SetSource(const void* data)
{
    this->services.painter.SetSource(data);
}

DrawOptions& ScenePainter::Options()
{
    return this->services.painter.Options();
}

void ScenePainter::SetWave(int mode)
{
    this->services.painter.SetWave(mode);
    this->scene.Add(
        [mode](BattleServices& drawing)
        {
            drawing.painter.SetWave(mode);
        },
        this->where
    );
}

void ScenePainter::SetClipClamped(int left, int top, int right, int bottom)
{
    this->services.painter.SetClipClamped(left, top, right, bottom);
    this->scene.Add(
        [left, top, right, bottom](BattleServices& drawing)
        {
            drawing.painter.SetClipClamped(left, top, right, bottom);
        },
        this->where
    );
}

int ScenePainter::DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument)
{
    this->Draw(x, y, width, height, mode, argument, false);
    return 0;
}

void ScenePainter::DrawOutline(int x, int y, int width, int height, u32 mode, const DrawArgument& argument)
{
    this->Draw(x, y, width, height, mode, argument, true);
}

void ScenePainter::Draw(int x, int y, int width, int height, u32 mode, const DrawArgument& argument, bool outline)
{
    // The options are the painter's own, set in the original's order; a draw takes them and clears them, as it does.
    DrawOptions& live = this->services.painter.Options();
    const DrawOptions options = live;
    live = DrawOptions{};
    const PictureRef source = this->Source();
    // The argument of a draw: the palette of a cached or kept picture (PaletteArgument), a colour pair, or none.
    PictureRef palette;
    palette.shown =
        argument.alphaMap != nullptr &&
        this->services.archive.LocatePalette(argument.alphaMap, palette.id, palette.frame, palette.kept) != nullptr;
    const u32 colorPair = argument.colorPair;

    if (argument.alphaMap != nullptr && !palette.shown && source.owned != nullptr && !source.owned->palette.empty() &&
        argument.alphaMap == PaletteArgument(*this->owner).alphaMap)
    {
        palette = source;
    }
    else if (argument.alphaMap != nullptr && !palette.shown)
    {
        LogWarning(u8"smooth battle: a draw with an alpha map outside the cache is not kept");
    }

    this->scene.Add(
        [source, options, x, y, width, height, mode, palette, colorPair, outline](BattleServices& drawing)
        {
            Painter& painter = drawing.painter;
            const Picture* picture = source.Get(drawing.archive);
            painter.SetSource(picture != nullptr ? picture->data.data() : nullptr);
            painter.Options() = options;
            DrawArgument drawn{.colorPair = colorPair};

            if (const Picture* owner = palette.Get(drawing.archive); owner != nullptr)
            {
                drawn = PaletteArgument(*owner);
                drawn.colorPair = colorPair;
            }

            if (outline)
            {
                painter.DrawOutline(x, y, width, height, mode, drawn);
            }
            else
            {
                painter.DrawPicture(x, y, width, height, mode, drawn);
            }
        },
        this->where
    );
}

// The picture the painter holds now as a reference of the cache or the kept pictures, or a copy of the picture its
// owner set (SetSource(const Picture&)); none otherwise, warned.
PictureRef ScenePainter::Source()
{
    const void* data = this->services.painter.Source();
    PictureRef source;
    source.shown =
        data != nullptr && this->services.archive.LocateData(data, source.id, source.frame, source.kept) != nullptr;

    if (data != nullptr && !source.shown && this->owner != nullptr && this->owner->data.data() == data)
    {
        if (this->ownedCopy == nullptr || this->ownedCopy->data != this->owner->data)
        {
            this->ownedCopy = std::make_shared<const Picture>(*this->owner);
        }

        source.owned = this->ownedCopy;
        source.shown = true;
    }

    if (data != nullptr && !source.shown && data == this->ownerPixels)
    {
        auto copy = std::make_shared<Picture>();
        const auto* bytes = static_cast<const u8*>(data);
        copy->data.assign(bytes, bytes + this->ownerPixelCount * sizeof(u16));
        copy->width = static_cast<u16>(this->ownerWidth);
        copy->height = static_cast<u16>(this->ownerHeight);
        source.owned = std::move(copy);
        source.shown = true;
    }

    if (data != nullptr && !source.shown)
    {
        LogWarning(u8"smooth battle: a draw of a picture outside the cache is not kept");
    }

    return source;
}

void ScenePainter::SetSource(const Picture& picture)
{
    this->owner = &picture;
    this->services.painter.SetSource(picture.data.data());
}

void ScenePainter::SetSource(const u16* pixels, int width, int height)
{
    this->ownerPixels = pixels;
    this->ownerWidth = width;
    this->ownerHeight = height;
    this->ownerPixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    this->services.painter.SetSource(pixels);
}

int ScenePainter::ScreenPitch() const
{
    return this->services.painter.ScreenPitch();
}

void ScenePainter::SetZoom(int anchorX, int anchorY, int zoomX, int zoomY)
{
    this->services.painter.SetZoom(anchorX, anchorY, zoomX, zoomY);
    this->scene.Add(
        [anchorX, anchorY, zoomX, zoomY](BattleServices& drawing)
        {
            drawing.painter.SetZoom(anchorX, anchorY, zoomX, zoomY);
        },
        this->where
    );
}

void ScenePainter::DrawZoomed(int x, int y, int width, int height, bool mirrored)
{
    const PictureRef source = this->Source();

    this->scene.Add(
        [source, x, y, width, height, mirrored](BattleServices& drawing)
        {
            Painter& painter = drawing.painter;
            const Picture* picture = source.Get(drawing.archive);
            painter.SetSource(picture != nullptr ? picture->data.data() : nullptr);

            if (mirrored)
            {
                painter.DrawZoomedMirrored(x, y, width, height);
            }
            else
            {
                painter.DrawZoomed(x, y, width, height);
            }
        },
        this->where
    );
}

void ScenePainter::DrawZoomedMirrored(int x, int y, int width, int height)
{
    this->DrawZoomed(x, y, width, height, true);
}

// The shape is kept without its screen copy, the copy taken where the scene draws.
void ScenePainter::Refract(const ScriptObject& ripple)
{
    auto shape = std::make_shared<ScriptObject>();
    shape->pitch = ripple.pitch;
    shape->lines = ripple.lines;
    shape->left = ripple.left;
    shape->top = ripple.top;
    shape->gridWidth = ripple.gridWidth;
    shape->gridHeight = ripple.gridHeight;
    shape->damping = ripple.damping;
    shape->page = ripple.page;
    shape->heights = ripple.heights;

    this->scene.Add(
        [shape](BattleServices& drawing)
        {
            const std::span<u16> screen(
                drawing.screen,
                static_cast<std::size_t>(drawing.painter.ScreenPitch()) * Display::Height
            );
            shape->CopyScreen(screen);
            shape->Refract(screen);
        },
        this->where
    );
}

std::size_t ScenePainter::ScreenPixels(std::span<u16> pixels) const
{
    if (pixels.data() != this->services.screen)
    {
        LogWarning(u8"smooth battle: a colour shift of pixels outside the screen is not kept");
        return 0;
    }

    return pixels.size();
}

void ScenePainter::TintPixels(std::span<u16> pixels, int red, int green, int blue)
{
    this->scene.Add(
        [count = this->ScreenPixels(pixels), red, green, blue](BattleServices& drawing)
        {
            drawing.painter.TintPixels(std::span<u16>(drawing.screen, count), red, green, blue);
        },
        this->where
    );
}

void ScenePainter::ShiftRed(std::span<u16> pixels, int amount)
{
    this->scene.Add(
        [count = this->ScreenPixels(pixels), amount](BattleServices& drawing)
        {
            drawing.painter.ShiftRed(std::span<u16>(drawing.screen, count), amount);
        },
        this->where
    );
}

void ScenePainter::ShiftGreenPairs(std::span<u16> pixels, int amount)
{
    this->scene.Add(
        [count = this->ScreenPixels(pixels), amount](BattleServices& drawing)
        {
            drawing.painter.ShiftGreenPairs(std::span<u16>(drawing.screen, count), amount);
        },
        this->where
    );
}

void ScenePainter::ShiftBluePairs(std::span<u16> pixels, int amount)
{
    this->scene.Add(
        [count = this->ScreenPixels(pixels), amount](BattleServices& drawing)
        {
            drawing.painter.ShiftBluePairs(std::span<u16>(drawing.screen, count), amount);
        },
        this->where
    );
}

void ScenePainter::CopyScreen(const Surface& from, Surface&)
{
    auto pixels = std::make_shared<std::vector<u16>>();
    const int width = std::min(from.Width(), Display::Width);
    const int height = std::min(from.Height(), Display::Height);
    pixels->reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    for (int row = 0; row < height; ++row)
    {
        pixels->insert(pixels->end(), from.Row(row), from.Row(row) + width);
    }

    this->scene.Add(
        [pixels, width, height](BattleServices& drawing)
        {
            const int pitch = drawing.painter.ScreenPitch();

            for (int row = 0; row < height; ++row)
            {
                std::copy_n(
                    pixels->data() + static_cast<std::ptrdiff_t>(row) * width,
                    width,
                    drawing.screen + static_cast<std::ptrdiff_t>(row) * pitch
                );
            }
        },
        this->where
    );
}

SceneFont::SceneFont(TextFont& font, Scene& scene, Scene::Where where)
    : font(font),
      scene(scene),
      where(where)
{
}

void SceneFont::SetAdvance(int advance)
{
    this->font.SetAdvance(advance);
}

void SceneFont::SetBackground(Pixel color)
{
    this->font.SetBackground(color);
}

void SceneFont::SetShadow(Pixel color)
{
    this->font.SetShadow(color);
}

void SceneFont::DrawText(Pixel*, int x, int y, std::string_view text, Pixel color, int style)
{
    this->scene.Add(
        [font = &this->font, state = this->font.SaveDrawState(), x, y, text = std::string(text), color, style](
            BattleServices& drawing
        )
        {
            const TextFont::DrawState live = font->SaveDrawState();
            font->RestoreDrawState(state);
            font->DrawText(drawing.screen, x, y, text, color, style);
            font->RestoreDrawState(live);
        },
        this->where
    );
}

void SceneFont::DrawText(Pixel*, int x, int y, std::u8string_view text, Pixel color, int style)
{
    this->scene.Add(
        [font = &this->font, state = this->font.SaveDrawState(), x, y, text = std::u8string(text), color, style](
            BattleServices& drawing
        )
        {
            const TextFont::DrawState live = font->SaveDrawState();
            font->RestoreDrawState(state);
            font->DrawText(drawing.screen, x, y, text, color, style);
            font->RestoreDrawState(live);
        },
        this->where
    );
}

void FillScreen(u16 color, std::size_t count, Scene& scene, Scene::Where where)
{
    scene.Add(
        [color, count](BattleServices& drawing)
        {
            std::fill_n(drawing.screen, count, color);
        },
        where
    );
}

void Box(
    Painter&,
    int x,
    int y,
    int width,
    int height,
    int red,
    int green,
    int blue,
    FillMode mode,
    Scene& scene,
    Scene::Where where
)
{
    scene.Add(
        [x, y, width, height, red, green, blue, mode](BattleServices& drawing)
        {
            FillBox(drawing.painter, x, y, width, height, red, green, blue, mode);
        },
        where
    );
}

void WindowFrame(
    Painter&,
    TswArchive&,
    int id,
    int left,
    int top,
    int right,
    int bottom,
    int alpha,
    int border,
    Scene& scene,
    Scene::Where where
)
{
    scene.Add(
        [id, left, top, right, bottom, alpha, border](BattleServices& drawing)
        {
            DrawWindowFrame(drawing.painter, drawing.archive, id, left, top, right, bottom, alpha, border);
        },
        where
    );
}

}  // namespace swd3::smooth
