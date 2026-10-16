#pragma once

#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "audio/audio_manager.h"
#include "battle/battle_enemy.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_role.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "config/config.h"
#include "core/types.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/databases.h"
#include "game/game_state.h"
#include "game/notices.h"
#include "game/scene.h"
#include "gfx/graphics.h"
#include "input/controls.h"
#include "input/input.h"
#include "launcher/launcher.h"
#include "map/field.h"
#include "map/field_view.h"
#include "map/map_cache.h"
#include "menu/menu.h"
#include "talk/talk.h"

#include "movie/movie.h"
#include "org/database.h"
#include "party/party.h"
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
#include "save/save_slots.h"
#include "talk/text_colors.h"
#include "text/big5.h"
#include "text/text_input.h"
#include "ui/act_draw.h"

namespace swd3
{

// The root object: the window procedure and message loop of WinMain and the subsystems the original keeps in global
// objects. Built step by step along the start of the game.
class Game
{
    friend class Menu;
    friend class FieldView;
    friend class Talk;

public:
    static constexpr std::u8string_view kWindowTitle = u8"軒轅劍參 DVD";                    // 0x0049FAC4
    static constexpr std::u8string_view kTitle = u8"軒轅劍三";                              // 0x004A0F78
    static constexpr std::u8string_view kDirectInputFailed = u8"DirectInput 初始化錯誤%n";  // 0x004A0F5C
    static constexpr std::string_view kExecutable = "swd32.exe";
    static constexpr u32 kStartSound = 0x78;
    static constexpr int kFrameInterval = 0x23;
    static constexpr u32 kTitleMenu = 0x80000003;
    static constexpr std::u8string_view kUnknownPlace = u8"不知道";  // 0x0049FEA4

    explicit Game(std::string installDirectory);

    int Run();

private:
    void Start();
    Launcher::Choice RunLauncher();
    void InitInstall();
    int FindCdDrives(std::string& firstDrive);
    void SizeCaches();
    void MakeDirectory(std::string_view name);
    void StartGraphics();
    void InitDrawKernel();
    bool InitDevices();
    void SetupFont(TextFont& font, int size);
    static u32 DiskFreeMegabytes();
    void ResetBattle();
    void NewGame();
    void RefreshCursor();
    void ShowFrame();
    std::string MusicPath(int index) const;
    void PlayMusic(const std::string& path);
    void ResetSaveSlots();
    void StartStory(int withMap);
    void LoadFameFile();
    void DrawTextInput(const TextInput& box);
    void MainFrame();
    void MusicCue();
    void AudioPump();
    void HandleEvent(const SDL_Event& event);
    void SendText(TextMessage message, u32 parameter);
    void ShowLoading(int percent);
    void MusicZone(int map);
    bool LoadRoleSound(Role& role);
    void BuildTriggers(MapTrigger& head);
    void InitRole(Role& role);
    bool MapName(int map, std::string& name) const;
    bool UpdateRoleRecord(
        u16 id,
        i16 act,
        i16 action,
        i16 direction,
        i16 x,
        i16 y,
        i16 event,
        i16 talk,
        i16 setBits,
        i16 clearBits,
        i16 map
    );
    void SaveRoleStates(i16 x, i16 y);
    void StoreRole(const Role& role);
    void FollowLeader(int index);
    int CheckCd(const std::string& root, int& drive);
    static bool ProbeFile(const std::string& path);
    void LoadCity(int map, int x, int y, i16 act, i16 action, i16 direction, int options);
    void PlaceRoles(int map, int x, int y, i16 act, i16 action, i16 direction, int previous);
    void Destroy();
    void PlaySoundAt(u16 sound, int x, int y);

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
    crt::Random crtRandom;
    GameState state;
    Config config;
    Controls controls;
    MapCache mapCache;
    Field field;
    FieldView fieldView;
    Talk talk;

    GameDatabases databases;
    PathNodePool pathNodes;
    TextColors textColors;
    Database database;
    Party party;
    Scene scene;
    BattleState battleState;
    BattleHud battleHud;
    BattleMessages battleMessages;
    BattleServices battleServices;
    std::array<BattleRole, 10> battleRoles;    // 0x005029D0
    std::array<BattleEnemy, 8> battleEnemies;  // 0x00525508
    ActDrawer actDrawer;
    Notices notices;
    i32 unk4B7518 = 0;
    i32 unk4B7920 = 0;
    i32 lastPointerAction = 0;  // 0x004CC2E0
    Menu menu;
    SaveSlots saveSlots;
    std::unique_ptr<TextInput> textInput;  // 0x004CACD8 the text box the keyboard goes to (with flag 0x50)
    ActState unk4CC200;
    u8 unk4CC160 = 0;
    u8 unk4CC161 = 0;
    i32 unk4CC298 = 0;
    i32 unk4CB240 = 0;
    u32 systemFlags = 0;   // 0x004B7A9C bit 2: quit at the end of the frame
    i32 counterAlpha = 0;  // 0x004C9A24
    i32 unk4A9484 = 0;
    i32 unk4A9A10 = 0;
    i32 unk4AB990 = 0;
    i32 unk4ACEF4 = 0;
    i32 unk4B7BC4 = 0;
    TextEntryState textEntry;
    i32 running = 0;     // 0x004CC2AC frames run (F8 toggles)
    u32 lastFrame = 0;   // 0x004CC2B0
    u32 frameStart = 0;  // 0x004AAECC
    u32 unk4C844C = 0;
    u32 unk4C8444 = 0;
    i32 frameTime = 0;  // 0x0049E1C8
    i32 unk4A99FC = 0;
    i32 unk4CAD24 = 0;
    i32 loadingPercent = 0;  // 0x004CC2B8
    i32 loadingUnk4CC2C0 = 0;
    i32 loadingRunner = 0;  // 0x004CC2D0 the act running along the bar (1 or 2)
    i32 loadingRed = 0;     // 0x004CC2BC
    i32 loadingGreen = 0;   // 0x004CC2B4
    i32 loadingBlue = 0;    // 0x004CC2CC
    i32 loadingUnk4CC2C4 = 0;
    ActState loadingAct;  // 0x004B7408
    ExeResources resources;
    SystemSound systemSound;
    std::unique_ptr<Surface> offscreen;  // 0x004ACBA0 the frame drawn by the game
    u16* drawSurface = nullptr;          // 0x004CD76C the locked frame (the target of the painter and the fonts)
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
    bool quit = false;
};

}  // namespace swd3
