#include "res/act_archive.h"

#include <cstdio>
#include <cstring>

#include "core/debug_log.h"
#include "core/format.h"
#include "platform/message_box.h"

namespace swd3
{

namespace
{

constexpr int kDirectoryRecordBytes = 0x2C;

int ChainOf(int id, int sub)
{
    return (id == ActArchive::kMapAct ? sub % 10 : id % 10) & 0xFFFF;
}

u16 ReadWord(const std::vector<u8>& data, std::size_t offset)
{
    u16 value = 0;

    if (offset + 2 <= data.size())
    {
        std::memcpy(&value, data.data() + offset, 2);
    }

    return value;
}

u32 ReadDword(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;

    if (offset + 4 <= data.size())
    {
        std::memcpy(&value, data.data() + offset, 4);
    }

    return value;
}

}  // namespace

ActArchive::ActArchive(ErrorReporter& errors)
    : errors(errors)
{
}

void ActArchive::SetGameDirectory(std::string directory)
{
    this->gameDirectory = std::move(directory);
}

int ActArchive::CacheBytes() const
{
    return this->cacheBytes;
}

// Set by a restart of Run and cleared once its script loaded (the original flag at 0x004FB300).
bool ActArchive::ScriptRestarted() const
{
    return this->restarted;
}

// Opens the six archives (inline at the start of 0x00432A50 and 0x00432BC0).
void ActArchive::OpenArchives()
{
    for (int i = 0; i < ActArchive::kFileCount; ++i)
    {
        this->OpenFile(
            this->files[static_cast<std::size_t>(i)],
            Concat({this->gameDirectory, act_text::kFileNames[i]})
        );
    }

    this->opened = true;
}

// swd32.exe 0x00432010
void ActArchive::SetCacheLimit(int bytes)
{
    this->cacheLimit = bytes;
}

// swd32.exe 0x00432020: read-only, not shared
bool ActArchive::OpenFile(File& file, const std::string& name)
{
    return file.Open(name, File::Disposition::OpenExisting, File::Access::Read);
}

// swd32.exe 0x00432050: script `index` of an entry: from its offset to the next non-zero offset (the last one to the
// end of the entry); nothing for an index beyond the count or a zero offset. Offsets past the entry data (read beyond
// the buffer in the original) end at the entry end here.
ActScript ActArchive::ExtractEntry(const std::vector<u8>& data, int index, u32& size) const
{
    const int count = ReadWord(data, 0);

    if (index >= count)
    {
        return nullptr;
    }

    std::size_t table = 2 + 4 * static_cast<std::size_t>(index);
    const u32 start = ReadDword(data, table);
    table += 4;

    if (start == 0)
    {
        return nullptr;
    }

    u32 end = static_cast<u32>(data.size());

    if (index != count - 1)
    {
        u32 next = 0;

        while (next == 0 && table + 4 <= data.size())
        {
            next = ReadDword(data, table);
            table += 4;
        }

        if (next != 0)
        {
            end = next;
        }
    }

    const u32 length = end - start;
    auto script = std::make_shared<std::vector<u8>>(length, 0);

    for (u32 i = 0; i < length && start + i < data.size(); ++i)
    {
        (*script)[i] = data[start + i];
    }

    size = length;
    return script;
}

// swd32.exe 0x004320E0: reads the whole entry (its directory record cached) and extracts the script
ActScript ActArchive::Load(File* file, int fileIndex, int entry, int sub, u32& size)
{
    u32 length = 0;
    u32 offset = 0;
    this->IndexLookup(fileIndex, entry, offset, length);

    if (offset == 0 && file != nullptr)
    {
        this->IndexInsert(*file, fileIndex, entry, offset, length);
    }

    std::vector<u8> buffer(length, 0);

    if (file != nullptr && length != 0)
    {
        file->Seek(File::Origin::Begin, static_cast<i32>(offset));
        u32 bytes = length;
        file->Read(buffer.data(), bytes);
    }

    ActScript script = this->ExtractEntry(buffer, sub, size);

    if (script != nullptr)
    {
        return script;
    }

    this->errorText = WinFormat(act_text::kBadEntry, {entry, sub});
    DebugLog(this->errorText);
    this->errors.ShowError(Big5(act_text::kErrorTitle), this->errorText);
    return nullptr;
}

// swd32.exe 0x00432A50: the cached script, loaded on a miss (the cache is trimmed first once it holds the limit)
ActScript ActArchive::Get(int id, int sub)
{
    if (!this->opened)
    {
        this->OpenArchives();
    }

    if (this->cacheBytes >= this->cacheLimit)
    {
        this->CacheEvict();
    }

    if (this->CacheFind(id, sub))
    {
        return this->current;
    }

    return this->CacheLoad(id, sub) ? this->current : nullptr;
}

// swd32.exe 0x00432BC0: a script outside the cache (the original allocates a new copy on every call)
ActScript ActArchive::LoadDirect(int id, int sub)
{
    if (!this->opened)
    {
        this->OpenArchives();
    }

    const int fileIndex = id / ActArchive::kIndicesPerFile;
    File* file = fileIndex < ActArchive::kFileCount ? &this->files[static_cast<std::size_t>(fileIndex)] : nullptr;
    u32 size = 0;
    this->direct = this->Load(file, fileIndex, id % ActArchive::kIndicesPerFile, sub, size);
    return this->direct;
}

// swd32.exe 0x00432D30: a new entry goes to the end of its chain before loading (a failed load leaves it there)
bool ActArchive::CacheLoad(int id, int sub)
{
    const int chain = ChainOf(id, sub);
    ++this->chainCounts[static_cast<std::size_t>(chain)];
    std::list<CacheEntry>& entries = this->chains[static_cast<std::size_t>(chain)];
    entries.push_back(CacheEntry{id, sub, 0, nullptr});
    CacheEntry& entry = entries.back();
    const int fileIndex = id / ActArchive::kIndicesPerFile;
    // Ids from 18000 on (the map id 0xFFFF among them) index past the six handles in the original.
    File* file = fileIndex < ActArchive::kFileCount ? &this->files[static_cast<std::size_t>(fileIndex)] : nullptr;
    this->current = this->Load(file, fileIndex, id % ActArchive::kIndicesPerFile, sub, entry.size);

    if (this->current == nullptr)
    {
        DebugLog(act_text::kLoadFailedLog, fileIndex, sub);
        return false;
    }

    entry.data = this->current;
    this->cacheBytes += static_cast<int>(entry.size);
    return true;
}

// swd32.exe 0x00432E40: a found entry moves to the front of its chain
bool ActArchive::CacheFind(int id, int sub)
{
    this->current = nullptr;
    std::list<CacheEntry>& entries = this->chains[static_cast<std::size_t>(ChainOf(id, sub))];

    for (auto entry = entries.begin(); entry != entries.end(); ++entry)
    {
        if (entry->id == id && entry->sub == sub)
        {
            entries.splice(entries.begin(), entries, entry);
            this->current = entries.front().data;
            return true;
        }
    }

    return false;
}

// swd32.exe 0x00432EE0: drops entries from the end of the longest chain until below the limit or the chain is empty
void ActArchive::CacheEvict()
{
    std::size_t longest = 0;

    for (std::size_t i = 0; i < this->chainCounts.size(); ++i)
    {
        if (this->chainCounts[i] > this->chainCounts[longest])
        {
            longest = i;
        }
    }

    if (this->cacheBytes < this->cacheLimit)
    {
        return;
    }

    std::list<CacheEntry>& entries = this->chains[longest];

    do
    {
        if (!entries.empty())
        {
            this->cacheBytes -= static_cast<int>(entries.back().size);
            entries.pop_back();
        }

        if (--this->chainCounts[longest] == 0)
        {
            return;
        }
    } while (this->cacheBytes >= this->cacheLimit);
}

// swd32.exe 0x00432FA0
void ActArchive::CacheClear()
{
    for (std::size_t i = 0; i < this->chains.size(); ++i)
    {
        for (const CacheEntry& entry : this->chains[i])
        {
            this->cacheBytes -= static_cast<int>(entry.size);
            --this->chainCounts[i];
        }

        this->chains[i].clear();
    }
}

// swd32.exe 0x00433010: closes everything; a cache that still counts bytes afterwards is reported
int ActArchive::CloseAll()
{
    for (File& file : this->files)
    {
        file.Close();
    }

    this->CacheClear();
    this->IndexClear();
    const int remaining = this->cacheBytes;
    this->opened = false;

    if (remaining != 0)
    {
        this->errors.ShowError(act_text::kMemoryErrorTitle, Big5(act_text::kMemoryError));
        return 1;
    }

    return remaining;
}

// swd32.exe 0x00433080: 1 makes Run take scripts from the cache, anything else loads them directly
void ActArchive::SetUseCache(int value)
{
    this->useCache = value;
}

// swd32.exe 0x00433090: reads the directory record of the entry into the front of its index chain
void ActArchive::IndexInsert(File& file, int fileIndex, int entry, u32& offset, u32& size)
{
    const int chain = (entry % 10) & 0xFFFF;
    ++this->indexCounts[static_cast<std::size_t>(chain)];
    u32 record[kDirectoryRecordBytes / 4] = {};
    file.Seek(File::Origin::Begin, kDirectoryRecordBytes * entry - 0x10);
    u32 bytes = kDirectoryRecordBytes;
    file.Read(record, bytes);
    this->index[static_cast<std::size_t>(chain)].push_front(IndexEntry{fileIndex, entry, record[6], record[5]});
    offset = record[6];
    size = record[5];
}

// swd32.exe 0x00433190: a found record moves to the front of its chain
void ActArchive::IndexLookup(int fileIndex, int entry, u32& offset, u32& size)
{
    std::list<IndexEntry>& records = this->index[static_cast<std::size_t>((entry % 10) & 0xFFFF)];

    for (auto record = records.begin(); record != records.end(); ++record)
    {
        if (record->file == fileIndex && record->entry == entry)
        {
            records.splice(records.begin(), records, record);
            offset = records.front().offset;
            size = records.front().size;
            return;
        }
    }
}

// swd32.exe 0x00433220
void ActArchive::IndexClear()
{
    for (std::size_t i = 0; i < this->index.size(); ++i)
    {
        this->indexCounts[i] = static_cast<u16>(this->indexCounts[i] - this->index[i].size());
        this->index[i].clear();
    }
}

// The playback reset of a new direction, action or id (inline in 0x004321E0).
void ActArchive::Restart(ActState& state, u32 keptFlags)
{
    state.delay = 0;
    state.frameDelay = 0;
    state.position = 0;
    state.effectA = 0;
    state.effectQ = 0;
    state.x = 0;
    state.y = 0;
    state.width = 0;
    state.height = 0;
    state.view = 0;
    state.target = 0;
    state.anchorX = 0;
    state.anchorY = 0;
    state.signal = 0;
    state.wait = 0;
    state.unknown89 = 0;
    state.drawLevel = 0;
    state.red = 0;
    state.green = 0;
    state.blue = 0;

    for (u16& light : state.lights)
    {
        light = 0;
    }

    state.waiting = 0;
    state.hold = 0;
    state.mask = 0;
    state.flags &= keptFlags;
    this->restarted = true;
}

// swd32.exe 0x004321E0: one game frame of an action script. A new direction, action or id restarts it. The steps up
// to 'DE' run at once and set the delay of the frame. 0 when the script cannot be loaded. The opcodes YX, HW, GC, RC
// and BC read their second value without moving past it, so that value also runs as the next opcode. Words beyond
// the script (read from the heap in the original) end the frame here.
int ActArchive::Run(ActState& state)
{
    if ((state.hold == 1 && state.position != 0) || state.id == 0)
    {
        return 1;
    }

    if (state.direction != state.lastDirection)
    {
        state.lastDirection = state.direction;
        this->Restart(state, act_flag::kKeptOnTurn);
    }

    if (state.action != state.lastAction)
    {
        state.lastAction = state.action;
        this->Restart(state, act_flag::kKeptOnRestart);
    }

    if (state.id != state.lastId)
    {
        state.lastId = state.id;
        this->Restart(state, act_flag::kKeptOnRestart);
    }

    const int sub = state.action + state.direction;
    state.script = this->useCache == 1 ? this->Get(state.id, sub) : this->LoadDirect(state.id, sub);

    if (state.script == nullptr)
    {
        return 0;
    }

    this->restarted = false;

    if (state.delay != 0)
    {
        --state.delay;
        return 1;
    }

    const std::vector<u8>& script = *state.script;

    auto word = [&script](u16 index) -> u16
    {
        if (2 * static_cast<std::size_t>(index) + 2 > script.size())
        {
            return static_cast<u16>(ActOpcode::EndFrame);
        }

        u16 value = 0;
        std::memcpy(&value, script.data() + 2 * static_cast<std::size_t>(index), 2);
        return value;
    };

    state.actionCode = 0;
    u16 next = 0;

    while (true)
    {
        const auto code = static_cast<ActOpcode>(word(state.position));
        next = static_cast<u16>(state.position + 1);
        state.position = next;

        if (code == ActOpcode::EndFrame)
        {
            break;
        }

        if (code == ActOpcode::Loop)
        {
            state.position = state.hold == 1 ? static_cast<u16>(next - 1) : 0;
            return 1;
        }

        if (code == ActOpcode::Stop)
        {
            if (state.delay == 0)
            {
                state.waiting = 1;
            }

            state.position = static_cast<u16>(next - 1);
            return 1;
        }

        state.delay = state.frameDelay;
        const i16 speed = state.speed;

        if (speed < 0)
        {
            state.delay = static_cast<u16>(speed & 0x7FFF);
        }

        switch (code)
        {
            case ActOpcode::View:
                state.view = word(state.position++);
                break;

            case ActOpcode::Position:
                state.x = word(state.position++);
                state.y = word(state.position);
                break;

            case ActOpcode::Size:
                state.width = word(state.position++);
                state.height = word(state.position);
                break;

            case ActOpcode::Counter:
                state.counter = word(state.position++);
                break;

            case ActOpcode::Wait:
                state.wait = static_cast<u8>(word(state.position++));
                break;

            case ActOpcode::Invisible:
                state.flags |= act_flag::kInvisible;
                break;

            case ActOpcode::Mask:
                state.mask = 1;
                break;

            case ActOpcode::Frame:
                state.frame = word(state.position++);
                break;

            case ActOpcode::Rotation:
                state.rotation = word(state.position++);
                break;

            case ActOpcode::Delay:
                state.frameDelay = word(state.position++);
                state.delay = state.frameDelay;

                if (speed < 0)
                {
                    state.delay = static_cast<u16>(speed & 0x7FFF);
                }

                break;

            case ActOpcode::EffectQ:
                state.effectQ = word(state.position++);
                break;

            case ActOpcode::OffsetX:
                state.offsetX = word(state.position++);
                break;

            case ActOpcode::OffsetY:
                state.offsetY = word(state.position++);
                break;

            case ActOpcode::Picture:
            {
                const u16 counter = state.counter;
                state.picture = word(state.position++);
                u16 round = static_cast<u16>((counter >> 8) + 1);
                const auto limit = static_cast<u8>(counter);

                if (round > limit)
                {
                    round = static_cast<u16>((round & 0xFF00) | 1);
                }

                state.counter = static_cast<u16>(limit | ((round & 0xFF) << 8));
                break;
            }

            case ActOpcode::Action:
                state.actionCode = word(state.position++);
                break;

            case ActOpcode::Signal:
                state.flags = (state.flags & act_flag::kKeptOnRestart) | act_flag::kSignal;
                state.signal = static_cast<u8>(word(state.position++));
                break;

            case ActOpcode::DrawLevel:
                state.flags = (state.flags & act_flag::kKeptOnRestart) | act_flag::kDrawLevel;
                state.drawLevel = static_cast<u8>(word(state.position++));
                break;

            case ActOpcode::Visible:
                state.flags &= ~act_flag::kInvisible;
                break;

            case ActOpcode::Lights:
            {
                const u16 at = state.position;

                for (int i = 0; i < 7; ++i)
                {
                    state.lights[i] = word(static_cast<u16>(at + i));
                }

                state.position = static_cast<u16>(at + 7);
                break;
            }

            case ActOpcode::Green:
                state.green = word(state.position++);
                state.green2 = word(state.position);
                break;

            case ActOpcode::ClearColors:
                state.red2 = 0;
                state.green2 = 0;
                state.blue2 = 0;
                break;

            case ActOpcode::Red:
                state.red = word(state.position++);
                state.red2 = word(state.position);
                break;

            case ActOpcode::Blue:
                state.blue = word(state.position++);
                state.blue2 = word(state.position);
                break;

            case ActOpcode::EffectA:
                state.effectA = word(state.position++);
                break;

            case ActOpcode::Half:
                state.flags = (state.flags & act_flag::kKeptOnRestart) | act_flag::kHalf;
                break;

            case ActOpcode::MaskBlend:
                state.flags = (state.flags & act_flag::kKeptOnRestart) | act_flag::kMaskBlend;
                break;

            case ActOpcode::NoBlend:
                state.flags = (state.flags & act_flag::kKeptOnRestart) | act_flag::kNoBlend;
                break;

            case ActOpcode::Target:
                state.target = word(state.position++);
                break;

            case ActOpcode::AnchorX:
                state.anchorX = word(state.position++);
                break;

            case ActOpcode::AnchorY:
                state.anchorY = word(state.position++);
                break;

            default:
                break;
        }
    }

    if (state.hold == 1)
    {
        state.position = static_cast<u16>(next - 1);
    }

    return 1;
}

}  // namespace swd3
