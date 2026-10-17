#include "res/tsw_archive.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

#include "codec/lzo.h"
#include "core/debug_log.h"
#include "core/format.h"
#include "core/words.h"
#include "pic/rle.h"
#include "platform/message_box.h"

namespace swd3
{

namespace
{

// Bucket of the cache chains: by id, map pictures by unit.
int ChainOf(u32 id, u32 frame)
{
    return static_cast<u16>(id) == TswArchive::MapPicture ? static_cast<int>((LowWord(frame)) % TswArchive::Chains)
                                                          : static_cast<int>(LowWord(id) % TswArchive::Chains);
}

bool IsMagic(u32 id)
{
    const auto value = static_cast<u16>(id);
    return value >= TswArchive::MagicFirst && value <= TswArchive::MagicLast;
}

template <typename Value>
bool ReadValue(File& file, Value& value)
{
    u32 bytes = sizeof(Value);
    return file.Read(&value, bytes);
}

std::u8string Format(std::u8string_view format, int first, std::string_view name, int second = 0)
{
    return Utf8Format(format, {first, name, second});
}

// A heap block for `size` bytes, as malloc gives it: false where the original 32-bit process gets no memory (requests
// of 1 GiB and more, as read from the garbage of a frame record past the end of an entry).
bool TryAssign(std::vector<u8>& bytes, u32 size)
{
    constexpr u32 kAllocationLimit = 0x40000000;

    if (size >= kAllocationLimit)
    {
        bytes.clear();
        return false;
    }

    try
    {
        bytes.assign(size, 0);
    }
    catch (const std::bad_alloc&)
    {
        bytes.clear();
        return false;
    }

    return true;
}

}  // namespace

TswArchive::TswArchive(const RleCodec& codec, ErrorReporter& errors)
    : codec(codec),
      errors(errors)
{
}

void TswArchive::SetGameDirectory(std::string directory)
{
    this->gameDirectory = std::move(directory);
}

void TswArchive::SetMapPictureSource(MapPictureSource* source)
{
    this->mapPictures = source;
}

int TswArchive::CacheBytes() const
{
    return this->cacheBytes;
}

// Opens the six archives from the game directory and reads the magic directory (inline at the start of 0x004315D0
// and 0x00431760).
void TswArchive::OpenArchives()
{
    for (int i = 0; i < TswArchive::FileCount; ++i)
    {
        this->OpenFile(
            this->files[static_cast<std::size_t>(i)],
            Concat({this->gameDirectory, TswArchive::FileNames[i]})
        );
    }

    this->magicDirectory.fill(TswDirectoryEntry{});
    std::fill(this->magicFrames.begin(), this->magicFrames.end(), TswFrameEntry{});
    this->ReadMagicDirectory(this->files[TswArchive::MagicFile]);
    this->opened = true;
}

File* TswArchive::ArchiveOf(u32 id)
{
    const auto index = static_cast<std::size_t>(LowWord(id) / TswArchive::IndicesPerFile);
    // Ids from 18000 on index past the six handles in the original; they find no archive here.
    return index < this->files.size() ? &this->files[index] : nullptr;
}

// The offset field of magic directory entry `index` (file index index + 1 at entry index - 1). Index 0 (id 9000) reads
// the word before the directory, which is the second magic slot number.
u32 TswArchive::MagicDirectoryOffset(int index) const
{
    if (index == 0)
    {
        return static_cast<u32>(this->magicSlots[1]);
    }

    return this->magicDirectory[static_cast<std::size_t>(index - 1)].offset;
}

// The five slots of 255 frame entries form one table; counts above 255 run into the next slot.
TswFrameEntry& TswArchive::MagicFrame(int slot, int frame)
{
    const std::size_t index = static_cast<std::size_t>(slot) * TswArchive::MagicFramesPerSlot + frame;
    // Records beyond the last slot lie outside the table in the original; they go to a spare record here.
    return index < this->magicFrames.size() ? this->magicFrames[index] : this->spareFrame;
}

void TswArchive::ReportError(std::u8string_view text)
{
    this->errors.ShowError(TswArchive::ErrorTitle, text);
}

// swd32.exe 0x004315C0
void TswArchive::SetCacheLimit(int bytes)
{
    this->cacheLimit = bytes;
}

// swd32.exe 0x004315D0: the cached picture, loaded on a miss; nullptr when it cannot be loaded. The cache is trimmed
// before the lookup once it holds the limit.
Picture* TswArchive::GetPicture(u32 id, u32 frame)
{
    if (!this->opened)
    {
        this->OpenArchives();
    }

    if (this->cacheBytes >= this->cacheLimit)
    {
        this->CacheEvict();
    }

    if (this->CacheFind(id, frame) == nullptr && !this->CacheLoad(id, frame))
    {
        this->current = nullptr;
    }

    return this->current;
}

// swd32.exe 0x00431760: a picture outside the cache, owned by the caller. The load result is not checked.
std::unique_ptr<Picture> TswArchive::LoadPictureDirect(u32 id, u32 frame)
{
    if (!this->opened)
    {
        this->OpenArchives();
    }

    this->magicMode = false;

    if (IsMagic(id))
    {
        this->magicMode = true;
        this->CacheMagicFrames(
            this->files[TswArchive::MagicFile],
            static_cast<int>(LowWord(id) % TswArchive::IndicesPerFile)
        );
    }

    auto picture = std::make_unique<Picture>();

    if (File* archive = this->ArchiveOf(id))
    {
        this->LoadPictureB(
            *archive,
            static_cast<int>(LowWord(id) % TswArchive::IndicesPerFile),
            LowWord(frame),
            *picture
        );
    }

    this->codec.ConvertTo16(*picture);
    this->magicMode = false;
    return picture;
}

// swd32.exe 0x00431960: closes the archives and empties every cache
int TswArchive::CloseAll()
{
    for (File& file : this->files)
    {
        file.Close();
    }

    this->extraFile.Close();
    this->opened = false;
    this->magicMode = false;
    this->CacheClear();
    this->FreeBuffers();
    this->magicAges.fill(0);
    this->magicSlots.fill(0);
    this->magicDirectory.fill(TswDirectoryEntry{});
    std::fill(this->magicFrames.begin(), this->magicFrames.end(), TswFrameEntry{});
    return 0;
}

// swd32.exe 0x00431A20: the cached picture without loading
Picture* TswArchive::FindPicture(u32 id, u32 frame)
{
    this->current = nullptr;
    this->CacheFind(id, frame);
    return this->current;
}

// swd32.exe 0x00431A50: the 3000 directory records of the magic archive
void TswArchive::ReadMagicDirectory(File& file)
{
    for (std::size_t i = 0; i < this->magicDirectory.size(); ++i)
    {
        file.Seek(File::Origin::Begin, static_cast<i32>(TswArchive::DirectoryStart + sizeof(TswDirectoryEntry) * i));
        ReadValue(file, this->magicDirectory[i]);
    }
}

// swd32.exe 0x00431AA0: keeps the frame table of magic entry `index` in one of five slots (a slot holding it, the
// first empty one, else the one unused longest; every slot passed over ages by one) with absolute frame offsets, and
// the entry's palette for the next load.
void TswArchive::CacheMagicFrames(File& file, int index)
{
    FrameInfo info;
    u16 count = 0;
    int slot = 0;
    bool found = false;

    for (; slot < TswArchive::MagicSlots; ++slot)
    {
        const int held = this->magicSlots[static_cast<std::size_t>(slot)];

        if (held == index)
        {
            file.Seek(File::Origin::Begin, static_cast<i32>(this->MagicDirectoryOffset(index)));
            this->ReadHeader(file, info, count);
            this->magicAges[static_cast<std::size_t>(slot)] = 0;
            this->magicPalette = info.palette;
            return;
        }

        if (held == 0)
        {
            this->magicSlots[static_cast<std::size_t>(slot)] = index;
            this->magicAges[static_cast<std::size_t>(slot)] = 0;
            found = true;
            break;
        }

        ++this->magicAges[static_cast<std::size_t>(slot)];
    }

    if (!found)
    {
        int oldest = this->magicAges[0];
        slot = 0;

        for (int i = 0; i < TswArchive::MagicSlots; ++i)
        {
            if (oldest < this->magicAges[static_cast<std::size_t>(i)])
            {
                oldest = this->magicAges[static_cast<std::size_t>(i)];
                slot = i;
            }
        }

        this->magicSlots[static_cast<std::size_t>(slot)] = index;
        this->magicAges[static_cast<std::size_t>(slot)] = 0;
    }

    const u32 base = this->MagicDirectoryOffset(index);
    file.Seek(File::Origin::Begin, static_cast<i32>(base));
    this->ReadHeader(file, info, count);
    this->magicPalette = info.palette;
    i32 skip = 0;

    for (int frame = 0; frame < count; ++frame)
    {
        TswFrameEntry& entry = this->MagicFrame(slot, frame);
        file.Seek(File::Origin::Begin, static_cast<i32>(base));
        this->ReadHeader(file, info, count);
        file.Seek(File::Origin::Current, skip);
        ReadValue(file, entry);
        entry.offset = file.Seek(File::Origin::Begin, static_cast<i32>(base + entry.offset));
        skip += sizeof(TswFrameEntry);
    }
}

// swd32.exe 0x00431C50: a new entry goes to the front of its chain before loading (a failed load leaves it there
// with an empty picture); map pictures come from the map module. The picture size counts towards the limit.
bool TswArchive::CacheLoad(u32 id, u32 frame)
{
    const int chain = ChainOf(id, frame);
    std::list<CacheEntry>& entries = this->chains[static_cast<std::size_t>(chain)];
    ++this->chainCounts[static_cast<std::size_t>(chain)];
    entries.push_front(CacheEntry{static_cast<u16>(id), static_cast<u16>(frame), Picture{}});
    Picture& picture = entries.front().picture;

    if (static_cast<u16>(id) == TswArchive::MapPicture)
    {
        if (this->mapPictures != nullptr)
        {
            this->mapPictures->LoadMapPicture(static_cast<int>(LowWord(frame)), picture);
        }
    }
    else
    {
        this->magicMode = false;

        if (IsMagic(id))
        {
            this->magicMode = true;
            this->CacheMagicFrames(
                this->files[TswArchive::MagicFile],
                static_cast<int>(LowWord(id) % TswArchive::IndicesPerFile)
            );
        }

        File* archive = this->ArchiveOf(id);
        const int index = static_cast<int>(LowWord(id) % TswArchive::IndicesPerFile);

        if (archive == nullptr || !this->LoadPicture(*archive, index, LowWord(frame), picture))
        {
            DebugLog(TswArchive::LoadFailedLog, LowWord(id), LowWord(frame));
            return false;
        }

        this->codec.ConvertTo16(picture);
        this->magicMode = false;
    }

    this->current = &picture;
    this->cacheBytes += static_cast<int>(picture.data.size());
    return true;
}

// swd32.exe 0x00431DF0: a found entry moves to the front of its chain
Picture* TswArchive::CacheFind(u32 id, u32 frame)
{
    this->current = nullptr;
    std::list<CacheEntry>& entries = this->chains[static_cast<std::size_t>(ChainOf(id, frame))];

    for (auto entry = entries.begin(); entry != entries.end(); ++entry)
    {
        if (entry->id == static_cast<u16>(id) && entry->frame == static_cast<u16>(frame))
        {
            entries.splice(entries.begin(), entries, entry);
            this->current = &entries.front().picture;
            return this->current;
        }
    }

    return nullptr;
}

// swd32.exe 0x00431EA0: drops entries from the end of the longest chain (the first of equal ones) until the cache is
// below its limit or that chain is empty
void TswArchive::CacheEvict()
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
            this->cacheBytes -= static_cast<int>(entries.back().picture.data.size());
            entries.pop_back();
        }

        if (--this->chainCounts[longest] == 0)
        {
            return;
        }
    } while (this->cacheBytes >= this->cacheLimit);
}

// swd32.exe 0x00431F80
void TswArchive::CacheClear()
{
    for (std::size_t i = 0; i < this->chains.size(); ++i)
    {
        for (const CacheEntry& entry : this->chains[i])
        {
            this->cacheBytes -= static_cast<int>(entry.picture.data.size());
            --this->chainCounts[i];
        }

        this->chains[i].clear();
    }
}

// swd32.exe 0x00433270: read-only, shared for reading
bool TswArchive::OpenFile(File& file, const std::string& name)
{
    return file.Open(name, File::Disposition::OpenExisting, File::Access::Read);
}

// swd32.exe 0x004332A0: the 12-byte entry header (magic 0xABCD, frame count, format) and the palette of an 8-bit
// entry
bool TswArchive::ReadHeader(File& file, FrameInfo& info, u16& frameCount)
{
    u8 header[TswArchive::EntryHeaderBytes] = {};
    u32 bytes = sizeof(header);
    file.Read(header, bytes);
    u16 magic = 0;
    std::memcpy(&magic, header + TswArchive::MagicAt, sizeof(magic));

    if (magic != TswArchive::EntryMagic)
    {
        return false;
    }

    std::memcpy(&frameCount, header + TswArchive::FrameCountAt, sizeof(frameCount));
    std::memcpy(&info.format, header + TswArchive::FormatAt, sizeof(info.format));

    if (info.format == TswArchive::PaletteFormat)
    {
        info.palette.assign(TswArchive::PaletteColors, 0);
        u32 paletteBytes = TswArchive::PaletteColors * sizeof(u16);
        file.Read(info.palette.data(), paletteBytes);
    }

    return true;
}

// swd32.exe 0x00433320: one picture from a tsw file outside the archives. A failed load leaves the file open.
bool TswArchive::LoadFromFile(const std::string& fileName, int index, u32 frame, Picture& picture)
{
    auto file = std::make_unique<File>();

    if (!this->OpenFile(*file, fileName))
    {
        this->ReportError(TswArchive::OpenFailed);
        return false;
    }

    if (!this->LoadPicture(*file, index, frame, picture))
    {
        this->leakedFiles.push_back(std::move(file));
        return false;
    }

    file->Close();
    return true;
}

// swd32.exe 0x00433380: a cached picture: in magic mode at the frame of the current magic slot (the palette stays
// empty), otherwise through the directory record. The original prints the name of its uninitialised directory record
// in the magic-mode error; it is empty here.
bool TswArchive::LoadPicture(File& file, int index, u32 frame, Picture& picture)
{
    TswDirectoryEntry entry;
    FrameInfo info;

    if (this->magicMode)
    {
        int slot = 0;

        for (;; ++slot)
        {
            if (slot == TswArchive::MagicSlots)
            {
                slot = this->magicSlot;
                break;
            }

            if (this->magicSlots[static_cast<std::size_t>(slot)] == index)
            {
                this->magicSlot = slot;
                break;
            }
        }

        file.Seek(File::Origin::Begin, static_cast<i32>(this->MagicFrame(slot, static_cast<int>(frame)).offset));
    }
    else
    {
        file.Seek(
            File::Origin::Begin,
            static_cast<i32>(TswArchive::DirectoryStart + sizeof(TswDirectoryEntry) * (index - 1))
        );
        ReadValue(file, entry);
        file.Seek(File::Origin::Begin, static_cast<i32>(entry.offset));
        u16 count = 0;

        if (!this->ReadHeader(file, info, count))
        {
            this->errorText = Format(TswArchive::BadHeader, index, entry.Name());
            DebugLog(this->errorText);
            this->ReportError(this->errorText);
            return false;
        }
    }

    if (!this->ReadFrame(file, entry.offset, info, static_cast<int>(frame)))
    {
        this->errorText = Format(TswArchive::BadFrame, index, entry.Name(), static_cast<int>(frame));
        DebugLog(this->errorText);
        this->ReportError(this->errorText);
        return false;
    }

    picture.data = std::move(info.data);
    picture.width = info.width;
    picture.height = info.height;
    picture.palette = std::move(info.palette);
    return true;
}

// Reads the packed frame into the buffer and decompresses it to its stored size.
bool TswArchive::Decompress(
    PackedBuffer& buffer,
    File& file,
    const TswFrameEntry& entry,
    FrameInfo& info,
    bool logMissingBuffer
)
{
    if (entry.packedSize > buffer.capacity)
    {
        buffer.present = TryAssign(buffer.bytes, entry.packedSize);
        buffer.capacity = entry.packedSize;
    }

    std::vector<u8> out;
    const bool outAllocated = TryAssign(out, entry.size);

    if (!buffer.present || !outAllocated)
    {
        if (logMissingBuffer)
        {
            DebugLog(TswArchive::NoBufferLog);
        }

        this->ReportError(TswArchive::NoBufferBox);
        return false;
    }

    u32 bytes = entry.packedSize;
    file.Read(buffer.bytes.data(), bytes);
    LzoDecoder decoder({buffer.bytes.data(), entry.packedSize}, out);
    const LzoStatus status = decoder.Run();
    const bool reachedEnd = status == LzoStatus::Ok || status == LzoStatus::InputNotConsumed;

    if (!reachedEnd || decoder.Written() != entry.size)
    {
        buffer.bytes.clear();
        buffer.present = false;
        this->ReportError(TswArchive::DecompressFailed);
        return false;
    }

    info.data = std::move(out);
    info.width = entry.width;
    info.height = entry.height;
    return true;
}

// swd32.exe 0x00433540: the frame record `frame` is taken from a read of the first frame + 1 records (magic mode: the
// cached record of the current slot)
bool TswArchive::ReadFrame(File& file, u32 base, FrameInfo& info, int frame)
{
    TswFrameEntry entry;

    if (this->magicMode)
    {
        entry = this->MagicFrame(this->magicSlot, frame);
    }
    else
    {
        std::vector<TswFrameEntry> records(static_cast<std::size_t>(frame) + 1);
        u32 bytes = static_cast<u32>(records.size() * sizeof(TswFrameEntry));
        file.Read(records.data(), bytes);
        entry = records.back();
        file.Seek(File::Origin::Begin, static_cast<i32>(entry.offset + base));
    }

    return this->Decompress(this->packed, file, entry, info, true);
}

// swd32.exe 0x00433720: the direct-load variant of 0x00433380 (no log; magic pictures take the slot palette)
bool TswArchive::LoadPictureB(File& file, int index, u32 frame, Picture& picture)
{
    TswDirectoryEntry entry;
    FrameInfo info;

    if (this->magicMode)
    {
        int slot = 0;

        for (;; ++slot)
        {
            if (slot == TswArchive::MagicSlots)
            {
                slot = this->magicSlot;
                break;
            }

            if (this->magicSlots[static_cast<std::size_t>(slot)] == index)
            {
                this->magicSlot = slot;
                break;
            }
        }

        file.Seek(File::Origin::Begin, static_cast<i32>(this->MagicFrame(slot, static_cast<int>(frame)).offset));
    }
    else
    {
        file.Seek(
            File::Origin::Begin,
            static_cast<i32>(TswArchive::DirectoryStart + sizeof(TswDirectoryEntry) * (index - 1))
        );
        ReadValue(file, entry);
        file.Seek(File::Origin::Begin, static_cast<i32>(entry.offset));
        u16 count = 0;

        if (!this->ReadHeader(file, info, count))
        {
            this->errorText = Format(TswArchive::BadHeader, index, entry.Name());
            this->ReportError(this->errorText);
            return false;
        }
    }

    if (!this->ReadFrameB(file, entry.offset, info, static_cast<int>(frame)))
    {
        this->errorText = Format(TswArchive::BadFrame, index, entry.Name(), static_cast<int>(frame));
        this->ReportError(this->errorText);
        return false;
    }

    picture.data = std::move(info.data);
    picture.width = info.width;
    picture.height = info.height;
    picture.palette = this->magicMode ? this->magicPalette : std::move(info.palette);
    return true;
}

// swd32.exe 0x004338F0: reads only the needed frame record, after the header
bool TswArchive::ReadFrameB(File& file, u32 base, FrameInfo& info, int frame)
{
    TswFrameEntry entry;

    if (this->magicMode)
    {
        entry = this->MagicFrame(this->magicSlot, frame);
    }
    else
    {
        file.Seek(File::Origin::Current, frame * static_cast<i32>(sizeof(TswFrameEntry)));
        ReadValue(file, entry);
        file.Seek(File::Origin::Begin, static_cast<i32>(entry.offset + base));
    }

    return this->Decompress(this->packedB, file, entry, info, false);
}

// swd32.exe 0x00433BF0
void TswArchive::FreeBuffers()
{
    this->packed = PackedBuffer{};
    this->packedB = PackedBuffer{};
}

}  // namespace swd3
