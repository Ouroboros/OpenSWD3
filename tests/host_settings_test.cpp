#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "platform/host_settings.h"
#include "test.h"

namespace
{

const std::filesystem::path kBase = "/game";

}  // namespace

TEST_CASE("host settings: the defaults without a file")
{
    const swd3::HostSettings settings = swd3::ParseHostSettings("", kBase);
    CHECK(settings.values == swd3::Settings{});
    CHECK(settings.DataDirectory().empty());
    CHECK(settings.SaveDirectory().empty());
    CHECK(settings.LogPath().empty());
    CHECK_EQ(settings.LogLevel(), SDL_LOG_PRIORITY_DEBUG);
    CHECK_EQ(settings.values.window.width, swd3::Display::DefaultWindowWidth);
    CHECK_EQ(settings.values.window.height, swd3::Display::DefaultWindowHeight);
    CHECK(!settings.values.window.maximized);
    CHECK(settings.warnings.empty());
}

TEST_CASE("host settings: every table read into its struct, relative paths from the executable's directory")
{
    const std::string text = "\xEF\xBB\xBF# comment\n"
                             "[paths]\n"
                             "data_dir = './data'   # the game data\n"
                             "save_dir = \"/saves/sw\\u0064\\u0033\"\n"
                             "\n"
                             "[logging]\n"
                             "path = \"logs/game.txt\"\n"
                             "level = \"warning\"\n"
                             "[window]\n"
                             "width = 960\n"
                             "height = 720\n"
                             "maximized = true\n"
                             "[display]\n"
                             "width = 640\n"
                             "height = 480\n";
    const swd3::HostSettings settings = swd3::ParseHostSettings(text, kBase);
    CHECK(settings.DataDirectory() == kBase / "./data");
    CHECK(settings.SaveDirectory() == std::filesystem::path("/saves/swd3"));
    CHECK(settings.LogPath() == kBase / "logs/game.txt");
    CHECK_EQ(settings.LogLevel(), SDL_LOG_PRIORITY_WARN);
    CHECK_EQ(settings.values.window.width, 960);
    CHECK_EQ(settings.values.window.height, 720);
    CHECK(settings.values.window.maximized);
    CHECK_EQ(settings.values.display.width.value_or(0), swd3::Display::Width);
    CHECK(settings.warnings.empty());
}

TEST_CASE("host settings: invalid values keep their defaults with a warning")
{
    const std::string text = "[logging]\n"
                             "level = \"loud\"\n"
                             "[window]\n"
                             "width = 100\n"
                             "height = \"big\"\n"
                             "maximized = 1\n"
                             "colour = 3\n"
                             "[sound]\n";
    const swd3::HostSettings settings = swd3::ParseHostSettings(text, kBase);
    CHECK(settings.values == swd3::Settings{});
    CHECK_EQ(settings.warnings.size(), static_cast<std::size_t>(6));
}

TEST_CASE("host settings: a file that is not TOML is ignored as a whole")
{
    const swd3::HostSettings settings = swd3::ParseHostSettings("[window]\nwidth = 960\nheight = big\n", kBase);
    CHECK(settings.values == swd3::Settings{});
    CHECK_EQ(settings.warnings.size(), static_cast<std::size_t>(1));
}

TEST_CASE("host settings: the display stays at 640x480")
{
    CHECK_EQ(
        swd3::ParseHostSettings("[display]\nwidth = 800\nheight = 600\n", kBase).warnings.size(),
        static_cast<std::size_t>(1)
    );
    CHECK_EQ(swd3::ParseHostSettings("[display]\nwidth = 800\n", kBase).warnings.size(), static_cast<std::size_t>(1));
    CHECK_EQ(swd3::ParseHostSettings("[display]\nscale = 1.5\n", kBase).warnings.size(), static_cast<std::size_t>(1));
    CHECK(swd3::ParseHostSettings("[display]\nscale = 1.0\n", kBase).warnings.empty());
    CHECK(swd3::ParseHostSettings("[display]\nscale = 1\n", kBase).warnings.empty());
}

TEST_CASE("host settings: smooth motion, vsync and the frame rate")
{
    constexpr swd3::i64 kFrameRate = 144;
    const swd3::HostSettings set =
        swd3::ParseHostSettings("[display]\nsmooth_motion = false\nvsync = false\nframe_rate = 144\n", kBase);
    CHECK(set.warnings.empty());
    CHECK(!set.values.display.smoothMotion.value_or(true));
    CHECK(!set.values.display.vsync.value_or(true));
    CHECK_EQ(set.values.display.frameRate.value_or(0), kFrameRate);
    CHECK(!swd3::ParseHostSettings("", kBase).values.display.smoothMotion.has_value());
    CHECK(swd3::ParseHostSettings("[display]\nframe_rate = 0\n", kBase).warnings.empty());
    const swd3::HostSettings slow = swd3::ParseHostSettings("[display]\nframe_rate = 10\n", kBase);
    CHECK_EQ(slow.warnings.size(), static_cast<std::size_t>(1));
    CHECK(!slow.values.display.frameRate.has_value());
}

TEST_CASE("host settings: the struct written by toml++ reads back the same")
{
    swd3::Settings settings;
    settings.paths.dataDir = "E:\\Game\\swd3";
    settings.logging.level = "info";
    settings.window = {960, 720, true};
    const std::string text = swd3::SettingsText(settings);
    std::printf("  written by toml++:\n%s", text.c_str());
    const swd3::HostSettings read = swd3::ParseHostSettings(text, kBase);
    CHECK(read.values == settings);
    CHECK(read.warnings.empty());
    CHECK(text.find("save_dir") == std::string::npos);
    CHECK(text.find("[display]") != std::string::npos);
}

TEST_CASE("host settings: the window saved into the file, nothing written when it is the same")
{
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "swd3r_settings_test.toml";
    std::filesystem::remove(file);
    std::string error;
    CHECK(swd3::SaveWindowSettings(file, {900, 700, false}, error));
    CHECK(swd3::LoadHostSettings({}, file, kBase).values.window == (swd3::WindowSettings{900, 700, false}));

    {
        std::ofstream stream(file, std::ios::binary | std::ios::trunc);
        stream << "[logging]\nlevel = \"info\"\n\n[window]\nwidth = 900\nheight = 700\nmaximized = false\n";
    }

    const auto written = std::filesystem::last_write_time(file);
    CHECK(swd3::SaveWindowSettings(file, {900, 700, false}, error));
    CHECK(std::filesystem::last_write_time(file) == written);
    CHECK(swd3::SaveWindowSettings(file, {1024, 768, true}, error));
    const swd3::HostSettings read = swd3::LoadHostSettings({}, file, kBase);
    CHECK(read.values.window == (swd3::WindowSettings{1024, 768, true}));
    CHECK_EQ(read.LogLevel(), SDL_LOG_PRIORITY_INFO);

    {
        std::ofstream stream(file, std::ios::binary | std::ios::trunc);
        stream << "[window\nwidth = 1\n";
    }

    CHECK(!swd3::SaveWindowSettings(file, {1024, 768, true}, error));
    std::filesystem::remove(file);
}

TEST_CASE("host settings: the user's file over the program's, [game] read with its keys")
{
    const std::string program =
        "[window]\nwidth = 1000\nheight = 800\n"
        "[game]\nsound_volume = 11\ncache_size = 140\neffects_off = false\nrunning = false\nsession_open = true\n"
        "[game.keys]\nup = 200\ngive_up = 1\n";
    const std::string user = "[window]\nwidth = 700\n[game]\nsound_volume = 3\n[game.keys]\nup = 72\n";
    const swd3::HostSettings settings = swd3::ParseHostSettings(user, program, kBase);
    CHECK(settings.programHasGame);
    CHECK_EQ(settings.values.window.width, 700);
    CHECK_EQ(settings.values.window.height, 800);
    CHECK_EQ(settings.values.game.soundVolume.value_or(-1), 3);
    CHECK_EQ(settings.values.game.cacheSize.value_or(-1), 140);
    CHECK(settings.values.game.sessionOpen.value_or(false));
    CHECK(!settings.values.game.running.value_or(true));
    CHECK_EQ(settings.values.game.keys.up.value_or(-1), 72);
    CHECK_EQ(settings.values.game.keys.giveUp.value_or(-1), 1);
    CHECK(!settings.values.game.keys.down.has_value());
    CHECK(settings.warnings.empty());
    CHECK(!swd3::ParseHostSettings(user, "[window]\n", kBase).programHasGame);
    const swd3::HostSettings bad =
        swd3::ParseHostSettings("", "[game]\nsound_volume = 300\n[game.keys]\nhelp = -1\n", kBase);
    CHECK(!bad.values.game.soundVolume.has_value());
    CHECK(!bad.values.game.keys.help.has_value());
    CHECK_EQ(bad.warnings.size(), static_cast<std::size_t>(2));
}

TEST_CASE("host settings: the program's file updated, the user's untouched")
{
    const std::filesystem::path file = std::filesystem::temp_directory_path() / "swd3r_program_test.toml";
    std::filesystem::remove(file);
    std::string error;
    CHECK(
        swd3::UpdateSettingsFile(
            file,
            [](swd3::Settings& settings)
            {
                settings.game.musicVolume = 4;
                settings.game.running = false;
                settings.game.keys.menu = 28;
            },
            error
        )
    );
    const swd3::HostSettings read = swd3::LoadHostSettings({}, file, kBase);
    CHECK(read.programHasGame);
    CHECK_EQ(read.values.game.musicVolume.value_or(-1), 4);
    CHECK(read.values.game.running.has_value() && !*read.values.game.running);
    CHECK_EQ(read.values.game.keys.menu.value_or(-1), 28);
    const auto written = std::filesystem::last_write_time(file);
    CHECK(
        swd3::UpdateSettingsFile(
            file,
            [](swd3::Settings& settings)
            {
                settings.game.musicVolume = 4;
            },
            error
        )
    );
    CHECK(std::filesystem::last_write_time(file) == written);
    CHECK(
        swd3::UpdateSettingsFile(
            {},
            [](swd3::Settings&)
            {
            },
            error
        )
    );
    std::filesystem::remove(file);
}
