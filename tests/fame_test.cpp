#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "core/byte_reader.h"
#include "game_data.h"
#include "org/fame.h"
#include "recording_reporter.h"
#include "test.h"

using swd3::CounterList;
using swd3::u16;
using swd3::u32;
using swd3::u8;

namespace
{

constexpr swd3::i16 kCapped = 7;  // a capped counter
constexpr swd3::i16 kFirstAmount = 3;
constexpr swd3::i16 kSecondAmount = 30;  // added while below the cap, so the counter passes it
constexpr u16 kOverCap = 50;
constexpr swd3::i16 kProgress = 9;  // a counter stepped towards a maximum
constexpr u16 kSteps = 4;
constexpr u16 kScale = 1000;
constexpr int kPercent = 100;
constexpr swd3::i16 kSet = 11;  // a counter set at once
constexpr u16 kSetValue = 8;
constexpr int kCounters = 3;  // the head counts the nodes it holds

// The fame block of an original save.
std::span<const u8> FameBlock(const std::vector<u8>& save)
{
    const std::size_t at = swd3test::FindSaveBlocks(save).fame;
    const u32 length = swd3::ByteReader::Word32At(save, at);
    return std::span<const u8>(save.data() + at + sizeof(u32), length);
}

}  // namespace

TEST_CASE("fame: the counters compare the head (its count) first")
{
    CounterList list;
    CHECK_EQ(swd3::AddCounterCapped(list, kCapped, kFirstAmount), kFirstAmount);
    CHECK_EQ(list.id, 1);
    CHECK_EQ(swd3::AddCounterCapped(list, kCapped, kSecondAmount), kFirstAmount + kSecondAmount);
    CHECK_EQ(swd3::AddCounterCapped(list, kCapped, 1), kFirstAmount + kSecondAmount);
    CHECK_EQ(swd3::FindCounterValue(list, kCapped), kFirstAmount + kSecondAmount);
    CHECK_EQ(swd3::SetCounterCapped(list, list.id, kOverCap), kOverCap);
    CHECK_EQ(list.value, swd3::CounterNode::Cap);
    CHECK_EQ(list.id, 1);
    CHECK_EQ(swd3::StepCounterProgress(list, kProgress, kSteps, kScale), kScale / kSteps);
    CHECK_EQ(swd3::StepCounterProgress(list, kProgress, kSteps, kScale), 2 * kScale / kSteps);
    CHECK_EQ(swd3::FindCounterPercent(list, kProgress), 2 * kPercent / kSteps);
    swd3::SetCounterProgress(list, kSet, kSetValue, kPercent);
    CHECK_EQ(swd3::FindCounterValue(list, kSet), kSetValue);
    CHECK_EQ(swd3::FindCounterPercent(list, kSet), kPercent);
    CHECK_EQ(list.id, kCounters);
    swd3::ClearCounterList(list);
    CHECK(list.next == nullptr);
    CHECK_EQ(list.id, 0);
}

TEST_CASE("fame: the fame block of original saves is saved again unchanged")
{
    int blocks = 0;

    for (const std::string name : {"Save/0.sav", "Save/1.sav"})
    {
        const std::vector<u8> save = swd3test::ReadGameFile(name);

        if (save.size() < swd3test::kSaveBytes)
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
