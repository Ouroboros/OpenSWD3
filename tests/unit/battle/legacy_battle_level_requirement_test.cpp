#include "legacy_battle_level_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_level_requirement.hpp"
#include "test.hpp"

#include <array>
#include <fstream>
#include <span>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleLevelDatabaseCall;
using openswd3::battle::LegacyBattleLevelDatabaseCallReply;
using openswd3::battle::LegacyBattleLevelDatabaseCallRequest;
using openswd3::battle::LegacyBattleLevelDatabasePort;
using openswd3::battle::LegacyBattleLevelRequirementLoadRequest;
using openswd3::battle::LegacyBattleLevelRequirementLoadStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::test::LegacyBattleLevelDatabaseFixture;

void append_record(std::vector<u8>& bytes, const u32 value) {
    LegacyBattleLevelDatabaseFixture::append_word(bytes, 0U);
    bytes.resize(bytes.size() + 0x16U, 0U);
    LegacyBattleLevelDatabaseFixture::append_dword(bytes, value);
}

#ifdef OPENSWD3_LEVEL_DATA_PATH
class RealLevelPort final : public LegacyBattleLevelDatabasePort {
public:
    LegacyBattleLevelDatabaseCallReply invoke_legacy_battle_level_database(
        const LegacyBattleLevelDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        switch (request.call) {
        case LegacyBattleLevelDatabaseCall::open_file:
            ++open_calls;
            file.open(OPENSWD3_LEVEL_DATA_PATH, std::ios::binary);
            return {
                .eax = file.is_open() ? file_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case LegacyBattleLevelDatabaseCall::seek_file:
            ++seek_calls;
            file.clear();
            file.seekg(
                static_cast<std::streamoff>(request.distance), std::ios::beg
            );
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};

        case LegacyBattleLevelDatabaseCall::read_file:
            ++read_calls;
            file.read(
                reinterpret_cast<char*>(destination.data()),
                static_cast<std::streamsize>(destination.size())
            );
            return {
                .eax = file.bad() ? 0U : 1U,
                .ecx = request.ecx,
                .edx = request.edx,
                .bytes_read = static_cast<u32>(file.gcount()),
            };

        case LegacyBattleLevelDatabaseCall::allocate_stream:
            ++allocation_calls;
            return {
                .eax = stream_token,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case LegacyBattleLevelDatabaseCall::release_stream:
            ++release_calls;
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        return {};
    }

    std::ifstream file;
    u32 file_handle{0x12345678U};
    u32 stream_token{0x71000000U};
    u32 open_calls{};
    u32 seek_calls{};
    u32 read_calls{};
    u32 allocation_calls{};
    u32 release_calls{};
};
#endif

class StaleDirectoryPort final : public LegacyBattleLevelDatabaseFixture {
public:
    LegacyBattleLevelDatabaseCallReply invoke_legacy_battle_level_database(
        const LegacyBattleLevelDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        if (request.call == LegacyBattleLevelDatabaseCall::read_file &&
            read_calls == 0U) {
            calls.push_back(request);
            ++read_calls;
            return {
                .eax = request.eax,
                .ecx = request.ecx,
                .edx = request.edx,
                .bytes_read = 0U,
            };
        }
        return LegacyBattleLevelDatabaseFixture::
            invoke_legacy_battle_level_database(request, destination);
    }
};

void test_real_level_data(openswd3::test::Context& test) {
#ifdef OPENSWD3_LEVEL_DATA_PATH
    RealLevelPort port;
    u32 first_output = 0xAAAAAAAAU;
    u32 second_output = 0xBBBBBBBBU;
    LegacyBattleLevelRequirementLoadRequest first;
    first.path = OPENSWD3_LEVEL_DATA_PATH;
    first.group = 1U;
    first.level = 2U;
    first.output_token = 0x1000U;
    const auto first_result =
        openswd3::battle::load_legacy_battle_level_requirement(
            first_output, port, first
        );
    auto second = first;
    second.group = 2U;
    second.level = 50U;
    second.output_token = 0x2000U;
    const auto second_result =
        openswd3::battle::load_legacy_battle_level_requirement(
            second_output, port, second
        );

    test.expect_true(
        first_result.status ==
                LegacyBattleLevelRequirementLoadStatus::completed &&
            first_result.record_found && first_output == 20U &&
            first_result.directory_entry_offset == 0x208U &&
            first_result.record_relative_offset == 1612U &&
            first_result.record_file_offset == 2124U &&
            second_result.status ==
                LegacyBattleLevelRequirementLoadStatus::completed &&
            second_result.record_found && second_output == 63950U &&
            second_result.directory_entry_offset == 0x458U &&
            second_result.record_relative_offset == 4950U &&
            second_result.record_file_offset == 5462U &&
            port.open_calls == 1U && port.seek_calls == 4U &&
            port.read_calls == 4U && port.allocation_calls == 2U &&
            port.release_calls == 2U,
        "real LEVEL.DAT queries share one open session and decode group one level two plus group two level fifty"
    );
#else
    test.expect_true(true, "real LEVEL.DAT is optional");
#endif
}

void test_complete_and_rejected_records(openswd3::test::Context& test) {
    LegacyBattleLevelDatabaseFixture port;
    port.level_value = 0x11223344U;
    std::vector<u8> stream;
    append_record(stream, 0x01020304U);
    LegacyBattleLevelDatabaseFixture::append_word(stream, 2U);
    append_record(stream, 0xA1B2C3D4U);
    LegacyBattleLevelDatabaseFixture::append_word(stream, 1U);
    LegacyBattleLevelDatabaseFixture::append_word(stream, 0xFFFFU);
    LegacyBattleLevelDatabaseFixture::append_word(stream, 5U);
    port.custom_stream = stream;

    u32 output = 0xDEADBEEFU;
    LegacyBattleLevelRequirementLoadRequest request;
    request.group = 3U;
    request.level = 7U;
    request.output_token = 0xCAFEBABEU;
    request.entry_eax = 0x11111111U;
    request.entry_ecx = 0x22223333U;
    request.entry_edx = 0x44445555U;
    const auto result = openswd3::battle::load_legacy_battle_level_requirement(
        output, port, request
    );

    test.expect_true(
        result.status == LegacyBattleLevelRequirementLoadStatus::completed &&
            result.record_found && result.directory_entry_offset == 0x53CU &&
            result.record_file_offset == 0x1200U &&
            result.output_write_count == 2U &&
            result.output_value == 0xA1B2C3D4U && output == 0xA1B2C3D4U &&
            result.stream_cursor == 64U && result.open_calls == 1U &&
            result.seek_calls == 2U && result.read_calls == 2U &&
            result.allocation_calls == 1U && result.release_calls == 1U &&
            result.return_eax == 1U &&
            result.return_ecx == port.release_return_ecx &&
            result.return_edx == port.release_return_edx &&
            port.requested_entries ==
                std::vector<std::pair<u32, u32>>{{3U, 7U}},
        "tag zero copies twenty-six bytes, tag two consumes no payload, tag one skips one word, and the last tag-zero value wins"
    );

    port.reset_level_calls();
    port.custom_stream.clear();
    port.record_available = false;
    output = 0x55667788U;
    const auto rejected =
        openswd3::battle::load_legacy_battle_level_requirement(
            output, port, request
        );
    test.expect_true(
        rejected.status == LegacyBattleLevelRequirementLoadStatus::completed &&
            !rejected.record_found && rejected.return_eax == 0U &&
            rejected.release_calls == 1U && output == 0x55667788U &&
            port.open_calls == 0U,
        "a nonzero first word releases the stream, returns zero, preserves output, and reuses the open file"
    );
}

void test_failure_and_typed_stops(openswd3::test::Context& test) {
    LegacyBattleLevelRequirementLoadRequest request;
    request.group = 1U;
    request.level = 2U;
    request.output_token = 0x12340000U;
    request.number_of_bytes_read_token = 0x0053C000U;

    {
        LegacyBattleLevelDatabaseFixture port;
        port.open_succeeds = false;
        u32 output = 7U;
        const auto result =
            openswd3::battle::load_legacy_battle_level_requirement(
                output, port, request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelRequirementLoadStatus::open_failed &&
                result.return_eax == 0U && result.open_calls == 1U &&
                result.seek_calls == 0U && result.allocation_calls == 0U &&
                output == 7U,
            "open failure returns zero before seeking or allocating"
        );
    }

    {
        LegacyBattleLevelDatabaseFixture port;
        port.allocation_succeeds = false;
        u32 output = 8U;
        const auto result =
            openswd3::battle::load_legacy_battle_level_requirement(
                output, port, request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelRequirementLoadStatus::
                        stream_zero_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == request.number_of_bytes_read_token &&
                result.read_calls == 1U && result.release_calls == 0U &&
                output == 8U,
            "a zero allocation stops at the original 1024-byte clear with live ECX and number-read token"
        );
    }

    {
        LegacyBattleLevelDatabaseFixture port;
        port.record_available = true;
        port.level_value = 0xAABBCCDDU;
        auto inaccessible = request;
        inaccessible.output_accessible = false;
        u32 output = 9U;
        const auto result =
            openswd3::battle::load_legacy_battle_level_requirement(
                output, port, inaccessible
            );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelRequirementLoadStatus::
                        output_access_typed_stop &&
                result.stream_cursor == 28U &&
                result.return_eax == port.stream_token + 28U &&
                result.return_ecx == 0xAABBCCDDU &&
                result.return_edx == request.output_token &&
                result.release_calls == 0U && output == 9U,
            "an inaccessible output stops only after the twenty-six-byte local copy and preserves parser registers"
        );
    }

    {
        LegacyBattleLevelDatabaseFixture port;
        port.custom_stream.assign(
            openswd3::battle::kLegacyBattleLevelStreamBytes, 0U
        );
        u32 output = 10U;
        const auto result =
            openswd3::battle::load_legacy_battle_level_requirement(
                output, port, request
            );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelRequirementLoadStatus::
                        stream_access_typed_stop &&
                result.stopped_stream_offset == 0x3FEU &&
                result.stream_cursor == 0x40CU &&
                result.copied_record_bytes == 12U && result.return_ecx == 3U &&
                result.release_calls == 0U && output == 0U,
            "a missing terminator repeats tag-zero records until the first out-of-window payload access"
        );
    }
}

void test_stale_directory_offset(openswd3::test::Context& test) {
    StaleDirectoryPort port;
    port.record_available = false;
    u32 output = 0x12345678U;
    LegacyBattleLevelRequirementLoadRequest request;
    request.group = 0x40000001U;
    request.level = 2U;
    request.output_token = 0x1000U;
    request.stale_directory_offset = 0xABCDE000U;
    const auto result = openswd3::battle::load_legacy_battle_level_requirement(
        output, port, request
    );

    test.expect_true(
        result.directory_entry_offset == 0x208U &&
            result.record_relative_offset == 0xABCDE000U &&
            result.record_file_offset == 0xABCDE200U &&
            result.status ==
                LegacyBattleLevelRequirementLoadStatus::completed &&
            !result.record_found && output == 0x12345678U,
        "group arithmetic wraps at 32 bits and a short directory read preserves the explicit stale stack dword"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_real_level_data(test);
    test_complete_and_rejected_records(test);
    test_failure_and_typed_stops(test);
    test_stale_directory_offset(test);
    return test.exit_code();
}
