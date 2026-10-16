#include <cstdio>
#include <cstring>
#include <span>
#include <vector>

#include "game_data.h"
#include "org/fame.h"
#include "recording_reporter.h"
#include "test.h"

using swd3::CounterList;
using swd3::u32;
using swd3::u8;

namespace
{

u32 Dword(const std::vector<u8>& data, std::size_t offset)
{
    u32 value = 0;
    std::memcpy(&value, data.data() + offset, 4);
    return value;
}

// The fame block of an original save (after the thumbnail, the three LZO blocks, the position and the fixed blocks).
std::span<const u8> FameBlock(const std::vector<u8>& save)
{
    std::size_t at = 0x962C;
    at += 8 + Dword(save, at);
    at += 8 + Dword(save, at) + 7 * 4;
    at += 8 + Dword(save, at) + 0x40 + 4 + 0x40 + 0x180;
    const u32 length = Dword(save, at);
    return std::span<const u8>(save.data() + at + 4, length);
}

}  // namespace

TEST_CASE("fame: the counters compare the head (its count) first")
{
    CounterList list;
    CHECK_EQ(swd3::AddCounterCapped(list, 7, 3), 3);
    CHECK_EQ(list.id, 1);
    CHECK_EQ(swd3::AddCounterCapped(list, 7, 30), 33);
    CHECK_EQ(swd3::AddCounterCapped(list, 7, 1), 33);
    CHECK_EQ(swd3::FindCounterValue(list, 7), 33);
    CHECK_EQ(swd3::SetCounterCapped(list, 1, 50), 50);
    CHECK_EQ(list.value, 20);
    CHECK_EQ(list.id, 1);
    CHECK_EQ(swd3::StepCounterProgress(list, 9, 4, 1000), 250);
    CHECK_EQ(swd3::StepCounterProgress(list, 9, 4, 1000), 500);
    CHECK_EQ(swd3::FindCounterPercent(list, 9), 50);
    swd3::SetCounterProgress(list, 11, 8, 100);
    CHECK_EQ(swd3::FindCounterValue(list, 11), 8);
    CHECK_EQ(swd3::FindCounterPercent(list, 11), 100);
    CHECK_EQ(list.id, 3);
    swd3::ClearCounterList(list);
    CHECK(list.next == nullptr);
    CHECK_EQ(list.id, 0);
}

TEST_CASE("fame: the fame block of original saves is saved again unchanged")
{
    int blocks = 0;

    for (const char* name : {"Save/0.sav", "Save/1.sav"})
    {
        const std::vector<u8> save = swd3test::ReadGameFile(name);

        if (save.size() < 0x9700)
        {
            continue;
        }

        const std::span<const u8> original = FameBlock(save);
        swd3::Fame fame;
        swd3test::RecordingReporter reporter;
        CHECK(fame.Load(original, reporter));
        std::vector<u8> saved;
        CHECK(fame.Save(saved, reporter));
        CHECK(saved == std::vector<u8>(original.begin(), original.end()));
        CHECK(reporter.messages.empty());
        fame.Free();
        CHECK(fame.lists[0].next == nullptr);
        ++blocks;
    }

    std::printf("  blocks: %d\n", blocks);
}
