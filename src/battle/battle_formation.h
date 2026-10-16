#pragma once

#include <cstdint>

#include "core/mem.h"

namespace swd3 {

class Projector;

// Battle formations of battle.ffd (global at 0x004FF5B8, 0x31F8 bytes): +0 the projector, +4 the file index
// (0x2714 bytes: formation numbers at +8, signed formation counts per group at +0x1F48), +0x3104 thirty pairs
// (k, 5k / 4). Fields are addressed by offset.
class BattleFormation {
public:
    explicit BattleFormation(Projector* projector);
    int LoadIndex(const char* fileName, std::uint8_t** counts);
    int LoadFormation(const char* fileName, void* formation, std::uint16_t group, std::int8_t index);

    template <typename T>
    T& at(std::intptr_t offset) {
        return mem::ref<T>(m_data, offset);
    }

    std::uint8_t m_data[0x31F8];
};

static_assert(sizeof(BattleFormation) == 0x31F8);

}  // namespace swd3
