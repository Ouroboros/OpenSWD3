#include <SDL3/SDL.h>

#include <string>

#include "battle/battle_items.h"
#include "core/debug_log.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "text/big5.h"

namespace swd3
{

namespace
{

// The command line of an installation: the installation type digit, then the installation directory.
constexpr char kFirstDigit = '0';
// The settings written by an installation: sound and music 6, a 60 MB cache, 100, the effects on, text speed 2, battle
// speed 10.
constexpr Config::Settings kInstalledSettings = {6, 6, 0x3C, 0x64, 0, 2, 0xA};

// The pause banner.
constexpr std::u8string_view kPaused = u8"遊戲暫停  按F8繼續遊戲";  // swd32.exe 0x004A0360
constexpr int kBannerCenter = 0x140;
constexpr int kBannerCharWidth = 0xB;
constexpr int kBannerCharBox = 0x16;
constexpr int kBannerY = 0xE5;
constexpr int kBannerHeight = 0x16;
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr int kPanelStyle = 4;

// The fonts made again when the game becomes active (Game::SetupFont).
constexpr int kLargeFont = 0x14;
constexpr int kMediumFont = 0x10;
constexpr int kSmallFont = 0xC;
constexpr int kInBattle = 1;  // Game::inBattle

}  // namespace

// swd32.exe 0x00411C90: an installation's command line (its type digit, then the directory): the default keys, Env.dat
// written with the default settings (the original records the first CD drive too; the rebuild has none, see
// deviations.md); true then (the game does not start)
bool Game::InstallFromCommandLine(std::string_view commandLine)
{
    if (commandLine.empty())
    {
        return false;
    }

    this->config.SetDefaultKeys();
    this->config.installDirectory = std::string(commandLine.substr(1));
    this->config.installType = static_cast<u8>(commandLine.front() - kFirstDigit);
    this->config.Write(this->config.installDirectory, this->config.cdRoot, kInstalledSettings);
    return true;
}

// swd32.exe 0x00411FA0: the pause banner "遊戲暫停  按F8繼續遊戲" centred in a box, shown
void Game::DrawPauseBanner()
{
    const std::string text = Big5(kPaused);
    const int half = static_cast<int>(text.size()) / 2;
    const int x = kBannerCenter - kBannerCharWidth * half;
    this->menu.DrawBox(x, kBannerY, kBannerCharBox * half, kBannerHeight, 0, 0, 0, 0);
    const auto color = static_cast<Pixel>(this->painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    this->largeFont.DrawText(this->drawSurface, x, kBannerY, text, color, kPanelStyle);
    this->ShowFrame();
}

// swd32.exe 0x00425B40: the check of WM_CREATE; it always passes
int Game::CheckStart()
{
    return 1;
}

// swd32.exe 0x0040AB50: the game made active (`state` 1: the frame interval cleared, the painter set on the frame
// again, the three fonts made again, the battle surfaces again in battle, the frames at their interval) or inactive
// (0: the music and the sounds stopped, the battle surfaces and the fonts released). The original also restores or
// minimises its window and restores the lost DirectDraw surfaces, waiting for the primary's flip chain; the window
// stays as it is here.
void Game::Activate(int state)
{
    if (this->graphics.GetSurface(Graphics::SurfaceId::Primary) == nullptr)
    {
        return;
    }

    LogInfo(u8"game made %s", AsUtf8(state != 0 ? "active" : "inactive"));
    this->state.ClearFrameInterval();

    if (state != 0)
    {
        this->active = 1;

        if (this->offscreen != nullptr)
        {
            this->painter.Init(*this->offscreen);
        }

        this->SetupFont(this->largeFont, kLargeFont);
        this->SetupFont(this->mediumFont, kMediumFont);
        this->SetupFont(this->smallFont, kSmallFont);
        this->inactive = 0;

        if (this->inBattle == kInBattle)
        {
            CreateBattleSurfaces(this->battleState, this->graphics);
        }

        this->state.SetFrameInterval(Game::FrameInterval);
        return;
    }

    this->audio.StopMusic();
    this->audio.StopAllSounds();
    this->AudioPump();

    if (this->inBattle == kInBattle)
    {
        ReleaseBattleSurfaces(this->battleState);
    }

    this->largeFont.Shutdown();
    this->mediumFont.Shutdown();
    this->smallFont.Shutdown();
    this->active = 0;
    this->inactive = 1;
}

// swd32.exe 0x0040F410: the inventory, the four skill lists and the equipment freed
void Game::FreeItemLists()
{
    FreeItemList(this->party.inventory);

    for (ItemLink& skills : this->party.skills)
    {
        skills.reset();
    }

    for (ItemLink& slot : this->party.equipment)
    {
        slot.reset();
    }
}

// swd32.exe 0x0045EA30: the battle's projector released, the buffers of the ten battle roles and the eight enemies
// freed
void Game::ReleaseBattle()
{
    this->battleState.projector.Release();

    for (BattleRole& role : this->battleRoles)
    {
        role.Release();
    }

    for (BattleEnemy& enemy : this->battleEnemies)
    {
        enemy.Release();
    }
}

// swd32.exe 0x004251B0: the game released as its window is destroyed: the fonts, the save previews, the battle, the
// counters, the map's lists and roles, the item lists, the archives, the music and sounds, the surfaces and the
// devices, the files (failures logged), the databases, the map cache marked clean, the pointer shown. The original
// frees the map's 8-bit palette twice (its pointer 0x004CD764 and 0x004B7404) after an 8-bit map; it is freed once
// here.
void Game::ReleaseGame()
{
    LogInfo(u8"game released");
    this->largeFont.Shutdown();
    this->mediumFont.Shutdown();
    this->smallFont.Shutdown();
    this->FreeSavePreviews();
    this->ReleaseBattle();
    this->party.fame.Free();
    this->scene.triggers.next.reset();
    this->scene.FreeAnimations();
    this->scene.FreeEffects();
    this->scene.FreeSpritesA();
    this->scene.FreeSpritesB();
    this->state.clickRegions.Free();
    this->scene.FreeNodes();
    this->scene.FreeMarks();
    this->scene.ResetLeaves();
    this->field.overlays.reset();
    this->notices.head.next.reset();
    this->pathNodes.Release();
    this->state.ClearRoles();

    for (std::vector<Role*>& row : this->field.rows)
    {
        row.clear();
    }

    this->FreeItemLists();
    this->acts.CloseAll();
    this->archive.CloseAll();
    this->audio.StopMusic();
    this->audio.StopAllSounds();
    this->scene.unk4CAE78.clear();
    this->offscreen.reset();
    this->graphics.ReleaseSurfaces();
    this->graphics.ReleaseDevice();
    this->input.Release();

    if (!this->databases.maps.Close())
    {
        ErrorLog(u8"ReleaseGame : RoleHandle.Close");
    }

    if (!this->databases.paths.UnmapView(this->databases.pathData))
    {
        ErrorLog(u8"ReleaseGame : PathHandle.CloseView");
    }

    if (!this->databases.paths.CloseMapping())
    {
        ErrorLog(u8"ReleaseGame : PathHandle.CloseMapping");
    }

    if (!this->databases.paths.Close())
    {
        ErrorLog(u8"ReleaseGame : PathHandle.Close");
    }

    if (!this->databases.story.Close())
    {
        ErrorLog(u8"ReleaseGame : TalkHandle.Close");
    }

    if (!this->mapCache.unitFile.UnmapView(this->mapCache.view))
    {
        ErrorLog(u8"ReleaseGame : PixelMapping.CloseView");
    }

    if (!this->mapCache.unitFile.CloseMapping())
    {
        ErrorLog(u8"ReleaseGame : PixelMapping.CloseMappingView");
    }

    if (!this->mapCache.unitFile.Close())
    {
        ErrorLog(u8"ReleaseGame : PixelMapping.Close");
    }

    this->field.palette.clear();
    this->state.map.blocks.clear();
    this->state.map.tiles.clear();
    this->databases.mapsData.clear();
    this->databases.script.clear();
    this->field.regions.next.reset();
    this->mapCache.MarkClean();
    SDL_ShowCursor();
}

}  // namespace swd3
