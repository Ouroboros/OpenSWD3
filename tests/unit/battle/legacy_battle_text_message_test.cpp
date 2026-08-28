#include "openswd3/battle/legacy_battle_text_message.hpp"

#include <map>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleTextMessageCall;
using openswd3::battle::LegacyBattleTextMessageCallReply;
using openswd3::battle::LegacyBattleTextMessageCallRequest;
using openswd3::compat::u32;

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
}
