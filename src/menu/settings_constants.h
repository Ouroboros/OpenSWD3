#pragma once

#include <array>
#include <string_view>

#include "core/types.h"

// The constants of the rebuild's settings (deviations.md) that the settings of the system tab (menu_options.cpp) and
// of the title page (menu_system.cpp) share: the launcher switch and the display setup.
namespace swd3
{

constexpr std::u8string_view kLauncherSetup = u8"啟動畫面";
constexpr std::u8string_view kDisplaySetup = u8"顯示設定";

// The rows of the display setup, and their labels.
constexpr i32 kSmoothRow = 0;
constexpr i32 kVSyncRow = 1;
constexpr i32 kFrameRateRow = 2;
constexpr i32 kRenderScaleRow = 3;
constexpr i32 kWideMapRow = 4;
constexpr i32 kDisplayBackRow = 5;
constexpr std::array<std::u8string_view, 6> kDisplayLabels = {
    u8"平滑畫面",
    u8"垂直同步",
    u8"幀率上限",
    u8"繪製倍率",
    u8"地圖填滿",
    u8"返回",
};

constexpr int kChoiceGap = 12;     // between the choices of a row
constexpr int kUnchosenTint = -6;  // the choices not in use, and all of a row without effect

// Whether display setup row `row` takes effect: the frame rate and the render scale only with smooth motion.
constexpr bool DisplayRowCounts(i32 row, bool smoothMotion)
{
    return smoothMotion || (row != kFrameRateRow && row != kRenderScaleRow);
}

}  // namespace swd3
