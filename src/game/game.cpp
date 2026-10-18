#include "game/game.h"
#include "core/format.h"

#include <array>
#include <filesystem>
#include <limits>
#include <span>
#include <system_error>
#include <utility>

#include <SDL3/SDL.h>

#include "core/debug_log.h"
#include "crt/msvc.h"
#include "game/screenshot.h"
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

constexpr Uint64 kSlowIteration = 100;  // milliseconds: a longer message loop iteration is logged
constexpr u32 kCatchUpFrames = 4;       // a game frame this many intervals late restarts the frame clock
constexpr u64 kNanosecondsPerMillisecond = 1000000;
constexpr u64 kNanosecondsPerMicrosecond = 1000;
constexpr u64 kNanosecondsPerSecond = 1000000000;
constexpr u32 kLeftButton = 0x1;   // MK_LBUTTON
constexpr u32 kRightButton = 0x2;  // MK_RBUTTON
constexpr u32 kShiftKey = 0x4;     // MK_SHIFT
constexpr u32 kControlKey = 0x8;   // MK_CONTROL
constexpr u32 kMiddleButton = 0x10;
constexpr Uint32 kLauncherWait = 16;
constexpr Uint64 kSlowLauncherFrame =
    50;  // a launcher frame this long (its messages, drawing and presenting) is logged
constexpr std::u8string_view kLogSeparator =
    u8"=========================================================================";  // the first line of the error log
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
constexpr std::size_t kMusicPathLimit = 0x400;  // MusicPath: a longer path is reported (the original buffer)
constexpr u32 kScreenshotDiskMegabytes = 0x40;  // key P: a screenshot needs more free space than this
constexpr int kScreenshotNames = 0x1869E;       // ScrnShot\00000.bmp..ScrnShot\99997.bmp
constexpr std::string_view kScreenshotName = "%sScrnShot\\%05d.bmp";  // swd32.exe 0x0049FB3C

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
constexpr i32 kTimerStopped = -1;  // Scene::unk4CAD20 and unk4A9918 once run out
constexpr int kBattleOver = 0;     // FightTalkFrame: the battle and its talk are over
constexpr int kBattleLost = 2;
constexpr int kBattleEscaped = 3;

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

// The dynamic initializers of the original's global objects: each constructs its object and registers the destructor
// with atexit. Here the objects are members of the game, constructed and destroyed with it; the ones nothing uses are
// not made.
// swd32.exe 0x00401000: MemMgr (0x004B6C78), used by nothing: not made
// swd32.exe 0x00401010
// swd32.exe 0x00401020
// swd32.exe 0x00401030
// swd32.exe 0x00401040: the Miles startup (0x004A9AC8): AudioManager::system
// swd32.exe 0x00401050
// swd32.exe 0x00401060
// swd32.exe 0x00401070
// swd32.exe 0x00401080: the sound effects (0x004C8450): AudioManager::effects
// swd32.exe 0x00401090
// swd32.exe 0x004010A0
// swd32.exe 0x004010B0
// swd32.exe 0x004010C0: the MIDI player (0x004B7EB8): AudioManager::midi
// swd32.exe 0x004010D0
// swd32.exe 0x004010E0
// swd32.exe 0x004010F0
// swd32.exe 0x00401100: the music stream player (0x004C9288): AudioManager::streams
// swd32.exe 0x00401110
// swd32.exe 0x00401120
// swd32.exe 0x00401130
// swd32.exe 0x00401140: the music queue (0x004BA6E8): AudioManager::queue
// swd32.exe 0x00401150
// swd32.exe 0x00401160
// swd32.exe 0x00401170
// swd32.exe 0x00401180: the head of the script objects (0x004AC990, an object of its own): Scene::objects
// swd32.exe 0x00401190
// swd32.exe 0x004011B0
// swd32.exe 0x004011C0
// swd32.exe 0x004011D0: the 20-pixel font (0x004AB998): largeFont
// swd32.exe 0x004011E0
// swd32.exe 0x004011F0
// swd32.exe 0x00401200
// swd32.exe 0x00401210: the 16-pixel font (0x004C9A28): mediumFont
// swd32.exe 0x00401220
// swd32.exe 0x00401230
// swd32.exe 0x00401240
// swd32.exe 0x00401250: the 12-pixel font (0x004A9ED0): smallFont
// swd32.exe 0x00401260
// swd32.exe 0x00401270
// swd32.exe 0x00401280
// swd32.exe 0x00401290: the file of maps.dat (0x004B74A0): GameDatabases::maps
// swd32.exe 0x004012A0
// swd32.exe 0x004012B0
// swd32.exe 0x004012C0
// swd32.exe 0x004012D0: the file of HUGE.lmf (0x004B8868): MapCache::lmf
// swd32.exe 0x004012E0
// swd32.exe 0x004012F0
// swd32.exe 0x00401300
// swd32.exe 0x00401310: a file (0x004CAD50) used by nothing: not made
// swd32.exe 0x00401320
// swd32.exe 0x00401330
// swd32.exe 0x00401340
// swd32.exe 0x00401350: the file of path.dat (0x004B9F18): GameDatabases::paths
// swd32.exe 0x00401360
// swd32.exe 0x00401370
// swd32.exe 0x00401380
// swd32.exe 0x00401390: the file of the story (0x004A98C8): GameDatabases::story
// swd32.exe 0x004013A0
// swd32.exe 0x004013B0
// swd32.exe 0x004013C0
// swd32.exe 0x004013D0: the file of the mapped unit (0x004CAA28): MapCache::unitFile
// swd32.exe 0x004013E0
// swd32.exe 0x004013F0
// swd32.exe 0x00401400
// swd32.exe 0x00401410: a file (0x004B7AD0) used by nothing: not made
// swd32.exe 0x00401420
// swd32.exe 0x00401430
// swd32.exe 0x00401440
// swd32.exe 0x00401450: the file of the map movies (0x004A9380): FieldView::file
// swd32.exe 0x00401460
// swd32.exe 0x00401470
// swd32.exe 0x00401480
// swd32.exe 0x00401490: the DirectDraw wrapper (0x004AB870, no destructor registered): graphics
// swd32.exe 0x004014A0
// swd32.exe 0x004014B0: the DirectInput object (0x004B8748): input
// swd32.exe 0x004014C0
// swd32.exe 0x004014D0
// swd32.exe 0x004014E0
// swd32.exe 0x004272C0: a file (0x004CF6E0) used by nothing: not made
// swd32.exe 0x004272D0
// swd32.exe 0x004272E0
// swd32.exe 0x004272F0
// swd32.exe 0x004517A0: the ten battle roles (0x005029D0, by the vector constructor and destructor iterators):
// battleRoles
// swd32.exe 0x004517B0
// swd32.exe 0x004517D0
// swd32.exe 0x004517E0
// swd32.exe 0x00451800: the eight enemies (0x00525508): battleEnemies
// swd32.exe 0x00451810
// swd32.exe 0x00451830
// swd32.exe 0x00451840
// swd32.exe 0x00451860: a battle unit (0x00521598) used by nothing: not made
// swd32.exe 0x00451870
// swd32.exe 0x00451880
// swd32.exe 0x00451890
// swd32.exe 0x004518A0: the battle's projector (0x0053B0B8, released at exit): BattleState::projector
// swd32.exe 0x004518B0
// swd32.exe 0x004518C0
// swd32.exe 0x004518D0
// swd32.exe 0x004518E0: battle.ffd's reader (0x004FF5B8, no destructor registered): BattleState::file
// swd32.exe 0x004518F0
// swd32.exe 0x00451900: a file (0x00521538) used by nothing: not made
// swd32.exe 0x00451910
// swd32.exe 0x00451920
// swd32.exe 0x00451930
Game::Game(std::u8string installDirectory)
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
      database(this->config.installDirectory),
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
    this->archive.SetAudioServe(
        [this]
        {
            this->audioDevice.Serve();
        }
    );
    this->glyphs.Load(GlyphLibraryPath());
}

// swd32.exe 0x00409EC0: creates the window (WM_CREATE ends it when the start check fails), sends itself the start
// message and runs the message loop: the frames while the game runs, the pause banner while F8 holds it. The original
// first leaves when a window of its class is open already, and handles an installation's command line alone (the
// rebuild ignores the command line, see deviations.md).
int Game::Run([[maybe_unused]] std::u8string_view commandLine)
{
    ErrorLog(kLogSeparator);

    if (!this->display.Open(
            Utf8Chars(Game::WindowTitle),
            static_cast<int>(this->host.values.window.width),
            static_cast<int>(this->host.values.window.height),
            this->host.values.window.maximized
        ))
    {
        return 0;
    }

    const DisplaySettings& shown = this->host.values.display;
    const bool vsync = shown.vsync.value_or(true);
    const i64 frameRate = shown.frameRate.value_or(0);
    this->smoothMotion = shown.smoothMotion.value_or(true);
    this->display.SetVSync(vsync);
    this->frameRatePeriodNs = frameRate > 0 ? kNanosecondsPerSecond / static_cast<u64>(frameRate) : 0;
    this->smoothScreen = std::make_unique<Surface>(Display::Width, Display::Height);
    LogInfo(
        u8"smooth_motion=%d vsync=%d frame_rate=%d",
        this->smoothMotion ? 1 : 0,
        vsync ? 1 : 0,
        static_cast<i32>(frameRate)
    );

    if (Game::CheckStart() == 0)
    {
        this->Destroy();
        return 0;
    }

    this->Start();
    this->crtRandom.Seed(static_cast<u32>(crt::Time()));
    this->random.Seed(static_cast<u32>(crt::Time()));
    this->running = 1;
    SDL_AddEventWatch(&Game::LiveResize, this);

    while (!this->quit)
    {
        SDL_Event event;
        const Uint64 started = SDL_GetTicks();

        while (SDL_PollEvent(&event))
        {
            this->HandleEvent(event);
        }

        if (this->quit)
        {
            break;
        }

        const Uint64 handled = SDL_GetTicks();
        this->LoopFrame();
        const bool presented = this->PresentBetween();
        const Uint64 finished = SDL_GetTicks();

        if (finished - started > kSlowIteration)
        {
            LogInfo(
                u8"slow iteration %d ms (events %d ms, frame %d ms)",
                static_cast<i32>(finished - started),
                static_cast<i32>(handled - started),
                static_cast<i32>(finished - handled)
            );
        }

        if (!presented)
        {
            SDL_Delay(1);
        }
    }

    SDL_RemoveEventWatch(&Game::LiveResize, this);
    LogInfo(
        u8"frames: %d game frames, %d drawn between them",
        static_cast<i32>(this->gameFrames),
        static_cast<i32>(this->framesBetween)
    );

    if (this->framesBetween != 0)
    {
        LogInfo(
            u8"frames between: painted in %d us on average, %d us at most",
            static_cast<i32>(this->paintBetweenNs / this->framesBetween / kNanosecondsPerMicrosecond),
            static_cast<i32>(this->paintBetweenMostNs / kNanosecondsPerMicrosecond)
        );
    }

    return 0;
}

// The work of an iteration of the message loop once its messages are handled (part of 0x00409EC0): the text box's
// input, the frame of the video, of the game or of the pause banner, and the screen shown while the game is inactive.
void Game::LoopFrame()
{
    if (this->inLoopFrame)
    {
        return;
    }

    if (this->hangWatch != nullptr)
    {
        this->hangWatch->Beat();
    }

    this->inLoopFrame = true;

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
    else if (this->running == 0 && this->active == 1)
    {
        this->DrawPauseBanner();
    }

    if (this->active == 0)
    {
        this->display.Present(this->screen);
    }

    this->inLoopFrame = false;
}

// While the window is resized by dragging, Windows runs its own message loop and SDL's polling does not return; SDL
// sends the live-resize exposes (data1 1) to the event watchers from there, and the game's frames go on in them.
bool SDLCALL Game::LiveResize(void* data, SDL_Event* event)
{
    auto* game = static_cast<Game*>(data);

    if (event->type == SDL_EVENT_WINDOW_EXPOSED && event->window.data1 == 1 && !game->quit)
    {
        game->LoopFrame();
        game->PresentBetween();
    }

    return true;
}

// swd32.exe 0x0040A0D0 (message 0x404): the start sound, the default keys, the installation, the saved game flag and
// the launcher, then the display for a new or continued game
// swd32.exe 0x004980F0 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x004980F8 (EH handler of 0x0040A0D0)
void Game::Start()
{
    LogInfo(u8"start install_directory=%s", this->config.installDirectory);

    if (!this->resources.Open(Concat({this->config.installDirectory, Game::Executable})))
    {
        LogError(u8"swd32.exe not readable in %s", this->config.installDirectory);
        this->errors.ShowError(u8"swd3r", u8"swd32.exe not found in the game directory");
        this->quit = true;
        return;
    }

    IconImage picture;

    if (this->resources.FindIcon(ExeResources::GroupIcon, Game::WindowIcon, picture))
    {
        this->display.SetIcon(picture);
    }

    this->systemSound.Play(this->resources.Find("WAVE", Game::StartSound));
    this->config.SetDefaultKeys();
    this->InitInstall();
    this->state.ClearGameFlag(Launcher::SavedGameFlag);

    if (SaveGames::AnyExists(this->config.SaveDirectory()))
    {
        this->state.SetGameFlag(Launcher::SavedGameFlag);
    }

    LogInfo(
        u8"installation sound=%d music=%d cache=%d text_speed=%d battle_speed=%d",
        this->config.soundVolume,
        this->config.musicVolume,
        this->config.cacheSize,
        this->config.textSpeed,
        this->config.battleSpeed
    );
    const Launcher::Choice choice = this->RunLauncher();
    LogInfo(u8"launcher choice=%d", static_cast<i32>(choice));

    // The launcher is a dialog with the arrow pointer; over the game window the pointer is the window class's cursor
    // 0x67, which is blank.
    if (this->resources.FindIcon(ExeResources::GroupCursor, Game::WindowCursor, picture))
    {
        this->display.SetCursor(picture);
    }

    switch (choice)
    {
        case Launcher::Choice::Continue:
            this->StartGraphics();
            LogInfo(u8"graphics started");
            this->menuState = Game::NoMenu;
            this->menuMode = Menu::TitleLoadMode;
            this->storyState = Game::SaveSlotsState;
            this->unk4CAEBC = 1;
            this->unk4CAEC0 = 1;
            this->ResetSaveSlots();
            LogInfo(u8"save slots reset");
            this->saveSlots.FindNewest(this->config.SaveDirectory(), this->painter);
            LogInfo(u8"newest save slot=%d", this->saveSlots.newest);
            break;

        case Launcher::Choice::Start:
            this->StartGraphics();
            LogInfo(u8"graphics started");
            break;

        case Launcher::Choice::Exit:
            this->Destroy();
            break;

        default:
            break;
    }
}

// WM_DESTROY sent to the main window (part of 0x0040A0D0): the game released when it ran (flag 1) or a quit was asked;
// the message loop ends.
void Game::Destroy()
{
    if (this->quit)
    {
        return;
    }

    const bool release = this->state.GameFlag(GameState::Running) || (this->systemFlags & Game::QuitRequested) != 0;
    LogInfo(u8"window destroyed release=%d", release ? 1 : 0);

    if (release)
    {
        this->ReleaseGame();
    }

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

        const bool waited = SDL_WaitEventTimeout(&event, static_cast<Sint32>(kLauncherWait));
        const Uint64 started = SDL_GetTicks();

        if (waited)
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
        const Uint64 spent = SDL_GetTicks() - started;

        if (spent >= kSlowLauncherFrame)
        {
            LogDebug(u8"launcher frame took %d ms", static_cast<int>(spent));
        }
    }

    this->screen.Fill(0);
    return choice;
}

// swd32.exe 0x00425040: the default settings, then the settings kept (Env.dat in the original, [game] of the settings
// files here, see deviations.md), the paths, the cache sizes, the map cache check and the directories of the
// installation
void Game::InitInstall()
{
    Config& settings = this->config;
    settings.soundVolume = Config::DefaultVolume;
    settings.musicVolume = Config::DefaultVolume;
    settings.cacheSize = Config::DefaultCacheMegabytes;
    settings.textSpeed = Config::DefaultTextSpeed;
    settings.battleSpeed = kDefaultBattleSpeed;
    settings.settingsFile = this->host.programFile;
    settings.Apply(this->host.values.game, this->unknown14, this->effectsOff);
    this->roleControl.running = this->host.values.game.running.value_or(true) ? 1 : 0;
    this->ApplyHostPaths();
    this->SizeCaches();
    this->mapCache.Check(MapCache::CheckSessionMark);
    this->MakeDirectory(u8"Save");
    this->MakeDirectory(u8"Data");
    this->MakeDirectory(u8"Music");
    this->MakeDirectory(u8"ScrnShot");
    this->MakeDirectory(u8"Video");
    this->state.SetGameFlag(kEffectsOffFlag);

    if (this->effectsOff == 0)
    {
        this->state.ClearGameFlag(kEffectsOffFlag);
    }
}

// swd32.exe 0x004242B0 (the CD-ROM drives found for the CD checks): removed with the CD checks (the rebuild never
// looks for a CD, see deviations.md)

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
void Game::MakeDirectory(std::u8string_view name)
{
    MakeGameDirectory(Concat({this->config.installDirectory, name}));
}

// The rebuild's [paths] of swd3r.toml over the install directory Env.dat gave (data_dir) and the save directory
// (save_dir, created when missing); both with a trailing separator as the game's paths.
void Game::ApplyHostPaths()
{
    auto gamePath = [](const std::filesystem::path& path)
    {
        std::u8string out = path.u8string();

        if (!out.empty() && out.back() != u8'\\' && out.back() != u8'/')
        {
            out += u8'\\';
        }

        return out;
    };

    if (!this->host.DataDirectory().empty())
    {
        this->config.installDirectory = gamePath(this->host.DataDirectory());
        LogInfo(u8"data_dir=%s (over Env.dat)", this->config.installDirectory);
    }

    if (!this->host.SaveDirectory().empty())
    {
        this->config.saveDirectory = gamePath(this->host.SaveDirectory());
        std::error_code error;
        std::filesystem::create_directories(this->host.SaveDirectory(), error);
        LogInfo(u8"save_dir=%s", this->config.saveDirectory);
    }
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
    const u32 neededMegabytes = static_cast<u32>(this->config.cacheSize) - cachedMegabytes + kDiskReserve;

    if (neededMegabytes >= Game::DiskFreeMegabytes())
    {
        SDL_Delay(kLowDiskPause);
        PlaySystemBeep();
        // 0x00424C79 (nullsub_1 with the title 0x004A0EEC and the text 0x004A0EFC)
        DebugLog(
            u8"!!! 注意 !!! 遊戲過程中,磁碟空間不足\n請確認至少剩餘 %dM磁碟空間.",
            static_cast<int>(neededMegabytes)
        );
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
        this->systemFlags |= Game::QuitRequested;
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

    auto logPointer = [](std::u8string_view error, const Role& role)
    {
        DebugLog(error, role.id, role.act.id, role.act.action, role.act.direction);
    };

    if (!setPointer(scene.pointerRole, 0))
    {
        logPointer(u8"Act Err(Misc:Init:Mouse:Norm) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]", scene.pointerRole);
    }

    if (!setPointer(scene.pointerButtonRole, Scene::ButtonPointer))
    {
        logPointer(u8"Act Err(Misc:Init:Mouse:Btn) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]", scene.pointerButtonRole);
    }

    // The original reports the page pointer with the values of the button pointer.
    if (!setPointer(scene.pointerPageRole, Scene::PagePointer))
    {
        logPointer(
            u8"Act Err(Misc:Init:Mouse:n"
            u8"Page) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
            scene.pointerButtonRole
        );
    }

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

    ResetPlayback(scene.unk4C83A8);
    scene.unk4C83A8.id = Scene::PointerAct;
    scene.unk4C83A8.action = kUnk4C83A8Action;
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

    if (words % MapsRecord::GroupWords != 0)
    {
        DebugLog(u8"戰鬥機率讀取 設定數質量必須是 8 的倍數");
    }

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

    if (this->acts.Run(pointer.act) == 0)
    {
        DebugLog(
            u8"Act Err(RefrashCursor) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
            static_cast<u32>(pointer.id),
            pointer.act.id,
            pointer.act.action,
            pointer.act.direction
        );
    }

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
        pointer.act.flags,
        PaletteArgument(*picture)
    );
}

// The frame drawn on the frame surface shown: the original blits it to the primary surface (DDBLT_WAIT).
void Game::ShowFrame()
{
    if (this->CopyToPrimary())
    {
        this->graphics.Present();
    }
}

bool Game::CopyToPrimary()
{
    Surface* primary = this->graphics.GetSurface(Graphics::SurfaceId::Primary);

    if (primary == nullptr || this->offscreen == nullptr)
    {
        return false;
    }

    for (int row = 0; row < std::min(primary->Height(), this->offscreen->Height()); ++row)
    {
        const u16* source = this->offscreen->Row(row);
        std::copy(source, source + std::min(primary->Width(), this->offscreen->Width()), primary->Row(row));
    }

    return true;
}

void Game::ShowMapFrame()
{
    if (!this->smoothMotion)
    {
        this->ShowFrame();
        return;
    }

    this->CopyToPrimary();
    this->mapFrameShown = true;
}

bool Game::PresentBetween()
{
    if (!this->mapFrameShown || this->smoothScreen == nullptr || this->active == 0 || this->running == 0 ||
        this->quit || (this->systemFlags & Game::VideoPlaying) != 0)
    {
        return false;
    }

    const u64 now = SDL_GetTicksNS();

    if (this->frameRatePeriodNs != 0)
    {
        if (now < this->nextDrawNs)
        {
            return false;
        }

        this->nextDrawNs += this->frameRatePeriodNs;

        if (this->nextDrawNs < now)
        {
            this->nextDrawNs = now + this->frameRatePeriodNs;
        }
    }

    const u64 interval = static_cast<u64>(std::max(this->state.frameInterval, 0)) * kNanosecondsPerMillisecond;
    double alpha = 1.0;

    if (interval != 0)
    {
        alpha = now <= this->frameDueNs
                    ? 0.0
                    : std::min(1.0, static_cast<double>(now - this->frameDueNs) / static_cast<double>(interval));
    }

    const u64 paintStart = SDL_GetTicksNS();
    this->fieldView.PaintBetween(alpha, *this->smoothScreen);
    const u64 painted = SDL_GetTicksNS() - paintStart;
    this->paintBetweenNs += painted;
    this->paintBetweenMostNs = std::max(this->paintBetweenMostNs, painted);
    this->display.Present(*this->smoothScreen);
    ++this->framesBetween;
    return true;
}

// swd32.exe 0x0040EB60: <install directory>Music\<name>.mp3 for tune `index` of the name list of maps.dat's table 2
// (the list its first dword points at); the name is copied up to the next dot after its first byte
std::u8string Game::MusicPath(int index) const
{
    const std::vector<u8>& data = this->databases.mapsData;
    const u32 names = MapsDword(data, MapsTable(data, MapsDataTable::Music));
    const u32 entry = MapsDword(data, names + sizeof(u32) * static_cast<u32>(index));
    const std::u8string directory = this->config.installDirectory + u8"Music\\";
    std::string name;  // game text (the path in UTF-8 converts it)
    std::size_t at = static_cast<std::size_t>(entry) + MapsRecord::MusicNameAt;

    do
    {
        name.push_back(at < data.size() ? static_cast<char>(data[at]) : '\0');
        ++at;

        if (directory.size() + name.size() > kMusicPathLimit)
        {
            DebugLog(u8"Music String 太長,%s", PathFormat("%s%s", {directory, name}));
        }
    } while (at < data.size() && data[at] != '.');

    return PathFormat("%s%s.mp3", {directory, name});
}

// swd32.exe 0x004856C0: the tune as stream 100 (once) while story flag 3 allows music
void Game::PlayMusic(const std::u8string& path)
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
    this->saveSlots.LoadPage(this->config.SaveDirectory(), this->painter);
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
// swd32.exe 0x004980D0 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x004980D8 (EH handler of 0x00409DD0)
void Game::LoadFameFile()
{
    std::array<u16, kFameWords> values = {};
    this->audioDevice.Serve();
    File file;

    if (file.Open(
            Concat({this->config.installDirectory, u8"Fame.dat"}),
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

// swd32.exe 0x00409CE0: counters 1..500 of the second fame list written to Fame.dat (0x200 words; nothing when it does
// not open)
// swd32.exe 0x004980B0 (EH unwind: the cFile, closed by its destructor)
// swd32.exe 0x004980B8 (EH handler of 0x00409CE0)
void Game::SaveFameFile()
{
    std::array<u16, kFameWords> values = {};

    for (int id = 1; id <= kFameCounters; ++id)
    {
        values[static_cast<std::size_t>(id)] =
            static_cast<u16>(FindCounterValue(this->party.fame.lists[Fame::FameList], static_cast<i16>(id)));
    }

    File file;

    if (file.Open(
            Concat({this->config.installDirectory, u8"Fame.dat"}),
            File::Disposition::OpenAlways,
            File::Access::ReadWrite
        ))
    {
        u32 bytes = sizeof(values);
        file.Write(values.data(), bytes);
    }

    file.Close();
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

// swd32.exe 0x0040A570: a frame of the game, at most one every frameInterval milliseconds while the window is active:
// the event timers (the first ends the scene of a timed event: the scene's lists and the roles released, the leader
// set to run the timer's event), the input; then the save screen, a battle starting or running (its end maps the map
// again; a lost battle opens the game over page), the menus and the shop, or the map with its events and the clock
void Game::MainFrame()
{
    if (this->active == 0)
    {
        SleepMilliseconds(0);
        return;
    }

    this->unk4A99FC = crt::Time();
    const u32 now = SystemUptime();
    this->frameStart = now;

    const auto interval = static_cast<u32>(this->state.frameInterval);

    if (now - this->lastFrame < interval)
    {
        return;
    }

    // The original takes the time of the loop pass the frame is due in (lastFrame = now), so its frames come late by
    // the pass; the rebuild keeps the frame clock (deviations.md): a frame is due an interval after the last was due,
    // restarting from now after a pause or with no interval.
    if (interval == 0 || now - this->lastFrame >= kCatchUpFrames * interval)
    {
        this->lastFrame = now;
    }
    else
    {
        this->lastFrame += interval;
    }

    this->frameDueNs = SDL_GetTicksNS() - static_cast<u64>(now - this->lastFrame) * kNanosecondsPerMillisecond;
    this->mapFrameShown = false;
    ++this->gameFrames;

    if ((this->deviceFlags & 1) != 0)
    {
        this->state.ClearGameFlag(GameState::Music);
        this->state.ClearGameFlag(GameState::SoundEffects);
    }

    if (this->menuState == Game::NoMenu)
    {
        this->RunTimers();
    }

    this->unk4C844C = this->frameStart;
    this->frameTime = static_cast<i32>(this->frameStart - this->unk4C8444);
    this->unk4C8444 = this->frameStart;
    this->input.PollKeyboard();
    this->controls.Poll(this->input, this->config, this->state, static_cast<i32>(this->frameStart));
    this->scene.pointerRole.act.action = Scene::ChoicePointer;

    if (this->storyState != Game::StoryRunning)
    {
        this->unk4CAD24 = kCounterHidden;
        this->counterAlpha = kCounterOpaque;
        this->SaveScreenFrame();
        this->EndMainFrame();
        return;
    }

    if (this->inBattle == 0 && !this->state.GameFlag(GameState::UnkD) && this->battleRequest < 0)
    {
        this->battleRequest &= static_cast<i32>(~Game::ScriptedBattle);

        if (this->battleRequest != 0)
        {
            this->EnterBattle();
        }
    }

    this->unk4C9A14 = 0;
    this->battleRequest = 0;

    if (this->inBattle != 0)
    {
        this->BattleMainFrame();
        return;
    }

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

            case Game::ItemChoiceMenu:
                this->ShopFrame();
                break;

            default:
                break;
        }

        this->EndMainFrame();
        return;
    }

    this->talk.HandlePointer();
    this->roleControl.ControlPlayer();

    if (this->running == 0 || this->inactive != 0 || this->menuState != Game::NoMenu || this->inBattle != 0 ||
        this->storyState != Game::StoryRunning)
    {
        this->unk4C8448 = this->frameStart - this->unk4C844C;
        this->EndMainFrame();
        return;
    }

    if (static_cast<u32>(++this->frameCounter) > Game::ClockFrames)
    {
        this->frameCounter = 0;

        if (static_cast<u32>(++this->party.clock) > Party::ClockLimit)
        {
            this->party.clock = 0;
        }
    }
    else if (this->frameCounter == 0 && static_cast<u32>(++this->party.clock) > Party::ClockLimit)
    {
        this->party.clock = 0;
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
    this->EndMainFrame();
}

// The end of a frame (inline in 0x0040A570): the audio served; a requested quit closes the window.
void Game::EndMainFrame()
{
    this->AudioPump();

    if ((this->systemFlags & Game::QuitRequested) != 0)
    {
        this->Destroy();
    }
}

// The event timers (inline in 0x0040A570, outside the menus). The first (flag 0x10) counts down while no event runs
// and the save screen is closed; at its end the scene's lists are freed, every role but the first released and its
// talk ended, and the leader set to run the timer's event. The second (flag 0x4A) only sets flag 0x4B at its end.
void Game::RunTimers()
{
    Scene& view = this->scene;
    GameState& game = this->state;

    if (game.GameFlag(GameState::Timer) && --view.unk4CAD20 < 0 && game.leader.id == Role::NoId &&
        this->menuState == Game::NoMenu && this->storyState == Game::StoryRunning)
    {
        view.unk4CAD20 = kTimerStopped;
        game.ClearGameFlag(GameState::Timer);
        game.ClearGameFlag(GameState::TimerEvent);
        game.SetGameFlag(GameState::TimerExpired);
        view.FreeAnimations();
        view.FreeEffects();
        view.FreeSpritesA();
        view.FreeSpritesB();
        game.clickRegions.Free();
        view.FreeNodes();

        for (int index = 1; index < game.roleCount; ++index)
        {
            this->ReleaseRole(index);
            game.roles[static_cast<std::size_t>(index)].talkState = Role::NoTalk;
        }

        game.leader.unk14 = 0;
        game.leader.unk20 = 0;
        game.leader.event = static_cast<u16>(view.unk4A93D4);
        game.leader.id = static_cast<u16>(this->unk4B7BC4);
        game.leader.flags = 0;
        game.leader.x = 0;
        game.leader.y = 0;
    }

    if (game.GameFlag(GameState::SecondTimer) && --view.unk4A9918 < 0)
    {
        view.unk4A9918 = kTimerStopped;
        game.ClearGameFlag(GameState::SecondTimer);
        game.SetGameFlag(GameState::SecondTimerExpired);
    }
}

// A battle requested by the map started (inline in 0x0040A570): the music and the sounds stopped, the archives closed,
// the map's view released, the battle started and the roles' one-shot sounds rearmed.
void Game::EnterBattle()
{
    MapCache& cache = this->mapCache;
    this->audio.StopMusic();
    this->audio.StopAllSounds();
    this->acts.CloseAll();
    this->archive.CloseAll();

    if (!cache.unitFile.UnmapView(cache.view))
    {
        ErrorLog(u8"into story fight CloseView fail ,%s", cache.unitFile.ErrorText());
    }

    cache.view = nullptr;
    cache.unitFile.CloseMapping();
    this->StartBattle(static_cast<u16>(this->battleRequest));
    this->inBattle = 1;

    for (int index = 1; index < this->state.roleCount; ++index)
    {
        this->state.roles[static_cast<std::size_t>(index)].flags &= ~Role::SoundOnce;
    }
}

// A frame of the battle from the map (inline in 0x0040A570): the battle talk runs it. At its end (0) the fonts are set
// up again and the map's view mapped again; a lost battle (2) ends the timed event and opens the game over page; an
// escape (3) maps the map again.
void Game::BattleMainFrame()
{
    MapCache& cache = this->mapCache;
    const int result = this->FightTalkFrame();
    this->AudioPump();

    auto remap = [&cache]
    {
        cache.unitFile.CreateMapping();
        cache.view = cache.unitFile.MapView(0, 0);

        if (cache.view == nullptr)
        {
            ErrorLog(u8"After fight ,Remapping map fail!");
        }
    };

    if (result == kBattleOver)
    {
        this->battleRequest = 0;
        this->inBattle = 0;
        this->SetupFont(this->largeFont, kLargeFontSize);
        this->SetupFont(this->mediumFont, kMediumFontSize);
        this->SetupFont(this->smallFont, kSmallFontSize);
        this->talk.unit = kTalkUnit;
        remap();
        this->audio.StopMusic();
        return;
    }

    if (result == kBattleLost)
    {
        this->state.ClearGameFlag(GameState::Timer);
        this->state.ClearGameFlag(GameState::TimerEvent);
        this->state.SetGameFlag(GameState::TimerExpired);
        this->inBattle = 0;
        this->battleRequest = 0;
        this->menuState = Game::MenuEntering | Game::SystemMenuA;
        this->storyState = Game::StoryRunning;
        this->ResetSaveSlots();
        this->unk4CAEBC = 0;
        this->unk4CAEC0 = 0;
        this->menuMode = Menu::NoSlotsMode;
        return;
    }

    if (result == kBattleEscaped)
    {
        this->inBattle = 0;
        this->battleRequest = 0;
        this->state.ClearGameFlag(GameState::TimerExpired);
        remap();
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

    // WM_SIZE: while the game runs, a minimised window makes it inactive and a restored one active again. The original
    // also deactivates it when another application is activated (WM_ACTIVATEAPP); the rebuild keeps running and
    // drawing without the focus (see deviations.md). Alt+Enter toggles the full screen instead of this.
    if (this->state.GameFlag(GameState::Running))
    {
        if (event.type == SDL_EVENT_WINDOW_MINIMIZED && this->active != 0)
        {
            this->Activate(0);
        }
        else if (
            (event.type == SDL_EVENT_WINDOW_RESTORED || event.type == SDL_EVENT_WINDOW_MAXIMIZED) && this->active == 0
        )
        {
            this->Activate(1);
        }
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

    if (event.type == SDL_EVENT_KEY_UP && this->state.GameFlag(GameState::Running) &&
        (this->systemFlags & Game::UnkSystem1) == 0 && event.key.key == SDLK_P)
    {
        this->TakeScreenshot();
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

// Key P on the map (inline in 0x0040A0D0): with more than 64 MB free, a beep and the screen saved under the first free
// name of ScrnShot\00000.bmp..ScrnShot\99997.bmp (AIL_serve before each try; with every name taken the last one is
// written over).
void Game::TakeScreenshot()
{
    if (Game::DiskFreeMegabytes() <= kScreenshotDiskMegabytes)
    {
        return;
    }

    PlaySystemBeep();
    std::u8string path;

    for (int index = 0; index < kScreenshotNames; ++index)
    {
        this->audioDevice.Serve();
        path = PathFormat(kScreenshotName, {this->config.installDirectory, index});
        File file;

        if (!file.Open(path, File::Disposition::OpenExisting, File::Access::Read))
        {
            break;
        }

        file.Close();
    }

    const std::span<const u16> pixels(this->drawSurface, static_cast<std::size_t>(Display::Width) * Display::Height);
    SaveScreenshot(
        this->painter,
        pixels,
        Display::Width,
        Display::Height,
        path,
        [this]
        {
            this->audioDevice.Serve();
        }
    );
}

}  // namespace swd3
