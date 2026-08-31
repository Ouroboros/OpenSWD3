#pragma once

#include "openswd3/battle/legacy_battle_mon_profile.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace openswd3::test {

class LegacyBattleMonDatabaseFixture
    : public virtual battle::LegacyBattleMonDatabasePort {
public:
    using u8 = compat::u8;
    using u16 = compat::u16;
    using u32 = compat::u32;

    [[nodiscard]] battle::LegacyBattleMonDatabaseCallReply
    invoke_legacy_battle_mon_database(
        const battle::LegacyBattleMonDatabaseCallRequest& request,
        const std::span<u8> destination
    ) override {
        calls.push_back(request);
        switch (request.call) {
        case battle::LegacyBattleMonDatabaseCall::open_file:
            ++open_calls;
            if (request.path != nullptr) {
                opened_path = *request.path;
            }
            return {
                .eax = open_succeeds ? file_handle : 0xFFFFFFFFU,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case battle::LegacyBattleMonDatabaseCall::seek_file:
            if (seek_calls % 3U == 1U) {
                requested_profile_ids.push_back(
                    static_cast<u16>(
                        (request.distance - auxiliary_root - 0x200U) / 4U
                    )
                );
            }
            ++seek_calls;
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};

        case battle::LegacyBattleMonDatabaseCall::read_file: {
            ++read_calls;
            const u32 phase = (read_calls - 1U) % 3U;
            if (phase == 0U) {
                write_dword(destination, 0U, auxiliary_root);
            } else if (phase == 1U) {
                write_dword(destination, 0U, profile_relative_offset);
            } else {
                const auto stream = make_stream();
                const std::size_t count =
                    std::min(destination.size(), stream.size());
                for (std::size_t i = 0U; i < count; ++i) {
                    destination[i] = stream[i];
                }
            }
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }

        case battle::LegacyBattleMonDatabaseCall::allocate_stream:
            ++allocation_calls;
            return {
                .eax = allocation_succeeds ? stream_token : 0U,
                .ecx = request.ecx,
                .edx = request.edx,
            };

        case battle::LegacyBattleMonDatabaseCall::release_stream:
            ++release_calls;
            return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
        }
        return {.eax = request.eax, .ecx = request.ecx, .edx = request.edx};
    }

    void reset_mon_calls() noexcept {
        calls.clear();
        requested_profile_ids.clear();
        open_calls = 0U;
        seek_calls = 0U;
        read_calls = 0U;
        allocation_calls = 0U;
        release_calls = 0U;
    }

    void reset_mon_session() noexcept {
        legacy_battle_mon_database_state() = {};
        reset_mon_calls();
    }

    void clear_profile() noexcept {
        profile.fill(std::byte{0});
    }

    void set_profile_word(const std::size_t offset, const u16 value) noexcept {
        profile[offset] = static_cast<std::byte>(value);
        profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
    }

    void set_profile_dword(const std::size_t offset, const u32 value) noexcept {
        profile[offset] = static_cast<std::byte>(value);
        profile[offset + 1U] = static_cast<std::byte>(value >> 8U);
        profile[offset + 2U] = static_cast<std::byte>(value >> 16U);
        profile[offset + 3U] = static_cast<std::byte>(value >> 24U);
    }

    [[nodiscard]] u16 profile_word(const std::size_t offset) const noexcept {
        return std::to_integer<u16>(profile[offset]) |
            static_cast<u16>(std::to_integer<u16>(profile[offset + 1U]) << 8U);
    }

    [[nodiscard]] u32 profile_dword(const std::size_t offset) const noexcept {
        return std::to_integer<u32>(profile[offset]) |
            (std::to_integer<u32>(profile[offset + 1U]) << 8U) |
            (std::to_integer<u32>(profile[offset + 2U]) << 16U) |
            (std::to_integer<u32>(profile[offset + 3U]) << 24U);
    }

    battle::LegacyBattleMonProfile profile{};
    bool open_succeeds{true};
    bool allocation_succeeds{true};
    u32 file_handle{0x11223344U};
    u32 stream_token{0x55667788U};
    u32 auxiliary_root{0x1AECU};
    u32 profile_relative_offset{0x2000U};
    u32 open_calls{};
    u32 seek_calls{};
    u32 read_calls{};
    u32 allocation_calls{};
    u32 release_calls{};
    std::string opened_path;
    std::vector<u16> requested_profile_ids;
    std::vector<battle::LegacyBattleMonDatabaseCallRequest> calls;

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

    static void append_word(std::vector<u8>& bytes, const u16 value) {
        bytes.push_back(static_cast<u8>(value));
        bytes.push_back(static_cast<u8>(value >> 8U));
    }

    static void append_dword(std::vector<u8>& bytes, const u32 value) {
        append_word(bytes, static_cast<u16>(value));
        append_word(bytes, static_cast<u16>(value >> 16U));
    }

    [[nodiscard]] std::array<u8, battle::kLegacyBattleMonStreamBytes>
    make_stream() const {
        std::vector<u8> bytes;
        bytes.reserve(128U);

        append_word(bytes, 0U);
        append_dword(bytes, profile_dword(0x0CU));
        append_dword(bytes, profile_dword(0x10U));

        append_word(bytes, 2U);
        bytes.push_back(static_cast<u8>(profile_word(0x24U)));
        bytes.push_back(0U);

        const u32 flags = profile_dword(0x04U);
        const auto append_flag = [&bytes,
                                  flags](const u32 mask, const u16 tag) {
            if ((flags & mask) != 0U) {
                append_word(bytes, tag);
            }
        };
        append_flag(0x00000001U, 3U);
        append_word(bytes, 4U);
        append_word(bytes, profile_word(0x16U));
        append_word(bytes, (flags & 0x00000080U) != 0U ? 6U : 7U);
        append_word(bytes, profile_word(0x18U));
        append_word(bytes, 8U);
        append_word(bytes, profile_word(0x14U));
        append_flag(0x00000002U, 9U);
        if ((profile_word(0x1EU) & 0x8000U) != 0U) {
            append_word(bytes, 10U);
            append_word(bytes, profile_word(0x1EU));
        }
        append_flag(0x00000004U, 11U);
        append_word(bytes, 12U);
        append_dword(bytes, profile_dword(0x08U));
        append_word(bytes, 13U);
        append_word(bytes, profile_word(0x1CU));
        if ((flags & 0x00000008U) != 0U) {
            append_word(bytes, 14U);
            append_word(bytes, profile_word(0x1AU));
        }
        append_flag(0x00000010U, 15U);
        append_flag(0x00000020U, 16U);
        append_flag(0x00000040U, 17U);
        append_flag(0x00000100U, 18U);
        append_flag(0x00000200U, 19U);
        append_flag(0x00000400U, 20U);
        append_flag(0x00000800U, 21U);
        if ((flags & 0x00001000U) != 0U) {
            append_word(bytes, 22U);
            append_word(bytes, profile_word(0x20U));
            bytes.push_back(static_cast<u8>(profile_word(0x24U)));
            bytes.push_back(0U);
        }
        append_word(bytes, 23U);
        append_word(bytes, profile_word(0x22U));
        append_flag(0x00002000U, 24U);
        append_flag(0x00004000U, 25U);
        append_word(bytes, 5U);

        std::array<u8, battle::kLegacyBattleMonStreamBytes> stream{};
        for (std::size_t i = 0U; i < bytes.size(); ++i) {
            stream[i] = bytes[i];
        }
        return stream;
    }
};

}  // namespace openswd3::test
