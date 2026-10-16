// Golden generator for the act archives and ActRun: runs tests/golden/act_scenarios.h through the legacy
// transliteration and writes "name hash" lines to $GOLDEN_OUT (see README.md).

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "data/globals.h"
#include "golden/act_scenarios.h"
#include "res/act.h"
#include "res/act_anim.h"
#include "test.h"

using namespace swd3;

namespace
{

class LegacyAct : public golden::ActBackend
{
public:
    void SetUseCache(int value) override
    {
        ActSetOption(value);
    }

    void SetCacheLimit(int bytes) override
    {
        ActSetCacheLimit(bytes);
    }

    void Reset(int slot, int id, int action, int direction, int speed) override
    {
        ActState& state = this->states[slot];
        std::memset(&state, 0, sizeof(state));
        state.id = id;
        state.action = action;
        state.direction = direction;
        state.speed = static_cast<std::int16_t>(speed);
    }

    void Change(int slot, int id, int action, int direction, int hold) override
    {
        ActState& state = this->states[slot];
        state.id = id;
        state.action = action;
        state.direction = direction;
        state.hold = hold;
    }

    int Run(int slot) override
    {
        return ActRun(&this->states[slot]);
    }

    golden::ActSnapshot Snapshot(int slot) override
    {
        const ActState& s = this->states[slot];
        golden::ActSnapshot snapshot;
        auto& f = snapshot.fields;
        f = {s.id, s.lastId, s.action, s.lastAction, s.x, s.y, static_cast<std::int32_t>(s.flags), s.unknown1C,
             s.unknown20, s.effectA, s.effectQ, s.width, s.height, s.direction, s.lastDirection, s.unknown3C,
             s.counter, s.pc, s.delay, s.frameDelay, s.speed, s.fr, s.ap, s.ro, s.ao, s.unknown52};
        std::uint16_t first[2] = {};

        if (s.script != nullptr)
        {
            std::memcpy(first, s.script, 4);
        }

        f.push_back(s.script != nullptr ? 1 : 0);
        f.push_back(first[0]);
        f.push_back(first[1]);
        const std::int32_t rest[] = {s.vw, s.ta, s.unknown5C, s.xo, s.yo, s.dl, s.rc, s.gc, s.bc, s.unknown6A[0],
                                     s.unknown6A[1], s.unknown6A[2], s.rc2, s.gc2, s.bc2, s.xa, s.ya};
        f.insert(f.end(), std::begin(rest), std::end(rest));

        for (std::uint16_t light : s.lf)
        {
            f.push_back(light);
        }

        const std::int32_t tail[] = {s.wt, s.unknown89, s.sg, s.unknown8B, s.waiting, s.hold, s.ms};
        f.insert(f.end(), std::begin(tail), std::end(tail));
        return snapshot;
    }

    int CacheBytes() override
    {
        return G.actCacheBytes;
    }

    void CloseAll() override
    {
        ActCloseAll();

        for (ActState& state : this->states)
        {
            state.script = nullptr;
        }
    }

private:
    ActState states[8] = {};
};

}  // namespace

TEST_CASE("golden: legacy act archives")
{
    const char* output = std::getenv("GOLDEN_OUT");
    const char* gameDir = std::getenv("SWD3_GAME_DIR");

    if (output == nullptr || gameDir == nullptr)
    {
        std::printf("  (GOLDEN_OUT / SWD3_GAME_DIR not set, skipped)\n");
        return;
    }

    std::snprintf(G.gameDir, sizeof(G.gameDir), "%s/", gameDir);
    std::FILE* file = std::fopen(output, "w");
    LegacyAct backend;
    golden::RunAct(
        backend,
        [file](const std::string& name, std::uint64_t hash)
        {
            std::fprintf(file, "%s %016llx\n", name.c_str(), static_cast<unsigned long long>(hash));
            std::fflush(file);
        }
    );
    std::fclose(file);
}
