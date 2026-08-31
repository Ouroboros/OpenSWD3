#include "openswd3/battle/legacy_battle_group_b_action_item_special_option.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleActorGroupBElementState;
using openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadReply;
using openswd3::battle::LegacyBattleGroupBActionItemDefinitionLoadRequest;
using openswd3::battle::LegacyBattleGroupBActionItemNameCopyReply;
using openswd3::battle::LegacyBattleGroupBActionItemNameCopyRequest;
using openswd3::battle::LegacyBattleGroupBActionItemOptionPort;
using openswd3::battle::LegacyBattleGroupBActionItemSpecialOptionStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class OptionPort final : public LegacyBattleGroupBActionItemOptionPort {
public:
    [[nodiscard]] LegacyBattleGroupBActionItemDefinitionLoadReply
    load_action_item_definition(
        const LegacyBattleGroupBActionItemDefinitionLoadRequest& request
    ) override {
        definition_requests.push_back(request);
        if (definition_reply_index >= definition_replies.size()) {
            return {};
        }
        return definition_replies[definition_reply_index++];
    }

    [[nodiscard]] LegacyBattleGroupBActionItemNameCopyReply
    copy_action_item_name(
        const LegacyBattleGroupBActionItemNameCopyRequest& request
    ) override {
        copy_requests.push_back(request);
        if (copy_reply_index >= copy_replies.size()) {
            return {};
        }
        return copy_replies[copy_reply_index++];
    }

    std::vector<LegacyBattleGroupBActionItemDefinitionLoadRequest>
        definition_requests;
    std::vector<LegacyBattleGroupBActionItemDefinitionLoadReply>
        definition_replies;
    std::size_t definition_reply_index{};
    std::vector<LegacyBattleGroupBActionItemNameCopyRequest> copy_requests;
    std::vector<LegacyBattleGroupBActionItemNameCopyReply> copy_replies;
    std::size_t copy_reply_index{};
};

void write_word(
    std::array<u8, 0xA4>& bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] std::shared_ptr<const std::array<u8, 0xA4>>
named_definition(const std::initializer_list<u8> name) {
    auto bytes = std::make_shared<std::array<u8, 0xA4>>();
    std::copy(name.begin(), name.end(), bytes->begin());
    return bytes;
}

[[nodiscard]] LegacyBattleActorGroupBElementState prepared_actor() {
    LegacyBattleActorGroupBElementState actor;
    actor.resource_token = 0x73001234U;
    write_word(actor.resource_bytes, 0x72U, 0x1111U);
    write_word(actor.resource_bytes, 0x76U, 0x2222U);
    return actor;
}

}  // namespace

void test_battle_group_b_action_item_special_option(
    openswd3::test::Context& test
) {
    {
        auto actor = prepared_actor();
        OptionPort port;
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 2U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        completed &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0x33333333U &&
                port.definition_requests.empty() && port.copy_requests.empty(),
            "selectors outside zero and one take the shared full-EAX failure return"
        );
    }

    for (const u32 selector : {0U, 1U}) {
        OptionPort port;
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                nullptr,
                text,
                port,
                {
                    .selector = selector,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        actor_state_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == 0x33333333U,
            "each selector stops at its first actor resource-token access"
        );
    }

    for (const u32 selector : {0U, 1U}) {
        auto actor = prepared_actor();
        actor.resource_token = 0U;
        OptionPort port;
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = selector,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        resource_read_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x00525508U &&
                result.return_edx == (selector == 0U ? 0U : 0x33333333U),
            "each selector preserves the registers established before its missing resource dereference"
        );
    }

    for (const u32 selector : {0U, 1U}) {
        auto actor = prepared_actor();
        write_word(actor.resource_bytes, selector == 0U ? 0x72U : 0x76U, 0U);
        OptionPort port;
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = selector,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        completed &&
                result.return_eax == 0U && port.definition_requests.empty() &&
                port.copy_requests.empty(),
            "zero dynamic definitions return full zero without loading or copying"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.definition_replies.push_back({
            .eax = 0xAAAAAAAAU,
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .definition = named_definition({'A', 0U}),
        });
        port.copy_replies.push_back({.ecx = 0xDDDDDDDDU, .edx = 0xEEEEEEEEU});
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        const auto& load = port.definition_requests[0U];
        const auto& copy = port.copy_requests[0U];
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        completed &&
                result.selected_definition == 0x1111U &&
                result.definition_argument == 0x1111U &&
                load.destination_token == 0x00525518U && load.eax == 0x1111U &&
                load.ecx == 0x00525508U && load.edx == 0x73001234U &&
                copy.eax == 0x0053C16CU && copy.ecx == 0xBBBBBBBBU &&
                copy.edx == 0xCCCCCCCCU && result.return_eax == 1U &&
                result.return_ecx == 0xDDDDDDDDU &&
                result.return_edx == 0xEEEEEEEEU &&
                result.text_bytes_written == 2U && text[0U] == 'A' &&
                text[1U] == 0U,
            "selector zero zero-extends resource word seventy-two and preserves the resource EDX at the loader"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.definition_replies.push_back({
            .eax = 0xAAAAAAAAU,
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .definition = named_definition({'B', 0U}),
        });
        port.copy_replies.push_back({.ecx = 0xDDDDDDDDU, .edx = 0xEEEEEEEEU});
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 1U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                    .entry_edx = 0x33333333U,
                }
            );
        const auto& load = port.definition_requests[0U];
        const auto& copy = port.copy_requests[0U];
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        completed &&
                result.selected_definition == 0x2222U &&
                result.definition_argument == 0x73002222U &&
                load.eax == 0x73002222U && load.ecx == 0x00525508U &&
                load.edx == 0x33333333U && copy.eax == 0xAAAAAAAAU &&
                copy.ecx == 0x0053C16CU && copy.edx == 0xCCCCCCCCU &&
                result.return_eax == 1U && result.return_ecx == 0xDDDDDDDDU &&
                result.return_edx == 0xEEEEEEEEU && text[0U] == 'B',
            "selector one preserves the resource high word and loader EAX around the destination ECX write"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.definition_replies.push_back({
            .eax = 0xAAAAAAAAU,
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .typed_stop = true,
            .definition = named_definition({'L', 0U}),
        });
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        definition_load_typed_stop &&
                result.return_eax == 0xAAAAAAAAU &&
                result.return_ecx == 0xBBBBBBBBU &&
                result.return_edx == 0xCCCCCCCCU &&
                actor.action_composition.resource_definition[0U] == 'L' &&
                port.copy_requests.empty(),
            "loader typed-stop preserves its published definition and return registers before the copy"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.definition_replies.push_back(
            {.definition = named_definition({'C', 0U})}
        );
        port.copy_replies.push_back({
            .ecx = 0xBBBBBBBBU,
            .edx = 0xCCCCCCCCU,
            .typed_stop = true,
        });
        std::array<u8, 24> text{};
        text.fill(0xEEU);
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        name_copy_typed_stop &&
                result.return_eax == 0x0053C16CU &&
                result.return_ecx == 0xBBBBBBBBU &&
                result.return_edx == 0xCCCCCCCCU &&
                result.text_bytes_written == 0U && text[0U] == 0xEEU,
            "the imported copy stop preserves the destination return token without local writes"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        auto definition = std::make_shared<std::array<u8, 0xA4>>();
        definition->fill('X');
        port.definition_replies.push_back({.definition = definition});
        std::array<u8, 24> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 0U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                }
            );
        bool copied = result.text_bytes_written == text.size();
        for (const u8 value : text) {
            copied = copied && value == 'X';
        }
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        name_copy_typed_stop &&
                copied,
            "an unterminated name writes every owned destination byte before the first unowned access"
        );
    }

    {
        auto actor = prepared_actor();
        OptionPort port;
        port.definition_replies.push_back(
            {.definition = named_definition({'D', 'E', 0U})}
        );
        std::array<u8, 2> text{};
        const auto result = openswd3::battle::
            load_legacy_battle_group_b_action_item_special_option(
                &actor,
                text,
                port,
                {
                    .selector = 1U,
                    .actor_token = 0x00525508U,
                    .text_destination_token = 0x0053C16CU,
                }
            );
        test.expect_true(
            result.status ==
                    LegacyBattleGroupBActionItemSpecialOptionStatus::
                        name_copy_typed_stop &&
                result.text_bytes_written == 2U && text[0U] == 'D' &&
                text[1U] == 'E',
            "a short destination preserves the copied prefix and stops before the source terminator"
        );
    }
}
