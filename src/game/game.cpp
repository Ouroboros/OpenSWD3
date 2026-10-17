#include "game/game.h"
#include "core/format.h"

#include <array>
#include <filesystem>
#include <limits>
#include <span>
#include <system_error>
#include <utility>

#include <SDL3/SDL.h>

#include "crt/msvc.h"
#include "platform/file_system.h"
#include "platform/system_info.h"
#include "save/save_games.h"
#include "text/big5.h"
#include "text/dbcs.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr u32 kLeftButton = 0x1;   // MK_LBUTTON
constexpr u32 kRightButton = 0x2;  // MK_RBUTTON
constexpr u32 kShiftKey = 0x4;     // MK_SHIFT
constexpr u32 kControlKey = 0x8;   // MK_CONTROL
constexpr u32 kMiddleButton = 0x10;
constexpr Uint32 kLauncherWait = 16;
constexpr int kEffectsOffFlag = GameState::PlainScreen;
constexpr int kMinimumTileCache = 0x400000;
constexpr int kMaximumTileCache = 0x1000000;
constexpr int kActCache = 0x80000;
constexpr Uint32 kLowDiskPause = 0x400;
constexpr int kDiskReserve = 0x40;
constexpr std::u8string_view kDrawKernelFailed = u8"DrawKernel init Failed.";
constexpr std::u8string_view kDirectDrawFailed = u8"DDraw init Failed.";
constexpr std::u8string_view kTimerFailed = u8"mmTimer init Failed.";
constexpr std::u8string_view kErrorTitle = u8"SWD3";
constexpr u32 kStopVideoButtons = kLeftButton | kRightButton | kControlKey | kMiddleButton;

// The defaults of the settings (before Env.dat).
constexpr i32 kDefaultBattleSpeed = 0xB;
constexpr u64 kTileCacheShare = 6;  // a sixth of the physical memory
constexpr int kMegabyteShift = 20;
constexpr u32 kMegabyte = 1u << kMegabyteShift;
constexpr int kStartPointerX = 0x1E0;
constexpr int kStartPointerY = 0x168;
constexpr u32 kInputFailed = 2;  // Game::deviceFlags
constexpr int kDisplayBits = 16;

// The fonts: MingLiU at three sizes; the advances on the map and in the menus.
constexpr int kLargeFontSize = 20;
constexpr int kMediumFontSize = 16;
constexpr int kSmallFontSize = 12;
constexpr int kLargeFontAdvance = 24;
constexpr int kMediumFontAdvance = 18;
constexpr int kMapLargeAdvance = 22;
constexpr int kMapSmallAdvance = 16;
constexpr i32 kTalkUnit = 0xB;

// A new game.
constexpr std::array<int, 5> kNewGameFlags = {
    GameState::Running,
    GameState::Music,
    GameState::SoundEffects,
    GameState::NoPointer,
    GameState::NoLoadingScreen,
};

constexpr std::array<int, 19> kNewGameClearedFlags = {
    GameState::Unk0,
    GameState::DebugReports,
    GameState::Weather,
    GameState::Leaves,
    GameState::HeatHaze,
    GameState::Lightning,
    GameState::NoChoicePointer,
    GameState::HideActRoles,
    GameState::Encounters,
    GameState::UnkD,
    GameState::UnkE,
    GameState::OverlaysFirst,
    GameState::Timer,
    GameState::TimerExpired,
    GameState::TimerEvent,
    GameState::Spotlight,
    GameState::NoSlide,
    GameState::Fireflies,
    GameState::Unk17,
};

constexpr i32 kStartGold = 100;
constexpr i32 kStartUnk4A94B0 = 0x40000000;
constexpr i32 kStartUnk4ACDB8 = 3;
constexpr i32 kStartUnk4ACBC8 = 2;
constexpr int kStartWalkSpeed = 0x10;
constexpr int kUnk232EAct = 0x232E;
constexpr int kUnk232EActs = 4;
constexpr int kUnk4C83A8Action = 8;
constexpr i32 kUnk4ACAF8Action = 0x38;
constexpr u32 kGroupSlackBytes = 0x80;  // the encounter groups get 0x10 spare words
constexpr u16 kOpeningEvent = 100;      // the leader's event of a new story

// The menu button in the top right corner.
constexpr i32 kButtonDelay = 0x10;
constexpr u32 kButtonLeft = 0x262;
constexpr u32 kButtonBottom = 0x18;
constexpr u16 kUnkLeader20Limit = 8;
constexpr i32 kCounterHidden = 0x400;
constexpr i32 kCounterOpaque = 0x10;

constexpr int kStreamVolume = 0x80;
constexpr int kPlayOnce = 1;
constexpr std::size_t kFameWords = 0x200;  // Fame.dat
constexpr int kFameCounters = 500;

// The name box.
constexpr u32 kNameBoxPicture = 0x2449;
constexpr int kNameBoxLeft = 0x6C;
constexpr int kNameBoxUp = 0x2E;
constexpr std::size_t kNameBytes = 0x40;
constexpr int kNameRed = 0x15;
constexpr int kNameGreen = 0xF;
constexpr int kNameBlue = 8;
constexpr int kNameGlyphWidth = 0xB;
constexpr int kCursorHeight = 0x16;
constexpr int kCursorRed = 0x14;
constexpr int kCursorGreen = 0xD;

// The wParam of a mouse message: the buttons and the modifier keys held.
u32 MouseKeys(SDL_MouseButtonFlags buttons)
{
    u32 keys = 0;
    keys |= (buttons & SDL_BUTTON_LMASK) != 0 ? kLeftButton : 0;
    keys |= (buttons & SDL_BUTTON_RMASK) != 0 ? kRightButton : 0;
    keys |= (buttons & SDL_BUTTON_MMASK) != 0 ? kMiddleButton : 0;
    const SDL_Keymod modifiers = SDL_GetModState();
    keys |= (modifiers & SDL_KMOD_SHIFT) != 0 ? kShiftKey : 0;
    keys |= (modifiers & SDL_KMOD_CTRL) != 0 ? kControlKey : 0;
    return keys;
}

}  // namespace

Game::Game(std::string installDirectory)
    : screen(Display::Width, Display::Height),
      inputDevice(this->display),
      input(this->inputDevice),
      graphics(this->display, this->errors),
      codec(this->painter),
      archive(this->codec, this->errors),
      acts(this->errors),
      rasterizer(this->glyphs),
      largeFont(this->rasterizer),
      mediumFont(this->rasterizer),
      smallFont(this->rasterizer),
      audio(this->audioDevice),
      mapCache(this->config, this->painter),
      field(this->state, this->mapCache, this->config, this->codec),
      fieldView(*this),
      talk(*this),
      roleControl(*this),
      rolePath(*this),
      database(installDirectory),
      battleHud(this->painter, this->archive, this->acts, this->random, this->audio),
      battleMessages(
          this->painter,
          this->archive,
          this->acts,
          this->battleHud,
          this->battleState,
          this->mediumFont,
          this->largeFont
      ),
      battleServices{
          this->battleState,
          this->random,
          this->archive,
          this->acts,
          this->painter,
          this->audio,
          this->battleHud,
          this->battleMessages,
          this->config,
          this->party,
          this->crtRandom,
          this->drawSurface,
      },
      actDrawer(this->acts, this->archive, this->painter),
      menu(*this)
{
    this->config.installDirectory = std::move(installDirectory);
    this->archive.SetMapPictureSource(&this->field);
    this->glyphs.Load(GlyphLibraryPath());
}

// swd32.exe 0x00409EC0: creates the window, sends itself the start message and runs the message loop
int Game::Run()
{
    if (!this->display.Open(Utf8Chars(Game::WindowTitle)))
    {
        return 0;
    }

    this->Start();
    this->crtRandom.Seed(static_cast<u32>(crt::Time()));
    this->random.Seed(static_cast<u32>(crt::Time()));
    this->running = 1;

    while (!this->quit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            this->HandleEvent(event);
        }

        if (this->state.GameFlag(GameState::TextBox) != SDL_TextInputActive(this->display.Window()))
        {
            if (this->state.GameFlag(GameState::TextBox))
            {
                SDL_StartTextInput(this->display.Window());
            }
            else
            {
                SDL_StopTextInput(this->display.Window());
            }
        }

        if (this->running != 0 && (this->systemFlags & Game::VideoPlaying) != 0)
        {
            this->VideoFrame();
            this->AudioPump();
        }
        else if (this->running != 0 && (this->systemFlags & Game::UnkSystem1) == 0 && this->inactive == 0)
        {
            this->MainFrame();
        }

        if (this->active == 0)
        {
            this->display.Present(this->screen);
        }

        SDL_Delay(1);
    }

    return 0;
}

// swd32.exe 0x0040A0D0 (message 0x404): the start sound, the default keys, the installation, the saved game flag and
// the launcher, then the display for a new or continued game
void Game::Start()
{
    if (!this->resources.Open(Concat({this->config.installDirectory, Game::Executable})))
    {
        this->errors.ShowError(u8"swd3r", u8"swd32.exe not found in the game directory");
        this->quit = true;
        return;
    }

    this->systemSound.Play(this->resources.Find("WAVE", Game::StartSound));
    this->config.SetDefaultKeys();
    this->InitInstall();
    this->state.ClearGameFlag(Launcher::SavedGameFlag);

    if (SaveGames::AnyExists(this->config.installDirectory))
    {
        this->state.SetGameFlag(Launcher::SavedGameFlag);
    }

    switch (this->RunLauncher())
    {
        case Launcher::Choice::Continue:
            this->StartGraphics();
            this->menuState = Game::NoMenu;
            this->menuMode = Menu::TitleLoadMode;
            this->storyState = Game::SaveSlotsState;
            this->unk4CAEBC = 1;
            this->unk4CAEC0 = 1;
            this->ResetSaveSlots();
            this->saveSlots.FindNewest(this->config.installDirectory, this->painter);
            break;

        case Launcher::Choice::Start:
            this->StartGraphics();
            break;

        case Launcher::Choice::Exit:
            this->Destroy();
            break;

        default:
            break;
    }
}

// WM_DESTROY sent to the main window.
void Game::Destroy()
{
    this->quit = true;
}

// DialogBoxParamA(0x6B): the launcher drawn in the game window until it ends
Launcher::Choice Game::RunLauncher()
{
    Launcher launcher(this->resources, this->state, this->systemSound, this->config.installDirectory);
    launcher.LoadPictures();
    launcher.Init();
    Launcher::Choice choice = Launcher::Choice::None;

    while (choice == Launcher::Choice::None)
    {
        SDL_Event event;

        if (SDL_WaitEventTimeout(&event, static_cast<Sint32>(kLauncherWait)))
        {
            do
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    choice = launcher.OnClose();
                }
                else if (
                    event.type == SDL_EVENT_MOUSE_MOTION ||
                    (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT)
                )
                {
                    const float windowX = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.x : event.button.x;
                    const float windowY = event.type == SDL_EVENT_MOUSE_MOTION ? event.motion.y : event.button.y;
                    const LogicalPoint point = this->display.WindowToLogical(windowX, windowY);
                    const SDL_MouseButtonFlags held = event.type == SDL_EVENT_MOUSE_MOTION
                                                          ? event.motion.state
                                                          : SDL_GetMouseState(nullptr, nullptr) | SDL_BUTTON_LMASK;
                    const Launcher::Choice result = launcher.OnMouse(MouseKeys(held), point.x, point.y);

                    if (result != Launcher::Choice::None)
                    {
                        choice = result;
                    }
                }
                else
                {
                    this->display.HandleEvent(event);
                }
            } while (choice == Launcher::Choice::None && SDL_PollEvent(&event));
        }

        launcher.Draw(this->screen);
        this->display.Present(this->screen);
    }

    this->screen.Fill(0);
    return choice;
}

// swd32.exe 0x00425040: the default settings, the install directory and the CD drives, Env.dat, the cache sizes, the
// map cache check and the directories of the installation
void Game::InitInstall()
{
    Config& settings = this->config;
    settings.installType = Config::FullInstall;
    settings.soundVolume = Config::DefaultVolume;
    settings.musicVolume = Config::DefaultVolume;
    settings.cacheSize = Config::DefaultCacheMegabytes;
    settings.textSpeed = Config::DefaultTextSpeed;
    settings.battleSpeed = kDefaultBattleSpeed;
    this->cdDriveCount = this->FindCdDrives(settings.cdRoot);
    settings.Load(this->unknown14, this->effectsOff);
    this->SizeCaches();
    this->mapCache.Check(MapCache::CheckSessionMark);
    this->MakeDirectory("Save");
    this->MakeDirectory("Data");
    this->MakeDirectory("Music");
    this->MakeDirectory("ScrnShot");
    this->MakeDirectory("Video");
    this->state.SetGameFlag(kEffectsOffFlag);

    if (this->effectsOff == 0)
    {
        this->state.ClearGameFlag(kEffectsOffFlag);
    }
}

// swd32.exe 0x004242B0: the CD-ROM drives a: .. z: (their letters, 0xFF after the last); the root of the first goes to
// `firstDrive`
int Game::FindCdDrives(std::string& firstDrive)
{
    this->cdDrives.fill(Game::NoDrive);
    int count = 0;

    for (const char letter : CdRomDrives())
    {
        if (count == 0)
        {
            firstDrive = std::string(1, letter) + ":\\";
        }

        this->cdDrives[static_cast<std::size_t>(count)] = static_cast<u8>(letter);
        ++count;
    }

    return count;
}

// swd32.exe 0x00424330: a sixth of the physical memory (4 to 16 MB) for the picture cache, 512 KB for the scripts
void Game::SizeCaches()
{
    int bytes = static_cast<int>(TotalPhysicalMemory() / kTileCacheShare);

    if (bytes < kMinimumTileCache)
    {
        bytes = kMinimumTileCache;
    }
    else if (bytes > kMaximumTileCache)
    {
        bytes = kMaximumTileCache;
    }

    this->archive.SetCacheLimit(bytes);
    this->acts.SetCacheLimit(kActCache);
}

// swd32.exe 0x00425150: creates <install directory><name> unless it exists. The original enters an existing directory
// as the current one, which no path of the game depends on.
void Game::MakeDirectory(std::string_view name)
{
    std::error_code error;
    std::filesystem::create_directory(ResolvePath(Concat({this->config.installDirectory, name})), error);
}

// swd32.exe 0x00424910
void Game::StartGraphics()
{
    this->InitDrawKernel();
}

// swd32.exe 0x00424B90: the devices, the drawing core and the fonts, the colour tables, the databases and the starting
// state; then the title menu runs
void Game::InitDrawKernel()
{
    this->inactive = 0;
    this->frameCounter = 0;
    SDL_HideCursor();
    this->InitDevices();
    this->input.SetSensitivity(2.0);
    this->input.SetMousePosition(kStartPointerX, kStartPointerY);
    this->audio.SetMusicVolume(this->config.musicVolume);

    if (this->offscreen == nullptr || !this->painter.Init(*this->offscreen))
    {
        this->errors.ShowError(kDrawKernelFailed, kErrorTitle);
        this->Destroy();
        return;
    }

    const u32 cachedMegabytes = (this->mapCache.Check(MapCache::CheckColorMasks) + kMegabyte - 1) >> kMegabyteShift;

    if (static_cast<u32>(this->config.cacheSize) - cachedMegabytes + kDiskReserve >= Game::DiskFreeMegabytes())
    {
        SDL_Delay(kLowDiskPause);
        PlaySystemBeep();
    }

    this->mapCache.MarkDirty();
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);

    for (u16& mask : this->config.channelMasks)
    {
        this->painter.ConvertPixels(std::span<u16>(&mask, 1));
    }

    this->graphics.SetClearColor(this->painter.FillColorPair());
    this->SetupFont(this->largeFont, kLargeFontSize);
    this->SetupFont(this->mediumFont, kMediumFontSize);
    this->SetupFont(this->smallFont, kSmallFontSize);
    this->talk.unit = kTalkUnit;
    this->painter.SetClip(0, 0, Display::Width, Display::Height);
    this->textColors.Convert(this->painter);
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);

    if (this->state.SetFrameInterval(Game::FrameInterval) == 0)
    {
        this->errors.ShowError(kErrorTitle, kTimerFailed);
        this->inactive = 1;
        this->Destroy();
        return;
    }

    if (!this->databases.Open(this->config.installDirectory, this->errors))
    {
        this->Destroy();
    }

    this->databases.script.assign(GameDatabases::ScriptBytes, 0);
    this->databases.LoadMaps();
    this->pathNodes.Init();
    this->NewGame();
    this->menu.InitActs();
    this->state.ClearWorld();
    this->menuState = Game::TitleMenu;
}

// swd32.exe 0x00424EF0: DirectInput, Miles with the sound effects, the MIDI and stream players, the display (640 x 480
// in 16 bits) with the frame surface, and Bink
bool Game::InitDevices()
{
    this->deviceFlags = 0;

    if (this->input.Init() != 0)
    {
        this->deviceFlags |= kInputFailed;
        this->errors.ShowError(Game::Title, Game::DirectInputFailed);
        this->Destroy();
        return false;
    }

    this->audio.System().Startup(this->config.installDirectory);
    this->audio.Effects().Open(this->config.installDirectory);
    this->audio.Midi().Open(this->audio.Effects().HasDriver() ? 1 : 0);
    this->audio.Streams().Open();

    if (!this->graphics.Init(Graphics::FullScreen, Display::Width, Display::Height, kDisplayBits))
    {
        this->errors.ShowError(kDirectDrawFailed, kErrorTitle);
        this->Destroy();
        return false;
    }

    this->offscreen = this->graphics.CreateOffscreen(Display::Width, Display::Height);
    this->active = 1;
    Movie::Init(false, this->audio.Effects().HasDriver());
    return true;
}

// swd32.exe 0x0040F340: one of the three fonts (MingLiU at `size`) on the display's surfaces; 20 and 16 pixel text
// advances by 24 and 18
void Game::SetupFont(TextFont& font, int size)
{
    font.Shutdown();
    font.Init(this->graphics);
    font.SetTarget(Display::Width, Display::Height);
    font.SetFont(TextFont::DefaultFace, size);

    if (size == kLargeFontSize)
    {
        font.SetAdvance(kLargeFontAdvance);
    }
    else if (size == kMediumFontSize)
    {
        font.SetAdvance(kMediumFontAdvance);
    }

    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(0);
}

// swd32.exe 0x00411D90: the free space of the current drive in megabytes
u32 Game::DiskFreeMegabytes()
{
    std::error_code error;
    const std::filesystem::space_info space = std::filesystem::space(std::filesystem::current_path(error), error);

    if (error)
    {
        return std::numeric_limits<u32>::max();
    }

    const u64 megabytes = space.available >> kMegabyteShift;
    return static_cast<u32>(megabytes);
}

// swd32.exe 0x00451A20: frees the fame lists, clears the battle blocks of the roles and resets every battle unit
void Game::ResetBattle()
{
    this->party.fame.Free();

    for (CounterList& list : this->party.fame.lists)
    {
        ClearCounterList(list);
    }

    this->party.battleBlocks = {};

    for (BattleEnemy& enemy : this->battleEnemies)
    {
        enemy.Reset(this->battleServices);
    }

    for (BattleRole& role : this->battleRoles)
    {
        role.Reset(this->battleServices);
    }
}

namespace
{

constexpr int kEquipmentFromData = 11;

void ResetAct(ActState& state, int id, int action)
{
    state = ActState{};
    ResetPlayback(state);
    state.id = id;
    state.action = action;
}

}  // namespace

// swd32.exe 0x0040E0B0: the state a new game starts from: the story flags, the gold, the pointer roles and the screen
// animations, and from maps.dat the record tables, the four roles, the inventory, the skills and the equipment
void Game::NewGame()
{
    this->ResetBattle();
    this->acts.SetUseCache(1);
    GameState& game = this->state;
    Scene& scene = this->scene;
    game.flags = {};

    for (const int flag : kNewGameFlags)
    {
        game.SetGameFlag(flag);
    }

    for (const int flag : kNewGameClearedFlags)
    {
        game.ClearGameFlag(flag);
    }

    for (int flag = GameState::PlainScreen; flag <= GameState::SecondDisc; ++flag)
    {
        game.ClearGameFlag(flag);
    }

    game.SetGameFlag(GameState::InParty);
    game.ClearGameFlag(GameState::Unk15);
    this->party.variables[Party::Gold] = kStartGold;
    scene.returnColumn = -1;
    scene.returnRow = -1;
    scene.unk4CAD20 = -1;
    scene.unk4A9918 = -1;
    scene.returnMap = 0;
    scene.unk4C8BE0 = 0;
    scene.unk4A93D4 = 0;

    for (std::vector<Role*>& layer : this->field.rows)
    {
        layer.clear();
    }

    scene.musicTunes = {};
    scene.unk4AB788 = 1;
    this->audio.SetFadeState(1, 0, 0);
    scene.musicCue = 0;
    scene.unk4A94B0 = kStartUnk4A94B0;
    scene.shake.fill(Scene::ShakeEnd);
    scene.unk4ACDB8 = kStartUnk4ACDB8;
    scene.talkWindows.reset();
    game.SetWalkSpeed(kStartWalkSpeed);

    auto setPointer = [this](Role& role, int action)
    {
        role = Role{};
        ResetPlayback(role.act);
        role.act.id = Scene::PointerAct;
        role.act.action = action;
        role.flags = Scene::PointerFlags;
        return this->acts.Run(role.act) != 0;
    };

    setPointer(scene.pointerRole, 0);
    setPointer(scene.pointerButtonRole, Scene::ButtonPointer);
    setPointer(scene.pointerPageRole, Scene::PagePointer);
    scene.unk4A9A30 = ActState{};
    ResetPlayback(scene.unk4A9A30);
    scene.unk4B751C.reset();
    scene.unk4B74F4.reset();

    for (std::size_t index = 0; index < scene.unk4B9F68.size(); ++index)
    {
        scene.unk4B9F68[index] = ActState{};
        ResetPlayback(scene.unk4B9F68[index]);
    }

    for (int index = 0; index < kUnk232EActs; ++index)
    {
        scene.unk4B9F68[static_cast<std::size_t>(index)].id = kUnk232EAct;
        scene.unk4B9F68[static_cast<std::size_t>(index)].action = index;
    }

    ResetAct(scene.unk4C83A8, Scene::PointerAct, kUnk4C83A8Action);
    scene.viewWidth = Display::Width;
    scene.viewHeight = Display::Height;
    scene.unk4ACBC8 = kStartUnk4ACBC8;
    scene.unk4ACBCC = 0;
    ResetPlayback(scene.unk4AD3F0);
    ResetPlayback(scene.unk4A93E8);
    constexpr std::array<int, 4> kScreenActs = {0x232D, 0x232F, 0x2330, 0x2331};
    constexpr std::array<int, 4> kPanelActs = {0x2337, 0x2339, 0x233A, 0x233B};

    for (std::size_t index = 0; index < kScreenActs.size(); ++index)
    {
        ResetAct(scene.unk4CAA78[index], kScreenActs[index], 0);
        ResetAct(scene.unk4A9120[index], kPanelActs[index], 0);
    }

    ResetPlayback(scene.unk4ACAF8);

    scene.weather.fill({MapParticle::Filled, MapParticle::Filled, MapParticle::Filled, MapParticle::Filled});

    scene.unk4ACAF8.id = Scene::EffectAct;
    scene.unk4ACAF8.action = kUnk4ACAF8Action;

    for (ActState& state : scene.unk4AD0D8)
    {
        ResetPlayback(state);
        state.id = Scene::EffectAct;
    }

    const std::vector<u8>& data = this->databases.mapsData;
    const u32 records = MapsTable(data, MapsDataTable::MapGroups);
    u32 words = 0;

    while (records + sizeof(u16) * words + 1 < data.size() &&
           MapsWord(data, records + sizeof(u16) * words) != MapsRecord::TableEnd)
    {
        ++words;
    }

    scene.unk4CAE78.assign((MapsRecord::GroupWords * words + kGroupSlackBytes) / MapsRecord::GroupWords, 0);
    scene.unk4CAE78.front() = static_cast<u16>(words / MapsRecord::GroupWords);

    if (words % MapsRecord::GroupWords == 0)
    {
        for (u32 word = 0; word < words; ++word)
        {
            scene.unk4CAE78[MapsRecord::GroupsAt + word] = MapsWord(data, records + sizeof(u16) * word);
        }
    }

    scene.unk4C8BD4 = 0;
    scene.unk4ACE40 = MapsTable(data, MapsDataTable::Roles);

    for (u32 at = scene.unk4ACE40; at + 1 < data.size() && MapsWord(data, at) != MapsRecord::TableEnd;
         at += sizeof(u16) * MapsRecord::RoleWords)
    {
        scene.unk4C8BD4 = static_cast<u16>(scene.unk4C8BD4 + sizeof(u16) * MapsRecord::RoleWords);
    }

    const u32 roles = MapsTable(data, MapsDataTable::StartRoles);

    for (std::size_t role = 0; role < Party::Roles; ++role)
    {
        const std::size_t at = roles + MapsRecord::StartRoleBytes * role;

        if (at + MapsRecord::StartRoleBytes <= data.size())
        {
            this->party.roles[role].Load(std::span<const u8>(data).subspan(at, MapsRecord::StartRoleBytes));
        }
    }

    this->party.battleBlocks = {};
    FreeItemList(this->party.inventory);
    scene.unk4C97E4 = MapsTable(data, MapsDataTable::Unk18);

    for (u32 at = MapsTable(data, MapsDataTable::StartItems); at + 1 < data.size() && MapsWord(data, at) != 0;
         at += MapsRecord::StartItemBytes)
    {
        AddItem(
            this->party.inventory,
            MapsWord(data, at),
            static_cast<i16>(MapsWord(data, at + MapsRecord::StartItemCountAt)),
            ItemCountMode::Count,
            this->database
        );
    }

    for (std::size_t role = 0; role < Party::Roles; ++role)
    {
        this->party.skills[role] = NewEmptyItem();
        const u32 list = MapsTable(data, MapsDataTable::StartSkills, role);

        for (u32 at = list; at + 1 < data.size() && MapsWord(data, at) != 0; at += sizeof(u16))
        {
            AddItem(this->party.skills[role]->next, MapsWord(data, at), 1, ItemCountMode::Count, this->database);
        }
    }

    for (std::unique_ptr<ItemNode>& slot : this->party.equipment)
    {
        slot = NewEmptyItem();
    }

    for (std::size_t role = 0; role < Party::Roles; ++role)
    {
        const u32 list = MapsTable(data, MapsDataTable::StartEquipment, role);

        for (std::size_t slot = 0; slot < kEquipmentFromData; ++slot)
        {
            const u16 id = MapsWord(data, list + sizeof(u16) * slot);
            ItemLink item;

            if (id != 0)
            {
                AddItem(item, id, 1, ItemCountMode::Count, this->database);
            }
            else
            {
                item = NewEmptyItem();
            }

            this->party.equipment[role * Party::SlotsPerRole + slot] = std::move(item);
        }
    }
}

// swd32.exe 0x004149B0: the mouse pointer at the pointer position (action 0xF while key 0x2E is down); outside the
// menus the system button slides in at the top right after 16 idle frames (a click on it with no leader opens menu 1)
void Game::RefreshCursor()
{
    constexpr int kWaitKey = 0x2E;
    constexpr int kButtonX = 0x282;
    constexpr int kButtonHidden = -0x20;
    Role& pointer = this->scene.pointerRole;
    Scene& view = this->scene;

    if (this->input.IsKeyDown(kWaitKey) != 0)
    {
        pointer.act.action = Scene::WaitPointer;
    }

    if (this->menuState == Game::NoMenu)
    {
        bool button = true;

        if ((this->unk4B7518 | this->unk4B7920) != 0)
        {
            if (++view.unk4ACBC8 > 0)
            {
                view.unk4ACBC8 = 0;
            }
        }
        else if (++view.unk4ACBCC > kButtonDelay)
        {
            if (--view.unk4ACBC8 < kButtonHidden)
            {
                view.unk4ACBC8 = kButtonHidden;
            }

            const ButtonState& click = this->controls.At(Button::MouseLeft);

            if (click.press != 0 && static_cast<u32>(this->state.pointerX) > kButtonLeft &&
                static_cast<u32>(this->state.pointerY) < kButtonBottom && this->state.leader.id == Role::NoId)
            {
                this->menuState = Game::MenuEntering | Game::MainMenu;
            }
        }

        if (this->state.leader.id != Role::NoId && this->state.leader.unk20 >= kUnkLeader20Limit)
        {
            button = false;
        }

        if (button)
        {
            this->actDrawer.Draw(view.unk4C83A8, view.unk4ACBC8 + kButtonX, 0);
        }
    }

    if (this->lastPointerAction != pointer.act.action)
    {
        pointer.act.delay = 0;
    }

    this->lastPointerAction = pointer.act.action;
    this->acts.Run(pointer.act);
    const Picture* picture = this->archive.GetPicture(pointer.act.frame, pointer.act.picture);

    if (picture == nullptr)
    {
        return;
    }

    this->painter.Options().alpha = pointer.act.signal;
    this->painter.SetSource(picture->data.data());
    this->painter.DrawPicture(
        this->state.pointerX - pointer.act.x,
        this->state.pointerY - pointer.act.y,
        picture->width,
        picture->height,
        pointer.act.flags
    );
}

// The frame drawn on the frame surface shown: the original blits it to the primary surface (DDBLT_WAIT).
void Game::ShowFrame()
{
    Surface* primary = this->graphics.GetSurface(Graphics::SurfaceId::Primary);

    if (primary == nullptr || this->offscreen == nullptr)
    {
        return;
    }

    for (int row = 0; row < std::min(primary->Height(), this->offscreen->Height()); ++row)
    {
        const u16* source = this->offscreen->Row(row);
        std::copy(source, source + std::min(primary->Width(), this->offscreen->Width()), primary->Row(row));
    }

    this->graphics.Present();
}

// swd32.exe 0x0040EB60: <install directory>Music\<name>.mp3 for tune `index` of maps.dat (its table 2)
std::string Game::MusicPath(int index) const
{
    const std::vector<u8>& data = this->databases.mapsData;
    const u32 names = MapsTable(data, MapsDataTable::Music);
    const u32 entry = MapsDword(data, names + sizeof(u32) * static_cast<u32>(index));
    std::string path = this->config.installDirectory + "Music\\";

    for (std::size_t at = static_cast<std::size_t>(entry) + MapsRecord::MusicNameAt;
         at < data.size() && data[at] != '.';
         ++at)
    {
        path.push_back(static_cast<char>(data[at]));
    }

    return path + ".mp3";
}

// swd32.exe 0x004856C0: the tune as stream 100 (once) while story flag 3 allows music
void Game::PlayMusic(const std::string& path)
{
    if (this->state.GameFlag(GameState::Music))
    {
        this->audio.Streams()
            .Play(path, AudioManager::MusicId, this->config.musicVolume * kStreamVolume / Config::MaxVolume, kPlayOnce);
    }
}

// swd32.exe 0x00406D30: the keys and the save screen cleared, the previews of the newest slot's page
void Game::ResetSaveSlots()
{
    this->controls.buttons = {};
    this->unk4CC160 = 0;
    this->unk4CC161 = 1;
    this->unk4CC298 = 0;
    this->unk4CB240 = 0;
    ResetPlayback(this->unk4CC200);
    this->saveSlots.LoadPage(this->config.installDirectory, this->painter);
}

// swd32.exe 0x0040F160: a new story: maps.dat again, the starting state, the menus and an empty world, the leader of
// maps.dat's table 4; with `withMap` 1 its first map is entered
void Game::StartStory(int withMap)
{
    this->databases.LoadMaps();
    this->NewGame();
    this->menu.InitActs();
    this->state.ClearWorld();
    const std::vector<u8>& data = this->databases.mapsData;
    const u32 start = MapsTable(data, MapsDataTable::NewStory);
    const u16 leader = MapsWord(data, start + MapsRecord::NewStoryLeaderAt);
    Role& lead = this->state.leader;
    lead.unk14 = 0;
    this->unk4B7BC4 = leader;
    lead.id = leader;
    lead.unk20 = 0;
    lead.event = kOpeningEvent;
    lead.flags = 0;
    this->scene.musicTunes = {};
    this->scene.musicCue = 0;

    if (withMap == 1)
    {
        this->currentMap = Field::VeniceTavern;
        this->LoadCity(
            MapsWord(data, start),
            MapsWord(data, start + MapsRecord::NewStoryColumnAt),
            MapsWord(data, start + MapsRecord::NewStoryRowAt),
            static_cast<i16>(MapsWord(data, start + MapsRecord::NewStoryActAt)),
            static_cast<i16>(MapsWord(data, start + MapsRecord::NewStoryActionAt)),
            static_cast<i16>(MapsWord(data, start + MapsRecord::NewStoryDirectionAt)),
            0
        );
    }
}

// swd32.exe 0x00409DD0: the fame of Fame.dat (0x200 words, zeros when missing) as counters 1..500 of the second list
void Game::LoadFameFile()
{
    std::array<u16, kFameWords> values = {};
    File file;

    if (file.Open(
            Concat({this->config.installDirectory, "Fame.dat"}),
            File::Disposition::OpenExisting,
            File::Access::ReadWrite
        ))
    {
        u32 bytes = sizeof(values);
        file.Read(values.data(), bytes);
    }

    file.Close();

    for (int id = 1; id <= kFameCounters; ++id)
    {
        SetCounterCapped(
            this->party.fame.lists[Fame::FameList],
            static_cast<i16>(id),
            values[static_cast<std::size_t>(id)]
        );
    }
}

// swd32.exe 0x00411700: the name box (picture 0x2449) behind the text typed (a lead byte left alone shows as '@') in
// the large font and the cursor box
void Game::DrawTextInput(const TextInput& box)
{
    const int x = box.GetX();
    const int y = box.GetY();
    const Picture* picture = this->archive.GetPicture(kNameBoxPicture, 0);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
        this->painter.DrawPicture(x - kNameBoxLeft, y - kNameBoxUp, picture->width, picture->height, 0);
    }

    std::string text = box.CopyText(kNameBytes);

    for (std::size_t at = 0; at < text.size();)
    {
        const std::size_t next = CharNext(text, at);

        if (next - at == 1 && static_cast<i8>(text[at]) < 0)
        {
            text[at] = '@';
        }

        at = next;
    }

    const auto color = static_cast<Pixel>(this->painter.MakeColor(kNameRed, kNameGreen, kNameBlue));
    this->largeFont.DrawText(this->painter.Target(), x, y, text, color, TextFont::BoldShadow);
    const int cursor = box.GetCursor();
    FillBox(
        this->painter,
        x + kNameGlyphWidth * cursor,
        y,
        kNameGlyphWidth,
        kCursorHeight,
        kCursorRed,
        kCursorGreen,
        0,
        FillMode::Gradient
    );
}

// swd32.exe 0x0040CF10: the music queue, the streams, the MIDI player and the sound effects
void Game::AudioPump()
{
    this->audio.Queue().Update();
    this->audio.Streams().Update();
    this->audio.Midi().Update();
    this->audio.Effects().Update();
}

// swd32.exe 0x0040CDD0: when no tune plays, the next of the scene's tunes (bit 23 of the cue word picks the second
// set; a negative entry first sets the fade mode)
void Game::MusicCue()
{
    this->audio.PollMusicFade();

    if (!this->audio.IsMusicStopped())
    {
        return;
    }

    Scene& view = this->scene;
    u32 cue = view.musicCue;
    const bool second = (cue & Scene::StoryMusic) != 0;
    const u32 thirdMark = second ? Scene::StoryLoops : Scene::SceneLoops;
    const u32 secondMark = second ? Scene::StoryRepeatFirst : Scene::SceneRepeatFirst;
    auto& tunes = view.musicTunes;

    if (tunes[Scene::StoryFade] < 0)
    {
        this->audio.SetFadeMode(tunes[Scene::StoryFade] & Scene::FadeModeMask);
        tunes[Scene::StoryFade] &= ~static_cast<i32>(Scene::FadePending);
    }

    if (tunes[Scene::SceneFade] < 0)
    {
        this->audio.SetFadeMode(tunes[Scene::SceneFade] & Scene::FadeModeMask);
        tunes[Scene::SceneFade] &= ~static_cast<i32>(Scene::FadePending);
    }

    int next = static_cast<int>(cue & Scene::TrackMask) + 1;

    if (next > static_cast<int>(Scene::SceneSecond))
    {
        if ((cue & thirdMark) == 0)
        {
            view.musicCue = (cue & ~Scene::TrackByte) | Scene::MusicStopped;
            return;
        }

        next = static_cast<int>((cue & secondMark) != 0 ? Scene::SceneFirst : Scene::SceneSecond);
    }

    if (next >= 1)
    {
        const i32 tune = tunes[static_cast<std::size_t>(next) + (second ? Scene::StoryFade : Scene::SceneFade)];

        if (tune != 0 && (tune & Scene::NoTune) == 0)
        {
            this->PlayMusic(this->MusicPath(tune));
            this->audio.SetMusicVolume(this->config.musicVolume);
            cue = view.musicCue & ~Scene::UnkMusic200000;
        }
    }

    view.musicCue = (cue & ~Scene::TrackByte) | static_cast<u32>(next);
}

// swd32.exe 0x0040A570 (the frame pacing, the input and the menus; the story, battle and map branches are restored with
// their modules): at most one frame every frameInterval milliseconds
void Game::MainFrame()
{
    if ((this->systemFlags & Game::CheckingCd) != 0)
    {
        return;
    }

    if (this->active == 0)
    {
        SDL_Delay(0);
        return;
    }

    this->unk4A99FC = crt::Time();
    const auto now = static_cast<u32>(SDL_GetTicks());
    this->frameStart = now;

    if (now - this->lastFrame < static_cast<u32>(this->state.frameInterval))
    {
        return;
    }

    this->lastFrame = now;

    if ((this->deviceFlags & 1) != 0)
    {
        this->state.ClearGameFlag(GameState::Music);
        this->state.ClearGameFlag(GameState::SoundEffects);
    }

    this->unk4C844C = now;
    this->frameTime = static_cast<i32>(now - this->unk4C8444);
    this->unk4C8444 = now;
    this->input.PollKeyboard();
    this->controls.Poll(this->input, this->config, this->state, static_cast<i32>(this->frameStart));
    this->scene.pointerRole.act.action = Scene::ChoicePointer;
    this->MusicCue();

    if (this->menuState != Game::NoMenu)
    {
        this->unk4CAD24 = kCounterHidden;
        this->counterAlpha = kCounterOpaque;
        this->AudioPump();
        this->largeFont.SetAdvance(kMapLargeAdvance);
        this->mediumFont.SetAdvance(kMediumFontAdvance);
        this->smallFont.SetAdvance(kMapSmallAdvance);

        switch (this->menuState & Game::MenuNumberMask)
        {
            case Game::MainMenu:
            case Game::TitleScreenMenu:
            case Game::SystemMenuA:
            case Game::SystemMenuB:
            case Game::NameEntryMenu:
                this->menu.Run();
                break;

            default:
                break;
        }
    }
    else
    {
        this->talk.HandlePointer();
        this->roleControl.ControlPlayer();

        if (this->running != 0 && this->inactive == 0 && this->menuState == Game::NoMenu && this->inBattle == 0 &&
            this->storyState == Game::StoryRunning)
        {
            if (static_cast<u32>(++this->frameCounter) > Game::ClockFrames)
            {
                this->frameCounter = 0;

                if (static_cast<u32>(++this->party.clock) > Party::ClockLimit)
                {
                    this->party.clock = 0;
                }
            }

            this->talk.RunEvent(this->state.leader);

            if ((this->systemFlags & (Game::UnkSystem1 | Game::QuitRequested)) == 0)
            {
                this->rolePath.Update();
                this->largeFont.SetAdvance(kMapLargeAdvance);
                this->mediumFont.SetAdvance(kMediumFontAdvance);
                this->smallFont.SetAdvance(kMapSmallAdvance);
                this->fieldView.Render();
            }

            this->unk4C8448 = this->frameStart - this->unk4C844C;
        }
    }

    this->AudioPump();

    if ((this->systemFlags & Game::QuitRequested) != 0)
    {
        this->Destroy();
    }
}

// A window message to the text box (WndProc, flag 0x50).
void Game::SendText(TextMessage message, u32 parameter)
{
    if (this->textInput != nullptr)
    {
        this->textInput
            ->HandleMessage(message, parameter, false, this->textEntry, this->audio, this->config.soundVolume);
    }
}

// swd32.exe 0x0040A0D0 (the messages of the running game): F8 pauses the frames; with flag 0x50 the keys and the typed
// text go to the text box (as WM_KEYDOWN and WM_CHAR in Big5)
void Game::HandleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT)
    {
        this->Destroy();
        return;
    }

    if (this->display.HandleEvent(event))
    {
        return;
    }

    this->inputDevice.HandleEvent(event);

    if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE && (this->systemFlags & Game::VideoPlaying) != 0)
    {
        this->StopVideo();
    }
    else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F8)
    {
        this->running = ((this->systemFlags & Game::VideoPlaying) != 0 || this->running == 0) ? 1 : 0;
    }

    // WM_LBUTTONDOWN: the original compares its key state (wParam) with VK_ESCAPE, so 0x1B (the left, right and middle
    // buttons with Ctrl) stops a movie.
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT &&
        (this->systemFlags & Game::VideoPlaying) != 0)
    {
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
        const SDL_Keymod modifiers = SDL_GetModState();
        u32 held = 0;
        held |= (buttons & SDL_BUTTON_LMASK) != 0 ? kLeftButton : 0u;
        held |= (buttons & SDL_BUTTON_RMASK) != 0 ? kRightButton : 0u;
        held |= (modifiers & SDL_KMOD_SHIFT) != 0 ? kShiftKey : 0u;
        held |= (modifiers & SDL_KMOD_CTRL) != 0 ? kControlKey : 0u;
        held |= (buttons & SDL_BUTTON_MMASK) != 0 ? kMiddleButton : 0u;

        if (held == kStopVideoButtons)
        {
            this->StopVideo();
        }
    }

    if (!this->state.GameFlag(GameState::TextBox))
    {
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        struct KeyCode
        {
            SDL_Keycode key;
            TextKey virtualKey;
            bool character;
        };

        constexpr std::array<KeyCode, 8> kKeys = {{
            {SDLK_BACKSPACE, TextKey::Back, true},
            {SDLK_RETURN, TextKey::Return, true},
            {SDLK_ESCAPE, TextKey::Escape, true},
            {SDLK_LEFT, TextKey::Left, false},
            {SDLK_RIGHT, TextKey::Right, false},
            {SDLK_HOME, TextKey::Home, false},
            {SDLK_END, TextKey::End, false},
            {SDLK_DELETE, TextKey::Delete, false},
        }};

        for (const KeyCode& code : kKeys)
        {
            if (event.key.key == code.key)
            {
                this->SendText(TextMessage::KeyDown, static_cast<u32>(code.virtualKey));

                if (code.character)
                {
                    this->SendText(TextMessage::Char, static_cast<u32>(code.virtualKey));
                }
            }
        }
    }
    else if (event.type == SDL_EVENT_TEXT_INPUT)
    {
        for (const char byte : Big5FromUtf8(event.text.text))
        {
            this->SendText(TextMessage::Char, static_cast<u8>(byte));
        }
    }
}

}  // namespace swd3
