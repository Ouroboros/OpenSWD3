#pragma once

#include <memory>
#include <string>

#include "audio/audio_manager.h"
#include "config/config.h"
#include "core/types.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/databases.h"
#include "game/game_state.h"
#include "gfx/graphics.h"
#include "input/controls.h"
#include "input/input.h"
#include "launcher/launcher.h"
#include "map/map_cache.h"

#include "movie/movie.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
#include "platform/display.h"
#include "platform/exe_resources.h"
#include "platform/glyph_rasterizer.h"
#include "platform/input_device.h"
#include "platform/message_box.h"
#include "platform/surface.h"
#include "platform/system_sound.h"
#include "random/random.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
#include "role/path_finder.h"
#include "talk/text_colors.h"

namespace swd3
{

// The root object: the window procedure and message loop of WinMain and the subsystems the original keeps in global
// objects. Built step by step along the start of the game.
class Game
{
public:
    static constexpr char kWindowTitle[] =
        "\xE8\xBB\x92\xE8\xBD\x85\xE5\x8A\x8D\xE5\x8F\x83 DVD";  // 0x0049FAC4 (Big5 there)
    static constexpr char kTitle[] = "\260a\301\325\274C\244T";  // 0x004A0F78 "軒轅劍三"
    static constexpr char kDirectInputFailed[] = "DirectInput \252\354\251l\244\306\277\371\273~%n";  // 0x004A0F5C
    static constexpr char kExecutable[] = "swd32.exe";
    static constexpr u32 kStartSound = 0x78;
    static constexpr int kFrameInterval = 0x23;
    static constexpr u32 kTitleMenu = 0x80000003;

    explicit Game(std::string installDirectory);

    int Run();

private:
    void Start();
    Launcher::Choice RunLauncher();
    void InitInstall();
    int FindCdDrives(std::string& firstDrive);
    void SizeCaches();
    void MakeDirectory(const char* name);
    void StartGraphics();
    void InitDrawKernel();
    bool InitDevices();
    void SetupFont(TextFont& font, int size);
    static u32 DiskFreeMegabytes();
    void Destroy();

    DialogErrorReporter errors;
    Display display;
    Surface screen;
    WindowInputDevice inputDevice;
    Input input;
    Graphics graphics;
    Painter painter;
    RleCodec codec;
    TswArchive archive;
    ActArchive acts;
    GlyphLibrary glyphs;
    LibraryGlyphRasterizer rasterizer;
    TextFont largeFont;   // 0x004AB998 (20 pixels)
    TextFont mediumFont;  // 0x004C9A28 (16 pixels)
    TextFont smallFont;   // 0x004A9ED0 (12 pixels)
    AudioDevice audioDevice;
    AudioManager audio;
    Movie movie;
    Random random;
    GameState state;
    Config config;
    Controls controls;
    MapCache mapCache;

    GameDatabases databases;
    PathNodePool pathNodes;
    TextColors textColors;
    ExeResources resources;
    SystemSound systemSound;
    std::unique_ptr<Surface> offscreen;  // 0x004ACBA0 the frame drawn by the game
    u16* drawSurface = nullptr;          // 0x004CD76C
    std::array<u8, 0x20> cdDrives = {};  // 0x004B74F8 the CD-ROM drive letters, 0xFF after the last
    i32 cdDriveCount = 0;                // 0x004ACEF8
    i32 unknown14 = 0;                   // the dword of Env.dat at +0x14
    u8 effectsOff = 0;                   // Env.dat +0x1B
    u32 deviceFlags = 0;                 // 0x004ACB90 bit 1: DirectInput failed
    i32 active = 0;                      // 0x004B7CAC 1 once the display runs
    i32 inactive = 0;                    // 0x004BABA4
    i32 frameCounter = 0;                // 0x004A94A0
    u32 menuState = 0;                   // 0x004B8740 bit 31: entered, low bits the menu
    i32 menuMode = 0;                    // 0x004FBAB0
    i32 storyState = 0;                  // 0x004CAEB0
    i32 unk4CAEBC = 0;
    i32 unk4CAEC0 = 0;
    u32 unk4AB8F8 = 0;  // 0x004AB8F8 a copy of the fill colour pair
    i32 unk4A99F8 = 0;  // 0x004A99F8
    bool quit = false;
};

}  // namespace swd3
