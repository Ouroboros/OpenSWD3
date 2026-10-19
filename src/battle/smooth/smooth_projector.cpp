#include "battle/smooth/smooth_projector.h"

#include <iterator>
#include <memory>
#include <span>

#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "pic/projector.h"

namespace swd3::smooth
{

namespace
{

constexpr std::size_t kUpperLeft = 0;  // a particle is two by two pixels
constexpr std::size_t kUpperRight = 1;
constexpr std::size_t kLowerLeft = 2;
constexpr std::size_t kLowerRight = 3;

void Mix(Painter& painter, u16 source, u16& target)
{
    const std::array<u16, 1> added = {source};
    painter.MixPixels(added, std::span<u16>(&target, 1));
}

}  // namespace

ExplosionFrame::ExplosionFrame(Projector& projector)
    : projector(projector)
{
}

// What Projector::DrawExplosion decides of the frame (see there): the burst, the particles moved and removed; the rest
// of the picture and the live particles into `view`.
int ExplosionFrame::Advance(Painter& painter, crt::Random& random, ParticleEffect& effect, ExplosionView& view)
{
    const u16 fill = painter.Rle().fill16;
    const u16 skip = painter.Rle().skip16;

    if (effect.started == 0)
    {
        random.Seed(static_cast<u32>(crt::Time()));
        DrawOptions& options = painter.Options();
        options.tintRed = effect.tintRed;
        options.tintGreen = effect.tintGreen;
        options.tintBlue = effect.tintBlue;
        const int pixelCount = effect.height * effect.width;
        effect.pixelCount = pixelCount;
        effect.particles = 0;
        effect.started = 1;
        effect.opaque = 0;

        for (int i = 0; i < pixelCount; ++i)
        {
            const u16 pixel = effect.pixels[static_cast<std::size_t>(i)];

            if (pixel != fill && pixel != skip)
            {
                ++effect.opaque;
            }
        }

        effect.slowLimit = effect.opaque / Projector::SlowShare;
        effect.burstStep = pixelCount / effect.bursts;
    }

    if (effect.bursts > 0 && effect.particles < effect.opaque / effect.divisor)
    {
        this->projector.ParticlePut(effect, effect.pixelCount / effect.bursts, painter, random);
    }

    if (effect.particles <= 0 && effect.bursts == 0)
    {
        return 1;
    }

    view.fill = fill;
    view.skip = skip;
    view.mixed = (effect.flags & ParticleEffect::Mixed) != 0;

    if (effect.bursts >= 1)
    {
        view.body = true;
        view.x = effect.x;
        view.y = effect.y;
        view.width = effect.width;
        view.height = effect.height;
        view.mirrored = (effect.flags & ParticleEffect::Mirrored) != 0;
        view.pixels.assign(effect.pixels.begin(), effect.pixels.end());
    }

    for (auto particle = effect.list.begin(); particle != effect.list.end();)
    {
        if (effect.particles <= effect.slowLimit)
        {
            const int speed = effect.speed;
            particle->speed = random.Next() % speed + speed;
        }

        for (int step = 0; step < particle->speed; ++step)
        {
            LineStep(particle->walker);
        }

        particle->life -= particle->speed;
        const int px = particle->walker.x;
        const int py = particle->walker.y;
        bool remove = particle->life <= 0;

        if (!remove && px >= effect.targetLeft && py >= effect.targetTop &&
            px < effect.targetWidth + effect.targetLeft && py < effect.targetHeight + effect.targetTop)
        {
            remove = true;
        }

        if (!remove)
        {
            ExplosionView::Block& block = view.blocks.emplace_back();
            block.x = px;
            block.y = py;
            std::copy(std::begin(particle->pixels), std::end(particle->pixels), block.pixels.begin());
            ++particle;
            continue;
        }

        const bool first = particle == effect.list.begin();
        const bool last = std::next(particle) == effect.list.end();

        if (first && last)
        {
            effect.list.clear();
            effect.tail = effect.list.end();
            effect.particles = 0;
            return 0;
        }

        if (!first && last)
        {
            effect.tail = std::prev(particle);
        }
        else if (effect.tail == particle)
        {
            effect.tail = effect.list.end();
        }

        particle = effect.list.erase(particle);
        --effect.particles;
        --effect.opaque;
    }

    return 0;
}

// What Projector::DrawExplosion draws of the frame (see there).
void ExplosionFrame::Draw(u16* screen, Painter& painter, const ExplosionView& view) const
{
    const Projector& owner = this->projector;
    const u16 fill = view.fill;

    if (view.body)
    {
        const int x = view.x;
        int y = view.y;
        std::size_t row = 0;

        for (int line = 0; line < view.height - 1; ++line, ++y)
        {
            for (int column = 0; column < view.width; ++column)
            {
                const int sourceColumn = view.mirrored ? view.width - column : column;
                const int clipX = view.mirrored ? sourceColumn + x : column + x;

                if (clipX < 0 || clipX >= owner.screenWidth || y < 0 || y >= owner.screenHeight)
                {
                    continue;
                }

                const u16 pixel = view.pixels[row + static_cast<std::size_t>(sourceColumn)];

                if (pixel == fill || pixel == view.skip)
                {
                    continue;
                }

                u16& target = screen[owner.screenRows[static_cast<std::size_t>(y)] + column + x];

                if (view.mixed)
                {
                    Mix(painter, pixel, target);
                }
                else
                {
                    target = pixel;
                }
            }

            row += static_cast<std::size_t>(view.width);
        }
    }

    for (const ExplosionView::Block& block : view.blocks)
    {
        const int px = block.x;
        const int py = block.y;
        const std::array<u16, 4>& pixels = block.pixels;

        if (px + 1 >= 0 && px + 1 < owner.screenWidth && py >= 0 && py < owner.screenHeight)
        {
            u16* line = screen + owner.screenRows[static_cast<std::size_t>(py)] + px;

            if (view.mixed)
            {
                if (pixels[kUpperLeft] != fill)
                {
                    Mix(painter, pixels[kUpperLeft], line[0]);
                }

                if (pixels[kUpperRight] != fill)
                {
                    Mix(painter, pixels[kUpperRight], line[1]);
                }

                line[1] = pixels[kUpperRight];
            }
            else
            {
                if (pixels[kUpperLeft] != fill)
                {
                    line[0] = pixels[kUpperLeft];
                }

                if (pixels[kUpperRight] != fill)
                {
                    line[1] = pixels[kUpperRight];
                }
            }
        }

        if (px + 1 >= 0 && px + 1 < owner.screenWidth && py + 1 >= 0 && py + 1 < owner.screenHeight)
        {
            u16* line = screen + owner.screenRows[static_cast<std::size_t>(py + 1)] + px;

            if (view.mixed)
            {
                if (pixels[kLowerLeft] != fill)
                {
                    Mix(painter, pixels[kLowerLeft], line[0]);
                }

                if (pixels[kLowerRight] != fill)
                {
                    Mix(painter, pixels[kLowerRight], line[1]);
                }
            }
            else
            {
                if (pixels[kLowerLeft] != fill)
                {
                    line[0] = pixels[kLowerLeft];
                }

                if (pixels[kLowerRight] != fill)
                {
                    line[1] = pixels[kLowerRight];
                }
            }
        }
    }
}

int ExplosionFrame::Explosion(u16*, Painter& painter, crt::Random& random, ParticleEffect& effect, Scene& scene)
{
    ExplosionView view;
    const int result = this->Advance(painter, random, effect, view);

    if (view.body || !view.blocks.empty())
    {
        scene.Add(
            [view = std::move(view)](BattleServices& drawing)
            {
                ExplosionFrame(drawing.state.projector).Draw(drawing.screen, drawing.painter, view);
            }
        );
    }

    return result;
}

void ExplosionFrame::Scaled(u16*, Painter& painter, const ProjectedPicture& picture, Scene& scene)
{
    DrawOptions& options = painter.Options();
    options.tintRed = picture.tintRed;
    options.tintGreen = picture.tintGreen;
    options.tintBlue = picture.tintBlue;
    auto pixels = std::make_shared<std::vector<u16>>(picture.pixels.begin(), picture.pixels.end());
    scene.Add(
        [picture, pixels](BattleServices& drawing)
        {
            ProjectedPicture drawn = picture;
            drawn.pixels = *pixels;
            drawing.state.projector.DrawScaled(drawing.screen, drawing.painter, drawn);
        }
    );
}

}  // namespace swd3::smooth
