#include "openswd3/battle/legacy_battle_actor_startup_reset.hpp"

#include <array>

namespace openswd3::battle {
namespace {

using compat::u8;
using compat::u32;

struct ZeroBlock {
    u32 instruction;
    u32 offset;
    u32 dwords;
    u32 trailing_word_instruction{};
};

constexpr std::array kZeroBlocks{
    ZeroBlock{0x0047D362U, 0x02A0U, 0x26U},
    ZeroBlock{0x0047D36FU, 0x0338U, 0x26U},
    ZeroBlock{0x0047D37CU, 0x03D0U, 0x26U},
    ZeroBlock{0x0047D389U, 0x0468U, 0x26U},
    ZeroBlock{0x0047D396U, 0x0500U, 0x26U},
    ZeroBlock{0x0047D3A3U, 0x0598U, 0x26U},
    ZeroBlock{0x0047D3B0U, 0x0C20U, 0x26U},
    ZeroBlock{0x0047D3BDU, 0x0630U, 0x130U},
    ZeroBlock{0x0047D3CAU, 0x0D50U, 8U},
    ZeroBlock{0x0047D3D7U, 0x0D90U, 10U},
    ZeroBlock{0x0047D3E4U, 0x295AU, 17U, 0x0047D3E6U},
    ZeroBlock{0x0047D3F5U, 0x29C4U, 17U, 0x0047D3F7U},
    ZeroBlock{0x0047D406U, 0x0FCCU, 0x130U},
    ZeroBlock{0x0047D413U, 0x283CU, 0x46U},
};

struct ScalarWrite {
    u32 instruction;
    u32 offset;
    u32 size;
    u32 value{};
};

constexpr std::array kScalarWrites{
    ScalarWrite{0x0047D44AU, 0x2A56U, 4U, 0xFFFFFFFFU},
    ScalarWrite{0x0047D44CU, 0x2A5AU, 4U, 0xFFFFFFFFU},
    ScalarWrite{0x0047D44FU, 0x2A5EU, 4U, 0xFFFFFFFFU},
    ScalarWrite{0x0047D452U, 0x2A62U, 4U, 0xFFFFFFFFU},
    ScalarWrite{0x0047D455U, 0x29A2U, 2U, 0xFFFFU},
    ScalarWrite{0x0047D45EU, 0x2668U, 4U, 15U},
    ScalarWrite{0x0047D468U, 0x266CU, 4U, 1U},
    ScalarWrite{0x0047D46EU, 0x2A68U, 2U, 2U},
    ScalarWrite{0x0047D477U, 0x2A6AU, 2U, 24U},
    ScalarWrite{0x0047D480U, 0x2954U, 2U},
    ScalarWrite{0x0047D487U, 0x2660U, 4U},
    ScalarWrite{0x0047D48DU, 0x2664U, 4U},
    ScalarWrite{0x0047D493U, 0x26D0U, 2U},
    ScalarWrite{0x0047D49AU, 0x26C0U, 4U},
    ScalarWrite{0x0047D4A0U, 0x26D2U, 2U},
    ScalarWrite{0x0047D4A7U, 0x2958U, 2U},
    ScalarWrite{0x0047D4AEU, 0x2A0EU, 2U},
    ScalarWrite{0x0047D4B5U, 0x29A4U, 2U},
    ScalarWrite{0x0047D4BCU, 0x2A0CU, 2U},
    ScalarWrite{0x0047D4C3U, 0x2A12U, 2U},
    ScalarWrite{0x0047D4CAU, 0x2A14U, 2U},
    ScalarWrite{0x0047D4D1U, 0x2A66U, 2U},
    ScalarWrite{0x0047D4D8U, 0x2A6CU, 2U},
    ScalarWrite{0x0047D4DFU, 0x2A6EU, 2U},
    ScalarWrite{0x0047D4E6U, 0x2A72U, 2U},
    ScalarWrite{0x0047D4EDU, 0x2A74U, 2U},
    ScalarWrite{0x0047D4F4U, 0x2A78U, 2U},
    ScalarWrite{0x0047D4FBU, 0x2A7AU, 2U},
    ScalarWrite{0x0047D502U, 0x2A7CU, 2U},
    ScalarWrite{0x0047D509U, 0x2A80U, 2U},
    ScalarWrite{0x0047D510U, 0x2A7EU, 2U},
    ScalarWrite{0x0047D517U, 0x2A82U, 2U},
    ScalarWrite{0x0047D51EU, 0x26B8U, 4U},
    ScalarWrite{0x0047D524U, 0x2A9CU, 1U},
    ScalarWrite{0x0047D52AU, 0x2A92U, 1U},
    ScalarWrite{0x0047D530U, 0x2A93U, 1U},
    ScalarWrite{0x0047D536U, 0x2A94U, 1U},
    ScalarWrite{0x0047D53CU, 0x2A95U, 1U},
    ScalarWrite{0x0047D542U, 0x2A86U, 2U},
    ScalarWrite{0x0047D549U, 0x2A97U, 1U},
    ScalarWrite{0x0047D54FU, 0x2A98U, 1U},
    ScalarWrite{0x0047D555U, 0x2A99U, 1U},
    ScalarWrite{0x0047D55BU, 0x2A9AU, 1U},
    ScalarWrite{0x0047D561U, 0x2A84U, 2U},
    ScalarWrite{0x0047D568U, 0x26C8U, 4U},
    ScalarWrite{0x0047D56EU, 0x26CCU, 4U},
    ScalarWrite{0x0047D574U, 0x2AA0U, 4U},
    ScalarWrite{0x0047D57AU, 0x2AA4U, 4U},
    ScalarWrite{0x0047D580U, 0x2AA8U, 4U},
    ScalarWrite{0x0047D586U, 0x2AACU, 4U},
    ScalarWrite{0x0047D58CU, 0x2AB0U, 4U},
    ScalarWrite{0x0047D592U, 0x2AB4U, 4U},
    ScalarWrite{0x0047D598U, 0x2AB8U, 4U},
    ScalarWrite{0x0047D59EU, 0x2AC0U, 4U},
    ScalarWrite{0x0047D5A4U, 0x2AC4U, 4U},
    ScalarWrite{0x0047D5AAU, 0x2AC8U, 4U},
    ScalarWrite{0x0047D5B0U, 0x267CU, 4U},
    ScalarWrite{0x0047D5B6U, 0x2AD0U, 4U},
    ScalarWrite{0x0047D5BCU, 0x2AD4U, 4U},
    ScalarWrite{0x0047D5C2U, 0x2AD8U, 4U},
    ScalarWrite{0x0047D5C8U, 0x2ADCU, 4U},
    ScalarWrite{0x0047D5CEU, 0x2AE0U, 4U},
    ScalarWrite{0x0047D5D4U, 0x2A8CU, 2U},
    ScalarWrite{0x0047D5DBU, 0x2AE4U, 4U},
    ScalarWrite{0x0047D5E1U, 0x2AE8U, 4U},
    ScalarWrite{0x0047D5E7U, 0x2AECU, 4U},
    ScalarWrite{0x0047D5EDU, 0x2AF0U, 4U},
    ScalarWrite{0x0047D5F3U, 0x2AF4U, 4U},
    ScalarWrite{0x0047D5F9U, 0x2ABCU, 4U},
    ScalarWrite{0x0047D5FFU, 0x2ACCU, 4U},
    ScalarWrite{0x0047D605U, 0x2AF8U, 4U},
    ScalarWrite{0x0047D60BU, 0x2AFCU, 4U},
    ScalarWrite{0x0047D611U, 0x2B00U, 4U},
    ScalarWrite{0x0047D617U, 0x2B04U, 4U},
    ScalarWrite{0x0047D61DU, 0x2B08U, 4U},
    ScalarWrite{0x0047D623U, 0x2B0CU, 4U},
    ScalarWrite{0x0047D629U, 0x2B20U, 4U},
};

}  // namespace

LegacyBattleActorStartupResetResult reset_legacy_battle_actor_for_startup(
    LegacyBattleActorStartupResetPort& port,
    LegacyBattleActorStartupResetHeapPort& heap
) {
    using Status = LegacyBattleActorStartupResetStatus;
    LegacyBattleActorStartupResetResult result{};
    const auto stop =
        [&](const Status status, const u32 instruction, const u32 location) {
            result.status = status;
            result.stopped_instruction = instruction;
            result.stopped_offset_or_token = location;
        };
    const auto write = [&](const u32 instruction,
                           const u32 offset,
                           const u32 size,
                           const u32 value) {
        const std::array<u8, 4> bytes{
            static_cast<u8>(value),
            static_cast<u8>(value >> 8U),
            static_cast<u8>(value >> 16U),
            static_cast<u8>(value >> 24U)
        };
        if (!port.write_actor_bytes(offset, std::span{bytes}.first(size))) {
            stop(Status::actor_write_typed_stop, instruction, offset);
            return false;
        }

        return true;
    };
    const auto release = [&](const u32 instruction, const u32 token) {
        const auto reply = heap.release_heap_block(token);
        if (!reply.has_value()) {
            stop(Status::heap_release_typed_stop, instruction, token);
            return false;
        }

        return true;
    };

    for (const auto& block : kZeroBlocks) {
        for (u32 index = 0U; index < block.dwords; ++index) {
            if (!write(block.instruction, block.offset + index * 4U, 4U, 0U)) {
                return result;
            }
        }

        if (block.trailing_word_instruction != 0U &&
            !write(
                block.trailing_word_instruction,
                block.offset + block.dwords * 4U,
                2U,
                0U
            )) {
            return result;
        }
    }

    // 0x0047E950(actor, 1): detach the chain before releasing its nodes.
    auto head = port.read_actor_dword(0x2584U);
    if (!head.has_value()) {
        stop(Status::actor_read_typed_stop, 0x0047F0BFU, 0x2584U);
        return result;
    }

    const auto mode = port.read_actor_word(0x26D0U);
    if (!mode.has_value()) {
        stop(Status::actor_read_typed_stop, 0x0047F0C5U, 0x26D0U);
        return result;
    }

    if (!write(0x0047F0C5U, 0x26D0U, 2U, *mode & 0xFEBDU) ||
        !write(0x0047F0D0U, 0x26C0U, 4U, 0U) ||
        !write(0x0047F0D6U, 0x2584U, 4U, 0U)) {
        return result;
    }

    while (*head != 0U) {
        const auto next = heap.read_linked_action_next(*head);
        if (!next.has_value()) {
            stop(Status::linked_action_read_typed_stop, 0x0047F0DEU, *head);
            return result;
        }

        if (!release(0x0047F0E1U, *head)) {
            return result;
        }

        head = next;
    }

    const auto runtime = port.read_actor_dword(0x2AA0U);
    if (!runtime.has_value()) {
        stop(Status::actor_read_typed_stop, 0x0047D422U, 0x2AA0U);
        return result;
    }

    if (*runtime == 1U) {
        const auto resource = port.read_actor_dword(0x000CU);
        if (!resource.has_value()) {
            stop(Status::actor_read_typed_stop, 0x0047D42EU, 0x000CU);
            return result;
        }

        if (*resource != 0U) {
            if (!release(0x0047D436U, *resource) ||
                !write(0x0047D43EU, 0x000CU, 4U, 0U)) {
                return result;
            }
        }
    }

    for (const auto& field : kScalarWrites) {
        if (!write(field.instruction, field.offset, field.size, field.value)) {
            return result;
        }
    }

    return result;
}

}  // namespace openswd3::battle
