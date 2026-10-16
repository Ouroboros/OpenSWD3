#include <SDL3/SDL.h>

#include "audio/audio_manager.h"
#include "battle/battle_hud.h"
#include "battle/battle_services.h"
#include "battle/battle_state.h"
#include "battle/battle_unit.h"
#include "config/config.h"
#include "draw/painter.h"
#include "org/object_record.h"
#include "party/party.h"
#include "pic/rle.h"
#include "platform/audio_device.h"
#include "platform/surface.h"
#include "random/random.h"
#include "recording_reporter.h"
#include "res/act_archive.h"
#include "res/tsw_archive.h"
#include "test.h"

namespace
{

struct Services
{
    Services()
        : screen(640, 480), codec(this->painter), archive(this->codec, this->errors), acts(this->errors),
          audio(this->device), hud(this->painter, this->archive, this->acts, this->random, this->audio),
          services{
              this->state,
              this->random,
              this->archive,
              this->acts,
              this->painter,
              this->audio,
              this->hud,
              this->config,
              this->party,
          }

    {
        this->painter.Init(this->screen);
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
    unit.kind = swd3::BattleUnit::kHero;
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
    swd3::HeroRecord hero;
    hero.hitPoints = 99;
    unit.hero = &hero;
    unit.StartDefeat(1);
    CHECK_EQ(hero.hitPoints, 0);
    CHECK_EQ(unit.unk2A94, 6);
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

    swd3::HeroRecord hero;
    hero.hitPoints = 5;
    hero.maxHitPoints = 80;
    unit.kind = swd3::BattleUnit::kHero;
    unit.hero = &hero;
    unit.SetVitals(-1, 30, 0);
    CHECK_EQ(hero.hitPoints, 80);
    CHECK_EQ(hero.spirit, 30);
    CHECK_EQ(hero.maxSpirit, 30);
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
