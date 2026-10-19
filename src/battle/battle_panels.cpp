#include <array>
#include <cstring>
#include <string_view>

#include "crt/msvc.h"
#include "draw/paint_record.h"
#include "game/game.h"
#include "ui/ui_draw.h"

namespace swd3
{

namespace
{

// The columns of the four role panels by party role: the gauges, the portrait and the name.
constexpr std::array<int, Party::Roles> kGaugeX = {0x188, 0xF6, 0x64, 0x21A};  // swd32.exe 0x004A7598
constexpr std::array<int, Party::Roles> kNameX = {0x17E, 0xEC, 0x5A, 0x210};   // swd32.exe 0x004A75B8

// The list of units with unk2B00 set, top right (top left with the sides swapped).
constexpr int kListX = 0x1C2;
constexpr int kMirroredListX = 0xA;
constexpr int kListTop = 0xA;
constexpr int kListStep = 0x1C;
constexpr int kListWidth = 0xAA;
constexpr int kListHeight = 0x14;
constexpr int kListTextX = 5;
constexpr int kListTextY = 2;
constexpr int kListBarX = 0x64;
constexpr int kListBarY = 8;
constexpr int kListGaugeY = 0xC;
constexpr int kListGaugeHeight = 3;
constexpr int kListGaugeBlue = 0x18;
constexpr int kListGlowUp = 4;
constexpr int kListGlowHeight = 0x1C;
constexpr i8 kListGlowStep = 3;
constexpr int kListGlowTop = 0x1C;

constexpr double kBarPixels = 56.0;  // swd32.exe 0x00499CF4: the pixels of a full panel gauge

// The panel of a role.
constexpr int kHighlightLeft = 4;
constexpr int kHighlightTop = 0x18A;
constexpr int kHighlightWidth = 0x7C;
constexpr int kHighlightHeight = 0x4C;
constexpr int kGlowFrames = 3;
constexpr i8 kGlowTop = 8;
constexpr u8 kGlowFalling = 0x80;
constexpr u8 kGlowMask = 0x7F;
constexpr int kNameY = 0x18A;
constexpr int kNameGauge = 0x234F;
constexpr int kNameStyle = 0x10;
constexpr Pixel kNameColor = 0xFFFF;
constexpr Pixel kDefeatedColor = 0xF000;
constexpr Pixel kFlashColor = 0xF3F0;
constexpr int kFlashPeriod = 4;
constexpr int kPortraitY = 0x1CC;
constexpr int kIconsY = 0x1CE;
constexpr int kBlockGauge = 0x2352;
constexpr int kBlockY = 0x1D2;
constexpr i32 kArrowBlock = 0x38;  // the block value under which the arrow is shown
constexpr int kFrameLeft = 8;
constexpr int kFrameIndex = 3;
constexpr int kNumberX = 0x26;
constexpr int kHitPointPicture = 0x2350;
constexpr int kHitPointFrameY = 0x1A3;
constexpr int kHitPointGaugeY = 0x1A6;
constexpr int kHitPointNumberY = 0x19C;
constexpr int kSpiritPicture = 0x235F;
constexpr int kSpiritFrameY = 0x1B4;
constexpr int kSpiritGaugeY = 0x1B7;
constexpr int kSpiritNumberY = 0x1AD;
constexpr int kStaminaPicture = 0x241F;
constexpr int kStaminaFrameY = 0x1C5;
constexpr int kStaminaGaugeY = 0x1C8;
constexpr int kStaminaNumberY = 0x1BE;
constexpr int kNormalDigits = 0x2354;
constexpr int kLowDigits = 0x2355;       // the hit points at most a third of the maximum
constexpr int kCriticalDigits = 0x2356;  // at most kCriticalHitPoints
constexpr i32 kCriticalHitPoints = 0xA;
constexpr i32 kLowShare = 3;

// The gauges move a step a frame towards their value: a large step when far.
constexpr i32 kNearGauge = 0xA;
constexpr i32 kGaugeUpNear = 1;
constexpr i32 kGaugeUpFar = 3;
constexpr i32 kGaugeDownNear = 1;
constexpr i32 kGaugeDownFar = 3;
constexpr i32 kBlockDownNear = 2;
constexpr i32 kBlockDownFar = 5;
constexpr i32 kRollingBit = 0x8000000;  // the original tests bit 27, not the sign, before negating a change
constexpr i32 kRollingShare = 0xA;      // a changed number rolls in a tenth of the change a frame

// The "自動攻擊" box of the automatic battle.
constexpr std::u8string_view kAutoBattle = u8"自動攻擊";  // swd32.exe 0x004A7814
constexpr int kAutoTargetX = 0x44;
constexpr int kAutoSlide = 3;
constexpr int kAutoLeft = 0x3A;
constexpr int kAutoY = 0x162;
constexpr int kAutoWidth = 0x46;
constexpr int kAutoHeight = 0x18;

// A NUL-terminated name kept in its array.
template <std::size_t Size>
std::string_view NameOf(const std::array<char, Size>& name)
{
    return std::string_view(name.data(), strnlen(name.data(), name.size()));
}

// The change of a number shown rolling: the remaining change shrinks by `step` a frame. `floor` is where a falling
// change stops (stamina stops at 1). A value at or below 0 clears it.
template <typename Value>
void RollNumber(Value& value, i32& last, i32& rolling, i32& step, i32 floor)
{
    if (last != value)
    {
        i32 change = last - value;
        rolling = change;

        if ((change & kRollingBit) != 0)
        {
            change = ~change;
        }

        step = change / kRollingShare + 1;
        last = value;
    }

    if (rolling > 0)
    {
        rolling -= step;

        if (rolling < floor)
        {
            rolling = 0;
        }

        if (value <= 0)
        {
            value = 0;
            rolling = 0;
            last = 0;
        }
    }
    else if (rolling < 0)
    {
        rolling += step;

        if (rolling > 0)
        {
            rolling = 0;
        }
    }
}

}  // namespace

// A panel gauge: drawn at its shown value, which moves towards `target`; when it has arrived the gauge takes the new
// value `next()`.
template <typename Next>
void Game::StepPanelGauge(i32& shown, i32& target, Next next, int picture, int x, int y, i32 downNear, i32 downFar)
{
    if (shown == target)
    {
        target = next();
        this->battleHud.DrawGauge(picture, x, y, shown);
    }
    else if (shown < target)
    {
        this->battleHud.DrawGaugeAlt(picture, x, y, shown);
        shown += target - shown <= kNearGauge ? kGaugeUpNear : kGaugeUpFar;
    }
    else
    {
        this->battleHud.DrawGaugeAlt(picture, x, y, shown);
        shown -= shown - target <= kNearGauge ? downNear : downFar;
    }
}

// swd32.exe 0x00459D10: the battle panels. The units with unk2B00 set are listed in boxes (name, hit points, gauge;
// the last one glows while listGlow is on). Every other role in battle that is not guarding gets its panel: the
// highlight of the current role, the name or its gauge, the portrait and effect icons, the block gauge (with the
// arrow), and the hit point, spirit and stamina gauges and numbers moving towards their values. With the automatic
// battle on, its box slides in.
void Game::DrawRolePanels()
{
    BattleState& battle = this->battleState;
    TextFont& font = this->mediumFont;
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    int listY = kListTop;

    for (int index = 0; index < static_cast<int>(battle.roleCount); ++index)
    {
        BattleRole& unit = this->battleRoles[static_cast<std::size_t>(index)];
        const int x = battle.mirrored == 1 ? kMirroredListX : kListX;

        if (unit.unk2B00 != 1)
        {
            continue;
        }

        const std::string_view name = NameOf(unit.Data()->data.name);
        this->battleMessages
            .DrawMessageBox(x, listY, kListWidth, kListHeight, x + kListTextX, listY + kListTextY, name);
        i32 hitPoints = 0;
        i32 maximum = 0;
        unit.GetHitPoints(hitPoints, maximum);
        const auto bar =
            static_cast<int>(crt::Ftol(static_cast<double>(hitPoints) / static_cast<double>(maximum) * kBarPixels));
        this->battleHud.DrawBar(kHitPointPicture, 1, x + kListBarX, listY + kListBarY, bar);
        const auto gauge = static_cast<u16>(unit.GaugeLength(this->battleServices));

        if (gauge != 0)
        {
            const u32 color = this->painter.MakeColor(0, 0, kListGaugeBlue);
            this->battleHud.FillColor(x + kListBarX, listY + kListGaugeY, gauge, kListGaugeHeight, color);
        }

        if (index == static_cast<int>(battle.roleCount) - 1 && (static_cast<u8>(battle.listGlow) & kGlowFalling) != 0)
        {
            battle.listGlow = static_cast<i8>(battle.listGlow + kListGlowStep);

            if ((static_cast<u8>(battle.listGlow) & kGlowMask) > kListGlowTop)
            {
                battle.listGlow = 0;
            }

            const int level = static_cast<u8>(battle.listGlow) & kGlowMask;
            FillBox(
                this->painter,
                x,
                listY - kListGlowUp,
                kListWidth,
                kListGlowHeight,
                level,
                0,
                0,
                FillMode::Gradient
            );
        }

        listY += kListStep;
    }

    for (int index = 0; index < static_cast<int>(battle.roleCount); ++index)
    {
        const auto slot = static_cast<std::size_t>(index);
        BattleRole& unit = this->battleRoles[slot];

        if (unit.unk2B00 == 1 || unit.unk2B04 == 1 || unit.IsGuarding())
        {
            continue;
        }

        const auto partyRole = static_cast<std::size_t>(battle.partyRoles[slot]);
        const int gaugeX = kGaugeX[partyRole];
        const int portraitX = BattleState::PortraitX[partyRole];
        const bool current = battle.currentUnit - BattleState::FirstRoleUnit == index;

        if (current)
        {
            const int level = battle.panelGlow & kGlowMask;
            FillBox(
                this->painter,
                BattleState::PortraitX[static_cast<std::size_t>(battle.partyRoles[static_cast<std::size_t>(index)])] -
                    kHighlightLeft,
                kHighlightTop,
                kHighlightWidth,
                kHighlightHeight,
                level,
                level,
                level,
                FillMode::Tint
            );

            if (++battle.glowFrames == kGlowFrames)
            {
                battle.glowFrames = 0;
                u8 glow = battle.panelGlow;

                if ((glow & kGlowFalling) != 0)
                {
                    --glow;

                    if ((glow & kGlowMask) == 0)
                    {
                        glow = 0;
                    }
                }

                if ((glow & kGlowFalling) == 0)
                {
                    ++glow;

                    if (static_cast<i8>(glow) > kGlowTop)
                    {
                        glow |= kGlowFalling;
                    }
                }

                battle.panelGlow = glow;
            }
        }

        const int nameX = kNameX[partyRole];
        const std::string_view name = NameOf(this->party.names[partyRole]);

        if (!unit.IsDefeated())
        {
            const auto gauge = static_cast<u16>(unit.GaugeLength(this->battleServices));

            if (unit.unk2AB0 == 0)
            {
                this->battleHud.DrawGaugeWide(kNameGauge, nameX, kNameY, gauge);
            }
        }

        if (unit.IsDefeated())
        {
            font.DrawText(this->drawSurface, nameX, kNameY, name, kDefeatedColor, kNameStyle);
        }

        if (unit.unk2AB0 == 1)
        {
            if (current)
            {
                font.DrawText(this->drawSurface, nameX, kNameY, name, kNameColor, kNameStyle);
                const Pixel color = battle.nameFlashes[slot] % kFlashPeriod != 0 ? kFlashColor : kNameColor;
                font.DrawText(this->drawSurface, nameX, kNameY, name, color, kNameStyle);
                ++battle.nameFlashes[slot];
            }
            else
            {
                font.DrawText(this->drawSurface, nameX, kNameY, name, kNameColor, kNameStyle);
            }
        }

        this->battleHud.DrawPortrait(static_cast<u16>(partyRole), portraitX, kPortraitY, unit.IsDefeated() ? 1 : 0);
        unit.DrawEffectIcons(this->battleServices, gaugeX, kIconsY);
        this->StepPanelGauge(
            battle.blockValuesShown[slot],
            battle.blockValues[slot],
            [&unit]
            {
                return *unit.roleBlock;
            },
            kBlockGauge,
            portraitX,
            kBlockY,
            kBlockDownNear,
            kBlockDownFar
        );

        if (*unit.roleBlock == kArrowBlock && !unit.IsDefeated())
        {
            this->battleHud.DrawArrow(portraitX, kBlockY, index);
        }

        i32 hitPoints = 0;
        i32 maximum = 0;
        unit.GetHitPoints(hitPoints, maximum);
        float ratio = static_cast<float>(static_cast<double>(hitPoints) / static_cast<double>(maximum) * kBarPixels);
        RollNumber(hitPoints, battle.hitPoints[slot], battle.hitPointsRolling[slot], battle.hitPointSteps[slot], 0);
        this->battleHud.DrawPictureFrame(kHitPointPicture, kFrameIndex, gaugeX - kFrameLeft, kHitPointFrameY);
        this->StepPanelGauge(
            battle.hitPointBarsShown[slot],
            battle.hitPointBars[slot],
            [ratio]
            {
                return static_cast<i32>(crt::Ftol(ratio));
            },
            kHitPointPicture,
            gaugeX,
            kHitPointGaugeY,
            kGaugeDownNear,
            kGaugeDownFar
        );
        const int digits = hitPoints > kCriticalHitPoints
                               ? (hitPoints > maximum / kLowShare ? kNormalDigits : kLowDigits)
                               : kCriticalDigits;
        this->battleHud
            .DrawSmallNumber(digits, battle.hitPointsRolling[slot] + hitPoints, gaugeX + kNumberX, kHitPointNumberY);

        u16 spiritMaximum = 0;
        u16 spiritValue = 0;
        unit.GetSpirit(spiritMaximum, spiritValue);
        i16 spirit = static_cast<i16>(spiritValue);
        ratio = static_cast<float>(
            static_cast<double>(spirit) / static_cast<double>(static_cast<i16>(spiritMaximum)) * kBarPixels
        );
        RollNumber(spirit, battle.spirit[slot], battle.spiritRolling[slot], battle.spiritSteps[slot], 0);
        this->battleHud.DrawPictureFrame(kSpiritPicture, kFrameIndex, gaugeX - kFrameLeft, kSpiritFrameY);
        this->StepPanelGauge(
            battle.spiritBarsShown[slot],
            battle.spiritBars[slot],
            [ratio]
            {
                return static_cast<i32>(crt::Ftol(ratio));
            },
            kSpiritPicture,
            gaugeX,
            kSpiritGaugeY,
            kGaugeDownNear,
            kGaugeDownFar
        );
        this->battleHud
            .DrawSmallNumber(kNormalDigits, battle.spiritRolling[slot] + spirit, gaugeX + kNumberX, kSpiritNumberY);

        u16 staminaMaximum = 0;
        u16 staminaValue = 0;
        unit.GetStamina(staminaMaximum, staminaValue);
        i16 stamina = static_cast<i16>(staminaValue);
        ratio = static_cast<float>(
            static_cast<double>(stamina) / static_cast<double>(static_cast<i16>(staminaMaximum)) * kBarPixels
        );
        RollNumber(stamina, battle.stamina[slot], battle.staminaRolling[slot], battle.staminaSteps[slot], 1);
        this->battleHud.DrawPictureFrame(kStaminaPicture, kFrameIndex, gaugeX - kFrameLeft, kStaminaFrameY);
        this->StepPanelGauge(
            battle.staminaBarsShown[slot],
            battle.staminaBars[slot],
            [ratio]
            {
                return static_cast<i32>(crt::Ftol(ratio));
            },
            kStaminaPicture,
            gaugeX,
            kStaminaGaugeY,
            kGaugeDownNear,
            kGaugeDownFar
        );
        this->battleHud
            .DrawSmallNumber(kNormalDigits, battle.staminaRolling[slot] + stamina, gaugeX + kNumberX, kStaminaNumberY);
    }

    if (battle.autoBattle == 1)
    {
        const i32 step = (kAutoTargetX - battle.autoBoxX) / kAutoSlide;
        battle.autoBoxX += step;
        battle.autoBoxStep = step;
        const PaintRecord::Motion moving({(kAutoTargetX - battle.autoBoxX) / kAutoSlide, 0});
        this->battleMessages
            .DrawMessageBox(battle.autoBoxX - kAutoLeft, kAutoY, kAutoWidth, kAutoHeight, 0, 0, kAutoBattle);
    }
}

}  // namespace swd3
