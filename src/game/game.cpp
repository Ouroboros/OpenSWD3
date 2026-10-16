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
      smallFont(this->rasterizer), audio(this->audioDevice), mapCache(this->config, this->painter)
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
    this->world.Clear();
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

}  // namespace swd3
