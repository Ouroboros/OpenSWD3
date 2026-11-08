#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

#include "audio/audio_manager.h"
#include "config/config.h"
#include "core/types.h"
#include "game_data.h"
#include "platform/audio_decoder.h"
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

constexpr int kLoud = 200;
constexpr int kQuiet = 3;
constexpr int kFarLeft = -70;  // pans run from -CenterPan to the right of the centre
constexpr int kFarRight = 100;
constexpr int kFadedVolume = 1600;
constexpr int kFadeSteps = 3;
constexpr int kTooManySteps = 5000;  // more steps than the volume: the step stays 1
constexpr std::size_t kFormAt = 8;   // a rebuilt RIFF image: "WAVEfmt " after the RIFF header, "data" after the format
constexpr std::size_t kDataAt = 0x24;
constexpr int kMixFrames = 256;
constexpr int kMusicVolume = 0x60;
constexpr int kFadeFrames = 3;
constexpr int kMidiVolume = 100;

}  // namespace

TEST_CASE("audio: clamps and fade steps")
{
    using swd3::SequenceNode;
    CHECK_EQ(swd3::ClampVolume(kLoud), SequenceNode::MaxLevel);
    CHECK_EQ(swd3::ClampVolume(-kQuiet), 0);
    CHECK_EQ(swd3::ClampPan(0), SequenceNode::CenterPan);
    CHECK_EQ(swd3::ClampPan(kFarLeft), 0);
    CHECK_EQ(swd3::ClampPan(kFarRight), SequenceNode::MaxLevel);
    SequenceNode node;
    node.volume = kFadedVolume;
    CHECK_EQ(swd3::SetFadeStep(node, kFadeSteps), kFadedVolume / kFadeSteps);
    CHECK_EQ(node.fadeStep, kFadedVolume / kFadeSteps);
    CHECK_EQ(swd3::SetFadeStep(node, kTooManySteps), 0);
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

    auto holds = [&image](std::size_t at, std::string_view tag)
    {
        return std::memcmp(image->data() + at, tag.data(), tag.size()) == 0;
    };

    CHECK(holds(0, "RIFF"));
    CHECK(holds(kFormAt, "WAVEfmt "));
    CHECK(holds(kDataAt, "data"));
    CHECK_EQ(audio.PlaySound(static_cast<swd3::u16>(id), swd3::Config::MaxVolume), 0);
    CHECK(effects.Find(id) != nullptr);
    CHECK_EQ(effects.SetPan(id, kFarRight), swd3::SequenceNode::MaxLevel);
    std::vector<i16> mixed;
    device.Mix(mixed, kMixFrames);
    CHECK_EQ(mixed.size(), static_cast<std::size_t>(kMixFrames) * swd3::kMixChannels);
    CHECK_EQ(audio.StopSound(static_cast<swd3::u16>(id)), 1);
    CHECK(effects.Find(id) == nullptr);
    CHECK(effects.Close());
}

TEST_CASE("audio: music stream plays, fades out and is retired")
{
    using swd3::AudioManager;

    if (swd3test::ReadGameFile("Music/Battle_Arab01.mp3").empty())
    {
        std::printf("  (music not found, skipped)\n");
        return;
    }

    UseDummyAudio();
    swd3::AudioDevice device;
    AudioManager audio(device);
    CHECK(audio.Streams().Open());
    const std::u8string file = swd3test::GameFile("Music/Battle_Arab01.mp3");
    CHECK_EQ(audio.Streams().Play(file, AudioManager::MusicId, kMusicVolume, 0), AudioManager::MusicId);
    CHECK(!audio.IsMusicStopped());
    CHECK_EQ(audio.Streams().Play(file, AudioManager::MusicId, kMusicVolume, 0), 0);
    CHECK_EQ(audio.SetMusicVolume(swd3::Config::MaxVolume), swd3::SequenceNode::MaxLevel);
    CHECK_EQ(audio.SetMusicFade(AudioManager::FadeOverLength, kFadeFrames), kFadeFrames);
    audio.UpdateMusicFade();

    for (int update = 0; update < kFadeFrames; ++update)
    {
        audio.Streams().Update();
    }

    CHECK(audio.IsMusicStopped());
    CHECK_EQ(audio.PollMusicFade(), 1);
    CHECK_EQ(audio.PollMusicFade(), 0);
}

TEST_CASE("audio: a stream decodes on its own thread, its start the start of the whole")
{
    constexpr std::size_t kStartSamples = static_cast<std::size_t>(swd3::kMixRate) * swd3::kMixChannels;
    constexpr std::size_t kMillisecondsPerSecond = 1000;
    const std::vector<u8> data = swd3test::ReadGameFile("Music/Battle_Arab01.mp3");

    if (data.empty())
    {
        std::printf("  (music not found, skipped)\n");
        return;
    }

    std::vector<i16> all;
    CHECK(swd3::DecodeAudio(data.data(), data.size(), all));
    std::vector<i16> start;
    bool whole = true;
    CHECK(swd3::DecodeAudioStart(data.data(), data.size(), kStartSamples, start, whole));
    CHECK(!whole);
    CHECK(start.size() >= kStartSamples);
    CHECK(start.size() < all.size());
    CHECK(std::equal(start.begin(), start.end(), all.begin()));

    UseDummyAudio();
    swd3::AudioDevice device;
    const swd3::VoiceHandle voice = device.OpenStream(swd3test::GameFile("Music/Battle_Arab01.mp3"));
    CHECK(voice != nullptr);
    int total = 0;
    int current = 0;
    device.Position(voice, total, current);
    const std::size_t frames = all.size() / swd3::kMixChannels;
    CHECK_EQ(total, static_cast<int>(frames * kMillisecondsPerSecond / swd3::kMixRate));
    CHECK(device.OpenStream(u8"Music/NoSuchTune.mp3") == nullptr);
}

TEST_CASE("audio: no MIDI device")
{
    swd3::AudioDevice device;
    swd3::MidiPlayer midi(device);
    CHECK(!midi.Open(0));
    CHECK(midi.Error() == "No MIDI output device //No MIDI output device");
    CHECK_EQ(midi.Play(u8"x.mid", 1, kMidiVolume, 1), 0);
    CHECK(midi.IsStopped(0));
}
