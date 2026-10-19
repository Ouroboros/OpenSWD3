#pragma once

#include <array>
#include <string>
#include <vector>

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
// box behind a sliding message at (x, y), a message's text at (x, y); `motion`: the rebuild's smooth battle reckoning.
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
    std::array<int, 2> motion = {};
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

private:
    BattleMessages& messages;
};

}  // namespace swd3::smooth
