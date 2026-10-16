#pragma once

#include <string>

#include "core/types.h"

namespace swd3
{

class Database;
class ErrorReporter;
class Party;
struct HeroRecord;

// The skill of the last level-up or learning (0x0053C154).
struct LearnedSkill
{
    std::string name;
};

bool LevelUp(
    HeroRecord& hero,
    int hero1,
    int level,
    int& learned,
    Party& party,
    Database& database,
    LearnedSkill& last,
    ErrorReporter& reporter
);
int LearnSkill(int hero, i16 id, Party& party, Database& database, LearnedSkill& last, ErrorReporter& reporter);

}  // namespace swd3
