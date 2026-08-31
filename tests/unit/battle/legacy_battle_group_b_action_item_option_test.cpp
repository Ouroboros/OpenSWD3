#include "openswd3/battle/legacy_battle_group_b_action_item_option.hpp"
#include "test.hpp"

#include <array>
#include <memory>
#include <vector>

namespace {

using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] openswd3::battle::LegacyBattleActorGroupBElementState
prepared_actor() {
    openswd3::battle::LegacyBattleActorGroupBElementState actor;
    actor.resource_token = 0x73001234U;
    write_word(actor.resource_bytes, 0x66U, 0x1111U);
    write_word(actor.resource_bytes, 0x6AU, 0x2222U);
    write_word(actor.resource_bytes, 0x6EU, 0x3333U);
    return actor;
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
named_definition(const std::initializer_list<u8> name) {
    auto bytes = std::make_shared<std::array<u8, 0xA4>>();
    std::size_t index = 0U;
    for (const u8 value : name) {
        (*bytes)[index] = value;
        ++index;
    }
    return bytes;
}

class OptionPort final
    : public openswd3::battle::LegacyBattleGroupBActionItemOptionPort {
public:
    [[nodiscard]] openswd3::battle::
        LegacyBattleGroupBActionItemDefinitionLoadReply
        load_action_item_definition(
            const openswd3::battle::
                LegacyBattleGroupBActionItemDefinitionLoadRequest& request
        ) override {
        load_requests.push_back(request);
        if (actor_to_mutate != nullptr) {
            actor_to_mutate->resource_token = replacement_resource_token;
            actor_to_mutate->resource_bytes = replacement_resource_bytes;
        }
        return load_reply;
    }

    [[nodiscard]] openswd3::battle::LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const openswd3::battle::LegacyBattleGroupBActionItemNameCopyRequest&
            request
    ) override {
        copy_requests.push_back(request);
        return copy_reply;
    }

    std::vector<
        openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadRequest>
        load_requests;
    std::vector<openswd3::battle::LegacyBattleGroupBActionItemNameCopyRequest>
        copy_requests;
    openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadReply
        load_reply{};
    openswd3::battle::LegacyBattleGroupBActionItemNameCopyReply copy_reply{};
    openswd3::battle::LegacyBattleActorGroupBElementState* actor_to_mutate{};
    u32 replacement_resource_token{};
    std::array<u8, 0xA4> replacement_resource_bytes{};
};

}  // namespace

void test_battle_group_b_action_item_option(openswd3::test::Context& test) {
    using openswd3::battle::LegacyBattleGroupBActionItemOptionStatus;

    for (const u32 selector : {3U, 0xFFFFFFFFU}) {
        OptionPort port;
        std::array<u8, 24> text{};
        u32 output = 0xAAAAAAAAU;
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                nullptr,
                text,
                &output,
                port,
                {
                    .selector = selector,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::completed &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0xCCCCCCCCU && output == 0xAAAAAAAAU &&
                port.load_requests.empty() && port.copy_requests.empty(),
            "selectors outside zero through two return failure before touching the actor"
        );
    }

    for (const u32 selector : {0U, 1U, 2U}) {
        OptionPort port;
        std::array<u8, 24> text{};
        u32 output{};
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                nullptr,
                text,
                &output,
                port,
                {
                    .selector = selector,
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0xCCCCCCCCU,
            "each valid selector reaches the original actor-resource access before stopping"
        );
    }

    {
        auto actor = prepared_actor();
        actor.resource_token = 0U;
        OptionPort port;
        std::array<u8, 24> text{};
        u32 output{};
        const auto zero =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        const auto one =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 1U,
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        const auto two =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 2U,
                    .actor_token = 0x00525508U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            zero.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        resource_read_typed_stop &&
                zero.return_ecx == 0U && zero.return_edx == 0xCCCCCCCCU &&
                one.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        resource_read_typed_stop &&
                one.return_ecx == 0x00525508U && one.return_edx == 0U &&
                two.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        resource_read_typed_stop &&
                two.return_eax == 0U && two.return_ecx == 0x00525508U &&
                two.return_edx == 0xCCCCCCCCU,
            "the three resource-pointer faults preserve their distinct register prefixes"
        );
    }

    {
        auto actor = prepared_actor();
        write_word(actor.resource_bytes, 0x66U, 0U);
        write_word(actor.resource_bytes, 0x6AU, 0U);
        write_word(actor.resource_bytes, 0x6EU, 0U);
        OptionPort port;
        std::array<u8, 24> text{};
        u32 output = 0xAAAAAAAAU;
        bool all_failed = true;
        for (const u32 selector : {0U, 1U, 2U}) {
            const auto result =
                openswd3::battle::load_legacy_battle_group_b_action_item_option(
                    &actor,
                    text,
                    &output,
                    port,
                    {
                        .selector = selector,
                        .actor_token = 0x00525508U,
                        .entry_edx = 0xCCCCCCCCU,
                    }
                );
            all_failed = all_failed &&
                result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::completed &&
                result.return_eax == 0U;
        }
        test.expect_true(
            all_failed && output == 0xAAAAAAAAU && port.load_requests.empty() &&
                port.copy_requests.empty(),
            "zero definitions fail without invoking either imported boundary"
        );
    }

    {
        const std::array<u32, 3> expected_arguments{
            0x00001111U, 0x00002222U, 0x73003333U
        };
        const std::array<u32, 3> expected_load_ecx{
            0x73001234U, 0x00525508U, 0x00525508U
        };
        const std::array<u32, 3> expected_load_edx{
            0xCCCCCCCCU, 0x73001234U, 0xCCCCCCCCU
        };
        for (u32 selector = 0U; selector < 3U; ++selector) {
            auto actor = prepared_actor();
            OptionPort port;
            port.load_reply.definition = named_definition({'A', 'B', 0U});
            port.copy_reply = {
                .eax = 0xAAAAAAAAU,
                .ecx = 0xBBBBBBBBU,
                .edx = 0xDDDDDDDDU,
            };
            std::array<u8, 24> text{};
            text.fill(0xEEU);
            u32 output{};
            const auto result =
                openswd3::battle::load_legacy_battle_group_b_action_item_option(
                    &actor,
                    text,
                    &output,
                    port,
                    {
                        .selector = selector,
                        .actor_token = 0x00525508U,
                        .text_destination_token = 0x0053C16CU,
                        .output_token = 0x0012FFF0U,
                        .entry_edx = 0xCCCCCCCCU,
                    }
                );
            test.expect_true(
                result.status ==
                        LegacyBattleGroupBActionItemOptionStatus::completed &&
                    result.return_eax == 1U &&
                    result.return_ecx == 0xBBBBBBBBU &&
                    result.return_edx == 0xDDDDDDDDU &&
                    result.definition_argument ==
                        expected_arguments[selector] &&
                    port.load_requests.size() == 1U &&
                    port.load_requests[0U].destination_token == 0x00525518U &&
                    port.load_requests[0U].ecx == expected_load_ecx[selector] &&
                    port.load_requests[0U].edx == expected_load_edx[selector] &&
                    output == 0x1111U && text[0U] == 'A' && text[1U] == 'B' &&
                    text[2U] == 0U && text[3U] == 0xEEU &&
                    port.copy_requests.size() == 1U &&
                    port.copy_requests[0U].destination_token == 0x0053C16CU &&
                    port.copy_requests[0U].source_token == 0x00525518U,
                "each selector preserves its loader registers, selector-two high word, shared output, and copied name"
            );
        }
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.actor_to_mutate = &actor;
        port.replacement_resource_token = 0x74005678U;
        write_word(port.replacement_resource_bytes, 0x66U, 0x4444U);
        port.load_reply.definition = named_definition({'N', 0U});
        std::array<u8, 24> text{};
        u32 output{};
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 1U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                    .entry_edx = 0xCCCCCCCCU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::completed &&
                output == 0x4444U && text[0U] == 'N' && text[1U] == 0U,
            "the loader return path rereads both the resource owner and the loaded definition"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.load_reply = {
            .eax = 0xAAAAAAAAU,
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .typed_stop = true,
            .definition = named_definition({'S', 0U}),
        };
        std::array<u8, 24> text{};
        u32 output = 0xDDDDDDDDU;
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        definition_load_typed_stop &&
                result.return_eax == 0xAAAAAAAAU &&
                result.return_ecx == 0xBBBBBBBBU &&
                result.return_edx == 0xCCCCCCCCU &&
                actor.action_composition.resource_definition[0U] == 'S' &&
                output == 0xDDDDDDDDU && port.copy_requests.empty(),
            "a loader stop publishes its completed definition prefix and register reply"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.actor_to_mutate = &actor;
        port.replacement_resource_token = 0U;
        port.load_reply.definition = named_definition({'R', 0U});
        std::array<u8, 24> text{};
        u32 output = 0xAAAAAAAAU;
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 2U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        resource_reread_typed_stop &&
                result.return_eax == 0x0012FFF0U && result.return_ecx == 0U &&
                result.return_edx == 0U && output == 0xAAAAAAAAU &&
                port.copy_requests.empty(),
            "selector two stops at its post-loader resource reread with the output-store prefix registers"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.load_reply.definition = named_definition({'C', 0U});
        port.copy_reply = {
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .typed_stop = true,
        };
        std::array<u8, 24> text{};
        text.fill(0xEEU);
        u32 output{};
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        name_copy_typed_stop &&
                result.return_eax == 0x0053C16CU &&
                result.return_ecx == 0xBBBBBBBBU &&
                result.return_edx == 0xCCCCCCCCU && output == 0x1111U &&
                result.text_bytes_written == 0U && text[0U] == 0xEEU,
            "the imported copy stop preserves the output prefix and destination return token"
        );
    }

    {
        auto actor = prepared_actor();
        actor.action_composition.resource_definition.fill('X');
        OptionPort port;
        std::array<u8, 24> text{};
        text.fill(0xEEU);
        u32 output{};
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                &output,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                }
            );
        bool copied_prefix = result.text_bytes_written == text.size();
        for (const u8 value : text) {
            copied_prefix = copied_prefix && value == 'X';
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        name_copy_typed_stop &&
                result.return_eax == 0x0053C16CU && output == 0x1111U &&
                copied_prefix,
            "an unterminated name writes every owned destination byte then stops at the first unowned byte"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.load_reply.definition = named_definition({'Z', 0U});
        std::array<u8, 24> text{};
        u32 output = 0xAAAAAAAAU;
        const auto result =
            openswd3::battle::load_legacy_battle_group_b_action_item_option(
                &actor,
                text,
                nullptr,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .output_token = 0x0012FFF0U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemOptionStatus::
                        output_write_typed_stop &&
                result.return_eax == 0x1111U &&
                result.return_ecx == 0x0012FFF0U &&
                result.return_edx == 0x0053C16CU && output == 0xAAAAAAAAU &&
                port.copy_requests.empty(),
            "the output typed-stop occurs after the dynamic resource reread and before string copy"
        );
    }
}
