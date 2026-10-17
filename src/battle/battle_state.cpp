#include "battle/battle_state.h"

#include <algorithm>

#include "crt/msvc.h"

namespace swd3
{

namespace
{

constexpr int kSecondKind = 2;
constexpr u16 kAllOnes16 = 0xFFFF;  // a free record has every byte 0xFF

}  // namespace

// swd32.exe 0x004694E0: takes the first non-zero entry of the 4 x 10 queue unless one is pending
void BattleState::TakePendingAction()
{
    if (this->pendingAction != 0)
    {
        return;
    }

    for (int row = 0; row < BattleState::QueueRows; ++row)
    {
        for (int column = 0; column < BattleState::QueueColumns; ++column)
        {
            i16& entry = this->actionQueue[static_cast<std::size_t>(row * BattleState::QueueColumns + column)];

            if (entry != 0)
            {
                this->pendingActionRow = static_cast<i16>(row);
                this->pendingAction = entry;
                entry = 0;
                return;
            }
        }
    }
}

// swd32.exe 0x0045B190: the units from the top down: each pass takes the first unit with a y not yet taken, then the
// lowest y among the enemies after it and the roles (a role at the same y as an enemy comes after it); a unit with y 0
// is never the first pick
void BattleState::SortDrawOrder()
{
    const u32 units = this->enemyCount + this->roleCount;

    for (u32 pass = 0; pass < units; ++pass)
    {
        u32 first = 0;

        while (first < static_cast<u32>(BattleState::Units) && (this->unitY[first] == 0 || this->drawTaken[first] == 1))
        {
            ++first;
        }

        i32 lowest = first < static_cast<u32>(BattleState::Units) ? this->unitY[first] : 0;
        u32 chosen = first;

        for (u32 enemy = first + 1; enemy < this->enemyCount; ++enemy)
        {
            if (this->drawTaken[enemy] != 1 && this->unitY[enemy] < lowest)
            {
                lowest = this->unitY[enemy];
                chosen = enemy;
            }
        }

        for (u32 unit = BattleState::FirstRoleUnit; unit < this->roleCount + BattleState::FirstRoleUnit; ++unit)
        {
            if (this->drawTaken[unit] != 1 && this->unitY[unit] < lowest)
            {
                lowest = this->unitY[unit];
                chosen = unit;
            }
        }

        if (chosen < static_cast<u32>(BattleState::Units))
        {
            this->drawTaken[chosen] = 1;
        }

        this->drawOrder[pass] = static_cast<i32>(chosen);
    }

    this->drawTaken.fill(0);
}

// swd32.exe 0x0045B5A0: the enemies in drawing order
void BattleState::ListEnemyOrder()
{
    u32 listed = 0;

    for (const i32 unit : this->drawOrder)
    {
        if (unit < BattleState::FirstRoleUnit)
        {
            this->enemyOrder[listed] = unit;
            ++listed;

            if (listed == this->enemyCount)
            {
                break;
            }
        }
    }
}

// swd32.exe 0x0045EDF0: `unit` with kind 1 or 2 put in the first free record (none when all are used); other kinds
// add nothing
void BattleState::AddSlotRecord(int kind, i32 unit)
{
    if (kind != 1 && kind != kSecondKind)
    {
        return;
    }

    for (BattleSlotRecord& record : this->unk524788)
    {
        if (record.unit == -1)
        {
            record.unit = unit;
            record.kind = static_cast<u16>(kind);
            return;
        }
    }
}

// swd32.exe 0x0045EFB0: the record of `unit` removed, the later ones moving up and the last made free with every byte
// 0xFF (the original first copies the bytes after the records into the last one)
void BattleState::RemoveSlotRecord(i32 unit)
{
    auto& records = this->unk524788;
    auto found = std::find_if(
        records.begin(),
        records.end(),
        [unit](const BattleSlotRecord& record)
        {
            return record.unit == unit;
        }
    );

    if (found == records.end())
    {
        return;
    }

    std::move(found + 1, records.end(), found);
    records.back() = BattleSlotRecord{-1, -1, kAllOnes16, kAllOnes16, -1, -1, -1, -1};
}

// swd32.exe 0x0045D3E0: the field fades from (red, green, blue) to (toRed, toGreen, toBlue) over `frames` frames; the
// steps are taken from the levels truncated to integers
void BattleState::StartFieldFade(int red, int green, int blue, int toRed, int toGreen, int toBlue, int frames)
{
    ColorFade& fade = this->fieldFade;
    fade.frames = frames;
    fade.red = static_cast<float>(red);
    fade.green = static_cast<float>(green);
    fade.blue = static_cast<float>(blue);
    fade.startRed = static_cast<float>(toRed);
    fade.startGreen = static_cast<float>(toGreen);
    fade.startBlue = static_cast<float>(toBlue);
    const auto count = static_cast<double>(frames);
    fade.stepRed = static_cast<float>(static_cast<double>(toRed - static_cast<i32>(crt::Ftol(fade.red))) / count);
    fade.stepGreen = static_cast<float>(static_cast<double>(toGreen - static_cast<i32>(crt::Ftol(fade.green))) / count);
    fade.stepBlue = static_cast<float>(static_cast<double>(toBlue - static_cast<i32>(crt::Ftol(fade.blue))) / count);
}
}  // namespace swd3
