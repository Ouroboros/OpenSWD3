#pragma once

#include "openswd3/battle/legacy_battle_level_requirement.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <deque>
#include <span>
#include <utility>
#include <vector>

namespace openswd3::test {

class LegacyBattleLevelDatabaseFixture
    : public virtual battle::LegacyBattleLevelDatabasePort {
public:
    using u8 = compat::u8;
    using u16 = compat::u16;
    using u32 = compat::u32;

    [[nodiscard]] battle::LegacyBattleLevelDatabaseCallReply
    invoke_legacy_battle_level_database(
        const battle::LegacyBattleLevelDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        calls.push_back(request);
        switch (request.call) {
        case battle::LegacyBattleLevelDatabaseCall::open_file:
            ++open_calls;
            if (request.path != nullptr) {
                opened_path = *request.path;
            }
            return {
                .eax = open_succeeds ? file_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case battle::LegacyBattleLevelDatabaseCall::seek_file:
            if ((seek_calls % 2U) == 0U && request.distance >= 0x70U) {
                const u32 index = (request.distance - 0x70U) / 4U;
                requested_entries.emplace_back(index / 100U, index % 100U);
            }
            ++seek_calls;
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};

        case battle::LegacyBattleLevelDatabaseCall::read_file:
            ++read_calls;
            if ((read_calls % 2U) == 1U) {
                write_dword(destination, 0U, relative_offset);
            } else {
                auto stream = make_stream();
                if (!custom_streams.empty()) {
                    stream = std::move(custom_streams.front());
                    custom_streams.pop_front();
                    stream.resize(battle::kLegacyBattleLevelStreamBytes, 0U);
                }
                const std::size_t count =
                    std::min(destination.size(), stream.size());
                std::copy_n(stream.begin(), count, destination.begin());
            }
            return {
                .eax = request.eax,
                .ecx = read_return_ecx,
                .edx = read_return_edx,
                .bytes_read = request.requested_bytes,
            };

        case battle::LegacyBattleLevelDatabaseCall::allocate_stream:
            ++allocation_calls;
            return {
                .eax = allocation_succeeds ? stream_token : 0U,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case battle::LegacyBattleLevelDatabaseCall::release_stream:
            ++release_calls;
            return {
                .eax = release_return_eax,
                .ecx = release_return_ecx,
                .edx = release_return_edx,
            };
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    void reset_level_calls() noexcept {
        calls.clear();
        requested_entries.clear();
        open_calls = 0U;
        seek_calls = 0U;
        read_calls = 0U;
        allocation_calls = 0U;
        release_calls = 0U;
    }

    void reset_level_session() noexcept {
        legacy_battle_level_database_state() = {};
        reset_level_calls();
    }

    [[nodiscard]] std::vector<u8> make_stream() const {
        std::vector<u8> stream;
        if (!custom_stream.empty()) {
            stream = custom_stream;
        } else if (!record_available) {
            append_word(stream, 1U);
        } else {
            append_word(stream, 0U);
            stream.resize(stream.size() + 0x16U, 0U);
            append_dword(stream, level_value);
            append_word(stream, 5U);
        }
        stream.resize(battle::kLegacyBattleLevelStreamBytes, 0U);
        return stream;
    }

    static void append_word(std::vector<u8>& bytes, const u16 value) {
        bytes.push_back(static_cast<u8>(value));
        bytes.push_back(static_cast<u8>(value >> 8U));
    }

    static void append_dword(std::vector<u8>& bytes, const u32 value) {
        append_word(bytes, static_cast<u16>(value));
        append_word(bytes, static_cast<u16>(value >> 16U));
    }

    bool open_succeeds{true};
    bool allocation_succeeds{true};
    bool record_available{};
    u32 level_value{};
    u32 file_handle{0x12345678U};
    u32 stream_token{0x76543210U};
    u32 relative_offset{0x1000U};
    u32 read_return_ecx{0xA1B2C3D4U};
    u32 read_return_edx{0x11223344U};
    u32 release_return_eax{0x55667788U};
    u32 release_return_ecx{0x99AABBCCU};
    u32 release_return_edx{0xDDEEFF00U};
    u32 open_calls{};
    u32 seek_calls{};
    u32 read_calls{};
    u32 allocation_calls{};
    u32 release_calls{};
    std::filesystem::path opened_path;
    std::vector<u8> custom_stream;
    std::deque<std::vector<u8>> custom_streams;
    std::vector<std::pair<u32, u32>> requested_entries;
    std::vector<battle::LegacyBattleLevelDatabaseCallRequest> calls;

private:
    static void write_dword(
        const std::span<u8> destination,
        const std::size_t offset,
        const u32 value
    ) noexcept {
        if (offset + 4U > destination.size()) {
            return;
        }
        destination[offset] = static_cast<u8>(value);
        destination[offset + 1U] = static_cast<u8>(value >> 8U);
        destination[offset + 2U] = static_cast<u8>(value >> 16U);
        destination[offset + 3U] = static_cast<u8>(value >> 24U);
    }
};

}  // namespace openswd3::test
