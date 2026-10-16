#include "core/debug_log.h"
#include "game/game.h"

namespace swd3
{

// The battle start (0x00451B10) is restored with the battle module (its main loop 0x00469D20 with it); until then a
// battle met on the map is not started. 0x00402F80 stays pending in the function table meanwhile.
void Game::StartBattle(u16 battle)
{
    DebugLog("battle start not restored", battle);
}

}  // namespace swd3
