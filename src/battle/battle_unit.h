#pragma once

#include <cstdint>

#include "core/mem.h"

namespace swd3 {

// Battle participant (0x2B28 bytes; G.battleUnit at 0x00521598, base of BattleEnemy and BattleHero). Fields are
// addressed by offset (at<T>) until they are named; the action states at +0x2A0, +0x338, +0x3D0, +0x468, +0x500,
// +0x630 (8) and +0xCB8 are ActState.
class BattleUnit {
public:
    BattleUnit();
    ~BattleUnit();
    int SetTarget(int value);
    int GaugeLength();
    std::int16_t ResetGauge();
    int RandomizeGauge();
    std::int16_t GetDrawPosition(std::uint16_t* x, std::uint16_t* y);
    std::uint16_t* GetOffset(std::uint16_t* x, std::uint16_t* y);
    std::int16_t GetOrigin(std::uint16_t* x, std::uint16_t* y);
    std::int16_t GetShadowRect(std::int32_t* rect);
    std::int16_t Move(std::int16_t dx, std::int16_t dy);
    std::int16_t SetPosition(std::int16_t x, std::int16_t y);
    std::int16_t GetPosition(std::uint16_t* x, std::uint16_t* y);
    int StepIdleAction();
    int SelectEntry(int second);
    int GetValueAAC();
    int GetValueAB4();
    std::uint16_t GetWord2A6C();
    std::uint16_t GetWord2A70();
    std::uint16_t GetWord2A74();
    std::uint16_t GetWord29A2();
    int ClearIdleAction();
    int SetCommand(int command);
    void ClearFlag26B8();
    void RaiseFlag26B8();
    int Flag26B8();
    int PushTarget(std::uint16_t target);
    int StartDefeat(int kind);
    int ToggleAF8();
    int ResetTurn();
    std::int16_t Retarget(std::int16_t target);
    void IncreaseCounter2A76();
    std::uint16_t Counter2A76();
    void IncreaseHits();
    std::int16_t DecreaseHits();
    void ClearWord29A2();
    void SetAA8();
    int GetAA8();
    int GetAE0();
    int SetAC0(int value);
    bool IsAE4Clear();
    std::uint16_t GetWordDA6();
    void MarkAC8();
    int DrawFadeOut();
    int DrawFadeIn(BattleUnit* anchor);
    int SetAD0(int value);
    int SetAD4(int value);
    int DrawFlash();
    int RollEvade(std::uint16_t chance);
    int Mirrored();
    bool IsDefeated();
    int StatusBit13();
    bool IsAB8();
    int StepTargetCount(int increase);
    int PushTargetAlt(std::uint16_t target);
    int ResetAfterGuard();
    int GetGuard();
    int SetStatus(int bits);
    int ClearStatus(int bits);
    std::int16_t SetWord2A0A(std::int16_t value);
    int SetAF0(int value);
    int ActFlag1();
    int ActFlag11();
    std::int16_t ActSpeed();
    int ActFlag0();
    int ActFlag13();
    int ActFlag14();
    int SetAction2A0E(std::int16_t action);
    bool IsGuarding();
    void SetStats(std::int16_t first, std::int16_t second, std::int16_t third);
    std::int16_t SetLimits(std::int16_t first, std::int16_t second, std::int16_t third);
    void SetEnemyWord54(std::int16_t value);
    int SetB00(int value);
    int SetB04(int value);
    int HasEffectMessage(std::int16_t message);
    int HasEffectKind(std::int16_t kind);
    int HasEffectBits(std::uint16_t bits);
    int DrawEffects(int x, int y);
    int DrawEffectIcons(int x, int y);
    int Damage(int hp, std::int16_t mp, std::int16_t sp);
    int EffectFlag2();
    int EffectFlag3();
    std::uint8_t SetDefeatPose(std::uint8_t pose);
    std::uint8_t SetDefeatPoseFixed(std::uint8_t pose);
    void SetDamageDigits(int amount);
    bool ApplyPendingDamage();
    void ExpireEffects(int clearAll);
    int DrawDarken(int unused);
    int DrawBrighten();
    int DrawPhase(int mode);
    int SetB08(int value);
    std::int16_t GetWord2A0C();
    int IsHeld();
    int DrawDamageNumbers();
    int DrawAttack();
    int DrawGuardText();
    int AddAilment(const std::uint8_t* skill);
    int AddBuff(const std::uint8_t* skill);
    std::uint8_t* QueueAction(int kind, int target, std::int16_t x, std::int16_t y, std::int16_t width,
                              std::int16_t height, std::int16_t delay, std::int16_t speed, int data);
    std::uint8_t* FinishQueuedAction(int first, int second, int third);
    int GetData0C();
    int DrawCastAction();
    int DrawFollowUpAction();
    std::uint32_t Defense();
    std::uint32_t Mark100Clear();
    int Mark2();
    void ResetActionPosition(std::uint16_t* x, std::uint16_t* y);
    int EndAction(std::int16_t next, int force);
    std::int16_t SetWords26D6(std::int16_t first, std::int16_t second);
    void DrawAfterimages(std::uint8_t* trail, int id, std::int16_t frame, std::uint32_t flags, std::int16_t x,
                         std::int16_t y);
    void AddGauge(int amount);
    bool GaugeIsFull();
    std::uint32_t GuardKind();
    bool IsActionFrame(std::int16_t frame);
    std::uint32_t RecordValue(int id);
    int AddRecordProgress(std::uint8_t* list, int unused, std::uint16_t amount);
    int DrawOverlay(int id, int frame, int x, int y, int offset, int offsetY);
    int Status20Or100();
    int GetActionTarget(std::uint16_t* target);
    void GetHeroWords0E08(std::uint16_t* first, std::uint16_t* second);
    void GetHeroWords0C06(std::uint16_t* first, std::uint16_t* second);
    int SetAAC(int value);
    int SetName(int name);
    int SetFacing(int facing);
    void GetHitPoints(std::int32_t* current, std::int32_t* maximum);
    void DrawState(BattleUnit* target, struct ActState* state);
    int DrawExtraState(struct ActState* state, std::int16_t offsetY);
    void DrawStateAt(std::int16_t x, std::int16_t y, struct ActState* state);

    template <typename T>
    T& at(std::intptr_t offset) {
        return mem::ref<T>(m_data, offset);
    }
    std::uint8_t* raw(std::intptr_t offset) { return m_data + offset; }

    std::uint8_t m_data[0x2B28];
};

static_assert(sizeof(BattleUnit) == 0x2B28);

int FreeUnitExtra(std::uint8_t* extra);
int ScaleByTenths(std::int8_t tenths, std::int16_t* value);

}  // namespace swd3
