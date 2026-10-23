#pragma once

#include <array>
#include <span>
#include <string_view>

#include "core/types.h"

// The constants of the rebuild's settings (deviations.md) that the settings of the system tab (menu_options.cpp) and
// of the title page (menu_system.cpp) share: the launcher switch and the setups opened from the settings (the display
// setup, the battle setup).
namespace swd3
{

constexpr std::u8string_view kLauncherSetup = u8"啟動畫面";
constexpr std::u8string_view kDisplaySetup = u8"顯示設定";
constexpr std::u8string_view kBattleSetup = u8"戰鬥設定";

// A setup opened from the settings: rows of choices, the last going back.
enum class Setup : i32
{
    Display,
    Battle,
};

// The rows of the display setup, and their labels.
constexpr i32 kSmoothRow = 0;
constexpr i32 kVSyncRow = 1;
constexpr i32 kFrameRateRow = 2;
constexpr i32 kRenderScaleRow = 3;
constexpr i32 kWideMapRow = 4;
constexpr i32 kItemHintsRow = 5;
constexpr i32 kDisplayBackRow = 6;
constexpr std::array<std::u8string_view, 7> kDisplayLabels = {
    u8"平滑畫面",
    u8"垂直同步",
    u8"幀率上限",
    u8"繪製倍率",
    u8"地圖填滿",
    u8"道具提示",
    u8"返回",
};

// The rows of the battle setup, and their labels.
constexpr i32 kAutoComboRow = 0;
constexpr i32 kBattleBackRow = 1;
constexpr std::array<std::u8string_view, 2> kBattleLabels = {
    u8"自動連擊",
    u8"返回",
};

constexpr int kChoiceGap = 12;     // between the choices of a row
constexpr int kUnchosenTint = -6;  // the choices not in use, and all of a row without effect

// The labels of the rows of `setup`, the last its going back.
constexpr std::span<const std::u8string_view> SetupLabels(Setup setup)
{
    return setup == Setup::Battle ? std::span<const std::u8string_view>(kBattleLabels)
                                  : std::span<const std::u8string_view>(kDisplayLabels);
}

constexpr i32 SetupBackRow(Setup setup)
{
    return static_cast<i32>(SetupLabels(setup).size()) - 1;
}

// Whether row `row` of `setup` takes effect: on the display setup the frame rate and the render scale only with smooth
// motion.
constexpr bool SetupRowCounts(Setup setup, i32 row, bool smoothMotion)
{
    return setup != Setup::Display || smoothMotion || (row != kFrameRateRow && row != kRenderScaleRow);
}

}  // namespace swd3
