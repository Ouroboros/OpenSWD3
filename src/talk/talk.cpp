#include "talk/talk.h"

#include <climits>
#include <span>
#include <string>

#include "battle/smooth/smooth_talk.h"
#include "core/debug_log.h"
#include "game/direction.h"
#include "game/game.h"
#include "talk/script_text.h"
#include "talk/talk_constants.h"
#include "talk/talk_event.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

constexpr int kDecimalBase = 0xA;
constexpr int kMaxDigits = 9;  // ParseNumber takes at most this many digits

// The talk windows open in kOpenSteps frames, or fade in (kOpenStep a frame) and out (kFadeStep a frame).

// The name plate over a window: its frame, the shade under it and its text.

constexpr u32 kNoIndex = 0xFFFFFFFF;  // no role under the pointer
constexpr i32 kIdleAction = 0x34;     // the controlled role stands idle (like action 0)

}  // namespace

Talk::Talk(Game& game)
    : game(game)
{
}

// swd32.exe 0x00430350: text in the large font with the colour and the shadow colour of the tables
void Talk::DrawColoredText(u16* pixels, int x, int y, int color, int shade, std::string_view text, int style)
{
    TextFont& font = this->game.largeFont;
    const TextColors& colors = this->game.textColors;
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(static_cast<Pixel>(colors.secondary[static_cast<std::size_t>(shade) & (TextColors::Colors - 1)]));
    font.DrawText(
        pixels,
        x,
        y,
        text,
        static_cast<Pixel>(colors.primary[static_cast<std::size_t>(color) & (TextColors::Colors - 1)]),
        style
    );
    font.SetBackground(TextFont::NoBackground);
    font.SetShadow(0);
}

// swd32.exe 0x00430BE0: an optionally signed decimal of at most 9 digits; false for anything else. The original reads
// backwards from the text without bound for an empty number; here that gives 0.
bool Talk::ParseNumber(std::string_view text, int& value)
{
    std::size_t at = 0;
    int sign = 1;

    if (!text.empty() && (text[0] == '+' || text[0] == '-'))
    {
        sign = text[0] == '-' ? -1 : 1;
        at = 1;
    }

    int digits = 0;

    for (std::size_t index = at; index < text.size() && text[index] != '\0'; ++index)
    {
        if (static_cast<u8>(text[index] - '0') >= kDecimalBase)
        {
            return false;
        }

        ++digits;
    }

    if (digits > kMaxDigits)
    {
        return false;
    }

    value = 0;
    int scale = sign;

    for (int index = digits - 1; index >= 0; --index)
    {
        value += scale * (text[at + static_cast<std::size_t>(index)] - '0');
        scale *= kDecimalBase;
    }

    return true;
}

// swd32.exe 0x0042ED40: every talk window drawn and advanced through a 40 x 11 cell surface; closed windows are freed
// (their speakers released), then the end and next page marks step
void Talk::Draw()
{
    smooth::Scene scene;
    smooth::TalkFrame(*this).DrawAdvance(scene);
    scene.Show(this->game.battleServices);
}

// swd32.exe 0x00427300: the pointer over the map. With no event running and the controlled role standing, a visible
// role with an event under the pointer shows pointer 9 (0xA for flag 0x40, which then blocks map events); a left click
// on it turns both to face each other and starts its event. Otherwise a clickable region of the map under the pointer
// shows pointer 0xB and a click starts its event; elsewhere the pointer shows the direction from the controlled role,
// the right button walks that way and a left click stands for the space key while a talk window is open. With choices
// on screen the pointer picks the choice under it and a click ends them.
void Talk::HandlePointer()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    Controls& controls = owner.controls;
    ButtonState& left = controls.At(Button::MouseLeft);
    i32& cursor = owner.scene.pointerRole.act.action;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    // The pointer counts from the game's 640x480 screen (the map filling the window lies around it).
    const int cameraLeft = field.ScreenLeft();
    const int cameraTop = field.ScreenTop();
    u32 found = kNoIndex;
    bool regionsAllowed = true;

    if (state.leader.id == Role::NoId && (player.act.action == 0 || player.act.action == kIdleAction))
    {
        for (found = 1; found < static_cast<u32>(state.roleCount); ++found)
        {
            const Role& role = state.roles[found];

            if ((role.flags & Role::Visible) == 0 || role.event == 0 || role.talkState != Role::NoTalk)
            {
                continue;
            }

            const Picture* picture = owner.archive.FindPicture(role.act.frame, role.act.picture);
            const int x = role.x - role.act.x - cameraLeft;
            const int y = role.y - role.act.y - cameraTop;

            // The rebuild compares signed (the original's unsigned places miss a role across the screen's left or
            // top edge, which the map filling the window shows whole).
            if (picture != nullptr && state.pointerX > x && state.pointerX < x + picture->width && state.pointerY > y &&
                state.pointerY < y + picture->height)
            {
                break;
            }
        }

        if (found < static_cast<u32>(state.roleCount) && (state.roles[found].flags & Role::NoTalkPointer) == 0)
        {
            cursor = Scene::TalkPointer;

            if ((state.roles[found].flags & Role::Examine) != 0)
            {
                cursor = Scene::ExaminePointer;
                regionsAllowed = false;
            }
        }
    }

    auto faceAway = [](int direction)
    {
        return (direction & ~1) + ((direction - 1) & 1);  // the opposite direction of its pair
    };

    auto standing = [&player]()
    {
        return player.act.action == kIdleAction || player.act.action == 0;
    };

    if (!state.GameFlag(GameState::NoChoicePointer) && !state.clickRegions.regions.empty() && standing())
    {
        ClickRegion* region = nullptr;
        const int index = state.clickRegions.Find(state.pointerX, state.pointerY, region);
        cursor = Scene::ChoicePointer;

        if (region != nullptr)
        {
            this->choice = index;

            if (left.press != 0 && left.heldFrames == 1)
            {
                state.clickRegions.Free();
                left = ButtonState{};
                DebugLog(
                    u8"\n"
                    u8"Button[%d]Pressed",
                    this->choice
                );
            }
        }

        return;
    }

    if (found == kNoIndex || found == static_cast<u32>(state.roleCount) ||
        found == static_cast<u32>(state.playerRole) || state.leader.id != Role::NoId ||
        state.roles[found].talkState != Role::NoTalk || left.press == 0 || left.heldFrames != 1)
    {
        const std::vector<u32>& blocks = state.map.blocks;
        const auto cellX = static_cast<u32>(cameraLeft + state.pointerX) >> MapHeader::TileShift;
        const auto cellY = static_cast<u32>(cameraTop + state.pointerY) >> MapHeader::TileShift;
        const std::size_t cell = cellX + static_cast<std::size_t>(state.map.width) * cellY;
        const u8 regionId = cell < blocks.size() ? static_cast<u8>(blocks[cell] & MapHeader::RegionMask) : u8{0};
        MapRegion* region = nullptr;

        if (state.leader.id == Role::NoId && regionsAllowed && standing() && regionId != 0 &&
            (region = owner.field.FindRegion(regionId)) != nullptr &&
            state.GameFlag(region->flag & MapRegion::ClickFlagMask))
        {
            cursor = Scene::RegionPointer;

            if (left.press == 0 || left.heldFrames != 1)
            {
                return;
            }

            Role& leader = state.leader;
            leader.unk14 = 0;
            leader.unk20 = 0;
            leader.flags = 0;
            leader.event = static_cast<u16>(region->event & MapRegion::EventMask);
            leader.x = cameraLeft + (state.pointerX & ~MapHeader::TileFraction);
            leader.id = TalkWindow::NoRole;
            leader.y = cameraTop + (state.pointerY & ~MapHeader::TileFraction);
            const int direction = state.DirectionToPlayer(cameraLeft + state.pointerX, cameraTop + state.pointerY);
            player.act.savedDirection = player.act.direction;
            player.act.savedAction = player.act.action;
            player.act.action = 0;
            player.act.direction = faceAway(direction);
            player.act.delay = 0;

            if (owner.acts.Run(player.act) == 0)
            {
                LogActError(u8"Act Err(Talk:MouseActiveStory)", player);
            }

            owner.scene.playerHold = Scene::HoldLocked;
            DebugLog(
                u8"\n"
                u8"MapEvent.Talk[%d]",
                leader.event
            );
            return;
        }

        i32 distance = 0;
        const int angle = AngleBetween(
            state.pointerX,
            state.pointerY,
            player.x + MapHeader::TileSize / 2 * player.act.width - cameraLeft,
            player.y + MapHeader::TileSize / 2 * player.act.height - cameraTop,
            distance
        );
        const int direction = AngleToDirection(angle);

        if (cursor == Scene::ChoicePointer)
        {
            cursor = direction;
        }

        const ButtonState& right = controls.At(Button::MouseRight);

        if (((player.x | player.y) & MapHeader::TileFraction) == 0 && owner.scene.playerHold == 0 && right.press != 0 &&
            distance >= MapHeader::TileSize)
        {
            if (direction == ActState::Up || direction == ActState::UpLeft || direction == ActState::UpRight)
            {
                controls.At(Button::Up).press = right.press;
            }

            if (direction == ActState::Down || direction == ActState::DownRight || direction == ActState::DownLeft)
            {
                controls.At(Button::Down).press = right.press;
            }

            if (direction == ActState::Left || direction == ActState::UpLeft || direction == ActState::DownLeft)
            {
                controls.At(Button::Left).press = right.press;
            }

            if (direction == ActState::Right || direction == ActState::DownRight || direction == ActState::UpRight)
            {
                controls.At(Button::Right).press = right.press;
            }
        }

        ButtonState& space = controls.At(Button::Space);

        if (owner.scene.talkWindows != nullptr && space.press == 0)
        {
            space = left;
        }

        return;
    }

    Role& target = state.roles[found];
    const int direction = state.DirectionToPlayer(
        target.x + MapHeader::TileSize / 2 * target.act.width,
        target.y + MapHeader::TileSize / 2 * target.act.height
    );

    if ((target.flags & Role::FacesTalker) != 0)
    {
        target.act.savedAction = target.act.action;
        target.act.savedDirection = target.act.direction;
        target.act.action = 0;
        target.act.delay = 0;
        target.act.direction = direction;

        if (owner.acts.Run(target.act) == 0)
        {
            LogActError(u8"Act Err(Talk:RoleTurn)", target);
        }
    }

    player.act.action = 0;
    player.act.direction = faceAway(direction);
    player.act.delay = 0;

    if (owner.acts.Run(player.act) == 0)
    {
        LogActError(u8"Act Err(Talk:MRoleTFace)", player);
    }

    Role& leader = state.leader;
    leader.unk14 = target.unk14;
    leader.unk20 = target.unk20;
    leader.event = target.event;
    leader.id = target.id;
    leader.flags = target.flags;
    leader.x = target.x;
    leader.y = target.y;
    left = ButtonState{};
    DebugLog(
        u8"\n"
        u8"RoleEvent.GUID[%d].Talk[%d].Ridx[%d].RArgu[%x]",
        target.id,
        target.event,
        state.FindRole(target.id),
        target.flags
    );
}

// The mark of a finished text: the act of the button pointer role (0x004C9708 inside 0x004C96C8).
ActState& Talk::EndAct()
{
    return this->game.scene.pointerButtonRole.act;
}

// The mark of a full page: the act of the page pointer role (0x004CADE0 inside 0x004CADA0).
ActState& Talk::NextAct()
{
    return this->game.scene.pointerPageRole.act;
}

}  // namespace swd3
