#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u32;

struct ObservedRead {
    u32 offset;
    u32 width;
    u32 value;
};

struct ObservedWriteRange {
    u32 offset;
    u32 size;
    u8 value;
};

#include "../../../analysis/04-reverse-engineering/artifacts/battle-actor-frame-reset-00479850/observed-state.inc"

static_assert(std::size(kObservedReads) == 6U);
static_assert(std::endian::native == std::endian::little);
static_assert(
    kObservedActor == openswd3::battle::kLegacyBattleActorGroupBBaseToken
);

class NoRandom final : public openswd3::battle::LegacyBattleBoundedRandomPort {
public:
    [[nodiscard]] u32 random_bounded(u32) override {
        // The measured +0x2AA0 is zero. No RNG reply may be synthesized.
        std::abort();
    }
};

[[nodiscard]] u32 flag_bits(
    const openswd3::battle::LegacyBattleActorCoordinateFlags& flags,
    const bool direction
) noexcept {
    return (flags.carry ? 0x001U : 0U) | (flags.parity ? 0x004U : 0U) |
        (flags.auxiliary_carry ? 0x010U : 0U) | (flags.zero ? 0x040U : 0U) |
        (flags.sign ? 0x080U : 0U) | (direction ? 0x400U : 0U) |
        (flags.overflow ? 0x800U : 0U);
}

}  // namespace

void test_battle_actor_frame_original_reset_suffix(
    openswd3::test::Context& test
) {
    using namespace openswd3::battle;
    std::size_t compared_bytes{};
    for (const auto& range : kObservedWrites) {
        compared_bytes += range.size;
    }

    for (const u32 sentinel : {0x13579BDFU, 0xA5A5A5A5U, 0xFFFFFFFFU}) {
        auto startup = std::make_unique<LegacyBattleStartupState>();
        startup->group_b_lifecycle = std::make_shared<std::array<
            LegacyBattleActorGroupBElementState,
            kLegacyBattleActorGroupBElementCount>>();
        const auto actor = resolve_legacy_battle_actor_runtime_reset(
            {.startup = startup.get()}, kObservedActor
        );
        LegacyBattleActorImage input{};
        materialize_legacy_battle_actor_image(actor, input);
        // Every compared old byte is dead on this measured path. Seed it
        // with test data so a missing clear cannot pass on default zeros.
        // This data is never represented as an original actor snapshot.
        for (const auto& range : kObservedWrites) {
            std::memset(
                input.data() + range.offset,
                static_cast<int>(sentinel & 0xFFU),
                range.size
            );
        }

        for (const auto& row : kObservedReads) {
            std::memcpy(input.data() + row.offset, &row.value, row.width);
        }

        for (const auto& range : kObservedWrites) {
            synchronize_legacy_battle_actor_image_write(
                actor, input, range.offset, range.size
            );
        }

        for (const auto& row : kObservedReads) {
            synchronize_legacy_battle_actor_image_write(
                actor, input, row.offset, row.width
            );
        }

        const LegacyBattleActorFrameEntryRequest request{
            .actor_token = kObservedActor,
            .entry_eax = kObservedEntry[0U],
            .entry_edx = kObservedEntry[2U],
            .entry_ebx = kObservedEntry[3U],
            .entry_ebp = kObservedEntry[4U],
            .entry_esi = kObservedEntry[5U],
            .entry_edi = kObservedEntry[6U],
            .entry_esp = kObservedEntry[7U],
            .entry_return_address = kObservedParentReturn,
            .direction_flag = (kObservedEntry[8U] & 0x400U) != 0U,
            .reset_random_callable = false,
        };
        // These are NOT a captured callee reply or full-function input.
        // Live values follow the LST prologue, balanced argument slots,
        // preserved ESI/EDI/EBX, and selector XOR EAX. Dead registers are
        // deliberately varied; the first phase CMP replaces unknown FLAGS.
        LegacyBattleActorFrameEntryResult prefix{
            .status =
                LegacyBattleActorFrameEntryStatus::update_selector_case_ready,
            .eax = sentinel & 0xFFFFU,
            .ecx = sentinel,
            .edx = sentinel,
            .ebx = 0U,
            .ebp = sentinel,
            .esi = kObservedActor,
            .edi = kObservedActor + 0x03D0U,
            .esp = kObservedEntry[7U] - 36U,
            .eip = 0x00479CA6U,
        };
        prefix.direction_flag = request.direction_flag;
        prefix = continue_legacy_battle_actor_frame_case_three_header(
            actor, request, prefix
        );
        prefix = continue_legacy_battle_actor_frame_common_reset_prefix(
            actor, request, prefix
        );
        NoRandom random;
        const auto result =
            continue_legacy_battle_actor_frame_common_reset_return(
                actor, random, request, prefix
            );
        const std::string label =
            "original seq372 reset suffix sentinel " + std::to_string(sentinel);
        const auto& child = result.reset_child;
        test.expect_true(
            result.status ==
                    LegacyBattleActorFrameEntryStatus::case_reset_returned &&
                result.returned && result.reset_calls == 1U && child.returned &&
                child.random_calls == 0U && child.actor_reads == 5U &&
                child.rep_iterations ==
                    std::array<u32, 8U>{
                        0U, 38U, 38U, 38U, 38U, 304U, 304U, 10U
                    } &&
                result.eax == kObservedLeave[0U] &&
                result.ecx == kObservedLeave[1U] &&
                result.edx == kObservedLeave[2U] &&
                result.ebx == kObservedLeave[3U] &&
                result.ebp == kObservedLeave[4U] &&
                result.esi == kObservedLeave[5U] &&
                result.edi == kObservedLeave[6U] &&
                result.esp == kObservedLeave[7U] + 4U &&
                result.eip == kObservedParentReturn && result.flags_known &&
                result.flags.auxiliary_carry_defined &&
                result.flags.overflow_defined &&
                flag_bits(result.flags, result.direction_flag) ==
                    (kObservedLeave[8U] & 0xCD5U),
            label + " captured parent-return GPR/defined FLAGS"
        );
        LegacyBattleActorImage output{};
        materialize_legacy_battle_actor_image(actor, output);
        for (const auto& range : kObservedWrites) {
            for (std::size_t index = 0U; index < range.size; ++index) {
                const std::size_t offset = range.offset + index;
                test.expect_equal(
                    std::to_integer<u8>(output[offset]),
                    range.value,
                    label + " captured written actor byte " +
                        std::to_string(offset)
                );
            }
        }
    }

    std::cout << "WP316 original reset-suffix diff: seq372, six observed "
              << "actor reads, " << compared_bytes
              << " written bytes and parent-return GPR/defined FLAGS; "
              << "three dead-input sentinel variants; not complete "
              << "sub_479850 replay.\n";
}
