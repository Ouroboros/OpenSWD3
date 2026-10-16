#include "battle/fame.h"

#include <cstring>

#include "codec/lzo.h"
#include "core/mem.h"
#include "crt/crt.h"
#include "data/globals.h"
#include "swd3/w32.h"

namespace swd3 {

using std::int16_t;
using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

namespace {

// the list holder itself is checked first (its count word at +4 compared with the id), then every node
uint8_t* findNode(uint8_t* list, int16_t id, uint8_t*& last) {
    uint8_t* node = list;
    last = list;
    if (id == mem::s16(node, 4)) {
        return node;
    }
    for (uint8_t* next = mem::ptr<uint8_t>(node, 0); next != nullptr; next = mem::ptr<uint8_t>(node, 0)) {
        node = next;
        last = node;
        if (id == mem::s16(node, 4)) {
            return node;
        }
    }
    return nullptr;
}

uint8_t* appendNode(uint8_t* last) {
    auto* node = static_cast<uint8_t*>(crt::malloc(0x14));
    mem::putPtr(last, 0, node);
    std::memset(node, 0, 0x14);
    return node;
}

uint8_t* loadList(uint32_t* holder, const uint8_t* block) {
    const uint16_t count = mem::u16(block, 4);
    const uint8_t* entry = block + 10;
    *reinterpret_cast<uint16_t*>(holder + 1) = count;
    uint32_t* link = holder;
    for (uint16_t left = count; left != 0; --left) {
        auto* node = static_cast<uint8_t*>(crt::malloc(0x14));
        *link = static_cast<uint32_t>(reinterpret_cast<std::uintptr_t>(node));
        std::memset(node, 0, 0x14);
        std::memcpy(node + 4, entry, 0xE);
        entry += 0xE;
        link = reinterpret_cast<uint32_t*>(node);
    }
    return const_cast<uint8_t*>(entry);
}

void freeList(uint32_t* holder) {
    while (holder[0] != 0) {
        uint32_t* link = holder;
        auto* node = reinterpret_cast<uint32_t*>(static_cast<std::uintptr_t>(holder[0]));
        while (node[0] != 0) {
            link = node;
            node = reinterpret_cast<uint32_t*>(static_cast<std::uintptr_t>(node[0]));
        }
        crt::free(reinterpret_cast<void*>(static_cast<std::uintptr_t>(*link)));
        *link = 0;
    }
    for (int i = 0; i < 5; ++i) {
        holder[i] = 0;
    }
}

}  // namespace

// swd32.exe 0x004776A0
void CloseRecordFiles() {
    auto first = static_cast<w32::HANDLE>(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.monsterFile)));
    if (first != w32::INVALID_HANDLE_VALUE && first != nullptr) {
        w32::CloseHandle(first);
        G.monsterFile = -1;
    }
    auto second = static_cast<w32::HANDLE>(reinterpret_cast<void*>(static_cast<std::uintptr_t>(G.monsterFile2)));
    if (second != w32::INVALID_HANDLE_VALUE && second != nullptr) {
        w32::CloseHandle(second);
        G.monsterFile2 = -1;
    }
    G.monsterFileOpen = 0;
    G.monsterFile2Open = 0;
}

// swd32.exe 0x004776F0
int ClearCounterList(uint32_t* list) {
    for (int i = 0; i < 5; ++i) {
        list[i] = 0;
    }
    return 0;
}

// swd32.exe 0x00477710: adds `amount` to the counter `id` while below 20 (a new counter takes it unchecked)
uint16_t AddCounterCapped(uint8_t* list, int16_t id, int16_t amount) {
    uint8_t* last = nullptr;
    if (uint8_t* node = findNode(list, id, last)) {
        uint16_t value = mem::u16(node, 6);
        if (value < 0x14) {
            value = static_cast<uint16_t>(value + amount);
            mem::put16(node, 6, value);
        }
        return value;
    }
    uint8_t* node = appendNode(last);
    mem::put16(node, 6, static_cast<uint16_t>(mem::u16(node, 6) + amount));
    mem::put16(node, 4, static_cast<uint16_t>(id));
    ++mem::ref<uint16_t>(list, 4);
    return static_cast<uint16_t>(amount);
}

// swd32.exe 0x00477780: sets the counter `id` to at most 20
uint16_t SetCounterCapped(uint8_t* list, int16_t id, uint16_t value) {
    uint8_t* last = nullptr;
    uint8_t* node = findNode(list, id, last);
    if (node == nullptr) {
        node = appendNode(last);
        mem::put16(node, 4, static_cast<uint16_t>(id));
        mem::put16(node, 6, value);
        if (value > 0x14) {
            mem::put16(node, 6, 0x14);
        }
        ++mem::ref<uint16_t>(list, 4);
        return value;
    }
    mem::put16(node, 6, value);
    if (value > 0x14) {
        mem::put16(node, 6, 0x14);
    }
    return value;
}

// swd32.exe 0x00477800
int16_t FindCounterValue(uint8_t* list, int16_t id) {
    uint8_t* last = nullptr;
    if (uint8_t* node = findNode(list, id, last)) {
        return mem::s16(node, 6);
    }
    return 0;
}

// swd32.exe 0x00477830: one more step of `id` up to `maximum`; the percent is kept and value / maximum * `scale`
// returned
int StepCounterProgress(uint8_t* list, int16_t id, uint16_t maximum, uint16_t scale) {
    uint8_t* last = nullptr;
    if (uint8_t* node = findNode(list, id, last)) {
        mem::put16(node, 6, static_cast<uint16_t>(mem::u16(node, 6) + 1));
        if (mem::u16(node, 6) >= maximum) {
            mem::put16(node, 6, maximum);
        }
        const double ratio = static_cast<double>(mem::u16(node, 6)) / static_cast<double>(maximum);
        mem::put16(node, 8, static_cast<uint16_t>(crt::ftol(ratio * 100.0)));
        return static_cast<int>(crt::ftol(ratio * static_cast<double>(scale)));
    }
    uint8_t* node = appendNode(last);
    const double ratio = 1.0 / static_cast<double>(maximum);
    mem::put16(node, 4, static_cast<uint16_t>(id));
    mem::put16(node, 6, 1);
    mem::put16(node, 8, static_cast<uint16_t>(crt::ftol(ratio * 100.0)));
    ++mem::ref<uint16_t>(list, 4);
    return static_cast<int>(crt::ftol(ratio * static_cast<double>(scale)));
}

// swd32.exe 0x00477920: sets `id` to `value` (at most `maximum`) with its percent
void SetCounterProgress(uint8_t* list, int16_t id, uint16_t maximum, uint16_t value) {
    uint8_t* last = nullptr;
    uint8_t* node = findNode(list, id, last);
    const bool added = node == nullptr;
    if (added) {
        node = appendNode(last);
        mem::put16(node, 4, static_cast<uint16_t>(id));
    }
    mem::put16(node, 6, value);
    if (value >= maximum) {
        mem::put16(node, 6, maximum);
    }
    const double percent = static_cast<double>(mem::u16(node, 6)) / static_cast<double>(maximum) * 100.0;
    mem::put16(node, 8, static_cast<uint16_t>(crt::ftol(percent)));
    if (added) {
        ++mem::ref<uint16_t>(list, 4);
    }
}

// swd32.exe 0x004779F0
int16_t FindCounterPercent(uint8_t* list, int16_t id) {
    uint8_t* last = nullptr;
    if (uint8_t* node = findNode(list, id, last)) {
        return mem::s16(node, 8);
    }
    return 0;
}

// swd32.exe 0x00477F10: the LZO block (+2 size, +6 packed size, +0xA data) holds the three fame lists, each a
// 10 byte header (count at +4) and 14 byte entries; the lists are replaced, not appended
int LoadFame(const uint8_t* block) {
    const uint32_t size = mem::u32(block, 2);
    const uint32_t packed = mem::u32(block, 6);
    const uint8_t* source = block + 0xA;
    auto* unpacked = static_cast<uint8_t*>(crt::malloc(size));
    std::memset(unpacked, 0, size);
    auto* copy = static_cast<uint8_t*>(crt::malloc(packed));
    std::memcpy(copy, source, packed);
    uint32_t length = 0;
    Lzo1xDecompress(source, packed, unpacked, &length);
    if (length != size) {
        w32::MessageBoxA(nullptr, "Load fame decompress false !!", "error", 0);
        crt::free(copy);
        return 0;
    }
    crt::free(copy);
    const uint8_t* next = loadList(G.fameList1, unpacked);
    next = loadList(G.fameList2, next);
    loadList(G.fameList3, next);
    crt::free(unpacked);
    return 1;
}

// swd32.exe 0x00478110: frees the three fame lists from their tails
void FreeFame() {
    freeList(G.fameList1);
    freeList(G.fameList2);
    freeList(G.fameList3);
}

}  // namespace swd3
