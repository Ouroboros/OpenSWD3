#include <cstdio>
#include <string>

#include "game_data.h"
#include "org/database.h"
#include "org/level.h"
#include "party/party.h"
#include "recording_reporter.h"
#include "test.h"

TEST_CASE("level: level.dat raises a party role and teaches skills once")
{
    if (swd3test::ReadGameFile("LEVEL.DAT").empty() || swd3test::ReadGameFile("MON.DAT").empty())
    {
        std::printf("  (level.dat or mon.dat not found, skipped)\n");
        return;
    }

    const std::string directory = swd3test::GameDirectory() + "/";
    swd3::Database database(directory);
    swd3::Party party;

    for (auto& head : party.skills)
    {
        head = swd3::NewEmptyItem();
    }

    swd3::LearnedSkill last;
    swd3test::RecordingReporter reporter;
    int learnedAny = 0;

    // Entry 1 of the table is not a level script in the game data (its first word is not 0).
    for (int level = 2; level <= 20; ++level)
    {
        swd3::RoleRecord& role = party.roles[0];
        int learned = 0;
        CHECK(swd3::LevelUp(role, 1, level, learned, party, database, last, reporter));
        CHECK_EQ(role.level, level);
        CHECK(role.maxHitPoints > 0);
        CHECK_EQ(role.hitPoints, role.maxHitPoints);
        CHECK_EQ(role.spirit, role.maxSpirit);
        CHECK_EQ(role.stamina, role.maxStamina);
        swd3::u32 experience = 0;
        CHECK(database.LoadLevelEntry(1, level, experience));
        CHECK_EQ(static_cast<swd3::u32>(role.experience), experience);
        learnedAny += learned;
    }

    std::printf("  levels with a new skill: %d\n", learnedAny);
    const swd3::ItemNode* first = party.skills[0]->next.get();

    if (first != nullptr)
    {
        CHECK_EQ(swd3::LearnSkill(0, static_cast<swd3::i16>(first->id), party, database, last, reporter), 0);
    }

    CHECK_EQ(swd3::LearnSkill(1, static_cast<swd3::i16>(0x8000), party, database, last, reporter), 1);
    CHECK(party.skills[1]->next == nullptr);
    CHECK(reporter.messages.empty());
    swd3::LearnSkill(1, 0, party, database, last, reporter);
    CHECK_EQ(reporter.messages.size(), 1u);
}
