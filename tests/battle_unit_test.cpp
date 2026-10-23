#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
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
#include "core/format.h"
#include "crt/msvc.h"
#include "draw/painter.h"
#include "font/text_font.h"
#include "game/scene.h"
#include "game_data.h"
#include "org/database.h"
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
        this->hud.Attach(this->services);
        this->messages.Attach(this->services);
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
    CHECK_EQ(unit.flashFall, BattleUnit::FlashPeak);
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
    CHECK_EQ(unit.commandFlags, BattleUnit::UnkCommandFlag4000);
    unit.SetCommand(kOtherCommand);
    CHECK_EQ(unit.commandA, kOtherCommand);
    unit.SetCommand(BattleUnit::UnkCommand6);
    CHECK_EQ(unit.commandA, 0);
    CHECK_EQ(unit.commandFlags, BattleUnit::UnkCommandFlag4000 | BattleUnit::UnkCommandFlag800);
    unit.gauge = static_cast<swd3::u16>(all.state.gaugeFull / kHalf);
    CHECK_EQ(unit.GaugeLength(all.services), kGaugePixels / kHalf);
    CHECK_EQ(unit.ResetGauge(all.services), all.state.gaugeFull);
    unit.flashFall = BattleUnit::FlashPeak;
    unit.command.flags = kResetClearsGuard;
    unit.guarding = 1;
    unit.slotActs[BattleUnit::UnkSlot9].id = kSlotAct;
    unit.slotActs[kTurnSlot].id = kSlotAct;
    unit.kind = BattleUnit::PartyRole;
    all.random.Seed(kSeed);
    const int gauge = unit.ResetTurn(all.services);
    CHECK(gauge >= kRoleGaugeBase && gauge < kRoleGaugeBase + kRoleGaugeSpread);
    CHECK_EQ(unit.gauge, gauge);
    CHECK_EQ(unit.flashFall, kTurnFlashFall);
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
    unit.evadeSkill = kEvadeSkill;
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

TEST_CASE("battle unit: the shake offsets of a queued act match swd32.exe")
{
    constexpr swd3::u32 kShakeOffsetsAt = 0x004A7E10;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kShakeOffsetsAt, swd3::BattleUnit::ShakeOffsets));
}

TEST_CASE("battle role: the swarm offsets match swd32.exe")
{
    constexpr swd3::u32 kSwarmOffsetXAt = 0x004A7C04;
    constexpr swd3::u32 kSwarmOffsetYAt = 0x004A7C14;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kSwarmOffsetXAt, swd3::BattleRole::SwarmOffsetX));
    CHECK(swd3test::SameTable(exe, kSwarmOffsetYAt, swd3::BattleRole::SwarmOffsetY));
}

TEST_CASE("battle unit: the hop heights of defeat pose 50 match swd32.exe")
{
    constexpr swd3::u32 kHopHeightsAt = 0x004A7DB4;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kHopHeightsAt, swd3::BattleUnit::HopHeights));
}

TEST_CASE("battle unit: the stagger tables match swd32.exe")
{
    constexpr swd3::u32 kStaggerShakeAt = 0x004A7E08;
    constexpr swd3::u32 kStaggerLiftAt = 0x004A7DF0;
    constexpr swd3::u32 kStaggerRiseAt = 0x004A7DF4;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kStaggerShakeAt, swd3::BattleUnit::StaggerShake));
    CHECK(swd3test::SameTable(exe, kStaggerLiftAt, swd3::BattleUnit::StaggerLift));
    CHECK(swd3test::SameTable(exe, kStaggerRiseAt, swd3::BattleUnit::StaggerRise));
}

namespace
{

constexpr swd3::u16 kEnemyLevel = 20;
constexpr swd3::u8 kCatchChance = 10;  // unk91: a roll of 0..9 below it always catches
constexpr int kNearAbove = 5;          // exactly this many levels above uses unk91
constexpr int kNearerAbove = 4;
constexpr int kMidAbove = 6;   // 6..10 above: 5 in 10
constexpr int kFarAbove = 11;  // more than 10 above: 8 in 10
constexpr swd3::u32 kUnusedCatchRoll = 12;
constexpr swd3::u32 kCatchRoll = 10;
constexpr swd3::u16 kMidCatch = 5;
constexpr swd3::u16 kFarCatch = 8;
constexpr swd3::u16 kAccessory19 = 0x19;
constexpr swd3::u16 kAccessory1C = 0x1C;
constexpr swd3::u16 kAccessory1D = 0x1D;
constexpr swd3::u16 kAccessory20 = 0x20;
constexpr swd3::u16 kSpiritFan = 0x39;
constexpr swd3::u32 kAccessory19Bit = 0x10;  // unk26CC bit set by accessory 0x19
constexpr swd3::u32 kAccessory1DBit = 0x100;
constexpr swd3::u16 kAccessoryCost = 30;
constexpr swd3::i16 kLowStamina = 20;
constexpr swd3::i16 kStamina = 40;
constexpr swd3::u16 kEffectTick = 0xF;
constexpr swd3::u16 kPoison = 20;
constexpr swd3::u16 kPoisonTurns = 2;
constexpr swd3::u32 kStaminaDrainBits = 0x2000000;
constexpr swd3::i16 kTooLittleStamina = 9;
constexpr std::string_view kNoData = "/swd3-test-no-data/";
constexpr swd3::u16 kEnemyAttack = 15;
constexpr swd3::u32 kIdleRoll = 2;  // the unused roll when no idle turn has passed
constexpr swd3::u16 kSkill = 0x123;
constexpr swd3::u8 kSureSkill = 10;  // a skill chance no roll of 0..9 reaches
constexpr swd3::u32 kFirstSkillRoll = 6;
constexpr swd3::u16 kAttackValue = 100;
constexpr swd3::u16 kSpread = 5;
constexpr swd3::u16 kSpreadBase = 5;
constexpr swd3::i16 kTargetDefense = 30;
constexpr swd3::i16 kHighDefense = 500;
constexpr swd3::u16 kLevelShare = 3;
constexpr swd3::u16 kLevelRollBase = 3;
constexpr swd3::u16 kHeal = 45;
constexpr int kInside = 10;
constexpr int kPitch = 640;
constexpr int kLines = 480;
constexpr int kGrid = 200;
constexpr int kPastRight = 600;
constexpr int kPastBottom = 400;

}  // namespace

TEST_CASE("battle unit: the catch roll")
{
    using swd3::BattleUnit;
    Services all;
    swd3::ObjectRecord object;
    object.data.level = kEnemyLevel;
    BattleUnit enemy;
    enemy.data = &object;
    swd3::Random reference;

    auto catches = [&](int above, swd3::u32 seed)
    {
        all.random.Seed(seed);
        reference.Seed(seed);
        return enemy.CatchRoll(all.services, static_cast<swd3::u8>(kEnemyLevel + above));
    };

    enemy.marks = BattleUnit::UnkMark800;
    CHECK_EQ(catches(kNearAbove, kSeed), 0);
    CHECK_EQ(all.random.Below(kCatchRoll), reference.Below(kCatchRoll));
    enemy.marks = 0;
    CHECK_EQ(catches(kNearAbove, kSeed), 0);
    reference.Below(kUnusedCatchRoll);
    CHECK_EQ(all.random.Below(kCatchRoll), reference.Below(kCatchRoll));
    object.data.catchChance = kCatchChance;
    CHECK_EQ(catches(kNearAbove, kSeed), 1);
    CHECK_EQ(catches(kNearerAbove, kSeed), 0);

    for (swd3::u32 seed = 0; seed < kCatchRoll; ++seed)
    {
        const int far = catches(kFarAbove, seed);
        reference.Below(kUnusedCatchRoll);
        CHECK_EQ(far, static_cast<swd3::u16>(reference.Below(kCatchRoll)) < kFarCatch ? 1 : 0);
        const int mid = catches(kMidAbove, seed);
        reference.Below(kUnusedCatchRoll);
        CHECK_EQ(mid, static_cast<swd3::u16>(reference.Below(kCatchRoll)) < kMidCatch ? 1 : 0);
    }
}

TEST_CASE("battle unit: accessory commands")
{
    using swd3::BattleUnit;
    BattleUnit role;
    swd3::RoleRecord record;
    record.stamina = kStamina;
    role.roleRecord = &record;
    CHECK_EQ(role.CanUseAccessory(kAccessory19), 1);
    CHECK_EQ(role.unk26CC & kAccessory19Bit, kAccessory19Bit);
    CHECK_EQ(role.CanUseAccessory(kAccessory1C), 1);
    role.status = BattleUnit::Invisible;
    CHECK_EQ(role.CanUseAccessory(kAccessory1C), 0);
    role.unk26CC |= kAccessory1DBit;
    CHECK_EQ(role.CanUseAccessory(kAccessory1D), 0);
    role.status = BattleUnit::Barrier;
    CHECK_EQ(role.CanUseAccessory(kAccessory20), 0);
    CHECK_EQ(role.CanUseAccessory(kSpiritFan), 0);
    role.equipmentFlags = BattleUnit::SpiritFan;
    CHECK_EQ(role.CanUseAccessory(kSpiritFan), 1);
    role.accessories[0].data.actionKind = kSpiritFan;
    role.accessories[0].data.costBits = kAccessoryCost;
    record.stamina = kLowStamina;
    CHECK_EQ(role.CanUseAccessory(kSpiritFan), 0);
}

TEST_CASE("battle unit: effect timers, poison and stamina drain")
{
    using swd3::BattleUnit;
    Services all;
    BattleUnit unit;
    swd3::RoleRecord record;
    record.stamina = kTooLittleStamina;
    unit.battleRecord = &record;
    unit.roleRecord = &record;
    unit.effects.next = std::make_unique<swd3::EffectNode>();
    swd3::EffectNode& poison = *unit.effects.next;
    poison.bits = BattleUnit::UnkStatus1000;
    poison.value = kPoison;
    poison.turns = kPoisonTurns;
    poison.timer = static_cast<swd3::u16>(all.state.gaugeFull - kEffectTick);
    poison.next = std::make_unique<swd3::EffectNode>();
    swd3::EffectNode& drain = *poison.next;
    drain.bits = kStaminaDrainBits;
    drain.turns = kPoisonTurns;
    drain.timer = poison.timer;
    unit.held = 1;
    unit.TickEffects(all.services);
    CHECK_EQ(poison.timer, all.state.gaugeFull - kEffectTick);
    unit.held = 0;
    unit.TickEffects(all.services);
    CHECK_EQ(unit.pendingDamage, kPoison);
    CHECK_EQ(poison.timer, 0);
    CHECK_EQ(poison.turns, kPoisonTurns - 1);
    CHECK_EQ(record.stamina, kTooLittleStamina);
    unit.TickEffects(all.services);
    CHECK_EQ(poison.timer, kEffectTick);
}

TEST_CASE("battle unit: an enemy with nothing to use attacks")
{
    using swd3::BattleUnit;
    Services all;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ObjectRecord object;
    object.data.attack = kEnemyAttack;
    BattleUnit enemy;
    enemy.data = &object;
    swd3::u32 act = 0;
    enemy.commandFlags = BattleUnit::UnkCommandFlag8000;
    CHECK_EQ(enemy.ChooseCommand(all.services, database, act), 1);
    CHECK_EQ(enemy.commandA, 0);
    enemy.commandFlags = 0;
    all.random.Seed(kSeed);
    swd3::Random reference;
    reference.Seed(kSeed);
    CHECK_EQ(enemy.ChooseCommand(all.services, database, act), 0);
    reference.Below(kIdleRoll);
    CHECK_EQ(all.random.Below(kCatchRoll), reference.Below(kCatchRoll));
    CHECK_EQ(enemy.commandA, BattleUnit::UnkCommand1);
    CHECK_EQ(enemy.choicePhase, 1);
    CHECK_EQ(enemy.idleTurns, 1);
    CHECK_EQ(enemy.itemWait, 1);
    CHECK_EQ(enemy.skillWait, 1);
    CHECK_EQ(enemy.unk29A4, kEnemyAttack);
    enemy.ChooseCommand(all.services, database, act);
    CHECK_EQ(enemy.idleTurns, 2);
    CHECK_EQ(enemy.unk29A4, 2 * kEnemyAttack);
}

TEST_CASE("battle unit: an enemy chooses its first skill")
{
    using swd3::BattleUnit;
    Services all;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);

    for (swd3::u32 seed = 0; seed < kCatchRoll; ++seed)
    {
        swd3::ObjectRecord object;
        object.data.skillA = kSkill;
        object.data.skillACount = 1;
        object.data.skillChance = kSureSkill;
        BattleUnit enemy;
        enemy.data = &object;
        enemy.idleTurns = 1;
        swd3::u32 act = 0;
        all.random.Seed(seed);
        swd3::Random reference;
        reference.Seed(seed);
        const bool first = reference.Below(kCatchRoll) <= kFirstSkillRoll;
        CHECK_EQ(enemy.ChooseCommand(all.services, database, act), 0);
        CHECK_EQ(enemy.choicePhase, 2);
        CHECK_EQ(enemy.commandA, BattleUnit::UnkCommand1);

        if (first)
        {
            CHECK_EQ(object.data.skillACount, 0);
            CHECK_EQ(enemy.unk2A8C, 1);
            CHECK_EQ(enemy.idleTurns, 0);
        }
        else
        {
            CHECK_EQ(object.data.skillACount, 1);
            CHECK_EQ(enemy.idleTurns, 2);
        }
    }
}

TEST_CASE("battle unit: the damage of an enemy's hit")
{
    using swd3::BattleUnit;
    Services all;
    swd3::ObjectRecord attackerData;
    attackerData.data.level = kEnemyLevel;
    attackerData.data.unk5C = kEnemyLevel;
    BattleUnit attacker;
    attacker.data = &attackerData;
    attacker.unk29A4 = kAttackValue;
    swd3::ObjectRecord targetData;
    targetData.data.defense = static_cast<swd3::u16>(kTargetDefense);
    BattleUnit target;
    target.data = &targetData;
    swd3::Random reference;

    attacker.commandA = BattleUnit::UnkCommand2;
    all.random.Seed(kSeed);
    reference.Seed(kSeed);
    const swd3::u32 spread = reference.Below(kSpread);
    const auto limit =
        static_cast<swd3::u16>((kEnemyLevel + kEnemyLevel) / static_cast<swd3::u16>(spread + kSpreadBase));
    const int bonus = static_cast<int>(reference.Below(limit)) + 1;
    CHECK_EQ(attacker.EnemyHitDamage(all.services, target), kAttackValue + bonus);

    attacker.commandA = BattleUnit::UnkCommand1;
    all.random.Seed(kSeed);
    reference.Seed(kSeed);
    const int roll = static_cast<int>(reference.Below(kEnemyLevel / kLevelShare + kLevelRollBase)) + 1;
    CHECK_EQ(attacker.EnemyHitDamage(all.services, target), kAttackValue + roll - kTargetDefense);

    targetData.data.defense = static_cast<swd3::u16>(kHighDefense);
    attacker.unk2A8C = 1;
    CHECK_EQ(attacker.EnemyHitDamage(all.services, target), 1);

    attacker.restoreHitPoints = kHeal;
    CHECK_EQ(attacker.EnemyHitDamage(all.services, target), -kHeal);
}

TEST_CASE("battle state: the slot records are wiped with 0xFF bytes")
{
    swd3::BattleState state;
    state.WipeSlotRecords();

    for (const swd3::BattleSlotRecord& record : state.slotRecords)
    {
        CHECK_EQ(record.unit, -1);
        CHECK_EQ(record.targetCount, -1);
        CHECK_EQ(record.kind, 0xFFFF);
        CHECK_EQ(record.unk0A, 0xFFFF);
        CHECK_EQ(record.unk18, -1);
    }
}

TEST_CASE("script object: a moved ripple stays inside the screen copy")
{
    swd3::ScriptObject ripple(kPitch, kLines, 0, 0, kGrid, kGrid);
    ripple.MoveTo(kInside, kInside);
    CHECK_EQ(ripple.left, kInside);
    CHECK_EQ(ripple.top, kInside);
    ripple.MoveTo(kPastRight, kPastBottom);
    CHECK_EQ(ripple.left, kPitch - kGrid - 1);
    CHECK_EQ(ripple.top, kLines - kGrid - 1);
}

TEST_CASE("battle state: the acting unit is drawn right after its target")
{
    constexpr std::size_t kFirstEnemy = 0;
    constexpr std::size_t kTargetEnemy = 1;
    constexpr std::size_t kActingRole = swd3::BattleState::FirstRoleUnit;
    constexpr std::size_t kOtherRole = swd3::BattleState::FirstRoleUnit + 1;
    constexpr swd3::i32 kTop = 100;
    constexpr swd3::i32 kActingY = 150;
    constexpr swd3::i32 kTargetY = 200;
    constexpr swd3::i32 kBottom = 250;
    constexpr swd3::u32 kTwo = 2;
    swd3::BattleState state;
    state.enemyCount = kTwo;
    state.roleCount = kTwo;
    state.unitY[kFirstEnemy] = kTop;
    state.unitY[kTargetEnemy] = kTargetY;
    state.unitY[kActingRole] = kActingY;
    state.unitY[kOtherRole] = kBottom;
    state.SortDrawOrderAround(static_cast<swd3::i32>(kActingRole), static_cast<swd3::i32>(kTargetEnemy));
    CHECK_EQ(state.drawOrder[0], static_cast<swd3::i32>(kFirstEnemy));
    CHECK_EQ(state.drawOrder[1], static_cast<swd3::i32>(kTargetEnemy));
    CHECK_EQ(state.drawOrder[2], static_cast<swd3::i32>(kActingRole));
    CHECK_EQ(state.drawOrder[3], static_cast<swd3::i32>(kOtherRole));
    CHECK_EQ(state.orderSorted, 1);
    CHECK(
        std::all_of(
            state.drawTaken.begin(),
            state.drawTaken.end(),
            [](swd3::i32 taken)
            {
                return taken == 0;
            }
        )
    );
}

TEST_CASE("battle state: with no unit left the draw order reads the dword after the y table")
{
    constexpr std::size_t kTargetEnemy = 0;
    constexpr std::size_t kActingRole = swd3::BattleState::FirstRoleUnit;
    constexpr swd3::i32 kActingY = 100;
    constexpr swd3::i32 kDarkFrames = 5;  // above the target's y of 0: the target is taken
    swd3::BattleState state;
    state.enemyCount = 1;
    state.roleCount = 1;
    state.unitY[kActingRole] = kActingY;
    state.darkFrames = kDarkFrames;
    state.SortDrawOrderAround(static_cast<swd3::i32>(kActingRole), static_cast<swd3::i32>(kTargetEnemy));
    CHECK_EQ(state.drawOrder[0], static_cast<swd3::i32>(kTargetEnemy));
    CHECK_EQ(state.drawOrder[1], static_cast<swd3::i32>(kActingRole));
    state.drawOrder.fill(0);
    state.darkFrames = 0;
    state.SortDrawOrderAround(static_cast<swd3::i32>(kActingRole), static_cast<swd3::i32>(kTargetEnemy));
    CHECK_EQ(state.drawOrder[0], static_cast<swd3::i32>(kActingRole));
}

TEST_CASE("battle state: the initial turn and push values match swd32.exe")
{
    constexpr swd3::u32 kTurnEnemyAt = 0x004A754C;
    constexpr swd3::u32 kAimedEnemyAt = 0x004A7626;
    constexpr swd3::u32 kLastHitsAt = 0x004A7620;
    constexpr swd3::u32 kAutoAimAt = 0x004A7554;
    constexpr swd3::u32 kOpeningPlacesAt = 0x004A7580;
    constexpr swd3::u32 kPanelsArgumentAt = 0x004A7628;
    constexpr swd3::u32 kEndFlagAt = 0x004A763C;
    constexpr swd3::u32 kPushAt = 0x004A7638;
    constexpr swd3::u32 kTurnPointerAt = 0x004A7640;
    constexpr swd3::u32 kTurnStepAt = 0x004A7550;
    constexpr swd3::u32 kListMarksAt = 0x004A7558;
    constexpr swd3::u32 kListRoleAt = 0x004A7570;
    constexpr swd3::u32 kRoleCycleAt = 0x004A796C;
    constexpr swd3::u32 kAutoEnemyAt = 0x004A7564;
    constexpr swd3::u32 kMenuCommandAt = 0x004A7548;
    constexpr swd3::u32 kMasteryRoleAt = 0x004A7646;
    constexpr swd3::u32 kListMarkAt = 0x004A7560;
    constexpr swd3::u32 kTalkRolesAt = 0x004A762C;
    constexpr swd3::u32 kFrameMillisecondsAt = 0x004A75E8;
    constexpr swd3::u32 kHoverTabsAt = 0x004A7578;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    const swd3::BattleState state;
    CHECK(swd3test::SameTable(exe, kTurnEnemyAt, std::array<swd3::i32, 1>{state.aimTarget}));
    CHECK(swd3test::SameTable(exe, kAimedEnemyAt, std::array<swd3::u16, 1>{state.unk4A7626}));
    CHECK(
        swd3test::SameTable(
            exe,
            kLastHitsAt,
            std::array<swd3::u16, 3>{state.lastRoleHit, state.lastEnemyHit, state.lastRoleDefeated}
        )
    );
    CHECK(swd3test::SameTable(exe, kAutoAimAt, std::array<swd3::i32, 1>{state.aiTarget}));
    CHECK(
        swd3test::SameTable(
            exe,
            kOpeningPlacesAt,
            std::array<swd3::i32, 6>{
                state.unk4A7580,
                state.unk4A7584,
                state.unk4A7588,
                state.unk4A758C,
                state.unk4A7590,
                state.unk4A7594
            }
        )
    );
    CHECK(swd3test::SameTable(exe, kPanelsArgumentAt, std::array<swd3::u16, 1>{state.unk4A7628}));
    CHECK(swd3test::SameTable(exe, kEndFlagAt, std::array<swd3::i32, 1>{state.unk4A763C}));
    CHECK(swd3test::SameTable(exe, kPushAt, std::array<swd3::i16, 1>{state.hitPush}));
    CHECK(swd3test::SameTable(exe, kTurnPointerAt, std::array<swd3::i32, 1>{state.turnPointer}));
    CHECK(swd3test::SameTable(exe, kTurnStepAt, std::array<swd3::i32, 1>{state.orderStep}));
    CHECK(swd3test::SameTable(exe, kListMarksAt, std::array<swd3::i32, 2>{state.pageListRow, state.roleListRow}));
    CHECK(swd3test::SameTable(exe, kListRoleAt, std::array<swd3::i32, 1>{state.listRole}));
    CHECK(swd3test::SameTable(exe, kRoleCycleAt, swd3::BattleState::RoleCycle));
    CHECK(swd3test::SameTable(exe, kAutoEnemyAt, std::array<swd3::i32, 1>{state.autoTarget}));
    CHECK(swd3test::SameTable(exe, kMenuCommandAt, std::array<swd3::i32, 1>{state.menuCommand}));
    CHECK(swd3test::SameTable(exe, kMasteryRoleAt, std::array<swd3::u8, 1>{state.levelUpRole}));
    CHECK(
        swd3test::SameTable(
            exe,
            kListMarkAt,
            std::array<swd3::i32, 5>{
                state.specialRow,
                state.autoTarget,
                state.controlRows,
                state.controlCursor,
                state.listRole
            }
        )
    );
    CHECK(swd3test::SameTable(exe, kTalkRolesAt, std::array<swd3::u16, 2>{state.controlledEnemy, state.talkRole}));
    CHECK(swd3test::SameTable(exe, kFrameMillisecondsAt, std::array<swd3::i32, 1>{state.frameMilliseconds}));
    CHECK(swd3test::SameTable(exe, kHoverTabsAt, std::array<swd3::i32, 2>{state.hoverPageTab, state.hoverRoleTab}));
}

TEST_CASE("battle unit: element orbs gain proficiency from hits of an element")
{
    constexpr swd3::u16 kElementOrb = 0x33;
    constexpr swd3::i16 kOrbCounter = 7;
    constexpr swd3::u16 kOrbMaximum = 100;
    constexpr swd3::u16 kOrbStep = 12;
    constexpr swd3::u16 kFireElement = 1;
    constexpr swd3::i16 kOtherCounter = 5;
    swd3::BattleUnit role;
    role.accessories[0].data.actionKind = kElementOrb;
    role.accessories[0].data.act = static_cast<swd3::u16>(kOrbCounter);
    role.accessories[0].data.counterMax = kOrbMaximum;
    swd3::CounterList list;
    swd3::UnitCommand command;
    CHECK_EQ(role.StepOrbCounters(list, command), 0);
    CHECK(list.next == nullptr);
    command.resistance = kFireElement;
    CHECK_EQ(role.StepOrbCounters(list, command), 1);
    CHECK_EQ(list.id, 1);
    CHECK(list.next != nullptr);

    if (list.next == nullptr)
    {
        return;
    }

    CHECK_EQ(list.next->id, kOrbCounter);
    CHECK_EQ(list.next->value, kOrbStep);
    CHECK_EQ(list.next->percent, kOrbStep);
    role.StepOrbCounters(list, command);
    CHECK_EQ(list.next->value, 2 * kOrbStep);

    // The head's count is compared as an id: an orb counter equal to it steps the head.
    swd3::CounterList counted;
    counted.next = std::make_unique<swd3::CounterNode>();
    counted.next->id = kOtherCounter;
    counted.id = 1;
    role.accessories[0].data.act = 1;
    role.StepOrbCounters(counted, command);
    CHECK_EQ(counted.value, kOrbStep);
    CHECK(counted.next->next == nullptr);
}

TEST_CASE("battle role: the share of a spirit drain")
{
    constexpr swd3::u16 kProficiency = 40;
    constexpr swd3::i32 kDrained = 50;
    constexpr int kPercent = 100;
    Services all;
    swd3::BattleRole role;
    swd3::i32 amount = kDrained;
    CHECK_EQ(role.DrainShare(all.services, amount), 0);
    CHECK_EQ(amount, kDrained);
    role.equipmentFlags = swd3::BattleUnit::SpiritDrain;
    role.proficiency = kProficiency;
    CHECK_EQ(role.DrainShare(all.services, amount), 1);
    CHECK_EQ(role.proficiency, kProficiency / 2);
    CHECK_EQ(amount, kDrained * (kProficiency / 2) / kPercent + 1);
}

TEST_CASE("battle state: the quake steps match swd32.exe")
{
    constexpr swd3::u32 kQuakeStepsAt = 0x004A75EC;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kQuakeStepsAt, swd3::BattleState::QuakeSteps));
}

TEST_CASE("battle state: taking a slot record")
{
    constexpr swd3::i32 kFirstUnit = 3;
    constexpr swd3::i32 kSecondUnit = 9;
    constexpr std::size_t kTakenAt = 2;
    swd3::BattleState state;
    auto& records = state.slotRecords;
    records[0].unit = kFirstUnit;
    records[1].unit = kSecondUnit;
    swd3::BattleSlotRecord taken;
    state.TakeSlotRecordAt(0, taken);
    CHECK_EQ(taken.unit, kFirstUnit);
    CHECK_EQ(records[0].unit, kSecondUnit);
    CHECK_EQ(records[1].unit, -1);

    // With no free record left after the move, the records are cleared from the one taken on.
    for (std::size_t place = 0; place < records.size(); ++place)
    {
        records[place].unit = static_cast<swd3::i32>(place);
    }

    state.TakeSlotRecordAt(kTakenAt, taken);
    CHECK_EQ(taken.unit, static_cast<swd3::i32>(kTakenAt));
    CHECK_EQ(records[1].unit, 1);
    CHECK_EQ(records[kTakenAt].unit, -1);
    CHECK_EQ(records[kTakenAt + 1].unit, -1);
}

TEST_CASE("battle unit: a damage number hops, rises and fades")
{
    constexpr swd3::u16 kDelay = 1;
    constexpr int kHopFrames = 9;  // the digits hop up together from step 0 to step 9
    constexpr int kFadeFrames = 0x30 / 2;
    constexpr int kFrames = kDelay + kHopFrames + kFadeFrames + 1;
    constexpr swd3::i16 kTens = 4;
    constexpr swd3::i16 kOnes = 2;
    constexpr swd3::i32 kExtraDigit = 5;
    using swd3::BattleUnit;
    Services all;
    BattleUnit unit;
    CHECK_EQ(unit.DrawDamageNumbers(all.services), 0);
    unit.unk2AD8 = 1;
    unit.targetCount = 1;
    unit.damageDigits[0] = {0, 0, kTens, kOnes};
    unit.damageDelays[0] = kDelay;
    unit.held = 1;
    unit.DrawDamageNumbers(all.services);
    unit.DrawDamageNumbers(all.services);
    CHECK_EQ(unit.unk283C[0][0], 0);
    unit.held = 0;
    unit.damageDelays[0] = kDelay;
    int frame = 1;

    for (; frame < kFrames; ++frame)
    {
        if (unit.DrawDamageNumbers(all.services) != 0)
        {
            break;
        }
    }

    CHECK_EQ(frame, kFrames);
    CHECK_EQ(unit.DrawDamageNumbers(all.services), 1);
    CHECK_EQ(unit.targetCount, 0);
    CHECK_EQ(unit.unk2AD8, 0);
    CHECK_EQ(unit.damageDigits[0][2], 0);

    unit.unk2AD8 = 1;
    unit.targetCount = 1;
    unit.damageSlot = 0;
    unit.damageDigits[0] = {0, 0, kTens, kOnes};
    unit.unk283C[0] = {kHopFrames, kHopFrames, kHopFrames, kHopFrames};
    unit.extraNumber = 1;
    all.state.unk53AEDC = kExtraDigit;
    unit.DrawDamageNumbers(all.services);
    CHECK_EQ(unit.extraNumber, 0);
    CHECK_EQ(unit.targetCount, 2);
    CHECK_EQ(unit.damageDigits[1][0], kExtraDigit);
}

TEST_CASE("battle unit: the damage digit hops match swd32.exe")
{
    constexpr swd3::u32 kDigitHopsAt = 0x004A7D8C;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kDigitHopsAt, swd3::BattleUnit::DigitHops));
}

TEST_CASE("battle unit: the names of an enemy's objects and skills")
{
    constexpr swd3::u16 kFirstObject = 0x31;
    constexpr swd3::u16 kThirdObject = 0x33;
    constexpr swd3::u16 kSecondSkill = 0x45;
    constexpr int kThird = 2;
    constexpr int kSecond = 1;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ObjectRecord object;
    swd3::BattleUnit enemy;
    enemy.data = &object;
    std::string name;
    swd3::u32 id = 0;
    CHECK_EQ(enemy.ItemName(database, 0, name, id), 0);
    object.data.itemA = kFirstObject;
    object.data.itemC = kThirdObject;
    CHECK_EQ(enemy.ItemName(database, kThird, name, id), 1);
    CHECK_EQ(id, kFirstObject);
    CHECK_EQ(enemy.SkillName(database, kSecond, name), 0);
    object.data.skillB = kSecondSkill;
    CHECK_EQ(enemy.SkillName(database, kSecond, name), 1);
}

TEST_CASE("battle state: the panel units match swd32.exe")
{
    constexpr swd3::u32 kPanelUnitsAt = 0x004A7960;
    constexpr swd3::u32 kPanelRoleAt = 0x004A7644;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    const swd3::BattleState state;
    CHECK(swd3test::SameTable(exe, kPanelUnitsAt, swd3::BattleState::PanelUnits));
    CHECK(swd3test::SameTable(exe, kPanelRoleAt, std::array<swd3::u16, 1>{state.anyStateRole}));
}

TEST_CASE("battle state: the names of the accessory commands match swd32.exe")
{
    constexpr swd3::u32 kNamesAt = 0x004A74A0;
    constexpr swd3::u32 kFirstKind = 0x15;
    constexpr swd3::u32 kSpiritPactAt = 0x004A79A0;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    for (std::size_t at = 0; at < swd3::BattleState::AccessoryCommandNames.size(); ++at)
    {
        const auto kind = kFirstKind + static_cast<swd3::u32>(at);
        const swd3::u32 address = swd3test::ExeDword(exe, kNamesAt + kind * sizeof(swd3::u32));
        CHECK(swd3test::SameText(exe, address, swd3::BattleState::AccessoryCommandNames[at]));
    }

    CHECK(swd3test::SameText(exe, kSpiritPactAt, swd3::BattleState::SpiritPactName));

    constexpr swd3::u32 kFixedNamesAt = 0x004A7534;

    for (std::size_t at = 0; at < swd3::BattleState::FixedCommandNames.size(); ++at)
    {
        const swd3::u32 address =
            swd3test::ExeDword(exe, kFixedNamesAt + static_cast<swd3::u32>(at * sizeof(swd3::u32)));
        CHECK(swd3test::SameText(exe, address, swd3::BattleState::FixedCommandNames[at]));
    }
}

TEST_CASE("battle unit: drops and finished proficiencies")
{
    constexpr swd3::u16 kDrop = 7;
    constexpr swd3::u16 kSureDrop = 20;  // above every roll of 0..19
    constexpr swd3::u16 kCounter = 9;
    constexpr swd3::u16 kMaximum = 50;
    Services all;
    swd3::ObjectRecord object;
    swd3::BattleUnit enemy;
    enemy.data = &object;
    CHECK_EQ(enemy.RollDrop(all.services), 0);
    object.data.dropItem = kDrop;
    CHECK_EQ(enemy.RollDrop(all.services), 0);
    object.data.dropChance = kSureDrop;
    CHECK_EQ(enemy.RollDrop(all.services), kDrop);

    swd3::BattleUnit role;
    role.accessories[0].data.act = kCounter;
    role.accessories[0].data.counterMax = kMaximum;
    swd3::CounterList list;
    list.next = std::make_unique<swd3::CounterNode>();
    list.next->id = static_cast<swd3::i16>(kCounter);
    list.next->value = kMaximum - 1;
    CHECK_EQ(role.FinishProficiency(list), 0);
    list.next->value = kMaximum;
    CHECK_EQ(role.FinishProficiency(list), kCounter);
    CHECK_EQ(list.next->done, 1);
    CHECK_EQ(role.FinishProficiency(list), 0);
}

TEST_CASE("battle role: counting and listing the inventory")
{
    constexpr swd3::u32 kCountedKind = 0x8000000;
    constexpr swd3::u16 kFirstRoleUser = 1;
    constexpr swd3::u16 kThrownObject = 0x300;
    constexpr swd3::u16 kListedBit = 0x2000;
    constexpr swd3::i16 kCount = 2;
    constexpr swd3::i16 kPending = 1;
    constexpr swd3::u16 kProficiency = 33;
    constexpr std::array<char, 4> kListedName = {'A', 'B', 'C', '\0'};
    Services all;
    swd3::ItemLink head = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& counted = *head;
    counted.id = 1;
    counted.record.data.kind = kCountedKind;
    counted.record.data.usableBy = kFirstRoleUser;
    counted.count = kCount;
    counted.pending = kPending;
    counted.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& thrown = *counted.next;
    thrown.id = kThrownObject;
    thrown.record.data.kind = kCountedKind;
    thrown.record.data.usableBy = kFirstRoleUser;
    thrown.count = kCount;
    thrown.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& listed = *thrown.next;
    listed.record.data.costBits = kListedBit;
    listed.count = kCount;
    std::copy(kListedName.begin(), kListedName.end(), listed.record.data.name.begin());
    swd3::BattleRole role;
    role.SetInventory(&head);
    swd3::u16 count = 0;
    role.CountUsableObjects(count);
    CHECK_EQ(count, kCount + kPending);
    CHECK(role.inventoryNode == nullptr);
    std::string name;
    CHECK_EQ(role.ListedObject(1, name, count), 1);
    CHECK(name == "ABC");
    CHECK_EQ(count, kCount);
    CHECK_EQ(role.ListedObject(2, name, count), 0);
    role.proficiency = kProficiency;
    CHECK_EQ(role.Unk1EProficiency(all.services), kProficiency);
}

TEST_CASE("battle role: the object and skill lists of the battle menu")
{
    constexpr swd3::u32 kHerbKind = 0x10;  // category 0
    constexpr swd3::u32 kThrowKind = 0x8000000;
    constexpr swd3::u16 kThrownObject = 0x300;
    constexpr swd3::u16 kUsable = 1;
    constexpr swd3::i16 kTwo = 2;
    constexpr swd3::u16 kListedBit = 0x2000;
    constexpr int kCategory4 = 4;
    constexpr swd3::u16 kCostly = 0x4000 | 50;
    constexpr swd3::i16 kStamina = 40;
    constexpr swd3::u16 kShown = 0x8000;
    constexpr swd3::u16 kShownCostly = 0x8000 | 0x4000;
    constexpr swd3::u16 kSkillType0 = 0x1C;
    constexpr swd3::u16 kSkillType1 = 0x1F;
    constexpr int kAnyIndex = 5;

    swd3::ItemLink head = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& herb = *head;
    herb.record.data.kind = kHerbKind;
    herb.record.data.usableBy = kUsable;
    herb.record.data.costBits = kCostly;
    herb.count = kTwo;
    herb.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& spent = *herb.next;
    spent.record.data.kind = kHerbKind;
    spent.record.data.usableBy = kUsable;
    spent.record.data.costBits = kListedBit;
    spent.count = kTwo;
    spent.workCount = kTwo;
    spent.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& thrown = *spent.next;
    thrown.id = kThrownObject;
    thrown.record.data.kind = kThrowKind;
    thrown.record.data.usableBy = kUsable;
    thrown.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& dart = *thrown.next;
    dart.record.data.kind = kThrowKind;
    dart.record.data.usableBy = kUsable;
    swd3::RoleRecord record;
    record.stamina = kStamina;
    swd3::BattleRole role;
    role.roleRecord = &record;
    role.SetInventory(&head);

    swd3::u16 count = 0;
    role.CountObjects(0, count);
    CHECK_EQ(count, 1);
    count = 0;
    role.CountObjects(kCategory4, count);
    CHECK_EQ(count, 1);
    std::string name;
    swd3::u16 flags = 0;
    CHECK_EQ(role.ListObject(0, 1, name, flags, count), 1);
    CHECK_EQ(flags, kShownCostly);
    CHECK_EQ(count, kTwo);
    record.stamina = static_cast<swd3::i16>(kCostly);
    role.ListObject(0, 1, name, flags, count);
    CHECK_EQ(flags, kShown);
    CHECK_EQ(role.ListObject(0, 2, name, flags, count), 0);
    CHECK_EQ(flags, 0);

    CHECK_EQ(role.ThrowableObject(1, 1, name, count), 1);
    CHECK(role.selectedItem == &dart);
    CHECK_EQ(role.ThrowableObject(0, kAnyIndex, name, count), 1);
    CHECK(role.selectedItem == &dart);
    CHECK(role.inventoryNode == &thrown);

    swd3::ItemNode skillHead;
    skillHead.next = std::make_unique<swd3::ItemNode>();
    skillHead.next->record.data.kind = kHerbKind;
    skillHead.next->record.data.usableBy = kUsable;
    skillHead.next->record.data.category = kSkillType0;
    skillHead.next->next = std::make_unique<swd3::ItemNode>();
    skillHead.next->next->record.data.kind = kHerbKind;
    skillHead.next->next->record.data.usableBy = kUsable;
    skillHead.next->next->record.data.category = kSkillType1;
    role.SetSkills(&skillHead);
    swd3::u8 skills = 0;
    role.CountSkills(0, 0, skills);
    CHECK_EQ(skills, 1);
    role.CountSkills(0, 1, skills);
    CHECK_EQ(skills, 2);
}

TEST_CASE("battle role: a skill of the battle menu and its cost")
{
    constexpr swd3::u32 kCategoryKind = 0x10;
    constexpr swd3::u16 kUsable = 1;
    constexpr swd3::u16 kSkillType0 = 0x1C;
    constexpr swd3::u16 kStaminaCost = 0x4000 | 12;
    constexpr swd3::u16 kShownCost = 0x8000 | 12;
    constexpr swd3::u16 kFixed = 0x800;
    constexpr swd3::u16 kFixedCost = 1000;
    constexpr swd3::u16 kNone = 0xFFFF;
    constexpr swd3::u16 kElementZeroIcon = 8;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ItemNode head;
    head.next = std::make_unique<swd3::ItemNode>();
    swd3::ObjectData& data = head.next->record.data;
    data.kind = kCategoryKind;
    data.usableBy = kUsable;
    data.category = kSkillType0;
    data.costBits = kStaminaCost;
    swd3::BattleRole role;
    role.SetSkills(&head);
    std::string name;
    swd3::u16 cost = 0;
    CHECK_EQ(role.ListSkill(database, 0, 0, 1, name, cost), kElementZeroIcon);
    CHECK_EQ(cost, kShownCost);
    data.costBits = kFixed;
    role.ListSkill(database, 0, 0, 1, name, cost);
    CHECK_EQ(cost, kFixedCost);
    role.ListSkill(database, 0, 1, 0, name, cost);
    CHECK_EQ(cost, 1);
    CHECK_EQ(role.ListSkill(database, 0, 0, 2, name, cost), kNone);
    CHECK_EQ(cost, kNone);
}

TEST_CASE("battle role: the worn accessories gain proficiency")
{
    constexpr swd3::u16 kCounter = 4;
    constexpr swd3::u16 kMaximum = 10;
    constexpr swd3::u16 kStep = 6;
    swd3::BattleRole role;
    role.accessories[1].data.act = kCounter;
    role.accessories[1].data.counterMax = kMaximum;
    swd3::CounterList list;
    CHECK_EQ(role.StepWornCounters(list, kStep), 1);
    CHECK_EQ(list.id, 1);
    CHECK_EQ(list.next->value, kStep);
    role.StepWornCounters(list, kStep);
    CHECK_EQ(list.next->value, kMaximum);
    CHECK_EQ(list.next->percent, 100);
}

TEST_CASE("battle unit: refining an enemy into a talisman")
{
    constexpr swd3::u16 kObject = 0x40;
    constexpr swd3::u16 kEnemyLevel = 20;
    constexpr swd3::u8 kHigherLevel = 30;
    constexpr swd3::u16 kLowProficiency = 10;
    constexpr swd3::u32 kRefineRoll = 100;
    constexpr swd3::u16 kSureChance = 90;
    Services all;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ObjectRecord object;
    object.data.level = kEnemyLevel;
    swd3::BattleUnit enemy;
    enemy.data = &object;
    CHECK_EQ(enemy.RefineTalisman(all.services, database, kLowProficiency, kHigherLevel), 0);
    enemy.marks = swd3::BattleUnit::MissedMark;
    object.data.itemA = kObject;

    for (swd3::u32 seed = 0; seed < kRefineRoll; ++seed)
    {
        enemy.marks = swd3::BattleUnit::MissedMark;
        all.random.Seed(seed);
        swd3::Random reference;
        reference.Seed(seed);
        reference.Below(1);
        const bool refined = static_cast<swd3::u16>(reference.Below(kRefineRoll)) < kSureChance;
        CHECK_EQ(enemy.RefineTalisman(all.services, database, kLowProficiency, kHigherLevel), 0);
        CHECK_EQ((enemy.marks & swd3::BattleUnit::MissedMark) == 0, refined);
    }
}

TEST_CASE("battle role: the skill element icons match swd32.exe")
{
    constexpr swd3::u32 kSkillElementsAt = 0x004A7BD4;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameTable(exe, kSkillElementsAt, swd3::BattleRole::SkillElements));
}

TEST_CASE("battle role: choosing a skill and paying for it")
{
    constexpr swd3::u32 kCategoryKind = 0x10;
    constexpr swd3::u16 kUsable = 1;
    constexpr swd3::u16 kSkillType0 = 0x1C;
    constexpr swd3::u16 kAct = 0x77;
    constexpr swd3::u16 kHeal = 30;
    constexpr swd3::u16 kSpiritCost = 0x8000 | 20;
    constexpr swd3::i16 kLowSpirit = 10;
    constexpr swd3::i16 kHighSpirit = 25;
    constexpr swd3::u16 kItemCost = 0x800 | 0x55;
    constexpr swd3::u16 kCostItem = 0x55;
    constexpr swd3::u16 kNoSkill = 0xFFFF;
    Services all;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ItemNode head;
    head.next = std::make_unique<swd3::ItemNode>();
    swd3::ObjectData& data = head.next->record.data;
    data.kind = kCategoryKind;
    data.usableBy = kUsable;
    data.category = kSkillType0;
    data.act = kAct;
    data.hitPointsUp = kHeal;
    data.costBits = kSpiritCost;
    swd3::RoleRecord record;
    record.spirit = kLowSpirit;
    swd3::BattleRole role;
    role.roleRecord = &record;
    role.SetSkills(&head);
    swd3::u32 act = 0;
    CHECK_EQ(role.SelectSkill(database, 0, 0, 0, act), kNoSkill);
    CHECK_EQ(role.SelectSkill(database, 0, 0, 1, act), 1);
    CHECK_EQ(act, kAct);
    CHECK_EQ(role.restoreHitPoints, kHeal);
    CHECK_EQ(role.commandFlags & swd3::BattleUnit::UnkCommandFlag8000, swd3::BattleUnit::UnkCommandFlag8000);

    CHECK_EQ(role.CanPaySkill(all.services, 0, 1), 0);
    CHECK(all.messages.messages != nullptr);
    record.spirit = kHighSpirit;
    CHECK_EQ(role.CanPaySkill(all.services, 0, 1), 1);

    data.costBits = kItemCost;
    swd3::ItemLink items = std::make_unique<swd3::ItemNode>();
    items->id = kCostItem;
    role.SetInventory(&items);
    CHECK_EQ(role.CanPaySkill(all.services, 0, 1), 0);
    items->count = 1;
    CHECK_EQ(role.CanPaySkill(all.services, 0, 1), 1);
    CHECK(role.selectedItem == items.get());
}

TEST_CASE("battle role: the cost refusals match swd32.exe")
{
    constexpr swd3::u32 kNotEnoughItemsAt = 0x004A7CD0;
    constexpr swd3::u32 kNotEnoughSpiritAt = 0x004A7CC0;
    constexpr swd3::u32 kNotEnoughStaminaAt = 0x004A7CB0;
    const std::vector<swd3::u8> exe = swd3test::ReadGameFile("swd32.exe");

    if (exe.empty())
    {
        std::printf("  (swd32.exe not found, skipped)\n");
        return;
    }

    CHECK(swd3test::SameText(exe, kNotEnoughItemsAt, swd3::BattleRole::NotEnoughItems));
    CHECK(swd3test::SameText(exe, kNotEnoughSpiritAt, swd3::BattleRole::NotEnoughSpirit));
    CHECK(swd3test::SameText(exe, kNotEnoughStaminaAt, swd3::BattleRole::NotEnoughStamina));
}

TEST_CASE("battle role: choosing an object in battle")
{
    constexpr swd3::u32 kHerbKind = 0x10;  // category 0
    constexpr swd3::u32 kThrowKind = 0x8000000;
    constexpr int kThrowCategory = 5;
    constexpr swd3::u16 kThrownObject = 0x300;
    constexpr swd3::u16 kUsable = 1;
    constexpr swd3::u16 kThrownChoice = 2;
    Services all;
    const std::u8string directory = swd3::Utf8Bytes(kNoData);
    swd3::Database database(directory);
    swd3::ItemLink head = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& herb = *head;
    herb.record.data.kind = kHerbKind;
    herb.record.data.usableBy = kUsable;
    herb.count = 1;
    herb.next = std::make_unique<swd3::ItemNode>();
    swd3::ItemNode& thrown = *herb.next;
    thrown.id = kThrownObject;
    thrown.record.data.kind = kThrowKind;
    thrown.record.data.usableBy = kUsable;
    thrown.count = 1;
    swd3::RoleRecord record;
    swd3::BattleRole role;
    role.roleRecord = &record;
    role.SetInventory(&head);
    swd3::u32 act = 0;
    swd3::u16 kind = 0;
    CHECK_EQ(role.SelectObject(database, all.errors, 0, 0, act, kind), 0);
    CHECK_EQ(role.SelectObject(database, all.errors, 0, 1, act, kind), 1);
    CHECK_EQ(kind, 1);
    CHECK(role.selectedItem == &herb);
    CHECK_EQ(herb.workCount, 1);
    CHECK_EQ(all.errors.messages.size(), 1U);
    CHECK_EQ(role.SelectObject(database, all.errors, 0, 1, act, kind), 0);
    CHECK_EQ(role.SelectObject(database, all.errors, kThrowCategory, 1, act, kind), 1);
    CHECK_EQ(kind, kThrownChoice);
    CHECK_EQ(role.commandFlags & swd3::BattleUnit::UnkCommandFlag2000, swd3::BattleUnit::UnkCommandFlag2000);
}

TEST_CASE("battle state: the menu kind words after the command kinds")
{
    constexpr swd3::u16 kAfter = 0x1234;
    constexpr swd3::i32 kFirst = 0x00070006;
    constexpr swd3::i32 kSecond = 0x00090008;
    constexpr std::size_t kGuard = 4;
    constexpr std::size_t kSecondAccessory = 6;
    constexpr std::size_t kThirdAccessory = 7;
    constexpr swd3::i32 kLowWord = 0xFFFF;
    constexpr int kHighShift = 16;
    swd3::BattleState state;
    state.unk4FE5D2 = kAfter;
    state.unk4FE5D4[0] = kFirst;
    state.unk4FE5D4[1] = kSecond;
    CHECK_EQ(state.MenuKindWord(kGuard), kAfter);
    CHECK_EQ(state.MenuKindWord(kGuard + 1), kFirst & kLowWord);
    CHECK_EQ(state.MenuKindWord(kSecondAccessory), kFirst >> kHighShift);
    CHECK_EQ(state.MenuKindWord(kThirdAccessory), kSecond & kLowWord);
}
