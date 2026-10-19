#pragma once

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class Game;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): the menus, lists and panels of the battle's turns (battle_turn.cpp)
// split into the part deciding the frame and the part drawing it.
namespace swd3::smooth
{

class TurnFrame
{
public:
    explicit TurnFrame(Game& game);

    int FadeFieldOutAdvance(Scene& scene);

    void FieldFadeFrameAdvance(int counting, Scene& scene);

    void DrawListTabsAdvance(int x, int y, Scene& scene);

    void DrawCommandMenuAdvance(int x, int y, int cursor, Scene& scene);

    void DrawThrowListAdvance(int x, int y, int cursor, int top, Scene& scene);

    void DrawRefineListAdvance(int x, int y, int cursor, Scene& scene);

    void DrawSpecialListAdvance(int x, int y, int cursor, Scene& scene);

    void DrawEscortListAdvance(int x, int y, Scene& scene);

    void DrawTargetTagAdvance(int x, int y, Scene& scene);

    void DrawControlMenuAdvance(int x, int y, int cursor, Scene& scene);

    void DrawSpellListAdvance(int x, int y, int cursor, int top, Scene& scene);

    void DrawItemListAdvance(int x, int y, int cursor, int top, Scene& scene);

    void DrawVictoryPanelAdvance(Scene& scene);

    void DrawLevelUpPanelAdvance(Scene& scene);

    void DrawLearnedBoxAdvance(std::u8string_view line, bool sound, Scene& scene);

    void DrawLevelUpStatsAdvance(Scene& scene);

    void DrawBattleDebugAdvance(Scene& scene);

    void DrawCommandPanelAdvance(i32 nameIndex, Scene& scene);

private:
    Game& game;
};

}  // namespace swd3::smooth
