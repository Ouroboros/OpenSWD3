#include <algorithm>
#include <array>

#include <SDL3/SDL.h>

#include "core/debug_log.h"
#include "core/words.h"
#include "game/cheat_dialog.h"
#include "game/game.h"
#include "input/input.h"
#include "item/item_list.h"
#include "platform/system_info.h"
#include "role/role_control.h"

namespace swd3
{

namespace
{

constexpr u32 kNone = 0xFFFFFFFF;
constexpr int kHalfTile = MapHeader::TileSize / 2;
constexpr std::size_t kDirectionMask = RolePath::Directions - 1;
constexpr i32 kRepeatMask = 7;       // a held arrow moves the choice every 8 frames
constexpr int kDebugKeyPause = 500;  // milliseconds slept after a debug key
constexpr int kSkipKeyPause = 350;
constexpr int kToggleKeyPause = 300;
constexpr int kGoldKeyPause = 250;
constexpr int kRunKeyPause = 200;
constexpr int kToggleStates = 2;
constexpr i32 kMaxMapSpeed = 0x40;
constexpr i32 kDebugGold = 1000;
constexpr u16 kFirstDebugItem = 0x65;  // F6 gives kDebugItemCount of the objects kFirstDebugItem..kDebugItemsEnd-1
constexpr u16 kDebugItemsEnd = 0x4B0;
constexpr u16 kDebugItemCount = 0xA;
constexpr u16 kFirstSkippedKind = 0xA;  // object kinds given besides those below kFirstSkippedKind
constexpr std::array<u16, 8> kDebugKinds = {0x12, 0x13, 0x11, 0x10, 0xF, 0x14, 0x15, 0x18};
constexpr i32 kCounterFade = 2;
constexpr i32 kCounterDelay = 0x64;  // standing frames before the counter fades in
constexpr i32 kCounterOpaque = 0x10;
constexpr u32 kScrollX = 0x130;  // the camera follows the controlled role past the middle of the screen
constexpr u32 kScrollY = 0xF0;
constexpr u32 kStepRightLimit = 0x258;
constexpr u32 kStepDownLimit = 0x1C2;
constexpr i32 kRegionLeaderSteps = 0x10;  // a region event is placed this many steps behind the controlled role
constexpr i32 kDoublePress = 2;           // ButtonState::press of a second click
constexpr int kRunSpeedFactor = 2;
constexpr int kSkipSpeed = 0x10;
constexpr i32 kIdlePoseFrames = 0x80;

}  // namespace

// The role faced turned to look back at the controlled role, the controlled role turned towards it, and the event of
// the role taken by the leader. (When the controlled role walks into the role, the original runs the role's act a
// second time where the controlled role's act would be run.)
void RoleControl::Approach(Role& player, Role& other, bool walking)
{
    Game& game = this->game;
    const int facing =
        game.state.DirectionToPlayer(other.x + kHalfTile * other.act.width, other.y + kHalfTile * other.act.height);

    if ((other.flags & Role::FacesTalker) != 0)
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
    player.act.direction = (facing & ~1) + ((static_cast<u8>(facing) - 1) & 1);
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

    if (owner.config.debugMode == 1 && input.IsKeyDown(Input::RightControl) && input.IsKeyDown(Input::RightShift) &&
        input.IsKeyDown(Input::Home) && input.IsKeyDown(Input::End))
    {
        state.SetGameFlag(GameState::DebugKeys);
    }

    if (state.GameFlag(GameState::DebugKeys))
    {
        const bool idle = leader.id == Role::NoId && view.talkWindows == nullptr;

        if (input.IsKeyDown(Input::F12) && idle)
        {
            SleepMilliseconds(kDebugKeyPause);
            owner.storyState = Game::SaveSlotsState;
            owner.menuMode = Menu::KeySaveMode;
            owner.unk4CAEBC = 0;
            owner.unk4CAEC0 = 0;
            owner.ResetSaveSlots();
            return;
        }

        if (input.IsKeyDown(Input::LeftControl) || input.IsKeyDown(Input::RightControl))
        {
            if (input.IsKeyDown(Input::F9))
            {
                owner.talk.skipAll = (owner.talk.skipAll + 1) % kToggleStates;
                SleepMilliseconds(kSkipKeyPause);
                return;
            }

            if (input.IsKeyDown(Input::Q))
            {
                owner.inactive = 1;
                state.ClearFrameInterval();
                owner.systemFlags |= Game::QuitRequested;
                state.ClearGameFlag(GameState::Running);
                return;
            }

            if (input.IsKeyDown(Input::S) && idle)
            {
                owner.unk4CAEBC = 1;
                owner.unk4CAEC0 = 0;
                SleepMilliseconds(kDebugKeyPause);
                owner.menuMode = Menu::KeySaveMode;
                owner.storyState = Game::SaveSlotsState;
                owner.ResetSaveSlots();
                return;
            }

            if (input.IsKeyDown(Input::L) && idle)
            {
                owner.unk4CAEBC = 1;
                owner.unk4CAEC0 = 1;
                SleepMilliseconds(kDebugKeyPause);
                owner.menuMode = Menu::KeyLoadMode;
                owner.storyState = Game::SaveSlotsState;
                owner.ResetSaveSlots();
                return;
            }

            if (input.IsKeyDown(Input::F1) || input.IsKeyDown(Input::F2))
            {
                i32& shown = input.IsKeyDown(Input::F1) ? owner.debugText : owner.debugCells;
                shown = shown == 0 ? 1 : 0;
                this->unk4CB220 = 0;
                SleepMilliseconds(kToggleKeyPause);
                return;
            }

            if (input.IsKeyDown(Input::F3))
            {
                state.SetGameFlagTo(GameState::NoSlide, state.GameFlag(GameState::NoSlide) ? 0 : 1);
                this->unk4CB220 = 0;
                SleepMilliseconds(kToggleKeyPause);
                return;
            }

            if (input.IsKeyDown(Input::Tab) && input.IsKeyDown(Input::NumpadPlus))
            {
                if (++view.unk49E0BC > kMaxMapSpeed)
                {
                    view.unk49E0BC = kMaxMapSpeed;
                }

                this->unk4CB220 = 0;
            }

            if (input.IsKeyDown(Input::Tab) && input.IsKeyDown(Input::NumpadMinus))
            {
                if (--view.unk49E0BC < 1)
                {
                    view.unk49E0BC = 1;
                }

                this->unk4CB220 = 0;
            }
        }

        if (input.IsKeyDown(Input::F4))
        {
            owner.running = 0;
            SDL_ShowCursor();
            CheatDialog dialog(owner);

            if (dialog.Run() == 0)
            {
                owner.running = 1;
            }

            return;
        }

        if (input.IsKeyDown(Input::F7))
        {
            owner.party.variables[Party::Gold] += kDebugGold;
            SleepMilliseconds(kGoldKeyPause);
        }

        if (input.IsKeyDown(Input::F6))
        {
            for (u16 id = kFirstDebugItem; id < kDebugItemsEnd; ++id)
            {
                ObjectRecord record;

                if (owner.database.LoadObject(record, id))
                {
                    const u16 kind = record.data.unk52;

                    if (kind < kFirstSkippedKind || std::ranges::find(kDebugKinds, kind) != kDebugKinds.end())
                    {
                        ItemLink& list =
                            owner.party.inventory != nullptr ? owner.party.inventory->next : owner.party.inventory;
                        AddItem(list, id, kDebugItemCount, ItemCountMode::Pending, owner.database);
                    }
                }
            }

            SleepMilliseconds(kDebugKeyPause);
        }
    }

    if (input.IsKeyDown(owner.config.keys[static_cast<std::size_t>(Button::KeyR)]))
    {
        this->running = RoleControl::RunToggle[static_cast<std::size_t>(this->running) % RoleControl::RunToggle.size()];
        SleepMilliseconds(kRunKeyPause);
    }

    const bool still = owner.unk4A9488 == 0 && owner.unk4A948C == 0 && owner.unk4B7518 == 0 && owner.unk4B7920 == 0;
    owner.talk.advance = 0;

    if (owner.unk4CAD24 == 0)
    {
        owner.counterAlpha -= kCounterFade;

        if (owner.counterAlpha < 0)
        {
            owner.counterAlpha = 0;
        }
    }

    if (controls.mouseSuppressed != 0 || !still || owner.menuState != Game::NoMenu)
    {
        return;
    }

    const auto screenX = static_cast<u32>(player.x - field.cameraLeft);
    const auto screenY = static_cast<u32>(player.y - field.cameraTop);
    this->scrollLeft = field.cameraLeft > 0 && screenX <= kScrollX ? 1 : 0;
    this->scrollRight = screenX >= kScrollX && field.cameraRight < MapHeader::TileSize * state.map.width ? 1 : 0;
    this->scrollUp = field.cameraTop > 0 && screenY <= kScrollY ? 1 : 0;
    this->scrollDown = screenY >= kScrollY && field.cameraBottom < MapHeader::TileSize * state.map.height ? 1 : 0;
    this->stepLeft = player.x != 0 ? 1 : 0;
    this->stepRight = screenX < kStepRightLimit ? 1 : 0;
    this->stepUp = player.y != 0 ? 1 : 0;
    this->stepDown = screenY < kStepDownLimit ? 1 : 0;
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
        if ((state.clickRegions.flags & ClickRegions::TalkChoices) != 0)
        {
            state.clickRegions.Free();
        }

        return;
    }

    if (spaceDown && leader.id == Role::NoId)
    {
        const auto facing = static_cast<std::size_t>(player.act.direction) & kDirectionMask;
        const u32 found =
            this->FindRoleAhead(state.playerRole, RoleControl::AheadX[facing], RoleControl::AheadY[facing]);

        if (found < GameState::Roles && state.roles[found].talkState == Role::NoTalk)
        {
            this->Approach(player, state.roles[found], false);
            owner.talk.advance = 0;
            return;
        }
    }

    if (enterDown && leader.id == Role::NoId)
    {
        owner.menuState = Game::MenuEntering | Game::MainMenu;
        return;
    }

    if (view.playerHold != 0 && !choices)
    {
        return;
    }

    auto changeChoice = [&owner, &state, this](const ButtonState& button, int by)
    {
        if (button.heldFrames == 1 || (button.heldFrames & kRepeatMask) == kRepeatMask)
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
            player.act.direction = ActState::Left;
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
            player.act.direction = ActState::Right;
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
                player.act.direction = player.act.direction == ActState::Left    ? ActState::UpLeft
                                       : player.act.direction == ActState::Right ? ActState::UpRight
                                                                                 : player.act.direction;
            }
            else
            {
                player.act.direction = ActState::Up;
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
                player.act.direction = player.act.direction == ActState::Left    ? ActState::DownLeft
                                       : player.act.direction == ActState::Right ? ActState::DownRight
                                                                                 : player.act.direction;
            }
            else
            {
                player.act.direction = ActState::Down;
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

    if (!state.GameFlag(GameState::NoSlide) && (stepX != 0 || stepY != 0))
    {
        this->Slide(player, stepX, stepY);
    }

    u32 found = kNone;
    u32 target = kNone;
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

    if (target != kNone && target < GameState::Roles)
    {
        event = state.roles[target].event;
    }

    if (event != 0)
    {
        const MapRegion* region = nullptr;

        if (target == kNone)
        {
            region = owner.field.FindRegion(static_cast<int>(event));

            if (region == nullptr)
            {
                DebugLog(u8"錯誤的劇情點(%d)", event);
            }
            else if (state.GameFlag(static_cast<int>(HighWord(static_cast<u32>(region->flag)))))
            {
                stepY = 0;
                stepX = 0;
            }

            target = found;
        }

        if (leader.id == Role::NoId)
        {
            if (target == kNone)
            {
                if (region != nullptr)
                {
                    const auto facing = static_cast<std::size_t>(player.act.direction) & kDirectionMask;
                    leader.unk14 = 0;
                    leader.unk20 = 0;
                    leader.event = static_cast<u16>(region->event);
                    leader.id = TalkWindow::NoRole;
                    leader.flags = 0;
                    leader.x = player.x - kRegionLeaderSteps * RolePath::StepX[facing];
                    leader.y = player.y - kRegionLeaderSteps * RolePath::StepY[facing];
                    DebugLog(u8"map region event", region->event);
                }
            }
            else if (found < GameState::Roles)
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

        if (player.act.action != Role::RunAction)
        {
            player.act.action = Role::WalkAction;
        }

        const int doubled = pressed & kDoublePress;

        if (doubled != 0 || this->running != 0)
        {
            player.act.action = Role::RunAction;
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
                speed = kRunSpeedFactor * view.unk4C8BD8;
            }

            if (owner.talk.skipAll != 0)
            {
                speed = kSkipSpeed;
            }

            state.SetWalkSpeed(speed);
        }

        if (!state.GameFlag(owner.currentMap + GameState::MapNoEncounter))
        {
            ++view.encounterSteps;

            if (owner.inBattle == 0 && state.GameFlag(GameState::Encounters) && owner.battleRequest == 0 &&
                leader.id == Role::NoId)
            {
                owner.battleRequest = static_cast<i32>(owner.RollEncounter(view.encounterSteps, 0));

                if (owner.battleRequest != 0)
                {
                    owner.audio.StopMusic();
                    owner.audio.StopAllSounds();
                    owner.acts.CloseAll();
                    owner.archive.CloseAll();
                    owner.StartBattle(static_cast<u16>(owner.battleRequest));
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

                    for (int index = 1; index < state.roleCount && index < GameState::Roles; ++index)
                    {
                        state.roles[static_cast<std::size_t>(index)].flags &= ~Role::SoundOnce;
                    }
                }
            }
        }

        owner.battleRequest = 0;
    }
    else
    {
        if (player.act.action != player.act.lastAction)
        {
            player.act.delay = 0;
        }

        player.act.action = 0;

        if (this->idleFrames > kIdlePoseFrames)
        {
            player.act.action = Role::IdleAction;
        }

        if ((++owner.unk4CAD24 > kCounterDelay || (owner.counterAlpha & (kCounterOpaque - 1)) != 0) &&
            ++owner.counterAlpha > kCounterOpaque)
        {
            owner.counterAlpha = kCounterOpaque;
        }
    }

    ++this->idleFrames;

    if (player.act.action != 0 && player.act.action != Role::IdleAction)
    {
        this->idleFrames = 0;
    }

    ++this->unk4CB220;
}

}  // namespace swd3
