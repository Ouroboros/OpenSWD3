#pragma once

// Action script scenarios shared by the legacy golden generator and the ActArchive test (see draw_scenarios.h).

#include <functional>
#include <iterator>
#include <string>
#include <vector>

#include "golden/draw_scenarios.h"

namespace golden
{

// The fields of an action state in their original order (the script pointer replaced by its length).
struct ActSnapshot
{
    std::vector<i32> fields;
};

class ActBackend
{
public:
    virtual ~ActBackend() = default;
    virtual void SetUseCache(int value) = 0;
    virtual void SetCacheLimit(int bytes) = 0;
    // Prepares state `slot` from scratch.
    virtual void Reset(int slot, int id, int action, int direction, int speed) = 0;
    virtual void Change(int slot, int id, int action, int direction, int hold) = 0;
    virtual int Run(int slot) = 0;
    virtual ActSnapshot Snapshot(int slot) = 0;
    virtual int CacheBytes() = 0;
    virtual void CloseAll() = 0;
};

inline u64 HashSnapshot(const ActSnapshot& snapshot)
{
    return Hash(snapshot.fields.data(), snapshot.fields.size() * sizeof(i32));
}

constexpr int kSmallCache = 4000;  // the cache limits of the two passes
constexpr int kLargeCache = 0x100000;
constexpr int kActFrames = 60;
constexpr int kTurnFrame = 20;  // every act turns, walks with every second one held, is released
constexpr int kWalkFrame = 35;
constexpr int kReleaseFrame = 45;
constexpr int kWalkAction = 8;
constexpr int kHoldEvery = 2;
constexpr int kSwitchFrame = 50;  // and one changes to an act of another file
constexpr int kSwitchSlot = 2;
constexpr int kSwitchAct = 9005;
constexpr int kSwitchAction = 100;

inline void RunAct(ActBackend& backend, const std::function<void(const std::string&, u64)>& report)
{
    struct Start
    {
        int id;
        int action;
        int direction;
        int speed;
    };

    // Character 1 and 9, system 9001 and 9002, magic 6011, item 3001, map 12003.
    const Start starts[] = {
        {1, 0, 0, 0},
        {9, 8, 1, 0},
        {9001, 0, 0, 0},
        {9002, 16, 2, -0x7FFE},
        {6011, 0, 1, 0},
        {3001, 8, 0, 0},
        {12003, 24, 3, 0},
    };

    for (int useCache = 1; useCache >= 0; --useCache)
    {
        backend.SetUseCache(useCache);
        backend.SetCacheLimit(useCache == 1 ? kSmallCache : kLargeCache);

        for (int slot = 0; slot < static_cast<int>(std::size(starts)); ++slot)
        {
            const Start& start = starts[slot];
            backend.Reset(slot, start.id, start.action, start.direction, start.speed);
        }

        for (int frame = 0; frame < kActFrames; ++frame)
        {
            for (int slot = 0; slot < static_cast<int>(std::size(starts)); ++slot)
            {
                const Start& start = starts[slot];

                if (frame == kTurnFrame)
                {
                    backend.Change(slot, start.id, start.action, start.direction + 1, 0);
                }
                else if (frame == kWalkFrame)
                {
                    backend.Change(slot, start.id, start.action + kWalkAction, start.direction + 1, slot % kHoldEvery);
                }
                else if (frame == kReleaseFrame)
                {
                    backend.Change(slot, start.id, start.action + kWalkAction, start.direction + 1, 0);
                }
                else if (frame == kSwitchFrame && slot == kSwitchSlot)
                {
                    backend.Change(slot, kSwitchAct, kSwitchAction, 0, 0);
                }

                const int result = backend.Run(slot);
                const std::string name = "act/" + std::to_string(useCache) + "/" + std::to_string(frame) + "/" +
                                         std::to_string(slot) + " result " + std::to_string(result);
                report(name, HashSnapshot(backend.Snapshot(slot)));
            }

            report(
                "act/" + std::to_string(useCache) + "/" + std::to_string(frame) + "/bytes",
                static_cast<u64>(backend.CacheBytes())
            );
        }

        backend.CloseAll();
    }
}

}  // namespace golden
