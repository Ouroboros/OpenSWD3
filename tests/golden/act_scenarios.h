#pragma once

// Action script scenarios shared by the legacy golden generator and the ActArchive test (see draw_scenarios.h).

#include <functional>
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
        backend.SetCacheLimit(useCache == 1 ? 4000 : 0x100000);

        for (int slot = 0; slot < static_cast<int>(sizeof(starts) / sizeof(starts[0])); ++slot)
        {
            const Start& start = starts[slot];
            backend.Reset(slot, start.id, start.action, start.direction, start.speed);
        }

        for (int frame = 0; frame < 60; ++frame)
        {
            for (int slot = 0; slot < static_cast<int>(sizeof(starts) / sizeof(starts[0])); ++slot)
            {
                const Start& start = starts[slot];

                if (frame == 20)
                {
                    backend.Change(slot, start.id, start.action, start.direction + 1, 0);
                }
                else if (frame == 35)
                {
                    backend.Change(slot, start.id, start.action + 8, start.direction + 1, slot % 2);
                }
                else if (frame == 45)
                {
                    backend.Change(slot, start.id, start.action + 8, start.direction + 1, 0);
                }
                else if (frame == 50 && slot == 2)
                {
                    backend.Change(slot, 9005, 100, 0, 0);
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
