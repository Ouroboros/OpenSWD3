#include "test.hpp"

#include "openswd3/battle/legacy_battle_action_rotation_cache.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void append_word(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

[[nodiscard]] std::vector<u8> make_literal_image() {
    constexpr u16 kWidth = 12U;
    std::vector<u8> bytes;
    append_word(bytes, openswd3::rendering::kLegacyImageCommandStreamMagic);
    append_word(bytes, kWidth);
    append_word(bytes, 1U);
    append_word(bytes, 0U);
    append_word(bytes, 0U);
    append_word(bytes, kWidth);
    for (u16 pixel = 1U; pixel <= kWidth; ++pixel) {
        append_word(bytes, pixel);
    }
    append_word(bytes, 0U);
    return bytes;
}

[[nodiscard]] u16
read_word(const std::vector<u8>& bytes, const std::size_t offset) {
    return static_cast<u16>(
        bytes[offset] | static_cast<u16>(bytes[offset + 1U] << 8U)
    );
}

struct UpdateStep {
    u16 field_4a{};
    u16 field_4c{};
    u16 command_cursor{};
    u16 wait_remaining{};
    u16 wait_default{};
    u8 field_88{};
    u32 draw_offset_x{};
    u32 draw_offset_y{};
    u32 mode_flags{};
    u32 field_8c{};
    u32 eax{};
    u32 edx{};
    std::uint64_t domain_token{};
};

class ScriptedRotationUpdatePort final
    : public openswd3::battle::LegacyBattleActionRotationUpdatePort {
public:
    ScriptedRotationUpdatePort(
        const std::initializer_list<UpdateStep> initial_steps
    )
        : steps(initial_steps) {}

    [[nodiscard]] openswd3::battle::LegacyBattleActionRotationUpdateSnapshot
    update_action(
        openswd3::asset_runtime::LegacyActionRecord& record
    ) override {
        const std::size_t index =
            calls < steps.size() ? calls : steps.size() - 1U;
        const UpdateStep& step = steps[index];
        ++calls;
        record.field_4a = step.field_4a;
        record.field_4c = step.field_4c;
        record.command_cursor = step.command_cursor;
        record.wait_remaining = step.wait_remaining;
        record.wait_default = step.wait_default;
        record.field_88 = step.field_88;
        record.draw_offset_x = step.draw_offset_x;
        record.draw_offset_y = step.draw_offset_y;
        record.mode_flags = step.mode_flags;
        record.field_8c = step.field_8c;
        return {
            .eax = step.eax,
            .edx = step.edx,
            .domain_token = step.domain_token,
        };
    }

    std::vector<UpdateStep> steps;
    std::size_t calls{};
};

class MutableRotationFramePort final
    : public openswd3::battle::LegacyBattleMutableFrameImagePort {
public:
    MutableRotationFramePort() {
        for (std::vector<u8>& image : images) {
            image = make_literal_image();
        }
    }

    [[nodiscard]] openswd3::battle::LegacyBattleMutableFrameImage
    query_frame_image(const u32 resource_id, const u32 frame_index) override {
        resource_ids.push_back(resource_id);
        frame_indices.push_back(frame_index);
        const u16 slot = static_cast<u16>(frame_index);
        if (slot >= images.size()) {
            return {};
        }
        return {
            .owner_token = static_cast<u32>(0x100U + slot),
            .pointer_valid = pointer_valid,
            .bytes = images[slot],
            .frame = {
                .source = {.bytes = images[slot]},
                .width = 12U,
                .height = 1U,
            },
        };
    }

    std::array<std::vector<u8>, 3> images;
    std::vector<u32> resource_ids;
    std::vector<u32> frame_indices;
    bool pointer_valid{true};
};

[[nodiscard]] bool action_record_is_zero(
    const openswd3::asset_runtime::LegacyActionRecord& record
) {
    std::array<u8, openswd3::asset_runtime::kLegacyActionRecordSize> zero{};
    return std::memcmp(&record, zero.data(), zero.size()) == 0;
}

void install_cached_literal_frames(
    openswd3::battle::LegacyBattleActionRotationCacheState& state,
    MutableRotationFramePort& frames
) {
    for (std::size_t slot = 0; slot < frames.images.size(); ++slot) {
        state.frame_owner_tokens[slot] = static_cast<u32>(0x100U + slot);
        state.cached_mutable_images[slot] = frames.images[slot];
        state.cached_frames[slot] = {
            .source = {.bytes = frames.images[slot]},
            .width = 12U,
            .height = 1U,
        };
    }
}

struct RotationPlaybackFixture {
    openswd3::battle::LegacyBattleActionRotationCacheState state;
    MutableRotationFramePort frames;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 160,
        .width = 80,
        .height = 60,
    }};
    openswd3::rendering::LegacyBlitRequest request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;

    RotationPlaybackFixture() {
        state.stored_action_id = 0x1234U;
        state.field_b4 = 20U;
        state.field_b8 = 20U;
        install_cached_literal_frames(state, frames);
    }

    [[nodiscard]] openswd3::battle::LegacyBattleActionRotationPlaybackResult
    play(
        ScriptedRotationUpdatePort& updater,
        const openswd3::compat::i32 rotation_amount
    ) {
        return openswd3::battle::play_legacy_battle_action_rotation_cache(
            state,
            updater,
            framebuffer,
            {.left = 0, .top = 0, .width = 80, .height = 60},
            request,
            effects,
            jitter,
            0xDEADBEEFU,
            rotation_amount
        );
    }
};

}  // namespace

void test_battle_action_rotation_cache(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.action_record.field_24 = 0xDEADBEEFU;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4a = 0x2345U,
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 0xA1A1ABCDU,
                    .edx = 0xB2B2DCBAU,
                    .domain_token = 1U,
                },
                UpdateStep{
                    .field_4a = 0x3456U,
                    .field_4c = 1U,
                    .command_cursor = 0U,
                    .eax = 0xC3C3ABCDU,
                    .edx = 0xD4D4DCBAU,
                    .domain_token = 2U,
                },
            },
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state,
                updater,
                frames,
                0x11111111U,
                0x22222222U,
                0x33333333U,
                0x12345678U,
                64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        completed &&
                result.action_update_calls == 2U &&
                result.loop_iterations == 2U &&
                result.frame_query_calls == 2U && result.rotation_calls == 2U &&
                result.rotation_shift == 10 &&
                result.record_clear_calls == 1U &&
                result.local_frame_slots ==
                    std::array<u16, 3>{0U, 1U, 0xFFFFU} &&
                frames.resource_ids ==
                    std::vector<u32>{0xB2B22345U, 0xD4D43456U} &&
                frames.frame_indices ==
                    std::vector<u32>{0xA1A10000U, 0xC3C30001U} &&
                state.frame_owner_tokens ==
                    std::array<u32, 3>{0x100U, 0x101U, 0U} &&
                state.field_b4 == 0x22222222U &&
                state.field_b8 == 0x33333333U &&
                state.stored_action_id == 0x5678U &&
                action_record_is_zero(state.action_record) &&
                read_word(frames.images[0U], 12U) == 3U &&
                read_word(frames.images[0U], 30U) == 12U &&
                read_word(frames.images[0U], 32U) == 1U,
            "two action frames preserve stale register highs rotate right and clear record"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 0x1111U,
                .field_4c = 2U,
                .command_cursor = 0U,
                .field_88 = 7U,
                .eax = 0xAAAA5555U,
                .edx = 0xBBBB6666U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 9U, 640U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        completed &&
                result.field_bc_writes == 1U && state.field_bc == 7U &&
                frames.frame_indices == std::vector<u32>{2U} &&
                frames.resource_ids == std::vector<u32>{0xBBBB1111U},
            "nonzero byte field clears stale eax high before frame index publication"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 1U,
                .field_4c = 0U,
                .command_cursor = 0U,
                .eax = 0U,
                .edx = 2U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state,
                updater,
                frames,
                0U,
                0x11112222U,
                0x33334444U,
                0xAAAA5678U,
                1U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        initial_action_update_stopped &&
                result.action_update_calls == 1U &&
                result.loop_iterations == 0U &&
                state.action_record.action_id == 0x5678U &&
                state.action_record.base_variant == 0U &&
                state.action_record.field_4a == 1U &&
                state.field_b4 == 0x11112222U && state.field_b8 == 0x33334444U,
            "initial zero eax keeps initialized and updater-written prefix"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 0x2222U,
                .field_4c = 0U,
                .command_cursor = 0U,
                .eax = 1U,
                .edx = 0xABCD0000U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        frames.pointer_valid = false;
        const auto divided =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 0x12340000U
            );
        test.expect_true(
            divided.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        division_by_zero &&
                divided.frame_query_calls == 1U &&
                divided.rotation_calls == 0U &&
                divided.local_frame_slots[0U] == 0U &&
                state.frame_owner_tokens[0U] == 0x100U,
            "zero divisor low word stops after frame owner and local slot publication"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 1U,
                .field_4c = 3U,
                .command_cursor = 0U,
                .eax = 1U,
                .edx = 2U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        frame_index_out_of_range &&
                result.frame_query_calls == 0U && result.rotation_calls == 0U,
            "frame index outside three local slots stops at first stack access"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 1U,
                .field_4c = 0U,
                .command_cursor = 0U,
                .eax = 1U,
                .edx = 2U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        frames.pointer_valid = false;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        frame_image_pointer_invalid &&
                result.rotation_shift == 10 && result.frame_query_calls == 1U &&
                result.local_frame_slots[0U] == 0U &&
                state.frame_owner_tokens[0U] == 0x100U,
            "invalid image pointer stops only after division and cache publication"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 1U,
                .field_4c = 0U,
                .command_cursor = 0U,
                .eax = 1U,
                .edx = 2U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        frames.images[0U] = {0xFFU, 0xFFU};
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        rotation_typed_stop &&
                result.rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        header_read_out_of_range &&
                result.rotation_calls == 1U && result.record_clear_calls == 0U,
            "short literal image stops outer loop at closed callee unsafe read"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4a = 1U,
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 1U,
                    .edx = 2U,
                    .domain_token = 1U,
                },
                UpdateStep{
                    .field_4a = 2U,
                    .field_4c = 1U,
                    .command_cursor = 1U,
                    .eax = 0U,
                    .edx = 3U,
                    .domain_token = 2U,
                },
            },
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 0x1234U, 64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        action_update_stopped &&
                result.action_update_calls == 2U &&
                result.frame_query_calls == 1U && result.rotation_calls == 1U &&
                result.record_clear_calls == 0U &&
                state.action_record.action_id == 0x1234U &&
                state.action_record.base_variant == 0U &&
                state.action_record.field_4c == 1U &&
                state.frame_owner_tokens[0U] == 0x100U,
            "later zero eax preserves rotated cache and second updater prefix"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4a = 1U,
                .field_4c = 0U,
                .command_cursor = 0U,
                .eax = 1U,
                .edx = 2U,
                .domain_token = 1U,
            }},
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 641U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        completed &&
                result.rotation_shift == 0 &&
                result.rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        shift_not_positive &&
                result.rotation_calls == 1U &&
                result.record_clear_calls == 1U &&
                action_record_is_zero(state.action_record),
            "zero shift callee early return remains normal and clears finished record"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4a = 1U,
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 1U,
                    .edx = 2U,
                    .domain_token = 9U,
                },
                UpdateStep{
                    .field_4a = 1U,
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 1U,
                    .edx = 2U,
                    .domain_token = 9U,
                },
            },
        };
        MutableRotationFramePort frames;
        const auto result =
            openswd3::battle::initialize_legacy_battle_action_rotation_cache(
                state, updater, frames, 0U, 0U, 0U, 1U, 64U
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationCacheStatus::
                        action_loop_nonterminating &&
                result.action_update_calls == 3U &&
                result.loop_iterations == 2U &&
                result.frame_query_calls == 1U && result.rotation_calls == 1U &&
                result.skipped_cached_frames == 1U,
            "full repeated updater token record slots and owners stop proven loop"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.stored_action_id = 0U;
        ScriptedRotationUpdatePort updater{{UpdateStep{.eax = 1U}}};
        MutableRotationFramePort frames;
        openswd3::rendering::LegacyFramebuffer framebuffer{{
            .pitch_bytes = 160,
            .width = 80,
            .height = 60,
        }};
        openswd3::rendering::LegacyBlitRequest request{
            .horizontal_resample_displacement = 9,
        };
        openswd3::rendering::LegacyBlitEffectState effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_action_rotation_frame(
                state,
                updater,
                framebuffer,
                {.left = 0, .top = 0, .width = 80, .height = 60},
                request,
                effects,
                jitter
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationDrawStatus::
                        completed &&
                result.return_value == 0U && result.action_update_calls == 0U &&
                result.frame_draw_calls == 0U && updater.calls == 0U &&
                request.horizontal_resample_displacement == 9,
            "zero stored action returns zero before update and shared displacement"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.stored_action_id = 0x1234U;
        state.field_b4 = 30U;
        state.field_b8 = 40U;
        state.field_bc = 1U;
        state.frame_owner_tokens[1U] = 0x101U;
        std::vector<u8> pixels{
            0x11U,
            0x11U,
            0x22U,
            0x22U,
            0x33U,
            0x33U,
            0x44U,
            0x44U,
        };
        state.cached_frames[1U] = {
            .source = {.bytes = pixels},
            .width = 2U,
            .height = 2U,
        };
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4c = 1U,
                .draw_offset_x = 10U,
                .draw_offset_y = 20U,
                .mode_flags = 0U,
                .field_8c = 0xCAFEBABEU,
                .eax = 0U,
            }},
        };
        openswd3::rendering::LegacyFramebuffer framebuffer{{
            .pitch_bytes = 160,
            .width = 80,
            .height = 60,
        }};
        openswd3::rendering::LegacyBlitRequest request{
            .horizontal_resample_displacement = 9,
        };
        openswd3::rendering::LegacyBlitEffectState effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_action_rotation_frame(
                state,
                updater,
                framebuffer,
                {.left = 0, .top = 0, .width = 80, .height = 60},
                request,
                effects,
                jitter
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationDrawStatus::
                        completed &&
                result.action_update_calls == 1U && updater.calls == 1U &&
                result.frame_draw_calls == 1U && result.frame_index == 1U &&
                result.draw_x == 20 && result.draw_y == 20 &&
                result.return_value == 0xCAFEBABEU && result.source_published &&
                framebuffer.row_pixels(20U)[20U] == 0x1111U &&
                request.target_height == 0 &&
                request.horizontal_resample_displacement == 0 &&
                request.vertical_resample_phase_10_10 == 0U &&
                request.opacity_step == 0 && effects.red_offset == 0 &&
                effects.green_offset == 0 && effects.blue_offset == 0 &&
                !effects.skip_every_third_row,
            "draw ignores zero update eax uses cached frame offsets and returns field eight-c"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.stored_action_id = 1U;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4c = 3U,
                .eax = 1U,
            }},
        };
        openswd3::rendering::LegacyFramebuffer framebuffer{{
            .pitch_bytes = 8,
            .width = 4,
            .height = 4,
        }};
        openswd3::rendering::LegacyBlitRequest request;
        openswd3::rendering::LegacyBlitEffectState effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_action_rotation_frame(
                state,
                updater,
                framebuffer,
                {.left = 0, .top = 0, .width = 4, .height = 4},
                request,
                effects,
                jitter
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationDrawStatus::
                        frame_index_out_of_range &&
                result.action_update_calls == 1U &&
                result.frame_draw_calls == 0U && !result.source_published,
            "updated frame index outside three owner slots stops at first access"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.stored_action_id = 1U;
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4c = 0U,
                .eax = 0U,
            }},
        };
        openswd3::rendering::LegacyFramebuffer framebuffer{{
            .pitch_bytes = 8,
            .width = 4,
            .height = 4,
        }};
        openswd3::rendering::LegacyBlitRequest request{
            .horizontal_resample_displacement = 6,
        };
        openswd3::rendering::LegacyBlitEffectState effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_action_rotation_frame(
                state,
                updater,
                framebuffer,
                {.left = 0, .top = 0, .width = 4, .height = 4},
                request,
                effects,
                jitter
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationDrawStatus::
                        cached_owner_invalid &&
                result.action_update_calls == 1U &&
                result.frame_draw_calls == 0U && !result.source_published &&
                request.horizontal_resample_displacement == 6,
            "valid frame slot with null owner stops before source and displacement"
        );
    }

    {
        openswd3::battle::LegacyBattleActionRotationCacheState state;
        state.stored_action_id = 1U;
        state.field_bc = 7U;
        state.frame_owner_tokens[0U] = 0x100U;
        std::vector<u8> indices(4U, 1U);
        std::array<u16, 2> palette{0U, 0x1234U};
        state.cached_frames[0U] = {
            .source =
                {
                    .bytes = indices,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::indexed_8,
                    .palette = palette,
                },
            .width = 2U,
            .height = 2U,
        };
        ScriptedRotationUpdatePort updater{
            {UpdateStep{
                .field_4c = 0U,
                .field_8c = 0x11223344U,
                .eax = 1U,
            }},
        };
        openswd3::rendering::LegacyFramebuffer framebuffer{{
            .pitch_bytes = 8,
            .width = 4,
            .height = 4,
        }};
        openswd3::rendering::LegacyBlitRequest request;
        openswd3::rendering::LegacyBlitEffectState effects;
        openswd3::rendering::LegacyRleRowJitterState jitter;
        const auto result =
            openswd3::battle::draw_legacy_battle_action_rotation_frame(
                state,
                updater,
                framebuffer,
                {.left = 0, .top = 0, .width = 4, .height = 4},
                request,
                effects,
                jitter
            );
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationDrawStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.frame_draw_calls == 1U && result.source_published &&
                request.horizontal_resample_displacement == 7 &&
                result.return_value == 0U,
            "fixed empty tail stops indexed cache before suffix clear and field return"
        );
    }

    {
        RotationPlaybackFixture fixture;
        fixture.state.stored_action_id = 0U;
        fixture.state.action_record.field_24 = 0xDEADBEEFU;
        ScriptedRotationUpdatePort updater{{UpdateStep{.eax = 1U}}};
        const auto result = fixture.play(updater, 1);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        completed &&
                result.return_value == 0U && result.action_update_calls == 0U &&
                result.record_clear_calls == 0U && updater.calls == 0U &&
                fixture.state.action_record.field_24 == 0xDEADBEEFU,
            "zero stored action returns zero before mandatory clear and update"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .wait_remaining = 7U,
                    .wait_default = 8U,
                    .eax = 1U,
                    .domain_token = 1U,
                },
                UpdateStep{
                    .field_4c = 1U,
                    .command_cursor = 0U,
                    .wait_remaining = 9U,
                    .wait_default = 10U,
                    .eax = 1U,
                    .domain_token = 2U,
                },
            },
        };
        const auto result = fixture.play(updater, 1);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        completed &&
                result.return_value == 1U && result.action_update_calls == 2U &&
                result.loop_iterations == 2U && result.rotation_calls == 2U &&
                result.frame_draw_calls == 2U &&
                result.wait_clear_calls == 2U &&
                result.record_clear_calls == 2U &&
                result.local_frame_slots ==
                    std::array<u16, 3>{0U, 1U, 0xFFFFU} &&
                result.rotation_mode ==
                    openswd3::battle::LegacyBattleImageRotationMode::
                        pixels_right &&
                result.rotation_shift == 1 &&
                action_record_is_zero(fixture.state.action_record) &&
                read_word(fixture.frames.images[0U], 12U) == 12U &&
                read_word(fixture.frames.images[1U], 12U) == 12U,
            "positive rotation plays each unique frame clears waits and returns one"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 0U,
            .command_cursor = 0U,
            .eax = 1U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, -1);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        completed &&
                result.rotation_mode ==
                    openswd3::battle::LegacyBattleImageRotationMode::
                        pixels_left &&
                result.rotation_shift == 1 && result.rotation_calls == 1U &&
                read_word(fixture.frames.images[0U], 12U) == 2U,
            "negative rotation uses low thirty-two bit negation and left mode"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .wait_remaining = 3U,
                    .wait_default = 4U,
                    .eax = 1U,
                    .domain_token = 1U,
                },
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 0U,
                    .wait_remaining = 5U,
                    .wait_default = 6U,
                    .eax = 1U,
                    .domain_token = 2U,
                },
            },
        };
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        completed &&
                result.rotation_calls == 0U && result.frame_draw_calls == 1U &&
                result.skipped_cached_frames == 1U &&
                result.wait_clear_calls == 2U &&
                read_word(fixture.frames.images[0U], 12U) == 1U,
            "zero rotation skips callee and repeated frame still clears waits"
        );
    }

    {
        RotationPlaybackFixture fixture;
        fixture.state.action_record.field_24 = 0xFFFFFFFFU;
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 0U,
            .command_cursor = 1U,
            .wait_remaining = 11U,
            .wait_default = 12U,
            .eax = 0U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        initial_action_update_stopped &&
                result.return_value == 0U && result.record_clear_calls == 1U &&
                result.loop_iterations == 0U &&
                fixture.state.action_record.field_24 == 0U &&
                fixture.state.action_record.field_4c == 0U &&
                fixture.state.action_record.wait_remaining == 11U,
            "initial update failure preserves prefix after mandatory entry clear"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .wait_remaining = 3U,
                    .wait_default = 4U,
                    .eax = 1U,
                    .domain_token = 1U,
                },
                UpdateStep{
                    .field_4c = 1U,
                    .command_cursor = 1U,
                    .wait_remaining = 9U,
                    .wait_default = 10U,
                    .eax = 0U,
                    .domain_token = 2U,
                },
            },
        };
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        action_update_stopped &&
                result.return_value == 0U && result.frame_draw_calls == 1U &&
                result.wait_clear_calls == 1U &&
                result.record_clear_calls == 1U &&
                fixture.state.action_record.field_4c == 1U &&
                fixture.state.action_record.wait_remaining == 9U,
            "later update failure preserves next update record after prior wait clear"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 3U,
            .command_cursor = 0U,
            .eax = 1U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        frame_index_out_of_range &&
                result.frame_draw_calls == 0U && result.wait_clear_calls == 0U,
            "frame index three stops at first local slot access"
        );
    }

    {
        RotationPlaybackFixture fixture;
        fixture.state.frame_owner_tokens[0U] = 0U;
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 0U,
            .command_cursor = 0U,
            .eax = 1U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        cached_owner_invalid &&
                result.local_frame_slots[0U] == 0U &&
                result.frame_draw_calls == 0U && result.wait_clear_calls == 0U,
            "zero rotation null owner stops after local slot before source draw"
        );
    }

    {
        RotationPlaybackFixture fixture;
        fixture.frames.images[0U] = {0xFFU, 0xFFU};
        fixture.state.cached_mutable_images[0U] = fixture.frames.images[0U];
        fixture.state.cached_frames[0U].source.bytes =
            fixture.frames.images[0U];
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 0U,
            .command_cursor = 0U,
            .eax = 1U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, 1);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        rotation_typed_stop &&
                result.rotation.status ==
                    openswd3::battle::LegacyBattleImageRotationStatus::
                        header_read_out_of_range &&
                result.frame_draw_calls == 0U &&
                result.wait_clear_calls == 0U &&
                result.local_frame_slots[0U] == 0U,
            "rotation unsafe read stops after local slot and before draw or waits"
        );
    }

    {
        RotationPlaybackFixture fixture;
        std::vector<u8> indices(12U, 1U);
        std::array<u16, 2> palette{0U, 0x1234U};
        fixture.state.cached_frames[0U] = {
            .source =
                {
                    .bytes = indices,
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::indexed_8,
                    .palette = palette,
                },
            .width = 12U,
            .height = 1U,
        };
        ScriptedRotationUpdatePort updater{{UpdateStep{
            .field_4c = 0U,
            .command_cursor = 0U,
            .wait_remaining = 7U,
            .wait_default = 8U,
            .eax = 1U,
            .domain_token = 1U,
        }}};
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        blit_typed_stop &&
                result.blit_status ==
                    openswd3::rendering::LegacyBlitExecutionStatus::
                        palette_out_of_bounds &&
                result.frame_draw_calls == 1U &&
                result.wait_clear_calls == 0U &&
                fixture.state.action_record.wait_remaining == 7U,
            "fixed empty tail indexed stop preserves wait words and blocks completion"
        );
    }

    {
        RotationPlaybackFixture fixture;
        ScriptedRotationUpdatePort updater{
            {
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 1U,
                    .domain_token = 9U,
                },
                UpdateStep{
                    .field_4c = 0U,
                    .command_cursor = 1U,
                    .eax = 1U,
                    .domain_token = 9U,
                },
            },
        };
        const auto result = fixture.play(updater, 0);
        test.expect_true(
            result.status ==
                    openswd3::battle::LegacyBattleActionRotationPlaybackStatus::
                        action_loop_nonterminating &&
                result.action_update_calls == 3U &&
                result.loop_iterations == 2U && result.frame_draw_calls == 1U &&
                result.skipped_cached_frames == 1U &&
                result.wait_clear_calls == 2U,
            "full playback state repeats only after draw skip waits and updates"
        );
    }
}
