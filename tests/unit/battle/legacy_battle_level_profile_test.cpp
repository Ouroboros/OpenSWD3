#include "legacy_battle_level_database_fixture.hpp"
#include "legacy_battle_mon_database_fixture.hpp"
#include "openswd3/battle/legacy_battle_level_profile.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <span>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleLevelDatabaseCall;
using openswd3::battle::LegacyBattleLevelDatabaseCallReply;
using openswd3::battle::LegacyBattleLevelDatabaseCallRequest;
using openswd3::battle::LegacyBattleLevelProfileCall;
using openswd3::battle::LegacyBattleLevelProfileCallReply;
using openswd3::battle::LegacyBattleLevelProfileCallRequest;
using openswd3::battle::LegacyBattleLevelProfileLoadRequest;
using openswd3::battle::LegacyBattleLevelProfileLoadStatus;
using openswd3::battle::LegacyBattleMonDatabaseCall;
using openswd3::battle::LegacyBattleMonDatabaseCallReply;
using openswd3::battle::LegacyBattleMonDatabaseCallRequest;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::test::LegacyBattleLevelDatabaseFixture;
using openswd3::test::LegacyBattleMonDatabaseFixture;
using openswd3::world_map::LegacyWorldStoryPartyMemberResources;

class Port final
    : public openswd3::battle::LegacyBattleLevelProfilePort,
      public LegacyBattleLevelDatabaseFixture,
      public LegacyBattleMonDatabaseFixture,
      public virtual openswd3::world_map::LegacyWorldItemListStatePort {
public:
    [[nodiscard]] LegacyBattleLevelProfileCallReply invoke_level_profile(
        const LegacyBattleLevelProfileCallRequest& request
    ) override {
        profile_calls.push_back(request);
        if (request.call == LegacyBattleLevelProfileCall::report_zero_item) {
            return {
                .eax = 0x11112222U,
                .ecx = 0x33334444U,
                .edx = 0x55556666U,
                .typed_stop = diagnostic_stops,
            };
        }
        if (!allocation_tokens.empty()) {
            const u32 token = allocation_tokens.front();
            allocation_tokens.pop_front();
            return {
                .eax = token,
                .ecx = request.ecx,
                .edx = request.edx,
                .publish_allocation_token = true,
                .allocation_token = token,
            };
        }
        return LegacyBattleLevelProfilePort::invoke_level_profile(request);
    }

    std::deque<u32> allocation_tokens;
    std::vector<LegacyBattleLevelProfileCallRequest> profile_calls;
    bool diagnostic_stops{};
};

#if defined(OPENSWD3_LEVEL_DATA_PATH) && defined(OPENSWD3_MON_DATA_PATH)
class RealPort final
    : public openswd3::battle::LegacyBattleLevelProfilePort,
      public virtual openswd3::world_map::LegacyWorldItemListStatePort {
public:
    [[nodiscard]] LegacyBattleLevelDatabaseCallReply
    invoke_legacy_battle_level_database(
        const LegacyBattleLevelDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        switch (request.call) {
        case LegacyBattleLevelDatabaseCall::open_file:
            ++level_open_calls;
            level_file.open(OPENSWD3_LEVEL_DATA_PATH, std::ios::binary);
            return {
                .eax = level_file.is_open() ? level_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        case LegacyBattleLevelDatabaseCall::seek_file:
            ++level_seek_calls;
            level_file.clear();
            level_file.seekg(
                static_cast<std::streamoff>(request.distance), std::ios::beg
            );
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        case LegacyBattleLevelDatabaseCall::read_file:
            ++level_read_calls;
            level_file.read(
                reinterpret_cast<char*>(destination.data()),
                static_cast<std::streamsize>(destination.size())
            );
            return {
                .eax = level_file.bad() ? 0U : 1U,
                .ecx = request.ecx,
                .edx = request.edx,
                .bytes_read = static_cast<u32>(level_file.gcount()),
            };
        case LegacyBattleLevelDatabaseCall::allocate_stream:
            return {
                .eax = level_stream_token,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        case LegacyBattleLevelDatabaseCall::release_stream:
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        return {};
    }

    [[nodiscard]] LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        switch (request.call) {
        case LegacyBattleMonDatabaseCall::open_file:
            ++mon_open_calls;
            mon_file.open(OPENSWD3_MON_DATA_PATH, std::ios::binary);
            return {
                .eax = mon_file.is_open() ? mon_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        case LegacyBattleMonDatabaseCall::seek_file: {
            ++mon_seek_calls;
            mon_file.clear();
            std::ios_base::seekdir direction = std::ios::beg;
            if (request.move_method == 1U) {
                direction = std::ios::cur;
            } else if (request.move_method == 2U) {
                direction = std::ios::end;
            }
            mon_file.seekg(
                static_cast<std::streamoff>(request.distance), direction
            );
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        case LegacyBattleMonDatabaseCall::read_file:
            ++mon_read_calls;
            mon_file.read(
                reinterpret_cast<char*>(destination.data()),
                static_cast<std::streamsize>(destination.size())
            );
            return {
                .eax = mon_file.bad() ? 0U : 1U,
                .ecx = request.ecx,
                .edx = request.edx,
                .bytes_read = static_cast<u32>(mon_file.gcount()),
            };
        case LegacyBattleMonDatabaseCall::allocate_stream:
            return {
                .eax = mon_stream_token,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        case LegacyBattleMonDatabaseCall::release_stream:
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        case LegacyBattleMonDatabaseCall::query_definition_text_size: {
            const auto found = text_sizes.find(request.block_token);
            return {
                .eax = found == text_sizes.end() ? 0U : found->second,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }
        case LegacyBattleMonDatabaseCall::allocate_definition_text: {
            const u32 token = next_text_token;
            next_text_token += 0x100U;
            text_sizes[token] = request.allocation_size;
            return {.eax = token, .ecx = request.ecx, .edx = request.edx};
        }
        case LegacyBattleMonDatabaseCall::release_definition_text:
            text_sizes.erase(request.block_token);
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        return {};
    }

    std::ifstream level_file;
    std::ifstream mon_file;
    std::unordered_map<u32, u32> text_sizes;
    u32 level_handle{0x11110001U};
    u32 mon_handle{0x22220001U};
    u32 level_stream_token{0x73000000U};
    u32 mon_stream_token{0x74000000U};
    u32 next_text_token{0x75000000U};
    u32 level_open_calls{};
    u32 level_seek_calls{};
    u32 level_read_calls{};
    u32 mon_open_calls{};
    u32 mon_seek_calls{};
    u32 mon_read_calls{};
};
#endif

void append_word(std::vector<u8>& bytes, const u16 value) {
    LegacyBattleLevelDatabaseFixture::append_word(bytes, value);
}

[[nodiscard]] std::vector<u8> make_profile_stream(
    const std::array<u8, 0x1AU>& payload,
    const std::span<const u16> item_ids = {}
) {
    std::vector<u8> stream;
    append_word(stream, 0U);
    stream.insert(stream.end(), payload.begin(), payload.end());
    for (const u16 item_id : item_ids) {
        append_word(stream, 1U);
        append_word(stream, item_id);
    }
    append_word(stream, 5U);
    return stream;
}

[[nodiscard]] std::array<u8, 0x1AU> sequential_payload() {
    std::array<u8, 0x1AU> payload{};
    for (std::size_t index = 0U; index < payload.size(); ++index) {
        payload[index] = static_cast<u8>(index + 1U);
    }
    return payload;
}

[[nodiscard]] LegacyBattleLevelProfileLoadRequest request() {
    LegacyBattleLevelProfileLoadRequest value;
    value.party_number_one_based = 2U;
    value.level = 7U;
    value.output_token = 0x005028C0U;
    value.number_of_bytes_read_token = 0x0053C000U;
    value.entry_eax = 0xAAAABBBBU;
    value.entry_ecx = 0xCCCCDDDDU;
    value.entry_edx = 0xEEEEFFFFU;
    return value;
}

void prepare_item_list(Port& port) {
    auto& list =
        *port.battle_level_profile_item_list_state()->party_item_lists[1U];
    list.sentinel.legacy_token = 0x71000000U;
    list.legacy_head_token = list.sentinel.legacy_token;
    list.sentinel.legacy_next_token = 0x710000B0U;
    list.nodes.emplace_back();
    auto& existing = list.nodes.back();
    existing.legacy_token = 0x710000B0U;
    existing.item_id = 0x0111U;
}

void prepare_definition(Port& port, const std::span<const u8> name) {
    port.LegacyBattleMonDatabaseFixture::definition.fill(0U);
    const std::size_t count = std::min(
        name.size(), port.LegacyBattleMonDatabaseFixture::definition.size() - 1U
    );
    std::copy_n(
        name.begin(),
        count,
        port.LegacyBattleMonDatabaseFixture::definition.begin()
    );
    port.LegacyBattleMonDatabaseFixture::definition_description = {
        0x44U,
        0x45U,
        0U,
    };
}

void test_real_profile_and_definition(openswd3::test::Context& test) {
#if defined(OPENSWD3_LEVEL_DATA_PATH) && defined(OPENSWD3_MON_DATA_PATH)
    RealPort port;
    auto& list =
        *port.battle_level_profile_item_list_state()->party_item_lists[0U];
    list.sentinel.legacy_token = 0x76000000U;
    list.legacy_head_token = list.sentinel.legacy_token;
    LegacyWorldStoryPartyMemberResources output{};
    std::array<u8, 24U> caption{};
    u32 transition_mode = 0U;
    LegacyBattleLevelProfileLoadRequest load_request;
    load_request.party_number_one_based = 1U;
    load_request.level = 5U;
    load_request.output_token = 0x005028C0U;
    const auto result = openswd3::battle::load_legacy_battle_level_profile(
        output, caption, transition_mode, port, load_request
    );

    const std::array<u8, 7U> expected_name{
        0xAFU,
        0x50U,
        0xA4U,
        0xF5U,
        0xB3U,
        0x4EU,
        0U,
    };
    test.expect_true(
        result.status == LegacyBattleLevelProfileLoadStatus::completed &&
            result.record_found && result.directory_entry_offset == 0x214U &&
            result.record_relative_offset == 1702U &&
            result.record_file_offset == 2214U && output.current_first == 63U &&
            output.current_second == 20U && output.current_third == 30U &&
            output.field_20 == 90U && output.field_2c == 5U &&
            result.appended_item_nodes == 1U && list.nodes.size() == 1U &&
            list.nodes.front().item_id == 1501U &&
            result.mon_definition_load.definition_id == 1501U &&
            result.mon_definition_load.definition_relative_offset == 139290U &&
            result.mon_definition_load.definition_file_offset == 139802U &&
            std::equal(
                expected_name.begin(), expected_name.end(), caption.begin()
            ) &&
            transition_mode == 1U && port.level_open_calls == 1U &&
            port.level_seek_calls == 2U && port.level_read_calls == 2U &&
            port.mon_open_calls == 1U && port.mon_seek_calls == 3U &&
            port.mon_read_calls == 3U,
        "real LEVEL.DAT group one level five builds the expected profile and appends MON.DAT item 1501 with its CP950 name"
    );
#else
    test.expect_true(true, "real LEVEL.DAT and MON.DAT are optional");
#endif
}

void test_profile_and_item_append(openswd3::test::Context& test) {
    Port port;
    prepare_item_list(port);
    prepare_definition(port, std::array<u8, 5U>{'S', 'w', 'o', 'r', 'd'});
    port.allocation_tokens.push_back(0x71000160U);

    const auto payload = sequential_payload();
    const std::array<u16, 3U> item_ids{0x0111U, 0x0222U, 0x8000U};
    port.LegacyBattleLevelDatabaseFixture::custom_stream =
        make_profile_stream(payload, item_ids);

    LegacyWorldStoryPartyMemberResources output{};
    std::array<u8, 24U> caption{};
    u32 transition_mode = 0U;
    const auto result = openswd3::battle::load_legacy_battle_level_profile(
        output, caption, transition_mode, port, request()
    );
    const auto& list =
        *port.battle_level_profile_item_list_state()->party_item_lists[1U];
    const auto& appended = list.nodes.back();

    test.expect_true(
        result.status == LegacyBattleLevelProfileLoadStatus::completed &&
            result.record_found && result.directory_entry_offset == 0x3ACU &&
            result.record_file_offset == 0x1200U &&
            result.stream_cursor == 42U && result.output_bytes_copied == 26U &&
            result.output_write_count == 11U &&
            result.traversed_item_nodes == 4U &&
            result.item_allocation_calls == 1U &&
            result.appended_item_nodes == 1U &&
            result.item_definition_load_calls == 1U &&
            result.transition_mode_writes == 1U && result.party_root_restored &&
            list.legacy_head_token == list.sentinel.legacy_token &&
            result.stream_release_calls == 1U && result.return_eax == 1U,
        "level profile parses its fixed record, reuses one item, appends one item, skips 0x8000, restores the root, and releases the stream"
    );
    test.expect_true(
        output.current_first == 0x0201U && output.current_second == 0x0403U &&
            output.current_third == 0x0605U && output.limit_first == 0x0201U &&
            output.limit_second == 0x0403U && output.limit_third == 0x0605U &&
            output.field_2c == 7U && output.field_20 == 0x1A191817U,
        "tag zero copies twenty-six bytes to +0x0A, mirrors its first three words to +0x04, and writes the low level byte"
    );
    test.expect_true(
        list.nodes.size() == 2U &&
            list.nodes.front().legacy_next_token == 0x71000160U &&
            appended.legacy_token == 0x71000160U &&
            appended.item_id == 0x0222U &&
            port.LegacyBattleMonDatabaseFixture::requested_definition_ids ==
                std::vector<u32>{0x0222U} &&
            transition_mode == 1U && caption[0U] == 'S' && caption[4U] == 'd' &&
            caption[5U] == 0U && result.caption_bytes_copied == 6U,
        "a newly appended node owns the MON definition, links at the raw tail, sets transition mode, and publishes its name"
    );

    port.LegacyBattleLevelDatabaseFixture::custom_stream =
        make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
    const auto cached = openswd3::battle::load_legacy_battle_level_profile(
        output, caption, transition_mode, port, request()
    );
    test.expect_true(
        cached.status == LegacyBattleLevelProfileLoadStatus::completed &&
            cached.appended_item_nodes == 0U &&
            cached.item_definition_load_calls == 0U &&
            list.nodes.size() == 2U &&
            port.LegacyBattleLevelDatabaseFixture::open_calls == 1U &&
            port.LegacyBattleMonDatabaseFixture::open_calls == 1U,
        "a second profile call shares both file sessions and does not reload an item already present in the party chain"
    );
}

void test_zero_item_and_rejected_record(openswd3::test::Context& test) {
    Port port;
    prepare_item_list(port);
    prepare_definition(port, std::array<u8, 1U>{'Z'});
    port.allocation_tokens.push_back(0x72000000U);
    const auto payload = sequential_payload();
    port.LegacyBattleLevelDatabaseFixture::custom_stream =
        make_profile_stream(payload, std::array<u16, 1U>{0U});

    LegacyWorldStoryPartyMemberResources output{};
    std::array<u8, 24U> caption{};
    u32 transition_mode = 0U;
    const auto zero = openswd3::battle::load_legacy_battle_level_profile(
        output, caption, transition_mode, port, request()
    );
    test.expect_true(
        zero.status == LegacyBattleLevelProfileLoadStatus::completed &&
            zero.diagnostic_calls == 1U && zero.appended_item_nodes == 1U &&
            port.profile_calls.size() == 2U &&
            port.profile_calls[0U].call ==
                LegacyBattleLevelProfileCall::report_zero_item &&
            port.profile_calls[0U].text_token == 0x004A7D04U &&
            port.profile_calls[0U].source_token == 0x004A7D18U &&
            port.profile_calls[0U].source_line == 0x330U,
        "zero item reports the fixed diagnostic payload and then continues through the ordinary append path"
    );

    port.LegacyBattleLevelDatabaseFixture::custom_stream = {1U, 0U};
    output.field_00 = 0xAABBCCDDU;
    const auto rejected = openswd3::battle::load_legacy_battle_level_profile(
        output, caption, transition_mode, port, request()
    );
    test.expect_true(
        rejected.status == LegacyBattleLevelProfileLoadStatus::completed &&
            !rejected.record_found && rejected.return_eax == 0U &&
            rejected.stream_release_calls == 1U &&
            output.field_00 == 0xAABBCCDDU &&
            port.LegacyBattleLevelDatabaseFixture::open_calls == 1U,
        "a nonzero first word releases the stream, returns zero, preserves output, and reuses the LEVEL session"
    );
}

void test_typed_stop_prefixes(openswd3::test::Context& test) {
    const auto payload = sequential_payload();

    {
        Port port;
        port.LegacyBattleLevelDatabaseFixture::open_succeeds = false;
        LegacyWorldStoryPartyMemberResources output{};
        output.field_00 = 9U;
        std::array<u8, 24U> caption{};
        u32 mode = 7U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status == LegacyBattleLevelProfileLoadStatus::open_failed &&
                result.return_eax == 0U && result.open_calls == 1U &&
                result.seek_calls == 0U && output.field_00 == 9U && mode == 7U,
            "LEVEL open failure returns zero without allocating or changing caller owners"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        port.LegacyBattleLevelDatabaseFixture::allocation_succeeds = false;
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        stream_zero_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == request().number_of_bytes_read_token &&
                result.stream_release_calls == 0U,
            "a zero LEVEL stream allocation stops at the original 256-dword clear"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload);
        auto limited = request();
        limited.output_accessible_bytes = 0x12U;
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, limited
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0x12U &&
                result.output_bytes_copied == 8U &&
                result.stream_cursor == 2U && result.stream_release_calls == 0U,
            "a profile destination fault preserves the completed dword-copy prefix and the unadvanced local cursor"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        std::vector<u8> stream(openswd3::battle::kLegacyBattleLevelStreamBytes);
        const auto payload_bytes = sequential_payload();
        std::copy(
            payload_bytes.begin(), payload_bytes.end(), stream.begin() + 2U
        );
        for (std::size_t offset = 28U; offset < stream.size(); offset += 2U) {
            stream[offset] = 0x80U;
        }
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            std::move(stream);
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        stream_access_typed_stop &&
                result.stopped_stream_offset == 0x400U &&
                result.stream_cursor == 0x400U &&
                result.return_eax == 0x0000FF7FU &&
                result.return_edx == result.stream_token + 0x400U &&
                result.stream_release_calls == 0U,
            "negative unknown tag bytes preserve movsx-AX and dec-EAX state until the next real stream access"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload);
        auto limited = request();
        limited.level = 0x77U;
        limited.output_accessible_bytes = 0x2CU;
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, limited
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0x2CU &&
                result.stream_cursor == 0x1CU &&
                result.output_bytes_copied == 26U &&
                result.return_eax == 0x00500201U &&
                result.return_ecx == 0x76540403U &&
                result.return_edx == 0x76543277U && output.field_2c == 0U,
            "the level-byte fault occurs after payload copy and the first two mirror loads but before the third mirror load"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        auto& list =
            *port.battle_level_profile_item_list_state()->party_item_lists[1U];
        list.nodes.front().legacy_next_token = 0x71FFFFFFU;
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::item_node_typed_stop &&
                result.stopped_item_token == 0x71FFFFFFU &&
                result.temporary_party_head_token == 0x71FFFFFFU &&
                list.legacy_head_token == 0x71FFFFFFU &&
                result.traversed_item_nodes == 1U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "a broken raw next token stops after the temporary global head advances to the inaccessible node"
        );
    }

    {
        Port port;
        port.battle_level_profile_item_list_state()
            ->party_item_lists[1U]
            .reset();
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        party_sentinel_typed_stop &&
                result.output_bytes_copied == 26U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "a missing party sentinel stops only when command one first dereferences the selected global root"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        port.diagnostic_stops = true;
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::diagnostic_typed_stop &&
                result.diagnostic_calls == 1U &&
                result.item_allocation_calls == 0U &&
                result.return_eax == 0x11112222U &&
                result.return_ecx == 0x33334444U &&
                result.return_edx == 0x55556666U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "the zero-item modal diagnostic can trap before list traversal while preserving its reply registers"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        port.allocation_tokens.push_back(0U);
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        item_allocation_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 0x2CU &&
                !result.party_root_restored &&
                result.appended_item_nodes == 0U &&
                port.battle_level_profile_item_list_state()
                        ->party_item_lists[1U]
                        ->nodes.front()
                        .legacy_next_token == 0U,
            "a zero item-node allocation publishes the null tail link and stops at the 44-dword clear"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        prepare_definition(port, std::array<u8, 1U>{'M'});
        port.allocation_tokens.push_back(0x72500000U);
        port.LegacyBattleMonDatabaseFixture::allocation_succeeds = false;
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        mon_definition_load_typed_stop &&
                result.mon_definition_load.status ==
                    openswd3::battle::LegacyBattleMonDefinitionLoadStatus::
                        stream_zero_typed_stop &&
                result.appended_item_nodes == 1U && mode == 0U &&
                port.battle_level_profile_item_list_state()
                        ->party_item_lists[1U]
                        ->legacy_head_token == 0x72500000U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "a MON stream-clear fault preserves the newly linked zeroed item and blocks transition mode, caption, root restoration, and LEVEL release"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        prepare_definition(port, std::array<u8, 1U>{'T'});
        port.allocation_tokens.push_back(0x72600000U);
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        auto inaccessible = request();
        inaccessible.transition_mode_accessible = false;
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        caption.fill(0xCCU);
        u32 mode = 9U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, inaccessible
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        transition_mode_typed_stop &&
                result.appended_item_nodes == 1U &&
                result.item_definition_load_calls == 1U && mode == 9U &&
                caption.front() == 0xCCU &&
                port.battle_level_profile_item_list_state()
                        ->party_item_lists[1U]
                        ->legacy_head_token == 0x72600000U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "an inaccessible transition-mode owner stops after MON load and before caption copy or root restoration"
        );
    }

    {
        Port port;
        prepare_item_list(port);
        prepare_definition(
            port,
            std::array<u8, 30U>{
                'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
                'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
                'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
            }
        );
        port.allocation_tokens.push_back(0x73000000U);
        port.LegacyBattleLevelDatabaseFixture::custom_stream =
            make_profile_stream(payload, std::array<u16, 1U>{0x0222U});
        LegacyWorldStoryPartyMemberResources output{};
        std::array<u8, 24U> caption{};
        u32 mode = 0U;
        const auto result = openswd3::battle::load_legacy_battle_level_profile(
            output, caption, mode, port, request()
        );
        test.expect_true(
            result.status ==
                    LegacyBattleLevelProfileLoadStatus::
                        caption_destination_typed_stop &&
                result.caption_bytes_copied == 24U &&
                result.stopped_caption_offset == 24U && mode == 1U &&
                result.appended_item_nodes == 1U &&
                port.battle_level_profile_item_list_state()
                        ->party_item_lists[1U]
                        ->legacy_head_token == 0x73000000U &&
                !result.party_root_restored &&
                result.stream_release_calls == 0U,
            "an overlong new-item name stops at the 24-byte caption boundary after linking, loading, and setting transition mode"
        );
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_real_profile_and_definition(test);
    test_profile_and_item_append(test);
    test_zero_item_and_rejected_record(test);
    test_typed_stop_prefixes(test);
    return test.exit_code();
}
