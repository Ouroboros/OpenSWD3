#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "battle/smooth/smooth_draw.h"
#include "core/types.h"

namespace swd3
{

class BattleMessages;

}  // namespace swd3

// The rebuild's smooth battle (deviations.md): BattleMessages::Draw split into the part deciding the frame and the
// part drawing it.
namespace swd3::smooth
{

// What BattleMessages::Draw draws, in its order: the framed bar (its frame picture `frame`), the black bar, the black
// box behind a sliding message at (x, y), a message's text at (x, y).
struct MessageStep
{
    enum class Kind : u8
    {
        Bar,
        BlackBar,
        Box,
        Text,
    };

    Kind kind = Kind::Text;
    int x = 0;
    int y = 0;
    u16 frame = 0;
    std::string text;
};

struct MessagesView
{
    std::vector<MessageStep> steps;
};

class MessagesFrame
{
public:
    explicit MessagesFrame(BattleMessages& messages);

    void Advance(MessagesView& view);
    void Draw(const MessagesView& view);
    // The same decided into a scene.
    void Frame(Scene& scene);

    void DrawPanelTextAdvance(int x, int y, std::string_view text, Scene& scene);
    void DrawPanelTextAdvance(int x, int y, std::u8string_view text, Scene& scene);

    void DrawMessageTextAdvance(int x, int y, std::string_view text, Scene& scene);

    void DrawMessageBoxAdvance(
        int x,
        int y,
        int width,
        int height,
        int textX,
        int textY,
        std::string_view text,
        Scene& scene
    );
    void DrawMessageBoxAdvance(
        int x,
        int y,
        int width,
        int height,
        int textX,
        int textY,
        std::u8string_view text,
        Scene& scene
    );

    void DrawLootPanelAdvance(Scene& scene);

    void DrawDefeatPanelAdvance(Scene& scene);

    void DrawCraftResultPanelAdvance(Scene& scene);

    void DrawResistancePanelAdvance(Scene& scene);

    void DrawMasteryAdvance(Scene& scene);

private:
    BattleMessages& messages;
};

}  // namespace swd3::smooth
