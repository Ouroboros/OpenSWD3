#pragma once

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
};

}  // namespace swd3
