#include <algorithm>
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
#include "battle/battle_unit.h"
#include "config/config.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "org/object_record.h"
#include "party/party.h"
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
        : screen(swd3::Display::Width, swd3::Display::Height),
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
              this->crtRandom,
              this->screenPixels,
          }

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
    swd3::Config config;
    swd3::Party party;
    swd3::crt::Random crtRandom;
    swd3::u16* screenPixels = nullptr;
    swd3::BattleServices services;
};

constexpr int kOtherCommand = 9;  // a command that goes to commandA
constexpr int kHalf = 2;
constexpr int kGaugePixels = 62;            // the length of a full gauge bar
constexpr swd3::u16 kResetClearsGuard = 8;  // the ResetTurn mode bit that ends the guard
constexpr swd3::i32 kSlotAct = 5;
constexpr std::size_t kTurnSlot = 2;  // one of the first eight slot acts, which ResetTurn clears
constexpr swd3::u32 kSeed = 7;
constexpr int kRoleGaugeBase = 0x32;  // a role starts a turn with a gauge of kRoleGaugeBase + 0..kRoleGaugeSpread-1
constexpr int kRoleGaugeSpread = 0x8C;
constexpr swd3::u16 kTurnFlashFall = 8;  // the tint pulse falls from here after ResetTurn
constexpr swd3::i16 kPlaceX = 30;
constexpr swd3::i16 kPlaceY = 40;
constexpr swd3::i16 kMoveStep = 5;
constexpr swd3::i16 kHitPoints = 99;
constexpr swd3::u8 kFallbackDefeatPose = 6;     // the pose of a unit defeated without one
constexpr swd3::u32 kRaiseBlocked = 0x2000000;  // status bit 25 blocks RaiseFlag26B8
constexpr swd3::u8 kEvadeSkill = 9;
constexpr swd3::u8 kSureEvadeStreak = 0x18;  // failed rolls after which the evasion succeeds
constexpr swd3::u16 kSureChance = 10;        // a chance of ten in ten
constexpr swd3::i16 kRefill = -1;            // SetVitals refills the current value
constexpr swd3::i16 kLowHitPoints = 5;
constexpr swd3::i16 kMaxHitPoints = 80;
constexpr swd3::i16 kSpirit = 30;
constexpr swd3::i16 kEnemyHitPoints = 500;
constexpr swd3::i16 kDefense = 12;
constexpr swd3::i16 kAgility = 7;
constexpr int kTenths = 10;
constexpr swd3::i16 kScaled = 100;
constexpr swd3::i8 kRaiseTenths = 3;
constexpr swd3::i8 kCutTenths = -15;  // below -10 the value is multiplied instead
constexpr swd3::u16 kDamageDelay = 4;
constexpr swd3::i16 kAilmentRoll = 10;  // an ailment chance that always lands
constexpr int kAttackBonus = 100;
constexpr int kStatScalePercent = 10;  // StatsScaled moves the stats of a role by this percent
constexpr int kPercent = 100;
constexpr swd3::i16 kHitPointsLeft = 50;
constexpr swd3::i16 kSpiritLeft = 5;
constexpr swd3::i16 kMaxSpirit = 20;
constexpr swd3::u16 kTurns = 3;
constexpr swd3::u16 kLongerTurns = 5;
constexpr swd3::i32 kEffectAct = 0x2AF8;  // the act of the effect animations
constexpr int kHit = 20;
constexpr swd3::i16 kSpiritGain = 30;  // more than the spirit can hold
constexpr swd3::i32 kGold = 10;
constexpr int kGoldHit = 25;
constexpr std::size_t kDigitsTarget = 2;
constexpr swd3::i16 kDamage = 1234;
constexpr swd3::i16 kHealing = 75;
constexpr int kThousands = 1000;  // the places of the four damage digits
constexpr int kHundreds = 100;
constexpr int kTens = 10;
constexpr std::size_t kThousandsDigit = 0;
constexpr std::size_t kHundredsDigit = 1;
constexpr std::size_t kTensDigit = 2;
constexpr std::size_t kOnesDigit = 3;
constexpr swd3::u16 kGauge = 9;
constexpr swd3::u32 kTarget = 2;

}  // namespace

TEST_CASE("battle unit: construction, commands, turns and hits")
{
    using swd3::BattleUnit;
    Services all;
    BattleUnit unit;
    CHECK_EQ(unit.fadeLevel, BattleUnit::Opaque);
    CHECK_EQ(unit.unk2A6A, BattleUnit::FlashPeak);
    CHECK(
        std::all_of(
            unit.targets.begin(),
            unit.targets.end(),
            [](swd3::u32 target)
            {
                return target == BattleUnit::NoTarget;
            }
        )
    );
    CHECK_EQ(unit.colorPair, BattleUnit::DefaultColorPair);
    CHECK_EQ(unit.SetCommand(BattleUnit::UnkCommand14), 1);
    CHECK_EQ(unit.commandB, BattleUnit::UnkCommand14);
    CHECK_EQ(unit.unk2A86, BattleUnit::UnkCommandFlag4000);
    unit.SetCommand(kOtherCommand);
    CHECK_EQ(unit.commandA, kOtherCommand);
    unit.SetCommand(BattleUnit::UnkCommand6);
    CHECK_EQ(unit.commandA, 0);
    CHECK_EQ(unit.unk2A86, BattleUnit::UnkCommandFlag4000 | BattleUnit::UnkCommandFlag800);
    unit.gauge = static_cast<swd3::u16>(all.state.gaugeFull / kHalf);
    CHECK_EQ(unit.GaugeLength(all.services), kGaugePixels / kHalf);
    CHECK_EQ(unit.ResetGauge(all.services), all.state.gaugeFull);
    unit.unk2A6A = BattleUnit::FlashPeak;
    unit.command.flags = kResetClearsGuard;
    unit.guarding = 1;
    unit.slotActs[BattleUnit::UnkSlot9].id = kSlotAct;
    unit.slotActs[kTurnSlot].id = kSlotAct;
    unit.kind = BattleUnit::PartyRole;
    all.random.Seed(kSeed);
    const int gauge = unit.ResetTurn(all.services);
    CHECK(gauge >= kRoleGaugeBase && gauge < kRoleGaugeBase + kRoleGaugeSpread);
    CHECK_EQ(unit.gauge, gauge);
    CHECK_EQ(unit.unk2A6A, kTurnFlashFall);
    CHECK_EQ(unit.guarding, 0);
    CHECK_EQ(unit.slotActs[kTurnSlot].id, 0);
    CHECK_EQ(unit.slotActs[BattleUnit::UnkSlot9].id, kSlotAct);
    CHECK_EQ(unit.command.flags, 0);
    unit.IncreaseHits();
    unit.IncreaseCounter2A76();
    CHECK_EQ(unit.GetAE0(), 1);
    CHECK_EQ(unit.DecreaseHits(), 0);
    CHECK_EQ(unit.GetAE0(), 0);
    CHECK_EQ(unit.SetPosition(kPlaceX, kPlaceY), kPlaceX);
    unit.Move(kMoveStep, -kMoveStep);
    swd3::u16 x = 0;
    swd3::u16 y = 0;
    unit.GetDrawPosition(x, y);
    CHECK_EQ(x, kPlaceX + kMoveStep);
    unit.drawAtSaved = 1;
    unit.GetDrawPosition(x, y);
    CHECK_EQ(x, kPlaceX);
    CHECK_EQ(y, kPlaceY);
    swd3::RoleRecord role;
    role.hitPoints = kHitPoints;
    unit.roleRecord = &role;
    unit.StartDefeat(1);
    CHECK_EQ(role.hitPoints, 0);
    CHECK_EQ(unit.defeatPose, kFallbackDefeatPose);
    unit.status = kRaiseBlocked;
    unit.RaiseFlag26B8();
    CHECK_EQ(unit.Flag26B8(), 0);
    unit.status = 0;
    unit.RaiseFlag26B8();
    CHECK_EQ(unit.Flag26B8(), 1);
}

TEST_CASE("battle unit: evasion, vitals and scaling")
{
    using swd3::BattleUnit;
    Services all;
    BattleUnit unit;
    CHECK_EQ(unit.RollEvade(all.services, 0), 0);
    unit.unk2A93 = kEvadeSkill;
    all.state.evadeStreak = kSureEvadeStreak;
    CHECK_EQ(unit.RollEvade(all.services, 0), 1);
    CHECK_EQ(all.state.evadeStreak, 0);
    CHECK_EQ(unit.marks & BattleUnit::UnkMark1, BattleUnit::UnkMark1);
    CHECK_EQ(unit.RollEvade(all.services, 0), 0);
    CHECK_EQ(all.state.evadeJustSucceeded, 0);
    all.state.criticalAttack = 1;
    CHECK_EQ(unit.RollEvade(all.services, kSureChance), 0);
    all.state.criticalAttack = 0;
    CHECK_EQ(unit.RollEvade(all.services, kSureChance), 1);

    swd3::RoleRecord role;
    role.hitPoints = kLowHitPoints;
    role.maxHitPoints = kMaxHitPoints;
    unit.kind = BattleUnit::PartyRole;
    unit.roleRecord = &role;
    unit.SetVitals(kRefill, kSpirit, 0);
    CHECK_EQ(role.hitPoints, kMaxHitPoints);
    CHECK_EQ(role.spirit, kSpirit);
    CHECK_EQ(role.maxSpirit, kSpirit);
    swd3::ObjectRecord enemy;
    BattleUnit other;
    other.data = &enemy;
    other.SetVitals(kEnemyHitPoints, 0, 0);
    CHECK_EQ(enemy.data.maxHitPoints, kEnemyHitPoints);
    CHECK_EQ(enemy.data.hitPoints, kEnemyHitPoints);
    other.SetAttributes(0, kDefense, kAgility);
    CHECK_EQ(enemy.data.defense, kDefense);
    CHECK_EQ(enemy.data.agility, kAgility);

    swd3::i16 value = kScaled;
    CHECK_EQ(swd3::ScaleByTenths(kRaiseTenths, value), 0);
    CHECK_EQ(value, kScaled + kScaled * kRaiseTenths / kTenths);
    value = kScaled;
    CHECK_EQ(swd3::ScaleByTenths(kCutTenths, value), 1);
    CHECK_EQ(value, kScaled * (kCutTenths + kTenths) / kTenths);

    constexpr swd3::u16 kLastTarget = BattleUnit::MaxTargets - 1;
    unit.targetCount = kLastTarget;
    unit.StepTargetCount(1);
    CHECK_EQ(unit.targetCount, kLastTarget);
    unit.StepTargetCount(0);
    CHECK_EQ(unit.targetCount, kLastTarget - 1);
    unit.PushTargetAlt(kDamageDelay);
    CHECK_EQ(unit.damageDelays[kLastTarget - 1], kDamageDelay);
    unit.SetStatus(BattleUnit::StatsScaled);
    CHECK_EQ(unit.brightLevel, BattleUnit::DelayedFadeIn);
    unit.SetGuarding(1);
    unit.ClearStatus(BattleUnit::UnkStatus1000);
    CHECK(!unit.IsGuarding());
}

TEST_CASE("battle unit: ailments, buffs, expiry and damage")
{
    using swd3::BattleUnit;
    using swd3::SkillScript;
    Services all;
    swd3::RoleRecord stats;
    stats.ailmentChance = kAilmentRoll;
    stats.attackBonus = kAttackBonus;
    swd3::RoleRecord role;
    role.hitPoints = kHitPointsLeft;
    role.maxHitPoints = kMaxHitPoints;
    role.spirit = kSpiritLeft;
    role.maxSpirit = kMaxSpirit;
    BattleUnit unit;
    unit.kind = BattleUnit::PartyRole;
    unit.battleRecord = &stats;
    unit.roleRecord = &role;
    unit.displayName = "A";

    SkillScript scaling;
    scaling.ailment = SkillScript::StatsScaledAilment;
    scaling.turns = kTurns;
    CHECK_EQ(unit.AddAilment(all.services, scaling), 1);
    constexpr int kScaledBonus = kAttackBonus - kAttackBonus * kStatScalePercent / kPercent;
    CHECK_EQ(stats.attackBonus, kScaledBonus);
    CHECK(unit.effects.next != nullptr);
    CHECK_EQ(unit.effects.next->icon, swd3::EffectNode::StatsScaledIcon);
    CHECK_EQ(unit.effects.next->act.id, kEffectAct);
    CHECK_EQ(unit.status & BattleUnit::StatsScaled, BattleUnit::StatsScaled);
    scaling.turns = kLongerTurns;
    CHECK_EQ(unit.AddAilment(all.services, scaling), 1);
    CHECK_EQ(unit.effects.next->turns, kLongerTurns);
    CHECK(unit.effects.next->next == nullptr);
    CHECK_EQ(unit.HasEffectBits(BattleUnit::StatsScaled), 1);

    SkillScript defenseUp;
    defenseUp.statusBits = SkillScript::DefenseUp;
    defenseUp.turns = kTurns;
    CHECK_EQ(unit.AddBuff(all.services, defenseUp), 1);
    CHECK_EQ(unit.status & BattleUnit::DefenseUp, BattleUnit::DefenseUp);
    CHECK(all.messages.messages != nullptr);
    const std::string_view shown(all.state.text.data(), all.state.text.size());
    CHECK(shown.substr(0, shown.find('\0')) == swd3::Big5(u8"A增加30%防禦力"));

    unit.effects.next->turns = 0;
    unit.effects.next->next->turns = 0;
    unit.ExpireEffects(all.services, 0);
    CHECK_EQ(stats.attackBonus, kScaledBonus + kScaledBonus * kStatScalePercent / kPercent);
    CHECK_EQ(unit.status & BattleUnit::StatsScaled, 0u);
    CHECK(unit.effects.next != nullptr);
    CHECK_EQ(unit.effects.next->bits, SkillScript::DefenseUp);
    unit.ExpireEffects(all.services, 0);
    CHECK(unit.effects.next == nullptr);
    CHECK_EQ(unit.status & BattleUnit::DefenseUp, 0u);
    unit.ExpireEffects(all.services, 0);

    CHECK_EQ(unit.Damage(all.services, kHit, kSpiritGain, 0), 0);
    CHECK_EQ(role.hitPoints, kHitPointsLeft - kHit);
    CHECK_EQ(role.spirit, kMaxSpirit);
    unit.goldGuard = 1;
    swd3::i32& gold = all.party.variables[swd3::Party::Gold];
    gold = kGold;
    CHECK_EQ(unit.Damage(all.services, kGoldHit, 0, 0), 0);
    CHECK_EQ(gold, kGold - kGoldHit);
    CHECK_EQ(role.hitPoints, kHitPointsLeft - kHit);
    unit.goldGuard = 0;
    CHECK_EQ(unit.Damage(all.services, kHitPointsLeft, 0, 0), 1);
    CHECK_EQ(role.hitPoints, 0);

    unit.targetCount = kDigitsTarget;
    const auto& digits = unit.damageDigits[kDigitsTarget];
    unit.SetDamageDigits(all.services, kDamage);
    CHECK_EQ(digits[kThousandsDigit], kDamage / kThousands);
    CHECK_EQ(digits[kOnesDigit], kDamage % kTens);
    unit.SetDamageDigits(all.services, static_cast<swd3::i16>(-kHealing));
    CHECK_EQ(digits[kHundredsDigit], kHealing / kHundreds);
    CHECK_EQ(digits[kTensDigit], kHealing / kTens);
    CHECK_EQ(digits[kOnesDigit], kHealing % kTens);
}

TEST_CASE("battle unit: role and enemy blocks, reset")
{
    using swd3::BattleUnit;
    Services all;
    swd3::BattleRole role;
    CHECK(role.battleRecord != nullptr);
    swd3::BattleEnemy enemy;
    CHECK(enemy.data != nullptr);
    role.kind = BattleUnit::PartyRole;
    role.gauge = kGauge;
    role.slotActs[BattleUnit::UnkSlot9].id = kSlotAct;
    role.slotActs[kTurnSlot].id = kSlotAct;
    role.targets[1] = kTarget;
    role.effects.next = std::make_unique<swd3::EffectNode>();
    role.Reset(all.services);
    CHECK_EQ(role.gauge, 0);
    CHECK_EQ(role.kind, 0);
    CHECK_EQ(role.slotActs[kTurnSlot].id, 0);
    CHECK_EQ(role.slotActs[BattleUnit::UnkSlot9].id, kSlotAct);
    CHECK_EQ(role.targets[1], BattleUnit::NoTarget);
    CHECK_EQ(role.fadeLevel, BattleUnit::Opaque);
    CHECK(role.effects.next == nullptr);
    CHECK(role.battleRecord != nullptr);
    enemy.Reset(all.services);
    CHECK(enemy.data != nullptr);
}
