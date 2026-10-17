#include "org/fame.h"

#include <cstring>

#include "codec/lzo.h"
#include "core/byte_reader.h"
#include "crt/msvc.h"
#include "org/database.h"
#include "platform/message_box.h"

namespace swd3
{

namespace
{

constexpr u16 kCap = 0x14;
constexpr std::size_t kHeaderBytes = 0xA;
constexpr std::size_t kBlockHeaderBytes = 0xA;
constexpr std::size_t kSlackBytes = 0x1E;
constexpr int kFameBits = 15;
constexpr double kPercent = 100.0;
constexpr std::size_t kCountAt = sizeof(u32);  // a list header: its bytes, then its node count
constexpr std::size_t kVersionAt = 0;          // the save block: 1, the unpacked and the packed size
constexpr u8 kBlockVersion = 1;
constexpr std::size_t kUnpackedAt = 2;
constexpr std::size_t kPackedAt = 6;

// The node `id` of a fame lookup: the head first, then every node; `last` is left on the last node visited.
CounterNode* FindNode(CounterList& list, i16 id, CounterNode*& last)
{
    CounterNode* node = &list;
    last = node;

    if (id == node->id)
    {
        return node;
    }

    while (node->next != nullptr)
    {
        node = node->next.get();
        last = node;

        if (id == node->id)
        {
            return node;
        }
    }

    return nullptr;
}

CounterNode& Append(CounterNode& last)
{
    last.next = std::make_unique<CounterNode>();
    return *last.next;
}

void CountOne(CounterList& list)
{
    list.id = static_cast<i16>(static_cast<u16>(list.id) + 1);
}

u16 Percent(u16 value, u16 maximum)
{
    return static_cast<u16>(crt::Ftol(static_cast<double>(value) / static_cast<double>(maximum) * kPercent));
}

void PutDword(std::vector<u8>& data, std::size_t at, u32 value)
{
    std::memcpy(data.data() + at, &value, sizeof(value));
}

u32 GetDword(std::span<const u8> data, std::size_t at)
{
    return ByteReader::Word32At(data, at);
}

void SaveNode(const CounterNode& node, u8* entry)
{
    auto put = [&entry](const void* field, std::size_t bytes)
    {
        std::memcpy(entry, field, bytes);
        entry += bytes;
    };

    put(&node.id, sizeof(node.id));
    put(&node.value, sizeof(node.value));
    put(&node.percent, sizeof(node.percent));
    put(&node.done, sizeof(node.done));
    put(node.unk0C.data(), node.unk0C.size());
}

void LoadNode(CounterNode& node, const u8* entry)
{
    auto get = [&entry](void* field, std::size_t bytes)
    {
        std::memcpy(field, entry, bytes);
        entry += bytes;
    };

    get(&node.id, sizeof(node.id));
    get(&node.value, sizeof(node.value));
    get(&node.percent, sizeof(node.percent));
    get(&node.done, sizeof(node.done));
    get(node.unk0C.data(), node.unk0C.size());
}

}  // namespace

// swd32.exe 0x004776F0: the head is cleared (the original drops the nodes without freeing them)
void ClearCounterList(CounterList& list)
{
    list = CounterNode{};
}

// swd32.exe 0x00477710: adds `amount` to the counter `id` while it is below 20 (a new counter takes it unchecked)
u16 AddCounterCapped(CounterList& list, i16 id, i16 amount)
{
    CounterNode* last = nullptr;

    if (CounterNode* node = FindNode(list, id, last))
    {
        u16 value = node->value;

        if (value < kCap)
        {
            value = static_cast<u16>(value + amount);
            node->value = value;
        }

        return value;
    }

    CounterNode& node = Append(*last);
    node.value = static_cast<u16>(node.value + amount);
    node.id = id;
    CountOne(list);
    return static_cast<u16>(amount);
}

// swd32.exe 0x00477780: sets the counter `id` to at most 20; returns the value asked for
u16 SetCounterCapped(CounterList& list, i16 id, u16 value)
{
    CounterNode* last = nullptr;
    CounterNode* node = FindNode(list, id, last);

    if (node == nullptr)
    {
        node = &Append(*last);
        node->id = id;
        node->value = value > kCap ? kCap : value;
        CountOne(list);
        return value;
    }

    node->value = value > kCap ? kCap : value;
    return value;
}

// swd32.exe 0x00477800
i16 FindCounterValue(CounterList& list, i16 id)
{
    CounterNode* last = nullptr;
    const CounterNode* node = FindNode(list, id, last);
    return node != nullptr ? static_cast<i16>(node->value) : i16{0};
}

// swd32.exe 0x00477830: one more step of `id` up to `maximum`; keeps the percent and returns value / maximum * scale
int StepCounterProgress(CounterList& list, i16 id, u16 maximum, u16 scale)
{
    CounterNode* last = nullptr;

    if (CounterNode* node = FindNode(list, id, last))
    {
        node->value = static_cast<u16>(node->value + 1);

        if (node->value >= maximum)
        {
            node->value = maximum;
        }

        const double ratio = static_cast<double>(node->value) / static_cast<double>(maximum);
        node->percent = static_cast<u16>(crt::Ftol(ratio * 100.0));
        return static_cast<int>(crt::Ftol(ratio * static_cast<double>(scale)));
    }

    CounterNode& node = Append(*last);
    const double ratio = 1.0 / static_cast<double>(maximum);
    node.id = id;
    node.value = 1;
    node.percent = static_cast<u16>(crt::Ftol(ratio * 100.0));
    CountOne(list);
    return static_cast<int>(crt::Ftol(ratio * static_cast<double>(scale)));
}

// swd32.exe 0x00477920: sets `id` to `value` (at most `maximum`) with its percent
void SetCounterProgress(CounterList& list, i16 id, u16 maximum, u16 value)
{
    CounterNode* last = nullptr;
    CounterNode* node = FindNode(list, id, last);
    const bool added = node == nullptr;

    if (added)
    {
        node = &Append(*last);
        node->id = id;
    }

    node->value = value >= maximum ? maximum : value;
    node->percent = Percent(node->value, maximum);

    if (added)
    {
        CountOne(list);
    }
}

// swd32.exe 0x004779F0
i16 FindCounterPercent(CounterList& list, i16 id)
{
    CounterNode* last = nullptr;
    const CounterNode* node = FindNode(list, id, last);
    return node != nullptr ? static_cast<i16>(node->percent) : i16{0};
}

// swd32.exe 0x00477A20: sets the proficiency `id` to `value`, at most the limit of the object (+0x44 of its record)
// with its percent; a finished one (done) is left. The search starts at the head while the list is empty and at the
// first node otherwise.
void SetProficiency(CounterList& list, u16 id, u16 value, Database& database)
{
    CounterNode* node = list.id != 0 ? list.next.get() : &list;
    database.LoadObject(database.scratch, id);
    Database::FreeDescription(database.scratch);
    const u16 maximum = database.scratch.data.unk44;

    if (static_cast<u16>(node->id) != id)
    {
        while (node->next != nullptr)
        {
            node = node->next.get();

            if (static_cast<u16>(node->id) == id)
            {
                break;
            }
        }

        if (static_cast<u16>(node->id) != id)
        {
            CounterNode& added = Append(*node);
            added.id = static_cast<i16>(id);
            added.value = value >= maximum ? maximum : value;
            added.percent = Percent(added.value, maximum);
            CountOne(list);
            return;
        }
    }

    if (node->done != 0)
    {
        return;
    }

    node->value = value >= maximum ? maximum : value;
    node->percent = Percent(node->value, maximum);
}

// swd32.exe 0x00477B40: the limit of the object `id` and its proficiency (0 when there is none)
bool GetProficiency(CounterList& list, u16 id, u16& maximum, u16& value, Database& database)
{
    CounterNode* node = &list;

    while (node != nullptr && static_cast<u16>(node->id) != id)
    {
        node = node->next.get();
    }

    database.LoadObject(database.scratch, id);
    Database::FreeDescription(database.scratch);
    maximum = database.scratch.data.unk44;
    value = node != nullptr ? node->value : u16{0};
    return node != nullptr;
}

// swd32.exe 0x00477C90: the three lists in sections of a 10-byte header (bytes, count) and 14-byte entries, compressed
// with the 15-bit dictionary. The buffer is sized from the counts of the heads; nodes beyond them are written past it
// in the original and left out of the compressed data, as here.
bool Fame::Save(std::vector<u8>& block, ErrorReporter& reporter) const
{
    const auto counts = static_cast<u16>(
        static_cast<u16>(this->lists[Fame::UnkList0].id) + static_cast<u16>(this->lists[Fame::FameList].id) +
        this->lists[Fame::ProficiencyList].id
    );
    const std::size_t size = CounterNode::SavedBytes * counts + kSlackBytes;
    std::vector<u8> data(size, 0);
    std::size_t section = 0;

    for (const CounterList& list : this->lists)
    {
        std::size_t entry = section + kHeaderBytes;
        u16 nodes = 0;

        for (const CounterNode* node = list.next.get(); node != nullptr; node = node->next.get())
        {
            if (entry + CounterNode::SavedBytes > data.size())
            {
                data.resize(entry + CounterNode::SavedBytes, 0);
            }

            SaveNode(*node, data.data() + entry);
            entry += CounterNode::SavedBytes;
            ++nodes;
        }

        if (nodes != 0)
        {
            PutDword(data, section, static_cast<u32>(kHeaderBytes + CounterNode::SavedBytes * nodes));
            std::memcpy(data.data() + section + kCountAt, &nodes, sizeof(u16));
        }

        section = entry;
    }

    const std::vector<u8> packed = LzoEncoder(kFameBits).Compress(std::span<const u8>(data.data(), size));

    if (packed.size() >= size)
    {
        reporter.ShowError(u8"error", u8"save fame compress false !!");
        return false;
    }

    block.assign(kBlockHeaderBytes + packed.size(), 0);
    block[kVersionAt] = kBlockVersion;
    PutDword(block, kUnpackedAt, static_cast<u32>(size));
    PutDword(block, kPackedAt, static_cast<u32>(packed.size()));
    std::memcpy(block.data() + kBlockHeaderBytes, packed.data(), packed.size());
    return true;
}

// swd32.exe 0x00477F10: replaces the three lists from a save block (the count goes into the low word of the head)
bool Fame::Load(std::span<const u8> block, ErrorReporter& reporter)
{
    const u32 size = GetDword(block, kUnpackedAt);
    const u32 packedSize = GetDword(block, kPackedAt);
    std::vector<u8> data(size, 0);
    const std::span<const u8> packed =
        block.subspan(std::min<std::size_t>(kBlockHeaderBytes, block.size()))
            .first(
                std::min<std::size_t>(
                    packedSize,
                    block.size() > kBlockHeaderBytes ? block.size() - kBlockHeaderBytes : 0
                )
            );
    LzoDecoder decoder(packed, data);
    decoder.Run();

    if (decoder.Written() != size)
    {
        reporter.ShowError(u8"error", u8"Load fame decompress false !!");
        return false;
    }

    std::size_t at = 0;

    for (CounterList& list : this->lists)
    {
        u16 count = 0;

        if (at + kCountAt + sizeof(u16) <= data.size())
        {
            std::memcpy(&count, data.data() + at + kCountAt, sizeof(u16));
        }

        list.id = static_cast<i16>(count);
        at += kHeaderBytes;
        std::unique_ptr<CounterNode>* link = &list.next;

        for (u16 i = 0; i < count; ++i)
        {
            *link = std::make_unique<CounterNode>();

            if (at + CounterNode::SavedBytes <= data.size())
            {
                LoadNode(**link, data.data() + at);
            }

            at += CounterNode::SavedBytes;
            link = &(*link)->next;
        }
    }

    return true;
}

// swd32.exe 0x00478110: frees the three lists and clears their heads
void Fame::Free()
{
    for (CounterList& list : this->lists)
    {
        list = CounterNode{};
    }
}

}  // namespace swd3
