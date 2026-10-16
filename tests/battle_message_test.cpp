#include <cstdio>
#include <cstring>
#include <string_view>
#include <vector>

#include "audio/audio_manager.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_state.h"
#include "battle/battle_text.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game_data.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
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
        : screen(640, 480), codec(this->painter), archive(this->codec, this->errors), acts(this->errors),
          audio(this->device), hud(this->painter, this->archive, this->acts, this->random, this->audio),
          panelFont(this->rasterizer), messageFont(this->rasterizer),
          messages(this->painter, this->archive, this->acts, this->hud, this->state, this->panelFont, this->messageFont)
    {
        this->painter.Init(this->screen);
        this->painter.SetTarget(this->screen.Pixels());
        this->painter.SetClipClamped(0, 0, 640, 480);
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

}  // namespace

TEST_CASE("battle message: texts match swd32.exe")
{
    const std::vector<u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    namespace text = swd3::battle_text;
    CHECK(swd3test::SameText(exe, 0x004A7E20, text::kResistAllEnd));
    CHECK(swd3test::SameText(exe, 0x004A80E4, text::kDefenseDownEnd));
    CHECK(swd3test::SameText(exe, 0x004A7F3C, text::kResistPhysicalEnd));
    CHECK(swd3test::SameText(exe, 0x004A7B3C, text::kCraftSuccess));
    CHECK(swd3test::SameText(exe, 0x004A7AE8, text::kLootLine));
    CHECK(swd3test::SameText(exe, 0x004A7EB4, text::kSpeedDown));
    constexpr swd3::u32 kElementTable = 0x004A79A8;

    for (int element = 0; element < 9; ++element)
    {
        swd3::u32 pointer = 0;
        std::memcpy(&pointer, exe.data() + (kElementTable - 0x400000) + 4 * element, 4);
        CHECK(swd3test::SameText(exe, pointer, text::kElementNames[element]));
    }
}

TEST_CASE("battle message: list, panel growth and the action queue")
{
    Messages all;
    all.messages.Add(1, 2, 0, "abcd", BattleMessage::kBlackBar);
    all.messages.Add(3, 4, 5, "xy", BattleMessage::kRise);
    all.messages.Add(5, 6, 0, "z", 0);
    BattleMessage* first = all.messages.messages.get();
    CHECK(first != nullptr);

    if (first == nullptr)
    {
        return;
    }

    CHECK_EQ(first->length, 4);
    BattleMessage* second = first->next.get();
    CHECK(second != nullptr);

    if (second == nullptr)
    {
        return;
    }

    CHECK_EQ(second->y, 1);
    CHECK_EQ(second->slideY, -0x20);
    CHECK(second->next != nullptr);
    second->frames = 0;
    second->flags = 0;
    all.messages.Draw();
    CHECK(all.messages.messages == nullptr);

    all.messages.panelHeight = 0;
    CHECK_EQ(all.messages.GrowPanel(0xD4, 0xF4, 3), 0);
    CHECK_EQ(all.messages.panelHeight, 10);
    int calls = 1;

    while (all.messages.GrowPanel(0xD4, 0xF4, 3) == 0)
    {
        ++calls;
    }

    CHECK_EQ(all.messages.panelHeight, 30);
    CHECK(calls > 1);

    all.state.actionQueue[13] = 7;
    all.state.actionQueue[25] = 9;
    all.state.TakePendingAction();
    CHECK_EQ(all.state.pendingAction, 7);
    CHECK_EQ(all.state.pendingActionRow, 1);
    CHECK_EQ(all.state.actionQueue[13], 0);
    all.state.TakePendingAction();
    CHECK_EQ(all.state.pendingAction, 7);
}
