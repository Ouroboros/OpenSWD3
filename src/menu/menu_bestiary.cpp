#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "core/debug_log.h"
#include "core/format.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "menu/menu_list.h"
#include "org/database.h"
#include "org/fame.h"
#include "platform/display.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr i32 kBestiaryBackground = 0x33;  // Menu::background actions
constexpr i32 kItemsBackground = 0x43;
constexpr u16 kMoveSound = 0x2E;
constexpr int kLastCategory = 0xB;
constexpr int kMouseLastCategory = 0xE;  // the mouse stops the categories here (past the twelve shown)
constexpr std::size_t kItemLines = 9;    // Bestiary::lines: nine lines of values, then the three objects
constexpr u16 kSubpageCode = 0x1F4;      // Menu::code while a subpage is open
constexpr u16 kRolePagesCode = 2;

// The categories shown in turn (the tabs 0..11) as values of a monster's category byte.
constexpr std::array<i8, Bestiary::Categories> kCategoryValues = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xD, 0xE};
// swd32.exe 0x0049E370: the names of the categories, eight bytes each.
constexpr std::array<std::u8string_view, 0xF> kCategoryNames = {
    u8"天神",
    u8"魔神",
    u8"天使",
    u8"活屍",
    u8"神獸",
    u8"魔獸",
    u8"仙靈",
    u8"死靈",
    u8"妖怪",
    u8"精怪",
    u8"QQ",
    u8"靈藥",
    u8"法寶",
    u8"魔機關",
    u8"生物",
};

// What the defeats teach (Bestiary::known): the line is exact from this many, approximate one or two before.
constexpr int kKnowsCategory = 1;
constexpr int kKnowsLevel = 2;
constexpr int kKnowsName = 3;
constexpr int kKnowsHitPoints = 5;
constexpr int kKnowsSpirit = 8;
constexpr int kKnowsStamina = 0xA;
constexpr int kKnowsAttack = 0xD;
constexpr int kKnowsDefense = 0x10;
constexpr int kKnowsAgility = 0x12;
constexpr int kKnowsItems = 0x13;
constexpr int kVitalMost = 0x270F;
constexpr int kAttributeMost = 0x3E7;
constexpr int kExactGap = 0;
constexpr int kNearGap = 3;  // KnownValue: below it the value is shown with an error
constexpr int kSmallRange = 0xA;
constexpr int kMiddleRange = 0x64;
constexpr int kLargeRange = 0x3E8;
constexpr int kSpiritItemA = 2;  // Bestiary::spirit: the level times this for each object the monster carries
constexpr int kSpiritItemB = 3;
constexpr int kSpiritItemC = 5;
constexpr int kSpiritHealA = 2;
constexpr int kSpiritHealB = 4;
constexpr int kStaminaSkillA = 3;
constexpr int kStaminaSkillB = 5;
constexpr int kPercentPerDefeat = 5;

constexpr std::string_view kUnknownShort = "????";
constexpr std::u8string_view kUnknownLevel = u8"????級";  // swd32.exe 0x004A6C48
constexpr std::u8string_view kLevelFormat = u8"%4d級";    // swd32.exe 0x004A6BF8
constexpr std::string_view kUnknownItem = "????????????";
constexpr std::string_view kUnknownName = "????????";
constexpr std::string_view kPercentFormat = "%3d%%";
constexpr std::string_view kNameFormat = "%-12s";
constexpr std::string_view kLabelFormat = "%4s  ";
constexpr std::u8string_view kExactFormat = u8"%s 是 %4d";      // swd32.exe 0x004A6C80
constexpr std::u8string_view kNearFormat = u8"%4s 大概是 %4d";  // swd32.exe 0x004A6C58
constexpr std::u8string_view kFarFormat = u8"%4s 似乎是 %4d";   // swd32.exe 0x004A6C68
constexpr std::string_view kUnknownValue = " ???";
constexpr std::size_t kUnknownMark = 7;  // KnownValue: the original sets this byte to '?' for large maxima
constexpr std::u8string_view kHitPointsLabel = u8"生命";  // swd32.exe 0x004A6BF0
constexpr std::u8string_view kSpiritLabel = u8"靈力";
constexpr std::u8string_view kStaminaLabel = u8"體力";
constexpr std::u8string_view kAttackLabel = u8"攻擊";
constexpr std::u8string_view kDefenseLabel = u8"防禦";
constexpr std::u8string_view kAgilityLabel = u8"敏捷";

// The layout.
constexpr u32 kTextRed = 0x19;
constexpr u32 kTextGreen = 0x17;
constexpr u32 kTextBlue = 0x11;
constexpr int kPanelStyle = 4;
constexpr int kListX = 0x12;
constexpr int kListTop = 0x5E;
constexpr int kListEnd = 0x1C6;
constexpr int kRowHeight = 0x18;
constexpr int kPercentX = 0xA8;
constexpr int kPercentDrop = 9;
constexpr int kLinesX = 0xF6;
constexpr int kLinesTop = 0x48;
constexpr int kLineHeight = 0x14;
constexpr int kItemsY = 0x126;
constexpr int kSecondItemX = 0x174;
constexpr int kThirdItemX = 0xF228;  // the original's third object, far off the screen
constexpr int kDescriptionX = 0xF2;
constexpr int kDescriptionY = 0x150;
constexpr int kDescriptionLines = 5;
constexpr int kDescriptionWidth = 0x168;
constexpr int kPictureX = 0x1FC;
constexpr int kPictureY = 0x3C;
constexpr i32 kPictureAction = 0x44;
constexpr int kHighlightX = 0xE;
constexpr int kHighlightWidth = 0xBD;
constexpr int kHighlightHeight = 0x18;
constexpr int kHighlightRed = 0x14;
constexpr int kHighlightGreen = 0xD;
constexpr int kBarX = 0xCE;
constexpr int kBarY = 0x62;
constexpr int kBarHeight = 0x15E;
constexpr u16 kScrollUp = 3;
constexpr u16 kScrollDown = 0x30;
constexpr u16 kScrollUpMask = 0xF;
constexpr u16 kScrollDownMask = 0xF0;
constexpr int kNibbleShift = 4;
constexpr u16 kTabsPicture = 0x2439;
constexpr u16 kTabPicture = 0x243A;
constexpr int kTabsClipLeft = 0xA;
constexpr int kTabsClipTop = 1;
constexpr int kTabsClipRight = 0xCE;
constexpr int kTabsClipBottom = 0x1DE;
constexpr int kTabsLeft = 6;
constexpr int kTabsY = 0x3D;
constexpr int kTabStep = 0x28;
constexpr int kTabsAround = 2;
constexpr int kTabX = 0x56;
constexpr int kTabY = 0x3A;

// The mouse areas.
constexpr u32 kRowsLeft = 0x12;
constexpr u32 kRowsRight = 0xCB;
constexpr u32 kRowsTop = 0x5E;
constexpr u32 kRowsBottom = 0x1C6;
constexpr u32 kTabsAreaLeft = 0xA;
constexpr u32 kTabsAreaRight = 0xCE;
constexpr u32 kTabsAreaTop = 0x3C;
constexpr u32 kTabsAreaBottom = 0x4E;
constexpr int kTabsMiddle = 0x6A;
constexpr int kTabWidth = 0x14;
constexpr u32 kBarLeft = 0xCE;
constexpr u32 kBarRight = 0xE0;
constexpr i32 kBarUpTop = 0x52;
constexpr i32 kBarUpBottom = 0x60;
constexpr i32 kBarDownTop = 0x1C4;
constexpr i32 kBarDownBottom = 0x1D0;
constexpr u32 kLeftClicks = 3;
constexpr u32 kRightClicks = 0xC;

// The wrapped text (DrawWrappedText).
constexpr u16 kBreakCode = 0x4E25;  // "%N"
constexpr u16 kColorCode = 0x4325;  // "%C" and a digit
constexpr u16 kEndCode = 0x5125;    // "%Q"
constexpr std::size_t kColorCodeBytes = 3;
constexpr int kGlyphGap = 0xA;  // DrawWrappedText: x moves by 11 per character drawn before a colour change
constexpr int kWrappedLineHeight = 0x19;
constexpr u32 kShadowRed = 6;
constexpr u32 kShadowGreen = 4;
constexpr u32 kShadowBlue = 3;

u16 TextWord(std::string_view text, std::size_t at)
{
    return at + 1 < text.size()
               ? static_cast<u16>(static_cast<u8>(text[at]) | static_cast<u8>(text[at + 1]) << CHAR_BIT)
               : u16{0};
}

}  // namespace

// The name of category `category` of the table at 0x0049E370 (eight bytes each; the original reads on past its fifteen
// names, here none).
std::u8string_view CategoryName(u16 category)
{
    return category < kCategoryNames.size() ? kCategoryNames[category] : std::u8string_view();
}

// swd32.exe 0x004306C0: `text` (to %Q) in the large font from (x, y), wrapped where it passes `width` (the text cell
// times the characters) or at %N for up to `lines` lines; %C and a digit change the colour to that digit (taken as a
// colour value). Past the last line the text goes on over it.
void Game::DrawWrappedText(std::string_view text, int x, int y, int lines, int width)
{
    int before = 0;
    int length = 0;
    int line = 1;
    int wrapped = 0;
    int lineY = y;
    std::size_t at = 0;
    std::string buffer;
    auto color = static_cast<Pixel>(this->painter.MakeColor(kTextRed, kTextGreen, kTextBlue));
    this->largeFont.SetBackground(TextFont::NoBackground);
    this->largeFont.SetShadow(static_cast<Pixel>(this->painter.MakeColor(kShadowRed, kShadowGreen, kShadowBlue)));

    auto flush = [this, &buffer, &before, x, &lineY, &color]
    {
        this->largeFont.DrawText(this->drawSurface, before + x + kGlyphGap * before, lineY, buffer, color, kPanelStyle);
        buffer.clear();
    };

    if (TextWord(text, 0) != kEndCode)
    {
        do
        {
            if (at >= text.size() || text[at] == '\0')
            {
                break;
            }

            const bool breaks = TextWord(text, at) == kBreakCode || this->talk.unit * (before + length) > width;

            if (breaks)
            {
                flush();
            }

            if (breaks && line < lines)
            {
                length = 0;
                before = 0;
                ++wrapped;
                ++line;
                lineY += kWrappedLineHeight;

                if (TextWord(text, at) == kBreakCode)
                {
                    at += sizeof(u16);
                }
            }
            else if (TextWord(text, at) == kColorCode)
            {
                at += kColorCodeBytes;
                flush();
                before += length;
                color = static_cast<Pixel>(text[at - 1] - '0');
                length = 0;
            }
            else
            {
                const auto lead = static_cast<signed char>(text[at]);
                buffer.push_back(text[at++]);
                ++length;

                if (lead < 0 && at < text.size())
                {
                    buffer.push_back(text[at++]);
                    ++length;
                }
            }
        } while (TextWord(text, at) != kEndCode);
    }

    this->largeFont.DrawText(
        this->drawSurface,
        before + x + kGlyphGap * before,
        y + kWrappedLineHeight * wrapped,
        buffer,
        color,
        kPanelStyle
    );
}

// swd32.exe 0x0043C0D0: the bestiary opened: the category of every monster of mon.dat 1..500 and how many of each the
// party defeated (the first fame list), the first category listed and its first monster chosen
void Menu::BestiaryEnter()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    book.categories.fill(-1);

    for (int id = 1; id <= Bestiary::Monsters; ++id)
    {
        ObjectRecord record;

        if (owner.database.LoadObject(record, static_cast<u16>(id)))
        {
            book.categories[static_cast<std::size_t>(id)] = static_cast<i8>(record.data.unk52);
        }
    }

    book.defeated.fill(0);

    for (int id = 1; id <= Bestiary::Monsters; ++id)
    {
        book.defeated[static_cast<std::size_t>(id)] =
            static_cast<u8>(FindCounterValue(owner.party.fame.lists[Fame::FameList], static_cast<i16>(id)));
    }

    book.known.fill(0);
    book.lines.fill(std::string());
    book.names.fill(std::string());
    book.ids.fill(0);
    book.firstShown = 0;
    book.row = 0;
    book.top = 0;
    book.shown = 0;
    book.count = 0;
    book.category = 0;
    this->BestiaryList(0);
    this->background.id = Scene::InterfaceAct;
    this->background.action = kBestiaryBackground;
    this->BestiaryChoose(book.ids[static_cast<std::size_t>(book.top + book.row)]);
    book.scrollArrows = 0;
}

// swd32.exe 0x0043C2F0: the bestiary closed: its tables freed, the items page's background back
void Menu::BestiaryLeave()
{
    Bestiary& book = this->bestiary;
    book.monster = ItemNode{};
    book.lines.fill(std::string());
    book.names.fill(std::string());
    this->background.id = Scene::InterfaceAct;
    this->background.action = kItemsBackground;
}

// swd32.exe 0x0043C3C0: the mouse on the bestiary: a click on a row chooses it, on the tabs a category beside the
// chosen one; a held left button on the scroll bar's arrows or beside its thumb scrolls; a right click goes back
void Menu::BestiaryFrame()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    const auto x = static_cast<u32>(owner.state.pointerX);
    const auto y = static_cast<u32>(owner.state.pointerY);

    if (y < kRowsBottom && y > kRowsTop && x < kRowsRight && x > kRowsLeft && (this->clicks & kLeftClicks) != 0)
    {
        i32 row = static_cast<i32>((y - kRowsTop) / kRowHeight);

        if (row >= book.shown)
        {
            row = book.shown - 1;
        }

        book.row = row - 1;
        this->BestiaryDown();
        return;
    }

    if (y < kTabsAreaBottom && y > kTabsAreaTop && x < kTabsAreaRight && x > kTabsAreaLeft &&
        (this->clicks & kLeftClicks) != 0)
    {
        const int step = (owner.state.pointerX - kTabsMiddle) / kTabWidth;

        if (step < 0 && book.category == 0)
        {
            return;
        }

        if (step > 0 && book.category == kMouseLastCategory)
        {
            return;
        }

        if (step != 0)
        {
            book.category += (step <= 0 ? -1 : 1) - 1;
            this->BestiaryShift();
        }

        return;
    }

    if (owner.controls.Repeating(Button::MouseLeft) && x < kBarRight && x > kBarLeft)
    {
        const MenuScrollBar& bar = this->scrollBar;
        const i32 pointerY = owner.state.pointerY;

        if (pointerY < kBarUpBottom && pointerY > kBarUpTop)
        {
            this->BestiaryUp();
        }

        if (owner.state.pointerY < kBarDownBottom && owner.state.pointerY > kBarDownTop)
        {
            this->BestiaryDown();
        }

        if (owner.state.pointerY < bar.thumbTop && owner.state.pointerY > bar.top)
        {
            this->BestiaryPageUp();
        }

        if (owner.state.pointerY < bar.bottom && owner.state.pointerY > bar.thumbBottom)
        {
            this->BestiaryPageDown();
        }

        return;
    }

    if ((this->clicks & kRightClicks) != 0)
    {
        this->BestiaryCancel();
    }
}

// The monster of the row chosen shown after a move (inline in the bestiary's keys).
void Menu::BestiaryMoved(u16 arrows)
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    this->BestiaryShowFrom(book.top);
    this->BestiaryCountShown();
    this->BestiaryChoose(
        book.ids[static_cast<std::size_t>(std::clamp(book.row + book.top, 0, static_cast<int>(Bestiary::Listed) - 1))]
    );
    book.scrollArrows = static_cast<u16>(book.scrollArrows | arrows);
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0043C520: the next monster
void Menu::BestiaryDown()
{
    Bestiary& book = this->bestiary;
    ListDown(book.count, book.top, book.row, book.shown);
    this->BestiaryMoved(kScrollDown);
}

// swd32.exe 0x0043C590: the monster before
void Menu::BestiaryUp()
{
    Bestiary& book = this->bestiary;
    ListUp(book.top, book.row);
    this->BestiaryMoved(kScrollUp);
}

// swd32.exe 0x0043C600: a page of fifteen on
void Menu::BestiaryPageDown()
{
    Bestiary& book = this->bestiary;
    ListPageDown(book.count, book.top, book.row, book.shown, Bestiary::Rows);
    this->BestiaryMoved(kScrollDown);
}

// swd32.exe 0x0043C670: a page of fifteen back
void Menu::BestiaryPageUp()
{
    Bestiary& book = this->bestiary;
    ListPageUp(book.top, book.row, Bestiary::Rows);
    this->BestiaryMoved(kScrollUp);
}

// swd32.exe 0x0043C6E0: the category before
void Menu::BestiaryLeft()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;

    if (--book.category < 0)
    {
        book.category = 0;
    }

    this->BestiaryList(book.category);
    this->BestiaryShowFrom(book.top);
    this->BestiaryCountShown();
    this->BestiaryChoose(
        book.ids[static_cast<std::size_t>(std::clamp(book.top + book.row, 0, static_cast<int>(Bestiary::Listed) - 1))]
    );
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0043C760: the next category (up to the twelfth)
void Menu::BestiaryRight()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;

    if (++book.category > kLastCategory)
    {
        book.category = kLastCategory;
    }

    this->BestiaryList(book.category);
    this->BestiaryShowFrom(book.top);
    this->BestiaryCountShown();
    this->BestiaryChoose(
        book.ids[static_cast<std::size_t>(std::clamp(book.top + book.row, 0, static_cast<int>(Bestiary::Listed) - 1))]
    );
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0043C7E0: Shift (and a click on the tabs): the next category, with a second sound
void Menu::BestiaryShift()
{
    Game& owner = this->game;
    this->BestiaryRight();
    owner.audio.PlaySound(kMoveSound, owner.config.soundVolume);
}

// swd32.exe 0x0043C800: Escape from the bestiary opened as a subpage: back to the role pages
void Menu::BestiaryCancel()
{
    if (this->code == kSubpageCode)
    {
        this->code = kRolePagesCode;
        this->BestiaryLeave();
    }
}

// swd32.exe 0x0043C820: the bestiary: the scroll bar (over fifteen monsters), the category tabs, the rows (the chosen
// one highlighted with its picture and lines)
void Menu::BestiaryDraw()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    const auto color = static_cast<Pixel>(owner.painter.MakeColor(kTextRed, kTextGreen, kTextBlue));

    if (book.count > Bestiary::Rows)
    {
        int arrows = 0;

        if ((book.scrollArrows & kScrollUpMask) != 0)
        {
            book.scrollArrows =
                static_cast<u16>((book.scrollArrows & ~kScrollUpMask) | ((book.scrollArrows & kScrollUpMask) - 1));
            arrows = 1;
        }

        if ((book.scrollArrows & kScrollDownMask) != 0)
        {
            const int down = (book.scrollArrows & kScrollDownMask) >> kNibbleShift;
            arrows |= 2;
            book.scrollArrows = static_cast<u16>((book.scrollArrows & ~kScrollDownMask) | ((down - 1) << kNibbleShift));
        }

        const auto count = static_cast<double>(book.count);
        const auto to = static_cast<float>(static_cast<double>(book.top + book.shown) / count);
        const auto from = static_cast<float>(static_cast<double>(book.top) / count);
        this->DrawScrollBar(kBarX, kBarY, kBarHeight, arrows, from, to, this->scrollBar);
    }

    this->DrawBestiaryTabs();
    int rowY = kListTop;

    for (std::size_t entry = book.firstShown, row = 0;
         entry < book.ids.size() && book.ids[entry] != 0 && rowY < kListEnd;
         ++entry, ++row, rowY += kRowHeight)
    {
        bool chosen = false;

        if (book.row == static_cast<i32>(row))
        {
            const std::size_t index = static_cast<std::size_t>(book.top) + row;

            if (index < book.names.size() && !book.names[index].empty())
            {
                this->gauge.id = static_cast<i32>(book.ids[index]);
                this->gauge.action = kPictureAction;
                this->gauge.direction = 0;
                owner.actDrawer.Draw(this->gauge, kPictureX, kPictureY);
            }

            chosen = true;
        }

        this->DrawBestiaryRow(book.top + static_cast<int>(row), static_cast<int>(row), color, chosen);

        if (book.row == static_cast<i32>(row))
        {
            FillBox(
                owner.painter,
                kHighlightX,
                rowY,
                kHighlightWidth,
                kHighlightHeight,
                kHighlightRed,
                kHighlightGreen,
                0,
                FillMode::Gradient
            );
        }
    }
}

// swd32.exe 0x0043C9C0: the monsters of category tab `category` listed (at most 64; the original writes on past its
// list), the names and defeat counts of those defeated filled in; the first row chosen
void Menu::BestiaryList(int category)
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    book.ids.fill(0);
    book.names.fill(std::string());
    const i8 value = kCategoryValues[static_cast<std::size_t>(std::clamp(category, 0, Bestiary::Categories - 1))];
    book.known.fill(0);
    std::size_t listed = 0;

    for (int id = 1; id <= Bestiary::Monsters; ++id)
    {
        if (book.categories[static_cast<std::size_t>(id)] == value && listed < Bestiary::Listed)
        {
            book.ids[listed] = static_cast<u32>(id);
            book.names[listed].clear();
            book.known[listed] = 0;
            ++listed;
        }
    }

    book.count = static_cast<i32>(listed);

    if (listed < book.ids.size())
    {
        book.ids[listed] = 0;
    }

    for (int id = 1; id <= Bestiary::Monsters; ++id)
    {
        if (book.defeated[static_cast<std::size_t>(id)] == 0)
        {
            continue;
        }

        for (std::size_t entry = 0; entry < Bestiary::Listed; ++entry)
        {
            if (book.ids[entry] != static_cast<u32>(id))
            {
                continue;
            }

            ObjectRecord record;

            if (owner.database.LoadObject(record, static_cast<u16>(id)))
            {
                book.names[entry] = ObjectName(record);
                book.known[entry] = static_cast<i8>(book.defeated[static_cast<std::size_t>(id)]);
            }
            else
            {
                DebugLog(u8"怪物集合 物品資料無法讀取(%d)", id);  // 0x004A6B48 + 0x004A6B54
            }
        }
    }

    book.top = 0;
    book.row = 0;
    book.firstShown = 0;
    this->BestiaryCountShown();
}

// swd32.exe 0x0043CBD0: the rows shown from the first one shown (at most fifteen)
void Menu::BestiaryCountShown()
{
    Bestiary& book = this->bestiary;
    book.shown = 0;

    for (std::size_t entry = book.firstShown;
         entry < book.ids.size() && book.ids[entry] != 0 && book.shown < Bestiary::Rows;
         ++entry)
    {
        ++book.shown;
    }
}

// swd32.exe 0x0043CC20: row `row` of the bestiary (listed monster `index`): its name ("????????" unless defeated) and
// how much is known of it; for the row chosen the lines of the monster, its objects and its description
void Menu::DrawBestiaryRow(int index, int row, Pixel color, bool chosen)
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    const auto entry = static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(Bestiary::Listed) - 1));
    const std::string& name = book.names[entry];
    const std::string text = name.empty() ? std::string(kUnknownName) : WinFormat(kNameFormat, {name});
    owner.largeFont.DrawText(owner.drawSurface, kListX, kRowHeight * row + kListTop, text, color, kPanelStyle);

    if (!name.empty())
    {
        const std::string percent = WinFormat(kPercentFormat, {kPercentPerDefeat * book.known[entry]});
        owner.smallFont.DrawText(
            owner.drawSurface,
            kPercentX,
            kRowHeight * row + kListTop + kPercentDrop,
            percent,
            color,
            kPanelStyle
        );
    }

    if (!chosen)
    {
        return;
    }

    for (std::size_t line = 0; line < kItemLines; ++line)
    {
        owner.mediumFont.DrawText(
            owner.drawSurface,
            kLinesX,
            kLinesTop + kLineHeight * static_cast<int>(line),
            book.lines[line],
            color,
            kPanelStyle
        );
    }

    if (name.empty())
    {
        return;
    }

    const std::array<int, 3> itemX = {kLinesX, kSecondItemX, kThirdItemX};

    for (std::size_t item = 0; item < itemX.size(); ++item)
    {
        const std::string& itemName = book.lines[kItemLines + item];

        if (!itemName.empty() && itemName[0] != '?')
        {
            owner.mediumFont.DrawText(owner.drawSurface, itemX[item], kItemsY, itemName, color, kPanelStyle);
        }
    }

    const std::string description = book.monster.record.description.value_or(std::string());
    owner.DrawWrappedText(description, kDescriptionX, kDescriptionY, kDescriptionLines, kDescriptionWidth);
}

// swd32.exe 0x0043CEF0: monster `id` chosen: its record loaded, the spirit and stamina it would have (from its level
// and the objects and skills it carries) and its lines
void Menu::BestiaryChoose(u32 id)
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    book.monster = ItemNode{};
    book.spirit = 0;
    book.stamina = 0;

    if (id == 0)
    {
        return;
    }

    book.monster.id = static_cast<u16>(id);
    book.monster.count = 1;
    book.monster.pending = 0;
    book.monster.unk06 = 0;

    if (!owner.database.LoadObject(book.monster.record, static_cast<u16>(id)))
    {
        DebugLog(u8"讀取道具(book) 沒有定義物品號碼(%d)", id);  // 0x004A6B90 + 0x004A6BA0
    }

    const ObjectData& data = book.monster.record.data;
    const int level = data.level;
    int spirit = 0;

    if (data.itemA != 0)
    {
        spirit = kSpiritItemA * level;
    }

    if (data.itemB != 0)
    {
        spirit += kSpiritItemB * level;
    }

    if (data.itemC != 0)
    {
        spirit += kSpiritItemC * level;
    }

    if (data.healItemA != 0)
    {
        spirit += kSpiritHealA * level;
    }

    book.spirit = spirit;

    if (data.healItemB != 0)
    {
        book.spirit = spirit + kSpiritHealB * level;
    }

    int stamina = 0;

    if (data.skillA != 0)
    {
        stamina = kStaminaSkillA * level;
    }

    book.stamina = stamina;

    if (data.skillB != 0)
    {
        book.stamina = stamina + kStaminaSkillB * level;
    }

    this->BestiaryLines(book.top + book.row);
}

// swd32.exe 0x0043D370: a line of a value known from `known` defeats: exact from `needed`, one or two before within a
// random error (of 10, 100 or 1000 by the value's size, a third or a half of it), earlier "???"
std::string Menu::KnownValue(std::u8string_view label, int known, int needed, int value, int most)
{
    int range = kSmallRange;

    if (value > kMiddleRange)
    {
        range = kMiddleRange;
    }

    if (value > kLargeRange)
    {
        range = kLargeRange;
    }

    const int gap = needed - known;
    std::string line = WinFormat(kLabelFormat, {label});

    if (gap <= kExactGap)
    {
        return WinFormat(kExactFormat, {label, value});
    }

    if (gap < kNearGap)
    {
        const int spread = range / (kNearGap - gap);
        int shown = static_cast<int>(this->game.random.Below(static_cast<u32>(spread))) - spread / 2 + value;
        shown = std::clamp(shown, 0, std::max(most, 0));
        return WinFormat(gap == 1 ? kNearFormat : kFarFormat, {label, shown});
    }

    line += kUnknownValue;

    if (most > kLargeRange && line.size() > kUnknownMark)
    {
        line[kUnknownMark] = '?';
    }

    return line;
}

// swd32.exe 0x0043D050: the lines of listed monster `index` by its defeats: its category, level and name, its six
// values and the three objects it carries (each named once)
void Menu::BestiaryLines(int index)
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    const auto entry = static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(Bestiary::Listed) - 1));
    const int known = book.known[entry];
    const ObjectData& data = book.monster.record.data;
    book.lines[0] = std::string(kUnknownShort);
    book.lines[1] = Big5(kUnknownLevel);
    book.lines[2] = std::string(kUnknownShort);

    if (known >= kKnowsCategory)
    {
        const i8 category = book.categories[std::min<std::size_t>(book.ids[entry], Bestiary::Monsters)];
        book.lines[0] = Big5(CategoryName(static_cast<u8>(category)));
    }

    if (known >= kKnowsLevel)
    {
        book.lines[1] = WinFormat(kLevelFormat, {static_cast<i32>(data.level)});
    }

    if (known >= kKnowsName)
    {
        book.lines[2] = WinFormat(kNameFormat, {ObjectName(book.monster.record)});
    }

    book.lines[3] = this->KnownValue(kHitPointsLabel, known, kKnowsHitPoints, data.hitPoints, kVitalMost);
    book.lines[4] = this->KnownValue(kSpiritLabel, known, kKnowsSpirit, book.spirit, kVitalMost);
    book.lines[5] = this->KnownValue(kStaminaLabel, known, kKnowsStamina, book.stamina, kVitalMost);
    book.lines[6] = this->KnownValue(kAttackLabel, known, kKnowsAttack, data.attack, kAttributeMost);
    book.lines[7] = this->KnownValue(kDefenseLabel, known, kKnowsDefense, data.defense, kAttributeMost);
    book.lines[8] = this->KnownValue(kAgilityLabel, known, kKnowsAgility, data.agility, kAttributeMost);

    for (std::size_t item = 0; item < 3; ++item)
    {
        book.lines[kItemLines + item] = std::string(kUnknownItem);
    }

    if (known < kKnowsItems)
    {
        return;
    }

    auto nameOf = [&owner](u16 id) -> std::string
    {
        ObjectRecord record;
        return id != 0 && owner.database.LoadObject(record, id) ? ObjectName(record) : std::string();
    };

    const std::string first = nameOf(data.itemA);

    if (!first.empty())
    {
        book.lines[kItemLines] = first;
    }

    const std::string second = nameOf(data.itemB);

    if (!second.empty() && second != book.lines[kItemLines])
    {
        book.lines[kItemLines + 1] = second;
    }

    const std::string third = nameOf(data.itemC);

    if (!third.empty() && third != book.lines[kItemLines] && third != book.lines[kItemLines + 1])
    {
        book.lines[kItemLines + 2] = third;
    }
}

// swd32.exe 0x0043D470: the category tabs: the two on each side of the chosen one, then the chosen one over them
void Menu::DrawBestiaryTabs()
{
    Game& owner = this->game;
    Bestiary& book = this->bestiary;
    owner.painter.SetClipClamped(kTabsClipLeft, kTabsClipTop, kTabsClipRight, kTabsClipBottom);
    int x = kTabsLeft;

    for (int tab = book.category - kTabsAround; tab <= book.category + kTabsAround; ++tab, x += kTabStep)
    {
        if (static_cast<u32>(tab) < static_cast<u32>(Bestiary::Categories) && tab != book.category)
        {
            const Picture* picture = owner.archive.GetPicture(kTabsPicture, static_cast<u32>(tab));

            if (picture != nullptr)
            {
                owner.painter.SetSource(picture->data.data());
                owner.painter.DrawPicture(x, kTabsY, picture->width, picture->height, 0);
            }
        }
    }

    const Picture* chosen = owner.archive.GetPicture(kTabPicture, static_cast<u32>(book.category));

    if (chosen != nullptr)
    {
        owner.painter.SetSource(chosen->data.data());
        owner.painter.DrawPicture(kTabX, kTabY, chosen->width, chosen->height, 0);
    }

    owner.painter.SetClipClamped(0, kTabsClipTop, Display::Width, kTabsClipBottom);
}

// swd32.exe 0x0043CC00: the first listed shown: entry `top` of the list (a pointer into it in the original)
void Menu::BestiaryShowFrom(int top)
{
    this->bestiary.firstShown = static_cast<std::size_t>(std::max(top, 0));
}

}  // namespace swd3
