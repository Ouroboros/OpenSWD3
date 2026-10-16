#include "res/act.h"

#include <cstring>

#include "core/debug_log.h"
#include "crt/crt.h"
#include "data/globals.h"

namespace swd3 {
namespace {

void openArchives() {
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllCharAct);
    G.actFiles[0] = ActOpenFile(G.pathBuffer);
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllItemAct);
    G.actFiles[1] = ActOpenFile(G.pathBuffer);
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllMagicAct);
    G.actFiles[2] = ActOpenFile(G.pathBuffer);
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllSysAct);
    G.actFiles[3] = ActOpenFile(G.pathBuffer);
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllMap1Act);
    G.actFiles[4] = ActOpenFile(G.pathBuffer);
    w32::lstrcpyA(G.pathBuffer, G.gameDir);
    w32::lstrcatA(G.pathBuffer, G.aAllMap2Act);
    G.actFiles[5] = ActOpenFile(G.pathBuffer);
    G.actOpened = 1;
}

int bucketFor(int id, int sub) {
    int bucket = id % 10;
    if (id == 0xFFFF) {
        bucket = sub % 10;
    }
    return bucket & 0xFFFF;
}

// The sentinel's first field doubles as a "next" pointer, so the chain can be walked from it as from a node.
ActCacheNode* chainHead(int bucket) { return &G.actBuckets[bucket]; }

}  // namespace

// swd32.exe 0x00432010
void ActSetCacheLimit(int bytes) { G.actCacheLimit = bytes; }

// swd32.exe 0x00432020
HANDLE ActOpenFile(LPCSTR fileName) {
    HANDLE file = w32::CreateFileA(fileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    return file == INVALID_HANDLE_VALUE ? nullptr : file;
}

// swd32.exe 0x00432050
std::uint8_t* ActExtractEntry(std::uint16_t* data, int index, int dataSize, std::uint32_t* size) {
    const int count = data[0];
    auto* table = reinterpret_cast<std::uint8_t*>(data) + index * 4 + 2;
    if (index >= count) {
        return nullptr;
    }
    std::uint32_t start;
    std::memcpy(&start, table, 4);
    table += 4;
    if (start == 0) {
        return nullptr;
    }
    std::uint32_t end;
    if (index == count - 1) {
        end = static_cast<std::uint32_t>(dataSize);
    } else {
        do {
            std::memcpy(&end, table, 4);
            table += 4;
        } while (end == 0);
    }
    const std::uint32_t length = end - start;
    auto* result = static_cast<std::uint8_t*>(crt::malloc(length));
    std::memset(result, 0, length);
    *size = length;
    std::memcpy(result, reinterpret_cast<std::uint8_t*>(data) + start, length);
    return result;
}

// swd32.exe 0x004320E0
std::uint8_t* ActLoad(HANDLE file, int fileIndex, int entry, int sub, std::uint32_t* size) {
    std::uint32_t length = 0;
    std::uint32_t offset = 0;
    ActIndexLookup(fileIndex, entry, &offset, &length);
    if (offset == 0) {
        ActIndexInsert(file, fileIndex, entry, &offset, &length);
    }
    auto* buffer = static_cast<std::uint16_t*>(crt::malloc(length));
    w32::SetFilePointer(file, static_cast<LONG>(offset), nullptr, FILE_BEGIN);
    DWORD read;
    w32::ReadFile(file, buffer, length, &read, nullptr);
    std::uint8_t* result = ActExtractEntry(buffer, sub, static_cast<int>(length), size);
    if (result) {
        crt::free(buffer);
        return result;
    }
    w32::wsprintfA(G.actErrorText, G.aActD, entry, sub);
    DebugLog(G.actErrorText);
    w32::MessageBoxA(nullptr, G.actErrorText, G.aAct, 0);
    crt::free(buffer);
    return nullptr;
}

// swd32.exe 0x00432A50
std::uint8_t* ActGet(int id, int sub) {
    if (!G.actOpened) {
        openArchives();
    }
    if (G.actCacheBytes >= G.actCacheLimit) {
        ActCacheEvict();
    }
    if (ActCacheFind(id, sub) == 1) {
        return G.actCurrent;
    }
    return ActCacheLoad(id, sub) ? G.actCurrent : nullptr;
}

// swd32.exe 0x00432BC0
std::uint8_t* ActLoadDirect(int id, int sub) {
    if (!G.actOpened) {
        openArchives();
    }
    std::uint32_t size;
    const int file = id / 0xBB8;
    G.actDirect = ActLoad(G.actFiles[file], file, id % 0xBB8, sub, &size);
    return G.actDirect;
}

// swd32.exe 0x00432D30
int ActCacheLoad(int id, int sub) {
    const int bucket = bucketFor(id, sub);
    ++G.actBucketCounts[bucket];
    G.actCursor = chainHead(bucket);
    for (ActCacheNode* node = G.actCursor->next; node; node = node->next) {
        G.actCursor = node;
    }
    G.actCursor->next = static_cast<ActCacheNode*>(crt::malloc(0x18));
    std::memset(G.actCursor->next, 0, 0x18);
    G.actCursor = G.actCursor->next;
    G.actCursor->id = id;
    const int file = id / 0xBB8;
    G.actCursor->sub = sub;
    G.actCurrent = ActLoad(G.actFiles[file], file, id % 0xBB8, sub, &G.actCursor->size);
    if (!G.actCurrent) {
        DebugLog(G.aActErrorActnoD, file, sub);
        return 0;
    }
    G.actCursor->data = G.actCurrent;
    G.actCacheBytes += static_cast<std::int32_t>(G.actCursor->size);
    return 1;
}

// swd32.exe 0x00432E40
int ActCacheFind(int id, int sub) {
    const int bucket = bucketFor(id, sub);
    G.actCurrent = nullptr;
    ActCacheNode* head = G.actBuckets[bucket].next;
    ActCacheNode* node = head;
    G.actCursor = node;
    ActCacheNode* prev = node;
    for (;;) {
        if (!node) {
            return 0;
        }
        if (node->id == id && node->sub == sub) {
            break;
        }
        prev = node;
        node = node->next;
        G.actCursor = node;
    }
    if (node != head) {
        prev->next = node->next;
        G.actCursor->next = G.actBuckets[bucket].next;
        node = G.actCursor;
        G.actBuckets[bucket].next = G.actCursor;
    }
    G.actCurrent = node->data;
    return 1;
}

// swd32.exe 0x00432EE0
void ActCacheEvict() {
    std::uint16_t best = 0;
    for (std::uint16_t i = 0; i < 10; ++i) {
        if (G.actBucketCounts[i] > G.actBucketCounts[best]) {
            best = i;
        }
    }
    if (G.actCacheBytes < G.actCacheLimit) {
        return;
    }
    ActCacheNode* sentinel = chainHead(best);
    do {
        ActCacheNode* node = sentinel;
        G.actCursor = node;
        ActCacheNode* prev = node;
        while (node->next) {
            prev = node;
            node = node->next;
            G.actCursor = node;
        }
        G.actCacheBytes -= static_cast<std::int32_t>(node->size);
        crt::free(node->data);
        crt::free(G.actCursor);
        prev->next = nullptr;
        if (--G.actBucketCounts[best] == 0) {
            return;
        }
    } while (G.actCacheBytes >= G.actCacheLimit);
}

// swd32.exe 0x00432FA0
void ActCacheClear() {
    for (int i = 0; i < 10; ++i) {
        ActCacheNode* node = G.actBuckets[i].next;
        G.actBuckets[i].next = nullptr;
        G.actCursor = node;
        while (node) {
            ActCacheNode* next = node->next;
            G.actCacheBytes -= static_cast<std::int32_t>(node->size);
            crt::free(node->data);
            crt::free(G.actCursor);
            --G.actBucketCounts[i];
            node = next;
            G.actCursor = node;
        }
    }
}

// swd32.exe 0x00433010
int ActCloseAll() {
    for (HANDLE& file : G.actFiles) {
        if (file != INVALID_HANDLE_VALUE) {
            w32::CloseHandle(file);
        }
        file = INVALID_HANDLE_VALUE;
    }
    ActCacheClear();
    ActIndexClear();
    const int remaining = G.actCacheBytes;
    G.actOpened = 0;
    if (remaining) {
        return w32::MessageBoxA(G.mainWindow, G.aAct_0, reinterpret_cast<const char*>(&G.off_4A6178), 0x40010);
    }
    return remaining;
}

// swd32.exe 0x00433080
void ActSetOption(int value) { G.actOption = value; }

// swd32.exe 0x00433090
ActIndexNode* ActIndexInsert(HANDLE file, int fileIndex, int entry, std::uint32_t* offset, std::uint32_t* size) {
    std::uint32_t buffer[11] = {};
    const int bucket = (entry % 10) & 0xFFFF;
    ++G.actIndexCounts[bucket];
    G.actIndexCursor = &G.actIndexBuckets[bucket];
    ActIndexNode* oldHead = G.actIndexCursor->next;
    G.actIndexCursor->next = static_cast<ActIndexNode*>(crt::malloc(0x14));
    ActIndexNode* node = G.actIndexCursor->next;
    node->next = nullptr;
    node->file = 0;
    node->entry = 0;
    node->offset = 0;
    node->size = 0;
    G.actIndexCursor = G.actIndexCursor->next;
    G.actIndexCursor->next = oldHead;
    w32::SetFilePointer(file, 0x2C * entry - 0x10, nullptr, FILE_BEGIN);
    DWORD read;
    w32::ReadFile(file, buffer, 0x2C, &read, nullptr);
    G.actIndexCursor->file = fileIndex;
    G.actIndexCursor->entry = entry;
    G.actIndexCursor->offset = buffer[6];
    G.actIndexCursor->size = buffer[5];
    *offset = G.actIndexCursor->offset;
    *size = G.actIndexCursor->size;
    return G.actIndexCursor;
}

// swd32.exe 0x00433190
void ActIndexLookup(int fileIndex, int entry, std::uint32_t* offset, std::uint32_t* size) {
    const int bucket = (entry % 10) & 0xFFFF;
    ActIndexNode* head = G.actIndexBuckets[bucket].next;
    ActIndexNode* node = head;
    G.actIndexCursor = node;
    ActIndexNode* prev = node;
    if (!node) {
        return;
    }
    while (node->file != fileIndex || node->entry != entry) {
        prev = node;
        node = node->next;
        G.actIndexCursor = node;
        if (!node) {
            return;
        }
    }
    if (node != head) {
        prev->next = node->next;
        G.actIndexCursor->next = G.actIndexBuckets[bucket].next;
        node = G.actIndexCursor;
        G.actIndexBuckets[bucket].next = G.actIndexCursor;
    }
    *offset = node->offset;
    *size = G.actIndexCursor->size;
}

// swd32.exe 0x00433220
void ActIndexClear() {
    for (int i = 0; i < 10; ++i) {
        ActIndexNode* node = G.actIndexBuckets[i].next;
        G.actIndexBuckets[i].next = nullptr;
        G.actIndexCursor = node;
        while (node) {
            ActIndexNode* next = node->next;
            crt::free(node);
            --G.actIndexCounts[i];
            node = next;
            G.actIndexCursor = node;
        }
    }
}

}  // namespace swd3
