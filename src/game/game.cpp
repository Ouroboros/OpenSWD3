#include "game/game.h"
#include "core/format.h"

#include <filesystem>
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
constexpr int kEffectsOffFlag = 0x48;
constexpr int kMinimumTileCache = 0x400000;
constexpr int kMaximumTileCache = 0x1000000;
constexpr int kActCache = 0x80000;
constexpr Uint32 kLowDiskPause = 0x400;
constexpr int kDiskReserve = 0x40;
constexpr std::string_view kDrawKernelFailed = "DrawKernel init Failed.";
constexpr std::string_view kDirectDrawFailed = "DDraw init Failed.";
constexpr std::string_view kTimerFailed = "mmTimer init Failed.";
constexpr std::string_view kErrorTitle = "SWD3";

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
    : screen(Display::kWidth, Display::kHeight),
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
    if (!this->display.Open(Utf8Chars(Game::kWindowTitle)))
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

        if (this->state.GameFlag(0x50) != SDL_TextInputActive(this->display.Window()))
        {
            if (this->state.GameFlag(0x50))
            {
                SDL_StartTextInput(this->display.Window());
            }
            else
            {
                SDL_StopTextInput(this->display.Window());
            }
        }

        if (this->running != 0 && (this->systemFlags & 0x20) != 0)
        {
            this->VideoFrame();
            this->AudioPump();
        }
        else if (this->running != 0 && (this->systemFlags & 1) == 0 && this->inactive == 0)
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
    if (!this->resources.Open(Concat({this->config.installDirectory, Game::kExecutable})))
    {
        this->errors.ShowError("swd3r", "swd32.exe not found in the game directory");
        this->quit = true;
        return;
    }

    this->systemSound.Play(this->resources.Find("WAVE", Game::kStartSound));
    this->config.SetDefaultKeys();
    this->InitInstall();
    this->state.ClearGameFlag(Launcher::kSavedGameFlag);

    if (SaveGames::AnyExists(this->config.installDirectory))
    {
        this->state.SetGameFlag(Launcher::kSavedGameFlag);
    }

    switch (this->RunLauncher())
    {
        case Launcher::Choice::Continue:
            this->StartGraphics();
            this->menuState = 0;
            this->menuMode = 3;
            this->storyState = 3;
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
    settings.installType = 2;
    settings.soundVolume = 6;
    settings.musicVolume = 6;
    settings.cacheSize = 0x3C;
    settings.textSpeed = 2;
    settings.battleSpeed = 0xB;
    this->cdDriveCount = this->FindCdDrives(settings.cdRoot);
    settings.Load(this->unknown14, this->effectsOff);
    this->SizeCaches();
    this->mapCache.Check(0);
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
    this->cdDrives.fill(0xFF);
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
    int bytes = static_cast<int>(TotalPhysicalMemory() / 6);

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
    this->input.SetMousePosition(0x1E0, 0x168);
    this->audio.SetMusicVolume(this->config.musicVolume);

    if (this->offscreen == nullptr || !this->painter.Init(*this->offscreen))
    {
        this->errors.ShowError(kDrawKernelFailed, kErrorTitle);
        this->Destroy();
        return;
    }

    const u32 cachedMegabytes = (this->mapCache.Check(1) + 0xFFFFF) >> 0x14;

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
    this->SetupFont(this->largeFont, 0x14);
    this->SetupFont(this->mediumFont, 0x10);
    this->SetupFont(this->smallFont, 0xC);
    this->talk.unit = 0xB;
    this->painter.SetClip(0, 0, Display::kWidth, Display::kHeight);
    this->textColors.Convert(this->painter);
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
    this->painter.SetTarget(this->drawSurface);
    this->painter.UnlockSurface(*this->offscreen);

    if (this->state.SetFrameInterval(Game::kFrameInterval) == 0)
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

    this->databases.script.assign(GameDatabases::kScriptBytes, 0);
    this->databases.LoadMaps();
    this->pathNodes.Init();
    this->NewGame();
    this->menu.InitActs();
    this->state.ClearWorld();
    this->menuState = Game::kTitleMenu;
}

// swd32.exe 0x00424EF0: DirectInput, Miles with the sound effects, the MIDI and stream players, the display (640 x 480
// in 16 bits) with the frame surface, and Bink
bool Game::InitDevices()
{
    this->deviceFlags = 0;

    if (this->input.Init() != 0)
    {
        this->deviceFlags |= 2;
        this->errors.ShowError(Big5(Game::kTitle), Big5(Game::kDirectInputFailed));
        this->Destroy();
        return false;
    }

    this->audio.System().Startup(this->config.installDirectory);
    this->audio.Effects().Open(this->config.installDirectory);
    this->audio.Midi().Open(this->audio.Effects().HasDriver() ? 1 : 0);
    this->audio.Streams().Open();

    if (!this->graphics.Init(Graphics::kFullScreen, Display::kWidth, Display::kHeight, 0x10))
    {
        this->errors.ShowError(kDirectDrawFailed, kErrorTitle);
        this->Destroy();
        return false;
    }

    this->offscreen = this->graphics.CreateOffscreen(Display::kWidth, Display::kHeight);
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
    font.SetTarget(Display::kWidth, Display::kHeight);
    font.SetFont(Big5(TextFont::kDefaultFace), size);

    if (size == 0x14)
    {
        font.SetAdvance(0x18);
    }
    else if (size == 0x10)
    {
        font.SetAdvance(0x12);
    }

    font.SetBackground(TextFont::kNoBackground);
    font.SetShadow(0);
}

// swd32.exe 0x00411D90: the free space of the current drive in megabytes
u32 Game::DiskFreeMegabytes()
{
    std::error_code error;
    const std::filesystem::space_info space = std::filesystem::space(std::filesystem::current_path(error), error);

    if (error)
    {
        return 0xFFFFFFFF;
    }

    const u64 megabytes = space.available >> 0x14;
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

constexpr u16 kTableEnd = 0xFFFF;
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

    for (const int flag : {1, 3, 4, 0xA, 0x46})
    {
        game.SetGameFlag(flag);
    }

    for (const int flag : {0, 2, 5, 6, 7, 8, 9, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x17})
    {
        game.ClearGameFlag(flag);
    }

    for (int flag = 0x48; flag <= 0x53; ++flag)
    {
        game.ClearGameFlag(flag);
    }

    game.SetGameFlag(0x1E);
    game.ClearGameFlag(0x15);
    this->party.variables[0] = 0x64;
    scene.unk4A9930 = -1;
    scene.unk4A9938 = -1;
    scene.unk4CAD20 = -1;
    scene.unk4A9918 = -1;
    scene.unk4CAE88 = 0;
    scene.unk4C8BE0 = 0;
    scene.unk4A93D4 = 0;

    for (std::vector<Role*>& layer : this->field.rows)
    {
        layer.clear();
    }

    scene.unk4B7C74 = {};
    scene.unk4AB788 = 1;
    this->audio.SetFadeState(1, 0, 0);
    scene.unk4ACDBC = 0;
    scene.unk4A94B0 = 0x40000000;
    scene.shake.fill(static_cast<i16>(0xCFCF));
    scene.unk4ACDB8 = 3;
    scene.talkWindows.reset();
    game.SetWalkSpeed(0x10);

    auto setPointer = [this](Role& role, int action)
    {
        role = Role{};
        ResetPlayback(role.act);
        role.act.id = Scene::kPointerAct;
        role.act.action = action;
        role.flags = Scene::kPointerFlags;
        return this->acts.Run(role.act) != 0;
    };

    setPointer(scene.pointerRole, 0);
    setPointer(scene.pointerButtonRole, 0xC);
    setPointer(scene.pointerPageRole, 0xE);
    scene.unk4A9A30 = ActState{};
    ResetPlayback(scene.unk4A9A30);
    scene.unk4B751C.reset();
    scene.unk4B74F4.reset();

    for (std::size_t index = 0; index < scene.unk4B9F68.size(); ++index)
    {
        scene.unk4B9F68[index] = ActState{};
        ResetPlayback(scene.unk4B9F68[index]);
    }

    for (int index = 0; index < 4; ++index)
    {
        scene.unk4B9F68[static_cast<std::size_t>(index)].id = 0x232E;
        scene.unk4B9F68[static_cast<std::size_t>(index)].action = index;
    }

    ResetAct(scene.unk4C83A8, 0x2329, 8);
    scene.viewWidth = Display::kWidth;
    scene.viewHeight = Display::kHeight;
    scene.unk4ACBC8 = 2;
    scene.unk4ACBCC = 0;
    ResetPlayback(scene.unk4AD3F0);
    ResetPlayback(scene.unk4A93E8);
    constexpr std::array<int, 4> kScreenActs = {0x232D, 0x232F, 0x2330, 0x2331};
    constexpr std::array<int, 4> kPanelActs = {0x2337, 0x2339, 0x233A, 0x233B};

    for (std::size_t index = 0; index < 4; ++index)
    {
        ResetAct(scene.unk4CAA78[index], kScreenActs[index], 0);
        ResetAct(scene.unk4A9120[index], kPanelActs[index], 0);
    }

    ResetPlayback(scene.unk4ACAF8);

    scene.weather.fill({0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F, 0x0F0F0F0F});

    scene.unk4ACAF8.id = 0x232B;
    scene.unk4ACAF8.action = 0x38;

    for (ActState& state : scene.unk4AD0D8)
    {
        ResetPlayback(state);
        state.id = 0x232B;
    }

    const std::vector<u8>& data = this->databases.mapsData;
    const u32 records = MapsTable(data, 0x12);
    u32 words = 0;

    while (records + 2 * words + 1 < data.size() && MapsWord(data, records + 2 * words) != kTableEnd)
    {
        ++words;
    }

    scene.unk4CAE78.assign((8 * words + 0x80) / 8, 0);
    scene.unk4CAE78[0] = static_cast<u16>(words / 8);

    if ((words & 7) == 0)
    {
        for (u32 word = 0; word < words; ++word)
        {
            scene.unk4CAE78[1 + word] = MapsWord(data, records + 2 * word);
        }
    }

    scene.unk4C8BD4 = 0;
    scene.unk4ACE40 = MapsTable(data, 1);

    for (u32 at = scene.unk4ACE40; at + 1 < data.size() && MapsWord(data, at) != kTableEnd; at += 0x16)
    {
        scene.unk4C8BD4 = static_cast<u16>(scene.unk4C8BD4 + 0x16);
    }

    const u32 roles = MapsTable(data, 6);

    for (std::size_t role = 0; role < Party::kRoles; ++role)
    {
        const std::size_t at = roles + 0x34 * role;

        if (at + 0x34 <= data.size())
        {
            this->party.roles[role].Load(std::span<const u8>(data).subspan(at, 0x34));
        }
    }

    this->party.battleBlocks = {};
    FreeItemList(this->party.inventory);
    scene.unk4C97E4 = MapsTable(data, 0x18);

    for (u32 at = MapsTable(data, 5); at + 1 < data.size() && MapsWord(data, at) != 0; at += 4)
    {
        AddItem(
            this->party.inventory,
            MapsWord(data, at),
            static_cast<i16>(MapsWord(data, at + 2)),
            ItemCountMode::Count,
            this->database
        );
    }

    for (std::size_t role = 0; role < Party::kRoles; ++role)
    {
        this->party.skills[role] = NewEmptyItem();
        const u32 list = MapsTable(data, 9 + static_cast<int>(role));

        for (u32 at = list; at + 1 < data.size() && MapsWord(data, at) != 0; at += 2)
        {
            AddItem(this->party.skills[role]->next, MapsWord(data, at), 1, ItemCountMode::Count, this->database);
        }
    }

    for (std::unique_ptr<ItemNode>& slot : this->party.equipment)
    {
        slot = NewEmptyItem();
    }

    for (std::size_t role = 0; role < Party::kRoles; ++role)
    {
        const u32 list = MapsTable(data, 0xD + static_cast<int>(role));

        for (std::size_t slot = 0; slot < kEquipmentFromData; ++slot)
        {
            const u16 id = MapsWord(data, list + 2 * slot);
            ItemLink item;

            if (id != 0)
            {
                AddItem(item, id, 1, ItemCountMode::Count, this->database);
            }
            else
            {
                item = NewEmptyItem();
            }

            this->party.equipment[role * Party::kSlotsPerRole + slot] = std::move(item);
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
        pointer.act.action = 0xF;
    }

    if (this->menuState == 0)
    {
        bool button = true;

        if ((this->unk4B7518 | this->unk4B7920) != 0)
        {
            if (++view.unk4ACBC8 > 0)
            {
                view.unk4ACBC8 = 0;
            }
        }
        else if (++view.unk4ACBCC > 0x10)
        {
            if (--view.unk4ACBC8 < kButtonHidden)
            {
                view.unk4ACBC8 = kButtonHidden;
            }

            const ButtonState& click = this->controls.At(Button::MouseLeft);

            if (click.press != 0 && static_cast<u32>(this->state.pointerX) > 0x262 &&
                static_cast<u32>(this->state.pointerY) < 0x18 && this->state.leader.id == 0xFFFF)
            {
                this->menuState = 0x80000001;
            }
        }

        if (this->state.leader.id != 0xFFFF && this->state.leader.unk20 >= 8)
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
    const u32 names = MapsTable(data, 2);
    const u32 entry = MapsWord(data, names + 4 * static_cast<u32>(index)) |
                      static_cast<u32>(MapsWord(data, names + 4 * static_cast<u32>(index) + 2)) << 16;
    std::string path = this->config.installDirectory + "Music\\";

    for (std::size_t at = static_cast<std::size_t>(entry) + 4; at < data.size() && data[at] != '.'; ++at)
    {
        path.push_back(static_cast<char>(data[at]));
    }

    return path + ".mp3";
}

// swd32.exe 0x004856C0: the tune as stream 100 (once) while story flag 3 allows music
void Game::PlayMusic(const std::string& path)
{
    if (this->state.GameFlag(3))
    {
        this->audio.Streams().Play(path, AudioManager::kMusicId, (this->config.musicVolume << 7) / 0xB, 1);
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
    const u32 start = MapsTable(data, 4);
    const u16 leader = MapsWord(data, start + 0xC);
    Role& lead = this->state.leader;
    lead.unk14 = 0;
    this->unk4B7BC4 = leader;
    lead.id = leader;
    lead.unk20 = 0;
    lead.event = 0x64;
    lead.flags = 0;
    this->scene.unk4B7C74 = {};
    this->scene.unk4ACDBC = 0;

    if (withMap == 1)
    {
        this->unk4A9A10 = 0x51;
        this->LoadCity(
            MapsWord(data, start),
            MapsWord(data, start + 2),
            MapsWord(data, start + 4),
            static_cast<i16>(MapsWord(data, start + 6)),
            static_cast<i16>(MapsWord(data, start + 8)),
            static_cast<i16>(MapsWord(data, start + 0xA)),
            0
        );
    }
}

// swd32.exe 0x00409DD0: the fame of Fame.dat (0x200 words, zeros when missing) as counters 1..500 of the second list
void Game::LoadFameFile()
{
    std::array<u16, 0x200> values = {};
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

    for (int id = 1; id <= 0x1F4; ++id)
    {
        SetCounterCapped(this->party.fame.lists[1], static_cast<i16>(id), values[static_cast<std::size_t>(id)]);
    }
}

// swd32.exe 0x00411700: the name box (picture 0x2449) behind the text typed (a lead byte left alone shows as '@') in
// the large font and the cursor box
void Game::DrawTextInput(const TextInput& box)
{
    const int x = box.GetX();
    const int y = box.GetY();
    const Picture* picture = this->archive.GetPicture(0x2449, 0);

    if (picture != nullptr)
    {
        this->painter.SetSource(picture->data.data());
        this->painter.DrawPicture(x - 0x6C, y - 0x2E, picture->width, picture->height, 0);
    }

    std::string text = box.CopyText(0x40);

    for (std::size_t at = 0; at < text.size();)
    {
        const std::size_t next = CharNext(text, at);

        if (next - at == 1 && static_cast<i8>(text[at]) < 0)
        {
            text[at] = '@';
        }

        at = next;
    }

    const auto color = static_cast<Pixel>(this->painter.MakeColor(0x15, 0xF, 8));
    this->largeFont.DrawText(this->painter.Target(), x, y, text, color, 4);
    const int cursor = box.GetCursor();
    FillBox(this->painter, x + 0xB * cursor, y, 0xB, 0x16, 0x14, 0xD, 0, FillMode::Gradient);
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
    u32 cue = static_cast<u32>(view.unk4ACDBC);
    const bool second = (cue & 0x800000) != 0;
    const u32 thirdMark = second ? 0x20000 : 0x80000;
    const u32 secondMark = second ? 0x10000 : 0x40000;
    std::array<i32, 6>& tunes = view.unk4B7C74;

    if (tunes[3] < 0)
    {
        this->audio.SetFadeMode(tunes[3] & 0xF);
        tunes[3] &= ~static_cast<i32>(0x80000000u);
    }

    if (tunes[0] < 0)
    {
        this->audio.SetFadeMode(tunes[0] & 0xF);
        tunes[0] &= ~static_cast<i32>(0x80000000u);
    }

    int next = static_cast<int>(cue & 0xF) + 1;

    if (next > 2)
    {
        if ((cue & thirdMark) == 0)
        {
            view.unk4ACDBC = static_cast<i32>((cue & ~0xFFu) | 3);
            return;
        }

        next = (cue & secondMark) != 0 ? 1 : 2;
    }

    if (next >= 1)
    {
        const i32 tune = tunes[static_cast<std::size_t>(next + (second ? 3 : 0))];

        if (tune != 0 && (tune & 0x8000) == 0)
        {
            this->PlayMusic(this->MusicPath(tune));
            this->audio.SetMusicVolume(this->config.musicVolume);
            cue = static_cast<u32>(view.unk4ACDBC) & 0xFFDFFFFFu;
        }
    }

    view.unk4ACDBC = static_cast<i32>((cue & ~0xFFu) | static_cast<u32>(next));
}

// swd32.exe 0x0040A570 (the frame pacing, the input and the menus; the story, battle and map branches are restored with
// their modules): at most one frame every frameInterval milliseconds
void Game::MainFrame()
{
    if ((this->systemFlags & 0x10) != 0)
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
        this->state.ClearGameFlag(3);
        this->state.ClearGameFlag(4);
    }

    this->unk4C844C = now;
    this->frameTime = static_cast<i32>(now - this->unk4C8444);
    this->unk4C8444 = now;
    this->input.PollKeyboard();
    this->controls.Poll(this->input, this->config, this->state, static_cast<i32>(this->frameStart));
    this->scene.pointerRole.act.action = 0xD;
    this->MusicCue();

    if (this->menuState != 0)
    {
        this->unk4CAD24 = 0x400;
        this->counterAlpha = 0x10;
        this->AudioPump();
        this->largeFont.SetAdvance(0x16);
        this->mediumFont.SetAdvance(0x12);
        this->smallFont.SetAdvance(0x10);

        switch (this->menuState & 0x0FFFFFFF)
        {
            case 1:
            case 3:
            case 4:
            case 5:
            case 6:
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

        if (this->running != 0 && this->inactive == 0 && this->menuState == 0 && this->inBattle == 0 &&
            this->storyState == 0)
        {
            if (static_cast<u32>(++this->frameCounter) > 0x14)
            {
                this->frameCounter = 0;

                if (static_cast<u32>(++this->party.unk4ACDB0) > 0x3E8)
                {
                    this->party.unk4ACDB0 = 0;
                }
            }

            this->talk.RunEvent(this->state.leader);

            if ((this->systemFlags & 5) == 0)
            {
                this->rolePath.Update();
                this->largeFont.SetAdvance(0x16);
                this->mediumFont.SetAdvance(0x12);
                this->smallFont.SetAdvance(0x10);
                this->fieldView.Render();
            }

            this->unk4C8448 = this->frameStart - this->unk4C844C;
        }
    }

    this->AudioPump();

    if ((this->systemFlags & 4) != 0)
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
    constexpr int kTextBoxFlag = 0x50;

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

    if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE && (this->systemFlags & 0x20) != 0)
    {
        this->StopVideo();
    }
    else if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_F8)
    {
        this->running = ((this->systemFlags & 0x20) != 0 || this->running == 0) ? 1 : 0;
    }

    // WM_LBUTTONDOWN: the original compares its key state (wParam) with VK_ESCAPE, so 0x1B (the left, right and middle
    // buttons with Ctrl) stops a movie.
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT &&
        (this->systemFlags & 0x20) != 0)
    {
        const SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
        const SDL_Keymod modifiers = SDL_GetModState();
        u32 held = 0;
        held |= (buttons & SDL_BUTTON_LMASK) != 0 ? 1u : 0u;
        held |= (buttons & SDL_BUTTON_RMASK) != 0 ? 2u : 0u;
        held |= (modifiers & SDL_KMOD_SHIFT) != 0 ? 4u : 0u;
        held |= (modifiers & SDL_KMOD_CTRL) != 0 ? 8u : 0u;
        held |= (buttons & SDL_BUTTON_MMASK) != 0 ? 0x10u : 0u;

        if (held == 0x1B)
        {
            this->StopVideo();
        }
    }

    if (!this->state.GameFlag(kTextBoxFlag))
    {
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN)
    {
        struct KeyCode
        {
            SDL_Keycode key;
            u32 virtualKey;
            bool character;
        };

        constexpr std::array<KeyCode, 8> kKeys = {{
            {SDLK_BACKSPACE, 8, true},
            {SDLK_RETURN, 0xD, true},
            {SDLK_ESCAPE, 0x1B, true},
            {SDLK_LEFT, 0x25, false},
            {SDLK_RIGHT, 0x27, false},
            {SDLK_HOME, 0x24, false},
            {SDLK_END, 0x23, false},
            {SDLK_DELETE, 0x2E, false},
        }};

        for (const KeyCode& code : kKeys)
        {
            if (event.key.key == code.key)
            {
                this->SendText(TextMessage::KeyDown, code.virtualKey);

                if (code.character)
                {
                    this->SendText(TextMessage::Char, code.virtualKey);
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
