#include <algorithm>
#include <bit>
#include <climits>
#include <cstdio>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include "game/game.h"
#include "game/game_state.h"
#include "launcher/launcher.h"
#include "platform/display.h"
#include "platform/exe_resources.h"
#include "platform/file_system.h"
#include "platform/surface.h"
#include "platform/system_sound.h"
#include "test.h"

using swd3::Launcher;

namespace
{

constexpr std::size_t kDialogBytes = 274;  // the launcher dialog template and its hover sound in swd32.exe
constexpr std::size_t kHoverSoundBytes = 2348;
constexpr int kNoSuchWave = 0x99;
constexpr int kHighlightWidth = 40;  // a highlight picture, a little larger than its static control
constexpr int kHighlightHeight = 182;
constexpr int kOutside = 300;  // below the hot rows
constexpr int kInsideY = 50;

}  // namespace

TEST_CASE("launcher: resources of swd32.exe and the hover hotspots")
{
    swd3::ExeResources resources;
    resources.Open(swd3::AssetDirectory() + std::u8string(swd3::ExeResources::Subdirectory));
    swd3::Surface picture;

    if (!resources.LoadBitmap(Launcher::Background, picture))
    {
        std::printf("  (asset not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    CHECK_EQ(resources.ReadDialog(Launcher::Dialog).size(), kDialogBytes);
    CHECK_EQ(resources.ReadSound(Launcher::HoverSound).size(), kHoverSoundBytes);
    CHECK(resources.ReadSound(kNoSuchWave).empty());
    CHECK_EQ(picture.Width(), swd3::Display::Width);
    CHECK_EQ(picture.Height(), swd3::Display::Height);
    CHECK(resources.LoadBitmap(Launcher::Layout.back().picture, picture));
    CHECK_EQ(picture.Width(), kHighlightWidth);
    CHECK_EQ(picture.Height(), kHighlightHeight);

    swd3::GameState state;
    swd3::SystemSound sound;
    Launcher launcher(resources, state, sound, u8"");
    CHECK(launcher.LoadPictures());
    launcher.Init();
    CHECK(launcher.shown[0]);
    CHECK(launcher.OnMouse(0, kOutside, kOutside) == Launcher::Choice::None);
    CHECK(!launcher.shown[0]);
    CHECK(launcher.OnMouse(1, Launcher::Layout.front().x, kInsideY) == Launcher::Choice::None);
    CHECK_EQ(launcher.lit[0], 0);
    state.SetGameFlag(Launcher::SavedGameFlag);
    CHECK(launcher.OnMouse(0, Launcher::Layout.front().x, kInsideY) == Launcher::Choice::None);
    CHECK_EQ(launcher.lit.front(), Launcher::Shown);
    CHECK(launcher.shown[0]);
    CHECK(launcher.OnMouse(1, Launcher::Layout.front().x, kInsideY) == Launcher::Choice::Continue);
    CHECK(launcher.OnMouse(1, Launcher::Layout[1].x, Launcher::HotBottom) == Launcher::Choice::Start);
    CHECK(launcher.OnMouse(1, Launcher::Layout[1].x, Launcher::HotBottom + 1) == Launcher::Choice::None);
    CHECK(launcher.OnMouse(1, Launcher::Layout.back().x, kInsideY) == Launcher::Choice::Exit);
    CHECK_EQ(launcher.lit.back(), Launcher::Shown);
    CHECK(launcher.OnClose() == Launcher::Choice::Exit);
    swd3::Surface screen(swd3::Display::Width, swd3::Display::Height);
    launcher.Draw(screen);
}

TEST_CASE("launcher: the window icon and cursor of swd32.exe")
{
    constexpr int kSide = 32;
    constexpr std::u8string_view kIconFile = u8"101.ico";  // Game::WindowIcon; its last member has 24 bits
    constexpr std::size_t kHeaderBytes = 40;
    constexpr std::size_t kBitsAt = 14;  // biBitCount
    constexpr std::size_t kPixelBytes = 3;
    constexpr std::size_t kMaskBytes = kSide * kSide / CHAR_BIT;
    constexpr std::size_t kMemberBytes = kHeaderBytes + kPixelBytes * kSide * kSide + kMaskBytes;
    constexpr swd3::u32 kOpaque = 0xFF000000u;
    swd3::ExeResources resources;
    resources.Open(swd3::AssetDirectory() + std::u8string(swd3::ExeResources::Subdirectory));
    swd3::IconImage icon;
    std::vector<swd3::u8> file;

    if (!resources.LoadIcon(swd3::Game::WindowIcon, icon) ||
        !swd3::ReadSharedFile(
            swd3::AssetDirectory() + std::u8string(swd3::ExeResources::Subdirectory) + std::u8string(kIconFile),
            file
        ))
    {
        std::printf("  (asset not found, skipped)\n");
        return;
    }

    CHECK_EQ(icon.width, kSide);
    CHECK_EQ(icon.height, kSide);
    CHECK_EQ(icon.pixels.size(), static_cast<std::size_t>(kSide * kSide));

    CHECK(file.size() > kMemberBytes);
    const std::span<const swd3::u8> member = std::span<const swd3::u8>(file).last(kMemberBytes);
    CHECK_EQ(member[0], kHeaderBytes);
    CHECK_EQ(member[kBitsAt], kPixelBytes * CHAR_BIT);
    std::size_t clear = 0;

    for (std::size_t at = member.size() - kMaskBytes; at < member.size(); ++at)
    {
        clear += static_cast<std::size_t>(CHAR_BIT - std::popcount(member[at]));
    }

    const auto opaque = static_cast<std::size_t>(std::count_if(
        icon.pixels.begin(),
        icon.pixels.end(),
        [](swd3::u32 pixel)
        {
            return pixel >= kOpaque;
        }
    ));
    CHECK_EQ(opaque, clear);

    const std::size_t topLeft = kHeaderBytes + kPixelBytes * kSide * (kSide - 1);
    const swd3::u32 color = static_cast<swd3::u32>(member[topLeft + 2]) << 16 |
                            static_cast<swd3::u32>(member[topLeft + 1]) << 8 | member[topLeft];
    CHECK_EQ(icon.pixels.front() & ~kOpaque, (icon.pixels.front() >= kOpaque ? color : 0));

    swd3::IconImage cursor;
    CHECK(resources.LoadCursor(swd3::Game::WindowCursor, cursor));
    CHECK_EQ(cursor.width, kSide);
    CHECK_EQ(cursor.height, kSide);
    CHECK_EQ(cursor.hotX, 0);
    CHECK_EQ(cursor.hotY, 0);
    CHECK(
        std::all_of(
            cursor.pixels.begin(),
            cursor.pixels.end(),
            [](swd3::u32 pixel)
            {
                return pixel == 0;
            }
        )
    );
    CHECK(!resources.LoadIcon(kNoSuchWave, icon));
}
