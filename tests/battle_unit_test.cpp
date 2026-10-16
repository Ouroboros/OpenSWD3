#include <cstring>
#include <string_view>

#include <SDL3/SDL.h>

#include "audio/audio_manager.h"
#include "battle/battle_enemy.h"
#include "battle/battle_hud.h"
#include "battle/battle_message.h"
#include "battle/battle_role.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/battle_text.h"
#include "battle/battle_unit.h"
#include "config/config.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "org/object_record.h"
#include "party/party.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
#include "platform/glyph_rasterizer.h"
#include "platform/surface.h"
#include "random/random.h"
#include "recording_reporter.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
#include "test.h"
#include "text/big5.h"

namespace
{

class NullRasterizer : public swd3::GlyphRasterizer
{
public:
    void RenderText(swd3::Surface&, std::string_view, int, std::string_view, swd3::u32) override
    {
    }
};

struct Services
{
    Services()
        : screen(640, 480),
          codec(this->painter),
          archive(this->codec, this->errors),
          acts(this->errors),
          audio(this->device),
          hud(this->painter, this->archive, this->acts, this->random, this->audio),
          panelFont(this->rasterizer),
          messageFont(this->rasterizer),
          messages(
              this->painter,
              this->archive,
              this->acts,
              this->hud,
              this->state,
              this->panelFont,
              this->messageFont
          ),
          services{
              this->state,
              this->random,
              this->archive,
              this->acts,
              this->painter,
              this->audio,
              this->hud,
              this->messages,
              this->config,
              this->party,
          }

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
    swd3::Config config;
    swd3::Party party;
    swd3::BattleServices services;
};

}  // namespace

TEST_CASE("battle unit: construction, commands, turns and hits")
{
    Services all;
    swd3::BattleUnit unit;
    CHECK_EQ(unit.fadeLevel, 0xF);
    CHECK_EQ(unit.unk2A6A, 0x18);
    CHECK_EQ(unit.targets[3], 0xFFFFFFFFu);
    CHECK_EQ(unit.colorPair, 0x062B062Bu);
    CHECK_EQ(unit.SetCommand(14), 1);
    CHECK_EQ(unit.commandB, 14);
    CHECK_EQ(unit.unk2A86, 0x4000);
    unit.SetCommand(9);
    CHECK_EQ(unit.commandA, 9);
    unit.SetCommand(6);
    CHECK_EQ(unit.commandA, 0);
    CHECK_EQ(unit.unk2A86, 0x4800);
    unit.gauge = 450;
    CHECK_EQ(unit.GaugeLength(all.services), 31);
    CHECK_EQ(unit.ResetGauge(all.services), 900);
    unit.unk2A6A = 0x18;
    unit.command.flags = 8;
    unit.guarding = 1;
    unit.slotActs[9].id = 5;
    unit.slotActs[2].id = 5;
    unit.kind = swd3::BattleUnit::kPartyRole;
    all.random.Seed(7);
    const int gauge = unit.ResetTurn(all.services);
    CHECK(gauge >= 50 && gauge < 190);
    CHECK_EQ(unit.gauge, gauge);
    CHECK_EQ(unit.unk2A6A, 8);
    CHECK_EQ(unit.guarding, 0);
    CHECK_EQ(unit.slotActs[2].id, 0);
    CHECK_EQ(unit.slotActs[9].id, 5);
    CHECK_EQ(unit.command.flags, 0);
    unit.IncreaseHits();
    unit.IncreaseCounter2A76();
    CHECK_EQ(unit.GetAE0(), 1);
    CHECK_EQ(unit.DecreaseHits(), 0);
    CHECK_EQ(unit.GetAE0(), 0);
    CHECK_EQ(unit.SetPosition(30, 40), 30);
    unit.Move(5, -5);
    swd3::u16 x = 0;
    swd3::u16 y = 0;
    unit.GetDrawPosition(x, y);
    CHECK_EQ(x, 35);
    unit.drawAtSaved = 1;
    unit.GetDrawPosition(x, y);
    CHECK_EQ(x, 30);
    CHECK_EQ(y, 40);
    swd3::RoleRecord role;
    role.hitPoints = 99;
    unit.roleRecord = &role;
    unit.StartDefeat(1);
    CHECK_EQ(role.hitPoints, 0);
    CHECK_EQ(unit.defeatPose, 6);
    unit.status = 0x2000000;
    unit.RaiseFlag26B8();
    CHECK_EQ(unit.Flag26B8(), 0);
    unit.status = 0;
    unit.RaiseFlag26B8();
    CHECK_EQ(unit.Flag26B8(), 1);
}

TEST_CASE("battle unit: evasion, vitals and scaling")
{
    Services all;
    swd3::BattleUnit unit;
    CHECK_EQ(unit.RollEvade(all.services, 0), 0);
    unit.unk2A93 = 9;
    all.state.evadeStreak = 0x18;
    CHECK_EQ(unit.RollEvade(all.services, 0), 1);
    CHECK_EQ(all.state.evadeStreak, 0);
    CHECK_EQ(unit.marks & 1, 1);
    CHECK_EQ(unit.RollEvade(all.services, 0), 0);
    CHECK_EQ(all.state.evadeJustSucceeded, 0);
    all.state.evadeDisabled = 1;
    CHECK_EQ(unit.RollEvade(all.services, 10), 0);
    all.state.evadeDisabled = 0;
    CHECK_EQ(unit.RollEvade(all.services, 10), 1);

    swd3::RoleRecord role;
    role.hitPoints = 5;
    role.maxHitPoints = 80;
    unit.kind = swd3::BattleUnit::kPartyRole;
    unit.roleRecord = &role;
    unit.SetVitals(-1, 30, 0);
    CHECK_EQ(role.hitPoints, 80);
    CHECK_EQ(role.spirit, 30);
    CHECK_EQ(role.maxSpirit, 30);
    swd3::ObjectRecord enemy;
    swd3::BattleUnit other;
    other.data = &enemy;
    other.SetVitals(500, 0, 0);
    CHECK_EQ(enemy.data.maxHitPoints, 500);
    CHECK_EQ(enemy.data.hitPoints, 500);
    other.SetAttributes(0, 12, 7);
    CHECK_EQ(enemy.data.defense, 12);
    CHECK_EQ(enemy.data.agility, 7);

    swd3::i16 value = 100;
    CHECK_EQ(swd3::ScaleByTenths(3, value), 0);
    CHECK_EQ(value, 130);
    value = 100;
    CHECK_EQ(swd3::ScaleByTenths(-15, value), 1);
    CHECK_EQ(value, -50);

    unit.targetCount = 0x22;
    unit.StepTargetCount(1);
    CHECK_EQ(unit.targetCount, 0x22);
    unit.StepTargetCount(0);
    CHECK_EQ(unit.targetCount, 0x21);
    unit.PushTargetAlt(4);
    CHECK_EQ(unit.damageDelays[0x21], 4);
    unit.SetStatus(0x200);
    CHECK_EQ(unit.brightLevel, -0x20);
    unit.SetGuarding(1);
    unit.ClearStatus(0x1000);
    CHECK(!unit.IsGuarding());
}

TEST_CASE("battle unit: ailments, buffs, expiry and damage")
{
    Services all;
    swd3::RoleRecord stats;
    stats.ailmentChance = 10;
    stats.attackBonus = 100;
    swd3::RoleRecord role;
    role.hitPoints = 50;
    role.maxHitPoints = 60;
    role.spirit = 5;
    role.maxSpirit = 20;
    swd3::BattleUnit unit;
    unit.kind = swd3::BattleUnit::kPartyRole;
    unit.battleRecord = &stats;
    unit.roleRecord = &role;
    unit.displayName = "A";

    swd3::SkillScript poison;
    poison.ailment = 0x200;
    poison.turns = 3;
    CHECK_EQ(unit.AddAilment(all.services, poison), 1);
    CHECK_EQ(stats.attackBonus, 90);
    CHECK(unit.effects.next != nullptr);
    CHECK_EQ(unit.effects.next->icon, 0xB);
    CHECK_EQ(unit.effects.next->act.id, 0x2AF8);
    CHECK_EQ(unit.status & 0x200, 0x200u);
    poison.turns = 5;
    CHECK_EQ(unit.AddAilment(all.services, poison), 1);
    CHECK_EQ(unit.effects.next->turns, 5);
    CHECK(unit.effects.next->next == nullptr);
    CHECK_EQ(unit.HasEffectBits(0x200), 1);

    swd3::SkillScript defenseUp;
    defenseUp.statusBits = 0x20000000;
    defenseUp.turns = 2;
    CHECK_EQ(unit.AddBuff(all.services, defenseUp), 1);
    CHECK_EQ(unit.status & 0x40000000, 0x40000000u);
    CHECK(all.messages.messages != nullptr);
    const std::string_view shown(all.state.text.data(), all.state.text.size());
    CHECK(shown.substr(0, shown.find('\0')) == swd3::Big5(u8"A增加30%防禦力"));

    unit.effects.next->turns = 0;
    unit.effects.next->next->turns = 0;
    unit.ExpireEffects(all.services, 0);
    CHECK_EQ(stats.attackBonus, 99);
    CHECK_EQ(unit.status & 0x200, 0u);
    CHECK(unit.effects.next != nullptr);
    CHECK_EQ(unit.effects.next->bits, 0x20000000u);
    unit.ExpireEffects(all.services, 0);
    CHECK(unit.effects.next == nullptr);
    CHECK_EQ(unit.status & 0x40000000, 0u);
    unit.ExpireEffects(all.services, 0);

    CHECK_EQ(unit.Damage(all.services, 20, 30, 0), 0);
    CHECK_EQ(role.hitPoints, 30);
    CHECK_EQ(role.spirit, 20);
    unit.goldGuard = 1;
    all.party.variables[0] = 10;
    CHECK_EQ(unit.Damage(all.services, 25, 0, 0), 0);
    CHECK_EQ(all.party.variables[0], -15);
    CHECK_EQ(role.hitPoints, 30);
    unit.goldGuard = 0;
    CHECK_EQ(unit.Damage(all.services, 40, 0, 0), 1);
    CHECK_EQ(role.hitPoints, 0);

    unit.targetCount = 2;
    unit.SetDamageDigits(all.services, 1234);
    CHECK_EQ(unit.damageDigits[2][0], 1);
    CHECK_EQ(unit.damageDigits[2][3], 4);
    unit.SetDamageDigits(all.services, static_cast<swd3::i16>(-75));
    CHECK_EQ(unit.damageDigits[2][1], 0);
    CHECK_EQ(unit.damageDigits[2][2], 7);
    CHECK_EQ(unit.damageDigits[2][3], 5);
}

TEST_CASE("battle unit: role and enemy blocks, reset")
{
    Services all;
    swd3::BattleRole role;
    CHECK(role.battleRecord != nullptr);
    swd3::BattleEnemy enemy;
    CHECK(enemy.data != nullptr);
    role.kind = swd3::BattleUnit::kPartyRole;
    role.gauge = 9;
    role.slotActs[9].id = 4;
    role.slotActs[3].id = 4;
    role.targets[1] = 2;
    role.effects.next = std::make_unique<swd3::EffectNode>();
    role.Reset(all.services);
    CHECK_EQ(role.gauge, 0);
    CHECK_EQ(role.kind, 0);
    CHECK_EQ(role.slotActs[3].id, 0);
    CHECK_EQ(role.slotActs[9].id, 4);
    CHECK_EQ(role.targets[1], 0xFFFFFFFFu);
    CHECK_EQ(role.fadeLevel, 0xF);
    CHECK(role.effects.next == nullptr);
    CHECK(role.battleRecord != nullptr);
    enemy.Reset(all.services);
    CHECK(enemy.data != nullptr);
}
