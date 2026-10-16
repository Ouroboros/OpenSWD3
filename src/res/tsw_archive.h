#pragma once

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"
#include "pic/picture.h"

namespace swd3
{

class ErrorReporter;
class RleCodec;

// Texts of the archive (Big5 as in the executable).
namespace tsw_text
{

constexpr char kOpenFailed[] = "TSW\300\311\256\327\266}\261\322\245\242\261\321!!";  // "TSW file open failed!!"
constexpr char kErrorTitle[] = "\277\371\273~";                                       // "Error"
constexpr char kBadHeader[] = "<<TSW%d\270\271>>(%s)\252\272\300\311\300Y\270\352\256\306\246\263\273~!!";
constexpr char kBadFrame[] =
    "<<TSW%d\270\271>>(%s)\252\272\262\304%d\261i\252\272\271\317\247\316\270\352\256\306\246\263\273~!!";
constexpr char kNoBufferBox[] = "\260t\270m\270\321\300\243\301Y\260O\276\320\305\351\245\242\261\321!!,";
constexpr char kNoBufferLog[] = "\260t\270m\270\321\300\243\301Y\260O\276\320\305\351\245\242\261\321";
constexpr char kDecompressFailed[] = "\270\321\300\243\245\242\261\321!!,";
constexpr char kLoadFailedLog[] = "Mem error::tsw:%d pa:%d";
constexpr const char* kFileNames[] = {
    "all_char.tsw",
    "all_item.tsw",
    "all_magic.tsw",
    "all_sys.tsw",
    "all_map1.tsw",
    "all_map2.tsw",
};

}  // namespace tsw_text

// Directory record of a tsw file: 0x2C bytes at 0x1C + 0x2C * (index - 1).
struct TswDirectoryEntry
{
    char name[0x18] = {};
    u32 offset = 0;
    u8 unknown1C[0x10] = {};
};

static_assert(sizeof(TswDirectoryEntry) == 0x2C);

// Frame record of an entry (0x24 bytes after the 12-byte entry header and the palette of 8-bit entries). The magic
// frame cache keeps the absolute offset.
struct TswFrameEntry
{
    u32 offset = 0;
    u32 packedSize = 0;
    u32 size = 0;
    u32 flag = 0;
    u32 unknown10[4] = {};
    u16 width = 0;
    u16 height = 0;
};

static_assert(sizeof(TswFrameEntry) == 0x24);

// Pictures of the map units come from the map module (picture id 0xFFFF, the frame is the unit).
class MapPictureSource
{
public:
    virtual ~MapPictureSource() = default;
    virtual bool LoadMapPicture(int unit, Picture& picture) = 0;
};

// The tsw picture archives (0x004315C0-0x00433BF0): six files addressed by id / 3000 (character, item, magic,
// system, two map sets) with a byte-limited picture cache in ten chains, a five-slot cache of magic frame tables and
// direct loading for callers that keep the picture.
class TswArchive
{
public:
    static constexpr int kFileCount = 6;
    static constexpr int kIndicesPerFile = 3000;
    static constexpr int kChains = 10;
    static constexpr int kMagicSlots = 5;
    static constexpr int kMagicFramesPerSlot = 255;
    static constexpr int kMagicFirst = 6001;
    static constexpr int kMagicLast = 9000;
    static constexpr u16 kMapPicture = 0xFFFF;

    TswArchive(const RleCodec& codec, ErrorReporter& errors);

    void SetGameDirectory(std::string directory);
    void SetMapPictureSource(MapPictureSource* source);

    void SetCacheLimit(int bytes);
    Picture* GetPicture(u32 id, u32 frame);
    std::unique_ptr<Picture> LoadPictureDirect(u32 id, u32 frame);
    int CloseAll();
    Picture* FindPicture(u32 id, u32 frame);
    bool LoadFromFile(const std::string& fileName, int index, u32 frame, Picture& picture);
    int CacheBytes() const;

private:
    // Entry header of a frame set as read by ReadHeader: format and palette.
    struct FrameInfo
    {
        std::vector<u8> data;
        std::vector<u16> palette;
        u16 format = 0;
        u16 width = 0;
        u16 height = 0;
    };

    struct CacheEntry
    {
        u16 id = 0;
        u16 frame = 0;
        Picture picture;
    };

    // A decompression buffer that grows to the largest packed frame; a failed decompression releases it while the
    // capacity stays, so the following loads of smaller frames fail until a larger one reallocates it.
    struct PackedBuffer
    {
        std::vector<u8> bytes;
        u32 capacity = 0;
        bool present = false;
    };

    void OpenArchives();
    void ReadMagicDirectory(File& file);
    void CacheMagicFrames(File& file, int index);
    bool CacheLoad(u32 id, u32 frame);
    Picture* CacheFind(u32 id, u32 frame);
    void CacheEvict();
    void CacheClear();
    bool OpenFile(File& file, const std::string& name);
    bool ReadHeader(File& file, FrameInfo& info, u16& frameCount);
    bool LoadPicture(File& file, int index, u32 frame, Picture& picture);
    bool ReadFrame(File& file, u32 base, FrameInfo& info, int frame);
    bool LoadPictureB(File& file, int index, u32 frame, Picture& picture);
    bool ReadFrameB(File& file, u32 base, FrameInfo& info, int frame);
    bool
    Decompress(PackedBuffer& buffer, File& file, const TswFrameEntry& entry, FrameInfo& info, bool logMissingBuffer);
    void FreeBuffers();
    File* ArchiveOf(u32 id);
    u32 MagicDirectoryOffset(int index) const;
    TswFrameEntry& MagicFrame(int slot, int frame);
    void ReportError(const std::string& text);

    const RleCodec& codec;
    ErrorReporter& errors;
    MapPictureSource* mapPictures = nullptr;
    std::string gameDirectory;
    std::array<File, kFileCount> files;
    File extraFile;  // closed by CloseAll; opened by another module
    bool opened = false;
    int cacheLimit = 0x600000;
    int cacheBytes = 0;
    std::array<std::list<CacheEntry>, kChains> chains;
    std::array<u16, kChains> chainCounts{};
    Picture* current = nullptr;
    bool magicMode = false;
    std::array<TswDirectoryEntry, kIndicesPerFile> magicDirectory{};
    std::vector<TswFrameEntry> magicFrames = std::vector<TswFrameEntry>(kMagicSlots * kMagicFramesPerSlot);
    std::array<int, kMagicSlots> magicSlots{};
    std::array<int, kMagicSlots> magicAges{};
    int magicSlot = 0;
    std::vector<u16> magicPalette;
    PackedBuffer packed;
    PackedBuffer packedB;
    std::string errorText;
    TswFrameEntry spareFrame;
    // Files of failed LoadFromFile calls, which the original leaves open.
    std::vector<std::unique_ptr<File>> leakedFiles;
};

}  // namespace swd3
