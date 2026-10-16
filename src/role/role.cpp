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
    ones(this->unk14);
    ones(this->unk18);
    ones(this->unk1C);
    ones(this->event);
    ones(this->unk20);
    ones(this->unk22);
    ones(this->id);
    ones(this->talkState);

    ones(this->unk28);
    ones(this->unk2A);
    ones(this->sound);
    ones(this->unk2E);
    ones(this->soundTimer);
    ones(this->labelColor);

    this->label.clear();
    this->attached = nullptr;
    swd3::FillUnused(this->act);
}

}  // namespace swd3
