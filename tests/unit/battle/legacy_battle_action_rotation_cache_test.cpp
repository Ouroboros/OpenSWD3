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
    u8 field_88{};
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
        record.field_88 = step.field_88;
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
}
