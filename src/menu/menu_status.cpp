#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

#include "core/fault.h"
#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "org/database.h"
#include "org/fame.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr u16 kRoleSound = 0x107;
constexpr u16 kRoleMask = 3;
constexpr std::size_t kResistanceSlots = 7;  // the slots whose resistances add up
constexpr std::size_t kFirstMagicSlot = 7;
constexpr std::size_t kLastMagicSlot = 8;
constexpr u16 kElementMagic = 0x33;  // ObjectData::unk48 of a magic whose proficiency adds to the resistances
constexpr i32 kMagicScale = -1000;
constexpr i32 kPercent = 100;
constexpr i32 kTenth = 10;
constexpr u16 kSubpageCode = 2;  // Menu::code the faces work on

// The clicks (Menu::clicks).
constexpr u32 kLeftClicks = 3;
constexpr u32 kRightClick = 4;

// The mouse areas.
constexpr u32 kFacesTop = 0xA;
constexpr u32 kFacesBottom = 0x1D4;
constexpr u32 kFacesLeft = 4;
constexpr u32 kFacesRight = 0xBC;
constexpr u32 kFaceHeight = 0x6E;

// The texts.
constexpr std::u8string_view kLevelFormat = u8"   等級  %-3d";      // swd32.exe 0x004A71F4
constexpr std::u8string_view kExperience = u8"   經驗";             // swd32.exe 0x004A71EC
constexpr std::u8string_view kNextLevel = u8"   升級";              // swd32.exe 0x004A71DC
constexpr std::u8string_view kStrengthFormat = u8"   力量  %-4d";   // swd32.exe 0x004A71CC
constexpr std::u8string_view kEnduranceFormat = u8"   耐力  %-4d";  // swd32.exe 0x004A71BC
constexpr std::u8string_view kWisdomFormat = u8"   智慧  %-4d";     // swd32.exe 0x004A71AC
constexpr std::u8string_view kAgilityFormat = u8"   敏捷  %-4d";    // swd32.exe 0x004A719C
constexpr std::u8string_view kElement = u8"   屬性  ";              // swd32.exe 0x004A7190
// swd32.exe 0x004A718C, 0x004A7188, 0x004A7184, 0x004A7180: the element of each role.
constexpr std::array<std::u8string_view, Party::Roles> kRoleElements = {u8"火", u8"暗", u8"風", u8"光"};
constexpr std::u8string_view kResistances = u8"抗力";  // swd32.exe 0x004A7178
// The element names of the resistance table (swd32.exe 0x004A7164..0x004A718C) in their two columns.
constexpr std::array<std::u8string_view, 5> kFirstColumn = {u8"火", u8"冰", u8"風", u8"土", u8"毒"};
constexpr std::array<std::u8string_view, 4> kSecondColumn = {u8"光", u8"暗", u8"電", u8"物"};
constexpr std::string_view kNumberFormat = "%-d";
constexpr std::string_view kWideNumberFormat = "%-10d";
constexpr std::string_view kBonusFormat = "%c%-3d";
constexpr std::u8string_view kWeakFormat = u8"損%2d0%%";    // swd32.exe 0x004A6E40
constexpr std::u8string_view kResistFormat = u8"抗%2d0%%";  // swd32.exe 0x004A6E34
constexpr std::u8string_view kAbsorbFormat = u8"吸%2d0%%";  // swd32.exe 0x004A6E28
constexpr std::string_view kNoResistance = "------";
constexpr i32 kAbsorbFrom = -0xA;

// The layout.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr u32 kBlueRed = 2;
constexpr u32 kBlueGreen = 0xE;
constexpr u32 kBlueBlue = 0x1D;
constexpr u32 kRedRed = 0x1C;
constexpr u32 kRedGreen = 2;
constexpr u32 kRedBlue = 2;
constexpr u32 kGreenRed = 2;
constexpr u32 kGreenGreen = 0x1C;
constexpr u32 kGreenBlue = 0xD;
constexpr u32 kGainShade = 0x1F;
constexpr u32 kLossRed = 0x1A;
constexpr int kPanelStyle = 4;
constexpr int kFrameBorder = static_cast<int>(0x80000008);
constexpr int kNameBoxLeft = 0xD0;
constexpr int kNameBoxTop = 0x3C;
constexpr int kNameBoxRight = 0x13C;
constexpr int kNameBoxBottom = 0x50;
constexpr int kNameX = 0xD4;
constexpr int kNameY = 0x3D;
constexpr int kLeftBoxX = 0xC8;
constexpr int kLeftBoxY = 0x60;
constexpr int kLeftBoxWidth = 0xB4;
constexpr int kLeftBoxHeight = 0x16E;
constexpr int kLeftFrameLeft = 0xD0;
constexpr int kLeftFrameTop = 0x68;
constexpr int kLeftFrameRight = 0x174;
constexpr int kLeftFrameBottom = 0x1C8;
constexpr int kRightBoxX = 0x186;
constexpr int kRightBoxWidth = 0xEC;
constexpr int kRightBoxHeight = 0x140;
constexpr int kRightFrameLeft = 0x18E;
constexpr int kRightFrameRight = 0x26A;
constexpr int kRightFrameBottom = 0x19A;
constexpr i32 kFaceAct = 0x232A;
constexpr i32 kFirstFaceAction = 0x2D;
constexpr std::array<int, 3> kFaceX = {0x14A, 0x1AE, 0x212};
constexpr int kTopY = 0x3E;
constexpr std::array<int, 3> kTopX = {0x17C, 0x1E0, 0x244};
constexpr int kLinesX = 0xB4;
constexpr int kValuesX = 0x117;
constexpr int kLevelY = 0x6A;
constexpr int kExperienceY = 0x83;
constexpr int kExperienceValueY = 0x88;
constexpr int kNextLevelY = 0x9C;
constexpr int kNextLevelValueY = 0xA1;
constexpr std::array<int, 4> kAttributeY = {0xB5, 0xCE, 0xE7, 0x100};
constexpr int kBonusDown = 7;
constexpr int kElementY = 0x132;
constexpr int kTableX = 0xD2;
constexpr int kSecondTableX = 0x127;
constexpr int kTableTop = 0x150;
constexpr int kTableLine = 0x14;
constexpr int kValuesColumn = 0x55;
constexpr int kValuesTableX = 0xEA;
constexpr int kValuesTableY = 0x169;
constexpr int kValuesPerColumn = 5;
constexpr int kSlotsX = 0x18E;
constexpr int kSlotsY = 0x66;
constexpr int kSlotsWidth = 0xA4;
constexpr int kNoSlot = 0x14;  // none chosen
constexpr int kBonusPad = 4;
constexpr std::array<int, 4> kBonusDigits = {1, 0xA, 0x64, 0x3E8};
constexpr int kDigitWidth = 0xB;

}  // namespace

// swd32.exe 0x00449FF0: the status page opened (on the first role when no tab is raised)
void Menu::StatusEnter()
{
    if (this->roleTab == Menu::NoTab)
    {
        this->roleTab = 0;
    }

    this->status = StatusPage{};
    this->StatusCompute();
}

// swd32.exe 0x0044A030: the status page's records freed
void Menu::StatusLeave()
{
    this->status = StatusPage{};
}

// swd32.exe 0x0044AB00: the role's totals: its record (equipment bonuses from 0) plus what its sixteen slots give, the
// resistances of its first seven slots, and the resistances of a magic of element 0x33 in slots 7 and 8 scaled by its
// proficiency (a magic without a maximum faults, as the original divides by it)
void Menu::StatusCompute()
{
    Game& owner = this->game;
    StatusPage& page = this->status;
    const std::size_t role = std::min<std::size_t>(this->roleTab, Party::Roles - 1);
    page.bonus = RoleRecord{};
    page.total = owner.party.roles[role];
    page.total.attackBonus = 0;
    page.total.defenseBonus = 0;

    auto slotOf = [&owner, role](std::size_t slot)
    {
        return owner.party.equipment[Party::SlotsPerRole * role + slot].get();
    };

    for (std::size_t slot = 0; slot < Party::SlotsPerRole; ++slot)
    {
        if (const ItemNode* worn = slotOf(slot); worn != nullptr)
        {
            ApplyObject(page.bonus, worn->record.data, owner.database);
        }
    }

    auto add = [](auto& total, auto value)
    {
        total = static_cast<std::remove_reference_t<decltype(total)>>(total + value);
    };

    add(page.total.hitPoints, page.bonus.hitPoints);
    add(page.total.spirit, page.bonus.spirit);
    add(page.total.stamina, page.bonus.stamina);
    add(page.total.maxHitPoints, page.bonus.maxHitPoints);
    add(page.total.maxSpirit, page.bonus.maxSpirit);
    add(page.total.maxStamina, page.bonus.maxStamina);
    add(page.total.strength, page.bonus.strength);
    add(page.total.endurance, page.bonus.endurance);
    add(page.total.wisdom, page.bonus.wisdom);
    add(page.total.agility, page.bonus.agility);
    add(page.total.rapport, page.bonus.rapport);
    add(page.total.reaction, page.bonus.reaction);
    add(page.total.ailmentChance, page.bonus.ailmentChance);
    add(page.total.evasion, page.bonus.evasion);
    add(page.total.attackBonus, page.bonus.attackBonus);
    add(page.total.defenseBonus, page.bonus.defenseBonus);

    for (std::size_t slot = 0; slot < kResistanceSlots; ++slot)
    {
        if (const ItemNode* worn = slotOf(slot); worn != nullptr)
        {
            for (std::size_t element = 0; element < page.total.resistances.size(); ++element)
            {
                page.total.resistances[element] =
                    static_cast<u8>(page.total.resistances[element] + worn->record.data.resistances[element]);
            }
        }
    }

    for (std::size_t slot = kFirstMagicSlot; slot <= kLastMagicSlot; ++slot)
    {
        const ItemNode* worn = slotOf(slot);

        if (worn == nullptr || worn->record.data.unk48 != kElementMagic)
        {
            continue;
        }

        u16 maximum = 0;
        u16 value = 0;
        GetProficiency(owner.party.fame.lists[Fame::ProficiencyList], worn->id, maximum, value, owner.database);

        for (std::size_t element = 0; element < page.total.resistances.size(); ++element)
        {
            int resistance = static_cast<i8>(worn->record.data.resistances[element]);

            if (resistance == 0)
            {
                continue;
            }

            if (maximum == 0)
            {
                Fault(
                    owner.errors,
                    u8"the status page: a magic without a proficiency maximum (an integer division by zero)"
                );
            }

            resistance = static_cast<i8>(std::abs(resistance));  // 0x0044ADE7 neg bl: -128 stays -128
            const auto scaled = static_cast<i32>(static_cast<u32>(kMagicScale) * value);
            page.total.resistances[element] =
                static_cast<u8>(page.total.resistances[element] + resistance * (scaled / maximum / kPercent) / kTenth);
        }
    }
}

// swd32.exe 0x0044A050: the mouse on the status page: a click on a face shows that role; a right click goes back
void Menu::StatusFrame()
{
    Game& owner = this->game;
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    if ((this->clicks & kLeftClicks) != 0 && this->code == kSubpageCode && y < kFacesBottom && y > kFacesTop &&
        x < kFacesRight && x > kFacesLeft)
    {
        const auto face = static_cast<u16>((y - kFacesTop) / kFaceHeight);

        if (!owner.state.GameFlag(face + Party::FirstRoleFlag))
        {
            return;
        }

        do
        {
            this->StatusNext();
        } while (this->roleTab != face);
    }

    if ((this->clicks & kRightClick) != 0)
    {
        this->StatusCancel();
    }
}

// swd32.exe 0x0044A0D0: the next party role
void Menu::StatusNext()
{
    Game& owner = this->game;
    u16 role = static_cast<u16>((this->roleTab + 1) & kRoleMask);

    while (this->tabs[role].role == Menu::NoRole)
    {
        role = static_cast<u16>((role + 1) & kRoleMask);
    }

    this->roleTab = role;
    this->StatusCompute();
    owner.audio.PlaySound(kRoleSound, owner.config.soundVolume);
}

// swd32.exe 0x0044A160: the party role before (the Up key)
// swd32.exe 0x0044A1D0: the Left key, the same (its test for the wrap differs, the result does not)
void Menu::StatusPrevious()
{
    Game& owner = this->game;
    u16 role = this->roleTab;

    do
    {
        role = static_cast<u16>((role - 1) & kRoleMask);
    } while (this->tabs[role].role == Menu::NoRole);

    this->roleTab = role;
    this->StatusCompute();
    owner.audio.PlaySound(kRoleSound, owner.config.soundVolume);
}

// swd32.exe 0x0044A250: Escape: the page closes
void Menu::StatusCancel()
{
    this->StatusLeave();

    if (--this->code == 0)
    {
        this->game.menuState = Game::NoMenu;
    }

    this->SetPage(this->code);
}

// swd32.exe 0x0044AE70: a gain from the equipment beside a value: "+n" in white or "-n" in red, further right the
// more digits the value has
void Menu::StatusDrawBonus(int bonus, int x, int y, int value)
{
    Game& owner = this->game;

    if (bonus == 0)
    {
        return;
    }

    auto color = static_cast<Pixel>(owner.painter.MakeColor(kGainShade, kGainShade, kGainShade));

    if (bonus < 0)
    {
        color = static_cast<Pixel>(owner.painter.MakeColor(kLossRed, 0, 0));
    }

    int shift = 0;

    for (std::size_t digits = 0; digits < kBonusDigits.size(); ++digits)
    {
        if (value >= kBonusDigits[digits])
        {
            shift = kDigitWidth * static_cast<int>(digits + 1);
        }
    }

    owner.smallFont.DrawText(
        owner.drawSurface,
        shift + x + kBonusPad,
        y,
        WinFormat(kBonusFormat, {static_cast<i32>(bonus <= 0 ? '-' : '+'), std::abs(bonus)}),
        color,
        kPanelStyle
    );
}

// swd32.exe 0x0044A280: the status page: the role's name and faces, its attack, defence and agility over them, its
// level, experience and the experience of the next level, its four attributes with the equipment's gains, its element,
// the resistance table and its slots
void Menu::StatusDraw()
{
    Game& owner = this->game;
    const StatusPage& page = this->status;
    const std::size_t role = std::min<std::size_t>(this->roleTab, Party::Roles - 1);
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    const auto blue = static_cast<Pixel>(owner.painter.MakeColor(kBlueRed, kBlueGreen, kBlueBlue));
    const auto red = static_cast<Pixel>(owner.painter.MakeColor(kRedRed, kRedGreen, kRedBlue));
    const auto green = static_cast<Pixel>(owner.painter.MakeColor(kGreenRed, kGreenGreen, kGreenBlue));
    ActState& panel = owner.battleMessages.panelAct;
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kNameBoxLeft,
        kNameBoxTop,
        kNameBoxRight,
        kNameBoxBottom,
        0,
        kFrameBorder
    );
    const Party::Name& name = owner.party.names[role];
    owner.mediumFont.DrawText(
        owner.drawSurface,
        kNameX,
        kNameY,
        std::string_view(name.data(), strnlen(name.data(), name.size())),
        color,
        kPanelStyle
    );
    FillBox(owner.painter, kLeftBoxX, kLeftBoxY, kLeftBoxWidth, kLeftBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kLeftFrameLeft,
        kLeftFrameTop,
        kLeftFrameRight,
        kLeftFrameBottom,
        0,
        kFrameBorder
    );
    FillBox(owner.painter, kRightBoxX, kLeftBoxY, kRightBoxWidth, kRightBoxHeight, 0, 0, 0, FillMode::Quarter);
    DrawWindowFrame(
        owner.painter,
        owner.archive,
        panel.frame,
        kRightFrameLeft,
        kLeftFrameTop,
        kRightFrameRight,
        kRightFrameBottom,
        0,
        kFrameBorder
    );
    this->face.id = kFaceAct;

    for (std::size_t index = 0; index < kFaceX.size(); ++index)
    {
        this->face.action = kFirstFaceAction + static_cast<i32>(index);
        owner.actDrawer.Draw(this->face, kFaceX[index], kTopY);
    }

    const RoleRecord& total = page.total;
    const std::array<int, 3> top = {
        static_cast<u16>(total.strength) + static_cast<int>(total.attackBonus),
        static_cast<u16>(total.endurance) + static_cast<int>(total.defenseBonus),
        static_cast<u16>(total.agility),
    };

    for (std::size_t index = 0; index < top.size(); ++index)
    {
        owner.largeFont.DrawText(
            owner.drawSurface,
            kTopX[index],
            kTopY,
            WinFormat(kNumberFormat, {top[index]}),
            color,
            kPanelStyle
        );
    }

    owner.largeFont.DrawText(
        owner.drawSurface,
        kLinesX,
        kLevelY,
        WinFormat(kLevelFormat, {static_cast<i32>(total.level)}),
        color,
        kPanelStyle
    );
    u32 next = 0;
    owner.database.LoadLevelEntry(static_cast<int>(role) + 1, total.level + 1, next);
    owner.largeFont.DrawText(owner.drawSurface, kLinesX, kExperienceY, kExperience, color, kPanelStyle);
    owner.mediumFont.DrawText(
        owner.drawSurface,
        kValuesX,
        kExperienceValueY,
        WinFormat(kWideNumberFormat, {owner.party.roles[role].totalExperience}),
        color,
        kPanelStyle
    );
    owner.largeFont.DrawText(owner.drawSurface, kLinesX, kNextLevelY, kNextLevel, color, kPanelStyle);
    owner.mediumFont.DrawText(
        owner.drawSurface,
        kValuesX,
        kNextLevelValueY,
        WinFormat(kWideNumberFormat, {next}),
        color,
        kPanelStyle
    );
    const RoleRecord& record = owner.party.roles[role];
    const std::array<std::u8string_view, 4> formats =
        {kStrengthFormat, kEnduranceFormat, kWisdomFormat, kAgilityFormat};
    const std::array<int, 4> values = {
        static_cast<u16>(record.strength),
        static_cast<u16>(record.endurance),
        static_cast<u16>(record.wisdom),
        static_cast<u16>(record.agility)
    };

    const std::array<int, 4> bonuses =
        {page.bonus.strength, page.bonus.endurance, page.bonus.wisdom, page.bonus.agility};
    const std::array<int, 4> totals = {
        static_cast<u16>(total.strength),
        static_cast<u16>(total.endurance),
        static_cast<u16>(total.wisdom),
        static_cast<u16>(total.agility)
    };

    for (std::size_t index = 0; index < formats.size(); ++index)
    {
        owner.largeFont.DrawText(
            owner.drawSurface,
            kLinesX,
            kAttributeY[index],
            WinFormat(formats[index], {values[index]}),
            color,
            kPanelStyle
        );
        this->StatusDrawBonus(bonuses[index], kValuesX, kAttributeY[index] + kBonusDown, totals[index]);
    }

    std::string element = Big5(kElement);
    element += Big5(kRoleElements[role]);
    owner.largeFont.DrawText(owner.drawSurface, kLinesX, kElementY, element, color, kPanelStyle);
    owner.mediumFont.DrawText(owner.drawSurface, kTableX, kTableTop, kResistances, color, kPanelStyle);

    for (std::size_t line = 0; line < kFirstColumn.size(); ++line)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            kTableX,
            kTableTop + kTableLine * static_cast<int>(line + 1),
            kFirstColumn[line],
            color,
            kPanelStyle
        );
    }

    for (std::size_t line = 0; line < kSecondColumn.size(); ++line)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            kSecondTableX,
            kTableTop + kTableLine * static_cast<int>(line + 1),
            kSecondColumn[line],
            color,
            kPanelStyle
        );
    }

    Pixel shade = green;

    for (std::size_t element = 0; element < total.resistances.size(); ++element)
    {
        const auto value = static_cast<i8>(total.resistances[element]);
        std::string text;

        if (value == 0)
        {
            text = std::string(kNoResistance);
            shade = color;
        }

        if (value > 0)
        {
            text = WinFormat(kWeakFormat, {static_cast<i32>(value)});
            shade = red;
        }

        if (value < 0 && value > kAbsorbFrom)
        {
            text = WinFormat(kResistFormat, {-static_cast<i32>(value)});
            shade = blue;
        }

        if (value <= kAbsorbFrom)
        {
            text = WinFormat(kAbsorbFormat, {kAbsorbFrom - static_cast<i32>(value)});
            shade = green;
        }

        const auto index = static_cast<int>(element);
        owner.smallFont.DrawText(
            owner.drawSurface,
            kValuesColumn * (index / kValuesPerColumn) + kValuesTableX,
            kTableLine * (index % kValuesPerColumn) + kValuesTableY,
            text,
            shade,
            kPanelStyle
        );
    }

    this->EquipDrawSlots(kSlotsX, kSlotsY, 0, kSlotsWidth, kNoSlot);
}

}  // namespace swd3
