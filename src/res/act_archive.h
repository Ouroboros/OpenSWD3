#pragma once

#include <array>
#include <list>
#include <string>
#include <vector>

#include "core/types.h"
#include "file/file.h"
#include "res/act_state.h"
#include "text/big5.h"

namespace swd3
{

class ErrorReporter;

// The act archives (0x00432010-0x00433220) and the action script interpreter (0x004321E0): six files addressed by
// id / 3000 whose directory records are cached in ten chains, a byte-limited script cache in ten chains, and direct
// loading. An entry holds a count and a table of offsets to its scripts (sub entries).
class ActArchive
{
public:
    // Texts of the archive (Big5 in the executable; u8 here, converted where drawn).
    static constexpr std::u8string_view ErrorTitle = u8"ACT錯誤";  // "ACT error"
    static constexpr std::u8string_view BadEntry = u8"<<ACT:%d號>>動作的第%d段ACT號碼指定錯誤!!";
    static constexpr std::u8string_view MemoryError = u8"ACT記憶體管理有誤!!";  // "ACT memory management error!!"
    static constexpr std::u8string_view LoadFailedLog = u8"Act error::actno:%d qq:%d";
    static constexpr std::u8string_view MemoryErrorTitle = u8"ERR";
    static constexpr std::array<std::string_view, 6> FileNames = {
        "all_char.act",
        "all_item.act",
        "all_magic.act",
        "all_sys.act",
        "all_map1.act",
        "all_map2.act",
    };

    static constexpr int FileCount = 6;
    static constexpr int IndicesPerFile = 3000;
    static constexpr int Chains = 10;
    static constexpr int MapAct = 0xFFFF;

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
    std::array<File, FileCount> files;
    bool opened = false;
    int cacheLimit = 0x100000;
    int cacheBytes = 0;
    int useCache = 0;
    bool restarted = false;
    std::array<std::list<CacheEntry>, Chains> chains;
    std::array<u16, Chains> chainCounts{};
    std::array<std::list<IndexEntry>, Chains> index;
    std::array<u16, Chains> indexCounts{};
    ActScript current;
    ActScript direct;
    std::u8string errorText;
};

}  // namespace swd3
