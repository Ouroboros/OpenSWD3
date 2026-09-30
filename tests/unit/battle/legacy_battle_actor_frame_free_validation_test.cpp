#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "test.hpp"

#include <array>
#include <bit>
#include <cstddef>

namespace {

using namespace openswd3::battle;
using openswd3::compat::u8;
using openswd3::compat::u32;

class OpaqueRelease final : public LegacyBattleActorFrameReleasePort {
public:
    std::size_t calls{};

    LegacyBattleActorFrameUpdateReply release_emitter(
        u32, u32, u32, u32, const LegacyBattleActorCoordinateFlags&
    ) override {
        ++calls;
        return {.returned = false};
    }
};

bool same_flags(
    const LegacyBattleActorCoordinateFlags& a,
    const LegacyBattleActorCoordinateFlags& b
) {
    return a.carry == b.carry && a.parity == b.parity &&
        a.auxiliary_carry == b.auxiliary_carry &&
        a.auxiliary_carry_defined == b.auxiliary_carry_defined &&
        a.zero == b.zero && a.sign == b.sign && a.overflow == b.overflow &&
        a.overflow_defined == b.overflow_defined;
}

}  // namespace

void test_battle_actor_frame_free_validation_prefix(
    openswd3::test::Context& test
) {
    // Independent LST access order: sub_4885C0 -> sub_488F90 -> sub_488F40.
    // No captured runtime values or Win32 returns are claimed by this test.
    constexpr std::array<u32, 18U> ips{
        0x00488658U,
        0x0048865BU,
        0x0048865CU,
        0x00488F90U,
        0x00488F93U,
        0x00488F94U,
        0x00488F9EU,
        0x00488FA0U,
        0x00488FA2U,
        0x00488FA8U,
        0x00488FA9U,
        0x00488F40U,
        0x00488F43U,
        0x00488F44U,
        0x00488F4AU,
        0x00488F4DU,
        0x00488F4EU,
        0x00488F51U
    };
    constexpr std::array<u32, 18U> esp_offsets{
        44U,
        44U,
        48U,
        52U,
        56U,
        60U,
        60U,
        64U,
        68U,
        68U,
        72U,
        76U,
        80U,
        84U,
        84U,
        84U,
        88U,
        88U
    };
    constexpr std::array<u32, 18U> token_offsets{
        20U,
        48U,
        52U,
        56U,
        60U,
        48U,
        64U,
        68U,
        48U,
        72U,
        76U,
        80U,
        84U,
        72U,
        68U,
        88U,
        72U,
        92U
    };
    constexpr std::array<bool, 18U> writes{
        false,
        true,
        true,
        true,
        true,
        false,
        true,
        true,
        false,
        true,
        true,
        true,
        true,
        false,
        false,
        true,
        false,
        true
    };
    constexpr u32 hook = 0x0048AA70U;
    constexpr u32 sign_bit = 0x80000000U;
    bool faults_exact = true;
    bool frontiers_exact = true;
    std::size_t fault_count{};
    for (const u32 argument : {0x00801000U, 0x80000010U, 0x10U, 0x20U}) {
        const u32 header = argument - 0x20U;
        const auto parity = [](const u32 value) {
            return (std::popcount(static_cast<u8>(value)) & 1) == 0;
        };

        const std::array<LegacyBattleActorCoordinateFlags, 4U> flags{
            LegacyBattleActorCoordinateFlags{
                .auxiliary_carry_defined = false
            },  // TEST 1,1.
            LegacyBattleActorCoordinateFlags{
                .parity = parity(argument), .sign = (argument & sign_bit) != 0U
            },  // CMP argument,0.
            LegacyBattleActorCoordinateFlags{
                .carry = argument < 0x20U,
                .parity = parity(header),
                .auxiliary_carry = ((argument ^ 0x20U ^ header) & 0x10U) != 0U,
                .zero = header == 0U,
                .sign = (header & sign_bit) != 0U,
                .overflow =
                    ((argument ^ 0x20U) & (argument ^ header) & sign_bit) != 0U
            },  // SUB argument,20h, including borrow and signed overflow.
            LegacyBattleActorCoordinateFlags{
                .parity = parity(header),
                .zero = header == 0U,
                .sign = (header & sign_bit) != 0U
            }  // CMP header,0 does not retain SUB's CF/OF.
        };
        for (const bool df : {false, true}) {
            for (const u32 debug : {0U, 1U, 4U}) {
                LegacyBattleActorFrameEntryRequest request{};
                request.decoder_heap_debug_flags_owner = &debug;
                request.decoder_heap_alloc_owner = &hook;
                LegacyBattleActorFrameEntryResult before{};
                before.status = LegacyBattleActorFrameEntryStatus::
                    case_two_release_call_ready;
                before.eip = 0x00479C93U;
                before.eax = argument;
                before.ecx = 0xDECAF001U;
                before.edx = 0xB1234000U;
                before.ebx = 0U;
                before.ebp = 0x88007766U;
                before.esi = 0x00525508U;
                before.edi = 0x35353353U;
                before.esp = 0x001AFE00U;
                before.accesses_completed = 41U;
                before.direction_flag = df;
                const u32 saved_ecx = debug == 4U ? 0U : before.ecx;
                const std::size_t start =
                    before.accesses_completed + 28U + (debug == 4U ? 12U : 0U);
                const std::array<u32, 18U> last_push{
                    before.esp - 28U,
                    before.esp - 28U,
                    argument,
                    0x00488661U,
                    before.esp - 28U,
                    saved_ecx,
                    saved_ecx,
                    1U,
                    0x20U,
                    0x20U,
                    header,
                    0x00488FAEU,
                    before.esp - 56U,
                    saved_ecx,
                    saved_ecx,
                    saved_ecx,
                    0x20U,
                    0x20U
                };
                const std::size_t access_count = header == 0U ? 14U : 18U;
                for (std::size_t i = 0U; i < access_count; ++i) {
                    request.stop_before_access = start + i;
                    OpaqueRelease port;
                    const auto stopped =
                        continue_legacy_battle_actor_frame_case_two_release_call(
                            port, request, before
                        );
                    const std::size_t flag_phase = i < 6U ? 0U
                        : i < 9U                          ? 1U
                        : i < 14U                         ? 2U
                                                          : 3U;
                    faults_exact = faults_exact &&
                        stopped.status ==
                            (writes[i] ? LegacyBattleActorFrameEntryStatus::
                                             stack_write_typed_stop
                                       : LegacyBattleActorFrameEntryStatus::
                                             stack_read_typed_stop) &&
                        stopped.eip == ips[i] &&
                        stopped.stopped_instruction == ips[i] &&
                        stopped.stopped_access_kind ==
                            (writes[i] ? LegacyBattleActorFrameEntryAccessKind::
                                             stack_write
                                       : LegacyBattleActorFrameEntryAccessKind::
                                             stack_read) &&
                        stopped.stopped_token ==
                            before.esp - token_offsets[i] &&
                        stopped.esp == before.esp - esp_offsets[i] &&
                        stopped.ebp ==
                            before.esp -
                                (i < 4U        ? 28U
                                     : i < 12U ? 56U
                                               : 80U) &&
                        stopped.eax ==
                            (i < 9U        ? 1U
                                 : i < 15U ? header
                                           : 0x20U) &&
                        stopped.ecx == (i < 17U ? saved_ecx : header) &&
                        stopped.edx == (i == 0U ? 1U : argument) &&
                        stopped.ebx == before.ebx &&
                        stopped.esi == before.esi &&
                        stopped.edi == before.edi &&
                        stopped.last_pushed_value == last_push[i] &&
                        stopped.accesses_completed == start + i &&
                        stopped.flags_known &&
                        same_flags(stopped.flags, flags[flag_phase]) &&
                        stopped.direction_flag == df && !stopped.returned &&
                        !stopped.release_child.returned &&
                        stopped.release_calls == 1U && port.calls == 0U;
                    ++fault_count;
                }

                request.stop_before_access = start + access_count;
                OpaqueRelease ordinal_port;
                const auto ordinal_stop =
                    continue_legacy_battle_actor_frame_case_two_release_call(
                        ordinal_port, request, before
                    );
                request.stop_before_access = static_cast<std::size_t>(-1);
                OpaqueRelease opaque_port;
                const auto opaque_stop =
                    continue_legacy_battle_actor_frame_case_two_release_call(
                        opaque_port, request, before
                    );
                for (const auto* stopped : {&ordinal_stop, &opaque_stop}) {
                    const bool null_header = header == 0U;
                    const bool after_null_returns =
                        null_header && stopped == &opaque_stop;
                    const LegacyBattleActorCoordinateFlags returned_zero_flags{
                        .parity = true,
                        .auxiliary_carry_defined = false,
                        .zero = true,
                    };
                    frontiers_exact = frontiers_exact &&
                        stopped->eip ==
                            (after_null_returns ? 0x00488668U
                                 : null_header  ? 0x00488F7DU
                                                : 0x00488F52U) &&
                        stopped->stopped_instruction == stopped->eip &&
                        stopped->stopped_access_kind ==
                            (null_header
                                 ? LegacyBattleActorFrameEntryAccessKind::
                                       stack_write
                                 : LegacyBattleActorFrameEntryAccessKind::
                                       global_read) &&
                        stopped->stopped_token ==
                            (after_null_returns ? before.esp - 48U
                                 : null_header  ? before.esp - 84U
                                                : 0x004990BCU) &&
                        stopped->esp ==
                            before.esp -
                                (after_null_returns ? 44U
                                     : null_header  ? 84U
                                                    : 92U) &&
                        stopped->ebp ==
                            before.esp - (after_null_returns ? 28U : 80U) &&
                        stopped->eax == (null_header ? 0U : 0x20U) &&
                        stopped->ecx == (null_header ? saved_ecx : header) &&
                        stopped->edx == argument &&
                        stopped->ebx == before.ebx &&
                        stopped->esi == before.esi &&
                        stopped->edi == before.edi &&
                        stopped->last_pushed_value ==
                            (null_header ? saved_ecx : header) &&
                        stopped->accesses_completed ==
                            start + access_count +
                                (after_null_returns ? 6U : 0U) &&
                        stopped->flags_known &&
                        same_flags(stopped->flags,
                                   after_null_returns ? returned_zero_flags
                                                      : flags[3U]) &&
                        stopped->direction_flag == df && !stopped->returned &&
                        !stopped->release_child.returned &&
                        stopped->release_calls == 1U;
                }

                frontiers_exact = frontiers_exact && ordinal_port.calls == 0U &&
                    opaque_port.calls == 1U &&
                    ordinal_stop.status ==
                        (header == 0U ? LegacyBattleActorFrameEntryStatus::
                                            stack_write_typed_stop
                                      : LegacyBattleActorFrameEntryStatus::
                                            global_read_typed_stop) &&
                    opaque_stop.status ==
                        LegacyBattleActorFrameEntryStatus::
                            case_two_release_child_typed_stop;
            }
        }
    }

    test.expect_true(
        fault_count == 408U && faults_exact,
        "free validation has 408 independent LST-derived stack faults including DF, debug masks, pointer borrow and signed overflow"
    );
    test.expect_true(
        frontiers_exact,
        "free validation stops before IsBadReadPtr IAT, null-header write fault or post-return assertion PUSH, without executing Win32 or fabricating a free return"
    );

    // Zero header writes its local result and returns through two distinct
    // RET slots. All six access rows are independently taken from the LST.
    constexpr std::array<u32, 6U> null_ips{
        0x00488F7DU,
        0x00488F84U,
        0x00488F89U,
        0x00488F8AU,
        0x00489014U,
        0x00489015U
    };
    constexpr std::array<u32, 6U> null_esp_offsets{
        84U, 84U, 80U, 76U, 56U, 52U
    };
    constexpr std::array<u32, 6U> null_ebp_offsets{
        80U, 80U, 80U, 56U, 56U, 28U
    };
    bool null_returns_exact = true;
    std::size_t null_fault_count{};
    for (const bool df : {false, true}) {
        for (const u32 debug : {0U, 1U, 4U}) {
            LegacyBattleActorFrameEntryRequest request{};
            request.decoder_heap_debug_flags_owner = &debug;
            request.decoder_heap_alloc_owner = &hook;
            LegacyBattleActorFrameEntryResult before{};
            before.status =
                LegacyBattleActorFrameEntryStatus::case_two_release_call_ready;
            before.eip = 0x00479C93U;
            before.eax = 0x20U;
            before.ecx = 0xDECAF001U;
            before.edx = 0xE5541234U;
            before.ebp = 0x88996677U;
            before.esi = 0x00525508U;
            before.edi = 0x12121212U;
            before.esp = 0x001AFE00U;
            before.accesses_completed = 41U;
            before.direction_flag = df;
            const u32 saved_ecx = debug == 4U ? 0U : before.ecx;
            const std::size_t start =
                before.accesses_completed + 42U + (debug == 4U ? 12U : 0U);
            for (std::size_t i = 0U; i < null_ips.size(); ++i) {
                request.stop_before_access = start + i;
                OpaqueRelease port;
                const auto stopped =
                    continue_legacy_battle_actor_frame_case_two_release_call(
                        port, request, before
                    );
                const LegacyBattleActorCoordinateFlags expected_flags{
                    .parity = true,
                    .auxiliary_carry_defined = i < 4U,
                    .zero = true,
                };
                null_returns_exact = null_returns_exact &&
                    stopped.status ==
                        (i == 0U ? LegacyBattleActorFrameEntryStatus::
                                       stack_write_typed_stop
                                 : LegacyBattleActorFrameEntryStatus::
                                       stack_read_typed_stop) &&
                    stopped.eip == null_ips[i] &&
                    stopped.stopped_instruction == null_ips[i] &&
                    stopped.stopped_access_kind ==
                        (i == 0U ? LegacyBattleActorFrameEntryAccessKind::
                                       stack_write
                                 : LegacyBattleActorFrameEntryAccessKind::
                                       stack_read) &&
                    stopped.stopped_token == before.esp - null_esp_offsets[i] &&
                    stopped.esp == before.esp - null_esp_offsets[i] &&
                    stopped.ebp == before.esp - null_ebp_offsets[i] &&
                    stopped.eax == 0U && stopped.ecx == saved_ecx &&
                    stopped.edx == 0x20U && stopped.ebx == before.ebx &&
                    stopped.esi == before.esi && stopped.edi == before.edi &&
                    stopped.last_pushed_value == saved_ecx &&
                    stopped.accesses_completed == start + i &&
                    stopped.flags_known &&
                    same_flags(stopped.flags, expected_flags) &&
                    stopped.direction_flag == df && !stopped.returned &&
                    !stopped.release_child.returned &&
                    stopped.release_calls == 1U && port.calls == 0U;
                ++null_fault_count;
            }

            request.stop_before_access = start + null_ips.size();
            OpaqueRelease assertion_port;
            const auto assertion_stop =
                continue_legacy_battle_actor_frame_case_two_release_call(
                    assertion_port, request, before
                );
            null_returns_exact = null_returns_exact &&
                assertion_stop.status ==
                    LegacyBattleActorFrameEntryStatus::stack_write_typed_stop &&
                assertion_stop.eip == 0x00488668U &&
                assertion_stop.stopped_instruction == assertion_stop.eip &&
                assertion_stop.stopped_access_kind ==
                    LegacyBattleActorFrameEntryAccessKind::stack_write &&
                assertion_stop.stopped_token == before.esp - 48U &&
                assertion_stop.esp == before.esp - 44U &&
                assertion_stop.ebp == before.esp - 28U &&
                assertion_stop.eax == 0U && assertion_stop.ecx == saved_ecx &&
                assertion_stop.edx == 0x20U &&
                assertion_stop.ebx == before.ebx &&
                assertion_stop.esi == before.esi &&
                assertion_stop.edi == before.edi &&
                assertion_stop.last_pushed_value == saved_ecx &&
                assertion_stop.accesses_completed == start + null_ips.size() &&
                assertion_stop.flags_known &&
                same_flags(assertion_stop.flags,
                           {.parity = true,
                            .auxiliary_carry_defined = false,
                            .zero = true}) &&
                assertion_stop.direction_flag == df &&
                !assertion_stop.returned &&
                !assertion_stop.release_child.returned &&
                assertion_port.calls == 0U;

            // A held hook can equal this new frontier without having run
            // either validator RET. Its incoming ESP still differs by 32.
            constexpr u32 held_assertion_address = 0x00488668U;
            request.decoder_heap_alloc_owner = &held_assertion_address;
            request.stop_before_access = static_cast<std::size_t>(-1);
            OpaqueRelease collision_port;
            const auto collision =
                continue_legacy_battle_actor_frame_case_two_release_call(
                    collision_port, request, before
                );
            null_returns_exact = null_returns_exact &&
                collision.status ==
                    LegacyBattleActorFrameEntryStatus::
                        case_two_release_child_typed_stop &&
                collision.eip == held_assertion_address &&
                collision.stopped_instruction == collision.eip &&
                collision.stopped_access_kind ==
                    LegacyBattleActorFrameEntryAccessKind::callee_call &&
                collision.stopped_token == 0U &&
                collision.esp == before.esp - 76U &&
                collision.ebp == before.esp - 28U && collision.eax == 0x20U &&
                collision.ecx == saved_ecx && collision.edx == 1U &&
                collision.ebx == before.ebx && collision.esi == before.esi &&
                collision.edi == before.edi &&
                collision.last_pushed_value == 0x00488626U &&
                collision.accesses_completed ==
                    before.accesses_completed + 25U +
                        (debug == 4U ? 12U : 0U) &&
                collision.flags_known && same_flags(collision.flags, {}) &&
                collision.direction_flag == df && !collision.returned &&
                !collision.release_child.returned && collision_port.calls == 1U;
        }
    }

    test.expect_true(
        null_fault_count == 36U && null_returns_exact,
        "null header has six independent local/POP/RET faults per DF/debug combination and cannot skip to complete free after returning zero"
    );
}
