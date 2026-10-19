#include "battle/smooth/smooth_draw.h"

#include <optional>
#include <utility>

#include "battle/battle_services.h"
#include "core/log.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "res/tsw_archive.h"

namespace swd3::smooth
{

const Picture* PictureRef::Get(TswArchive& archive) const
{
    return this->shown ? archive.GetPicture(this->id, this->frame) : nullptr;
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

        std::optional<PaintRecord::Motion> moving;

        if (step.motion[0] != 0 || step.motion[1] != 0)
        {
            moving.emplace(step.motion);
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

void Scene::Add(Draw draw)
{
    this->draws.push_back(std::move(draw));
}

void Scene::Add(PictureSteps steps)
{
    if (steps.empty())
    {
        return;
    }

    this->draws.emplace_back(
        [steps = std::move(steps)](BattleServices& services)
        {
            DrawSteps(services, steps);
        }
    );
}

void Scene::Add(const PictureStep& step)
{
    this->Add(PictureSteps{step});
}

void Scene::DrawAll(BattleServices& services) const
{
    for (const Draw& draw : this->draws)
    {
        draw(services);
    }
}

ScenePainter::ScenePainter(BattleServices& services, Scene& scene)
    : services(services),
      scene(scene)
{
}

ScenePainter::~ScenePainter()
{
    if (this->options == DrawOptions{})
    {
        return;
    }

    this->scene.Add(
        [options = this->options](BattleServices& drawing)
        {
            drawing.painter.Options() = options;
        }
    );
}

// The source is the painter's own: the parts deciding a frame set it in the original's order, so what it holds when
// a draw is decided is what the original draws then.
void ScenePainter::SetSource(const void* data)
{
    this->services.painter.SetSource(data);
}

DrawOptions& ScenePainter::Options()
{
    return this->options;
}

void ScenePainter::SetWave(int mode)
{
    this->scene.Add(
        [mode](BattleServices& drawing)
        {
            drawing.painter.SetWave(mode);
        }
    );
}

void ScenePainter::SetClipClamped(int left, int top, int right, int bottom)
{
    this->scene.Add(
        [left, top, right, bottom](BattleServices& drawing)
        {
            drawing.painter.SetClipClamped(left, top, right, bottom);
        }
    );
}

void ScenePainter::DrawPicture(int x, int y, int width, int height, u32 mode, const DrawArgument& argument)
{
    TswArchive& archive = this->services.archive;
    const DrawOptions options = this->options;
    this->options = DrawOptions{};
    const void* data = this->services.painter.Source();
    PictureRef source;
    source.shown = data != nullptr && archive.LocateData(data, source.id, source.frame) != nullptr;
    // The argument of a draw: the palette of a cached picture (PaletteArgument), a colour pair, or none.
    PictureRef palette;
    palette.shown =
        argument.alphaMap != nullptr && archive.LocatePalette(argument.alphaMap, palette.id, palette.frame) != nullptr;
    const u32 colorPair = argument.colorPair;

    if ((data != nullptr && !source.shown) || (argument.alphaMap != nullptr && !palette.shown))
    {
        LogWarning(u8"smooth battle: a draw of a picture outside the cache or with an alpha map is not kept");
    }

    this->scene.Add(
        [source, options, x, y, width, height, mode, palette, colorPair](BattleServices& drawing)
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

            painter.DrawPicture(x, y, width, height, mode, drawn);
        }
    );
}

// The picture the painter holds now as a cached reference (none outside the cache, warned).
PictureRef ScenePainter::Source() const
{
    const void* data = this->services.painter.Source();
    PictureRef source;
    source.shown = data != nullptr && this->services.archive.LocateData(data, source.id, source.frame) != nullptr;

    if (data != nullptr && !source.shown)
    {
        LogWarning(u8"smooth battle: a draw of a picture outside the cache is not kept");
    }

    return source;
}

void ScenePainter::SetZoom(int anchorX, int anchorY, int zoomX, int zoomY)
{
    this->scene.Add(
        [anchorX, anchorY, zoomX, zoomY](BattleServices& drawing)
        {
            drawing.painter.SetZoom(anchorX, anchorY, zoomX, zoomY);
        }
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
        }
    );
}

void ScenePainter::DrawZoomedMirrored(int x, int y, int width, int height)
{
    this->DrawZoomed(x, y, width, height, true);
}

}  // namespace swd3::smooth
