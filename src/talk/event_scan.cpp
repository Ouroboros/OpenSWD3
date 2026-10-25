#include "talk/event_scan.h"

#include <algorithm>
#include <span>

#include "core/format.h"
#include "file/file.h"
#include "party/party.h"
#include "talk/event_command.h"
#include "talk/script_text.h"
#include "talk/talk.h"

namespace swd3
{

namespace
{

constexpr u16 kListEnd = 0xFF00;              // ends the flag lists of 0x17 and 0x18 and the offsets of 0x3F
constexpr u16 kCountMask = 0x3FFF;            // the role count of 0x14 and 0xA9 (bits 14 and 15 mark its progress)
constexpr std::size_t kMoveBytes = 6;         // a role of 0x14 and of 0x74: role, x, y
constexpr std::size_t kActingMoveBytes = 12;  // a role of 0xA9: role, x, y, act, action, direction
constexpr std::size_t kDwordBytes = 4;

u32 ReadDword(std::span<const u8> bytes, std::size_t at)
{
    return TextWord(bytes, at) | static_cast<u32>(TextWord(bytes, at + kWordBytes)) << 16;
}

// The bytes of the command at `at` as the interpreter moves past it; 0 for one it cannot tell.
std::size_t ScannedBytes(std::span<const u8> bytes, std::size_t at, EventCommand code)
{
    const std::size_t operands = at + kOpcodeBytes;
    std::size_t cursor = operands;

    switch (code)
    {
        case EventCommand::DialogWait:
        case EventCommand::Dialog:
        case EventCommand::SizedDialogWait:
        case EventCommand::SizedDialog:
        case EventCommand::PlainDialogWait:
        case EventCommand::PlainDialog:
        case EventCommand::RecordDialogWait:
        case EventCommand::RecordDialog:
        case EventCommand::PlayVideo:
        case EventCommand::MapMovie:
        case EventCommand::AddLog:
            return TextEndAt(bytes, operands) + kWordBytes - at;

        case EventCommand::CopyMedia:
        case EventCommand::DeleteMedia:
            return TextEndAt(bytes, operands + kWordBytes) + kWordBytes - at;

        case EventCommand::Jump:
            return kOpcodeBytes + kDwordBytes;

        case EventCommand::OpenScript:
            return kOpcodeBytes + kWordBytes;

        case EventCommand::MoveRoles:
            return kOpcodeBytes + kWordBytes + kMoveBytes * (TextWord(bytes, operands) & kCountMask);

        case EventCommand::MoveRolesActing:
            return kOpcodeBytes + kWordBytes + kActingMoveBytes * (TextWord(bytes, operands) & kCountMask);

        case EventCommand::PutRoles:
            return kOpcodeBytes + kWordBytes + kMoveBytes * TextWord(bytes, operands);

        case EventCommand::StepRoles:
            return kOpcodeBytes + kWordBytes + kWordBytes * TextWord(bytes, operands);

        case EventCommand::JumpIfAllFlags:
        case EventCommand::JumpIfAnyFlag:
            while (cursor + 1 < bytes.size() && TextWord(bytes, cursor) != kListEnd)
            {
                cursor += kWordBytes;
            }

            return cursor + kWordBytes + kDwordBytes - at;

        case EventCommand::Quake:
            cursor += kWordBytes;

            while (cursor + 1 < bytes.size() && TextWord(bytes, cursor) != kListEnd)
            {
                cursor += kWordBytes;
            }

            return cursor + kWordBytes - at;

        case EventCommand::JumpByChoice:
        case EventCommand::RandomJump:
            while (cursor + kDwordBytes <= bytes.size() && ReadDword(bytes, cursor) != kTargetsEnd)
            {
                cursor += kDwordBytes;
            }

            return cursor + kDwordBytes - at;

        case EventCommand::ItemMenu:
            while (cursor + 1 < bytes.size() && TextWord(bytes, cursor) != 0)
            {
                cursor += kWordBytes;
            }

            return cursor + kWordBytes - at;

        default:
            return static_cast<std::size_t>(CommandBytes(code));
    }
}

}  // namespace

bool EventScan::GivesItem(const std::u8string& directory, u32 event)
{
    if (const auto found = this->items.find(event); found != this->items.end())
    {
        return found->second;
    }

    const bool gives = this->Scan(
        directory,
        event,
        0,
        [](EventCommand code, std::span<const u8> command)
        {
            // Gold given: an amount above 0 added to variable 0 (a word, or a dword for the wide command); a payment
            // adds a negative amount. An item given: a count above 0 (a negative count takes the item).
            constexpr std::size_t kAmountAt = kOpcodeBytes + kWordBytes;
            constexpr std::size_t kCountAt = kOpcodeBytes + kWordBytes;
            const bool adds = code == EventCommand::AddVariable || code == EventCommand::AddVariableWide;
            const i32 amount = code == EventCommand::AddVariableWide ? static_cast<i32>(ReadDword(command, kAmountAt))
                                                                     : static_cast<i16>(TextWord(command, kAmountAt));
            const bool gold = adds && TextWord(command, kOpcodeBytes) == Party::Gold && amount > 0;
            const bool item = code == EventCommand::AddItem && static_cast<i16>(TextWord(command, kCountAt)) > 0;
            return item || gold;
        }
    );
    this->items.emplace(event, gives);
    return gives;
}

bool EventScan::TakesItself(const std::u8string& directory, u32 event, u16 role)
{
    const u64 key = static_cast<u64>(event) << 16 | role;

    if (const auto found = this->taken.find(key); found != this->taken.end())
    {
        return found->second;
    }

    const bool takes = this->Scan(
        directory,
        event,
        0,
        [role](EventCommand code, std::span<const u8> command)
        {
            const bool acts = code == EventCommand::RemoveRole || code == EventCommand::RoleNoTalkPointer ||
                              code == EventCommand::SetAction;
            const u16 target = TextWord(command, kOpcodeBytes);
            return acts && (target == Talk::Current || target == role);
        }
    );
    this->taken.emplace(key, takes);
    return takes;
}

const std::vector<u16>& EventScan::EnteredMaps(const std::u8string& directory, u32 event)
{
    if (const auto found = this->maps.find(event); found != this->maps.end())
    {
        return found->second;
    }

    std::vector<u16> entered;
    this->Scan(
        directory,
        event,
        0,
        [&entered](EventCommand code, std::span<const u8> command)
        {
            if (code == EventCommand::ChangeMap)
            {
                const u16 map = TextWord(command, kOpcodeBytes);

                if (std::find(entered.begin(), entered.end(), map) == entered.end())
                {
                    entered.push_back(map);
                }
            }

            return false;
        }
    );
    return this->maps.emplace(event, std::move(entered)).first->second;
}

bool EventScan::Scan(const std::u8string& directory, u32 event, int depth, const Found& found)
{
    const std::size_t file = event / Talk::EventsPerFile;

    if (file >= EventScan::Files)
    {
        return false;
    }

    const std::span<const u8> bytes = this->Script(directory, file);
    const std::size_t table = Talk::FileHeaderBytes;
    const u32 offset = ReadDword(bytes, table + kDwordBytes * (event % Talk::EventsPerFile));
    std::size_t end = bytes.size();

    for (u32 index = 0; index < Talk::EventsPerFile; ++index)
    {
        const u32 other = ReadDword(bytes, table + kDwordBytes * index);

        if (other > offset)
        {
            end = std::min(end, Talk::FileHeaderBytes + static_cast<std::size_t>(other));
        }
    }

    std::size_t at = Talk::FileHeaderBytes + offset;

    while (at + kOpcodeBytes <= end)
    {
        const auto code = static_cast<EventCommand>(TextWord(bytes, at) & kCommandMask);

        if (code == EventCommand::End)
        {
            at += kOpcodeBytes;
            continue;
        }

        if (code == EventCommand::OpenScript && depth < EventScan::OpenDepth &&
            this->Scan(directory, TextWord(bytes, at + kOpcodeBytes), depth + 1, found))
        {
            return true;
        }

        const std::size_t length = ScannedBytes(bytes, at, code);

        if (length == 0)
        {
            return false;
        }

        if (found(code, bytes.subspan(at, std::min(length, end - at))))
        {
            return true;
        }

        at += length;
    }

    return false;
}

const std::vector<u8>& EventScan::Script(const std::u8string& directory, std::size_t file)
{
    std::vector<u8>& bytes = this->files[file];

    if (this->loaded[file])
    {
        return bytes;
    }

    this->loaded[file] = true;
    File talk;

    if (talk.Open(
            PathFormat("%sTalk%d.dat", {directory, static_cast<u32>(file + 1)}),
            File::Disposition::OpenExisting,
            File::Access::Read
        ))
    {
        u32 size = talk.Size();

        if (size != File::InvalidPosition)
        {
            bytes.resize(size);

            if (!talk.Read(bytes.data(), size))
            {
                size = 0;
            }

            bytes.resize(size);
        }

        talk.Close();
    }

    return bytes;
}

}  // namespace swd3
