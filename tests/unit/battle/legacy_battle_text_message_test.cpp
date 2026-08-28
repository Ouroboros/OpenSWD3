#include "openswd3/battle/legacy_battle_text_message.hpp"
#include "openswd3/battle/legacy_battle_text_message_frame.hpp"

#include <array>
#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTextMessageCall;
using openswd3::battle::LegacyBattleTextMessageCallReply;
using openswd3::battle::LegacyBattleTextMessageCallRequest;
using openswd3::compat::u32;

class ActionStreamProvider final
    : public openswd3::asset_runtime::LegacyActionStreamProvider {
public:
    [[nodiscard]] openswd3::asset_runtime::LegacyActionStreamLoadResult
    load_action_stream(u32, u32, bool) override {
        return {};
    }
};

class FrameProvider final
    : public openswd3::rendering::LegacyFramePieceProvider {
public:
    FrameProvider() {
        for (std::size_t index = 0U; index < storage.size(); ++index) {
            storage[index] = {0x11U, 0x22U};
        }
    }

    [[nodiscard]] bool load_frame_piece(
        const u32 resource_id,
        const u32 piece_index,
        openswd3::rendering::LegacyFramePiece& piece
    ) noexcept override {
        resource_ids.push_back(resource_id);
        if (piece_index >= storage.size()) {
            return false;
        }
        piece = {
            .source =
                {
                    .bytes = storage[piece_index],
                    .layout =
                        openswd3::rendering::LegacyBlitSourceLayout::direct_16,
                },
            .width = 1U,
            .height = 1U,
        };
        return true;
    }

    std::array<std::vector<openswd3::compat::u8>, 9> storage;
    std::vector<u32> resource_ids;
};

class FramePort final
    : public openswd3::battle::LegacyBattleTextMessageFramePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageFrameCallReply
    invoke_text_message_frame(
        const openswd3::battle::LegacyBattleTextMessageFrameCallRequest& request
    ) override {
        calls.push_back(request);
        return {
            .eax = request.eax + 1U, .ecx = request.ecx, .edx = request.edx
        };
    }

    std::vector<openswd3::battle::LegacyBattleTextMessageFrameCallRequest>
        calls;
};

struct FrameFixture {
    FrameFixture()
        : raster(framebuffer.geometry()), action_updater(action_streams) {
        panel_action.field_4a = 0x66U;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleTextMessageFrameBindings
    bindings() {
        return {
            .messages = messages,
            .head_token = head,
            .freeze_gate = freeze_gate,
            .panel_action_record = panel_action,
            .color_fade = color_fade,
            .framebuffer = framebuffer,
            .clip = clip,
            .raster = raster,
            .shared_request = shared_request,
            .shared_effects = effects,
            .jitter = jitter,
            .action_updater = action_updater,
            .frame_provider = frame_provider,
            .pixel_conversion = pixel_conversion,
        };
    }

    void push(
        const u32 token,
        const openswd3::battle::LegacyBattleTextMessageRecord& record
    ) {
        if (head == 0U) {
            head = token;
        } else {
            messages.allocations.back().record.next_token = token;
        }
        messages.allocations.push_back({.token = token, .record = record});
    }

    openswd3::battle::LegacyBattleTextMessageState messages;
    u32 head{};
    u32 freeze_gate{};
    openswd3::asset_runtime::LegacyActionRecord panel_action{};
    openswd3::battle::LegacyBattleColorFadeState color_fade;
    openswd3::rendering::LegacyFramebuffer framebuffer{{
        .pitch_bytes = 1280,
        .width = 640,
        .height = 480,
    }};
    openswd3::rendering::LegacyBlitClipRectangle clip{0, 0, 640, 480};
    openswd3::rendering::LegacyRasterGeometryState raster;
    openswd3::rendering::LegacyBlitRequest shared_request;
    openswd3::rendering::LegacyBlitEffectState effects;
    openswd3::rendering::LegacyRleRowJitterState jitter;
    openswd3::rendering::LegacyPixelConversionState pixel_conversion;
    ActionStreamProvider action_streams;
    openswd3::asset_runtime::LegacyActionUpdater action_updater;
    FrameProvider frame_provider;
    FramePort port;
};

class Port final : public openswd3::battle::LegacyBattleTextMessagePort {
public:
    [[nodiscard]] LegacyBattleTextMessageCallReply invoke_text_message(
        const LegacyBattleTextMessageCallRequest& request
    ) override {
        calls.push_back(request);
        auto& index = indices[request.call];
        const auto found = replies.find(request.call);
        if (found == replies.end() || index >= found->second.size()) {
            return {};
        }
        return found->second[index++];
    }

    void push(
        const LegacyBattleTextMessageCall call,
        const LegacyBattleTextMessageCallReply reply
    ) {
        replies[call].push_back(reply);
    }

    std::vector<LegacyBattleTextMessageCallRequest> calls;
    std::map<
        LegacyBattleTextMessageCall,
        std::vector<LegacyBattleTextMessageCallReply>>
        replies;
    std::map<LegacyBattleTextMessageCall, std::size_t> indices;
};

[[nodiscard]] openswd3::battle::LegacyBattleTextMessageRequest request() {
    return {
        .value_04 = 0x118U,
        .value_08 = 0xAU,
        .kind = 0x28U,
        .text_token = 0x004A77E4U,
        .flags = 0x80000002U,
        .entry = {.eax = 1U, .ecx = 2U, .edx = 0xABCD0003U},
    };
}

}  // namespace

void test_battle_text_message(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleTextMessageStatus;
    using openswd3::battle::enqueue_legacy_battle_text_message;

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0U;
        Port port;
        port.push(
            LegacyBattleTextMessageCall::allocate,
            {.eax = 0U, .edx = 0x12340000U}
        );
        const auto result =
            enqueue_legacy_battle_text_message(state, head, port, request());
        test.expect_true(
            result.status ==
                    LegacyBattleTextMessageStatus::allocation_typed_stop &&
                result.allocation_calls == 1U && result.measure_calls == 0U &&
                result.return_registers.eax == 0U &&
                result.return_registers.ecx == 9U &&
                result.return_registers.edx == 0x12340028U && head == 0U &&
                state.allocations.empty(),
            "null allocation stops at the first REP STOSD write"
        );
    }

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0U;
        Port port;
        port.push(
            LegacyBattleTextMessageCall::allocate,
            {.eax = 0x70001000U, .edx = 0xAAAA0000U}
        );
        port.push(LegacyBattleTextMessageCall::measure_text, {.eax = 5U});
        const auto result =
            enqueue_legacy_battle_text_message(state, head, port, request());
        const auto& record = state.allocations[0U].record;
        test.expect_true(
            result.status == LegacyBattleTextMessageStatus::completed &&
                result.appended && result.traversal_count == 0U &&
                head == 0x70001000U && state.allocations.size() == 1U &&
                record.next_token == 0U && record.value_04 == 0x118U &&
                record.value_08 == 0xAU && record.text_length == 5U &&
                record.value_10 == 0U && record.value_14 == 0U &&
                record.flags == 0x80000002U && record.kind == 0x28U &&
                record.padding_1e == 0U && record.text_token == 0x004A77E4U &&
                result.return_registers.eax == 0U &&
                result.return_registers.ecx ==
                    openswd3::battle::kLegacyBattleTextMessageHeadToken &&
                result.return_registers.edx == 0x80000002U,
            "empty list receives one fully initialized record"
        );
        test.expect_true(
            port.calls.size() == 2U && port.calls[0U].argument == 0x24U &&
                port.calls[1U].argument == 0x004A77E4U &&
                port.calls[1U].eax == 0x004A77E4U &&
                port.calls[1U].ecx == 0xAU && port.calls[1U].edx == 0xAAAA0028U,
            "allocator and lstrlen boundaries retain their arguments and registers"
        );
    }

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0U;
        Port first_port;
        first_port.push(
            LegacyBattleTextMessageCall::allocate, {.eax = 0x70001000U}
        );
        first_port.push(LegacyBattleTextMessageCall::measure_text, {.eax = 3U});
        static_cast<void>(enqueue_legacy_battle_text_message(
            state, head, first_port, request()
        ));

        Port second_port;
        second_port.push(
            LegacyBattleTextMessageCall::allocate, {.eax = 0x70002000U}
        );
        second_port.push(
            LegacyBattleTextMessageCall::measure_text, {.eax = 7U}
        );
        auto second = request();
        second.text_token = 0x004A77F0U;
        const auto result = enqueue_legacy_battle_text_message(
            state, head, second_port, second
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageStatus::completed &&
                result.traversal_count == 1U && head == 0x70001000U &&
                state.allocations[0U].record.next_token == 0x70002000U &&
                state.allocations[1U].record.next_token == 0U &&
                result.return_registers.ecx == 0x70001000U,
            "nonempty list traverses to the tail and appends exactly once"
        );
    }

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0U;
        Port port;
        port.push(LegacyBattleTextMessageCall::allocate, {.eax = 0x70003000U});
        port.push(LegacyBattleTextMessageCall::measure_text, {.eax = 9U});
        auto special = request();
        special.value_08 = 0x55667788U;
        special.flags = 0x80000040U;
        const auto result =
            enqueue_legacy_battle_text_message(state, head, port, special);
        const auto& record = state.allocations[0U].record;
        test.expect_true(
            result.status == LegacyBattleTextMessageStatus::completed &&
                record.value_08 == 1U && record.value_14 == 0xFFFFFFE0U &&
                record.flags == 0x80000040U,
            "low-byte bit six overrides value eight and publishes minus thirty-two"
        );
    }

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0U;
        Port port;
        port.push(LegacyBattleTextMessageCall::allocate, {.eax = 0x70004000U});
        port.push(
            LegacyBattleTextMessageCall::measure_text,
            {.eax = 0x11U,
             .ecx = 0x22U,
             .edx = 0x33U,
             .text_access_failed = true}
        );
        const auto result =
            enqueue_legacy_battle_text_message(state, head, port, request());
        const auto& record = state.allocations[0U].record;
        test.expect_true(
            result.status == LegacyBattleTextMessageStatus::text_typed_stop &&
                head == 0U && record.value_04 == 0x118U &&
                record.value_08 == 0xAU && record.kind == 0x28U &&
                record.text_token == 0x004A77E4U && record.text_length == 0U &&
                record.flags == 0U && result.return_registers.eax == 0x11U &&
                result.return_registers.ecx == 0x22U &&
                result.return_registers.edx == 0x33U,
            "text fault preserves field writes before lstrlen and skips the chain"
        );
    }

    {
        openswd3::battle::LegacyBattleTextMessageState state;
        u32 head = 0xDEAD0000U;
        Port port;
        port.push(LegacyBattleTextMessageCall::allocate, {.eax = 0x70005000U});
        port.push(LegacyBattleTextMessageCall::measure_text, {.eax = 4U});
        const auto result =
            enqueue_legacy_battle_text_message(state, head, port, request());
        test.expect_true(
            result.status == LegacyBattleTextMessageStatus::chain_typed_stop &&
                result.stopped_chain_token == 0xDEAD0000U &&
                result.return_registers.eax == 0xDEAD0000U &&
                result.return_registers.ecx == 0xDEAD0000U &&
                result.return_registers.edx == 0x80000002U &&
                head == 0xDEAD0000U && state.allocations.size() == 1U &&
                state.allocations[0U].record.text_length == 4U,
            "missing chain node stops at the first next-pointer read"
        );
    }

    using openswd3::battle::LegacyBattleTextMessageFrameCall;
    using openswd3::battle::LegacyBattleTextMessageFrameStatus;
    using openswd3::battle::advance_legacy_battle_text_message_frame;

    {
        FrameFixture fixture;
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.first_pass_nodes == 0U &&
                result.second_pass_nodes == 0U && fixture.port.calls.empty(),
            "empty text message list completes without crossing a call boundary"
        );
    }

    {
        FrameFixture fixture;
        fixture.push(
            0x70010000U,
            {
                .value_04 = 100U,
                .value_08 = 20U,
                .text_length = 5U,
                .flags = 3U,
                .kind = 2U,
                .text_token = 0x71000000U,
            }
        );
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        const auto& draw = fixture.port.calls[0U];
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.text_calls == 1U && result.release_calls == 0U &&
                fixture.messages.allocations[0U].record.kind == 1U &&
                draw.call == LegacyBattleTextMessageFrameCall::draw_text &&
                draw.arguments[2U] == 295U && draw.arguments[3U] == 26U &&
                draw.arguments[4U] == 0x71000000U &&
                result.retained_tokens == std::vector<u32>{0x70010000U},
            "active node applies low-bit x overrides, draws, decrements, and remains linked"
        );
    }

    {
        FrameFixture fixture;
        fixture.push(0x70020000U, {.kind = 0U});
        fixture.push(
            0x70020024U,
            {.value_04 = 8U, .value_08 = 10U, .kind = 2U, .text_token = 9U}
        );
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.release_calls == 1U && fixture.head == 0x70020024U &&
                fixture.messages.allocations.size() == 1U &&
                fixture.port.calls.back().call ==
                    LegacyBattleTextMessageFrameCall::release_node &&
                fixture.port.calls.back().arguments[0U] == 0x70020000U,
            "expired head is unlinked before release while its active successor remains"
        );
    }

    {
        FrameFixture fixture;
        fixture.push(
            0x70030000U,
            {
                .text_length = 4U,
                .value_10 = 0U,
                .flags = 0x10U,
                .kind = 1U,
                .text_token = 0x72000000U,
            }
        );
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        const auto& record = fixture.messages.allocations[0U].record;
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.color_fade_calls == 2U && result.text_calls == 2U &&
                record.kind == 0U && record.value_10 == 386U &&
                result.retained_tokens == std::vector<u32>{0x70030000U},
            "left-entry node initializes at 640, averages inward, then begins its expiry slide"
        );
    }

    {
        FrameFixture fixture;
        fixture.freeze_gate = 7U;
        fixture.push(
            0x70040000U,
            {
                .text_length = 4U,
                .value_10 = 600U,
                .flags = 0x20U,
                .kind = 0U,
                .text_token = 0x73000000U,
            }
        );
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                fixture.head == 0x70040000U &&
                fixture.messages.allocations[0U].record.value_10 == 600U &&
                result.release_calls == 0U && result.text_calls == 1U,
            "nonzero freeze gate suppresses horizontal expiry movement but still redraws"
        );
    }

    {
        FrameFixture fixture;
        fixture.push(
            0x70050000U,
            {
                .value_08 = 4U,
                .text_length = 2U,
                .value_10 = 230U,
                .value_14 = 0xFFFFFFF0U,
                .flags = 0x40U,
                .kind = 0U,
                .text_token = 0x74000000U,
            }
        );
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        const auto& record = fixture.messages.allocations[0U].record;
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                record.value_14 == 0xFFFFFFECU && record.value_08 == 8U &&
                result.text_calls == 1U && result.release_calls == 0U,
            "vertical expiry subtracts the live step, doubles it, and retains above minus thirty-two"
        );
    }

    {
        FrameFixture fixture;
        for (u32 index = 0U; index < 4U; ++index) {
            fixture.push(
                0x70058000U + index * 0x24U,
                {
                    .text_length = 4U,
                    .flags = 0x20U,
                    .kind = 1U,
                    .text_token = 0x75000000U + index,
                }
            );
        }
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.color_fade_calls == 8U && result.text_calls == 8U &&
                result.release_calls == 0U,
            "multiple nodes may execute more than the seven static fade call sites"
        );
    }

    {
        FrameFixture fixture;
        fixture.head = 0xDEAD1000U;
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status ==
                    LegacyBattleTextMessageFrameStatus::chain_typed_stop &&
                result.stopped_chain_token == 0xDEAD1000U &&
                fixture.head == 0xDEAD1000U && fixture.port.calls.empty(),
            "unknown head token stops at the first record field access"
        );
    }

    {
        FrameFixture fixture;
        fixture.push(0x70060000U, {.flags = 0x80000000U, .kind = 0U});
        const auto result = advance_legacy_battle_text_message_frame(
            fixture.bindings(), fixture.port
        );
        test.expect_true(
            result.status == LegacyBattleTextMessageFrameStatus::completed &&
                result.panel_action_update_calls == 1U &&
                result.rectangle_calls == 1U &&
                result.tiled_frame_calls == 1U &&
                fixture.panel_action.action_id == 0x233BU &&
                fixture.panel_action.base_variant == 0U &&
                result.release_calls == 1U,
            "panel flag updates the shared action, draws its background and frame, then expires"
        );
    }
}
