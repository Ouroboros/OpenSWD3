#include <algorithm>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "game/game.h"
#include "platform/message_box.h"
#include "talk/talk.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

constexpr u16 kTextEnd = 0x5125;  // "%Q"
constexpr u16 kEventRole = 0xFFF0;
constexpr u16 kListEnd = 0xFF00;

}  // namespace

void LogActError(std::string_view where, const Role& role)
{
    DebugLog(where, role.id, role.act.id, role.act.action, role.act.direction);
}

// swd32.exe 0x00427920: the event script of `context` (the leader) from where it stopped. On its first frame the
// script is read from Talk<event / 2000 + 1>.dat (0x8000 bytes, the rest 0x0C) once the controlled role stands on a
// tile, and the role of the event is marked (flag 0x80000). The commands run until one waits for the next frame;
// between 0x400 and 0x401 they run on regardless. A missing Talk file ends the game.
void Talk::RunEvent(Role& context)
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (context.id == 0xFFFF)
    {
        return;
    }

    if (context.unk14 == 0)
    {
        Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];

        if (((player.x | player.y) & 0xF) != 0)
        {
            return;
        }

        if (context.id != TalkWindow::kNoRole)
        {
            u32 index = 0;
            state.FindRoleOrPlayer(context.id, index);

            if (index < GameState::kRoles)
            {
                state.roles[index].flags |= role_flag::kInEvent;
            }
        }

        const u32 file = context.event / 0x7D0u;
        const std::string path = WinFormat("%sTalk%d.dat", {owner.config.installDirectory, file + 1});
        File& story = owner.databases.story;
        owner.AudioPump();
        story.Close();

        if (!story.Open(path, File::Disposition::OpenExisting, File::Access::Read))
        {
            DebugLog("SWD3-Talk", Big5(u8"找不到 Talk%d"), file + 1);
            owner.Destroy();
            return;
        }

        story.SeekBegin(static_cast<i32>(4 * (context.event % 0x7D0u) + 0x200));
        u32 offset = 0;
        story.ReadDword(offset);
        context.unk14 = static_cast<i32>(offset);
        context.unk20 = 0;
        story.SeekBegin(static_cast<i32>(offset + 0x200));
        std::fill(owner.databases.script.begin(), owner.databases.script.end(), u8{0xC});
        u32 bytes = static_cast<u32>(owner.databases.script.size());
        story.Read(owner.databases.script.data(), bytes);
        owner.scene.unk4A9920 |= 0x8000;

        if (owner.acts.Run(player.act) == 0)
        {
            LogActError("Act Err(Talk:LoadTalk)", player);
        }

        owner.AudioPump();
    }

    EventRun run{context, context.unk20};

    for (;;)
    {
        run.code = static_cast<EventCommand>(this->Word(run, 0) & kCommandMask);
        const bool next = this->RunCommand(run);

        if (run.stopped)
        {
            return;
        }

        this->lastCommand = static_cast<u16>(run.code);

        if (!next && !run.running)
        {
            owner.AudioPump();
            return;
        }
    }
}

// Word `index` of the command (0 past the script).
u16 Talk::Word(const EventRun& run, std::size_t index) const
{
    const std::vector<u8>& script = this->game.databases.script;
    const std::size_t at = run.cursor + 2 * index;
    return at + 1 < script.size() ? static_cast<u16>(script[at] | script[at + 1] << 8) : u16{0};
}

// The dword at byte `at` of the command.
u32 Talk::Dword(const EventRun& run, std::size_t at) const
{
    const std::vector<u8>& script = this->game.databases.script;
    const std::size_t from = run.cursor + at;
    u32 value = 0;

    for (std::size_t index = 0; index < 4; ++index)
    {
        if (from + index < script.size())
        {
            value |= static_cast<u32>(script[from + index]) << (8 * index);
        }
    }

    return value;
}

// The byte `at` of the command.
u8 Talk::Byte(const EventRun& run, std::size_t at) const
{
    const std::vector<u8>& script = this->game.databases.script;
    return run.cursor + at < script.size() ? script[run.cursor + at] : u8{0};
}

// The script changes itself: word `index` of the command.
void Talk::SetWord(EventRun& run, std::size_t index, u16 value)
{
    std::vector<u8>& script = this->game.databases.script;
    const std::size_t at = run.cursor + 2 * index;

    if (at + 1 < script.size())
    {
        script[at] = static_cast<u8>(value);
        script[at + 1] = static_cast<u8>(value >> 8);
    }
}

// Past the command: the cursor and the offset of the role.
void Talk::Advance(EventRun& run, int bytes)
{
    run.cursor = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(run.cursor) + bytes);
    run.context.unk20 = static_cast<u16>(run.context.unk20 + bytes);
}

// The offset of the role set to where the cursor is.
void Talk::SyncOffset(EventRun& run)
{
    run.context.unk20 = static_cast<u16>(run.cursor);
}

// A jump of the leader's script to `offset`, read again from the start.
void Talk::Jump(EventRun& run, u32 offset)
{
    this->SeekScript(this->game.state.leader, offset);
    run.cursor = 0;
}

// The role last looked up (nullptr when it was not found: the original goes on with role -1, before the roles).
Role* Talk::FoundRole(const EventRun& run)
{
    return run.found < GameState::kRoles ? &this->game.state.roles[run.found] : nullptr;
}

// The role of a command: 0xFFF0 stands for the role of the event.
u16 Talk::EventRoleId(const EventRun& run, u16 id) const
{
    return id == kEventRole ? run.context.id : id;
}

// A story flag of a command (flags past the 0x400 bytes read as clear and are not set).
bool Talk::StoryFlag(int flag) const
{
    return flag >= 0 && flag < 8 * GameState::kFlagBytes && this->game.state.GameFlag(flag);
}

void Talk::SetStoryFlag(int flag, bool value)
{
    if (flag < 0 || flag >= 8 * GameState::kFlagBytes)
    {
        return;
    }

    if (value)
    {
        this->game.state.SetGameFlag(flag);
    }
    else
    {
        this->game.state.ClearGameFlag(flag);
    }
}

// An unknown command: a beep, and the script stays on it.
bool Talk::UnknownCommand(EventRun& run)
{
    PlaySystemBeep();
    DebugLog(Big5(u8"對話系統"), Big5(u8"沒有指令(%d,%x),(%d,%x)"), static_cast<u16>(run.code), this->lastCommand);
    return false;
}

// One command; true to go on with the next in the same frame.
bool Talk::RunCommand(EventRun& run)
{
    const EventCommand code = run.code;

    if (code > EventCommand::WaitMovieFrame)
    {
        if (code > EventCommand::RunOn)
        {
            if (code == EventCommand::RunOff)
            {
                this->Advance(run, 2);
                run.running = false;
                return false;
            }

            if (code == EventCommand::SkipQuick)
            {
                this->Advance(run, 2);
                return true;
            }

            if (code == EventCommand::End)
            {
                this->EndEvent(run);
                return false;
            }

            return this->UnknownCommand(run);
        }

        if (code == EventCommand::RunOn)
        {
            this->Advance(run, 2);
            run.running = true;
            return false;
        }

        return code < EventCommand::EndGame ? this->RunStateCommand(run) : this->RunSystemCommand(run);
    }

    if (code == EventCommand::WaitMovieFrame)
    {
        run.valueA = this->Word(run, 1);

        if (this->game.fieldView.movie.frameIndex > run.valueA)
        {
            this->Advance(run, 4);
            return true;
        }

        return false;
    }

    return code < EventCommand::PanCamera ? this->RunRoleCommand(run) : this->RunSceneCommand(run);
}

// Commands 0x3FFF: the end of the event. The role of the event leaves it (its move resumed, the act it had before
// turning back), the turned roles forget their old acts, the scripted moves are freed, and the leader is cleared.
void Talk::EndEvent(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const u16 id = run.context.id;

    if (id != 0xFFFF && id != TalkWindow::kNoRole)
    {
        if (state.FindRoleOrPlayer(id, run.found))
        {
            Role& role = state.roles[run.found];
            owner.rolePath.ResumeMove(static_cast<int>(run.found));
            role.flags &= ~role_flag::kHeld;

            if ((role.flags & role_flag::kFacesTalker) != 0)
            {
                if (role.act.savedAction != -1)
                {
                    role.act.action = role.act.savedAction;
                }

                if (role.act.savedDirection != -1)
                {
                    role.act.direction = role.act.savedDirection;
                }
            }

            role.act.savedAction = -1;
            role.act.savedDirection = -1;

            if (owner.acts.Run(role.act) == 0)
            {
                LogActError("Act Err(Talk:Talkend)", role);
            }

            if ((role.flags & role_flag::kInEvent) != 0)
            {
                owner.rolePath.ResumeMove(static_cast<int>(run.found));
                role.flags &= ~role_flag::kHeld;
            }
        }
        else if (state.GameFlag(story_flag::kDebugReports) && run.found >= 0x100)
        {
            DebugLog("TalkEnd role", id, owner.unk4A9A10);
        }
    }

    for (int index = 0; index < state.roleCount && index < GameState::kRoles; ++index)
    {
        state.roles[static_cast<std::size_t>(index)].act.savedAction = -1;
        state.roles[static_cast<std::size_t>(index)].act.savedDirection = -1;
    }

    for (MoveRecord& move : state.moves)
    {
        run.found = move.role;

        if (move.role != 0xFFFF && (move.mode & 0xF) > 1)
        {
            move.Clear();

            if (run.found < GameState::kRoles)
            {
                state.roles[run.found].unk1C = 0;
                state.roles[run.found].unk18 = 0;
            }
        }
    }

    DebugLog("TalkEnd [%d]", run.context.event);
    owner.scene.unk4A9920 &= ~0x8000;
    owner.databases.script[0] = 0xFF;
    owner.databases.script[1] = 0xFF;
    state.leader.FillUnused();
    owner.unk4CAD24 = 0;
    owner.counterAlpha = 0;
}

// Commands 1..6, 0x59 and 0x5A: a talk window from the script (its speaker, frame act, place and text up to "%Q"),
// with the settings the commands before it made (place, offset, timeout, name plate, options), which are then
// reset. The odd commands wait for the window. The window opens out of the speaker, or the event's place for none
// (0xFFFD); the command keeps the speaker it found.
bool Talk::OpenDialog(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Field& field = owner.field;
    Role& context = run.context;
    const EventCommand code = run.code;
    run.found = TalkWindow::kNoRole;
    const u16 speaker = this->EventRoleId(run, this->Word(run, 1));

    if (speaker != TalkWindow::kNoRole)
    {
        state.FindRoleOrPlayer(speaker, run.found);
    }

    this->SetWord(run, 1, speaker);
    auto window = std::make_unique<TalkWindow>();

    if (speaker == TalkWindow::kNoRole)
    {
        if (context.id != TalkWindow::kNoRole)
        {
            DebugLog("talk window without a speaker", 1);
        }

        window->fromX = static_cast<u16>(context.x);
        window->fromY = static_cast<u16>(context.y);
    }

    if ((this->dialogOrigin & 0xFFFF) != 0x8000)
    {
        window->fromX = static_cast<u16>(this->dialogOrigin + static_cast<u32>(field.cameraLeft));
        window->fromY = static_cast<u16>((this->dialogOrigin >> 16) + static_cast<u32>(field.cameraTop));
    }

    u32 options = 0;

    if (code >= EventCommand::SizedDialogWait && code <= EventCommand::PlainDialog)
    {
        options = 1;
    }

    if (code == EventCommand::RecordDialogWait || code == EventCommand::RecordDialog)
    {
        options = 2;
    }

    window->flags = code == EventCommand::PlainDialogWait || code == EventCommand::PlainDialog ? 0x40 : 0;
    const u32 settings = this->dialogOptions;

    if ((settings & 0x80000000) == 0)
    {
        window->flags |= 0x20;
    }

    if (this->dialogTimed == 1)
    {
        window->timeout = this->dialogTimeout;
        window->flags |= 8;
    }

    if ((settings & 0x40000000) == 0)
    {
        window->flags |= 0x400;
    }

    if ((settings & 0x20000000) == 0)
    {
        window->flags |= 0x80;
    }

    if ((settings & 0x4000000) == 0)
    {
        window->flags |= 2;
    }

    if ((settings & 0x10000000) == 0)
    {
        options |= 0x8000;
    }

    if ((settings & 0x8000000) == 0)
    {
        options |= 0x10000;
    }

    if ((settings & 0x2000000) == 0)
    {
        options |= 0x20000;
    }

    if (this->dialogCentred == 1)
    {
        options |= 0x40000;
    }

    owner.AudioPump();
    const std::vector<u8>& script = owner.databases.script;
    const std::size_t recordAt = std::min(run.cursor + 2, script.size());
    const std::string_view name(this->dialogName.data(), strnlen(this->dialogName.data(), this->dialogName.size()));
    this->SetupDialog(
        *window,
        std::span<const u8>(script).subspan(recordAt),
        name,
        options,
        static_cast<i16>(this->dialogOffsetX),
        static_cast<i16>(this->dialogOffsetY)
    );
    Role* role = this->FoundRole(run);

    if (run.found == TalkWindow::kNoRole)
    {
        context.unk26 = 2;
    }
    else if (role != nullptr)
    {
        role->unk26 = 2;
    }

    if (WaitsForWindow(code))
    {
        window->flags |= 0x10;

        if (run.found == TalkWindow::kNoRole)
        {
            context.unk26 = 1;
        }
        else if (role != nullptr)
        {
            role->unk26 = 1;
        }

        ++owner.scene.unk4A9920;
    }

    this->AppendWindow(std::move(window));
    std::size_t at = run.cursor + 2;

    while (at < script.size() && (at + 1 >= script.size() || (script[at] | script[at + 1] << 8) != kTextEnd))
    {
        ++at;
    }

    run.cursor = at + 2;
    this->SyncOffset(run);
    this->dialogOrigin = 0x80008000;
    this->dialogOptions = 0xFFFFFFFF;
    this->dialogTimeout = 0x3C;
    this->dialogTimed = 0;
    this->dialogName[0] = '\0';
    this->dialogOffsetY = 0;
    this->dialogOffsetX = 0;
    this->dialogCentred = 0;
    return false;
}

// Commands 0x14 and 0xA9: roles sent to tiles (0xFFFF: the controlled role's), all at once; 0x14 lists role and tile,
// 0xA9 adds the act, action and direction to end with. The command marks itself sent (bit 14 of its count) and waits
// until every role has arrived; bit 15 lets the event go on meanwhile.
bool Talk::MoveRoles(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    const Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    const u16 header = this->Word(run, 1);
    const std::size_t entry = run.code == EventCommand::MoveRoles ? 3 : 6;
    const u32 count = header & 0x3FFF;

    if ((header & 0x4000) == 0)
    {
        run.valueA = header & 0x8000;
        std::size_t at = 2;

        for (u32 index = 0; index < count; ++index, at += entry)
        {
            const u16 id = this->Word(run, at);

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog("Bmove", id);
                continue;
            }

            Role& role = state.roles[run.found];

            if (id == run.context.id)
            {
                role.flags &= ~role_flag::kInEvent;
                role.act.lastAction = -1;
                role.act.lastDirection = -1;
            }

            if (role.act.speed == 0)
            {
                role.act.speed = static_cast<i16>(0x8001);
            }

            const u16 tileY = this->Word(run, at + 2) == 0xFFFF ? static_cast<u16>(static_cast<u32>(player.y) >> 4)
                                                                : this->Word(run, at + 2);
            const u16 tileX = this->Word(run, at + 1) == 0xFFFF ? static_cast<u16>(static_cast<u32>(player.x) >> 4)
                                                                : this->Word(run, at + 1);

            if (run.code == EventCommand::MoveRoles)
            {
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(0x10 * tileX),
                    static_cast<u16>(0x10 * tileY),
                    static_cast<u32>(run.valueA),
                    -1,
                    -1,
                    -1
                );
            }
            else
            {
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(0x10 * tileX),
                    static_cast<u16>(0x10 * tileY),
                    static_cast<u32>(run.valueA),
                    static_cast<i16>(this->Word(run, at + 3)),
                    static_cast<i16>(this->Word(run, at + 4)),
                    static_cast<i16>(this->Word(run, at + 5))
                );
            }

            if (static_cast<i32>(run.found) == state.playerRole)
            {
                owner.scene.unk4A9920 |= 0x8000;
            }
        }

        this->SetWord(run, 1, static_cast<u16>(header | 0x4000));
        return false;
    }

    u32 arrived = 0;
    std::size_t at = 2;

    for (u32 index = 0; index < count; ++index, at += entry)
    {
        state.FindRoleOrPlayer(this->Word(run, at), run.found);
        Role* role = this->FoundRole(run);

        if (role == nullptr)
        {
            continue;
        }

        if ((role->flags & role_flag::kArrived) != 0 || owner.rolePath.ScriptStep(static_cast<int>(run.found)) == 2)
        {
            ++arrived;
        }
    }

    if (arrived != count)
    {
        return false;
    }

    this->SetWord(run, 1, static_cast<u16>(header & 0x3FFF));
    run.valueA = static_cast<i32>(2 * entry);
    this->Advance(run, static_cast<int>(count * 2 * entry + 4));
    return true;
}

// Commands 0x1D..0x21 and 0xB5..0xB9: a script variable (0..0x3F) set, raised, lowered (not below 0) or compared
// (a jump when at least or at most the value); the second set takes a dword. The gold never stays negative.
bool Talk::ChangeVariable(EventRun& run)
{
    Party& party = this->game.party;
    const bool wide = run.code > EventCommand::JumpIfVariableAtMost;
    const i32 index = static_cast<i16>(this->Word(run, 1));
    const i32 value = wide ? static_cast<i32>(this->Dword(run, 4)) : static_cast<i16>(this->Word(run, 2));
    run.valueA = index;
    run.valueB = value;

    if (index < 0 || index >= Party::kVariables)
    {
        DebugLog(Big5(u8"對話系統"), index, value);
        return false;
    }

    i32& variable = party.variables[static_cast<std::size_t>(index)];
    int length = 0;

    switch (run.code)
    {
        case EventCommand::SetVariable:
        case EventCommand::SetVariableWide:
            variable = value;
            length = wide ? 8 : 6;
            break;

        case EventCommand::AddVariable:
        case EventCommand::AddVariableWide:
            variable += value;
            length = wide ? 8 : 6;
            break;

        case EventCommand::SubtractVariable:
        case EventCommand::SubtractVariableWide:
            variable -= value;

            if (variable < 0)
            {
                variable = 0;
            }

            length = wide ? 8 : 6;
            break;

        case EventCommand::JumpIfVariableAtLeast:
        case EventCommand::JumpIfVariableAtLeastWide:
        case EventCommand::JumpIfVariableAtMost:
        case EventCommand::JumpIfVariableAtMostWide:
        {
            length = wide ? 0xC : 0xA;
            const u32 target = wide ? this->Dword(run, 8) : this->Dword(run, 6);
            const bool jumps =
                run.code == EventCommand::JumpIfVariableAtLeast || run.code == EventCommand::JumpIfVariableAtLeastWide
                    ? variable >= value
                    : variable <= value;

            if (jumps)
            {
                this->Jump(run, target);
                length = 0;
            }

            break;
        }

        default:
            break;
    }

    if (party.variables[0] < 0)
    {
        party.variables[0] = 0;
    }

    run.found = static_cast<u32>(length);
    this->Advance(run, length);
    return true;
}

// Commands 0x01..0x31: the talk windows, the roles' acts, places and moves, jumps on flags, choices and the clock.
bool Talk::RunRoleCommand(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& context = run.context;
    const std::span<const u8> script(owner.databases.script);
    const auto leader = static_cast<u16>(owner.unk4B7BC4);

    switch (run.code)
    {
        case EventCommand::DialogWait:
        case EventCommand::Dialog:
        case EventCommand::SizedDialogWait:
        case EventCommand::SizedDialog:
        case EventCommand::PlainDialogWait:
        case EventCommand::PlainDialog:
            return this->OpenDialog(run);

        case EventCommand::DialogNoSkip:
            this->dialogOptions &= 0x7FFFFFFF;
            this->Advance(run, 2);
            return true;

        case EventCommand::DialogTimeout:
            this->dialogTimed = 1;
            this->dialogTimeout = this->Word(run, 1);
            this->Advance(run, 4);
            return true;

        case EventCommand::DialogNoAdvance:
            this->dialogOptions &= 0xBFFFFFFF;
            this->Advance(run, 2);
            return true;

        case EventCommand::SetAction:
        case EventCommand::SetDirection:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));
            const u16 value = this->Word(run, 2);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                const std::span<const u8> next = script.subspan(std::min(run.cursor + 6, script.size()));

                if (run.code == EventCommand::SetAction)
                {
                    role.act.delay = 0;
                    role.act.action = value;

                    if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                    {
                        LogActError("Act Err(Talk:ChangQQ)", role);
                    }
                }
                else
                {
                    role.act.direction = value;
                    role.act.delay = 0;

                    if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                    {
                        LogActError("Act Err(Talk:ChangDir)", role);
                    }

                    role.flags |= role_flag::kStepsAct;
                }
            }
            else if (run.code == EventCommand::SetAction)
            {
                owner.UpdateRoleRecord(id, -1, static_cast<i16>(value), -1, -1, -1, -1, -1, 0x1000, -1, -1);
            }
            else
            {
                owner.UpdateRoleRecord(id, -1, -1, static_cast<i16>(value), -1, -1, -1, -1, 0x1000, -1, -1);
            }

            this->Advance(run, 6);
            return true;
        }

        case EventCommand::PutRoleOnTile:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog("setpos", id);
                this->Advance(run, 8);

                if (static_cast<i32>(run.found) == state.playerRole)
                {
                    owner.scene.unk4A9920 |= 0x8000;
                }

                return true;
            }

            Role& role = state.roles[run.found];

            if (this->Word(run, 1) == context.id)
            {
                role.act.lastAction = -1;
                role.act.lastDirection = -1;
                role.flags &= ~role_flag::kInEvent;
            }

            owner.rolePath.MoveRoleTo(
                static_cast<int>(run.found),
                static_cast<u16>(0x10 * this->Word(run, 2)),
                static_cast<u16>(0x10 * this->Word(run, 3)),
                0,
                -1,
                -1,
                -1
            );
            this->Advance(run, 8);

            if (static_cast<i32>(run.found) == state.playerRole)
            {
                owner.scene.unk4A9920 |= 0x8000;
            }

            return true;
        }

        case EventCommand::StepRole:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                if ((state.roles[run.found].flags & role_flag::kArrived) == 0)
                {
                    owner.rolePath.ScriptStep(static_cast<int>(run.found));
                }
            }
            else
            {
                DebugLog("Talk(Step)", id, owner.unk4A9A10);
            }

            this->Advance(run, 4);
            return false;
        }

        case EventCommand::WaitDialogClosed:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (id == TalkWindow::kNoRole)
            {
                if (context.unk26 == 0)
                {
                    this->Advance(run, 4);
                }

                return false;
            }

            state.FindRoleOrPlayer(id, run.found);
            const Role* role = this->FoundRole(run);

            if (role != nullptr && role->unk26 != 0)
            {
                return false;
            }

            this->Advance(run, 4);
            return false;
        }

        case EventCommand::Jump:
            this->Jump(run, this->Dword(run, 2));
            return true;

        case EventCommand::JumpIfMoveIdle:
        case EventCommand::JumpIfMoveRunning:
        {
            state.FindRoleOrPlayer(this->Word(run, 1), run.found);
            const Role* role = this->FoundRole(run);
            const u32 moving = role != nullptr ? role->flags & role_flag::kStepping : 0;
            int slot = 0;

            while (slot < GameState::kMoves)
            {
                const MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

                if (move.role == run.found && (move.mode & 0xF) == 2 &&
                    (run.code == EventCommand::JumpIfMoveIdle ? moving == 0 : moving != 0))
                {
                    break;
                }

                ++slot;
            }

            if (slot == GameState::kMoves)
            {
                this->Advance(run, 8);
                return true;
            }

            this->Jump(run, this->Dword(run, 4));
            run.running = false;
            return true;
        }

        case EventCommand::ResumeMove:
        {
            state.FindRoleOrPlayer(this->Word(run, 1), run.found);

            if (run.found < GameState::kRoles && owner.rolePath.ResumeMove(static_cast<int>(run.found)))
            {
                this->Advance(run, 4);
            }

            if (Role* role = this->FoundRole(run))
            {
                role->flags &= ~role_flag::kHeld;
                role->act.delay = 0;
            }

            return true;
        }

        case EventCommand::ResumeAllMoves:
            for (int index = 1; index < state.roleCount && index < GameState::kRoles; ++index)
            {
                Role& role = state.roles[static_cast<std::size_t>(index)];

                if ((role.flags & role_flag::kHeld) != 0)
                {
                    owner.rolePath.ResumeMove(index);
                    role.flags &= ~role_flag::kHeld;
                    role.act.delay = 0;
                }
            }

            this->Advance(run, 2);
            return true;

        case EventCommand::MoveRoles:
            return this->MoveRoles(run);

        case EventCommand::JumpIfFlag:
        case EventCommand::JumpIfNotFlag:
            if (this->StoryFlag(this->Word(run, 1)) != (run.code == EventCommand::JumpIfNotFlag))
            {
                this->Jump(run, this->Dword(run, 4));
                return true;
            }

            this->Advance(run, 8);
            return true;

        case EventCommand::JumpIfAllFlags:
        case EventCommand::JumpIfAnyFlag:
        {
            std::size_t count = 0;
            std::size_t set = 0;

            for (u16 flag = this->Word(run, 1); flag != kListEnd; flag = this->Word(run, 1 + count))
            {
                if (run.cursor + 2 * (1 + count) + 1 >= script.size())
                {
                    break;
                }

                run.found = static_cast<u32>(flag >> 3);

                if (this->StoryFlag(flag))
                {
                    ++set;
                }

                ++count;
            }

            const bool jumps = run.code == EventCommand::JumpIfAllFlags ? count == set : set != 0;

            if (jumps)
            {
                this->Jump(run, this->Dword(run, 2 * (1 + count) + 2));
                return true;
            }

            this->Advance(run, static_cast<int>(2 * count + 8));
            return true;
        }

        case EventCommand::SetFlag:
        case EventCommand::ClearFlag:
            this->SetStoryFlag(this->Word(run, 1), run.code == EventCommand::SetFlag);
            DebugLog(run.code == EventCommand::SetFlag ? "fON [%d]" : "fOFF[%d]", this->Word(run, 1));
            this->Advance(run, 4);
            return true;

        case EventCommand::ChangeMap:
            owner.ChangeMap(
                this->Word(run, 1),
                this->Word(run, 2),
                this->Word(run, 3),
                this->Word(run, 4),
                this->Word(run, 5),
                this->Word(run, 6),
                1
            );
            this->Advance(run, 0xE);
            return true;

        case EventCommand::SetRoleTalk:
        {
            const u16 id = this->Word(run, 1);

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                owner
                    .UpdateRoleRecord(id, -1, -1, -1, -1, -1, -1, static_cast<i16>(this->Word(run, 2)), 0x1000, -1, -1);
                this->Advance(run, 6);
                return false;
            }

            Role& role = state.roles[run.found];

            if (!role.label.empty())
            {
                role.label.clear();
                role.labelColor = 0;
            }

            int slot = 0;

            while (slot < GameState::kMoves && state.moves[static_cast<std::size_t>(slot)].role != run.found)
            {
                ++slot;
            }

            if (slot < GameState::kMoves)
            {
                MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

                if ((move.mode & 0xF) == 2)
                {
                    move.nextRole = 0xFFFF;
                    move.unk0A = 0xFFFF;
                    move.nextX = 0xFFFF;
                    move.nextY = 0xFFFF;
                }

                if ((move.mode & 0xF) == 1)
                {
                    if ((role.x & 0xF) != 0 || (role.y & 0xF) != 0)
                    {
                        const int row = static_cast<int>(static_cast<u32>(role.y) >> 4) - 1;

                        if ((role.flags & role_flag::kSolid) == 0)
                        {
                            owner.field.ClearRoleCells(role, leader);
                        }

                        const u8 back =
                            move.directions[static_cast<std::size_t>(move.step & 0x7FFF) % move.directions.size()];

                        while ((role.x & 0xF) != 0 && back < 8)
                        {
                            role.x -= RolePath::kStepX[back];
                        }

                        while ((role.y & 0xF) != 0 && back < 8)
                        {
                            role.y -= RolePath::kStepY[back];
                        }

                        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & role_flag::kLayerMask), row, 0);
                    }

                    move.Clear();
                }
            }

            role.flags |= role_flag::kStepsAct;
            role.unk1C = this->Word(run, 2);
            role.unk18 = 0;
            this->Advance(run, 6);
            return false;
        }

        case EventCommand::SetVariable:
        case EventCommand::AddVariable:
        case EventCommand::SubtractVariable:
        case EventCommand::JumpIfVariableAtLeast:
        case EventCommand::JumpIfVariableAtMost:
            return this->ChangeVariable(run);

        case EventCommand::SetClock:
            owner.party.unk4ACDB0 = this->Word(run, 1);

            if (static_cast<u32>(owner.party.unk4ACDB0) > 0x3E8)
            {
                owner.party.unk4ACDB0 = 0;
            }

            this->Advance(run, 4);
            return true;

        case EventCommand::JumpIfClockReached:
            if (this->Byte(run, 2) > static_cast<u16>(owner.party.unk4ACDB0))
            {
                this->Advance(run, 8);
                return true;
            }

            this->SeekScript(state.leader, this->Dword(run, 4));
            return true;

        case EventCommand::JumpIfClockPassed:
            if (static_cast<u32>(owner.party.unk4ACDB0) <=
                static_cast<u32>(this->Word(run, 1)) + static_cast<u32>(owner.rolePath.timeMark))
            {
                this->Advance(run, 8);
                return true;
            }

            this->SeekScript(state.leader, this->Dword(run, 4));
            this->Advance(run, 8);
            return true;

        case EventCommand::MarkClock:
            owner.rolePath.timeMark = owner.party.unk4ACDB0;
            this->Advance(run, 2);
            return true;

        case EventCommand::RemoveRole:
        case EventCommand::HideRole:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (run.code == EventCommand::RemoveRole)
                {
                    role.flags &= role_flag::kBelowVisible;
                    owner.field.ClearRoleCells(role, leader);
                    const u32 index = state.FindRole(role.id);

                    for (MoveRecord& move : state.moves)
                    {
                        if (static_cast<u32>(move.role) == index)
                        {
                            move.Clear();
                        }
                    }
                }
                else
                {
                    role.flags |= role_flag::kVisible;
                    owner.field.ClearRoleCells(role, leader);
                    role.act.savedAction = -1;
                    role.act.savedDirection = -1;
                }
            }
            else if (run.code == EventCommand::RemoveRole)
            {
                owner.UpdateRoleRecord(this->Word(run, 1), -1, -1, -1, -1, -1, -1, -1, 0, 0x7FFF, -1);
            }
            else
            {
                owner
                    .UpdateRoleRecord(this->Word(run, 1), -1, -1, -1, -1, -1, -1, -1, static_cast<i16>(0x8000), -1, -1);
            }

            this->Advance(run, 4);
            return true;
        }

        case EventCommand::PutRole:
            if (state.FindRoleOrPlayer(this->Word(run, 1), run.found))
            {
                Role& role = state.roles[run.found];
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(0x10 * this->Word(run, 2)),
                    static_cast<u16>(0x10 * this->Word(run, 3)),
                    1,
                    -1,
                    -1,
                    -1
                );
                owner.rolePath.ResumeMove(static_cast<int>(run.found));
                role.flags &= ~role_flag::kHeld;

                if (this->Word(run, 1) == context.id)
                {
                    role.act.lastAction = -1;
                    role.act.lastDirection = -1;
                }
            }
            else
            {
                owner.UpdateRoleRecord(
                    this->Word(run, 1),
                    -1,
                    -1,
                    -1,
                    static_cast<i16>(this->Word(run, 2)),
                    static_cast<i16>(this->Word(run, 3)),
                    -1,
                    -1,
                    0,
                    -1,
                    -1
                );
            }

            this->Advance(run, 8);
            return true;

        case EventCommand::JumpByChoice:
        {
            i32 choice = this->choice;
            i32 count = 0;

            while (this->Dword(run, static_cast<std::size_t>(2 + 4 * count)) != 0xFF00FF00 &&
                   run.cursor + 2 + 4 * static_cast<std::size_t>(count) < script.size())
            {
                ++count;
            }

            if (this->choice > count)
            {
                DebugLog("choice past the list", this->Word(run, 0), this->choice);
                choice = 0;
            }

            this->Jump(run, this->Dword(run, static_cast<std::size_t>(2 + 4 * choice)));
            this->choice = 0;
            return true;
        }

        case EventCommand::LockPlayer:
        case EventCommand::UnlockPlayer:
            if (run.code == EventCommand::LockPlayer)
            {
                Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
                owner.scene.unk4A9920 |= 0x8000;
                player.act.action = 0;

                if (owner.acts.Run(player.act) == 0)
                {
                    LogActError("Act Err(Talk:Rmlock)", player);
                }
            }
            else
            {
                owner.scene.unk4A9920 &= ~0x8000;
            }

            this->Advance(run, 2);
            return true;

        case EventCommand::SetSpeed:
        {
            state.FindRoleOrPlayer(this->EventRoleId(run, this->Word(run, 1)), run.found);

            if (Role* role = this->FoundRole(run))
            {
                role->act.speed = static_cast<i16>(this->Word(run, 2));
                role->act.delay = 0;

                if (owner.acts.Run(role->act) == 0)
                {
                    LogActError("Act Err(Talk:ChangSpd)", *role);
                }
            }

            this->Advance(run, 6);
            return true;
        }

        case EventCommand::SetAct:
        {
            const u16 id = this->EventRoleId(run, this->Word(run, 1));

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (this->Word(run, 2) == 0)
                {
                    DebugLog("act 0", id, 0);
                }

                role.act.id = this->Word(run, 2);
                const std::span<const u8> next = script.subspan(std::min(run.cursor + 6, script.size()));

                if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                {
                    LogActError("Act Err(Talk:ChangAct)", role);
                }

                role.flags |= role_flag::kStepsAct;
            }
            else
            {
                owner
                    .UpdateRoleRecord(id, static_cast<i16>(this->Word(run, 2)), -1, -1, -1, -1, -1, -1, 0x1000, -1, -1);
            }

            this->Advance(run, 6);
            return true;
        }

        case EventCommand::RestoreAct:
        case EventCommand::RestoreAction:
        case EventCommand::RestoreDirection:
        case EventCommand::ClearSpeed:
        {
            state.FindRoleOrPlayer(this->Word(run, 1), run.found);

            if (Role* role = this->FoundRole(run))
            {
                ActState& act = role->act;

                if (run.code == EventCommand::RestoreAct)
                {
                    act.id = act.unknown1C;
                    act.action = act.savedAction;
                    act.direction = act.savedDirection;
                    ResetPlayback(act);
                }
                else if (run.code == EventCommand::RestoreAction)
                {
                    if (act.savedAction != -1)
                    {
                        act.action = act.savedAction;
                        act.savedAction = -1;
                    }
                }
                else if (run.code == EventCommand::RestoreDirection)
                {
                    if (act.savedDirection != -1)
                    {
                        act.direction = act.savedDirection;
                        act.savedDirection = -1;
                    }
                }
                else if (act.speed != -1)
                {
                    act.speed = -1;
                }

                if (owner.acts.Run(act) == 0)
                {
                    LogActError("Act Err(Talk:RestoreOpSet)", *role);
                }
            }

            this->Advance(run, 4);
            return true;
        }

        default:
            return this->UnknownCommand(run);
    }
}

}  // namespace swd3
