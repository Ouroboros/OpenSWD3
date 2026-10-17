#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "draw/painter.h"
#include "game_data.h"
#include "menu/menu.h"
#include "menu/menu_text.h"
#include "party/party.h"
#include "platform/display.h"
#include "platform/surface.h"
#include "save/save_games.h"
#include "save/save_slots.h"
#include "test.h"
#include "text/big5.h"

using swd3::u32;
using swd3::u8;

namespace
{

// Where swd32.exe keeps the texts and tables.
constexpr std::array<std::pair<swd3::u32, std::u8string_view>, 6> kTexts = {{
    {0x004A70EC, swd3::menu_text::kSoundVolume},
    {0x004A70A4, swd3::menu_text::kCacheLine},
    {0x004A7054, swd3::menu_text::kSample},
    {0x004A7068, swd3::menu_text::kSpeedSlow},
    {0x004A7088, swd3::menu_text::kSpeedFastest},
    {0x0049FA9C, swd3::SaveSlots::Hours},
}};

constexpr swd3::u32 kDateTemplateAt = 0x0049FAA4;
constexpr std::array<swd3::u32, 2> kNameCopies = {0x0049E188, 0x0049E148};
constexpr swd3::u32 kInitialTabsAt = 0x004A6A38;

constexpr swd3::i16 kAttack = 3;
constexpr swd3::i16 kHeRanAttributes = 0x19;  // the name cheats of 何然, 寧采臣 and 小倩
constexpr swd3::i16 kNingCaiChenAttackLoss = 0xA;
constexpr swd3::i32 kNingCaiChenGold = 0x1F4;
constexpr swd3::i16 kXiaoQianValue = 5;
constexpr std::size_t kYearDigits = 4;  // the date starts with the year
constexpr swd3::u16 kAbsentRole = 0xFFFF;

}  // namespace

TEST_CASE("menu: texts, names and tabs match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    for (const auto& [address, text] : kTexts)
    {
        CHECK(swd3test::SameText(exe, address, text));
    }

    CHECK(swd3test::SameText(exe, kDateTemplateAt, swd3::SaveSlots::DateTemplate));

    for (const swd3::u32 copy : kNameCopies)
    {
        CHECK(swd3test::SameTable(exe, copy, swd3::Party::DefaultNames()));
    }

    static_assert(sizeof(swd3::MenuTab) == 0x1C);
    CHECK(swd3test::SameTable(exe, kInitialTabsAt, swd3::Menu::InitialTabs));
}

TEST_CASE("menu: name cheats")
{
    swd3::Party party;
    party.roles[0].attack = kAttack;
    party.names[0] = swd3::Party::MakeName(swd3::Big5(u8"何然"));
    party.ApplyNameCheats();
    CHECK_EQ(party.roles[0].attack, kHeRanAttributes);
    CHECK_EQ(party.roles[0].agility, kHeRanAttributes);
    party.names[0] = swd3::Party::MakeName(swd3::Big5(u8"寧采臣"));
    party.ApplyNameCheats();
    CHECK_EQ(party.variables[swd3::Party::Gold], kNingCaiChenGold);
    CHECK_EQ(party.roles[0].attack, kHeRanAttributes - kNingCaiChenAttackLoss);
    party.names[1] = swd3::Party::MakeName(swd3::Big5(u8"小倩"));
    party.ApplyNameCheats();
    CHECK_EQ(party.roles[1].maxSpirit, kXiaoQianValue);
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

    swd3::Surface surface(swd3::Display::Width, swd3::Display::Height);
    swd3::Painter painter;
    painter.Init(surface);
    swd3::SavePreview preview;
    swd3::SaveSlots::LoadPreview(preview, 0, directory, painter);
    CHECK_EQ(preview.state, swd3::SavePreview::Loaded);
    CHECK_EQ(preview.date.size(), swd3::SaveSlots::DateTemplate.size());
    CHECK_EQ(preview.date[kYearDigits], swd3::SaveSlots::DateTemplate[kYearDigits]);
    CHECK_EQ(preview.thumbnail.size(), swd3::SaveSlots::ThumbnailPixels);
    CHECK(!preview.location.empty());
    CHECK(preview.roles[0].maxHitPoints != kAbsentRole);
    CHECK(preview.roles[0].level >= 1);
    CHECK(preview.names[0] != 0);
    CHECK(!preview.playTime.empty());
    swd3::SavePreview missing;
    swd3::SaveSlots::LoadPreview(missing, swd3::SaveGames::LastSlot - 1, directory, painter);
    CHECK_EQ(missing.state, swd3::SavePreview::Missing);
}
