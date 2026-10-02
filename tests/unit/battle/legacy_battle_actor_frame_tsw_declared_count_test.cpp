#include "test.hpp"

#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/battle/legacy_battle_actor_frame_tsw_lookup.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;

constexpr std::size_t kIndexBytes = 0x1CU + 3000U * 0x2CU;
// 401C49 reads the next row header after the zero command at +14.
// Bit 15 is preserved in the bytes, but does not change the stream depth.
constexpr std::array<u8, 18U> kStream{
    0xFFU, 0xFFU, 1U, 0U, 1U, 0U, 16U, 0x80U,
    8U, 0U, 1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
};

void put_word(std::span<u8> bytes, std::size_t at, u16 value) {
    bytes[at] = static_cast<u8>(value);
    bytes[at + 1U] = static_cast<u8>(value >> 8U);
}

void put_dword(std::span<u8> bytes, std::size_t at, u32 value) {
    for (const u32 shift : {0U, 8U, 16U, 24U}) {
        bytes[at++] = static_cast<u8>(value >> shift);
    }
}

class TswFixture final {
public:
    TswFixture() {
        root = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("wp316-tsw-declared-count-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()
            ));
        std::filesystem::create_directories(root);
    }

    ~TswFixture() {
        std::error_code ignored;
        std::filesystem::remove_all(root, ignored);
    }

    void write(const std::span<const u8> bytes) const {
        for (const char* name : {
                 "all_char.tsw", "all_item.tsw", "all_magic.tsw",
                 "all_sys.tsw", "all_map1.tsw", "all_map2.tsw"
             }) {
            std::ofstream file{root / name, std::ios::binary | std::ios::trunc};
            for (const u8 byte : bytes) {
                file.put(static_cast<char>(byte));
            }
        }
    }

    std::filesystem::path root;
};

class UnexpectedActionUpdate final
    : public openswd3::battle::LegacyBattleActorFrameUpdatePort {
public:
    openswd3::battle::LegacyBattleActorFrameUpdateReply update(
        openswd3::asset_runtime::LegacyActionRecord&,
        u32, u32, u32, u32
    ) override {
        // lookup_frame must not dispatch an unrelated action update.
        throw std::logic_error{"unexpected action update during TSW lookup"};
    }

    openswd3::battle::LegacyBattleActorFrameUpdateReply lookup_frame(
        u32, u32, u32, u32, u32
    ) override {
        throw std::logic_error{"unexpected delegated TSW lookup"};
    }
};

}  // namespace

void test_battle_actor_frame_tsw_declared_count(
    openswd3::test::Context& test
) {
    std::vector<u8> compressed(kStream.size() + 67U);
    const auto packed = openswd3::resource_io::compress_legacy_lzo1x_14(
        kStream, compressed
    );
    test.expect_true(
        packed.status == openswd3::resource_io::LegacyLzo1xStatus::success,
        "battle physical variant fixture compresses its fixed word stream"
    );
    if (packed.status != openswd3::resource_io::LegacyLzo1xStatus::success) {
        return;
    }

    compressed.resize(packed.bytes_written);
    const TswFixture fixture;
    struct FixtureCase {
        u16 declared;
        u16 storage_bpp;
    };

    constexpr std::array<FixtureCase, 9U> cases{{
        {0U, 16U}, {1U, 16U}, {2U, 16U},
        {0U, 24U}, {1U, 24U}, {2U, 24U},
        {0U, 8U}, {1U, 8U}, {2U, 8U},
    }};
    for (const auto candidate : cases) {
        const std::size_t descriptor_base = kIndexBytes + 12U +
            (candidate.storage_bpp == 8U ? 512U : 0U);
        const std::size_t payload = descriptor_base + 2U * 36U;
        std::vector<u8> bytes(payload + compressed.size(), 0U);
        put_dword(bytes, 0x1CU + 0x14U,
                  static_cast<u32>(bytes.size() - kIndexBytes));
        put_dword(bytes, 0x1CU + 0x18U, static_cast<u32>(kIndexBytes));
        put_dword(bytes, 0x1CU + 0x1CU, 1U);
        put_word(bytes, kIndexBytes + 4U, 0xABCDU);
        put_word(bytes, kIndexBytes + 6U, candidate.declared);
        put_word(bytes, kIndexBytes + 8U, candidate.storage_bpp);
        put_word(bytes, kIndexBytes + 10U, 12U);
        for (const u32 variant : {0U, 1U}) {
            const std::size_t at = descriptor_base + variant * 36U;
            put_dword(bytes, at, static_cast<u32>(payload - kIndexBytes));
            put_dword(bytes, at + 4U, static_cast<u32>(compressed.size()));
            put_dword(bytes, at + 8U, static_cast<u32>(kStream.size()));
            put_word(bytes, at + 0x20U, static_cast<u16>(31U + variant));
            put_word(bytes, at + 0x22U, static_cast<u16>(7U + variant));
        }

        std::ranges::copy(
            compressed,
            bytes.begin() + static_cast<std::ptrdiff_t>(payload)
        );
        fixture.write(bytes);
        for (const u32 variant : {0U, 1U}) {
            openswd3::asset_runtime::LegacyTswRuntime tsw{fixture.root};
            UnexpectedActionUpdate update;
            openswd3::battle::LegacyBattleActorFrameTswUpdatePort port{
                tsw, update
            };
            const auto miss = port.lookup_frame(
                0xCAFE0001U, 0xBEEF0000U | variant,
                0x11223344U, 0x55667788U, 0x99AABBCCU
            );
            const auto hit = port.lookup_frame(1U, variant, 0U, 0U, 0U);
            test.expect_true(
                miss.returned && miss.physical_state_known && miss.flags_known &&
                    miss.eax != 0U && miss.ecx == 18U && miss.edx == 18U &&
                    !miss.flags.carry && !miss.flags.parity && !miss.flags.zero &&
                    !miss.flags.sign && !miss.flags.overflow &&
                    !miss.flags.auxiliary_carry_defined &&
                    miss.resource_header_known && miss.decoder_source_known &&
                    // 401CAB/401CAC pass record+0C/+0E to 401B70,
                    // which writes the word stream's 1x1 dimensions.
                    miss.resource_value_0c == 1U &&
                    miss.resource_value_0e == 1U &&
                    miss.resource_value_00 != miss.eax &&
                    miss.decoder_source.token == miss.resource_value_00 &&
                    std::ranges::equal(miss.decoder_source.bytes, kStream) &&
                    hit.returned && hit.physical_state_known && hit.flags_known &&
                    hit.eax == miss.eax && hit.ecx == (variant << 16U | 1U) &&
                    hit.edx == 0x004CF86CU && !hit.flags.carry &&
                    hit.flags.parity ==
                        ((std::popcount(hit.eax & 0xFFU) & 1) == 0) &&
                    !hit.flags.zero && !hit.flags.sign && !hit.flags.overflow &&
                    !hit.flags.auxiliary_carry_defined &&
                    hit.resource_header_known && hit.decoder_source_known &&
                    hit.resource_value_0c == 1U && hit.resource_value_0e == 1U &&
                    hit.decoder_source.frame_owner ==
                        miss.decoder_source.frame_owner &&
                    std::ranges::equal(hit.decoder_source.bytes, kStream),
                "battle TSW consumer reads the complete physical variant regardless of declared count, then 401B70 replaces descriptor dimensions with the word stream's dimensions; miss/hit ABI and decoder lease remain intact"
            );
        }
        if (candidate.declared == 2U && candidate.storage_bpp != 8U) {
            openswd3::asset_runtime::LegacyTswRuntime prepared{fixture.root};
            prepared.set_cache_limit(0x00400000U);
            UnexpectedActionUpdate update;
            openswd3::battle::LegacyBattleActorFrameTswUpdatePort port{
                prepared, update
            };
            const auto first = port.lookup_frame(6001U, 0U, 0U, 0U, 0U);
            const auto second = port.lookup_frame(6001U, 1U, 0U, 0U, 0U);
            const auto repeated = port.lookup_frame(6001U, 0U, 0U, 0U, 0U);
            test.expect_true(
                first.returned && first.resource_header_known &&
                    first.decoder_source_known &&
                    first.resource_value_0c == 1U &&
                    first.resource_value_0e == 1U &&
                    std::ranges::equal(first.decoder_source.bytes, kStream) &&
                    second.returned && second.resource_header_known &&
                    second.decoder_source_known &&
                    std::ranges::equal(second.decoder_source.bytes, kStream) &&
                    repeated.returned &&
                    repeated.decoder_source.frame_owner ==
                        first.decoder_source.frame_owner &&
                    prepared.cache_entry_count() == 2U &&
                    prepared.magic_preparation_slots()[0U].key == 1U &&
                    prepared.magic_prepared_stream_position(0U, 0U) ==
                        static_cast<u32>(payload) &&
                    prepared.magic_prepared_stream_position(0U, 1U) ==
                        static_cast<u32>(payload),
                "synthetic non-palette magic descriptors are prepared once, consumed by two variants, and retained across a node hit"
            );
        }
    }

    // The same complete physical record must not bypass the five-slot
    // 431AA0 path when the resource lies in 6001..9000.
    openswd3::asset_runtime::LegacyTswRuntime magic{fixture.root};
    magic.set_cache_limit(0x00400000U);
    UnexpectedActionUpdate update;
    openswd3::battle::LegacyBattleActorFrameTswUpdatePort port{magic, update};
    const auto unresolved = port.lookup_frame(6001U, 0U, 0U, 0U, 0U);
    test.expect_true(
        !unresolved.returned && !unresolved.physical_state_known &&
            !unresolved.flags_known && !unresolved.resource_header_known &&
            !unresolved.decoder_source_known && magic.cache_entry_count() == 1U,
        "battle consumer must not manufacture a normal image return before magic descriptor preparation"
    );
    const auto record_only = port.lookup_frame(6001U, 0U, 0U, 0U, 0U);
    test.expect_true(
        record_only.returned && record_only.physical_state_known &&
            record_only.flags_known && !record_only.resource_header_known &&
            !record_only.decoder_source_known && record_only.eax != 0U &&
            record_only.ecx == 6001U && record_only.edx == 0x004CF86CU,
        "battle repeat key hit returns its published record but not an invented decoded image"
    );
}
