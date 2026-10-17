#include <filesystem>
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
    CHECK(settings.dataDirectory.empty());
    CHECK(settings.saveDirectory.empty());
    CHECK(settings.logPath.empty());
    CHECK_EQ(settings.logLevel, SDL_LOG_PRIORITY_DEBUG);
    CHECK_EQ(settings.windowWidth, swd3::Display::DefaultWindowWidth);
    CHECK_EQ(settings.windowHeight, swd3::Display::DefaultWindowHeight);
    CHECK(!settings.windowMaximized);
    CHECK_EQ(settings.displayWidth, swd3::Display::Width);
    CHECK_EQ(settings.displayHeight, swd3::Display::Height);
    CHECK(settings.warnings.empty());
}

TEST_CASE("host settings: every section read, relative paths from the executable's directory")
{
    const std::string text = "\xEF\xBB\xBF# comment\n"
                             "[paths]\n"
                             "data_dir = './data'   # the game data\n"
                             "save_dir = \"/saves/swd3\"\n"
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
    CHECK(settings.dataDirectory == kBase / "./data");
    CHECK(settings.saveDirectory == std::filesystem::path("/saves/swd3"));
    CHECK(settings.logPath == kBase / "logs/game.txt");
    CHECK_EQ(settings.logLevel, SDL_LOG_PRIORITY_WARN);
    CHECK_EQ(settings.windowWidth, 960);
    CHECK_EQ(settings.windowHeight, 720);
    CHECK(settings.windowMaximized);
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
    CHECK_EQ(settings.logLevel, SDL_LOG_PRIORITY_DEBUG);
    CHECK_EQ(settings.windowWidth, swd3::Display::DefaultWindowWidth);
    CHECK_EQ(settings.windowHeight, swd3::Display::DefaultWindowHeight);
    CHECK(!settings.windowMaximized);
    CHECK_EQ(settings.warnings.size(), static_cast<std::size_t>(6));
}

TEST_CASE("host settings: a file that is not TOML is ignored as a whole")
{
    const swd3::HostSettings settings = swd3::ParseHostSettings("[window]\nwidth = 960\nheight = big\n", kBase);
    CHECK_EQ(settings.windowWidth, swd3::Display::DefaultWindowWidth);
    CHECK_EQ(settings.warnings.size(), static_cast<std::size_t>(1));
    const swd3::HostSettings escaped =
        swd3::ParseHostSettings("[paths]\ndata_dir = \"/games/sw\\u0064\\u0033\"\n", kBase);
    CHECK(escaped.dataDirectory == std::filesystem::path("/games/swd3"));
}

TEST_CASE("host settings: the display stays at 640x480")
{
    const swd3::HostSettings larger = swd3::ParseHostSettings("[display]\nwidth = 800\nheight = 600\n", kBase);
    CHECK_EQ(larger.displayWidth, swd3::Display::Width);
    CHECK_EQ(larger.warnings.size(), static_cast<std::size_t>(1));
    const swd3::HostSettings half = swd3::ParseHostSettings("[display]\nwidth = 800\n", kBase);
    CHECK_EQ(half.warnings.size(), static_cast<std::size_t>(1));
    const swd3::HostSettings scaled = swd3::ParseHostSettings("[display]\nscale = 1.5\n", kBase);
    CHECK_EQ(scaled.displayHeight, swd3::Display::Height);
    CHECK_EQ(scaled.warnings.size(), static_cast<std::size_t>(1));
    const swd3::HostSettings one = swd3::ParseHostSettings("[display]\nscale = 1.0\n", kBase);
    CHECK(one.warnings.empty());
}
