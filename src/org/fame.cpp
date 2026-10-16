#include "org/fame.h"

#include <cstring>

#include "codec/lzo.h"
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
    return static_cast<u16>(crt::Ftol(static_cast<double>(value) / static_cast<double>(maximum) * 100.0));
}

void PutDword(std::vector<u8>& data, std::size_t at, u32 value)
{
    std::memcpy(data.data() + at, &value, 4);
}

u32 GetDword(std::span<const u8> data, std::size_t at)
{
    u32 value = 0;

    if (at + 4 <= data.size())
    {
        std::memcpy(&value, data.data() + at, 4);
    }

    return value;
}

void SaveNode(const CounterNode& node, u8* entry)
{
    std::memcpy(entry, &node.id, 2);
    std::memcpy(entry + 2, &node.value, 2);
    std::memcpy(entry + 4, &node.percent, 2);
    std::memcpy(entry + 6, &node.done, 2);
    std::memcpy(entry + 8, node.unk0C.data(), node.unk0C.size());
}

void LoadNode(CounterNode& node, const u8* entry)
{
    std::memcpy(&node.id, entry, 2);
    std::memcpy(&node.value, entry + 2, 2);
    std::memcpy(&node.percent, entry + 4, 2);
    std::memcpy(&node.done, entry + 6, 2);
    std::memcpy(node.unk0C.data(), entry + 8, node.unk0C.size());
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
    const auto counts =
        static_cast<u16>(static_cast<u16>(this->lists[0].id) + static_cast<u16>(this->lists[1].id) + this->lists[2].id);
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
            std::memcpy(data.data() + section + 4, &nodes, 2);
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
    block[0] = 1;
    PutDword(block, 2, static_cast<u32>(size));
    PutDword(block, 6, static_cast<u32>(packed.size()));
    std::memcpy(block.data() + kBlockHeaderBytes, packed.data(), packed.size());
    return true;
}

// swd32.exe 0x00477F10: replaces the three lists from a save block (the count goes into the low word of the head)
bool Fame::Load(std::span<const u8> block, ErrorReporter& reporter)
{
    const u32 size = GetDword(block, 2);
    const u32 packedSize = GetDword(block, 6);
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

        if (at + 6 <= data.size())
        {
            std::memcpy(&count, data.data() + at + 4, 2);
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
