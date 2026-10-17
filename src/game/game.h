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
#include "battle/fight_talk.h"
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
#include "role/role_control.h"
#include "role/role_path.h"
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
    friend class RoleControl;
    friend class RolePath;

public:
    static constexpr std::u8string_view WindowTitle = u8"軒轅劍參 DVD";                    // 0x0049FAC4
    static constexpr std::u8string_view Title = u8"軒轅劍三";                              // 0x004A0F78
    static constexpr std::u8string_view DirectInputFailed = u8"DirectInput 初始化錯誤%n";  // 0x004A0F5C
    static constexpr std::string_view Executable = "swd32.exe";
    static constexpr u32 StartSound = 0x78;
    static constexpr int FrameInterval = 0x23;

    // `menuState`: the menu shown (0: none), bit 31 while it is being entered.
    enum MenuNumber : u32
    {
        NoMenu = 0,
        MainMenu = 1,
        ItemChoiceMenu = 2,        // the items listed by talk command ItemMenu
        TitleScreenMenu = 3,       // the system page in mode 0
        SystemMenuA = 4,           // the system page in mode 1 (talk command EndGame)
        SystemMenuB = 5,           // the system page in mode 2
        NameEntryMenu = 6,         // the system page in mode 3
        MenuOnItems = 0x40000000,  // the main menu opens on its items page
        MenuEntering = 0x80000000,
        MenuNumberMask = 0x0FFFFFFF,
        MenuEnteredMask = 0x3FFFFFFF,
    };

    static constexpr u32 TitleMenu = MenuEntering | TitleScreenMenu;

    // The bits of `systemFlags`.
    enum SystemFlag : u32
    {
        UnkSystem1 = 1,       // the map frame is skipped (no setter is known)
        MovieOpened = 2,      // a map movie plays (talk command MapMovie)
        QuitRequested = 4,    // the game ends after the frame
        CheckingCd = 0x10,    // CheckCd waits for the disc; the map frame is skipped
        VideoPlaying = 0x20,  // a full-screen video plays
    };

    // The results of CheckCd.
    static constexpr int CdFound = 1;
    static constexpr int CdMissing = 2;
    static constexpr u8 NoDrive = 0xFF;  // ends cdDrives

    // `storyState`: 0 while the story runs, SaveSlotsState while the save slots are shown.
    static constexpr i32 StoryRunning = 0;
    static constexpr i32 SaveSlotsState = 3;
    static constexpr std::u8string_view UnknownPlace = u8"不知道";  // 0x0049FEA4

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
    static constexpr i16 KeepValue = -1;      // an UpdateRoleRecord argument that leaves its field
    static constexpr u32 ClockFrames = 0x14;  // frames between the ticks of Party::clock, less one
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
    static constexpr int SaveRoles = 1;  // LoadCity option: the roles of the map left are written back
    void LoadCity(int map, int x, int y, i16 act, i16 action, i16 direction, int options);
    void PlaceRoles(int map, int x, int y, i16 act, i16 action, i16 direction, int previous);
    void Destroy();
    void PlaySoundAt(u16 sound, int x, int y);
    u32 RollEncounter(int steps, int force);
    u32 LoadRoleRecord(Role& role, u16 id);
    void LeaveParty(u16 id, u16 path, u16 event, i32 act, i32 action, i32 direction, u16 flags);
    bool ChangeMap(int map, int x, int y, int act, int action, int direction, int options);
    void StartTimer(int minutes, int seconds, int event, int which);
    bool PlayVideo(std::size_t& cursor, u16& pc);
    int VideoPosition() const;
    void StopVideo();
    void VideoFrame();
    void StartBattle(u16 battle);
    void LoadBattleBackground(int picture, u16 animation, int x, int y, u16 scene);
    void SlideBattleIn(const Surface& incoming, const Surface& outgoing);
    void ShakeBattleBackground(int shake);
    void DrawBattleBackground(int shake);
    void PrepareDarkenedCopies();
    void ApplyDrain(BattleUnit& attacker, BattleUnit& target);
    void TryRetreat(int role);
    int DrainSpellFrame(BattleRole& role, BattleUnit& target);
    int MagicAttackFrame(BattleRole& role, BattleUnit& target);
    int EnemyAttackFrame(BattleUnit& enemy, BattleUnit& target);
    void EnemyStrike(BattleUnit& enemy, BattleUnit& target, u16 bits, u16& chance, bool attackFirst);
    void StartRoleTurn(int unit);
    void RetargetRoles(int role, int enemy);
    int RemoveDefeated(int index, int role);
    int PlayHitAct(BattleUnit& unit, u32 act, int index);
    int FadeHitAct(BattleUnit& unit, u32 act, i32 action, int index);
    int WalkLine(i32& x, i32& y, int fromX, int fromY, int toX, int toY, int speed, int index);
    void SortActingDrawOrder();
    int PushUnits(u16 picture, int restart);
    void ShiftUnits(int shift);
    int FadeFieldOut();
    void ReturnBattleItems();
    void CheckBattleEnd();
    void DrawBattlePrompt();
    void QuakeFrame();
    void FieldFadeFrame(int counting);
    int CheckStoneSides();
    void TakeSlotRecord(BattleSlotRecord& taken);
    int DamageNumbersFrame(int index, int role);
    void ApplyHits();
    void NextEnemyTurn();
    void NextRoleTurn();
    void ScrollListBack();
    void ScrollListForward();
    void AutoBattleTurn();
    bool CanSelectRole(int unit);
    static std::size_t PanelPlace(int unit);
    int NextSelectableRole(int unit);
    int PreviousSelectableRole(int unit);
    void SelectRoleUnit(int unit);
    void SelectPreviousRole();
    void SelectNextRole();
    void MenuRight();
    void MenuLeft();
    void BuildCommandMenu();
    void DrawCommandMenu(int x, int y, int cursor);
    void AnnounceMastery();
    void MenuCancel();
    void DrawListTabs(int x, int y);
    void DrawThrowList(int x, int y, int cursor, int top);
    void DrawRefineList(int x, int y, int cursor);
    void DrawSpecialList(int x, int y, int cursor);
    void DrawEscortList(int x, int y);
    void DrawTargetTag(int x, int y);
    void DrawControlMenu(int x, int y, int cursor);
    void DrawSpellList(int x, int y, int cursor, int top);
    void DrawItemList(int x, int y, int cursor, int top);
    void DrawVictoryPanel();
    void DrawLevelUpPanel();
    void CheckLevelUps();
    void DrawNewSpell();
    void DrawNewSpecial();
    void DrawLearnedBox(std::u8string_view line, bool sound);
    void LearnSpecials();
    void DrawLevelUpStats();
    void MenuUp();
    bool CommandEnabled(i32 command) const;
    static void StepWrapped(i32& place, int step, i32 count);
    void AimStep(int step);
    void MenuDown();
    int BattleDebugKeys();
    void DrawBattleDebug();
    int MouseHover();
    int HoverRefineCell();
    int HoverTarget();
    void MenuConfirm();
    bool PrepareOrder();
    void AimAtParty();
    void OpenTargets();
    void EndChoice();
    void ConfirmCommand();
    void ConfirmSpell();
    void ConfirmTarget();
    void ConfirmItem();
    void CallEscort(BattleRole& role);
    void ConfirmEscort();
    void ConfirmTalk();
    void ConfirmSpecial();
    void ConfirmThrow();
    void ConfirmRefine();
    void ConfirmEnd();
    void ConfirmKey();
    void DrawRolePanels();
    template <typename Next>
    void StepPanelGauge(i32& shown, i32& target, Next next, int picture, int x, int y, i32 downNear, i32 downFar);
    void MeasureUnits();

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
    RoleControl roleControl;
    RolePath rolePath;

    GameDatabases databases;
    PathNodePool pathNodes;
    TextColors textColors;
    Database database;
    Party party;
    Scene scene;
    BattleState battleState;
    BattleHud battleHud;
    BattleMessages battleMessages;
    FightTalk fightTalk;
    BattleServices battleServices;
    std::array<BattleRole, BattleState::RoleUnits> battleRoles;   // 0x005029D0
    std::array<BattleEnemy, BattleState::Enemies> battleEnemies;  // 0x00525508
    ActDrawer actDrawer;
    Notices notices;
    i32 unk4B7518 = 0;          // the step of the camera and the controlled role across (-1, 0, 1)
    i32 unk4B7920 = 0;          // and down
    i32 unk4A9488 = 0;          // the step of the controlled role alone across
    i32 unk4A948C = 0;          // and down
    i32 lastPointerAction = 0;  // 0x004CC2E0
    Menu menu;
    SaveSlots saveSlots;
    std::unique_ptr<TextInput> textInput;  // 0x004CACD8 the text box the keyboard goes to (with flag 0x50)
    ActState unk4CC200;
    u8 unk4CC160 = 0;
    u8 unk4CC161 = 0;
    i32 unk4CC298 = 0;
    i32 unk4CB240 = 0;
    u32 systemFlags = 0;   // 0x004B7A9C SystemFlag bits
    i32 counterAlpha = 0;  // 0x004C9A24
    i32 unk4A9484 = 0;
    static constexpr u32 ScriptedBattle = 0x80000000;  // in `battleRequest`: started by a talk command
    i32 battleRequest = 0;                             // 0x004A94AC a battle to start (its number in the low word)
    i32 inBattle = 0;                                  // 0x004CAEB4 a battle from the map runs
    std::vector<u16> unk4B7928;                        // the words of talk command 0x85 for menu 2, ended by 0
    i32 currentMap = 0;                                // 0x004A9A10 the number of the map shown
    i32 unk4AB990 = 0;
    i32 unk4ACEF4 = 0;
    i32 unk4B7BC4 = 0;
    TextEntryState textEntry;
    i32 running = 0;     // 0x004CC2AC frames run (F8 toggles)
    u32 lastFrame = 0;   // 0x004CC2B0
    u32 frameStart = 0;  // 0x004AAECC
    u32 unk4C844C = 0;
    u32 unk4C8444 = 0;
    u32 unk4C8448 = 0;   // the time the last frame took
    i32 debugText = 0;   // 0x004CAE9C the debug lines (debug key F1)
    i32 debugCells = 0;  // 0x004CAEA0 the debug cell marks (debug key F2)
    i32 frameTime = 0;   // 0x0049E1C8
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
    std::unique_ptr<Surface> offscreen;   // 0x004ACBA0 the frame drawn by the game
    u16* drawSurface = nullptr;           // 0x004CD76C the locked frame (the target of the painter and the fonts)
    std::array<u8, 0x20> cdDrives = {};   // 0x004B74F8 the CD-ROM drive letters, 0xFF after the last
    i32 cdDriveCount = 0;                 // 0x004ACEF8
    i32 unknown14 = 0;                    // the dword of Env.dat at +0x14
    u8 effectsOff = 0;                    // Env.dat +0x1B
    u32 deviceFlags = 0;                  // 0x004ACB90 bit 1: DirectInput failed
    i32 active = 0;                       // 0x004B7CAC 1 once the display runs
    i32 inactive = 0;                     // 0x004BABA4
    i32 frameCounter = 0;                 // 0x004A94A0
    u32 menuState = 0;                    // 0x004B8740 bit 31: entered, low bits the menu
    i32 menuMode = 0;                     // 0x004FBAB0
    i32 storyState = Game::StoryRunning;  // 0x004CAEB0
    i32 unk4CAEBC = 0;
    i32 unk4CAEC0 = 0;
    bool quit = false;
};

}  // namespace swd3
