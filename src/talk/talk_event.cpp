#include <algorithm>
#include <climits>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "game/game.h"
#include "platform/message_box.h"
#include "talk/script_text.h"
#include "talk/talk.h"
#include "talk/talk_event.h"

namespace swd3
{

namespace
{

constexpr u16 kListEnd = 0xFF00;     // ends the flag lists of JumpIfAllFlags and JumpIfAnyFlag
constexpr u8 kNoCommandByte = 0xFF;  // both bytes of EventCommand::None

}  // namespace

void LogActError(std::u8string_view where, const Role& role)
{
    const std::u8string format = std::u8string(where) + u8" 設定錯誤 GUID[%d]\nact[%d] qq[%d] dir[%d]";
    DebugLog(format, role.id, role.act.id, role.act.action, role.act.direction);
}

// swd32.exe 0x00427920: the event script of `context` (the leader) from where it stopped. On its first frame the
// script is read from Talk<event / 2000 + 1>.dat (0x8000 bytes, the rest 0x0C) once the controlled role stands on a
// tile, and the role of the event is marked (flag 0x80000). The commands run until one waits for the next frame;
// between 0x400 and 0x401 they run on regardless. A missing Talk file ends the game.
// swd32.exe 0x00498290 (EH unwind: the new object freed (operator delete))
// swd32.exe 0x0049829B (EH unwind: the new object freed (operator delete))
// swd32.exe 0x004982A6 (EH handler of 0x00427920)
void Talk::RunEvent(Role& context)
{
    Game& owner = this->game;
    GameState& state = owner.state;

    if (context.id == Role::NoId)
    {
        return;
    }

    Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
    bool loaded = false;

    if (context.eventOffset == 0)
    {
        if (((player.x | player.y) & MapHeader::TileFraction) != 0)
        {
            return;
        }

        if (context.id != TalkWindow::NoRole)
        {
            u32 index = 0;
            state.FindRoleOrPlayer(context.id, index);

            if (index < GameState::Roles)
            {
                state.roles[index].flags |= Role::InEvent;
            }
        }

        const u32 file = context.event / Talk::EventsPerFile;
        const std::u8string path = PathFormat("%sTalk%d.dat", {owner.config.installDirectory, file + 1});
        File& story = owner.databases.story;
        owner.audioDevice.Serve();
        story.Close();

        if (!story.Open(path, File::Disposition::OpenExisting, File::Access::Read))
        {
            DebugLog(u8"SWD3-Talk 找不到 Talk%d", file + 1);  // 0x00427A1F (nullsub_1 with the text 0x004A1C00)
            owner.Destroy();
            return;
        }

        story.SeekBegin(static_cast<i32>(sizeof(u32) * (context.event % Talk::EventsPerFile) + Talk::FileHeaderBytes));
        u32 offset = 0;
        story.ReadDword(offset);
        context.eventOffset = static_cast<i32>(offset);
        context.eventCursor = 0;
        story.SeekBegin(static_cast<i32>(offset + Talk::FileHeaderBytes));
        std::fill(owner.databases.script.begin(), owner.databases.script.end(), Talk::ScriptFill);
        u32 bytes = static_cast<u32>(owner.databases.script.size());
        story.Read(owner.databases.script.data(), bytes);
        owner.scene.playerHold |= Scene::HoldLocked;
        loaded = true;
        owner.audioDevice.Serve();
    }

    EventRun run{context, context.eventCursor};

    for (;;)
    {
        run.code = static_cast<EventCommand>(this->Word(run, kCodeWord) & kCommandMask);
        const bool next = this->RunCommand(run);

        if (run.stopped)
        {
            return;
        }

        this->lastCommand = static_cast<u16>(run.code);

        if (!next && !run.running)
        {
            // The original steps the controlled role's act on loading, before the script's conditions; an event that
            // ends on its first frame would step it twice that frame. Here it steps once the event goes on.
            if (loaded && context.id != Role::NoId && owner.acts.Run(player.act) == 0)
            {
                LogActError(u8"Act Err(Talk:LoadTalk)", player);
            }

            owner.audioDevice.Serve();
            return;
        }
    }
}

Operands::Operands(std::span<const u8> script, std::size_t cursor)
    : script(script),
      cursor(cursor),
      at(cursor + static_cast<std::size_t>(kOpcodeBytes))
{
}

u16 Operands::Word()
{
    const u16 value = TextWord(this->script, this->at);
    this->at += kWordBytes;
    return value;
}

u32 Operands::Dword()
{
    u32 value = 0;

    for (std::size_t index = 0; index < sizeof(u32); ++index)
    {
        if (this->at + index < this->script.size())
        {
            value |= static_cast<u32>(this->script[this->at + index]) << (CHAR_BIT * index);
        }
    }

    this->at += sizeof(u32);
    return value;
}

std::size_t Operands::Index() const
{
    return (this->at - this->cursor) / kWordBytes;
}

// The operands of the command.
Operands Talk::OperandsOf(const EventRun& run) const
{
    return Operands(this->game.databases.script, run.cursor);
}

// Word `index` of the command (0 past the script).
u16 Talk::Word(const EventRun& run, std::size_t index) const
{
    const std::vector<u8>& script = this->game.databases.script;
    return TextWord(script, run.cursor + kWordBytes * index);
}

// The dword at byte `at` of the command.
u32 Talk::Dword(const EventRun& run, std::size_t at) const
{
    const std::vector<u8>& script = this->game.databases.script;
    const std::size_t from = run.cursor + at;
    u32 value = 0;

    for (std::size_t index = 0; index < sizeof(u32); ++index)
    {
        if (from + index < script.size())
        {
            value |= static_cast<u32>(script[from + index]) << (CHAR_BIT * index);
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
    const std::size_t at = run.cursor + kWordBytes * index;

    if (at + 1 < script.size())
    {
        script[at] = static_cast<u8>(value);
        script[at + 1] = static_cast<u8>(value >> CHAR_BIT);
    }
}

// Past the command: the cursor and the offset of the role.
void Talk::Advance(EventRun& run, int bytes)
{
    run.cursor = static_cast<std::size_t>(static_cast<std::ptrdiff_t>(run.cursor) + bytes);
    run.context.eventCursor = static_cast<u16>(run.context.eventCursor + bytes);
}

// Past a command of fixed length (CommandBytes).
void Talk::Next(EventRun& run)
{
    this->Advance(run, CommandBytes(run.code));
}

// The offset of the role set to where the cursor is.
void Talk::SyncOffset(EventRun& run)
{
    run.context.eventCursor = static_cast<u16>(run.cursor);
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
    return run.found < GameState::Roles ? &this->game.state.roles[run.found] : nullptr;
}

// The role of a command: Talk::Current stands for the role of the event.
u16 Talk::EventRoleId(const EventRun& run, u16 id) const
{
    return id == Talk::Current ? run.context.id : id;
}

// A story flag of a command (flags past the 0x400 bytes read as clear and are not set).
bool Talk::StoryFlag(int flag) const
{
    return flag >= 0 && flag < CHAR_BIT * GameState::FlagBytes && this->game.state.GameFlag(flag);
}

void Talk::SetStoryFlag(int flag, bool value)
{
    if (flag < 0 || flag >= CHAR_BIT * GameState::FlagBytes)
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
    // 0x0042B0A2 (nullsub_1 with the title 0x004A1738 and the text 0x004A1430)
    DebugLog(u8"對話系統 沒有指令(%d,%x),(%d,%x)", static_cast<u16>(run.code), this->lastCommand);
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
                this->Next(run);
                run.running = false;
                return false;
            }

            if (code == EventCommand::SkipQuick)
            {
                this->Next(run);
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
            this->Next(run);
            run.running = true;
            return false;
        }

        return code < EventCommand::EndGame ? this->RunStateCommand(run) : this->RunSystemCommand(run);
    }

    if (code == EventCommand::WaitMovieFrame)
    {
        Operands operands = this->OperandsOf(run);
        run.valueA = operands.Word();

        if (this->game.fieldView.movie.frameIndex > run.valueA)
        {
            this->Next(run);
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

    if (id != Role::NoId && id != TalkWindow::NoRole)
    {
        if (state.FindRoleOrPlayer(id, run.found))
        {
            if (run.found >= GameState::Roles)
            {
                // 0x0042D29D (nullsub_1 with the title 0x004A01B4 and the text 0x004A01C8)
                DebugLog(
                    u8"對話系統(TalkEnd) GUID(%d)\n陣列索引錯誤(crID %d) (manMax %d) ",
                    run.found,
                    state.roleCount
                );
            }

            Role& role = state.roles[run.found];
            owner.rolePath.ResumeMove(static_cast<int>(run.found));
            role.flags &= ~Role::Held;

            if ((role.flags & Role::FacesTalker) != 0)
            {
                if (role.act.savedAction != ActState::NotSaved)
                {
                    role.act.action = role.act.savedAction;
                }

                if (role.act.savedDirection != ActState::NotSaved)
                {
                    role.act.direction = role.act.savedDirection;
                }
            }

            role.act.savedAction = ActState::NotSaved;
            role.act.savedDirection = ActState::NotSaved;

            if (owner.acts.Run(role.act) == 0)
            {
                LogActError(u8"Act Err(Talk:Talkend)", role);
            }

            if ((role.flags & Role::InEvent) != 0)
            {
                owner.rolePath.ResumeMove(static_cast<int>(run.found));
                role.flags &= ~Role::Held;
            }
        }
        else if (state.GameFlag(GameState::DebugReports) && run.found >= GameState::Roles)
        {
            DebugLog(u8"G(%d)啟動的角色不在地圖上(%d) ", id, owner.currentMap);  // 0x0042D3AA
        }
    }

    for (int index = 0; index < state.roleCount && index < GameState::Roles; ++index)
    {
        state.roles[static_cast<std::size_t>(index)].act.savedAction = ActState::NotSaved;
        state.roles[static_cast<std::size_t>(index)].act.savedDirection = ActState::NotSaved;
    }

    for (MoveRecord& move : state.moves)
    {
        run.found = move.role;

        if (move.role != MoveRecord::NoRole && move.Kind() > MoveKind::Path)
        {
            move.Clear();

            if (run.found < GameState::Roles)
            {
                state.roles[run.found].pathNumber = 0;
                state.roles[run.found].pathStep = 0;
            }
        }
    }

    DebugLog(u8"TalkEnd [%d]", run.context.event);
    owner.scene.playerHold &= ~Scene::HoldLocked;
    // The script buffer starts with an empty command word.
    owner.databases.script[0] = kNoCommandByte;
    owner.databases.script[1] = kNoCommandByte;
    state.leader.FillUnused();
    owner.counterWait = 0;
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
    run.found = TalkWindow::NoRole;
    const u16 speaker = this->EventRoleId(run, this->Word(run, kFirstOperand));

    if (speaker != TalkWindow::NoRole)
    {
        state.FindRoleOrPlayer(speaker, run.found);
    }

    this->SetWord(run, kFirstOperand, speaker);
    auto window = std::make_unique<TalkWindow>();

    if (speaker == TalkWindow::NoRole)
    {
        if (context.id != TalkWindow::NoRole)
        {
            DebugLog(u8"不是從劇情點啟動 !不要用R_TRIG!");  // wsprintfA of 0x004A1BBC, then nullsub_1
        }

        window->fromX = static_cast<u16>(context.x);
        window->fromY = static_cast<u16>(context.y);
    }

    if ((this->dialogOrigin & Talk::OriginXMask) != Talk::NoOriginX)
    {
        window->fromX = static_cast<u16>(this->dialogOrigin + static_cast<u32>(field.ScreenLeft()));
        window->fromY =
            static_cast<u16>((this->dialogOrigin >> Talk::OriginYShift) + static_cast<u32>(field.ScreenTop()));
    }

    u32 options = 0;

    if (code >= EventCommand::SizedDialogWait && code <= EventCommand::PlainDialog)
    {
        options = Talk::SizedLayout;
    }

    if (code == EventCommand::RecordDialogWait || code == EventCommand::RecordDialog)
    {
        options = Talk::RecordLayout;
    }

    window->flags = code == EventCommand::PlainDialogWait || code == EventCommand::PlainDialog ? TalkWindow::Fade : 0;
    const u32 settings = this->dialogOptions;

    if ((settings & Talk::Skippable) == 0)
    {
        window->flags |= TalkWindow::Skippable;
    }

    if (this->dialogTimed == 1)
    {
        window->timeout = this->dialogTimeout;
        window->flags |= TalkWindow::Timed;
    }

    if ((settings & Talk::NoAdvance) == 0)
    {
        window->flags |= TalkWindow::NoAdvance;
    }

    if ((settings & Talk::Overlay) == 0)
    {
        window->flags |= TalkWindow::Overlay;
    }

    if ((settings & Talk::ShowMark) == 0)
    {
        window->flags |= TalkWindow::ShowMark;
    }

    if ((settings & Talk::Offset) == 0)
    {
        options |= Talk::UseOffset;
    }

    if ((settings & Talk::SingleLine) == 0)
    {
        options |= Talk::OneLine;
    }

    if ((settings & Talk::Logged) == 0)
    {
        options |= Talk::LogText;
    }

    if (this->dialogCentred == 1)
    {
        options |= Talk::Centred;
    }

    owner.audioDevice.Serve();
    const std::vector<u8>& script = owner.databases.script;
    const std::size_t recordAt = std::min(run.cursor + kOpcodeBytes, script.size());
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

    if (run.found == TalkWindow::NoRole)
    {
        context.talkState = Role::TalkOpen;
    }
    else if (role != nullptr)
    {
        role->talkState = Role::TalkOpen;
    }

    if (WaitsForWindow(code))
    {
        window->flags |= TalkWindow::Blocking;

        if (run.found == TalkWindow::NoRole)
        {
            context.talkState = Role::TalkWaiting;
        }
        else if (role != nullptr)
        {
            role->talkState = Role::TalkWaiting;
        }

        ++owner.scene.playerHold;
    }

    this->AppendWindow(std::move(window));
    std::size_t at = run.cursor + kOpcodeBytes;

    while (at < script.size() && TextWord(script, at) != kTextEnd)
    {
        ++at;
    }

    run.cursor = at + kWordBytes;
    this->SyncOffset(run);
    this->dialogOrigin = Talk::NoOrigin;
    this->dialogOptions = Talk::AllOff;
    this->dialogTimeout = Talk::DefaultTimeout;
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
    constexpr u16 kCountMask = 0x3FFF;
    constexpr u16 kSent = 0x4000;  // the moves were started
    constexpr std::size_t kCountWord = kFirstOperand;
    constexpr std::size_t kEntriesAt = kCountWord + 1;
    constexpr std::size_t kColumnWord = 1;  // the words of an entry after its role
    constexpr std::size_t kRowWord = 2;
    constexpr std::size_t kActWord = 3;
    constexpr std::size_t kActionWord = 4;
    constexpr std::size_t kDirectionWord = 5;
    constexpr std::size_t kTileWords = 3;    // a role, a column and a row
    constexpr std::size_t kActingWords = 6;  // and the act, action and direction
    const u16 header = this->Word(run, kCountWord);
    const std::size_t entry = run.code == EventCommand::MoveRoles ? kTileWords : kActingWords;
    const u32 count = header & kCountMask;

    if ((header & kSent) == 0)
    {
        run.valueA = header & RolePath::EventGoesOn;
        std::size_t at = kEntriesAt;

        for (u32 index = 0; index < count; ++index, at += entry)
        {
            const u16 id = this->Word(run, at);

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog(u8"Bmove 無效的角色\nGUID %d", id);  // the text 0x004A174C
                continue;
            }

            Role& role = state.roles[run.found];

            if (id == run.context.id)
            {
                role.flags &= ~Role::InEvent;
                role.act.lastAction = ActState::NoLast;
                role.act.lastDirection = ActState::NoLast;
            }

            if (role.act.speed == 0)
            {
                role.act.speed = static_cast<i16>(ActState::FixedDelay | 1);  // a fixed delay of 1 frame
            }

            const u16 column = this->Word(run, at + kColumnWord);
            const u16 row = this->Word(run, at + kRowWord);
            const u16 tileY =
                row == Talk::Keep ? static_cast<u16>(static_cast<u32>(player.y) >> MapHeader::TileShift) : row;
            const u16 tileX =
                column == Talk::Keep ? static_cast<u16>(static_cast<u32>(player.x) >> MapHeader::TileShift) : column;

            if (run.code == EventCommand::MoveRoles)
            {
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(MapHeader::TileSize * tileX),
                    static_cast<u16>(MapHeader::TileSize * tileY),
                    static_cast<u32>(run.valueA),
                    RolePath::Keep,
                    RolePath::Keep,
                    RolePath::Keep
                );
            }
            else
            {
                const auto act = static_cast<i16>(this->Word(run, at + kActWord));
                const auto action = static_cast<i16>(this->Word(run, at + kActionWord));
                const auto direction = static_cast<i16>(this->Word(run, at + kDirectionWord));
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(MapHeader::TileSize * tileX),
                    static_cast<u16>(MapHeader::TileSize * tileY),
                    static_cast<u32>(run.valueA),
                    act,
                    action,
                    direction
                );
            }

            if (static_cast<i32>(run.found) == state.playerRole)
            {
                owner.scene.playerHold |= Scene::HoldLocked;
            }
        }

        this->SetWord(run, kCountWord, static_cast<u16>(header | kSent));
        return false;
    }

    u32 arrived = 0;
    std::size_t at = kEntriesAt;

    for (u32 index = 0; index < count; ++index, at += entry)
    {
        state.FindRoleOrPlayer(this->Word(run, at), run.found);
        Role* role = this->FoundRole(run);

        if (role == nullptr)
        {
            continue;
        }

        if ((role->flags & Role::Arrived) != 0 ||
            owner.rolePath.ScriptStep(static_cast<int>(run.found)) == RolePath::PathEnded)
        {
            ++arrived;
        }
    }

    if (arrived != count)
    {
        return false;
    }

    this->SetWord(run, kCountWord, static_cast<u16>(header & kCountMask));
    run.valueA = static_cast<i32>(kWordBytes * entry);
    this->Advance(run, static_cast<int>(kWordBytes * (kEntriesAt + count * entry)));
    return true;
}

// Commands 0x1D..0x21 and 0xB5..0xB9: a script variable (0..0x3F) set, raised, lowered (not below 0) or compared
// (a jump when at least or at most the value); the second set takes a dword. The gold never stays negative.
bool Talk::ChangeVariable(EventRun& run)
{
    Party& party = this->game.party;
    const bool wide = run.code > EventCommand::JumpIfVariableAtMost;
    Operands operands = this->OperandsOf(run);
    const i32 index = static_cast<i16>(operands.Word());
    const i32 value = wide ? static_cast<i32>(operands.Dword()) : static_cast<i16>(operands.Word());
    const int length = CommandBytes(run.code);
    run.valueA = index;
    run.valueB = value;

    if (index < 0 || index >= Party::Variables)
    {
        DebugLog(u8"對話系統 變數不合法(%d)(%d)\n", index, value);  // the title 0x004A1738, the text 0x0049F7C4
        return false;
    }

    i32& variable = party.variables[static_cast<std::size_t>(index)];
    bool jumped = false;

    switch (run.code)
    {
        case EventCommand::SetVariable:
        case EventCommand::SetVariableWide:
            variable = value;
            break;

        case EventCommand::AddVariable:
        case EventCommand::AddVariableWide:
            variable += value;
            break;

        case EventCommand::SubtractVariable:
        case EventCommand::SubtractVariableWide:
            variable -= value;

            if (variable < 0)
            {
                variable = 0;
            }

            break;

        case EventCommand::JumpIfVariableAtLeast:
        case EventCommand::JumpIfVariableAtLeastWide:
        case EventCommand::JumpIfVariableAtMost:
        case EventCommand::JumpIfVariableAtMostWide:
        {
            const u32 target = operands.Dword();
            const bool jumps =
                run.code == EventCommand::JumpIfVariableAtLeast || run.code == EventCommand::JumpIfVariableAtLeastWide
                    ? variable >= value
                    : variable <= value;

            if (jumps)
            {
                this->Jump(run, target);
                jumped = true;
            }

            break;
        }

        default:
            break;
    }

    if (party.variables[Party::Gold] < 0)
    {
        party.variables[Party::Gold] = 0;
    }

    const int advance = jumped ? 0 : length;
    run.found = static_cast<u32>(advance);
    this->Advance(run, advance);
    return true;
}

// Commands 0x01..0x31: the talk windows, the roles' acts, places and moves, jumps on flags, choices and the clock.
bool Talk::RunRoleCommand(EventRun& run)
{
    Game& owner = this->game;
    GameState& state = owner.state;
    Role& context = run.context;
    const std::span<const u8> script(owner.databases.script);
    const i32 leader = owner.partyLeaderId;

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
            this->dialogOptions &= ~Talk::Skippable;
            this->Next(run);
            return true;

        case EventCommand::DialogTimeout:
            this->dialogTimed = 1;
            this->dialogTimeout = this->OperandsOf(run).Word();
            this->Next(run);
            return true;

        case EventCommand::DialogNoAdvance:
            this->dialogOptions &= ~Talk::NoAdvance;
            this->Next(run);
            return true;

        case EventCommand::SetAction:
        case EventCommand::SetDirection:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = this->EventRoleId(run, operands.Word());
            const u16 value = operands.Word();

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                const std::span<const u8> next =
                    script.subspan(std::min(run.cursor + CommandBytes(run.code), script.size()));

                if (run.code == EventCommand::SetAction)
                {
                    role.act.delay = 0;
                    role.act.action = value;

                    if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                    {
                        LogActError(u8"Act Err(Talk:ChangQQ)", role);
                    }
                }
                else
                {
                    role.act.direction = value;
                    role.act.delay = 0;

                    if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                    {
                        LogActError(u8"Act Err(Talk:ChangDir)", role);
                    }

                    role.flags |= Role::StepsAct;
                }
            }
            else if (run.code == EventCommand::SetAction)
            {
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    static_cast<i16>(value),
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Role::StepsAct,
                    Game::KeepValue,
                    Game::KeepValue
                );
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(value),
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

        case EventCommand::PutRoleOnTile:
        {
            Operands operands = this->OperandsOf(run);
            const u16 operand = operands.Word();
            const u16 id = this->EventRoleId(run, operand);

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                DebugLog(u8"setpos 無效的角色\nGUID[%d]", id);  // nullsub_1 with the text 0x004A1B78
                this->Next(run);

                if (static_cast<i32>(run.found) == state.playerRole)
                {
                    owner.scene.playerHold |= Scene::HoldLocked;
                }

                return true;
            }

            Role& role = state.roles[run.found];
            const u16 column = operands.Word();
            const u16 row = operands.Word();

            if (operand == context.id)
            {
                role.act.lastAction = ActState::NoLast;
                role.act.lastDirection = ActState::NoLast;
                role.flags &= ~Role::InEvent;
            }

            owner.rolePath.MoveRoleTo(
                static_cast<int>(run.found),
                static_cast<u16>(MapHeader::TileSize * column),
                static_cast<u16>(MapHeader::TileSize * row),
                RolePath::WalkThere,
                RolePath::Keep,
                RolePath::Keep,
                RolePath::Keep
            );
            this->Next(run);

            if (static_cast<i32>(run.found) == state.playerRole)
            {
                owner.scene.playerHold |= Scene::HoldLocked;
            }

            return true;
        }

        case EventCommand::StepRole:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = this->EventRoleId(run, operands.Word());

            if (state.FindRoleOrPlayer(id, run.found))
            {
                if ((state.roles[run.found].flags & Role::Arrived) == 0)
                {
                    owner.rolePath.ScriptStep(static_cast<int>(run.found));
                }
            }
            else
            {
                DebugLog(
                    u8"Talk(Step) 角色號碼(GUID:%d)不存在此地圖(%d)",
                    id,
                    owner.currentMap
                );  // the text 0x004A1B4C
            }

            this->Next(run);
            return false;
        }

        case EventCommand::WaitDialogClosed:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = this->EventRoleId(run, operands.Word());

            if (id == TalkWindow::NoRole)
            {
                if (context.talkState == Role::NoTalk)
                {
                    this->Next(run);
                }

                return false;
            }

            state.FindRoleOrPlayer(id, run.found);
            const Role* role = this->FoundRole(run);

            if (role != nullptr && role->talkState != Role::NoTalk)
            {
                return false;
            }

            this->Next(run);
            return false;
        }

        case EventCommand::Jump:
        {
            const u32 target = this->OperandsOf(run).Dword();
            this->Jump(run, target);
            return true;
        }

        case EventCommand::JumpIfMoveIdle:
        case EventCommand::JumpIfMoveRunning:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = operands.Word();
            const u32 target = operands.Dword();
            state.FindRoleOrPlayer(id, run.found);
            const Role* role = this->FoundRole(run);
            const u32 moving = role != nullptr ? role->flags & Role::Stepping : 0;
            int slot = 0;

            while (slot < GameState::Moves)
            {
                const MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

                if (move.role == run.found && move.Kind() == MoveKind::Scripted &&
                    (run.code == EventCommand::JumpIfMoveIdle ? moving == 0 : moving != 0))
                {
                    break;
                }

                ++slot;
            }

            if (slot == GameState::Moves)
            {
                this->Next(run);
                return true;
            }

            this->Jump(run, target);
            run.running = false;
            return true;
        }

        case EventCommand::ResumeMove:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = operands.Word();
            state.FindRoleOrPlayer(id, run.found);

            if (run.found == GameState::NotFound || owner.rolePath.ResumeMove(static_cast<int>(run.found)))
            {
                this->Next(run);
            }

            if (Role* role = this->FoundRole(run))
            {
                role->flags &= ~Role::Held;
                role->act.delay = 0;
            }

            return true;
        }

        case EventCommand::ResumeAllMoves:
            for (int index = 1; index < state.roleCount && index < GameState::Roles; ++index)
            {
                Role& role = state.roles[static_cast<std::size_t>(index)];

                if ((role.flags & Role::Held) != 0)
                {
                    owner.rolePath.ResumeMove(index);
                    role.flags &= ~Role::Held;
                    role.act.delay = 0;
                }
            }

            this->Next(run);
            return true;

        case EventCommand::MoveRoles:
            return this->MoveRoles(run);

        case EventCommand::JumpIfFlag:
        case EventCommand::JumpIfNotFlag:
        {
            Operands operands = this->OperandsOf(run);
            const u16 flag = operands.Word();
            const u32 target = operands.Dword();

            if (this->StoryFlag(flag) != (run.code == EventCommand::JumpIfNotFlag))
            {
                this->Jump(run, target);
                return true;
            }

            this->Next(run);
            return true;
        }

        case EventCommand::JumpIfAllFlags:
        case EventCommand::JumpIfAnyFlag:
        {
            constexpr std::size_t kFlagsAt = 1;  // the flags, ended by kListEnd, then the target
            std::size_t count = 0;
            std::size_t set = 0;

            for (u16 flag = this->Word(run, kFlagsAt); flag != kListEnd; flag = this->Word(run, kFlagsAt + count))
            {
                if (run.cursor + kWordBytes * (kFlagsAt + count) + 1 >= script.size())
                {
                    break;
                }

                run.found = static_cast<u32>(flag / CHAR_BIT);  // the byte of the flag

                if (this->StoryFlag(flag))
                {
                    ++set;
                }

                ++count;
            }

            const bool jumps = run.code == EventCommand::JumpIfAllFlags ? count == set : set != 0;

            if (jumps)
            {
                this->Jump(run, this->Dword(run, kWordBytes * (kFlagsAt + count + 1)));
                return true;
            }

            this->Advance(run, static_cast<int>(kWordBytes * (kFlagsAt + count + 1) + sizeof(u32)));
            return true;
        }

        case EventCommand::SetFlag:
        case EventCommand::ClearFlag:
        {
            Operands operands = this->OperandsOf(run);
            const u16 flag = operands.Word();
            this->SetStoryFlag(flag, run.code == EventCommand::SetFlag);
            DebugLog(run.code == EventCommand::SetFlag ? u8"fON [%d]" : u8"fOFF[%d]", flag);
            this->Next(run);
            return true;
        }

        case EventCommand::ChangeMap:
        {
            Operands operands = this->OperandsOf(run);
            const u16 map = operands.Word();
            const u16 column = operands.Word();
            const u16 row = operands.Word();
            const u16 act = operands.Word();
            const u16 action = operands.Word();
            const u16 direction = operands.Word();
            owner.ChangeMap(map, column, row, act, action, direction, Game::SaveRoles);
            this->Next(run);
            return true;
        }

        case EventCommand::SetRoleTalk:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = operands.Word();
            const u16 talk = operands.Word();

            if (!state.FindRoleOrPlayer(id, run.found))
            {
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(talk),
                    Role::StepsAct,
                    Game::KeepValue,
                    Game::KeepValue
                );
                this->Next(run);
                return false;
            }

            Role& role = state.roles[run.found];

            if (role.label.has_value())
            {
                role.label.reset();
                role.labelColor = 0;
            }

            int slot = 0;

            while (slot < GameState::Moves && state.moves[static_cast<std::size_t>(slot)].role != run.found)
            {
                ++slot;
            }

            if (slot < GameState::Moves)
            {
                MoveRecord& move = state.moves[static_cast<std::size_t>(slot)];

                if (move.Kind() == MoveKind::Scripted)
                {
                    move.nextRole = MoveRecord::NoRole;
                    move.unk0A = MoveRecord::Unused;
                    move.nextX = MoveRecord::Unused;
                    move.nextY = MoveRecord::Unused;
                }

                if (move.Kind() == MoveKind::Path)
                {
                    i32 aheadX = 0;
                    i32 aheadY = 0;

                    // The rebuild's smooth motion: a role between tiles walks on to its tile ahead instead.
                    if (!owner.fieldView.FinishToTile(run.found, move.CurrentDirection(), aheadX, aheadY) &&
                        ((role.x & MapHeader::TileFraction) != 0 || (role.y & MapHeader::TileFraction) != 0))
                    {
                        const int row = static_cast<int>(static_cast<u32>(role.y) >> MapHeader::TileShift) - 1;

                        if ((role.flags & Role::Solid) == 0)
                        {
                            owner.field.ClearRoleCells(role, leader);
                        }

                        const u8 back = move.CurrentDirection();

                        if (back < RolePath::Directions)
                        {
                            role.x = RolePath::TileStart(role.x, RolePath::StepX[back]);
                            role.y = RolePath::TileStart(role.y, RolePath::StepY[back]);
                        }

                        owner.field.RemoveRole(role.id, static_cast<int>(role.flags & Role::LayerMask), row, 0);
                    }

                    move.Clear();
                }
            }

            role.flags |= Role::StepsAct;
            role.pathNumber = talk;
            role.pathStep = 0;
            this->Next(run);
            return false;
        }

        case EventCommand::SetVariable:
        case EventCommand::AddVariable:
        case EventCommand::SubtractVariable:
        case EventCommand::JumpIfVariableAtLeast:
        case EventCommand::JumpIfVariableAtMost:
            return this->ChangeVariable(run);

        case EventCommand::SetClock:
            owner.party.clock = this->OperandsOf(run).Word();

            if (static_cast<u32>(owner.party.clock) > Party::ClockLimit)
            {
                owner.party.clock = 0;
            }

            this->Next(run);
            return true;

        case EventCommand::JumpIfClockReached:
        {
            const u8 time = this->Byte(run, kOpcodeBytes);  // the low byte of the first operand
            const u32 target = this->Dword(run, kOpcodeBytes + kWordBytes);

            if (time > static_cast<u16>(owner.party.clock))
            {
                this->Next(run);
                return true;
            }

            this->SeekScript(state.leader, target);
            return true;
        }

        case EventCommand::JumpIfClockPassed:
        {
            Operands operands = this->OperandsOf(run);
            const u16 time = operands.Word();
            const u32 target = operands.Dword();

            if (static_cast<u32>(owner.party.clock) <=
                static_cast<u32>(time) + static_cast<u32>(owner.rolePath.timeMark))
            {
                this->Next(run);
                return true;
            }

            this->SeekScript(state.leader, target);
            this->Next(run);
            return true;
        }

        case EventCommand::MarkClock:
            owner.rolePath.timeMark = owner.party.clock;
            this->Next(run);
            return true;

        case EventCommand::RemoveRole:
        case EventCommand::HideRole:
        {
            Operands operands = this->OperandsOf(run);
            const u16 operand = operands.Word();
            const u16 id = this->EventRoleId(run, operand);

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (run.code == EventCommand::RemoveRole)
                {
                    role.flags &= Role::BelowVisible;
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
                    role.flags |= Role::Visible;
                    owner.field.ClearRoleCells(role, leader);
                    role.act.savedAction = ActState::NotSaved;
                    role.act.savedDirection = ActState::NotSaved;
                }
            }
            else if (run.code == EventCommand::RemoveRole)
            {
                owner.UpdateRoleRecord(
                    operand,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    0,
                    Role::BelowVisible,
                    Game::KeepValue
                );
            }
            else
            {
                owner.UpdateRoleRecord(
                    operand,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(Role::Visible),
                    Game::KeepValue,
                    Game::KeepValue
                );
            }

            this->Next(run);
            return true;
        }

        case EventCommand::PutRole:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = operands.Word();
            const u16 column = operands.Word();
            const u16 row = operands.Word();

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];
                owner.rolePath.MoveRoleTo(
                    static_cast<int>(run.found),
                    static_cast<u16>(MapHeader::TileSize * column),
                    static_cast<u16>(MapHeader::TileSize * row),
                    RolePath::PlaceThere,
                    RolePath::Keep,
                    RolePath::Keep,
                    RolePath::Keep
                );
                owner.rolePath.ResumeMove(static_cast<int>(run.found));
                role.flags &= ~Role::Held;

                if (id == context.id)
                {
                    role.act.lastAction = ActState::NoLast;
                    role.act.lastDirection = ActState::NoLast;
                }
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    Game::KeepValue,
                    Game::KeepValue,
                    Game::KeepValue,
                    static_cast<i16>(column),
                    static_cast<i16>(row),
                    Game::KeepValue,
                    Game::KeepValue,
                    0,
                    Game::KeepValue,
                    Game::KeepValue
                );
            }

            this->Next(run);
            return true;
        }

        case EventCommand::JumpByChoice:
        {
            auto targetAt = [](i32 index)
            {
                return kOpcodeBytes + sizeof(u32) * static_cast<std::size_t>(index);
            };

            i32 choice = this->choice;
            i32 count = 0;

            while (this->Dword(run, targetAt(count)) != kTargetsEnd && run.cursor + targetAt(count) < script.size())
            {
                ++count;
            }

            if (this->choice > count)
            {
                DebugLog(
                    u8"對話系統(BTN_JUMP) GUID(%d) 輸入值(%d)大於分支數量",
                    this->Word(run, kCodeWord),
                    this->choice
                );
                choice = 0;
            }

            this->Jump(run, this->Dword(run, targetAt(choice)));
            this->choice = 0;
            return true;
        }

        case EventCommand::LockPlayer:
        case EventCommand::UnlockPlayer:
            if (run.code == EventCommand::LockPlayer)
            {
                Role& player = state.roles[static_cast<std::size_t>(state.playerRole)];
                owner.scene.playerHold |= Scene::HoldLocked;
                player.act.action = 0;

                if (owner.acts.Run(player.act) == 0)
                {
                    LogActError(u8"Act Err(Talk:Rmlock)", player);
                }
            }
            else
            {
                owner.scene.playerHold &= ~Scene::HoldLocked;
            }

            this->Next(run);
            return true;

        case EventCommand::SetSpeed:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = this->EventRoleId(run, operands.Word());
            const auto speed = static_cast<i16>(operands.Word());
            state.FindRoleOrPlayer(id, run.found);

            if (Role* role = this->FoundRole(run))
            {
                role->act.speed = speed;
                role->act.delay = 0;

                if (owner.acts.Run(role->act) == 0)
                {
                    LogActError(u8"Act Err(Talk:ChangSpd)", *role);
                }
            }

            this->Next(run);
            return true;
        }

        case EventCommand::SetAct:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = this->EventRoleId(run, operands.Word());
            const u16 act = operands.Word();

            if (state.FindRoleOrPlayer(id, run.found))
            {
                Role& role = state.roles[run.found];

                if (act == 0)
                {
                    DebugLog(u8"對話系統(Chanmg_act) 請設定正確的圖列碼GUID(%d)act(%d)", id, 0);
                }

                role.act.id = act;
                const std::span<const u8> next =
                    script.subspan(std::min(run.cursor + CommandBytes(run.code), script.size()));

                if (!this->CommandsRole(next, run.found) && owner.acts.Run(role.act) == 0)
                {
                    LogActError(u8"Act Err(Talk:ChangAct)", role);
                }

                role.flags |= Role::StepsAct;
            }
            else
            {
                owner.UpdateRoleRecord(
                    id,
                    static_cast<i16>(act),
                    Game::KeepValue,
                    Game::KeepValue,
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

        case EventCommand::RestoreAct:
        case EventCommand::RestoreAction:
        case EventCommand::RestoreDirection:
        case EventCommand::ClearSpeed:
        {
            Operands operands = this->OperandsOf(run);
            const u16 id = operands.Word();
            state.FindRoleOrPlayer(id, run.found);

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
                    if (act.savedAction != ActState::NotSaved)
                    {
                        act.action = act.savedAction;
                        act.savedAction = ActState::NotSaved;
                    }
                }
                else if (run.code == EventCommand::RestoreDirection)
                {
                    if (act.savedDirection != ActState::NotSaved)
                    {
                        act.direction = act.savedDirection;
                        act.savedDirection = ActState::NotSaved;
                    }
                }
                else if (act.speed != -1)
                {
                    act.speed = -1;
                }

                if (owner.acts.Run(act) == 0)
                {
                    LogActError(u8"Act Err(Talk:RestoreOpSet)", *role);
                }
            }

            this->Next(run);
            return true;
        }

        default:
            return this->UnknownCommand(run);
    }
}

}  // namespace swd3
