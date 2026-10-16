#include <algorithm>
#include <cmath>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "game/game.h"
#include "item/item_list.h"
#include "platform/file_system.h"
#include "talk/talk.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

constexpr u16 kTextEnd = 0x5125;  // "%Q"

// The role flag commands 0x66, 0x67, 0x75, 0x88, 0x8C, 0x91, 0x92 and 0xAE set or clear.
u32 RoleFlagOf(u16 code)
{
    switch (code)
    {
        case 0x66:
            return 0x40;

        case 0x67:
            return 0x20;

        case 0x75:
            return 0x10;

        case 0x88:
            return 0x1000;

        case 0x8C:
            return 0x800;

        case 0x91:
            return 0x2000;

        case 0x92:
            return 0x100;

        default:
            return 0x4000;
    }
}

}  // namespace

// Commands 0x9E and 0x9F: a file of the CD's Video\ (mode 0) or Music\ (mode 1) folder copied into the installation
// (0x9E) or deleted from the CD folder (0x9F). The name runs to "%Q"; the mode is the second word after it, and the
// script goes on with the word after "%Q".
bool Talk::CopyMedia(EventRun& run)
{
    Game& owner = this->game;
    const std::vector<u8>& script = owner.databases.script;
    std::size_t at = run.cursor + 4;
    std::string name;

    while (at + 1 < script.size() && (script[at] | script[at + 1] << 8) != kTextEnd)
    {
        name.push_back(static_cast<char>(script[at]));
        ++at;
    }

    run.valueA = static_cast<i32>(name.size());
    owner.inactive = 1;
    run.context.unk20 = static_cast<u16>(at + 2);
    run.cursor = at + 2;
    const u16 mode = at + 5 < script.size() ? static_cast<u16>(script[at + 4] | script[at + 5] << 8) : u16{0};
    run.valueB = static_cast<i16>(mode);
    std::string source = owner.config.cdRoot;
    std::string target = owner.config.installDirectory;

    if (mode == 0)
    {
        source += "Video\\";
        target += "Video\\";
    }
    else if (mode == 1)
    {
        source += "Music\\";
        target += "Music\\";
    }

    source += name;
    target += name;

    if (run.code == 0x9E)
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
        case 0x64:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            const u16 event = this->Word(run, 2);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                state.roles[run.found].event = event;
            }
            else
            {
                owner.UpdateRoleRecord(id, -1, -1, -1, -1, -1, static_cast<i16>(event), -1, 0, -1, -1);
            }

            this->Advance(run, 6);
            return true;
        }

        case 0x65:
            if (state.FindRoleOrPlayer(this->EventRoleId(run, this->Word(run, 1)), run.found))
            {
                state.roles[run.found].flags |= 0x4000000;
            }

            this->Advance(run, 4);
            return true;

        case 0x66:
        case 0x67:
        case 0x75:
        case 0x88:
        case 0x8C:
        case 0x91:
        case 0x92:
        case 0xAE:
        {
            const u32 bit = RoleFlagOf(run.code);
            const u16 id = this->Word(run, 1) == 0xFFF0 ? static_cast<u16>(state.playerRole) : this->Word(run, 1);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                role.flags &= 0xFFFFFFFF - bit;

                if (this->Word(run, 2) != 0)
                {
                    role.flags |= bit;
                }

                owner.field.ClearRoleCells(role, leader);
                owner.field.MarkRoleCells(role, leader);
            }
            else
            {
                DebugLog("set role flag", id);
                const bool set = this->Word(run, 2) != 0;
                run.valueA = set ? static_cast<i32>(bit) : 0;
                run.valueB = set ? -1 : static_cast<i32>(0xFFFFFFFF - bit);
                owner.UpdateRoleRecord(
                    id,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1,
                    static_cast<i16>(set ? bit : 0),
                    static_cast<i16>(set ? 0xFFFF : 0xFFFF - bit),
                    -1
                );
            }

            this->Advance(run, 6);
            return true;
        }

        case 0x68:
            this->dialogOptions &= ~0x10000000u;
            this->dialogOffsetX = static_cast<i16>(this->Word(run, 1));
            this->dialogOffsetY = static_cast<i16>(this->Word(run, 2));
            this->Advance(run, 6);
            return true;

        case 0x69:
            this->dialogOptions &= 0xF7FFFFFF;
            this->Advance(run, 2);
            return true;

        case 0x6A:
        case 0x9A:
        {
            const ScriptAnimation* first = run.code == 0x9A ? view.animationsB.get() : view.animationsA.get();

            if (first == nullptr || (first->act.counter >> 8) > this->Word(run, 1))
            {
                this->Advance(run, 4);
                return true;
            }

            return false;
        }

        case 0x6B:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog("Talk(tilpa1)", this->Word(run, 2));
                this->Advance(run, 6);
                return true;
            }

            const u16 rounds = state.roles[run.found].act.counter;
            const u16 wanted = this->Word(run, 2);

            if ((rounds & 0xFF) < wanted)
            {
                DebugLog("Talk(tilpa1)", wanted, rounds & 0xFF);
                this->Advance(run, 6);
                return true;
            }

            if ((rounds >> 8) >= wanted)
            {
                this->Advance(run, 6);
                return true;
            }

            return false;
        }

        case 0x6C:
        {
            u32 origin = (this->dialogOrigin & 0xFFFF0000) | this->Word(run, 1);
            const u16 y = this->Word(run, 2);
            origin = (origin & 0xFFFF) | static_cast<u32>(y) << 16;

            if ((origin & 0xFFFF) > 0x27F)
            {
                origin = (origin & 0xFFFF0000) | 0x10;
            }

            if (y > 0x1DF)
            {
                origin = (origin & 0xFFFF) | 0x100000;
            }

            this->dialogOrigin = origin;
            this->Advance(run, 6);
            return true;
        }

        case 0x6D:
        {
            const u16 count = this->Word(run, 1);

            for (u16 index = 0; index < count; ++index)
            {
                if (state.FindRoleOrPlayer(this->Word(run, 2 + index), run.found))
                {
                    owner.rolePath.ScriptStep(static_cast<int>(run.found));
                }
            }

            this->Advance(run, 2 * this->Word(run, 1) + 4);
            return false;
        }

        case 0x6E:
        case 0x6F:
        {
            bool moving = false;

            for (int index = 1; index < state.roleCount && index < GameState::kRoles; ++index)
            {
                if ((state.roles[static_cast<std::size_t>(index)].flags & 0x40000000) != 0)
                {
                    moving = true;
                    break;
                }
            }

            if (run.code == 0x6F)
            {
                moving = !moving;
            }

            if (moving)
            {
                this->Advance(run, 6);
            }
            else
            {
                this->Jump(run, this->Dword(run, 2));
            }

            return true;
        }

        case 0x70:
            if (view.effects != nullptr)
            {
                return false;
            }

            if (view.spritesB == nullptr)
            {
                this->Advance(run, 2);
            }

            return false;

        case 0x71:
            owner.audio.PlaySound(this->Word(run, 1), owner.config.soundVolume);
            this->Advance(run, 6);
            return false;

        case 0x72:
        {
            std::array<i32, 6>& tunes = view.unk4B7C74;
            tunes[3] = static_cast<i32>(0x80000001u);
            tunes[4] = this->Word(run, 1);
            tunes[5] = this->Word(run, 2);

            if (owner.audio.FadeMode() == 0)
            {
                owner.audio.SetFadeMode(1);
            }

            owner.audio.UpdateMusicFade();
            DebugLog("StreamStartOut");
            view.unk4ACDBC |= 0x800000;
            const u16 cue = this->Word(run, 3);
            run.valueA = cue;
            u32 marks = static_cast<u32>(view.unk4ACDBC) & 0xFFFCFFFF;

            if ((cue & 0x8000) == 0)
            {
                if ((cue & 0x4000) != 0)
                {
                    marks |= 0x30000;
                }

                if ((cue & 0x2000) != 0)
                {
                    marks |= 0x20000;
                }
            }

            view.unk4ACDBC = static_cast<i32>(marks & ~0xFFu);
            this->Advance(run, 8);
            DebugLog("S_M[%d,%d]", tunes[4], tunes[5]);
            return true;
        }

        case 0x73:
        {
            const u16 volume = this->Word(run, 1);
            run.valueA = volume <= 0xB ? volume : 0xB;
            owner.audio.SetMusicVolume(run.valueA);
            this->Advance(run, 4);
            return false;
        }

        case 0x74:
        {
            const u16 count = this->Word(run, 1);
            run.valueA = 0;

            for (u16 index = 0; index < count; ++index)
            {
                const u16 id = this->EventRoleId(run, this->Word(run, 2 + 3 * index));
                state.FindRoleOrPlayer(id, run.found);

                if (run.found < GameState::kRoles)
                {
                    owner.rolePath.MoveRoleTo(
                        static_cast<int>(run.found),
                        static_cast<u16>(0x10 * this->Word(run, 3 + 3 * index)),
                        static_cast<u16>(0x10 * this->Word(run, 4 + 3 * index)),
                        0,
                        -1,
                        -1,
                        -1
                    );
                }

                if (static_cast<i32>(run.found) == state.playerRole)
                {
                    view.unk4A9920 |= 0x8000;
                }
            }

            this->Advance(run, 6 * this->Word(run, 1) + 4);
            return true;
        }

        case 0x76:
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

                if (window.role != TalkWindow::kNoRole && window.role < GameState::kRoles)
                {
                    state.roles[window.role].unk26 = 0;
                }

                std::unique_ptr<TalkWindow> gone = std::move(*link);
                *link = std::move(gone->next);
                run.valueA = view.unk4A9920 & 0x8000;
                run.valueB = std::max((view.unk4A9920 & 0x7FFF) - 1, 0);
                view.unk4A9920 = run.valueA | run.valueB;
            }

            this->Advance(run, 4);
            return true;
        }

        case 0x77:
        case 0x8B:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            run.found = TalkWindow::kNoRole;

            if (id != TalkWindow::kNoRole && !state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog("Talk(wait_dlg2)", id, owner.unk4A9A10);
                this->Advance(run, 4);
                return true;
            }

            const TalkWindow* window = view.talkWindows.get();

            while (window != nullptr && static_cast<u32>(window->role) != run.found)
            {
                window = window->next.get();
            }

            if (window == nullptr)
            {
                this->Advance(run, 4);
                return true;
            }

            const u32 done = run.code == 0x77 ? window->flags & 1 : window->flags & 0x8000;

            if (done != 0)
            {
                this->Advance(run, 4);
                return true;
            }

            return false;
        }

        case 0x78:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (this->Word(run, 2) != 0xFFFF)
                {
                    role.act.id = static_cast<i16>(this->Word(run, 2));
                }

                if (this->Word(run, 3) != 0xFFFF)
                {
                    role.act.action = static_cast<i16>(this->Word(run, 3));
                }

                if (this->Word(run, 4) != 0xFFFF)
                {
                    role.act.direction = this->Word(run, 4);
                }

                role.act.delay = 0;

                if (owner.acts.Run(role.act) == 0)
                {
                    LogActError("TakeActData(Talk,TakeActData)", role);
                }

                role.flags |= 0x1000;
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    static_cast<i16>(this->Word(run, 2)),
                    static_cast<i16>(this->Word(run, 3)),
                    static_cast<i16>(this->Word(run, 4)),
                    -1,
                    -1,
                    -1,
                    -1,
                    0x1000,
                    -1,
                    -1
                );
            }

            this->Advance(run, 0xA);
            return true;
        }

        case 0x79:
            this->dialogOptions &= 0xFBFFFFFF;
            this->Advance(run, 2);
            return true;

        case 0x7A:
            this->skipAll = 0;
            this->Advance(run, 2);
            return true;

        case 0x7B:
        {
            const u16 map = this->Word(run, 1) == 0xFFF0 ? static_cast<u16>(owner.unk4A9A10) : this->Word(run, 1);
            std::vector<u8>& data = owner.databases.mapsData;
            std::size_t at = MapsDword(data, MapsDword(data, MapsTable(data, 2) + 4));
            run.valueA = 0;

            while (MapsWord(data, at) != 0 && at + 1 < data.size() && MapsWord(data, at) != map)
            {
                at += 8;
            }

            if (MapsWord(data, at) != 0 && at + 5 < data.size())
            {
                for (std::size_t index = 0; index < 3; ++index)
                {
                    const u16 value = this->Word(run, 1 + index);
                    data[at + 2 * index] = static_cast<u8>(value);
                    data[at + 2 * index + 1] = static_cast<u8>(value >> 8);
                }
            }

            if (MapsWord(data, at) == 0)
            {
                DebugLog(
                    "TalkOp:Scene_Music",
                    this->Word(run, 1),
                    this->Word(run, 2),
                    this->Word(run, 3),
                    this->Word(run, 4)
                );
            }

            this->Advance(run, 0xA);
            return true;
        }

        case 0x7C:
            this->dialogOptions &= 0xFDFFFFFF;
            this->Advance(run, 2);
            return true;

        case 0x7D:
        {
            const std::vector<u8>& script = owner.databases.script;
            std::size_t at = run.cursor + 2;
            std::string text;

            while (at + 1 < script.size() && (script[at] | script[at + 1] << 8) != kTextEnd)
            {
                text.push_back(static_cast<char>(script[at]));
                ++at;
            }

            run.valueA = static_cast<i32>(text.size());

            if (text.size() > 0xFD)
            {
                text.resize(0xFD);
            }

            this->history.push_back(text + "%Q");
            run.context.unk20 = static_cast<u16>(at + 2);
            run.cursor = at + 2;
            return false;
        }

        case 0x7E:
        case 0x7F:
        {
            const u16 id = this->Word(run, 1) == 0xFFF0 ? static_cast<u16>(state.playerRole) : this->Word(run, 1);
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

            if ((run.code == 0x7F) != (run.valueA == this->Word(run, 2)))
            {
                this->Jump(run, this->Dword(run, 6));
            }
            else
            {
                this->Advance(run, 0xA);
            }

            return true;
        }

        case 0x80:
        {
            const ItemNode* item = AddItem(
                party.inventory,
                this->Word(run, 1),
                static_cast<i16>(this->Word(run, 2)),
                ItemCountMode::Pending,
                owner.database
            );
            DebugLog("%s,%d", item != nullptr ? item->id : this->Word(run, 1), static_cast<i16>(this->Word(run, 2)));
            this->Advance(run, 6);
            return false;
        }

        case 0x81:
        case 0x82:
        case 0xA7:
        case 0xA8:
        {
            const auto id = static_cast<i16>(this->Word(run, 1));
            const ItemNode* carried = FindItemOfKind(party.inventory, id);
            const ItemNode* worn = FindRoleItem(party.equipment, id);
            const bool wanted = run.code == 0x82 || run.code == 0xA8;

            if (run.code == 0xA7 || run.code == 0xA8)
            {
                carried = nullptr;
            }

            if (wanted == (carried != nullptr || worn != nullptr))
            {
                this->Advance(run, 8);
                return true;
            }

            this->Jump(run, this->Dword(run, 4));
            return true;
        }

        case 0x83:
        {
            const u16 role = this->Word(run, 1);
            const u16 skill = this->Word(run, 2);
            run.found = role;

            if (role >= 4)
            {
                DebugLog("skill role", role);
                this->Advance(run, 6);
                return true;
            }

            ItemLink& skills = party.skills[role]->next;

            if (FindItemOfKind(skills, static_cast<i16>(skill)) == nullptr)
            {
                ItemNode* learned = AddItem(skills, skill, 1, ItemCountMode::Count, owner.database);

                if (learned != nullptr)
                {
                    learned->record.data.kind &= ~0x8000u;

                    if ((learned->record.data.unk3A & (0x8000 >> role)) == 0)
                    {
                        DebugLog("skill not for the role", role, learned->id);
                        AddFlaggedItem(skills, learned->id, -1, 0, owner.database);
                    }
                }
            }

            this->Advance(run, 6);
            return true;
        }

        case 0x84:
        {
            const u16 role = this->Word(run, 1);
            const u16 slot = this->Word(run, 2);
            run.found = role;

            if (role > 3 || slot > 0xB)
            {
                DebugLog("equipment slot", role, slot);
                this->Advance(run, 8);
                return true;
            }

            const ItemNode* item = FindItemOfKind(party.inventory, static_cast<i16>(this->Word(run, 3)));

            if (item == nullptr)
            {
                DebugLog("item missing", this->Word(run, 3));
                this->Advance(run, 8);
                return true;
            }

            std::unique_ptr<ItemNode>& worn =
                party.equipment[static_cast<std::size_t>(Party::kSlotsPerRole * role + slot)];
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
            this->Advance(run, 8);
            return true;
        }

        case 0x85:
        {
            std::size_t count = 0;

            while (this->Word(run, 1 + count) != 0 && run.cursor + 2 * (1 + count) + 1 < owner.databases.script.size())
            {
                ++count;
            }

            owner.unk4B7928.clear();

            for (std::size_t index = 0; index < count && index < 0x7F; ++index)
            {
                owner.unk4B7928.push_back(this->Word(run, 1 + index));
            }

            owner.unk4B7928.push_back(0);

            owner.menuState = 0x80000002;
            this->Advance(run, static_cast<int>(2 * count + 4));
            return false;
        }

        case 0x86:
        {
            const auto index = static_cast<u16>(this->Word(run, 1) - 1);

            if (index >= 4)
            {
                DebugLog("party role", index);
                this->Advance(run, 0xA);
                return true;
            }

            RoleRecord& record = party.roles[index];
            record.hitPoints = static_cast<i16>(record.hitPoints + this->Word(run, 2));
            record.spirit = static_cast<i16>(record.spirit + this->Word(run, 3));
            record.stamina = static_cast<i16>(record.stamina + this->Word(run, 4));
            record.hitPoints = std::min(record.hitPoints, record.maxHitPoints);
            record.spirit = std::min(record.spirit, record.maxSpirit);
            record.stamina = std::min(record.stamina, record.maxStamina);

            if (record.hitPoints <= 0)
            {
                record.hitPoints = 0;
                this->SetWord(run, 5, 0x90);
            }

            record.spirit = std::max<i16>(record.spirit, 0);
            record.stamina = std::max<i16>(record.stamina, 0);
            record.unk24 = 0;
            this->Advance(run, 0xA);
            return true;
        }

        case 0x87:
            owner.menuMode = 4;
            owner.unk4CAEBC = 1;
            owner.unk4CAEC0 = 0;
            owner.storyState = 3;
            owner.ResetSaveSlots();
            this->Advance(run, 2);
            owner.AudioPump();
            return false;

        case 0x89:
            if ((view.unk4ACDBC & 0x800000) != 0)
            {
                owner.audio.UpdateMusicFade();
            }
            else
            {
                owner.audio.SetFadeMode(0);
                owner.audio.SetFadeLength(0);
            }

            DebugLog("StreamStartOut");
            view.unk4B7C74[0] = static_cast<i32>(0x80000001u);
            view.unk4B7C74[3] = 0;
            view.unk4B7C74[4] = 0;
            view.unk4B7C74[5] = 0;
            view.unk4ACDBC &= static_cast<i32>(0xFF5CFF00u);
            this->Advance(run, 2);
            DebugLog("S_M_off[%d,%d]", view.unk4B7C74[1], view.unk4B7C74[2]);
            return true;

        case 0x8A:
        {
            const u16 id = this->Word(run, 3) == 0xFFF0 ? static_cast<u16>(state.playerRole) : this->Word(run, 3);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                const Role& role = state.roles[run.found];
                run.valueA = role.x - 0x10 * this->Word(run, 1) + 8;
                run.valueB = role.y - 0x10 * this->Word(run, 2);
                const i32 square = run.valueA * run.valueA + run.valueB * run.valueB;
                const auto distance = static_cast<i32>(crt::Ftol(std::sqrt(static_cast<double>(square))));

                if (distance > 0x10 * this->Word(run, 4))
                {
                    this->Jump(run, this->Dword(run, 0xA));
                    return true;
                }
            }
            else
            {
                DebugLog("distance role", id);
            }

            this->Advance(run, 0xE);
            return true;
        }

        case 0x8D:
        {
            const u16 mode = this->Word(run, 1);
            const u16 length = this->Word(run, 2);
            owner.audio.SetFadeMode(mode);
            owner.audio.SetFadeLength(length);
            this->Advance(run, 6);
            DebugLog("Fmode[%d,%d]", mode, length);
            return true;
        }

        case 0x8E:
            owner.StartTimer(this->Word(run, 1), this->Word(run, 2), this->Word(run, 3), 0);
            this->Advance(run, 8);
            return false;

        case 0x8F:
            view.unk4CAD20 = -1;
            state.ClearGameFlag(0x10);
            state.ClearGameFlag(0x4C);
            state.ClearGameFlag(0x12);
            this->Advance(run, 2);
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
        case 0x90:
            owner.menuState = this->Byte(run, 2) == 1 ? 0x80000005 : 0x80000004;
            owner.storyState = 0;
            owner.ResetSaveSlots();
            owner.unk4CAEBC = 0;
            owner.unk4CAEC0 = 0;
            owner.menuMode = 0;
            this->Advance(run, 4);
            return false;

        case 0x91:
        case 0x92:
        case 0xAE:
            return this->RunStateCommand(run);

        case 0x93:
            state.SetGameFlag(0x46);
            this->Advance(run, 2);
            return false;

        case 0x94:
            state.SetGameFlag(0x13);
            this->Advance(run, 2);
            return false;

        case 0x95:
            state.ClearGameFlag(0x13);
            this->Advance(run, 2);
            return false;

        case 0x96:
        {
            i32 x = 0x10 * static_cast<i16>(this->Word(run, 1));
            view.unk4B9EF8 = x;
            const i32 y = 0x10 * static_cast<i16>(this->Word(run, 2));
            view.unk4B9EFC = y;

            if (x == -1)
            {
                x = 0x140;
                view.unk4B9EF8 = 0x140;
            }

            if (y == -1)
            {
                x = 0xF0;
                view.unk4B9EF8 = 0xF0;
                view.unk4B9EFC = 0xD0;
            }
            else
            {
                if (x > 0x1B0 || x < 0xD0)
                {
                    x = x > 0x1B0 ? 0x1B0 : 0xD0;
                    view.unk4B9EF8 = x;
                }

                if (y > 0x110)
                {
                    view.unk4B9EFC = 0x110;
                }
                else if (y < 0xD0)
                {
                    view.unk4B9EFC = 0xD0;
                }
            }

            view.unk4A94A8 = x;
            view.unk4ACE58 = 0;
            view.unk4ACE5C = 0;
            this->Advance(run, 6);
            return false;
        }

        case 0x97:
            view.unk4A94A8 = 0x10 * static_cast<i16>(this->Word(run, 1));
            view.unk4CAE80 = 0x10 * static_cast<i16>(this->Word(run, 2));
            view.unk4ACE58 = static_cast<i16>(this->Word(run, 3));
            view.unk4ACE5C = static_cast<i16>(this->Word(run, 4));
            this->Advance(run, 0xA);
            return false;

        case 0x98:
            if (view.unk4B9EF8 == view.unk4A94A8 && view.unk4B9EFC == view.unk4CAE80)
            {
                this->Advance(run, 2);
            }

            return false;

        case 0x99:
        {
            auto animation = std::make_unique<ScriptAnimation>();
            ResetPlayback(animation->act);
            animation->x = static_cast<i16>(this->Word(run, 1));
            animation->y = static_cast<i16>(this->Word(run, 2));
            animation->act.id = this->Word(run, 3);
            animation->act.action = this->Word(run, 4);
            animation->next = std::move(view.animationsB);
            view.animationsB = std::move(animation);
            this->Advance(run, 0xA);
            return false;
        }

        case 0x9A:
            return this->RunStateCommand(run);

        case 0x9B:
            if (owner.unk4A9A10 == 0x16)
            {
                DebugLog("already on map 22");
            }
            else
            {
                view.unk4A9930 = static_cast<i32>(static_cast<u32>(player.x) >> 4);
                view.unk4A9938 = static_cast<i32>(static_cast<u32>(player.y) >> 4);
                view.unk4CAE88 = owner.unk4A9A10;
                owner.ChangeMap(0x16, 0x3B, 0x3B, player.act.id, 0, 1, 1);
            }

            this->Advance(run, 2);
            return false;

        case 0x9C:
            if (view.unk4CAE88 <= 0)
            {
                DebugLog("no map to return to");
            }
            else
            {
                owner.ChangeMap(view.unk4CAE88, view.unk4A9930, view.unk4A9938, player.act.id, 0, 1, 1);
                view.unk4A9930 = -1;
                view.unk4A9938 = -1;
                view.unk4CAE88 = 0;
            }

            this->Advance(run, 2);
            return false;

        case 0x9D:
            if (this->Word(run, 1) == 0x16)
            {
                DebugLog("return map 22");
            }
            else
            {
                view.unk4CAE88 = this->Word(run, 1);
                view.unk4A9930 = static_cast<i16>(this->Word(run, 2));
                view.unk4A9938 = static_cast<i16>(this->Word(run, 3));
            }

            this->Advance(run, 8);
            return true;

        case 0x9E:
        case 0x9F:
            return this->CopyMedia(run);

        case 0xA0:
            this->dialogCentred = 1;
            this->Advance(run, 2);
            return true;

        case 0xA1:
            owner.AudioPump();
            run.valueA = static_cast<i16>(this->Word(run, 1));
            this->OpenScript(state.leader, static_cast<u32>(run.valueA));
            run.cursor = 0;
            return true;

        case 0xA2:
            return this->LoadName(run);

        case 0xA3:
        case 0xA4:
        {
            run.valueA = static_cast<i16>(this->Word(run, 1));

            if ((run.code == 0xA3) == (run.valueA == owner.unk4A9A10))
            {
                this->Advance(run, 8);
                return true;
            }

            this->Jump(run, this->Dword(run, 4));
            return true;
        }

        case 0xA5:
        case 0xA6:
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
            const bool jumps = count == 0 ? run.code == 0xA6 : run.code == 0xA5 ? count >= wanted : count <= wanted;

            if (jumps)
            {
                this->Jump(run, this->Dword(run, 6));
                return true;
            }

            this->Advance(run, 0xA);
            return true;
        }

        case 0xA7:
        case 0xA8:
            return this->RunStateCommand(run);

        case 0xA9:
            return this->MoveRoles(run);

        case 0xAA:
        case 0xAB:
        case 0xAC:
        case 0xAD:
        {
            std::string name;

            if ((run.code & 1) == 1)
            {
                const std::vector<u8>& script = owner.databases.script;
                std::size_t at = run.cursor + 2;

                while (at + 1 < script.size() && (script[at] | script[at + 1] << 8) != kTextEnd)
                {
                    name.push_back(static_cast<char>(script[at]));
                    ++at;
                }

                run.context.unk20 = static_cast<u16>(at + 2);
                run.cursor = at + 2;
            }
            else
            {
                this->Advance(run, 2);
            }

            const bool second = run.code > 0xAB;
            run.valueA = second ? 1 : 0;
            std::optional<std::string>& slot = second ? view.unk4B74F4 : view.unk4B751C;

            if ((run.code & 1) != 0)
            {
                slot = name.substr(0, 0x33);
                state.SetGameFlag(run.valueA + 0x4D);
            }
            else
            {
                slot.reset();
                state.ClearGameFlag(run.valueA + 0x4D);
            }

            return false;
        }

        case 0xAF:
            this->Advance(run, 2);
            owner.fieldView.movie.options |= 0x10;
            return true;

        case 0xB0:
            this->Advance(run, 2);
            owner.fieldView.movie.options &= ~0x10u;
            return false;

        case 0xB1:
        {
            const u16 command = this->Word(run, 0);

            if ((command & 0x8000) == 0)
            {
                run.valueA = 1;
                this->SetWord(run, 0, static_cast<u16>(command | 0x8000));
                view.unk4A9920 |= 0x8000;
                player.act.action = 0;
                view.trailY.fill(player.y);
                view.trailX.fill(player.x);

                for (int index = 1; index < state.roleCount && index < GameState::kRoles; ++index)
                {
                    Role& role = state.roles[static_cast<std::size_t>(index)];

                    if ((role.flags & 0x80) != 0)
                    {
                        role.flags |= 0x4000000;
                        role.act.speed = static_cast<i16>(0x8000);
                    }

                    run.valueA = state.roleCount;
                }

                return false;
            }

            run.valueB = 0;
            run.valueA = 1;

            for (int index = 1; index < state.roleCount && index < GameState::kRoles; ++index)
            {
                Role& role = state.roles[static_cast<std::size_t>(index)];

                if ((role.flags & 0x80) != 0 && role.x == player.x && role.y == player.y)
                {
                    ++run.valueB;
                    role.flags &= ~0x4000000u;
                    role.act.speed = 0;
                }

                run.valueA = state.roleCount;
            }

            if (run.valueB + 1 == view.unk4BABA0)
            {
                this->SetWord(run, 0, static_cast<u16>(command & 0x7FFF));
                this->Advance(run, 2);
            }

            return false;
        }

        case 0xB2:
        {
            const u16 id = this->Word(run, 1) == 0xFFF0 ? static_cast<u16>(state.playerRole) : this->Word(run, 1);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                state.roles[run.found].flags |= 0x40000;
            }

            this->Advance(run, 4);
            return true;
        }

        case 0xB3:
        {
            const i32 x = static_cast<i16>(this->Word(run, 1));
            const i32 y = static_cast<i16>(this->Word(run, 2));
            const i32 radius = static_cast<i16>(this->Word(run, 3));
            const i32 depth = static_cast<i16>(this->Word(run, 4));
            run.valueA = x;
            run.valueB = y;
            auto ripple = std::make_unique<ScriptObject>(0x280, 0x1E0, x - radius, y - radius, 2 * radius, 2 * radius);
            ripple->Splash(radius, radius, 0x18, depth, owner.crtRandom);
            ripple->next = std::move(view.objects);
            view.objects = std::move(ripple);
            this->Advance(run, 0xA);
            return true;
        }

        case 0xB4:
            owner.running = 0;
            this->Advance(run, 2);
            return false;

        case 0xB5:
        case 0xB6:
        case 0xB7:
        case 0xB8:
        case 0xB9:
            return this->ChangeVariable(run);

        case 0xBA:
        case 0xBB:
        {
            const i32 field = static_cast<i16>(this->Word(run, 1));
            run.valueB = field;

            if (field > 0x10)
            {
                DebugLog("attribute number", field);
                return false;
            }

            const int value = party.RoleStat(field, 1);
            run.valueA = value;
            const i32 wanted = this->Word(run, 2);

            if (run.code == 0xBA ? value < wanted : value > wanted)
            {
                this->Advance(run, 0xA);
                return true;
            }

            this->Jump(run, this->Dword(run, 6));
            return false;
        }

        case 0xBC:
        case 0xBD:
        case 0xBE:
        {
            const i32 field = static_cast<i16>(this->Word(run, 1));
            const i32 value = static_cast<i16>(this->Word(run, 2));
            run.valueB = field;

            if (field > 0x10)
            {
                DebugLog("attribute number", field);
                return false;
            }

            if (run.code == 0xBC)
            {
                party.SetRoleStat(field, value, 1, owner.database);
            }
            else if (run.code == 0xBD)
            {
                party.SetRoleStat(field, value + party.RoleStat(field, 1), 1, owner.database);
            }
            else
            {
                party.SetRoleStat(field, party.RoleStat(field, 1) - value, 1, owner.database);
            }

            this->Advance(run, 6);
            return true;
        }

        case 0xBF:
            if ((view.unk4A99F0 == 0 && view.unk4A98C0 == 0) || field.cameraTop == static_cast<i16>(this->Word(run, 1)))
            {
                this->Advance(run, 4);
                return true;
            }

            return false;

        case 0xC0:
            if (owner.audio.FadeMode() != 2 && owner.audio.Fading() == 0)
            {
                this->Advance(run, 2);
            }

            return false;

        case 0xC1:
            run.valueA = owner.VideoPosition();

            if (run.valueA >= 0)
            {
                return false;
            }

            this->Advance(run, 2);
            return true;

        default:
            return this->UnknownCommand(run);
    }
}

}  // namespace swd3
