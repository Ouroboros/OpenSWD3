#pragma once

#include "text/big5.h"

#include <array>

// The texts of the menus (Big5 in the executable; u8 here, converted where drawn).
namespace swd3::menu_text
{

constexpr std::u8string_view kSoundVolume = u8"音效";           // swd32.exe 0x004A70EC
constexpr std::u8string_view kMusicVolume = u8"音樂";           // swd32.exe 0x004A70E4
constexpr std::u8string_view kCacheSize = u8"置換空間";         // swd32.exe 0x004A70D8
constexpr std::u8string_view kMapEffects = u8"地圖效果";        // swd32.exe 0x004A70CC
constexpr std::u8string_view kTextSpeed = u8"文字速度";         // swd32.exe 0x004A70C0
constexpr std::u8string_view kBattleSpeed = u8"戰鬥速度";       // swd32.exe 0x004A70B4
constexpr std::u8string_view kCacheLine = u8"%c碟 ,需要%4dMB";  // swd32.exe 0x004A70A4
constexpr std::u8string_view kOn = u8"開";                      // swd32.exe 0x004A70A0
constexpr std::u8string_view kOff = u8"關";                     // swd32.exe 0x004A709C
constexpr std::u8string_view kEffectsLine = u8"遊戲效果 %s";    // swd32.exe 0x004A7090
constexpr std::u8string_view kSpeedFastest = u8"最快  ";        // swd32.exe 0x004A7088
constexpr std::u8string_view kSpeedFast = u8"快    ";           // swd32.exe 0x004A7080
constexpr std::u8string_view kSpeedMedium = u8"適中  ";         // swd32.exe 0x004A7078
constexpr std::u8string_view kSpeedSlower = u8"稍慢  ";         // swd32.exe 0x004A7070
constexpr std::u8string_view kSpeedSlow = u8"慢    ";           // swd32.exe 0x004A7068
constexpr std::u8string_view kSample = u8"雲和山的彼端    ";    // swd32.exe 0x004A7054

constexpr std::array<std::u8string_view, 5> kSpeeds =
    {kSpeedFastest, kSpeedFast, kSpeedMedium, kSpeedSlower, kSpeedSlow};

}  // namespace swd3::menu_text
