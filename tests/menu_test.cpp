#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "draw/painter.h"
#include "game_data.h"
#include "menu/menu.h"
#include "menu/menu_text.h"
#include "party/party.h"
#include "platform/surface.h"
#include "save/save_slots.h"
#include "test.h"
#include "text/big5.h"

using swd3::u32;
using swd3::u8;

TEST_CASE("menu: texts, names and tabs match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    namespace text = swd3::menu_text;
    CHECK(swd3test::SameText(exe, 0x004A70EC, text::kSoundVolume));
    CHECK(swd3test::SameText(exe, 0x004A70A4, text::kCacheLine));
    CHECK(swd3test::SameText(exe, 0x004A7054, text::kSample));
    CHECK(swd3test::SameText(exe, 0x004A7068, text::kSpeedSlow));
    CHECK(swd3test::SameText(exe, 0x004A7088, text::kSpeedFastest));
    CHECK(swd3test::SameText(exe, 0x0049FAA4, swd3::SaveSlots::DateTemplate));
    CHECK(swd3test::SameText(exe, 0x0049FA9C, swd3::SaveSlots::Hours));
    CHECK(std::memcmp(exe.data() + (0x0049E188 - 0x400000), swd3::Party::DefaultNames().data(), 0x40) == 0);
    CHECK(std::memcmp(exe.data() + (0x0049E148 - 0x400000), swd3::Party::DefaultNames().data(), 0x40) == 0);
    static_assert(sizeof(swd3::MenuTab) == 0x1C);
    CHECK(std::memcmp(exe.data() + (0x004A6A38 - 0x400000), swd3::Menu::InitialTabs.data(), 0x70) == 0);
}

TEST_CASE("menu: name cheats")
{
    swd3::Party party;
    party.roles[0].attack = 3;
    party.names[0] = swd3::Party::MakeName(swd3::Big5(u8"何然"));
    party.ApplyNameCheats();
    CHECK_EQ(party.roles[0].attack, 0x19);
    CHECK_EQ(party.roles[0].agility, 0x19);
    party.names[0] = swd3::Party::MakeName(swd3::Big5(u8"寧采臣"));
    party.ApplyNameCheats();
    CHECK_EQ(party.variables[0], 0x1F4);
    CHECK_EQ(party.roles[0].attack, 0xF);
    party.names[1] = swd3::Party::MakeName(swd3::Big5(u8"小倩"));
    party.ApplyNameCheats();
    CHECK_EQ(party.roles[1].maxSpirit, 5);
}

TEST_CASE("menu: the preview of a saved game")
{
    const std::string directory = swd3test::GameDirectory() + "/";
    const std::vector<u8> save = swd3test::ReadGameFile("Save/0.sav");

    if (save.empty())
    {
        std::printf("  (Save/0.sav not found, skipped)\n");
        return;
    }

    swd3::Surface surface(640, 480);
    swd3::Painter painter;
    painter.Init(surface);
    swd3::SavePreview preview;
    swd3::SaveSlots::LoadPreview(preview, 0, directory, painter);
    CHECK_EQ(preview.state, swd3::SavePreview::Loaded);
    CHECK_EQ(preview.date.size(), 16u);
    CHECK_EQ(preview.date[4], '/');
    CHECK_EQ(preview.thumbnail.size(), 0x4B00u);
    CHECK(!preview.location.empty());
    CHECK(preview.roles[0].maxHitPoints != 0xFFFF);
    CHECK(preview.roles[0].level >= 1);
    CHECK(preview.names[0] != 0);
    CHECK(!preview.playTime.empty());
    swd3::SavePreview missing;
    swd3::SaveSlots::LoadPreview(missing, 97, directory, painter);
    CHECK_EQ(missing.state, swd3::SavePreview::Missing);
}
