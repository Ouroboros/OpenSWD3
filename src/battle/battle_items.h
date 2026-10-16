#pragma once

#include "core/types.h"
#include "item/item_list.h"

namespace swd3
{

class BattleState;
class GameState;
class Graphics;
class Party;

int TotalItemValue(const BattleState& battle);
void CountListB(BattleState& battle);
void SortBattleItems(BattleState& battle, ItemLink& source);
void FreeBattleItems(BattleState& battle);
int CountItemTotal(const BattleState& battle, const Party& party, const GameState& state, int id);
u32 ItemPriority(u32 kind);
int SetGaugeFull(BattleState& battle, int speed);
int CreateBattleSurfaces(BattleState& battle, Graphics& graphics);
void ReleaseBattleSurfaces(BattleState& battle);

}  // namespace swd3
