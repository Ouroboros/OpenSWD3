#include "map/field_view_internal.h"

namespace swd3
{

// Where the game frame about to be painted draws the camera and the roles: each starts a glide to its new place when
// it moved (from where it is shown at the start of this frame), a role other than the controlled one over the frames
// to its next step (its act's delay plus one), the camera and the controlled role over the frames since their last
// move. At the end of the frame (AimNextFrame) whatever the game's rules move heads for its next place instead; the
// controlled role moved by the keys (no move record of its own, no quake) and its camera are left to it.
void FieldView::CapturePlaces()
{
    Game& owner = this->game;

    if (!owner.smoothMotion)
    {
        return;
    }

    const Field& field = owner.field;
    const GameState& state = owner.state;
    const Scene& view = owner.scene;
    const u32 frame = owner.gameFrames;
    const bool continuous = this->placesValid && frame == this->placesFrame + 1 && owner.currentMap == this->placesMap;
    const auto playerIndex = static_cast<std::size_t>(state.playerRole);
    const bool quaking = view.shake[0] != Scene::ShakeEnd && owner.currentMap != Field::DemonRefiningPot;
    const bool led = std::ranges::any_of(
        state.moves,
        [playerIndex](const MoveRecord& move)
        {
            return static_cast<std::size_t>(move.role) == playerIndex && move.step < MoveRecord::StepMask;
        }
    );
    this->keyed = !quaking && !led;
    this->placesContinuous = continuous;
    this->glides.resize(1 + GameState::Roles);

    this->rolesAimed.assign(GameState::Roles, 0);

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        const bool controlled = index == playerIndex;

        if (controlled && this->keyed)
        {
            continue;
        }

        const Role& role = state.roles[index];
        const u32 next = role.act.delay + 1u;
        const u32 pace = controlled || next > kMaxStepFrames ? 0 : next;
        this->glides[1 + index].Follow(role.x, role.y, frame, continuous, pace);
    }

    if (!this->keyed)
    {
        this->glides[0].Follow(field.cameraLeft, field.cameraTop, frame, continuous, 0);
    }

    if (!continuous)
    {
        this->effectGlides.clear();
    }
    else
    {
        std::erase_if(
            this->effectGlides,
            [frame](const auto& item)
            {
                return item.second.seen + 1 < frame;
            }
        );
    }

    this->placesValid = true;
    this->placesFrame = frame;
    this->placesMap = owner.currentMap;
    // The spotlight is drawn where it is now and moves after (0x00416B30).
    this->FollowEffect(
        EffectKey(EffectKind::Spotlight, u64{0}),
        owner.scene.spotlightX,
        owner.scene.spotlightY,
        true,
        0,
        kEffectSnap
    );
    this->cameraWidth = field.cameraRight - field.cameraLeft;
    this->cameraHeight = field.cameraBottom - field.cameraTop;
}

// The end of the map's frame (0x004120B0), once the cells are where the frame's steps left them (as the next frame's
// path scripts find them): every role the game's rules move (FieldView::PredictRole) heads for its next place, and so
// does the camera following a role the event walks or panning; while the keys move the controlled role, it and the
// camera head for where the next frame puts them: within a tile the step goes on, on a tile ControlPlayer takes the
// step RoleControl::PeekPlayerStep finds, and a pan goes on.
void FieldView::AimNextFrame()
{
    Game& owner = this->game;

    if (!owner.smoothMotion || this->glides.size() != 1 + GameState::Roles ||
        this->rolesAimed.size() != GameState::Roles)
    {
        return;
    }

    const Field& field = owner.field;
    const GameState& state = owner.state;
    const Scene& view = owner.scene;
    const u32 frame = this->placesFrame;
    const auto playerIndex = static_cast<std::size_t>(state.playerRole);
    const Role& player = state.roles[playerIndex];
    const bool panning = view.panAcross != 0 || view.panDown != 0;
    NextPlace playerNext;

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        if (index == playerIndex && this->keyed)
        {
            continue;
        }

        const NextPlace next = this->PredictRole(index);

        if (index == playerIndex)
        {
            playerNext = next;
        }

        if (!next.known)
        {
            continue;
        }

        const Role& role = state.roles[index];
        this->rolesAimed[index] = 1;
        this->glides[1 + index]
            .Aim(next.x, next.y, role.x, role.y, frame, this->placesContinuous, static_cast<double>(next.frames));
    }

    if (!this->keyed)
    {
        // The camera of a role the event walks follows it (CenterCamera each frame), and a pan goes on by its step.
        const bool quaking = view.shake[0] != Scene::ShakeEnd && owner.currentMap != Field::DemonRefiningPot;
        const bool centred = playerNext.known && (view.screenFlags & Scene::CameraHeld) == 0;

        if (quaking || (!panning && !centred))
        {
            return;
        }

        std::array<i32, 2> camera = {field.cameraLeft, field.cameraTop};

        if (centred)
        {
            camera = field.CameraCentredOn(playerNext.x, playerNext.y);
        }

        if (panning)
        {
            camera[0] += view.panStepX;
            camera[1] += view.panStepY;
        }

        const double frames = centred ? static_cast<double>(playerNext.frames) : 1.0;
        this->glides[0]
            .Aim(camera[0], camera[1], field.cameraLeft, field.cameraTop, frame, this->placesContinuous, frames);
        return;
    }

    this->rolesAimed[playerIndex] = 1;
    const bool stepping = owner.unk4B7518 != 0 || owner.unk4B7920 != 0 || owner.unk4A9488 != 0 || owner.unk4A948C != 0;
    RoleControl::PlayerStep next{owner.unk4B7518, owner.unk4B7920, owner.unk4A9488, owner.unk4A948C, state.walkSpeed};

    if (!stepping)
    {
        next = owner.roleControl.PeekPlayerStep();
    }

    this->glides[0].Aim(
        field.cameraLeft + next.speed * next.scrollX + (panning ? view.panStepX : 0),
        field.cameraTop + next.speed * next.scrollY + (panning ? view.panStepY : 0),
        field.cameraLeft,
        field.cameraTop,
        frame,
        this->placesContinuous
    );
    this->glides[1 + playerIndex].Aim(
        player.x + next.speed * (next.scrollX + next.aloneX),
        player.y + next.speed * (next.scrollY + next.aloneY),
        player.x,
        player.y,
        frame,
        this->placesContinuous
    );
}

// The moves of the map's frame (0x004120B0) one frame on: a role steps when its act's delay runs out, so delay plus one
// frames away. Within a tile it goes on by the step it takes now (doubled for a slow act or a fast move). On a tile the
// step is set as the next frame's path script sets it, from the cells as they are now: along its record's next
// direction (a path move only when its script runs on to a step, RolePath::StepsNext) unless the cells ahead are
// blocked (RolePath::Step, 0x00406580: walls and roles, walls only after RolePath::BlockedTries tries;
// RolePath::ScriptStep, 0x0042E280: walls, none with Role::PassWalls), none at the end of the record or where it
// arrived; a follower, finding a new path then (RolePath::Follow, 0x00406960), is taken to go on the way it went unless
// walls block it. A role that does not step (held for the event leader, facing a talker) stays.
FieldView::NextPlace FieldView::PredictRole(std::size_t index) const
{
    const Game& owner = this->game;
    const GameState& state = owner.state;
    const Scene& view = owner.scene;
    const Role& role = state.roles[index];
    NextPlace next{role.x, role.y, role.act.delay + 1u, false};

    if ((role.flags & Role::Visible) == 0 || next.frames > kMaxStepFrames)
    {
        return next;
    }

    const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;

    auto stepTowards = [&](std::size_t direction, u32 mask, int factor)
    {
        if (direction >= RolePath::Directions)
        {
            return;
        }

        const u32 probe = mask == 0 ? 0u : owner.field.ProbeCells(cell, role.act.width, role.act.height, mask);

        if ((probe & RolePath::BlockedBy[direction]) == 0)
        {
            next.x += RolePath::StepX[direction] * factor;
            next.y += RolePath::StepY[direction] * factor;
        }
    };

    for (const MoveRecord& move : state.moves)
    {
        if (static_cast<std::size_t>(move.role) != index || move.role == MoveRecord::NoRole)
        {
            continue;
        }

        next.known = true;

        if ((role.id == state.leader.id && move.Kind() == MoveKind::Path) ||
            ((role.flags & Role::FacesTalker) != 0 && role.talkState == Role::TalkWaiting))
        {
            return next;
        }

        int factor = role.unkD4 == 0 ? kFastFactor : 1;
        factor *= (role.flags & Role::FastMove) != 0 ? kFastFactor : 1;

        if (move.step < MoveRecord::StepMask)
        {
            next.x += move.stepX * factor;
            next.y += move.stepY * factor;
            return next;
        }

        if (role.x == move.x && role.y == move.y)
        {
            return next;
        }

        u32 mask = MapHeader::Wall;

        if (move.Kind() == MoveKind::Scripted)
        {
            mask = (role.flags & Role::PassWalls) != 0 ? 0u : static_cast<u32>(MapHeader::Wall);
        }
        else if (!owner.rolePath.StepsNext(static_cast<int>(index)))
        {
            return next;
        }
        else if (move.unk1A <= RolePath::BlockedTries)
        {
            mask = MapHeader::Wall | MapHeader::SolidRole;
        }

        stepTowards(move.CurrentDirection(), mask, factor);
        return next;
    }

    for (u32 follower = 1; follower < static_cast<u32>(view.unk4BABA0) && follower < view.followerMoves.size();
         ++follower)
    {
        const MoveRecord& move = view.followerMoves[follower];

        if (static_cast<std::size_t>(move.role) != index)
        {
            continue;
        }

        next.known = true;

        if (move.step < MoveRecord::StepMask)
        {
            next.x += move.stepX;
            next.y += move.stepY;
            return next;
        }

        const auto sign = [](i32 value)
        {
            return (value > 0) - (value < 0);
        };

        for (std::size_t direction = 0; direction < RolePath::Directions; ++direction)
        {
            if ((move.stepX != 0 || move.stepY != 0) && sign(RolePath::StepX[direction]) == sign(move.stepX) &&
                sign(RolePath::StepY[direction]) == sign(move.stepY))
            {
                const u32 mask = state.GameFlag(GameState::FollowersPassWalls) ? 0u : static_cast<u32>(MapHeader::Wall);
                stepTowards(direction, mask, RolePath::FollowerSpeed);
                break;
            }
        }

        return next;
    }

    return next;
}

// Between frames: a controlled role standing still that the keys now start heads for its first step at once, with
// the camera, to be there when the next frame takes the step.
void FieldView::RefreshAim(double time)
{
    Game& owner = this->game;

    if (!this->keyed || this->glides.size() != 1 + GameState::Roles || owner.unk4B7518 != 0 || owner.unk4B7920 != 0 ||
        owner.unk4A9488 != 0 || owner.unk4A948C != 0)
    {
        return;
    }

    const Field& field = owner.field;
    const Scene& view = owner.scene;
    const Role& player = owner.state.roles[static_cast<std::size_t>(owner.state.playerRole)];
    Glide& role = this->glides[1 + static_cast<std::size_t>(owner.state.playerRole)];

    if (role.toX != player.x || role.toY != player.y)
    {
        return;
    }

    const RoleControl::PlayerStep next = owner.roleControl.PeekPlayerStep();
    const i32 x = player.x + next.speed * (next.scrollX + next.aloneX);
    const i32 y = player.y + next.speed * (next.scrollY + next.aloneY);

    if (x == player.x && y == player.y)
    {
        return;
    }

    const bool panning = view.panAcross != 0 || view.panDown != 0;
    const double arrive = static_cast<double>(this->placesFrame) + 1.0;
    role.Retarget(time, x, y, arrive);
    this->glides[0].Retarget(
        time,
        field.cameraLeft + next.speed * next.scrollX + (panning ? view.panStepX : 0),
        field.cameraTop + next.speed * next.scrollY + (panning ? view.panStepY : 0),
        arrive
    );
}

std::array<i32, 2> FieldView::FollowEffect(u64 key, i32 x, i32 y, bool screen, u32 pace, i32 snap)
{
    if (!this->game.smoothMotion)
    {
        return {x, y};
    }

    if (this->replaying)
    {
        const auto found = this->effectGlides.find(key);

        if (found == this->effectGlides.end() || found->second.seen != this->placesFrame)
        {
            return {x, y};
        }

        return this->EffectPlace(found->second, this->paintColumn, this->paintRow);
    }

    const u32 frame = this->placesFrame;
    auto [entry, added] = this->effectGlides.try_emplace(key);
    EffectGlide& effect = entry->second;
    const bool continuous = !added && effect.seen + 1 == frame && effect.screen == screen &&
                            std::abs(x - effect.glide.toX) <= snap && std::abs(y - effect.glide.toY) <= snap;
    effect.glide.Follow(x, y, frame, continuous, pace);
    effect.screen = screen;
    effect.seen = frame;
    return {x, y};
}

std::array<i32, 2> FieldView::EffectPlace(const EffectGlide& effect, int column, int row) const
{
    const int scale = this->paintScale;

    if (effect.screen)
    {
        return {CeilDivide(effect.fineX - column, scale), CeilDivide(effect.fineY - row, scale)};
    }

    return {
        FloorDivide(this->paintCameraX + column, scale) + CeilDivide(effect.fineX - this->paintCameraX - column, scale),
        FloorDivide(this->paintCameraY + row, scale) + CeilDivide(effect.fineY - this->paintCameraY - row, scale),
    };
}

void FieldView::CaptureLayer(std::vector<Pixel>& layer) const
{
    const Pixel* pixels = this->game.drawSurface;

    if (pixels != nullptr)
    {
        layer.assign(pixels, pixels + Display::Pixels);
    }
}

void FieldView::PaintBetween(double alpha, Surface& out)
{
    Game& owner = this->game;
    const int scale = std::max(1, out.Width() / Display::Width);
    const std::span<Pixel> pixels(out.Pixels(), static_cast<std::size_t>(out.Width()) * out.Height());
    const bool layered = this->finalLayer.size() == Display::Pixels && this->worldLayer.size() == Display::Pixels &&
                         this->talkLayer.size() == Display::Pixels && this->headsLayer.size() == Display::Pixels &&
                         this->interfaceLayer.size() == Display::Pixels && this->fadedLayer.size() == Display::Pixels;

    if (!layered)
    {
        return;
    }

    if (this->movie.active != 0 || !this->placesValid || this->glides.size() != 1 + GameState::Roles)
    {
        FieldView::ScaleLayer(this->finalLayer, scale, pixels);
        this->paintedValid = false;
        return;
    }

    GameState& state = owner.state;
    Field& field = owner.field;
    Scene& view = owner.scene;
    Painter& painter = owner.painter;
    const double time = static_cast<double>(this->placesFrame) + alpha;
    this->RefreshAim(time);

    // The places in 1/scale pixels: the camera's, and every role's from the camera's (so that a role moving with
    // the camera keeps its place on the screen); a role that is not gliding stays on its pixel.
    const std::array<double, 2> camera = this->glides[0].Shown(time);
    const auto fineCameraX = static_cast<i32>(std::lround(camera[0] * scale));
    const auto fineCameraY = static_cast<i32>(std::lround(camera[1] * scale));
    std::array<i32, GameState::Roles> fineX = {};
    std::array<i32, GameState::Roles> fineY = {};
    this->placesKey.clear();
    this->placesKey.push_back(scale);
    this->placesKey.push_back(fineCameraX);
    this->placesKey.push_back(fineCameraY);

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        const Role& role = state.roles[index];
        const Glide& glide = this->glides[1 + index];

        const bool aimed = index < this->rolesAimed.size() && this->rolesAimed[index] != 0;

        if (aimed || (role.x == glide.toX && role.y == glide.toY))
        {
            const std::array<double, 2> place = glide.Shown(time);
            fineX[index] = fineCameraX + static_cast<i32>(std::lround((place[0] - camera[0]) * scale));
            fineY[index] = fineCameraY + static_cast<i32>(std::lround((place[1] - camera[1]) * scale));
        }
        else
        {
            fineX[index] = role.x * scale;
            fineY[index] = role.y * scale;
        }

        this->placesKey.push_back(fineX[index]);
        this->placesKey.push_back(fineY[index]);
    }

    // The effect elements the last game frame drew, placed like the roles (on the map) or on the screen.
    this->paintScale = scale;
    this->paintCameraX = fineCameraX;
    this->paintCameraY = fineCameraY;

    for (auto& [key, effect] : this->effectGlides)
    {
        if (effect.seen != this->placesFrame)
        {
            continue;
        }

        const std::array<double, 2> place = effect.glide.Shown(time);
        const std::array<double, 2> from = effect.screen ? std::array<double, 2>{} : camera;
        const std::array<i32, 2> base =
            effect.screen ? std::array<i32, 2>{} : std::array<i32, 2>{fineCameraX, fineCameraY};
        effect.fineX = base[0] + static_cast<i32>(std::lround((place[0] - from[0]) * scale));
        effect.fineY = base[1] + static_cast<i32>(std::lround((place[1] - from[1]) * scale));
        this->placesKey.push_back(effect.fineX);
        this->placesKey.push_back(effect.fineY);
    }

    // Within a game frame the picture depends on these places alone (the rest is put back the same every time):
    // with every place as last painted, the last picture stands.
    if (this->paintedValid && this->paintedFrame == this->placesFrame && this->paintedKey == this->placesKey &&
        this->paintedTarget == out.Pixels())
    {
        return;
    }

    std::swap(this->paintedKey, this->placesKey);
    this->paintedFrame = this->placesFrame;
    this->paintedTarget = out.Pixels();
    this->paintedValid = true;

    // What the map part changes is kept and put back after each painting: the camera, the roles' places, the random
    // numbers, the particles and acts of the screen effects, the spotlight, the tile grid's origin, and the painter's
    // options and target.
    const i32 cameraLeft = field.cameraLeft;
    const i32 cameraTop = field.cameraTop;
    const i32 cameraRight = field.cameraRight;
    const i32 cameraBottom = field.cameraBottom;
    std::array<i32, GameState::Roles> roleX = {};
    std::array<i32, GameState::Roles> roleY = {};

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        roleX[index] = state.roles[index].x;
        roleY[index] = state.roles[index].y;
    }

    const Random random = owner.random;
    const auto weather = view.weather;
    const auto weatherShades = view.weatherShades;
    const auto weatherMotion = view.weatherMotion;
    const auto leaves = view.leaves;
    const auto leafActs = view.unk4AD0D8;
    const ActState effectAct = view.unk4ACAF8;
    const ActState spotlightAct = view.unk4AD3F0;
    const auto heatLengths = view.heatLengths;
    const auto heatCounts = view.heatCounts;
    const auto heatOffsets = view.heatOffsets;
    const u16 heatFrame = view.heatFrame;
    const i16 lightningCount = view.lightningCount;
    const i16 lightningShown = view.lightningShown;
    const auto bolts = view.bolts;
    const i16 fireflyCount = view.fireflyCount;
    const i16 firefliesShown = view.firefliesShown;
    const auto fireflies = view.fireflies;
    const i32 spotlightX = view.spotlightX;
    const i32 spotlightY = view.spotlightY;
    const i32 spotlightStepX = view.spotlightStepX;
    const i32 spotlightStepY = view.spotlightStepY;
    const std::array<i32, 2> tileOrigin = {this->tileX, this->tileY};
    const DrawOptions options = painter.Options();
    const int wavePhase = painter.WavePhase();
    const auto [clipX, clipY, clipWidth, clipHeight] = painter.ClipRect();
    u16* const target = painter.Target();
    Pixel* const drawSurface = owner.drawSurface;

    auto putBack = [&]()
    {
        owner.random = random;
        view.weather = weather;
        view.weatherShades = weatherShades;
        view.weatherMotion = weatherMotion;
        view.leaves = leaves;
        view.unk4AD0D8 = leafActs;
        view.unk4ACAF8 = effectAct;
        view.unk4AD3F0 = spotlightAct;
        view.heatLengths = heatLengths;
        view.heatCounts = heatCounts;
        view.heatOffsets = heatOffsets;
        view.heatFrame = heatFrame;
        view.lightningCount = lightningCount;
        view.lightningShown = lightningShown;
        view.bolts = bolts;
        view.fireflyCount = fireflyCount;
        view.firefliesShown = firefliesShown;
        view.fireflies = fireflies;
        view.spotlightX = spotlightX;
        view.spotlightY = spotlightY;
        view.spotlightStepX = spotlightStepX;
        view.spotlightStepY = spotlightStepY;
        this->tileX = tileOrigin[0];
        this->tileY = tileOrigin[1];
        painter.Options() = options;
        painter.SetWavePhase(wavePhase);
        painter.SetClip(clipX, clipY, clipX + clipWidth, clipY + clipHeight);
        painter.SetTarget(target);
        owner.drawSurface = drawSurface;
    };

    // At a render scale N the screen is N x N logical screens woven together: the one of column a and row b of the
    // blocks shows a place p (in 1/N pixels) at the pixel ceil((p - a) / N), so it is the map painted at the camera
    // floor((camera + a) / N) with each role at that camera plus ceil((its place - camera - a) / N). Columns that
    // place the camera and every role alike (and rows alike) show the same screen, which is painted once. The
    // interface, the fade and what follows it lie on the pixels of the logical screen, so they are laid on each
    // screen before the weaving.
    const auto blocks = static_cast<std::size_t>(scale);
    std::array<std::vector<i32>, Game::MaxRenderScale> columnPlaces;
    std::array<std::vector<i32>, Game::MaxRenderScale> rowPlaces;
    std::array<std::size_t, Game::MaxRenderScale> columnShown = {};
    std::array<std::size_t, Game::MaxRenderScale> rowShown = {};

    auto placesOf = [this, &fineX, &fineY, scale](std::vector<i32>& places, i32 fineCamera, int offset, bool across)
    {
        const i32 cameraPlace = FloorDivide(fineCamera + offset, scale);
        places.resize(1 + GameState::Roles);
        places[0] = cameraPlace;

        for (std::size_t index = 0; index < GameState::Roles; ++index)
        {
            const i32 fine = across ? fineX[index] : fineY[index];
            places[1 + index] = cameraPlace + CeilDivide(fine - fineCamera - offset, scale);
        }

        for (const auto& [key, effect] : this->effectGlides)
        {
            if (effect.seen == this->placesFrame)
            {
                const std::array<i32, 2> spot = this->EffectPlace(effect, across ? offset : 0, across ? 0 : offset);
                places.push_back(across ? spot[0] : spot[1]);
            }
        }
    };

    for (std::size_t offset = 0; offset < blocks; ++offset)
    {
        placesOf(columnPlaces[offset], fineCameraX, static_cast<int>(offset), true);
        placesOf(rowPlaces[offset], fineCameraY, static_cast<int>(offset), false);
        columnShown[offset] = offset;
        rowShown[offset] = offset;

        for (std::size_t earlier = 0; earlier < offset; ++earlier)
        {
            if (columnShown[offset] == offset && columnPlaces[earlier] == columnPlaces[offset])
            {
                columnShown[offset] = earlier;
            }

            if (rowShown[offset] == offset && rowPlaces[earlier] == rowPlaces[offset])
            {
                rowShown[offset] = earlier;
            }
        }
    }

    if (scale > 1)
    {
        this->parts.resize(blocks * blocks);
    }

    auto spotlightGlide = this->effectGlides.find(EffectKey(EffectKind::Spotlight, u64{0}));

    if (spotlightGlide != this->effectGlides.end() && spotlightGlide->second.seen != this->placesFrame)
    {
        spotlightGlide = this->effectGlides.end();
    }

    this->replaying = true;

    for (std::size_t row = 0; row < blocks; ++row)
    {
        for (std::size_t column = 0; column < blocks; ++column)
        {
            if (rowShown[row] != row || columnShown[column] != column)
            {
                continue;
            }

            Surface& part = scale > 1 ? this->parts[row * blocks + column] : out;
            this->paintColumn = static_cast<int>(column);
            this->paintRow = static_cast<int>(row);

            if (part.Width() != Display::Width || part.Height() != Display::Height)
            {
                part.Resize(Display::Width, Display::Height);
            }

            field.cameraLeft = columnPlaces[column][0];
            field.cameraTop = rowPlaces[row][0];
            field.cameraRight = field.cameraLeft + this->cameraWidth;
            field.cameraBottom = field.cameraTop + this->cameraHeight;

            for (std::size_t index = 0; index < GameState::Roles; ++index)
            {
                state.roles[index].x = columnPlaces[column][1 + index];
                state.roles[index].y = rowPlaces[row][1 + index];
            }

            if (spotlightGlide != this->effectGlides.end())
            {
                const std::array<i32, 2> light =
                    this->EffectPlace(spotlightGlide->second, static_cast<int>(column), static_cast<int>(row));
                view.spotlightX = light[0];
                view.spotlightY = light[1];
            }

            owner.drawSurface = painter.LockSurface(part);
            painter.SetTarget(owner.drawSurface);
            painter.UnlockSurface(part);
            painter.SetClipClamped(0, 0, Display::Width, Display::Height);

            if ((view.screenFlags & Scene::SpotlightClip) != 0)
            {
                this->ClipToSpotlight();
            }

            this->PaintWorld();

            // The interface of the last game frame: the lines and talk windows it drew over its map, its role heads
            // drawn again where they glide, what it drew after them, its fade, then what it drew after the fade.
            const std::span<Pixel> screen(part.Pixels(), Display::Pixels);
            FieldView::CopySpans(this->talkSpans, this->talkLayer, 1, screen);
            painter.SetClipClamped(0, 0, Display::Width, Display::Height);
            this->DrawRoleHeads();
            putBack();
            FieldView::CopySpans(this->interfaceSpans, this->interfaceLayer, 1, screen);

            if (this->fadeTinted)
            {
                painter.TintPixelsFast(screen, this->fadeLevels.red, this->fadeLevels.green, this->fadeLevels.blue);
            }

            FieldView::CopySpans(this->finalSpans, this->finalLayer, 1, screen);
        }
    }

    this->replaying = false;
    field.cameraLeft = cameraLeft;
    field.cameraTop = cameraTop;
    field.cameraRight = cameraRight;
    field.cameraBottom = cameraBottom;

    for (std::size_t index = 0; index < GameState::Roles; ++index)
    {
        state.roles[index].x = roleX[index];
        state.roles[index].y = roleY[index];
    }

    if (scale > 1)
    {
        std::array<const Surface*, Game::MaxRenderScale * Game::MaxRenderScale> shown = {};

        for (std::size_t row = 0; row < blocks; ++row)
        {
            for (std::size_t column = 0; column < blocks; ++column)
            {
                shown[row * blocks + column] = &this->parts[rowShown[row] * blocks + columnShown[column]];
            }
        }

        FieldView::Weave(std::span<const Surface* const>(shown.data(), blocks * blocks), scale, pixels);
    }
}

// The runs where the frame being drawn now differs from `before` (once a game frame).
void FieldView::MarkChanges(const std::vector<Pixel>& before, std::vector<Span>& spans) const
{
    spans.clear();
    const Pixel* now = this->game.drawSurface;

    if (now == nullptr || before.size() != Display::Pixels)
    {
        return;
    }

    u32 index = 0;

    while (index < Display::Pixels)
    {
        while (index < Display::Pixels && now[index] == before[index])
        {
            ++index;
        }

        const u32 start = index;

        while (index < Display::Pixels && now[index] != before[index])
        {
            ++index;
        }

        if (index != start)
        {
            spans.push_back(Span{start, index - start});
        }
    }
}

void FieldView::CopySpans(
    const std::vector<Span>& spans,
    const std::vector<Pixel>& from,
    int scale,
    std::span<Pixel> to
)
{
    if (scale == 1)
    {
        for (const Span& run : spans)
        {
            std::copy_n(from.begin() + run.start, run.length, to.begin() + run.start);
        }

        return;
    }

    constexpr auto kWidth = static_cast<u32>(Display::Width);
    const auto blocks = static_cast<std::size_t>(scale);
    const std::size_t pitch = kWidth * blocks;

    for (const Span& run : spans)
    {
        const u32 end = run.start + run.length;

        for (u32 at = run.start; at < end;)
        {
            const u32 y = at / kWidth;
            const u32 piece = std::min(end, (y + 1) * kWidth) - at;
            Pixel* first = to.data() + y * blocks * pitch + (at % kWidth) * blocks;

            for (u32 pixel = 0; pixel < piece; ++pixel)
            {
                std::fill_n(first + pixel * blocks, blocks, from[at + pixel]);
            }

            for (std::size_t row = 1; row < blocks; ++row)
            {
                std::copy_n(first, piece * blocks, first + row * pitch);
            }

            at += piece;
        }
    }
}

void FieldView::ScaleLayer(const std::vector<Pixel>& from, int scale, std::span<Pixel> to)
{
    FieldView::CopySpans({Span{0, Display::Pixels}}, from, scale, to);
}

void FieldView::Weave(std::span<const Surface* const> parts, int scale, std::span<Pixel> to)
{
    const auto blocks = static_cast<std::size_t>(scale);
    const std::size_t pitch = static_cast<std::size_t>(Display::Width) * blocks;

    constexpr auto kWidth = static_cast<std::size_t>(Display::Width);
    std::array<const Pixel*, Game::MaxRenderScale> sources = {};

    for (int y = 0; y < Display::Height; ++y)
    {
        for (std::size_t row = 0; row < blocks; ++row)
        {
            Pixel* line = to.data() + (static_cast<std::size_t>(y) * blocks + row) * pitch;

            for (std::size_t column = 0; column < blocks; ++column)
            {
                sources[column] = parts[row * blocks + column]->Row(y);
            }

            std::size_t x = 0;

#if defined(__SSE2__)
            // Eight pixels of each part at a time, interleaved by unpacking.
            constexpr std::size_t kLanes = sizeof(__m128i) / sizeof(Pixel);
            constexpr std::size_t kPairBlocks = 2;  // the scales woven by unpacking pairs, then pairs of pairs
            constexpr std::size_t kQuadBlocks = 4;

            if (blocks == kPairBlocks)
            {
                for (; x + kLanes <= kWidth; x += kLanes)
                {
                    const __m128i first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[0] + x));
                    const __m128i second = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[1] + x));
                    auto* out = reinterpret_cast<__m128i*>(line + kPairBlocks * x);
                    _mm_storeu_si128(out, _mm_unpacklo_epi16(first, second));
                    _mm_storeu_si128(out + 1, _mm_unpackhi_epi16(first, second));
                }
            }
            else if (blocks == kQuadBlocks)
            {
                for (; x + kLanes <= kWidth; x += kLanes)
                {
                    const __m128i first = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[0] + x));
                    const __m128i second = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[1] + x));
                    const __m128i third = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[2] + x));
                    const __m128i fourth = _mm_loadu_si128(reinterpret_cast<const __m128i*>(sources[3] + x));
                    const __m128i lowPairs = _mm_unpacklo_epi16(first, second);
                    const __m128i highPairs = _mm_unpackhi_epi16(first, second);
                    const __m128i lowOthers = _mm_unpacklo_epi16(third, fourth);
                    const __m128i highOthers = _mm_unpackhi_epi16(third, fourth);
                    auto* out = reinterpret_cast<__m128i*>(line + kQuadBlocks * x);
                    _mm_storeu_si128(out, _mm_unpacklo_epi32(lowPairs, lowOthers));
                    _mm_storeu_si128(out + 1, _mm_unpackhi_epi32(lowPairs, lowOthers));
                    _mm_storeu_si128(out + 2, _mm_unpacklo_epi32(highPairs, highOthers));
                    _mm_storeu_si128(out + 3, _mm_unpackhi_epi32(highPairs, highOthers));
                }
            }
#endif

            for (; x < kWidth; ++x)
            {
                for (std::size_t column = 0; column < blocks; ++column)
                {
                    line[x * blocks + column] = sources[column][x];
                }
            }
        }
    }
}

}  // namespace swd3
