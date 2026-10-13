#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <string>
#include <utility>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class SelectionPort final
    : public openswd3::battle::LegacyBattleActorResourceSelectionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::compat::u32 seek_mon_file(
        const openswd3::compat::u32 handle,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        if (seek_calls % 3U == 1U) {
            profile_ids.push_back(
                static_cast<openswd3::compat::u16>(
                    (std::bit_cast<openswd3::compat::u32>(distance) -
                     auxiliary_root - 0x200U) /
                    4U
                )
            );
        }

        return LegacyBattleMonDatabaseFixture::seek_mon_file(
            handle, distance, origin
        );
    }

    void report_missing_runtime_word(
        const openswd3::compat::u16 resource_id
    ) override {
        diagnostics.push_back(resource_id);
    }
    std::vector<openswd3::compat::u16> profile_ids;
    std::vector<openswd3::compat::u16> diagnostics;
};

class StatePort final
    : public openswd3::battle::LegacyBattleActorListStatePort {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleActorListStateCallReply
    publish_message(const openswd3::compat::u32 text_token) override {
        ++message_calls;
        last_text = text_token;
        return {.publish_message_token = true, .message_token = 0x74000000U};
    }
    [[nodiscard]] openswd3::battle::LegacyBattleActorListStateCallReply
    play_sample(
        const openswd3::compat::u32, const openswd3::compat::u32
    ) override {
        ++sample_calls;
        return {};
    }
    openswd3::compat::u32 message_calls{};
    openswd3::compat::u32 sample_calls{};
    openswd3::compat::u32 last_text{};
};

class QueryPort final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::compat::u32 seek_mon_file(
        const openswd3::compat::u32 handle,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        set_profile_dword(0x10U, index);
        if (seek_calls % 3U == 1U) {
            last_profile = static_cast<openswd3::compat::u16>(
                (std::bit_cast<openswd3::compat::u32>(distance) -
                 auxiliary_root - 0x200U) /
                4U
            );
        }

        return LegacyBattleMonDatabaseFixture::seek_mon_file(
            handle, distance, origin
        );
    }

    openswd3::compat::u16 last_profile{};
    openswd3::compat::u32 index{};
};

void test_actor_resource_release_prefixes(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using openswd3::compat::u32;
    for (const u32 path : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
        LegacyBattleActorListQueryState list;
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xBEEFU;
        list.resource_head_token = 0x12345678U;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000020U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x777U,
             .secondary_quantity = 7,
             .name = {}},
            {.token = 0x76000020U,
             .resource_id = 0x777U,
             .primary_quantity = 4U,
             .secondary_quantity = 2,
             .name = {}},
        };

        if (path == 0U) {
            list.selected_resource_token = 0U;
            list.next_resource_head_token = 0xDEADBEEFU;
        } else if (path == 1U) {
            list.resources[0U].next_token = 0U;
        } else if (path == 2U) {
            list.next_resource_head_token = 0xDEADBEEFU;
        } else if (path == 3U) {
            list.resources[0U].next_token = 0xDEADBEEFU;
        } else if (path == 4U) {
            list.selected_resource_token = 0xDEADBEEFU;
        } else if (path == 6U) {
            list.selected_resource_token = 0x76000000U;
            list.resources[0U].resource_id = 0x777U;
            list.resources[0U].primary_quantity = 4U;
            list.resources[0U].secondary_quantity = 1;
        }

        const auto result = release_legacy_battle_actor_resource(
            path == 7U ? nullptr : &list, path == 5U ? nullptr : &workspace
        );
        const auto expected_status = path < 2U
            ? LegacyBattleActorListQueryStatus::completed
            : path == 5U
            ? LegacyBattleActorListQueryStatus::actor_state_typed_stop
            : path == 7U
            ? LegacyBattleActorListQueryStatus::list_owner_typed_stop
            : LegacyBattleActorListQueryStatus::resource_node_typed_stop;
        const u32 expected_head = path == 1U ? 0U
            : path == 4U                     ? 0x76000010U
            : path == 5U                     ? 0x76000020U
            : path == 6U                     ? 0x76000000U
            : path == 7U                     ? 0x12345678U
                                             : 0xDEADBEEFU;
        test.expect_true(
            result.status == expected_status &&
                (path >= 2U || result.output_word == 0U) &&
                list.resource_head_token == expected_head &&
                list.selected_resource_token ==
                    (path == 0U || path == 6U ? 0U
                         : path == 4U         ? 0xDEADBEEFU
                                              : 0x76000020U) &&
                workspace.tail_words[2U] == (path == 6U ? 0x777U : 0xBEEFU) &&
                list.resources.size() == (path == 6U ? 2U : 3U) &&
                list.resources.back().primary_quantity == 4U &&
                list.resources.back().secondary_quantity == 2 &&
                list.resources[path == 6U ? 0U : 1U].secondary_quantity == 7,
            "resource release preserves each normal-zero or missing-owner prefix including selected publication before workspace failure and actual erase before relink failure"
        );
    }

    for (const u32 path : {0U, 1U, 2U, 3U}) {
        LegacyBattleActorListQueryState list;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        configuration.actor_record_token = 0x71000000U;
        configuration.actor_record[2U] = 0xABCD000AU;
        workspace.tail_words[2U] = 0xBEEFU;
        item_effect.mode_flags = path == 0U ? 0U : 0x80U;
        list.next_resource_head_token = path == 2U ? 0xDEADBEEFU : 0x76000000U;
        list.selected_resource_token = path == 3U ? 0x76000000U : 0x76000010U;
        list.primary_required = 9U;
        list.secondary_required = 3U;
        list.resources = {
            {.token = 0x76000000U,
             .next_token = 0x76000010U,
             .resource_id = 0x777U,
             .primary_quantity = 4U,
             .secondary_quantity = 1,
             .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x777U,
             .secondary_quantity = 2,
             .name = {},
             .gate_word_48 = 0x0402U,
             .capacity_gate_flags = 0x2000U},
        };

        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.status ==
                    (path < 2U ? LegacyBattleActorListQueryStatus::completed
                               : LegacyBattleActorListQueryStatus::
                                     resource_node_typed_stop) &&
                configuration.actor_record[2U] ==
                    (path < 2U ? 0xABCD0007U : 0xABCD000AU) &&
                list.primary_required == (path == 1U ? 0U : 9U) &&
                list.secondary_required == (path < 2U ? 0U : 3U) &&
                list.selected_resource_token ==
                    (path == 1U || path == 3U ? 0U : 0x76000010U) &&
                list.resources.size() == (path == 3U ? 1U : 2U) &&
                list.resources.back().secondary_quantity == 2 &&
                list.resources.back().gate_word_48 ==
                    (path == 1U ? 0x0401U : 0x0402U) &&
                workspace.tail_words[2U] ==
                    (path == 1U || path == 3U ? 0x777U : 0xBEEFU),
            "list action discards the normal zero WORD, clears required capacity only after successful consumption and preserves unrefreshed capacity after resource failure"
        );
    }
}

void test_resource_head_publication(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    using openswd3::compat::u32;
    constexpr std::array<u32, 6> heads{
        0U, 1U, 0x7FFFFFFFU, 0x80000000U, 0xFFFFFFFFU, 0x76000000U
    };

    for (const u32 head : heads) {
        for (const bool already_current : {false, true}) {
            LegacyBattleActorListQueryState list;
            list.resource_head_token = already_current ? head : ~head;
            list.next_resource_head_token = head;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = 0x76000000U,
                 .next_token = 0x76000010U,
                 .resource_id = 0xFFFFU,
                 .primary_quantity = 0xABCDU,
                 .secondary_quantity = -1,
                 .tertiary_quantity = 2,
                 .name = "retained",
                 .category_mask = 0xFFFFFFFFU,
                 .mode_flags = 0xFFU,
                 .capacity_gate_flags = 0xFFFFU},
                {.token = 0x76000010U, .name = "next"},
            };

            const auto* first = &list.resources.front();
            const auto status = commit_legacy_battle_actor_resource_list(&list);
            test.expect_true(
                status == LegacyBattleActorListQueryStatus::completed &&
                    list.resource_head_token == head &&
                    list.next_resource_head_token == head &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    list.resources.size() == 2U &&
                    &list.resources.front() == first &&
                    first->next_token == 0x76000010U &&
                    first->resource_id == 0xFFFFU &&
                    first->primary_quantity == 0xABCDU &&
                    first->secondary_quantity == -1 &&
                    first->tertiary_quantity == 2 &&
                    first->name == "retained" &&
                    first->category_mask == 0xFFFFFFFFU &&
                    first->mode_flags == 0xFFU &&
                    first->capacity_gate_flags == 0xFFFFU,
                "resource publication copies the full next head without node access, selection changes or ownership changes"
            );
        }
    }

    test.expect_true(
        commit_legacy_battle_actor_resource_list(nullptr) ==
            LegacyBattleActorListQueryStatus::list_owner_typed_stop,
        "resource publication propagates the existing missing-owner failure"
    );

    for (u32 consumer = 0U; consumer < 5U; ++consumer) {
        for (const bool missing_root : {false, true}) {
            LegacyBattleActorListQueryState list;
            list.resource_head_token = 0xDEAD0000U;
            list.next_resource_head_token =
                missing_root ? 0xFEDCBA98U : 0x76000000U;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x0300U,
                 .primary_quantity = 1U,
                 .secondary_quantity = 2,
                 .tertiary_quantity = 3,
                 .name = "published",
                 .category_mask = 0x08000010U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x2000U},
            };

            LegacyBattleActorListQueryStatus status;
            switch (consumer) {
            case 0U: {
                std::array<u8, 20> name{};
                u16 quantity = 0U;
                const auto result = query_legacy_battle_actor_flagged_resource(
                    &list, 1U, name, quantity
                );
                status = result.status;
                break;
            }

            case 1U: {
                std::array<u8, 20> name{};
                u16 quantity = 0U;
                const auto result = query_legacy_battle_actor_mode_resource(
                    &list, 0U, 1U, name, quantity
                );
                status = result.status;
                break;
            }

            case 2U: {
                const auto result = count_legacy_battle_actor_mode_resources(
                    &list, 0x005029D0U, {}
                );
                status = result.status;
                test.expect_true(
                    result.count == 0U,
                    "mode count clears its output before a missing first node"
                );
                break;
            }

            case 3U: {
                const auto result = count_legacy_battle_actor_resource_list(
                    &list,
                    0x005029D0U,
                    {.category_selector = 0U, .initial_count = 0xABCDU}
                );
                status = result.status;
                test.expect_true(
                    result.count == (missing_root ? 0xABCDU : 0xABCEU),
                    "resource count retains the caller's initial word before the first-node failure"
                );
                break;
            }

            default: {
                const auto result = query_legacy_battle_actor_resource_list(
                    &list,
                    nullptr,
                    0x005029D0U,
                    {.category_selector = 0U, .occurrence = 1U}
                );
                status = result.status;
                test.expect_true(
                    !missing_root || result.output_flags == 0U,
                    "resource query clears flags after publication and before a missing first node"
                );
                break;
            }
            }

            const auto failure = consumer < 3U
                ? LegacyBattleActorListQueryStatus::resource_node_typed_stop
                : LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
            const u32 final_head = missing_root
                ? 0xFEDCBA98U
                : (consumer == 2U || consumer == 3U ? 0U : 0x76000010U);
            test.expect_true(
                status ==
                        (missing_root
                             ? failure
                             : LegacyBattleActorListQueryStatus::completed) &&
                    list.resource_head_token == final_head &&
                    list.next_resource_head_token ==
                        (missing_root ? 0xFEDCBA98U : 0x76000000U) &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    list.resources.size() == 2U &&
                    list.resources.back().primary_quantity == 1U &&
                    list.resources.back().secondary_quantity == 2 &&
                    list.resources.back().tertiary_quantity == 3,
                "resource consumers start from the published root and preserve publication on their existing first-node failure"
            );
        }
    }
}

void test_flagged_resource_display(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    struct QuantityVector {
        u16 secondary;
        u16 tertiary;
        u16 expected;
    };

    constexpr std::array<QuantityVector, 7> quantities{{
        {0U, 0U, 0U},
        {1U, 2U, 3U},
        {0xFFFFU, 1U, 0U},
        {0x7FFFU, 1U, 0x8000U},
        {0x8000U, 0U, 0x8000U},
        {0xFFFFU, 0xFFFFU, 0xFFFEU},
        {0x8000U, 0x8000U, 0U},
    }};
    const std::array<std::string, 3> names{
        "",
        std::string(19U, 'n'),
        std::string("a\0", 2U) + std::string(25U, 'z')
    };

    for (const auto& quantity : quantities) {
        for (const auto& source_name : names) {
            for (const u32 occurrence : {0U, 1U, 2U, 3U, 0xFFFFFFFFU}) {
                LegacyBattleActorListQueryState list;
                list.resource_head_token = 0xDEAD0000U;
                list.next_resource_head_token = 0x76000000U;
                list.selected_resource_token = 0xCAFEBABEU;
                list.resources = {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}}
                };

                for (u32 index = 1U; index <= 4U; ++index) {
                    list.resources.push_back({
                        .token = 0x76000000U + index * 0x10U,
                        .next_token = index == 4U
                            ? 0U
                            : 0x76000000U + (index + 1U) * 0x10U,
                        .secondary_quantity =
                            std::bit_cast<openswd3::compat::i16>(
                                quantity.secondary
                            ),
                        .tertiary_quantity =
                            std::bit_cast<openswd3::compat::i16>(
                                quantity.tertiary
                            ),
                        .name = source_name,
                        .capacity_gate_flags = static_cast<u16>(
                            index == 2U || index == 4U ? 0x2000U : 0U
                        ),
                    });
                }

                const auto* original_nodes = list.resources.data();
                std::array<u8, 20> name;
                name.fill(0xA5U);
                auto expected_name = name;
                const bool found = occurrence <= 2U;
                if (found) {
                    const auto end = source_name.find('\0');
                    const auto length =
                        end == std::string::npos ? source_name.size() : end;
                    std::copy_n(
                        source_name.begin(), length, expected_name.begin()
                    );
                    expected_name[length] = 0U;
                }

                u16 output_quantity = 0xBEEFU;
                const auto result = query_legacy_battle_actor_flagged_resource(
                    &list, occurrence, name, output_quantity
                );
                const u32 expected_head = occurrence == 0U ? 0x76000010U
                    : occurrence == 1U                     ? 0x76000020U
                    : occurrence == 2U                     ? 0x76000040U
                                                           : 0U;
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.found == found && name == expected_name &&
                        output_quantity ==
                            (found ? quantity.expected : 0xBEEFU) &&
                        list.resource_head_token == expected_head &&
                        list.next_resource_head_token == 0x76000000U &&
                        list.selected_resource_token == 0xCAFEBABEU &&
                        list.resources.data() == original_nodes &&
                        list.resources.size() == 5U &&
                        list.resources.back().name == source_name &&
                        std::bit_cast<u16>(
                            list.resources.back().secondary_quantity
                        ) == quantity.secondary &&
                        std::bit_cast<u16>(
                            list.resources.back().tertiary_quantity
                        ) == quantity.tertiary,
                    "actual flagged query preserves destructive ordinal scans, null-terminated names, untouched tails and word sums"
                );
            }
        }
    }

    for (u32 failure = 0U; failure < 3U; ++failure) {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token =
            failure == 0U ? 0xBAD00000U : 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
        };

        if (failure == 2U) {
            list.resources.push_back({
                .token = 0x76000010U,
                .name = "name",
                .capacity_gate_flags = 0x2000U,
            });
        }

        std::array<u8, 20> name;
        name.fill(0xA5U);
        const auto original_name = name;
        u16 quantity = 0xBEEFU;
        const std::span<u8> destination =
            failure == 2U ? std::span<u8>{} : std::span<u8>{name};
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 1U, destination, quantity
        );
        test.expect_true(
            !result.found &&
                result.status ==
                    (failure == 2U ? LegacyBattleActorListQueryStatus::
                                         list_text_typed_stop
                                   : LegacyBattleActorListQueryStatus::
                                         resource_node_typed_stop) &&
                name == original_name && quantity == 0xBEEFU &&
                list.resource_head_token ==
                    (failure == 0U ? 0xBAD00000U : 0x76000010U),
            "missing root, missing next and unavailable text retain the actual publication and unchanged output prefix"
        );
    }

    for (const bool missing_reloaded_node : {false, true}) {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .secondary_quantity = 99,
             .name = std::string(1U, missing_reloaded_node ? '\x30' : '\x20'),
             .capacity_gate_flags = 0x2000U},
            {.token = 0x76000020U,
             .secondary_quantity = 3,
             .tertiary_quantity = 4,
             .name = "reloaded"},
        };

        const std::span<u8> aliased_name{
            reinterpret_cast<u8*>(&list.resource_head_token),
            sizeof(list.resource_head_token)
        };

        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 1U, aliased_name, quantity
        );
        test.expect_true(
            result.found == !missing_reloaded_node &&
                result.status ==
                    (missing_reloaded_node
                         ? LegacyBattleActorListQueryStatus::
                               resource_node_typed_stop
                         : LegacyBattleActorListQueryStatus::completed) &&
                list.resource_head_token ==
                    (missing_reloaded_node ? 0x76000030U : 0x76000020U) &&
                quantity == (missing_reloaded_node ? 0xBEEFU : 7U),
            "real text alias writes precede the shared-head reload and quantity access without callbacks or fabricated replies"
        );
    }

    std::array<u8, 20> name;
    name.fill(0xA5U);
    const auto original_name = name;
    u16 quantity = 0xBEEFU;
    const auto result =
        query_legacy_battle_actor_flagged_resource(nullptr, 1U, name, quantity);
    test.expect_true(
        result.status ==
                LegacyBattleActorListQueryStatus::list_owner_typed_stop &&
            !result.found && name == original_name && quantity == 0xBEEFU,
        "missing actual list preserves both output owners on the existing failure"
    );
}

void test_mode_resource_display(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    const std::array<u32, 5> modes{0U, 1U, 2U, 0x80000000U, 0xFFFFFFFFU};
    const std::array<u32, 5> occurrences{0U, 1U, 2U, 3U, 0xFFFFFFFFU};
    const std::array<u32, 5> fixed_nodes{1U, 2U, 2U, 2U, 2U};
    const std::array<u32, 5> filtered_nodes{1U, 3U, 4U, 0U, 0U};
    const std::array<std::string, 5> labels{
        "", "stale", "fixed", "first", "second"
    };

    const std::array<u16, 5> quantities{0U, 0U, 3U, 0x8000U, 0xFFFEU};
    const std::array<std::pair<u8, bool>, 7> output_gates{{
        {0U, false},
        {1U, true},
        {2U, false},
        {4U, true},
        {5U, true},
        {0x80U, false},
        {0xFFU, true},
    }};
    for (const u32 mode : modes) {
        for (std::size_t ordinal_index = 0U; ordinal_index < occurrences.size();
             ++ordinal_index) {
            for (const auto& [mode_flags, writes_output] : output_gates) {
                LegacyBattleActorListQueryState list;
                list.resource_head_token = 0xDEAD0000U;
                list.next_resource_head_token = 0x76000000U;
                list.selected_resource_token = 0xCAFEBABEU;
                list.resources = {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .next_token = 0x76000020U,
                     .resource_id = 0x0111U,
                     .secondary_quantity = -1,
                     .tertiary_quantity = 1,
                     .name = labels[1U],
                     .category_mask = 0x80000000U,
                     .mode_flags = mode_flags},
                    {.token = 0x76000020U,
                     .next_token = 0x76000030U,
                     .resource_id = 0x0300U,
                     .secondary_quantity = 1,
                     .tertiary_quantity = 2,
                     .name = labels[2U],
                     .category_mask = 0x08000000U,
                     .mode_flags = mode_flags},
                    {.token = 0x76000030U,
                     .next_token = 0x76000040U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = 0x7FFF,
                     .tertiary_quantity = 1,
                     .name = labels[3U],
                     .category_mask = 0x08000000U,
                     .mode_flags = mode_flags},
                    {.token = 0x76000040U,
                     .resource_id = 0x0333U,
                     .secondary_quantity = -1,
                     .tertiary_quantity = -1,
                     .name = labels[4U],
                     .category_mask = 0x88000000U,
                     .mode_flags = mode_flags},
                };

                const auto* original_storage = list.resources.data();
                std::array<u8, 20> name;
                name.fill(0xA5U);
                const auto original_name = name;
                u16 quantity = 0xBEEFU;
                const auto result = query_legacy_battle_actor_mode_resource(
                    &list, mode, occurrences[ordinal_index], name, quantity
                );
                const u32 node_index = mode == 0U
                    ? fixed_nodes[ordinal_index]
                    : filtered_nodes[ordinal_index];
                const u32 expected_head =
                    node_index == 0U ? 0U : 0x76000000U + node_index * 0x10U;
                const bool fixed_selection = mode == 0U && ordinal_index != 0U;
                const u32 expected_selected =
                    node_index == 0U || fixed_selection ? 0xCAFEBABEU
                                                        : expected_head;
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.found == (node_index != 0U) &&
                        list.resource_head_token == expected_head &&
                        list.next_resource_head_token == 0x76000000U &&
                        list.selected_resource_token == expected_selected &&
                        list.resources.data() == original_storage &&
                        list.resources.size() == 5U &&
                        list.resources[1U].name == labels[1U] &&
                        list.resources[4U].name == labels[4U],
                    "full DWORD mode and occurrence retain fixed-ID selection asymmetry, stale zero matches and destructive exhaustion"
                );
                if (node_index != 0U && writes_output) {
                    test.expect_true(
                        quantity == quantities[node_index] &&
                            std::string(
                                reinterpret_cast<const char*>(name.data())
                            ) == labels[node_index] &&
                            name[labels[node_index].size()] == 0U &&
                            name[labels[node_index].size() + 1U] == 0xA5U,
                        "each mode output BYTE gate writes the actual NUL-terminated name and wrapped quantity without clearing its tail"
                    );
                } else {
                    test.expect_true(
                        name == original_name && quantity == 0xBEEFU,
                        "closed output gate and normal exhaustion preserve both caller outputs"
                    );
                }
            }
        }
    }

    const std::array<std::array<u16, 3>, 7> quantity_cases{{
        {0U, 0U, 0U},
        {1U, 2U, 3U},
        {0xFFFFU, 1U, 0U},
        {0x7FFFU, 1U, 0x8000U},
        {0x8000U, 0U, 0x8000U},
        {0xFFFFU, 0xFFFFU, 0xFFFEU},
        {0x8000U, 0x8000U, 0U},
    }};
    const std::array<std::string, 3> source_names{
        "",
        std::string(19U, 'n'),
        std::string("a\0", 2U) + std::string(25U, 'z')
    };

    const std::array<std::size_t, 3> copied_sizes{0U, 19U, 1U};
    for (const u32 mode : {0U, 1U}) {
        for (const auto& quantity_case : quantity_cases) {
            for (std::size_t name_index = 0U; name_index < source_names.size();
                 ++name_index) {
                LegacyBattleActorListQueryState list;
                list.next_resource_head_token = 0x76000000U;
                list.selected_resource_token = 0xCAFEBABEU;
                list.resources = {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .resource_id =
                         static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                     .secondary_quantity = std::bit_cast<openswd3::compat::i16>(
                         quantity_case[0U]
                     ),
                     .tertiary_quantity = std::bit_cast<openswd3::compat::i16>(
                         quantity_case[1U]
                     ),
                     .name = source_names[name_index],
                     .category_mask = 0x08000000U,
                     .mode_flags = 5U},
                };

                std::array<u8, 20> name;
                name.fill(0xA5U);
                auto expected_name = name;
                std::copy_n(
                    source_names[name_index].begin(),
                    copied_sizes[name_index],
                    expected_name.begin()
                );
                expected_name[copied_sizes[name_index]] = 0U;
                u16 quantity = 0xBEEFU;
                const auto result = query_legacy_battle_actor_mode_resource(
                    &list, mode, 1U, name, quantity
                );
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.found && name == expected_name &&
                        quantity == quantity_case[2U] &&
                        list.resources[1U].name == source_names[name_index],
                    "both mode paths consume WORD quantities and only the source bytes through its first NUL"
                );
            }
        }
    }

    for (const u32 mode : {0U, 1U}) {
        for (const bool available_quantity_node : {false, true}) {
            LegacyBattleActorListQueryState list;
            list.next_resource_head_token = 0x76000000U;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id =
                     static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                 .secondary_quantity = 10,
                 .tertiary_quantity = 11,
                 .name =
                     std::string(1U, available_quantity_node ? '\x20' : '\x30'),
                 .category_mask = 0x08000000U,
                 .mode_flags = 1U},
                {.token = 0x76000020U,
                 .secondary_quantity = 3,
                 .tertiary_quantity = 4,
                 .name = "quantity"},
            };

            const std::span<u8> name(
                reinterpret_cast<u8*>(&list.resource_head_token), sizeof(u32)
            );
            u16 quantity = 0xBEEFU;
            const auto result = query_legacy_battle_actor_mode_resource(
                &list, mode, 1U, name, quantity
            );
            test.expect_true(
                result.status ==
                        (available_quantity_node
                             ? LegacyBattleActorListQueryStatus::completed
                             : LegacyBattleActorListQueryStatus::
                                   resource_node_typed_stop) &&
                    result.found == available_quantity_node &&
                    quantity == (available_quantity_node ? 7U : 0xBEEFU) &&
                    list.resource_head_token ==
                        (available_quantity_node ? 0x76000020U : 0x76000030U) &&
                    list.selected_resource_token ==
                        (mode == 0U ? 0xCAFEBABEU : 0x76000010U),
                "the actual name alias changes the shared head before quantity lookup and retains selection and copy on failure"
            );
        }

        for (const bool empty_output : {false, true}) {
            LegacyBattleActorListQueryState list;
            list.next_resource_head_token = 0x76000000U;
            list.selected_resource_token = 0xCAFEBABEU;
            list.resources = {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id =
                     static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                 .name = empty_output ? "ok" : std::string(20U, 'x'),
                 .category_mask = 0x08000000U,
                 .mode_flags = 1U},
            };

            std::array<u8, 20> name;
            name.fill(0xA5U);
            const auto original_name = name;
            u16 quantity = 0xBEEFU;
            const auto result = query_legacy_battle_actor_mode_resource(
                &list,
                mode,
                1U,
                empty_output ? std::span<u8>{} : std::span<u8>(name),
                quantity
            );
            test.expect_true(
                result.status ==
                        LegacyBattleActorListQueryStatus::
                            list_text_typed_stop &&
                    !result.found && name == original_name &&
                    quantity == 0xBEEFU &&
                    list.resource_head_token == 0x76000010U &&
                    list.selected_resource_token ==
                        (mode == 0U ? 0xCAFEBABEU : 0x76000010U),
                "both text capacity failures preserve the original fixed-ID or ordinal selection prefix"
            );
        }
    }

    for (const bool missing_root : {false, true}) {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0xCAFEBABEU;
        list.resources = missing_root
            ? std::vector<LegacyBattleActorListResourceNode>{}
            : std::vector<LegacyBattleActorListResourceNode>{
                  {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}}
              };

        std::array<u8, 20> name;
        name.fill(0xA5U);
        const auto original_name = name;
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 1U, 1U, name, quantity
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::
                        resource_node_typed_stop &&
                !result.found && name == original_name && quantity == 0xBEEFU &&
                list.resource_head_token ==
                    (missing_root ? 0x76000000U : 0x76000010U) &&
                list.selected_resource_token == 0xCAFEBABEU,
            "missing root and candidate fail after the original current publication and preserve selection and outputs"
        );
    }

    std::array<u8, 20> name;
    name.fill(0xA5U);
    const auto original_name = name;
    u16 quantity = 0xBEEFU;
    const auto result = query_legacy_battle_actor_mode_resource(
        nullptr, 0U, 1U, name, quantity
    );
    test.expect_true(
        result.status ==
                LegacyBattleActorListQueryStatus::list_owner_typed_stop &&
            !result.found && name == original_name && quantity == 0xBEEFU,
        "missing actual mode-list owner preserves both output owners on the existing failure"
    );
}

}  // namespace

void test_battle_actor_list_query(openswd3::test::Context& test) {
    test_actor_resource_release_prefixes(test);
    test_resource_head_publication(test);
    test_flagged_resource_display(test);
    test_mode_resource_display(test);
    using namespace openswd3::battle;
    const std::array<openswd3::compat::u16, 4> table{10U, 20U, 30U, 40U};

    LegacyBattleGroupAActionExecutionState actor;
    actor.next_list_index = 0x71000000U;
    LegacyBattleActorListQueryState list{
        .owner_token = 0x71000000U,
        .head_token = 0x72000000U,
        .nodes =
            {
                {.token = 0x72000000U,
                 .next_token = 0x72000100U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x05U,
                 .type = 0x1AU,
                 .text = {}},
                {.token = 0x72000100U,
                 .next_token = 0x72000200U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x05U,
                 .type = 0x1BU,
                 .profile_id = 7U,
                 .value_flags = 0xC800U,
                 .text = "first"},
                {.token = 0x72000200U,
                 .category_flags = 0x10U,
                 .mode_flags = 0x01U,
                 .type = 0x1EU,
                 .profile_id = 8U,
                 .value_flags = 0x8005U,
                 .text = "second"},
            },
        .resource_owner_token = 0U,
        .resource_head_token = 0U,
        .resources = {},
        .selected_resource_token = 0U,
        .primary_required = 0U,
        .secondary_required = 0U,
    };

    {
        QueryPort port;
        port.index = 2U;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            port,
            {.category_selector = 0U,
             .type_selector = 0U,
             .occurrence = 1U,
             .entry_output_word = 0x7777U,
             .return_table = table}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                actor.current_list_index == list.owner_token &&
                result.nodes_visited == 2U && result.matches == 1U &&
                result.output_text == "first" && result.output_word == 1000U &&
                result.return_eax == 30U && port.last_profile == 7U,
            "list query commits the index then applies ordered category, type, and value overrides"
        );
    }

    {
        QueryPort port;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            port,
            {.category_selector = 0U,
             .type_selector = 0U,
             .occurrence = 0U,
             .entry_output_word = 3U,
             .return_table = table}
        );
        test.expect_true(
            result.return_eax == 0xFFFFU && result.output_word == 0xFFFFU &&
                result.matches == 2U && port.read_calls == 0U,
            "zero occurrence never matches because the counter increments before comparison"
        );
    }

    {
        LegacyBattleActorListQueryState type31{
            .owner_token = list.owner_token,
            .head_token = 0x73000000U,
            .nodes =
                {{.token = 0x73000000U,
                  .category_flags = 0x0CU,
                  .mode_flags = 0x04U,
                  .type = 0x1FU,
                  .profile_id = 9U,
                  .value_flags = 0xFFFFU,
                  .text = "profile"}},
            .resource_owner_token = 0U,
            .resource_head_token = 0U,
            .resources = {},
            .selected_resource_token = 0U,
            .primary_required = 0U,
            .secondary_required = 0U,
        };
        QueryPort port;
        port.index = 1U;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &type31,
            0x005029D0U,
            port,
            {.category_selector = 1U,
             .type_selector = 1U,
             .occurrence = 1U,
             .entry_output_word = 0xAAAAU,
             .return_table = table}
        );
        test.expect_true(
            result.output_word == 1U && result.return_eax == 20U &&
                result.profile_load_calls == 1U,
            "type thirty-one forces output one after all value flag overrides"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        const auto result = count_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            0x00610000U,
            {.category_selector = 0U,
             .type_selector = 0U,
             .entry_count = 0xFFU,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.nodes_visited == 3U && result.matches == 2U &&
                result.count == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x00610000U &&
                result.return_edx == 0xBBBBBBBBU,
            "list count preserves the caller byte then wraps once per matching node"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        const auto result = count_legacy_battle_actor_list(
            &actor,
            &list,
            0x005029D0U,
            0x00610000U,
            {.category_selector = 0U, .type_selector = 7U, .entry_count = 9U}
        );
        test.expect_true(
            result.count == 9U && result.matches == 0U &&
                result.nodes_visited == 3U,
            "every non-twenty-eight type selector still tests only type thirty-one"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[1U] = 5U << 16U;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        item_effect.mode_flags = 0x80U;
        list.selected_resource_token = 0U;
        list.primary_required = 7U;
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, nullptr, 0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                (configuration.actor_record[1U] >> 16U) == 0U &&
                list.primary_required == 0U && result.capacity_writes == 2U &&
                result.refresh_calls == 1U,
            "actor list action wraps subtraction then clamps a negative signed capacity"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        LegacyBattleGroupAWorkspaceState workspace;
        item_effect.mode_flags = 0x80U;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.primary_required = 9U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 4U,
             .secondary_quantity = 2,
             .name = {}},
        };
        const auto result = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, &workspace, 0x005029D0U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.refresh_calls == 1U &&
                list.resource_head_token == 0x76000010U &&
                list.selected_resource_token == 0U &&
                list.primary_required == 0U &&
                list.resources[1U].secondary_quantity == 1 &&
                workspace.tail_words[2U] == 4U,
            "selected resource path consumes the owned node then clears selection and required capacity before refreshing"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 0xA5A50003U;
        list.secondary_required = 5U;
        const auto result = refresh_legacy_battle_actor_list_action(
            &list, &configuration, 0x005029D0U, 10U, 0xBEEF0000U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                configuration.actor_record[2U] == 0xA5A50000U &&
                list.secondary_required == 0U && result.capacity_writes == 2U &&
                result.secondary_required_clears == 1U &&
                result.return_eax == configuration.actor_record_token &&
                result.return_edx == 0xBEEF0005U,
            "secondary refresh preserves high words, wraps subtraction, clamps signed negative and returns stale register halves"
        );
    }

    {
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        list.secondary_required = 0U;
        const auto result = execute_legacy_battle_actor_list_action(
            &list,
            &item_effect,
            nullptr,
            nullptr,
            0x005029D0U,
            {.entry_eax = 10U, .entry_edx = 12U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.refresh_calls == 1U && result.return_eax == 10U &&
                result.return_edx == 0U,
            "mode bit clear skips primary state and still runs typed secondary refresh"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[1U] = 5U << 16U;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xFFFFU;
        LegacyBattleGroupAActionExecutionState action;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 2U,
             .primary_quantity = 1U,
             .tertiary_quantity = 4,
             .name = {},
             .category_mask = 0x10U,
             .derived_words_40 = {11U, 12U, 13U},
             .flags_49 = 0x02U,
             .profile_id_4a = 7U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x8003U,
             .alternate_profile_id_54 = 9U},
        };
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            &configuration,
            &final_state,
            &item,
            &workspace,
            &action,
            0x005029D0U,
            port,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.output_mode == 1U &&
                port.profile_ids[0U] == 9U && result.diagnostic_calls == 1U &&
                list.selected_resource_token == 0x76000010U &&
                list.primary_required == 3U && item.mode_flags == 0x40U &&
                item.derived_words[1U] == 11U &&
                final_state.profile_copy_latch == 1U &&
                list.resources[1U].primary_quantity == 2U,
            "resource selection loads the alternate profile, applies gates, diagnoses zero runtime word and increments quantity"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        LegacyBattleGroupAActionExecutionState action;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .tertiary_quantity = 1,
             .name = {},
             .category_mask = 0x2000U,
             .derived_word_30 = 0x1234U,
             .profile_id_4a = 7U,
             .capacity_gate_flags = 0x2000U},
        };
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            nullptr,
            &final_state,
            &item,
            &workspace,
            &action,
            0x005029D0U,
            port,
            port,
            {.category_selector = 4U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 1U && port.profile_ids[0U] == 7U &&
                item.derived_words[0U] == 0x1234U &&
                list.resources[1U].primary_quantity == 0U,
            "category four bit-thirteen selection takes the early profile path without quantity mutation"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xFFFFU;
        SelectionPort port;
        const auto result = select_legacy_battle_actor_resource(
            nullptr,
            nullptr,
            &final_state,
            nullptr,
            &workspace,
            nullptr,
            0x005029D0U,
            port,
            port,
            {.occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 0U && final_state.profile_buffer[0U] == 0U &&
                final_state.pre_effect_words[0U] == 0U &&
                workspace.tail_words[2U] == 0U,
            "zero occurrence preserves the mandatory clearing prefix before failure"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .name = "ordinary"},
            {.token = 0x76000020U,
             .secondary_quantity = -1,
             .tertiary_quantity = 2,
             .name = "flagged",
             .capacity_gate_flags = 0x2000U},
        };

        std::array<u8, 20> name{};
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 1U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && list.resource_head_token == 0x76000020U &&
                std::string(name.begin(), name.begin() + 7U) == "flagged" &&
                name[7U] == 0U && quantity == 1U,
            "flagged resource query counts bit thirteen and wraps the secondary plus tertiary word sum"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .secondary_quantity = 3,
             .tertiary_quantity = 4,
             .name = "stale-zero"},
        };

        std::array<u8, 20> name{};
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 0U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && list.resource_head_token == 0x76000010U &&
                std::string(name.begin(), name.begin() + 10U) == "stale-zero" &&
                name[10U] == 0U && quantity == 7U,
            "flagged resource occurrence zero preserves the stale success on the first unflagged node"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = "twenty-byte-name-xxxx",
             .capacity_gate_flags = 0x2000U},
        };

        std::array<u8, 20> name{};
        name.fill(0xA5U);
        const auto original_name = name;
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 1U, name, quantity
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                !result.found && list.resource_head_token == 0x76000010U &&
                name == original_name && quantity == 0xBEEFU,
            "flagged resource query stops at the original caller buffer boundary instead of truncating lstrcpy"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = "only",
             .capacity_gate_flags = 0x2000U},
        };

        std::array<u8, 20> name{};
        name.fill(0xA5U);
        const auto original_name = name;
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_flagged_resource(
            &list, 2U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                !result.found && list.resource_head_token == 0U &&
                name == original_name && quantity == 0xBEEFU,
            "flagged resource query destructively reaches null when the requested occurrence is absent"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0xDEADBEEFU;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .next_token = 0x77000020U,
             .resource_id = 0x222U,
             .name = "ordinary"},
            {.token = 0x77000020U,
             .resource_id = 0x0300U,
             .secondary_quantity = -1,
             .tertiary_quantity = 2,
             .name = "fixed",
             .mode_flags = 0x01U},
        };

        std::array<u8, 20> name;
        name.fill(0xA5U);
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 0U, 1U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && quantity == 1U && name[5U] == 0U &&
                name[6U] == 0xA5U &&
                std::string(reinterpret_cast<const char*>(name.data())) ==
                    "fixed" &&
                list.resource_head_token == 0x77000020U &&
                list.selected_resource_token == 0xDEADBEEFU,
            "mode resource query finds the fixed identifier without rewriting the selected token and wraps both quantity words"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .resource_id = 0x0300U,
             .name = "twenty-byte-name-xxx",
             .mode_flags = 0x01U},
        };

        std::array<u8, 20> name;
        name.fill(0xA5U);
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 0U, 1U, name, quantity
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                !result.found && list.resource_head_token == 0x77000010U &&
                quantity == 0xBEEFU && name.front() == 0xA5U,
            "mode resource query stops at the caller string boundary before publishing wrapped quantities"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .next_token = 0x77000020U,
             .resource_id = 0x0300U,
             .name = "excluded",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x77000020U,
             .next_token = 0x77000030U,
             .resource_id = 0x222U,
             .name = "first",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x77000030U,
             .resource_id = 0x333U,
             .secondary_quantity = 3,
             .tertiary_quantity = 4,
             .name = "second",
             .category_mask = 0x08000000U,
             .mode_flags = 0x04U},
        };

        std::array<u8, 20> name;
        name.fill(0xA5U);
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 1U, 2U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && quantity == 7U && name[6U] == 0U &&
                name[7U] == 0xA5U &&
                std::string(reinterpret_cast<const char*>(name.data())) ==
                    "second" &&
                list.resource_head_token == 0x77000030U &&
                list.selected_resource_token == 0x77000030U,
            "mode resource query counts bit twenty-seven nodes while excluding the fixed identifier"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.selected_resource_token = 0U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .secondary_quantity = 9,
             .tertiary_quantity = 8,
             .name = "stale",
             .mode_flags = 0U},
        };

        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 1U, 0U, {}, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && quantity == 0xBEEFU &&
                list.resource_head_token == 0x77000010U &&
                list.selected_resource_token == 0x77000010U,
            "mode resource occurrence zero selects the first unqualified node but preserves stale outputs when mode bits are clear"
        );
    }

    {
        list.next_resource_head_token = 0x77000000U;
        list.resources = {
            {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
            {.token = 0x77000010U,
             .resource_id = 0x222U,
             .name = "only",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
        };

        std::array<u8, 20> name;
        name.fill(0xA5U);
        u16 quantity = 0xBEEFU;
        const auto result = query_legacy_battle_actor_mode_resource(
            &list, 1U, 2U, name, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                !result.found && list.resource_head_token == 0U &&
                quantity == 0xBEEFU && name.front() == 0xA5U,
            "mode resource query destructively reaches null when the requested filtered occurrence is absent"
        );
    }

    {
        list.next_resource_head_token = 0x79000000U;
        list.resources = {
            {.token = 0x79000000U, .next_token = 0x79000010U, .name = {}},
            {.token = 0x79000010U,
             .next_token = 0x79000020U,
             .resource_id = 0x0300U,
             .name = "fixed",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x79000020U,
             .next_token = 0x79000030U,
             .resource_id = 0x0222U,
             .name = "unflagged",
             .mode_flags = 0x01U},
            {.token = 0x79000030U,
             .next_token = 0x79000040U,
             .resource_id = 0x0223U,
             .name = "closed",
             .category_mask = 0x08000000U,
             .mode_flags = 0U},
            {.token = 0x79000040U,
             .next_token = 0x79000050U,
             .resource_id = 0x0224U,
             .secondary_quantity = 32767,
             .tertiary_quantity = 32767,
             .name = "large",
             .category_mask = 0x08000000U,
             .mode_flags = 0x01U},
            {.token = 0x79000050U,
             .resource_id = 0x0225U,
             .secondary_quantity = 1,
             .tertiary_quantity = 2,
             .name = "small",
             .category_mask = 0x08000000U,
             .mode_flags = 0x04U},
        };
        const auto result = count_legacy_battle_actor_mode_resources(
            &list,
            0x005029D0U,
            {.output_token = 0x0012FFD4U,
             .entry_eax = 0xAAAAAAAAU,
             .entry_edx = 0xBBBBBBBBU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.nodes_visited == 6U && result.matches == 2U &&
                result.count == 1U && result.return_eax == 0U &&
                result.return_ecx == 0x0012FFD4U &&
                result.return_edx == 0x0300U && list.resource_head_token == 0U,
            "mode resource count filters bit twenty-seven, fixed identifiers and output mode bits before wrapping all quantity words"
        );
    }

    {
        list.next_resource_head_token = 0x79000000U;
        list.resources = {
            {.token = 0x79000000U, .next_token = 0x79000010U, .name = {}},
            {.token = 0x79000010U,
             .resource_id = 0x0222U,
             .secondary_quantity = 7,
             .tertiary_quantity = 8,
             .name = "excluded"},
        };
        const auto result = count_legacy_battle_actor_mode_resources(
            &list, 0x005029D0U, {.output_token = 0x0012FFD4U}
        );
        test.expect_true(
            result.return_eax == 0U && result.matches == 0U &&
                result.count == 0U,
            "mode resource count zeroes its output before traversing an excluded chain"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x33U,
             .primary_quantity = 4U,
             .secondary_quantity = 2,
             .name = {},
             .gate_word_48 = 0x0402U},
        };

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                list.resources.size() == 2U &&
                list.resources[1U].gate_word_48 == 0x0401U &&
                list.resources[1U].primary_quantity == 4U &&
                list.resources[1U].secondary_quantity == 2 &&
                list.selected_resource_token == 0x76000010U &&
                list.resource_head_token == 0x76000010U &&
                workspace.tail_words[2U] == 0x33U &&
                result.output_word == 0x33U,
            "resource release decrements the low-byte gate first and preserves quantities while the gate remains nonzero"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .resource_id = 0x44U,
             .primary_quantity = 2U,
             .secondary_quantity = 2,
             .name = {},
             .capacity_gate_flags = 0x2000U},
        };

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                list.resources.size() == 2U &&
                list.resources[1U].primary_quantity == 1U &&
                list.resources[1U].secondary_quantity == 1 &&
                list.selected_resource_token == 0U &&
                list.resource_head_token == 0x76000010U &&
                workspace.tail_words[2U] == 0x44U && result.output_word == 0U,
            "resource release decrements secondary before tertiary, also decrements primary, clears selection and applies bit-thirteen return suppression"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000020U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .resource_id = 1U,
             .name = {}},
            {.token = 0x76000020U,
             .next_token = 0x76000030U,
             .resource_id = 2U,
             .primary_quantity = 1U,
             .secondary_quantity = 1,
             .name = {}},
            {.token = 0x76000030U, .resource_id = 3U, .name = {}},
        };

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                list.resources.size() == 3U &&
                list.resources[1U].next_token == 0x76000030U &&
                list.selected_resource_token == 0U &&
                list.resource_head_token == 0x76000010U &&
                workspace.tail_words[2U] == 2U && result.output_word == 2U,
            "resource release removes a depleted middle node and replays the destructive traversal count to patch its predecessor"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0xABCD0000U;
        list.selected_resource_token = 0U;
        list.resources = {{.token = 0xABCD0000U, .name = {}}};
        workspace.tail_words[2U] = 0xBEEFU;
        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.output_word == 0U &&
                list.resource_head_token == 0xABCD0000U &&
                list.resources.size() == 1U &&
                workspace.tail_words[2U] == 0xBEEFU,
            "missing selected resource returns a normal zero after publishing the head and preserving the workspace"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .next_token = 0x76000020U,
             .primary_quantity = 1U,
             .tertiary_quantity = 2,
             .name = {},
             .category_mask = 0x2000U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x2000U},
            {.token = 0x76000020U, .name = {}, .capacity_gate_flags = 0x2000U},
        };
        const auto result = count_legacy_battle_actor_resource_list(
            &list,
            0x005029D0U,
            {.category_selector = 4U, .initial_count = 0xFFFEU}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.count == 1U && result.nodes_visited == 2U &&
                result.positive_matches == 1U && result.extra_matches == 2U &&
                list.resource_head_token == 0U,
            "resource count preserves the initial word, wraps increments, and category four counts its extra bit independently"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 2U;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .primary_quantity = 3U,
             .secondary_quantity = -1,
             .tertiary_quantity = 10,
             .name = "resource",
             .category_mask = 0x10U,
             .mode_flags = 0x01U,
             .capacity_gate_flags = 0x4003U},
        };
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            &configuration,
            0x005029D0U,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.nodes_visited == 1U &&
                result.matches == 1U && result.copied_name == "resource" &&
                result.output_quantity == 6U &&
                result.output_flags == 0xC000U &&
                list.resource_head_token == 0x76000010U,
            "resource query destructively advances the chain, counts a positive match and publishes quantity plus both flag bits"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            {.token = 0x76000010U,
             .name = {},
             .category_mask = 0U,
             .mode_flags = 0U},
        };
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            nullptr,
            0x005029D0U,
            {.category_selector = 0U, .occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 1U && result.matches == 0U &&
                result.output_flags == 0U && result.copied_name.empty(),
            "occurrence zero takes the stale success branch after the first nonmatch and rechecks filters without copying"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_40 = 11U;
        list.nodes[1U].value_42 = 12U;
        list.nodes[1U].value_44 = 13U;
        list.nodes[1U].copy_flags = 0U;
        list.nodes[1U].output_value = 77U;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        QueryPort port;
        const auto result = apply_legacy_battle_actor_list(
            &actor,
            &list,
            &final_state,
            &item_effect,
            0x005029D0U,
            port,
            {.category_selector = 0U, .type_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.return_eax == 1U && result.output_value == 77U &&
                result.profile_buffer_dwords_zeroed == 10U &&
                result.pre_effect_dwords_zeroed == 4U &&
                (item_effect.mode_flags & 0x80U) != 0U &&
                item_effect.derived_words[1U] == 11U &&
                item_effect.derived_words[2U] == 12U &&
                item_effect.derived_words[3U] == 13U &&
                reinterpret_cast<const char*>(
                    final_state.pre_effect_words.data()
                )[0] == 'f',
            "list apply clears both buffers, loads the profile, copies text and publishes derived words"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer[0U] = 0xAABBCCDDU;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        item_effect.mode_flags = 0x40U;
        QueryPort port;
        const auto result = apply_legacy_battle_actor_list(
            &actor,
            &list,
            &final_state,
            &item_effect,
            0x005029D0U,
            port,
            {.category_selector = 0U, .type_selector = 0U, .occurrence = 0U}
        );
        test.expect_true(
            result.return_eax == 0xFFFFU && result.index_commit_calls == 0U &&
                final_state.profile_buffer[0U] == 0xAABBCCDDU &&
                item_effect.mode_flags == 0x40U,
            "zero occurrence returns minus one before mode flags, clear, and index commit"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x800AU;
        list.primary_required = 0U;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U,
             .occurrence = 1U,
             .actor_primary_capacity = 10}
        );
        test.expect_true(
            result.return_eax == 1U && result.index_commit_calls == 2U &&
                list.primary_required == 10U &&
                list.selected_resource_token == 0U &&
                result.message_calls == 0U,
            "state processing publishes the primary threshold then succeeds on equal signed capacity"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x0805U;
        list.resource_owner_token = 0x75000000U;
        list.resource_head_token = 0x76000010U;
        list.next_resource_head_token = 0x76000000U;
        list.resources = {
            {.token = 0x76000000U,
             .resource_id = 5U,
             .primary_quantity = 1,
             .secondary_quantity = 0,
             .name = {}}
        };
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 1U &&
                list.resource_head_token == 0x76000000U &&
                result.resource_nodes_visited == 1U &&
                list.selected_resource_token == 0x76000000U &&
                result.index_commit_calls == 1U,
            "bit eleven rebuilds and selects a resource node with either positive quantity"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        list.nodes[1U].value_flags = 0x0806U;
        list.resource_head_token = 0x76000000U;
        list.next_resource_head_token = 0U;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.category_selector = 0U, .occurrence = 1U}
        );
        test.expect_true(
            result.return_eax == 0U && result.message_calls == 1U &&
                result.sample_calls == 1U &&
                result.message_token == 0x74000000U &&
                port.last_text == 0x004A7CD0U,
            "missing resource publishes one guarded message and sample"
        );
    }

    {
        actor.next_list_index = list.owner_token;
        StatePort port;
        const auto result = process_legacy_battle_actor_list_state(
            &actor,
            &list,
            0x005029D0U,
            0U,
            port,
            {.occurrence = 0U, .entry_eax = 0xFFFFFFFFU}
        );
        test.expect_true(
            result.return_eax == 0U && result.index_commit_calls == 0U &&
                result.nodes_visited == 0U,
            "zero occurrence returns before list-index commit"
        );
    }

    {
        QueryPort port;
        LegacyBattleActorListQueryState bad = list;
        bad.head_token = 0xDEADBEEFU;
        const auto result = query_legacy_battle_actor_list(
            &actor,
            &bad,
            0x005029D0U,
            port,
            {.occurrence = 1U, .return_table = table}
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                result.index_commit_calls == 1U && result.nodes_visited == 0U,
            "missing node stops after the shared list-index commit"
        );
    }
}
