#pragma once

#include <array>
#include <string>
#include <vector>

#include "core/types.h"
#include "platform/surface.h"

namespace swd3
{

class ExeResources;
class GameState;
class SystemSound;

// The launcher shown before the game (dialog 0x6B: the 640 x 480 picture 0x71 with five highlight pictures that light
// up under the mouse). A click on a hotspot ends it with the code of the choice; the highlights and the hover sound
// follow the mouse messages as in the dialog procedure.
class Launcher
{
public:
    static constexpr int Items = 5;
    static constexpr u32 Dialog = 0x6B;
    static constexpr u32 Background = 0x71;
    static constexpr u32 HoverSound = 0x77;
    static constexpr int SavedGameFlag = 0x12;
    static constexpr std::string_view Website = "www.softstar.com.tw";
    static constexpr std::u8string_view Readme = u8"Readme.txt";

    // How the launcher ended (the EndDialog codes).
    enum class Choice
    {
        None = 0,
        Continue = 1,  // the newest save (only with a saved game)
        Start = 2,
        Command3 = 3,
        Exit = 6,
    };

    // Where the highlight pictures are placed (WM_INITDIALOG) and the mouse ranges that light them, in client pixels.
    struct Item
    {
        u32 picture = 0;
        int x = 0;
        int left = 0;
        int right = 0;
    };

    static constexpr i8 Shown = 5;  // `lit` of an item under the mouse (SW_SHOW)
    static constexpr int ItemY = 4;
    static constexpr int ItemWidth = 0x27;
    static constexpr int ItemHeight = 0xB4;
    static constexpr int HotTop = 7;
    static constexpr int HotBottom = 0xBB;
    static constexpr std::array<Item, Items> Layout = {{
        {0x72, 0x30, 0x2B, 0x52},
        {0x73, 0x5C, 0x57, 0x7E},
        {0x74, 0x1E9, 0x1E4, 0x20B},
        {0x75, 0x213, 0x20E, 0x235},
        {0x76, 0x23A, 0x235, 0x25C},
    }};

    Launcher(const ExeResources& resources, const GameState& state, SystemSound& sound, std::u8string installDirectory);

    // The background, the highlights and the hover sound; false when a picture is missing.
    bool LoadPictures();
    void Init();
    Choice OnMouse(u32 buttons, int x, int y);
    Choice OnCommand(int command);
    Choice OnClose();
    void Draw(Surface& screen) const;

    static void RunBrowser(const std::string& address);
    static void RunTextViewer(const std::u8string& path);

    std::array<i8, Items> lit = {};        // 0x004CDE94 (4 bytes), 0x004CDE98: SW_SHOW (5) under the mouse
    std::array<i8, Items> litBefore = {};  // 0x004CDE99 (4 bytes), 0x004CDE9D
    std::array<bool, Items> shown = {true, true, true, true, true};  // the highlight windows (visible at first)

private:
    const ExeResources& resources;
    const GameState& state;
    SystemSound& sound;
    std::u8string installDirectory;
    Surface background;
    std::array<Surface, Items> pictures;
    std::vector<u8> hoverSound;  // read with the pictures
};

}  // namespace swd3
