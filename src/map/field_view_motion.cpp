#include "map/field_view_internal.h"

namespace swd3
{

namespace
{

// A gap longer than this many milliseconds (a pause, a slow frame) is skipped rather than run.
constexpr u32 kMostTicks = 0x100;
constexpr int kPinScale = 0x100;  // the fineness of a pinned glide's place

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
    const i32 leaderId = owner.unk4B7BC4;
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
        const std::ptrdiff_t at = player.cell - state.map.blocks.data() + owner.unk4B7518 + owner.unk4A9488 +
                                  static_cast<std::ptrdiff_t>(width) * (owner.unk4B7920 + owner.unk4A948C);
        player.cell = at >= 0 && static_cast<std::size_t>(at) < state.map.blocks.size()
                          ? &state.map.blocks[static_cast<std::size_t>(at)]
                          : nullptr;
    }

    field.MarkRoleCells(player, leaderId);
    owner.unk4B7920 = 0;
    owner.unk4A948C = 0;
    owner.unk4B7518 = 0;
    owner.unk4A9488 = 0;

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
        if (this->playerMotion.active)
        {
            this->EndPlayerMotion(true);
        }

        this->motionClockSet = false;
        return;
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
    const bool stepping = owner.unk4B7518 != 0 || owner.unk4B7920 != 0 || owner.unk4A9488 != 0 || owner.unk4A948C != 0;

    if (motion.active)
    {
        const bool moved = player.x != motion.leftX || player.y != motion.leftY ||
                           field.cameraLeft != motion.leftCameraX || field.cameraTop != motion.leftCameraY;
        const bool same = motion.across == Sign(owner.unk4B7518 + owner.unk4A9488) &&
                          motion.down == Sign(owner.unk4B7920 + owner.unk4A948C) &&
                          motion.scrollAcross == owner.unk4B7518 && motion.scrollDown == owner.unk4B7920;

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
    this->PinGlides();
    this->SettlePlayer();
}

// A step set by ControlPlayer starts from where the role and the camera are.
void FieldView::BeginPlayerMotion()
{
    Game& owner = this->game;
    const Field& field = owner.field;
    const Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
    PlayerMotion& motion = this->playerMotion;
    motion.across = Sign(owner.unk4B7518 + owner.unk4A9488);
    motion.down = Sign(owner.unk4B7920 + owner.unk4A948C);
    motion.scrollAcross = owner.unk4B7518;
    motion.scrollDown = owner.unk4B7920;
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

// The glides of the camera and the controlled role held where the continuous motion has them, so that whatever glides
// them next (a pan, an event's move) starts there.
void FieldView::PinGlides()
{
    if (this->glides.size() != 1 + GameState::Roles)
    {
        return;
    }

    const Field& field = this->game.field;
    const Role& player = this->game.state.roles[static_cast<std::size_t>(this->game.state.playerRole)];
    const u32 frame = this->placesFrame;

    const auto pin = [frame](Glide& glide, std::array<i32, 2> fine, i32 x, i32 y)
    {
        const double fromX = static_cast<double>(fine[0]) / kPinScale;
        const double fromY = static_cast<double>(fine[1]) / kPinScale;
        glide = Glide{fromX, fromY, x, y, static_cast<double>(frame), 1.0, frame};
    };

    pin(this->glides[0], this->CameraFine(kPinScale), field.cameraLeft, field.cameraTop);
    pin(this->glides[1 + static_cast<std::size_t>(this->game.state.playerRole)],
        this->PlayerFine(kPinScale),
        player.x,
        player.y);
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

std::array<i32, 2> FieldView::CameraFine(int scale) const
{
    const Field& field = this->game.field;
    const PlayerMotion& motion = this->playerMotion;

    if (!motion.active)
    {
        return {field.cameraLeft * scale, field.cameraTop * scale};
    }

    const i32 pixels = this->MotionPixels(scale);
    return {motion.cameraX * scale + motion.scrollAcross * pixels, motion.cameraY * scale + motion.scrollDown * pixels};
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
