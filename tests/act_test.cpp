#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <string>

#include "core/types.h"
#include "game_data.h"
#include "golden/act_scenarios.h"
#include "golden_check.h"
#include "recording_reporter.h"
#include "res/act_archive.h"
#include "test.h"

using swd3::i32;
using swd3::u16;
using swd3::u64;

namespace
{

constexpr std::size_t kLeadWords = 2;  // the first words of a script go into the snapshot
constexpr std::size_t kStates = 8;

class ArchiveBackend : public golden::ActBackend
{
public:
    explicit ArchiveBackend(swd3::ActArchive& archive)
        : archive(archive)
    {
    }

    void SetUseCache(int value) override
    {
        this->archive.SetUseCache(value);
    }

    void SetCacheLimit(int bytes) override
    {
        this->archive.SetCacheLimit(bytes);
    }

    void Reset(int slot, int id, int action, int direction, int speed) override
    {
        swd3::ActState& state = this->states[static_cast<std::size_t>(slot)];
        state = swd3::ActState{};
        state.id = id;
        state.action = action;
        state.direction = direction;
        state.speed = static_cast<swd3::i16>(speed);
    }

    void Change(int slot, int id, int action, int direction, int hold) override
    {
        swd3::ActState& state = this->states[static_cast<std::size_t>(slot)];
        state.id = id;
        state.action = action;
        state.direction = direction;
        state.hold = hold;
    }

    int Run(int slot) override
    {
        return this->archive.Run(this->states[static_cast<std::size_t>(slot)]);
    }

    golden::ActSnapshot Snapshot(int slot) override
    {
        const swd3::ActState& s = this->states[static_cast<std::size_t>(slot)];
        golden::ActSnapshot snapshot;
        auto& f = snapshot.fields;
        f = {
            s.id,
            s.lastId,
            s.action,
            s.lastAction,
            s.x,
            s.y,
            static_cast<i32>(s.flags),
            s.unknown1C,
            s.savedAction,
            s.effectA,
            s.effectQ,
            s.width,
            s.height,
            s.direction,
            s.lastDirection,
            s.savedDirection,
            s.counter,
            s.position,
            s.delay,
            s.frameDelay,
            s.speed,
            s.frame,
            s.picture,
            s.rotation,
            s.actionCode,
            s.unknown52,
        };

        std::array<u16, kLeadWords> first = {};

        if (s.script != nullptr && s.script->size() >= sizeof(first))
        {
            std::memcpy(first.data(), s.script->data(), sizeof(first));
        }

        f.push_back(s.script != nullptr ? 1 : 0);
        f.push_back(first[0]);
        f.push_back(first[1]);
        const i32 rest[] = {
            s.view,
            s.target,
            s.unknown5C,
            s.offsetX,
            s.offsetY,
            s.drawLevel,
            s.red,
            s.green,
            s.blue,
            s.unknown6A[0],
            s.unknown6A[1],
            s.unknown6A[std::size(s.unknown6A) - 1],
            s.red2,
            s.green2,
            s.blue2,
            s.anchorX,
            s.anchorY,
        };

        f.insert(f.end(), std::begin(rest), std::end(rest));

        for (u16 light : s.lights)
        {
            f.push_back(light);
        }

        const i32 tail[] = {s.wait, s.wavePhase, s.signal, s.unknown8B, s.waiting, s.hold, s.mask};
        f.insert(f.end(), std::begin(tail), std::end(tail));
        return snapshot;
    }

    int CacheBytes() override
    {
        return this->archive.CacheBytes();
    }

    void CloseAll() override
    {
        this->archive.CloseAll();

        for (swd3::ActState& state : this->states)
        {
            state.script = nullptr;
        }
    }

private:
    swd3::ActArchive& archive;
    std::array<swd3::ActState, kStates> states;
};

}  // namespace

TEST_CASE("act: matches the golden results of the legacy archives and ActRun")
{
    const std::map<std::string, std::string> golden =
        swd3test::LoadGolden(SWD3_SOURCE_DIR "/tests/golden/act_golden.txt");

    if (golden.empty() || swd3test::ReadGameFile("all_sys.act").empty())
    {
        std::printf("  (game data or golden file not found, skipped)\n");
        return;
    }

    swd3test::RecordingReporter errors;
    swd3::ActArchive archive(errors);
    archive.SetGameDirectory(swd3test::GameDirectory());
    ArchiveBackend backend(archive);
    swd3test::GoldenCheck check(golden);
    golden::RunAct(backend, std::ref(check));
    std::printf("  error dialogs: %zu\n", errors.messages.size());
    std::printf("  compared %d scenarios\n", check.compared);
    CHECK_EQ(check.mismatches, 0);
    CHECK_EQ(check.compared, static_cast<int>(golden.size()));
}
