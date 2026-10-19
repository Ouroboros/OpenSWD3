#include "battle/smooth/smooth_draw.h"

#include <optional>

#include "battle/battle_services.h"
#include "draw/paint_record.h"
#include "draw/painter.h"
#include "res/tsw_archive.h"

namespace swd3::smooth
{

const Picture* PictureRef::Get(TswArchive& archive) const
{
    return this->shown ? archive.GetPicture(this->id, this->frame) : nullptr;
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

        painter.DrawPicture(step.x, step.y, picture->width, picture->height, step.mode);
    }
}

}  // namespace swd3::smooth
