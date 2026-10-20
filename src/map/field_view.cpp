#include "map/field_view_internal.h"

namespace swd3
{

namespace
{

// swd32.exe 0x00499534 and 0x00499554: the cell step of each path direction.
constexpr std::array<i32, 8> kCellX = {1, 0, -1, -1, -1, 0, 1, 1};
constexpr std::array<i32, 8> kCellY = {1, 1, 1, 0, -1, -1, -1, 0};

// The cell of a role moved a step in path direction `direction` (inline in 0x004120B0).
void MoveCell(GameState& state, Role& role, int direction)
{
    if (role.cell == nullptr || direction < 0 || direction >= RolePath::Directions)
    {
        return;
    }

    const std::ptrdiff_t at =
        role.cell - state.map.blocks.data() + kCellX[static_cast<std::size_t>(direction)] +
        static_cast<std::ptrdiff_t>(state.map.width) * kCellY[static_cast<std::size_t>(direction)];
    role.cell = at >= 0 && static_cast<std::size_t>(at) < state.map.blocks.size()
                    ? &state.map.blocks[static_cast<std::size_t>(at)]
                    : nullptr;
}

}  // namespace

FieldView::FieldView(Game& game)
    : game(game)
{
}

// swd32.exe 0x00430180: the outline of a cell mark: the top and bottom lines as `width / 2` pixel pairs, the sides
// `height` pixels long
void FieldView::FillCellMark(int x, int y, int width, int height, int color)
{
    Game& owner = this->game;
    const int pitch = owner.painter.ScreenPitch();
    const auto pixels = static_cast<std::ptrdiff_t>(pitch) * Display::Height;
    const auto value = static_cast<u16>(color);
    // The lines are written as pixel pairs of (colour << 16) + colour (0x0043019B..0x0043019E): the whole colour is
    // added, so bits above 16 change the second pixel.
    const u32 pairValue = (static_cast<u32>(value) << (CHAR_BIT * sizeof(u16))) + static_cast<u32>(color);
    const auto second = static_cast<u16>(pairValue >> (CHAR_BIT * sizeof(u16)));

    auto put = [&owner, pixels](std::ptrdiff_t at, u16 pixel)
    {
        if (at >= 0 && at < pixels)
        {
            owner.drawSurface[at] = pixel;
        }
    };

    const int pairs = width >> 1;  // sar (0x004301CF)

    for (int pair = 0; pair < pairs; ++pair)
    {
        const std::ptrdiff_t top = x + static_cast<std::ptrdiff_t>(pitch) * y + kPixelsPerPair * pair;
        const std::ptrdiff_t bottom = x + static_cast<std::ptrdiff_t>(pitch) * (y + height) + kPixelsPerPair * pair;
        put(top, value);
        put(top + 1, second);
        put(bottom, value);
        put(bottom + 1, second);
    }

    for (int line = 0; line < height; ++line)
    {
        const std::ptrdiff_t left = x + static_cast<std::ptrdiff_t>(pitch) * (y + line);
        put(left, value);
        put(left + kPixelsPerPair * pairs, value);
    }
}

// swd32.exe 0x00430230: the debug marks of the cells under the camera holding any bit of `mask` (38 x 28 cells from
// one past the camera), inset by `inset` pixels
void FieldView::MarkCells(u32 mask, int color, int offsetX, int offsetY, int inset)
{
    const Field& field = this->game.field;
    const int width = this->game.state.map.width;
    const int shiftY = offsetY & MapHeader::TileFraction;
    const int shiftX = -(offsetX & MapHeader::TileFraction);
    int cell = ((field.cameraLeft + MapHeader::TileSize) >> MapHeader::TileShift) +
               width * ((field.cameraTop + MapHeader::TileSize) >> MapHeader::TileShift);
    int y = inset - shiftY + MapHeader::TileSize;

    for (int row = 0; row < kScreenRows - kMarkBorder; ++row)
    {
        for (int x = MapHeader::TileSize; x < Display::Width - MapHeader::TileSize - 1; x += MapHeader::TileSize)
        {
            const std::vector<u32>& blocks = this->game.state.map.blocks;
            const u32 value = cell >= 0 && static_cast<std::size_t>(cell) < blocks.size()
                                  ? blocks[static_cast<std::size_t>(cell)]
                                  : 0;

            if ((mask & value) != 0)
            {
                this->FillCellMark(
                    inset + x + shiftX,
                    y,
                    MapHeader::TileFraction - inset,
                    MapHeader::TileFraction - inset,
                    color
                );
            }

            ++cell;
        }

        y += MapHeader::TileSize;
        cell += width - (kScreenColumns - kMarkBorder);
    }
}

// swd32.exe 0x00414570: the camera pans by the step of the pan still to go (0x004A99F0 across, 0x004A98C0 down); a step
// is cleared once its distance is covered
void FieldView::PanCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    if (view.panAcross == 0 && view.panDown == 0)
    {
        return;
    }

    // The rebuild's continuous motion moves the camera by the step over the frame instead (StartPan).
    if (this->game.smoothMotion)
    {
        this->StartPan(view.panStepX, view.panStepY);
    }
    else
    {
        field.cameraLeft += view.panStepX;
        field.cameraRight += view.panStepX;
        field.cameraTop += view.panStepY;
        field.cameraBottom += view.panStepY;
    }

    const int remaining = view.panDown - view.panStepY;
    view.panAcross -= view.panStepX;
    view.panDown -= view.panStepY;

    if (view.panAcross == 0)
    {
        view.panStepX = 0;
    }

    if (remaining == 0)
    {
        view.panStepY = 0;
    }
}

// swd32.exe 0x004148F0: the quake: the camera is offset by the pairs of the shake table (0x004ACE70, ended by 0xCFCF)
// in turn, each held for 0x004AD0D0 frames (not on map 0x16); the camera before it is kept to be restored
void FieldView::ShakeCamera()
{
    Field& field = this->game.field;
    Scene& view = this->game.scene;

    this->shakeShift = {0, 0};

    if (view.shake[0] == Scene::ShakeEnd || this->game.currentMap == Field::DemonRefiningPot)
    {
        return;
    }

    auto at = static_cast<std::size_t>(view.shakeIndex);

    if (at >= view.shake.size() || view.shake[at] == Scene::ShakeEnd)
    {
        at = 0;
        view.shakeIndex = 0;
    }

    const int dx = view.shake[at];
    const int dy = at + 1 < view.shake.size() ? view.shake[at + 1] : 0;

    if (--view.shakeFrames <= 0)
    {
        view.shakeFrames = view.shakeHold;
        view.shakeIndex = static_cast<i32>(at) + kShakeEntry;
    }

    this->shakeShift = {dx, dy};
    view.unk4A93E4 = field.cameraLeft;
    field.cameraLeft += dx;
    view.unk4A992C = field.cameraTop;
    field.cameraTop += dy;
    field.cameraRight += dx;
    field.cameraBottom += dy;
}

// swd32.exe 0x00412930: the frame of the map: the movie, or the overlays, the tiles (8-bit maps through their palette),
// reflections, roles, animations, sprites and the screen effects (all but the spotlight left out with story flag
// 0x48); then the lines, the talk windows, the role heads, the gold, the pointer, the fade and the notices
void FieldView::Paint()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Painter& painter = owner.painter;
    Scene& view = owner.scene;
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);

    if ((view.screenFlags & Scene::SpotlightClip) != 0)
    {
        this->ClipToSpotlight();
    }

    if (this->movie.active != 0)
    {
        this->UpdateMovie();
    }
    else
    {
        this->PaintWorld();
    }

    const bool capture = owner.smoothMotion;

    if (capture)
    {
        this->CaptureLayer(this->worldLayer);
        this->StartRecord(this->talkRecord);
    }

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    this->DrawLines();
    owner.talk.Draw();

    if (capture)
    {
        this->CaptureLayer(this->talkLayer);
        this->MarkChanges(this->worldLayer, this->talkSpans);
        this->talkRedrawn = this->FinishRecord(this->talkRecord, this->talkLayer);
    }

    this->DrawRoleHeads();

    if (capture)
    {
        this->CaptureLayer(this->headsLayer);
        this->StartRecord(this->interfaceRecord);
    }

    if ((state.leader.id == Role::NoId || state.leader.unk20 < kUnkLeader20Limit) &&
        !state.GameFlag(GameState::HideHud))
    {
        owner.actDrawer.DrawCounter(
            ActDrawer::GoldX,
            ActDrawer::GoldY,
            static_cast<u32>(owner.party.variables[Party::Gold]),
            owner.counterAlpha
        );
    }

    const bool pointer = (!state.GameFlag(GameState::NoPointer) && !state.GameFlag(GameState::NoChoicePointer) &&
                          !state.GameFlag(GameState::HideHud)) ||
                         owner.input.IsKeyDown(Input::C) != 0;

    if (pointer && !capture)
    {
        owner.RefreshCursor();
    }

    if (capture)
    {
        // The pointer itself stays out of the interface layer: between frames it is drawn where the mouse is now.
        if (pointer)
        {
            owner.StepPointer();
        }

        const ColorFade& fade = view.fade;
        this->CaptureLayer(this->interfaceLayer);
        this->MarkChanges(this->headsLayer, this->interfaceSpans);
        this->interfaceRedrawn = this->FinishRecord(this->interfaceRecord, this->interfaceLayer);
        this->fadeTinted = fade.stepRed != 0.0F || fade.stepGreen != 0.0F || fade.stepBlue != 0.0F;
        this->pointerShown = pointer;

        if (pointer)
        {
            // The rebuild's continuous motion: the picture sways (its act moves its anchor) over the next frame.
            const ActState& act = view.pointerRole.act;
            const std::array<i32, 2> sway = {-act.x, -act.y};
            const u64 key = EffectKey(EffectKind::Pointer, u64{0});
            std::array<i32, 2> shown = this->EffectAt(key, this->pointerSway[0], this->pointerSway[1], true);

            if (std::abs(sway[0] - shown[0]) > kPointerSnap || std::abs(sway[1] - shown[1]) > kPointerSnap)
            {
                shown = sway;
            }

            owner.DrawPointer(state.pointerX + shown[0] + act.x, state.pointerY + shown[1] + act.y);
            this->MoveEffect(key, this->pointerSway[0], this->pointerSway[1], sway[0], sway[1], 1, true, kPointerSnap);
            this->pointerSway = sway;
        }
    }

    this->FadeColors(1);

    if (capture)
    {
        const ColorFade& fade = view.fade;
        this->fadeLevels = {
            static_cast<int>(crt::Ftol(fade.red)),
            static_cast<int>(crt::Ftol(fade.green)),
            static_cast<int>(crt::Ftol(fade.blue)),
        };

        this->CaptureLayer(this->fadedLayer);
        this->StartRecord(this->finalRecord);
    }

    owner.notices.Draw(owner.smallFont, painter, state, owner.textColors);
}

// The spotlight's square of the screen as the clip (part of 0x00412930).
void FieldView::ClipToSpotlight()
{
    const Scene& view = this->game.scene;
    this->game.painter.SetClipClamped(
        view.spotlightX - Scene::SpotlightRadius,
        view.spotlightY - Scene::SpotlightRadius,
        view.spotlightX + Scene::SpotlightRadius,
        view.spotlightY + Scene::SpotlightRadius
    );
}

// The map part of the frame without a movie (part of 0x00412930): the overlays, the tiles, reflections, roles,
// animations, sprites and the screen effects.
void FieldView::PaintWorld()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Painter& painter = owner.painter;
    Scene& view = owner.scene;
    const Field& field = owner.field;
    bool later = true;

    if (state.GameFlag(GameState::OverlaysFirst) || state.GameFlag(GameState::Spotlight))
    {
        std::memset(owner.drawSurface, 0, sizeof(Pixel) * Display::Pixels);
    }

    if ((view.screenFlags & Scene::BlackScreen) != 0)
    {
        std::memset(owner.drawSurface, 0, sizeof(Pixel) * Display::Pixels);
    }
    else
    {
        this->DrawOverlays();

        if (!state.GameFlag(GameState::PlainScreen) && state.GameFlag(GameState::Spotlight))
        {
            this->ClipToSpotlight();
        }

        const bool scrolled =
            (field.cameraLeft & MapHeader::TileFraction) != 0 || (field.cameraTop & MapHeader::TileFraction) != 0;

        if (state.map.bitsPerPixel == MapHeader::TrueColorBits)
        {
            scrolled ? this->DrawTiles16Scrolled(0) : this->DrawTiles16(0);
        }
        else
        {
            painter.SetPalette(field.palette.data());
            scrolled ? this->DrawTiles8Scrolled(0) : this->DrawTiles8(0);
        }

        painter.SetPalette(nullptr);

        if (!state.GameFlag(GameState::HideActRoles))
        {
            this->DrawReflections();
        }

        this->DrawRoles();
        this->DrawAnimations(view.animationsA);
        this->DrawSprites();

        if (!state.GameFlag(GameState::PlainScreen) && (view.screenFlags & Scene::BlackScreen) == 0)
        {
            this->DrawLeaves();
            this->DrawLightning();
            this->DrawFireflies();
            this->DrawWeather();
            this->DrawHeat();
            this->DrawRipples();
        }

        if (!state.GameFlag(GameState::PlainScreen))
        {
            this->DrawSpotlight();
            this->DrawAnimations(view.animationsB);
            later = false;
        }
    }

    if (later)
    {
        this->DrawAnimations(view.animationsB);
    }
}

// The flags a role takes from its cell on entering it (inline in the original): a reflecting cell (bit 11) sets flag
// 29, the shade bits 12..15 go to flags 20..23
void FieldView::UpdateRoleCellFlags(Role& role)
{
    if ((role.flags & Role::CellEffects) == 0)
    {
        return;
    }

    role.flags &= ~(Role::Reflects | Role::CellShades);
    const u32 cell = role.cell != nullptr ? *role.cell : 0;

    if ((cell & MapHeader::Reflecting) != 0)
    {
        role.flags |= Role::Reflects;
    }

    if ((cell & MapHeader::ShadeMask) != 0)
    {
        role.flags |= (cell & MapHeader::ShadeMask) << MapHeader::ShadeShift;
    }
}

// A role on a move whose step lands on a tile (part of 0x004120B0): it leaves its old line; at the end of the move it
// takes the move's act, action and direction, resumes a held move and, for role 1, stops the keys and recentres; its
// cells follow its step.
void FieldView::ArriveMoving(std::size_t index, std::size_t slot, u8 direction)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Field& field = owner.field;
    MoveRecord& move = state.moves[slot];
    Role& role = state.roles[index];
    const u16 id = role.id;
    const i32 leaderId = owner.unk4B7BC4;
    field.RemoveRole(
        id,
        static_cast<int>(role.flags & Role::LayerMask),
        (static_cast<u32>(role.y) >> MapHeader::TileShift) - 1,
        0
    );

    if (role.x == move.x && role.y == move.y)
    {
        role.act.speed = 0;
        role.flags = (role.flags & ~(Role::Stepping | Role::FastMove | Role::Arrived)) | Role::Arrived;

        if (move.act != -1)
        {
            role.act.id = move.act;
        }

        if (move.action != -1)
        {
            role.act.action = move.action;
        }

        if (move.direction != -1)
        {
            role.act.direction = move.direction;
        }

        if ((move.mode & MoveRecord::HoldAtEnd) == 0)
        {
            owner.rolePath.ResumeMove(static_cast<int>(index));
            role.act.delay = 0;
            role.flags &= ~Role::Held;
        }

        if (role.id == 1)
        {
            owner.unk4B7920 = 0;
            owner.controls.buttons = {};
            owner.unk4B7518 = 0;
            owner.unk4A948C = 0;
            owner.unk4A9488 = 0;

            if ((view.screenFlags & Scene::CameraHeld) == 0)
            {
                field.CenterCamera();
            }
        }

        role.flags &= ~Role::PassWalls;
    }

    field.ClearRoleCells(role, leaderId);

    if (direction != MoveRecord::EndOfPath)
    {
        MoveCell(state, role, direction);
    }

    field.MarkRoleCells(role, leaderId);
    move.step = static_cast<u16>((move.step + 1) | MoveRecord::Done);
    this->UpdateRoleCellFlags(role);
}

// A follower whose step lands on a tile (part of 0x004120B0): its cells follow its step.
void FieldView::ArriveFollower(std::size_t index, std::size_t slot, u8 direction)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Field& field = owner.field;
    MoveRecord& move = owner.scene.followerMoves[slot];
    Role& role = state.roles[index];
    const i32 leaderId = owner.unk4B7BC4;
    field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), 0, 0);
    field.ClearRoleCells(role, leaderId);
    MoveCell(state, role, direction);
    field.MarkRoleCells(role, leaderId);
    move.step = static_cast<u16>((move.step + 1) | MoveRecord::Done);
    this->UpdateRoleCellFlags(role);
}

// swd32.exe 0x004120B0: a frame of the map: the screen acts step, the camera and the controlled role move by their
// step, every role on a move steps along it (twice as fast without flag 0xD4 or with flag 0x4000000; at the end it
// takes the move's act, resumes a held move and, for role 1, stops the keys and recentres), touch roles start their
// event, the followers step, the camera pans and shakes; then the frame is painted with the timer and the debug display
// and shown; the controlled role's cells and trail follow its step, the tile layers cycle, and a shaken camera is put
// back
void FieldView::Render()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Field& field = owner.field;

    for (std::size_t act = view.unk4B9F68.size(); act-- > 0;)
    {
        ActState& slot = view.unk4B9F68[act];

        if (slot.id != 0 && owner.acts.Run(slot) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:HeadSgn) 頭定符號設定錯誤 \nact[%d] qq[%d] dir[%d]",
                slot.id,
                slot.action,
                slot.direction
            );
        }
    }

    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];

    // The rebuild's continuous motion moves the controlled role and the camera a millisecond at a time instead.
    if (!owner.smoothMotion)
    {
        const int scroll = owner.state.walkSpeed;
        player.x += scroll * (owner.unk4B7518 + owner.unk4A9488);
        player.y += scroll * (owner.unk4B7920 + owner.unk4A948C);
        field.cameraLeft += owner.unk4B7518 * scroll;
        field.cameraTop += owner.unk4B7920 * scroll;
        field.cameraRight += owner.unk4B7518 * scroll;
        field.cameraBottom += owner.unk4B7920 * scroll;
    }

    for (MoveRecord& move : state.moves)
    {
        if (move.role == MoveRecord::NoRole || move.step >= MoveRecord::StepMask)
        {
            continue;
        }

        const u16 index = move.role;

        if (index >= GameState::Roles)
        {
            continue;
        }

        Role& role = state.roles[index];

        if ((role.flags & Role::Visible) == 0)
        {
            continue;
        }

        const u16 id = role.id;

        if ((id == state.leader.id && move.Kind() == MoveKind::Path) ||
            ((role.flags & Role::FacesTalker) != 0 && role.talkState == Role::TalkWaiting))
        {
            continue;
        }

        if (role.act.delay == 0)
        {
            int stepX = move.stepX;
            int stepY = move.stepY;

            if (role.unkD4 == 0)
            {
                stepX *= kFastFactor;
                stepY *= kFastFactor;
            }

            if ((role.flags & Role::FastMove) != 0)
            {
                stepX *= kFastFactor;
                stepY *= kFastFactor;
            }

            const u8 direction = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];

            if (direction != MoveRecord::EndOfPath && direction < RolePath::Directions)
            {
                role.act.direction = RolePath::Facing[direction];
            }

            const auto slot = static_cast<std::size_t>(&move - state.moves.data());

            if (owner.smoothMotion)
            {
                this->StartRoleStep(index, stepX, stepY, slot, direction, false);
            }
            else
            {
                role.x += stepX;
                role.y += stepY;

                if (((role.x | role.y) & MapHeader::TileFraction) == 0)
                {
                    this->ArriveMoving(index, slot, direction);
                }
            }

            if (state.leader.id == Role::NoId && (role.flags & Role::TouchEvent) != 0)
            {
                const std::ptrdiff_t cell = role.cell != nullptr ? role.cell - state.map.blocks.data() : 0;

                if (static_cast<u8>(field.ProbeCells(cell, role.act.width, role.act.height, MapHeader::LeaderCell)) !=
                    0)
                {
                    state.leader.unk14 = role.unk14;
                    state.leader.unk20 = role.unk20;
                    state.leader.event = role.event;
                    state.leader.id = role.id;
                    state.leader.flags = role.flags;
                }
            }
        }

        if (owner.acts.Run(role.act) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:PathMove) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
                role.id,
                role.act.id,
                role.act.action,
                role.act.direction
            );
        }

        this->TimeRoleStep(index);

        if (index == state.playerRole && (view.screenFlags & Scene::CameraHeld) == 0)
        {
            field.CenterCamera();
        }
    }

    for (u32 follower = 1; follower < static_cast<u32>(view.unk4BABA0) && follower < view.followerMoves.size();
         ++follower)
    {
        MoveRecord& move = view.followerMoves[follower];

        if (move.role == MoveRecord::NoRole || move.role >= GameState::Roles)
        {
            continue;
        }

        Role& role = state.roles[move.role];

        if (move.step >= MoveRecord::StepMask)
        {
            owner.acts.Run(role.act);
            continue;
        }

        if (role.act.delay == 0)
        {
            const u8 direction = move.directions[static_cast<std::size_t>(move.step) % move.directions.size()];

            if (direction < RolePath::Directions)
            {
                role.act.direction = RolePath::Facing[direction];
            }

            if (owner.smoothMotion)
            {
                this->StartRoleStep(move.role, move.stepX, move.stepY, follower, direction, true);
            }
            else
            {
                role.x += move.stepX;
                role.y += move.stepY;

                if (((role.x | role.y) & MapHeader::TileFraction) == 0)
                {
                    this->ArriveFollower(move.role, follower, direction);
                }
            }
        }

        if (owner.acts.Run(role.act) == 0)
        {
            DebugLog(
                u8"Act Err(picPaint:CompanyMove) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
                role.id,
                role.act.id,
                role.act.action,
                role.act.direction
            );
        }

        this->TimeRoleStep(move.role);
    }

    this->PanCamera();
    this->ShakeCamera();
    owner.audioDevice.Serve();
    owner.drawSurface = owner.painter.LockSurface(*owner.offscreen);
    owner.painter.SetTarget(owner.drawSurface);
    owner.painter.UnlockSurface(*owner.offscreen);
    this->CapturePlaces();
    this->Paint();
    this->DrawTimer(kTimerX, kTimerY, 0);

    if (owner.config.debugMode == 1)
    {
        this->DrawDebug(field.cameraLeft, field.cameraTop);
    }

    owner.audioDevice.Serve();

    if (owner.smoothMotion)
    {
        this->CaptureLayer(this->finalLayer);
        this->MarkChanges(this->fadedLayer, this->finalSpans);
        this->finalRedrawn = this->FinishRecord(this->finalRecord, this->finalLayer);
    }

    owner.ShowMapFrame();

    // The rebuild's continuous motion settles the controlled role when it reaches a tile; here only while it stands.
    const bool stepping = owner.unk4B7518 != 0 || owner.unk4B7920 != 0 || owner.unk4A9488 != 0 || owner.unk4A948C != 0;

    if ((player.x & MapHeader::TileFraction) == 0 && (player.y & MapHeader::TileFraction) == 0 &&
        (!owner.smoothMotion || !stepping))
    {
        this->SettlePlayer();
    }

    if ((player.flags & Role::Stepping) == 0 && (player.flags & Role::StepsAct) != 0 && owner.acts.Run(player.act) == 0)
    {
        DebugLog(
            u8"Act Err(PicPaint:Render:MRl) 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]",
            player.id,
            player.act.id,
            player.act.action,
            player.act.direction
        );
    }

    if (++view.unk4CC2DC >= view.unk49E0BC && state.map.layers != 1)
    {
        int layer = view.unk49E0C0 + view.unk4B7944;
        view.unk4CC2DC = 0;
        view.unk4B7944 += view.unk49E0C0;

        if (static_cast<u32>(view.unk4B7944) >= static_cast<u32>(state.map.layers))
        {
            view.unk49E0C0 = view.unk49E0C0 != 1 ? 1 : -1;
            layer += kLayerSwing * view.unk49E0C0;
            view.unk4B7944 = layer;
        }

        view.unk4B873C = layer * view.unk4B7A98;
    }

    if (view.shake[0] != Scene::ShakeEnd && owner.currentMap != Field::DemonRefiningPot)
    {
        field.cameraLeft = view.unk4A93E4;
        field.cameraTop = view.unk4A992C;
        field.cameraRight = view.unk4A93E4 + Display::Width;
        field.cameraBottom = view.unk4A992C + Display::Height;
    }
}

}  // namespace swd3
