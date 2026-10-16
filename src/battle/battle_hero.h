#pragma once

#include <cstdint>

#include "battle/battle_unit.h"

namespace swd3 {

// Hero in battle (0x2F34 bytes; ten at G.battleHeroes, 0x005029D0): BattleUnit plus 0x40C bytes of hero state
// (+0x2B28..), still addressed by offset.
class BattleHero : public BattleUnit {
public:
    BattleHero();
    int Flag2B00();
    int Flag2B04();
    int StepGauge(int quick);
    int ResetHeroState();
    int SetBoth2EC8(int value);
    int SetBoth2EC0(int value);
    int AddRecordProgress(std::uint8_t* list, std::uint16_t amount);
    int AddFixedRecordProgress(std::uint8_t* list, const std::uint8_t* item);
    std::uint32_t CompleteRecord(std::uint8_t* list);
    int TryFirstStrike();
    int Restore2EC0();
    void ApplyPendingMpCost();
    int MarkGuarded();
    int Restore2EC8();
    int ActionFlag13();
    int ActionFlag12();
    std::uint32_t Mark800Clear();
    int ClearActionState();
    int StartExplosion(BattleUnit* target);
    bool RollLevelChance(std::uint16_t level);
    int DrawRecoil(int mode);
    int DrawLeap(BattleUnit* target);

    std::uint8_t m_heroData[0x2F34 - 0x2B28];
};

static_assert(sizeof(BattleHero) == 0x2F34);

int GetEnemyDataAlways(BattleUnit* unit);
bool RollChance(int percent);

}  // namespace swd3
