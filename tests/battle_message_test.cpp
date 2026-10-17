#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

#include "audio/audio_manager.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_state.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game_data.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
#include "platform/display.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"
#include "random/random.h"
#include "recording_reporter.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
#include "test.h"

namespace
{

using swd3::BattleMessage;
using swd3::u8;

class NullRasterizer : public swd3::GlyphRasterizer
{
public:
    void RenderText(swd3::Surface&, std::string_view, int, std::string_view, swd3::u32) override
    {
    }
};

struct Messages
{
    Messages()
        : screen(swd3::Display::Width, swd3::Display::Height),
          codec(this->painter),
          archive(this->codec, this->errors),
          acts(this->errors),
          audio(this->device),
          hud(this->painter, this->archive, this->acts, this->random, this->audio),
          panelFont(this->rasterizer),
          messageFont(this->rasterizer),
          messages(this->painter, this->archive, this->acts, this->hud, this->state, this->panelFont, this->messageFont)
    {
        this->painter.Init(this->screen);
        this->painter.SetTarget(this->screen.Pixels());
        this->painter.SetClipClamped(0, 0, swd3::Display::Width, swd3::Display::Height);
    }

    swd3::Surface screen;
    swd3::Painter painter;
    swd3::RleCodec codec;
    swd3test::RecordingReporter errors;
    swd3::TswArchive archive;
    swd3::ActArchive acts;
    swd3::Random random;
    swd3::AudioDevice device;
    swd3::AudioManager audio;
    swd3::BattleHud hud;
    swd3::BattleState state;
    NullRasterizer rasterizer;
    swd3::TextFont panelFont;
    swd3::TextFont messageFont;
    swd3::BattleMessages messages;
};

// Where swd32.exe keeps the texts.
constexpr std::array<std::pair<swd3::u32, std::u8string_view>, 5> kTexts = {{
    {0x004A7E20, swd3::BattleMessages::ResistAllEnd},
    {0x004A80E4, swd3::BattleMessages::DefenseDownEnd},
    {0x004A7F3C, swd3::BattleMessages::ResistPhysicalEnd},
    {0x004A7B3C, swd3::BattleMessages::CraftSuccess},
    {0x004A7EB4, swd3::BattleMessages::SpeedDown},
}};

constexpr swd3::u32 kLootLineAt = 0x004A7AE8;
constexpr swd3::u32 kElementTable = 0x004A79A8;  // the pointers to the element names

constexpr int kFirstX = 1;
constexpr int kFirstY = 2;
constexpr std::string_view kFirstText = "abcd";
constexpr int kSecondX = 3;
constexpr int kSecondY = 4;
constexpr int kSecondFrames = 5;
constexpr int kThirdX = 5;
constexpr int kThirdY = 6;
constexpr swd3::i32 kRiseStart = -0x20;  // a rising message starts this far below
constexpr int kPanelTop = 0xD4;
constexpr int kPanelBottom = 0xF4;
constexpr int kGrowDivisor = 3;
constexpr std::size_t kEarlierRow = 1;  // two queued actions; the one in the earlier row is taken
constexpr std::size_t kEarlierColumn = 3;
constexpr std::size_t kLaterRow = 2;
constexpr std::size_t kLaterColumn = 5;
constexpr swd3::i16 kEarlierAction = 7;
constexpr swd3::i16 kLaterAction = 9;

}  // namespace

TEST_CASE("battle message: texts match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    for (const auto& [address, text] : kTexts)
    {
        CHECK(swd3test::SameText(exe, address, text));
    }

    CHECK(swd3test::SameText(exe, kLootLineAt, swd3::BattleMessages::LootLine));

    for (std::size_t element = 0; element < swd3::BattleMessages::ElementNames.size(); ++element)
    {
        const swd3::u32 pointer = swd3test::ExeDword(exe, kElementTable + sizeof(swd3::u32) * element);
        CHECK(swd3test::SameText(exe, pointer, swd3::BattleMessages::ElementNames[element]));
    }
}

TEST_CASE("battle message: list, panel growth and the action queue")
{
    Messages all;
    all.messages.Add(kFirstX, kFirstY, 0, kFirstText, BattleMessage::BlackBar);
    all.messages.Add(kSecondX, kSecondY, kSecondFrames, "xy", BattleMessage::Rise);
    all.messages.Add(kThirdX, kThirdY, 0, "z", 0);
    BattleMessage* first = all.messages.messages.get();
    CHECK(first != nullptr);

    if (first == nullptr)
    {
        return;
    }

    CHECK_EQ(first->length, static_cast<int>(kFirstText.size()));
    BattleMessage* second = first->next.get();
    CHECK(second != nullptr);

    if (second == nullptr)
    {
        return;
    }

    CHECK_EQ(second->y, 1);
    CHECK_EQ(second->slideY, kRiseStart);
    CHECK(second->next != nullptr);
    second->frames = 0;
    second->flags = 0;
    all.messages.Draw();
    CHECK(all.messages.messages == nullptr);

    all.messages.panelHeight = 0;
    CHECK_EQ(all.messages.GrowPanel(kPanelTop, kPanelBottom, kGrowDivisor), 0);
    CHECK_EQ(all.messages.panelHeight, (kPanelBottom - kPanelTop) / kGrowDivisor);
    int calls = 1;

    while (all.messages.GrowPanel(kPanelTop, kPanelBottom, kGrowDivisor) == 0)
    {
        ++calls;
    }

    CHECK_EQ(all.messages.panelHeight, kPanelBottom - kPanelTop - (kGrowDivisor - 1));  // a step of 0 ends it
    CHECK(calls > 1);

    constexpr std::size_t kEarlier = kEarlierRow * swd3::BattleState::QueueColumns + kEarlierColumn;
    constexpr std::size_t kLater = kLaterRow * swd3::BattleState::QueueColumns + kLaterColumn;
    all.state.actionQueue[kEarlier] = kEarlierAction;
    all.state.actionQueue[kLater] = kLaterAction;
    all.state.TakePendingAction();
    CHECK_EQ(all.state.pendingAction, kEarlierAction);
    CHECK_EQ(all.state.pendingActionRow, kEarlierRow);
    CHECK_EQ(all.state.actionQueue[kEarlier], 0);
    all.state.TakePendingAction();
    CHECK_EQ(all.state.pendingAction, kEarlierAction);  // kept until it is used
}
