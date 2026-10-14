#include "legacy_battle_party_list_fixture.hpp"
#include "legacy_battle_inventory_fixture.hpp"
#include "legacy_battle_mon_database_fixture.hpp"
#include "legacy_battle_samples_fixture.hpp"
#include "openswd3/battle/legacy_battle_actor_list_query.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <functional>
#include <string>
#include <utility>

#include "test.hpp"

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

class InventoryReleaseMemory final : public std::pmr::memory_resource {
public:
    std::function<void()> on_release;

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(
        void* block, std::size_t bytes, std::size_t alignment
    ) override {
        if (on_release) {
            on_release();
        }

        std::pmr::new_delete_resource()->deallocate(block, bytes, alignment);
    }

    bool do_is_equal(
        const std::pmr::memory_resource& other
    ) const noexcept override {
        return this == &other;
    }
};

class SelectionPort final
    : public openswd3::battle::LegacyBattleActorResourceSelectionPort,
      public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] u32
    open_mon_file(const std::filesystem::path& path) override {
        if (before_open) {
            before_open();
        }

        return LegacyBattleMonDatabaseFixture::open_mon_file(path);
    }

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
        if (after_diagnostic) {
            after_diagnostic();
        }
    }

    std::function<void()> before_open;
    std::function<void()> after_diagnostic;
    std::vector<openswd3::compat::u16> profile_ids;
    std::vector<openswd3::compat::u16> diagnostics;
};

class StateMessagePort final
    : public openswd3::battle::LegacyBattleTextMessagePort {
public:
    openswd3::battle::LegacyBattleTextMessageCallReply invoke_text_message(
        const openswd3::battle::LegacyBattleTextMessageCallRequest& request
    ) override {
        if (request.call ==
            openswd3::battle::LegacyBattleTextMessageCall::allocate) {
            return {.eax = fail_allocation ? 0U : 0x74000000U};
        }

        if (on_measure) {
            on_measure();
        }

        return {.eax = 4U, .text_access_failed = fail_measure};
    }

    std::function<void()> on_measure;
    bool fail_allocation{};
    bool fail_measure{};
};

struct ListProfileFixture {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    ListProfileFixture() {
        actor.next_list_index = 0x70000000U;
        openswd3::test::bind_party_list(inventories, list, 0x70000000U);
        openswd3::test::party_root(list).legacy_next_token = 0x70000010U;
        openswd3::test::assign_party_nodes(
            list,
            {{
                .token = 0x70000010U,
                .category_flags = 0xFFFFFFFFU,
                .mode_flags = 1U,
                .type = 0x1BU,
                .profile_id = 17U,
                .value_40 = 11U,
                .value_42 = 12U,
                .value_44 = 13U,
                .value_flags = 31U,
                .mode_value = 41U,
                .output_value = 0xFFFFU,
                .text = "profile",
            }}
        );
        final_state.profile_buffer.fill(0xCCCCCCCCU);
        final_state.pre_effect_words.fill(0xDDDDDDDDU);
    }

    openswd3::battle::LegacyBattleActorListApplyResult apply(
        const u32 category = 0U, const u32 type = 0U, const u32 occurrence = 1U
    ) {
        return openswd3::battle::apply_legacy_battle_actor_list(
            &actor,
            &list,
            &final_state,
            &item_effect,
            port,
            category,
            type,
            occurrence,
            output
        );
    }

    openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
    openswd3::battle::LegacyBattleActorListQueryState list;
    openswd3::battle::LegacyBattleGroupAFinalProcessingState final_state;
    openswd3::battle::LegacyBattleGroupAItemEffectApplicationState item_effect;
    SelectionPort port;
    u32 output{0xABCD1234U};
};

void test_list_profile_application(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (const u32 selector : {0U, 1U, 0x1CU, 0x1FU, 0xFFFFFFFFU}) {
        for (const u16 type :
             {0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU, 0x20U}) {
            for (const u8 mode : {0U, 1U, 2U, 4U, 5U}) {
                ListProfileFixture fixture;
                openswd3::test::inventory_word(
                    openswd3::test::party_node(fixture.list, 0U), 0x5EU
                ) = type;
                openswd3::test::party_node(fixture.list, 0U)
                    .definition_snapshot[0x46U - 0x0CU] = mode;
                fixture.item_effect.mode_flags = 0x40U;
                const bool range = selector == 0U || selector == 0x1CU;
                const bool found = (mode == 1U || mode == 4U || mode == 5U) &&
                    (range ? type >= 0x1BU && type <= 0x1EU : type == 0x1FU);
                const auto result = fixture.apply(0U, selector);
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.selection_mode.has_value() == found &&
                        fixture.actor.current_list_index ==
                            (found ? 0x70000010U : 0U) &&
                        fixture.output == (found ? 0xFFFFU : 0xABCD1234U) &&
                        fixture.item_effect.mode_flags ==
                            (selector == 0U       ? 0xC0U
                                 : selector == 1U ? 0x42U
                                                  : 0x40U),
                    "profile scan publishes its cursor and filters mapped and raw types with both mode bits"
                );
            }
        }
    }

    struct CategoryCase {
        u32 category;
        u32 node_flags;
        std::optional<u16> expected;
    };

    const std::array<CategoryCase, 8> categories{{
        {0U, 0x10U, 1U},
        {0U, 8U, std::nullopt},
        {1U, 4U, 0U},
        {1U, 8U, 1U},
        {2U, 1U, 0U},
        {2U, 0x1000U, 0U},
        {0x18U, 0x10U, 1U},
        {0x10002U, 2U, 0U},
    }};
    for (const auto& category : categories) {
        ListProfileFixture fixture;
        openswd3::test::inventory_category(
            openswd3::test::party_node(fixture.list, 0U)
        ) = category.node_flags;
        const auto result = fixture.apply(category.category);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selection_mode == category.expected,
            "profile selection maps the three categories and retains raw masks and asymmetric return bits"
        );
    }

    for (const u16 flags : {0U, 1U, 0x100U, 0x200U, 0x201U, 0xFFFFU}) {
        ListProfileFixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x48U
        ) = flags;
        fixture.final_state.profile_copy_latch = 7U;
        fixture.item_effect.derived_words[1U] = 21U;
        fixture.item_effect.derived_words[2U] = 22U;
        fixture.item_effect.derived_words[3U] = 23U;
        const auto result = fixture.apply();
        const bool copy = flags == 0U || (flags & 0x200U) != 0U;
        test.expect_true(
            result.selection_mode == 1U &&
                fixture.final_state.profile_copy_latch ==
                    ((flags & 0x200U) != 0U ? 1U : 0U) &&
                fixture.item_effect.derived_words[1U] == (copy ? 11U : 21U) &&
                fixture.item_effect.derived_words[2U] == (copy ? 12U : 22U) &&
                fixture.item_effect.derived_words[3U] == (copy ? 13U : 23U),
            "profile copy clears the latch and distinguishes the complete zero WORD from bit nine"
        );
    }

    {
        ListProfileFixture fixture;
        openswd3::test::inventory_category(
            openswd3::test::party_node(fixture.list, 0U)
        ) = 4U;
        bool prefix_seen = false;
        fixture.port.before_open = [&] {
            prefix_seen = fixture.output == 0xFFFFU &&
                fixture.actor.current_list_index == 0x70000010U &&
                std::ranges::all_of(
                              fixture.final_state.profile_buffer,
                              [](const u32 value) { return value == 0U; }
                ) &&
                fixture.final_state.pre_effect_words[0U] == 0xDDDDDDDDU;
            openswd3::test::assign_party_nodes(
                fixture.list,
                {
                    {.token = 0x70000020U, .text = "wrong"},
                    {.token = 0x70000030U,
                     .category_flags = 8U,
                     .value_40 = 51U,
                     .value_42 = 52U,
                     .value_44 = 53U,
                     .copy_flags = 0x200U,
                     .value_flags = 61U,
                     .mode_value = 62U,
                     .output_value = 63U,
                     .text = "changed"},
                }
            );

            fixture.actor.current_list_index = 0x70000030U;
            fixture.item_effect.mode_flags = 2U;
            fixture.output = 0xAA55AA55U;
        };

        const auto result = fixture.apply(1U);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                prefix_seen && result.selection_mode == 1U &&
                fixture.output == 0xAA55AA55U &&
                fixture.port.profile_ids == std::vector<u16>{17U} &&
                std::string(
                    reinterpret_cast<const char*>(
                        fixture.final_state.pre_effect_words.data()
                    )
                ) == "changed" &&
                fixture.final_state.applied_mode_value == 62U &&
                fixture.final_state.applied_output_value == 63U &&
                fixture.final_state.replacement_action_kind == 61U &&
                fixture.item_effect.derived_words[1U] == 51U &&
                fixture.final_state.profile_copy_latch == 1U,
            "profile loading observes the published output and zeroed buffer before reallocation and live cursor, mode and return-bit rereads"
        );
    }

    {
        ListProfileFixture fixture;
        fixture.port.allocation_succeeds = false;
        const auto result = fixture.apply();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::profile_load_typed_stop &&
                !result.selection_mode && fixture.output == 0xFFFFU &&
                fixture.actor.current_list_index == 0x70000010U &&
                fixture.final_state.pre_effect_words[0U] == 0xDDDDDDDDU &&
                fixture.item_effect.mode_flags == 0x80U,
            "profile allocation failure retains published output and mode before name clearing"
        );
    }

    for (const std::string name :
         {std::string{},
          std::string(15U, 'a'),
          std::string(16U, 'b'),
          std::string("short\0ignored suffix", 20U)}) {
        ListProfileFixture fixture;
        openswd3::test::set_inventory_name(
            openswd3::test::party_node(fixture.list, 0U), name
        );
        const auto result = fixture.apply();
        const bool too_long = name.size() == 16U;
        test.expect_true(
            result.status ==
                    (too_long ? LegacyBattleActorListQueryStatus::
                                    list_text_typed_stop
                              : LegacyBattleActorListQueryStatus::completed) &&
                result.selection_mode.has_value() != too_long &&
                (too_long ||
                 std::string(
                     reinterpret_cast<const char*>(
                         fixture.final_state.pre_effect_words.data()
                     )
                 ) == name.c_str()),
            "profile names stop at the first NUL and retain the existing destination overflow prefix"
        );
    }

    {
        SelectionPort port;
        u32 output = 0xABCDEF01U;
        const auto result = apply_legacy_battle_actor_list(
            nullptr, nullptr, nullptr, nullptr, port, 0U, 0U, 0U, output
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                !result.selection_mode && output == 0xABCDEF01U &&
                port.open_calls == 0U,
            "zero profile occurrence accesses neither actor owners nor the output"
        );
    }
}

void test_list_profile_cursor(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (const bool broken_tail : {false, true}) {
        for (const u32 occurrence : {2U, 3U}) {
            ListProfileFixture fixture;
            openswd3::test::party_node(fixture.list, 0U).legacy_next_token =
                0x70000020U;
            openswd3::test::append_party_item(
                fixture.list,
                {
                    .token = 0x70000020U,
                    .next_token = 0x70000030U,
                    .text = "skipped",
                }
            );
            openswd3::test::append_party_item(
                fixture.list,
                {
                    .token = 0x70000030U,
                    .next_token = broken_tail ? 0xDEADBEEFU : 0U,
                    .category_flags = 0x10U,
                    .mode_flags = 4U,
                    .type = 0x1EU,
                    .profile_id = 19U,
                    .output_value = 9U,
                    .text = "last",
                }
            );
            const auto result = fixture.apply(0U, 0U, occurrence);
            const bool selected = occurrence == 2U;
            test.expect_true(
                result.status ==
                        (!selected && broken_tail
                             ? LegacyBattleActorListQueryStatus::
                                   list_node_typed_stop
                             : LegacyBattleActorListQueryStatus::completed) &&
                    result.selection_mode.has_value() == selected &&
                    fixture.output == (selected ? 9U : 0xABCD1234U) &&
                    fixture.actor.current_list_index ==
                        (selected          ? 0x70000030U
                             : broken_tail ? 0xDEADBEEFU
                                           : 0U) &&
                    fixture.port.profile_ids ==
                        (selected ? std::vector<u16>{19U} : std::vector<u16>{}),
                "profile occurrence counts matching nodes and publishes exhaustion or a missing node before returning"
            );
        }
    }

    {
        ListProfileFixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x5CU
        ) = 0x30U;
        openswd3::test::append_party_item(
            fixture.list,
            {
                .token = 0x30U,
                .profile_id = 23U,
                .text = "aliased",
            }
        );
        const auto result = apply_legacy_battle_actor_list(
            &fixture.actor,
            &fixture.list,
            &fixture.final_state,
            &fixture.item_effect,
            fixture.port,
            0U,
            0U,
            1U,
            fixture.actor.current_list_index
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selection_mode == 1U &&
                fixture.actor.current_list_index == 0x30U &&
                fixture.port.profile_ids == std::vector<u16>{23U} &&
                std::string(
                    reinterpret_cast<const char*>(
                        fixture.final_state.pre_effect_words.data()
                    )
                ) == "aliased",
            "an output alias changes the live cursor before the profile ID and text are read"
        );
    }
}

struct ListStateFixture {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    ListStateFixture() {
        actor.next_list_index = 0x70000000U;
        actor.current_list_index = 0x12345678U;
        openswd3::test::bind_party_list(inventories, list, 0x70000000U);
        openswd3::test::party_root(list).legacy_next_token = 0x70000010U;
        openswd3::test::assign_party_nodes(
            list,
            {{.token = 0x70000010U,
              .category_flags = 0x10U,
              .mode_flags = 1U,
              .type = 0x1BU,
              .value_flags = 0x800AU}}
        );

        list.selected_resource_token = 0xDEADBEEFU;
        configuration.actor_record_token = 0x72000000U;
        configuration.actor_record[1U] = 10U << 16U;
        configuration.actor_record[2U] = 10U;
    }

    openswd3::battle::LegacyBattleActorListStateResult
    run(const u32 category = 0U, const u32 occurrence = 1U) {
        return openswd3::battle::process_legacy_battle_actor_list_state(
            &actor,
            &list,
            &configuration,
            messages,
            message_head,
            port,
            audio.samples,
            mix_level,
            category,
            occurrence
        );
    }

    openswd3::battle::LegacyBattleGroupAActionExecutionState actor;
    openswd3::battle::LegacyBattleActorListQueryState list;
    openswd3::battle::LegacyBattleGroupAConfigurationState configuration;
    openswd3::battle::LegacyBattleTextMessageState messages;
    u32 message_head{};
    openswd3::compat::i32 mix_level{100};
    StateMessagePort port;
    openswd3::test::LegacyBattleSamplesFixture audio;
};

void test_list_state_behavior(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    using openswd3::compat::i16;
    for (const i16 capacity :
         {i16{-32768}, i16{-1}, i16{0}, i16{9}, i16{10}, i16{32767}}) {
        for (const u16 flags : {u16{0x800AU}, u16{0x400AU}, u16{0xC00AU}}) {
            ListStateFixture fixture;
            openswd3::test::inventory_word(
                openswd3::test::party_node(fixture.list, 0U), 0x4CU
            ) = flags;
            fixture.configuration.actor_record[1U] =
                static_cast<u32>(std::bit_cast<u16>(capacity)) << 16U;
            fixture.configuration.actor_record[2U] =
                std::bit_cast<u16>(capacity);
            const auto result = fixture.run();
            test.expect_true(
                result.status == LegacyBattleActorListQueryStatus::completed &&
                    result.available == (capacity >= 10) &&
                    fixture.actor.current_list_index ==
                        fixture.actor.next_list_index &&
                    fixture.list.selected_resource_token == 0U &&
                    fixture.messages.allocations.size() ==
                        (capacity >= (flags == 0xC00AU ? 0x400A : 10) ? 0U
                                                                      : 1U),
                "state capacity compares signed live words, restores current and suppresses repeated warnings"
            );
        }
    }

    for (const i16 secondary :
         {i16{-32768}, i16{-1}, i16{0}, i16{1}, i16{32767}}) {
        for (const i16 tertiary :
             {i16{-32768}, i16{-1}, i16{0}, i16{1}, i16{32767}}) {
            for (const u16 primary : {u16{0U}, u16{0xFFFFU}}) {
                ListStateFixture fixture;
                openswd3::test::inventory_word(
                    openswd3::test::party_node(fixture.list, 0U), 0x4CU
                ) = 0x0805U;
                fixture.list.next_resource_head_token = 0x75000000U;
                inventories.assign(
                    fixture.list,
                    {{.token = 0x75000000U,
                      .next_token = 0x75000010U,
                      .name = {}},
                     {.token = 0x75000010U,
                      .resource_id = 5U,
                      .primary_quantity = primary,
                      .secondary_quantity = secondary,
                      .tertiary_quantity = tertiary,
                      .name = {}}}
                );

                const auto result = fixture.run();
                const bool available = secondary > 0 || tertiary > 0;
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.available == available &&
                        fixture.actor.current_list_index == 0x70000010U &&
                        fixture.list.resource_head_token == 0x75000010U &&
                        fixture.list.primary_required == 5U &&
                        fixture.list.selected_resource_token ==
                            (available ? 0x75000010U : 0xDEADBEEFU) &&
                        fixture.messages.allocations.size() ==
                            (available ? 0U : 1U),
                    "state resource availability uses signed words at eight and ten, never the quantity at six"
                );
            }
        }
    }

    {
        ListStateFixture fixture;
        fixture.list.secondary_required = 1U;
        fixture.configuration.actor_record[1U] = 9U << 16U;
        fixture.configuration.actor_record[2U] = 0U;
        fixture.port.on_measure = [&]() {
            fixture.list.secondary_required = 11U;
            fixture.configuration.actor_record[2U] = 11U;
        };

        const auto result = fixture.run();
        test.expect_true(
            result.available && fixture.message_head == 0x74000000U &&
                fixture.messages.allocations.size() == 1U &&
                fixture.messages.allocations[0U].record.text_token ==
                    0x004A7CC0U &&
                fixture.messages.allocations[0U].record.value_04 == 0x118U &&
                fixture.messages.allocations[0U].record.value_08 == 10U &&
                fixture.messages.allocations[0U].record.kind == 20U &&
                fixture.messages.allocations[0U].record.flags == 0x40000002U,
            "primary warning publishes a real message before secondary capacity and requirement are reread"
        );
    }

    for (const bool allocation_failure : {false, true}) {
        ListStateFixture fixture;
        fixture.configuration.actor_record[1U] = 0U;
        fixture.port.fail_allocation = allocation_failure;
        fixture.port.fail_measure = !allocation_failure;
        const auto result = fixture.run();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::message_typed_stop &&
                !result.available && fixture.message_head == 0U &&
                fixture.list.primary_required == 10U &&
                fixture.messages.allocations.size() ==
                    (allocation_failure ? 0U : 1U),
            "state message failure preserves the published threshold and allocation prefix"
        );
    }

    {
        ListStateFixture fixture;
        const auto result = process_legacy_battle_actor_list_state(
            nullptr,
            nullptr,
            nullptr,
            fixture.messages,
            fixture.message_head,
            fixture.port,
            fixture.audio.samples,
            fixture.mix_level,
            0U,
            0U
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                !result.available && fixture.messages.allocations.empty(),
            "zero occurrence returns before accessing any actor or list"
        );
    }

    {
        ListStateFixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x4CU
        ) = 0x0806U;
        fixture.list.next_resource_head_token = 0x75000000U;
        inventories.assign(fixture.list, {{.token = 0x75000000U, .name = {}}});
        const auto result = fixture.run();
        test.expect_true(
            !result.available &&
                result.status == LegacyBattleActorListQueryStatus::completed &&
                fixture.list.resource_head_token == 0U &&
                fixture.message_head == 0x74000000U &&
                fixture.messages.allocations[0U].record.text_token ==
                    0x004A7CD0U,
            "empty resource chain publishes its null cursor before the actual missing-resource message"
        );
    }
}

void test_list_state_filters(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    struct CategoryCase {
        u32 selector;
        u32 mask;
        bool matches;
    };

    constexpr std::array categories{
        CategoryCase{0U, 0x10U, true},
        CategoryCase{0U, 8U, false},
        CategoryCase{1U, 4U, true},
        CategoryCase{1U, 8U, true},
        CategoryCase{1U, 0x10U, false},
        CategoryCase{2U, 1U, true},
        CategoryCase{2U, 0x1000U, true},
        CategoryCase{2U, 0x10U, false},
        CategoryCase{3U, 2U, true},
        CategoryCase{3U, 4U, false},
        CategoryCase{0xFFFFFFFFU, 0x80000000U, true}
    };

    for (const auto& category : categories) {
        for (const u16 type :
             {u16{0x1AU},
              u16{0x1BU},
              u16{0x1CU},
              u16{0x1EU},
              u16{0x1FU},
              u16{0xFFFFU}}) {
            for (const u8 mode :
                 {u8{0U}, u8{1U}, u8{2U}, u8{4U}, u8{5U}, u8{8U}}) {
                ListStateFixture fixture;
                auto& node = openswd3::test::party_node(fixture.list, 0U);
                openswd3::test::inventory_category(node) = category.mask;
                openswd3::test::inventory_word(node, 0x5EU) = type;
                node.definition_snapshot[0x46U - 0x0CU] = mode;
                const bool available = category.matches &&
                    (type == 0x1BU || type == 0x1CU || type == 0x1EU) &&
                    (mode == 1U || mode == 4U || mode == 5U);
                const auto result = fixture.run(category.selector);
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.available == available &&
                        fixture.actor.current_list_index ==
                            (available
                                 ? openswd3::test::party_root(fixture.list)
                                       .legacy_token
                                 : 0U) &&
                        fixture.messages.allocations.empty(),
                    "state list filtering preserves raw masks, byte modes, inclusive type limits and final cursor"
                );
            }
        }
    }

    {
        ListStateFixture fixture;
        auto node = openswd3::test::party_node(fixture.list, 0U);
        openswd3::test::party_node(fixture.list, 0U).legacy_next_token =
            0x70000020U;
        node.legacy_token = 0x70000020U;
        openswd3::test::append_party_item(fixture.list, node);
        test.expect_true(
            fixture.run(0U, 2U).available,
            "state list selects the second matching node"
        );
        test.expect_true(
            !fixture.run(0U, 3U).available &&
                fixture.actor.current_list_index == 0U,
            "missing occurrence publishes the exhausted list cursor"
        );
    }

    {
        ListStateFixture fixture;
        openswd3::test::party_root(fixture.list).legacy_next_token =
            0xBAD00000U;
        const auto result = fixture.run();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                fixture.actor.current_list_index == 0xBAD00000U,
            "unmapped next node retains the cursor publication before access fails"
        );
    }

    struct FlagCase {
        u16 flags;
        u16 primary;
        u16 secondary;
        u32 selected;
    };

    constexpr std::array flags{
        FlagCase{0U, 17U, 19U, 0xDEADBEEFU},
        FlagCase{0x8005U, 5U, 19U, 0U},
        FlagCase{0x4006U, 17U, 6U, 0U},
        FlagCase{0xC007U, 0x4007U, 7U, 0U},
        FlagCase{0x8805U, 5U, 19U, 0U},
        FlagCase{0x4806U, 6U, 0x806U, 0U},
        FlagCase{0xC807U, 7U, 0x807U, 0U}
    };

    for (const auto& entry : flags) {
        ListStateFixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x4CU
        ) = entry.flags;
        fixture.list.primary_required = 17U;
        fixture.list.secondary_required = 19U;
        fixture.list.next_resource_head_token = 0x75000000U;
        inventories.assign(fixture.list, {{.token = 0x75000000U, .name = {}}});
        const auto result = fixture.run();
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                fixture.list.primary_required == entry.primary &&
                fixture.list.secondary_required == entry.secondary &&
                fixture.list.selected_resource_token == entry.selected,
            "state flag combinations preserve untouched thresholds and apply resource overwrite last"
        );
    }

    {
        ListStateFixture fixture;
        fixture.message_head = 0xABCDEF00U;
        fixture.configuration.actor_record[1U] = 0U;
        fixture.list.secondary_required = 0xFFFFU;
        const auto result = fixture.run();
        test.expect_true(
            !result.available &&
                result.status == LegacyBattleActorListQueryStatus::completed &&
                fixture.message_head == 0xABCDEF00U &&
                fixture.messages.allocations.empty(),
            "existing message suppresses both warnings without dereferencing its token"
        );
    }
}

void test_list_state_audio_order(openswd3::test::Context& test) {
    using namespace openswd3::audio_video;
    ListStateFixture fixture;
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT};
    std::filesystem::create_directories(root);
    const auto path =
        root / ("list-state-warning-" + std::to_string(stamp) + ".snd");
    constexpr u32 payload = 0x1CU + 3000U * 0x2CU;
    std::vector<u8> bytes(payload + 48U, 0U);
    const auto write_word = [&](const std::size_t offset, const u32 value) {
        for (u32 index = 0U; index < 4U; ++index) {
            bytes[offset + index] = static_cast<u8>(value >> (index * 8U));
        }
    };

    constexpr u32 record = 0x1CU + 139U * 0x2CU;
    write_word(record + 0x14U, 48U);
    write_word(record + 0x18U, payload);
    bytes[payload] = 1U;
    {
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(
            reinterpret_cast<const char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size())
        );
    }

    test.expect_true(
        fixture.audio.archive->open(path) == LegacySndOpenStatus::ready &&
            fixture.audio.samples.initialize_pool(1) ==
                LegacySampleManagerInitializeStatus::ready,
        "state warning fixture opens the real SND index and sample pool"
    );
    fixture.configuration.actor_record[1U] = 0U;
    fixture.configuration.actor_record[2U] = 0U;
    fixture.list.secondary_required = 1U;
    bool measured_before_audio = false;
    bool audio_after_publication = false;
    fixture.port.on_measure = [&]() {
        measured_before_audio = fixture.message_head == 0U &&
            fixture.audio.backend.started_ids.empty();
        fixture.mix_level = 5;
    };

    fixture.audio.backend.on_start = [&]() {
        audio_after_publication = fixture.message_head == 0x74000000U &&
            fixture.messages.allocations[0U].record.text_token == 0x004A7CC0U;
        fixture.list.secondary_required = 11U;
        fixture.configuration.actor_record[2U] = 11U;
    };

    const auto result = fixture.run();
    test.expect_true(
        result.available, "secondary capacity reflects the audio callback"
    );
    test.expect_true(
        measured_before_audio, "message measurement precedes audio"
    );
    test.expect_true(
        audio_after_publication, "message publication precedes audio"
    );
    test.expect_true(
        fixture.audio.backend.started_ids == std::vector<u32>{140U},
        "state warning plays sound 140 exactly once"
    );
    test.expect_equal(
        fixture.audio.backend.last_volume,
        58,
        "warning rereads the mix level after message measurement"
    );
    test.expect_equal(
        fixture.audio.backend.last_pan, 63, "warning keeps centered pan"
    );
    test.expect_equal(
        fixture.audio.backend.last_loop_count,
        1,
        "warning plays without repetition"
    );
    test.expect_equal(
        fixture.audio.samples.active_sample_count(),
        std::size_t{1U},
        "warning sample remains owned by its manager"
    );
    static_cast<void>(fixture.audio.samples.shutdown());
    fixture.audio.archive->close();
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
}

class QueryPort final : public openswd3::test::LegacyBattleMonDatabaseFixture {
public:
    [[nodiscard]] openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const u32 handle, const std::span<u8> destination, const u32 size
    ) override {
        const auto result = LegacyBattleMonDatabaseFixture::read_mon_file(
            handle, destination, size
        );
        if (read_calls % 3U == 0U && on_profile_read) {
            on_profile_read();
        }

        return result;
    }

    std::function<void()> on_profile_read;

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

void test_list_query_live_state(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    struct Fixture {
        openswd3::test::LegacyBattleInventoryFixture inventories;
        Fixture() {
            actor.next_list_index = 0x10U;
            openswd3::test::bind_party_list(inventories, list, 0x10U);
            openswd3::test::party_root(list).legacy_next_token = 1U;
            openswd3::test::append_party_item(
                list,
                {
                    .token = 1U,
                    .category_flags = 0x10U,
                    .mode_flags = 1U,
                    .type = 0x1BU,
                    .profile_id = 7U,
                    .value_flags = 0x8007U,
                    .text = "name",
                }
            );
            text.fill(0xCCU);
        }

        [[nodiscard]] LegacyBattleActorListQueryResult query(
            const u32 category = 0U,
            const u32 type = 0U,
            const u32 occurrence = 1U
        ) {
            return query_legacy_battle_actor_list(
                &actor,
                &list,
                port,
                category,
                type,
                occurrence,
                text,
                value,
                profile
            );
        }

        LegacyBattleGroupAActionExecutionState actor;
        LegacyBattleActorListQueryState list;
        QueryPort port;
        LegacyBattleMonProfile profile{};
        std::array<u8, 32> text{};
        u32 value{0xAABB0011U};
    };

    {
        Fixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x4CU
        ) = 0x8002U;
        openswd3::test::append_party_item(
            fixture.list,
            {.token = 2U, .value_flags = 0x4003U, .text = "second"}
        );
        openswd3::test::append_party_item(
            fixture.list,
            {.token = 0x8003U, .value_flags = 0x0800U, .text = "third"}
        );
        fixture.port.index = 3U;
        const auto result = query_legacy_battle_actor_list(
            &fixture.actor,
            &fixture.list,
            fixture.port,
            0U,
            0U,
            1U,
            fixture.text,
            fixture.actor.current_list_index,
            fixture.profile
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 3U &&
                fixture.actor.current_list_index == 1000U &&
                fixture.text[0U] == 'n',
            "query rereads the cursor after each aliased WORD output store"
        );
    }

    {
        Fixture fixture;
        openswd3::test::append_party_item(
            fixture.list,
            {
                .token = 2U,
                .value_flags = 0xFFFFU,
                .text = std::string("new\0tail", 8U),
            }
        );
        fixture.port.index = 2U;
        fixture.port.on_profile_read = [&]() {
            openswd3::test::party_nodes(fixture.list).pop_front();
            fixture.actor.current_list_index = 2U;
            fixture.value = 0xCCDD0011U;
        };
        const auto result = fixture.query();
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 2U && fixture.value == 0xCCDD03E8U &&
                fixture.port.requested_profile_ids == std::vector<u16>{7U} &&
                fixture.actor.current_list_index == 2U &&
                fixture.text[0U] == 'n' && fixture.text[1U] == 'e' &&
                fixture.text[2U] == 'w' && fixture.text[3U] == 0U &&
                fixture.text[4U] == 0xCCU,
            "query reloads the node after MON releases the original node and copies only through the first NUL"
        );
    }

    {
        Fixture fixture;
        fixture.port.on_profile_read = [&]() {
            openswd3::test::party_nodes(fixture.list).clear();
            fixture.actor.current_list_index = 99U;
        };
        const auto result = fixture.query();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                fixture.value == 0xAABB0011U && fixture.text[0U] == 0xCCU &&
                fixture.actor.current_list_index == 99U,
            "missing callback-selected node stops before text and value stores"
        );
    }

    {
        Fixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x5EU
        ) = 0x1FU;
        fixture.profile[0x10U] = std::byte{3U};
        const auto result = fixture.query(0U, 7U);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 3U && fixture.value == 0xAABB0000U &&
                fixture.port.requested_profile_ids.empty() &&
                fixture.text[0U] == 0xCCU,
            "raw type queries retain caller profile bytes and skip MON and text"
        );
    }

    {
        Fixture fixture;
        fixture.port.open_succeeds = false;
        fixture.profile[0x10U] = std::byte{2U};
        const auto result = fixture.query();
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 2U && fixture.value == 0xAABB0007U &&
                fixture.text[0U] == 'n',
            "MON open failure retains profile contents and continues the original suffix"
        );
    }

    {
        Fixture fixture;
        fixture.port.allocation_succeeds = false;
        const auto result = fixture.query();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::profile_load_typed_stop &&
                fixture.actor.current_list_index == 1U &&
                fixture.value == 0xAABB0011U && fixture.text[0U] == 0xCCU,
            "MON allocation stop preserves the published cursor and untouched outputs"
        );
    }

    {
        Fixture fixture;
        const auto result = query_legacy_battle_actor_list(
            &fixture.actor,
            &fixture.list,
            fixture.port,
            0U,
            0U,
            1U,
            std::span{fixture.text}.first(3U),
            fixture.value,
            fixture.profile
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_text_typed_stop &&
                fixture.text[0U] == 'n' && fixture.text[1U] == 'a' &&
                fixture.text[2U] == 'm' && fixture.text[3U] == 0xCCU &&
                fixture.value == 0xAABB0011U,
            "bounded text failure retains its copied prefix before value publication"
        );
    }

    {
        Fixture fixture;
        fixture.port.index = 24U;
        const auto result = fixture.query();
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::return_table_typed_stop &&
                fixture.value == 0xAABB0007U && fixture.text[4U] == 0U,
            "display table bounds are checked after all text and value effects"
        );
    }

    {
        Fixture fixture;
        openswd3::test::inventory_word(
            openswd3::test::party_node(fixture.list, 0U), 0x4CU
        ) = 0U;
        const auto result = fixture.query();
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                fixture.value == 0xAABB0011U,
            "unmarked primary values retain the caller output WORD"
        );
    }

    {
        Fixture fixture;
        const auto result = fixture.query(0U, 0U, 0U);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 0xFFFFU &&
                fixture.actor.current_list_index == 0U &&
                fixture.value == 0xAABBFFFFU &&
                fixture.port.requested_profile_ids.empty(),
            "zero occurrence reaches the tail when the first node increments the match count"
        );
    }

    {
        Fixture fixture;
        openswd3::test::party_root(fixture.list).legacy_next_token = 0U;
        const auto result = query_legacy_battle_actor_list(
            &fixture.actor,
            &fixture.list,
            fixture.port,
            0U,
            0U,
            1U,
            fixture.text,
            fixture.actor.current_list_index,
            fixture.profile
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 0xFFFFU &&
                fixture.actor.current_list_index == 0xFFFFU,
            "tail publication precedes the aliased sentinel WORD store"
        );
    }
}

void test_resource_selection_outputs(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    struct SelectionCase {
        u32 category;
        u32 mask;
        u16 capacity;
        u8 mode;
        u16 primary;
        openswd3::compat::i16 secondary;
        openswd3::compat::i16 tertiary;
        u32 occurrence;
        bool selected;
        bool special;
        u16 output_mode;
    };

    constexpr std::array<SelectionCase, 17> cases{{
        {0U, 0x10U, 0U, 1U, 0U, 0, 1, 0U, false, false, 0xABCDU},
        {0U, 0x10U, 0U, 1U, 0U, 0, 1, 1U, true, false, 1U},
        {1U, 4U, 0U, 1U, 0U, 0, 1, 1U, true, false, 0U},
        {1U, 8U, 0U, 1U, 0U, 0, 1, 1U, true, false, 1U},
        {2U, 0x1000U, 0U, 4U, 0U, 0, 1, 1U, true, false, 0U},
        {3U, 0x800U, 0U, 1U, 0U, 0, 1, 1U, true, false, 0U},
        {4U, 0x2000U, 0x2000U, 1U, 0U, 0, 1, 1U, false, false, 0xABCDU},
        {4U, 0x2000U, 0x2000U, 1U, 0U, 0, 1, 2U, true, true, 0U},
        {4U, 0U, 0x2000U, 0U, 0U, 0, 0, 1U, true, true, 0U},
        {5U, 0x08000000U, 0U, 1U, 0U, 0, 1, 1U, true, false, 3U},
        {8U, 8U, 0U, 1U, 0U, 0, 1, 1U, true, false, 1U},
        {0x10001U, 1U, 0U, 1U, 0U, 0, 1, 1U, true, false, 0U},
        {0U, 0x10U, 0U, 2U, 0U, 0, 1, 1U, false, false, 0xABCDU},
        {0U, 0x10U, 0U, 1U, 0U, 0, 1, 0x10001U, false, false, 0xABCDU},
        {0U, 0x10U, 0U, 1U, 0xFFFFU, 32767, 32767, 1U, false, false, 0xABCDU},
        {0U, 0x10U, 0U, 1U, 0xFFFEU, 32767, 32767, 1U, false, false, 0xABCDU},
        {0U, 0x10U, 0U, 1U, 0xFFFDU, 32767, 32767, 1U, true, false, 1U},
    }};
    for (const auto& item : cases) {
        LegacyBattleActorListQueryState list;
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState effect;
        LegacyBattleGroupAWorkspaceState workspace;
        SelectionPort port;
        port.set_profile_word(0x14U, 0xBEEFU);
        list.resource_head_token = 0x99U;
        list.next_resource_head_token = 0x10U;
        list.selected_resource_token = 0x77U;
        inventories.assign(
            list,
            {
                {.token = 0x10U, .next_token = 0x20U, .name = {}},
                {.token = 0x20U,
                 .resource_id = 0x301U,
                 .primary_quantity = item.primary,
                 .secondary_quantity = item.secondary,
                 .tertiary_quantity = item.tertiary,
                 .name = {},
                 .category_mask = item.mask,
                 .derived_word_30 = 0x1234U,
                 .profile_id_4a = 7U,
                 .mode_flags = item.mode,
                 .capacity_gate_flags = item.capacity},
            }
        );

        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        workspace.tail_words[2U] = 0xFFFFU;
        u32 runtime_word = 0xDEADBEEFU;
        u16 output_mode = 0xABCDU;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            &configuration,
            &final_state,
            &effect,
            &workspace,
            port,
            port,
            item.category,
            item.occurrence,
            runtime_word,
            output_mode
        );
        const u16 expected_primary = static_cast<u16>(
            item.primary +
            (item.selected && !item.special && item.category != 3U ? 1U : 0U)
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selected == item.selected &&
                output_mode == item.output_mode &&
                runtime_word ==
                    (item.selected && !item.special ? 0xBEEFU : 0xDEADBEEFU) &&
                list.selected_resource_token ==
                    (item.selected ? 0x20U : 0x77U) &&
                list.resource_head_token ==
                    (item.occurrence == 0U ? 0x99U
                         : item.selected   ? 0x20U
                                           : 0U) &&
                openswd3::test::inventory_node(list, 1U).selected_count ==
                    expected_primary &&
                std::ranges::all_of(
                    final_state.pre_effect_words,
                    [](const u32 value) { return value == 0U; }
                ) &&
                workspace.tail_words[2U] == 0U &&
                (!item.special || effect.derived_words[0U] == 0x1234U),
            "resource selection preserves DWORD ordinals, signed eligibility, independent special counts and semantic output widths"
        );
    }

    for (const u32 mutation : {0U, 1U, 2U, 3U}) {
        LegacyBattleActorListQueryState list;
        LegacyBattleGroupAConfigurationState configuration;
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState effect;
        LegacyBattleGroupAWorkspaceState workspace;
        SelectionPort port;
        list.next_resource_head_token = 0x10U;
        inventories.assign(
            list,
            {
                {.token = 0x10U, .next_token = 0x20U, .name = {}},
                {.token = 0x20U,
                 .resource_id = 0x301U,
                 .tertiary_quantity = 2,
                 .name = {},
                 .category_mask = 0x10U,
                 .profile_id_4a = 7U,
                 .mode_flags = 1U},
                {.token = 0x30U,
                 .resource_id = 0x302U,
                 .primary_quantity = 0xFFFFU,
                 .name = {},
                 .category_mask = 0x10U},
            }
        );

        port.set_profile_word(0x14U, mutation == 2U ? 0U : 0x30U);
        if (mutation == 1U) {
            port.before_open = [&]() { list.resource_head_token = 0x30U; };
        } else if (mutation == 2U) {
            port.after_diagnostic = [&]() { list.resource_head_token = 0x30U; };
        }

        u32 runtime_word = 0xDEADBEEFU;
        u16 output_mode = 0xABCDU;
        auto& runtime_output =
            mutation == 0U ? list.resource_head_token : runtime_word;
        auto& mode_output = mutation == 3U
            ? openswd3::test::inventory_node(list, 1U).selected_count
            : output_mode;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            &configuration,
            &final_state,
            &effect,
            &workspace,
            port,
            port,
            0U,
            1U,
            runtime_output,
            mode_output
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selected && list.selected_resource_token == 0x20U &&
                list.resource_head_token == (mutation == 3U ? 0x20U : 0x30U) &&
                mode_output == (mutation == 3U ? 2U : 1U) &&
                openswd3::test::inventory_node(list, 1U).selected_count ==
                    (mutation == 3U ? 2U : 0U) &&
                openswd3::test::inventory_node(list, 2U).selected_count ==
                    (mutation == 3U ? 0xFFFFU : 0U) &&
                (mutation != 2U ||
                 port.diagnostics == std::vector<u16>{0x301U}),
            "selection rereads current resources after profile I/O, diagnostics and aliased output stores, then wraps quantity as a WORD"
        );
    }
}

void test_resource_selection_capacity_prefixes(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    constexpr std::array<std::array<u16, 2>, 9> capacities{{
        {0U, 0U},
        {1U, 1U},
        {0x7FFFU, 0x7FFFU},
        {0x8000U, 0x7FFFU},
        {0x7FFFU, 0x8000U},
        {0xFFFFU, 0U},
        {0U, 0xFFFFU},
        {1U, 0U},
        {0U, 1U},
    }};
    for (const u16 gates :
         {0x8000U, 0x8001U, 0xBFFFU, 0xC001U, 0x4000U, 0x4001U, 0x7FFFU}) {
        for (const auto& capacity : capacities) {
            for (const bool alternate : {false, true}) {
                LegacyBattleActorListQueryState list;
                LegacyBattleGroupAConfigurationState configuration;
                LegacyBattleGroupAFinalProcessingState final_state;
                LegacyBattleGroupAItemEffectApplicationState effect;
                LegacyBattleGroupAWorkspaceState workspace;
                SelectionPort port;
                port.set_profile_word(0x14U, 0xBEEFU);
                configuration.actor_record_token = 1U;
                configuration.actor_record[1U] = static_cast<u32>(capacity[0U])
                    << 16U;
                configuration.actor_record[2U] = capacity[1U];
                list.primary_required = 0xABCDU;
                list.secondary_required = 0xDCBAU;
                list.next_resource_head_token = 0x10U;
                inventories.assign(
                    list,
                    {
                        {.token = 0x10U, .next_token = 0x20U, .name = {}},
                        {.token = 0x20U,
                         .resource_id = 0x301U,
                         .tertiary_quantity = 1,
                         .name = {},
                         .category_mask = 0x10U,
                         .profile_id_4a = 7U,
                         .mode_flags = 1U,
                         .capacity_gate_flags = gates,
                         .alternate_profile_id_54 =
                             static_cast<u16>(alternate ? 9U : 0U)},
                    }
                );

                const bool checks_primary = (gates & 0x8000U) != 0U;
                const bool primary_passes = !checks_primary ||
                    (capacity[0U] < 0x8000U &&
                     (gates & 0x7FFFU) <= capacity[0U]);
                const bool checks_secondary =
                    primary_passes && (gates & 0x4000U) != 0U;
                const bool succeeds = primary_passes &&
                    (!checks_secondary ||
                     (capacity[1U] < 0x8000U &&
                      (gates & 0x3FFFU) <= capacity[1U]));
                u32 runtime_word = 0xDEADBEEFU;
                u16 output_mode = 0xABCDU;
                const auto result = select_legacy_battle_actor_resource(
                    &list,
                    &configuration,
                    &final_state,
                    &effect,
                    &workspace,
                    port,
                    port,
                    0U,
                    1U,
                    runtime_word,
                    output_mode
                );
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.selected == succeeds &&
                        list.selected_resource_token == 0x20U &&
                        (effect.mode_flags & 0x40U) != 0U &&
                        final_state.profile_buffer[5U] == 0xBEEFU &&
                        runtime_word == (succeeds ? 0xBEEFU : 0xDEADBEEFU) &&
                        output_mode == (succeeds ? 1U : 0U) &&
                        openswd3::test::inventory_node(list, 1U)
                                .selected_count == (succeeds ? 1U : 0U) &&
                        list.primary_required ==
                            (checks_primary ? gates & 0x7FFFU : 0xABCDU) &&
                        list.secondary_required ==
                            (checks_secondary ? gates & 0x3FFFU : 0xDCBAU) &&
                        port.profile_ids ==
                            std::vector<u16>{
                                static_cast<u16>(alternate ? 9U : 7U)
                            },
                    "capacity gates compare signed configuration WORDs in order and retain selection/profile/output prefixes on rejection"
                );
            }
        }
    }
}

void test_actor_resource_release_prefixes(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    using openswd3::compat::u32;
    for (const u32 path : {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U}) {
        LegacyBattleActorListQueryState list;
        LegacyBattleGroupAWorkspaceState workspace;
        workspace.tail_words[2U] = 0xBEEFU;
        list.resource_head_token = 0x12345678U;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000020U;
        inventories.assign(
            list,
            {
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
            }
        );

        if (path == 0U) {
            list.selected_resource_token = 0U;
            list.next_resource_head_token = 0xDEADBEEFU;
        } else if (path == 1U) {
            openswd3::test::inventory_node(list, 0U).legacy_next_token = 0U;
        } else if (path == 2U) {
            list.next_resource_head_token = 0xDEADBEEFU;
        } else if (path == 3U) {
            openswd3::test::inventory_node(list, 0U).legacy_next_token =
                0xDEADBEEFU;
        } else if (path == 4U) {
            list.selected_resource_token = 0xDEADBEEFU;
        } else if (path == 6U) {
            list.selected_resource_token = 0x76000000U;
            openswd3::test::inventory_node(list, 0U).item_id = 0x777U;
            openswd3::test::inventory_node(list, 0U).selected_count = 4U;
            openswd3::test::inventory_node(list, 0U).quantity_a = 1;
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
                openswd3::test::inventory_nodes(list).size() ==
                    (path == 6U ? 2U : 3U) &&
                openswd3::test::inventory_nodes(list).back().selected_count ==
                    4U &&
                openswd3::test::inventory_nodes(list).back().quantity_a == 2 &&
                openswd3::test::inventory_node(list, path == 6U ? 0U : 1U)
                        .quantity_a == 7,
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
        inventories.assign(
            list,
            {
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
            }
        );

        const auto status = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, &workspace
        );
        test.expect_true(
            status ==
                    (path < 2U ? LegacyBattleActorListQueryStatus::completed
                               : LegacyBattleActorListQueryStatus::
                                     resource_node_typed_stop) &&
                configuration.actor_record[2U] ==
                    (path < 2U ? 0xABCD0007U : 0xABCD000AU) &&
                list.primary_required == (path == 1U ? 0U : 9U) &&
                list.secondary_required == (path < 2U ? 0U : 3U) &&
                list.selected_resource_token ==
                    (path == 1U || path == 3U ? 0U : 0x76000010U) &&
                openswd3::test::inventory_nodes(list).size() ==
                    (path == 3U ? 1U : 2U) &&
                openswd3::test::inventory_nodes(list).back().quantity_a == 2 &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_nodes(list).back(), 0x48U
                ) == (path == 1U ? 0x0401U : 0x0402U) &&
                workspace.tail_words[2U] ==
                    (path == 1U || path == 3U ? 0x777U : 0xBEEFU),
            "list action discards the normal zero WORD, clears required capacity only after successful consumption and preserves unrefreshed capacity after resource failure"
        );
    }
}

void test_shared_inventory_release(openswd3::test::Context& test) {
    using namespace openswd3::battle;
    for (const bool invalidate_predecessor : {false, true}) {
        for (const bool suppress_return : {false, true}) {
            InventoryReleaseMemory memory;
            openswd3::world_map::LegacyWorldItemListState items{memory};
            LegacyBattleActorListQueryState list;
            LegacyBattleGroupAWorkspaceState workspace;
            list.inventory = &items;
            list.next_resource_head_token = kLegacyBattlePlayerItemHeadToken;
            list.selected_resource_token = 0x71000000U;
            items.player_inventory_head_token = 0x71000000U;
            items.player_inventory_head_alias.legacy_next_token = 0xDEADBEEFU;
            auto& selected = items.player_inventory.emplace_back();
            selected.legacy_token = 0x71000000U;
            selected.legacy_next_token = 0x710000B0U;
            selected.item_id = 0x0111U;
            selected.selected_count = 1U;
            selected.quantity_a = 1U;
            auto& successor = items.player_inventory.emplace_back();
            successor.legacy_token = 0x710000B0U;
            successor.item_id = 0x0222U;
            successor.quantity_b = 3U;
            auto description = std::make_shared<LegacyBattleMonText::Storage>(
                LegacyBattleMonText::Storage{0x41U, 0U}
            );
            u32 description_releases = 0U;
            selected.description.bind(
                description,
                std::make_shared<const LegacyBattleMonText::Release>([&] {
                    ++description_releases;
                    return true;
                })
            );
            bool release_prefix = false;
            memory.on_release = [&] {
                release_prefix =
                    items.player_inventory_head_token == 0x71000000U &&
                    list.resource_head_token == 0x71000000U &&
                    list.selected_resource_token == 0U &&
                    workspace.tail_words[2U] == 0x0111U;
                workspace.tail_words[2U] = 0xBEEFU;
                if (invalidate_predecessor) {
                    list.next_resource_head_token = 0xDEADBEEFU;
                }

                openswd3::test::set_inventory_definition_word(
                    items.player_inventory_head_alias,
                    0x4CU,
                    suppress_return ? 0x2000U : 0U
                );
            };
            const auto result =
                release_legacy_battle_actor_resource(&list, &workspace);
            memory.on_release = {};
            test.expect_true(
                release_prefix && description_releases == 0U &&
                    *description == LegacyBattleMonText::Storage{0x41U, 0U} &&
                    items.player_inventory.size() == 1U &&
                    items.player_inventory.front().legacy_token ==
                        0x710000B0U &&
                    workspace.tail_words[2U] == 0xBEEFU,
                "the real allocation is released before relinking and its description receives no extra release"
            );
            if (invalidate_predecessor) {
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::
                                resource_node_typed_stop &&
                        items.player_inventory_head_token == 0x71000000U &&
                        list.resource_head_token == 0xDEADBEEFU &&
                        list.selected_resource_token == 0U,
                    "a missing predecessor after release retains deletion and quantity-update prefixes"
                );
            } else {
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        items.player_inventory_head_token == 0x710000B0U &&
                        list.resource_head_token ==
                            kLegacyBattlePlayerItemHeadToken &&
                        result.output_word == (suppress_return ? 0U : 0xBEEFU),
                    "release rereads the global head flags and actual workspace after the allocator callback"
                );
            }
        }
    }
}

void test_resource_head_publication(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
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
            inventories.assign(
                list,
                {
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
                }
            );

            const auto* first = &openswd3::test::inventory_nodes(list).front();
            const auto status = commit_legacy_battle_actor_resource_list(&list);
            test.expect_true(
                status == LegacyBattleActorListQueryStatus::completed &&
                    list.resource_head_token == head &&
                    list.next_resource_head_token == head &&
                    list.selected_resource_token == 0xCAFEBABEU &&
                    openswd3::test::inventory_nodes(list).size() == 2U &&
                    &openswd3::test::inventory_nodes(list).front() == first &&
                    first->legacy_next_token == 0x76000010U &&
                    first->item_id == 0xFFFFU &&
                    first->selected_count == 0xABCDU &&
                    first->quantity_a == 0xFFFFU && first->quantity_b == 2 &&
                    LegacyBattleItemDefinitionView{*first}.name() ==
                        "retained" &&
                    LegacyBattleItemDefinitionView{*first}.category_mask() ==
                        0xFFFFFFFFU &&
                    LegacyBattleItemDefinitionView{*first}.mode_flags() ==
                        0xFFU &&
                    LegacyBattleItemDefinitionView{*first}
                            .capacity_gate_flags() == 0xFFFFU,
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
            inventories.assign(
                list,
                {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .resource_id = 0x0300U,
                     .primary_quantity = 1U,
                     .secondary_quantity = 2,
                     .tertiary_quantity = 3,
                     .name = "published",
                     .category_mask = 0x08000010U,
                     .mode_flags = 0x01U,
                     .capacity_gate_flags = 0x2000U},
                }
            );

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
                u16 quantity = 0xBEEFU;
                status =
                    count_legacy_battle_actor_mode_resources(&list, quantity);
                test.expect_true(
                    quantity == 0U,
                    "mode count clears its output before a missing first node"
                );
                break;
            }

            case 3U: {
                u16 count = 0xABCDU;
                status =
                    count_legacy_battle_actor_resource_list(&list, 0U, count);
                test.expect_true(
                    count == (missing_root ? 0xABCDU : 0xABCEU),
                    "resource count retains the caller's initial word before the first-node failure"
                );
                break;
            }

            default: {
                std::array<u8, 20> name{};
                u16 flags = 0xBEEFU;
                u16 quantity = 0xABCDU;
                const auto result = query_legacy_battle_actor_resource_list(
                    &list, nullptr, 0U, 1U, name, flags, quantity
                );
                status = result.status;
                test.expect_true(
                    !missing_root || flags == 0U,
                    "resource query clears flags after publication and before a missing first node"
                );
                break;
            }
            }

            const auto failure = consumer == 3U
                ? LegacyBattleActorListQueryStatus::resource_owner_typed_stop
                : LegacyBattleActorListQueryStatus::resource_node_typed_stop;
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
                    openswd3::test::inventory_nodes(list).size() == 2U &&
                    openswd3::test::inventory_nodes(list)
                            .back()
                            .selected_count == 1U &&
                    openswd3::test::inventory_nodes(list).back().quantity_a ==
                        2 &&
                    openswd3::test::inventory_nodes(list).back().quantity_b ==
                        3,
                "resource consumers start from the published root and preserve publication on their existing first-node failure"
            );
        }
    }
}

void test_flagged_resource_display(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
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
                inventories.assign(
                    list,
                    {{.token = 0x76000000U,
                      .next_token = 0x76000010U,
                      .name = {}}}
                );

                for (u32 index = 1U; index <= 4U; ++index) {
                    openswd3::test::append_inventory_item(
                        list,
                        {
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
                        }
                    );
                }

                const auto* original_nodes =
                    openswd3::test::inventory_data(list);
                const auto original_definition =
                    openswd3::test::inventory_nodes(list)
                        .back()
                        .definition_snapshot;
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
                        openswd3::test::inventory_data(list) ==
                            original_nodes &&
                        openswd3::test::inventory_nodes(list).size() == 5U &&
                        openswd3::test::inventory_nodes(list)
                                .back()
                                .definition_snapshot == original_definition &&
                        std::bit_cast<u16>(openswd3::test::inventory_nodes(list)
                                               .back()
                                               .quantity_a) ==
                            quantity.secondary &&
                        std::bit_cast<u16>(openswd3::test::inventory_nodes(list)
                                               .back()
                                               .quantity_b) ==
                            quantity.tertiary,
                    "actual flagged query preserves destructive ordinal scans, null-terminated names, untouched tails and word sums"
                );
            }
        }
    }

    for (u32 failure = 0U; failure < 3U; ++failure) {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token =
            failure == 0U ? 0xBAD00000U : 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
            }
        );

        if (failure == 2U) {
            openswd3::test::append_inventory_item(
                list,
                {
                    .token = 0x76000010U,
                    .name = "name",
                    .capacity_gate_flags = 0x2000U,
                }
            );
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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .secondary_quantity = 99,
                 .name =
                     std::string(1U, missing_reloaded_node ? '\x30' : '\x20'),
                 .capacity_gate_flags = 0x2000U},
                {.token = 0x76000020U,
                 .secondary_quantity = 3,
                 .tertiary_quantity = 4,
                 .name = "reloaded"},
            }
        );

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
    openswd3::test::LegacyBattleInventoryFixture inventories;
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
                inventories.assign(
                    list,
                    {
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
                    }
                );

                const auto* original_storage =
                    openswd3::test::inventory_data(list);
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
                        openswd3::test::inventory_data(list) ==
                            original_storage &&
                        openswd3::test::inventory_nodes(list).size() == 5U &&
                        openswd3::battle::LegacyBattleItemDefinitionView{
                            openswd3::test::inventory_node(list, 1U)
                        }
                                .name() == labels[1U] &&
                        openswd3::battle::LegacyBattleItemDefinitionView{
                            openswd3::test::inventory_node(list, 4U)
                        }
                                .name() == labels[4U],
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
                inventories.assign(
                    list,
                    {
                        {.token = 0x76000000U,
                         .next_token = 0x76000010U,
                         .name = {}},
                        {.token = 0x76000010U,
                         .resource_id =
                             static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                         .secondary_quantity =
                             std::bit_cast<openswd3::compat::i16>(
                                 quantity_case[0U]
                             ),
                         .tertiary_quantity =
                             std::bit_cast<openswd3::compat::i16>(
                                 quantity_case[1U]
                             ),
                         .name = source_names[name_index],
                         .category_mask = 0x08000000U,
                         .mode_flags = 5U},
                    }
                );

                std::array<u8, 20> name;
                name.fill(0xA5U);
                auto expected_name = name;
                std::copy_n(
                    source_names[name_index].begin(),
                    copied_sizes[name_index],
                    expected_name.begin()
                );
                expected_name[copied_sizes[name_index]] = 0U;
                const auto original_definition =
                    openswd3::test::inventory_node(list, 1U)
                        .definition_snapshot;
                u16 quantity = 0xBEEFU;
                const auto result = query_legacy_battle_actor_mode_resource(
                    &list, mode, 1U, name, quantity
                );
                test.expect_true(
                    result.status ==
                            LegacyBattleActorListQueryStatus::completed &&
                        result.found && name == expected_name &&
                        quantity == quantity_case[2U] &&
                        openswd3::test::inventory_node(list, 1U)
                                .definition_snapshot == original_definition,
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
            inventories.assign(
                list,
                {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .resource_id =
                         static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                     .secondary_quantity = 10,
                     .tertiary_quantity = 11,
                     .name = std::string(
                         1U, available_quantity_node ? '\x20' : '\x30'
                     ),
                     .category_mask = 0x08000000U,
                     .mode_flags = 1U},
                    {.token = 0x76000020U,
                     .secondary_quantity = 3,
                     .tertiary_quantity = 4,
                     .name = "quantity"},
                }
            );

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
            inventories.assign(
                list,
                {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .resource_id =
                         static_cast<u16>(mode == 0U ? 0x0300U : 0x0222U),
                     .name = empty_output ? "ok" : std::string(20U, 'x'),
                     .category_mask = 0x08000000U,
                     .mode_flags = 1U},
                }
            );

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
        inventories.assign(
            list,
            missing_root
                ? std::vector<openswd3::test::BattleInventoryItemFixture>{}
                : std::vector<openswd3::test::BattleInventoryItemFixture>{
                      {.token = 0x76000000U,
                       .next_token = 0x76000010U,
                       .name = {}}
                  }
        );

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

void test_resource_list_display(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    using openswd3::compat::i16;
    constexpr u32 root = 0x93000000U;
    const auto make_list = [&inventories] {
        LegacyBattleActorListQueryState list;
        list.resource_head_token = 0xDEAD0000U;
        list.next_resource_head_token = root;
        list.selected_resource_token = 0xCAFEBABEU;
        inventories.assign(
            list,
            {
                {.token = root, .next_token = root + 0x10U, .name = {}},
                {.token = root + 0x10U,
                 .tertiary_quantity = 1,
                 .name = "R",
                 .category_mask = 0x10U,
                 .mode_flags = 1U},
            }
        );

        return list;
    };

    struct Category {
        u32 selector;
        u32 mask;
    };

    constexpr std::array<Category, 8> categories{{
        {0U, 0x10U},
        {1U, 0x0CU},
        {2U, 0x1001U},
        {3U, 0x0800U},
        {4U, 4U},
        {5U, 5U},
        {0x80000000U, 0x80000000U},
        {0xFFFFFFFFU, 0xFFFFFFFFU},
    }};
    constexpr std::array<u32, 10> masks{
        0U,
        0x10U,
        0x0CU,
        0x1001U,
        0x0800U,
        4U,
        5U,
        0x2000U,
        0x80000000U,
        0xFFFFFFFFU,
    };

    struct Mode {
        u8 flags;
        bool enabled;
    };

    constexpr std::array<Mode, 7> modes{{
        {0U, false},
        {1U, true},
        {2U, false},
        {4U, true},
        {5U, true},
        {0x80U, false},
        {0xFFU, true},
    }};
    struct Quantity {
        u16 primary;
        i16 secondary;
        i16 tertiary;
        bool positive;
        u16 word;
    };

    constexpr std::array<Quantity, 6> quantities{{
        {0U, 0, 0, false, 0U},
        {0U, 0, 1, true, 1U},
        {0xFFFFU, 32767, 32767, false, 0xFFFFU},
        {0U, 32767, 32767, true, 0xFFFEU},
        {0xFFFFU, -32768, -32768, false, 1U},
        {0U, 0, -1, false, 0xFFFFU},
    }};
    for (const auto category : categories) {
        for (const u32 mask : masks) {
            for (const auto mode : modes) {
                for (const auto quantity : quantities) {
                    for (const u32 occurrence : {0U, 1U}) {
                        auto list = make_list();
                        auto& node = openswd3::test::inventory_node(list, 1U);
                        openswd3::test::inventory_category(node) = mask;
                        node.definition_snapshot[0x46U - 0x0CU] = mode.flags;
                        node.selected_count = quantity.primary;
                        node.quantity_a =
                            std::bit_cast<u16>(quantity.secondary);
                        node.quantity_b = std::bit_cast<u16>(quantity.tertiary);
                        const auto* storage =
                            openswd3::test::inventory_data(list);
                        std::array<u8, 20> name;
                        name.fill(0xCCU);
                        u16 flags = 0xBEEFU;
                        u16 output = 0xABCDU;
                        const auto result =
                            query_legacy_battle_actor_resource_list(
                                &list,
                                nullptr,
                                category.selector,
                                occurrence,
                                name,
                                flags,
                                output
                            );
                        const bool qualified =
                            (mask & category.mask) != 0U && mode.enabled;
                        const bool counted = qualified && quantity.positive;
                        const bool found =
                            occurrence == 0U ? !counted : counted;
                        const bool copied = found && qualified;
                        test.expect_true(
                            result.status ==
                                    LegacyBattleActorListQueryStatus::
                                        completed &&
                                result.found == found &&
                                flags == (copied ? 0x8000U : 0U) &&
                                output == (copied ? quantity.word : 0xABCDU) &&
                                name[0U] == (copied ? 'R' : 0xCCU) &&
                                name[1U] == (copied ? 0U : 0xCCU) &&
                                name[2U] == 0xCCU &&
                                list.resource_head_token ==
                                    (found ? root + 0x10U : 0U) &&
                                list.next_resource_head_token == root &&
                                list.selected_resource_token == 0xCAFEBABEU &&
                                openswd3::test::inventory_data(list) == storage,
                            "resource row preserves raw category masks, short-circuit modes, signed scan values and stale occurrence-zero outputs"
                        );
                    }
                }
            }
        }
    }

    struct Capacity {
        u16 threshold;
        u16 available;
        bool blocked;
    };

    constexpr std::array<Capacity, 9> capacities{{
        {0U, 0U, false},
        {1U, 0U, true},
        {1U, 1U, false},
        {0x3FFFU, 0x3FFFU, false},
        {0x3FFFU, 0x7FFFU, false},
        {0U, 0x8000U, true},
        {1U, 0xFFFFU, true},
        {0x3FFFU, 0x8000U, true},
        {0x3FFFU, 0xFFFFU, true},
    }};
    for (const auto capacity : capacities) {
        for (const bool enabled : {false, true}) {
            auto list = make_list();
            openswd3::test::inventory_word(
                openswd3::test::inventory_node(list, 1U), 0x4CU
            ) = static_cast<u16>(capacity.threshold | (enabled ? 0x4000U : 0U));
            LegacyBattleGroupAConfigurationState configuration;
            configuration.actor_record_token = 0x00600000U;
            configuration.actor_record[2U] = 0xAABB0000U | capacity.available;
            std::array<u8, 20> name{};
            u16 flags = 0xFFFFU;
            u16 quantity = 0xBEEFU;
            const auto result = query_legacy_battle_actor_resource_list(
                &list, &configuration, 0U, 1U, name, flags, quantity
            );
            test.expect_true(
                result.status == LegacyBattleActorListQueryStatus::completed &&
                    result.found && quantity == 1U &&
                    flags == (enabled && capacity.blocked ? 0xC000U : 0x8000U),
                "resource capacity gate compares the masked threshold with the signed low word and preserves equality"
            );
        }
    }

    for (const u32 occurrence : {0U, 1U, 2U, 0x10000U, 0xFFFFFFFFU}) {
        auto list = make_list();
        openswd3::test::inventory_node(list, 1U).legacy_next_token =
            root + 0x20U;
        openswd3::test::append_inventory_item(
            list,
            {
                .token = root + 0x20U,
                .tertiary_quantity = 2,
                .name = "S",
                .category_mask = 0x10U,
                .mode_flags = 1U,
            }
        );
        std::array<u8, 20> name{};
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, occurrence, name, flags, quantity
        );
        const bool found = occurrence == 1U || occurrence == 2U;
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found == found && flags == (found ? 0x8000U : 0U) &&
                quantity == (found ? occurrence : 0xABCDU) &&
                list.resource_head_token ==
                    (found ? root + occurrence * 0x10U : 0U),
            "resource row uses the complete DWORD ordinal and resets only flags on exhaustion"
        );
    }

    for (const std::string source : {
             std::string{},
             std::string("AB\0ignored", 10U),
             std::string(19U, 'n'),
             std::string(20U, 'm'),
         }) {
        auto list = make_list();
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, 1U), source
        );
        std::array<u8, 20> name;
        name.fill(0xCCU);
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 1U, name, flags, quantity
        );
        const auto terminator = source.find('\0');
        const auto length =
            terminator == std::string::npos ? source.size() : terminator;
        const bool complete = length < name.size();
        test.expect_true(
            result.status ==
                    (complete ? LegacyBattleActorListQueryStatus::completed
                              : LegacyBattleActorListQueryStatus::
                                    list_text_typed_stop) &&
                result.found == complete &&
                flags == (complete ? 0x8000U : 0U) &&
                quantity == (complete ? 1U : 0xABCDU) &&
                list.resource_head_token == root + 0x10U &&
                std::equal(
                    source.begin(), source.begin() + length, name.begin()
                ) &&
                (!complete || name[length] == 0U) &&
                (length + 1U >= name.size() || name[length + 1U] == 0xCCU),
            "resource row copies through the first NUL, preserves the old tail and retains the written prefix on a short destination"
        );
    }

    for (const bool enabled : {false, true}) {
        auto list = make_list();
        openswd3::test::inventory_word(
            openswd3::test::inventory_node(list, 1U), 0x4CU
        ) = enabled ? 0x4001U : 0U;
        std::array<u8, 20> name{};
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 1U, name, flags, quantity
        );
        test.expect_true(
            result.status ==
                    (enabled ? LegacyBattleActorListQueryStatus::
                                   list_owner_typed_stop
                             : LegacyBattleActorListQueryStatus::completed) &&
                result.found == !enabled && name[0U] == 'R' && quantity == 1U &&
                flags == 0x8000U,
            "missing capacity owner is accessed only after the name, quantity and visible flag have been published"
        );
    }

    {
        auto list = make_list();
        std::array<u8, 20> name{};
        u16 shared = 0xBEEFU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 1U, name, shared, shared
        );
        test.expect_true(
            result.found && shared == 0x8001U,
            "shared quantity and flags keep the quantity write before the high-byte visible bit"
        );
    }

    {
        auto list = make_list();
        openswd3::test::inventory_node(list, 1U).selected_count = 0xBEEFU;
        std::array<u8, 20> name{};
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            nullptr,
            0U,
            1U,
            name,
            openswd3::test::inventory_node(list, 1U).selected_count,
            quantity
        );
        test.expect_true(
            result.found && quantity == 1U &&
                openswd3::test::inventory_node(list, 1U).selected_count ==
                    0x8000U,
            "actual flags clear the aliased node quantity before qualification and publish visibility after output arithmetic"
        );
    }

    {
        auto list = make_list();
        openswd3::test::inventory_word(
            openswd3::test::inventory_node(list, 1U), 0x4CU
        ) = 0x4001U;
        std::array<u8, 20> name{};
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            nullptr,
            0U,
            1U,
            name,
            openswd3::test::inventory_word(
                openswd3::test::inventory_node(list, 1U), 0x4CU
            ),
            quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && quantity == 1U &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_node(list, 1U), 0x4CU
                ) == 0x8000U,
            "actual flags clear their aliased capacity gate before the conditional owner access"
        );
    }

    {
        auto list = make_list();
        openswd3::test::inventory_node(list, 1U).quantity_b = 0x4001;
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        std::array<u8, 20> name{};
        u16 flags = 0xBEEFU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list,
            &configuration,
            0U,
            1U,
            name,
            flags,
            openswd3::test::inventory_word(
                openswd3::test::inventory_node(list, 1U), 0x4CU
            )
        );
        test.expect_true(
            result.found && flags == 0xC000U &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_node(list, 1U), 0x4CU
                ) == 0x4001U,
            "quantity publication changes the actual capacity word before the bit-fourteen test"
        );
    }

    {
        auto list = make_list();
        openswd3::test::inventory_node(list, 0U).quantity_b = 7;
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, 1U), ""
        );
        const std::span<u8> name{
            reinterpret_cast<u8*>(&list.resource_head_token), sizeof(u32)
        };

        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 1U, name, flags, quantity
        );
        test.expect_true(
            result.found && quantity == 7U && flags == 0x8000U &&
                list.resource_head_token == root,
            "copying the terminator into the actual head changes the subsequent quantity and capacity lookup"
        );
    }

    {
        auto list = make_list();
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, 1U), "X"
        );
        u16 flags = 0xBEEFU;
        const std::span<u8> name{reinterpret_cast<u8*>(&flags), sizeof(flags)};
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 1U, name, flags, quantity
        );
        test.expect_true(
            result.found && flags == 0x8058U && quantity == 1U,
            "visible-bit publication preserves the actual low flag byte written by the name copy"
        );
    }

    {
        auto list = make_list();
        openswd3::test::set_inventory_name(
            openswd3::test::inventory_node(list, 1U), std::string(2U, '\xFF')
        );
        openswd3::test::inventory_word(
            openswd3::test::inventory_node(list, 1U), 0x4CU
        ) = 0x4001U;
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        const std::span<u8> name{
            reinterpret_cast<u8*>(&configuration.actor_record[2U]), sizeof(u32)
        };

        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, &configuration, 0U, 1U, name, flags, quantity
        );
        test.expect_true(
            result.found && flags == 0xC000U && quantity == 1U &&
                configuration.actor_record[2U] == 0xFFFFU,
            "the capacity comparison reads its signed configuration word after a real name alias has changed it"
        );
    }

    for (const u32 failure : {0U, 1U, 2U, 3U, 4U}) {
        auto list = make_list();
        if (failure == 1U) {
            list.next_resource_head_token = 0U;
        } else if (failure == 2U) {
            list.next_resource_head_token = 0xFEDCBA98U;
        } else if (failure == 3U) {
            openswd3::test::inventory_node(list, 0U).legacy_next_token =
                0xFEDCBA98U;
        } else if (failure == 4U) {
            openswd3::test::inventory_node(list, 0U).legacy_next_token = 0U;
        }

        std::array<u8, 20> name;
        name.fill(0xCCU);
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            failure == 0U ? nullptr : &list,
            nullptr,
            0U,
            1U,
            name,
            flags,
            quantity
        );
        const auto expected = failure == 0U
            ? LegacyBattleActorListQueryStatus::list_owner_typed_stop
            : (failure == 4U ? LegacyBattleActorListQueryStatus::completed
                             : LegacyBattleActorListQueryStatus::
                                   resource_node_typed_stop);
        test.expect_true(
            result.status == expected && !result.found &&
                flags == (failure == 0U ? 0xBEEFU : 0U) &&
                quantity == 0xABCDU && name[0U] == 0xCCU &&
                list.resource_head_token ==
                    (failure == 0U
                         ? 0xDEAD0000U
                         : (failure == 1U || failure == 4U ? 0U : 0xFEDCBA98U)),
            "resource row separates a valid exhausted root from missing list, zero root and unknown resource access"
        );
    }
}

void test_resource_list_item_count(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    using openswd3::compat::i16;
    constexpr u32 root = 0x87000000U;
    const auto make_list = [&inventories] {
        LegacyBattleActorListQueryState list;
        list.resource_head_token = 0xDEAD0000U;
        list.next_resource_head_token = root;
        list.selected_resource_token = 0xCAFEBABEU;
        inventories.assign(
            list,
            {
                {.token = root,
                 .next_token = root + 0x10U,
                 .name = {},
                 .category_mask = 0xFFFFFFFFU,
                 .mode_flags = 0x05U,
                 .capacity_gate_flags = 0x2000U},
                {.token = root + 0x10U,
                 .resource_id = 0x0300U,
                 .tertiary_quantity = 1,
                 .name = std::string(100U, 'n'),
                 .category_mask = 0x2000U,
                 .mode_flags = 0x01U},
            }
        );

        return list;
    };

    constexpr std::array<std::pair<u32, u32>, 11> categories{{
        {0U, 0x10U},
        {1U, 0x0CU},
        {2U, 0x1001U},
        {3U, 0x0800U},
        {4U, 0x2000U},
        {5U, 0x08000000U},
        {0x10U, 0x10U},
        {0x2000U, 0x2000U},
        {0x80000000U, 0x80000000U},
        {0x80000004U, 0x80000004U},
        {0xFFFFFFFFU, 0xFFFFFFFFU},
    }};
    constexpr std::array<u32, 11> masks{
        0U,
        0x10U,
        0x04U,
        0x08U,
        0x1000U,
        0x01U,
        0x0800U,
        0x2000U,
        0x08000000U,
        0x80000000U,
        0xFFFFFFFFU
    };

    constexpr std::array<u8, 7> modes{0U, 1U, 2U, 4U, 5U, 0x80U, 0xFFU};
    constexpr std::array<u8, 4> extra_bytes{0U, 0x20U, 0x80U, 0xFFU};
    for (const auto& [selector, mask] : categories) {
        for (const u32 node_mask : masks) {
            for (const u8 mode : modes) {
                for (const bool positive : {false, true}) {
                    for (const u8 extra : extra_bytes) {
                        auto list = make_list();
                        auto& node = openswd3::test::inventory_node(list, 1U);
                        openswd3::test::inventory_category(node) = node_mask;
                        node.definition_snapshot[0x46U - 0x0CU] = mode;
                        node.quantity_b = positive ? 1 : 0;
                        openswd3::test::inventory_word(node, 0x4CU) =
                            static_cast<u16>(static_cast<u16>(extra) << 8U);
                        const auto* storage =
                            openswd3::test::inventory_data(list);
                        const auto original_definition =
                            node.definition_snapshot;
                        std::array<u16, 3> words{0xCAFEU, 0xBEEFU, 0xA55AU};
                        const bool ordinary = (node_mask & mask) != 0U &&
                            (mode & 0x05U) != 0U && positive;
                        const bool additional =
                            mask == 0x2000U && (extra & 0x20U) != 0U;
                        const u16 expected = static_cast<u16>(
                            0xBEEFU + static_cast<u32>(ordinary) +
                            static_cast<u32>(additional)
                        );
                        const auto status =
                            count_legacy_battle_actor_resource_list(
                                &list, selector, words[1U]
                            );
                        test.expect_true(
                            status ==
                                    LegacyBattleActorListQueryStatus::
                                        completed &&
                                words ==
                                    std::array<u16, 3>{
                                        0xCAFEU, expected, 0xA55AU
                                    } &&
                                list.resource_head_token == 0U &&
                                list.next_resource_head_token == root &&
                                list.selected_resource_token == 0xCAFEBABEU &&
                                openswd3::test::inventory_data(list) ==
                                    storage &&
                                openswd3::test::inventory_nodes(list).size() ==
                                    2U &&
                                node.item_id == 0x0300U &&
                                node.definition_snapshot ==
                                    original_definition &&
                                openswd3::test::inventory_category(node) ==
                                    node_mask &&
                                node.definition_snapshot[0x46U - 0x0CU] ==
                                    mode &&
                                openswd3::test::inventory_word(node, 0x4CU) ==
                                    static_cast<u16>(
                                        static_cast<u16>(extra) << 8U
                                    ),
                            "resource count preserves the real word, full masks, independent extra increments and resource ownership"
                        );
                    }
                }
            }
        }
    }

    struct QuantityVector {
        u16 tertiary;
        u16 primary;
        u16 secondary;
        bool positive;
    };

    constexpr std::array<QuantityVector, 10> quantities{{
        {1U, 0U, 0U, true},
        {0U, 0U, 0U, false},
        {0xFFFFU, 0U, 0U, false},
        {0x7FFFU, 0xFFFFU, 0x7FFFU, false},
        {0x7FFFU, 0U, 0x7FFFU, true},
        {0x8000U, 0U, 0x7FFFU, false},
        {0x7FFFU, 0x8000U, 2U, true},
        {0x7FFFU, 0x8001U, 2U, false},
        {0x7FFFU, 0x8002U, 2U, false},
        {0x8000U, 0U, 0x8000U, false},
    }};
    constexpr std::array<u16, 5> initial_counts{
        0U, 0xFFFFU, 0xFFFEU, 0x8000U, 0xBEEFU
    };

    for (const auto& quantity : quantities) {
        for (const u16 initial : initial_counts) {
            for (const u32 selector : {0U, 4U}) {
                for (const bool extra : {false, true}) {
                    auto list = make_list();
                    auto& node = openswd3::test::inventory_node(list, 1U);
                    openswd3::test::inventory_category(node) = 0xFFFFFFFFU;
                    node.quantity_b = quantity.tertiary;
                    node.selected_count = quantity.primary;
                    node.quantity_a = quantity.secondary;
                    openswd3::test::inventory_word(node, 0x4CU) =
                        extra ? 0x2000U : 0U;
                    u16 count = initial;
                    const u16 expected = static_cast<u16>(
                        initial + static_cast<u32>(quantity.positive) +
                        static_cast<u32>(selector == 4U && extra)
                    );
                    const auto status = count_legacy_battle_actor_resource_list(
                        &list, selector, count
                    );
                    test.expect_true(
                        status == LegacyBattleActorListQueryStatus::completed &&
                            count == expected &&
                            list.resource_head_token == 0U &&
                            node.selected_count == quantity.primary &&
                            std::bit_cast<u16>(node.quantity_a) ==
                                quantity.secondary &&
                            std::bit_cast<u16>(node.quantity_b) ==
                                quantity.tertiary,
                        "resource count uses signed quantity words and an unsigned primary word before each wrapping increment"
                    );
                }
            }
        }
    }

    constexpr std::array<u16, 7> alias_expected{
        0xBEF1U, 2U, 1U, 2U, 2U, 0x2001U, 0x4000U
    };

    for (u32 scenario = 0U; scenario < alias_expected.size(); ++scenario) {
        auto list = make_list();
        openswd3::test::inventory_node(list, 1U).legacy_next_token =
            root + 0x20U;
        openswd3::test::inventory_node(list, 1U).quantity_a = 1;
        openswd3::test::inventory_node(list, 1U).quantity_b = 0;
        openswd3::test::append_inventory_item(
            list,
            {
                .token = root + 0x20U,
                .secondary_quantity = 1,
                .name = {},
                .category_mask = 0x2000U,
                .mode_flags = 0x01U,
            }
        );
        auto& first = openswd3::test::inventory_node(list, 1U);
        auto& second = openswd3::test::inventory_node(list, 2U);
        u16* count = &openswd3::test::inventory_node(list, 0U).selected_count;
        switch (scenario) {
        case 0U:
            *count = 0xBEEFU;
            break;

        case 1U:
            count = reinterpret_cast<u16*>(&first.quantity_a);
            openswd3::test::inventory_category(second) = 0U;
            break;

        case 2U:
            count = &second.selected_count;
            break;

        case 3U:
            second.quantity_a = 0;
            count = reinterpret_cast<u16*>(&second.quantity_a);
            break;

        case 4U:
            second.quantity_a = 0;
            count = reinterpret_cast<u16*>(&second.quantity_b);
            break;

        default:
            openswd3::test::inventory_word(first, 0x4CU) =
                scenario == 5U ? 0x1FFFU : 0x3FFFU;
            count = &openswd3::test::inventory_word(first, 0x4CU);
            openswd3::test::inventory_category(second) = 0U;
            break;
        }

        const auto* storage = openswd3::test::inventory_data(list);
        const auto status =
            count_legacy_battle_actor_resource_list(&list, 4U, *count);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                *count == alias_expected[scenario] &&
                list.resource_head_token == 0U &&
                list.next_resource_head_token == root &&
                list.selected_resource_token == 0xCAFEBABEU &&
                openswd3::test::inventory_data(list) == storage &&
                openswd3::test::inventory_nodes(list).size() == 3U,
            "direct count increments change later quantity and bit-thirteen reads through actual word aliases"
        );
    }

    for (u32 scenario = 0U; scenario < 8U; ++scenario) {
        auto list = make_list();
        auto* owner = &list;
        u16 count = 0xBEEFU;
        u16 expected_count = count;
        u32 expected_head = 0xFEDCBA98U;
        auto expected_status =
            LegacyBattleActorListQueryStatus::resource_owner_typed_stop;
        switch (scenario) {
        case 0U:
            owner = nullptr;
            expected_head = 0xDEAD0000U;
            expected_status =
                LegacyBattleActorListQueryStatus::list_owner_typed_stop;
            break;

        case 1U:
            list.next_resource_head_token = 0U;
            expected_head = 0U;
            break;

        case 2U:
            list.next_resource_head_token = expected_head;
            break;

        case 3U:
            openswd3::test::inventory_node(list, 0U).legacy_next_token =
                expected_head;
            break;

        case 4U:
            openswd3::test::inventory_node(list, 1U).legacy_next_token =
                expected_head;
            expected_count = 0xBEF0U;
            break;

        case 5U:
            openswd3::test::inventory_node(list, 1U).legacy_next_token =
                expected_head;
            openswd3::test::inventory_word(
                openswd3::test::inventory_node(list, 1U), 0x4CU
            ) = 0x2000U;
            expected_count = 0xBEF1U;
            break;

        case 6U:
            openswd3::test::inventory_node(list, 1U).legacy_next_token =
                expected_head;
            openswd3::test::inventory_category(
                openswd3::test::inventory_node(list, 1U)
            ) = 0U;
            break;

        default:
            openswd3::test::inventory_node(list, 0U).legacy_next_token = 0U;
            expected_head = 0U;
            expected_status = LegacyBattleActorListQueryStatus::completed;
            break;
        }

        const u32 next_root = list.next_resource_head_token;
        const auto* storage = openswd3::test::inventory_data(list);
        const auto status =
            count_legacy_battle_actor_resource_list(owner, 4U, count);
        test.expect_true(
            status == expected_status && count == expected_count &&
                list.resource_head_token == expected_head &&
                list.next_resource_head_token == next_root &&
                list.selected_resource_token == 0xCAFEBABEU &&
                openswd3::test::inventory_data(list) == storage &&
                openswd3::test::inventory_nodes(list).size() == 2U,
            "resource count separates actual empty chains from invalid roots and preserves every published failure prefix"
        );
    }

    {
        auto list = make_list();
        openswd3::test::inventory_nodes(list).resize(1U);
        for (u32 index = 1U; index <= 512U; ++index) {
            openswd3::test::append_inventory_item(
                list,
                {
                    .token = root + index * 0x10U,
                    .next_token =
                        index == 512U ? 0U : root + (index + 1U) * 0x10U,
                    .resource_id =
                        static_cast<u16>(index % 2U == 0U ? 0x0300U : 0xFFFFU),
                    .tertiary_quantity = 1,
                    .name = std::string(100U, 'n'),
                    .category_mask = 0x2000U,
                    .mode_flags = 0x01U,
                    .capacity_gate_flags = 0x2000U,
                }
            );
        }

        const auto* storage = openswd3::test::inventory_data(list);
        u16 count = 0xFFFEU;
        const auto status =
            count_legacy_battle_actor_resource_list(&list, 4U, count);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                count == 0x03FEU && list.resource_head_token == 0U &&
                list.next_resource_head_token == root &&
                list.selected_resource_token == 0xCAFEBABEU &&
                openswd3::test::inventory_data(list) == storage &&
                openswd3::test::inventory_nodes(list).size() == 513U,
            "resource count traverses every finite node, allows two increments and wraps across the entire chain"
        );
    }
}

void test_mode_resource_quantity_count(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    using namespace openswd3::battle;
    using openswd3::compat::i16;
    const std::array<std::pair<u32, bool>, 7> categories{{
        {0U, false},
        {0x80U, false},
        {0x07FFFFFFU, false},
        {0x08000000U, true},
        {0x08000001U, true},
        {0x88000000U, true},
        {0xFFFFFFFFU, true},
    }};
    const std::array<std::pair<u8, bool>, 7> flags{{
        {0U, false},
        {1U, true},
        {2U, false},
        {4U, true},
        {5U, true},
        {0x80U, false},
        {0xFFU, true},
    }};
    for (const auto& category : categories) {
        for (const u16 identifier :
             std::array<u16, 4>{0U, 0x0111U, 0x0300U, 0xFFFFU}) {
            for (const auto& flag : flags) {
                LegacyBattleActorListQueryState list;
                list.resource_head_token = 0xDEAD0000U;
                list.next_resource_head_token = 0x76000000U;
                list.selected_resource_token = 0xCAFEBABEU;
                inventories.assign(
                    list,
                    {
                        {.token = 0x76000000U,
                         .next_token = 0x76000010U,
                         .name = "root"},
                        {.token = 0x76000010U,
                         .resource_id = identifier,
                         .primary_quantity = 0x4321U,
                         .secondary_quantity = -1,
                         .tertiary_quantity = 2,
                         .name = std::string(100U, 'n'),
                         .category_mask = category.first,
                         .mode_flags = flag.first},
                    }
                );

                const auto* original_storage =
                    openswd3::test::inventory_data(list);
                const auto definition_before =
                    openswd3::test::inventory_node(list, 1U)
                        .definition_snapshot;
                std::array<u16, 3> output{0xCAFEU, 0xBEEFU, 0xA55AU};
                const auto status =
                    count_legacy_battle_actor_mode_resources(&list, output[1U]);
                const bool included =
                    category.second && identifier != 0x0300U && flag.second;
                test.expect_true(
                    status == LegacyBattleActorListQueryStatus::completed &&
                        output[1U] == (included ? 1U : 0U) &&
                        output.front() == 0xCAFEU && output.back() == 0xA55AU &&
                        list.resource_head_token == 0U &&
                        list.next_resource_head_token == 0x76000000U &&
                        list.selected_resource_token == 0xCAFEBABEU &&
                        openswd3::test::inventory_data(list) ==
                            original_storage &&
                        openswd3::test::inventory_nodes(list).size() == 2U &&
                        openswd3::test::inventory_node(list, 0U)
                                .legacy_next_token == 0x76000010U &&
                        openswd3::test::inventory_node(list, 1U)
                                .selected_count == 0x4321U &&
                        openswd3::test::inventory_node(list, 1U).quantity_a ==
                            0xFFFFU &&
                        openswd3::test::inventory_node(list, 1U).quantity_b ==
                            2 &&
                        openswd3::test::inventory_node(list, 1U)
                                .definition_snapshot == definition_before &&
                        openswd3::test::inventory_category(
                            openswd3::test::inventory_node(list, 1U)
                        ) == category.first &&
                        openswd3::test::inventory_node(list, 1U)
                                .definition_snapshot[0x46U - 0x0CU] ==
                            flag.first,
                    "mode count clears only its actual WORD, filters category/id/BYTE gates and leaves source resources unchanged"
                );
            }
        }
    }

    const std::array<std::array<u16, 4>, 7> quantities{{
        {0U, 0U, 0U, 1U},
        {1U, 2U, 3U, 4U},
        {0xFFFFU, 1U, 0U, 1U},
        {0x7FFFU, 1U, 0x8000U, 0x8001U},
        {0x8000U, 0U, 0x8000U, 0x8001U},
        {0xFFFFU, 0xFFFFU, 0xFFFEU, 0xFFFFU},
        {0x8000U, 0x8000U, 0U, 1U},
    }};
    for (const auto& quantity : quantities) {
        for (const bool append_tail : {false, true}) {
            LegacyBattleActorListQueryState list;
            list.next_resource_head_token = 0x76000000U;
            inventories.assign(
                list,
                {
                    {.token = 0x76000000U,
                     .next_token = 0x76000010U,
                     .name = {}},
                    {.token = 0x76000010U,
                     .next_token = append_tail ? 0x76000020U : 0U,
                     .resource_id = 0x0222U,
                     .secondary_quantity = std::bit_cast<i16>(quantity[0U]),
                     .tertiary_quantity = std::bit_cast<i16>(quantity[1U]),
                     .name = {},
                     .category_mask = 0x08000000U,
                     .mode_flags = 4U},
                }
            );

            if (append_tail) {
                openswd3::test::append_inventory_item(
                    list,
                    {
                        .token = 0x76000020U,
                        .resource_id = 0x0333U,
                        .secondary_quantity = -1,
                        .tertiary_quantity = 2,
                        .name = {},
                        .category_mask = 0x88000000U,
                        .mode_flags = 5U,
                    }
                );
            }

            std::array<u16, 3> output{0xCAFEU, 0xBEEFU, 0xA55AU};
            const auto status =
                count_legacy_battle_actor_mode_resources(&list, output[1U]);
            test.expect_true(
                status == LegacyBattleActorListQueryStatus::completed &&
                    output[1U] == quantity[append_tail ? 3U : 2U] &&
                    output.front() == 0xCAFEU && output.back() == 0xA55AU &&
                    list.resource_head_token == 0U &&
                    openswd3::test::inventory_node(list, 1U).quantity_a ==
                        quantity[0U] &&
                    openswd3::test::inventory_node(list, 1U).quantity_b ==
                        quantity[1U],
                "mode quantities preserve node and cross-node WORD wrap without signed rejection or writes to adjacent output words"
            );
        }
    }

    const std::array<u16, 5> alias_totals{10U, 10U, 6U, 21U, 21U};
    for (std::size_t alias = 0U; alias < alias_totals.size(); ++alias) {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0xCAFEBABEU;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U,
                 .next_token = 0x76000010U,
                 .secondary_quantity = 9,
                 .name = {}},
                {.token = 0x76000010U,
                 .next_token = 0x76000020U,
                 .resource_id = 0x0222U,
                 .secondary_quantity = static_cast<i16>(alias == 1U ? 5 : 3),
                 .tertiary_quantity =
                     static_cast<i16>(alias == 1U ? 7 : (alias == 2U ? 8 : 4)),
                 .name = {},
                 .category_mask = 0x08000000U,
                 .mode_flags = 1U},
                {.token = 0x76000020U,
                 .resource_id = 0x0333U,
                 .secondary_quantity =
                     static_cast<i16>(alias == 3U ? 5 : (alias == 4U ? 7 : 1)),
                 .tertiary_quantity =
                     static_cast<i16>(alias == 3U ? 7 : (alias == 4U ? 5 : 2)),
                 .name = {},
                 .category_mask = 0x08000000U,
                 .mode_flags = 4U},
            }
        );

        auto& output_field = alias == 0U
            ? openswd3::test::inventory_node(list, 0U).quantity_a
            : (alias == 1U
                   ? openswd3::test::inventory_node(list, 1U).quantity_a
                   : (alias == 2U
                          ? openswd3::test::inventory_node(list, 1U).quantity_b
                          : (alias == 3U
                                 ? openswd3::test::inventory_node(list, 2U)
                                       .quantity_a
                                 : openswd3::test::inventory_node(list, 2U)
                                       .quantity_b)));
        auto& output_word = reinterpret_cast<u16&>(output_field);
        const auto* original_storage = openswd3::test::inventory_data(list);
        const auto status =
            count_legacy_battle_actor_mode_resources(&list, output_word);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                output_word == alias_totals[alias] &&
                list.resource_head_token == 0U &&
                list.next_resource_head_token == 0x76000000U &&
                list.selected_resource_token == 0xCAFEBABEU &&
                openswd3::test::inventory_data(list) == original_storage &&
                openswd3::test::inventory_nodes(list).size() == 3U &&
                openswd3::test::inventory_node(list, 0U).legacy_next_token ==
                    0x76000010U &&
                openswd3::test::inventory_node(list, 1U).legacy_next_token ==
                    0x76000020U &&
                openswd3::test::inventory_node(list, 2U).legacy_next_token ==
                    0U,
            "a real signed/unsigned WORD alias observes clear and each partial addition before subsequent quantity reads"
        );
    }

    for (u32 scenario = 0U; scenario < 9U; ++scenario) {
        LegacyBattleActorListQueryState list;
        list.resource_head_token = 0xDEAD0000U;
        list.next_resource_head_token =
            scenario == 1U ? 0xFEDCBA98U : 0x76000000U;
        list.selected_resource_token = 0xCAFEBABEU;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U,
                 .next_token = scenario == 2U ? 0xFEDCBA98U : 0x76000010U,
                 .name = {}},
                {.token = 0x76000010U,
                 .next_token = 0xFEDCBA98U,
                 .resource_id =
                     static_cast<u16>(scenario == 5U ? 0x0300U : 0x0222U),
                 .secondary_quantity = 3,
                 .tertiary_quantity = 4,
                 .name = {},
                 .category_mask = scenario == 4U ? 0U : 0x08000000U,
                 .mode_flags = static_cast<u8>(scenario == 6U ? 0U : 1U)},
            }
        );

        if (scenario == 7U) {
            openswd3::test::inventory_node(list, 0U).legacy_next_token = 0U;
        }

        if (scenario == 8U) {
            list.next_resource_head_token = 0U;
        }

        u16 quantity = 0xBEEFU;
        const auto status = count_legacy_battle_actor_mode_resources(
            scenario == 0U ? nullptr : &list, quantity
        );
        const auto expected_status = scenario == 0U
            ? LegacyBattleActorListQueryStatus::list_owner_typed_stop
            : (scenario == 7U ? LegacyBattleActorListQueryStatus::completed
                              : LegacyBattleActorListQueryStatus::
                                    resource_node_typed_stop);
        const u16 expected_quantity =
            scenario == 0U ? 0xBEEFU : (scenario == 3U ? 7U : 0U);
        const u32 expected_head =
            scenario == 0U ? 0xDEAD0000U : (scenario >= 7U ? 0U : 0xFEDCBA98U);
        test.expect_true(
            status == expected_status && quantity == expected_quantity &&
                list.resource_head_token == expected_head &&
                list.selected_resource_token == 0xCAFEBABEU &&
                openswd3::test::inventory_nodes(list).size() == 2U,
            "mode count distinguishes an empty real root from missing objects and retains the published head and actual accumulated WORD at each failure"
        );
    }

    {
        LegacyBattleActorListQueryState list;
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(list, {});
        openswd3::test::append_inventory_item(
            list,
            {
                .token = 0x76000000U,
                .next_token = 0x76000010U,
                .name = {},
            }
        );
        for (u32 node_index = 1U; node_index <= 512U; ++node_index) {
            openswd3::test::append_inventory_item(
                list,
                {
                    .token = 0x76000000U + node_index * 0x10U,
                    .next_token = node_index == 512U
                        ? 0U
                        : 0x76000000U + (node_index + 1U) * 0x10U,
                    .resource_id = 0x0222U,
                    .secondary_quantity = 3,
                    .tertiary_quantity = 4,
                    .name = {},
                    .category_mask = 0x08000000U,
                    .mode_flags = 1U,
                }
            );
        }

        const auto* original_storage = openswd3::test::inventory_data(list);
        u16 quantity = 0xBEEFU;
        const auto status =
            count_legacy_battle_actor_mode_resources(&list, quantity);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                quantity == 3584U && list.resource_head_token == 0U &&
                openswd3::test::inventory_nodes(list).size() == 513U &&
                openswd3::test::inventory_data(list) == original_storage,
            "mode count traverses a long finite actual resource chain without truncating its sum or transferring ownership"
        );
    }
}

}  // namespace

void test_battle_actor_list_query(openswd3::test::Context& test) {
    openswd3::test::LegacyBattleInventoryFixture inventories;
    test_shared_inventory_release(test);
    test_resource_selection_outputs(test);
    test_resource_selection_capacity_prefixes(test);
    test_actor_resource_release_prefixes(test);
    test_resource_head_publication(test);
    test_flagged_resource_display(test);
    test_mode_resource_display(test);
    test_resource_list_display(test);
    test_resource_list_item_count(test);
    test_mode_resource_quantity_count(test);
    using namespace openswd3::battle;
    const std::array<openswd3::compat::u16, 4> table{10U, 20U, 30U, 40U};

    LegacyBattleGroupAActionExecutionState actor;
    actor.next_list_index = 0x71000000U;
    LegacyBattleActorListQueryState list;
    openswd3::test::bind_party_list(inventories, list, 0x71000000U);
    openswd3::test::party_root(list).legacy_next_token = 0x72000000U;
    openswd3::test::assign_party_nodes(
        list,
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
        }
    );

    {
        QueryPort port;
        port.index = 2U;
        LegacyBattleMonProfile profile{};
        std::array<u8, 20> text{};
        u32 value = 0xAABB7777U;
        const auto result = query_legacy_battle_actor_list(
            &actor, &list, port, 0U, 0U, 1U, text, value, profile, table
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                actor.current_list_index == 0x72000100U &&
                std::string(reinterpret_cast<const char*>(text.data())) ==
                    "first" &&
                value == 0xAABB03E8U && result.display_frame == 30U &&
                port.last_profile == 7U,
            "list query commits the index then applies ordered category, type, and value overrides"
        );
    }

    {
        QueryPort port;
        LegacyBattleMonProfile profile{};
        std::array<u8, 20> text{};
        u32 value = 3U;
        const auto result = query_legacy_battle_actor_list(
            &actor, &list, port, 0U, 0U, 0U, text, value, profile, table
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.display_frame == 10U && value == 3U &&
                actor.current_list_index ==
                    openswd3::test::party_root(list).legacy_next_token &&
                port.read_calls != 0U,
            "zero occurrence selects the first mismatching node before a match increments the counter"
        );
    }

    {
        LegacyBattleActorListQueryState type31;
        openswd3::test::bind_party_list(
            inventories, type31, openswd3::test::party_root(list).legacy_token
        );
        openswd3::test::party_root(type31).legacy_next_token = 0x73000000U;
        openswd3::test::assign_party_nodes(
            type31,
            {{.token = 0x73000000U,
              .category_flags = 0x0CU,
              .mode_flags = 0x04U,
              .type = 0x1FU,
              .profile_id = 9U,
              .value_flags = 0xFFFFU,
              .text = "profile"}}
        );
        QueryPort port;
        port.index = 1U;
        LegacyBattleMonProfile profile{};
        std::array<u8, 20> text{};
        u32 value = 0xBEEFAAAAU;
        const auto result = query_legacy_battle_actor_list(
            &actor, &type31, port, 1U, 1U, 1U, text, value, profile, table
        );
        test.expect_true(
            value == 0xBEEF0001U && result.display_frame == 20U &&
                port.last_profile == 9U,
            "type thirty-one forces output one after all value flag overrides"
        );
    }

    {
        actor.next_list_index = openswd3::test::party_root(list).legacy_token;
        u8 count = 0xFFU;
        const auto status =
            count_legacy_battle_actor_list(&actor, &list, 0U, 0U, count);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                count == 1U && actor.current_list_index == 0U,
            "list count preserves the caller byte then wraps once per matching node"
        );
    }

    {
        actor.next_list_index = openswd3::test::party_root(list).legacy_token;
        u8 count = 9U;
        const auto status =
            count_legacy_battle_actor_list(&actor, &list, 0U, 7U, count);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                count == 9U && actor.current_list_index == 0U,
            "every non-twenty-eight type selector still tests only type thirty-one"
        );
    }

    {
        LegacyBattleActorListQueryState counted;
        openswd3::test::bind_party_list(inventories, counted, 0x10U);
        openswd3::test::party_root(counted).legacy_next_token = 1U;
        constexpr std::array<u16, 9> types{
            0x1AU, 0x1BU, 0x1EU, 0x1CU, 0x1DU, 0x1FU, 0x20U, 0x1CU, 0x1DU
        };
        constexpr std::array<u8, 9> modes{1U, 4U, 5U, 0U, 2U, 1U, 4U, 1U, 4U};
        for (u32 index = 0U; index < types.size(); ++index) {
            openswd3::test::append_party_item(
                counted,
                {
                    .token = index + 1U,
                    .next_token = index + 1U == types.size() ? 0U : index + 2U,
                    .category_flags = 0x1018U,
                    .mode_flags = modes[index],
                    .type = types[index],
                }
            );
        }

        for (const u32 category : {0U, 1U, 2U, 0x10U, 0x20U}) {
            for (const u32 type : {0U, 1U, 0x1CU, 0x99U}) {
                LegacyBattleGroupAActionExecutionState counted_actor;
                counted_actor.next_list_index =
                    openswd3::test::party_root(counted).legacy_token;
                u8 count = 7U;
                const auto status = count_legacy_battle_actor_list(
                    &counted_actor, &counted, category, type, count
                );
                const u8 expected = category == 0x20U ? 7U
                    : type == 0U || type == 0x1CU     ? 11U
                                                      : 8U;
                test.expect_true(
                    status == LegacyBattleActorListQueryStatus::completed &&
                        count == expected &&
                        counted_actor.current_list_index == 0U,
                    "list count maps selectors and filters mode bits and inclusive type limits"
                );
            }
        }
    }

    {
        LegacyBattleActorListQueryState counted;
        openswd3::test::bind_party_list(inventories, counted, 0x10U);
        LegacyBattleGroupAActionExecutionState counted_actor;
        counted_actor.next_list_index =
            openswd3::test::party_root(counted).legacy_token;
        u8 count = 7U;
        const auto empty = count_legacy_battle_actor_list(
            &counted_actor, &counted, 0U, 0U, count
        );
        test.expect_true(
            empty == LegacyBattleActorListQueryStatus::completed &&
                count == 7U && counted_actor.current_list_index == 0U,
            "empty list count preserves the incoming byte and publishes the null tail"
        );
        openswd3::test::party_root(counted).legacy_next_token = 0x100U;
        openswd3::test::append_party_item(
            counted,
            {
                .token = 0x100U,
                .next_token = 0x200U,
                .category_flags = 0x10U,
                .mode_flags = 1U,
                .type = 0x1CU,
            }
        );
        const auto missing = count_legacy_battle_actor_list(
            &counted_actor, &counted, 0U, 0U, count
        );
        test.expect_true(
            missing == LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                count == 8U && counted_actor.current_list_index == 0x200U,
            "missing count node preserves prior increments and the published failing cursor"
        );
        openswd3::test::append_party_item(
            counted,
            {
                .token = 0x200U,
                .category_flags = 0x10U,
                .mode_flags = 1U,
                .type = 0x1CU,
            }
        );
        counted_actor.next_list_index = 0x100U;
        count = 7U;
        const auto resumed = count_legacy_battle_actor_list(
            &counted_actor, &counted, 0U, 0U, count
        );
        test.expect_true(
            resumed == LegacyBattleActorListQueryStatus::completed &&
                count == 8U && counted_actor.current_list_index == 0U,
            "count starts after a live node cursor instead of requiring the list owner"
        );
        counted_actor.next_list_index = 0x99U;
        const auto owner = count_legacy_battle_actor_list(
            &counted_actor, nullptr, 0U, 0U, count
        );
        test.expect_true(
            owner == LegacyBattleActorListQueryStatus::list_owner_typed_stop &&
                count == 8U && counted_actor.current_list_index == 0x99U,
            "count owner failure follows cursor publication without changing the byte"
        );
    }

    {
        LegacyBattleActorListQueryState counted;
        openswd3::test::bind_party_list(inventories, counted, 0x10U);
        openswd3::test::party_root(counted).legacy_next_token = 0x100U;
        openswd3::test::append_party_item(
            counted,
            {
                .token = 0x100U,
                .next_token = 0xDEADU,
                .category_flags = 0x10U,
                .mode_flags = 1U,
                .type = 0x1CU,
            }
        );
        openswd3::test::append_party_item(
            counted, {.token = 0x101U, .next_token = 0U}
        );
        LegacyBattleGroupAActionExecutionState counted_actor;
        counted_actor.next_list_index =
            openswd3::test::party_root(counted).legacy_token;
        auto& count = *reinterpret_cast<u8*>(&counted_actor.current_list_index);
        const auto status = count_legacy_battle_actor_list(
            &counted_actor, &counted, 0U, 0U, count
        );
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                counted_actor.current_list_index == 0U,
            "aliased count increment redirects the live cursor before reading its next link"
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
        const auto status = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, nullptr
        );
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                (configuration.actor_record[1U] >> 16U) == 0U &&
                list.primary_required == 0U,
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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 4U,
                 .secondary_quantity = 2,
                 .name = {}},
            }
        );
        const auto status = execute_legacy_battle_actor_list_action(
            &list, &item_effect, &configuration, &workspace
        );
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                list.resource_head_token == 0x76000010U &&
                list.selected_resource_token == 0U &&
                list.primary_required == 0U &&
                openswd3::test::inventory_node(list, 1U).quantity_a == 1 &&
                workspace.tail_words[2U] == 4U,
            "selected resource path consumes the owned node then clears selection and required capacity before refreshing"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 0xA5A50003U;
        list.secondary_required = 5U;
        const auto status =
            refresh_legacy_battle_actor_list_action(&list, &configuration);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                configuration.actor_record[2U] == 0xA5A50000U &&
                list.secondary_required == 0U,
            "secondary consumption preserves the adjacent word and clamps the wrapped negative result"
        );
    }

    {
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        list.primary_required = 17U;
        list.selected_resource_token = 0xDEADU;
        list.secondary_required = 0U;
        const auto status = execute_legacy_battle_actor_list_action(
            &list, &item_effect, nullptr, nullptr
        );
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                list.primary_required == 17U &&
                list.selected_resource_token == 0xDEADU &&
                list.secondary_required == 0U,
            "disabled primary consumption and zero secondary requirement leave the record unaccessed"
        );
    }

    {
        constexpr std::array<std::array<u16, 3>, 6> cases{{
            {5U, 7U, 0U},
            {10U, 3U, 7U},
            {0U, 0xFFFFU, 1U},
            {0x8000U, 1U, 0x7FFFU},
            {0x7FFFU, 0xFFFFU, 0U},
            {0xFFFFU, 0x8000U, 0x7FFFU},
        }};
        for (const auto& values : cases) {
            LegacyBattleActorListQueryState spending;
            spending.primary_required = values[1U];
            spending.secondary_required = values[1U];
            LegacyBattleGroupAItemEffectApplicationState effect;
            effect.mode_flags = 0x80U;
            LegacyBattleGroupAConfigurationState record;
            record.actor_record_token = 0x6000000U;
            record.actor_record[1U] =
                (static_cast<u32>(values[0U]) << 16U) | 0xBEEFU;
            record.actor_record[2U] = 0xCAFE0000U | values[0U];
            const auto status = execute_legacy_battle_actor_list_action(
                &spending, &effect, &record, nullptr
            );
            test.expect_true(
                status == LegacyBattleActorListQueryStatus::completed &&
                    record.actor_record[1U] ==
                        ((static_cast<u32>(values[2U]) << 16U) | 0xBEEFU) &&
                    record.actor_record[2U] == (0xCAFE0000U | values[2U]) &&
                    spending.primary_required == 0U &&
                    spending.secondary_required == 0U &&
                    effect.mode_flags == 0x80U,
                "action consumption wraps each word before signed clamping and preserves adjacent data"
            );
        }
    }

    {
        LegacyBattleActorListQueryState spending;
        spending.next_resource_head_token = 0x100U;
        spending.selected_resource_token = 0x200U;
        spending.primary_required = 9U;
        spending.secondary_required = 3U;
        inventories.assign(
            spending,
            {
                {.token = 0x100U, .next_token = 0x200U, .name = {}},
                {.token = 0x200U,
                 .resource_id = 4U,
                 .secondary_quantity = 2,
                 .name = {}},
            }
        );
        LegacyBattleGroupAItemEffectApplicationState effect;
        effect.mode_flags = 0x80U;
        LegacyBattleGroupAWorkspaceState workspace;
        const auto status = execute_legacy_battle_actor_list_action(
            &spending, &effect, nullptr, &workspace
        );
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::list_owner_typed_stop &&
                openswd3::test::inventory_node(spending, 1U).quantity_a == 1 &&
                spending.selected_resource_token == 0U &&
                spending.primary_required == 0U &&
                spending.secondary_required == 3U &&
                workspace.tail_words[2U] == 4U,
            "secondary record failure preserves the completed resource consumption and primary cleanup"
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
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 2U,
                 .primary_quantity = 1U,
                 .tertiary_quantity = 4,
                 .name = {},
                 .category_mask = 0x10U,
                 .derived_words_40 = {11U, 12U, 13U},
                 .gate_word_48 = 0x0200U,
                 .profile_id_4a = 7U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x8003U,
                 .alternate_profile_id_54 = 9U},
            }
        );
        SelectionPort port;
        u32 runtime_word = 0xAABBCCDDU;
        u16 selection_mode = 0xFFFFU;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            &configuration,
            &final_state,
            &item,
            &workspace,
            port,
            port,
            0U,
            1U,
            runtime_word,
            selection_mode
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selected && selection_mode == 1U && runtime_word == 0U &&
                port.profile_ids[0U] == 9U &&
                port.diagnostics == std::vector<u16>{2U} &&
                list.selected_resource_token == 0x76000010U &&
                list.primary_required == 3U && item.mode_flags == 0x40U &&
                item.derived_words[1U] == 11U &&
                final_state.profile_copy_latch == 1U &&
                openswd3::test::inventory_node(list, 1U).selected_count == 2U,
            "resource selection loads the alternate profile, applies gates, diagnoses zero runtime word and increments quantity"
        );
    }

    {
        LegacyBattleGroupAFinalProcessingState final_state;
        LegacyBattleGroupAItemEffectApplicationState item;
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .tertiary_quantity = 1,
                 .name = {},
                 .category_mask = 0x2000U,
                 .derived_word_30 = 0x1234U,
                 .profile_id_4a = 7U,
                 .capacity_gate_flags = 0x2000U},
            }
        );
        SelectionPort port;
        u32 runtime_word = 0xAABBCCDDU;
        u16 selection_mode = 0xFFFFU;
        const auto result = select_legacy_battle_actor_resource(
            &list,
            nullptr,
            &final_state,
            &item,
            &workspace,
            port,
            port,
            4U,
            1U,
            runtime_word,
            selection_mode
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selected && runtime_word == 0xAABBCCDDU &&
                selection_mode == 0U && port.profile_ids[0U] == 7U &&
                item.derived_words[0U] == 0x1234U &&
                openswd3::test::inventory_node(list, 1U).selected_count == 0U,
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
        u32 runtime_word = 0xAABBCCDDU;
        u16 selection_mode = 0xFFFFU;
        const auto result = select_legacy_battle_actor_resource(
            nullptr,
            nullptr,
            &final_state,
            nullptr,
            &workspace,
            port,
            port,
            0U,
            0U,
            runtime_word,
            selection_mode
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                !result.selected && runtime_word == 0xAABBCCDDU &&
                selection_mode == 0xFFFFU &&
                final_state.profile_buffer[0U] == 0U &&
                final_state.pre_effect_words[0U] == 0U &&
                workspace.tail_words[2U] == 0U,
            "zero occurrence preserves the mandatory clearing prefix before failure"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .next_token = 0x76000020U,
                 .name = "ordinary"},
                {.token = 0x76000020U,
                 .secondary_quantity = -1,
                 .tertiary_quantity = 2,
                 .name = "flagged",
                 .capacity_gate_flags = 0x2000U},
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .secondary_quantity = 3,
                 .tertiary_quantity = 4,
                 .name = "stale-zero"},
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .name = "twenty-byte-name-xxxx",
                 .capacity_gate_flags = 0x2000U},
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .name = "only",
                 .capacity_gate_flags = 0x2000U},
            }
        );

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
        inventories.assign(
            list,
            {
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
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
                {.token = 0x77000010U,
                 .resource_id = 0x0300U,
                 .name = "twenty-byte-name-xxx",
                 .mode_flags = 0x01U},
            }
        );

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
        inventories.assign(
            list,
            {
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
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
                {.token = 0x77000010U,
                 .secondary_quantity = 9,
                 .tertiary_quantity = 8,
                 .name = "stale",
                 .mode_flags = 0U},
            }
        );

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
        inventories.assign(
            list,
            {
                {.token = 0x77000000U, .next_token = 0x77000010U, .name = {}},
                {.token = 0x77000010U,
                 .resource_id = 0x222U,
                 .name = "only",
                 .category_mask = 0x08000000U,
                 .mode_flags = 0x01U},
            }
        );

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
        inventories.assign(
            list,
            {
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
            }
        );

        u16 quantity = 0xBEEFU;
        const auto status =
            count_legacy_battle_actor_mode_resources(&list, quantity);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                quantity == 1U && list.resource_head_token == 0U,
            "mode resource count filters bit twenty-seven, fixed identifiers and output mode bits before wrapping all quantity words"
        );
    }

    {
        list.next_resource_head_token = 0x79000000U;
        inventories.assign(
            list,
            {
                {.token = 0x79000000U, .next_token = 0x79000010U, .name = {}},
                {.token = 0x79000010U,
                 .resource_id = 0x0222U,
                 .secondary_quantity = 7,
                 .tertiary_quantity = 8,
                 .name = "excluded"},
            }
        );

        u16 quantity = 0xBEEFU;
        const auto status =
            count_legacy_battle_actor_mode_resources(&list, quantity);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                quantity == 0U && list.resource_head_token == 0U,
            "mode resource count zeroes its output before traversing an excluded chain"
        );
    }

    {
        LegacyBattleGroupAWorkspaceState workspace;
        list.next_resource_head_token = 0x76000000U;
        list.selected_resource_token = 0x76000010U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x33U,
                 .primary_quantity = 4U,
                 .secondary_quantity = 2,
                 .name = {},
                 .gate_word_48 = 0x0402U},
            }
        );

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                openswd3::test::inventory_nodes(list).size() == 2U &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_node(list, 1U), 0x48U
                ) == 0x0401U &&
                openswd3::test::inventory_node(list, 1U).selected_count == 4U &&
                openswd3::test::inventory_node(list, 1U).quantity_a == 2 &&
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
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .resource_id = 0x44U,
                 .primary_quantity = 2U,
                 .secondary_quantity = 2,
                 .name = {},
                 .capacity_gate_flags = 0x2000U},
            }
        );

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                openswd3::test::inventory_nodes(list).size() == 2U &&
                openswd3::test::inventory_node(list, 1U).selected_count == 1U &&
                openswd3::test::inventory_node(list, 1U).quantity_a == 1 &&
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
        inventories.assign(
            list,
            {
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
            }
        );

        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                openswd3::test::inventory_nodes(list).size() == 3U &&
                openswd3::test::inventory_node(list, 1U).legacy_next_token ==
                    0x76000030U &&
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
        inventories.assign(list, {{.token = 0xABCD0000U, .name = {}}});
        workspace.tail_words[2U] = 0xBEEFU;
        const auto result =
            release_legacy_battle_actor_resource(&list, &workspace);
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.output_word == 0U &&
                list.resource_head_token == 0xABCD0000U &&
                openswd3::test::inventory_nodes(list).size() == 1U &&
                workspace.tail_words[2U] == 0xBEEFU,
            "missing selected resource returns a normal zero after publishing the head and preserving the workspace"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .next_token = 0x76000020U,
                 .primary_quantity = 1U,
                 .tertiary_quantity = 2,
                 .name = {},
                 .category_mask = 0x2000U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x2000U},
                {.token = 0x76000020U,
                 .name = {},
                 .capacity_gate_flags = 0x2000U},
            }
        );

        u16 count = 0xFFFEU;
        const auto status =
            count_legacy_battle_actor_resource_list(&list, 4U, count);
        test.expect_true(
            status == LegacyBattleActorListQueryStatus::completed &&
                count == 1U &&
                openswd3::test::inventory_nodes(list).size() == 3U &&
                openswd3::test::inventory_node(list, 1U).selected_count == 1U &&
                openswd3::test::inventory_node(list, 1U).quantity_b == 2 &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_node(list, 1U), 0x4CU
                ) == 0x2000U &&
                openswd3::test::inventory_word(
                    openswd3::test::inventory_node(list, 2U), 0x4CU
                ) == 0x2000U &&
                list.resource_head_token == 0U,
            "resource count preserves the initial word, wraps increments, and category four counts its extra bit independently"
        );
    }

    {
        LegacyBattleGroupAConfigurationState configuration;
        configuration.actor_record_token = 0x00600000U;
        configuration.actor_record[2U] = 2U;
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .primary_quantity = 3U,
                 .secondary_quantity = -1,
                 .tertiary_quantity = 10,
                 .name = "resource",
                 .category_mask = 0x10U,
                 .mode_flags = 0x01U,
                 .capacity_gate_flags = 0x4003U},
            }
        );

        std::array<u8, 20> name{};
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, &configuration, 0U, 1U, name, flags, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && name[0U] == 'r' && name[8U] == 0U &&
                quantity == 6U && flags == 0xC000U &&
                list.resource_head_token == 0x76000010U,
            "resource query destructively advances the chain, counts a positive match and publishes quantity plus both flag bits"
        );
    }

    {
        list.next_resource_head_token = 0x76000000U;
        inventories.assign(
            list,
            {
                {.token = 0x76000000U, .next_token = 0x76000010U, .name = {}},
                {.token = 0x76000010U,
                 .name = {},
                 .category_mask = 0U,
                 .mode_flags = 0U},
            }
        );

        std::array<u8, 20> name{};
        name.fill(0xCCU);
        u16 flags = 0xBEEFU;
        u16 quantity = 0xABCDU;
        const auto result = query_legacy_battle_actor_resource_list(
            &list, nullptr, 0U, 0U, name, flags, quantity
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.found && flags == 0U && quantity == 0xABCDU &&
                name[0U] == 0xCCU,
            "occurrence zero takes the stale success branch after the first nonmatch and rechecks filters without copying"
        );
    }

    {
        actor.next_list_index = openswd3::test::party_root(list).legacy_token;
        openswd3::test::inventory_word(
            openswd3::test::party_node(list, 1U), 0x40U
        ) = 11U;
        openswd3::test::inventory_word(
            openswd3::test::party_node(list, 1U), 0x42U
        ) = 12U;
        openswd3::test::inventory_word(
            openswd3::test::party_node(list, 1U), 0x44U
        ) = 13U;
        openswd3::test::inventory_word(
            openswd3::test::party_node(list, 1U), 0x48U
        ) = 0U;
        openswd3::test::inventory_word(
            openswd3::test::party_node(list, 1U), 0x5CU
        ) = 77U;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer.fill(0xFFFFFFFFU);
        final_state.pre_effect_words.fill(0xFFFFFFFFU);
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        QueryPort port;
        u32 output = 0xAABBCCDDU;
        const auto result = apply_legacy_battle_actor_list(
            &actor, &list, &final_state, &item_effect, port, 0U, 0U, 1U, output
        );
        test.expect_true(
            result.status == LegacyBattleActorListQueryStatus::completed &&
                result.selection_mode == 1U && output == 77U &&
                actor.current_list_index ==
                    openswd3::test::party_node(list, 1U).legacy_token &&
                final_state.profile_buffer[0U] != 0xFFFFFFFFU &&
                final_state.pre_effect_words[3U] == 0U &&
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
        actor.next_list_index = openswd3::test::party_root(list).legacy_token;
        LegacyBattleGroupAFinalProcessingState final_state;
        final_state.profile_buffer[0U] = 0xAABBCCDDU;
        LegacyBattleGroupAItemEffectApplicationState item_effect;
        item_effect.mode_flags = 0x40U;
        QueryPort port;
        u32 output = 0xAABBCCDDU;
        const auto cursor = actor.current_list_index;
        const auto result = apply_legacy_battle_actor_list(
            &actor, &list, &final_state, &item_effect, port, 0U, 0U, 0U, output
        );
        test.expect_true(
            !result.selection_mode && output == 0xAABBCCDDU &&
                actor.current_list_index == cursor &&
                final_state.profile_buffer[0U] == 0xAABBCCDDU &&
                item_effect.mode_flags == 0x40U,
            "zero occurrence returns minus one before mode flags, clear, and index commit"
        );
    }

    test_list_query_live_state(test);
    test_list_profile_application(test);
    test_list_profile_cursor(test);
    test_list_state_behavior(test);
    test_list_state_filters(test);
    test_list_state_audio_order(test);

    {
        QueryPort port;
        LegacyBattleActorListQueryState bad = list;
        openswd3::test::party_root(bad).legacy_next_token = 0xDEADBEEFU;
        LegacyBattleMonProfile profile{};
        std::array<u8, 20> text{};
        u32 value = 0xABCD1234U;
        const auto result = query_legacy_battle_actor_list(
            &actor, &bad, port, 0U, 0U, 1U, text, value, profile, table
        );
        test.expect_true(
            result.status ==
                    LegacyBattleActorListQueryStatus::list_node_typed_stop &&
                actor.current_list_index == 0xDEADBEEFU && value == 0xABCD1234U,
            "missing node stops after the shared list-index commit"
        );
    }
}
