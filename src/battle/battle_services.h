#pragma once

#include "core/types.h"

namespace swd3
{

class ActArchive;
class AudioManager;
class BattleHud;
class BattleMessages;
class BattleState;
class Config;
class Painter;
class Party;
class Random;
class TswArchive;

namespace crt
{
class Random;
}

// What the battle code reaches besides the units: shared by the methods of BattleUnit and its kinds.
struct BattleServices
{
    BattleState& state;
    Random& random;
    TswArchive& archive;
    ActArchive& acts;
    Painter& painter;
    AudioManager& audio;
    BattleHud& hud;
    BattleMessages& messages;
    Config& config;
    Party& party;
    crt::Random& crtRandom;  // the C runtime rand (seeded by the explosions)
    u16*& screen;            // Game::drawSurface (0x004CD76C): the locked frame the painter draws to
};

}  // namespace swd3
