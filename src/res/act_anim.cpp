#include "res/act_anim.h"

#include <cstring>

#include "data/globals.h"
#include "res/act.h"

namespace swd3 {

namespace {

constexpr std::uint16_t op(char a, char b) {
    return static_cast<std::uint16_t>((static_cast<unsigned char>(b) << 8) | static_cast<unsigned char>(a));
}

void resetPlayback(ActState* state, std::uint32_t keepFlags) {
    state->delay = 0;
    state->frameDelay = 0;
    state->pc = 0;
    state->effectA = 0;
    state->effectQ = 0;
    state->x = 0;
    state->y = 0;
    state->width = 0;
    state->height = 0;
    state->vw = 0;
    state->ta = 0;
    state->xa = 0;
    state->ya = 0;
    state->sg = 0;
    state->wt = 0;
    state->unknown89 = 0;
    state->dl = 0;
    state->rc = 0;
    state->gc = 0;
    state->bc = 0;
    for (std::uint16_t& value : state->lf) {
        value = 0;
    }
    state->waiting = 0;
    state->hold = 0;
    state->ms = 0;
    state->flags &= keepFlags;
    G.actStateReset = 1;
}

}  // namespace

// swd32.exe 0x004321E0: advances an action script by one game frame. A change of direction, action or id restarts
// the script. Returns 0 when the script cannot be loaded. The opcodes YX, HW, GC, RC and BC read their second
// value without moving past it, so it is executed as the next opcode.
int ActRun(ActState* state) {
    if ((state->hold == 1 && state->pc != 0) || state->id == 0) {
        return 1;
    }
    if (state->direction != state->lastDirection) {
        state->lastDirection = state->direction;
        resetPlayback(state, 0x80000000u);
    }
    if (state->action != state->lastAction) {
        state->lastAction = state->action;
        resetPlayback(state, 0x80000003u);
    }
    if (state->id != state->lastId) {
        state->lastId = state->id;
        resetPlayback(state, 0x80000003u);
    }
    std::uint8_t* script = G.actOption == 1 ? ActGet(state->id, state->action + state->direction)
                                            : ActLoadDirect(state->id, state->action + state->direction);
    state->script = script;
    if (script == nullptr) {
        return 0;
    }
    G.actStateReset = 0;
    if (state->delay != 0) {
        --state->delay;
        return 1;
    }
    auto word = [script](std::uint16_t index) {
        std::uint16_t value;
        std::memcpy(&value, script + 2 * index, 2);
        return value;
    };
    state->ao = 0;
    std::uint16_t next;
    for (;;) {
        const std::uint16_t code = word(state->pc);
        next = static_cast<std::uint16_t>(state->pc + 1);
        state->pc = next;
        if (code == op('D', 'E')) {
            break;
        }
        if (code == op('V', 'O')) {
            state->pc = state->hold == 1 ? static_cast<std::uint16_t>(next - 1) : 0;
            return 1;
        }
        if (code == op('2', 'O')) {
            if (state->delay == 0) {
                state->waiting = 1;
            }
            state->pc = static_cast<std::uint16_t>(next - 1);
            return 1;
        }
        state->delay = state->frameDelay;
        const std::int16_t speed = state->speed;
        if (speed < 0) {
            state->delay = static_cast<std::uint16_t>(speed & 0x7FFF);
        }
        switch (code) {
        case op('V', 'W'):
            state->vw = word(state->pc++);
            break;
        case op('Y', 'X'):
            state->x = word(state->pc++);
            state->y = word(state->pc);
            break;
        case op('H', 'W'):
            state->width = word(state->pc++);
            state->height = word(state->pc);
            break;
        case op('N', 'T'):
            state->counter = 0;
            state->counter = word(state->pc++);
            break;
        case op('W', 'T'):
            state->wt = static_cast<std::uint8_t>(word(state->pc++));
            break;
        case op('I', 'V'):
            state->flags |= 1;
            break;
        case op('M', 'S'):
            state->ms = 1;
            break;
        case op('F', 'R'):
            state->fr = word(state->pc++);
            break;
        case op('O', 'R'):
            state->ro = word(state->pc++);
            break;
        case op('D', 'S'):
            state->frameDelay = word(state->pc++);
            state->delay = state->frameDelay;
            if (speed < 0) {
                state->delay = static_cast<std::uint16_t>(speed & 0x7FFF);
            }
            break;
        case op('E', 'Q'):
            state->effectQ = word(state->pc++);
            break;
        case op('X', 'O'):
            state->xo = word(state->pc++);
            break;
        case op('Y', 'O'):
            state->yo = word(state->pc++);
            break;
        case op('A', 'P'): {
            const std::uint16_t counter = state->counter;
            state->ap = word(state->pc++);
            std::uint16_t round = static_cast<std::uint16_t>((counter >> 8) + 1);
            const std::uint8_t limit = static_cast<std::uint8_t>(counter);
            if (round > limit) {
                round = static_cast<std::uint16_t>((round & 0xFF00) | 1);
            }
            state->counter = static_cast<std::uint16_t>(limit | ((round & 0xFF) << 8));
            break;
        }
        case op('A', 'O'):
            state->ao = word(state->pc++);
            break;
        case op('S', 'G'):
            state->flags = (state->flags & 0x80000003u) | 0x14;
            state->sg = static_cast<std::uint8_t>(word(state->pc++));
            break;
        case op('D', 'L'):
            state->flags = (state->flags & 0x80000003u) | 0x10;
            state->dl = static_cast<std::uint8_t>(word(state->pc++));
            break;
        case op('O', 'N'):
            state->flags &= ~1u;
            break;
        case op('L', 'F'): {
            const std::uint16_t at = state->pc;
            for (int i = 0; i < 7; ++i) {
                state->lf[i] = word(static_cast<std::uint16_t>(at + i));
            }
            state->pc = static_cast<std::uint16_t>(at + 7);
            break;
        }
        case op('G', 'C'):
            state->gc = word(state->pc++);
            state->gc2 = word(state->pc);
            break;
        case op('L', 'C'):
            state->rc2 = 0;
            state->gc2 = 0;
            state->bc2 = 0;
            break;
        case op('R', 'C'):
            state->rc = word(state->pc++);
            state->rc2 = word(state->pc);
            break;
        case op('B', 'C'):
            state->bc = word(state->pc++);
            state->bc2 = word(state->pc);
            break;
        case op('E', 'A'):
            state->effectA = word(state->pc++);
            break;
        case op('H', 'A'):
            state->flags = (state->flags & 0x80000003u) | 8;
            break;
        case op('M', 'A'):
            state->flags = (state->flags & 0x80000003u) | 4;
            break;
        case op('N', 'A'):
            state->flags = (state->flags & 0x80000003u) | 0x2C;
            break;
        case op('T', 'A'):
            state->ta = word(state->pc++);
            break;
        case op('X', 'A'):
            state->xa = word(state->pc++);
            break;
        case op('Y', 'A'):
            state->ya = word(state->pc++);
            break;
        default:
            break;
        }
    }
    if (state->hold == 1) {
        state->pc = static_cast<std::uint16_t>(next - 1);
    }
    return 1;
}

}  // namespace swd3
