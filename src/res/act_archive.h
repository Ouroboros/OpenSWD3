#pragma once

#include <array>
#include <list>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"
#include "res/act_state.h"

namespace swd3
{

class ErrorReporter;

// Texts of the archive (Big5 as in the executable).
namespace act_text
{

constexpr char kErrorTitle[] = "ACT\277\371\273~";  // "ACT error"
// "<<ACT:%d>> action, part %d: wrong ACT number!!"
constexpr char kBadEntry[] =
    "<<ACT:%d\270\271>>\260\312\247@\252\272\262\304%d\254qACT\270\271\275X\253\374\251w\277\371\273~!!";
constexpr char kMemoryError[] =
    "ACT\260O\276\320\305\351\272\336\262z\246\263\273~!!";  // "ACT memory management error!!"
constexpr char kLoadFailedLog[] = "Act error::actno:%d qq:%d";
constexpr char kMemoryErrorTitle[] = "ERR";
constexpr const char* kFileNames[] = {
    "all_char.act",
    "all_item.act",
    "all_magic.act",
    "all_sys.act",
    "all_map1.act",
    "all_map2.act",
};

}  // namespace act_text

// The act archives (0x00432010-0x00433220) and the action script interpreter (0x004321E0): six files addressed by
// id / 3000 whose directory records are cached in ten chains, a byte-limited script cache in ten chains, and direct
// loading. An entry holds a count and a table of offsets to its scripts (sub entries).
class ActArchive
{
public:
    static constexpr int kFileCount = 6;
    static constexpr int kIndicesPerFile = 3000;
    static constexpr int kChains = 10;
    static constexpr int kMapAct = 0xFFFF;

    explicit ActArchive(ErrorReporter& errors);

    void SetGameDirectory(std::string directory);
    void SetCacheLimit(int bytes);
    ActScript Get(int id, int sub);
    ActScript LoadDirect(int id, int sub);
    int CloseAll();
    void SetUseCache(int value);
    int CacheBytes() const;
    bool ScriptRestarted() const;
    int Run(ActState& state);

private:
    struct IndexEntry
    {
        int file = 0;
        int entry = 0;
        u32 offset = 0;
        u32 size = 0;
    };

    struct CacheEntry
    {
        int id = 0;
        int sub = 0;
        u32 size = 0;
        ActScript data;
    };

    void OpenArchives();
    bool OpenFile(File& file, const std::string& name);
    ActScript ExtractEntry(const std::vector<u8>& data, int index, u32& size) const;
    ActScript Load(File* file, int fileIndex, int entry, int sub, u32& size);
    bool CacheLoad(int id, int sub);
    bool CacheFind(int id, int sub);
    void CacheEvict();
    void CacheClear();
    void IndexInsert(File& file, int fileIndex, int entry, u32& offset, u32& size);
    void IndexLookup(int fileIndex, int entry, u32& offset, u32& size);
    void IndexClear();
    void Restart(ActState& state, u32 keptFlags);

    ErrorReporter& errors;
    std::string gameDirectory;
    std::array<File, kFileCount> files;
    bool opened = false;
    int cacheLimit = 0x100000;
    int cacheBytes = 0;
    int useCache = 0;
    bool restarted = false;
    std::array<std::list<CacheEntry>, kChains> chains;
    std::array<u16, kChains> chainCounts{};
    std::array<std::list<IndexEntry>, kChains> index;
    std::array<u16, kChains> indexCounts{};
    ActScript current;
    ActScript direct;
    std::string errorText;
};

}  // namespace swd3
