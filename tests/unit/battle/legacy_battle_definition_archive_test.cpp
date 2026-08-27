#include "openswd3/battle/legacy_battle_definition_archive.hpp"

#include <algorithm>
#include <vector>

#include "test.hpp"

namespace {

using openswd3::battle::LegacyBattleDefinitionArchiveApiReply;
using openswd3::battle::LegacyBattleDefinitionArchiveCloseRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderLoadRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderLoadStatus;
using openswd3::battle::LegacyBattleDefinitionArchiveHeaderPort;
using openswd3::battle::LegacyBattleDefinitionArchiveOpenRequest;
using openswd3::battle::LegacyBattleDefinitionArchiveReadReply;
using openswd3::battle::LegacyBattleDefinitionArchiveReadRequest;
using openswd3::compat::u8;
using openswd3::compat::u32;

class HeaderPort final : public LegacyBattleDefinitionArchiveHeaderPort {
public:
    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply open_header(
        const LegacyBattleDefinitionArchiveOpenRequest& request
    ) override {
        open_request = request;
        events.push_back(1U);
        return open_reply;
    }

    [[nodiscard]] LegacyBattleDefinitionArchiveReadReply read_header(
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

    [[nodiscard]] LegacyBattleDefinitionArchiveApiReply close_header(
        const LegacyBattleDefinitionArchiveCloseRequest& request
    ) override {
        close_request = request;
        events.push_back(3U);
        return close_reply;
    }

    LegacyBattleDefinitionArchiveApiReply open_reply{};
    LegacyBattleDefinitionArchiveReadReply read_reply{};
    LegacyBattleDefinitionArchiveApiReply close_reply{};
    LegacyBattleDefinitionArchiveOpenRequest open_request{};
    LegacyBattleDefinitionArchiveReadRequest read_request{};
    LegacyBattleDefinitionArchiveCloseRequest close_request{};
    std::vector<u8> data;
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
}
