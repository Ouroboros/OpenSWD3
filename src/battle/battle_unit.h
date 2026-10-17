#pragma once

#include <array>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "core/types.h"
#include "org/object_record.h"
#include "pic/projector.h"
#include "res/act_state.h"

namespace swd3
{

struct BattleServices;
struct RoleRecord;
struct Picture;
struct SkillScript;

// A status effect of a unit (a list node in the original: next, the effect animation, the status bits, the timer, the
// turns left, the element, the icon and a value). The unit holds the head as a node of its own: some walks start there.
struct EffectNode
{
    // EffectNode::icon of each ailment (meanings follow SkillScript::Ailment).
    enum Icon : u16
    {
        UnkIcon1 = 1,
        Ailment1Icon = 2,
        Ailment4Icon = 3,
        Ailment8Icon = 4,
        Ailment10Icon = 5,
        SlowedIcon = 6,
        Ailment80Icon = 7,
        Ailment2Icon = 8,
        Ailment20Icon = 9,
        Ailment100Icon = 0xA,
        StatsScaledIcon = 0xB,
    };

    // The action of the effect act shown for each ailment.
    enum Frame : int
    {
        Ailment8Frame = 0,
        Ailment1Frame = 1,
        Ailment80Frame = 2,
        SlowedFrame = 3,
        Ailment20Frame = 4,
        Ailment10Frame = 5,
        StatsScaledFrame = 6,
        Ailment2Frame = 7,
        Ailment100Frame = 8,
    };

    std::unique_ptr<EffectNode> next;  // +0x00
    ActState act;                      // +0x04
    u32 bits = 0;                      // +0x9C
    u16 timer = 0;                     // +0xA0
    u16 turns = 0;                     // +0xA2
    u16 element = 0;                   // +0xA4 the element of a resistance effect
    u16 icon = 0;                      // +0xA6
    u16 value = 0;                     // +0xA8
};

// The afterimages of a moving act (0x98 bytes; in a queued action at +0x134 and eight in a unit at +0xFCC). DrawTrail
// pushes the current picture in front every third frame and draws the images fading by fadeStep.
struct AfterimageTrail
{
    static constexpr std::size_t Images = 8;
    static constexpr u16 NoPicture = 0xFFFF;
    static constexpr u16 Left = 1;     // direction: the images trail to the left
    static constexpr u16 InPlace = 2;  // direction: the images stay on the picture

    struct Image
    {
        u32 mode = 0;     // +0x00 the draw mode
        u16 picture = 0;  // +0x04 NoPicture: none
        i16 level = 0;    // +0x06 the blend level, falling by fadeStep
        i16 x = 0;        // +0x08
        i16 y = 0;        // +0x0A
        i32 unk0C = 0;
    };

    std::array<Image, Images> images = {};  // +0x00 the newest first
    i32 tintRed = 0;                        // +0x80 the tint of the images under the translucent ones
    i32 tintGreen = 0;
    i32 tintBlue = 0;
    i32 unk8C = 0;
    u16 direction = 0;  // +0x90 0: the images trail to the right, Left, InPlace
    u16 started = 0;    // +0x92
    i8 count = 0;       // +0x94 the images drawn
    i8 fadeStep = 0;    // +0x95
    u8 delay = 0;       // +0x96 frames until the next image is pushed
};

// An act flying along a line, queued on a unit (0x1DC bytes; AddQueuedAction appends one, RunQueuedActions runs them).
struct QueuedAction
{
    std::unique_ptr<QueuedAction> next;  // +0x00
    ActState act;                        // +0x04 the flying act
    ActState impact;                     // +0x9C the act at the end (id 0: none; a negative id lands no hit)
    AfterimageTrail trail;               // +0x134 drawn while `trailed`
    i16 toX = 0;                         // +0x1CC
    i16 toY = 0;                         // +0x1CE 0: the line stays level
    i16 fromX = 0;                       // +0x1D0
    i16 fromY = 0;                       // +0x1D2 0: the line stays level
    u16 shakeFrames = 0;                 // +0x1D4 frames the act shakes before it flies
    i16 steps = 0;                       // +0x1D6 line steps per frame flown (1: a speeding flight)
    i16 flown = 0;                       // +0x1D8 the frames flown
    u8 done = 0;                         // +0x1DA removed at the end of RunQueuedActions
    u8 trailed = 0;                      // +0x1DB
};

// The placement of a unit (0x20 bytes, copied whole into the saved placement).
struct UnitPlacement
{
    std::array<u8, 0x14> unk00 = {};
    u16 actId = 0;  // +0x14 the act of the unit
    i16 x = 0;      // +0x16
    i16 y = 0;      // +0x18
    u16 unk1A = 0;
    i32 kind = 0;  // +0x1C 1: a party role
};

// The command block of a unit (0x28 bytes, cleared whole).
// The rectangle GetShadowRect fills (four dwords in the original).
struct UnitRect
{
    i32 x = 0;
    i32 y = 0;
    i32 width = 0;
    i32 height = 0;
};

struct UnitCommand
{
    // The bits of `actionFlags` (flag n is bit n).
    enum ActionFlag : u32
    {
        UnkAction1 = 1,
        UnkAction2 = 2,      // the swarm acts aim at 0, 0 (BattleRole::SwarmSpellFrame)
        DelaysFadeIn = 4,    // the unit fades in later (BattleUnit::DelayedFadeIn)
        StaysAcross = 0x10,  // the move starts at an edge across
        StaysDown = 0x20,    // the move starts at the top
        Restores = 0x80,     // the hit restores (HitDamage): it cures and heals the target
        DrainsLife = 0x100,
        DrainsSpirit = 0x200,
        DrainsStamina = 0x400,
        UnkAction800 = 0x800,
        UnkAction1000 = 0x1000,
        UnkAction2000 = 0x2000,
        UnkAction4000 = 0x4000,
    };

    // The bits of `flags`.
    enum Flag : u16
    {
        TargetsUnit = 1,  // a target command (ClearMoveAxes)
        UnkFlag2 = 2,
        UnkFlag8 = 8,      // with UnkFlag20: a party role walks home after it acts (ReturnHome)
        UnkFlag10 = 0x10,  // an enemy walks home after it acts
        UnkFlag20 = 0x20,  // the walk home waits for the act to follow up, then covers a sixth a frame
        AtTarget = 0x38,   // any of bits 3..5: DrawAction places the act on the target (inferred from DrawAction)
        FadesIn = 0x18,    // bits 3 and 4: a unit with unk2B00 fades in before it attacks (AttackFrame)
    };

    i32 unk00 = 0;
    u32 actionFlags = 0;  // +0x04 (0xD94)
    i32 unk08 = 0;
    u16 flags = 0;       // +0x0C (0xD9C)
    i16 value = 0;       // +0x0E (0xD9E) the value of the skill (SkillValue)
    u16 resistance = 0;  // +0x10 (0xDA0) the element 1..9 whose resistance scales the damage
    u16 followAct = 0;   // +0x12 (0xDA2) the follow-up action's act
    u16 castAct = 0;     // +0x14 (0xDA4)
    u16 unk16 = 0;       // +0x16 (0xDA6)
    u16 ailment = 0;     // +0x18 (0xDA8)
    u16 element = 0;     // +0x1A (0xDAA)
    u16 pose = 0;        // +0x1C (0xDAC) the pose of a defeat (its low byte)
    i16 unk1E = 0;       // +0x1E (0xDAE)
    u16 unk20 = 0;       // +0x20 (0xDB0)
    u16 target = 0;      // +0x22 (0xDB2)
    u8 unk24 = 0;        // +0x24 (0xDB4)
    std::array<u8, 3> unk25 = {};
};

class Database;
class Random;
class ErrorReporter;
struct CounterNode;
using CounterList = CounterNode;

class BattleUnit
{
public:
    // BattleUnit::marks bits.
    enum Mark : u16
    {
        UnkMark1 = 1,  // cleared when the guard text has risen
        UnkMark2 = 2,
        UnkMark4 = 4,       // the unit is drawn over a grey copy (IdleFrame)
        UnkMark8 = 8,       // cleared by ResetTurn
        MissedMark = 0x20,  // the ailment missed (SkillScript::MissedAilment)
        Held = 0x40,
        InvisibleMark = 0x80,
        UnkMark100 = 0x100,
        UnkMark200 = 0x200,  // set by ResetTurn
        UnkMark400 = 0x400,  // set when invisibility ends
        UnkMark800 = 0x800,
        GuardingMark = 0x1000,
        DefeatedMark = 0x2000,
        Unk4000 = 0x4000,      // the gauge stops (set elsewhere in the battle loop)
        UnkMark8000 = 0x8000,  // the damage of the unit is not raised
    };

    // BattleUnit::equipmentFlags set by the accessories (BattleRole::ApplyAccessory).
    enum EquipmentFlag : u32
    {
        Unk32 = 1,
        BladeReturn = 4,
        Unk36 = 8,
        SpiritDrain = 0x10,
        LifeDrain = 0x20,
        SpiritFan = 0x40,
    };

    // A unit in battle (0x2B28 bytes in the original; the base of BattleRole and BattleEnemy). Offsets of the original
    // are given for reference; fields not yet named keep their offset. BattleUnit::status bits, named by the texts the
    // buffs announce (AddBuff 0x0047BC50) and their effects.
    enum Status : u32
    {
        UnkStatus1 = 1,
        UnkStatus2 = 2,
        UnkStatus4 = 4,
        UnkStatus8 = 8,
        UnkStatus10 = 0x10,  // cleared with mark UnkMark100
        UnkStatus20 = 0x20,
        UnkStatus80 = 0x80,  // a struck unit evades 8 times in 10 (Strike)
        Slowed = 0x40,       // ailment 0x40: the gauge fills at half speed
        UnkStatus100 = 0x100,
        StatsScaled = 0x200,  // a party role's attributes are scaled (undone when it ends)
        UnkStatus1000 = 0x1000,
        DefenseDown = 0x100000,
        AttackDown = 0x1000000,
        Barrier = 0x4000000,
        SpeedDown = 0x8000000,
        AttackUp = 0x10000000,
        SpeedUp = 0x20000000,
        DefenseUp = 0x40000000,
        Invisible = 0x80000000,
    };

    static constexpr int SlotActs = 11;
    static constexpr i32 PartyRole = 1;

    // BattleUnit::commandA / commandB values (meanings not yet known).
    enum Command : u16
    {
        UnkCommand1 = 1,  // fades in with action 0x28
        UnkCommand2 = 2,
        UnkCommand3 = 3,
        UnkCommand6 = 6,
        UnkCommand13 = 0xD,
        UnkCommand14 = 0xE,
        UnkCommand15 = 0xF,
        UnkCommand16 = 0x16,
        UnkCommand18 = 0x18,
        UnkCommand1A = 0x1A,
        UnkCommand1B = 0x1B,
        UnkCommand1C = 0x1C,
        UnkCommand1F = 0x1F,
        UnkCommand22 = 0x22,
        UnkCommand23 = 0x23,
        UnkCommand24 = 0x24,
    };

    // BattleUnit::unk267C bits: the stages of an attack (AttackFrame).
    enum Stage : u32
    {
        ChargeFollowing = 0x800,  // the follow-up of a charge runs (ChargeAttackFrame)
        Recoiling = 0x4000,       // the recoil act runs (DrawRecoil)
        FollowingUp = 0x8000,     // the follow-up action runs in its slot act
    };

    // BattleUnit::unk2A86 bits.
    enum CommandFlag : u16
    {
        UnkCommandFlag200 = 0x200,  // GetOffset uses slot act UnkSlot8
        UnkCommandFlag400 = 0x400,  // set by the accessory commands that take command B
        UnkCommandFlag800 = 0x800,  // set by command 6
        UnkCommandFlag1000 = 0x1000,
        UnkCommandFlag2000 = 0x2000,
        UnkCommandFlag4000 = 0x4000,  // set by commands 14 and 15
        UnkCommandFlag8000 = 0x8000,  // set when the skill action is loaded
    };

    // BattleUnit::DrainKind values.
    enum Drain : u16
    {
        DrainedHitPoints = 1,
        DrainedSpirit = 2,
        DrainedStamina = 4,
        DrainedLife = 8,
    };

    // The pictures a hit pushes on its unit (PushTarget): the damage number, healing, a spirit or stamina gain.
    static constexpr u16 DamageTarget = 0x235E;
    static constexpr u16 HealTarget = 0x246F;
    static constexpr u16 SpiritGainTarget = 0x2367;
    static constexpr u16 StaminaGainTarget = 0x2366;
    static constexpr u16 ReviveBit = 0x8000;  // CureEffects: revives a defeated party role

    static constexpr std::size_t Trails = 8;
    static constexpr std::size_t Drains = 4;
    // HurtFrame: the shake (0x004A7E08) and the lift of the flash (0x004A7DF0, read one entry on at 0x004A7DF4, its
    // last entry then the bytes of the shake table: as in the original) by hurtStep.
    static constexpr std::array<u8, 6> StaggerShake = {30, 40, 45, 47, 49, 50};
    static constexpr std::array<i32, 6> StaggerLift = {14, -12, 10, -8, 6, -4};
    static constexpr std::array<i32, 6> StaggerRise = {-12, 10, -8, 6, -4, 0x2F2D281E};
    // RunQueuedActions: the y offsets of a shaking queued act by its frames left (0x004A7E10).
    static constexpr std::array<i16, 8> ShakeOffsets = {-1, -3, -3, -6, -6, -3, -3, 0};
    // DefeatHop: the heights of the hop by frame (0x004A7DB4).
    static constexpr std::array<i32, 15> HopHeights = {0, 4, 12, 24, 32, 24, 16, 8, 4, 0, -4, -12, -16, -20, -32};
    static constexpr int UnkSlot8 = 8;
    static constexpr int UnkSlot9 = 9;
    static constexpr u32 NoTarget = 0xFFFFFFFF;
    static constexpr u16 NoUnk29A2 = 0xFFFF;
    static constexpr u16 MaxTargets = 0x23;      // pushedTargets entries
    static constexpr u16 Healing = 0x8000;       // a damage amount that heals
    static constexpr i32 Opaque = 0xF;           // fadeLevel of a fully visible unit; the fade out counts down from it
    static constexpr i32 FadedIn = 0x10;         // brightLevel when the act ends the fade in early
    static constexpr i32 DelayedFadeIn = -0x20;  // brightLevel of a unit that fades in 0x20 frames later
    static constexpr u16 FlashPeak = 0x18;       // the tint pulse rises from FlashStart to FlashPeak and falls back
    static constexpr u16 FlashStart = 2;
    static constexpr std::size_t DamageDigits = 4;  // the decimal digits of a damage number
    static constexpr std::size_t GuardLetters = 4;  // the letters of the guard text
    static constexpr u16 DefaultFadeStep = 4;       // unk2A0A: the alpha step of a fade
    static constexpr u32 DefaultColorPair = 0x062B062B;
    static constexpr u32 OutlineColorPair = 0x07E007E0;  // the outline of DrawAction

    BattleUnit();
    ~BattleUnit();
    BattleUnit(const BattleUnit&) = delete;
    BattleUnit& operator=(const BattleUnit&) = delete;

    int SetTarget(int value);
    int GaugeLength(const BattleServices& services) const;
    i16 ResetGauge(const BattleServices& services);
    int RandomizeGauge(BattleServices& services);
    void GetDrawPosition(u16& x, u16& y) const;
    void GetOffset(u16& x, u16& y) const;
    i16 GetOrigin(u16& x, u16& y) const;
    void GetShadowRect(BattleServices& services, UnitRect& rect);
    i16 Move(i16 dx, i16 dy);
    i16 SetPosition(i16 x, i16 y);
    i16 GetPosition(u16& x, u16& y) const;
    const Picture* StepIdleAction(BattleServices& services);
    const void* SelectEntry(int second) const;
    int GetValueAAC() const;
    int GetValueAB4() const;
    u16 GetCommandA() const;
    u16 GetCommandB() const;
    u16 GetWord2A74() const;
    u16 GetWord29A2() const;
    int ClearIdleAction();
    int SetCommand(int command);
    void ClearFlag26B8();
    void RaiseFlag26B8();
    int Flag26B8() const;
    int PushTarget(u16 target);
    int StartDefeat(int kind);
    int ToggleAF8();
    int ResetTurn(BattleServices& services);
    i16 Retarget(i16 target);
    void IncreaseCounter2A76();
    u16 Counter2A76() const;
    void IncreaseHits();
    i16 DecreaseHits();
    void ClearWord29A2();
    void SetAA8();
    int GetAA8() const;
    int GetAE0() const;
    int SetAC0(int value);
    bool IsAE4Clear() const;
    u16 GetWordDA6() const;
    void MarkAC8();
    int DrawFadeOut(BattleServices& services);
    int DrawFadeIn(BattleServices& services, const BattleUnit& anchor);
    int SetHeld(int value);
    int SetAD4(int value);
    int DrawFlash(BattleServices& services);
    int RollEvade(BattleServices& services, u16 chance);
    int Mirrored() const;
    bool IsDefeated() const;
    int StatusBit13() const;
    bool IsAB8() const;
    int StepTargetCount(int increase);
    int PushTargetAlt(u16 value);
    int ResetAfterGuard();
    int GetGuard() const;
    int SetStatus(int bits);
    int ClearStatus(int bits);
    i16 SetWord2A0A(i16 value);
    int SetGuarding(int value);
    int ActFlag1() const;
    int ActFlag11() const;
    i16 ActSpeed() const;
    int ActFlag0() const;
    int ActFlag13() const;
    int ActFlag14() const;
    int SetAction(i16 value);
    bool IsGuarding() const;
    void SetVitals(i16 hitPoints, i16 spirit, i16 stamina);
    i16 SetAttributes(i16 attack, i16 defense, i16 agility);
    void SetLevel(i16 level);
    int SetB00(int value);
    int SetB04(int value);
    void Reset(BattleServices& services);
    void DrawGuardText(BattleServices& services);
    int AddAilment(BattleServices& services, const SkillScript& skill);
    int AddBuff(BattleServices& services, const SkillScript& skill);
    int HasEffectElement(i16 element) const;
    int HasEffectKind(i16 kind) const;
    int HasEffectBits(u16 bits) const;
    bool ApplyPendingDamage(BattleServices& services);
    void DrawEffects(BattleServices& services, int x, int y);
    void DrawEffectIcons(BattleServices& services, int x, int y);
    void ExpireEffects(BattleServices& services, int clearAll);
    SkillScript CommandSkill() const;
    i16 HitDamage(BattleServices& services, BattleUnit& target, i16& spiritDamage, i16& staminaDamage);
    void DrawAction(BattleServices& services, const BattleUnit& target, ActState& act);
    void AimAction(BattleServices& services, BattleUnit& target, ActState& act, i16 shiftX, i16 shiftY);
    int DrawAimed(BattleServices& services, BattleUnit& target, ActState& act, i16 shiftX, i16 shiftY);
    int RunQueuedActions(BattleServices& services, BattleUnit& target, i32 action);
    void AddQueuedAction(
        i32 id,
        i32 action,
        i16 fromX,
        i16 fromY,
        i16 toX,
        i16 toY,
        i16 steps,
        u16 shakeFrames,
        i32 impact
    );
    void TrailLastQueuedAction(i32 tintRed, i32 tintGreen, i32 tintBlue);
    void DrawOverlay(BattleServices& services, u16 frame, u16 picture, int x, int y, int offsetX, int offsetY);
    void DrawActionAt(BattleServices& services, i16 x, i16 y, ActState& act);
    int ReturnHome(BattleServices& services, const BattleUnit& target, i32 id, i32 action);
    int LoadEnemyCommand(Database& database, int choice, u32& act);
    int FadeInvisible(BattleServices& services, int mode);
    int StepAccessoryCounters(CounterList& list, u16 amount);
    int StepGauge(BattleServices& services, int boost);
    int FadeMainOut(BattleServices& services);
    int FadeMainIn(BattleServices& services);
    void DrawMainFaded(BattleServices& services, int tint);
    int Defense() const;
    int DefeatFrame(BattleServices& services);
    int EndDefeat(BattleServices& services);
    int EndParticleDefeat(BattleServices& services);
    void StartDefeatDraw(BattleServices& services, const Picture& picture);
    void DrawDefeatPiece(
        BattleServices& services,
        const Picture& picture,
        int left,
        int top,
        int right,
        int bottom,
        int x,
        int y,
        int height
    );
    int DefeatRising(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatBurst(BattleServices& services, const Picture& picture, int x, int y);
    int RunDefeatParticles(BattleServices& services);
    void DecodeForParticles(BattleServices& services, const Picture& picture);
    int DefeatSplitRows(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatSplitColumns(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatFill(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatFadeOut(BattleServices& services, const Picture& picture);
    int DefeatQuarters(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatWave(BattleServices& services, const Picture& picture);
    int DefeatSink(BattleServices& services, const Picture& picture);
    int DefeatZoom(BattleServices& services, const Picture& picture);
    int DefeatStrips(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatHalves(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatSlide(BattleServices& services, const Picture& picture);
    int DefeatHop(BattleServices& services, const Picture& picture, int x, int y);
    int DefeatProjected(BattleServices& services, const Picture& picture);
    int DefeatParticles(BattleServices& services, const Picture& picture, int x, int y);
    void WalkToTarget(BattleServices& services, const BattleUnit& target);
    void PlayIdleEffect(BattleServices& services);
    void PlayIdleFollowUp(BattleServices& services);
    void TickEffects(BattleServices& services);
    int CanUseAccessory(u16 kind);
    void StartDrainEffect(BattleServices& services, int slot, BattleUnit& source);
    int DrainEffectFrame(BattleServices& services, int slot);
    int CatchRoll(BattleServices& services, u8 level);
    int HurtFrame(BattleServices& services);
    void IdleFrame(BattleServices& services, int paused);
    void MoveIdle(BattleServices& services);
    void DrawTrail(BattleServices& services, AfterimageTrail& trail, u16 frame, u16 picture, u32 mode, i16 x, i16 y);
    int DrawRecoil(BattleServices& services, ActState& act, i16 y);
    int FlyAct(
        BattleServices& services,
        const BattleUnit& target,
        ActState& act,
        int slot,
        i32 id,
        int fromX,
        int fromY,
        int steps,
        int backwards
    );
    void CureEffects(BattleServices& services, u16 bits);
    int Damage(BattleServices& services, int hitPoints, i16 spirit, i16 stamina);
    int EffectFlag2() const;
    int EffectFlag3() const;
    void SetDefeatPose(u8 pose);
    void SetDefeatPoseFixed(u8 pose);
    void SetDamageDigits(BattleServices& services, int amount);
    int AdvanceGauge(const BattleServices& services, int boost);
    ObjectRecord* Data() const;
    bool MarkBit11Clear() const;
    int IsHeldOrHolding() const;
    u16 ActId() const;
    u16 DrainKind() const;
    int StatusBit5Or8() const;
    int SetValueAAC(int value);
    int SetFacingIfAble(int value);
    void SetNudge(u16 nudge, u16 saved);
    bool CommandTarget(u16& target) const;
    int MarkBit1() const;
    bool MarkBit8Clear() const;
    bool HasBlock38() const;
    int Word2A86Bit13() const;
    int Word2A86Bit12() const;
    bool RestartMainAct(i16 value, int force);
    int CommandStatusBit() const;
    void CommandValues(u16& value, u16& turns) const;
    bool IsSmallMachine() const;
    void Unk62And8A(u16& first, u16& second) const;
    u16 AccessoryValue(u16 kind) const;
    i16 AccessoryProficiency(u16 kind, CounterList& proficiency);
    void SetWords72(i16 first, i16 second, i16 third, i16 fourth);
    void SetWords66(i16 a, i16 b, i16 c, i16 d, i16 e, i16 f);
    void SetResistances(std::span<const i16, 9> values);
    bool ShowsPicture(i16 picture) const;
    void LoadSkillAction(Database& database, u16 id, u16& act);
    u16 PrepareCommand(Database& database);
    bool ChooseHeal(BattleServices& services);
    void ClearMoveAxes(u16& x, u16& y) const;
    void LoadCommand(Database& database, u16 id);
    int SetFacing(int value);
    void SetDisplayName(std::span<const char> name);
    void GetStamina(u16& maximum, u16& value) const;
    void GetSpirit(u16& maximum, u16& value) const;
    void GetHitPoints(i32& value, i32& maximum) const;

    // swd32.exe 0x004A7D8C: the height of a popping letter or digit by phase.
    static constexpr std::array<i32, 10> PopOffsets = {0, 12, 18, 12, 8, 4, 0, 2, 0, 0};

private:
    void MirrorRecoil(u32 actFlags, bool forced);
    void Announce(BattleServices& services, std::u8string_view format, int flags);

public:
    // +0x00..+0x0F: what the unit is.
    RoleRecord* battleRecord =
        nullptr;                       // +0x00 the battle copy of a party role's record (+0x26, +0x28: attack, defence)
    RoleRecord* roleRecord = nullptr;  // +0x04 the role's party record
    i32* roleBlock = nullptr;          // +0x08 the role's 0x60-byte block at 0x004ACF50 ([0]: the fury gauge, inferred)
    ObjectRecord* data = nullptr;      // +0x0C an enemy's mon.dat object (usually `object`)
    std::unique_ptr<ObjectRecord> ownedData;  // a party role's block at +0x0C (freed by Reset)
    ObjectRecord object;                      // +0x10 (its description at +0xB0)
    ObjectRecord weapon;                      // +0xB4 a party role's weapon (equipment slot 0)
    std::array<ObjectRecord, 2> accessories;  // +0x158 equipment slots 7 and 8

    ActState mainAct;                           // +0x2A0
    ActState standAct;                          // +0x338
    ActState castAct;                           // +0x3D0
    ActState recoilAct;                         // +0x468
    ActState attackAct;                         // +0x500
    ActState unk598;                            // +0x598
    std::array<ActState, SlotActs> slotActs;    // +0x630
    ActState stepAct;                           // +0xCB8
    UnitPlacement placement;                    // +0xD50
    UnitPlacement savedPlacement;               // +0xD70
    UnitCommand command;                        // +0xD90
    ProjectedPicture projected;                 // +0xDB8 the picture of defeat pose 51 (DefeatProjected)
    std::vector<u16> projectedPixels;           // its raw pixels
    LineWalker flight;                          // +0xDF4 the line of a flying act (FlyAct)
    ParticleEffect absorb;                      // +0xE14 a picture breaking up into the unit (StartAbsorb)
    std::vector<u16> absorbPixels;              // the raw pixels of absorb (decoded by 0x004019A0, freed by 0x004885A0)
    std::array<ParticleEffect, Drains> drains;  // +0xE6C the drain effects (StartDrainEffect)
    std::array<std::vector<u16>, Drains> drainPixels;  // their raw pixels
    std::array<AfterimageTrail, Trails> trails;        // +0xFCC the afterimages of the unit's acts
    std::unique_ptr<QueuedAction> actions;             // +0x236C
    const Picture* shadowPicture = nullptr;            // +0x2548
    const Picture* picture = nullptr;                  // +0x254C the current picture
    const Picture* unk2550 = nullptr;                  // +0x2550
    const Picture* unk2554 = nullptr;                  // +0x2554
    const Picture* absorbSource = nullptr;             // +0x255C the picture StartAbsorb breaks up
    std::span<const char> displayName;      // +0x2560 the name shown in the messages (read up to its NUL when used)
    EffectNode effects;                     // +0x2584 (the head is a node: +0x2588 its act, +0x2620 its bits)
    std::array<char, 0x30> skillName = {};  // +0x2630 the name of the skill used
    std::array<i32, 8> flightSteps = {};    // +0x2640 the steps per frame of FlyAct by its slot (callers use 0)
    i32 unk2660 = 0;
    i32 unk2664 = 0;
    i32 fadeLevel = Opaque;  // +0x2668 (also the dash offset of ChargeAttackFrame)
    i32 brightLevel = 1;     // +0x266C
    i32 unk2670 = 0;         // +0x2670 the x offset of the stand pose while walking home (ReturnHome)
    i32 unk2674 = 0;
    i32 unk2678 = 0;
    u32 unk267C = 0;     // +0x267C BattleUnit::Stage bits
    i32 throwSteps = 0;  // +0x2680 the frames of the thrown act so far (ThrowAct)
    i32 unk2684 = 0;
    u32 glowFlags = 0;    // +0x2688 the draw mode of the glow act
    i32 glowOffsetX = 0;  // +0x268C
    i32 unk2690 = 0;
    u32 drawFlags = 0;   // +0x2694 bit 0: mirrored
    i32 unk2698 = 0;     // +0x2698 the x offset of the overlay (IdleFrame)
    u32 frameFlags = 0;  // +0x26A0 flag 1 toggled for mirrored frames
    u32 unk26A4 = 0;
    i32 unk26A8 = 0;
    i32 unk26AC = 0;
    i32 damageCarry = 0;               // +0x26B0
    i32 hitPointsOverride = 0;         // +0x26B4 an enemy's current hit points when set
    u32 unk26B8 = 0;                   // +0x26B8 bit 31
    u32 colorPair = DefaultColorPair;  // +0x26BC
    u32 status = 0;                    // +0x26C0 status bits (2: stone, 12: frozen, ...)
    u32 equipmentFlags = 0;            // +0x26C8 bits 0, 2..6 from the equipment
    u32 unk26CC = 0;
    u16 marks = 0;  // +0x26D0 (bit 1, 0x20, 0x40: held, 0x100, bit 13)
    i16 unk26D2 = 0;
    u16 unk26D4 = 0;
    u16 nudge = 0;                                                            // +0x26D6
    u16 drawAtSaved = 0;                                                      // +0x26D8
    u16 unk26DC = 0;                                                          // +0x26DC
    std::array<u16, MaxTargets> damageDelays = {};                            // +0x26DE
    std::array<std::array<i16, DamageDigits>, MaxTargets> damageDigits = {};  // +0x2724 the four digits per target slot
    std::array<std::array<i16, DamageDigits>, MaxTargets> unk283C = {};       // +0x283C
    std::array<i16, GuardLetters> guardPhases = {};  // +0x2884 the pop phases of the four letters of the guard text
    i16 slotTint = 0;    // +0x2954 the tint of a waiting slot act, darkening to its end (AttackFrame)
    i16 recoilTint = 0;  // +0x2956 the tint of a waiting recoil act, darkening to its end (DrawRecoil)
    i16 glowLevel = 0;   // +0x2958 the brightness of a glow act (GlowAct)
    std::array<u16, MaxTargets> unk295A = {};  // +0x295A
    i16 guardRise = 0;                         // +0x29A0
    u16 unk29A2 = NoUnk29A2;
    u16 unk29A4 = 0;
    u16 unk29A6 = 0;
    u16 unk29A8 = 0;
    u16 unk29AA = 0;
    u16 actAnchorX = 0;  // +0x29AC the act's anchor x, from the picture's right edge when mirrored (DrawAction)
    u16 aimAnchorX = 0;  // +0x29AE the same for AimAction
    u16 overlayX = 0;    // +0x29B0 the x of the overlay picture from the act (DrawAction)
    u16 originX = 0;     // +0x29B2
    u16 actOffsetX = 0;  // +0x29B4 the act's x offset, mirrored like the picture (DrawAction)
    u16 aimOffsetX = 0;  // +0x29B6 the act's x offset of AimAction, mirrored like the picture
    u16 actX = 0;        // +0x29B8 where DrawAction draws the act
    u16 actY = 0;        // +0x29BA
    u16 aimX = 0;        // +0x29BC where AimAction places the act
    u16 aimY = 0;        // +0x29BE
    std::array<u16, MaxTargets> pushedTargets = {};  // +0x29C4
    u16 unk2A0A = DefaultFadeStep;
    u16 actId = 0;   // +0x2A0C the unit's act
    u16 action = 0;  // +0x2A0E
    u16 gauge = 0;   // +0x2A12 the action gauge
    u16 unk2A14 = 0;
    std::array<u32, 4> targets = {NoTarget, NoTarget, NoTarget, NoTarget};  // +0x2A56
    u16 glowPhase = 0;  // +0x2A66 0: brightening, 1: dimming, 2: over (GlowAct)
    u16 unk2A68 = FlashStart;
    u16 unk2A6A = FlashPeak;
    u16 commandA = 0;  // +0x2A6C
    u16 unk2A6E = 0;
    u16 commandB = 0;  // +0x2A70 commands 2, 3, 6, 13, 14 and 15
    u16 unk2A72 = 0;
    u16 unk2A74 = 0;
    u16 hurtStep = 0;  // +0x2A78 the step of the stagger (HurtFrame)
    u16 unk2A76 = 0;
    u16 attackStage = 0;  // +0x2A78 1..5
    u16 unk2A7A = 0;
    u16 targetCount = 0;    // +0x2A7C
    u16 damageSlot = 0;     // +0x2A7E
    u16 actionCount = 0;    // +0x2A80
    u16 queuedActions = 0;  // +0x2A80 the acts AddQueuedAction appended
    u16 landedActions = 0;  // +0x2A82 the queued acts that reached their end
    u16 pendingDamage = 0;  // +0x2A84 bit 15: healing
    u16 unk2A86 = 0;        // +0x2A86 (high byte at +0x2A87)
    u16 unk2A88 = 0;        // +0x2A88 the rest action of the frame (IdleFrame)
    u16 unk2A8A = 0;
    u16 unk2A8C = 0;
    u16 unk2A8E = 0;
    u16 barrierTurns = 0;  // +0x2A90
    u8 unk2A92 = 0;
    u8 unk2A93 = 0;
    u8 defeatPose = 0;  // +0x2A94 the pose of the defeat (a fixed pose overrides it)
    u8 fixedPose = 0;   // +0x2A95
    std::array<u8, 4> unk2A97 = {};
    u8 extraAttacks = 0;  // +0x2A9B the attacks the unit repeats (set by skill 0x15)
    u8 healChoice = 0;    // +0x2A9C
    u8 healPercent = 0;   // +0x2A9E 1: unk29A6..unk29AA are percentages of the target's maxima
    i32 kind = 0;         // +0x2AA0 1: a party role
    i32 unk2AA4 = 0;
    i32 unk2AA8 = 0;
    i32 unk2AAC = 0;
    i32 unk2AB0 = 0;
    i32 unk2AB4 = 0;
    i32 defeated = 0;  // +0x2AB8
    i32 dying = 0;     // +0x2ABC 1 while the defeat runs (DefeatFrame)
    i32 outline = 0;   // +0x2AC0
    i32 charging = 0;  // +0x2AC4 1: ChargeAttackFrame is in its charge (also set when a stagger ends)
    i32 unk2AC8 = 0;
    i32 unk2ACC = 0;
    i32 held = 0;  // +0x2AD0
    i32 unk2AD4 = 0;
    i32 unk2AD8 = 0;
    i32 unk2ADC = 0;
    i32 unk2AE0 = 0;
    i32 target = 0;       // +0x2AE4
    i32 extraNumber = 0;  // +0x2AE8
    i32 unk2AEC = 0;
    i32 guarding = 0;  // +0x2AF0
    i32 unk2AF4 = 0;
    i32 unk2AF8 = 0;
    i32 absorbing = 0;  // +0x2AFC 1 while an absorb runs (AbsorbFrame)
    i32 unk2B00 = 0;
    i32 unk2B04 = 0;
    i32 facing = 0;  // +0x2B08
    i32 unk2B0C = 0;
    i32 fadedIn = 0;  // +0x2B1C a forced unit has faded in for its attack (AttackFrame)
    i32 unk2B20 = 0;  // +0x2B20 1: the facing turns back when the gauge fills (StepGauge)
    i32 unk2B10 = 0;
    i32 unk2B14 = 0;
    i32 goldGuard = 0;  // +0x2B18
    i32 name = 0;       // +0x2B20
};

int ScaleByTenths(i8 tenths, i16& value);
bool RollChance(Random& random, int percent);

}  // namespace swd3
