#pragma once

#include <array>
#include <memory>
#include <span>
#include <string>
#include <string_view>

#include "core/types.h"
#include "res/act_state.h"

namespace swd3
{

namespace smooth
{
class MessagesFrame;
}

class ActArchive;
class BattleHud;
class BattleState;
class Painter;
class TextFont;
class TswArchive;
struct BattleServices;
struct ObjectData;

// A floating text of the battle (a 0x24-byte list node in the original). The text is not copied: messages formatted
// into the shared battle text show whatever it holds when they are drawn.
struct BattleMessage
{
    static constexpr i32 Left = 1;               // drawn at x 0x12
    static constexpr i32 Centered = 2;           // centred on x 0x13B
    static constexpr i32 FromRight = 0x10;       // slides in from the right, leaves to the left
    static constexpr i32 FromLeft = 0x20;        // slides in, leaves to the right
    static constexpr i32 Rise = 0x40;            // drops in from the top, leaves upwards (y holds the speed)
    static constexpr i32 BlackBar = 0x40000000;  // a black bar behind the text
    static constexpr i32 Frame = static_cast<i32>(0x80000000u);  // a framed box behind the text

    std::unique_ptr<BattleMessage> next;  // +0x00
    i32 x = 0;                            // +0x04
    i32 y = 0;                            // +0x08 the rise speed with Rise
    i32 length = 0;                       // +0x0C bytes of the text
    i32 slideX = 0;                       // +0x10
    i32 slideY = 0;                       // +0x14
    i32 flags = 0;                        // +0x18
    i16 frames = 0;                       // +0x1C frames left before leaving
    // +0x20 the text. The original keeps the pointer it is given, so a buffer rewritten while the message shows draws
    // its new text; a constant text, or one formatted for the message alone, is kept in `constant`.
    std::span<const char> source;
    std::string constant;

    // The text up to its NUL.
    std::string_view Text() const;
};

// The battle messages and the result panels of a battle (0x00469080..0x00469960).
class BattleMessages
{
    friend class smooth::MessagesFrame;  // the rebuild's smooth battle: Draw split (smooth/smooth_messages.h)

public:
    // The texts of the battle (Big5 in the executable; UTF-8 here, converted where drawn).
    static constexpr std::u8string_view AttackDownEnd = u8"%s降低攻擊力效果解除";       // swd32.exe 0x004A8038
    static constexpr std::u8string_view AttackDown = u8"%s降低攻擊力30%%";              // swd32.exe 0x004A7E7C
    static constexpr std::u8string_view AttackUpEnd = u8"%s增加攻擊力效果解除";         // swd32.exe 0x004A8008
    static constexpr std::u8string_view AttackUp = u8"%s增加30%%攻擊力";                // swd32.exe 0x004A7E58
    static constexpr std::u8string_view BarrierEnd = u8"%s護罩效果解除";                // swd32.exe 0x004A8060
    static constexpr std::u8string_view BarrierStart = u8"%s護罩效果啟動";              // swd32.exe 0x004A7E90
    static constexpr std::u8string_view DarkAttackUpEnd = u8"%s增加暗系攻擊效果解除";   // swd32.exe 0x004A80CC
    static constexpr std::u8string_view DarkResistUp = u8"%s增加50%%暗系抗力";          // swd32.exe 0x004A7ED8
    static constexpr std::u8string_view DefenseDownEnd = u8"%s降低防禦力效果解除";      // swd32.exe 0x004A80E4
    static constexpr std::u8string_view DefenseDown = u8"%s降低防禦力30%%";             // swd32.exe 0x004A7EA0
    static constexpr std::u8string_view DefenseUpEnd = u8"%s增加防禦力效果解除";        // swd32.exe 0x004A8020
    static constexpr std::u8string_view DefenseUp = u8"%s增加30%%防禦力";               // swd32.exe 0x004A7E34
    static constexpr std::u8string_view FireAttackUpEnd = u8"%s增加火系攻擊效果解除";   // swd32.exe 0x004A8088
    static constexpr std::u8string_view FireResistUp = u8"%s增加50%%火系抗力";          // swd32.exe 0x004A7F28
    static constexpr std::u8string_view IceAttackUpEnd = u8"%s增加冰系攻擊效果解除";    // swd32.exe 0x004A8070
    static constexpr std::u8string_view InvisibleEnd = u8"%s隱形效果解除";              // swd32.exe 0x004A8050
    static constexpr std::u8string_view InvisibleStart = u8"%s隱形效果啟動";            // swd32.exe 0x004A7E6C
    static constexpr std::u8string_view LightAttackUpEnd = u8"%s增加光系攻擊效果解除";  // swd32.exe 0x004A80A0
    static constexpr std::u8string_view LightResistUp = u8"%s增加50%%光系抗力";         // swd32.exe 0x004A7EEC
    static constexpr std::u8string_view PhysicalResistUp = u8"%s增加50%%物理抗力";      // swd32.exe 0x004A7EC4
    static constexpr std::u8string_view PoisonResistUp = u8"%s增加50%%毒系抗力";        // swd32.exe 0x004A7F00
    static constexpr std::u8string_view ResistAllEnd = u8"%s抵抗所有攻擊解除";          // swd32.exe 0x004A7E20
    static constexpr std::u8string_view ResistDarkEnd = u8"%s抵抗暗系效果解除";         // swd32.exe 0x004A7F68
    static constexpr std::u8string_view ResistEarthEnd = u8"%s抵抗土系效果解除";        // swd32.exe 0x004A7FA4
    static constexpr std::u8string_view ResistFireEnd = u8"%s抵抗火系效果解除";         // swd32.exe 0x004A7FE0
    static constexpr std::u8string_view ResistIceEnd = u8"%s抵抗冰系效果解除";          // swd32.exe 0x004A7FCC
    static constexpr std::u8string_view ResistLightEnd = u8"%s抵抗光系效果解除";        // swd32.exe 0x004A7F7C
    static constexpr std::u8string_view ResistPhysicalEnd = u8"%s抵抗物理系效果解除";   // swd32.exe 0x004A7F3C
    static constexpr std::u8string_view ResistPoisonEnd = u8"%s抵抗毒系效果解除";       // swd32.exe 0x004A7F90
    static constexpr std::u8string_view ResistThunderEnd = u8"%s抵抗電系效果解除";      // swd32.exe 0x004A7F54
    static constexpr std::u8string_view ResistWindEnd = u8"%s抵抗風系效果解除";         // swd32.exe 0x004A7FB8
    static constexpr std::u8string_view SpeedDownEnd = u8"%s降低速度效果解除";          // swd32.exe 0x004A80B8
    static constexpr std::u8string_view SpeedDown = u8"%s降低速度30%%";                 // swd32.exe 0x004A7EB4
    static constexpr std::u8string_view SpeedUpEnd = u8"%s增加速度效果解除";            // swd32.exe 0x004A7FF4
    static constexpr std::u8string_view SpeedUp = u8"%s增加30%%速度";                   // swd32.exe 0x004A7E48
    static constexpr std::u8string_view WaterResistUp = u8"%s增加50%%水系抗力";         // swd32.exe 0x004A7F14
    static constexpr std::string_view LootLine = "%-12s X %2d";                // swd32.exe 0x004A7AE8 "%-12s X %2d"
    static constexpr std::u8string_view LootTitle = u8"戰利品";                // swd32.exe 0x004A7AF4
    static constexpr std::u8string_view PartyWiped = u8"隊伍全滅!!";           // swd32.exe 0x004A7AFC
    static constexpr std::u8string_view DefeatTitle = u8"戰鬥失敗";            // swd32.exe 0x004A7B08
    static constexpr std::u8string_view NothingGained = u8"沒有得到東西";      // swd32.exe 0x004A7B14
    static constexpr std::u8string_view CraftFailed = u8"煉符失敗";            // swd32.exe 0x004A7B24
    static constexpr std::u8string_view CraftGot = u8"得到符咒:%s";            // swd32.exe 0x004A7B30
    static constexpr std::u8string_view CraftSuccess = u8"煉符成功";           // swd32.exe 0x004A7B3C
    static constexpr std::u8string_view MasteryLine = u8"法寶%s已完全成長!!";  // swd32.exe 0x004A7AD4

    // swd32.exe 0x004A79A8: the names of the nine elements (pointers to the strings in the original)
    static constexpr std::array<std::u8string_view, 9> ElementNames = {
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

    static constexpr int LootSlots = 10;
    static constexpr int PanelAct = 0x233B;

    BattleMessages(
        Painter& painter,
        TswArchive& archive,
        ActArchive& acts,
        BattleHud& hud,
        BattleState& state,
        TextFont& panelFont,
        TextFont& messageFont
    );

    // The rebuild's smooth battle: the services its draws are handed to (BattleServices::frame), set once the battle's
    // services are built.
    void Attach(BattleServices& battle);

    void DrawLootPanel();
    void DrawDefeatPanel();
    void DrawCraftResultPanel();
    void DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::string_view text);
    void DrawMessageBox(int x, int y, int width, int height, int textX, int textY, std::u8string_view text);
    int GrowPanel(int top, int bottom, int divisor);
    void DrawResistancePanel();
    void DrawMastery();
    void Add(int x, int y, i16 frames, std::span<const char> text, int flags);
    void Add(int x, int y, i16 frames, std::u8string_view text, int flags);
    void AddFormatted(int x, int y, i16 frames, std::string text, int flags);
    void Draw();

    std::unique_ptr<BattleMessage> messages;                 // 0x005214F8
    std::array<i32, 8> unk5214FC = {};                       // cleared with messages by Game::EndBattle
    ActState panelAct;                                       // 0x004FC5B0 act 0x233B: the frame picture of the panels
    i32 panelHeight = 0;                                     // 0x0053C4BC grown by GrowPanel
    u16 lootCount = 0;                                       // 0x0053C498
    std::array<std::string_view, LootSlots> lootNames = {};  // 0x00524468
    std::array<u16, LootSlots> lootCounts = {};              // 0x00525434
    u8 craftSucceeded = 0;                                   // 0x0053C4B4
    const ObjectData* panelUnit = nullptr;                   // 0x0053C4B8 the object shown by DrawResistancePanel

private:
    int PanelFrame();
    void Append(std::unique_ptr<BattleMessage> message, int x, int y, i16 frames, int flags);
    void DrawPanelText(int x, int y, std::string_view text);
    void DrawPanelText(int x, int y, std::u8string_view text);
    void DrawMessageText(int x, int y, std::string_view text);

    Painter& painter;
    TswArchive& archive;
    ActArchive& acts;
    BattleHud& hud;
    BattleState& state;
    TextFont& panelFont;    // 0x004C9A28 (16 pixels)
    TextFont& messageFont;  // 0x004AB998 (20 pixels)
    BattleServices* services = nullptr;
};

}  // namespace swd3
