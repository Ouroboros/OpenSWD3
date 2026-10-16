#pragma once

#include "text/big5.h"

// The texts of the battle (Big5 in the executable; u8 here, converted where drawn).
namespace swd3::battle_text
{

constexpr std::u8string_view kAttackDownEnd = u8"%s降低攻擊力效果解除";       // swd32.exe 0x004A8038
constexpr std::u8string_view kAttackDown = u8"%s降低攻擊力30%%";              // swd32.exe 0x004A7E7C
constexpr std::u8string_view kAttackUpEnd = u8"%s增加攻擊力效果解除";         // swd32.exe 0x004A8008
constexpr std::u8string_view kAttackUp = u8"%s增加30%%攻擊力";                // swd32.exe 0x004A7E58
constexpr std::u8string_view kBarrierEnd = u8"%s護罩效果解除";                // swd32.exe 0x004A8060
constexpr std::u8string_view kBarrierStart = u8"%s護罩效果啟動";              // swd32.exe 0x004A7E90
constexpr std::u8string_view kDarkAttackUpEnd = u8"%s增加暗系攻擊效果解除";   // swd32.exe 0x004A80CC
constexpr std::u8string_view kDarkResistUp = u8"%s增加50%%暗系抗力";          // swd32.exe 0x004A7ED8
constexpr std::u8string_view kDefenseDownEnd = u8"%s降低防禦力效果解除";      // swd32.exe 0x004A80E4
constexpr std::u8string_view kDefenseDown = u8"%s降低防禦力30%%";             // swd32.exe 0x004A7EA0
constexpr std::u8string_view kDefenseUpEnd = u8"%s增加防禦力效果解除";        // swd32.exe 0x004A8020
constexpr std::u8string_view kDefenseUp = u8"%s增加30%%防禦力";               // swd32.exe 0x004A7E34
constexpr std::u8string_view kFireAttackUpEnd = u8"%s增加火系攻擊效果解除";   // swd32.exe 0x004A8088
constexpr std::u8string_view kFireResistUp = u8"%s增加50%%火系抗力";          // swd32.exe 0x004A7F28
constexpr std::u8string_view kIceAttackUpEnd = u8"%s增加冰系攻擊效果解除";    // swd32.exe 0x004A8070
constexpr std::u8string_view kInvisibleEnd = u8"%s隱形效果解除";              // swd32.exe 0x004A8050
constexpr std::u8string_view kInvisibleStart = u8"%s隱形效果啟動";            // swd32.exe 0x004A7E6C
constexpr std::u8string_view kLightAttackUpEnd = u8"%s增加光系攻擊效果解除";  // swd32.exe 0x004A80A0
constexpr std::u8string_view kLightResistUp = u8"%s增加50%%光系抗力";         // swd32.exe 0x004A7EEC
constexpr std::u8string_view kPhysicalResistUp = u8"%s增加50%%物理抗力";      // swd32.exe 0x004A7EC4
constexpr std::u8string_view kPoisonResistUp = u8"%s增加50%%毒系抗力";        // swd32.exe 0x004A7F00
constexpr std::u8string_view kResistAllEnd = u8"%s抵抗所有攻擊解除";          // swd32.exe 0x004A7E20
constexpr std::u8string_view kResistDarkEnd = u8"%s抵抗暗系效果解除";         // swd32.exe 0x004A7F68
constexpr std::u8string_view kResistEarthEnd = u8"%s抵抗土系效果解除";        // swd32.exe 0x004A7FA4
constexpr std::u8string_view kResistFireEnd = u8"%s抵抗火系效果解除";         // swd32.exe 0x004A7FE0
constexpr std::u8string_view kResistIceEnd = u8"%s抵抗冰系效果解除";          // swd32.exe 0x004A7FCC
constexpr std::u8string_view kResistLightEnd = u8"%s抵抗光系效果解除";        // swd32.exe 0x004A7F7C
constexpr std::u8string_view kResistPhysicalEnd = u8"%s抵抗物理系效果解除";   // swd32.exe 0x004A7F3C
constexpr std::u8string_view kResistPoisonEnd = u8"%s抵抗毒系效果解除";       // swd32.exe 0x004A7F90
constexpr std::u8string_view kResistThunderEnd = u8"%s抵抗電系效果解除";      // swd32.exe 0x004A7F54
constexpr std::u8string_view kResistWindEnd = u8"%s抵抗風系效果解除";         // swd32.exe 0x004A7FB8
constexpr std::u8string_view kSpeedDownEnd = u8"%s降低速度效果解除";          // swd32.exe 0x004A80B8
constexpr std::u8string_view kSpeedDown = u8"%s降低速度30%%";                 // swd32.exe 0x004A7EB4
constexpr std::u8string_view kSpeedUpEnd = u8"%s增加速度效果解除";            // swd32.exe 0x004A7FF4
constexpr std::u8string_view kSpeedUp = u8"%s增加30%%速度";                   // swd32.exe 0x004A7E48
constexpr std::u8string_view kWaterResistUp = u8"%s增加50%%水系抗力";         // swd32.exe 0x004A7F14
constexpr std::string_view kLootLine = "%-12s X %2d";                         // swd32.exe 0x004A7AE8 "%-12s X %2d"
constexpr std::u8string_view kLootTitle = u8"戰利品";                         // swd32.exe 0x004A7AF4
constexpr std::u8string_view kPartyWiped = u8"隊伍全滅!!";                    // swd32.exe 0x004A7AFC
constexpr std::u8string_view kDefeatTitle = u8"戰鬥失敗";                     // swd32.exe 0x004A7B08
constexpr std::u8string_view kNothingGained = u8"沒有得到東西";               // swd32.exe 0x004A7B14
constexpr std::u8string_view kCraftFailed = u8"煉符失敗";                     // swd32.exe 0x004A7B24
constexpr std::u8string_view kCraftGot = u8"得到符咒:%s";                     // swd32.exe 0x004A7B30
constexpr std::u8string_view kCraftSuccess = u8"煉符成功";                    // swd32.exe 0x004A7B3C

// swd32.exe 0x004A79A8: the names of the nine elements (pointers to the strings in the original)
constexpr std::array<std::u8string_view, 9> kElementNames = {
    u8"火",  // 0x004A718C
    u8"冰",  // 0x004A7174
    u8"風",  // 0x004A7184
    u8"土",  // 0x004A7170
    u8"毒",  // 0x004A716C
    u8"光",  // 0x004A7180
    u8"暗",  // 0x004A7188
    u8"電",  // 0x004A7168
    u8"物",  // 0x004A7164
};

}  // namespace swd3::battle_text
