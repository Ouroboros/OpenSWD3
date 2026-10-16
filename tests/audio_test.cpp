#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <new>

#include "audio/audio.h"
#include "data/globals.h"
#include "test.h"

using namespace swd3;

namespace {

#if defined(_WIN32)
const char* kGameDir = "E:\\Game\\swd3\\";
#else
const char* kGameDir = "/mnt/e/Game/swd3/";
#endif

}  // namespace

TEST_CASE("audio.sound effects index, RIFF rebuild and playback bookkeeping") {
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    SoundEffects* effects = new (&G.soundEffects) SoundEffects();
    if (!effects->LoadIndex(kGameDir)) {
        std::printf("  (all.snd not found at %s, skipped)\n", kGameDir);
        return;
    }
    int id = 0;
    for (int i = 0; i < 3000; ++i) {
        if (effects->m_table[i].offset != 0 && (effects->m_table[i].sizeFlags >> 26) == 0) {
            id = i + 1;
            break;
        }
    }
    CHECK(id != 0);
    std::uint8_t* image = effects->LoadSound(id);
    CHECK(std::memcmp(image, "RIFF", 4) == 0);
    CHECK(std::memcmp(image + 8, "WAVEfmt ", 8) == 0);
    CHECK(std::memcmp(image + 0x24, "data", 4) == 0);
    std::uint16_t tag;
    std::memcpy(&tag, image + 0x14, 2);
    CHECK_EQ(tag, 1);
    mss::AIL_mem_free_lock(image);
    effects->FreeIndex();

    CHECK(effects->Open(kGameDir));
    CHECK(effects->IsOpen());
    CHECK_EQ(effects->m_voices, 16);
    CHECK_EQ(effects->m_rate, 0xAC44);
    CHECK_EQ(effects->Play(nullptr, id, 100, 0, 1, 0), 0);
    CHECK(effects->Find(id) != nullptr);
    CHECK_EQ(effects->m_table[id - 1].users, 1);
    CHECK_EQ(effects->SetPan(id, 100), 0x7F);
    CHECK_EQ(effects->Stop(id), 0);
    CHECK(effects->Find(id) == nullptr);
    CHECK_EQ(effects->m_table[id - 1].users, 0);
    CHECK(effects->m_table[id - 1].data == nullptr);
    effects->~SoundEffects();
    std::memset(static_cast<void*>(&G.soundEffects), 0, sizeof(G.soundEffects));
}

TEST_CASE("audio.clamps and fade steps") {
    CHECK_EQ(ClampVolume(200), 0x7F);
    CHECK_EQ(ClampVolume(-3), 0);
    CHECK_EQ(ClampPan(0), 0x3F);
    CHECK_EQ(ClampPan(-70), 0);
    SequenceNode node{};
    node.volume = 0x10 * 100;
    CHECK_EQ(SetFadeStep(&node, 3), 533);
    CHECK_EQ(node.fadeStep, 533);
    CHECK_EQ(SetFadeStep(&node, 5000), 0);
    CHECK_EQ(node.fadeStep, 1);
}

TEST_CASE("audio.midi open fails without a MIDI device") {
    MidiPlayer* midi = new (&G.midiPlayer) MidiPlayer();
    CHECK(!midi->Open(0));
    CHECK(std::strstr(midi->m_error, " //") != nullptr);
    CHECK_EQ(midi->Play("x.mid", 1, 100, 1), 0);
    CHECK(midi->IsStopped(0));
    midi->~MidiPlayer();
}
