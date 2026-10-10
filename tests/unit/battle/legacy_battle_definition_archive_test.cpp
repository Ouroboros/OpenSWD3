#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#endif

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleDefinitionArchiveFiles;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderLoadStatus;
using openswd3::battle::LegacyBattleDefinitionArchiveRecord;
using openswd3::battle::LegacyBattleDefinitionArchiveRecordLoadStatus;
using openswd3::battle::LegacyBattleRenderGeometryBindingObject;
using openswd3::compat::u8;
using openswd3::compat::u32;

void write_u16(std::vector<u8>& bytes, const u32 offset, const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(std::vector<u8>& bytes, const u32 offset, const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

class ArchiveTestFiles {
public:
    ArchiveTestFiles() {
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("battle-archive-" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()
             ));
        std::filesystem::create_directories(root_);
    }

    ~ArchiveTestFiles() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path path(const char* name) const {
        return root_ / name;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream file{path(name), std::ios::binary | std::ios::trunc};
        file.exceptions(std::ios::failbit | std::ios::badbit);
        for (const u8 byte : bytes) {
            file.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

void test_header_reads(openswd3::test::Context& test) {
    const ArchiveTestFiles fixtures;
    for (const u32 count : {0U, 1U, 3U, 0x2714U}) {
        const std::vector<u8> data(count, 0xA5U);
        fixtures.write("header.ffd", data);
        LegacyBattleDefinitionArchiveFiles files;
        LegacyBattleRenderGeometryBindingObject object;
        object.render_geometry_owner_token = 0x12345678U;
        object.battle_header_bytes.fill(0xCCU);
        object.reserved_2718_3103.fill(0x5AU);
        object.index_records[0].ordinal = 0xABCDEF01U;
        u32 published = 0xFACEU;
        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_header(
                object, published, files, fixtures.path("header.ffd")
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveHeaderLoadStatus::completed &&
                result.bytes_read == count && published == 0x1F48U &&
                std::equal(
                    data.begin(), data.end(), object.battle_header_bytes.begin()
                ) &&
                std::all_of(
                    object.battle_header_bytes.begin() + count,
                    object.battle_header_bytes.end(),
                    [](const u8 value) { return value == 0xCCU; }
                ) &&
                std::ranges::all_of(
                    object.reserved_2718_3103,
                    [](const u8 value) { return value == 0x5AU; }
                ) &&
                object.index_records[0].ordinal == 0xABCDEF01U &&
                object.render_geometry_owner_token == 0x12345678U &&
                files.size() == 0U,
            "actual header reads preserve the untouched suffix and surrounding object"
        );
    }

    LegacyBattleDefinitionArchiveFiles files;
    LegacyBattleRenderGeometryBindingObject object;
    object.battle_header_bytes.fill(0xA5U);
    u32 published = 0xFACEU;
    const auto missing =
        openswd3::battle::load_legacy_battle_definition_archive_header(
            object, published, files, fixtures.path("missing.ffd")
        );
    test.expect_true(
        missing.status ==
                LegacyBattleDefinitionArchiveHeaderLoadStatus::open_failed &&
            missing.bytes_read == 0U && published == 0xFACEU &&
            files.size() == 0U && !files.close(nullptr) &&
            std::ranges::all_of(
                object.battle_header_bytes,
                [](const u8 value) { return value == 0xA5U; }
            ),
        "failed open preserves the header and published offset without owning a file"
    );

#ifdef __linux__
    std::filesystem::create_directory(fixtures.path("read-error"));
    const auto failed_read =
        openswd3::battle::load_legacy_battle_definition_archive_header(
            object, published, files, fixtures.path("read-error")
        );
    test.expect_true(
        failed_read.status ==
                LegacyBattleDefinitionArchiveHeaderLoadStatus::completed &&
            failed_read.bytes_read == 0U && published == 0x1F48U &&
            files.size() == 0U &&
            std::ranges::all_of(
                object.battle_header_bytes,
                [](const u8 value) { return value == 0xA5U; }
            ),
        "an actual failed read still publishes the index and closes the opened directory"
    );
#endif

#ifdef _WIN32
    const auto path = fixtures.path("header.ffd");
    const auto permissions = std::filesystem::status(path).permissions();
    const auto write_bits = std::filesystem::perms::owner_write |
        std::filesystem::perms::group_write |
        std::filesystem::perms::others_write;
    std::filesystem::permissions(
        path, write_bits, std::filesystem::perm_options::remove
    );
    const auto readonly = std::filesystem::status(path).permissions();
    const auto loaded =
        openswd3::battle::load_legacy_battle_definition_archive_header(
            object, published, files, path
        );
    const auto after = std::filesystem::status(path).permissions();
    std::filesystem::permissions(path, permissions);
    test.expect_true(
        (readonly & write_bits) == std::filesystem::perms::none &&
            after == readonly && loaded.bytes_read == 0x2714U,
        "direct archive open preserves the Windows read-only attribute"
    );
#endif
}

void test_record_reads(openswd3::test::Context& test) {
    const ArchiveTestFiles fixtures;
    for (const u32 count : {0U, 1U, 3U, 0x10CU}) {
        std::vector<u8> data(0x2714U, 0U);
        data[0x1F45U] = 1U;
        data.insert(data.end(), count, 0xA5U);
        fixtures.write("record.ffd", data);
        LegacyBattleDefinitionArchiveFiles files;
        LegacyBattleRenderGeometryBindingObject object;
        LegacyBattleDefinitionArchiveRecord record;
        record.bytes.fill(0xCCU);
        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object, record, files, fixtures.path("record.ffd"), 0x10001U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::completed &&
                result.battle_index == 1U &&
                result.prefix_bytes_read == 0x2714U &&
                result.record_bytes_read == count &&
                result.file_offset == 0x2714U && files.size() == 0U &&
                std::all_of(
                    record.bytes.begin(),
                    record.bytes.begin() + count,
                    [](const u8 value) { return value == 0xA5U; }
                ) &&
                std::all_of(
                    record.bytes.begin() + count,
                    record.bytes.end(),
                    [](const u8 value) { return value == 0xCCU; }
                ),
            "actual record reads use the low WORD id and keep the unread tail"
        );
    }

    for (const u8 count : {u8{0U}, u8{0x80U}, u8{0xFFU}, u8{1U}, u8{127U}}) {
        for (const u8 variant :
             {u8{0U}, u8{1U}, u8{2U}, u8{127U}, u8{0x80U}, u8{0xFFU}}) {
            std::vector<u8> data(0x2715U, 0U);
            data[0x1F45U] = count;
            data[0x2714U] = 0x79U;
            fixtures.write("gates.ffd", data);
            LegacyBattleDefinitionArchiveFiles files;
            LegacyBattleRenderGeometryBindingObject object;
            LegacyBattleDefinitionArchiveRecord record;
            record.bytes.fill(0xCCU);
            const auto result =
                openswd3::battle::load_legacy_battle_definition_archive_record(
                    object,
                    record,
                    files,
                    fixtures.path("gates.ffd"),
                    1U,
                    variant
                );
            const auto signed_count =
                std::bit_cast<openswd3::compat::i8>(count);
            const auto signed_variant =
                std::bit_cast<openswd3::compat::i8>(variant);
            if (signed_count <= 0 || signed_variant > signed_count) {
                const auto expected = signed_count <= 0
                    ? LegacyBattleDefinitionArchiveRecordLoadStatus::
                          rejected_count
                    : LegacyBattleDefinitionArchiveRecordLoadStatus::
                          rejected_variant;
                test.expect_true(
                    result.status == expected && files.size() == 0U &&
                        std::ranges::all_of(
                            record.bytes,
                            [](const u8 value) { return value == 0xCCU; }
                        ),
                    "signed rejection closes the actual file and preserves the record"
                );
            } else if (variant == 0x80U) {
                test.expect_true(
                    result.status ==
                            LegacyBattleDefinitionArchiveRecordLoadStatus::
                                offset_table_typed_stop &&
                        result.combined_record_index == 0xFFFFFF80U &&
                        files.size() == 1U && record.bytes.front() == 0xCCU,
                    "negative out-of-object offset retains its file and stops before record replacement"
                );
            } else {
                test.expect_true(
                    result.status ==
                            LegacyBattleDefinitionArchiveRecordLoadStatus::
                                completed &&
                        result.record_bytes_read == 1U &&
                        record.bytes[0U] == 0x79U &&
                        record.bytes[1U] == 0xCCU && files.size() == 0U,
                    "equal and negative variants continue when their wrapped table address is valid"
                );
            }
        }
    }

    for (const u32 offset : {0x00800000U, 1U}) {
        std::vector<u8> data(0x2714U, 0U);
        data[0x1F45U] = 1U;
        write_u32(data, 4U, offset);
        data.insert(data.end(), {0x10U, 0x20U, 0x30U});
        fixtures.write("seek.ffd", data);
        LegacyBattleDefinitionArchiveFiles files;
        LegacyBattleRenderGeometryBindingObject object;
        LegacyBattleDefinitionArchiveRecord record;
        record.bytes.fill(0xCCU);
        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object, record, files, fixtures.path("seek.ffd"), 1U, 0U
            );
        const bool failed_seek = offset == 0x00800000U;
        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::completed &&
                result.file_offset == (failed_seek ? 0x86002714U : 0x2820U) &&
                result.record_bytes_read == (failed_seek ? 3U : 0U) &&
                record.bytes[0U] == (failed_seek ? 0x10U : 0xCCU) &&
                record.bytes[3U] == 0xCCU && files.size() == 0U,
            "failed negative seek keeps the cursor while seeking beyond EOF keeps old record bytes"
        );
    }

    LegacyBattleDefinitionArchiveFiles files;
    LegacyBattleRenderGeometryBindingObject object;
    LegacyBattleDefinitionArchiveRecord record;
    record.bytes.fill(0xCCU);
    const auto missing =
        openswd3::battle::load_legacy_battle_definition_archive_record(
            object, record, files, fixtures.path("missing.ffd"), 1U, 0U
        );
    test.expect_true(
        missing.status ==
                LegacyBattleDefinitionArchiveRecordLoadStatus::open_failed &&
            files.size() == 0U &&
            std::ranges::all_of(
                record.bytes, [](const u8 value) { return value == 0xCCU; }
            ),
        "failed record open preserves the actual destination"
    );

    for (const u32 count : {0U, 3U}) {
        fixtures.write("short.ffd", std::vector<u8>(count, 0U));
        object.battle_header_bytes[0x1F45U] = 1U;
        const auto rejected =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object, record, files, fixtures.path("short.ffd"), 1U, 2U
            );
        test.expect_true(
            rejected.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        rejected_variant &&
                rejected.prefix_bytes_read == count && files.size() == 0U &&
                record.bytes[0U] == 0xCCU,
            "short headers retain the live count used by the signed variant gate"
        );
    }
}

void test_signed_selection_and_decode(openswd3::test::Context& test) {
    const ArchiveTestFiles fixtures;
    std::vector<u8> data(0x292CU + 0xF2U, 0U);
    data[0x1F47U] = 2U;
    data[0x1F45U] = 0xFEU;
    data[0x1F46U] = 3U;
    write_u32(data, 4U, 2U);
    write_u32(data, 0x292CU + 0x04U, 0xFFFFFFFCU);
    write_u16(data, 0x292CU + 0x24U, 5U);
    write_u16(data, 0x292CU + 0x28U, 0x1234U);
    write_u32(data, 0x292CU + 0x58U, 0x11112222U);
    write_u32(data, 0x292CU + 0x78U, 0x33334444U);
    write_u16(data, 0x292CU + 0x98U, 2U);
    write_u16(data, 0x292CU + 0x9CU, 11U);
    write_u16(data, 0x292CU + 0xBCU, 1U);
    write_u16(data, 0x292CU + 0xCCU, 100U);
    write_u16(data, 0x292CU + 0xECU, 200U);
    write_u16(data, 0x292CU + 0xA0U, 12U);
    write_u16(data, 0x292CU + 0xD0U, 300U);
    write_u16(data, 0x292CU + 0xF0U, 400U);
    fixtures.write("signed.ffd", data);
    LegacyBattleDefinitionArchiveFiles files;
    LegacyBattleRenderGeometryBindingObject object;
    LegacyBattleDefinitionArchiveRecord record;
    record.bytes.fill(0xCCU);
    const auto result =
        openswd3::battle::load_legacy_battle_definition_archive_record(
            object, record, files, fixtures.path("signed.ffd"), 3U, 0xFFU
        );
    const auto definition =
        openswd3::battle::decode_legacy_battle_definition(record);
    test.expect_true(
        result.status ==
                LegacyBattleDefinitionArchiveRecordLoadStatus::completed &&
            result.record_bytes_read == 0xF2U &&
            result.signed_prefix_sum == 1U &&
            result.combined_record_index == 0U &&
            result.record_offset_value == 2U && result.file_offset == 0x292CU &&
            record.bytes[0xF2U] == 0xCCU && record.bytes.back() == 0xCCU &&
            files.size() == 0U &&
            definition.background_resource == 0xFFFFFFFCU &&
            definition.secondary_count == 5U &&
            definition.background_action_id == 0x1234U &&
            definition.background_field_b4 == 0x11112222U &&
            definition.background_field_b8 == 0x33334444U &&
            definition.enemy_count == 2U &&
            definition.enemies[0].role_id == 11U &&
            definition.enemies[0].mode_flag == 1U &&
            definition.enemies[0].position_x == 100U &&
            definition.enemies[0].position_y == 200U &&
            definition.enemies[1].role_id == 12U &&
            definition.enemies[1].mode_flag == 0U &&
            definition.enemies[1].position_x == 300U &&
            definition.enemies[1].position_y == 400U,
        "signed prefix and negative variant select physical record bytes and all decoded fields"
    );
}

#ifdef __linux__
int find_descriptor(const std::filesystem::path& path) {
    for (const auto& entry :
         std::filesystem::directory_iterator("/proc/self/fd")) {
        std::error_code error;
        const auto target = std::filesystem::read_symlink(entry.path(), error);
        if (!error && target == std::filesystem::absolute(path)) {
            return std::stoi(entry.path().filename().string());
        }
    }

    return -1;
}
#endif

void test_file_ownership(openswd3::test::Context& test) {
    const ArchiveTestFiles fixtures;
    const std::array<u8, 3> payload{0x10U, 0x20U, 0x30U};
    fixtures.write("first.ffd", payload);
    fixtures.write("second.ffd", payload);
    {
        LegacyBattleDefinitionArchiveFiles files;
        auto* const first = files.open(fixtures.path("first.ffd"));
        auto* const second = files.open(fixtures.path("second.ffd"));
        test.expect_true(
            first != nullptr && second != nullptr && first != second,
            "archive ownership creates two actual file objects"
        );
        if (first == nullptr || second == nullptr) {
            return;
        }

        std::array<u8, 2> bytes{0xCCU, 0xCCU};
        u32 count = 1U;
        test.expect_true(
            first->read(bytes, count) && count == 1U && bytes[0] == 0x10U,
            "first file advances its own cursor"
        );
        test.expect_true(
            files.close(first) && files.size() == 1U,
            "closing a file releases its owned object"
        );
        count = 1U;
        test.expect_true(
            second->read(bytes, count) && count == 1U && bytes[0] == 0x10U &&
                files.close(second) && files.size() == 0U,
            "closing one file does not move or close the other cursor"
        );
    }

    fixtures.write("fault.ffd", std::vector<u8>(0x2714U, 0U));
    {
        LegacyBattleDefinitionArchiveFiles files;
        LegacyBattleRenderGeometryBindingObject object;
        LegacyBattleDefinitionArchiveRecord record;
        record.bytes.fill(0xA5U);
        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object, record, files, fixtures.path("fault.ffd"), 0x2000U, 0U
            );
        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        header_count_typed_stop &&
                result.battle_index == 0x2000U &&
                result.prefix_bytes_read == 0x2714U && files.size() == 1U &&
                record.bytes[0U] == 0xA5U,
            "count access fault retains the opened resource and original destination"
        );
#ifdef __linux__
        test.expect_true(
            find_descriptor(fixtures.path("fault.ffd")) >= 0,
            "the faulting path really leaves its descriptor open"
        );
#endif
    }

#ifdef __linux__
    test.expect_true(
        find_descriptor(fixtures.path("fault.ffd")) == -1,
        "destroying the archive owner really closes retained descriptors"
    );
    {
        LegacyBattleDefinitionArchiveFiles files;
        auto* const file = files.open(fixtures.path("first.ffd"));
        const int descriptor = find_descriptor(fixtures.path("first.ffd"));
        test.expect_true(
            file != nullptr && descriptor >= 0,
            "locate the actual descriptor for close failure injection"
        );
        if (file != nullptr && descriptor >= 0) {
            test.expect_true(
                ::close(descriptor) == 0,
                "invalidate only the fixture descriptor"
            );
            test.expect_true(
                !files.close(file) && files.size() == 1U,
                "failed system close retains ownership until owner destruction"
            );
        }
    }
#endif
}

void test_real_archive(openswd3::test::Context& test) {
#ifdef OPENSWD3_GAME_DATA_ROOT
    const auto path =
        std::filesystem::path{OPENSWD3_GAME_DATA_ROOT} / "battle.ffd";
    std::array<u8, 0x2714U> expected_header{};
    std::array<u8, 0x10CU> expected_record{};
    u32 offset = 0U;
    {
        std::ifstream file{path, std::ios::binary};
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.read(
            reinterpret_cast<char*>(expected_header.data()),
            static_cast<std::streamsize>(expected_header.size())
        );
        for (u32 index = 0U; index < 4U; ++index) {
            offset |= static_cast<u32>(expected_header[4U + index])
                << (index * 8U);
        }

        offset = 0x2714U + offset * 0x10CU;
        file.seekg(static_cast<std::streamoff>(offset));
        file.read(
            reinterpret_cast<char*>(expected_record.data()),
            static_cast<std::streamsize>(expected_record.size())
        );
    }

    LegacyBattleDefinitionArchiveFiles files;
    LegacyBattleRenderGeometryBindingObject object;
    LegacyBattleDefinitionArchiveRecord record;
    u32 published = 0U;
    const auto header =
        openswd3::battle::load_legacy_battle_definition_archive_header(
            object, published, files, path
        );
    const auto loaded =
        openswd3::battle::load_legacy_battle_definition_archive_record(
            object, record, files, path, 1U, 0U
        );
    test.expect_true(
        header.bytes_read == 0x2714U && published == 0x1F48U &&
            object.battle_header_bytes == expected_header &&
            loaded.status ==
                LegacyBattleDefinitionArchiveRecordLoadStatus::completed &&
            loaded.record_bytes_read == 0x10CU &&
            loaded.file_offset == offset && record.bytes == expected_record &&
            files.size() == 0U,
        "real battle.ffd header and first record match independent physical file reads"
    );
#else
    static_cast<void>(test);
#endif
}

}  // namespace

void test_battle_definition_archive(openswd3::test::Context& test) {
    test_header_reads(test);
    test_record_reads(test);
    test_signed_selection_and_decode(test);
    test_file_ownership(test);
    test_real_archive(test);
}
