#include <algorithm>
#include <cmath>

#include "core/log.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/game.h"
#include "menu/menu.h"
#include "platform/display.h"
#include "platform/system_info.h"

namespace swd3
{

// What the drawing of a menu frame (Menu::PaintPage and the pages it draws, Game::DrawShop) changes, kept from before
// it so that a painting between frames runs it again from the same state, and from after it to be put back.
struct Menu::FrameState
{
    u16 code = 0;
    u16 page = 0;
    u16 mode = 0;
    u16 unk4FC200 = 0;
    u16 unk4FBAB8 = 0;
    u16 unk4FBECC = 0;
    u16 unk4FC82C = 0;
    u16 unk4A6AA8 = 0;
    i32 tint = 0;
    i32 frames = 0;
    u32 flags = 0;
    i32 pointerX = 0;
    i32 pointerY = 0;
    i32 escapeDelay = 0;
    u32 clicks = 0;
    i32 slide = 0;
    i32 lastPage = 0;
    i32 titleGlow = 0;
    std::array<i8, Menu::Tabs> tabGlow = {};
    std::array<MenuTab, Menu::Tabs> tabs = {};
    std::array<Handler, 13> handlers = {};
    std::array<ActState, 18> acts;
    MenuScrollBar scrollBar;
    MenuScrollBar townBar;
    MenuScrollBar unk4FBAB4;
    i32 townTop = 0;
    i32 townRow = 0;
    i32 townShown = 0;
    i32 descriptionSlide = 0;
    i32 descriptionTop = 0;
    i32 unk4FC904 = 0;
    std::string description;
    OptionsPage options;
    InventoryPage itemsPage;

    // The equipment page.
    ItemNode* equipInventoryHead = nullptr;
    i32 equipOpenedAt = 0;
    i32 equipSlide = 0;
    i32 equipUnk4FCD30 = 0;
    i32 equipShown = 0;
    i32 equipSlot = 0;
    i32 equipRow = 0;
    i32 equipListX = 0;
    i32 equipListY = 0;
    Handler equipSavedBeforeFrame = nullptr;
    RoleRecord equipWorn;
    i32 equipStage = 0;
    ItemNode* equipDescribed = nullptr;
    i32 equipUnk4FCD5C = 0;
    ItemNode* equipFirstShown = nullptr;
    u16 equipScrollArrows = 0;
    i32 equipTop = 0;
    std::array<EquipStats, Party::Roles + 1> equipStats = {};
    i32 equipCount = 0;
    i32 equipPreviousStage = 0;

    // The skills page.
    ItemNode* skillsFirstShown = nullptr;
    i32 skillsTop = 0;
    i32 skillsRoles = 0;
    i32 skillsShown = 0;
    i32 skillsButtonOffset = 0;
    i32 skillsTab = 0;
    i32 skillsTarget = 0;
    i32 skillsTabs = 0;
    i32 skillsDescriptionSlide = 0;
    i32 skillsCount = 0;
    i32 skillsRow = 0;
    i32 skillsShownTabs = 0;
    i32 skillsStage = 0;
    i32 skillsDescriptionTop = 0;
    u16 skillsScrollArrows = 0;
    SkillScript skill;

    // The bestiary.
    std::size_t bestiaryFirstShown = 0;
    i32 bestiaryTop = 0;
    i32 bestiaryCategory = 0;
    i32 bestiaryShown = 0;
    i32 bestiaryRow = 0;
    u16 bestiaryScrollArrows = 0;

    // The game.
    Random random;
    ActState pointerAct;
    ActState panelAct;
    ActState shopTitle;
    i32 lastPointerAction = 0;
    u32 menuState = 0;
    i32 menuMode = 0;
    i32 storyState = 0;
    i32 inactive = 0;
    u32 systemFlags = 0;
    i32 saveScreenPage = 0;
    i32 slotsLoad = 0;
    std::array<i16, Party::Roles> faceSlides = {};
    u16 shopScrollArrows = 0;
    bool actsRestarted = false;
    Painter::DrawState painter;
    std::array<TextFont::DrawState, 3> fonts;
};

Menu::Menu(Game& game)
    : frameBefore(std::make_unique<FrameState>()),
      frameAfter(std::make_unique<FrameState>()),
      game(game)
{
}

Menu::~Menu() = default;

void Menu::SaveFrame(FrameState& kept) const
{
    const Game& owner = this->game;
    kept.code = this->code;
    kept.page = this->page;
    kept.mode = this->mode;
    kept.unk4FC200 = this->unk4FC200;
    kept.unk4FBAB8 = this->unk4FBAB8;
    kept.unk4FBECC = this->unk4FBECC;
    kept.unk4FC82C = this->unk4FC82C;
    kept.unk4A6AA8 = this->unk4A6AA8;
    kept.tint = this->tint;
    kept.frames = this->frames;
    kept.flags = this->flags;
    kept.pointerX = this->pointerX;
    kept.pointerY = this->pointerY;
    kept.escapeDelay = this->escapeDelay;
    kept.clicks = this->clicks;
    kept.slide = this->slide;
    kept.lastPage = this->lastPage;
    kept.titleGlow = this->titleGlow;
    kept.tabGlow = this->tabGlow;
    kept.tabs = this->tabs;
    kept.handlers = {
        this->beforeFrame,
        this->frame,
        this->down,
        this->up,
        this->pageDown,
        this->pageUp,
        this->left,
        this->right,
        this->cancel,
        this->shift,
        this->end,
        this->select,
        this->draw,
    };

    kept.acts = {
        this->background,
        this->hitPointBar,
        this->unk4FB8D8,
        this->unk4FBA18,
        this->items[0],
        this->items[1],
        this->items[2],
        this->items[3],
        this->unk4FBEE0,
        this->face,
        this->unk4FC328,
        this->spiritBar,
        this->staminaBar,
        this->gauge,
        this->unk4FC6E8,
        this->unk4FC790,
        this->unk4FC840,
        this->unk4FD0A8,
    };

    kept.scrollBar = this->scrollBar;
    kept.townBar = this->townBar;
    kept.unk4FBAB4 = this->unk4FBAB4;
    kept.townTop = this->townTop;
    kept.townRow = this->townRow;
    kept.townShown = this->townShown;
    kept.descriptionSlide = this->descriptionSlide;
    kept.descriptionTop = this->descriptionTop;
    kept.unk4FC904 = this->unk4FC904;
    kept.description = this->description;
    kept.options = this->options;
    kept.itemsPage = this->itemsPage;

    const EquipPage& equip = this->equip;
    kept.equipInventoryHead = equip.inventoryHead;
    kept.equipOpenedAt = equip.openedAt;
    kept.equipSlide = equip.slide;
    kept.equipUnk4FCD30 = equip.unk4FCD30;
    kept.equipShown = equip.shown;
    kept.equipSlot = equip.slot;
    kept.equipRow = equip.row;
    kept.equipListX = equip.listX;
    kept.equipListY = equip.listY;
    kept.equipSavedBeforeFrame = equip.savedBeforeFrame;
    kept.equipWorn = equip.worn;
    kept.equipStage = equip.stage;
    kept.equipDescribed = equip.described;
    kept.equipUnk4FCD5C = equip.unk4FCD5C;
    kept.equipFirstShown = equip.firstShown;
    kept.equipScrollArrows = equip.scrollArrows;
    kept.equipTop = equip.top;
    kept.equipStats = equip.stats;
    kept.equipCount = equip.count;
    kept.equipPreviousStage = equip.previousStage;

    const SkillsPage& skills = this->skills;
    kept.skillsFirstShown = skills.firstShown;
    kept.skillsTop = skills.top;
    kept.skillsRoles = skills.roles;
    kept.skillsShown = skills.shown;
    kept.skillsButtonOffset = skills.buttonOffset;
    kept.skillsTab = skills.tab;
    kept.skillsTarget = skills.target;
    kept.skillsTabs = skills.tabs;
    kept.skillsDescriptionSlide = skills.descriptionSlide;
    kept.skillsCount = skills.count;
    kept.skillsRow = skills.row;
    kept.skillsShownTabs = skills.shownTabs;
    kept.skillsStage = skills.stage;
    kept.skillsDescriptionTop = skills.descriptionTop;
    kept.skillsScrollArrows = skills.scrollArrows;
    kept.skill = skills.skill;

    const Bestiary& book = this->bestiary;
    kept.bestiaryFirstShown = book.firstShown;
    kept.bestiaryTop = book.top;
    kept.bestiaryCategory = book.category;
    kept.bestiaryShown = book.shown;
    kept.bestiaryRow = book.row;
    kept.bestiaryScrollArrows = book.scrollArrows;

    kept.random = owner.random;
    kept.pointerAct = owner.scene.pointerRole.act;
    kept.panelAct = owner.battleMessages.panelAct;
    kept.shopTitle = owner.battleState.shopTitle;
    kept.lastPointerAction = owner.lastPointerAction;
    kept.menuState = owner.menuState;
    kept.menuMode = owner.menuMode;
    kept.storyState = owner.storyState;
    kept.inactive = owner.inactive;
    kept.systemFlags = owner.systemFlags;
    kept.saveScreenPage = owner.saveScreenPage;
    kept.slotsLoad = owner.slotsLoad;
    kept.faceSlides = owner.battleState.faceSlides;
    kept.shopScrollArrows = owner.battleState.scrollArrows;
    kept.actsRestarted = owner.acts.ScriptRestarted();
    kept.painter = owner.painter.SaveDrawState();
    kept.fonts = {
        owner.largeFont.SaveDrawState(),
        owner.mediumFont.SaveDrawState(),
        owner.smallFont.SaveDrawState(),
    };
}

void Menu::LoadFrame(const FrameState& kept)
{
    Game& owner = this->game;
    this->code = kept.code;
    this->page = kept.page;
    this->mode = kept.mode;
    this->unk4FC200 = kept.unk4FC200;
    this->unk4FBAB8 = kept.unk4FBAB8;
    this->unk4FBECC = kept.unk4FBECC;
    this->unk4FC82C = kept.unk4FC82C;
    this->unk4A6AA8 = kept.unk4A6AA8;
    this->tint = kept.tint;
    this->frames = kept.frames;
    this->flags = kept.flags;
    this->pointerX = kept.pointerX;
    this->pointerY = kept.pointerY;
    this->escapeDelay = kept.escapeDelay;
    this->clicks = kept.clicks;
    this->slide = kept.slide;
    this->lastPage = kept.lastPage;
    this->titleGlow = kept.titleGlow;
    this->tabGlow = kept.tabGlow;
    this->tabs = kept.tabs;
    this->beforeFrame = kept.handlers[0];
    this->frame = kept.handlers[1];
    this->down = kept.handlers[2];
    this->up = kept.handlers[3];
    this->pageDown = kept.handlers[4];
    this->pageUp = kept.handlers[5];
    this->left = kept.handlers[6];
    this->right = kept.handlers[7];
    this->cancel = kept.handlers[8];
    this->shift = kept.handlers[9];
    this->end = kept.handlers[10];
    this->select = kept.handlers[11];
    this->draw = kept.handlers[12];
    this->background = kept.acts[0];
    this->hitPointBar = kept.acts[1];
    this->unk4FB8D8 = kept.acts[2];
    this->unk4FBA18 = kept.acts[3];
    this->items[0] = kept.acts[4];
    this->items[1] = kept.acts[5];
    this->items[2] = kept.acts[6];
    this->items[3] = kept.acts[7];
    this->unk4FBEE0 = kept.acts[8];
    this->face = kept.acts[9];
    this->unk4FC328 = kept.acts[10];
    this->spiritBar = kept.acts[11];
    this->staminaBar = kept.acts[12];
    this->gauge = kept.acts[13];
    this->unk4FC6E8 = kept.acts[14];
    this->unk4FC790 = kept.acts[15];
    this->unk4FC840 = kept.acts[16];
    this->unk4FD0A8 = kept.acts[17];
    this->scrollBar = kept.scrollBar;
    this->townBar = kept.townBar;
    this->unk4FBAB4 = kept.unk4FBAB4;
    this->townTop = kept.townTop;
    this->townRow = kept.townRow;
    this->townShown = kept.townShown;
    this->descriptionSlide = kept.descriptionSlide;
    this->descriptionTop = kept.descriptionTop;
    this->unk4FC904 = kept.unk4FC904;
    this->description = kept.description;
    this->options = kept.options;
    this->itemsPage = kept.itemsPage;

    EquipPage& equip = this->equip;
    equip.inventoryHead = kept.equipInventoryHead;
    equip.openedAt = kept.equipOpenedAt;
    equip.slide = kept.equipSlide;
    equip.unk4FCD30 = kept.equipUnk4FCD30;
    equip.shown = kept.equipShown;
    equip.slot = kept.equipSlot;
    equip.row = kept.equipRow;
    equip.listX = kept.equipListX;
    equip.listY = kept.equipListY;
    equip.savedBeforeFrame = kept.equipSavedBeforeFrame;
    equip.worn = kept.equipWorn;
    equip.stage = kept.equipStage;
    equip.described = kept.equipDescribed;
    equip.unk4FCD5C = kept.equipUnk4FCD5C;
    equip.firstShown = kept.equipFirstShown;
    equip.scrollArrows = kept.equipScrollArrows;
    equip.top = kept.equipTop;
    equip.stats = kept.equipStats;
    equip.count = kept.equipCount;
    equip.previousStage = kept.equipPreviousStage;

    SkillsPage& skills = this->skills;
    skills.firstShown = kept.skillsFirstShown;
    skills.top = kept.skillsTop;
    skills.roles = kept.skillsRoles;
    skills.shown = kept.skillsShown;
    skills.buttonOffset = kept.skillsButtonOffset;
    skills.tab = kept.skillsTab;
    skills.target = kept.skillsTarget;
    skills.tabs = kept.skillsTabs;
    skills.descriptionSlide = kept.skillsDescriptionSlide;
    skills.count = kept.skillsCount;
    skills.row = kept.skillsRow;
    skills.shownTabs = kept.skillsShownTabs;
    skills.stage = kept.skillsStage;
    skills.descriptionTop = kept.skillsDescriptionTop;
    skills.scrollArrows = kept.skillsScrollArrows;
    skills.skill = kept.skill;

    Bestiary& book = this->bestiary;
    book.firstShown = kept.bestiaryFirstShown;
    book.top = kept.bestiaryTop;
    book.category = kept.bestiaryCategory;
    book.shown = kept.bestiaryShown;
    book.row = kept.bestiaryRow;
    book.scrollArrows = kept.bestiaryScrollArrows;

    owner.random = kept.random;
    owner.scene.pointerRole.act = kept.pointerAct;
    owner.battleMessages.panelAct = kept.panelAct;
    owner.battleState.shopTitle = kept.shopTitle;
    owner.lastPointerAction = kept.lastPointerAction;
    owner.menuState = kept.menuState;
    owner.menuMode = kept.menuMode;
    owner.storyState = kept.storyState;
    owner.inactive = kept.inactive;
    owner.systemFlags = kept.systemFlags;
    owner.saveScreenPage = kept.saveScreenPage;
    owner.slotsLoad = kept.slotsLoad;
    owner.battleState.faceSlides = kept.faceSlides;
    owner.battleState.scrollArrows = kept.shopScrollArrows;
    owner.acts.SetScriptRestarted(kept.actsRestarted);
    owner.painter.RestoreDrawState(kept.painter);
    owner.largeFont.RestoreDrawState(kept.fonts[0]);
    owner.mediumFont.RestoreDrawState(kept.fonts[1]);
    owner.smallFont.RestoreDrawState(kept.fonts[2]);
}

void Menu::Move(std::size_t which, i32 drawn, i32 from, i32 to)
{
    const Game& owner = this->game;

    if (!owner.smoothMotion || this->replaying)
    {
        return;
    }

    ValueMotion& motion = this->motions[which];
    const u32 start = owner.lastFrame;
    double begin = from;

    if (motion.frame + 1 == owner.gameFrames && motion.to == from)
    {
        begin = this->ValueAt(motion, start);
    }

    motion = ValueMotion{begin, to, drawn, start, std::max(owner.state.frameInterval, 1), owner.gameFrames};
}

i32 Menu::Shown(std::size_t which, i32 value) const
{
    return static_cast<i32>(std::round(this->ShownFine(which, value)));
}

double Menu::ShownFine(std::size_t which, i32 value) const
{
    const ValueMotion& motion = this->motions[which];

    if (!this->replaying || this->replayExact || motion.frame != this->game.gameFrames || motion.drawn != value)
    {
        return value;
    }

    return this->ValueAt(motion, this->replayNow);
}

double Menu::ValueAt(const ValueMotion& motion, u32 now) const
{
    const double progress =
        std::clamp(static_cast<double>(static_cast<i32>(now - motion.start)) / motion.length, 0.0, 1.0);
    return motion.from + (motion.to - motion.from) * progress;
}

// Whether a value moves on from the frame drawn: a step of this frame not yet taken.
bool Menu::Moving(u32 now) const
{
    constexpr double kStill = 1e-6;  // a value this close to the one drawn shows the frame drawn
    const u32 frame = this->game.gameFrames;

    return std::any_of(
        this->motions.begin(),
        this->motions.end(),
        [this, frame, now](const ValueMotion& motion)
        {
            return motion.frame == frame && std::abs(this->ValueAt(motion, now) - motion.drawn) > kStill;
        }
    );
}

void Menu::KeepFrame()
{
    Game& owner = this->game;
    this->frameKept = false;
    this->frameReplayable = owner.smoothMotion;

    if (!owner.smoothMotion)
    {
        return;
    }

    this->SaveFrame(*this->frameBefore);
    const Pixel* pixels = owner.painter.LockSurface(*owner.offscreen);
    owner.painter.UnlockSurface(*owner.offscreen);
    this->frameBase.assign(pixels, pixels + Display::Pixels);
}

void Menu::FrameDrawn()
{
    Game& owner = this->game;
    this->frameKept = this->frameReplayable && owner.menuMode == Menu::NoSlotsMode;
    this->frameChecked = false;

    if (this->frameKept)
    {
        this->frameKeptAt = owner.gameFrames;
        this->frameShown.assign(owner.drawSurface, owner.drawSurface + Display::Pixels);
        ++this->framesKept;
    }
}

void Menu::KeepFrameFrom(bool replayable)
{
    this->frameReplayable = this->frameReplayable && replayable;
}

bool Menu::Replaying() const
{
    return this->replaying;
}

// The frame's drawing run again on `target` from the state it started from, its values shown at `now` (as drawn when
// `exact`) and the pointer at (pointerX, pointerY); the state it left is put back.
void Menu::Replay(Pixel* target, u32 now, bool exact, i32 pointerX, i32 pointerY)
{
    Game& owner = this->game;
    this->SaveFrame(*this->frameAfter);
    const i32 shownX = owner.state.pointerX;
    const i32 shownY = owner.state.pointerY;
    Pixel* const drawSurface = owner.drawSurface;
    this->LoadFrame(*this->frameBefore);
    owner.state.pointerX = pointerX;
    owner.state.pointerY = pointerY;
    this->replaying = true;
    this->replayExact = exact;
    this->replayNow = now;
    owner.audio.SetMuted(true);
    std::copy(this->frameBase.begin(), this->frameBase.end(), target);
    owner.drawSurface = target;
    owner.painter.SetTarget(target);

    if ((owner.menuState & Game::MenuNumberMask) == Game::ItemChoiceMenu)
    {
        owner.DrawShop();
    }
    else
    {
        this->PaintPage(true);
    }

    owner.audio.SetMuted(false);
    this->replaying = false;
    this->replayExact = false;
    this->LoadFrame(*this->frameAfter);
    owner.state.pointerX = shownX;
    owner.state.pointerY = shownY;
    owner.drawSurface = drawSurface;
}

bool Menu::PaintBetween(Pixel* target)
{
    Game& owner = this->game;
    const u32 now = SystemUptime();

    if (!this->frameKept || this->frameKeptAt != owner.gameFrames)
    {
        return false;
    }

    // Once a frame: the drawing run again twice as the frame drew it must give the frame drawn, or the frame stays.
    if (!this->frameChecked)
    {
        this->frameChecked = true;

        for (int pass = 0; pass < 2 && this->frameKept; ++pass)
        {
            this->Replay(target, now, true, owner.state.pointerX, owner.state.pointerY);

            if (!std::equal(this->frameShown.begin(), this->frameShown.end(), target))
            {
                this->frameKept = false;
                ++this->framesMismatched;
                return false;
            }
        }
    }

    // The pointer is drawn where the mouse is now (the frames read it where it was).
    int mouseX = 0;
    int mouseY = 0;
    owner.input.PeekMouse(mouseX, mouseY);

    if (!this->Moving(now) && mouseX == owner.state.pointerX && mouseY == owner.state.pointerY)
    {
        return false;
    }

    this->Replay(target, now, false, mouseX, mouseY);
    ++this->framesPainted;
    return true;
}

void Menu::LogTotals() const
{
    if (this->framesKept == 0)
    {
        return;
    }

    LogInfo(
        u8"smooth menu: %d frames kept, %d not drawn again as drawn, %d painted between",
        static_cast<i32>(this->framesKept),
        static_cast<i32>(this->framesMismatched),
        static_cast<i32>(this->framesPainted)
    );
}

}  // namespace swd3
