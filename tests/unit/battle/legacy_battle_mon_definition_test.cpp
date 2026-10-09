#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <string_view>
#include <cstddef>
#include <fstream>
#include <span>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDatabaseState;
using openswd3::battle::LegacyBattleMonDefinitionBytes;
using openswd3::battle::LegacyBattleMonDefinitionLoadRequest;
using openswd3::battle::LegacyBattleMonDefinitionLoadStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void append_word(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

void append_dword(std::vector<u8>& bytes, const u32 value) {
    append_word(bytes, static_cast<u16>(value));
    append_word(bytes, static_cast<u16>(value >> 16U));
}

void append_text(std::vector<u8>& bytes, const std::vector<u8>& text) {
    bytes.insert(bytes.end(), text.begin(), text.end());
    bytes.push_back(0x24U);
    bytes.push_back(0x24U);
}

u16 read_word(
    const LegacyBattleMonDefinitionBytes& bytes, const std::size_t offset
) {
    return static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U);
}

u32 read_dword(
    const LegacyBattleMonDefinitionBytes& bytes, const std::size_t offset
) {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_dword(
    LegacyBattleMonDefinitionBytes& bytes,
    const std::size_t offset,
    const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

#ifdef OPENSWD3_MON_DATA_PATH
class RealMonDefinitionPort final : public LegacyBattleMonDatabasePort {
public:
    u32 open_mon_file(const std::filesystem::path&) override {
        ++open_calls;
        file.open(OPENSWD3_MON_DATA_PATH, std::ios::binary);
        return file.is_open() ? file_handle : 0xFFFFFFFFU;
    }

    u32 seek_mon_file(
        const u32,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        ++seek_calls;
        file.clear();
        const auto direction =
            origin == openswd3::battle::LegacyBattleMonSeekOrigin::current
            ? std::ios::cur
            : std::ios::beg;
        file.seekg(static_cast<std::streamoff>(distance), direction);
        return static_cast<u32>(file.tellg());
    }

    openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const u32, const std::span<u8> destination, const u32 requested_bytes
    ) override {
        ++read_calls;
        file.read(
            reinterpret_cast<char*>(destination.data()),
            static_cast<std::streamsize>(requested_bytes)
        );
        return {
            .succeeded = !file.bad(),
            .bytes_read = static_cast<u32>(file.gcount())
        };
    }

    openswd3::battle::LegacyBattleMonStreamAllocation
    allocate_mon_stream(const u32) override {
        ++stream_allocation_calls;
        return {.block_token = stream_token, .bytes = allocated_stream};
    }

    void release_mon_stream(const u32) override {
        ++stream_release_calls;
    }

    u32 mon_text_size(const u32 block_token) override {
        ++text_size_query_calls;
        return text_sizes.at(block_token);
    }

    openswd3::battle::LegacyBattleMonTextAllocation
    allocate_mon_text(const u32 size) override {
        ++text_allocation_calls;
        const u32 token = next_text_token;
        next_text_token += 0x100U;
        text_sizes[token] = size;
        return {
            .block_token = token,
            .storage = std::make_shared<
                openswd3::battle::LegacyBattleMonText::Storage>(size)
        };
    }

    void release_mon_text(const u32 block_token) override {
        ++text_release_calls;
        text_sizes.erase(block_token);
    }

    std::ifstream file;
    std::array<u8, openswd3::battle::kLegacyBattleMonStreamBytes> allocated_stream{};
    std::unordered_map<u32, u32> text_sizes;
    u32 file_handle{0x77U};
    u32 stream_token{0x71000000U};
    u32 next_text_token{0x72000000U};
    u32 open_calls{};
    u32 seek_calls{};
    u32 read_calls{};
    u32 stream_allocation_calls{};
    u32 stream_release_calls{};
    u32 text_size_query_calls{};
    u32 text_allocation_calls{};
    u32 text_release_calls{};
};
#endif

class MonDefinitionPort final : public LegacyBattleMonDatabasePort {
public:
    LegacyBattleMonDatabaseState&
    legacy_battle_mon_database_state() noexcept override {
        return state;
    }

    u32 open_mon_file(const std::filesystem::path&) override {
        operations.push_back("open");
        return opened_handle;
    }

    u32 seek_mon_file(
        const u32,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        operations.push_back("seek");
        seek_distances.push_back(distance);
        seek_origins.push_back(origin);
        return std::bit_cast<u32>(distance);
    }

    openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const u32, const std::span<u8> destination, const u32 requested_bytes
    ) override {
        operations.push_back("read");
        const auto target = destination.first(requested_bytes);
        const auto index = read_index++;
        if (index == 0U) {
            copy_dword(directory_probe, target, directory_probe_bytes_written);
            return {
                .succeeded = true,
                .bytes_read = static_cast<u32>(directory_probe_bytes_written)
            };
        }

        if (index == 1U) {
            copy_dword(relative_offset, target, relative_bytes_written);
            return {
                .succeeded = true,
                .bytes_read = static_cast<u32>(relative_bytes_written)
            };
        }

        const auto count = std::min(stream.size(), target.size());
        std::copy_n(stream.begin(), count, target.begin());
        return {.succeeded = true, .bytes_read = static_cast<u32>(count)};
    }

    openswd3::battle::LegacyBattleMonStreamAllocation
    allocate_mon_stream(const u32 size) override {
        operations.push_back("allocate_stream");
        stream_allocation_size = size;
        return {
            .block_token = stream_token,
            .bytes = std::span{allocated_stream}.first(stream_writable_bytes)
        };
    }

    void release_mon_stream(const u32 block_token) override {
        operations.push_back("release_stream");
        released_stream_token = block_token;
    }

    u32 mon_text_size(const u32 block_token) override {
        operations.push_back("text_size");
        const auto found = text_sizes.find(block_token);
        return found == text_sizes.end() ? queried_text_size : found->second;
    }

    openswd3::battle::LegacyBattleMonTextAllocation
    allocate_mon_text(const u32 size) override {
        operations.push_back("allocate_text");
        text_allocation_size = size;
        if (text_token == 0U) {
            return {};
        }

        text_sizes[text_token] = size;
        text_storage =
            std::make_shared<openswd3::battle::LegacyBattleMonText::Storage>(
                std::min(size, text_allocation_limit), 0xA5U
            );
        return {.block_token = text_token, .storage = text_storage};
    }

    void release_mon_text(const u32 block_token) override {
        operations.push_back("release_text");
        text_sizes.erase(block_token);
    }

    static void copy_dword(
        const u32 value,
        const std::span<u8> destination,
        const std::size_t bytes_written
    ) {
        const std::array<u8, 4U> bytes{
            static_cast<u8>(value),
            static_cast<u8>(value >> 8U),
            static_cast<u8>(value >> 16U),
            static_cast<u8>(value >> 24U),
        };
        std::copy_n(
            bytes.begin(),
            std::min({bytes.size(), destination.size(), bytes_written}),
            destination.begin()
        );
    }

    LegacyBattleMonDatabaseState state{};
    u32 opened_handle{0x77U};
    u32 stream_token{0x71000000U};
    u32 text_token{0x72000000U};
    u32 stream_allocation_size{};
    u32 text_allocation_size{};
    u32 released_stream_token{};
    std::shared_ptr<openswd3::battle::LegacyBattleMonText::Storage>
        text_storage;
    u32 directory_probe{0x1AECU};
    u32 relative_offset{0x2244U};
    u32 queried_text_size{5U};
    u32 text_allocation_limit{0xFFFFFFFFU};
    std::size_t directory_probe_bytes_written{4U};
    std::size_t relative_bytes_written{4U};
    std::vector<u8> stream;
    std::array<u8, openswd3::battle::kLegacyBattleMonStreamBytes>
        allocated_stream{};
    std::size_t stream_writable_bytes{allocated_stream.size()};
    std::vector<std::string_view> operations;
    std::vector<openswd3::compat::i32> seek_distances;
    std::vector<openswd3::battle::LegacyBattleMonSeekOrigin> seek_origins;
    std::unordered_map<u32, u32> text_sizes;
    std::size_t read_index{};
};

LegacyBattleMonDefinitionLoadRequest request() {
    return {
        .path = "mon.dat",
        .definition_id = 0xABCD0126U,
        .stale_directory_probe_value = 0xAABBCCDDU,
        .stale_relative_offset_value = 0x11223344U,
    };
}

std::vector<u8> full_stream() {
    std::vector<u8> stream;
    append_word(stream, 1000U);
    append_text(stream, {'B', 'l', 'a', 'd', 'e'});

    append_word(stream, 1U);
    for (u8 index = 0U; index < 0x4DU; ++index) {
        stream.push_back(static_cast<u8>(0x80U + index));
    }

    for (u16 tag = 6U; tag <= 22U; ++tag) {
        append_word(stream, tag);
        append_word(stream, static_cast<u16>(0x1000U + tag));
    }

    append_word(stream, 23U);
    append_word(stream, 24U);
    append_word(stream, 25U);
    append_word(stream, 0xAAAAU);
    append_dword(stream, 0x12345678U);
    append_word(stream, 26U);
    append_word(stream, 0x101AU);
    append_word(stream, 27U);
    append_word(stream, 0x101BU);
    append_word(stream, 28U);
    stream.push_back(0xE1U);
    append_word(stream, 29U);
    stream.push_back(0xE2U);
    append_word(stream, 30U);
    append_text(
        stream, {'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n'}
    );
    append_word(stream, 100U);
    append_word(stream, 0x1064U);
    append_word(stream, 2000U);
    for (u8 index = 0U; index < 9U; ++index) {
        stream.push_back(static_cast<u8>(0x40U + index));
    }
    stream.push_back(0xF1U);
    stream.push_back(0xF2U);
    append_word(stream, 0xBEEFU);
    append_word(stream, 0xCAFEU);
    append_word(stream, 5U);
    return stream;
}

void test_real_definition_load(openswd3::test::Context& test) {
#ifdef OPENSWD3_MON_DATA_PATH
    RealMonDefinitionPort port;
    LegacyBattleMonDefinitionBytes definition{};
    openswd3::battle::LegacyBattleMonText description;

    auto first_request = request();
    first_request.path = OPENSWD3_MON_DATA_PATH;
    first_request.definition_id = 1U;
    const auto first = openswd3::battle::load_legacy_battle_mon_definition(
        definition, description, port, first_request
    );

    auto second_request = request();
    second_request.path = OPENSWD3_MON_DATA_PATH;
    second_request.definition_id = 0x126U;
    const auto second = openswd3::battle::load_legacy_battle_mon_definition(
        definition, description, port, second_request
    );

    test.expect_true(
        first.status == LegacyBattleMonDefinitionLoadStatus::completed &&
            first.definition_found &&
            first.definition_relative_offset == 0x716EU &&
            first.definition_file_offset == 0x736EU &&
            first.stream_cursor == 109U && first.definition_text_bytes == 1U &&
            second.status == LegacyBattleMonDefinitionLoadStatus::completed &&
            second.definition_found && second.open_calls == 0U &&
            second.definition_id == 0x126U &&
            second.definition_relative_offset == 0xF020U &&
            second.definition_file_offset == 0xF220U &&
            second.stream_cursor == 93U,
        "real MON directory resolves stable definition one and extended definition 0x126 streams"
    );
    test.expect_true(
        std::equal(
            definition.begin(),
            definition.begin() + 8U,
            std::array<u8, 8U>{
                0xAAU, 0xF7U, 0xB5U, 0xA3U, 0xA5U, 0xC9U, 0xA4U, 0x6BU
            }
                .begin()
        ) && read_dword(definition, 0xA0U) == 0x72000100U &&
            description.size() == 39U && description.back() == 0U &&
            port.open_calls == 1U && port.seek_calls == 6U &&
            port.read_calls == 6U && port.stream_allocation_calls == 2U &&
            port.stream_release_calls == 2U &&
            port.text_size_query_calls == 1U &&
            port.text_allocation_calls == 2U && port.text_release_calls == 1U,
        "real definition loads share one file session and release the prior dynamic description before replacement"
    );

    auto saved_item_request = request();
    saved_item_request.path = OPENSWD3_MON_DATA_PATH;
    saved_item_request.definition_id = 829U;
    const auto saved_item = openswd3::battle::load_legacy_battle_mon_definition(
        definition, description, port, saved_item_request
    );
    test.expect_true(
        saved_item.status == LegacyBattleMonDefinitionLoadStatus::completed &&
            saved_item.definition_found && saved_item.definition_id == 829U,
        "the first Save/0.sav inventory ID resolves through the real MON loader"
    );
#else
    static_cast<void>(test);
#endif
}

void test_complete_definition_load(openswd3::test::Context& test) {
    MonDefinitionPort port;
    port.state.definition_text_allocation_bytes = 100U;
    port.stream = full_stream();
    LegacyBattleMonDefinitionBytes definition{};
    definition.fill(0x5AU);
    write_dword(definition, 0xA0U, 0x70000000U);
    port.text_sizes[0x70000000U] = 5U;
    openswd3::battle::LegacyBattleMonText description{'o', 'l', 'd', 0U};

    const auto result = openswd3::battle::load_legacy_battle_mon_definition(
        definition, description, port, request()
    );

    test.expect_true(
        result.status == LegacyBattleMonDefinitionLoadStatus::completed &&
            result.handle == 0x77U && result.definition_id == 0x126U &&
            result.directory_probe_value == 0x1AECU &&
            result.definition_directory_offset == 0x69CU &&
            result.definition_relative_offset == 0x2244U &&
            result.definition_file_offset == 0x2444U &&
            result.open_calls == 1U && result.seek_calls == 3U &&
            result.read_calls == 3U && result.stream_allocation_calls == 1U &&
            result.stream_release_calls == 1U &&
            result.definition_text_size_query_calls == 1U &&
            result.definition_text_release_calls == 1U &&
            result.definition_text_allocation_calls == 1U &&
            result.definition_found,
        "definition load preserves directory, shared handle and lifecycle calls"
    );
    test.expect_true(
        std::equal(
            definition.begin(),
            definition.begin() + 5,
            std::array<u8, 5U>{'B', 'l', 'a', 'd', 'e'}.begin()
        ) && read_dword(definition, 0x20U) == 0x12345678U &&
            read_word(definition, 0x24U) == 0x1007U &&
            read_word(definition, 0x26U) == 0x1008U &&
            read_word(definition, 0x28U) == 0x100FU &&
            read_word(definition, 0x3EU) == 0x1064U &&
            read_word(definition, 0x40U) == 0x1006U &&
            read_word(definition, 0x46U) == 0x100BU &&
            read_word(definition, 0x48U) == 0x1016U &&
            definition[0x92U] == 0x40U && definition[0x9AU] == 0x48U &&
            definition[0x9BU] == 0xF1U && definition[0x9CU] == 0xF2U &&
            read_word(definition, 0x52U) == 0xCAFEU &&
            read_word(definition, 0x54U) == 0xBEEFU &&
            read_dword(definition, 0xA0U) == 0x72000000U,
        "all definition tags preserve exact widths, aliases and extended fields"
    );
    test.expect_true(
        description ==
                std::vector<u8>{
                    'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0U
                } &&
            port.state.definition_text_allocation_bytes == 107U,
        "owned description replaces the released text and wraps allocation accounting"
    );
    test.expect_true(
        port.operations ==
                std::vector<std::string_view>{
                    "text_size",
                    "release_text",
                    "open",
                    "seek",
                    "read",
                    "seek",
                    "read",
                    "seek",
                    "allocate_stream",
                    "read",
                    "allocate_text",
                    "release_stream"
                } &&
            port.seek_origins[1U] ==
                openswd3::battle::LegacyBattleMonSeekOrigin::current &&
            port.seek_distances[1U] == 0x494 &&
            port.stream_allocation_size == 0x400U &&
            port.text_allocation_size == 12U &&
            port.released_stream_token == port.stream_token,
        "definition loader preserves relative directory seek and allocation order"
    );
}

void test_failure_prefixes(openswd3::test::Context& test) {
    {
        MonDefinitionPort port;
        port.opened_handle = 0xFFFFFFFFU;
        LegacyBattleMonDefinitionBytes definition{};
        definition.fill(0xA5U);
        openswd3::battle::LegacyBattleMonText description{'x'};

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::open_failed &&
                !result.definition_found && description.empty() &&
                std::all_of(
                    definition.begin(),
                    definition.end(),
                    [](const u8 value) { return value == 0U; }
                ),
            "open failure keeps the unconditional definition clear prefix"
        );
    }

    {
        MonDefinitionPort port;
        port.stream = {0U, 0U};
        LegacyBattleMonDefinitionBytes definition{};
        definition.fill(0xA5U);
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::completed &&
                !result.definition_found && result.stream_release_calls == 1U &&
                std::all_of(
                    definition.begin(),
                    definition.end(),
                    [](const u8 value) { return value == 0U; }
                ),
            "invalid first tag frees the stream after preserving the clear prefix"
        );
    }

    {
        MonDefinitionPort port;
        port.stream_token = 0U;
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        stream_zero_typed_stop &&
                result.read_calls == 2U && result.stream_release_calls == 0U &&
                !result.definition_found,
            "zero stream allocation stops at the original memset access"
        );
    }

    {
        MonDefinitionPort port;
        port.stream = full_stream();
        port.text_token = 0U;
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        definition_text_zero_typed_stop &&
                result.definition_text_allocation_calls == 1U &&
                result.stream_release_calls == 0U &&
                read_dword(definition, 0xA0U) == 0U && description.empty() &&
                !result.definition_found,
            "zero description allocation preserves parsed fields and stops before text writes"
        );
    }

    {
        MonDefinitionPort port;
        append_word(port.stream, 1000U);
        append_text(port.stream, {});
        append_word(port.stream, 30U);
        append_text(port.stream, {});
        append_word(port.stream, 5U);
        port.text_token = 0U;
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        definition_text_zero_typed_stop &&
                !result.definition_found,
            "one-byte description allocation stops at the original byte memset with its live count"
        );
    }
}

void test_access_and_stale_boundaries(openswd3::test::Context& test) {
    {
        MonDefinitionPort port;
        port.stream = full_stream();
        std::array<u8, 0xA3U> partial{};
        openswd3::battle::LegacyBattleMonText description{'x'};

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            partial, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0xA0U &&
                port.operations.empty() && description == std::vector<u8>{'x'},
            "short output stops at the initial owned-text token read"
        );
    }

    {
        MonDefinitionPort port;
        port.relative_offset = 0x55667788U;
        port.relative_bytes_written = 2U;
        port.stream = full_stream();
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::completed &&
                result.definition_relative_offset == 0x11227788U &&
                result.definition_file_offset == 0x11227988U,
            "short relative-offset read preserves the request-supplied stale high bytes"
        );
    }

    {
        MonDefinitionPort port;
        append_word(port.stream, 1000U);
        port.stream.insert(port.stream.end(), 170U, static_cast<u8>('A'));
        port.stream.push_back(0x24U);
        port.stream.push_back(0x24U);
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0xA4U &&
                result.stream_release_calls == 0U &&
                std::all_of(
                    definition.begin(),
                    definition.end(),
                    [](const u8 value) { return value == static_cast<u8>('A'); }
                ),
            "overlong name preserves all 164 written bytes before typed stop"
        );
    }

    {
        MonDefinitionPort port;
        append_word(port.stream, 1000U);
        append_text(port.stream, {});
        append_word(port.stream, 30U);
        port.stream.insert(port.stream.end(), 0xFFU, static_cast<u8>('D'));
        append_word(port.stream, 5U);
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::completed &&
                result.stream_cursor == 0x107U &&
                result.definition_text_allocation_calls == 0U &&
                result.stream_release_calls == 1U && description.empty(),
            "unterminated description advances by 255 bytes before the next tag read"
        );
    }

    {
        MonDefinitionPort port;
        port.stream.resize(0x400U, 0U);
        port.stream[0U] = 0xE8U;
        port.stream[1U] = 0x03U;
        port.stream[2U] = 0x24U;
        port.stream[3U] = 0x24U;
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        stream_access_typed_stop &&
                result.stopped_stream_offset == 0x400U &&
                result.stream_cursor == 0x400U &&
                result.stream_release_calls == 0U,
            "missing terminator scans the fixed zero-filled 1024-byte window"
        );
    }
}

void test_allocated_text_extent(openswd3::test::Context& test) {
    // 0x0047707B clears whole dwords; 0x00477085 clears the byte tail.
    constexpr std::array<std::array<u32, 3U>, 6U> cases{{
        {11U, 0U, 0U},
        {11U, 2U, 0U},
        {11U, 6U, 4U},
        {11U, 11U, 8U},
        {4U, 4U, 4U},
        {2U, 1U, 1U},
    }};
    for (const auto& [length, extent, stopped] : cases) {
        MonDefinitionPort port;
        append_word(port.stream, 1000U);
        append_text(port.stream, {});
        append_word(port.stream, 30U);
        append_text(port.stream, std::vector<u8>(length, 'D'));
        append_word(port.stream, 5U);
        port.text_allocation_limit = extent;
        LegacyBattleMonDefinitionBytes definition{};
        openswd3::battle::LegacyBattleMonText description;
        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );
        test.expect_equal(
            result.status,
            LegacyBattleMonDefinitionLoadStatus::
                definition_text_access_typed_stop,
            "text clear stops at its first inaccessible store"
        );
        test.expect_equal(
            result.stopped_definition_text_offset,
            stopped,
            "text stop preserves the instruction boundary"
        );
        test.expect_equal(
            result.stream_release_calls,
            0U,
            "text access stop does not release the stream"
        );
        test.expect_equal(
            read_dword(definition, 0xA0U),
            port.text_token,
            "text identity is published before zeroing"
        );
        test.expect_true(
            description.data() == port.text_storage->data(),
            "text writes target the allocation returned by the heap port"
        );
        for (std::size_t index = 0U; index < description.size(); ++index) {
            test.expect_equal(
                description[index],
                index < stopped ? 0U : 0xA5U,
                "failed text clear preserves its exact prefix"
            );
        }
    }
}

void test_allocated_stream_extent(openswd3::test::Context& test) {
    for (const std::size_t extent : {0U, 2U, 6U, 1023U}) {
        MonDefinitionPort port;
        port.allocated_stream.fill(0xA5U);
        port.stream_writable_bytes = extent;
        std::array<u8, 0xA4U> output{};
        openswd3::battle::LegacyBattleMonText description;
        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            output, description, port, request()
        );
        const auto stopped = static_cast<u32>(extent / 4U * 4U);
        test.expect_equal(
            result.status,
            LegacyBattleMonDefinitionLoadStatus::stream_access_typed_stop,
            "definition stops at the original inaccessible STOSD"
        );
        test.expect_equal(
            result.stopped_stream_offset,
            stopped,
            "definition stops before a partial dword store"
        );
        test.expect_equal(
            result.read_calls, 2U, "no stream read after failed memset"
        );
        test.expect_equal(
            result.stream_release_calls,
            0U,
            "definition does not release on memset stop"
        );
        for (std::size_t index = 0U; index < port.allocated_stream.size();
             ++index) {
            test.expect_equal(
                port.allocated_stream[index],
                index < stopped ? 0U : 0xA5U,
                "definition preserves the exact STOSD prefix"
            );
        }
    }
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_real_definition_load(test);
    test_complete_definition_load(test);
    test_failure_prefixes(test);
    test_access_and_stale_boundaries(test);
    test_allocated_stream_extent(test);
    test_allocated_text_extent(test);
    return test.exit_code();
}
