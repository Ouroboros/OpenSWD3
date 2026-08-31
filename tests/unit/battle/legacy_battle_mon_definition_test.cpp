#include "openswd3/battle/legacy_battle_mon_definition.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <span>
#include <unordered_map>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleMonDatabaseCall;
using openswd3::battle::LegacyBattleMonDatabaseCallReply;
using openswd3::battle::LegacyBattleMonDatabaseCallRequest;
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
    LegacyBattleMonDatabaseCallReply invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        switch (request.call) {
        case LegacyBattleMonDatabaseCall::open_file:
            ++open_calls;
            file.open(OPENSWD3_MON_DATA_PATH, std::ios::binary);
            return {
                .eax = file.is_open() ? file_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case LegacyBattleMonDatabaseCall::seek_file: {
            ++seek_calls;
            file.clear();
            std::ios_base::seekdir direction = std::ios::beg;
            if (request.move_method == 1U) {
                direction = std::ios::cur;
            } else if (request.move_method == 2U) {
                direction = std::ios::end;
            }
            file.seekg(
                static_cast<std::streamoff>(request.distance), direction
            );
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }

        case LegacyBattleMonDatabaseCall::read_file:
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

        case LegacyBattleMonDatabaseCall::allocate_stream:
            ++stream_allocation_calls;
            return {
                .eax = stream_token,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case LegacyBattleMonDatabaseCall::release_stream:
            ++stream_release_calls;
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};

        case LegacyBattleMonDatabaseCall::query_definition_text_size: {
            ++text_size_query_calls;
            const auto found = text_sizes.find(request.block_token);
            return {
                .eax = found == text_sizes.end() ? 0U : found->second,
                .ecx = request.ecx,
                .edx = request.edx,
            };
        }

        case LegacyBattleMonDatabaseCall::allocate_definition_text: {
            ++text_allocation_calls;
            const u32 token = next_text_token;
            next_text_token += 0x100U;
            text_sizes[token] = request.allocation_size;
            return {.eax = token, .ecx = request.ecx, .edx = request.edx};
        }

        case LegacyBattleMonDatabaseCall::release_definition_text:
            ++text_release_calls;
            text_sizes.erase(request.block_token);
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    std::ifstream file;
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

    LegacyBattleMonDatabaseCallReply invoke_legacy_battle_mon_database(
        const LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        calls.push_back(request);
        switch (request.call) {
        case LegacyBattleMonDatabaseCall::open_file:
            return open_reply;

        case LegacyBattleMonDatabaseCall::seek_file:
            return {
                .eax = request.distance, .ecx = request.ecx, .edx = request.edx
            };

        case LegacyBattleMonDatabaseCall::read_file:
            if (read_index == 0U) {
                copy_dword(
                    directory_probe, destination, directory_probe_bytes_written
                );
            } else if (read_index == 1U) {
                copy_dword(
                    relative_offset, destination, relative_bytes_written
                );
            } else {
                std::copy_n(
                    stream.begin(),
                    std::min(stream.size(), destination.size()),
                    destination.begin()
                );
            }
            ++read_index;
            return {.eax = 1U, .ecx = request.ecx, .edx = request.edx};

        case LegacyBattleMonDatabaseCall::allocate_stream:
            return stream_allocation_reply;

        case LegacyBattleMonDatabaseCall::release_stream:
            return stream_release_reply;

        case LegacyBattleMonDatabaseCall::query_definition_text_size: {
            const auto found = text_sizes.find(request.block_token);
            return {
                .eax = found == text_sizes.end() ? queried_text_size
                                                 : found->second,
                .ecx = query_reply.ecx,
                .edx = query_reply.edx,
            };
        }

        case LegacyBattleMonDatabaseCall::allocate_definition_text:
            if (text_allocation_reply.eax != 0U) {
                text_sizes[text_allocation_reply.eax] = request.allocation_size;
            }
            return text_allocation_reply;

        case LegacyBattleMonDatabaseCall::release_definition_text:
            text_sizes.erase(request.block_token);
            return text_release_reply;
        }
        return {};
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
    LegacyBattleMonDatabaseCallReply open_reply{
        .eax = 0x77U,
        .ecx = 0x11112222U,
        .edx = 0x33334444U,
    };
    LegacyBattleMonDatabaseCallReply stream_allocation_reply{
        .eax = 0x71000000U,
        .ecx = 0x55556666U,
        .edx = 0x77778888U,
    };
    LegacyBattleMonDatabaseCallReply stream_release_reply{
        .eax = 0xAAAA0001U,
        .ecx = 0xBBBB0002U,
        .edx = 0xCCCC0003U,
    };
    LegacyBattleMonDatabaseCallReply query_reply{
        .eax = 0x11110000U,
        .ecx = 0x22220000U,
        .edx = 0x33330000U,
    };
    LegacyBattleMonDatabaseCallReply text_allocation_reply{
        .eax = 0x72000000U,
        .ecx = 0x44440000U,
        .edx = 0x55550000U,
    };
    LegacyBattleMonDatabaseCallReply text_release_reply{
        .eax = 0x66660000U,
        .ecx = 0x77770000U,
        .edx = 0x88880000U,
    };
    u32 directory_probe{0x1AECU};
    u32 relative_offset{0x2244U};
    u32 queried_text_size{5U};
    std::size_t directory_probe_bytes_written{4U};
    std::size_t relative_bytes_written{4U};
    std::vector<u8> stream;
    std::vector<LegacyBattleMonDatabaseCallRequest> calls;
    std::unordered_map<u32, u32> text_sizes;
    std::size_t read_index{};
};

LegacyBattleMonDefinitionLoadRequest request() {
    return {
        .path = "mon.dat",
        .output_token = 0x0053BC28U,
        .definition_id = 0xABCD0126U,
        .file_name_token = 0x004AAED0U,
        .directory_buffer_token = 0x0012FF20U,
        .stale_directory_probe_value = 0xAABBCCDDU,
        .stale_relative_offset_value = 0x11223344U,
        .number_of_bytes_read_token = 0x0012FF24U,
        .entry_eax = 0x10101010U,
        .entry_ecx = 0x20202020U,
        .entry_edx = 0x30303030U,
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
    std::vector<u8> description;

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
    std::vector<u8> description{'o', 'l', 'd', 0U};

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
            result.return_eax == 1U && result.return_ecx == 0xBBBB0002U &&
            result.return_edx == 0xCCCC0003U,
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
        port.calls.size() == 12U &&
            port.calls[0U].call ==
                LegacyBattleMonDatabaseCall::query_definition_text_size &&
            port.calls[1U].call ==
                LegacyBattleMonDatabaseCall::release_definition_text &&
            port.calls[5U].move_method == 1U &&
            port.calls[5U].distance == 0x494U &&
            port.calls[8U].allocation_size == 0x400U &&
            port.calls[10U].call ==
                LegacyBattleMonDatabaseCall::allocate_definition_text &&
            port.calls[10U].allocation_size == 12U &&
            port.calls[11U].call == LegacyBattleMonDatabaseCall::release_stream,
        "definition loader preserves relative directory seek and allocation order"
    );
}

void test_failure_prefixes(openswd3::test::Context& test) {
    {
        MonDefinitionPort port;
        port.open_reply = {
            .eax = 0xFFFFFFFFU, .ecx = 0x12345678U, .edx = 0x9ABCDEF0U
        };
        LegacyBattleMonDefinitionBytes definition{};
        definition.fill(0xA5U);
        std::vector<u8> description{'x'};

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::open_failed &&
                result.return_eax == 0U && result.return_ecx == 0x12345678U &&
                result.return_edx == 0x9ABCDEF0U && description.empty() &&
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
        std::vector<u8> description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonDefinitionLoadStatus::completed &&
                result.return_eax == 0U && result.stream_release_calls == 1U &&
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
        port.stream_allocation_reply = {
            .eax = 0U,
            .ecx = 0x11110000U,
            .edx = 0x22220000U,
        };
        LegacyBattleMonDefinitionBytes definition{};
        std::vector<u8> description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        stream_zero_typed_stop &&
                result.read_calls == 2U && result.stream_release_calls == 0U &&
                result.return_eax == 0U && result.return_ecx == 0x100U &&
                result.return_edx == 0x22220000U,
            "zero stream allocation stops at the original memset access"
        );
    }

    {
        MonDefinitionPort port;
        port.stream = full_stream();
        port.text_allocation_reply.eax = 0U;
        LegacyBattleMonDefinitionBytes definition{};
        std::vector<u8> description;

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
                result.return_eax == 0U && result.return_ecx == 3U &&
                result.return_edx == 12U,
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
        port.text_allocation_reply.eax = 0U;
        LegacyBattleMonDefinitionBytes definition{};
        std::vector<u8> description;

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            definition, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        definition_text_zero_typed_stop &&
                result.return_eax == 0U && result.return_ecx == 1U &&
                result.return_edx == 1U,
            "one-byte description allocation stops at the original byte memset with its live count"
        );
    }
}

void test_access_and_stale_boundaries(openswd3::test::Context& test) {
    {
        MonDefinitionPort port;
        port.stream = full_stream();
        std::array<u8, 0xA3U> partial{};
        std::vector<u8> description{'x'};

        const auto result = openswd3::battle::load_legacy_battle_mon_definition(
            partial, description, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonDefinitionLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0xA0U && port.calls.empty() &&
                description == std::vector<u8>{'x'},
            "short output stops at the initial owned-text token read"
        );
    }

    {
        MonDefinitionPort port;
        port.relative_offset = 0x55667788U;
        port.relative_bytes_written = 2U;
        port.stream = full_stream();
        LegacyBattleMonDefinitionBytes definition{};
        std::vector<u8> description;

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
        std::vector<u8> description;

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
        std::vector<u8> description;

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
        std::vector<u8> description;

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

}  // namespace

int main() {
    openswd3::test::Context test;
    test_real_definition_load(test);
    test_complete_definition_load(test);
    test_failure_prefixes(test);
    test_access_and_stale_boundaries(test);
    return test.exit_code();
}
