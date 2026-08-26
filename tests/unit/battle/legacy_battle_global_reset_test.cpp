#include "openswd3/battle/legacy_battle_global_reset.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace {

using openswd3::audio_video::LegacySampleBackend;
using openswd3::audio_video::LegacySampleHandle;
using openswd3::audio_video::LegacySampleManager;
using openswd3::audio_video::LegacySndArchive;
using openswd3::battle::LegacyBattleGlobalResetCall;
using openswd3::battle::LegacyBattleGlobalResetCallReply;
using openswd3::battle::LegacyBattleGlobalResetCallStage;
using openswd3::battle::LegacyBattleGlobalResetRuntimePort;
using openswd3::battle::LegacyBattleGlobalResetState;
using openswd3::battle::LegacyBattleStartupCall;
using openswd3::battle::LegacyBattleStartupCallReply;
using openswd3::battle::LegacyBattleStartupCallRequest;
using openswd3::battle::LegacyBattleStartupState;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class SilentSampleBackend final : public LegacySampleBackend {
public:
    [[nodiscard]] u32 driver_token() const override {
        return 1U;
    }
    [[nodiscard]] LegacySampleHandle allocate_sample_handle() override {
        return 1U;
    }
    void initialize_sample(LegacySampleHandle) override {}
    void release_sample_handle(LegacySampleHandle) override {}
    [[nodiscard]] bool
    set_sample_file(LegacySampleHandle, std::span<const u8>) override {
        return true;
    }
    [[nodiscard]] bool set_named_sample_file(
        LegacySampleHandle, std::string_view, std::span<const u8>, u32
    ) override {
        return true;
    }
    void set_sample_user_data(LegacySampleHandle, u32, u32) override {}
    [[nodiscard]] u32 sample_user_data(LegacySampleHandle, u32) override {
        return 0U;
    }
    void set_sample_volume(LegacySampleHandle, i32) override {}
    void set_sample_pan(LegacySampleHandle, i32) override {}
    void set_sample_loop_count(LegacySampleHandle, i32) override {}
    void start_sample(LegacySampleHandle) override {}
    void end_sample(LegacySampleHandle) override {
        ++end_calls;
    }
    [[nodiscard]] u32 sample_status(LegacySampleHandle) override {
        return 0U;
    }
    void close_output() override {}

    u32 end_calls{};
};

class ResetPort final : public LegacyBattleGlobalResetRuntimePort {
public:
    ResetPort() : samples(sample_backend, archive) {}

    [[nodiscard]] LegacyBattleStartupCallReply
    invoke(const LegacyBattleStartupCallRequest& request) override {
        startup_calls.push_back(request);
        return {.return_value = startup_return};
    }

    void release_image(const u32 token) noexcept override {
        released_images.push_back(token);
    }
    void release_owner(const u32 token) noexcept override {
        released_owners.push_back(token);
    }
    void release(const u32 token) noexcept override {
        released_render_tokens.push_back(token);
    }

    [[nodiscard]] LegacyBattleGlobalResetCallReply invoke_reset(
        const LegacyBattleGlobalResetCall call, const u32 argument
    ) override {
        reset_calls.push_back(call);
        reset_arguments.push_back(argument);
        return {.eax = reset_return};
    }

    [[nodiscard]] LegacySampleManager& sample_manager() noexcept override {
        return samples;
    }

    [[nodiscard]] u32
    startup_call_count(const LegacyBattleStartupCall call) const {
        return static_cast<u32>(
            std::ranges::count_if(startup_calls, [call](const auto& request) {
                return request.call == call;
            })
        );
    }

    u32 startup_return{0xAABBCCDDU};
    u32 reset_return{0x11223344U};
    std::vector<LegacyBattleStartupCallRequest> startup_calls;
    std::vector<LegacyBattleGlobalResetCall> reset_calls;
    std::vector<u32> reset_arguments;
    std::vector<u32> released_images;
    std::vector<u32> released_owners;
    std::vector<u32> released_render_tokens;
    SilentSampleBackend sample_backend;
    LegacySndArchive archive;
    LegacySampleManager samples;
};

[[nodiscard]] std::uint64_t
write_trace_hash(const LegacyBattleGlobalResetState& state) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    const auto append = [&hash](const u32 value) {
        for (u32 byte = 0U; byte < 4U; ++byte) {
            hash ^= static_cast<u8>(value >> (byte * 8U));
            hash *= 0x100000001B3ULL;
        }
    };
    for (const auto& write : state.write_trace) {
        append(write.address);
        append(write.size);
        append(write.count);
        append(write.value);
    }
    return hash;
}

[[nodiscard]] u8
byte_at(const LegacyBattleGlobalResetState& state, const u32 address) {
    const auto found = state.unmapped_bytes.find(address);
    return found == state.unmapped_bytes.end() ? 0xEEU : found->second;
}

void seed_state(
    LegacyBattleGlobalResetState& state,
    LegacyBattleStartupState& startup,
    ResetPort& port
) {
    state.unmapped_bytes[0x00ABCDEFU] = 0x5AU;
    state.unmapped_bytes[0x00520E40U] = 0x5AU;
    startup.display_surfaces = {11U, 22U};
    startup.background_rotation_cache.frame_owner_tokens[0] = 101U;
    startup.background_rotation_cache.cached_image_tokens[0] = 202U;
    startup.render_geometry.primary_row_offsets = std::make_unique<u32[]>(2U);
    startup.render_geometry.surface_row_offsets = std::make_unique<u32[]>(2U);
    startup.render_geometry.auxiliary_buffer_token = 303U;
    startup.reset.values_502940.fill(9U);
    startup.reset.values_502940[0] = 404U;
    startup.reset.block_525470.fill(9U);
    startup.reset.block_5244e8.fill(9U);
    startup.reset.records_524788[0].value_00 = 9U;
    startup.enemies[0].role_id = 9U;
    startup.party[0].role_id = 9U;
    startup.enemy_count = 8U;
    startup.party_count = 10U;

    auto& metrics = port.actor_metric_state();
    metrics.values.fill(9);
    metrics.actor_order.fill(9U);
    metrics.selected_mask.fill(9U);
    metrics.group_b_order.fill(9U);
    metrics.group_b_count = 8U;
    metrics.group_a_count = 10U;
    metrics.priority_update_gate = 9U;
    metrics.group_a_mode = 9U;
    metrics.group_b_mode = 9U;
    metrics.priority_actor_index = 9U;
    metrics.priority_order_ready = 9U;
}

}  // namespace

void test_battle_global_reset(openswd3::test::Context& test) {
    {
        LegacyBattleGlobalResetState state;
        LegacyBattleStartupState startup;
        ResetPort port;
        seed_state(state, startup, port);

        const auto result =
            openswd3::battle::reset_legacy_battle_globals(state, startup, port);

        const std::array expected_order{
            LegacyBattleGlobalResetCallStage::display_surfaces,
            LegacyBattleGlobalResetCallStage::rotation_cache,
            LegacyBattleGlobalResetCallStage::render_resources,
            LegacyBattleGlobalResetCallStage::conditional_allocation,
            LegacyBattleGlobalResetCallStage::pre_battle_resource_431960,
            LegacyBattleGlobalResetCallStage::pre_battle_resource_433010,
            LegacyBattleGlobalResetCallStage::all_samples,
            LegacyBattleGlobalResetCallStage::audio_stream,
            LegacyBattleGlobalResetCallStage::post_reset_initialization,
        };
        test.expect_true(
            result.call_count == expected_order.size() &&
                result.call_order == expected_order &&
                result.conditional_allocation_token == 404U &&
                result.conditional_allocation_released &&
                result.write_operations == 234U &&
                result.physical_writes == 3300U &&
                result.bytes_written == 13106U && result.return_value == 0U,
            "global reset preserves all nine call stages and the complete fixed write program"
        );
        test.expect_true(
            port.startup_call_count(
                LegacyBattleStartupCall::release_display_surface
            ) == 2U &&
                startup.display_surfaces == std::array<u32, 2>{0U, 0U} &&
                result.display_surfaces.release_calls == 2U &&
                result.display_surfaces.return_value == port.startup_return,
            "display surface helper releases nonzero slots in order and clears each after its call"
        );
        test.expect_true(
            port.released_images == std::vector<u32>{202U} &&
                port.released_owners == std::vector<u32>{101U} &&
                startup.background_rotation_cache.frame_owner_tokens[0] == 0U &&
                startup.background_rotation_cache.cached_image_tokens[0] == 0U,
            "closed rotation cache release runs before global stores"
        );
        test.expect_true(
            port.released_render_tokens == std::vector<u32>{303U} &&
                startup.render_geometry.primary_row_offsets == nullptr &&
                startup.render_geometry.surface_row_offsets == nullptr &&
                startup.render_geometry.auxiliary_buffer_token == 0U,
            "closed render cleanup runs before the fixed render owner zero range"
        );
        test.expect_true(
            port.reset_calls ==
                    std::vector<LegacyBattleGlobalResetCall>{
                        LegacyBattleGlobalResetCall::
                            release_conditional_allocation,
                        LegacyBattleGlobalResetCall::
                            release_pre_battle_resource_431960,
                        LegacyBattleGlobalResetCall::
                            release_pre_battle_resource_433010,
                        LegacyBattleGlobalResetCall::
                            suspend_audio_stream_485710,
                        LegacyBattleGlobalResetCall::
                            initialize_post_reset_4776a0,
                    } &&
                port.reset_arguments.front() == 404U,
            "only pending callees remain behind the narrow reset port in original order"
        );

        const auto& metrics = port.actor_metric_state();
        test.expect_true(
            std::ranges::all_of(
                metrics.values, [](const auto value) { return value == 0; }
            ) &&
                std::ranges::all_of(
                    metrics.actor_order,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    metrics.selected_mask,
                    [](const auto value) { return value == 0U; }
                ) &&
                std::ranges::all_of(
                    metrics.group_b_order,
                    [](const auto value) { return value == 9U; }
                ) &&
                metrics.group_b_count == 0U && metrics.group_a_count == 0U &&
                metrics.priority_actor_index == 0U,
            "metric order mask counts and priority aliases clear while the untouched group-B order remains"
        );
        test.expect_true(
            std::ranges::all_of(
                startup.reset.block_525470,
                [](const auto value) { return value == 0U; }
            ) &&
                std::ranges::all_of(
                    startup.reset.block_5244e8,
                    [](const auto value) { return value == 0xFFFFFFFFU; }
                ) &&
                startup.reset.values_502940 ==
                    std::array<u32, 5>{0U, 0U, 0U, 0U, 0U} &&
                startup.reset.records_524788[0].value_00 == 0U &&
                startup.enemies[0].role_id == 0U &&
                startup.party[0].role_id == 0U && startup.enemy_count == 0U &&
                startup.party_count == 0U,
            "existing startup typed aliases share the fixed global reset stores"
        );

        test.expect_true(
            state.write_trace.size() == 234U &&
                state.write_trace.front().address == 0x00502940U &&
                state.write_trace.front().value == 0U &&
                state.write_trace[232].address == 0x0053BFFCU &&
                state.write_trace.back().address == 0x0053C154U &&
                state.write_trace.back().count == 6U &&
                write_trace_hash(state) == 0x970D7E940E1225B2ULL,
            "write trace preserves all authoritative store tuples, the post-callee repeat, and final clear"
        );
        test.expect_true(
            byte_at(state, 0x004A7568U) == 2U &&
                byte_at(state, 0x004A7569U) == 0U &&
                byte_at(state, 0x004A7574U) == 0xFFU &&
                byte_at(state, 0x004A75FEU) == 0x10U &&
                byte_at(state, 0x0053C154U) == 0U &&
                state.unmapped_bytes.contains(0x00520E40U) == false &&
                byte_at(state, 0x00ABCDEFU) == 0x5AU,
            "unmapped byte image is little-endian, keeps mapped aliases unique, and preserves untouched bytes"
        );
    }

    {
        LegacyBattleGlobalResetState state;
        LegacyBattleStartupState startup;
        ResetPort port;
        startup.reset.values_502940[0] = 0U;

        const auto result =
            openswd3::battle::reset_legacy_battle_globals(state, startup, port);
        test.expect_true(
            !result.conditional_allocation_released &&
                result.call_count == 8U &&
                std::ranges::find(
                    port.reset_calls,
                    LegacyBattleGlobalResetCall::release_conditional_allocation
                ) == port.reset_calls.end() &&
                result.call_order[3] ==
                    LegacyBattleGlobalResetCallStage::
                        pre_battle_resource_431960,
            "zero conditional token skips only the allocator call and keeps later call ordering"
        );
    }
}
