#include "openswd3/battle/legacy_battle_actor_action_presentation.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/battle/legacy_battle_startup.hpp"
#include "openswd3/rendering/legacy_blitter.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace openswd3::battle {
namespace {

using compat::i16;
using compat::i32;
using compat::u8;
using compat::u16;
using compat::u32;

constexpr u32 kNullsub1Address = 0x0044A240U;

struct CallReply {
    u32 eax{};
    u32 ecx{};
    u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
    std::array<u32, 8> outputs{};
};

[[nodiscard]] constexpr u16 low_word(const u32 value) noexcept {
    return static_cast<u16>(value);
}

[[nodiscard]] constexpr u32
replace_low_word(const u32 value, const u16 word) noexcept {
    return (value & 0xFFFF0000U) | word;
}

[[nodiscard]] constexpr i32 signed_word(const u16 value) noexcept {
    return static_cast<i32>(std::bit_cast<i16>(value));
}

[[nodiscard]] constexpr bool even_parity(const u8 value) noexcept {
    return (std::popcount(value) & 1) == 0;
}

[[nodiscard]] constexpr LegacyBattleActorCoordinateFlags
subtract_flags(const u32 left, const u32 right) noexcept {
    const u32 value = left - right;
    return {
        .carry = left < right,
        .parity = even_parity(static_cast<u8>(value)),
        .auxiliary_carry = ((left ^ right ^ value) & 0x10U) != 0U,
        .auxiliary_carry_defined = true,
        .zero = value == 0U,
        .sign = (value & 0x80000000U) != 0U,
        .overflow = ((left ^ right) & (left ^ value) & 0x80000000U) != 0U,
    };
}

[[nodiscard]] bool
valid_actor(const LegacyBattleActorRuntimeResetView& actor) noexcept {
    return actor.residual != nullptr && actor.progress != nullptr &&
        actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
}

template <typename Value>
[[nodiscard]] Value
load_image(const LegacyBattleActorImage& image, const u32 offset) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    Value value{};
    std::memcpy(&value, image.data() + offset, sizeof(Value));
    return value;
}

template <typename Value>
void store_image(
    LegacyBattleActorImage& image, const u32 offset, const Value value
) noexcept {
    static_assert(std::is_trivially_copyable_v<Value>);
    std::memcpy(image.data() + offset, &value, sizeof(Value));
}

void publish_resource(
    LegacyBattleGroupAActionResourceRecord& resource, const CallReply& reply
) noexcept {
    resource.token = reply.eax;
    resource.value_04 = reply.outputs[3U];
    resource.value_0c = low_word(reply.outputs[1U]);
    resource.value_0e = low_word(reply.outputs[2U]);
}

}  // namespace

LegacyBattleActorActionPresentationPhysicalCallArray::
    LegacyBattleActorActionPresentationPhysicalCallArray(
        const LegacyBattleActorActionPresentationPhysicalCallArray& other
    ) {
    if (other.calls_ != nullptr) {
        calls_ = std::make_unique<Storage>(*other.calls_);
    }
}

LegacyBattleActorActionPresentationPhysicalCallArray&
LegacyBattleActorActionPresentationPhysicalCallArray::operator=(
    const LegacyBattleActorActionPresentationPhysicalCallArray& other
) {
    if (this == &other) {
        return *this;
    }
    if (other.calls_ == nullptr) {
        calls_.reset();
        return *this;
    }
    calls_ = std::make_unique<Storage>(*other.calls_);
    return *this;
}

LegacyBattleActorActionPresentationPhysicalCall&
LegacyBattleActorActionPresentationPhysicalCallArray::operator[](
    const std::size_t index
) {
    if (calls_ == nullptr) {
        calls_ = std::make_unique<Storage>();
    }
    return (*calls_)[index];
}

const LegacyBattleActorActionPresentationPhysicalCall&
LegacyBattleActorActionPresentationPhysicalCallArray::operator[](
    const std::size_t index
) const noexcept {
    if (calls_ == nullptr) {
        static constexpr LegacyBattleActorActionPresentationPhysicalCall
            empty{};
        return empty;
    }
    return (*calls_)[index];
}

LegacyBattleActorActionPresentationRequestArray::
    LegacyBattleActorActionPresentationRequestArray(
        const LegacyBattleActorActionPresentationRequestArray& other
    ) {
    if (other.requests_ != nullptr) {
        requests_ = std::make_unique<Storage>(*other.requests_);
    }
}

LegacyBattleActorActionPresentationRequestArray&
LegacyBattleActorActionPresentationRequestArray::operator=(
    const LegacyBattleActorActionPresentationRequestArray& other
) {
    if (this == &other) {
        return *this;
    }
    if (other.requests_ == nullptr) {
        requests_.reset();
        return *this;
    }
    requests_ = std::make_unique<Storage>(*other.requests_);
    return *this;
}

LegacyBattleActorActionPresentationRequest&
LegacyBattleActorActionPresentationRequestArray::operator[](
    const std::size_t index
) {
    if (requests_ == nullptr) {
        requests_ = std::make_unique<Storage>();
    }
    return (*requests_)[index];
}

const LegacyBattleActorActionPresentationRequest&
LegacyBattleActorActionPresentationRequestArray::operator[](
    const std::size_t index
) const noexcept {
    if (requests_ == nullptr) {
        static constexpr LegacyBattleActorActionPresentationRequest empty{};
        return empty;
    }
    return (*requests_)[index];
}

LegacyBattleActorActionPresentationView
resolve_legacy_battle_actor_action_presentation(
    const LegacyBattleActorActionPresentationOwners& owners,
    const u32 actor_token
) noexcept {
    return {
        .actor = resolve_legacy_battle_actor_runtime_reset(
            {.action = owners.action, .startup = owners.startup}, actor_token
        ),
        .shared = owners.action == nullptr
            ? nullptr
            : &owners.action->group_a_action_shared,
    };
}

LegacyBattleActorActionPresentationResult
advance_legacy_battle_actor_action_presentation(
    const LegacyBattleActorActionPresentationView actor,
    const LegacyBattleActorActionPresentationPlatform platform,
    const LegacyBattleActorActionPresentationRequest& request
) noexcept {
    LegacyBattleActorActionPresentationResult result{
        .return_eax = request.entry_eax,
        .return_ecx = request.actor_token,
        .return_edx = request.entry_edx,
        .return_ebx = request.entry_ebx,
        .return_ebp = request.entry_ebp,
        .return_esi = request.entry_esi,
        .return_edi = request.entry_edi,
        .return_esp = request.entry_esp,
        .flags = request.entry_flags,
        .flags_known = request.entry_flags_known,
    };
    LegacyBattleActorImage image{};
    materialize_legacy_battle_actor_image(actor.actor, image);

    u32 eax = request.entry_eax;
    u32 ecx = request.actor_token;
    u32 edx = request.entry_edx;
    u32 ebx = 1U;
    u32 ebp = request.actor_token;
    u32 esi = request.effect_argument;
    u32 edi = 0U;
    u32 esp = request.entry_esp - 16U;
    u32 eip = kLegacyBattleActorActionPresentationAddress;
    auto flags = request.entry_flags;
    bool flags_known = request.entry_flags_known;

    const auto finish = [&]() {
        result.return_eax = eax;
        result.return_ecx = ecx;
        result.return_edx = edx;
        result.return_ebx = ebx;
        result.return_ebp = ebp;
        result.return_esi = esi;
        result.return_edi = edi;
        result.return_esp = esp;
        result.return_eip = eip;
        result.flags = flags;
        result.flags_known = flags_known;
        return result;
    };

    const auto stop_access = [&](const auto kind,
                                 const u32 instruction,
                                 const u32 token) {
        result.stopped_access_ordinal = result.actor_accesses_completed;
        result.stopped_access_kind = kind;
        result.stopped_instruction = instruction;
        result.stopped_token = token;
        eip = instruction;
        switch (kind) {
        case LegacyBattleActorActionPresentationAccessKind::actor_read:
            result.status = LegacyBattleActorActionPresentationStatus::
                actor_read_typed_stop;
            break;

        case LegacyBattleActorActionPresentationAccessKind::actor_write:
            result.status = LegacyBattleActorActionPresentationStatus::
                actor_write_typed_stop;
            break;

        case LegacyBattleActorActionPresentationAccessKind::nested_record_read:
            result.status = LegacyBattleActorActionPresentationStatus::
                nested_record_read_typed_stop;
            break;

        case LegacyBattleActorActionPresentationAccessKind::
            resource_record_read:
            result.status = LegacyBattleActorActionPresentationStatus::
                resource_record_read_typed_stop;
            break;

        case LegacyBattleActorActionPresentationAccessKind::return_address_read:
            result.status = LegacyBattleActorActionPresentationStatus::
                return_address_read_typed_stop;
            break;
        }
    };

    const auto touch = [&](const auto kind,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible = true) {
        const bool actor_memory =
            kind == LegacyBattleActorActionPresentationAccessKind::actor_read ||
            kind == LegacyBattleActorActionPresentationAccessKind::actor_write;
        if (result.actor_accesses_completed ==
                request.stop_before_actor_access ||
            (actor_memory && !valid_actor(actor.actor)) || !accessible) {
            stop_access(kind, instruction, token);
            return false;
        }

        ++result.actor_accesses_completed;
        switch (kind) {
        case LegacyBattleActorActionPresentationAccessKind::actor_read:
            ++result.actor_reads;
            break;

        case LegacyBattleActorActionPresentationAccessKind::actor_write:
            ++result.actor_writes;
            break;

        case LegacyBattleActorActionPresentationAccessKind::nested_record_read:
            ++result.nested_record_reads;
            break;

        case LegacyBattleActorActionPresentationAccessKind::
            resource_record_read:
            ++result.resource_record_reads;
            break;

        case LegacyBattleActorActionPresentationAccessKind::return_address_read:
            break;
        }
        return true;
    };

    const auto read_u8 =
        [&](const u32 instruction, const u32 offset, u8& value) {
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset
                )) {
                return false;
            }
            value = load_image<u8>(image, offset);
            return true;
        };
    const auto read_u16 =
        [&](const u32 instruction, const u32 offset, u16& value) {
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset
                )) {
                return false;
            }
            value = load_image<u16>(image, offset);
            return true;
        };
    const auto read_u32 =
        [&](const u32 instruction, const u32 offset, u32& value) {
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::actor_read,
                    instruction,
                    request.actor_token + offset
                )) {
                return false;
            }
            value = load_image<u32>(image, offset);
            return true;
        };
    const auto write_u16 =
        [&](const u32 instruction, const u32 offset, const u16 value) {
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::actor_write,
                    instruction,
                    request.actor_token + offset
                )) {
                return false;
            }
            store_image(image, offset, value);
            synchronize_legacy_battle_actor_image_write(
                actor.actor, image, offset, sizeof(value)
            );
            return true;
        };
    const auto write_u32 =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::actor_write,
                    instruction,
                    request.actor_token + offset
                )) {
                return false;
            }
            store_image(image, offset, value);
            synchronize_legacy_battle_actor_image_write(
                actor.actor, image, offset, sizeof(value)
            );
            return true;
        };
    const auto read_nested_word =
        [&](const u32 instruction, const u32 offset, u16& value) {
            const bool accessible = actor.actor.live_record_bytes != nullptr &&
                offset + sizeof(value) <= actor.actor.live_record_size;
            if (!touch(
                    LegacyBattleActorActionPresentationAccessKind::
                        nested_record_read,
                    instruction,
                    actor.actor.live_record_token + offset,
                    accessible
                )) {
                return false;
            }
            std::memcpy(
                &value, actor.actor.live_record_bytes + offset, sizeof(value)
            );
            return true;
        };
    const auto read_resource = [&](const std::size_t resource_index,
                                   const u32 instruction,
                                   const u32 token,
                                   const u32 offset,
                                   const u32 value,
                                   u32& output) {
        if (!touch(
                LegacyBattleActorActionPresentationAccessKind::
                    resource_record_read,
                instruction,
                token + offset,
                request.resource_record_readable[resource_index] && token != 0U
            )) {
            return false;
        }
        output = value;
        return true;
    };

    const auto begin_call = [&](const u32 call_address,
                                const u32 return_address,
                                const u32 callee,
                                const std::initializer_list<u32> arguments,
                                std::size_t& index) {
        if (result.physical_call_count == request.stop_before_call) {
            result.status =
                LegacyBattleActorActionPresentationStatus::call_typed_stop;
            result.stopped_call_ordinal = result.physical_call_count;
            eip = call_address;
            return false;
        }
        index = result.physical_call_count++;
        auto& call = result.physical_calls[index];
        call.call_address = call_address;
        call.return_address = return_address;
        call.callee_token = callee;
        call.argument_count = static_cast<u32>(arguments.size());
        std::copy(arguments.begin(), arguments.end(), call.arguments.begin());
        call.entry_eax = eax;
        call.entry_ecx = ecx;
        call.entry_edx = edx;
        return true;
    };

    const auto invoke_port = [&](const u32 call_address,
                                 const u32 return_address,
                                 const u32 callee,
                                 const std::initializer_list<u32> arguments,
                                 CallReply& reply) {
        std::size_t index{};
        if (!begin_call(
                call_address, return_address, callee, arguments, index
            )) {
            return false;
        }
        LegacyBattleActionCallRequest call_request{};
        call_request.callee_token = callee;
        std::copy(
            arguments.begin(), arguments.end(), call_request.arguments.begin()
        );
        call_request.eax = eax;
        call_request.ecx = ecx;
        call_request.edx = edx;
        const auto port_reply = platform.port->invoke(call_request);
        ++result.port_calls;
        reply = {
            .eax = port_reply.eax,
            .ecx = port_reply.ecx,
            .edx = port_reply.edx,
            .flags = port_reply.flags,
            .outputs = port_reply.outputs,
        };
        eax = reply.eax;
        ecx = reply.ecx;
        edx = reply.edx;
        flags = reply.flags;
        flags_known = true;
        auto& trace = result.physical_calls[index];
        trace.return_eax = eax;
        trace.return_ecx = ecx;
        trace.return_edx = edx;
        trace.outputs = reply.outputs;
        return true;
    };

    const auto record_null_call = [&](const u32 call_address,
                                      const u32 return_address) {
        std::size_t index{};
        if (!begin_call(
                call_address, return_address, kNullsub1Address, {}, index
            )) {
            return false;
        }
        auto& trace = result.physical_calls[index];
        trace.return_eax = eax;
        trace.return_ecx = ecx;
        trace.return_edx = edx;
        return true;
    };

    const auto return_from = [&](const u32 exit_instruction) {
        result.exit_instruction = exit_instruction;
        edi = request.entry_edi;
        esi = request.entry_esi;
        ebp = request.entry_ebp;
        ebx = request.entry_ebx;
        esp = request.entry_esp;
        if (!request.return_address_readable) {
            stop_access(
                LegacyBattleActorActionPresentationAccessKind::
                    return_address_read,
                exit_instruction,
                esp
            );
            return false;
        }
        esp += 8U;
        eip = request.entry_return_address;
        result.returned = true;
        return true;
    };

    if (!write_u32(0x00478B71U, 0x2AF4U, esi)) {
        return finish();
    }

    u16 word{};
    if (!read_u16(0x00478B77U, 0x26D6U, word)) {
        return finish();
    }
    if (word > 0U) {
        u16 mode{};
        if (!read_u16(0x00478B86U, 0x26D8U, mode)) {
            return finish();
        }
        if (mode == 3U) {
            u16 position_y{};
            u16 alternate_y{};
            if (!read_u16(0x00478B9AU, 0x0D68U, position_y)) {
                return finish();
            }
            position_y = static_cast<u16>(position_y + 30U);
            if (!write_u16(0x00478B9AU, 0x0D68U, position_y) ||
                !read_u16(0x00478BA9U, 0x0D88U, alternate_y)) {
                return finish();
            }
            if (signed_word(position_y) >= signed_word(alternate_y)) {
                if (!write_u16(0x00478BB9U, 0x0D68U, alternate_y) ||
                    !write_u16(0x00478BC0U, 0x26D6U, 0U) ||
                    !write_u16(0x00478BC7U, 0x26D8U, 0U) ||
                    !write_u32(0x00478BD0U, 0x2B10U, 1U)) {
                    return finish();
                }
                ecx = request.actor_token;
                std::size_t index{};
                if (!begin_call(
                        0x00478BD6U,
                        0x00478BDBU,
                        kLegacyBattleActorField26b8HighBitSetAddress,
                        {},
                        index
                    )) {
                    return finish();
                }
                auto child_request = request.high_bit_set_request;
                child_request.actor_token = request.actor_token;
                child_request.entry_eax = eax;
                child_request.entry_edx = edx;
                child_request.entry_esp = esp - 4U;
                child_request.entry_return_address = 0x00478BDBU;
                child_request.entry_flags = flags;
                child_request.entry_flags_known = flags_known;
                result.high_bit_set =
                    set_legacy_battle_actor_field_26b8_high_bit(
                        resolve_legacy_battle_actor_field_26b8_high_bit_set(
                            actor.actor.action_execution, request.actor_token
                        ),
                        child_request
                    );
                auto& trace = result.physical_calls[index];
                trace.return_eax = result.high_bit_set.return_eax;
                trace.return_ecx = result.high_bit_set.return_ecx;
                trace.return_edx = result.high_bit_set.return_edx;
                eax = result.high_bit_set.return_eax;
                ecx = result.high_bit_set.return_ecx;
                edx = result.high_bit_set.return_edx;
                flags = result.high_bit_set.flags;
                flags_known = result.high_bit_set.flags_known;
                if (!result.high_bit_set.returned) {
                    result.status = LegacyBattleActorActionPresentationStatus::
                        high_bit_set_typed_stop;
                    esp = result.high_bit_set.return_esp;
                    eip = result.high_bit_set.return_eip;
                    return finish();
                }
                materialize_legacy_battle_actor_image(actor.actor, image);
            }
        } else if (mode == 2U) {
            u16 position_y{};
            if (!read_u16(0x00478BDDU, 0x0D68U, position_y)) {
                return finish();
            }
            position_y = static_cast<u16>(position_y + 0xFFF6U);
            if (!write_u16(0x00478BDDU, 0x0D68U, position_y)) {
                return finish();
            }
            if (signed_word(position_y) <= 0) {
                if (!write_u16(0x00478BEEU, 0x0D68U, 0U) ||
                    !write_u16(0x00478BF5U, 0x26D6U, 0U) ||
                    !write_u16(0x00478BFCU, 0x26D8U, 0U)) {
                    return finish();
                }
            }
        } else if (mode == 1U) {
            CallReply random{};
            if (!invoke_port(
                    0x00478C07U, 0x00478C0CU, 0x00439070U, {2U}, random
                )) {
                return finish();
            }
            if (low_word(eax) == 1U) {
                u32 source_runtime{};
                u32 mirror{};
                u16 position_x{};
                if (!read_u32(0x00478C14U, 0x2AA0U, source_runtime) ||
                    !read_u32(0x00478C1CU, 0x2B08U, mirror) ||
                    !read_u16(0x00478C28U, 0x0D66U, position_x)) {
                    return finish();
                }
                const bool decrement =
                    source_runtime == 1U ? mirror != 0U : mirror == 0U;
                position_x = decrement ? static_cast<u16>(position_x - 1U)
                                       : static_cast<u16>(position_x + 1U);
                if (!write_u16(
                        decrement ? 0x00478C28U : 0x00478C3EU,
                        0x0D66U,
                        position_x
                    )) {
                    return finish();
                }
            }
            u16 completion{};
            if (!read_u16(0x00478C45U, 0x26D6U, completion) ||
                !write_u16(
                    0x00478C45U, 0x26D6U, static_cast<u16>(completion - 1U)
                )) {
                return finish();
            }
        }
    }

    if (!write_u32(0x00478C4EU, 0x0330U, 0U)) {
        return finish();
    }
    u8 byte{};
    if (request.effect_argument == 1U) {
        if (!write_u32(0x00478C5FU, 0x0330U, 1U)) {
            return finish();
        }
    } else {
        if (!read_u8(0x00478C56U, 0x26D0U, byte)) {
            return finish();
        }
        if ((byte & 0x40U) != 0U && !write_u32(0x00478C5FU, 0x0330U, 1U)) {
            return finish();
        }
    }
    if (!read_u16(0x00478C6AU, 0x02FAU, word)) {
        return finish();
    }
    if ((word & 0x0200U) != 0U && !write_u32(0x00478C73U, 0x0330U, 1U)) {
        return finish();
    }

    u32 field_2ab8{};
    u32 source_runtime{};
    u32 motion_mode{};
    u32 scene_identity{};
    u32 script_state{};
    if (!read_u32(0x00478C79U, 0x2AB8U, field_2ab8)) {
        return finish();
    }
    if (field_2ab8 == 0U) {
        if (!read_u32(0x00478C81U, 0x2AA0U, source_runtime)) {
            return finish();
        }
        bool gate = source_runtime == 0U;
        if (!gate) {
            if (!read_u32(0x00478C89U, 0x2B00U, motion_mode)) {
                return finish();
            }
            if (motion_mode == 1U) {
                if (!read_u32(0x00478C91U, 0x2B04U, scene_identity)) {
                    return finish();
                }
                gate = scene_identity == 1U;
            }
        }
        if (gate) {
            if (!read_u32(0x00478C99U, 0x2AF8U, script_state)) {
                return finish();
            }
            if (script_state == 0U) {
                static_cast<void>(return_from(0x00479849U));
                return finish();
            }
        }
    }

    u32 field_26b8{};
    if (!read_u32(0x00478CA5U, 0x26B8U, field_26b8)) {
        return finish();
    }
    if ((field_26b8 & 0x80000000U) != 0U) {
        ecx = request.actor_token;
        CallReply query{};
        if (!invoke_port(0x00478CB3U, 0x00478CB8U, 0x0047BA80U, {}, query)) {
            return finish();
        }
        flags = subtract_flags(eax, 1U);
        flags_known = true;
        if (eax != 1U) {
            static_cast<void>(return_from(0x00479849U));
            return finish();
        }
        ecx = request.actor_token;
        if (!write_u32(0x00478CC2U, 0x2AC4U, 0U)) {
            return finish();
        }
        std::size_t index{};
        if (!begin_call(
                0x00478CC8U,
                0x00478CCDU,
                kLegacyBattleActorField26b8HighBitClearAddress,
                {},
                index
            )) {
            return finish();
        }
        auto child_request = request.high_bit_clear_request;
        child_request.actor_token = request.actor_token;
        child_request.entry_eax = eax;
        child_request.entry_edx = edx;
        child_request.entry_esp = esp - 4U;
        child_request.entry_return_address = 0x00478CCDU;
        child_request.entry_flags = flags;
        child_request.entry_flags_known = flags_known;
        result.high_bit_clear = clear_legacy_battle_actor_field_26b8_high_bit(
            {.field_26b8 = &actor.actor.action_execution->field_26b8},
            child_request
        );
        auto& trace = result.physical_calls[index];
        trace.return_eax = result.high_bit_clear.return_eax;
        trace.return_ecx = result.high_bit_clear.return_ecx;
        trace.return_edx = result.high_bit_clear.return_edx;
        eax = result.high_bit_clear.return_eax;
        ecx = result.high_bit_clear.return_ecx;
        edx = result.high_bit_clear.return_edx;
        flags = result.high_bit_clear.flags;
        flags_known = result.high_bit_clear.flags_known;
        if (!result.high_bit_clear.returned) {
            result.status = LegacyBattleActorActionPresentationStatus::
                high_bit_clear_typed_stop;
            esp = result.high_bit_clear.return_esp;
            eip = result.high_bit_clear.return_eip;
            return finish();
        }
        static_cast<void>(return_from(0x00478CD1U));
        return finish();
    }

    u32 completion_latch{};
    if (!read_u32(0x00478CD4U, 0x2AACU, completion_latch)) {
        return finish();
    }
    if (completion_latch == 1U) {
        static_cast<void>(return_from(0x00479849U));
        return finish();
    }

    u16 profile_value{};
    if (!read_u16(0x00478CE2U, 0x2A0CU, profile_value) ||
        !write_u32(0x00478CE9U, 0x02A0U, profile_value) ||
        !write_u32(0x00478CEFU, 0x02A8U, 0x24U) ||
        !write_u16(0x00478D00U, 0x2A88U, 0x24U) ||
        !read_u32(0x00478D07U, 0x2AA0U, source_runtime)) {
        return finish();
    }
    if (source_runtime == 1U) {
        ecx = request.actor_token;
        if (!record_null_call(0x00478D13U, 0x00478D18U)) {
            return finish();
        }
    } else {
        u32 variant_delta{};
        u32 action_id{};
        if (!read_u32(0x00478D1AU, 0x2684U, variant_delta) ||
            !read_u32(0x00478D20U, 0x02A8U, action_id) ||
            !write_u32(0x00478D28U, 0x02A8U, action_id + variant_delta)) {
            return finish();
        }
    }

    if (source_runtime == 1U) {
        if (!read_u32(0x00478D36U, 0x2B00U, motion_mode) ||
            !read_u32(0x00478D3EU, 0x2B04U, scene_identity)) {
            return finish();
        }
        if (motion_mode == 0U && scene_identity == 0U) {
            u32 live_token{};
            u16 live_word_0a{};
            u16 live_word_04{};
            if (!read_u32(0x00478D46U, 0x0004U, live_token) ||
                !read_nested_word(0x00478D49U, 0x0AU, live_word_0a) ||
                !read_nested_word(0x00478D4DU, 0x04U, live_word_04)) {
                return finish();
            }
            const i32 numerator = signed_word(live_word_0a);
            const i32 floor = (numerator + (numerator < 0 ? 3 : 0)) >> 2;
            if (signed_word(live_word_04) <= floor &&
                !write_u32(0x00478D5EU, 0x02A8U, 0x26U)) {
                return finish();
            }
            static_cast<void>(live_token);
        }
    }

    u16 override_flags{};
    u32 idle_latch{};
    u32 field_2ac8{};
    if (!read_u16(0x00478D68U, 0x2A86U, override_flags)) {
        return finish();
    }
    if (override_flags != 0U) {
        if (!read_u32(0x00478D7AU, 0x2AB4U, idle_latch) ||
            !read_u32(0x00478D86U, 0x2AC8U, field_2ac8)) {
            return finish();
        }
        if (idle_latch != 1U && field_2ac8 != 1U) {
            if (!read_u32(0x00478D92U, 0x2B00U, motion_mode)) {
                return finish();
            }
            if (motion_mode != 1U && !write_u32(0x00478D9AU, 0x2AF0U, 0U)) {
                return finish();
            }
            if (!read_u32(0x00478DA0U, 0x2B04U, scene_identity)) {
                return finish();
            }
            if (scene_identity != 1U) {
                if (!write_u16(0x00478DA8U, 0x0D66U, 0x0140U) ||
                    !write_u16(0x00478DB1U, 0x0D68U, 0x0136U)) {
                    return finish();
                }
            }
            if (!write_u32(0x00478DBAU, 0x2670U, 0U)) {
                return finish();
            }
            if (source_runtime == 1U && !write_u32(0x00478DCAU, 0x2674U, 0U)) {
                return finish();
            }
            if (!write_u32(0x00478DD0U, 0x02A8U, 0x2CU)) {
                return finish();
            }
            if (profile_value == 0x1CU) {
                u32 alternate_action{};
                if (!read_u32(0x00478DE4U, 0x2690U, alternate_action) ||
                    !write_u32(0x00478DEAU, 0x02A8U, alternate_action)) {
                    return finish();
                }
            }
            u32 selected_action{};
            if (!read_u32(0x00478DF0U, 0x02A8U, selected_action) ||
                !write_u16(0x00478DF7U, 0x2A88U, low_word(selected_action))) {
                return finish();
            }
            const u8 high = static_cast<u8>(override_flags >> 8U);
            if ((high & 0x40U) != 0U &&
                !write_u32(0x00478E0CU, 0x02A8U, 0x2DU)) {
                return finish();
            }
            if ((high & 0x20U) != 0U &&
                !write_u32(0x00478E1BU, 0x02A8U, 0x2DU)) {
                return finish();
            }
            if ((high & 0x80U) != 0U) {
                u32 record_gate{};
                if (!read_u32(0x00478E2AU, 0x032CU, record_gate)) {
                    return finish();
                }
                if (record_gate == 1U && source_runtime == 1U) {
                    u16 record_word{};
                    if (!write_u32(0x00478E3AU, 0x032CU, 0U) ||
                        !read_u16(0x00478E40U, 0x02E2U, record_word) ||
                        !write_u16(
                            0x00478E40U,
                            0x02E2U,
                            static_cast<u16>(record_word + 0xFFFBU)
                        )) {
                        return finish();
                    }
                }
            }
            if ((high & 0x04U) != 0U &&
                !write_u32(0x00478E50U, 0x02A8U, 0x2DU)) {
                return finish();
            }
            if ((high & 0x08U) != 0U &&
                !write_u32(0x00478E5FU, 0x02A8U, 0x31U)) {
                return finish();
            }
            if ((override_flags & 0x0200U) != 0U) {
                static_cast<void>(return_from(0x00479849U));
                return finish();
            }
        }
    }

    if (!read_u32(0x00478E76U, 0x2B00U, motion_mode) ||
        !read_u32(0x00478E89U, 0x2B04U, scene_identity)) {
        return finish();
    }
    if (motion_mode == 0U && scene_identity == 0U) {
        u16 presentation_kind{};
        if (!read_u16(0x00478E96U, 0x26D2U, presentation_kind)) {
            return finish();
        }
        if (presentation_kind == 1U) {
            if (!write_u32(0x00478E9FU, 0x2AD4U, 0U) ||
                !write_u32(0x00478EA5U, 0x02A8U, 0x48U)) {
                return finish();
            }
        }
        if (presentation_kind == 2U) {
            if (!write_u32(0x00478EB9U, 0x02A8U, 0x49U)) {
                return finish();
            }
            u32 record_gate{};
            if (!read_u32(0x00478EC3U, 0x032CU, record_gate)) {
                return finish();
            }
            if (record_gate == 1U) {
                u16 record_word{};
                if (!write_u32(0x00478ECDU, 0x032CU, 0U) ||
                    !read_u16(0x00478ED3U, 0x02E2U, record_word) ||
                    !write_u16(
                        0x00478ED3U,
                        0x02E2U,
                        static_cast<u16>(record_word + 0xFFFBU)
                    ) ||
                    !write_u16(0x00478EDAU, 0x02F8U, 0U)) {
                    return finish();
                }
            }
        }
        if (presentation_kind == 4U &&
            !write_u32(0x00478EEBU, 0x02A8U, 0x4AU)) {
            return finish();
        }
        if (presentation_kind == 8U &&
            !write_u32(0x00478EFEU, 0x02A8U, 0x4BU)) {
            return finish();
        }
    }

    if (!read_u32(0x00478F0FU, 0x2AA0U, source_runtime) ||
        !read_u32(0x00478F17U, 0x2AB8U, field_2ab8) ||
        !read_u32(0x00478F1FU, 0x2B00U, motion_mode) ||
        !read_u32(0x00478F27U, 0x2B04U, scene_identity)) {
        return finish();
    }
    if (source_runtime == 1U && field_2ab8 == 1U && motion_mode == 0U &&
        scene_identity == 0U && !write_u32(0x00478F2FU, 0x02A8U, 0x33U)) {
        return finish();
    }
    u16 variant_override{};
    if (!read_u16(0x00478F39U, 0x2A0EU, variant_override)) {
        return finish();
    }
    if (variant_override != 0U &&
        !write_u32(0x00478F4AU, 0x02A8U, variant_override)) {
        return finish();
    }

    u8 snapshot_flags{};
    if (!read_u8(0x00478F50U, 0x02FAU, snapshot_flags)) {
        return finish();
    }
    if ((snapshot_flags & 0x08U) != 0U) {
        if (!read_u16(0x00478F58U, 0x2A86U, override_flags)) {
            return finish();
        }
        const bool preserve_gate = (override_flags & 0x8000U) != 0U &&
            (override_flags & 0x4000U) == 0U;
        u16 display_kind{};
        if (!read_u16(
                preserve_gate ? 0x00478F69U : 0x00478F89U, 0x2A70U, display_kind
            ) ||
            !write_u16(
                preserve_gate ? 0x00478F70U : 0x00478F90U, 0x2A6CU, display_kind
            ) ||
            !write_u16(
                preserve_gate ? 0x00478F77U : 0x00478F97U, 0x02FAU, 0U
            )) {
            return finish();
        }
        if (!preserve_gate && !write_u32(0x00478F9EU, 0x0330U, 0U)) {
            return finish();
        }
        if (!read_u8(0x00478FA4U, 0x0D94U, byte)) {
            return finish();
        }
        if ((byte & 4U) != 0U) {
            u16 mode_gate{};
            if (!read_u16(0x00478FAEU, 0x26D0U, mode_gate) ||
                !write_u16(
                    0x00478FAEU, 0x26D0U, static_cast<u16>(mode_gate | 8U)
                )) {
                return finish();
            }
            for (u32 index = 0U; index < 0x26U; ++index) {
                u32 copied{};
                const u32 source = 0x02A0U + index * 4U;
                const u32 destination = 0x0468U + index * 4U;
                if (!read_u32(0x00478FC6U, source, copied) ||
                    !write_u32(0x00478FC6U, destination, copied)) {
                    return finish();
                }
            }
        }
    }

    eax = request.actor_token + 0x02A0U;
    CallReply update{};
    if (!invoke_port(0x00478FD1U, 0x00478FD6U, 0x004321E0U, {eax}, update)) {
        return finish();
    }
    if (eax == 0U) {
        static_cast<void>(return_from(0x00479849U));
        return finish();
    }
    u16 lookup_high{};
    u16 lookup_low{};
    if (!read_u16(0x00478FE1U, 0x02ECU, lookup_high) ||
        !read_u16(0x00478FE8U, 0x02EAU, lookup_low)) {
        return finish();
    }
    eax = replace_low_word(eax, lookup_high);
    ecx = replace_low_word(ecx, lookup_low);
    CallReply main_frame{};
    if (!invoke_port(
            0x00478FF1U, 0x00478FF6U, 0x004315D0U, {eax, ecx}, main_frame
        )) {
        return finish();
    }
    if (!write_u32(0x00478FF6U, 0x2548U, main_frame.eax)) {
        return finish();
    }
    publish_resource(actor.actor.action_execution->resource, main_frame);
    u32 resource_value{};
    if (!read_resource(
            0U,
            0x00478FFCU,
            main_frame.eax,
            0U,
            main_frame.outputs[0U],
            resource_value
        )) {
        return finish();
    }
    actor.shared->turn_frame_source_token = resource_value;

    u16 record_x{};
    u16 render_x{};
    u32 record_flags{};
    if (!read_u16(0x00479004U, 0x02B0U, record_x) ||
        !read_u16(0x0047900BU, 0x0316U, render_x) ||
        !write_u16(0x00479012U, 0x29B2U, record_x) ||
        !read_u32(0x00479019U, 0x02B8U, record_flags) ||
        !write_u32(0x00479024U, 0x2694U, record_flags) ||
        !read_u32(0x0047902AU, 0x2B00U, motion_mode) ||
        !write_u16(0x00479035U, 0x29ACU, render_x) ||
        !read_u32(0x0047903EU, 0x2B04U, scene_identity)) {
        return finish();
    }
    u32 mirror_mode{};
    if (motion_mode == 1U || scene_identity == 1U) {
        if (!read_u32(0x00479046U, 0x2B08U, mirror_mode)) {
            return finish();
        }
        if (mirror_mode == 1U) {
            u32 adjusted_flags = (record_flags & 1U) != 0U
                ? record_flags & 0xFFFFFFFEU
                : record_flags | 1U;
            if (!write_u32(0x00479064U, 0x2694U, adjusted_flags)) {
                return finish();
            }
            if (record_x != 0U) {
                u32 width{};
                if (!read_resource(
                        0U,
                        0x0047907AU,
                        main_frame.eax,
                        0x0CU,
                        main_frame.outputs[1U],
                        width
                    ) ||
                    !write_u16(
                        0x00479085U, 0x29B2U, static_cast<u16>(width - record_x)
                    )) {
                    return finish();
                }
            }
            if (render_x != 0U) {
                u32 width{};
                if (!read_resource(
                        0U,
                        0x0047909EU,
                        main_frame.eax,
                        0x0CU,
                        main_frame.outputs[1U],
                        width
                    ) ||
                    !write_u16(
                        0x004790A5U, 0x29ACU, static_cast<u16>(width - render_x)
                    )) {
                    return finish();
                }
            }
        }
        u32 adjusted{};
        if (!read_u32(0x004790ACU, 0x2694U, adjusted)) {
            return finish();
        }
        if ((adjusted & 0x2CU) == 0U &&
            !write_u32(0x004790B8U, 0x2694U, adjusted | 4U)) {
            return finish();
        }
    } else {
        if (!read_u32(0x004790BEU, 0x2B08U, mirror_mode)) {
            return finish();
        }
        if (mirror_mode == 1U) {
            u32 adjusted_flags = (record_flags & 1U) != 0U
                ? record_flags & 0xFFFFFFFEU
                : record_flags | 1U;
            if (!write_u32(0x004790ECU, 0x2694U, adjusted_flags)) {
                return finish();
            }
            if (record_x != 0U) {
                u32 width{};
                if (!read_resource(
                        0U,
                        0x00479102U,
                        main_frame.eax,
                        0x0CU,
                        main_frame.outputs[1U],
                        width
                    ) ||
                    !write_u16(
                        0x0047910DU, 0x29B2U, static_cast<u16>(width - record_x)
                    )) {
                    return finish();
                }
            }
            if (render_x != 0U) {
                u32 width{};
                if (!read_resource(
                        0U,
                        0x00479126U,
                        main_frame.eax,
                        0x0CU,
                        main_frame.outputs[1U],
                        width
                    ) ||
                    !write_u16(
                        0x0047912DU, 0x29ACU, static_cast<u16>(width - render_x)
                    )) {
                    return finish();
                }
            }
        }
    }

    u16 mode_gate{};
    if (!read_u16(0x00479134U, 0x26D0U, mode_gate)) {
        return finish();
    }
    if ((mode_gate & 8U) != 0U) {
        ecx = request.actor_token;
        CallReply reply{};
        if (!invoke_port(
                0x00479143U, 0x00479148U, 0x0047F3C0U, {0x34U}, reply
            )) {
            return finish();
        }
        static_cast<void>(return_from(0x0047914CU));
        return finish();
    }
    if ((mode_gate & 0x0200U) != 0U) {
        ecx = request.actor_token;
        CallReply reply{};
        if (!invoke_port(0x00479156U, 0x0047915BU, 0x0047F580U, {}, reply)) {
            return finish();
        }
        static_cast<void>(return_from(0x0047915FU));
        return finish();
    }
    u16 action_kind{};
    if (!read_u16(0x00479162U, 0x2A6CU, action_kind)) {
        return finish();
    }
    if (action_kind == 0U || action_kind == 0x1CU) {
        if ((mode_gate & 0x80U) != 0U) {
            ecx = request.actor_token;
            CallReply reply{};
            if (!invoke_port(
                    0x0047917BU, 0x00479180U, 0x0047F710U, {0U}, reply
                )) {
                return finish();
            }
            static_cast<void>(return_from(0x00479184U));
            return finish();
        }
        if ((mode_gate & 0x0400U) != 0U) {
            ecx = request.actor_token;
            CallReply reply{};
            if (!invoke_port(
                    0x0047918FU, 0x00479194U, 0x0047F710U, {1U}, reply
                )) {
                return finish();
            }
            static_cast<void>(return_from(0x00479198U));
            return finish();
        }
    }

    u32 suppress_draw{};
    if (!read_u32(0x0047919BU, 0x2AF0U, suppress_draw)) {
        return finish();
    }
    const bool main_draw = suppress_draw == 0U && (mode_gate & 0x1000U) == 0U;
    if (main_draw) {
        u16 coordinate_mode{};
        if (!read_u16(0x004791B0U, 0x26D8U, coordinate_mode)) {
            return finish();
        }
        u32 field_26c0{};
        if (!read_u32(0x004791CEU, 0x26C0U, field_26c0)) {
            return finish();
        }
        const u32 sign_mask = 0x80000000U;
        if (coordinate_mode != 2U && coordinate_mode != 3U &&
            (field_26c0 & sign_mask) == 0U) {
            u32 height{};
            if (!read_resource(
                    0U,
                    0x004791E9U,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    height
                )) {
                return finish();
            }
            actor.shared->draw_height_third =
                static_cast<u32>(static_cast<i32>(height) / 3);
            actor.shared->draw_height_quarter = height >> 2U;
            actor.shared->draw_motion_a = 0xFFFFFFFAU;
            actor.shared->draw_motion_b = 0xFFFFFFFAU;
            actor.shared->draw_motion_c = 0xFFFFFFFAU;
            if ((field_26c0 & sign_mask) != 0U || motion_mode == 1U) {
                actor.shared->draw_motion_a = 0xFFFFFFFFU;
                actor.shared->draw_motion_b = 0xFFFFFFFFU;
                actor.shared->draw_motion_c = 0xFFFFFFFFU;
            }
            u32 render_flags{};
            u32 width{};
            u32 draw_height{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            if (!read_u32(0x00479260U, 0x2694U, render_flags)) {
                return finish();
            }
            render_flags = (render_flags & 0x8000000FU) | 0x0CU;
            if (!write_u32(0x00479276U, 0x26A4U, render_flags)) {
                return finish();
            }
            if (!read_resource(
                    0U,
                    0x0047927CU,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    draw_height
                ) ||
                !read_resource(
                    0U,
                    0x00479283U,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u16(0x00479288U, 0x0D68U, position_y) ||
                !read_u16(0x0047928FU, 0x0D66U, position_x) ||
                !read_u16(0x0047929EU, 0x29B2U, source_y_offset)) {
                return finish();
            }
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) -
                static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                actor.shared->draw_height_third;
            CallReply draw{};
            if (!invoke_port(
                    0x004792A9U,
                    0x004792AEU,
                    0x004170E0U,
                    {0U, render_flags, draw_height, width, draw_y, draw_x},
                    draw
                )) {
                return finish();
            }
        }

        if ((field_26c0 & 0x02000000U) != 0U) {
            u32 render_flags{};
            u32 draw_height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 y_adjust{};
            u32 frame_anchor{};
            if (!read_u32(0x004792CAU, 0x2694U, render_flags) ||
                !write_u32(0x004792DEU, 0x26BCU, 0xFFFFFFFFU) ||
                !read_u32(0x004792E0U, 0x26ACU, y_adjust) ||
                !read_resource(
                    0U,
                    0x004792EAU,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    draw_height
                ) ||
                !read_resource(
                    0U,
                    0x004792EEU,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u16(0x004792F4U, 0x0D68U, position_y) ||
                !read_u32(0x004792FBU, 0x02B4U, frame_anchor) ||
                !read_u16(0x00479303U, 0x0D66U, position_x) ||
                !read_u16(0x0047930CU, 0x29B2U, source_y_offset) ||
                !read_u32(0x00479314U, 0x26A8U, x_adjust)) {
                return finish();
            }
            render_flags &= 0x80000003U;
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x0047931FU,
                    0x00479324U,
                    0x00417050U,
                    {request.actor_token + 0x26BCU,
                     render_flags,
                     draw_height,
                     width,
                     draw_y,
                     draw_x},
                    draw
                )) {
                return finish();
            }
        }

        u32 overlay{};
        if (!read_u32(0x00479327U, 0x2AC0U, overlay)) {
            return finish();
        }
        if (overlay == 1U) {
            u32 render_flags{};
            u32 draw_height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 y_adjust{};
            u32 frame_anchor{};
            if (!read_u32(0x00479336U, 0x2694U, render_flags) ||
                !write_u32(0x0047934BU, 0x26BCU, 0x07E007E0U) ||
                !read_resource(
                    0U,
                    0x00479351U,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    draw_height
                ) ||
                !read_u16(0x00479357U, 0x0D68U, position_y) ||
                !read_resource(
                    0U,
                    0x00479360U,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u32(0x00479364U, 0x26ACU, y_adjust) ||
                !read_u32(0x0047936BU, 0x02B4U, frame_anchor) ||
                !read_u16(0x00479373U, 0x0D66U, position_x) ||
                !read_u32(0x0047937CU, 0x26A8U, x_adjust) ||
                !read_u16(0x00479385U, 0x29B2U, source_y_offset)) {
                return finish();
            }
            render_flags &= 0x80000003U;
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x0047938FU,
                    0x00479394U,
                    0x00417050U,
                    {request.actor_token + 0x26BCU,
                     render_flags,
                     draw_height,
                     width,
                     draw_y,
                     draw_x},
                    draw
                ) ||
                !write_u32(0x00479397U, 0x26BCU, 0x07E007E0U)) {
                return finish();
            }
        }

        if (!read_resource(
                0U,
                0x004793A3U,
                main_frame.eax,
                0U,
                main_frame.outputs[0U],
                resource_value
            ) ||
            !read_u32(0x004793AAU, 0x26C0U, field_26c0)) {
            return finish();
        }
        actor.shared->turn_frame_source_token = resource_value;
        u32 render_flags{};
        if ((field_26c0 & 0x80000000U) == 0U) {
            if (!read_u32(0x004793B8U, 0x2694U, render_flags)) {
                return finish();
            }
            if ((render_flags & 0x14U) == 0x14U) {
                u8 action_byte{};
                if (!read_u8(0x004793C8U, 0x032AU, action_byte)) {
                    return finish();
                }
                actor.shared->special_render_mode = action_byte;
            }
            u8 opacity{};
            if (!read_u8(0x004793D4U, 0x0328U, opacity)) {
                return finish();
            }
            if (opacity != 0U) {
                actor.shared->draw_opacity = opacity;
            }
            if ((mode_gate & 0x40U) != 0U) {
                actor.shared->draw_opacity = 0U;
            }
            u32 height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 y_adjust{};
            u32 frame_anchor{};
            if (!read_u32(0x004793FDU, 0x2694U, render_flags) ||
                !read_u32(0x00479403U, 0x26ACU, y_adjust) ||
                !read_resource(
                    0U,
                    0x0047940BU,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    height
                ) ||
                !read_u16(0x00479412U, 0x0D68U, position_y) ||
                !read_resource(
                    0U,
                    0x0047941DU,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u32(0x00479421U, 0x26A8U, x_adjust) ||
                !read_u16(0x00479427U, 0x0D66U, position_x) ||
                !read_u32(0x0047942FU, 0x02B4U, frame_anchor) ||
                !read_u16(0x00479437U, 0x29B2U, source_y_offset)) {
                return finish();
            }
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x00479444U,
                    0x00479449U,
                    0x004170E0U,
                    {0U, render_flags, height, width, draw_y, draw_x},
                    draw
                )) {
                return finish();
            }
            actor.shared->draw_opacity = 0U;
            if ((field_26c0 & 0x04000000U) != 0U) {
                ecx = request.actor_token;
                CallReply child{};
                if (!invoke_port(
                        0x00479461U, 0x00479466U, 0x0047CC60U, {}, child
                    )) {
                    return finish();
                }
            }
        } else {
            if (!read_u32(0x00479468U, 0x2694U, render_flags)) {
                return finish();
            }
            render_flags &= 0x80000003U;
            u32 y_adjust{};
            if (!read_u32(0x00479479U, 0x26ACU, y_adjust) ||
                !write_u32(0x0047947FU, 0x26A4U, render_flags)) {
                return finish();
            }
            render_flags = (render_flags & 0xFFFFFF00U) |
                static_cast<u8>(render_flags | 0x30U);
            u32 height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 frame_anchor{};
            if (!read_resource(
                    0U,
                    0x0047948DU,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    height
                ) ||
                !read_resource(
                    0U,
                    0x00479491U,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u16(0x00479495U, 0x0D68U, position_y) ||
                !read_u32(0x0047949DU, 0x02B4U, frame_anchor) ||
                !read_u32(0x004794A5U, 0x26A8U, x_adjust) ||
                !read_u16(0x004794AEU, 0x0D66U, position_x) ||
                !read_u16(0x004794B5U, 0x29B2U, source_y_offset)) {
                return finish();
            }
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x004794C2U,
                    0x004794C7U,
                    0x004170E0U,
                    {0U, render_flags, height, width, draw_y, draw_x},
                    draw
                )) {
                return finish();
            }
        }

        if ((mode_gate & 4U) != 0U) {
            if (!read_u32(0x004794D3U, 0x2694U, render_flags)) {
                return finish();
            }
            render_flags &= 0x80000003U;
            u32 y_adjust{};
            if (!read_u32(0x004794E4U, 0x26ACU, y_adjust) ||
                !write_u32(0x004794EAU, 0x26A4U, render_flags)) {
                return finish();
            }
            render_flags = (render_flags & 0xFFFFFF00U) |
                static_cast<u8>(render_flags | 0x28U);
            u32 height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 frame_anchor{};
            if (!read_resource(
                    0U,
                    0x004794F8U,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    height
                ) ||
                !read_resource(
                    0U,
                    0x004794FCU,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u16(0x00479500U, 0x0D68U, position_y) ||
                !read_u32(0x00479508U, 0x02B4U, frame_anchor) ||
                !read_u32(0x00479510U, 0x26A8U, x_adjust) ||
                !read_u16(0x00479519U, 0x0D66U, position_x) ||
                !read_u16(0x00479520U, 0x29B2U, source_y_offset)) {
                return finish();
            }
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x0047952DU,
                    0x00479532U,
                    0x004170E0U,
                    {0U, render_flags, height, width, draw_y, draw_x},
                    draw
                )) {
                return finish();
            }
        }

        if (action_kind == 2U) {
            if (!read_u32(0x0047953FU, 0x2694U, render_flags)) {
                return finish();
            }
            render_flags &= 0x80000003U;
            if (!write_u32(0x0047954CU, 0x26A4U, render_flags)) {
                return finish();
            }
            actor.shared->draw_motion_a = 0x10U;
            render_flags |= 0x10U;
            u32 height{};
            u32 width{};
            u16 position_x{};
            u16 position_y{};
            u16 source_y_offset{};
            u32 x_adjust{};
            u32 y_adjust{};
            u32 frame_anchor{};
            if (!read_u32(0x0047956BU, 0x26ACU, y_adjust) ||
                !read_resource(
                    0U,
                    0x00479574U,
                    main_frame.eax,
                    0x0EU,
                    main_frame.outputs[2U],
                    height
                ) ||
                !read_resource(
                    0U,
                    0x0047957AU,
                    main_frame.eax,
                    0x0CU,
                    main_frame.outputs[1U],
                    width
                ) ||
                !read_u16(0x0047957FU, 0x0D68U, position_y) ||
                !read_u16(0x00479586U, 0x0D66U, position_x) ||
                !read_u32(0x0047958EU, 0x02B4U, frame_anchor) ||
                !read_u32(0x00479596U, 0x26A8U, x_adjust) ||
                !read_u16(0x0047959EU, 0x29B2U, source_y_offset)) {
                return finish();
            }
            const u32 draw_x = static_cast<u32>(signed_word(position_x)) +
                x_adjust - static_cast<u32>(signed_word(source_y_offset));
            const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
                y_adjust - frame_anchor;
            CallReply draw{};
            if (!invoke_port(
                    0x004795ABU,
                    0x004795B0U,
                    0x004170E0U,
                    {0U, render_flags, height, width, draw_y, draw_x},
                    draw
                )) {
                return finish();
            }
        }

        u32 special_item{};
        u16 presentation_kind{};
        if (!read_u32(0x004795B3U, 0x2B18U, special_item) ||
            !read_u16(0x004795BCU, 0x26D2U, presentation_kind)) {
            return finish();
        }
        if (special_item == 1U && presentation_kind == 0U) {
            u16 position_y{};
            u16 position_x{};
            u32 y_adjust{};
            if (!read_u16(0x004795C5U, 0x0D68U, position_y) ||
                !read_u32(0x004795CCU, 0x02B4U, y_adjust) ||
                !read_u16(0x004795D8U, 0x0D66U, position_x)) {
                return finish();
            }
            const i32 x = signed_word(position_x) + 10;
            const i32 y =
                signed_word(position_y) - static_cast<i32>(y_adjust) - 15;
            std::size_t index{};
            if (!begin_call(
                    0x004795EFU,
                    0x004795F4U,
                    0x004507A0U,
                    {std::bit_cast<u32>(y),
                     std::bit_cast<u32>(x),
                     std::bit_cast<u32>(actor.shared->decimal_value),
                     0x2354U},
                    index
                )) {
                return finish();
            }
            rendering::LegacyBlitClipRectangle clip{
                .left = platform.raster->clip_left,
                .top = platform.raster->clip_top,
                .width = platform.raster->clip_width,
                .height = platform.raster->clip_height,
            };
            LegacyBattleTenPlaceDecimalState decimal_state{};
            result.decimal_draw = coordinate_legacy_battle_ten_place_decimal(
                decimal_state,
                *platform.framebuffer,
                clip,
                *platform.shared_request,
                *platform.shared_effects,
                *platform.jitter,
                *platform.frame_provider,
                0x2354U,
                actor.shared->decimal_value,
                x,
                y
            );
            ++result.decimal_draw_calls;
            auto& trace = result.physical_calls[index];
            trace.return_eax = result.decimal_draw.legacy_return_value;
            eax = result.decimal_draw.legacy_return_value;
            if (result.decimal_draw.status !=
                LegacyBattleTenPlaceDecimalStatus::completed) {
                result.status = LegacyBattleActorActionPresentationStatus::
                    decimal_draw_typed_stop;
                eip = 0x004507A0U;
                return finish();
            }
        }

        u16 sample_word{};
        if (!read_u16(0x004795F7U, 0x02F8U, sample_word)) {
            return finish();
        }
        if (sample_word != 0U) {
            CallReply sample{};
            if (!invoke_port(
                    0x0047960BU,
                    0x00479610U,
                    0x00485610U,
                    {sample_word, actor.shared->sample_handle},
                    sample
                )) {
                return finish();
            }
            u16 position_x{};
            if (!read_u16(0x00479613U, 0x0D66U, position_x)) {
                return finish();
            }
            CallReply pan{};
            if (!invoke_port(
                    0x00479634U,
                    0x00479639U,
                    0x00485650U,
                    {sample_word,
                     signed_word(position_x) < 0x140 ? 0xFFFFFFF0U : 0x10U},
                    pan
                ) ||
                !write_u16(0x0047963CU, 0x02F8U, 0U)) {
                return finish();
            }
        } else if (!write_u16(0x0047963CU, 0x02F8U, 0U)) {
            return finish();
        }
    }

    if (!read_u16(0x00479648U, 0x2A6CU, action_kind) ||
        !read_u32(0x00479656U, 0x2AA0U, source_runtime)) {
        return finish();
    }
    if (action_kind == 6U && source_runtime == 1U) {
        if (!write_u32(0x00479668U, 0x0470U, 0U) ||
            !write_u32(0x0047966FU, 0x0468U, 0x186AU)) {
            return finish();
        }
        eax = request.actor_token + 0x0468U;
        CallReply update_turn{};
        if (!invoke_port(
                0x00479675U, 0x0047967AU, 0x004321E0U, {eax}, update_turn
            )) {
            return finish();
        }
        if (eax == 0U) {
            static_cast<void>(return_from(0x00479849U));
            return finish();
        }
        u16 turn_low{};
        u16 turn_high{};
        if (!read_u16(0x00479685U, 0x04B4U, turn_low) ||
            !read_u16(0x0047968CU, 0x04B2U, turn_high)) {
            return finish();
        }
        ecx = replace_low_word(ecx, turn_low);
        edx = replace_low_word(edx, turn_high);
        CallReply turn_frame{};
        if (!invoke_port(
                0x00479695U, 0x0047969AU, 0x004315D0U, {ecx, edx}, turn_frame
            ) ||
            !write_u32(0x0047969AU, 0x254CU, turn_frame.eax)) {
            return finish();
        }
        publish_resource(
            actor.actor.action_execution->turn_resource, turn_frame
        );
        if (!read_resource(
                1U,
                0x004796A0U,
                turn_frame.eax,
                0U,
                turn_frame.outputs[0U],
                resource_value
            )) {
            return finish();
        }
        actor.shared->turn_frame_source_token = resource_value;
        u32 turn_flags{};
        u32 turn_anchor_y{};
        u32 turn_anchor_x{};
        u32 height{};
        u32 width{};
        u16 position_x{};
        u16 position_y{};
        u16 source_y_offset{};
        u16 add_x{};
        u16 add_y{};
        if (!read_u32(0x004796ADU, 0x0480U, turn_flags) ||
            !read_u32(0x004796B3U, 0x047CU, turn_anchor_y) ||
            !read_resource(
                1U,
                0x004796BCU,
                turn_frame.eax,
                0x0EU,
                turn_frame.outputs[2U],
                height
            ) ||
            !read_u16(0x004796C4U, 0x0D68U, position_y) ||
            !read_resource(
                1U,
                0x004796CBU,
                turn_frame.eax,
                0x0CU,
                turn_frame.outputs[1U],
                width
            ) ||
            !read_u16(0x004796D1U, 0x0318U, add_y) ||
            !read_u32(0x004796DAU, 0x02B4U, turn_anchor_x) ||
            !read_u16(0x004796E3U, 0x0D66U, position_x) ||
            !read_u16(0x004796EAU, 0x0316U, add_x) ||
            !read_u32(0x004796F2U, 0x0478U, record_flags) ||
            !read_u16(0x004796FAU, 0x29B2U, source_y_offset)) {
            return finish();
        }
        const u32 draw_x = static_cast<u32>(signed_word(position_x)) -
            turn_anchor_x - static_cast<u32>(signed_word(source_y_offset)) +
            static_cast<u32>(signed_word(add_x));
        const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
            turn_anchor_y + static_cast<u32>(signed_word(add_y)) -
            turn_anchor_x;
        CallReply draw{};
        if (!invoke_port(
                0x00479706U,
                0x0047970BU,
                0x004170E0U,
                {0U, turn_flags, height, width, draw_y, draw_x},
                draw
            )) {
            return finish();
        }
        u16 turn_sample{};
        if (!read_u16(0x00479711U, 0x04C0U, turn_sample)) {
            return finish();
        }
        CallReply sample{};
        if (!invoke_port(
                0x0047971AU,
                0x0047971FU,
                0x00485610U,
                {turn_sample, actor.shared->sample_handle},
                sample
            )) {
            return finish();
        }
        u32 cleanup_gate{};
        if (!read_u32(0x0047971FU, 0x04F4U, cleanup_gate) ||
            !write_u16(0x0047972AU, 0x04C0U, 0U)) {
            return finish();
        }
        if (cleanup_gate == 1U) {
            if (!write_u32(0x00479733U, 0x04F4U, 0U) ||
                !write_u16(0x00479739U, 0x04AAU, 0U)) {
                return finish();
            }
        }
    }

    if (!read_u32(0x00479740U, 0x2AF0U, suppress_draw) ||
        !read_u8(0x0047974CU, 0x26D1U, byte)) {
        return finish();
    }
    u8 field_26c0_low{};
    u16 additional_kind{};
    u16 additional_index{};
    if (!read_u8(0x00479759U, 0x26C0U, field_26c0_low) ||
        !read_u16(0x00479766U, 0x02EEU, additional_kind) ||
        !read_u16(0x00479776U, 0x02F0U, additional_index)) {
        return finish();
    }
    if (suppress_draw == 0U && (byte & 0x10U) == 0U &&
        (field_26c0_low & 4U) == 0U && additional_kind != 0U &&
        additional_index != 0U) {
        eax = replace_low_word(eax, static_cast<u16>(additional_index - 1U));
        ecx = replace_low_word(ecx, additional_kind);
        CallReply additional_frame{};
        if (!invoke_port(
                0x00479785U,
                0x0047978AU,
                0x004315D0U,
                {eax, ecx},
                additional_frame
            ) ||
            !write_u32(0x0047978AU, 0x2554U, additional_frame.eax)) {
            return finish();
        }
        publish_resource(
            actor.actor.action_execution->additional_resource, additional_frame
        );
        if (!read_resource(
                2U,
                0x00479790U,
                additional_frame.eax,
                0U,
                additional_frame.outputs[0U],
                resource_value
            )) {
            return finish();
        }
        actor.shared->turn_frame_source_token = resource_value;
        u32 additional_flags{};
        u16 additional_x{};
        u16 additional_y{};
        u32 height{};
        u32 width{};
        u16 position_x{};
        u16 position_y{};
        u16 source_y_offset{};
        u32 frame_anchor{};
        u32 y_adjust{};
        if (!read_u32(0x00479797U, 0x2694U, additional_flags) ||
            !read_u16(0x0047979DU, 0x02FEU, additional_x) ||
            !write_u32(0x004797B1U, 0x2698U, additional_x) ||
            !read_resource(
                2U,
                0x004797B7U,
                additional_frame.eax,
                0x0EU,
                additional_frame.outputs[2U],
                height
            ) ||
            !read_u32(0x004797BBU, 0x26ACU, y_adjust) ||
            !read_resource(
                2U,
                0x004797C4U,
                additional_frame.eax,
                0x0CU,
                additional_frame.outputs[1U],
                width
            ) ||
            !read_u16(0x004797C8U, 0x0D68U, position_y) ||
            !read_u16(0x004797D2U, 0x0300U, additional_y) ||
            !read_u32(0x004797DBU, 0x02B4U, frame_anchor) ||
            !read_u16(0x004797E3U, 0x29B2U, source_y_offset) ||
            !read_u16(0x004797EBU, 0x0D66U, position_x)) {
            return finish();
        }
        additional_flags |= 4U;
        const u32 draw_x = static_cast<u32>(signed_word(position_x)) -
            static_cast<u32>(signed_word(source_y_offset)) + additional_x;
        const u32 draw_y = static_cast<u32>(signed_word(position_y)) -
            y_adjust + static_cast<u32>(signed_word(additional_y)) -
            frame_anchor;
        CallReply draw{};
        if (!invoke_port(
                0x004797F7U,
                0x004797FCU,
                0x004170E0U,
                {0U, additional_flags, height, width, draw_y, draw_x},
                draw
            )) {
            return finish();
        }
    }

    ecx = request.actor_token;
    CallReply tail{};
    if (!invoke_port(0x00479801U, 0x00479806U, 0x00480AE0U, {}, tail)) {
        return finish();
    }
    ecx = request.actor_token;
    if (!invoke_port(0x00479808U, 0x0047980DU, 0x00480D40U, {}, tail)) {
        return finish();
    }
    u16 position_y{};
    u16 position_x{};
    u16 source_y_offset{};
    u32 y_adjust{};
    u32 x_adjust{};
    u32 frame_anchor{};
    if (!read_u16(0x0047980DU, 0x0D68U, position_y) ||
        !read_u32(0x00479814U, 0x26ACU, y_adjust) ||
        !read_u32(0x0047981AU, 0x02B4U, frame_anchor) ||
        !read_u32(0x00479820U, 0x26A8U, x_adjust) ||
        !read_u16(0x00479828U, 0x29B2U, source_y_offset) ||
        !read_u16(0x00479831U, 0x0D66U, position_x)) {
        return finish();
    }
    const u32 final_y =
        static_cast<u32>(signed_word(position_y)) - y_adjust - frame_anchor;
    const u32 final_x = static_cast<u32>(signed_word(position_x)) + x_adjust -
        static_cast<u32>(signed_word(source_y_offset));
    ecx = request.actor_token;
    if (!invoke_port(
            0x00479840U, 0x00479845U, 0x0047E650U, {final_y, final_x}, tail
        )) {
        return finish();
    }
    static_cast<void>(return_from(0x00479849U));
    return finish();
}

bool execute_legacy_battle_actor_action_presentation_call(
    const LegacyBattleActorActionPresentationOwners& owners,
    const LegacyBattleActorActionPresentationPlatform platform,
    LegacyBattleActorActionPresentationCallTrace& trace,
    const LegacyBattleActorActionPresentationCallRequests& requests,
    const u32 call_address,
    const u32 return_address,
    const u32 actor_token,
    const u32 effect_argument,
    const u32 entry_eax,
    const u32 entry_edx,
    const LegacyBattleActorCoordinateFlags& entry_flags,
    const bool entry_flags_known,
    const std::size_t request_offset
) noexcept {
    const std::size_t trace_index = trace.calls;
    const std::size_t request_index = request_offset + trace_index;
    LegacyBattleActorActionPresentationRequest request{};
    if (request_index < requests.count) {
        request = requests.requests[request_index];
    }
    request.actor_token = actor_token;
    request.effect_argument = effect_argument;
    request.entry_eax = entry_eax;
    request.entry_edx = entry_edx;
    request.entry_return_address = return_address;
    request.entry_flags = entry_flags;
    request.entry_flags_known = entry_flags_known;

    trace.last = advance_legacy_battle_actor_action_presentation(
        resolve_legacy_battle_actor_action_presentation(owners, actor_token),
        platform,
        request
    );
    if (trace_index < trace.call_addresses.size()) {
        trace.call_addresses[trace_index] = call_address;
        trace.return_addresses[trace_index] = return_address;
        trace.actor_tokens[trace_index] = actor_token;
        trace.effect_arguments[trace_index] = effect_argument;
    }
    ++trace.calls;
    return trace.last.returned;
}

}  // namespace openswd3::battle
