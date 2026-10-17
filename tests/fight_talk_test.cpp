#include <array>
#include <filesystem>
#include <fstream>
#include <vector>

#include "battle/fight_talk.h"
#include "game_data.h"
#include "test.h"

namespace
{

constexpr std::size_t kHeaderBytes = 0x200;
constexpr swd3::u16 kBattle = 1;
constexpr swd3::u32 kScriptAt = 0x10;  // the battle's script, after the header
constexpr swd3::i32 kSeekAt = 0x20;
constexpr std::size_t kFileBytes = kHeaderBytes + 0x2000;
constexpr std::size_t kSeekBytes = 0x1000;
constexpr swd3::u8 kScriptMark = 0x5A;
constexpr swd3::u8 kSeekMark = 0xA5;
constexpr std::string_view kDirectory = "swd3_fight_talk_test";

}  // namespace

TEST_CASE("fight talk: load, seek and close")
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / kDirectory;
    std::filesystem::create_directories(directory);
    std::vector<swd3::u8> bytes(kFileBytes, 0);
    const std::size_t offsetAt = kHeaderBytes + sizeof(swd3::u32) * kBattle;

    for (std::size_t byte = 0; byte < sizeof(swd3::u32); ++byte)
    {
        bytes[offsetAt + byte] = static_cast<swd3::u8>(kScriptAt >> (8 * byte));
    }

    bytes[kHeaderBytes + kScriptAt] = kScriptMark;

    bytes[kHeaderBytes + kSeekAt] = kSeekMark;
    {
        std::ofstream file(directory / "figtalk.dat", std::ios::binary);
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    swd3::FightTalk talk;
    talk.Load(directory.string() + "/", kBattle);
    CHECK(talk.open);
    CHECK_EQ(talk.script.size(), swd3::FightTalk::ScriptBytes);
    CHECK_EQ(talk.script[0], kScriptMark);
    talk.cursor = 1;
    talk.Seek(kSeekAt);
    CHECK_EQ(talk.script.size(), kSeekBytes);
    CHECK_EQ(talk.script[0], kSeekMark);
    CHECK_EQ(talk.cursor, 0);
    CHECK_EQ(talk.offset, kSeekAt);
    talk.unk53CEAC = 1;
    talk.enabled = 0;
    talk.Close();
    CHECK(!talk.open);
    CHECK(!talk.file.IsOpen());
    CHECK(talk.script.empty());
    CHECK_EQ(talk.unk53CEAC, 0);
    CHECK_EQ(talk.enabled, 1);
    CHECK_EQ(talk.offset, 0);
    std::filesystem::remove_all(directory);
}

TEST_CASE("fight talk: the initial state matches swd32.exe")
{
    constexpr swd3::u32 kTalkAt = 0x004A7B54;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    const swd3::FightTalk talk;
    CHECK(swd3test::SameTable(exe, kTalkAt, std::array<swd3::i32, 3>{talk.unk4A7B54, talk.unk4A7B58, talk.enabled}));
}
