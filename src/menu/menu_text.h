#pragma once

#include <array>

// The texts of the menus (Big5 as in the executable).
namespace swd3::menu_text
{

constexpr char kSoundVolume[] = "\255\265\256\304";                           // swd32.exe 0x004A70EC "音效"
constexpr char kMusicVolume[] = "\255\265\274\326";                           // swd32.exe 0x004A70E4 "音樂"
constexpr char kCacheSize[] = "\270m\264\253\252\305\266\241";                // swd32.exe 0x004A70D8 "置換空間"
constexpr char kMapEffects[] = "\246a\271\317\256\304\252G";                  // swd32.exe 0x004A70CC "地圖效果"
constexpr char kTextSpeed[] = "\244\345\246r\263t\253\327";                   // swd32.exe 0x004A70C0 "文字速度"
constexpr char kBattleSpeed[] = "\276\324\260\253\263t\253\327";              // swd32.exe 0x004A70B4 "戰鬥速度"
constexpr char kCacheLine[] = "%c\272\320 ,\273\335\255n%4dMB";               // swd32.exe 0x004A70A4 "%c碟 ,需要%4dMB"
constexpr char kOn[] = "\266}";                                               // swd32.exe 0x004A70A0 "開"
constexpr char kOff[] = "\303\366";                                           // swd32.exe 0x004A709C "關"
constexpr char kEffectsLine[] = "\271C\300\270\256\304\252G %s";              // swd32.exe 0x004A7090 "遊戲效果 %s"
constexpr char kSpeedFastest[] = "\263\314\247\326  ";                        // swd32.exe 0x004A7088 "最快  "
constexpr char kSpeedFast[] = "\247\326    ";                                 // swd32.exe 0x004A7080 "快    "
constexpr char kSpeedMedium[] = "\276A\244\244  ";                            // swd32.exe 0x004A7078 "適中  "
constexpr char kSpeedSlower[] = "\265y\272C  ";                               // swd32.exe 0x004A7070 "稍慢  "
constexpr char kSpeedSlow[] = "\272C    ";                                    // swd32.exe 0x004A7068 "慢    "
constexpr char kSample[] = "\266\263\251M\244s\252\272\251\274\272\335    ";  // swd32.exe 0x004A7054 "雲和山的彼端    "

constexpr std::array<const char*, 5> kSpeeds = {kSpeedFastest, kSpeedFast, kSpeedMedium, kSpeedSlower, kSpeedSlow};

}  // namespace swd3::menu_text
