#include "battle/smooth/smooth_battle.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <string_view>

#include "battle/battle_frame_constants.h"
#include "battle/smooth/smooth_hud.h"
#include "battle/smooth/smooth_messages.h"
#include "battle/smooth/smooth_unit.h"
#include "crt/msvc.h"
#include "draw/paint_record.h"
#include "game/game.h"
#include "pic/picture_ops.h"
#include "platform/surface.h"
#include "ui/ui_draw.h"

namespace swd3::smooth
{

namespace
{

constexpr std::array<int, Party::Roles> kGaugeX = {0x188, 0xF6, 0x64, 0x21A};  // swd32.exe 0x004A7598
constexpr std::array<int, Party::Roles> kNameX = {0x17E, 0xEC, 0x5A, 0x210};   // swd32.exe 0x004A75B8
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
constexpr i32 kGaugeDownNear = 1;
constexpr i32 kGaugeDownFar = 3;
constexpr i32 kBlockDownNear = 2;
constexpr i32 kBlockDownFar = 5;
constexpr std::u8string_view kAutoBattle = u8"自動攻擊";  // swd32.exe 0x004A7814
constexpr int kAutoTargetX = 0x44;
constexpr int kAutoSlide = 3;
constexpr int kAutoLeft = 0x3A;
constexpr int kAutoY = 0x162;
constexpr int kAutoWidth = 0x46;
constexpr int kAutoHeight = 0x18;

// The gauges move a step a frame towards their value: a large step when far.
constexpr i32 kNearGauge = 0xA;
constexpr i32 kGaugeUpNear = 1;
constexpr i32 kGaugeUpFar = 3;

constexpr i32 kRollingBit = 0x8000000;  // the original tests bit 27, not the sign, before negating a change
constexpr i32 kRollingShare = 0xA;      // a changed number rolls in a tenth of the change a frame

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

BattleFrame::BattleFrame(Game& game)
    : game(game)
{
}

// A panel gauge (inline in Game::DrawRolePanels): drawn at its shown value, which moves towards `target`; when it has
// arrived the gauge takes the new value `next()`.
template <typename Next>
void BattleFrame::StepPanelGaugeAdvance(
    i32& shown,
    i32& target,
    Next next,
    int picture,
    int x,
    int y,
    i32 downNear,
    i32 downFar,
    Scene& scene
)
{
    HudFrame hud(this->game.battleHud);

    if (shown == target)
    {
        target = next();
        hud.DrawGaugeAdvance(picture, x, y, shown, scene);
    }
    else if (shown < target)
    {
        hud.DrawGaugeAltAdvance(picture, x, y, shown, scene);
        shown += target - shown <= kNearGauge ? kGaugeUpNear : kGaugeUpFar;
    }
    else
    {
        hud.DrawGaugeAltAdvance(picture, x, y, shown, scene);
        shown -= shown - target <= kNearGauge ? downNear : downFar;
    }
}

// What Game::DrawBattleBackground decides of the frame; the draws into `scene`.
void BattleFrame::DrawBattleBackgroundAdvance(int shake, Scene& scene)
{
    Game& game = this->game;
    BattleServices& services = game.battleServices;
    ScenePainter painter(services, scene);
    BattleState& battle = game.battleState;
    Picture& background = battle.background;
    const std::span<u16> field(game.drawSurface, kFieldPixels);

    auto drawBackground = [&painter, &background](u32 mode)
    {
        painter.SetSource(background);
        painter.DrawPicture(0, 0, background.width, background.height, mode);
    };

    painter.SetSource(background);
    painter.SetClipClamped(0, 0, Display::Width, Display::Height);

    if (battle.unk53BF94 == 0 && battle.unk53BF98 == 0)
    {
        if (shake != 0)
        {
            game.ShakeBattleBackground(shake);
            drawBackground(Painter::Copy);
            HudFrame(game.battleHud).PlayAnimationShiftedAdvance(battle.backgroundAnimation, 0, shake, scene);
        }
        else
        {
            drawBackground(Painter::Copy);
            HudFrame(game.battleHud).DrawAnimationAdvance(battle.backgroundAnimation, scene);

            if (battle.grayBandShown == 1)
            {
                u16 band = battle.grayBand;

                if (band < BattleState::BandFull)
                {
                    band = band < BattleState::BandFastFrom ? static_cast<u16>(band << 1)
                                                            : static_cast<u16>(band + BattleState::BandStep);
                    battle.grayBand = band;
                }

                painter.SetClipClamped(0, BattleState::BandRow - band, Display::Width, BattleState::BandRow);
                drawBackground(Painter::GrayUnder);
                painter.SetClipClamped(0, BattleState::BandRow, Display::Width, battle.grayBand + BattleState::BandRow);
                drawBackground(Painter::GrayUnder);
            }
        }

        battle.shake = 0;

        if (battle.flashing == 1)
        {
            DrawOptions& options = painter.Options();
            const int level = battle.flashLevel;
            options.tintRed = level;
            options.tintGreen = level;
            options.tintBlue = level;
            painter.ShiftRed(field, level);
            painter.ShiftGreenPairs(field, options.tintGreen);
            painter.ShiftBluePairs(field, options.tintBlue);
            battle.flashLevel = static_cast<i8>(battle.flashLevel - BattleState::FlashStep);

            if (battle.flashLevel == 0)
            {
                battle.flashLevel = BattleState::FlashStart;
                battle.flashing = 0;
            }
        }
    }

    painter.SetClipClamped(0, 0, Display::Width, Display::Height);
    i16 level = battle.darkLevel;

    if (battle.unk4A7630 == battle.unk53AE70 && (battle.unk53BF94 == 1 || battle.unk53BF98 == 1))
    {
        if (battle.darkenByTint != 0)
        {
            if (shake != 0)
            {
                game.ShakeBattleBackground(shake);
                HudFrame(game.battleHud).PlayAnimationShiftedAdvance(battle.backgroundAnimation, 0, shake, scene);
                battle.shake = 0;
            }

            drawBackground(Painter::Copy);
            painter.TintPixels(
                field,
                battle.darkLevel * (battle.darkRed >> 1),
                battle.darkLevel * (battle.darkGreen >> 1),
                battle.darkLevel * (battle.darkBlue >> 1)
            );
            level = battle.darkLevel;
        }
        else if (level >= 1)
        {
            painter.CopyScreen(*battle.surfaces[static_cast<std::size_t>(level - 1)], *game.offscreen);
            level = battle.darkLevel;
        }

        i32 frames = battle.darkFrames;

        if (frames > BattleState::DarkLevelFrames)
        {
            frames = 0;
            ++level;
            battle.darkLevel = level;

            if (level > BattleState::MaxDarkLevel)
            {
                level = BattleState::MaxDarkLevel;
                battle.darkLevel = level;
            }
        }

        battle.darkFrames = frames + 1;
    }

    if (battle.unk53BFEC != 1 || battle.unk53C030 != 0)
    {
        return;
    }

    if (level < 1)
    {
        battle.darkRed = 0;
        battle.darkGreen = 0;
        battle.darkBlue = 0;
        battle.darkLevel = 0;
        battle.unk4A7630 = -1;
        battle.unk53BF98 = 0;
        battle.unk53BF94 = 0;
        battle.darkenByTint = 0;
        battle.unk53BFEC = 0;
        return;
    }

    --level;
    battle.darkLevel = level;

    if (battle.unk4A7574 != -1 && battle.darkenByTint == 0)
    {
        // Level 0 reads the null pointer before the copies (0x005244C0): the blit fails and nothing is drawn.
        if (level >= 1)
        {
            painter.CopyScreen(*battle.surfaces[static_cast<std::size_t>(level - 1)], *game.offscreen);
        }

        return;
    }

    drawBackground(Painter::Copy);
}

// What Game::DrawRolePanels decides of the frame; the draws into `scene`.
void BattleFrame::DrawRolePanelsAdvance(Scene& scene)
{
    Game& game = this->game;
    BattleState& battle = game.battleState;
    SceneFont font(game.mediumFont, scene);
    font.SetShadow(0);
    font.SetBackground(TextFont::NoBackground);
    int listY = kListTop;

    for (int index = 0; index < static_cast<int>(battle.roleCount); ++index)
    {
        BattleRole& unit = game.battleRoles[static_cast<std::size_t>(index)];
        const int x = battle.mirrored == 1 ? kMirroredListX : kListX;

        if (unit.unk2B00 != 1)
        {
            continue;
        }

        const std::string_view name = NameOf(unit.Data()->data.name);
        MessagesFrame(game.battleMessages)
            .DrawMessageBoxAdvance(x, listY, kListWidth, kListHeight, x + kListTextX, listY + kListTextY, name, scene);
        i32 hitPoints = 0;
        i32 maximum = 0;
        unit.GetHitPoints(hitPoints, maximum);
        const auto bar =
            static_cast<int>(crt::Ftol(static_cast<double>(hitPoints) / static_cast<double>(maximum) * kBarPixels));
        HudFrame(game.battleHud).DrawBarAdvance(kHitPointPicture, 1, x + kListBarX, listY + kListBarY, bar, scene);
        const auto gauge = static_cast<u16>(unit.GaugeLength(game.battleServices));

        if (gauge != 0)
        {
            const u32 color = game.painter.MakeColor(0, 0, kListGaugeBlue);
            HudFrame(game.battleHud)
                .FillColor(x + kListBarX, listY + kListGaugeY, gauge, kListGaugeHeight, color, scene);
        }

        if (index == static_cast<int>(battle.roleCount) - 1 && (static_cast<u8>(battle.listGlow) & kGlowFalling) != 0)
        {
            battle.listGlow = static_cast<i8>(battle.listGlow + kListGlowStep);

            if ((static_cast<u8>(battle.listGlow) & kGlowMask) > kListGlowTop)
            {
                battle.listGlow = 0;
            }

            const int level = static_cast<u8>(battle.listGlow) & kGlowMask;
            Box(game.painter,
                x,
                listY - kListGlowUp,
                kListWidth,
                kListGlowHeight,
                level,
                0,
                0,
                FillMode::Gradient,
                scene);
        }

        listY += kListStep;
    }

    for (int index = 0; index < static_cast<int>(battle.roleCount); ++index)
    {
        const auto slot = static_cast<std::size_t>(index);
        BattleRole& unit = game.battleRoles[slot];

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
            Box(game.painter,
                BattleState::PortraitX[static_cast<std::size_t>(battle.partyRoles[static_cast<std::size_t>(index)])] -
                    kHighlightLeft,
                kHighlightTop,
                kHighlightWidth,
                kHighlightHeight,
                level,
                level,
                level,
                FillMode::Tint,
                scene);

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
        const std::string_view name = NameOf(game.party.names[partyRole]);

        if (!unit.IsDefeated())
        {
            const auto gauge = static_cast<u16>(unit.GaugeLength(game.battleServices));

            if (unit.unk2AB0 == 0)
            {
                HudFrame(game.battleHud).DrawGaugeWideAdvance(kNameGauge, nameX, kNameY, gauge, scene);
            }
        }

        if (unit.IsDefeated())
        {
            font.DrawText(game.drawSurface, nameX, kNameY, name, kDefeatedColor, kNameStyle);
        }

        if (unit.unk2AB0 == 1)
        {
            if (current)
            {
                font.DrawText(game.drawSurface, nameX, kNameY, name, kNameColor, kNameStyle);
                const Pixel color = battle.nameFlashes[slot] % kFlashPeriod != 0 ? kFlashColor : kNameColor;
                font.DrawText(game.drawSurface, nameX, kNameY, name, color, kNameStyle);
                ++battle.nameFlashes[slot];
            }
            else
            {
                font.DrawText(game.drawSurface, nameX, kNameY, name, kNameColor, kNameStyle);
            }
        }

        HudFrame(game.battleHud)
            .DrawPortraitAdvance(static_cast<u16>(partyRole), portraitX, kPortraitY, unit.IsDefeated() ? 1 : 0, scene);
        UnitFrame(unit).DrawEffectIconsAdvance(game.battleServices, gaugeX, kIconsY, scene);
        this->StepPanelGaugeAdvance(
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
            kBlockDownFar,
            scene
        );

        if (*unit.roleBlock == kArrowBlock && !unit.IsDefeated())
        {
            HudFrame(game.battleHud).DrawArrowAdvance(portraitX, kBlockY, index, scene);
        }

        i32 hitPoints = 0;
        i32 maximum = 0;
        unit.GetHitPoints(hitPoints, maximum);
        float ratio = static_cast<float>(static_cast<double>(hitPoints) / static_cast<double>(maximum) * kBarPixels);
        RollNumber(hitPoints, battle.hitPoints[slot], battle.hitPointsRolling[slot], battle.hitPointSteps[slot], 0);
        HudFrame(game.battleHud)
            .DrawPictureFrameAdvance(kHitPointPicture, kFrameIndex, gaugeX - kFrameLeft, kHitPointFrameY, scene);
        this->StepPanelGaugeAdvance(
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
            kGaugeDownFar,
            scene
        );
        const int digits = hitPoints > kCriticalHitPoints
                               ? (hitPoints > maximum / kLowShare ? kNormalDigits : kLowDigits)
                               : kCriticalDigits;
        HudFrame(game.battleHud)
            .DrawSmallNumberAdvance(
                digits,
                battle.hitPointsRolling[slot] + hitPoints,
                gaugeX + kNumberX,
                kHitPointNumberY,
                scene
            );

        u16 spiritMaximum = 0;
        u16 spiritValue = 0;
        unit.GetSpirit(spiritMaximum, spiritValue);
        i16 spirit = static_cast<i16>(spiritValue);
        ratio = static_cast<float>(
            static_cast<double>(spirit) / static_cast<double>(static_cast<i16>(spiritMaximum)) * kBarPixels
        );
        RollNumber(spirit, battle.spirit[slot], battle.spiritRolling[slot], battle.spiritSteps[slot], 0);
        HudFrame(game.battleHud)
            .DrawPictureFrameAdvance(kSpiritPicture, kFrameIndex, gaugeX - kFrameLeft, kSpiritFrameY, scene);
        this->StepPanelGaugeAdvance(
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
            kGaugeDownFar,
            scene
        );
        HudFrame(game.battleHud)
            .DrawSmallNumberAdvance(
                kNormalDigits,
                battle.spiritRolling[slot] + spirit,
                gaugeX + kNumberX,
                kSpiritNumberY,
                scene
            );

        u16 staminaMaximum = 0;
        u16 staminaValue = 0;
        unit.GetStamina(staminaMaximum, staminaValue);
        i16 stamina = static_cast<i16>(staminaValue);
        ratio = static_cast<float>(
            static_cast<double>(stamina) / static_cast<double>(static_cast<i16>(staminaMaximum)) * kBarPixels
        );
        RollNumber(stamina, battle.stamina[slot], battle.staminaRolling[slot], battle.staminaSteps[slot], 1);
        HudFrame(game.battleHud)
            .DrawPictureFrameAdvance(kStaminaPicture, kFrameIndex, gaugeX - kFrameLeft, kStaminaFrameY, scene);
        this->StepPanelGaugeAdvance(
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
            kGaugeDownFar,
            scene
        );
        HudFrame(game.battleHud)
            .DrawSmallNumberAdvance(
                kNormalDigits,
                battle.staminaRolling[slot] + stamina,
                gaugeX + kNumberX,
                kStaminaNumberY,
                scene
            );
    }

    if (battle.autoBattle == 1)
    {
        const i32 step = (kAutoTargetX - battle.autoBoxX) / kAutoSlide;
        battle.autoBoxX += step;
        battle.autoBoxStep = step;
        const PaintRecord::Motion moving({(kAutoTargetX - battle.autoBoxX) / kAutoSlide, 0});
        MessagesFrame(game.battleMessages)
            .DrawMessageBoxAdvance(
                battle.autoBoxX - kAutoLeft,
                kAutoY,
                kAutoWidth,
                kAutoHeight,
                0,
                0,
                kAutoBattle,
                scene
            );
    }
}

}  // namespace swd3::smooth
