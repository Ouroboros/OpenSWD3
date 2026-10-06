#include "openswd3/special_modes/legacy_save_preview.hpp"

#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <new>
#include <string_view>

namespace {

// This executable alone replaces allocations, with injection disabled by default.
thread_local std::size_t allocations_until_failure{};

}  // namespace

void* operator new(const std::size_t size) {
    if (allocations_until_failure != 0U && --allocations_until_failure == 0U) {
        throw std::bad_alloc{};
    }

    if (void* memory = std::malloc(size == 0U ? 1U : size)) {
        return memory;
    }

    throw std::bad_alloc{};
}

void* operator new[](const std::size_t size) {
    return ::operator new(size);
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept {
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

namespace {

using namespace openswd3;
using special_modes::LegacySavePreviewPopulateStatus;
using special_modes::LegacySavePreviewRecord;
using special_modes::populate_legacy_save_preview;

resource_io::LegacySavePreviewPayload make_payload() {
    resource_io::LegacySavePreviewPayload payload;
    constexpr std::string_view date{"202610061234"};
    std::copy(date.begin(), date.end(), payload.timestamp.begin());
    payload.preview[1U] = 0x7CU;
    payload.nul_terminated_label = {'M', 'a', 'p', 0U};
    payload.flags.bytes.resize(11U);
    payload.party.bytes.resize(0x1E4U);
    payload.raw_after_primary[4U] = 0x78U;
    payload.raw_after_primary[5U] = 0x56U;
    payload.raw_after_primary[6U] = 0x34U;
    payload.raw_after_primary[7U] = 0x12U;
    payload.party.bytes[4U] = 0xABU;
    payload.party.bytes[5U] = 0xCDU;
    payload.role_names[13U] = 0x83U;
    payload.elapsed_seconds = 3661U;
    for (std::size_t index = 0U; index < 4U; ++index) {
        auto* role = payload.party.bytes.data() + 0x104U + index * 0x38U;
        role[0x0AU] = static_cast<compat::u8>(index + 1U);
        role[0x0BU] = 0x10U;
        role[0x0CU] = 0x22U;
        role[0x0DU] = 0x20U;
        role[0x0EU] = 0x33U;
        role[0x0FU] = 0x30U;
        role[0x2CU] = static_cast<compat::u8>(0xF0U + index);
    }

    return payload;
}

void test_population(test::Context& test) {
    rendering::LegacyPixelConversionState conversion;
    rendering::select_legacy_pixel_conversion(
        conversion, {0xF800U, 0x07E0U, 0x001FU}
    );
    for (unsigned mask = 0U; mask < 16U; ++mask) {
        for (const auto display_bit : {0U, 8U}) {
            auto payload = make_payload();
            payload.flags.bytes[3U] = static_cast<compat::u8>(mask << 6U);
            payload.flags.bytes[4U] = static_cast<compat::u8>(mask >> 2U);
            payload.flags.bytes[10U] = static_cast<compat::u8>(display_bit);
            LegacySavePreviewRecord record;
            record.bytes.fill(0x5AU);
            test.expect_equal(
                populate_legacy_save_preview(record, payload, conversion),
                LegacySavePreviewPopulateStatus::completed,
                "all sixteen party masks and both display types populate"
            );
            for (std::size_t index = 0U; index < 4U; ++index) {
                const auto* actual = record.bytes.data() + index * 8U;
                if ((mask & (1U << index)) == 0U) {
                    test.expect_true(
                        actual[0U] == 0xFFU && actual[1U] == 0xFFU &&
                            std::all_of(
                                actual + 2U,
                                actual + 8U,
                                [](auto value) { return value == 0x5AU; }
                            ),
                        "absent role changes only its first word"
                    );
                } else {
                    const std::array<compat::u8, 8U> expected{
                        static_cast<compat::u8>(index + 1U),
                        0x10U,
                        0x22U,
                        0x20U,
                        0x33U,
                        0x30U,
                        static_cast<compat::u8>(0xF0U + index),
                        0U,
                    };
                    test.expect_true(
                        std::equal(expected.begin(), expected.end(), actual),
                        "present role copies three words and zero-extends level"
                    );
                }
            }

            constexpr std::string_view expected_date{"2026/10/06 12:34"};
            test.expect_true(
                record.timestamp.size() == 17U &&
                    std::equal(
                        expected_date.begin(),
                        expected_date.end(),
                        record.timestamp.begin()
                    ) &&
                    record.timestamp[16U] == 0U &&
                    record.map_name == payload.nul_terminated_label &&
                    record.role_names.size() == 0x40U &&
                    record.role_names[13U] == 0x83U,
                "timestamp, map name and raw role names retain their bytes"
            );
            test.expect_true(
                record.pixels.size() == 0x4B00U &&
                    record.pixels[0U] == 0xF800U &&
                    record.bytes[0x20U] == 0x78U &&
                    record.bytes[0x23U] == 0x12U &&
                    record.bytes[0x24U] == (display_bit == 0U ? 2U : 3U) &&
                    record.bytes[0x25U] == 0U && record.bytes[0x28U] == 0xABU &&
                    record.bytes[0x29U] == 0xCDU && record.bytes[0x30U] == 2U &&
                    record.bytes[0x31U] == 0U && record.bytes[0x32U] == 0x5AU,
                "pixel conversion and exact-width record writes match LST"
            );
            const std::array<compat::u8, 13U> expected_time{
                '0',
                '1',
                0xAEU,
                0xC9U,
                '0',
                '1',
                0xA4U,
                0xC0U,
                '0',
                '1',
                0xACU,
                0xEDU,
                0U,
            };
            test.expect_true(
                record.play_time.size() == 0x40U &&
                    std::equal(
                        expected_time.begin(),
                        expected_time.end(),
                        record.play_time.begin()
                    ),
                "play time retains the three original encoded suffixes"
            );
        }
    }
}

void test_stops_and_reset(test::Context& test) {
    for (const std::size_t flag_size : {0U, 4U, 5U, 10U}) {
        auto payload = make_payload();
        payload.flags.bytes.resize(flag_size);
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        test.expect_equal(
            populate_legacy_save_preview(record, payload, {}),
            LegacySavePreviewPopulateStatus::flags_truncated,
            "stop at the first unavailable flag byte"
        );
        const std::size_t changed = flag_size >= 5U ? 4U
            : flag_size >= 4U                       ? 2U
                                                    : 0U;
        for (std::size_t index = 0U; index < 4U; ++index) {
            test.expect_equal(
                record.bytes[index * 8U],
                static_cast<compat::u8>(index < changed ? 0xFFU : 0x5AU),
                "flags stop retains only preceding absent-role writes"
            );
        }

        test.expect_true(
            record.timestamp.size() == 17U && !record.pixels.empty() &&
                !record.map_name.empty() && record.role_names.empty() &&
                record.play_time.empty() && record.bytes[0x24U] == 0x5AU &&
                record.bytes[0x30U] == 0x5AU,
            "flags stop retains the first three resources without ready status"
        );
        special_modes::reset_legacy_save_preview(record);
        test.expect_true(
            record.pixels.capacity() == 0U &&
                record.timestamp.capacity() == 0U &&
                record.map_name.capacity() == 0U &&
                record.role_names.capacity() == 0U &&
                record.play_time.capacity() == 0U &&
                std::all_of(
                    record.bytes.begin(),
                    record.bytes.end(),
                    [](auto value) { return value == 0U; }
                ),
            "reset frees resources and clears all scalar storage"
        );
    }

    for (const std::size_t party_size : {7U, 8U, 0x1E3U}) {
        auto payload = make_payload();
        payload.party.bytes.resize(party_size);
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        test.expect_true(
            populate_legacy_save_preview(record, payload, {}) ==
                    LegacySavePreviewPopulateStatus::party_truncated &&
                record.bytes[0x20U] == 0x78U && record.bytes[0x24U] == 2U &&
                record.bytes[0x28U] == (party_size < 8U ? 0x5AU : 0xABU) &&
                record.bytes[0x30U] == 0x5AU && record.role_names.empty(),
            "party truncation retains preceding scalar writes only"
        );
    }

    auto bad_label = make_payload();
    bad_label.nul_terminated_label.pop_back();
    LegacySavePreviewRecord stopped;
    test.expect_true(
        populate_legacy_save_preview(stopped, bad_label, {}) ==
                LegacySavePreviewPopulateStatus::label_unterminated &&
            !stopped.timestamp.empty() && !stopped.pixels.empty() &&
            stopped.map_name.empty() && stopped.bytes[0x30U] == 0U,
        "unterminated label preserves date and pixels without completion"
    );

    auto payload = make_payload();
    payload.flags.bytes[3U] = 0xC0U;
    payload.flags.bytes[4U] = 3U;
    LegacySavePreviewRecord record;
    record.bytes[0U] = record.bytes[1U] = 0xFFU;
    record.bytes[2U] = 0xA5U;
    test.expect_true(
        populate_legacy_save_preview(record, payload, {}) ==
                LegacySavePreviewPopulateStatus::completed &&
            record.bytes[0U] == 0xFFU && record.bytes[2U] == 0xA5U,
        "existing absent sentinel is not overwritten by a present party bit"
    );
    payload.elapsed_seconds = 0xFFFFFFFFU;
    test.expect_equal(
        populate_legacy_save_preview(record, payload, {}),
        LegacySavePreviewPopulateStatus::completed,
        "maximum unsigned play time remains valid"
    );
    const std::array<compat::u8, 18U> expected{
        '1',
        '1',
        '9',
        '3',
        '0',
        '4',
        '6',
        0xAEU,
        0xC9U,
        '2',
        '8',
        0xA4U,
        0xC0U,
        '1',
        '5',
        0xACU,
        0xEDU,
        0U,
    };
    test.expect_true(
        std::equal(expected.begin(), expected.end(), record.play_time.begin()),
        "hours are minimum width two, not truncated or signed"
    );
}

void test_read_failure_prefixes(test::Context& test) {
#ifdef OPENSWD3_GAME_DATA_ROOT
    std::ifstream file(
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT} / "Save" / "0.sav",
        std::ios::binary
    );
    std::vector<compat::u8> source{
        std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}
    };
    test.expect_true(source.size() > 0x9634U, "real preview source exists");
    if (source.size() <= 0x9634U) {
        return;
    }

    const auto word = [&source](const std::size_t offset) {
        return static_cast<std::size_t>(source[offset]) |
            (static_cast<std::size_t>(source[offset + 1U]) << 8U) |
            (static_cast<std::size_t>(source[offset + 2U]) << 16U) |
            (static_cast<std::size_t>(source[offset + 3U]) << 24U);
    };
    const auto primary = 0x962CU + 8U + word(0x962CU);
    const auto raw = primary + 8U + word(primary);
    const auto party = raw + 0x1CU;
    const auto names = party + 8U + word(party);
    // Make conversion distinguishable even if the original first pixel is black.
    source[12U] = 0U;
    source[13U] = 0x7CU;
    rendering::LegacyPixelConversionState conversion;
    rendering::select_legacy_pixel_conversion(
        conversion, {0xF800U, 0x07E0U, 0x001FU}
    );
    using Stage = resource_io::LegacySavePreviewReadStage;
    struct Stop {
        std::size_t end;
        Stage stage;
        std::size_t resources;
    };
    const std::array stops{
        Stop{0U, Stage::timestamp, 1U},
        Stop{18U, Stage::pixels, 2U},
        Stop{0x960CU, Stage::label, 2U},
        Stop{0x962CU + 7U, Stage::flags, 3U},
        Stop{primary + 7U, Stage::primary, 3U},
        Stop{raw + 7U, Stage::primary, 3U},
        Stop{raw + 8U, Stage::party, 3U},
        Stop{party + 7U, Stage::party, 3U},
        Stop{names + 6U, Stage::role_names, 4U},
        Stop{names + 0x43U, Stage::elapsed_seconds, 4U},
    };
    for (const auto& stop : stops) {
        const auto parsed = resource_io::read_legacy_save_preview_payload(
            std::span<const compat::u8>{source}.first(stop.end)
        );
        test.expect_equal(
            parsed.next_read, stop.stage, "exact read stop stage"
        );
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        test.expect_equal(
            populate_legacy_save_preview(
                record, parsed.payload, conversion, parsed.next_read
            ),
            LegacySavePreviewPopulateStatus::payload_unavailable,
            "file-read stop cannot publish a completed preview"
        );
        const auto count = static_cast<std::size_t>(!record.timestamp.empty()) +
            static_cast<std::size_t>(!record.pixels.empty()) +
            static_cast<std::size_t>(!record.map_name.empty()) +
            static_cast<std::size_t>(!record.role_names.empty()) +
            static_cast<std::size_t>(!record.play_time.empty());
        test.expect_equal(
            count, stop.resources, "only preceding resources exist"
        );
        test.expect_equal(
            record.bytes[0x30U],
            compat::u8{0x5AU},
            "stopped preview preserves the old status word"
        );
        if (stop.stage == Stage::pixels) {
            test.expect_true(
                parsed.payload.preview_bytes_read == 4U &&
                    record.pixels[0U] == 0x7C00U,
                "partial REP MOVSD copies complete dwords without conversion"
            );
        } else if (stop.stage > Stage::pixels) {
            test.expect_equal(
                record.pixels[0U],
                compat::u16{0xF800U},
                "complete pixel copy is converted before later stop"
            );
        }

        if (stop.stage >= Stage::party) {
            test.expect_true(
                std::equal(
                    source.data() + raw + 4U,
                    source.data() + raw + 8U,
                    record.bytes.data() + 0x20U
                ),
                "map value survives a missing party header or body"
            );
        } else {
            test.expect_equal(
                record.bytes[0x20U],
                compat::u8{0x5AU},
                "earlier stop does not write the map value"
            );
        }

        if (stop.stage == Stage::role_names) {
            test.expect_true(
                parsed.payload.role_name_bytes_read == 4U &&
                    std::equal(
                        source.data() + names,
                        source.data() + names + 4U,
                        record.role_names.data()
                    ),
                "name copy retains complete dwords before its stop"
            );
        }
    }

    const auto complete = resource_io::read_legacy_save_preview_payload(source);
    for (const auto header : {std::size_t{0x962CU}, party}) {
        const auto actual_size = header == 0x962CU
            ? complete.payload.flags.bytes.size()
            : complete.payload.party.bytes.size();
        for (const auto declared :
             {actual_size + 1U, (actual_size + 1U) / 2U}) {
            auto changed = source;
            for (std::size_t index = 0U; index < 4U; ++index) {
                changed[header + 4U + index] =
                    static_cast<compat::u8>(declared >> (index * 8U));
            }

            const auto parsed =
                resource_io::read_legacy_save_preview_payload(changed);
            LegacySavePreviewRecord record;
            test.expect_true(
                parsed.status ==
                        resource_io::LegacySaveContainerStatus::ready &&
                    parsed.payload.flags.bytes ==
                        complete.payload.flags.bytes &&
                    parsed.payload.party.bytes ==
                        complete.payload.party.bytes &&
                    populate_legacy_save_preview(
                        record, parsed.payload, conversion, parsed.next_read
                    ) == LegacySavePreviewPopulateStatus::completed,
                "preview does not compare decoded count against the declared size"
            );
        }
    }

    for (const auto header : {std::size_t{0x962CU}, party}) {
        auto padded = source;
        const auto end = header + 8U + word(header);
        padded.insert(padded.begin() + static_cast<std::ptrdiff_t>(end), 0xA5U);
        for (std::size_t index = 0U; index < 4U; ++index) {
            padded[header + index] =
                static_cast<compat::u8>((word(header) + 1U) >> (index * 8U));
        }

        const auto parsed =
            resource_io::read_legacy_save_preview_payload(padded);
        test.expect_true(
            parsed.status == resource_io::LegacySaveContainerStatus::ready &&
                parsed.payload.flags.bytes == complete.payload.flags.bytes &&
                parsed.payload.party.bytes == complete.payload.party.bytes,
            "native minus-eight return after EOF does not reject a preview"
        );

        auto shortened = source;
        for (std::size_t index = 0U; index < 4U; ++index) {
            shortened[header + index] =
                static_cast<compat::u8>((word(header) - 1U) >> (index * 8U));
        }

        const auto overread =
            resource_io::read_legacy_save_preview_payload(shortened);
        if (header == 0x962CU) {
            LegacySavePreviewRecord record;
            record.bytes.fill(0x5AU);
            test.expect_true(
                overread.next_read == Stage::primary &&
                    overread.payload.flags.bytes ==
                        complete.payload.flags.bytes &&
                    populate_legacy_save_preview(
                        record, overread.payload, conversion, overread.next_read
                    ) == LegacySavePreviewPopulateStatus::payload_unavailable &&
                    (record.bytes[0x24U] == 2U || record.bytes[0x24U] == 3U) &&
                    record.bytes[0x20U] == 0x5AU,
                "minus-four decoder return precedes the later primary-cursor stop"
            );
        } else {
            test.expect_true(
                overread.status ==
                        resource_io::LegacySaveContainerStatus::ready &&
                    overread.payload.party.bytes ==
                        complete.payload.party.bytes &&
                    std::equal(
                        source.data() + names - 1U,
                        source.data() + names + 0x3FU,
                        overread.payload.role_names.data()
                    ),
                "decoder may reach EOF past the declared end inside the mapped file"
            );
        }

        auto wrapped = source;
        std::fill_n(wrapped.data() + header + 4U, 4U, 0U);
        wrapped[header + 7U] = 0x80U;
        test.expect_equal(
            resource_io::read_legacy_save_preview_payload(wrapped).status,
            resource_io::LegacySaveContainerStatus::decompression_failed,
            "twice 0x80000000 wraps to zero capacity before the checked output stop"
        );
    }

    LegacySavePreviewRecord reference;
    reference.bytes.fill(0x5AU);
    test.expect_equal(
        populate_legacy_save_preview(reference, complete.payload, conversion),
        LegacySavePreviewPopulateStatus::completed,
        "reference preview reaches the final status write"
    );
    const auto remaining = source.size() - party - 8U;
    struct NameBoundary {
        std::size_t packed_size;
        std::size_t copied;
        Stage stage;
    };
    const std::array name_boundaries{
        NameBoundary{remaining - 68U, 64U, Stage::complete},
        NameBoundary{remaining - 67U, 64U, Stage::elapsed_seconds},
        NameBoundary{remaining - 64U, 64U, Stage::elapsed_seconds},
        NameBoundary{remaining - 63U, 60U, Stage::role_names},
        NameBoundary{remaining - 4U, 4U, Stage::role_names},
        NameBoundary{remaining - 3U, 0U, Stage::role_names},
        NameBoundary{remaining, 0U, Stage::role_names},
        NameBoundary{remaining + 1U, 0U, Stage::role_names},
        NameBoundary{0x7FFFFFFFU, 0U, Stage::role_names},
        NameBoundary{0xFFFFFFFFU, 64U, Stage::complete},
        NameBoundary{
            static_cast<compat::u32>(0U - static_cast<compat::u32>(party + 8U)),
            64U,
            Stage::complete,
        },
    };
    for (const auto& boundary : name_boundaries) {
        auto changed = source;
        for (std::size_t index = 0U; index < 4U; ++index) {
            changed[party + index] = static_cast<compat::u8>(
                boundary.packed_size >> (index * 8U)
            );
        }

        const auto parsed =
            resource_io::read_legacy_save_preview_payload(changed);
        const bool completed = boundary.stage == Stage::complete;
        const auto expected_read = completed
            ? resource_io::LegacySaveContainerStatus::ready
            : resource_io::LegacySaveContainerStatus::truncated;
        const auto expected_populate = completed
            ? LegacySavePreviewPopulateStatus::completed
            : LegacySavePreviewPopulateStatus::payload_unavailable;
        test.expect_true(
            parsed.status == expected_read &&
                parsed.next_read == boundary.stage &&
                parsed.payload.party.bytes == complete.payload.party.bytes &&
                parsed.payload.role_name_bytes_read == boundary.copied,
            "name mapping boundary retains decoded party and complete dwords"
        );
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        record.role_names.assign(0x40U, 0xA5U);
        test.expect_true(
            populate_legacy_save_preview(
                record, parsed.payload, conversion, parsed.next_read
            ) == expected_populate &&
                std::equal(
                    record.bytes.data(), record.bytes.data() + 0x2CU,
                    reference.bytes.data()
                ) &&
                record.pixels[0U] == 0xF800U &&
                record.role_names.size() == 0x40U &&
                record.play_time.empty() == !completed &&
                record.bytes[0x30U] == (completed ? 2U : 0x5AU) &&
                record.bytes[0x31U] == (completed ? 0U : 0x5AU),
            "name stop keeps prior record writes and does not publish completion"
        );
        if (boundary.copied != 0U) {
            const auto name_offset = static_cast<compat::u32>(
                party + 8U + boundary.packed_size
            );
            const auto* expected = source.data() + name_offset;
            test.expect_true(
                std::equal(
                    expected, expected + boundary.copied,
                    record.role_names.data()
                ),
                "only mapped complete name dwords are copied"
            );
        }

        test.expect_true(
            std::all_of(
                record.role_names.data() + boundary.copied,
                record.role_names.data() + record.role_names.size(),
                [](const compat::u8 value) { return value == 0xA5U; }
            ),
            "name bytes after the stopped dword retain their previous contents"
        );
    }

    auto oversized = source;
    std::fill_n(oversized.data() + 0x962CU, 4U, 0xFFU);
    const auto outside =
        resource_io::read_legacy_save_preview_payload(oversized);
    test.expect_true(
        outside.next_read == Stage::primary &&
            outside.payload.flags.bytes == complete.payload.flags.bytes,
        "oversized packed extent is checked at the next actual mapped read"
    );

    const std::array allocation_stages{
        Stage::label, Stage::flags, Stage::party
    };
    for (std::size_t index = 0U; index < allocation_stages.size(); ++index) {
        allocations_until_failure = index + 1U;
        const auto parsed =
            resource_io::read_legacy_save_preview_payload(source);
        const bool injected = allocations_until_failure == 0U;
        allocations_until_failure = 0U;
        test.expect_true(
            injected &&
                parsed.status ==
                    resource_io::LegacySaveContainerStatus::allocation_failed &&
                parsed.next_read == allocation_stages[index],
            "allocation exception returns its actual read stage"
        );
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        test.expect_true(
            populate_legacy_save_preview(
                record, parsed.payload, conversion, parsed.next_read
            ) == LegacySavePreviewPopulateStatus::payload_unavailable &&
                record.timestamp.size() == 17U &&
                record.pixels.size() == 0x4B00U &&
                record.map_name.empty() == (index == 0U) &&
                record.role_names.empty() && record.play_time.empty() &&
                record.bytes[0x30U] == 0x5AU,
            "parser allocation failure retains the preceding record resources"
        );
    }

    for (const auto offset : {std::size_t{0x962CU}, party}) {
        auto corrupt = source;
        constexpr std::array<compat::u8, 4U> invalid_back_reference{
            0x12U, 0xA5U, 0x40U, 0xFFU
        };
        std::copy(
            invalid_back_reference.begin(),
            invalid_back_reference.end(),
            corrupt.data() + offset + 8U
        );
        const auto parsed =
            resource_io::read_legacy_save_preview_payload(corrupt);
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        test.expect_true(
            parsed.status ==
                    resource_io::LegacySaveContainerStatus::
                        decompression_failed &&
                populate_legacy_save_preview(
                    record, parsed.payload, conversion, parsed.next_read
                ) == LegacySavePreviewPopulateStatus::payload_unavailable &&
                record.timestamp.size() == 17U &&
                record.pixels.size() == 0x4B00U && !record.map_name.empty() &&
                record.role_names.empty() && record.play_time.empty() &&
                record.bytes[0x30U] == 0x5AU,
            "decompression stop retains date, pixels and map-name resources"
        );
    }
#else
    static_cast<void>(test);
#endif
}

void test_record_allocation_failures(test::Context& test) {
    auto payload = make_payload();
    payload.flags.bytes[3U] = 0xC0U;
    payload.flags.bytes[4U] = 3U;
    constexpr std::array<std::size_t, 6U> retained{0U, 1U, 2U, 3U, 3U, 4U};
    for (std::size_t allocation = 1U; allocation <= 6U; ++allocation) {
        LegacySavePreviewRecord record;
        record.bytes.fill(0x5AU);
        allocations_until_failure = allocation;
        const auto result = populate_legacy_save_preview(record, payload, {});
        const bool injected = allocations_until_failure == 0U;
        allocations_until_failure = 0U;
        test.expect_true(
            injected &&
                result == LegacySavePreviewPopulateStatus::allocation_failed,
            "five resources and the temporary role snapshot can stop the loader"
        );
        const auto count = static_cast<std::size_t>(!record.timestamp.empty()) +
            static_cast<std::size_t>(!record.pixels.empty()) +
            static_cast<std::size_t>(!record.map_name.empty()) +
            static_cast<std::size_t>(!record.role_names.empty()) +
            static_cast<std::size_t>(!record.play_time.empty());
        test.expect_true(
            count == retained[allocation - 1U] && record.bytes[0x30U] == 0x5AU,
            "allocation failure retains preceding resources without completion"
        );
        if (allocation == 4U) {
            test.expect_true(
                record.bytes[0x28U] == 0xABU && record.bytes[0U] == 0x5AU,
                "role snapshot allocation follows +0x28 and precedes summaries"
            );
        } else if (allocation >= 5U) {
            test.expect_equal(
                record.bytes[0U],
                compat::u8{1U},
                "later allocations retain the completed summaries"
            );
        }
    }
}

void test_action_refresh(test::Context& test) {
    constexpr std::array<std::array<compat::i32, 2U>, 11U> cases{{
        {-4, -1},
        {-3, 0},
        {-2, -2},
        {-1, -1},
        {0, 0},
        {1, 1},
        {2, 2},
        {8, 2},
        {98, 2},
        {-2147483647 - 1, -2},
        {2147483647, 1},
    }};
    for (const auto& sample : cases) {
        special_modes::LegacyInputMenuSavePreviewResetState state;
        state.selected_save_slot = sample[0U];
        for (auto& action : state.preview_actions) {
            action.mode_flags = 0x12345678U;
            action.field_94 = 0xA5A5A5A5U;
            action.cached_action_id = 77U;
            action.command_cursor = 19U;
            action.wait_default = 23U;
            action.external_mode = 5U;
        }

        special_modes::refresh_legacy_save_preview_actions(state);
        constexpr std::array<compat::u32, 4U> ids{1U, 2U, 8U, 17U};
        for (std::size_t index = 0U; index < 12U; ++index) {
            const auto& action = state.preview_actions[index];
            const auto column = static_cast<compat::i32>(index / 4U);
            test.expect_true(
                action.action_id == ids[index % 4U] &&
                    action.base_variant == (column == sample[1U] ? 8U : 0U) &&
                    action.variant_delta == 6U &&
                    action.field_1c == 0xFFFFFFFFU &&
                    action.one_shot_base_variant == 0xFFFFFFFFU &&
                    action.one_shot_variant_delta == 0xFFFFFFFFU &&
                    action.command_cursor == 0U && action.wait_default == 0U &&
                    action.external_mode == 0U,
                "twelve actions preserve signed remainder and initializer order"
            );
            test.expect_true(
                action.mode_flags == 0x12345678U &&
                    action.field_94 == 0xA5A5A5A5U &&
                    action.cached_action_id == 77U,
                "preview refresh is not a whole-record value initialization"
            );
        }
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_population(test);
    test_stops_and_reset(test);
    test_action_refresh(test);
    test_read_failure_prefixes(test);
    test_record_allocation_failures(test);
    return test.exit_code();
}
