#include "role/role.h"

#include <type_traits>

namespace swd3
{

// A role memset to 0xFF in the original (every field all ones; its buffer empty).
void Role::FillUnused()
{
    auto ones = [](auto& field)
    {
        using Field = std::remove_cvref_t<decltype(field)>;
        field = static_cast<Field>(-1);
    };

    ones(this->unk00);
    ones(this->x);
    ones(this->y);
    ones(this->layer);
    ones(this->flags);
    ones(this->unk14);
    ones(this->unk18);
    ones(this->unk1C);
    ones(this->event);
    ones(this->unk20);
    ones(this->unk22);
    ones(this->id);
    ones(this->unk26);

    for (i32& value : this->unk28)
    {
        ones(value);
    }

    this->unk38.clear();
    ones(this->unk3C);
    swd3::FillUnused(this->act);
}

}  // namespace swd3
