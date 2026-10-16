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

    for (int row = 0; row < BattleState::kQueueRows; ++row)
    {
        for (int column = 0; column < BattleState::kQueueColumns; ++column)
        {
            i16& entry = this->actionQueue[static_cast<std::size_t>(row * BattleState::kQueueColumns + column)];

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

}  // namespace swd3
