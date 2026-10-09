#include "openswd3/battle/legacy_battle_mon_profile.hpp"
#include "test.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <vector>

namespace {

using openswd3::battle::LegacyBattleMonDatabasePort;
using openswd3::battle::LegacyBattleMonDatabaseState;
using openswd3::battle::LegacyBattleMonProfile;
using openswd3::battle::LegacyBattleMonProfileLoadRequest;
using openswd3::battle::LegacyBattleMonProfileLoadStatus;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

void write_u16(std::vector<u8>& bytes, const u16 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
}

void write_u32(std::vector<u8>& bytes, const u32 value) {
    bytes.push_back(static_cast<u8>(value));
    bytes.push_back(static_cast<u8>(value >> 8U));
    bytes.push_back(static_cast<u8>(value >> 16U));
    bytes.push_back(static_cast<u8>(value >> 24U));
}

u16 read_u16(const LegacyBattleMonProfile& profile, const std::size_t offset) {
    return static_cast<u16>(profile[offset]) |
        static_cast<u16>(static_cast<u16>(profile[offset + 1U]) << 8U);
}

u32 read_u32(const LegacyBattleMonProfile& profile, const std::size_t offset) {
    return static_cast<u32>(profile[offset]) |
        (static_cast<u32>(profile[offset + 1U]) << 8U) |
        (static_cast<u32>(profile[offset + 2U]) << 16U) |
        (static_cast<u32>(profile[offset + 3U]) << 24U);
}

void write_profile_u32(
    LegacyBattleMonProfile& profile, const std::size_t offset, const u32 value
) {
    profile[offset] = static_cast<std::byte>(value);
    profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
    profile[offset + 2U] = static_cast<std::byte>(value >> 16U);
    profile[offset + 3U] = static_cast<std::byte>(value >> 24U);
}

class MonPort final : public LegacyBattleMonDatabasePort {
public:
    LegacyBattleMonDatabaseState&
    legacy_battle_mon_database_state() noexcept override {
        return state;
    }

    u32 open_mon_file(const std::filesystem::path& path) override {
        opened_path = path;
        return opened_handle;
    }

    u32 seek_mon_file(
        const u32 handle,
        const openswd3::compat::i32 distance,
        const openswd3::battle::LegacyBattleMonSeekOrigin origin
    ) override {
        seek_handles.push_back(handle);
        seek_positions.push_back(std::bit_cast<u32>(distance));
        seek_origins.push_back(origin);
        return std::bit_cast<u32>(distance);
    }

    openswd3::battle::LegacyBattleMonReadResult read_mon_file(
        const u32, const std::span<u8> destination, const u32 requested_bytes
    ) override {
        read_sizes.push_back(requested_bytes);
        const auto target = destination.first(requested_bytes);
        const auto index = read_index++;
        if (index == 0U) {
            copy_dword(root, target, root_bytes_written);
            return {
                .succeeded = true,
                .bytes_read = static_cast<u32>(root_bytes_written)
            };
        }

        if (index == 1U) {
            copy_dword(relative, target, relative_bytes_written);
            return {
                .succeeded = true,
                .bytes_read = static_cast<u32>(relative_bytes_written)
            };
        }

        const auto count = std::min(target.size(), stream.size());
        std::copy_n(stream.begin(), count, target.begin());
        return {.succeeded = true, .bytes_read = static_cast<u32>(count)};
    }

    openswd3::battle::LegacyBattleMonStreamAllocation
    allocate_mon_stream(const u32 size) override {
        allocation_sizes.push_back(size);
        return {
            .block_token = allocation_token,
            .bytes = std::span{allocated_stream}.first(stream_writable_bytes),
        };
    }

    void release_mon_stream(const u32 block_token) override {
        released_tokens.push_back(block_token);
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
    u32 allocation_token{0x71000000U};
    u32 root{0x00001AECU};
    u32 relative{0x00002244U};
    std::size_t root_bytes_written{4U};
    std::size_t relative_bytes_written{4U};
    std::vector<u8> stream;
    std::array<u8, openswd3::battle::kLegacyBattleMonStreamBytes>
        allocated_stream{};
    std::size_t stream_writable_bytes{allocated_stream.size()};
    std::filesystem::path opened_path;
    std::vector<u32> seek_handles;
    std::vector<u32> seek_positions;
    std::vector<openswd3::battle::LegacyBattleMonSeekOrigin> seek_origins;
    std::vector<u32> read_sizes;
    std::vector<u32> allocation_sizes;
    std::vector<u32> released_tokens;
    std::size_t read_index{};
};

#ifdef OPENSWD3_MON_DATA_PATH
class RealMonPort final : public LegacyBattleMonDatabasePort {
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
        ++allocation_calls;
        return {.block_token = stream_token, .bytes = allocated_stream};
    }

    void release_mon_stream(const u32) override {
        ++release_calls;
    }

    std::ifstream file;
    std::array<u8, openswd3::battle::kLegacyBattleMonStreamBytes>
        allocated_stream{};
    u32 file_handle{0x00000077U};
    u32 stream_token{0x71000000U};
    u32 open_calls{};
    u32 seek_calls{};
    u32 read_calls{};
    u32 allocation_calls{};
    u32 release_calls{};
};
#endif

LegacyBattleMonProfileLoadRequest request() {
    return {
        .path = std::filesystem::path{"data"} / "mon.dat",
        .profile_id = 0xCAFE0002U,
        .stale_root_buffer_value = 0xAABBCCDDU,
    };
}

std::vector<u8> full_stream() {
    std::vector<u8> stream;
    write_u16(stream, 0U);
    write_u32(stream, 0x11223344U);
    write_u32(stream, 0x55667788U);
    write_u16(stream, 1U);
    write_u16(stream, 0x99U);
    write_u16(stream, 2U);
    write_u16(stream, 0x00ABU);
    write_u16(stream, 3U);
    write_u16(stream, 4U);
    write_u16(stream, 0x1004U);
    write_u16(stream, 6U);
    write_u16(stream, 0x1006U);
    write_u16(stream, 7U);
    write_u16(stream, 0x1007U);
    write_u16(stream, 8U);
    write_u16(stream, 0x1008U);
    write_u16(stream, 9U);
    write_u16(stream, 10U);
    write_u16(stream, 0x1234U);
    write_u16(stream, 11U);
    write_u16(stream, 12U);
    write_u32(stream, 0x89ABCDEFU);
    write_u16(stream, 13U);
    write_u16(stream, 0x100DU);
    write_u16(stream, 14U);
    write_u16(stream, 0x100EU);
    for (u16 tag = 15U; tag <= 21U; ++tag) {
        write_u16(stream, tag);
    }
    write_u16(stream, 22U);
    write_u32(stream, 0xA1B2C3D4U);
    write_u16(stream, 23U);
    write_u16(stream, 0x1017U);
    write_u16(stream, 24U);
    write_u16(stream, 25U);
    write_u16(stream, 5U);
    return stream;
}

}  // namespace

void test_battle_mon_profile(openswd3::test::Context& test) {
    for (const std::size_t extent : {0U, 2U, 6U, 1023U}) {
        MonPort port;
        port.allocated_stream.fill(0xA5U);
        port.stream_writable_bytes = extent;
        LegacyBattleMonProfile profile{};
        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );
        const auto stopped = static_cast<u32>(extent / 4U * 4U);
        test.expect_equal(
            result.status,
            LegacyBattleMonProfileLoadStatus::stream_access_typed_stop,
            "missing allocated storage stops at original STOSD"
        );
        test.expect_equal(
            result.stopped_stream_offset,
            stopped,
            "profile stops before a partial dword store"
        );
        test.expect_equal(
            result.read_calls, 2U, "no stream read after failed memset"
        );
        test.expect_equal(
            result.release_calls, 0U, "no premature stream release"
        );
        for (std::size_t index = 0U; index < port.allocated_stream.size();
             ++index) {
            test.expect_equal(
                port.allocated_stream[index],
                index < stopped ? 0U : 0xA5U,
                "profile preserves the exact STOSD prefix"
            );
        }
    }

#ifdef OPENSWD3_MON_DATA_PATH
    {
        RealMonPort port;
        LegacyBattleMonProfile profile_zero{};
        auto zero_request = request();
        zero_request.path = OPENSWD3_MON_DATA_PATH;
        zero_request.profile_id = 0U;
        const auto zero_result =
            openswd3::battle::load_legacy_battle_mon_profile(
                profile_zero, port, zero_request
            );

        LegacyBattleMonProfile profile_twenty_one{};
        auto twenty_one_request = request();
        twenty_one_request.path = OPENSWD3_MON_DATA_PATH;
        twenty_one_request.profile_id = 21U;
        const auto twenty_one_result =
            openswd3::battle::load_legacy_battle_mon_profile(
                profile_twenty_one, port, twenty_one_request
            );

        test.expect_true(
            zero_result.status == LegacyBattleMonProfileLoadStatus::completed &&
                zero_result.auxiliary_root == 0x1AECU &&
                zero_result.profile_relative_offset == 0x2244U &&
                zero_result.profile_file_offset == 0x2444U &&
                zero_result.stream_cursor == 24U &&
                twenty_one_result.status ==
                    LegacyBattleMonProfileLoadStatus::completed &&
                twenty_one_result.open_calls == 0U &&
                twenty_one_result.profile_relative_offset == 0x2430U &&
                twenty_one_result.profile_file_offset == 0x2630U &&
                twenty_one_result.stream_cursor == 22U,
            "real MON directory resolves stable profile zero and twenty-one streams"
        );
        test.expect_true(
            read_u32(profile_zero, 0x0CU) == 8U &&
                read_u32(profile_zero, 0x10U) == 9U &&
                read_u16(profile_zero, 0x14U) == 0x1F44U &&
                read_u16(profile_zero, 0x16U) == 0x000AU &&
                read_u16(profile_zero, 0x1CU) == 3U &&
                read_u32(profile_twenty_one, 0x04U) == 1U &&
                read_u32(profile_twenty_one, 0x0CU) == 4U &&
                read_u16(profile_twenty_one, 0x14U) == 0x1965U &&
                read_u16(profile_twenty_one, 0x16U) == 1U,
            "real MON samples project their exact tag payload fields"
        );
        test.expect_true(
            port.open_calls == 1U && port.seek_calls == 6U &&
                port.read_calls == 6U && port.allocation_calls == 2U &&
                port.release_calls == 2U,
            "real MON samples share one lazy handle across consecutive loads"
        );
    }
#endif

    {
        MonPort port;
        port.stream = full_stream();
        LegacyBattleMonProfile profile{};
        write_profile_u32(profile, 0x04U, 0x80000000U);

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonProfileLoadStatus::completed &&
                result.handle == 0x77U && result.profile_id == 2U &&
                result.auxiliary_root == 0x1AECU &&
                result.profile_relative_offset == 0x2244U &&
                result.profile_file_offset == 0x2444U &&
                result.open_calls == 1U && result.seek_calls == 3U &&
                result.read_calls == 3U && result.allocation_calls == 1U &&
                result.release_calls == 1U && result.profile_found,
            "successful load reuses the lazy handle and releases the temporary stream"
        );
        test.expect_true(
            read_u32(profile, 0x04U) == 0x80007FFFU &&
                read_u32(profile, 0x08U) == 0x89ABCDEFU &&
                read_u32(profile, 0x0CU) == 0x11223344U &&
                read_u32(profile, 0x10U) == 0x55667788U &&
                read_u16(profile, 0x14U) == 0x1008U &&
                read_u16(profile, 0x16U) == 0x1004U &&
                read_u16(profile, 0x18U) == 0x1007U &&
                read_u16(profile, 0x1AU) == 0x100EU &&
                read_u16(profile, 0x1CU) == 0x100DU &&
                read_u16(profile, 0x1EU) == 0x9234U &&
                read_u16(profile, 0x20U) == 0xC3D4U &&
                read_u16(profile, 0x22U) == 0x1017U &&
                static_cast<u8>(profile[0x24U]) == 0xB2U,
            "all command payloads and flags use their exact legacy widths"
        );
        test.expect_true(
            port.opened_path == request().path &&
                port.seek_positions ==
                    std::vector<u32>{0x204U, 0x1CF4U, 0x2444U} &&
                port.read_sizes == std::vector<u32>{4U, 4U, 0x400U} &&
                port.allocation_sizes == std::vector<u32>{0x400U} &&
                port.released_tokens == std::vector<u32>{0x71000000U},
            "file offsets, low-word profile id, buffer size and released allocation match LST"
        );
    }

    {
        MonPort port;
        port.state = {.open = true, .handle = 0x88U};
        port.root = 0x11223344U;
        port.root_bytes_written = 2U;
        port.relative = 0x00000010U;
        port.stream = full_stream();
        auto load_request = request();
        load_request.profile_id = 1U;
        LegacyBattleMonProfile profile{};

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, load_request
        );

        test.expect_true(
            result.status == LegacyBattleMonProfileLoadStatus::completed &&
                result.open_calls == 0U && result.handle == 0x88U &&
                result.auxiliary_root == 0xAABB3344U &&
                port.seek_handles.front() == 0x88U,
            "cached path reuses the handle and ReadFile short writes keep local stale bytes"
        );
    }

    {
        MonPort port;
        port.opened_handle = 0xFFFFFFFFU;
        LegacyBattleMonProfile profile{};

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonProfileLoadStatus::open_failed &&
                !port.state.open && port.state.handle == 0xFFFFFFFFU &&
                result.open_calls == 1U && result.seek_calls == 0U &&
                !result.profile_found,
            "CreateFile failure returns zero and leaves lazy-open clear"
        );
    }

    {
        MonPort port;
        port.stream = {1U, 0U};
        LegacyBattleMonProfile profile{};
        profile.fill(std::byte{0x5A});

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );

        test.expect_true(
            result.status == LegacyBattleMonProfileLoadStatus::completed &&
                result.release_calls == 1U && !result.profile_found &&
                std::all_of(
                    profile.begin(),
                    profile.end(),
                    [](const std::byte value) {
                        return value == std::byte{0x5A};
                    }
                ),
            "invalid auxiliary stream preserves output and frees the temporary block"
        );
    }

    {
        MonPort port;
        port.allocation_token = 0U;
        LegacyBattleMonProfile profile{};

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonProfileLoadStatus::stream_zero_typed_stop &&
                result.read_calls == 2U && result.release_calls == 0U &&
                !result.profile_found,
            "allocator zero preserves prior seeks and stops before ReadFile"
        );
    }

    {
        MonPort port;
        port.stream = full_stream();
        std::array<std::byte, 0x0FU> partial{};

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            partial, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonProfileLoadStatus::
                        output_access_typed_stop &&
                result.stopped_output_offset == 0x0CU &&
                result.stream_cursor == 10U && result.release_calls == 0U,
            "typed stop is exactly after payload read and cursor advance"
        );
    }

    {
        MonPort port;
        port.stream.resize(0x400U, 0U);
        port.stream[0U] = 0U;
        port.stream[1U] = 0U;
        for (std::size_t offset = 10U; offset < port.stream.size();
             offset += 2U) {
            port.stream[offset] = 1U;
        }
        LegacyBattleMonProfile profile{};

        const auto result = openswd3::battle::load_legacy_battle_mon_profile(
            profile, port, request()
        );

        test.expect_true(
            result.status ==
                    LegacyBattleMonProfileLoadStatus::
                        stream_access_typed_stop &&
                result.stopped_stream_offset == 0x400U &&
                result.stream_cursor == 0x400U && result.release_calls == 0U,
            "missing terminator preserves command-one scan through the fixed read window"
        );
    }
}
