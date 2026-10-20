#include "role/role.h"

#include <type_traits>

namespace swd3
{

// A role memset to 0xFF in the original (every field all ones; its links and buffer empty).
void Role::FillUnused()
{
    auto ones = [](auto& field)
    {
        using Field = std::remove_cvref_t<decltype(field)>;
        field = static_cast<Field>(-1);
    };

    this->next = nullptr;
    ones(this->x);
    ones(this->y);
    this->cell = nullptr;
    ones(this->flags);
    ones(this->eventOffset);
    ones(this->pathStep);
    ones(this->pathNumber);
    ones(this->event);
    ones(this->eventCursor);
    ones(this->pathWait);
    ones(this->id);
    ones(this->talkState);

    ones(this->drawOffsetX);
    ones(this->drawOffsetY);
    ones(this->sound);
    ones(this->unk2E);
    ones(this->soundTimer);
    ones(this->labelColor);

    this->label.reset();
    this->attached = nullptr;
    swd3::FillUnused(this->act);
}

}  // namespace swd3
