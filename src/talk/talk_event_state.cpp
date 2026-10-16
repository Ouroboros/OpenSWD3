#include <algorithm>
#include <climits>
#include <cmath>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "game/game.h"
#include "item/item_list.h"
#include "platform/file_system.h"
#include "talk/script_text.h"
#include "talk/talk.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

// The range the spotlight command keeps the spotlight in (screen pixels).
constexpr int kPotEntry = 0x3B;               // the tile EnterDemonPot puts the controlled role on (both ways)
constexpr u16 kCommandStarted = 0x8000;       // GatherParty marks its own command word while the party gathers
constexpr std::size_t kFileNameBytes = 0x33;  // the longest name SetFileNameA/B keeps
constexpr int kSpotlightLeft = 0xD0;
constexpr int kSpotlightRight = 0x1B0;
constexpr int kSpotlightTop = 0xD0;
constexpr int kSpotlightBottom = 0x110;

// The role flag commands 0x66, 0x67, 0x75, 0x88, 0x8C, 0x91, 0x92 and 0xAE set or clear.
u32 RoleFlagOf(EventCommand code)
{
    switch (code)
    {
        case EventCommand::RoleExamine:
            return Role::Examine;

        case EventCommand::RoleNoTalkPointer:
            return Role::NoTalkPointer;

        case EventCommand::RoleUnk10:
            return Role::Unk10;

        case EventCommand::RoleStepsAct:
            return Role::StepsAct;

        case EventCommand::RoleFacesTalker:
            return Role::FacesTalker;

        case EventCommand::RoleTouchEvent:
            return Role::TouchEvent;

        case EventCommand::RoleCellEffects:
            return Role::CellEffects;

        default:
            return Role::Solid;
    }
}

}  // namespace

// Commands 0x9E and 0x9F: a file of the CD's Video\ (mode 0) or Music\ (mode 1) folder copied into the installation
// (0x9E) or deleted from the CD folder (0x9F). The name runs to "%Q"; the mode is the second word after it, and the
// script goes on with the word after "%Q".
bool Talk::CopyMedia(EventRun& run)
{
    Game& owner = this->game;
    constexpr u16 kVideoFolder = 0;
    constexpr u16 kMusicFolder = 1;
    const std::vector<u8>& script = owner.databases.script;
    const std::size_t start = run.cursor + kOpcodeBytes + kWordBytes;
    const std::size_t end = TextEndAt(script, start);
    const std::string name = start < end ? std::string(script.begin() + start, script.begin() + end) : std::string();
    run.valueA = static_cast<i32>(name.size());
    owner.inactive = 1;
    run.cursor = end + kWordBytes;
    this->SyncOffset(run);
    const u16 mode = TextWord(script, end + 2 * kWordBytes);
    run.valueB = static_cast<i16>(mode);
    std::string source = owner.config.cdRoot;
    std::string target = owner.config.installDirectory;

    if (mode == kVideoFolder)
    {
        source += "Video\\";
        target += "Video\\";
    }
    else if (mode == kMusicFolder)
    {
        source += "Music\\";
        target += "Music\\";
    }

    source += name;
    target += name;

    if (run.code == EventCommand::CopyMedia)
    {
        CopyGameFile(source, target);
    }
    else
    {
        DeleteGameFile(source);
    }

    return true;
}

// Commands 0x64..0x8F: the roles' events and flags, the talk window settings, waits for windows and animations, the
// music, the talk log, the items and skills, the party's state and the timers.
bool Talk::RunStateCommand(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Party& party = owner.party;
    const auto leader = static_cast<u16>(owner.unk4B7BC4);

    switch (run.code)
    {
        case EventCommand::SetEvent:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            const u16 event = this->Word(run, 2);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                state.roles[run.found].event = event;
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(event),
                    Game::KeepValue,
                    0,
                    Game::KeepValue,
                    Game::KeepValue
                );
            }

            this->Next(run);
            return true;
        }

        case EventCommand::SetRoleFastMove:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                state.roles[run.found].flags |= Role::FastMove;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::RoleExamine:
        case EventCommand::RoleNoTalkPointer:
        case EventCommand::RoleUnk10:
        case EventCommand::RoleStepsAct:
        case EventCommand::RoleFacesTalker:
        case EventCommand::RoleTouchEvent:
        case EventCommand::RoleCellEffects:
        case EventCommand::RoleSolid:
        {
            const u32 bit = RoleFlagOf(run.code);
            const u16 operand = this->Word(run, 1);
            const u16 id = operand == Talk::Current ? static_cast<u16>(state.playerRole) : operand;
            const bool set = this->Word(run, 2) != 0;

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                role.flags &= ~bit;

                if (set)
                {
                    role.flags |= bit;
                }

                owner.field.ClearRoleCells(role, leader);
                owner.field.MarkRoleCells(role, leader);
            }
            else
            {
                DebugLog(u8"set role flag", id);
                run.valueA = set ? static_cast<i32>(bit) : 0;
                run.valueB = set ? -1 : static_cast<i32>(~bit);
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(set ? bit : 0),
                    static_cast<i16>(set ? ~0u : ~bit),
                    Game::KeepValue
                );
            }

            this->Next(run);
            return true;
        }

        case EventCommand::DialogOffset:
        {
            const auto x = static_cast<i16>(this->Word(run, 1));
            const auto y = static_cast<i16>(this->Word(run, 2));
            this->dialogOptions &= ~Talk::Offset;
            this->dialogOffsetX = x;
            this->dialogOffsetY = y;
            this->Next(run);
            return true;
        }

        case EventCommand::DialogSingleLine:
            this->dialogOptions &= ~Talk::SingleLine;
            this->Next(run);
            return true;

        case EventCommand::WaitAnimationA:
        case EventCommand::WaitAnimationB:
        {
            const u16 rounds = this->Word(run, 1);
            const ScriptAnimation* first =
                run.code == EventCommand::WaitAnimationB ? view.animationsB.get() : view.animationsA.get();

            if (first == nullptr || (first->act.counter >> ActState::RoundShift) > rounds)
            {
                this->Next(run);
                return true;
            }

            return false;
        }

        case EventCommand::WaitActRounds:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            const u16 wanted = this->Word(run, 2);

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog(u8"Talk(tilpa1)", wanted);
                this->Next(run);
                return true;
            }

            const u16 rounds = state.roles[run.found].act.counter;

            if ((rounds & ActState::RoundsMask) < wanted)
            {
                DebugLog(u8"Talk(tilpa1)", wanted, rounds & ActState::RoundsMask);
                this->Next(run);
                return true;
            }

            if ((rounds >> ActState::RoundShift) >= wanted)
            {
                this->Next(run);
                return true;
            }

            return false;
        }

        case EventCommand::DialogOrigin:
        {
            u32 x = this->Word(run, 1);
            u32 y = this->Word(run, 2);

            if (x > Display::Width - 1)
            {
                x = Talk::OriginEdge;
            }

            if (y > Display::Height - 1)
            {
                y = Talk::OriginEdge;
            }

            this->dialogOrigin = x | y << Talk::OriginYShift;
            this->Next(run);
            return true;
        }

        case EventCommand::StepRoles:
        {
            constexpr std::size_t kRolesAt = 2;  // the roles follow the count
            const u16 count = this->Word(run, 1);

            for (u16 index = 0; index < count; ++index)
            {
                if (state.FindRoleOrPlayer(this->Word(run, kRolesAt + index), run.found))
                {
                    owner.rolePath.ScriptStep(static_cast<int>(run.found));
                }
            }

            this->Advance(run, static_cast<int>(kWordBytes * (kRolesAt + count)));
            return false;
        }

        case EventCommand::JumpIfNoneMoving:
        case EventCommand::JumpIfAnyMoving:
        {
            bool moving = false;

            for (int index = 1; index < state.roleCount && index < GameState::Roles; ++index)
            {
                if ((state.roles[static_cast<std::size_t>(index)].flags & Role::Stepping) != 0)
                {
                    moving = true;
                    break;
                }
            }

            if (run.code == EventCommand::JumpIfAnyMoving)
            {
                moving = !moving;
            }

            if (moving)
            {
                this->Next(run);
            }
            else
            {
                this->Jump(run, this->Dword(run, 2));
            }

            return true;
        }

        case EventCommand::WaitEffects:
            if (view.effects != nullptr)
            {
                return false;
            }

            if (view.spritesB == nullptr)
            {
                this->Next(run);
            }

            return false;

        case EventCommand::SoundLong:
            owner.audio.PlaySound(this->Word(run, 1), owner.config.soundVolume);
            this->Next(run);
            return false;

        case EventCommand::ChangeMusic:
        {
            const u16 first = this->Word(run, 1);
            const u16 second = this->Word(run, 2);
            const u16 marks = this->Word(run, 3);
            auto& tunes = view.musicTunes;
            tunes[Scene::StoryFade] = static_cast<i32>(Scene::FadePending | AudioManager::StopAtOnce);
            tunes[Scene::StoryFirst] = first;
            tunes[Scene::StorySecond] = second;

            if (owner.audio.FadeMode() == AudioManager::NoFade)
            {
                owner.audio.SetFadeMode(AudioManager::StopAtOnce);
            }

            owner.audio.UpdateMusicFade();
            DebugLog(u8"StreamStartOut");
            view.musicCue |= Scene::StoryMusic;
            run.valueA = marks;
            u32 cue = view.musicCue & ~(Scene::StoryLoops | Scene::StoryRepeatFirst);

            if ((marks & Scene::MarksNone) == 0)
            {
                if ((marks & Scene::MarkRepeatFirst) != 0)
                {
                    cue |= Scene::StoryLoops | Scene::StoryRepeatFirst;
                }

                if ((marks & Scene::MarkLoops) != 0)
                {
                    cue |= Scene::StoryLoops;
                }
            }

            view.musicCue = cue & ~Scene::TrackByte;
            this->Next(run);
            DebugLog(u8"S_M[%d,%d]", tunes[Scene::StoryFirst], tunes[Scene::StorySecond]);
            return true;
        }

        case EventCommand::MusicVolume:
        {
            const u16 volume = this->Word(run, 1);
            run.valueA = volume <= Config::MaxVolume ? volume : Config::MaxVolume;
            owner.audio.SetMusicVolume(run.valueA);
            this->Next(run);
            return false;
        }

        case EventCommand::PutRoles:
        {
            constexpr std::size_t kRolesAt = 2;     // the entries follow the count
            constexpr std::size_t kEntryWords = 3;  // a role, a column and a row
            const u16 count = this->Word(run, 1);
            run.valueA = 0;

            for (u16 index = 0; index < count; ++index)
            {
                const std::size_t entry = kRolesAt + kEntryWords * index;
                const u16 id = this->EventRoleId(run, this->Word(run, entry));
                const u16 column = this->Word(run, entry + 1);
                const u16 row = this->Word(run, entry + 2);
                state.FindRoleOrPlayer(id, run.found);

                if (run.found < GameState::Roles)
                {
                    owner.rolePath.MoveRoleTo(
                        static_cast<int>(run.found),
                        static_cast<u16>(MapHeader::TileSize * column),
                        static_cast<u16>(MapHeader::TileSize * row),
                        RolePath::WalkThere,
                        RolePath::Keep,
                        RolePath::Keep,
                        RolePath::Keep
                    );
                }

                if (static_cast<i32>(run.found) == state.playerRole)
                {
                    view.playerHold |= Scene::HoldLocked;
                }
            }

            this->Advance(run, static_cast<int>(kWordBytes * (kRolesAt + kEntryWords * count)));
            return true;
        }

        case EventCommand::CloseDialog:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            std::unique_ptr<TalkWindow>* link = &view.talkWindows;

            while (*link != nullptr)
            {
                TalkWindow& window = **link;

                if (owner.rolePath.RoleId(window.role) != id)
                {
                    link = &window.next;
                    continue;
                }

                if (window.role != TalkWindow::NoRole && window.role < GameState::Roles)
                {
                    state.roles[window.role].talkState = Role::NoTalk;
                }

                std::unique_ptr<TalkWindow> gone = std::move(*link);
                *link = std::move(gone->next);
                run.valueA = view.playerHold & Scene::HoldLocked;
                run.valueB = std::max((view.playerHold & Scene::HoldCountMask) - 1, 0);
                view.playerHold = run.valueA | run.valueB;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::WaitDialogRead:
        case EventCommand::WaitDialogTyped:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            run.found = TalkWindow::NoRole;

            if (id != TalkWindow::NoRole && !state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog(u8"Talk(wait_dlg2)", id, owner.currentMap);
                this->Next(run);
                return true;
            }

            const TalkWindow* window = view.talkWindows.get();

            while (window != nullptr && static_cast<u32>(window->role) != run.found)
            {
                window = window->next.get();
            }

            if (window == nullptr)
            {
                this->Next(run);
                return true;
            }

            const u32 done = run.code == EventCommand::WaitDialogRead ? window->flags & TalkWindow::Read
                                                                      : window->flags & TalkWindow::Typed;

            if (done != 0)
            {
                this->Next(run);
                return true;
            }

            return false;
        }

        case EventCommand::SetActData:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            const u16 act = this->Word(run, 2);
            const u16 action = this->Word(run, 3);
            const u16 direction = this->Word(run, 4);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (act != Talk::Keep)
                {
                    role.act.id = static_cast<i16>(act);
                }

                if (action != Talk::Keep)
                {
                    role.act.action = static_cast<i16>(action);
                }

                if (direction != Talk::Keep)
                {
                    role.act.direction = direction;
                }

                role.act.delay = 0;

                if (owner.acts.Run(role.act) == 0)
                {
                    LogActError(u8"TakeActData(Talk,TakeActData)", role);
                }

                role.flags |= Role::StepsAct;
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    static_cast<i16>(act),
                    static_cast<i16>(action),
                    static_cast<i16>(direction),
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Role::StepsAct,
                    Game::KeepValue,
                    Game::KeepValue
                );
            }

            this->Next(run);
            return true;
        }

        case EventCommand::DialogBubble:
            this->dialogOptions &= ~Talk::ShowMark;
            this->Next(run);
            return true;

        case EventCommand::StopSkipping:
            this->skipAll = 0;
            this->Next(run);
            return true;

        case EventCommand::SceneMusic:
        {
            const u16 operand = this->Word(run, 1);
            const u16 map = operand == Talk::Current ? static_cast<u16>(owner.currentMap) : operand;
            std::vector<u8>& data = owner.databases.mapsData;
            std::size_t at = MapsDword(
                data,
                MapsDword(data, MapsTable(data, MapsDataTable::Music) + sizeof(u32) * MapsRecord::SceneMusicItem)
            );
            run.valueA = 0;

            while (MapsWord(data, at) != 0 && at + 1 < data.size() && MapsWord(data, at) != map)
            {
                at += MapsRecord::MusicBytes;
            }

            constexpr std::size_t kRecordWords = 3;  // the map and its two tunes; the marks stay

            if (MapsWord(data, at) != 0 && at + MapsRecord::MusicMarksAt - 1 < data.size())
            {
                for (std::size_t index = 0; index < kRecordWords; ++index)
                {
                    const u16 value = this->Word(run, 1 + index);
                    data[at + sizeof(u16) * index] = static_cast<u8>(value);
                    data[at + sizeof(u16) * index + 1] = static_cast<u8>(value >> CHAR_BIT);
                }
            }

            if (MapsWord(data, at) == 0)
            {
                DebugLog(u8"TalkOp:Scene_Music", operand, this->Word(run, 2), this->Word(run, 3), this->Word(run, 4));
            }

            this->Next(run);
            return true;
        }

        case EventCommand::DialogLogged:
            this->dialogOptions &= ~Talk::Logged;
            this->Next(run);
            return true;

        case EventCommand::AddLog:
        {
            constexpr std::size_t kLogLineBytes = 0xFD;  // a log line before its "%Q"
            const std::vector<u8>& script = owner.databases.script;
            const std::size_t start = run.cursor + kOpcodeBytes;
            const std::size_t end = TextEndAt(script, start);
            std::string text = start < end ? std::string(script.begin() + start, script.begin() + end) : std::string();
            run.valueA = static_cast<i32>(text.size());

            if (text.size() > kLogLineBytes)
            {
                text.resize(kLogLineBytes);
            }

            this->history.push_back(text + "%Q");
            run.cursor = end + kWordBytes;
            this->SyncOffset(run);
            return false;
        }

        case EventCommand::JumpIfActionNot:
        case EventCommand::JumpIfAction:
        {
            const u16 operand = this->Word(run, 1);
            const u16 id = operand == Talk::Current ? static_cast<u16>(state.playerRole) : operand;
            const u16 action = this->Word(run, 2);
            const u32 target = this->Dword(run, 6);
            Role loaded;
            const Role* role = &loaded;

            if (state.FindRoleOrPlayer(id, run.found))
            {
                role = &state.roles[run.found];
            }
            else
            {
                owner.LoadRoleRecord(loaded, id);
            }

            run.valueA = role->act.action;

            if ((run.code == EventCommand::JumpIfAction) != (run.valueA == action))
            {
                this->Jump(run, target);
            }
            else
            {
                this->Next(run);
            }

            return true;
        }

        case EventCommand::AddItem:
        {
            const u16 id = this->Word(run, 1);
            const auto count = static_cast<i16>(this->Word(run, 2));
            const ItemNode* item = AddItem(party.inventory, id, count, ItemCountMode::Pending, owner.database);
            DebugLog(u8"%s,%d", item != nullptr ? item->id : id, count);
            this->Next(run);
            return false;
        }

        case EventCommand::JumpIfItem:
        case EventCommand::JumpIfNoItem:
        case EventCommand::JumpIfWorn:
        case EventCommand::JumpIfNotWorn:
        {
            const auto id = static_cast<i16>(this->Word(run, 1));
            const ItemNode* carried = FindItemOfKind(party.inventory, id);
            const ItemNode* worn = FindRoleItem(party.equipment, id);
            const bool wanted = run.code == EventCommand::JumpIfNoItem || run.code == EventCommand::JumpIfNotWorn;

            if (run.code == EventCommand::JumpIfWorn || run.code == EventCommand::JumpIfNotWorn)
            {
                carried = nullptr;
            }

            if (wanted == (carried != nullptr || worn != nullptr))
            {
                this->Next(run);
                return true;
            }

            this->Jump(run, this->Dword(run, 4));
            return true;
        }

        case EventCommand::LearnSkill:
        {
            const u16 role = this->Word(run, 1);
            const u16 skill = this->Word(run, 2);
            run.found = role;

            if (role >= Party::Roles)
            {
                DebugLog(u8"skill role", role);
                this->Next(run);
                return true;
            }

            ItemLink& skills = party.skills[role]->next;

            if (FindItemOfKind(skills, static_cast<i16>(skill)) == nullptr)
            {
                ItemNode* learned = AddItem(skills, skill, 1, ItemCountMode::Count, owner.database);

                if (learned != nullptr)
                {
                    learned->record.data.kind &= ~ObjectData::UnkKind8000;

                    if ((learned->record.data.usableBy & (ObjectData::FirstRoleBit >> role)) == 0)
                    {
                        DebugLog(u8"skill not for the role", role, learned->id);
                        AddFlaggedItem(skills, learned->id, -1, 0, owner.database);
                    }
                }
            }

            this->Next(run);
            return true;
        }

        case EventCommand::Equip:
        {
            constexpr u16 kEquipSlots = 0xC;  // of the Party::SlotsPerRole a role has
            const u16 role = this->Word(run, 1);
            const u16 slot = this->Word(run, 2);
            const auto kind = static_cast<i16>(this->Word(run, 3));
            run.found = role;

            if (role >= Party::Roles || slot >= kEquipSlots)
            {
                DebugLog(u8"equipment slot", role, slot);
                this->Next(run);
                return true;
            }

            const ItemNode* item = FindItemOfKind(party.inventory, kind);

            if (item == nullptr)
            {
                DebugLog(u8"item missing", static_cast<u16>(kind));
                this->Next(run);
                return true;
            }

            std::unique_ptr<ItemNode>& worn =
                party.equipment[static_cast<std::size_t>(Party::SlotsPerRole * role + slot)];
            const u16 oldId = worn != nullptr ? worn->id : u16{0};
            const u16 newId = item->id;

            if (worn == nullptr)
            {
                worn = std::make_unique<ItemNode>();
            }

            worn->id = item->id;
            worn->record = item->record;
            worn->count = 1;
            worn->pending = 0;
            worn->unk06 = 0;
            AddItem(party.inventory, oldId, 1, ItemCountMode::Pending, owner.database);
            AddItem(party.inventory, newId, -1, ItemCountMode::Pending, owner.database);
            this->Next(run);
            return true;
        }

        case EventCommand::ItemMenu:
        {
            constexpr std::size_t kItemsAt = 1;  // the items follow the command, ended by 0
            constexpr std::size_t kMaxItems = 0x7F;
            std::size_t count = 0;

            while (this->Word(run, kItemsAt + count) != 0 &&
                   run.cursor + kWordBytes * (kItemsAt + count) + 1 < owner.databases.script.size())
            {
                ++count;
            }

            owner.unk4B7928.clear();

            for (std::size_t index = 0; index < count && index < kMaxItems; ++index)
            {
                owner.unk4B7928.push_back(this->Word(run, kItemsAt + index));
            }

            owner.unk4B7928.push_back(0);

            owner.menuState = Game::MenuEntering | Game::ItemChoiceMenu;
            this->Advance(run, static_cast<int>(kWordBytes * (kItemsAt + count + 1)));
            return false;
        }

        case EventCommand::ChangeVitals:
        {
            const auto index = static_cast<u16>(this->Word(run, 1) - 1);  // the roles count from 1
            const u16 hitPoints = this->Word(run, 2);
            const u16 spirit = this->Word(run, 3);
            const u16 stamina = this->Word(run, 4);

            if (index >= Party::Roles)
            {
                DebugLog(u8"party role", index);
                this->Next(run);
                return true;
            }

            RoleRecord& record = party.roles[index];
            record.hitPoints = static_cast<i16>(record.hitPoints + hitPoints);
            record.spirit = static_cast<i16>(record.spirit + spirit);
            record.stamina = static_cast<i16>(record.stamina + stamina);
            record.hitPoints = std::min(record.hitPoints, record.maxHitPoints);
            record.spirit = std::min(record.spirit, record.maxSpirit);
            record.stamina = std::min(record.stamina, record.maxStamina);

            if (record.hitPoints <= 0)
            {
                record.hitPoints = 0;
                // The command after this one becomes EndGame.
                this->SetWord(run, CommandBytes(run.code) / kWordBytes, static_cast<u16>(EventCommand::EndGame));
            }

            record.spirit = std::max<i16>(record.spirit, 0);
            record.stamina = std::max<i16>(record.stamina, 0);
            record.unk24 = 0;
            this->Next(run);
            return true;
        }

        case EventCommand::SaveMenu:
            owner.menuMode = Menu::ScriptSaveMode;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 0;
            owner.storyState = Game::SaveSlotsState;
            owner.ResetSaveSlots();
            this->Next(run);
            owner.AudioPump();
            return false;

        case EventCommand::StopMusic:
            if ((view.musicCue & Scene::StoryMusic) != 0)
            {
                owner.audio.UpdateMusicFade();
            }
            else
            {
                owner.audio.SetFadeMode(AudioManager::NoFade);
                owner.audio.SetFadeLength(0);
            }

            DebugLog(u8"StreamStartOut");
            view.musicTunes[Scene::SceneFade] = static_cast<i32>(Scene::FadePending | AudioManager::StopAtOnce);
            view.musicTunes[Scene::StoryFade] = 0;
            view.musicTunes[Scene::StoryFirst] = 0;
            view.musicTunes[Scene::StorySecond] = 0;
            view.musicCue &=
                ~(Scene::StoryMusic | Scene::UnkMusic200000 | Scene::StoryLoops | Scene::StoryRepeatFirst |
                  Scene::TrackByte);
            this->Next(run);
            DebugLog(u8"S_M_off[%d,%d]", view.musicTunes[Scene::SceneFirst], view.musicTunes[Scene::SceneSecond]);
            return true;

        case EventCommand::JumpIfFar:
        {
            const u16 column = this->Word(run, 1);
            const u16 row = this->Word(run, 2);
            const u16 operand = this->Word(run, 3);
            const u16 tiles = this->Word(run, 4);
            const u32 target = this->Dword(run, 0xA);
            const u16 id = operand == Talk::Current ? static_cast<u16>(state.playerRole) : operand;

            if (state.FindRoleOrPlayer(id, run.found))
            {
                const Role& role = state.roles[run.found];
                run.valueA = role.x - MapHeader::TileSize * column + MapHeader::TileSize / 2;
                run.valueB = role.y - MapHeader::TileSize * row;
                const i32 square = run.valueA * run.valueA + run.valueB * run.valueB;
                const auto distance = static_cast<i32>(crt::Ftol(std::sqrt(static_cast<double>(square))));

                if (distance > MapHeader::TileSize * tiles)
                {
                    this->Jump(run, target);
                    return true;
                }
            }
            else
            {
                DebugLog(u8"distance role", id);
            }

            this->Next(run);
            return true;
        }

        case EventCommand::MusicFade:
        {
            const u16 mode = this->Word(run, 1);
            const u16 length = this->Word(run, 2);
            owner.audio.SetFadeMode(mode);
            owner.audio.SetFadeLength(length);
            this->Next(run);
            DebugLog(u8"Fmode[%d,%d]", mode, length);
            return true;
        }

        case EventCommand::StartTimer:
        {
            const u16 minutes = this->Word(run, 1);
            const u16 seconds = this->Word(run, 2);
            const u16 event = this->Word(run, 3);
            owner.StartTimer(minutes, seconds, event, 0);
            this->Next(run);
            return false;
        }

        case EventCommand::StopTimer:
            view.unk4CAD20 = -1;
            state.ClearGameFlag(GameState::Timer);
            state.ClearGameFlag(GameState::HideTimer);
            state.ClearGameFlag(GameState::TimerEvent);
            this->Next(run);
            return false;

        default:
            return this->UnknownCommand(run);
    }
}

// Commands 0x90..0xC1: the end of the game, story flags, the camera's target, animations, the second map, media
// files, scripts, names, item counts, moves, file names, the movie pause, the followers, ripples, variables, the
// first party member's attributes and waits for the camera, the music and the movie.
bool Talk::RunSystemCommand(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Scene& view = owner.scene;
    Party& party = owner.party;
    const Field& field = owner.field;
    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];

    switch (run.code)
    {
        case EventCommand::EndGame:
            owner.menuState = Game::MenuEntering | (this->Byte(run, 2) == 1 ? Game::SystemMenuB : Game::SystemMenuA);
            owner.storyState = Game::StoryRunning;
            owner.ResetSaveSlots();
            owner.unk4CAEBC = 0;
            owner.unk4CAEC0 = 0;
            owner.menuMode = Menu::NoSlotsMode;
            this->Next(run);
            return false;

        case EventCommand::RoleTouchEvent:
        case EventCommand::RoleCellEffects:
        case EventCommand::RoleSolid:
            return this->RunStateCommand(run);

        case EventCommand::HideLoadingScreen:
            state.SetGameFlag(GameState::NoLoadingScreen);
            this->Next(run);
            return false;

        case EventCommand::SpotlightOn:
            state.SetGameFlag(GameState::Spotlight);
            this->Next(run);
            return false;

        case EventCommand::SpotlightOff:
            state.ClearGameFlag(GameState::Spotlight);
            this->Next(run);
            return false;

        case EventCommand::SpotlightAt:
        {
            i32 x = MapHeader::TileSize * static_cast<i16>(this->Word(run, 1));
            view.spotlightX = x;
            const i32 y = MapHeader::TileSize * static_cast<i16>(this->Word(run, 2));
            view.spotlightY = y;

            if (x == -1)
            {
                x = Display::Width / 2;
                view.spotlightX = Display::Width / 2;
            }

            if (y == -1)
            {
                // The original stores the centre row in x (and the scaled -1 values never occur).
                x = Display::Height / 2;
                view.spotlightX = Display::Height / 2;
                view.spotlightY = kSpotlightTop;
            }
            else
            {
                if (x > kSpotlightRight || x < kSpotlightLeft)
                {
                    x = x > kSpotlightRight ? kSpotlightRight : kSpotlightLeft;
                    view.spotlightX = x;
                }

                if (y > kSpotlightBottom)
                {
                    view.spotlightY = kSpotlightBottom;
                }
                else if (y < kSpotlightTop)
                {
                    view.spotlightY = kSpotlightTop;
                }
            }

            view.spotlightTargetX = x;
            view.spotlightStepX = 0;
            view.spotlightStepY = 0;
            this->Next(run);
            return false;
        }

        case EventCommand::SpotlightMove:
            view.spotlightTargetX = MapHeader::TileSize * static_cast<i16>(this->Word(run, 1));
            view.spotlightTargetY = MapHeader::TileSize * static_cast<i16>(this->Word(run, 2));
            view.spotlightStepX = static_cast<i16>(this->Word(run, 3));
            view.spotlightStepY = static_cast<i16>(this->Word(run, 4));
            this->Next(run);
            return false;

        case EventCommand::WaitSpotlight:
            if (view.spotlightX == view.spotlightTargetX && view.spotlightY == view.spotlightTargetY)
            {
                this->Next(run);
            }

            return false;

        case EventCommand::ScreenAnimationB:
        {
            const auto x = static_cast<i16>(this->Word(run, 1));
            const auto y = static_cast<i16>(this->Word(run, 2));
            const u16 act = this->Word(run, 3);
            const u16 action = this->Word(run, 4);
            auto animation = std::make_unique<ScriptAnimation>();
            ResetPlayback(animation->act);
            animation->x = x;
            animation->y = y;
            animation->act.id = act;
            animation->act.action = action;
            animation->next = std::move(view.animationsB);
            view.animationsB = std::move(animation);
            this->Next(run);
            return false;
        }

        case EventCommand::WaitAnimationB:
            return this->RunStateCommand(run);

        case EventCommand::EnterDemonPot:
            if (owner.currentMap == Field::DemonRefiningPot)
            {
                DebugLog(u8"already on map 22");
            }
            else
            {
                view.returnColumn = static_cast<i32>(static_cast<u32>(player.x) >> MapHeader::TileShift);
                view.returnRow = static_cast<i32>(static_cast<u32>(player.y) >> MapHeader::TileShift);
                view.returnMap = owner.currentMap;
                owner.ChangeMap(
                    Field::DemonRefiningPot,
                    kPotEntry,
                    kPotEntry,
                    player.act.id,
                    0,
                    ActState::Down,
                    Game::SaveRoles
                );
            }

            this->Next(run);
            return false;

        case EventCommand::LeaveDemonPot:
            if (view.returnMap <= 0)
            {
                DebugLog(u8"no map to return to");
            }
            else
            {
                owner.ChangeMap(
                    view.returnMap,
                    view.returnColumn,
                    view.returnRow,
                    player.act.id,
                    0,
                    ActState::Down,
                    Game::SaveRoles
                );
                view.returnColumn = -1;
                view.returnRow = -1;
                view.returnMap = 0;
            }

            this->Next(run);
            return false;

        case EventCommand::SetReturnMap:
        {
            const u16 map = this->Word(run, 1);
            const auto column = static_cast<i16>(this->Word(run, 2));
            const auto row = static_cast<i16>(this->Word(run, 3));

            if (map == Field::DemonRefiningPot)
            {
                DebugLog(u8"return map 22");
            }
            else
            {
                view.returnMap = map;
                view.returnColumn = column;
                view.returnRow = row;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::CopyMedia:
        case EventCommand::DeleteMedia:
            return this->CopyMedia(run);

        case EventCommand::DialogCentred:
            this->dialogCentred = 1;
            this->Next(run);
            return true;

        case EventCommand::OpenScript:
            owner.AudioPump();
            run.valueA = static_cast<i16>(this->Word(run, 1));
            this->OpenScript(state.leader, static_cast<u32>(run.valueA));
            run.cursor = 0;
            return true;

        case EventCommand::VariableNamePlate:
            return this->LoadName(run);

        case EventCommand::JumpIfNotMap:
        case EventCommand::JumpIfMap:
        {
            const auto map = static_cast<i16>(this->Word(run, 1));
            const u32 target = this->Dword(run, 4);
            run.valueA = map;

            if ((run.code == EventCommand::JumpIfNotMap) == (run.valueA == owner.currentMap))
            {
                this->Next(run);
                return true;
            }

            this->Jump(run, target);
            return true;
        }

        case EventCommand::JumpIfItemsAtLeast:
        case EventCommand::JumpIfItemsAtMost:
        {
            const auto id = static_cast<i16>(this->Word(run, 1));
            i32 count = 0;

            if (const ItemNode* carried = FindItemOfKind(party.inventory, id))
            {
                count = carried->count + carried->pending;
            }

            if (const ItemNode* worn = FindRoleItem(party.equipment, id))
            {
                count += worn->pending + worn->count;
            }

            run.valueB = count;
            const i32 wanted = static_cast<i16>(this->Word(run, 2));
            run.valueA = wanted;
            const bool jumps = count == 0 ? run.code == EventCommand::JumpIfItemsAtMost
                               : run.code == EventCommand::JumpIfItemsAtLeast ? count >= wanted
                                                                              : count <= wanted;

            if (jumps)
            {
                this->Jump(run, this->Dword(run, 6));
                return true;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::JumpIfWorn:
        case EventCommand::JumpIfNotWorn:
            return this->RunStateCommand(run);

        case EventCommand::MoveRolesActing:
            return this->MoveRoles(run);

        case EventCommand::ClearFileNameA:
        case EventCommand::SetFileNameA:
        case EventCommand::ClearFileNameB:
        case EventCommand::SetFileNameB:
        {
            std::string name;

            if (SetsFileName(run.code))
            {
                const std::vector<u8>& script = owner.databases.script;
                const std::size_t start = run.cursor + kOpcodeBytes;
                const std::size_t end = TextEndAt(script, start);

                if (start < end)
                {
                    name.assign(script.begin() + start, script.begin() + end);
                }

                run.cursor = end + kWordBytes;
                this->SyncOffset(run);
            }
            else
            {
                this->Next(run);
            }

            const bool second = run.code > EventCommand::SetFileNameA;
            run.valueA = second ? 1 : 0;
            std::optional<std::string>& slot = second ? view.unk4B74F4 : view.unk4B751C;

            if (SetsFileName(run.code))
            {
                slot = name.substr(0, kFileNameBytes);
                state.SetGameFlag(run.valueA + GameState::FileNameA);
            }
            else
            {
                slot.reset();
                state.ClearGameFlag(run.valueA + GameState::FileNameA);
            }

            return false;
        }

        case EventCommand::PauseMapMovie:
            this->Next(run);
            owner.fieldView.movie.options |= MapMovie::Paused;
            return true;

        case EventCommand::ResumeMapMovie:
            this->Next(run);
            owner.fieldView.movie.options &= ~MapMovie::Paused;
            return false;

        case EventCommand::GatherParty:
        {
            const u16 command = this->Word(run, 0);

            if ((command & kCommandStarted) == 0)
            {
                run.valueA = 1;
                this->SetWord(run, 0, static_cast<u16>(command | kCommandStarted));
                view.playerHold |= Scene::HoldLocked;
                player.act.action = 0;
                view.trailY.fill(player.y);
                view.trailX.fill(player.x);

                for (int index = 1; index < state.roleCount && index < GameState::Roles; ++index)
                {
                    Role& role = state.roles[static_cast<std::size_t>(index)];

                    if ((role.flags & Role::Follower) != 0)
                    {
                        role.flags |= Role::FastMove;
                        role.act.speed = ActState::FixedDelay;  // a fixed delay of 0 frames
                    }

                    run.valueA = state.roleCount;
                }

                return false;
            }

            run.valueB = 0;
            run.valueA = 1;

            for (int index = 1; index < state.roleCount && index < GameState::Roles; ++index)
            {
                Role& role = state.roles[static_cast<std::size_t>(index)];

                if ((role.flags & Role::Follower) != 0 && role.x == player.x && role.y == player.y)
                {
                    ++run.valueB;
                    role.flags &= ~Role::FastMove;
                    role.act.speed = 0;
                }

                run.valueA = state.roleCount;
            }

            if (run.valueB + 1 == view.unk4BABA0)
            {
                this->SetWord(run, 0, static_cast<u16>(command & ~kCommandStarted));
                this->Next(run);
            }

            return false;
        }

        case EventCommand::PassWalls:
        {
            const u16 operand = this->Word(run, 1);
            const u16 id = operand == Talk::Current ? static_cast<u16>(state.playerRole) : operand;

            if (state.FindRoleOrPlayer(id, run.found))
            {
                state.roles[run.found].flags |= Role::PassWalls;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::Ripple:
        {
            const i32 x = static_cast<i16>(this->Word(run, 1));
            const i32 y = static_cast<i16>(this->Word(run, 2));
            const i32 radius = static_cast<i16>(this->Word(run, 3));
            const i32 depth = static_cast<i16>(this->Word(run, 4));
            run.valueA = x;
            run.valueB = y;
            constexpr int kSplashRadius = 0x18;
            auto ripple = std::make_unique<ScriptObject>(
                Display::Width,
                Display::Height,
                x - radius,
                y - radius,
                2 * radius,
                2 * radius
            );
            ripple->Splash(radius, radius, kSplashRadius, depth, owner.crtRandom);
            ripple->next = std::move(view.objects);
            view.objects = std::move(ripple);
            this->Next(run);
            return true;
        }

        case EventCommand::PauseGame:
            owner.running = 0;
            this->Next(run);
            return false;

        case EventCommand::SetVariableWide:
        case EventCommand::AddVariableWide:
        case EventCommand::SubtractVariableWide:
        case EventCommand::JumpIfVariableAtLeastWide:
        case EventCommand::JumpIfVariableAtMostWide:
            return this->ChangeVariable(run);

        case EventCommand::JumpIfStatAtLeast:
        case EventCommand::JumpIfStatAtMost:
        {
            const i32 field = static_cast<i16>(this->Word(run, 1));
            run.valueB = field;

            if (field > Party::LastStat)
            {
                DebugLog(u8"attribute number", field);
                return false;
            }

            const int value = party.RoleStat(field, 1);
            run.valueA = value;
            const i32 wanted = this->Word(run, 2);

            if (run.code == EventCommand::JumpIfStatAtLeast ? value < wanted : value > wanted)
            {
                this->Next(run);
                return true;
            }

            this->Jump(run, this->Dword(run, 6));
            return false;
        }

        case EventCommand::SetStat:
        case EventCommand::AddStat:
        case EventCommand::SubtractStat:
        {
            const i32 field = static_cast<i16>(this->Word(run, 1));
            const i32 value = static_cast<i16>(this->Word(run, 2));
            run.valueB = field;

            if (field > Party::LastStat)
            {
                DebugLog(u8"attribute number", field);
                return false;
            }

            if (run.code == EventCommand::SetStat)
            {
                party.SetRoleStat(field, value, 1, owner.database);
            }
            else if (run.code == EventCommand::AddStat)
            {
                party.SetRoleStat(field, value + party.RoleStat(field, 1), 1, owner.database);
            }
            else
            {
                party.SetRoleStat(field, party.RoleStat(field, 1) - value, 1, owner.database);
            }

            this->Next(run);
            return true;
        }

        case EventCommand::WaitCameraY:
            if ((view.panAcross == 0 && view.panDown == 0) || field.cameraTop == static_cast<i16>(this->Word(run, 1)))
            {
                this->Next(run);
                return true;
            }

            return false;

        case EventCommand::WaitMusicFade:
            if (owner.audio.FadeMode() != AudioManager::FadeOverLength && owner.audio.Fading() == 0)
            {
                this->Next(run);
            }

            return false;

        case EventCommand::WaitVideo:
            run.valueA = owner.VideoPosition();

            if (run.valueA >= 0)
            {
                return false;
            }

            this->Next(run);
            return true;

        default:
            return this->UnknownCommand(run);
    }
}

}  // namespace swd3
