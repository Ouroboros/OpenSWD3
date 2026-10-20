#include "map/field_view_internal.h"

namespace swd3
{

namespace
{

// A gap longer than this many milliseconds (a pause, a slow frame) is skipped rather than run.
constexpr u32 kMostTicks = 0x100;

i32 Sign(i32 value)
{
    return (value > 0) - (value < 0);
}

}  // namespace

// The end of the map's frame (0x004120B0) for the controlled role on a tile: its cells and trail follow its step and
// the step ends.
void FieldView::SettlePlayer()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Field& field = owner.field;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const i32 leaderId = owner.partyLeaderId;
    const int width = state.map.width;
    field.RemoveRole(
        player.id,
        static_cast<int>(player.flags & Role::LayerMask),
        (static_cast<u32>(player.y) >> MapHeader::TileShift) - 1,
        0
    );
    field.ClearRoleCells(player, leaderId);

    if (player.cell != nullptr)
    {
        const std::ptrdiff_t at = player.cell - state.map.blocks.data() + owner.scrollStepX + owner.walkStepX +
                                  static_cast<std::ptrdiff_t>(width) * (owner.scrollStepY + owner.walkStepY);
        player.cell = at >= 0 && static_cast<std::size_t>(at) < state.map.blocks.size()
                          ? &state.map.blocks[static_cast<std::size_t>(at)]
                          : nullptr;
    }

    field.MarkRoleCells(player, leaderId);
    owner.scrollStepY = 0;
    owner.walkStepY = 0;
    owner.scrollStepX = 0;
    owner.walkStepX = 0;

    if (view.trailX[0] != player.x || view.trailY[0] != player.y)
    {
        std::copy_backward(view.trailX.begin(), view.trailX.end() - 1, view.trailX.end());
        std::copy_backward(view.trailY.begin(), view.trailY.end() - 1, view.trailY.end());
        std::copy_backward(view.trailDirection.begin(), view.trailDirection.end() - 1, view.trailDirection.end());
        view.trailX[0] = player.x;
        view.trailY[0] = player.y;
        view.trailDirection[0] = player.act.direction;
    }

    this->UpdateRoleCellFlags(player);
}

// The milliseconds of the map up to `now`, run while the map's frames run (no menu, battle, story page, video or
// pause): a gap after a pause starts the clock again.
void FieldView::AdvanceMotion(u32 now)
{
    Game& owner = this->game;

    if (!owner.smoothMotion)
    {
        // Smooth motion turned off: every motion under way ends where the game frames would have it.
        if (this->playerMotion.active)
        {
            this->EndPlayerMotion(true);
        }

        for (std::size_t index = 0; index < this->roleMotions.size(); ++index)
        {
            this->FinishRoleStep(index);
        }

        if (this->panMotion.active)
        {
            this->panMotion.elapsed = std::max(static_cast<i32>(owner.state.frameInterval), 1);
            this->PanTick();
        }

        this->motionClockSet = false;
        return;
    }

    // Another map drops the motions of the map left.
    if (owner.currentMap != this->motionMap)
    {
        this->motionMap = owner.currentMap;
        this->playerMotion.active = false;
        this->roleMotions.clear();
        this->panMotion.active = false;
    }

    const bool moving = owner.running != 0 && owner.inactive == 0 && owner.active != 0 &&
                        (owner.systemFlags & (Game::VideoPlaying | Game::UnkSystem1 | Game::QuitRequested)) == 0 &&
                        owner.storyState == Game::StoryRunning && owner.inBattle == 0 &&
                        owner.menuState == Game::NoMenu && owner.state.frameInterval > 0;

    if (!moving || !this->motionClockSet || now - this->motionClock > kMostTicks)
    {
        this->motionClock = now;
        this->motionClockSet = moving;
        return;
    }

    while (static_cast<i32>(now - this->motionClock) > 0)
    {
        ++this->motionClock;
        this->MotionTick();

        for (std::size_t index = 0; index < this->roleMotions.size(); ++index)
        {
            this->RoleTick(index);
        }

        this->PanTick();
    }
}

// A millisecond of the controlled role: a step ControlPlayer set in a frame moves it (and the camera when it follows)
// by its speed, a game frame's worth every frame interval; the step starts at the frame, so it reaches the next tile
// at a later frame's time, where the role settles as the end of the original's frame does (SettlePlayer) and that
// frame's ControlPlayer decides the next step.
void FieldView::MotionTick()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Field& field = owner.field;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    PlayerMotion& motion = this->playerMotion;
    const bool stepping =
        owner.scrollStepX != 0 || owner.scrollStepY != 0 || owner.walkStepX != 0 || owner.walkStepY != 0;

    if (motion.active)
    {
        const bool moved = player.x != motion.leftX || player.y != motion.leftY ||
                           field.cameraLeft != motion.leftCameraX || field.cameraTop != motion.leftCameraY;
        const bool same = motion.across == Sign(owner.scrollStepX + owner.walkStepX) &&
                          motion.down == Sign(owner.scrollStepY + owner.walkStepY) &&
                          motion.scrollAcross == owner.scrollStepX && motion.scrollDown == owner.scrollStepY;

        if (moved)
        {
            motion.active = false;
        }
        else if (!same)
        {
            this->EndPlayerMotion(true);
        }
    }

    if (!motion.active)
    {
        if (!stepping)
        {
            return;
        }

        this->BeginPlayerMotion();

        if (!motion.active)
        {
            return;
        }
    }

    ++motion.elapsed;
    const auto interval = static_cast<i32>(state.frameInterval);
    const i32 pixels = motion.speed * motion.elapsed / interval;
    const i32 cameraX = motion.cameraX + motion.scrollAcross * pixels;
    const i32 cameraY = motion.cameraY + motion.scrollDown * pixels;
    field.cameraRight += cameraX - field.cameraLeft;
    field.cameraBottom += cameraY - field.cameraTop;
    field.cameraLeft = cameraX;
    field.cameraTop = cameraY;
    player.x = motion.startX + motion.across * pixels;
    player.y = motion.startY + motion.down * pixels;
    motion.leftX = player.x;
    motion.leftY = player.y;
    motion.leftCameraX = field.cameraLeft;
    motion.leftCameraY = field.cameraTop;

    if (((player.x | player.y) & MapHeader::TileFraction) != 0 ||
        (player.x == motion.startX && player.y == motion.startY))
    {
        return;
    }

    motion.active = false;
    this->SettlePlayer();
}

// A step set by ControlPlayer starts from where the role and the camera are.
void FieldView::BeginPlayerMotion()
{
    Game& owner = this->game;
    const Field& field = owner.field;
    const Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
    PlayerMotion& motion = this->playerMotion;
    motion.across = Sign(owner.scrollStepX + owner.walkStepX);
    motion.down = Sign(owner.scrollStepY + owner.walkStepY);
    motion.scrollAcross = owner.scrollStepX;
    motion.scrollDown = owner.scrollStepY;
    motion.active = (motion.across != 0 || motion.down != 0) && owner.state.walkSpeed > 0;
    motion.startX = player.x;
    motion.startY = player.y;
    motion.cameraX = field.cameraLeft;
    motion.cameraY = field.cameraTop;
    motion.speed = owner.state.walkSpeed;
    motion.elapsed = 0;
    motion.leftX = player.x;
    motion.leftY = player.y;
    motion.leftCameraX = field.cameraLeft;
    motion.leftCameraY = field.cameraTop;
}

// A step left before its tile (its keys cleared, smooth motion turned off) ends where the game frames would have moved
// it (`atFrame`: a whole number of frames of its speed), so that they go on from a place of theirs.
void FieldView::EndPlayerMotion(bool atFrame)
{
    Game& owner = this->game;
    Field& field = owner.field;
    Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
    PlayerMotion& motion = this->playerMotion;
    motion.active = false;

    if (!atFrame || owner.state.frameInterval <= 0)
    {
        return;
    }

    const i32 pixels = motion.speed * (motion.elapsed / static_cast<i32>(owner.state.frameInterval));
    const i32 cameraX = motion.cameraX + motion.scrollAcross * pixels;
    const i32 cameraY = motion.cameraY + motion.scrollDown * pixels;
    field.cameraRight += cameraX - field.cameraLeft;
    field.cameraBottom += cameraY - field.cameraTop;
    field.cameraLeft = cameraX;
    field.cameraTop = cameraY;
    player.x = motion.startX + motion.across * pixels;
    player.y = motion.startY + motion.down * pixels;
}

// The distance of the step under way in 1/scale pixels.
i32 FieldView::MotionPixels(int scale) const
{
    const PlayerMotion& motion = this->playerMotion;
    const auto interval = static_cast<i32>(this->game.state.frameInterval);

    if (!motion.active || interval <= 0)
    {
        return 0;
    }

    return motion.speed * motion.elapsed * scale / interval;
}

// The camera where the motion under way has it: following the step of the keys, centred on the controlled role an
// event walks (as CenterCamera keeps it), or panning.
std::array<i32, 2> FieldView::CameraFine(int scale) const
{
    const Game& owner = this->game;
    const Field& field = owner.field;
    const PlayerMotion& motion = this->playerMotion;

    if (motion.active)
    {
        const i32 pixels = this->MotionPixels(scale);
        return {
            motion.cameraX * scale + motion.scrollAcross * pixels,
            motion.cameraY * scale + motion.scrollDown * pixels,
        };
    }

    const auto playerIndex = static_cast<std::size_t>(owner.state.playerRole);
    const Role& player = owner.state.roles[playerIndex];

    if (playerIndex < this->roleMotions.size() && this->roleMotions[playerIndex].active &&
        (owner.scene.screenFlags & Scene::CameraHeld) == 0 &&
        field.CameraCentredOn(player.x, player.y) == std::array<i32, 2>{field.cameraLeft, field.cameraTop})
    {
        const std::array<i32, 2> place = this->RoleFine(playerIndex, scale);
        return field.CameraCentredOn(place[0], place[1], scale);
    }

    const PanMotion& pan = this->panMotion;
    const auto interval = static_cast<i32>(owner.state.frameInterval);

    if (pan.active && interval > 0)
    {
        return {
            pan.fromX * scale + pan.byX * pan.elapsed * scale / interval,
            pan.fromY * scale + pan.byY * pan.elapsed * scale / interval,
        };
    }

    return {field.cameraLeft * scale, field.cameraTop * scale};
}

// A role where its step under way has it (the controlled role where the step of the keys has it).
std::array<i32, 2> FieldView::RoleFine(std::size_t index, int scale) const
{
    const Game& owner = this->game;
    const Role& role = owner.state.roles[index];

    if (index == static_cast<std::size_t>(owner.state.playerRole) && this->playerMotion.active)
    {
        return this->PlayerFine(scale);
    }

    if (index < this->roleMotions.size() && this->roleMotions[index].active)
    {
        const RoleMotion& step = this->roleMotions[index];
        return {
            step.fromX * scale + step.byX * step.elapsed * scale / step.length,
            step.fromY * scale + step.byY * step.elapsed * scale / step.length,
        };
    }

    return {role.x * scale, role.y * scale};
}

// A step a frame takes for a role on a move or a follower: one still under way is finished first; it moves from the
// frame on, over a frame until TimeRoleStep reads its act.
void FieldView::StartRoleStep(std::size_t index, i32 byX, i32 byY, std::size_t move, u8 direction, bool follower)
{
    const Game& owner = this->game;
    const Role& role = owner.state.roles[index];

    if (this->roleMotions.size() <= index)
    {
        this->roleMotions.resize(GameState::Roles);
    }

    this->FinishRoleStep(index);
    RoleMotion& step = this->roleMotions[index];
    step = RoleMotion{};
    step.active = byX != 0 || byY != 0;
    step.follower = follower;
    step.fromX = role.x;
    step.fromY = role.y;
    step.byX = byX;
    step.byY = byY;
    step.length = std::max(static_cast<i32>(owner.state.frameInterval), 1);
    step.leftX = role.x;
    step.leftY = role.y;
    step.move = move;
    step.direction = direction;
}

// The act run after the step sets the frames to the role's next step (its delay plus one): the step takes them, at
// most RoleStepMostFrames (a longer hold is the role stopping: the step takes a frame), so that it ends as the next
// one starts.
void FieldView::TimeRoleStep(std::size_t index)
{
    if (index >= this->roleMotions.size() || !this->roleMotions[index].active || this->roleMotions[index].timed)
    {
        return;
    }

    const Game& owner = this->game;
    RoleMotion& step = this->roleMotions[index];
    const auto next = static_cast<i32>(owner.state.roles[index].act.delay) + 1;
    const i32 frames = next > RoleStepMostFrames ? 1 : next;
    step.length = frames * std::max(static_cast<i32>(owner.state.frameInterval), 1);
    step.timed = true;
}

// The step under way ends at once where it goes (a new step before its end, smooth motion turned off); whether the
// role arrived on a tile.
bool FieldView::FinishRoleStep(std::size_t index)
{
    if (index >= this->roleMotions.size() || !this->roleMotions[index].active)
    {
        return false;
    }

    this->roleMotions[index].elapsed = this->roleMotions[index].length;
    this->RoleTick(index);
    const Role& role = this->game.state.roles[index];
    return ((role.x | role.y) & MapHeader::TileFraction) == 0;
}

// A millisecond of a role's step: the role moves by its share of the step; at the end, on a tile, it arrives as the
// original's frame does when the step lands there. The controlled role an event walks keeps the camera centred on
// it. A role moved by anything else ends its step where it is.
void FieldView::RoleTick(std::size_t index)
{
    if (index >= this->roleMotions.size() || !this->roleMotions[index].active)
    {
        return;
    }

    Game& owner = this->game;
    Role& role = owner.state.roles[index];
    RoleMotion& step = this->roleMotions[index];

    if (role.x != step.leftX || role.y != step.leftY)
    {
        step.active = false;
        return;
    }

    step.elapsed = std::min(step.elapsed + 1, step.length);
    role.x = step.fromX + step.byX * step.elapsed / step.length;
    role.y = step.fromY + step.byY * step.elapsed / step.length;
    step.leftX = role.x;
    step.leftY = role.y;
    const bool controlled = index == static_cast<std::size_t>(owner.state.playerRole);

    if (controlled && (owner.scene.screenFlags & Scene::CameraHeld) == 0)
    {
        owner.field.CenterCamera();
    }

    if (step.elapsed < step.length)
    {
        return;
    }

    step.active = false;

    if (((role.x | role.y) & MapHeader::TileFraction) != 0)
    {
        return;
    }

    if (step.follower)
    {
        this->ArriveFollower(index, step.move, step.direction);
    }
    else
    {
        this->ArriveMoving(index, step.move, step.direction);
    }
}

// The pan of a frame (0x00414570) moves the camera by its step over the frame.
void FieldView::StartPan(i32 byX, i32 byY)
{
    const Field& field = this->game.field;

    if (this->panMotion.active)
    {
        this->panMotion.elapsed = std::max(static_cast<i32>(this->game.state.frameInterval), 1);
        this->PanTick();
    }

    this->panMotion = PanMotion{
        byX != 0 || byY != 0,
        field.cameraLeft,
        field.cameraTop,
        byX,
        byY,
        0,
        field.cameraLeft,
        field.cameraTop
    };
}

// A millisecond of the pan: the camera moves by its share of the step; moved by anything else, the pan ends there.
void FieldView::PanTick()
{
    PanMotion& pan = this->panMotion;

    if (!pan.active)
    {
        return;
    }

    Field& field = this->game.field;

    if (field.cameraLeft != pan.leftX || field.cameraTop != pan.leftY)
    {
        pan.active = false;
        return;
    }

    const auto interval = std::max(static_cast<i32>(this->game.state.frameInterval), 1);
    pan.elapsed = std::min(pan.elapsed + 1, interval);
    const i32 cameraX = pan.fromX + pan.byX * pan.elapsed / interval;
    const i32 cameraY = pan.fromY + pan.byY * pan.elapsed / interval;
    field.cameraRight += cameraX - field.cameraLeft;
    field.cameraBottom += cameraY - field.cameraTop;
    field.cameraLeft = cameraX;
    field.cameraTop = cameraY;
    pan.leftX = cameraX;
    pan.leftY = cameraY;
    pan.active = pan.elapsed < interval;
}

std::array<i32, 2> FieldView::PlayerFine(int scale) const
{
    const Role& player = this->game.state.roles[static_cast<std::size_t>(this->game.state.playerRole)];
    const PlayerMotion& motion = this->playerMotion;

    if (!motion.active)
    {
        return {player.x * scale, player.y * scale};
    }

    const i32 pixels = this->MotionPixels(scale);
    return {motion.startX * scale + motion.across * pixels, motion.startY * scale + motion.down * pixels};
}

}  // namespace swd3
