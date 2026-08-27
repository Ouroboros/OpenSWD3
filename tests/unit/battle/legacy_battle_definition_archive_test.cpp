#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <algorithm>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleDefinitionArchiveApiReply;
using openswd3::battle::LegacyBattleDefinitionArchiveCloseRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderLoadRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderLoadStatus;
using openswd3::battle::LegacyBattleDefinitionArchiveFilePort;
using openswd3::battle::LegacyBattleDefinitionArchiveSeekRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveOpenRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveReadReply;
using openswd3::battle::LegacyBattleDefinitionArchiveReadRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveRecordLoadStatus;
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

class HeaderPort final : public LegacyBattleDefinitionArchiveFilePort {
public:
    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply open_archive_file(
        const LegacyBattleDefinitionArchiveOpenRequest& request
    ) override {
        open_request = request;
        events.push_back(1U);
        return open_reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveReadReply read_archive_file(
        const LegacyBattleDefinitionArchiveReadRequest& request,
        const std::span<u8> destination
    ) override {
        read_request = request;
        const auto copied = std::min(data.size(), destination.size());
        std::copy_n(data.begin(), copied, destination.begin());
        events.push_back(2U);
        auto reply = read_reply;
        reply.bytes_read = static_cast<u32>(copied);
        return reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply seek_archive_file(
        const LegacyBattleDefinitionArchiveSeekRequest& request
    ) override {
        seek_request = request;
        events.push_back(4U);
        return seek_reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply close_archive_file(
        const LegacyBattleDefinitionArchiveCloseRequest& request
    ) override {
        close_request = request;
        events.push_back(3U);
        return close_reply;
    }

    LegacyBattleDefinitionArchiveApiReply open_reply{};
    LegacyBattleDefinitionArchiveReadReply read_reply{};
    LegacyBattleDefinitionArchiveApiReply seek_reply{};
    LegacyBattleDefinitionArchiveApiReply close_reply{};
    LegacyBattleDefinitionArchiveOpenRequest open_request{};
    LegacyBattleDefinitionArchiveReadRequest read_request{};
    LegacyBattleDefinitionArchiveSeekRequest seek_request{};
    LegacyBattleDefinitionArchiveCloseRequest close_request{};
    std::vector<u8> data;
    std::vector<u32> events;
};

class RecordPort final : public LegacyBattleDefinitionArchiveFilePort {
public:
    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply open_archive_file(
        const LegacyBattleDefinitionArchiveOpenRequest& request
    ) override {
        open_request = request;
        events.push_back(1U);
        return open_reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveReadReply read_archive_file(
        const LegacyBattleDefinitionArchiveReadRequest& request,
        const std::span<u8> destination
    ) override {
        read_requests.push_back(request);
        const auto& source =
            read_requests.size() == 1U ? header_data : record_data;
        const auto copied = std::min(source.size(), destination.size());
        std::copy_n(source.begin(), copied, destination.begin());
        events.push_back(2U);
        auto reply = read_reply;
        reply.bytes_read = static_cast<u32>(copied);
        return reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply seek_archive_file(
        const LegacyBattleDefinitionArchiveSeekRequest& request
    ) override {
        seek_request = request;
        events.push_back(3U);
        return seek_reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply close_archive_file(
        const LegacyBattleDefinitionArchiveCloseRequest& request
    ) override {
        close_request = request;
        events.push_back(4U);
        return close_reply;
    }

    LegacyBattleDefinitionArchiveApiReply open_reply{
        .eax = 0x70000001U,
        .ecx = 0x11111111U,
        .edx = 0x22222222U,
    };
    LegacyBattleDefinitionArchiveReadReply read_reply{
        .eax = 0U,
        .ecx = 0x33333333U,
        .edx = 0x44444444U,
    };
    LegacyBattleDefinitionArchiveApiReply seek_reply{
        .eax = 0x55555555U,
        .ecx = 0x66666666U,
        .edx = 0x77777777U,
    };
    LegacyBattleDefinitionArchiveApiReply close_reply{
        .eax = 0x88888888U,
        .ecx = 0x99999999U,
        .edx = 0xAAAAAAAAU,
    };
    LegacyBattleDefinitionArchiveOpenRequest open_request{};
    std::vector<LegacyBattleDefinitionArchiveReadRequest> read_requests;
    LegacyBattleDefinitionArchiveSeekRequest seek_request{};
    LegacyBattleDefinitionArchiveCloseRequest close_request{};
    std::vector<u8> header_data;
    std::vector<u8> record_data;
    std::vector<u32> events;
};

}  // namespace

void test_battle_definition_archive(openswd3::test::Context& test) {
    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        object.battle_header_bytes.fill(0xA5U);
        u32 published = 0x11223344U;
        HeaderPort port;
        port.open_reply = {
            .eax = 0xFFFFFFFFU,
            .ecx = 0x12345678U,
            .edx = 0x23456789U,
        };
        port.close_reply = {
            .eax = 0x3456789AU,
            .ecx = 0x456789ABU,
            .edx = 0x56789ABCU,
        };

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_header(
                object,
                published,
                port,
                {
                    .path = "missing/battle.ffd",
                    .output_token = 0x005241FCU,
                    .entry_edx = 0x6789ABCDU,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveHeaderLoadStatus::
                        open_failed &&
                result.open_calls == 1U && result.read_calls == 0U &&
                result.close_calls == 1U &&
                port.events == std::vector<u32>{1U, 3U} &&
                port.open_request.path ==
                    std::filesystem::path("missing/battle.ffd") &&
                port.open_request.desired_access == 0x80000000U &&
                port.open_request.share_mode == 0U &&
                port.open_request.security_attributes_token == 0U &&
                port.open_request.creation_disposition == 3U &&
                port.open_request.flags_and_attributes == 0x80U &&
                port.open_request.template_file_token == 0U &&
                port.open_request.entry_eax == 0x004AAED0U &&
                port.open_request.entry_ecx == 0x004FF5B8U &&
                port.open_request.entry_edx == 0x6789ABCDU &&
                port.close_request.handle == 0xFFFFFFFFU &&
                port.close_request.entry_eax == 0xFFFFFFFFU &&
                port.close_request.entry_ecx == 0x12345678U &&
                port.close_request.entry_edx == 0x23456789U &&
                published == 0x11223344U &&
                object.battle_header_bytes.front() == 0xA5U &&
                result.return_eax == 0U && result.return_ecx == 0x004FF5B8U &&
                result.return_edx == 0x56789ABCU,
            "an invalid handle is still closed before the header loader returns zero"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        object.battle_header_bytes.fill(0xCCU);
        object.index_records[0] = {
            .ordinal = 0xAAAAAAAAU, .five_step_quarter = 0x11111111
        };
        u32 published = 0xFFFFFFFFU;
        HeaderPort port;
        port.open_reply = {
            .eax = 0x70000001U,
            .ecx = 0x11111111U,
            .edx = 0x22222222U,
        };
        port.read_reply = {
            .eax = 0U,
            .ecx = 0x33333333U,
            .edx = 0x44444444U,
        };
        port.close_reply = {
            .eax = 0x55555555U,
            .ecx = 0x66666666U,
            .edx = 0x77777777U,
        };
        port.data = {0x10U, 0x20U, 0x30U};

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_header(
                object,
                published,
                port,
                LegacyBattleDefinitionArchiveHeaderLoadRequest{
                    .path = "data/battle.ffd",
                    .binding_object_token = 0x004FF5B8U,
                    .output_token = 0x005241FCU,
                    .file_name_token = 0x004AAED0U,
                    .number_of_bytes_read_token = 0x12340000U,
                    .entry_edx = 0x87654321U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveHeaderLoadStatus::completed &&
                result.open_calls == 1U && result.read_calls == 1U &&
                result.close_calls == 1U && result.bytes_read == 3U &&
                port.events == std::vector<u32>{1U, 2U, 3U} &&
                port.read_request.handle == 0x70000001U &&
                port.read_request.destination_token == 0x004FF5BCU &&
                port.read_request.requested_bytes == 0x2714U &&
                port.read_request.overlapped_token == 0U &&
                port.read_request.entry_eax == 0x70000001U &&
                port.read_request.entry_ecx == 0x12340000U &&
                port.read_request.entry_edx == 0x004FF5BCU &&
                port.close_request.handle == 0x70000001U &&
                port.close_request.entry_eax == 0x005241FCU &&
                port.close_request.entry_ecx == 0x33333333U &&
                port.close_request.entry_edx == 0x44444444U &&
                object.battle_header_bytes[0] == 0x10U &&
                object.battle_header_bytes[1] == 0x20U &&
                object.battle_header_bytes[2] == 0x30U &&
                object.battle_header_bytes[3] == 0xCCU &&
                object.battle_header_bytes.back() == 0xCCU &&
                object.index_records[0].ordinal == 0xAAAAAAAAU &&
                published == 0x00501500U && result.header_index_published &&
                result.published_header_index_token == 0x00501500U &&
                result.return_eax == 1U && result.return_ecx == 0x004FF5B8U &&
                result.return_edx == 0x77777777U,
            "a short failed ReadFile result is ignored after preserving its written prefix and publishing the header index"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        RecordPort port;
        port.open_reply = {
            .eax = 0xFFFFFFFFU,
            .ecx = 0x11111111U,
            .edx = 0x22222222U,
        };

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 1U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        open_failed &&
                result.open_calls == 1U && result.read_calls == 0U &&
                result.seek_calls == 0U && result.close_calls == 1U &&
                port.events == std::vector<u32>{1U, 4U} &&
                port.close_request.handle == 0xFFFFFFFFU &&
                result.return_eax == 0U && result.return_ecx == 0x004FF5B8U &&
                result.return_edx == 0xAAAAAAAAU,
            "a failed record open still closes the all-ones handle before returning zero"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        record.bytes.fill(0xCCU);
        RecordPort port;
        port.header_data.assign(0x2714U, 0U);
        port.header_data[0x1F45U] = 0x80U;

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 1U,
                    .variant = 0U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        rejected_count &&
                result.open_calls == 1U && result.read_calls == 1U &&
                result.seek_calls == 0U && result.close_calls == 1U &&
                port.events == std::vector<u32>{1U, 2U, 4U} &&
                record.bytes.front() == 0xCCU && result.return_eax == 0U &&
                result.return_ecx == 0x004FF5B8U &&
                result.return_edx == 0xAAAAAAAAU,
            "a signed nonpositive record count closes the file and preserves the destination"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        RecordPort port;
        port.header_data.assign(0x2714U, 0U);
        port.header_data[0x1F45U] = 1U;

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 1U,
                    .variant = 2U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        rejected_variant &&
                result.read_calls == 1U && result.seek_calls == 0U &&
                result.close_calls == 1U &&
                port.events == std::vector<u32>{1U, 2U, 4U},
            "a signed variant greater than the positive count closes without seeking"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        record.bytes.fill(0xCCU);
        RecordPort port;
        port.header_data.assign(0x2714U, 0U);
        port.header_data[0x1F47U] = 2U;
        port.header_data[0x1F45U] = 0xFEU;
        port.header_data[0x1F46U] = 3U;
        write_u32(port.header_data, 4U, 2U);
        port.record_data.assign(0xF2U, 0U);
        write_u32(port.record_data, 0x04U, 0xFFFFFFFCU);
        write_u16(port.record_data, 0x24U, 5U);
        write_u16(port.record_data, 0x28U, 0x1234U);
        write_u32(port.record_data, 0x58U, 0x11112222U);
        write_u32(port.record_data, 0x78U, 0x33334444U);
        write_u16(port.record_data, 0x98U, 2U);
        write_u16(port.record_data, 0x9CU, 11U);
        write_u16(port.record_data, 0xBCU, 1U);
        write_u16(port.record_data, 0xCCU, 100U);
        write_u16(port.record_data, 0xECU, 200U);
        write_u16(port.record_data, 0xA0U, 12U);
        write_u16(port.record_data, 0xBEU, 0U);
        write_u16(port.record_data, 0xD0U, 300U);
        write_u16(port.record_data, 0xF0U, 400U);

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 3U,
                    .variant = 0xFFU,
                    .number_of_bytes_read_token = 0x12345678U,
                    .entry_edx = 0x87654321U,
                }
            );
        const auto definition =
            openswd3::battle::decode_legacy_battle_definition(record);

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::completed &&
                result.read_calls == 2U && result.seek_calls == 1U &&
                result.record_bytes_read == 0xF2U &&
                record.bytes[0xF2U] == 0xCCU && record.bytes.back() == 0xCCU &&
                result.close_calls == 1U && result.battle_index == 3U &&
                result.signed_prefix_sum == 1U &&
                result.combined_record_index == 0U &&
                result.record_offset_value == 2U &&
                result.file_offset == 0x292CU &&
                port.events == std::vector<u32>{1U, 2U, 3U, 2U, 4U} &&
                port.seek_request.handle == 0x70000001U &&
                port.seek_request.distance == 0x292CU &&
                port.seek_request.distance_high_token == 0U &&
                port.seek_request.move_method == 0U &&
                port.seek_request.entry_eax == 0x292CU &&
                port.seek_request.entry_ecx == 66U &&
                port.seek_request.entry_edx == 134U &&
                port.read_requests.size() == 2U &&
                port.read_requests[1].destination_token == 0x004FF1E0U &&
                port.read_requests[1].requested_bytes == 0x10CU &&
                port.read_requests[1].entry_eax == 0x55555555U &&
                port.read_requests[1].entry_ecx == 0x12345678U &&
                port.read_requests[1].entry_edx == 0x004FF1E0U &&
                port.close_request.entry_eax == 0U &&
                port.close_request.entry_ecx == 0x33333333U &&
                port.close_request.entry_edx == 0x44444444U &&
                result.return_eax == 1U && result.return_ecx == 0x004FF5B8U &&
                result.return_edx == 0xAAAAAAAAU &&
                definition.rotation_divisor == -4 &&
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
                definition.enemies[1].position_x == 300U &&
                definition.enemies[1].position_y == 400U,
            "signed prefix bytes and a negative variant select the exact record offset and decode its fixed fields"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        RecordPort port;
        port.header_data.assign(0x2714U, 0U);
        port.header_data[0x1F45U] = 1U;

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 1U,
                    .variant = 0x80U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        offset_table_typed_stop &&
                result.combined_record_index == 0xFFFFFF80U &&
                result.read_calls == 1U && result.seek_calls == 0U &&
                result.close_calls == 0U &&
                port.events == std::vector<u32>{1U, 2U} &&
                result.return_eax == 0xFFFFFF80U && result.return_ecx == 1U &&
                result.return_edx == 0U,
            "a negative variant stops only at the original out-of-object offset-table read"
        );
    }

    {
        openswd3::battle::LegacyBattleRenderGeometryBindingObject object;
        openswd3::battle::LegacyBattleDefinitionArchiveRecord record;
        RecordPort port;
        port.header_data.assign(0x2714U, 0U);

        const auto result =
            openswd3::battle::load_legacy_battle_definition_archive_record(
                object,
                record,
                port,
                {
                    .path = "data/battle.ffd",
                    .output_token = 0x004FF1E0U,
                    .battle_id = 0x2000U,
                }
            );

        test.expect_true(
            result.status ==
                    LegacyBattleDefinitionArchiveRecordLoadStatus::
                        header_count_typed_stop &&
                result.read_calls == 1U && result.seek_calls == 0U &&
                result.close_calls == 0U && result.battle_index == 0x2000U &&
                result.return_eax == 0x2000U &&
                result.return_ecx == 0x33333333U &&
                result.return_edx == 0x44444444U,
            "an oversized battle index stops at its first real count-byte access without closing the faulting path"
        );
    }
}
