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
#include "text/big5.h"

namespace swd3
{

class ErrorReporter;
class RleCodec;

// Directory record of a tsw file: 0x2C bytes at 0x1C + 0x2C * (index - 1).
struct TswDirectoryEntry
{
    std::array<char, 0x18> name = {};
    u32 offset = 0;
    std::array<u8, 0x10> unknown1C = {};

    // The name up to its NUL.
    std::string_view Name() const
    {
        const std::string_view all(this->name.data(), this->name.size());
        return all.substr(0, all.find('\0'));
    }
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
    std::array<u32, 4> unknown10 = {};
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
    // Texts of the archive (Big5 in the executable; u8 here, converted where drawn).
    static constexpr std::u8string_view OpenFailed = u8"TSW檔案開啟失敗!!";  // "TSW file open failed!!"
    static constexpr std::u8string_view ErrorTitle = u8"錯誤";               // "Error"
    static constexpr std::u8string_view BadHeader = u8"<<TSW%d號>>(%s)的檔頭資料有誤!!";
    static constexpr std::u8string_view BadFrame = u8"<<TSW%d號>>(%s)的第%d張的圖形資料有誤!!";
    static constexpr std::u8string_view NoBufferBox = u8"配置解壓縮記憶體失敗!!,";
    static constexpr std::u8string_view NoBufferLog = u8"配置解壓縮記憶體失敗";
    static constexpr std::u8string_view DecompressFailed = u8"解壓失敗!!,";
    static constexpr std::u8string_view LoadFailedLog = u8"Mem error::tsw:%d pa:%d";
    static constexpr std::array<std::string_view, 6> FileNames = {
        "all_char.tsw",
        "all_item.tsw",
        "all_magic.tsw",
        "all_sys.tsw",
        "all_map1.tsw",
        "all_map2.tsw",
    };

    // The file format: the directory of TswDirectoryEntry records, then entries of a header (a dword, the magic, the
    // frame count, the format), the palette of 8-bit entries and the TswFrameEntry records.
    static constexpr u32 DirectoryStart = 0x1C;
    static constexpr std::size_t EntryHeaderBytes = 12;
    static constexpr std::size_t MagicAt = 4;
    static constexpr std::size_t FrameCountAt = 6;
    static constexpr std::size_t FormatAt = 8;
    static constexpr u16 EntryMagic = 0xABCD;
    static constexpr u16 PaletteFormat = 8;
    static constexpr u32 PaletteColors = 0x100;
    static constexpr int FileCount = 6;
    static constexpr std::size_t MagicFile = 2;  // all_magic.tsw
    static constexpr int IndicesPerFile = 3000;
    static constexpr int Chains = 10;
    static constexpr int MagicSlots = 5;
    static constexpr int MagicFramesPerSlot = 255;
    static constexpr int MagicFirst = 6001;
    static constexpr int MagicLast = 9000;
    static constexpr u16 MapPicture = 0xFFFF;

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
    bool Decompress(
        PackedBuffer& buffer,
        File& file,
        const TswFrameEntry& entry,
        FrameInfo& info,
        bool logMissingBuffer
    );
    void FreeBuffers();
    File* ArchiveOf(u32 id);
    u32 MagicDirectoryOffset(int index) const;
    TswFrameEntry& MagicFrame(int slot, int frame);
    void ReportError(std::u8string_view text);

    const RleCodec& codec;
    ErrorReporter& errors;
    MapPictureSource* mapPictures = nullptr;
    std::string gameDirectory;
    std::array<File, FileCount> files;
    File extraFile;  // closed by CloseAll; opened by another module
    bool opened = false;
    int cacheLimit = 0x600000;
    int cacheBytes = 0;
    std::array<std::list<CacheEntry>, Chains> chains;
    std::array<u16, Chains> chainCounts{};
    Picture* current = nullptr;
    bool magicMode = false;
    std::array<TswDirectoryEntry, IndicesPerFile> magicDirectory{};
    std::vector<TswFrameEntry> magicFrames = std::vector<TswFrameEntry>(MagicSlots * MagicFramesPerSlot);
    std::array<int, MagicSlots> magicSlots{};
    std::array<int, MagicSlots> magicAges{};
    int magicSlot = 0;
    std::vector<u16> magicPalette;
    PackedBuffer packed;
    PackedBuffer packedB;
    std::u8string errorText;
    TswFrameEntry spareFrame;
    // Files of failed LoadFromFile calls, which the original leaves open.
    std::vector<std::unique_ptr<File>> leakedFiles;
};

}  // namespace swd3
