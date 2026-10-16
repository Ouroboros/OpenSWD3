#include "core/debug_log.h"
#include "game/game.h"
#include "input/dik.h"
#include "item/item_list.h"
#include "platform/system_info.h"
#include "role/role_control.h"

namespace swd3
{

namespace
{

}  // namespace

// The role faced turned to look back at the controlled role, the controlled role turned towards it, and the event of
// the role taken by the leader. (When the controlled role walks into the role, the original runs the role's act a
// second time where the controlled role's act would be run.)
void RoleControl::Approach(Role& player, Role& other, bool walking)
{
    Game& game = this->game;
    const int facing = game.state.DirectionToPlayer(other.x + 8 * other.act.width, other.y + 8 * other.act.height);

    if ((other.flags & role_flag::kFacesTalker) != 0)
    {
        other.act.savedDirection = other.act.direction;
        other.act.savedAction = other.act.action;
        other.act.action = 0;
        other.act.direction = facing;
        other.act.delay = 0;

        if (game.acts.Run(other.act) == 0)
        {
            DebugLog(u8"Act Err(approach)", other.id);
        }
    }

    player.act.action = 0;
    player.act.direction = (facing & 0xFFFE) + ((static_cast<u8>(facing) - 1) & 1);
    player.act.delay = 0;

    if (game.acts.Run(walking ? other.act : player.act) == 0)
    {
        DebugLog(u8"Act Err(approach)", player.id);
    }

    Role& leader = game.state.leader;
    leader.unk14 = other.unk14;
    leader.unk20 = other.unk20;
    leader.event = other.event;
    leader.id = other.id;
    leader.flags = other.flags;
    DebugLog(u8"talk to role", other.id);
}

// swd32.exe 0x00402F80: the keys of the map. The debug keys (story flag 0x52, turned on in debug mode with Ctrl,
// right Shift, Home and End): F12 or Ctrl+S / Ctrl+L the save and load screens, Ctrl+F9 skipping texts, Ctrl+Q
// quitting, Ctrl+F1..F3 the debug lines, cells and flag 0x14, Tab with keypad + / - the map's speed, F4 the cheat
// dialog, F7 1000 gold, F6 ten of every item. Key slot 16 toggles running. Then, when the controlled role stands
// still and no menu is open: Space and Enter move the talk windows on (or close the choices); Space talks to the
// role in front, Enter opens the menu; the arrows move the role (or the choice): a step slides along walls, runs into
// map regions and roles with events, scrolls the camera or moves the role alone, and may meet enemies.
void RoleControl::ControlPlayer()
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Input& input = owner.input;
    Scene& view = owner.scene;
    const Field& field = owner.field;
    Controls& controls = owner.controls;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    Role& leader = state.leader;
    int pressed = 0;
    int stepX = 0;
    int stepY = 0;

    if (owner.config.debugMode == 1 && input.IsKeyDown(dik::kRightControl) && input.IsKeyDown(dik::kRightShift) &&
        input.IsKeyDown(dik::kHome) && input.IsKeyDown(dik::kEnd))
    {
        state.SetGameFlag(story_flag::kDebugKeys);
    }

    if (state.GameFlag(story_flag::kDebugKeys))
    {
        const bool idle = leader.id == 0xFFFF && view.talkWindows == nullptr;

        if (input.IsKeyDown(dik::kF12) && idle)
        {
            SleepMilliseconds(0x1F4);
            owner.storyState = 3;
            owner.menuMode = 1;
            owner.unk4CAEBC = 0;
            owner.unk4CAEC0 = 0;
            owner.ResetSaveSlots();
            return;
        }

        if (input.IsKeyDown(dik::kLeftControl) || input.IsKeyDown(dik::kRightControl))
        {
            if (input.IsKeyDown(dik::kF9))
            {
                owner.talk.skipAll = (owner.talk.skipAll + 1) % 2;
                SleepMilliseconds(0x15E);
                return;
            }

            if (input.IsKeyDown(dik::kQ))
            {
                owner.inactive = 1;
                state.ClearFrameInterval();
                owner.systemFlags |= 4;
                state.ClearGameFlag(story_flag::kRunning);
                return;
            }

            if (input.IsKeyDown(dik::kS) && idle)
            {
                owner.unk4CAEBC = 1;
                owner.unk4CAEC0 = 0;
                SleepMilliseconds(0x1F4);
                owner.menuMode = 1;
                owner.storyState = 3;
                owner.ResetSaveSlots();
                return;
            }

            if (input.IsKeyDown(dik::kL) && idle)
            {
                owner.unk4CAEBC = 1;
                owner.unk4CAEC0 = 1;
                SleepMilliseconds(0x1F4);
                owner.menuMode = 2;
                owner.storyState = 3;
                owner.ResetSaveSlots();
                return;
            }

            if (input.IsKeyDown(dik::kF1) || input.IsKeyDown(dik::kF2))
            {
                i32& shown = input.IsKeyDown(dik::kF1) ? owner.debugText : owner.debugCells;
                shown = shown == 0 ? 1 : 0;
                this->unk4CB220 = 0;
                SleepMilliseconds(0x12C);
                return;
            }

            if (input.IsKeyDown(dik::kF3))
            {
                state.SetGameFlagTo(story_flag::kNoSlide, state.GameFlag(story_flag::kNoSlide) ? 0 : 1);
                this->unk4CB220 = 0;
                SleepMilliseconds(0x12C);
                return;
            }

            if (input.IsKeyDown(dik::kTab) && input.IsKeyDown(dik::kNumpadPlus))
            {
                if (++view.unk49E0BC > 0x40)
                {
                    view.unk49E0BC = 0x40;
                }

                this->unk4CB220 = 0;
            }

            if (input.IsKeyDown(dik::kTab) && input.IsKeyDown(dik::kNumpadMinus))
            {
                if (--view.unk49E0BC < 1)
                {
                    view.unk49E0BC = 1;
                }

                this->unk4CB220 = 0;
            }
        }

        if (input.IsKeyDown(dik::kF4))
        {
            owner.running = 0;
            owner.running = 1;
            return;
        }

        if (input.IsKeyDown(dik::kF7))
        {
            owner.party.variables[0] += 0x3E8;
            SleepMilliseconds(0xFA);
        }

        if (input.IsKeyDown(dik::kF6))
        {
            for (u16 id = 0x65; id < 0x4B0; ++id)
            {
                ObjectRecord record;

                if (owner.database.LoadObject(record, id))
                {
                    const u16 kind = record.data.unk52;

                    if (kind < 0xA || kind == 0x12 || kind == 0x13 || kind == 0x11 || kind == 0x10 || kind == 0xF ||
                        kind == 0x14 || kind == 0x15 || kind == 0x18)
                    {
                        ItemLink& list =
                            owner.party.inventory != nullptr ? owner.party.inventory->next : owner.party.inventory;
                        AddItem(list, id, 0xA, ItemCountMode::Pending, owner.database);
                    }
                }
            }

            SleepMilliseconds(0x1F4);
        }
    }

    if (input.IsKeyDown(owner.config.keys[16]))
    {
        this->running =
            RoleControl::kRunToggle[static_cast<std::size_t>(this->running) % RoleControl::kRunToggle.size()];
        SleepMilliseconds(0xC8);
    }

    const bool still = owner.unk4A9488 == 0 && owner.unk4A948C == 0 && owner.unk4B7518 == 0 && owner.unk4B7920 == 0;
    owner.talk.advance = 0;

    if (owner.unk4CAD24 == 0)
    {
        owner.counterAlpha -= 2;

        if (owner.counterAlpha < 0)
        {
            owner.counterAlpha = 0;
        }
    }

    if (controls.mouseSuppressed != 0 || !still || owner.menuState != 0)
    {
        return;
    }

    const auto screenX = static_cast<u32>(player.x - field.cameraLeft);
    const auto screenY = static_cast<u32>(player.y - field.cameraTop);
    this->scrollLeft = field.cameraLeft > 0 && screenX <= 0x130 ? 1 : 0;
    this->scrollRight = screenX >= 0x130 && field.cameraRight < 0x10 * state.map.width ? 1 : 0;
    this->scrollUp = field.cameraTop > 0 && screenY <= 0xF0 ? 1 : 0;
    this->scrollDown = screenY >= 0xF0 && field.cameraBottom < 0x10 * state.map.height ? 1 : 0;
    this->stepLeft = player.x != 0 ? 1 : 0;
    this->stepRight = screenX < 0x258 ? 1 : 0;
    this->stepUp = player.y != 0 ? 1 : 0;
    this->stepDown = screenY < 0x1C2 ? 1 : 0;
    const bool choices = !state.clickRegions.regions.empty();
    const ButtonState& space = controls.At(Button::Space);
    const ButtonState& enter = controls.At(Button::Enter);
    const bool spaceDown = space.press != 0 && space.heldFrames == 1;
    const bool enterDown = enter.press != 0 && enter.heldFrames == 1;

    if (!choices && spaceDown)
    {
        owner.talk.advance = 1;
    }

    if (view.talkWindows != nullptr && enterDown)
    {
        ++owner.talk.advance;
    }

    if ((spaceDown || enterDown) && choices)
    {
        if ((state.clickRegions.flags & 0x1000) != 0)
        {
            state.clickRegions.Free();
        }

        return;
    }

    if (spaceDown && leader.id == 0xFFFF)
    {
        const auto facing = static_cast<std::size_t>(player.act.direction) & 7;
        const u32 found =
            this->FindRoleAhead(state.playerRole, RoleControl::kAheadX[facing], RoleControl::kAheadY[facing]);

        if (found < GameState::kRoles && state.roles[found].unk26 == 0)
        {
            this->Approach(player, state.roles[found], false);
            owner.talk.advance = 0;
            return;
        }
    }

    if (enterDown && leader.id == 0xFFFF)
    {
        owner.menuState = 0x80000001;
        return;
    }

    if (view.unk4A9920 != 0 && !choices)
    {
        return;
    }

    auto changeChoice = [&owner, &state, this](const ButtonState& button, int by)
    {
        if (button.heldFrames == 1 || (button.heldFrames & 7) == 7)
        {
            if (by > 0 && static_cast<u32>(++owner.talk.choice) >= static_cast<u32>(state.clickRegions.Count()))
            {
                owner.talk.choice = 0;
            }

            if (by < 0 && --owner.talk.choice < 0)
            {
                owner.talk.choice = state.clickRegions.Count() - 1;
            }

            this->choiceMoved = 1;
        }
    };

    bool across = false;
    const ButtonState& left = controls.At(Button::Left);
    const ButtonState& right = controls.At(Button::Right);
    const ButtonState& up = controls.At(Button::Up);
    const ButtonState& down = controls.At(Button::Down);

    if (left.press > 0)
    {
        pressed = left.press;

        if (!choices)
        {
            player.act.direction = 2;
            across = true;
            stepX = -1;
        }
        else
        {
            changeChoice(left, -1);
        }
    }
    else if (right.press > 0)
    {
        pressed = right.press;

        if (!choices)
        {
            player.act.direction = 3;
            across = true;
            stepX = 1;
        }
        else
        {
            changeChoice(right, 1);
        }
    }

    if (up.press > 0)
    {
        pressed |= up.press;

        if (!choices)
        {
            if (across)
            {
                player.act.direction = player.act.direction == 2   ? 4
                                       : player.act.direction == 3 ? 7
                                                                   : player.act.direction;
            }
            else
            {
                player.act.direction = 0;
            }

            stepY = -1;
        }
        else
        {
            changeChoice(up, -1);
        }
    }
    else if (down.press > 0)
    {
        pressed |= down.press;

        if (!choices)
        {
            if (across)
            {
                player.act.direction = player.act.direction == 2   ? 6
                                       : player.act.direction == 3 ? 5
                                                                   : player.act.direction;
            }
            else
            {
                player.act.direction = 1;
            }

            stepY = 1;
        }
        else
        {
            changeChoice(down, 1);
        }
    }

    const int wantedX = stepX;
    const int wantedY = stepY;

    if (!state.GameFlag(story_flag::kNoSlide) && (stepX != 0 || stepY != 0))
    {
        this->Slide(player, stepX, stepY);
    }

    u32 found = 0xFFFFFFFF;
    u32 target = 0xFFFFFFFF;
    u32 event = 0;

    if (stepX != 0 || stepY != 0)
    {
        event = this->StepEvent(state.playerRole, stepX, stepY, found);
        target = found;
    }

    if (event == 0 && (wantedX != 0 || wantedY != 0))
    {
        event = this->StepEvent(state.playerRole, wantedX, wantedY, found);
        target = found;
    }

    if (target != 0xFFFFFFFF && target < GameState::kRoles)
    {
        event = state.roles[target].event;
    }

    if (event != 0)
    {
        const MapRegion* region = nullptr;

        if (target == 0xFFFFFFFF)
        {
            region = owner.field.FindRegion(static_cast<int>(event));

            if (region == nullptr)
            {
                DebugLog(u8"錯誤的劇情點(%d)", event);
            }
            else if (state.GameFlag(static_cast<int>(static_cast<u32>(region->flag) >> 16)))
            {
                stepY = 0;
                stepX = 0;
            }

            target = found;
        }

        if (leader.id == 0xFFFF)
        {
            if (target == 0xFFFFFFFF)
            {
                if (region != nullptr)
                {
                    const auto facing = static_cast<std::size_t>(player.act.direction) & 7;
                    leader.unk14 = 0;
                    leader.unk20 = 0;
                    leader.event = static_cast<u16>(region->event);
                    leader.id = TalkWindow::kNoRole;
                    leader.flags = 0;
                    leader.x = player.x - 0x10 * RolePath::kStepX[facing];
                    leader.y = player.y - 0x10 * RolePath::kStepY[facing];
                    DebugLog(u8"map region event", region->event);
                }
            }
            else if (found < GameState::kRoles)
            {
                this->Approach(player, state.roles[found], true);
                owner.talk.advance = 0;
            }
        }
    }

    if (stepX != 0 || stepY != 0)
    {
        owner.unk4CAD24 = 0;

        if (stepX == -1)
        {
            if (this->scrollLeft != 0)
            {
                owner.unk4B7518 = -1;
            }
            else if (this->stepLeft != 0)
            {
                owner.unk4A9488 = -1;
            }
        }
        else if (stepX != 0)
        {
            if (this->scrollRight != 0)
            {
                owner.unk4B7518 = 1;
            }
            else if (this->stepRight != 0)
            {
                owner.unk4A9488 = 1;
            }
        }

        if (stepY == -1)
        {
            if (this->scrollUp != 0)
            {
                owner.unk4B7920 = -1;
            }
            else if (this->stepUp != 0)
            {
                owner.unk4A948C = -1;
            }
        }
        else if (stepY != 0)
        {
            if (this->scrollDown != 0)
            {
                owner.unk4B7920 = 1;
            }
            else if (this->stepDown != 0)
            {
                owner.unk4A948C = 1;
            }
        }

        if (player.act.action != 0x10)
        {
            player.act.action = 8;
        }

        const int doubled = pressed & 2;

        if (doubled != 0 || this->running != 0)
        {
            player.act.action = 0x10;
        }

        if (player.act.direction != player.act.lastDirection)
        {
            player.act.delay = 0;
        }

        if (player.act.action != player.act.lastAction)
        {
            player.act.delay = 0;
            int speed = view.unk4C8BD8;

            if (this->running != 0 || doubled != 0)
            {
                speed = 2 * view.unk4C8BD8;
            }

            if (owner.talk.skipAll != 0)
            {
                speed = 0x10;
            }

            state.SetWalkSpeed(speed);
        }

        if (!state.GameFlag(owner.unk4A9A10 + story_flag::kMapNoEncounter))
        {
            ++view.unk4C9A20;

            if (owner.inBattle == 0 && state.GameFlag(story_flag::kEncounters) && owner.unk4A94AC == 0 &&
                leader.id == 0xFFFF)
            {
                owner.unk4A94AC = static_cast<i32>(owner.RollEncounter(view.unk4C9A20, 0));

                if (owner.unk4A94AC != 0)
                {
                    owner.audio.StopMusic();
                    owner.audio.StopAllSounds();
                    owner.acts.CloseAll();
                    owner.archive.CloseAll();
                    owner.StartBattle(static_cast<u16>(owner.unk4A94AC));
                    owner.inBattle = 1;
                    owner.unk4B7920 = 0;
                    owner.unk4B7518 = 0;
                    owner.unk4A948C = 0;
                    owner.unk4A9488 = 0;

                    if (owner.mapCache.unitFile.UnmapView(owner.mapCache.view))
                    {
                        owner.mapCache.view = nullptr;
                    }
                    else
                    {
                        ErrorLog(u8"Fight by map ,CloseView fail ,%s", owner.mapCache.unitFile.ErrorText());
                    }

                    owner.mapCache.unitFile.CloseMapping();

                    for (int index = 1; index < state.roleCount && index < GameState::kRoles; ++index)
                    {
                        state.roles[static_cast<std::size_t>(index)].flags &= ~role_flag::kSoundOnce;
                    }
                }
            }
        }

        owner.unk4A94AC = 0;
    }
    else
    {
        if (player.act.action != player.act.lastAction)
        {
            player.act.delay = 0;
        }

        player.act.action = 0;

        if (this->idleFrames > 0x80)
        {
            player.act.action = 0x34;
        }

        if ((++owner.unk4CAD24 > 0x64 || (owner.counterAlpha & 0xF) != 0) && ++owner.counterAlpha > 0x10)
        {
            owner.counterAlpha = 0x10;
        }
    }

    ++this->idleFrames;

    if (player.act.action != 0 && player.act.action != 0x34)
    {
        this->idleFrames = 0;
    }

    ++this->unk4CB220;
}

}  // namespace swd3
