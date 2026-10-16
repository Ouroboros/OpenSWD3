#include "game/game.h"

#include <filesystem>
#include <span>
#include <system_error>
#include <utility>

#include <SDL3/SDL.h>

#include "platform/file_system.h"
#include "platform/system_info.h"
#include "save/save_games.h"

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
constexpr char kDrawKernelFailed[] = "DrawKernel init Failed.";
constexpr char kDirectDrawFailed[] = "DDraw init Failed.";
constexpr char kTimerFailed[] = "mmTimer init Failed.";
constexpr char kErrorTitle[] = "SWD3";

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
    : screen(Display::kWidth, Display::kHeight), inputDevice(this->display), input(this->inputDevice),
      graphics(this->display, this->errors), codec(this->painter), archive(this->codec, this->errors),
      acts(this->errors), rasterizer(this->glyphs), largeFont(this->rasterizer), mediumFont(this->rasterizer),
      smallFont(this->rasterizer), audio(this->audioDevice), mapCache(this->config, this->painter),
      database(installDirectory), battleHud(this->painter, this->archive, this->acts, this->random, this->audio),
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
      }

{
    this->config.installDirectory = std::move(installDirectory);
    this->glyphs.Load(GlyphLibraryPath());
}

// swd32.exe 0x00409EC0: creates the window, sends itself the start message and runs the message loop
int Game::Run()
{
    if (!this->display.Open(Game::kWindowTitle))
    {
        return 0;
    }

    this->Start();

    while (!this->quit)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                this->quit = true;
            }
            else if (!this->display.HandleEvent(event))
            {
                this->inputDevice.HandleEvent(event);
            }
        }

        this->display.Present(this->screen);
        SDL_Delay(kLauncherWait);
    }

    return 0;
}

// swd32.exe 0x0040A0D0 (message 0x404): the start sound, the default keys, the installation, the saved game flag and
// the launcher, then the display for a new or continued game
void Game::Start()
{
    if (!this->resources.Open(this->config.installDirectory + Game::kExecutable))
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
                    const Launcher::Choice result =
                        launcher.OnMouse(MouseKeys(SDL_GetMouseState(nullptr, nullptr)), point.x, point.y);

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
void Game::MakeDirectory(const char* name)
{
    std::error_code error;
    std::filesystem::create_directory(ResolvePath(this->config.installDirectory + name), error);
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
    this->painter.UnlockSurface(*this->offscreen);

    for (u16& mask : this->config.channelMasks)
    {
        this->painter.ConvertPixels(std::span<u16>(&mask, 1));
    }

    this->unk4AB8F8 = this->painter.FillColorPair();
    this->SetupFont(this->largeFont, 0x14);
    this->SetupFont(this->mediumFont, 0x10);
    this->SetupFont(this->smallFont, 0xC);
    this->unk4A99F8 = 0xB;
    this->painter.SetClip(0, 0, Display::kWidth, Display::kHeight);
    this->textColors.Convert(this->painter);
    this->drawSurface = this->painter.LockSurface(*this->offscreen);
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

    this->databases.LoadMaps();
    this->pathNodes.Init();
    this->NewGame();
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
        this->errors.ShowError(Game::kTitle, Game::kDirectInputFailed);
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
    font.SetFont(TextFont::kDefaultFace, size);

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

// The word at `offset` of maps.dat past its header.
u16 MapsWord(const std::vector<u8>& data, std::size_t offset)
{
    return offset + 1 < data.size() ? static_cast<u16>(data[offset] | data[offset + 1] << 8) : u16{kTableEnd};
}

// The offset of table `index` of maps.dat (the dwords at its start).
u32 MapsTable(const std::vector<u8>& data, int index)
{
    const auto at = static_cast<std::size_t>(4 * index);
    return static_cast<u32>(MapsWord(data, at) | static_cast<u32>(MapsWord(data, at + 2)) << 16);
}

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
    scene.unk4A9A04.clear();
    scene.unk4A9A08.clear();
    scene.unk4A9A0C.clear();
    scene.unk4B7C74 = {};
    scene.unk4AB788 = 1;
    this->audio.SetFadeState(1, 0, 0);
    scene.unk4ACDBC = 0;
    scene.unk4A94B0 = 0x40000000;
    scene.unk4ACE70.fill(0xCF);
    scene.unk4ACDB8 = 3;
    scene.unk4ACF00 = SceneNode{};
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
    scene.unk4B751C = 0;
    scene.unk4B74F4 = 0;

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
    scene.unk4C97A0.fill(0xF);
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

}  // namespace swd3
