#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <span>
#include <vector>

namespace {

using openswd3::asset_runtime::initialize_legacy_action_record;
using openswd3::asset_runtime::LegacyActActionStreamProvider;
using openswd3::asset_runtime::LegacyActionRecord;
using openswd3::asset_runtime::LegacyActionStreamLoadResult;
using openswd3::asset_runtime::LegacyActionStreamProvider;
using openswd3::asset_runtime::LegacyActionStreamStatus;
using openswd3::asset_runtime::LegacyActionUpdater;
using openswd3::asset_runtime::LegacyActionUpdateStatus;
using openswd3::asset_runtime::LegacyActRuntime;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

class FakeStreamProvider final : public LegacyActionStreamProvider {
public:
    [[nodiscard]] LegacyActionStreamLoadResult
    load_action_stream(const u32 action_id, const u32 variant_index,
                       const bool cached) override {
        ++calls;
        last_action_id = action_id;
        last_variant_index = variant_index;
        last_cached = cached;
        if (fail) {
            return {};
        }
        return LegacyActionStreamLoadResult{
            LegacyActionStreamStatus::ready,
            bytes,
            cache_hit,
        };
    }

    void set_words(const std::span<const u16> words) {
        bytes.clear();
        bytes.reserve(words.size() * 2U);
        for (const u16 word : words) {
            bytes.push_back(static_cast<u8>(word));
            bytes.push_back(static_cast<u8>(word >> 8U));
        }
    }

    std::vector<u8> bytes;
    std::size_t calls{};
    u32 last_action_id{};
    u32 last_variant_index{};
    bool last_cached{};
    bool fail{};
    bool cache_hit{};
};

[[nodiscard]] LegacyActionRecord zero_record() {
    LegacyActionRecord record{};
    initialize_legacy_action_record(record);
    return record;
}

void make_keys_stable(LegacyActionRecord& record) {
    record.action_id = 1U;
    record.cached_action_id = 1U;
    record.base_variant = 0U;
    record.cached_base_variant = 0U;
    record.variant_delta = 0U;
    record.cached_variant_delta = 0U;
}

void test_initializer_is_selective(openswd3::test::Context& test) {
    LegacyActionRecord record;
    std::memset(&record, 0xA5, sizeof(record));
    initialize_legacy_action_record(record);

    test.expect_equal(record.field_1c, 0xFFFFFFFFU, "+1c sentinel");
    test.expect_equal(record.one_shot_base_variant, 0xFFFFFFFFU,
                      "+20 sentinel");
    test.expect_equal(record.one_shot_variant_delta, 0xFFFFFFFFU,
                      "+3c sentinel");
    test.expect_equal(record.command_cursor, u16{0U}, "+42 cleared");
    test.expect_equal(record.wait_remaining, u16{0U}, "+44 cleared");
    test.expect_equal(record.wait_default, u16{0U}, "+46 cleared");
    test.expect_equal(record.wait_override, u16{0U}, "+48 cleared");
    test.expect_equal(record.external_mode, 0U, "+90 cleared");
    test.expect_equal(record.action_id, 0xA5A5A5A5U,
                      "initializer does not clear action ID");
    test.expect_equal(record.field_50, u16{0xA5A5U},
                      "initializer preserves unrelated fields");
}

void test_early_returns_and_stream_failure(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    constexpr u16 kDe = 0x4544U;
    provider.set_words(std::span<const u16>{&kDe, 1U});
    LegacyActionUpdater updater{provider};

    LegacyActionRecord record = zero_record();
    record.action_id = 1U;
    record.external_mode = 1U;
    record.command_cursor = 2U;
    const auto external_early = updater.update(record);
    test.expect_equal(external_early.return_value, 1U,
                      "external mode early return is true");
    test.expect_equal(provider.calls, std::size_t{0U},
                      "external mode early return skips ACT lookup");

    record = zero_record();
    const auto zero_action = updater.update(record);
    test.expect_equal(zero_action.return_value, 1U,
                      "zero action ID early return is true");
    test.expect_equal(provider.calls, std::size_t{0U},
                      "zero action skips ACT lookup");

    record = zero_record();
    make_keys_stable(record);
    provider.fail = true;
    const auto failed = updater.update(record);
    test.expect_equal(failed.status,
                      LegacyActionUpdateStatus::stream_load_failed,
                      "ACT failure is explicit");
    test.expect_equal(failed.return_value, 0U,
                      "original update returns zero on ACT failure");
    test.expect_equal(record.stream_pointer_32, 0U,
                      "failed load writes a null legacy pointer slot");
}

void test_key_reset_order_and_wait(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    provider.cache_hit = true;
    constexpr u16 kDe = 0x4544U;
    provider.set_words(std::span<const u16>{&kDe, 1U});
    LegacyActionUpdater updater{provider};
    updater.set_stream_cache_mode(1U);

    LegacyActionRecord record = zero_record();
    record.action_id = 7U;
    record.cached_action_id = 70U;
    record.base_variant = 11U;
    record.cached_base_variant = 110U;
    record.variant_delta = 13U;
    record.cached_variant_delta = 130U;
    record.mode_flags = 0xFFFFFFFFU;
    record.packed_ap_state = 0x2211U;
    record.wait_override = 0x8123U;
    record.field_4a = 0x4001U;
    record.field_4c = 0x4002U;
    record.field_4e = 0x4003U;
    record.field_5e = 0x5001U;
    record.field_60 = 0x5002U;
    record.field_70 = 0x7001U;
    record.field_72 = 0x7002U;
    record.field_74 = 0x7003U;
    record.field_50 = 0x5050U;

    const auto changed = updater.update(record);
    test.expect_true(changed.key_changed, "three key groups report change");
    test.expect_true(changed.cache_hit, "provider hit is forwarded");
    test.expect_equal(provider.last_action_id, 7U, "action key forwarded");
    test.expect_equal(provider.last_variant_index, 24U,
                      "base plus delta selects ACT variant");
    test.expect_true(provider.last_cached,
                     "only exact cache mode one selects cached loader");
    test.expect_equal(record.cached_action_id, 7U, "action cache updated");
    test.expect_equal(record.cached_base_variant, 11U, "base cache updated");
    test.expect_equal(record.cached_variant_delta, 13U, "delta cache updated");
    test.expect_equal(record.mode_flags, 0x80000000U,
                      "delta reset runs first and removes low bits");
    test.expect_equal(record.packed_ap_state, u16{0x2211U},
                      "+40 persists across key reset");
    test.expect_equal(record.wait_override, u16{0x8123U},
                      "+48 persists across key reset");
    test.expect_equal(record.field_4a, u16{0x4001U},
                      "+4a persists across key reset");
    test.expect_equal(record.field_5e, u16{0x5001U},
                      "+5e persists across key reset");
    test.expect_equal(record.field_70, u16{0x7001U},
                      "+70 persists across key reset");
    test.expect_equal(record.field_50, u16{0U},
                      "+50 clears only when parsing starts");
    test.expect_equal(record.command_cursor, u16{1U},
                      "DE remains consumed in normal mode");
    test.expect_equal(record.stream_pointer_32, 1U,
                      "successful load writes non-null compatibility token");

    record = zero_record();
    make_keys_stable(record);
    record.wait_remaining = 2U;
    const auto waiting = updater.update(record);
    test.expect_equal(waiting.return_value, 1U, "wait path returns true");
    test.expect_equal(record.wait_remaining, u16{1U},
                      "wait decrements after ACT lookup");
    test.expect_equal(record.command_cursor, u16{0U},
                      "wait path does not parse a command");
    test.expect_equal(record.stream_pointer_32, 1U,
                      "wait path still refreshes stream pointer slot");
}

void test_field_and_mode_commands(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    constexpr u16 kWords[]{
        0x4148U, 0x414DU, 0x414EU, 0x4C44U, 0x12ABU, 0x4753U, 0x34CDU, 0x4E4FU,
        0x5649U, 0x4154U, 0x1001U, 0x4158U, 0x1002U, 0x4159U, 0x1003U, 0x434CU,
        0x4342U, 0x2001U, 0x0200U, 0x4347U, 0x2002U, 0x0201U, 0x4352U, 0x2003U,
        0x0202U, 0x464CU, 0x3001U, 0x3002U, 0x3003U, 0x3004U, 0x3005U, 0x3006U,
        0x3007U, 0x4F41U, 0x4001U, 0x4F58U, 0x4002U, 0x4F59U, 0x4003U, 0x544EU,
        0x0002U, 0x5041U, 0x4004U, 0x5145U, 0x4005U, 0x5246U, 0x4006U, 0x524FU,
        0x4007U, 0x5457U, 0xABCDU, 0x534DU, 0x5748U, 0x5001U, 0x0500U, 0x5756U,
        0x5002U, 0x5859U, 0x5003U, 0x0501U, 0x4145U, 0x6001U, 0x4544U,
    };
    provider.set_words(kWords);
    LegacyActionUpdater updater{provider};
    LegacyActionRecord record = zero_record();
    make_keys_stable(record);
    record.mode_flags = 0x80000000U;

    const auto updated = updater.update(record);
    test.expect_equal(updated.status, LegacyActionUpdateStatus::completed,
                      "field command group completes");
    test.expect_equal(record.mode_flags, 0x80000015U,
                      "mode masks execute in stream order");
    test.expect_equal(record.field_62, u16{0x00ABU},
                      "DL keeps parameter low byte");
    test.expect_equal(record.field_8a, u8{0xCDU},
                      "SG keeps parameter low byte");
    test.expect_equal(record.field_5a, u16{0x1001U}, "TA field");
    test.expect_equal(record.field_76, u16{0x1002U}, "XA field");
    test.expect_equal(record.field_78, u16{0x1003U}, "YA field");
    test.expect_equal(record.field_68, u16{0x2001U}, "BC first field");
    test.expect_equal(record.field_74, u16{0x0200U}, "BC second field");
    test.expect_equal(record.field_66, u16{0x2002U}, "GC first field");
    test.expect_equal(record.field_72, u16{0x0201U}, "GC second field");
    test.expect_equal(record.field_64, u16{0x2003U}, "RC first field");
    test.expect_equal(record.field_70, u16{0x0202U}, "RC second field");
    test.expect_equal(record.field_7a, u16{0x3001U}, "LF field one");
    test.expect_equal(record.field_86, u16{0x3007U}, "LF field seven");
    test.expect_equal(record.field_50, u16{0x4001U}, "AO field");
    test.expect_equal(record.field_5e, u16{0x4002U}, "XO field");
    test.expect_equal(record.field_60, u16{0x4003U}, "YO field");
    test.expect_equal(record.packed_ap_state, u16{0x0102U},
                      "AP advances one-based high byte");
    test.expect_equal(record.field_4c, u16{0x4004U}, "AP parameter field");
    test.expect_equal(record.field_28, 0x4005U, "EQ zero extends to dword");
    test.expect_equal(record.field_4a, u16{0x4006U}, "FR field");
    test.expect_equal(record.field_4e, u16{0x4007U}, "OR field");
    test.expect_equal(record.field_88, u8{0xCDU},
                      "WT keeps parameter low byte");
    test.expect_equal(record.field_94, 1U, "MS sets dword flag");
    test.expect_equal(record.field_2c, 0x5001U, "HW first dword");
    test.expect_equal(record.field_30, 0x0500U, "HW second dword");
    test.expect_equal(record.field_58, u16{0x5002U}, "VW field");
    test.expect_equal(record.draw_offset_x, 0x5003U, "YX X dword");
    test.expect_equal(record.draw_offset_y, 0x0501U, "YX Y dword");
    test.expect_equal(record.field_24, 0x6001U, "EA zero extends to dword");
    test.expect_equal(record.command_cursor,
                      static_cast<u16>(std::size(kWords)),
                      "two-parameter second words are reprocessed");
}

void test_wait_and_terminator_commands(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    LegacyActionUpdater updater{provider};

    constexpr u16 kDsWords[]{0x5344U, 7U, 0x4544U};
    provider.set_words(kDsWords);
    LegacyActionRecord record = zero_record();
    make_keys_stable(record);
    record.wait_override = 0x8003U;
    static_cast<void>(updater.update(record));
    test.expect_equal(record.wait_default, u16{7U},
                      "DS stores ordinary wait value");
    test.expect_equal(record.wait_remaining, u16{3U},
                      "DS applies external override immediately");
    const std::size_t calls_before_wait = provider.calls;
    static_cast<void>(updater.update(record));
    test.expect_equal(provider.calls, calls_before_wait + 1U,
                      "waiting frame still performs ACT lookup");
    test.expect_equal(record.wait_remaining, u16{2U},
                      "waiting frame decrements once");

    constexpr u16 k2OWords[]{0x1234U, 0x4F32U};
    provider.set_words(k2OWords);
    record = zero_record();
    make_keys_stable(record);
    record.wait_default = 5U;
    static_cast<void>(updater.update(record));
    test.expect_equal(record.command_cursor, u16{1U},
                      "2O always rewinds to its own marker");
    test.expect_equal(record.field_8c, 0U,
                      "2O with wait does not set completion flag");

    record = zero_record();
    make_keys_stable(record);
    static_cast<void>(updater.update(record));
    test.expect_equal(record.field_8c, 1U,
                      "2O without wait sets completion flag");

    constexpr u16 kVoWords[]{0x1234U, 0x4F56U};
    provider.set_words(kVoWords);
    record = zero_record();
    make_keys_stable(record);
    static_cast<void>(updater.update(record));
    test.expect_equal(record.command_cursor, u16{0U},
                      "VO normal mode resets cursor");

    record = zero_record();
    make_keys_stable(record);
    record.external_mode = 1U;
    static_cast<void>(updater.update(record));
    test.expect_equal(record.command_cursor, u16{1U},
                      "VO external mode rewinds to marker");
    const std::size_t calls_before_early = provider.calls;
    static_cast<void>(updater.update(record));
    test.expect_equal(provider.calls, calls_before_early,
                      "rewound nonzero cursor triggers next-call early return");

    constexpr u16 kDeWords[]{0x1234U, 0x4544U};
    provider.set_words(kDeWords);
    record = zero_record();
    make_keys_stable(record);
    record.external_mode = 1U;
    static_cast<void>(updater.update(record));
    test.expect_equal(record.command_cursor, u16{1U},
                      "DE external mode also rewinds to marker");
}

void test_malformed_stream_guard(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    constexpr u16 kTruncated[]{0x4145U};
    provider.set_words(kTruncated);
    LegacyActionUpdater updater{provider};
    LegacyActionRecord record = zero_record();
    make_keys_stable(record);

    const auto updated = updater.update(record);
    test.expect_equal(updated.status,
                      LegacyActionUpdateStatus::malformed_stream,
                      "missing command parameter is isolated");
    test.expect_equal(updated.return_value, 0U,
                      "malformed modern safety path reports failure");
}

void test_real_act_provider(openswd3::test::Context& test,
                            const std::filesystem::path& root) {
    LegacyActRuntime runtime{root};
    runtime.set_cache_limit(0x00080000U);
    LegacyActActionStreamProvider provider{runtime};
    LegacyActionUpdater updater{provider};

    struct Query {
        u32 action_id;
        u32 variant_index;
    };
    constexpr Query kQueries[]{
        {1U, 0U},    {3001U, 68U}, {6001U, 0U},
        {9001U, 0U}, {12001U, 0U}, {15001U, 0U},
    };
    for (const Query query : kQueries) {
        LegacyActionRecord record = zero_record();
        record.action_id = query.action_id;
        record.cached_action_id = query.action_id;
        record.base_variant = query.variant_index;
        record.cached_base_variant = query.variant_index;
        const auto updated = updater.update(record);
        test.expect_equal(updated.status, LegacyActionUpdateStatus::completed,
                          "real ACT stream executes safely");
        test.expect_equal(updated.return_value, 1U,
                          "real ACT stream returns original success value");
        test.expect_equal(record.stream_pointer_32, 1U,
                          "real ACT stream writes non-null token");

        if (query.action_id == 1U) {
            test.expect_equal(record.packed_ap_state, u16{0x0101U},
                              "real NT/AP final packed state");
            test.expect_equal(record.field_4a, u16{0x0171U},
                              "real FR field snapshot");
            test.expect_equal(record.wait_default, u16{0U},
                              "real DS zero wait snapshot");
            test.expect_equal(record.draw_offset_x, 6U, "real YX X snapshot");
            test.expect_equal(record.draw_offset_y, 0x47U,
                              "real YX Y snapshot");
            test.expect_equal(record.field_2c, 2U, "real HW first snapshot");
            test.expect_equal(record.field_30, 1U, "real HW second snapshot");
            test.expect_equal(record.command_cursor, u16{0U},
                              "real VO resets cursor");
        }
    }

    updater.set_stream_cache_mode(1U);
    LegacyActionRecord cached = zero_record();
    make_keys_stable(cached);
    const auto first = updater.update(cached);
    const auto second = updater.update(cached);
    test.expect_false(first.cache_hit,
                      "first real cached updater query misses");
    test.expect_true(second.cache_hit, "second real cached updater query hits");
    runtime.close();
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_initializer_is_selective(test);
    test_early_returns_and_stream_failure(test);
    test_key_reset_order_and_wait(test);
    test_field_and_mode_commands(test);
    test_wait_and_terminator_commands(test);
    test_malformed_stream_guard(test);
    if (argument_count == 2) {
        test_real_act_provider(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
