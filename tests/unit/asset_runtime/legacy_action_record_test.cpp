#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_record.hpp"

#include <algorithm>
#include <array>
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
    [[nodiscard]] LegacyActionStreamLoadResult load_action_stream(
        const u32 action_id, const u32 variant_index, const bool cached
    ) override {
        ++calls;
        last_action_id = action_id;
        last_variant_index = variant_index;
        last_cached = cached;
        if (stop) {
            return {
                .status = LegacyActionStreamStatus::load_stopped,
                .stream = {},
                .stop = openswd3::asset_runtime::LegacyActionStreamStop{
                    openswd3::asset_runtime::LegacyActRuntimeStatus::
                        allocation_failed,
                    openswd3::asset_runtime::LegacyActVariantStatus::
                        allocation_failed,
                },
            };
        }
        if (fail) {
            return {.stream = {}, .return_edx = return_edx};
        }
        return LegacyActionStreamLoadResult{
            LegacyActionStreamStatus::ready,
            bytes,
            cache_hit,
            {},
            return_edx,
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
    bool stop{};
    bool cache_hit{};
    std::optional<u32> return_edx{};
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
    test.expect_equal(
        record.one_shot_base_variant, 0xFFFFFFFFU, "+20 sentinel"
    );
    test.expect_equal(
        record.one_shot_variant_delta, 0xFFFFFFFFU, "+3c sentinel"
    );
    test.expect_equal(record.command_cursor, u16{0U}, "+42 cleared");
    test.expect_equal(record.wait_remaining, u16{0U}, "+44 cleared");
    test.expect_equal(record.wait_default, u16{0U}, "+46 cleared");
    test.expect_equal(record.wait_override, u16{0U}, "+48 cleared");
    test.expect_equal(record.external_mode, 0U, "+90 cleared");
    test.expect_equal(
        record.action_id, 0xA5A5A5A5U, "initializer does not clear action ID"
    );
    test.expect_equal(
        record.field_50, u16{0xA5A5U}, "initializer preserves unrelated fields"
    );
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
    test.expect_equal(
        external_early.return_value, 1U, "external mode early return is true"
    );
    test.expect_equal(
        provider.calls,
        std::size_t{0U},
        "external mode early return skips ACT lookup"
    );

    record = zero_record();
    const auto zero_action = updater.update(record);
    test.expect_equal(
        zero_action.return_value, 1U, "zero action ID early return is true"
    );
    test.expect_equal(
        provider.calls, std::size_t{0U}, "zero action skips ACT lookup"
    );

    record = zero_record();
    make_keys_stable(record);
    provider.fail = true;
    const auto failed = updater.update(record);
    test.expect_equal(
        failed.status,
        LegacyActionUpdateStatus::stream_load_failed,
        "ACT failure is explicit"
    );
    test.expect_equal(
        failed.return_value, 0U, "original update returns zero on ACT failure"
    );
    test.expect_equal(
        record.stream_pointer_32,
        0U,
        "failed load writes a null legacy pointer slot"
    );
}

void test_stream_stop_prefix(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    provider.stop = true;
    LegacyActionUpdater updater{provider};
    for (const bool changed : {false, true}) {
        LegacyActionRecord record = zero_record();
        make_keys_stable(record);
        if (changed) {
            record.action_id = 2U;
        }
        record.stream_pointer_32 = 0xCAFEBABEU;
        record.wait_remaining = 7U;
        record.command_cursor = 9U;
        record.field_50 = 0x1234U;
        const auto result = updater.update(record);
        test.expect_true(
            result.status == LegacyActionUpdateStatus::stream_load_stopped &&
                result.stream_stop.has_value() &&
                result.stream_stop->runtime_status ==
                    openswd3::asset_runtime::LegacyActRuntimeStatus::
                        allocation_failed &&
                result.stream_stop->physical_status ==
                    openswd3::asset_runtime::LegacyActVariantStatus::
                        allocation_failed &&
                result.key_changed == changed &&
                record.cached_action_id == record.action_id &&
                record.stream_pointer_32 == 0xCAFEBABEU &&
                record.wait_remaining == (changed ? 0U : 7U) &&
                record.command_cursor == (changed ? 0U : 9U) &&
                record.field_50 == 0x1234U,
            "unfinished loader preserves +54 and only earlier key-reset writes"
        );
    }
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
    test.expect_equal(
        provider.last_variant_index, 24U, "base plus delta selects ACT variant"
    );
    test.expect_true(
        provider.last_cached, "only exact cache mode one selects cached loader"
    );
    test.expect_equal(record.cached_action_id, 7U, "action cache updated");
    test.expect_equal(record.cached_base_variant, 11U, "base cache updated");
    test.expect_equal(record.cached_variant_delta, 13U, "delta cache updated");
    test.expect_equal(
        record.mode_flags,
        0x80000000U,
        "delta reset runs first and removes low bits"
    );
    test.expect_equal(
        record.packed_ap_state, u16{0x2211U}, "+40 persists across key reset"
    );
    test.expect_equal(
        record.wait_override, u16{0x8123U}, "+48 persists across key reset"
    );
    test.expect_equal(
        record.field_4a, u16{0x4001U}, "+4a persists across key reset"
    );
    test.expect_equal(
        record.field_5e, u16{0x5001U}, "+5e persists across key reset"
    );
    test.expect_equal(
        record.field_70, u16{0x7001U}, "+70 persists across key reset"
    );
    test.expect_equal(
        record.field_50, u16{0U}, "+50 clears only when parsing starts"
    );
    test.expect_equal(
        record.command_cursor, u16{1U}, "DE remains consumed in normal mode"
    );
    test.expect_equal(
        record.stream_pointer_32,
        1U,
        "successful load writes non-null compatibility token"
    );

    record = zero_record();
    make_keys_stable(record);
    record.wait_remaining = 2U;
    const auto waiting = updater.update(record);
    test.expect_equal(waiting.return_value, 1U, "wait path returns true");
    test.expect_equal(
        record.wait_remaining, u16{1U}, "wait decrements after ACT lookup"
    );
    test.expect_equal(
        record.command_cursor, u16{0U}, "wait path does not parse a command"
    );
    test.expect_equal(
        record.stream_pointer_32,
        1U,
        "wait path still refreshes stream pointer slot"
    );
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
    test.expect_equal(
        updated.status,
        LegacyActionUpdateStatus::completed,
        "field command group completes"
    );
    test.expect_equal(
        record.mode_flags, 0x80000015U, "mode masks execute in stream order"
    );
    test.expect_equal(
        record.field_62, u16{0x00ABU}, "DL keeps parameter low byte"
    );
    test.expect_equal(
        record.field_8a, u8{0xCDU}, "SG keeps parameter low byte"
    );
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
    test.expect_equal(
        record.packed_ap_state, u16{0x0102U}, "AP advances one-based high byte"
    );
    test.expect_equal(record.field_4c, u16{0x4004U}, "AP parameter field");
    test.expect_equal(record.field_28, 0x4005U, "EQ zero extends to dword");
    test.expect_equal(record.field_4a, u16{0x4006U}, "FR field");
    test.expect_equal(record.field_4e, u16{0x4007U}, "OR field");
    test.expect_equal(
        record.field_88, u8{0xCDU}, "WT keeps parameter low byte"
    );
    test.expect_equal(record.field_94, 1U, "MS sets dword flag");
    test.expect_equal(record.field_2c, 0x5001U, "HW first dword");
    test.expect_equal(record.field_30, 0x0500U, "HW second dword");
    test.expect_equal(record.field_58, u16{0x5002U}, "VW field");
    test.expect_equal(record.draw_offset_x, 0x5003U, "YX X dword");
    test.expect_equal(record.draw_offset_y, 0x0501U, "YX Y dword");
    test.expect_equal(record.field_24, 0x6001U, "EA zero extends to dword");
    test.expect_equal(
        record.command_cursor,
        static_cast<u16>(std::size(kWords)),
        "two-parameter second words are reprocessed"
    );
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
    test.expect_equal(
        record.wait_default, u16{7U}, "DS stores ordinary wait value"
    );
    test.expect_equal(
        record.wait_remaining,
        u16{3U},
        "DS applies external override immediately"
    );
    const std::size_t calls_before_wait = provider.calls;
    static_cast<void>(updater.update(record));
    test.expect_equal(
        provider.calls,
        calls_before_wait + 1U,
        "waiting frame still performs ACT lookup"
    );
    test.expect_equal(
        record.wait_remaining, u16{2U}, "waiting frame decrements once"
    );

    constexpr u16 k2OWords[]{0x1234U, 0x4F32U};
    provider.set_words(k2OWords);
    record = zero_record();
    make_keys_stable(record);
    record.wait_default = 5U;
    static_cast<void>(updater.update(record));
    test.expect_equal(
        record.command_cursor, u16{1U}, "2O always rewinds to its own marker"
    );
    test.expect_equal(
        record.field_8c, 0U, "2O with wait does not set completion flag"
    );

    record = zero_record();
    make_keys_stable(record);
    static_cast<void>(updater.update(record));
    test.expect_equal(
        record.field_8c, 1U, "2O without wait sets completion flag"
    );

    constexpr u16 kVoWords[]{0x1234U, 0x4F56U};
    provider.set_words(kVoWords);
    record = zero_record();
    make_keys_stable(record);
    static_cast<void>(updater.update(record));
    test.expect_equal(
        record.command_cursor, u16{0U}, "VO normal mode resets cursor"
    );

    record = zero_record();
    make_keys_stable(record);
    record.external_mode = 1U;
    static_cast<void>(updater.update(record));
    test.expect_equal(
        record.command_cursor, u16{1U}, "VO external mode rewinds to marker"
    );
    const std::size_t calls_before_early = provider.calls;
    static_cast<void>(updater.update(record));
    test.expect_equal(
        provider.calls,
        calls_before_early,
        "rewound nonzero cursor triggers next-call early return"
    );

    constexpr u16 kDeWords[]{0x1234U, 0x4544U};
    provider.set_words(kDeWords);
    record = zero_record();
    make_keys_stable(record);
    record.external_mode = 1U;
    static_cast<void>(updater.update(record));
    test.expect_equal(
        record.command_cursor,
        u16{1U},
        "DE external mode also rewinds to marker"
    );
}

void test_malformed_stream_guard(openswd3::test::Context& test) {
    FakeStreamProvider provider;
    constexpr u16 kTruncated[]{0x4145U};
    provider.set_words(kTruncated);
    LegacyActionUpdater updater{provider};
    LegacyActionRecord record = zero_record();
    make_keys_stable(record);

    const auto updated = updater.update(record);
    test.expect_equal(
        updated.status,
        LegacyActionUpdateStatus::malformed_stream,
        "missing command parameter is isolated"
    );
    test.expect_equal(
        updated.return_value, 0U, "malformed modern safety path reports failure"
    );
}

void test_lf_contiguous_read_and_cursor_commit(openswd3::test::Context& test) {
    for (const std::size_t command_index : {0U, 0xFFFEU}) {
        for (std::size_t available = 0U; available <= 7U; ++available) {
            const std::size_t operand_start = command_index + 1U;
            std::vector<u16> words(operand_start + available, 0xEEEEU);
            words[command_index] = 0x464CU;
            for (std::size_t index = 0U; index < available; ++index) {
                words[operand_start + index] =
                    static_cast<u16>(0x1000U + index);
            }

            if (available == 7U) {
                const auto next = static_cast<u16>(operand_start + 7U);
                if (next == words.size()) {
                    words.push_back(0x4544U);
                } else {
                    words[next] = 0x4544U;
                }
            }

            for (const bool partial_word : {false, true}) {
                FakeStreamProvider provider;
                provider.return_edx = 0xA1235566U;
                provider.set_words(words);
                if (partial_word) {
                    provider.bytes.push_back(0x55U);
                }

                LegacyActionUpdater updater{provider};
                LegacyActionRecord record = zero_record();
                make_keys_stable(record);
                record.command_cursor = static_cast<u16>(command_index);
                const std::array<u16*, 7> fields{
                    &record.field_7a, &record.field_7c, &record.field_7e,
                    &record.field_80, &record.field_82, &record.field_84,
                    &record.field_86,
                };
                for (std::size_t index = 0U; index < fields.size(); ++index) {
                    *fields[index] = static_cast<u16>(0xA500U + index);
                }

                const auto result = updater.update(record);
                test.expect_true(
                    result.status == (available == 7U
                        ? LegacyActionUpdateStatus::completed
                        : LegacyActionUpdateStatus::malformed_stream) &&
                        record.command_cursor == static_cast<u16>(
                            operand_start + (available == 7U ? 8U : 0U)
                        ),
                    "LF publishes cursor only after seven contiguous reads"
                );
                test.expect_true(
                    available == 7U
                        ? result.return_edx == (command_index == 0U
                              ? 0xA1230009U : 0xA1240007U)
                        : !result.return_edx.has_value(),
                    "LF carries into EDX high word only on normal return"
                );
                for (std::size_t index = 0U; index < fields.size(); ++index) {
                    test.expect_equal(
                        *fields[index],
                        static_cast<u16>(
                            (index < available ? 0x1000U : 0xA500U) + index
                        ),
                        "LF preserves completed stores and unread suffix"
                    );
                }
            }
        }
    }
}

void test_operand_fault_prefixes(openswd3::test::Context& test) {
    const auto check_pair = [&](const u16 command, auto first, auto second) {
        for (const bool present : {false, true}) {
            FakeStreamProvider provider;
            const u16 words[]{command, 0x4567U};
            provider.set_words(std::span{words}.first(present ? 2U : 1U));
            LegacyActionUpdater updater{provider};
            auto record = zero_record();
            make_keys_stable(record);
            record.*first = 0xA111U;
            record.*second = 0xB222U;
            const auto result = updater.update(record);
            test.expect_true(
                result.status == LegacyActionUpdateStatus::malformed_stream &&
                    !result.return_edx.has_value() &&
                    record.command_cursor == (present ? 2U : 1U) &&
                    record.*first == (present ? 0x4567U : 0xA111U) &&
                    record.*second == 0xB222U,
                "paired operand fault retains first store before second read"
            );
        }
    };
    check_pair(
        0x4342U, &LegacyActionRecord::field_68, &LegacyActionRecord::field_74
    );
    check_pair(
        0x4347U, &LegacyActionRecord::field_66, &LegacyActionRecord::field_72
    );
    check_pair(
        0x4352U, &LegacyActionRecord::field_64, &LegacyActionRecord::field_70
    );
    check_pair(
        0x5748U, &LegacyActionRecord::field_2c, &LegacyActionRecord::field_30
    );
    check_pair(
        0x5859U, &LegacyActionRecord::draw_offset_x,
        &LegacyActionRecord::draw_offset_y
    );

    for (const u16 command : std::array<u16, 3>{0x4753U, 0x4C44U, 0x5457U}) {
        for (const bool present : {false, true}) {
            FakeStreamProvider provider;
            provider.set_words(std::span{&command, 1U});
            if (present) {
                provider.bytes.push_back(0xABU);
            }

            LegacyActionUpdater updater{provider};
            auto record = zero_record();
            make_keys_stable(record);
            record.mode_flags = 0x80000000U;
            record.field_8a = 0xA5U;
            record.field_62 = 0x5A5AU;
            record.field_88 = 0x5AU;
            const auto result = updater.update(record);
            const u32 value = command == 0x4753U ? record.field_8a
                : command == 0x4C44U ? record.field_62 : record.field_88;
            const u32 initial = command == 0x4753U ? 0xA5U
                : command == 0x4C44U ? 0x5A5AU : 0x5AU;
            const u32 flags = command == 0x4753U ? 0x80000014U
                : command == 0x4C44U ? 0x80000010U : 0x80000000U;
            test.expect_true(
                result.status == LegacyActionUpdateStatus::malformed_stream &&
                    !result.return_edx.has_value() &&
                    record.command_cursor == (present ? 2U : 1U) &&
                    value == (present ? 0xABU : initial) &&
                    record.mode_flags == flags,
                "byte operand needs one byte and retains mode-before-read prefix"
            );
        }
    }
}

void test_return_edx(openswd3::test::Context& test) {
    struct Case {
        std::vector<u16> words;
        u32 expected;
        u16 wait_override{};
    };
    const Case cases[]{
        {{0x4544U}, 0xA1230001U},
        {{0x4F56U}, 0xA1230001U},
        {{0x4F32U}, 0xA1230000U},
        {{0x4146U, 0x4544U}, 2U},
        {{0x4148U, 0x4544U}, 2U},
        {{0x414EU, 0x4544U}, 2U},
        {{0x414DU, 0x4544U}, 0x80000002U},
        {{0x434CU, 0x4544U}, 0xA1230002U},
        {{0x4E4FU, 0x4544U}, 0xA1230002U},
        {{0x5649U, 0x4544U}, 0xA1230002U},
        {{0x534DU, 0x4544U}, 0xA1230002U},
        {{0x1234U, 0x4544U}, 0xA1230002U},
        {{0x5344U, 5U, 0x4544U}, 0xA1230003U},
        {{0x5344U, 5U, 0x4544U}, 3U, 0x8007U},
        {{0x5756U, 0xFFFFU, 0x4544U}, 0xA1230003U},
    };
    for (const auto& item : cases) {
        FakeStreamProvider provider;
        provider.return_edx = 0xA1235566U;
        provider.set_words(item.words);
        LegacyActionUpdater updater{provider};
        auto record = zero_record();
        make_keys_stable(record);
        record.mode_flags = 0x80000000U;
        record.wait_override = item.wait_override;
        const auto result = updater.update(record, 0xDEADBEEFU);
        test.expect_true(
            result.status == LegacyActionUpdateStatus::completed &&
                result.return_edx == item.expected,
            "EDX follows command writes rather than final action cursor"
        );
    }

    for (const u16 command : std::array<u16, 15>{
             0x4145U, 0x4154U, 0x4158U, 0x4159U, 0x4753U, 0x4C44U,
             0x4F41U, 0x4F58U, 0x4F59U, 0x5041U, 0x5145U, 0x5246U,
             0x524FU, 0x544EU, 0x5457U,
         }) {
        FakeStreamProvider provider;
        const u16 words[]{command, 0xF123U, 0x4544U};
        provider.set_words(words);
        LegacyActionUpdater updater{provider};
        auto record = zero_record();
        make_keys_stable(record);
        const auto result = updater.update(record);
        test.expect_true(
            result.status == LegacyActionUpdateStatus::completed &&
                result.return_edx == 3U,
            "full register writes recover known EDX after unknown loader"
        );
    }

    for (const u16 marker : std::array<u16, 3>{0x4544U, 0x4F56U, 0x4F32U}) {
        FakeStreamProvider provider;
        provider.return_edx = 0xFFFF1234U;
        std::vector<u16> words(0x10000U, 0U);
        words.back() = marker;
        provider.set_words(words);
        LegacyActionUpdater updater{provider};
        auto record = zero_record();
        make_keys_stable(record);
        record.command_cursor = 0xFFFFU;
        const auto result = updater.update(record);
        test.expect_true(
            result.return_edx == (marker == 0x4F32U ? 0xFFFFFFFFU : 0U),
            "marker increment and decrement wrap the complete EDX"
        );
    }

    FakeStreamProvider provider;
    LegacyActionUpdater updater{provider};
    auto record = zero_record();
    test.expect_true(
        updater.update(record, 0xABCD1234U).return_edx == 0xABCD1234U,
        "empty action preserves entry EDX without invoking the loader"
    );
    make_keys_stable(record);
    const u16 marker[]{0x4544U};
    provider.set_words(marker);
    test.expect_true(
        !updater.update(record, 0xABCD1234U).return_edx.has_value(),
        "unknown loader EDX does not inherit caller EDX"
    );
    record.wait_remaining = 3U;
    for (const auto loaded_edx : {0xABCD1234U, 0x9876FEDCU}) {
        provider.return_edx = loaded_edx;
        test.expect_true(
            updater.update(record).return_edx == loaded_edx,
            "waiting reloads and retains this call's loader EDX"
        );
    }

    provider.fail = true;
    test.expect_true(
        updater.update(record).return_edx == provider.return_edx,
        "normal null loader retains its EDX"
    );
    provider.stop = true;
    test.expect_true(
        !updater.update(record, 0xABCD1234U).return_edx.has_value(),
        "stopped loader has no normal return EDX"
    );
}

void test_real_act_provider(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    LegacyActRuntime runtime{root};
    runtime.set_cache_limit(0x00080000U);
    LegacyActActionStreamProvider provider{runtime};
    LegacyActionUpdater updater{provider};

    struct Query {
        u32 action_id;
        u32 variant_index;
    };
    constexpr Query kQueries[]{
        {1U, 0U},
        {3001U, 68U},
        {6001U, 0U},
        {9001U, 0U},
        {12001U, 0U},
        {15001U, 0U},
    };
    for (const Query query : kQueries) {
        LegacyActionRecord record = zero_record();
        record.action_id = query.action_id;
        record.cached_action_id = query.action_id;
        record.base_variant = query.variant_index;
        record.cached_base_variant = query.variant_index;
        const auto updated = updater.update(record);
        test.expect_equal(
            updated.status,
            LegacyActionUpdateStatus::completed,
            "real ACT stream executes safely"
        );
        test.expect_equal(
            updated.return_value,
            1U,
            "real ACT stream returns original success value"
        );
        test.expect_equal(
            record.stream_pointer_32,
            1U,
            "real ACT stream writes non-null token"
        );

        if (query.action_id == 1U) {
            test.expect_equal(
                record.packed_ap_state,
                u16{0x0101U},
                "real NT/AP final packed state"
            );
            test.expect_equal(
                record.field_4a, u16{0x0171U}, "real FR field snapshot"
            );
            test.expect_equal(
                record.wait_default, u16{0U}, "real DS zero wait snapshot"
            );
            test.expect_equal(record.draw_offset_x, 6U, "real YX X snapshot");
            test.expect_equal(
                record.draw_offset_y, 0x47U, "real YX Y snapshot"
            );
            test.expect_equal(record.field_2c, 2U, "real HW first snapshot");
            test.expect_equal(record.field_30, 1U, "real HW second snapshot");
            test.expect_equal(
                record.command_cursor, u16{0U}, "real VO resets cursor"
            );
        }
    }

    updater.set_stream_cache_mode(1U);
    LegacyActionRecord cached = zero_record();
    make_keys_stable(cached);
    const auto first = updater.update(cached);
    const auto second = updater.update(cached);
    test.expect_false(
        first.cache_hit, "first real cached updater query misses"
    );
    test.expect_true(second.cache_hit, "second real cached updater query hits");
    for (const bool use_cache : {false, true}) {
        updater.set_stream_cache_mode(use_cache ? 1U : 0U);
        LegacyActionRecord stopped = zero_record();
        make_keys_stable(stopped);
        stopped.base_variant = 0x7FFFFFFFU;
        stopped.cached_base_variant = stopped.base_variant;
        stopped.stream_pointer_32 = 0x12345678U;
        stopped.wait_remaining = 7U;
        const auto result = updater.update(stopped);
        test.expect_true(
            result.status == LegacyActionUpdateStatus::stream_load_stopped &&
                result.stream_stop.has_value() &&
                result.stream_stop->runtime_status ==
                    openswd3::asset_runtime::LegacyActRuntimeStatus::
                        physical_variant_failed &&
                result.stream_stop->physical_status ==
                    openswd3::asset_runtime::LegacyActVariantStatus::
                        variant_out_of_range &&
                stopped.stream_pointer_32 == 0x12345678U &&
                stopped.wait_remaining == 7U,
            "real direct and cached loaders preserve their failure reasons before pointer store"
        );
    }
    runtime.close();
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_initializer_is_selective(test);
    test_early_returns_and_stream_failure(test);
    test_stream_stop_prefix(test);
    test_key_reset_order_and_wait(test);
    test_field_and_mode_commands(test);
    test_wait_and_terminator_commands(test);
    test_malformed_stream_guard(test);
    test_lf_contiguous_read_and_cursor_commit(test);
    test_return_edx(test);
    test_operand_fault_prefixes(test);
    if (argument_count == 2) {
        test_real_act_provider(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
