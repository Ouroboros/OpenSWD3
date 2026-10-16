#pragma once

#include <cstdint>

#include "battle/battle_unit.h"

namespace swd3 {

// Enemy in battle (0x2B28 bytes; eight at G.battleEnemies, 0x00525508); its data block (0xA4 bytes) is at +0xC.
class BattleEnemy : public BattleUnit {
public:
    BattleEnemy();
    ~BattleEnemy();
    int FreeData();
    std::uint16_t* GetDropInfo(std::uint16_t* first, std::uint16_t* second);
    int StatusFlag1000Or10000000();
    std::uint16_t RollDrop();
    bool IsCatchable();
    std::int16_t GetCatchInfo(std::uint16_t* act, std::uint16_t* frame);
    const std::uint8_t* SetResistances(const std::uint8_t* tenths);
    std::int16_t SetStats66(std::int16_t first, std::int16_t second, std::int16_t third, std::int16_t fourth,
                            std::int16_t fifth, std::int16_t sixth);
    std::int16_t SetStats72(std::int16_t first, std::int16_t second, std::int16_t third, std::int16_t fourth);
    int StepGauge(int quick);
    int ChooseHealing();
    bool RollSteal(std::uint8_t level);
};

static_assert(sizeof(BattleEnemy) == 0x2B28);

int LoadMonsterSkill(std::uint8_t* skill, std::uint16_t id);
int LoadMonster(std::uint8_t* data, std::uint16_t id);
int LoadLevelEntry(int group, int index, std::uint32_t* value);

}  // namespace swd3
