#include "battle/battle_state.h"

namespace swd3
{

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

}  // namespace swd3
