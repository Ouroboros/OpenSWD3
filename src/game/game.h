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
#include "battle/smooth/smooth_stage.h"
#include "config/config.h"
#include "core/types.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "draw/weave.h"
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
#include "platform/host_settings.h"
#include "platform/input_device.h"
#include "platform/message_box.h"
#include "platform/surface.h"
#include "platform/system_info.h"
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

namespace swd3::smooth
{

class HitFrame;
class TurnFrame;
class DamageFrame;
class BattleFrame;
class TalkFrame;

}  // namespace swd3::smooth

namespace swd3
{

// The root object: the window procedure and message loop of WinMain and the subsystems the original keeps in global
// objects. Built step by step along the start of the game.
class Game
{
    friend class CheatDialog;
    friend class Menu;
    friend class FieldView;
    friend class Talk;
    friend class RoleControl;
    friend class RolePath;
    friend class smooth::HitFrame;
    friend class smooth::TurnFrame;
    friend class smooth::DamageFrame;
    friend class smooth::BattleFrame;
    friend class smooth::TalkFrame;

public:
    static constexpr std::u8string_view WindowTitle = u8"軒轅劍參 DVD";                    // 0x0049FAC4
    static constexpr std::u8string_view Title = u8"軒轅劍三";                              // 0x004A0F78
    static constexpr std::u8string_view DirectInputFailed = u8"DirectInput 初始化錯誤%n";  // 0x004A0F5C
    static constexpr u32 StartSound = 0x78;
    static constexpr u32 WindowIcon = 0x65;    // the icon group of the window class (swd32.exe 0x00409F49)
    static constexpr u32 WindowCursor = 0x67;  // the cursor group (swd32.exe 0x00409F52)
    static constexpr int FrameInterval = 0x23;
    static constexpr int AutoRenderScale = 0;  // renderScaleSetting: the largest whole scale the window shows
    static constexpr int MaxRenderScale = MaxWeaveScale;  // DisplaySettings::MaximumRenderScale
    // swd32.exe 0x0049FBA8: the signed speeds of the nibbles of a map setting (8 and above count down from -7).
    static constexpr std::array<i32, 16> SpeedNibbles = {0, 1, 2, 3, 4, 5, 6, 7, 8, -7, -6, -5, -4, -3, -2, -1};

    // `menuState`: the menu shown (0: none), bit 31 while it is being entered.
    enum MenuNumber : u32
    {
        NoMenu = 0,
        MainMenu = 1,
        ItemChoiceMenu = 2,            // the items listed by talk command ItemMenu
        TitleScreenMenu = 3,           // the system page in mode 0
        SystemMenuA = 4,               // the system page in mode 1 (talk command EndGame)
        SystemMenuB = 5,               // the system page in mode 2
        NameEntryMenu = 6,             // the system page in mode 3
        MenuOnEquipment = 0x40000000,  // the main menu opens on its equipment page (the shop)
        MenuActsReset = 0x20000000,    // the shop resets its acts (ShopFrame)
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
        VideoPlaying = 0x20,  // a full-screen video plays
    };

    // `storyState`: 0 while the story runs, SaveSlotsState while the save slots are shown.
    static constexpr i32 StoryRunning = 0;
    static constexpr i32 SaveSlotsState = 3;
    static constexpr std::u8string_view UnknownPlace = u8"不知道";  // 0x0049FEA4

    explicit Game(std::u8string installDirectory);

    int Run(std::u8string_view commandLine);

    HostSettings host;               // the rebuild's swd3r.toml (set before Run)
    HangWatch* hangWatch = nullptr;  // told of every round of the main loop (set before Run)

    // The window as left at the end of Run (written back to swd3r.toml).
    Display::WindowState WindowState() const
    {
        return this->display.State();
    }

private:
    void Start();
    Launcher::Choice RunLauncher();
    void InitInstall();
    void ApplyHostPaths();
    void SizeCaches();
    void MakeDirectory(std::u8string_view name);
    void StartGraphics();
    void InitDrawKernel();
    bool InitDevices();
    void SetupFont(TextFont& font, int size);
    static u32 DiskFreeMegabytes();
    void ResetBattle();
    void NewGame();
    void RefreshCursor();
    void ShowFrame();
    // The rebuild's smooth motion (deviations.md): a map frame kept for the frames drawn until the next game frame
    // (shown at once without smooth motion), and one such frame drawn and shown when it is due.
    void ShowMapFrame();
    // The same for a frame of the main menu or the shop (Menu::PaintBetween).
    void ShowMenuFrame();
    bool PresentBetween();
    // The rebuild's continuous motion (deviations.md) up to now, at most to the due time of the next game frame.
    void AdvanceMotion();
    bool CopyToPrimary();
    // The display setup of the system tab (deviations.md): smoothMotion, vsync, frameRate and renderScaleSetting put
    // to use, and written to [display] of the program's settings file.
    void ApplyDisplay();
    void StoreDisplay() const;
    std::u8string MusicPath(int index) const;
    void PlayMusic(const std::u8string& path);
    void ResetSaveSlots();
    void StartStory(int withMap);
    void LoadFameFile();
    void SaveFameFile();
    void SaveScreenFrame();
    void SaveScreenKeys();
    void FlagViewKeys();
    void LeaveSaveScreen();
    void FreeSavePreviews();
    void ReloadSavePage();
    void DrawSaveScreen();
    void DrawFlagView();

    // What the slots page does after the mouse and the keys (SaveSlotsFrame).
    enum class SlotsStep
    {
        Keys,    // the keys are read, then Escape
        Chosen,  // a slot is taken
        Leave,   // the screen is left
        Done,    // nothing more this frame
    };

    void SaveSlotsFrame();
    SlotsStep SlotsMouse();
    SlotsStep TakeSlot();
    SlotsStep SlotsKeys();
    void SaveGame();
    void WriteSave(File& file);
    std::vector<u8> PackParty() const;
    void LoadGame();
    void ReadSave(File& file);
    void UnpackParty(std::span<const u8> data);
    void DrawSlotsPage();
    void ShopFrame();
    // The page of the shop drawn over the grey map (part of 0x0044EA60), also painted again between frames.
    void DrawShop();
    void OpenShop();
    void ResetShopActs();
    void CloseShop();
    void ShopBack();
    void ShopRowChanged(bool sound, u16 arrows);
    void ShopDown();
    void ShopUp();
    void ShopPageDown();
    void ShopPageUp();
    ItemNode* ShopChosenItem();
    void ShopLess();
    void ShopMore();
    void ShopConfirm();
    void ShopOpenList();
    void ShopSettle();
    void ShopAnswered();
    void ShopKeys();
    void ShopMouse();
    void ShopListMouse();
    void CompareEquipment(const ItemNode* item);
    void DrawEquipmentCompare();
    void DrawShopList(Pixel color);
    void DrawShopChosen(const ItemNode& item, Pixel color);
    void DrawWrappedText(std::string_view text, int x, int y, int lines, int width);
    void DrawTextInput(const TextInput& box);
    void MainFrame();
    void EndMainFrame();
    void RunTimers();
    void EnterBattle();
    void BattleMainFrame();
    void MusicCue();
    void AudioPump();
    void HandleEvent(const SDL_Event& event);
    void LoopFrame();
    static bool SDLCALL LiveResize(void* data, SDL_Event* event);
    void TakeScreenshot();
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
    void ReleaseRole(int index);
    void StoreRole(const Role& role);
    void FollowLeader(int index);
    static constexpr int SaveRoles = 1;  // LoadCity option: the roles of the map left are written back
    void LoadCity(int map, int x, int y, i16 act, i16 action, i16 direction, int options);
    void PlaceRoles(int map, int x, int y, i16 act, i16 action, i16 direction, int previous);
    void Destroy();
    void DrawPauseBanner();
    static int CheckStart();
    void Activate(int state);
    void FreeItemLists();
    void ReleaseBattle();
    void ReleaseGame();
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
    static bool KeyRepeats(const ButtonState& button);
    void BattleKeys();
    void DrawBattleMenu();
    void DrawCommandPanel(i32 nameIndex);
    void DrawAimPointers();
    void BattleEndFrame();
    void EndCheckFrame();
    int HitTargetFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index);
    void RippleHitFrame(BattleUnit& target, BattleUnit& attacker, int index, u16& targetX);
    bool DrawHitPicture(BattleUnit& target, BattleUnit& attacker, u32 act, int index, u16& x, u16& y, int& originX);
    void TakeHitBits(BattleUnit& target, BattleUnit& attacker, int index);
    int HitAllFrame(BattleUnit& target, BattleUnit& attacker, u32 act, int index, int all);
    void PlaceAllHit(BattleUnit& attacker, int width, int index, u16& delay);
    void NudgeAllHit(BattleUnit& target, BattleUnit& attacker, int all);
    void RaiseAllHit(BattleUnit& target, int all);
    void ShowAllHitDigits(BattleUnit& unit, i16 damage, u16& delay, i32& spirit, i32& stamina);
    void StrikeAllHit(BattleUnit& target, BattleUnit& attacker, int index, int all, u16& delay);
    int HitFrame();
    void FlashFrameWhite();
    void ClearHitDamage();
    void ClearHitTotals();
    void StepSweep(u32 units);
    void DrainToSpirit(BattleRole& role, i32& damage);
    int RoleHitFrame(int role);
    bool SweepFromRole(BattleRole& attacker, i16 target, u32 act);
    int EnemyHitFrame(int enemy);
    bool SweepFromEnemy(BattleEnemy& attacker, i16 target, u32 act);
    int EnemyActionFrame(int enemy, int target);
    int EnemyAttackAction(BattleEnemy& unit, int target);
    int CallEnemies(BattleEnemy& unit);
    int EnemyFrame(int enemy);
    void EnemyGatherSweep(BattleEnemy& unit);
    void EnemyPickTarget(BattleEnemy& unit, int enemy);
    bool AimedDown(i32 index) const;
    void EnemyChooseCommand(BattleEnemy& unit, int enemy);
    void ReturnHeldEscort(BattleRole& role, std::size_t index, bool dropCommand);
    void EnemyAfterAction(BattleEnemy& unit, int enemy);
    int RoleActionFrame(int role, int target);
    void AddSpecialPoints(int role, i32 points);
    void RoleHitLanded(int target);
    void RoleDefeated(int target);
    int RoleSpecialFrame(BattleRole& unit, int role, int target, u16 command);
    int ComboFrame(BattleRole& unit, int role, int target);
    int RoleCommandFrame(BattleRole& unit, int role, int target, u16 command);
    CounterList& ProficiencyList();
    int RoleAttackFrame(BattleRole& unit, int target);
    int RoleSpellFrame(BattleRole& unit, int role, u16 command);
    int AbsorbFrame(BattleRole& unit, int target);
    int ThrowTalismanFrame(BattleRole& unit, int role, int target);
    int SummonEscortFrame(BattleRole& unit, int role);
    int CoinTossFrame(BattleRole& unit, int role);
    int StealFrame(BattleRole& unit, int target);
    int PoseSweepFrame(BattleRole& unit);
    int ControlFrame(BattleRole& unit, int role, int target);
    int TimingAttackFrame(BattleRole& unit, int target);
    int AccessoryBuffFrame(BattleRole& unit, int target, int command);
    int TimerAttackFrame(BattleRole& unit, int target);
    int SelfHitFrame(BattleRole& unit, int command);
    int RoleFrame(int role);
    void RoleFrameEnd(int role);
    void RoleAutoCommand(BattleRole& unit, int role);
    void RoleOrderFrame(BattleRole& unit, int role);
    void RoleGatherSweep(BattleRole& unit, PendingOrder& order);
    void RoleTakeOrder(BattleRole& unit, int role);
    bool RoleActFrame(BattleRole& unit, int role);
    void RoleAfterAction(BattleRole& unit, int role);
    void RoleRetreatFrame(BattleRole& unit, int role);
    void UpdateUnits();
    int BattleFrame();
    void EndBattle();
    void DrawIntroField();
    std::vector<u8> PackSurface(Surface& surface);
    void TargetFrame();
    void ClearFrame();
    void SwayFrame(int step, int scale);
    void BattleIntro(u16 style);
    void StartBattleMusic();
    void FirstStrike();
    int FightTalkFrame();
    void FightTalkEnd();
    int FightTalkWait();
    int FightTalkWindow();
    int FightTalkMove();
    int FightTalkPause();
    int FightTalkHitPoints();
    int FightTalkOrder();
    BattleUnit& TalkUnit(i32 unit);
    u16 TalkCommandUnit() const;
    void SetTalkCommandUnit(u16 unit);
    int TalkCommandStarted();
    int FightTalkDefeat();
    int FightTalkCommand(int command);
    int FightTalkWin();
    int FightTalkLift();
    int FightTalkAllDone();
    int FightTalkMarks(bool set);
    int FightTalkRepeat();
    int FightTalkRandomJump();
    int FightTalkShift();
    int FightTalkSkill();
    void TalkStandingRole(i32& unit);
    int FightTalkNextBattle();
    int FightTalkLoot();
    int FightTalkUnits(u16 command);
    int FightTalkChoice();
    int FightTalkLines();
    int FightTalkLinesMode();
    void ShiftTalkUnits();
    int FightTalkCurve();
    int FightTalkSlide();
    int FightTalkSpeaker();
    int FightTalkTimedWindow();
    void ClearTalkMarks(bool actions);
    void SkipTalkText();
    int FightTalkMirror();
    int FightTalkEnemyCommand();
    int FightTalkLineWindow();
    int FightTalkFieldFade();
    int FightTalkObjects(bool take);
    int FightTalkSlideTo();
    int FightTalkCatch();
    int FightTalkDamage();
    int FightTalkMisc(u16 command);
    int FightTalkAddLoot();
    int FightTalkMusic();
    int FightTalkCounters();
    void DrawRolePanels();
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
    smooth::Stage battleStage;                   // the rebuild's smooth battle
    std::unique_ptr<ScriptObject> battleRipple;  // 0x00520DA4 the ripple of the ripple battle (unk53C03C)
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
    i32 flagCursor = 0;   // 0x004CC29C the flag chosen in the flag view
    u32 saveFrames = 0;   // 0x004CC2A4 frames since the save screen opened
    u16 unk4CC2AA = 0;    // 0x004CC2AA
    ActState slotsTitle;  // 0x004CC168 the title of the slots page (act 0x232A)
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
    i32 running = 0;    // 0x004CC2AC frames run (F8 toggles)
    u32 lastFrame = 0;  // 0x004CC2B0
    // The rebuild's smooth motion: whether the last game frame showed the map, the game frames run so far, when the
    // last game frame was due (SDL nanoseconds), the frame drawn between game frames and when the next one is due.
    bool smoothMotion = false;
    bool mapFrameShown = false;
    bool battleFrameShown = false;  // the last game frame showed a battle frame that can be drawn again
    // The rebuild's smooth battle checked: with smooth motion off, the battle frame's draws gathered into one scene
    // and drawn once the frame is decided, where they are (off: each drawn at once, in the original's order).
    bool battleDeferred = false;
    bool menuFrameShown = false;  // the last game frame showed a frame of the main menu or the shop
    u32 gameFrames = 0;
    u32 framesBetween = 0;
    u64 paintBetweenNs = 0;  // the time the frames between took to paint, in all and the longest
    u64 paintBetweenMostNs = 0;
    u64 frameDueNs = 0;
    u64 frameRatePeriodNs = 0;
    u64 nextDrawNs = 0;
    std::unique_ptr<Surface> smoothScreen;  // the frames between, at the render scale
    std::unique_ptr<Surface> menuScreen;    // the menu frames between
    int renderScaleSetting = 0;             // [display] render_scale (AutoRenderScale: by the window)
    bool vsync = true;                      // [display] vsync
    i64 frameRate = 0;                      // [display] frame_rate (0: no limit)
    u32 frameStart = 0;                     // 0x004AAECC
    u32 unk4C844C = 0;
    u32 unk4C8444 = 0;
    u32 unk4C8448 = 0;   // the time the last frame took
    i32 debugText = 0;   // 0x004CAE9C the debug lines (debug key F1)
    i32 debugCells = 0;  // 0x004CAEA0 the debug cell marks (debug key F2)
    i32 frameTime = 0;   // 0x0049E1C8
    i32 unk4A99FC = 0;
    i32 unk4C9A14 = 0;  // 0x004C9A14 only cleared (MainFrame)
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
    bool inLoopFrame = false;  // LoopFrame runs (the live-resize exposes come from inside it too)
};

}  // namespace swd3
