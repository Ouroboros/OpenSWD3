#include "openswd3/battle/legacy_battle_actor_frame_presentation.hpp"

#include "legacy_battle_actor_frame_io_helpers.hpp"

#include "openswd3/battle/legacy_battle_action_dispatch.hpp"
#include "openswd3/battle/legacy_battle_actor_lifecycle.hpp"
#include "openswd3/battle/legacy_battle_actor_progress.hpp"
#include "openswd3/battle/legacy_battle_directional_scan.hpp"
#include "openswd3/battle/legacy_battle_group_a_action_execution_state.hpp"
#include "openswd3/rendering/legacy_framebuffer.hpp"
#include "openswd3/rendering/legacy_scaled_rle_writer.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>

namespace openswd3::battle {
namespace {

[[nodiscard]] bool raw_pixel_span_matches(
    const std::shared_ptr<LegacyBattleActorFrameRawBlock>& owner,
    const compat::u32 raw_token,
    const compat::u32 pixel_token,
    const std::span<compat::u16> pixels,
    const bool already_reserved
) noexcept {
    if (owner == nullptr || raw_token == 0U || pixel_token < raw_token ||
        pixels.empty()) {
        return false;
    }
    const compat::u32 offset = pixel_token - raw_token;
    if ((offset & 1U) != 0U) {
        return false;
    }
    const auto words = owner->words();
    const std::size_t word_offset = offset / sizeof(compat::u16);
    return word_offset < words.size() &&
        pixels.size() <= words.size() - word_offset &&
        pixels.data() == words.data() + word_offset &&
        (already_reserved || owner->claim_external_guest_base(raw_token));
}

}  // namespace

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_publish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_publish = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_two_decoder_token_write_ready &&
        prefix.eip == 0x00479B4BU;
    const bool case_eight_publish = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_decoder_token_write_ready &&
        prefix.eip == 0x0047A643U;
    const bool case_hundred_publish = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_decoder_token_write_ready &&
        prefix.eip == 0x0047B570U;
    if (!case_two_publish && !case_eight_publish && !case_hundred_publish) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        actor.residual == nullptr || actor.progress == nullptr ||
        actor.action_execution == nullptr ||
        actor.primary_coordinates == nullptr ||
        actor.base_initialization == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = case_hundred_publish ? 0x0047B570U
            : case_eight_publish                          ? 0x0047A643U
                                                          : 0x00479B4BU;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    LegacyBattleActorImage image{};
    materialize_legacy_battle_actor_image(actor, image);
    std::memcpy(image.data() + 0x0E14U, &prefix.eax, sizeof(prefix.eax));
    synchronize_legacy_battle_actor_image_write(
        actor, image, 0x0E14U, sizeof(prefix.eax)
    );
    if (actor.particle_phase_owner != nullptr) {
        auto& emitter = actor.particle_phase_owner->emitter;
        emitter.source_pixels_owner.reset();
        if (prefix.decoder_child.source_pixels_known) {
            emitter.source_pixels = prefix.decoder_child.source_pixels;
            const auto& explicit_owner =
                prefix.decoder_child.source_pixels_raw_owner;
            const bool explicit_lease_requested = explicit_owner != nullptr ||
                prefix.decoder_child.source_pixels_raw_token != 0U;
            if (explicit_lease_requested) {
                if (raw_pixel_span_matches(
                        explicit_owner,
                        prefix.decoder_child.source_pixels_raw_token,
                        prefix.eax,
                        emitter.source_pixels,
                        explicit_owner == prefix.decoder_heap_block_owner &&
                            prefix.decoder_child.source_pixels_raw_token ==
                                prefix.decoder_heap_block_token
                    )) {
                    emitter.source_pixels_owner = explicit_owner;
                } else {
                    // A contradictory explicit owner cannot leave an
                    // unleased span in the persistent emitter.
                    emitter.source_pixels = {};
                }
            }
            if (!explicit_lease_requested && prefix.eax != 0U &&
                prefix.decoder_pixels_direct16 &&
                prefix.decoder_heap_block_owner != nullptr &&
                prefix.eax == prefix.decoder_heap_block_token + 0x20U) {
                const auto raw_words =
                    prefix.decoder_heap_block_owner->words();
                constexpr std::size_t kPayloadStartWords = 0x20U / 2U;
                if (raw_words.size() >= kPayloadStartWords &&
                    emitter.source_pixels.data() ==
                        raw_words.data() + kPayloadStartWords &&
                    emitter.source_pixels.size() <=
                        raw_words.size() - kPayloadStartWords) {
                    emitter.source_pixels_owner =
                        prefix.decoder_heap_block_owner;
                }
            }
        } else {
            emitter.source_pixels = {};
            if (prefix.eax != 0U &&
                prefix.decoder_heap_block_owner != nullptr &&
                prefix.eax == prefix.decoder_heap_block_token + 0x20U) {
                const auto raw_words = prefix.decoder_heap_block_owner->words();
                constexpr std::size_t kPayloadStartWords = 0x20U / 2U;
                const std::size_t pixel_count = prefix.decoder_pixel_count;
                const std::size_t byte_count =
                    prefix.decoder_heap_block_owner->size_bytes();
                if (raw_words.size() >= kPayloadStartWords &&
                    byte_count >= 0x20U &&
                    (prefix.decoder_pixels_direct16
                         ? pixel_count <= raw_words.size() - kPayloadStartWords
                         : pixel_count <= byte_count - 0x20U)) {
                    // The byte stream can terminate before filling its
                    // declared dimensions. Borrow only complete committed
                    // byte pairs; never expose its odd tail or heap padding.
                    const std::size_t committed_bytes =
                        prefix.decoder_byte_output_written < pixel_count
                        ? prefix.decoder_byte_output_written
                        : pixel_count;
                    const std::size_t readable_words =
                        prefix.decoder_pixels_direct16 ? pixel_count
                                                       : committed_bytes / 2U;
                    emitter.source_pixels =
                        raw_words.subspan(kPayloadStartWords, readable_words);
                    emitter.source_pixels_owner =
                        prefix.decoder_heap_block_owner;
                }
            }
        }
    }
    prefix.status = case_hundred_publish
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_post_decoder_source_ready
        : case_eight_publish ? LegacyBattleActorFrameEntryStatus::
                                   case_eight_post_decoder_frame_read_ready
                             : LegacyBattleActorFrameEntryStatus::
                                   case_two_post_decoder_frame_read_ready;
    prefix.eip = case_hundred_publish ? 0x0047B576U
        : case_eight_publish          ? 0x0047A649U
                                      : 0x00479B51U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_dimensions(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_two_post_decoder_frame_read_ready &&
        prefix.eip == 0x00479B51U;
    const bool case_eight_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_post_decoder_frame_read_ready &&
        prefix.eip == 0x0047A649U;
    if (!case_two_start && !case_eight_start) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_token = [&](const u32 instruction, u32& destination) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + 0x2548U,
                actor.action_execution != nullptr && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = actor.action_execution->render_source_token;
        return true;
    };
    const auto read_resource = [&](const u32 instruction,
                                   const u32 token,
                                   const u32 offset,
                                   const bool known,
                                   const u16 value,
                                   u16& output) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                case_eight_start ? LegacyBattleActorFrameEntryStatus::
                                       case_eight_frame_resource_read_typed_stop
                                 : LegacyBattleActorFrameEntryStatus::
                                       case_two_frame_resource_read_typed_stop,
                instruction,
                token + offset,
                token != 0U &&
                    actor.action_execution->resource.token == token && known &&
                    request.actor_resource_readable
            )) {
            return false;
        }
        output = value;
        return true;
    };
    const auto write_word = [&](const u32 instruction,
                                const u32 offset,
                                const u16 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                instruction,
                prefix.esi + offset,
                full_actor && actor.particle_phase_owner != nullptr &&
                    request.actor_writable && prefix.esi == request.actor_token
            )) {
            return false;
        }
        std::memcpy(image.data() + offset, &value, sizeof(value));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(value)
        );
        return true;
    };
    if (!read_token(case_eight_start ? 0x0047A649U : 0x00479B51U, prefix.eax)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x10U);
    prefix.flags_known = true;
    prefix.esp += 0x10U;
    u16 width{};
    if (!read_resource(
            case_eight_start ? 0x0047A652U : 0x00479B5AU,
            prefix.eax,
            0x0CU,
            actor.action_execution->resource.value_0c_known,
            actor.action_execution->resource.value_0c,
            width
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | width;
    if (!write_word(
            case_eight_start ? 0x0047A656U : 0x00479B5EU, 0x0E18U, width
        ) ||
        !read_token(case_eight_start ? 0x0047A65DU : 0x00479B65U, prefix.edx)) {
        return prefix;
    }
    u16 height{};
    if (!read_resource(
            case_eight_start ? 0x0047A663U : 0x00479B6BU,
            prefix.edx,
            0x0EU,
            actor.action_execution->resource.value_0e_known,
            actor.action_execution->resource.value_0e,
            height
        )) {
        return prefix;
    }
    prefix.eax = (prefix.eax & 0xFFFF0000U) | height;
    if (!write_word(
            case_eight_start ? 0x0047A667U : 0x00479B6FU, 0x0E1AU, height
        )) {
        return prefix;
    }
    prefix.status = case_eight_start
        ? LegacyBattleActorFrameEntryStatus::case_eight_geometry_ready
        : LegacyBattleActorFrameEntryStatus::case_two_geometry_ready;
    prefix.eip = case_eight_start ? 0x0047A66EU : 0x00479B76U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_geometry_ready ||
        prefix.eip != 0x00479B76U) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 source,
                                const bool owner_known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                owner_known && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = source;
        return true;
    };
    const auto write_dword = [&](const u32 instruction,
                                 const u32 offset,
                                 const u32 value) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                instruction,
                prefix.esi + offset,
                full_actor && actor.particle_phase_owner != nullptr &&
                    request.actor_writable && prefix.esi == request.actor_token
            )) {
            return false;
        }
        std::memcpy(image.data() + offset, &value, sizeof(value));
        synchronize_legacy_battle_actor_image_write(
            actor, image, offset, sizeof(value)
        );
        return true;
    };
    const auto read_resource = [&](const u32 instruction,
                                   const u32 token,
                                   const u32 offset,
                                   const bool known,
                                   const u16 value,
                                   u16& output) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                LegacyBattleActorFrameEntryStatus::
                    case_two_frame_resource_read_typed_stop,
                instruction,
                token + offset,
                token != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == token && known &&
                    request.actor_resource_readable
            )) {
            return false;
        }
        output = value;
        return true;
    };
    if (!read_actor(
            0x00479B76U,
            0x0D66U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!write_dword(0x00479B7FU, 0x0E1CU, prefix.ecx) ||
        !read_actor(
            0x00479B85U,
            0x03E4U,
            prefix.edi,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->reserved_action_record_02
                      .draw_offset_y,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479B8BU,
            0x0D68U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.edi);
    prefix.edx -= prefix.edi;
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!write_dword(0x00479B96U, 0x0E20U, prefix.edx) ||
        !read_actor(
            0x00479B9CU,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479BA2U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    u16 width{};
    if (!read_resource(
            0x00479BA9U,
            prefix.eax,
            0x0CU,
            actor.action_execution->resource.value_0c_known,
            actor.action_execution->resource.value_0c,
            width
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | width;
    const u32 before_shift = prefix.ecx;
    prefix.ecx >>= 1U;
    prefix.flags = {
        .carry = (before_shift & 1U) != 0U,
        .parity = even_parity(static_cast<u8>(prefix.ecx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.ecx == 0U,
        .sign = (prefix.ecx & 0x80000000U) != 0U,
        .overflow = (before_shift & 0x80000000U) != 0U,
    };
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    if (!write_dword(0x00479BB3U, 0x0E24U, prefix.ecx) ||
        !read_actor(
            0x00479BB9U,
            0x2548U,
            prefix.eax,
            actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x00479BBFU,
            0x03E4U,
            prefix.ebp,
            actor.action_execution->reserved_action_record_02.draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    u16 height{};
    if (!read_resource(
            0x00479BC7U,
            prefix.eax,
            0x0EU,
            actor.action_execution->resource.value_0e_known,
            actor.action_execution->resource.value_0e,
            height
        )) {
        return prefix;
    }
    prefix.ecx = (prefix.ecx & 0xFFFF0000U) | height;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_emitter_flags_ready;
    prefix.eip = 0x00479BCBU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_fields(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_emitter_flags_ready ||
        prefix.eip != 0x00479BCBU) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const bool emitter_owned =
        full_actor && actor.particle_phase_owner != nullptr;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x00479BCBU,
            prefix.esi + 0x0E3CU,
            emitter_owned && request.actor_readable &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    const u8 updated_flags = std::to_integer<u8>(image[0x0E3CU]) | 0x16U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_write,
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
            0x00479BCBU,
            prefix.esi + 0x0E3CU,
            request.actor_writable && prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    image[0x0E3CU] = static_cast<std::byte>(updated_flags);
    synchronize_legacy_battle_actor_image_write(actor, image, 0x0E3CU, 1U);
    prefix.flags = {
        .carry = false,
        .parity = even_parity(updated_flags),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = updated_flags == 0U,
        .sign = (updated_flags & 0x80U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x00479BD2U,
            prefix.esi + 0x0D68U,
            actor.primary_coordinates != nullptr && request.actor_readable &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.edx = static_cast<u32>(static_cast<std::int32_t>(
        std::bit_cast<std::int16_t>(actor.primary_coordinates->position_y)
    ));
    prefix.ecx >>= 1U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    prefix.eax = 2U;
    prefix.flags = add_flags(prefix.ecx, prefix.edx);
    prefix.ecx += prefix.edx;
    const auto write =
        [&](const u32 instruction, const u32 offset, const auto value) {
            if (!touch(
                    LegacyBattleActorFrameEntryAccessKind::actor_write,
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                    instruction,
                    prefix.esi + offset,
                    emitter_owned && request.actor_writable &&
                        prefix.esi == request.actor_token
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };
    if (!write(0x00479BE4U, 0x0E30U, prefix.eax) ||
        !write(0x00479BEAU, 0x0E2CU, prefix.ecx)) {
        return prefix;
    }
    prefix.ecx = prefix.esi;
    if (!write(0x00479BF2U, 0x0E28U, prefix.eax) ||
        !write(0x00479BF8U, 0x0E36U, static_cast<u16>(0x32U)) ||
        !write(0x00479C01U, 0x0E34U, static_cast<u16>(0xFAU)) ||
        !write(0x00479C0AU, 0x0E38U, static_cast<u16>(1U)) ||
        !write(0x00479C13U, 0x0E3AU, static_cast<u16>(0x0AU))) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_property_call_ready;
    prefix.eip = 0x00479C1CU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_property(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_property_call_ready &&
        prefix.eip == 0x00479C1CU;
    const bool case_eight_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eight_property_call_ready &&
        prefix.eip == 0x0047A70EU;
    const bool case_hundred_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_attribute_call_ready &&
        prefix.eip == 0x0047B634U;
    if (!case_two_call && !case_eight_call && !case_hundred_call) {
        return prefix;
    }
    const u32 call_ip = case_hundred_call ? 0x0047B634U
        : case_eight_call                 ? 0x0047A70EU
                                          : 0x00479C1CU;
    const u32 return_ip = case_hundred_call ? 0x0047B639U
        : case_eight_call                   ? 0x0047A713U
                                            : 0x00479C21U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const u32 return_slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            call_ip,
            return_slot,
            request.call_stack_writable
        )) {
        return prefix;
    }
    prefix.esp = return_slot;
    prefix.last_pushed_value = return_ip;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            case_hundred_call ? LegacyBattleActorFrameEntryStatus::
                                    case_hundred_attribute_child_read_typed_stop
                : case_eight_call
                ? LegacyBattleActorFrameEntryStatus::
                      case_eight_property_child_read_typed_stop
                : LegacyBattleActorFrameEntryStatus::
                      case_two_property_child_read_typed_stop,
            0x0047CE70U,
            prefix.ecx + 0x2694U,
            actor.action_execution != nullptr &&
                prefix.ecx == request.actor_token && request.actor_readable
        )) {
        return prefix;
    }
    const u8 value = static_cast<u8>(
        actor.action_execution->presentation_render_flags & 0xFFU
    );
    const u32 sign_extended = static_cast<u32>(
        static_cast<std::int32_t>(std::bit_cast<std::int8_t>(value))
    );
    prefix.eax = sign_extended & 1U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.eax)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.eax == 0U,
        .sign = false,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_read,
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop,
            0x0047CE7AU,
            prefix.esp,
            request.stack_readable
        )) {
        return prefix;
    }
    prefix.esp += 4U;
    prefix.flags = subtract_flags(
        prefix.eax,
        case_hundred_call     ? prefix.edi
            : case_eight_call ? prefix.ebp
                              : 1U
    );
    if (prefix.flags.zero) {
        const u32 field_token = prefix.esi + 0x0E3CU;
        const bool full_actor = actor.residual != nullptr &&
            actor.progress != nullptr && actor.action_execution != nullptr &&
            actor.primary_coordinates != nullptr &&
            actor.base_initialization != nullptr &&
            actor.particle_phase_owner != nullptr;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                case_hundred_call     ? 0x0047B63DU
                    : case_eight_call ? 0x0047A717U
                                      : 0x00479C26U,
                field_token,
                full_actor && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return prefix;
        }
        LegacyBattleActorImage image{};
        materialize_legacy_battle_actor_image(actor, image);
        u16 old_flags{};
        std::memcpy(
            &old_flags,
            image.data() + 0x0E3CU,
            (case_eight_call || case_hundred_call) ? sizeof(u16) : sizeof(u8)
        );
        const u16 updated_flags = old_flags |
            static_cast<u16>(case_hundred_call     ? prefix.edi
                                 : case_eight_call ? prefix.ebp
                                                   : 1U);
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_write,
                LegacyBattleActorFrameEntryStatus::actor_write_typed_stop,
                case_hundred_call     ? 0x0047B63DU
                    : case_eight_call ? 0x0047A717U
                                      : 0x00479C26U,
                field_token,
                request.actor_writable && prefix.esi == request.actor_token
            )) {
            return prefix;
        }
        const std::size_t width =
            (case_eight_call || case_hundred_call) ? sizeof(u16) : sizeof(u8);
        std::memcpy(image.data() + 0x0E3CU, &updated_flags, width);
        synchronize_legacy_battle_actor_image_write(
            actor, image, 0x0E3CU, width
        );
        prefix.flags = {
            .carry = false,
            .parity = even_parity(static_cast<u8>(updated_flags)),
            .auxiliary_carry = false,
            .auxiliary_carry_defined = false,
            .zero = updated_flags == 0U,
            .sign = (updated_flags &
                     ((case_eight_call || case_hundred_call) ? 0x8000U
                                                             : 0x80U)) != 0U,
            .overflow = false,
        };
    }
    prefix.status = case_hundred_call
        ? LegacyBattleActorFrameEntryStatus::case_hundred_metrics_iat_read_ready
        : case_eight_call
        ? LegacyBattleActorFrameEntryStatus::case_eight_metrics_iat_read_ready
        : LegacyBattleActorFrameEntryStatus::case_two_metrics_iat_read_ready;
    prefix.eip = case_hundred_call ? 0x0047B644U
        : case_eight_call          ? 0x0047A71EU
                                   : 0x00479C2CU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_metrics(
    LegacyBattleActorFrameMetricsPort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_two_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_two_metrics_iat_read_ready &&
        prefix.eip == 0x00479C2CU;
    const bool case_eight_start = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_metrics_iat_read_ready &&
        prefix.eip == 0x0047A71EU;
    if (!case_two_start && !case_eight_start) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_read,
            case_eight_start ? LegacyBattleActorFrameEntryStatus::
                                   case_eight_metrics_iat_read_typed_stop
                             : LegacyBattleActorFrameEntryStatus::
                                   case_two_metrics_iat_read_typed_stop,
            case_eight_start ? 0x0047A71EU : 0x00479C2CU,
            0x00499214U,
            request.global_readable && request.system_metrics_iat_known
        )) {
        return prefix;
    }
    prefix.edi = request.system_metrics_function_token;
    if (!push(case_eight_start ? 0x0047A724U : 0x00479C32U, 1U) ||
        !push(
            case_eight_start ? 0x0047A725U : 0x00479C34U,
            case_eight_start ? 0x0047A727U : 0x00479C36U
        )) {
        return prefix;
    }
    ++prefix.metrics_calls;
    prefix.metrics_child = port.get_system_metrics(
        1U, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    if (!prefix.metrics_child.returned) {
        prefix.status = case_eight_start
            ? LegacyBattleActorFrameEntryStatus::
                  case_eight_metrics_child_typed_stop
            : LegacyBattleActorFrameEntryStatus::
                  case_two_metrics_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = prefix.edi;
        prefix.eip = prefix.edi;
        return prefix;
    }
    prefix.esp += 8U;  // Win32 stdcall RET 4, plus CALL return slot.
    prefix.eax = prefix.metrics_child.eax;
    prefix.ecx = prefix.metrics_child.ecx;
    prefix.edx = prefix.metrics_child.edx;
    prefix.flags = prefix.metrics_child.flags;
    prefix.flags_known = prefix.metrics_child.flags_known;
    if (!push(case_eight_start ? 0x0047A727U : 0x00479C36U, prefix.eax)) {
        return prefix;
    }
    prefix.metric_height_on_stack = prefix.eax;
    if (!push(case_eight_start ? 0x0047A728U : 0x00479C37U, prefix.ebx) ||
        !push(
            case_eight_start ? 0x0047A729U : 0x00479C38U,
            case_eight_start ? 0x0047A72BU : 0x00479C3AU
        )) {
        return prefix;
    }
    ++prefix.metrics_calls;
    prefix.metrics_child = port.get_system_metrics(
        prefix.ebx, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    if (!prefix.metrics_child.returned) {
        prefix.status = case_eight_start
            ? LegacyBattleActorFrameEntryStatus::
                  case_eight_metrics_child_typed_stop
            : LegacyBattleActorFrameEntryStatus::
                  case_two_metrics_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = prefix.edi;
        prefix.eip = prefix.edi;
        return prefix;
    }
    prefix.esp += 8U;
    prefix.eax = prefix.metrics_child.eax;
    prefix.ecx = prefix.metrics_child.ecx;
    prefix.edx = prefix.metrics_child.edx;
    prefix.flags = prefix.metrics_child.flags;
    prefix.flags_known = prefix.metrics_child.flags_known;
    if (!push(case_eight_start ? 0x0047A72BU : 0x00479C3AU, prefix.eax)) {
        return prefix;
    }
    prefix.metric_width_on_stack = prefix.eax;
    prefix.ecx = 0x0053B0B8U;
    prefix.status = case_eight_start
        ? LegacyBattleActorFrameEntryStatus::case_eight_rectangle_call_ready
        : LegacyBattleActorFrameEntryStatus::case_two_rectangle_call_ready;
    prefix.eip = case_eight_start ? 0x0047A731U : 0x00479C40U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready ||
        prefix.eip != 0x0047B6B4U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.global_readable || !request.particle_global_4cd76c_known) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_two_particle_global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x0047B6B4U;
        prefix.stopped_token = 0x004CD76CU;
        prefix.eip = 0x0047B6B4U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.ecx = request.particle_global_4cd76c;
    prefix.eax = prefix.esi + 0x0E14U;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = slot;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047B6C0U, prefix.eax) || !push(0x0047B6C1U, prefix.ecx)) {
        return prefix;
    }
    prefix.particle_global_argument_on_stack = prefix.ecx;
    prefix.ecx = 0x0053B0B8U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_particle_call_ready;
    prefix.eip = 0x0047B6C7U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_rectangle_call(
    LegacyBattleActorFrameRectanglePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_two_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_rectangle_call_ready &&
        prefix.eip == 0x00479C40U;
    const bool case_eight_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_rectangle_call_ready &&
        prefix.eip == 0x0047A731U;
    const bool case_hundred_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_hundred_rectangle_call_ready &&
        prefix.eip == 0x0047B66EU;
    if (!case_two_call && !case_eight_call && !case_hundred_call) {
        return prefix;
    }
    const u32 call_ip = case_hundred_call ? 0x0047B66EU
        : case_eight_call                 ? 0x0047A731U
                                          : 0x00479C40U;
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = return_slot;
        prefix.eip = call_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = case_hundred_call ? 0x0047B673U
        : case_eight_call                        ? 0x0047A736U
                                                 : 0x00479C45U;
    ++prefix.rectangle_calls;
    prefix.rectangle_child = port.set_host_surface(
        prefix.metric_width_on_stack,
        prefix.metric_height_on_stack,
        prefix.ecx,
        prefix.eax,
        prefix.ecx,
        prefix.edx,
        prefix.flags
    );
    if (!prefix.rectangle_child.returned) {
        prefix.status = case_hundred_call
            ? LegacyBattleActorFrameEntryStatus::
                  case_hundred_rectangle_child_typed_stop
            : case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                    case_eight_rectangle_child_typed_stop
                              : LegacyBattleActorFrameEntryStatus::
                                    case_two_rectangle_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00433F30U;
        prefix.eip = 0x00433F30U;
        return prefix;
    }
    prefix.esp += 12U;  // sub_433F30 RET 8 pops two metric results.
    prefix.eax = prefix.rectangle_child.eax;
    prefix.ecx = prefix.rectangle_child.ecx;
    prefix.edx = prefix.rectangle_child.edx;
    prefix.flags = prefix.rectangle_child.flags;
    prefix.flags_known = prefix.rectangle_child.flags_known;
    prefix.status = case_hundred_call
        ? LegacyBattleActorFrameEntryStatus::
              case_hundred_post_rectangle_sample_ready
        : case_eight_call
        ? LegacyBattleActorFrameEntryStatus::case_eight_sample_handle_read_ready
        : LegacyBattleActorFrameEntryStatus::case_two_sample_code_write_ready;
    prefix.eip = case_hundred_call ? 0x0047B673U
        : case_eight_call          ? 0x0047A736U
                                   : 0x00479C45U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_two_sample_code_write_ready ||
        prefix.eip != 0x00479C45U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x00479C45U;
        prefix.stopped_token = prefix.esi + 0x0428U;
        prefix.eip = 0x00479C45U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->reserved_action_record_02.field_58 = 0x31U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x00479C4EU;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = 0x00479C4EU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = actor.shared_action->sample_handle;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = slot;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x00479C53U, prefix.eax) || !push(0x00479C54U, 0x31U)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_sample_call_ready;
    prefix.eip = 0x00479C56U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    const bool case_two_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_two_sample_call_ready &&
        prefix.eip == 0x00479C56U;
    const bool case_eight_call = prefix.status ==
            LegacyBattleActorFrameEntryStatus::case_eight_sample_call_ready &&
        prefix.eip == 0x0047A73EU;
    if (!case_two_call && !case_eight_call) {
        return prefix;
    }
    const u32 call_ip = case_eight_call ? 0x0047A73EU : 0x00479C56U;
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = call_ip;
        prefix.stopped_token = return_slot;
        prefix.eip = call_ip;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = case_eight_call ? 0x0047A743U : 0x00479C5BU;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.eax, 0x31U)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned ? callee.sample_child
                                                       : sound.play_sample(
                                                             0x31U,
                                                             prefix.eax,
                                                             prefix.eax,
                                                             prefix.ecx,
                                                             prefix.edx,
                                                             prefix.flags
                                                         );
    if (!prefix.sample_child.returned) {
        prefix.accesses_completed -=
            20U;  // Entry-only stop rolls back the uncommitted callee prefix.
        prefix.status = case_eight_call ? LegacyBattleActorFrameEntryStatus::
                                              case_eight_audio_child_typed_stop
                                        : LegacyBattleActorFrameEntryStatus::
                                              case_two_audio_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00485610U;
        prefix.eip = 0x00485610U;
        return prefix;
    }
    prefix.esp += 4U;  // sub_485610 RET does not clean its parent's args.
    prefix.eax = prefix.sample_child.eax;
    prefix.ecx = prefix.sample_child.ecx;
    prefix.edx = prefix.sample_child.edx;
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status = case_eight_call
        ? LegacyBattleActorFrameEntryStatus::case_eight_sample_phase_write_ready
        : LegacyBattleActorFrameEntryStatus::case_two_sample_phase_write_ready;
    prefix.eip = case_eight_call ? 0x0047A746U : 0x00479C5EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    const bool case_two_phase = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_two_sample_phase_write_ready &&
        prefix.eip == 0x00479C5EU;
    const bool case_eight_phase = prefix.status ==
            LegacyBattleActorFrameEntryStatus::
                case_eight_sample_phase_write_ready &&
        prefix.eip == 0x0047A746U;
    if (!case_two_phase && !case_eight_phase) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction =
            case_eight_phase ? 0x0047A746U : 0x00479C5EU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = prefix.stopped_instruction;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold =
        case_eight_phase ? static_cast<u16>(prefix.ebp) : 1U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready;
    prefix.eip = 0x0047B6B4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_call(
    LegacyBattleActorFrameParticlePort& port,
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_two_particle_call_ready ||
        prefix.eip != 0x0047B6C7U) {
        return prefix;
    }
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047B6C7U;
        prefix.stopped_token = return_slot;
        prefix.eip = 0x0047B6C7U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = 0x0047B6CCU;
    const bool canonical_owner = actor.particle_phase_owner != nullptr &&
        actor.particle_source_token_owner ==
            &actor.particle_phase_owner->decoded_resource_token;
    if (!canonical_owner) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_two_particle_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00434790U;
        prefix.eip = 0x00434790U;
        return prefix;
    }
    ++prefix.particle_calls;
    prefix.particle_child = port.update_particles(
        *actor.particle_phase_owner,
        prefix.particle_global_argument_on_stack,
        prefix.eax,
        prefix.ecx,
        prefix.eax,
        prefix.ecx,
        prefix.edx,
        prefix.flags
    );
    if (!prefix.particle_child.returned) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_two_particle_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00434790U;
        prefix.eip = 0x00434790U;
        return prefix;
    }
    prefix.esp += 12U;  // sub_434790 RET 8 pops both caller arguments.
    prefix.eax = prefix.particle_child.eax;
    prefix.ecx = prefix.particle_child.ecx;
    prefix.edx = prefix.particle_child.edx;
    prefix.flags = prefix.particle_child.flags;
    prefix.flags_known = prefix.particle_child.flags_known;
    prefix.flags = subtract_flags(prefix.eax, 1U);
    prefix.flags_known = true;
    prefix.status = prefix.eax == 1U
        ? LegacyBattleActorFrameEntryStatus::
              case_two_particle_phase_100_write_ready
        : LegacyBattleActorFrameEntryStatus::update_selector_default_ready;
    prefix.eip = prefix.eax == 1U ? 0x0047B6D5U : 0x0047A80BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_return(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_two_particle_phase_100_write_ready ||
        prefix.eip != 0x0047B6D5U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_writable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047B6D5U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047B6D5U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = 100U;
    const auto pop = [&](const u32 instruction, u32& target, const u32 saved) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.stack_readable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = prefix.esp;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        target = saved;
        prefix.esp += 4U;
        return true;
    };
    if (!pop(0x0047B6DEU, prefix.edi, request.entry_edi) ||
        !pop(0x0047B6DFU, prefix.esi, request.entry_esi) ||
        !pop(0x0047B6E0U, prefix.ebp, request.entry_ebp)) {
        return prefix;
    }
    prefix.eax = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags_known = true;
    if (!pop(0x0047B6E3U, prefix.ebx, request.entry_ebx)) {
        return prefix;
    }
    prefix.flags = add_flags(prefix.esp, 0x14U);
    prefix.esp += 0x14U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.return_address_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047B6E7U;
        prefix.stopped_token = prefix.esp;
        prefix.eip = 0x0047B6E7U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp += 4U;
    prefix.eip = request.entry_return_address;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_two_particle_returned;
    prefix.returned = true;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047A752U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047A752U;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047A752U;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 phase = actor.action_execution->turn_threshold;
    prefix.eax = (prefix.eax & 0xFFFF0000U) | phase;
    prefix.flags = subtract_flags_16(phase, 45U);
    prefix.flags_known = true;
    if (prefix.flags.sign != prefix.flags.overflow) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_nine_active_ready;
        prefix.eip = 0x0047A763U;
        return prefix;
    }
    const auto write_word =
        [&](const u32 instruction, const u32 offset, u16& owner) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.actor_writable) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::actor_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = prefix.esi + offset;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            owner = static_cast<u16>(prefix.ebx);
            return true;
        };
    if (!write_word(
            0x0047A253U, 0x2958U, actor.action_execution->turn_threshold
        ) ||
        !write_word(
            0x0047A25AU, 0x2954U, actor.action_execution->motion_word
        )) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_reset_progress_write_ready;
    prefix.eip = 0x0047B808U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_active_ready ||
        prefix.eip != 0x0047A763U) {
        return prefix;
    }
    prefix.flags = subtract_flags_16(
        static_cast<u16>(prefix.eax), static_cast<u16>(prefix.ebx)
    );
    prefix.flags_known = true;
    if (!prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_nine_source_ready;
        prefix.eip = 0x0047A779U;
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x0047A768U;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = 0x0047A768U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.edx = actor.shared_action->sample_handle;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = slot;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047A76EU, prefix.edx) || !push(0x0047A76FU, 0x31U)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_nine_audio_call_ready;
    prefix.eip = 0x0047A771U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_audio_call_ready ||
        prefix.eip != 0x0047A771U) {
        return prefix;
    }
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047A771U;
        prefix.stopped_token = return_slot;
        prefix.eip = 0x0047A771U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = 0x0047A776U;
    ++prefix.sample_calls;
    auto callee = prefix;
    if (!read_sound_callee_arguments(request, callee, prefix.edx, 0x31U)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    prefix.sample_child = callee.sample_child.returned ? callee.sample_child
                                                       : sound.play_sample(
                                                             0x31U,
                                                             prefix.edx,
                                                             prefix.eax,
                                                             prefix.ecx,
                                                             prefix.edx,
                                                             prefix.flags
                                                         );
    if (!prefix.sample_child.returned) {
        prefix.accesses_completed -=
            20U;  // Entry-only stop rolls back the uncommitted callee prefix.
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_nine_audio_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x00485610U;
        prefix.eip = 0x00485610U;
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eax = prefix.sample_child.eax;
    prefix.ecx = prefix.sample_child.ecx;
    prefix.edx = prefix.sample_child.edx;
    prefix.flags = add_flags(prefix.esp, 8U);
    prefix.flags_known = true;
    prefix.esp += 8U;
    prefix.status = LegacyBattleActorFrameEntryStatus::case_nine_source_ready;
    prefix.eip = 0x0047A779U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_source_and_opacity(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_source_ready ||
        prefix.eip != 0x0047A779U) {
        return prefix;
    }
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047A779U,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr && request.actor_readable &&
                prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.eax = actor.action_execution->render_source_token;
    const u32 stack_slot = prefix.esp - 4U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::stack_write,
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
            0x0047A77FU,
            stack_slot,
            request.call_stack_writable
        )) {
        return prefix;
    }
    prefix.esp = stack_slot;
    prefix.last_pushed_value = prefix.ebx;
    prefix.draw_auxiliary_value = prefix.ebx;
    prefix.draw_auxiliary_pushed = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
            LegacyBattleActorFrameEntryStatus::
                case_nine_source_resource_read_typed_stop,
            0x0047A780U,
            prefix.eax,
            prefix.eax != 0U &&
                actor.action_execution->resource.token == prefix.eax &&
                actor.action_execution->resource.value_00_known &&
                request.actor_resource_readable
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->resource.value_00;
    prefix.eax = 0x55555556U;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047A787U,
            0x004CD730U,
            actor.shared_action != nullptr && request.global_writable
        )) {
        return prefix;
    }
    actor.shared_action->turn_frame_source_token = prefix.ecx;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::actor_read,
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
            0x0047A78DU,
            prefix.esi + 0x2958U,
            request.actor_readable && prefix.esi == request.actor_token
        )) {
        return prefix;
    }
    prefix.ecx = static_cast<u32>(static_cast<std::int32_t>(
        std::bit_cast<std::int16_t>(actor.action_execution->turn_threshold)
    ));
    const std::int64_t product = static_cast<std::int64_t>(0x55555556U) *
        static_cast<std::int64_t>(std::bit_cast<std::int32_t>(prefix.ecx));
    const std::uint64_t product_bits = std::bit_cast<std::uint64_t>(product);
    prefix.edx = static_cast<u32>(product_bits >> 32U);
    prefix.eax = prefix.edx;
    prefix.ecx = 15U;
    prefix.eax = prefix.edx + (prefix.eax >> 31U) + 1U;
    prefix.flags = subtract_flags(prefix.ecx, prefix.eax);
    prefix.ecx -= prefix.eax;
    prefix.flags_known = true;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047A7A6U,
            0x004CD724U,
            actor.shared_action != nullptr && request.global_writable
        )) {
        return prefix;
    }
    actor.shared_action->draw_opacity = prefix.eax;
    if (!touch(
            LegacyBattleActorFrameEntryAccessKind::global_write,
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop,
            0x0047A7ABU,
            0x004CC2F0U,
            request.global_writable
        )) {
        return prefix;
    }
    actor.shared_action->special_render_mode = prefix.ecx;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_nine_draw_flags_ready;
    prefix.eip = 0x0047A7B1U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_draw_flags_ready ||
        prefix.eip != 0x0047A7B1U) {
        return prefix;
    }
    if (!prefix.draw_auxiliary_pushed) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_nine_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047A7B1U;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    prefix.draw_argument_count = 0U;
    const auto touch = [&](const LegacyBattleActorFrameEntryAccessKind kind,
                           const LegacyBattleActorFrameEntryStatus status,
                           const u32 instruction,
                           const u32 token,
                           const bool accessible) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !accessible) {
            prefix.status = status;
            prefix.stopped_access_kind = kind;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto read_actor = [&](const u32 instruction,
                                const u32 offset,
                                u32& destination,
                                const u32 source,
                                const bool owner_known) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::actor_read,
                LegacyBattleActorFrameEntryStatus::actor_read_typed_stop,
                instruction,
                prefix.esi + offset,
                owner_known && request.actor_readable &&
                    prefix.esi == request.actor_token
            )) {
            return false;
        }
        destination = source;
        return true;
    };
    const auto read_frame = [&](const u32 instruction,
                                const u32 token,
                                const u32 offset,
                                const bool known,
                                const u16 value,
                                u32& destination) {
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::frame_resource_read,
                LegacyBattleActorFrameEntryStatus::
                    case_nine_draw_resource_read_typed_stop,
                instruction,
                token + offset,
                token != 0U && actor.action_execution != nullptr &&
                    actor.action_execution->resource.token == token && known &&
                    request.actor_resource_readable
            )) {
            return false;
        }
        destination = (destination & 0xFFFF0000U) | value;
        return true;
    };
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (!touch(
                LegacyBattleActorFrameEntryAccessKind::stack_write,
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop,
                instruction,
                slot,
                request.call_stack_writable
            )) {
            return false;
        }
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        prefix.draw_argument_pushes[prefix.draw_argument_count++] = value;
        return true;
    };
    const auto signed_word = [](const u16 word) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(word))
        );
    };
    if (!read_actor(
            0x0047A7B1U,
            0x2694U,
            prefix.edx,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->presentation_render_flags,
            actor.action_execution != nullptr
        ) ||
        !read_actor(
            0x0047A7B7U,
            0x2548U,
            prefix.eax,
            actor.action_execution == nullptr
                ? 0U
                : actor.action_execution->render_source_token,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.edx |= 0x14U;
    prefix.flags = {
        .carry = false,
        .parity = even_parity(static_cast<u8>(prefix.edx)),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = prefix.edx == 0U,
        .sign = (prefix.edx & 0x80000000U) != 0U,
        .overflow = false,
    };
    prefix.flags_known = true;
    if (!push(0x0047A7C0U, prefix.edx)) {
        return prefix;
    }
    prefix.ecx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_frame(
            0x0047A7C3U,
            prefix.eax,
            0x0EU,
            actor.action_execution->resource.value_0e_known,
            actor.action_execution->resource.value_0e,
            prefix.ecx
        )) {
        return prefix;
    }
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    if (!read_frame(
            0x0047A7C9U,
            prefix.eax,
            0x0CU,
            actor.action_execution->resource.value_0c_known,
            actor.action_execution->resource.value_0c,
            prefix.edx
        ) ||
        !push(0x0047A7CDU, prefix.ecx) ||
        !read_actor(
            0x0047A7CEU,
            0x0D68U,
            prefix.eax,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_y),
            actor.primary_coordinates != nullptr
        ) ||
        !read_actor(
            0x0047A7D5U,
            0x29B2U,
            prefix.ecx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->source_y_offset),
            actor.primary_coordinates != nullptr
        ) ||
        !push(0x0047A7DCU, prefix.edx) ||
        !read_actor(
            0x0047A7DDU,
            0x02B4U,
            prefix.edx,
            actor.action_execution->frame_source_action_record.draw_offset_y,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.eax, prefix.edx);
    prefix.eax -= prefix.edx;
    if (!read_actor(
            0x0047A7E5U,
            0x0D66U,
            prefix.edx,
            actor.primary_coordinates == nullptr
                ? 0U
                : signed_word(actor.primary_coordinates->position_x),
            actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.edx -= prefix.ecx;
    if (!push(0x0047A7EEU, prefix.eax) || !push(0x0047A7EFU, prefix.edx)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_nine_draw_call_ready;
    prefix.eip = 0x0047A7F0U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_draw_call_ready ||
        prefix.eip != 0x0047A7F0U) {
        return prefix;
    }
    if (!prefix.draw_auxiliary_pushed ||
        prefix.draw_argument_count != prefix.draw_argument_pushes.size()) {
        prefix.status = LegacyBattleActorFrameEntryStatus::
            case_nine_draw_arguments_unbacked;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_read;
        prefix.stopped_instruction = 0x0047A7F0U;
        prefix.stopped_token = prefix.esp;
        return prefix;
    }
    const u32 return_slot = prefix.esp - 4U;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.call_stack_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::stack_write;
        prefix.stopped_instruction = 0x0047A7F0U;
        prefix.stopped_token = return_slot;
        prefix.eip = 0x0047A7F0U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.esp = return_slot;
    prefix.last_pushed_value = 0x0047A7F5U;
    ++prefix.draw_calls;
    const std::size_t pre_callee_accesses = prefix.accesses_completed;
    auto callee = prefix;
    if (!read_draw_callee_global(request, callee)) {
        return callee;
    }
    prefix.accesses_completed = callee.accesses_completed;
    const std::array<u32, 6U> arguments{
        prefix.draw_argument_pushes[4U],
        prefix.draw_argument_pushes[3U],
        prefix.draw_argument_pushes[2U],
        prefix.draw_argument_pushes[1U],
        prefix.draw_argument_pushes[0U],
        prefix.draw_auxiliary_value,
    };
    prefix.draw_child = draw.draw_frame(
        arguments, prefix.eax, prefix.ecx, prefix.edx, prefix.flags
    );
    if (!prefix.draw_child.returned) {
        prefix.accesses_completed = pre_callee_accesses;
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_nine_draw_child_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::callee_call;
        prefix.stopped_instruction = 0x004170E0U;
        prefix.eip = 0x004170E0U;
        return prefix;
    }
    prefix.esp += 4U;
    prefix.eax = prefix.draw_child.eax;
    prefix.ecx = prefix.draw_child.ecx;
    prefix.edx = prefix.draw_child.edx;
    prefix.flags = add_flags(prefix.esp, 0x18U);
    prefix.flags_known = true;
    prefix.esp += 0x18U;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_nine_draw_globals_ready;
    prefix.eip = 0x0047A7F8U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_nine_draw_globals_ready ||
        prefix.eip != 0x0047A7F8U) {
        return prefix;
    }
    const auto write_global =
        [&](const u32 instruction, const u32 token, u32& owner) {
            if (prefix.accesses_completed == request.stop_before_access ||
                !request.global_writable) {
                prefix.status =
                    LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
                prefix.stopped_access_kind =
                    LegacyBattleActorFrameEntryAccessKind::global_write;
                prefix.stopped_instruction = instruction;
                prefix.stopped_token = token;
                prefix.eip = instruction;
                return false;
            }
            ++prefix.accesses_completed;
            owner = prefix.ebx;
            return true;
        };
    if (actor.shared_action == nullptr) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_write;
        prefix.stopped_instruction = 0x0047A7F8U;
        prefix.stopped_token = 0x004CC2F0U;
        prefix.eip = 0x0047A7F8U;
        return prefix;
    }
    if (!write_global(
            0x0047A7F8U, 0x004CC2F0U, actor.shared_action->special_render_mode
        ) ||
        !write_global(
            0x0047A7FEU, 0x004CD724U, actor.shared_action->draw_opacity
        )) {
        return prefix;
    }
    const u32 phase_token = prefix.esi + 0x2958U;
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047A804U;
        prefix.stopped_token = phase_token;
        prefix.eip = 0x0047A804U;
        return prefix;
    }
    ++prefix.accesses_completed;
    const u16 before = actor.action_execution->turn_threshold;
    if (prefix.accesses_completed == request.stop_before_access ||
        !request.actor_writable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_write_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_write;
        prefix.stopped_instruction = 0x0047A804U;
        prefix.stopped_token = phase_token;
        prefix.eip = 0x0047A804U;
        return prefix;
    }
    ++prefix.accesses_completed;
    actor.action_execution->turn_threshold = static_cast<u16>(before + 1U);
    auto inc_flags = add_flags_16(before, 1U);
    inc_flags.carry = prefix.flags.carry;  // INC does not change CF.
    prefix.flags = inc_flags;
    prefix.flags_known = true;
    prefix.status =
        LegacyBattleActorFrameEntryStatus::update_selector_default_ready;
    prefix.eip = 0x0047A80BU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::update_selector_case_ready ||
        prefix.eip != 0x0047A5FEU) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.action_execution == nullptr || !request.actor_readable ||
        prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047A5FEU;
        prefix.stopped_token = prefix.esi + 0x2958U;
        prefix.eip = 0x0047A5FEU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.flags = subtract_flags_16(
        actor.action_execution->turn_threshold, static_cast<u16>(prefix.ecx)
    );
    prefix.flags_known = true;
    if (prefix.flags.zero) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::case_two_release_ready;
        prefix.eip = 0x00479C6CU;
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.particle_source_token_owner == nullptr ||
        !request.actor_readable || prefix.esi != request.actor_token) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::actor_read;
        prefix.stopped_instruction = 0x0047A60BU;
        prefix.stopped_token = prefix.esi + 0x0E14U;
        prefix.eip = 0x0047A60BU;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.flags =
        subtract_flags(*actor.particle_source_token_owner, prefix.ebx);
    prefix.status = prefix.flags.zero
        ? LegacyBattleActorFrameEntryStatus::case_eight_particle_init_ready
        : LegacyBattleActorFrameEntryStatus::case_two_particle_tail_ready;
    prefix.eip = prefix.flags.zero ? 0x0047A617U : 0x0047B6B4U;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::case_eight_geometry_ready ||
        prefix.eip != 0x0047A66EU) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto touch = [&](const bool write,
                           const u32 instruction,
                           const u32 offset,
                           const bool backed) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || prefix.esi != request.actor_token ||
            (write ? !request.actor_writable : !request.actor_readable)) {
            prefix.status = write
                ? LegacyBattleActorFrameEntryStatus::actor_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind = write
                ? LegacyBattleActorFrameEntryAccessKind::actor_write
                : LegacyBattleActorFrameEntryAccessKind::actor_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = prefix.esi + offset;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto write_dword =
        [&](const u32 instruction, const u32 offset, const u32 value) {
            if (!touch(
                    true,
                    instruction,
                    offset,
                    full_actor && actor.particle_phase_owner != nullptr
                )) {
                return false;
            }
            std::memcpy(image.data() + offset, &value, sizeof(value));
            synchronize_legacy_battle_actor_image_write(
                actor, image, offset, sizeof(value)
            );
            return true;
        };
    const auto signed_word = [](const u16 value) {
        return static_cast<u32>(
            static_cast<std::int32_t>(std::bit_cast<std::int16_t>(value))
        );
    };
    if (!touch(
            false, 0x0047A66EU, 0x0D66U, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = signed_word(actor.primary_coordinates->position_x);
    prefix.flags = subtract_flags(prefix.ecx, prefix.ebp);
    prefix.ecx -= prefix.ebp;
    if (!write_dword(0x0047A677U, 0x0E1CU, prefix.ecx) ||
        !touch(
            false, 0x0047A67DU, 0x03E4U, actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ecx =
        actor.action_execution->reserved_action_record_02.draw_offset_y;
    if (!touch(
            false, 0x0047A683U, 0x0D68U, actor.primary_coordinates != nullptr
        )) {
        return prefix;
    }
    prefix.edx = signed_word(actor.primary_coordinates->position_y);
    prefix.flags = subtract_flags(prefix.edx, prefix.ecx);
    prefix.edx -= prefix.ecx;
    if (!write_dword(0x0047A68CU, 0x0E20U, prefix.edx) ||
        !write_dword(0x0047A692U, 0x0E24U, 0xFFFFFFE2U) ||
        !touch(false, 0x0047A69CU, 0x2B08U, actor.progress != nullptr)) {
        return prefix;
    }
    prefix.flags = subtract_flags(actor.progress->post_action_value, 1U);
    prefix.flags_known = true;
    if (prefix.flags.zero && !write_dword(0x0047A6A5U, 0x0E24U, 0x29EU)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_eight_geometry_value_ready;
    prefix.eip = 0x0047A6AFU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_fields(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eight_geometry_value_ready ||
        prefix.eip != 0x0047A6AFU) {
        return prefix;
    }
    const bool full_actor = actor.residual != nullptr &&
        actor.progress != nullptr && actor.action_execution != nullptr &&
        actor.primary_coordinates != nullptr &&
        actor.base_initialization != nullptr &&
        actor.particle_phase_owner != nullptr;
    LegacyBattleActorImage image{};
    if (full_actor) {
        materialize_legacy_battle_actor_image(actor, image);
    }
    const auto touch = [&](const bool write,
                           const u32 instruction,
                           const u32 token,
                           const bool backed,
                           const bool resource = false) {
        if (prefix.accesses_completed == request.stop_before_access ||
            !backed || (!resource && prefix.esi != request.actor_token) ||
            (resource    ? !request.actor_resource_readable
                 : write ? !request.actor_writable
                         : !request.actor_readable)) {
            prefix.status = resource
                ? LegacyBattleActorFrameEntryStatus::
                      case_eight_frame_resource_read_typed_stop
                : write
                ? LegacyBattleActorFrameEntryStatus::actor_write_typed_stop
                : LegacyBattleActorFrameEntryStatus::actor_read_typed_stop;
            prefix.stopped_access_kind = resource
                ? LegacyBattleActorFrameEntryAccessKind::frame_resource_read
                : write ? LegacyBattleActorFrameEntryAccessKind::actor_write
                        : LegacyBattleActorFrameEntryAccessKind::actor_read;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = token;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        return true;
    };
    const auto write_field = [&](const u32 instruction,
                                 const u32 offset,
                                 const u32 value,
                                 const std::size_t size) {
        if (!touch(true, instruction, prefix.esi + offset, full_actor)) {
            return false;
        }
        std::memcpy(image.data() + offset, &value, size);
        synchronize_legacy_battle_actor_image_write(actor, image, offset, size);
        return true;
    };
    if (!touch(false, 0x0047A6AFU, prefix.esi + 0x0E20U, full_actor)) {
        return prefix;
    }
    std::memcpy(&prefix.eax, image.data() + 0x0E20U, sizeof(u32));
    prefix.edx = 0U;
    prefix.flags = logical_zero_flags();
    prefix.flags = subtract_flags(prefix.eax, 10U);
    prefix.eax -= 10U;
    prefix.ebp = 1U;
    if (!write_field(0x0047A6BFU, 0x0E2CU, prefix.eax, sizeof(u32)) ||
        !write_field(0x0047A6C5U, 0x0E28U, 2U, sizeof(u32)) ||
        !touch(
            false,
            0x0047A6CFU,
            prefix.esi + 0x2548U,
            actor.action_execution != nullptr
        )) {
        return prefix;
    }
    prefix.ecx = actor.action_execution->render_source_token;
    prefix.eax = 100U;
    const auto& resource = actor.action_execution->resource;
    if (!touch(
            false,
            0x0047A6DAU,
            prefix.ecx + 0x0EU,
            prefix.ecx != 0U && resource.token == prefix.ecx &&
                resource.value_0e_known,
            true
        )) {
        return prefix;
    }
    prefix.edx = (prefix.edx & 0xFFFF0000U) | resource.value_0e;
    if (!touch(false, 0x0047A6DEU, prefix.esi + 0x0E3CU, full_actor)) {
        return prefix;
    }
    const u8 old_flags = std::to_integer<u8>(image[0x0E3CU]);
    const u8 new_flags = old_flags | 0x16U;
    if (!write_field(0x0047A6DEU, 0x0E3CU, new_flags, sizeof(u8))) {
        return prefix;
    }
    prefix.flags = {
        .carry = false,
        .parity = even_parity(new_flags),
        .auxiliary_carry = false,
        .auxiliary_carry_defined = false,
        .zero = new_flags == 0U,
        .sign = (new_flags & 0x80U) != 0U,
        .overflow = false,
    };
    prefix.flags = add_flags(prefix.edx, 10U);
    prefix.edx += 10U;
    prefix.ecx = prefix.esi;
    if (!write_field(0x0047A6EAU, 0x0E30U, prefix.edx, sizeof(u32)) ||
        !write_field(0x0047A6F0U, 0x0E36U, prefix.eax, sizeof(u16)) ||
        !write_field(0x0047A6F7U, 0x0E34U, prefix.eax, sizeof(u16)) ||
        !write_field(0x0047A6FEU, 0x0E38U, prefix.ebp, sizeof(u16)) ||
        !write_field(0x0047A705U, 0x0E3AU, 10U, sizeof(u16))) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_eight_property_call_ready;
    prefix.eip = 0x0047A70EU;
    return prefix;
}

LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_sample_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept {
    if (prefix.status !=
            LegacyBattleActorFrameEntryStatus::
                case_eight_sample_handle_read_ready ||
        prefix.eip != 0x0047A736U) {
        return prefix;
    }
    if (prefix.accesses_completed == request.stop_before_access ||
        actor.shared_action == nullptr || !request.global_readable) {
        prefix.status =
            LegacyBattleActorFrameEntryStatus::global_read_typed_stop;
        prefix.stopped_access_kind =
            LegacyBattleActorFrameEntryAccessKind::global_read;
        prefix.stopped_instruction = 0x0047A736U;
        prefix.stopped_token = 0x004AB784U;
        prefix.eip = 0x0047A736U;
        return prefix;
    }
    ++prefix.accesses_completed;
    prefix.eax = actor.shared_action->sample_handle;
    const auto push = [&](const u32 instruction, const u32 value) {
        const u32 slot = prefix.esp - 4U;
        if (prefix.accesses_completed == request.stop_before_access ||
            !request.call_stack_writable) {
            prefix.status =
                LegacyBattleActorFrameEntryStatus::stack_write_typed_stop;
            prefix.stopped_access_kind =
                LegacyBattleActorFrameEntryAccessKind::stack_write;
            prefix.stopped_instruction = instruction;
            prefix.stopped_token = slot;
            prefix.eip = instruction;
            return false;
        }
        ++prefix.accesses_completed;
        prefix.esp = slot;
        prefix.last_pushed_value = value;
        return true;
    };
    if (!push(0x0047A73BU, prefix.eax) || !push(0x0047A73CU, 0x31U)) {
        return prefix;
    }
    prefix.status =
        LegacyBattleActorFrameEntryStatus::case_eight_sample_call_ready;
    prefix.eip = 0x0047A73EU;
    return prefix;
}

}  // namespace openswd3::battle
