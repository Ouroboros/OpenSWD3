#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "audio/audio_manager.h"
#include "core/types.h"
#include "game_data.h"
#include "platform/audio_device.h"
#include "test.h"

using swd3::i16;
using swd3::u8;

namespace
{

// The dummy driver plays nothing and needs no sound hardware.
void UseDummyAudio()
{
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
}

}  // namespace

TEST_CASE("audio: clamps and fade steps")
{
    CHECK_EQ(swd3::ClampVolume(200), 0x7F);
    CHECK_EQ(swd3::ClampVolume(-3), 0);
    CHECK_EQ(swd3::ClampPan(0), 0x3F);
    CHECK_EQ(swd3::ClampPan(-70), 0);
    CHECK_EQ(swd3::ClampPan(100), 0x7F);
    swd3::SequenceNode node;
    node.volume = 1600;
    CHECK_EQ(swd3::SetFadeStep(node, 3), 533);
    CHECK_EQ(node.fadeStep, 533);
    CHECK_EQ(swd3::SetFadeStep(node, 5000), 0);
    CHECK_EQ(node.fadeStep, 1);
}

TEST_CASE("audio: all.snd index, RIFF rebuild and sample bookkeeping")
{
    if (swd3test::ReadGameFile("all.snd").empty())
    {
        std::printf("  (all.snd not found, skipped)\n");
        return;
    }

    UseDummyAudio();
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    swd3::SoundEffects& effects = audio.Effects();
    CHECK(effects.Open(swd3test::GameDirectory()));
    CHECK(effects.IsOpen());
    CHECK(effects.HasDriver());
    int id = 0;

    for (int candidate = 1; candidate <= swd3::SoundEffects::Sounds && id == 0; ++candidate)
    {
        if (effects.LoadSound(candidate) != nullptr)
        {
            id = candidate;
        }
    }

    CHECK(id != 0);
    const auto image = effects.LoadSound(id);
    CHECK(std::memcmp(image->data(), "RIFF", 4) == 0);
    CHECK(std::memcmp(image->data() + 8, "WAVEfmt ", 8) == 0);
    CHECK(std::memcmp(image->data() + 0x24, "data", 4) == 0);
    CHECK_EQ(audio.PlaySound(static_cast<swd3::u16>(id), 11), 0);
    CHECK(effects.Find(id) != nullptr);
    CHECK_EQ(effects.SetPan(id, 100), 0x7F);
    std::vector<i16> mixed;
    device.Mix(mixed, 256);
    CHECK_EQ(mixed.size(), 512u);
    CHECK_EQ(audio.StopSound(static_cast<swd3::u16>(id)), 1);
    CHECK(effects.Find(id) == nullptr);
    CHECK(effects.Close());
}

TEST_CASE("audio: music stream plays, fades out and is retired")
{
    if (swd3test::ReadGameFile("Music/Battle_Arab01.mp3").empty())
    {
        std::printf("  (music not found, skipped)\n");
        return;
    }

    UseDummyAudio();
    swd3::AudioDevice device;
    swd3::AudioManager audio(device);
    CHECK(audio.Streams().Open());
    const std::string file = swd3test::GameFile("Music/Battle_Arab01.mp3");
    CHECK_EQ(audio.Streams().Play(file, swd3::AudioManager::MusicId, 0x60, 0), swd3::AudioManager::MusicId);
    CHECK(!audio.IsMusicStopped());
    CHECK_EQ(audio.Streams().Play(file, swd3::AudioManager::MusicId, 0x60, 0), 0);
    CHECK_EQ(audio.SetMusicVolume(11), 0x7F);
    CHECK_EQ(audio.SetMusicFade(2, 3), 3);
    audio.UpdateMusicFade();

    for (int update = 0; update < 3; ++update)
    {
        audio.Streams().Update();
    }

    CHECK(audio.IsMusicStopped());
    CHECK_EQ(audio.PollMusicFade(), 1);
    CHECK_EQ(audio.PollMusicFade(), 0);
}

TEST_CASE("audio: no MIDI device")
{
    swd3::AudioDevice device;
    swd3::MidiPlayer midi(device);
    CHECK(!midi.Open(0));
    CHECK(midi.Error() == "No MIDI output device //No MIDI output device");
    CHECK_EQ(midi.Play("x.mid", 1, 100, 1), 0);
    CHECK(midi.IsStopped(0));
}
