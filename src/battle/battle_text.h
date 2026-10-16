#pragma once

// The texts of the battle (Big5 as in the executable).
namespace swd3::battle_text
{

constexpr char kAttackDownEnd[] =
    "%s\255\260\247C\247\360\300\273\244O\256\304\252G\270\321\260\243";  // swd32.exe 0x004A8038 "%s降低攻擊力效果解除"
constexpr char kAttackDown[] = "%s\255\260\247C\247\360\300\273\244O30%%";  // swd32.exe 0x004A7E7C "%s降低攻擊力30%%"
constexpr char kAttackUpEnd[] =
    "%s\274W\245[\247\360\300\273\244O\256\304\252G\270\321\260\243";  // swd32.exe 0x004A8008 "%s增加攻擊力效果解除"
constexpr char kAttackUp[] = "%s\274W\245[30%%\247\360\300\273\244O";  // swd32.exe 0x004A7E58 "%s增加30%%攻擊力"
constexpr char kBarrierEnd[] = "%s\305@\270n\256\304\252G\270\321\260\243";    // swd32.exe 0x004A8060 "%s護罩效果解除"
constexpr char kBarrierStart[] = "%s\305@\270n\256\304\252G\261\322\260\312";  // swd32.exe 0x004A7E90 "%s護罩效果啟動"
constexpr char kDarkAttackUpEnd[] =
    "%s\274W\245[\267t\250t\247\360\300\273\256\304\252G\270\321\260\243";  // swd32.exe 0x004A80CC
                                                                            // "%s增加暗系攻擊效果解除"
constexpr char kDarkResistUp[] =
    "%s\274W\245[50%%\267t\250t\247\334\244O";  // swd32.exe 0x004A7ED8 "%s增加50%%暗系抗力"
constexpr char kDefenseDownEnd[] =
    "%s\255\260\247C\250\276\277m\244O\256\304\252G\270\321\260\243";     // swd32.exe 0x004A80E4 "%s降低防禦力效果解除"
constexpr char kDefenseDown[] = "%s\255\260\247C\250\276\277m\244O30%%";  // swd32.exe 0x004A7EA0 "%s降低防禦力30%%"
constexpr char kDefenseUpEnd[] =
    "%s\274W\245[\250\276\277m\244O\256\304\252G\270\321\260\243";   // swd32.exe 0x004A8020 "%s增加防禦力效果解除"
constexpr char kDefenseUp[] = "%s\274W\245[30%%\250\276\277m\244O";  // swd32.exe 0x004A7E34 "%s增加30%%防禦力"
constexpr char kFireAttackUpEnd[] =
    "%s\274W\245[\244\365\250t\247\360\300\273\256\304\252G\270\321\260\243";  // swd32.exe 0x004A8088
                                                                               // "%s增加火系攻擊效果解除"
constexpr char kFireResistUp[] =
    "%s\274W\245[50%%\244\365\250t\247\334\244O";  // swd32.exe 0x004A7F28 "%s增加50%%火系抗力"
constexpr char kIceAttackUpEnd[] =
    "%s\274W\245[\246B\250t\247\360\300\273\256\304\252G\270\321\260\243";  // swd32.exe 0x004A8070
                                                                            // "%s增加冰系攻擊效果解除"
constexpr char kInvisibleEnd[] =
    "%s\301\364\247\316\256\304\252G\270\321\260\243";  // swd32.exe 0x004A8050 "%s隱形效果解除"
constexpr char kInvisibleStart[] =
    "%s\301\364\247\316\256\304\252G\261\322\260\312";  // swd32.exe 0x004A7E6C "%s隱形效果啟動"
constexpr char kLightAttackUpEnd[] =
    "%s\274W\245[\245\372\250t\247\360\300\273\256\304\252G\270\321\260\243";  // swd32.exe 0x004A80A0
                                                                               // "%s增加光系攻擊效果解除"
constexpr char kLightResistUp[] =
    "%s\274W\245[50%%\245\372\250t\247\334\244O";  // swd32.exe 0x004A7EEC "%s增加50%%光系抗力"
constexpr char kPhysicalResistUp[] =
    "%s\274W\245[50%%\252\253\262z\247\334\244O";  // swd32.exe 0x004A7EC4 "%s增加50%%物理抗力"
constexpr char kPoisonResistUp[] =
    "%s\274W\245[50%%\254r\250t\247\334\244O";  // swd32.exe 0x004A7F00 "%s增加50%%毒系抗力"
constexpr char kResistAllEnd[] =
    "%s\251\350\247\334\251\322\246\263\247\360\300\273\270\321\260\243";  // swd32.exe 0x004A7E20 "%s抵抗所有攻擊解除"
constexpr char kResistDarkEnd[] =
    "%s\251\350\247\334\267t\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7F68 "%s抵抗暗系效果解除"
constexpr char kResistEarthEnd[] =
    "%s\251\350\247\334\244g\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7FA4 "%s抵抗土系效果解除"
constexpr char kResistFireEnd[] =
    "%s\251\350\247\334\244\365\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7FE0 "%s抵抗火系效果解除"
constexpr char kResistIceEnd[] =
    "%s\251\350\247\334\246B\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7FCC "%s抵抗冰系效果解除"
constexpr char kResistLightEnd[] =
    "%s\251\350\247\334\245\372\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7F7C "%s抵抗光系效果解除"
constexpr char kResistPhysicalEnd[] =
    "%s\251\350\247\334\252\253\262z\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7F3C "%s抵抗物理系效果解除"
constexpr char kResistPoisonEnd[] =
    "%s\251\350\247\334\254r\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7F90 "%s抵抗毒系效果解除"
constexpr char kResistThunderEnd[] =
    "%s\251\350\247\334\271q\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7F54 "%s抵抗電系效果解除"
constexpr char kResistWindEnd[] =
    "%s\251\350\247\334\255\267\250t\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7FB8 "%s抵抗風系效果解除"
constexpr char kSpeedDownEnd[] =
    "%s\255\260\247C\263t\253\327\256\304\252G\270\321\260\243";   // swd32.exe 0x004A80B8 "%s降低速度效果解除"
constexpr char kSpeedDown[] = "%s\255\260\247C\263t\253\32730%%";  // swd32.exe 0x004A7EB4 "%s降低速度30%%"
constexpr char kSpeedUpEnd[] =
    "%s\274W\245[\263t\253\327\256\304\252G\270\321\260\243";  // swd32.exe 0x004A7FF4 "%s增加速度效果解除"
constexpr char kSpeedUp[] = "%s\274W\245[30%%\263t\253\327";   // swd32.exe 0x004A7E48 "%s增加30%%速度"
constexpr char kWaterResistUp[] =
    "%s\274W\245[50%%\244\364\250t\247\334\244O";                     // swd32.exe 0x004A7F14 "%s增加50%%水系抗力"
constexpr char kLootLine[] = "%-12s X %2d";                           // swd32.exe 0x004A7AE8 "%-12s X %2d"
constexpr char kLootTitle[] = "\276\324\247Q\253~";                   // swd32.exe 0x004A7AF4 "戰利品"
constexpr char kPartyWiped[] = "\266\244\245\356\245\376\267\300!!";  // swd32.exe 0x004A7AFC "隊伍全滅!!"
constexpr char kDefeatTitle[] = "\276\324\260\253\245\242\261\321";   // swd32.exe 0x004A7B08 "戰鬥失敗"
constexpr char kNothingGained[] = "\250S\246\263\261o\250\354\252F\246\350";  // swd32.exe 0x004A7B14 "沒有得到東西"
constexpr char kCraftFailed[] = "\267\322\262\305\245\242\261\321";           // swd32.exe 0x004A7B24 "煉符失敗"
constexpr char kCraftGot[] = "\261o\250\354\262\305\251G:%s";                 // swd32.exe 0x004A7B30 "得到符咒:%s"
constexpr char kCraftSuccess[] = "\267\322\262\305\246\250\245\134";          // swd32.exe 0x004A7B3C "煉符成功"

// swd32.exe 0x004A79A8: the names of the nine elements (pointers to the strings in the original)
constexpr const char* kElementNames[] = {
    "\244\365",  // 0x004A718C 火
    "\246B",     // 0x004A7174 冰
    "\255\267",  // 0x004A7184 風
    "\244g",     // 0x004A7170 土
    "\254r",     // 0x004A716C 毒
    "\245\372",  // 0x004A7180 光
    "\267t",     // 0x004A7188 暗
    "\271q",     // 0x004A7168 電
    "\252\253",  // 0x004A7164 物
};

}  // namespace swd3::battle_text
