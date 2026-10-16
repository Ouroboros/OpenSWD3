#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

#include "game/game_state.h"
#include "game_data.h"
#include "launcher/launcher.h"
#include "platform/exe_resources.h"
#include "platform/surface.h"
#include "platform/system_sound.h"
#include "test.h"

using swd3::Launcher;

TEST_CASE("launcher: resources of swd32.exe and the hover hotspots")
{
    swd3::ExeResources resources;

    if (!resources.Open(swd3test::GameDirectory() + "/swd32.exe"))
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    CHECK_EQ(resources.Find(swd3::ExeResources::Dialog, Launcher::Dialog).size(), 274u);
    CHECK_EQ(resources.Find("WAVE", Launcher::HoverSound).size(), 2348u);
    CHECK(resources.Find("WAVE", 0x99).empty());
    swd3::Surface picture;
    CHECK(swd3::DecodeBitmap(resources.Find(swd3::ExeResources::Bitmap, Launcher::Background), picture));
    CHECK_EQ(picture.Width(), 640);
    CHECK_EQ(picture.Height(), 480);
    CHECK(swd3::DecodeBitmap(resources.Find(swd3::ExeResources::Bitmap, 0x76), picture));
    CHECK_EQ(picture.Width(), 40);
    CHECK_EQ(picture.Height(), 182);

    swd3::GameState state;
    swd3::SystemSound sound;
    Launcher launcher(resources, state, sound, "");
    CHECK(launcher.LoadPictures());
    launcher.Init();
    CHECK(launcher.shown[0]);
    CHECK(launcher.OnMouse(0, 300, 300) == Launcher::Choice::None);
    CHECK(!launcher.shown[0]);
    CHECK(launcher.OnMouse(1, 0x30, 50) == Launcher::Choice::None);
    CHECK_EQ(launcher.lit[0], 0);
    state.SetGameFlag(Launcher::SavedGameFlag);
    CHECK(launcher.OnMouse(0, 0x30, 50) == Launcher::Choice::None);
    CHECK_EQ(launcher.lit[0], 5);
    CHECK(launcher.shown[0]);
    CHECK(launcher.OnMouse(1, 0x30, 50) == Launcher::Choice::Continue);
    CHECK(launcher.OnMouse(1, 0x60, 0xBB) == Launcher::Choice::Start);
    CHECK(launcher.OnMouse(1, 0x60, 0xBC) == Launcher::Choice::None);
    CHECK(launcher.OnMouse(1, 0x240, 10) == Launcher::Choice::Exit);
    CHECK_EQ(launcher.lit[4], 5);
    CHECK(launcher.OnClose() == Launcher::Choice::Exit);
    swd3::Surface screen(640, 480);
    launcher.Draw(screen);
}
