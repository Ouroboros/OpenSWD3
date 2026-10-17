#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

#include "game/game_state.h"
#include "game_data.h"
#include "launcher/launcher.h"
#include "platform/display.h"
#include "platform/exe_resources.h"
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

    if (!resources.Open(swd3test::GameDirectory() + u8"/swd32.exe"))
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    CHECK_EQ(resources.Find(swd3::ExeResources::Dialog, Launcher::Dialog).size(), kDialogBytes);
    CHECK_EQ(resources.Find("WAVE", Launcher::HoverSound).size(), kHoverSoundBytes);
    CHECK(resources.Find("WAVE", kNoSuchWave).empty());
    swd3::Surface picture;
    CHECK(swd3::DecodeBitmap(resources.Find(swd3::ExeResources::Bitmap, Launcher::Background), picture));
    CHECK_EQ(picture.Width(), swd3::Display::Width);
    CHECK_EQ(picture.Height(), swd3::Display::Height);
    CHECK(swd3::DecodeBitmap(resources.Find(swd3::ExeResources::Bitmap, Launcher::Layout.back().picture), picture));
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
