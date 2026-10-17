#include "launcher/launcher.h"
#include "core/format.h"

#include <utility>

#include "game/game_state.h"
#include "platform/exe_resources.h"
#include "platform/shell.h"
#include "platform/system_sound.h"

namespace swd3
{

namespace
{

constexpr u32 kLeftButton = 1;
constexpr std::size_t kContinueItem = 0;  // the items of the dialog
constexpr std::size_t kStartItem = 1;
constexpr std::size_t kWebsiteItem = 2;
constexpr i16 kStartCommand = 0x3FE;  // WM_COMMAND ids
constexpr i16 kWebsiteCommand = 0x3FF;
constexpr i16 kReadmeCommand = 0x400;
constexpr i16 kExitCommand = 0x401;
constexpr i16 kCommand3 = 0x402;
constexpr int kReadmeItem = 3;

}  // namespace

Launcher::Launcher(
    const ExeResources& resources,
    const GameState& state,
    SystemSound& sound,
    std::u8string installDirectory
)
    : resources(resources),
      state(state),
      sound(sound),
      installDirectory(std::move(installDirectory))
{
}

bool Launcher::LoadPictures()
{
    bool loaded = DecodeBitmap(this->resources.Find(ExeResources::Bitmap, Launcher::Background), this->background);

    for (std::size_t item = 0; item < Launcher::Layout.size(); ++item)
    {
        loaded = DecodeBitmap(
                     this->resources.Find(ExeResources::Bitmap, Launcher::Layout[item].picture),
                     this->pictures[item]
                 ) &&
                 loaded;
    }

    return loaded;
}

// swd32.exe 0x00424440 (WM_INITDIALOG): the dialog is centred and the pictures placed; the highlights stay as the
// template shows them until the first mouse message
void Launcher::Init()
{
    this->lit = {};
    this->litBefore = {};
}

// swd32.exe 0x00424440 (WM_MOUSEMOVE, WM_LBUTTONDOWN): lights the hotspot under the mouse (the first only with a saved
// game), sounds when one newly lights, and acts on a held left button: the first two end the launcher, the third
// opens the website, the fourth the Readme, the fifth exits. Moving with the button held acts again.
Launcher::Choice Launcher::OnMouse(u32 buttons, int x, int y)
{
    Choice choice = Choice::None;
    this->litBefore = this->lit;
    this->lit = {};
    bool newlyLit = false;
    const auto column = static_cast<u16>(x);
    const auto row = static_cast<u16>(y);

    if (row >= Launcher::HotTop && row <= Launcher::HotBottom)
    {
        for (std::size_t item = 0; item < Launcher::Layout.size(); ++item)
        {
            const Item& place = Launcher::Layout[item];

            if (column < place.left || column >= place.right)
            {
                continue;
            }

            if (item == 0 && !this->state.GameFlag(Launcher::SavedGameFlag))
            {
                continue;
            }

            this->lit[item] = Launcher::Shown;
            newlyLit = newlyLit || this->litBefore[item] == 0;

            if (buttons != kLeftButton)
            {
                continue;
            }

            switch (item)
            {
                case kContinueItem:
                    choice = Choice::Continue;
                    break;

                case kStartItem:
                    choice = Choice::Start;
                    break;

                case kWebsiteItem:
                    Launcher::RunBrowser(std::string(Launcher::Website));
                    break;

                case kReadmeItem:
                    Launcher::RunTextViewer(Concat({this->installDirectory, Launcher::Readme}));
                    break;

                default:
                    choice = Choice::Exit;
                    break;
            }
        }
    }

    for (std::size_t item = 0; item < Launcher::Layout.size(); ++item)
    {
        this->shown[item] = this->lit[item] != 0;
    }

    if (newlyLit)
    {
        this->sound.Play(this->resources.Find("WAVE", Launcher::HoverSound));
    }

    return choice;
}

// swd32.exe 0x00424440 (WM_COMMAND): commands no control of the dialog sends; 0x3FE would first ask for a name
// (dialog 0x70)
Launcher::Choice Launcher::OnCommand(int command)
{
    switch (static_cast<i16>(command))
    {
        case kStartCommand:
            return Choice::Start;

        case kWebsiteCommand:
            Launcher::RunBrowser(std::string(Launcher::Website));
            return Choice::None;

        case kReadmeCommand:
            Launcher::RunTextViewer(Concat({this->installDirectory, Launcher::Readme}));
            return Choice::None;

        case kExitCommand:
            return Choice::Exit;

        case kCommand3:
            return Choice::Command3;

        default:
            return Choice::None;
    }
}

// swd32.exe 0x00424440 (WM_CLOSE)
Launcher::Choice Launcher::OnClose()
{
    return Choice::Exit;
}

// The dialog as the window shows it: the picture, then the visible highlights (a 40 x 182 picture centred in its
// 39 x 180 static control).
void Launcher::Draw(Surface& screen) const
{
    auto blit = [&screen](const Surface& source, int left, int top, int width, int height)
    {
        const int offsetX = (width - source.Width()) / 2;
        const int offsetY = (height - source.Height()) / 2;

        for (int row = 0; row < height; ++row)
        {
            const int sourceRow = row - offsetY;
            const int targetRow = top + row;

            if (sourceRow < 0 || sourceRow >= source.Height() || targetRow < 0 || targetRow >= screen.Height())
            {
                continue;
            }

            for (int column = 0; column < width; ++column)
            {
                const int sourceColumn = column - offsetX;
                const int targetColumn = left + column;

                if (sourceColumn >= 0 && sourceColumn < source.Width() && targetColumn >= 0 &&
                    targetColumn < screen.Width())
                {
                    screen.Row(targetRow)[targetColumn] = source.Row(sourceRow)[sourceColumn];
                }
            }
        }
    };

    screen.Fill(0);
    blit(this->background, 0, 0, this->background.Width(), this->background.Height());

    for (std::size_t item = 0; item < Launcher::Layout.size(); ++item)
    {
        if (this->shown[item])
        {
            blit(
                this->pictures[item],
                Launcher::Layout[item].x,
                Launcher::ItemY,
                Launcher::ItemWidth,
                Launcher::ItemHeight
            );
        }
    }
}

// swd32.exe 0x00424930
void Launcher::RunBrowser(const std::string& address)
{
    OpenWebsite(address);
}

// swd32.exe 0x00424A60
void Launcher::RunTextViewer(const std::u8string& path)
{
    OpenTextFile(path);
}

}  // namespace swd3
