#include "test.hpp"

#include "openswd3/asset_runtime/legacy_action_draw_bridge.hpp"
#include "openswd3/asset_runtime/legacy_guest_address_reservation.hpp"
#include "openswd3/asset_runtime/legacy_tsw_runtime.hpp"
#include "openswd3/rendering/legacy_image_command_stream.hpp"
#include "openswd3/resource_io/legacy_lzo1x.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::asset_runtime::LegacyTswDirectResult;
using openswd3::asset_runtime::LegacyTswFrameStatus;
using openswd3::asset_runtime::LegacyTswQueryResult;
using openswd3::asset_runtime::LegacyTswRuntime;
using openswd3::asset_runtime::LegacyTswRuntimeFrame;
using openswd3::asset_runtime::LegacyTswRuntimeStatus;
using openswd3::asset_runtime::LegacyTswSpecialFrameLoader;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::compat::u8;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kBlockOffset = kIndexOffset + kRecordSize * kSlotCount;
constexpr std::size_t kPaletteSize = 512U;

constexpr std::array<const char*, 6> kArchiveNames{
    "all_char.tsw",
    "all_item.tsw",
    "all_magic.tsw",
    "all_sys.tsw",
    "all_map1.tsw",
    "all_map2.tsw",
};

constexpr std::array<u8, 38> kStream8{
    0xFFU, 0xFFU, 0x06U, 0x00U, 0x02U, 0x00U, 0x08U, 0x00U, 0x0CU, 0x80U,
    0x02U, 0x80U, 0x02U, 0x00U, 0x02U, 0x04U, 0x02U, 0xC0U, 0x00U, 0x00U,
    0x10U, 0x80U, 0x01U, 0x00U, 0x05U, 0x01U, 0x80U, 0x01U, 0xC0U, 0x03U,
    0x00U, 0x06U, 0x07U, 0x08U, 0x00U, 0x00U, 0x00U, 0x00U,
};

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-tsw-runtime-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{root_ / name, std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

[[nodiscard]] std::array<u16, 256> synthetic_palette_words() noexcept {
    std::array<u16, 256> palette{};
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        palette[index] = static_cast<u16>((index * 97U) & 0x7FFFU);
    }
    return palette;
}

[[nodiscard]] std::vector<u8> synthetic_archive() {
    std::vector<u8> compressed(kStream8.size() + kStream8.size() / 16U + 67U);
    const auto compression =
        openswd3::resource_io::compress_legacy_lzo1x_14(kStream8, compressed);
    compressed.resize(compression.bytes_written);

    constexpr std::size_t descriptor_offset = kBlockOffset + 12U + kPaletteSize;
    constexpr std::size_t payload_offset = descriptor_offset + 36U;
    std::vector<u8> bytes(payload_offset + compressed.size(), 0U);
    const u32 block_size = static_cast<u32>(bytes.size() - kBlockOffset);

    for (std::size_t record = 0U; record < 10U; ++record) {
        const std::size_t offset = kIndexOffset + record * kRecordSize;
        write_u32(bytes, offset + 0x14U, block_size);
        write_u32(bytes, offset + 0x18U, static_cast<u32>(kBlockOffset));
        write_u32(bytes, offset + 0x1CU, static_cast<u32>(record + 1U));
    }

    write_u32(bytes, kBlockOffset, 0x12345678U);
    write_u16(bytes, kBlockOffset + 4U, 0xABCDU);
    write_u16(bytes, kBlockOffset + 6U, 1U);
    write_u16(bytes, kBlockOffset + 8U, 8U);
    write_u16(bytes, kBlockOffset + 10U, 12U);
    const std::array<u16, 256> palette = synthetic_palette_words();
    for (std::size_t index = 0U; index < palette.size(); ++index) {
        write_u16(bytes, kBlockOffset + 12U + index * 2U, palette[index]);
    }

    write_u32(
        bytes,
        descriptor_offset + 0x00U,
        static_cast<u32>(payload_offset - kBlockOffset)
    );
    write_u32(
        bytes, descriptor_offset + 0x04U, static_cast<u32>(compressed.size())
    );
    write_u32(
        bytes, descriptor_offset + 0x08U, static_cast<u32>(kStream8.size())
    );
    write_u16(bytes, descriptor_offset + 0x20U, 47U);
    write_u16(bytes, descriptor_offset + 0x22U, 95U);
    std::ranges::copy(
        compressed, bytes.begin() + static_cast<std::ptrdiff_t>(payload_offset)
    );
    return bytes;
}

void write_six_archives(const TestTree& tree) {
    const std::vector<u8> bytes = synthetic_archive();
    for (const char* const name : kArchiveNames) {
        tree.write(name, bytes);
    }
}

void test_physical_variant_ignores_declared_count(
    openswd3::test::Context& test
) {
    const TestTree tree;
    constexpr std::size_t descriptor_base = kBlockOffset + 12U + kPaletteSize;
    constexpr std::size_t descriptor_size = 36U;
    constexpr std::size_t old_payload = descriptor_base + descriptor_size;
    for (const u16 declared : {u16{0U}, u16{1U}, u16{2U}}) {
        std::vector<u8> bytes = synthetic_archive();
        bytes.insert(
            bytes.begin() + static_cast<std::ptrdiff_t>(old_payload),
            descriptor_size,
            u8{0U}
        );
        const auto first = bytes.begin() +
            static_cast<std::ptrdiff_t>(descriptor_base);
        std::copy_n(first, descriptor_size, first + descriptor_size);
        for (std::size_t record = 0U; record < 10U; ++record) {
            write_u32(
                bytes,
                kIndexOffset + record * kRecordSize + 0x14U,
                static_cast<u32>(bytes.size() - kBlockOffset)
            );
        }

        write_u16(bytes, kBlockOffset + 6U, declared);
        for (const u32 variant : {0U, 1U}) {
            const std::size_t descriptor = descriptor_base +
                variant * descriptor_size;
            write_u32(
                bytes, descriptor,
                static_cast<u32>(old_payload + descriptor_size - kBlockOffset)
            );
            write_u16(
                bytes, descriptor + 0x20U, static_cast<u16>(47U + variant)
            );
            write_u16(
                bytes, descriptor + 0x22U, static_cast<u16>(95U + variant)
            );
        }

        for (const char* const name : kArchiveNames) {
            tree.write(name, bytes);
        }

        for (const u32 variant : {0U, 1U}) {
            LegacyTswRuntime runtime{tree.root()};
            const u32 variant_slot = 0xBEEF0000U | variant;
            const auto direct = runtime.load_direct(0xCAFE0001U, variant_slot);
            const auto loaded = runtime.query_cached(0xCAFE0001U, variant_slot);
            const auto hit = runtime.query_cached(1U, variant);
            test.expect_true(
                direct.status == LegacyTswRuntimeStatus::ready &&
                    direct.physical_status == LegacyTswFrameStatus::ready &&
                    direct.frame.width == 47U + variant &&
                    direct.frame.height == 95U + variant &&
                    !direct.frame.primary_stream.empty() &&
                    loaded.status == LegacyTswRuntimeStatus::ready &&
                    loaded.physical_status == LegacyTswFrameStatus::ready &&
                    !loaded.cache_hit && loaded.frame.width == 47U + variant &&
                    loaded.frame.height == 95U + variant &&
                    std::ranges::equal(
                        loaded.frame.primary_stream, direct.frame.primary_stream
                    ) &&
                    hit.status == LegacyTswRuntimeStatus::ready &&
                    hit.cache_hit &&
                    hit.frame_owner == loaded.frame_owner &&
                    runtime.cache_entry_count() == 1U &&
                    runtime.cached_primary_bytes() ==
                        direct.frame.primary_stream.size() &&
                    loaded.lookup_return_ecx == runtime.cached_primary_bytes() &&
                    loaded.lookup_return_edx == runtime.cached_primary_bytes(),
                "ordinary TSW direct and cached queries load the complete requested physical descriptor despite a smaller declared count; low16 keys and normal hit remain"
            );
        }
    }
}

class FakeSpecialLoader final : public LegacyTswSpecialFrameLoader {
public:
    [[nodiscard]] bool load_special_frame(
        const u16 variant_index, LegacyTswRuntimeFrame& frame
    ) override {
        ++calls;
        if (fail) {
            return false;
        }
        frame.primary_stream = {
            static_cast<u8>(variant_index),
            static_cast<u8>(variant_index >> 8U),
            0xAAU,
            0x55U,
        };
        frame.width = static_cast<u16>(variant_index + 1U);
        frame.height = 2U;
        return true;
    }

    std::size_t calls{};
    bool fail{};
};

void test_owned_frames(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    FakeSpecialLoader special;
    LegacyTswRuntime runtime{tree.root(), {}, &special};
    runtime.set_cache_limit(0U);
    auto first = runtime.load_owned(0xABCD0001U, 0x98760000U);
    auto second = runtime.load_owned(1U, 0U);
    test.expect_true(
        first.status == LegacyTswRuntimeStatus::ready &&
            second.status == LegacyTswRuntimeStatus::ready &&
            first.frame != nullptr && second.frame != nullptr,
        "owned loads truncate both slots and bypass cache eviction"
    );
    if (first.frame == nullptr || second.frame == nullptr ||
        first.status != LegacyTswRuntimeStatus::ready ||
        second.status != LegacyTswRuntimeStatus::ready) {
        return;
    }

    const u32 second_token = second.frame->record_token;
    const auto expected = second.frame->primary_stream;
    test.expect_true(
        first.return_record_token == first.frame->record_token &&
            second.return_record_token == second_token &&
            first.frame->record_token != second_token &&
            first.frame->primary_stream_token != 0U &&
            first.frame->primary_stream_token !=
                second.frame->primary_stream_token &&
            first.frame->width == 47U && first.frame->height == 95U &&
            expected.size() > 8U && expected[6U] == 16U &&
            first.frame->primary_stream == expected &&
            runtime.published_owned_record_token() == second_token &&
            runtime.cache_entry_count() == 0U &&
            runtime.cached_primary_bytes() == 0U,
        "each owned record has a separate converted writable image"
    );
    first.frame->primary_stream[0U] = 0U;
    runtime.set_cache_limit(0x00400000U);
    const auto cached = runtime.query_cached(1U, 0U);
    test.expect_true(
        cached.status == LegacyTswRuntimeStatus::ready &&
            std::ranges::equal(cached.frame.primary_stream, expected) &&
            second.frame->primary_stream == expected &&
            runtime.published_owned_record_token() == second_token,
        "owned mutations do not affect another owner or the cache"
    );
    runtime.clear_cache();
    runtime.close();
    test.expect_equal(
        second.frame->primary_stream,
        expected,
        "closing archives and cache does not release caller-owned bytes"
    );
    first.frame.reset();
    second.frame.reset();
    test.expect_equal(
        runtime.published_owned_record_token(),
        second_token,
        "freeing an owned record leaves the non-owning return slot stale"
    );

    // 6001 and 9000 prepare before allocation; 6000 and 9001 do not.
    for (const u32 resource : {6000U, 6001U, 9000U, 9001U, 0xFFFFU}) {
        const auto previous = runtime.published_owned_record_token();
        const auto loaded = runtime.load_owned(resource, 0U);
        const bool magic = resource == 6001U || resource == 9000U;
        if (magic) {
            test.expect_true(
                loaded.status ==
                        LegacyTswRuntimeStatus::
                            magic_preparation_io_unavailable &&
                    loaded.frame == nullptr &&
                    runtime.published_owned_record_token() == previous &&
                    runtime.magic_loading_flag() == 1U,
                "unbound magic preparation stops before record allocation"
            );
        } else {
            test.expect_true(
                loaded.frame != nullptr &&
                    runtime.published_owned_record_token() ==
                        loaded.frame->record_token &&
                    runtime.published_owned_record_token() != previous &&
                    runtime.magic_loading_flag() == 0U,
                "ordinary loading publishes the record before physical access"
            );
            if (resource == 9001U) {
                test.expect_equal(
                    loaded.status,
                    LegacyTswRuntimeStatus::ready,
                    "9001 follows the normal fourth archive route"
                );
            } else {
                test.expect_true(
                    loaded.status != LegacyTswRuntimeStatus::ready &&
                        loaded.return_record_token == 0U,
                    "unavailable payload is not a normal owned-frame return"
                );
            }
        }
    }

    test.expect_equal(
        special.calls,
        std::size_t{0U},
        "431760 does not route FFFF to the cached special loader"
    );

    const auto cached_normal = runtime.query_cached(1U, 0U);
    const auto stopped_magic = runtime.load_owned(6001U, 0U);
    const auto owned_slot = runtime.published_owned_record_token();
    const auto special_cached = runtime.query_cached(0xFFFFU, 0U);
    const auto normal_hit = runtime.query_cached(1U, 0U);
    test.expect_true(
        cached_normal.status == LegacyTswRuntimeStatus::ready &&
            stopped_magic.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            special_cached.status == LegacyTswRuntimeStatus::ready &&
            special.calls == 1U && normal_hit.cache_hit &&
            runtime.magic_loading_flag() == 1U &&
            runtime.published_owned_record_token() == owned_slot,
        "special loads and cache hits preserve the shared magic flag"
    );
    const auto ordinary_miss = runtime.query_cached(2U, 0U);
    test.expect_true(
        ordinary_miss.status == LegacyTswRuntimeStatus::ready &&
            runtime.magic_loading_flag() == 0U &&
            runtime.published_owned_record_token() == owned_slot,
        "an ordinary cache miss clears only the shared loading flag"
    );
}

void test_lazy_open_route_and_conversion(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyTswRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00400000U);
    test.expect_false(runtime.is_initialized(), "six TSW files open lazily");
    test.expect_equal(
        runtime.find_cached(1U, 0U).status,
        LegacyTswRuntimeStatus::cache_miss,
        "cache-only lookup does not initialize or load"
    );
    test.expect_false(
        runtime.is_initialized(), "cache-only miss preserves lazy state"
    );

    const LegacyTswQueryResult first = runtime.query_cached(1U, 0U);
    test.expect_equal(
        first.status, LegacyTswRuntimeStatus::ready, "first cached query loads"
    );
    test.expect_true(
        runtime.is_initialized(), "first query opens all archives"
    );
    test.expect_false(first.cache_hit, "first query is a miss");
    test.expect_true(
        first.frame_owner != nullptr &&
            first.frame_owner->record_token != 0U &&
            first.frame_owner->primary_stream_token != 0U &&
            first.frame_owner->record_token !=
                first.frame_owner->primary_stream_token &&
            first.lookup_return_ecx ==
                first.frame_owner->primary_stream.size() &&
            first.lookup_return_edx == runtime.cached_primary_bytes(),
        "loaded cache node has separate 32-bit record/source identities and miss registers"
    );
    test.expect_true(
        first.frame.auxiliary_stream.empty(),
        "physical TSW auxiliary pointer stays null"
    );
    test.expect_true(
        first.frame.palette.empty(), "sub_401C70 releases the external palette"
    );
    test.expect_equal(first.frame.width, u16{47U}, "descriptor width is kept");
    test.expect_equal(
        first.frame.height, u16{95U}, "descriptor height is kept"
    );

    const auto expected =
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream8,
            synthetic_palette_words(),
            openswd3::rendering::LegacyPixelConversionState{}
        );
    test.expect_equal(
        expected.status,
        openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        "fixture conversion succeeds"
    );
    test.expect_true(
        std::ranges::equal(first.frame.primary_stream, expected.bytes),
        "cached frame is the exact converted command stream"
    );
    test.expect_equal(
        runtime.cached_primary_bytes(),
        static_cast<u32>(expected.bytes.size()),
        "cache counts converted primary bytes only"
    );

    const u8* const first_pointer = first.frame.primary_stream.data();
    const LegacyTswQueryResult repeated = runtime.query_cached(1U, 0U);
    test.expect_equal(
        repeated.status, LegacyTswRuntimeStatus::ready, "repeat query succeeds"
    );
    test.expect_true(repeated.cache_hit, "repeat query hits cache");
    test.expect_true(
        repeated.frame_owner->record_token ==
                first.frame_owner->record_token &&
            repeated.frame_owner->primary_stream_token ==
                first.frame_owner->primary_stream_token &&
            repeated.lookup_return_ecx == 1U &&
            repeated.lookup_return_edx == 0x004CF86CU,
        "head hit preserves cache identity and packed key/bucket registers"
    );
    test.expect_true(
        repeated.frame.primary_stream.data() == first_pointer,
        "cache returns a borrowed stable view"
    );
    LegacyTswRuntime independent{tree.root()};
    independent.set_cache_limit(0x00400000U);
    const auto other_runtime = independent.query_cached(1U, 0U);
    test.expect_true(
        other_runtime.status == LegacyTswRuntimeStatus::ready &&
            other_runtime.frame_owner->record_token !=
                first.frame_owner->record_token &&
            other_runtime.frame_owner->primary_stream_token !=
                first.frame_owner->primary_stream_token,
        "two simultaneously live TSW archives never reuse guest node or stream tokens"
    );

    const LegacyTswQueryResult truncated =
        runtime.query_cached(0x00010001U, 0x00010000U);
    test.expect_true(
        truncated.cache_hit, "both four-byte ABI slots truncate to low 16 bits"
    );
    test.expect_true(
        runtime.find_cached(0x00010001U, 0x00010000U).cache_hit,
        "sub_431A20-style cache-only lookup returns the view"
    );

    const LegacyTswDirectResult second_archive = runtime.load_direct(3001U, 0U);
    test.expect_equal(
        second_archive.status,
        LegacyTswRuntimeStatus::ready,
        "resource quotient selects the second archive"
    );
    test.expect_equal(
        runtime.cache_entry_count(),
        std::size_t{1U},
        "direct load does not enter the cache"
    );

    const LegacyTswDirectResult record_zero = runtime.load_direct(3000U, 0U);
    test.expect_equal(
        record_zero.status,
        LegacyTswRuntimeStatus::physical_frame_failed,
        "physical remainder zero is safely isolated"
    );
    test.expect_equal(
        record_zero.physical_status,
        LegacyTswFrameStatus::physical_record_out_of_range,
        "remainder zero retains the physical failure reason"
    );
    test.expect_equal(
        runtime.load_direct(18001U, 0U).status,
        LegacyTswRuntimeStatus::resource_group_out_of_range,
        "group beyond the six-handle table is isolated"
    );

    runtime.close();
    test.expect_false(runtime.is_initialized(), "close resets lazy state");
    test.expect_equal(
        runtime.cache_entry_count(),
        std::size_t{0U},
        "close clears cached ownership"
    );
}

void test_magic_preparation_before_physical_load(
    openswd3::test::Context& test
) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyTswRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00400000U);
    const auto first = runtime.query_cached(6001U, 0U);
    test.expect_true(
        first.status != LegacyTswRuntimeStatus::ready &&
            first.frame_owner != nullptr &&
            first.frame_owner->record_token != 0U &&
            first.frame_owner->primary_stream_token == 0U &&
            runtime.cache_entry_count() == 1U &&
            runtime.cached_primary_bytes() == 0U,
        "magic lookup must not return a physical frame before its original five-slot preparation/I/O"
    );
    const auto hit = runtime.query_cached(6001U, 0U);
    test.expect_true(
        hit.cache_hit && hit.status == first.status &&
            hit.frame_owner == first.frame_owner,
        "a published but unresolved magic cache node remains a key hit"
    );
    const auto next_variant = runtime.query_cached(6001U, 1U);
    const auto first_slots = runtime.magic_preparation_slots();
    test.expect_true(
        next_variant.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            first_slots[0U].key == 1U && first_slots[0U].age == 0U &&
            first_slots[1U].key == 0U && runtime.cache_entry_count() == 2U,
        "same magic key reaches 431AAF hit across variant nodes; age reset awaits the unmodeled header read"
    );
    for (u32 resource = 6002U; resource <= 6006U; ++resource) {
        const auto stopped = runtime.query_cached(resource, 0U);
        test.expect_equal(
            stopped.status,
            LegacyTswRuntimeStatus::magic_preparation_io_unavailable,
            "further magic records stop only after the key/age prefix"
        );
    }
    const auto replaced = runtime.magic_preparation_slots();
    test.expect_true(
        replaced[0U].key == 6U && replaced[0U].age == 0U &&
            replaced[1U].key == 2U && replaced[1U].age == 4U &&
            replaced[2U].key == 3U && replaced[2U].age == 3U &&
            replaced[3U].key == 4U && replaced[3U].age == 2U &&
            replaced[4U].key == 5U && replaced[4U].age == 1U,
        "full five-slot miss ages visited keys and replaces the signed32 maximum with earliest tie precedence"
    );

    LegacyTswRuntime zero_key{tree.root()};
    zero_key.set_cache_limit(0x00400000U);
    const auto zero = zero_key.query_cached(9000U, 0U);
    test.expect_true(
        zero.status != LegacyTswRuntimeStatus::physical_frame_failed &&
            zero.status != LegacyTswRuntimeStatus::ready &&
            zero.frame_owner != nullptr &&
            zero_key.cache_entry_count() == 1U,
        "resource 9000 reaches zero-key preparation before the ordinary physical-record-zero check"
    );
    const auto zero_slots = zero_key.magic_preparation_slots();
    test.expect_true(
        zero.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            zero_slots[0U].key == 0U && zero_slots[0U].age == 0U &&
            zero_slots[1U].key == 0U && zero_slots[1U].age == 0U,
        "key zero hits the initial slot before the empty-key test; no host insertion"
    );
    test.expect_equal(
        zero_key.load_direct(9000U, 0U).status,
        LegacyTswRuntimeStatus::physical_frame_failed,
        "host direct physical-read API remains distinct from the cached 431C50 route"
    );

    const TestTree overflow_tree;
    std::vector<u8> overflow = synthetic_archive();
    write_u16(overflow, kBlockOffset + 6U, 256U);
    write_u16(overflow, kBlockOffset + 8U, 16U);
    for (const char* const name : kArchiveNames) {
        overflow_tree.write(name, overflow);
    }

    LegacyTswRuntime overflowing{overflow_tree.root()};
    const auto boundary = overflowing.query_cached(6001U, 0U);
    test.expect_true(
        boundary.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            overflowing.magic_preparation_slots()[0U].key == 1U &&
            !overflowing.magic_prepared_stream_position(0U, 0U).has_value(),
        "count 256 crosses the original 255-descriptor slot; do not treat a host buffer bound as a successful original reply"
    );
}

// The open archive permits truncation of the same inode only on POSIX;
// Windows opens it without FILE_SHARE_WRITE.
#ifndef _WIN32
void test_magic_prepared_stream_short_read(openswd3::test::Context& test) {
    const TestTree tree;
    std::vector<u8> bytes = synthetic_archive();
    constexpr std::size_t descriptor = kBlockOffset + 12U;
    constexpr std::size_t old_descriptor = descriptor + kPaletteSize;
    constexpr std::size_t payload = old_descriptor + 36U;
    write_u16(bytes, kBlockOffset + 8U, 16U);
    std::ranges::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(old_descriptor),
        36U,
        bytes.begin() + static_cast<std::ptrdiff_t>(descriptor)
    );
    for (const char* const name : kArchiveNames) {
        tree.write(name, bytes);
    }

    LegacyTswRuntime runtime{tree.root()};
    runtime.set_cache_limit(0x00400000U);
    const auto initialized = runtime.load_direct(0U, 0U);
    test.expect_true(
        initialized.status == LegacyTswRuntimeStatus::physical_frame_failed &&
            initialized.physical_status ==
                LegacyTswFrameStatus::physical_record_out_of_range,
        "opening all six archives succeeds before the invalid record-zero probe"
    );
    bytes.resize(descriptor + 36U);
    tree.write("all_magic.tsw", bytes);

    const auto stopped = runtime.query_cached(6001U, 0U);
    test.expect_true(
        stopped.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            stopped.frame_owner != nullptr &&
            stopped.frame_owner->primary_stream_token == 0U &&
            runtime.cache_entry_count() == 1U &&
            runtime.magic_preparation_slots()[0U].key == 1U &&
            runtime.magic_prepared_stream_position(0U, 0U) ==
                static_cast<u32>(payload),
        "a short compressed host read cannot fabricate success after the actual prepared seek reply and published node"
    );
    test.expect_equal(
        stopped.physical_status,
        LegacyTswFrameStatus::primary_stream_read_failed,
        "prepared magic short read retains the actual host failure reason, not a ready default"
    );
    const auto hit = runtime.query_cached(6001U, 0U);
    test.expect_true(
        hit.cache_hit && hit.status == stopped.status &&
            hit.physical_status == stopped.physical_status &&
            hit.physical_status ==
                LegacyTswFrameStatus::primary_stream_read_failed &&
            hit.frame_owner == stopped.frame_owner &&
            runtime.cached_primary_bytes() == 0U,
        "published magic key hit preserves its host short-read reason without retrying or returning image data"
    );
}

#endif

void test_prepared_magic_host_failure_status(openswd3::test::Context& test) {
    // 433678 compares the actual decoded length with descriptor+8. These
    // are host diagnostics, not the original API/allocator/report replies.
    const auto converted =
        openswd3::rendering::convert_legacy_image_command_stream(
            kStream8,
            synthetic_palette_words(),
            openswd3::rendering::LegacyPixelConversionState{}
        );
    test.expect_equal(
        converted.status,
        openswd3::rendering::LegacyImageCommandStreamStatus::completed,
        "prepared magic diagnostic fixture has a valid non-palette word stream"
    );
    constexpr std::size_t descriptor_offset = kBlockOffset + 12U;
    constexpr std::size_t payload_offset = descriptor_offset + 36U;
    for (const LegacyTswFrameStatus expected : {
             LegacyTswFrameStatus::ready,
             LegacyTswFrameStatus::decompressed_size_mismatch,
             LegacyTswFrameStatus::decompression_failed,
         }) {
        const TestTree tree;
        std::vector<u8> compressed(
            converted.bytes.size() + converted.bytes.size() / 16U + 67U
        );
        const auto compression =
            openswd3::resource_io::compress_legacy_lzo1x_14(
                converted.bytes, compressed
            );
        compressed.resize(compression.bytes_written);
        if (expected == LegacyTswFrameStatus::decompression_failed) {
            compressed = {0U};  // No complete LZO command or terminator.
        }

        std::vector<u8> bytes = synthetic_archive();
        bytes.resize(payload_offset + compressed.size());
        std::fill_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(descriptor_offset),
            36U,
            u8{0U}
        );
        write_u16(bytes, kBlockOffset + 8U, 16U);
        const u32 block_size = static_cast<u32>(bytes.size() - kBlockOffset);
        for (std::size_t record = 0U; record < 10U; ++record) {
            write_u32(
                bytes, kIndexOffset + record * kRecordSize + 0x14U, block_size
            );
        }

        write_u32(
            bytes,
            descriptor_offset,
            static_cast<u32>(payload_offset - kBlockOffset)
        );
        write_u32(
            bytes, descriptor_offset + 4U, static_cast<u32>(compressed.size())
        );
        write_u32(
            bytes,
            descriptor_offset + 8U,
            static_cast<u32>(converted.bytes.size()) +
                (expected == LegacyTswFrameStatus::decompressed_size_mismatch
                     ? 1U
                     : 0U)
        );
        std::ranges::copy(
            compressed,
            bytes.begin() + static_cast<std::ptrdiff_t>(payload_offset)
        );
        for (const char* const name : kArchiveNames) {
            tree.write(name, bytes);
        }

        openswd3::asset_runtime::LegacyTswArchive archive;
        test.expect_equal(
            archive.open(tree.root() / "all_magic.tsw"),
            openswd3::asset_runtime::LegacyTswOpenStatus::ready,
            "prepared magic diagnostic archive opens before the physical read"
        );
        openswd3::asset_runtime::LegacyTswIndexRecord index;
        index.block_offset = static_cast<u32>(kBlockOffset);
        index.block_size = block_size;
        std::array<u8, 36U> prepared{};
        std::ranges::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(descriptor_offset),
            prepared.size(),
            prepared.begin()
        );
        write_u32(prepared, 0U, static_cast<u32>(payload_offset));
        const auto physical = archive.read_prepared_magic_frame(
            index, 0x12345678U, 1U, 16U, prepared
        );
        test.expect_equal(
            physical.status,
            expected,
            "prepared host read distinguishes exact decoded length, length mismatch and invalid compressed data"
        );

        LegacyTswRuntime runtime{tree.root()};
        const auto queried = runtime.query_cached(6001U, 0U);
        const auto hit = runtime.query_cached(6001U, 0U);
        const LegacyTswRuntimeStatus runtime_expected =
            expected == LegacyTswFrameStatus::ready
            ? LegacyTswRuntimeStatus::ready
            : LegacyTswRuntimeStatus::magic_preparation_io_unavailable;
        test.expect_true(
            queried.status == runtime_expected &&
                queried.physical_status == expected && !queried.cache_hit &&
                queried.frame_owner != nullptr &&
                runtime.magic_prepared_stream_position(0U, 0U) ==
                    static_cast<u32>(payload_offset),
            "magic query retains the actual host physical reason while unresolved original failure remains unavailable"
        );
        test.expect_true(
            hit.cache_hit && hit.status == runtime_expected &&
                hit.physical_status == expected &&
                hit.frame_owner == queried.frame_owner &&
                runtime.cache_entry_count() == 1U,
            "cached magic key preserves the diagnostic reason and published identity on success or failure"
        );
        if (expected == LegacyTswFrameStatus::ready) {
            test.expect_true(
                std::ranges::equal(
                    queried.frame.primary_stream, converted.bytes
                ) && runtime.cached_primary_bytes() == converted.bytes.size(),
                "equal decoded length retains the normal image stream and byte accounting"
            );
        } else {
            test.expect_true(
                queried.frame.primary_stream.empty() &&
                    queried.frame_owner != nullptr &&
                    queried.frame_owner->primary_stream_token == 0U &&
                    runtime.cached_primary_bytes() == 0U,
                "host failure metadata cannot create an image pointer, decoded bytes or original return state"
            );
        }
    }
}

#ifdef OPENSWD3_REAL_TSW_ROOT
void test_real_owned_frames(openswd3::test::Context& test) {
    LegacyTswRuntime runtime{OPENSWD3_REAL_TSW_ROOT};
    for (const u32 resource : {15001U, 6001U}) {
        const auto physical = runtime.load_direct(resource, 0U);
        auto owned = runtime.load_owned(resource, 0U);
        test.expect_true(
            physical.status == LegacyTswRuntimeStatus::ready &&
                owned.status == LegacyTswRuntimeStatus::ready &&
                owned.frame != nullptr,
            "real background and magic frames have independent owners"
        );
        if (physical.status != LegacyTswRuntimeStatus::ready ||
            owned.status != LegacyTswRuntimeStatus::ready ||
            owned.frame == nullptr) {
            continue;
        }

        test.expect_true(
            owned.frame->primary_stream == physical.frame.primary_stream &&
                owned.frame->width == physical.frame.width &&
                owned.frame->height == physical.frame.height &&
                owned.frame->primary_stream_token != 0U &&
                owned.return_record_token == owned.frame->record_token &&
                runtime.cache_entry_count() == 0U &&
                runtime.magic_loading_flag() == 0U,
            "owned real bytes match physical decoding without cache nodes"
        );
    }

    test.expect_equal(
        runtime.magic_prepared_stream_position(0U, 0U),
        std::optional<u32>{0x00020434U},
        "owned magic loading uses the actual prepared absolute position"
    );
    const auto prior = runtime.published_owned_record_token();
    const auto stopped = runtime.load_owned(6001U, 0x1234FFFFU);
    test.expect_true(
        stopped.status ==
                LegacyTswRuntimeStatus::magic_preparation_io_unavailable &&
            stopped.frame != nullptr && stopped.frame->record_token != prior &&
            runtime.published_owned_record_token() ==
                stopped.frame->record_token &&
            runtime.magic_loading_flag() == 1U &&
            stopped.return_record_token == 0U &&
            runtime.cache_entry_count() == 0U,
        "descriptor unavailability follows preparation and owner publication"
    );
}

void test_real_magic_first_frame_requires_preparation(
    openswd3::test::Context& test
) {
    const std::filesystem::path root{OPENSWD3_REAL_TSW_ROOT};
    LegacyTswRuntime runtime{root};
    runtime.set_cache_limit(0x00400000U);
    const auto physical = runtime.load_direct(6001U, 0U);
    test.expect_equal(
        physical.status,
        LegacyTswRuntimeStatus::ready,
        "real magic first physical frame is a valid independent image input"
    );
    if (physical.status != LegacyTswRuntimeStatus::ready) {
        return;
    }

    const auto prepared = runtime.query_cached(6001U, 0U);
    test.expect_true(
        prepared.status == LegacyTswRuntimeStatus::ready &&
            prepared.frame_owner != nullptr &&
            prepared.frame_owner->primary_stream_token != 0U &&
            prepared.frame.width == physical.frame.width &&
            prepared.frame.height == physical.frame.height &&
            std::ranges::equal(
                prepared.frame.primary_stream, physical.frame.primary_stream
            ) &&
            runtime.magic_preparation_slots()[0U].key == 1U,
        "the original five-slot I/O path must yield the valid real magic image, not permanently stop at its first seek"
    );
    test.expect_equal(
        runtime.magic_prepared_stream_position(0U, 0U),
        std::optional<u32>{0x00020434U},
        "431C22 stores the actual file seek reply rather than the descriptor's relative 0x78"
    );
    const auto second_physical = runtime.load_direct(6001U, 1U);
    test.expect_equal(
        second_physical.status,
        LegacyTswRuntimeStatus::ready,
        "real second magic frame is a valid physical comparison input"
    );
    if (second_physical.status != LegacyTswRuntimeStatus::ready) {
        return;
    }

    const auto second = runtime.query_cached(6001U, 1U);
    test.expect_true(
        second.status == LegacyTswRuntimeStatus::ready &&
            second.frame_owner != nullptr &&
            second.frame.width == second_physical.frame.width &&
            second.frame.height == second_physical.frame.height &&
            std::ranges::equal(
                second.frame.primary_stream,
                second_physical.frame.primary_stream
            ) &&
            runtime.magic_preparation_slots()[0U].key == 1U &&
            runtime.magic_preparation_slots()[0U].age == 0U &&
            runtime.cache_entry_count() == 2U,
        "a second variant reuses the prepared five-slot descriptors after the header read"
    );
    test.expect_equal(
        runtime.magic_prepared_stream_position(0U, 1U),
        std::optional<u32>{0x00023306U},
        "cached variant one retains its actual absolute stream position"
    );

    for (const u32 resource : {6251U, 6331U}) {
        LegacyTswRuntime additional{root};
        additional.set_cache_limit(0x00400000U);
        const auto direct = additional.load_direct(resource, 0U);
        test.expect_equal(
            direct.status,
            LegacyTswRuntimeStatus::ready,
            "real 24-bit magic archive record must first load independently"
        );
        if (direct.status != LegacyTswRuntimeStatus::ready) {
            continue;
        }

        const auto cached = additional.query_cached(resource, 0U);
        test.expect_true(
            cached.status == LegacyTswRuntimeStatus::ready &&
                cached.frame_owner != nullptr &&
                cached.frame.width == direct.frame.width &&
                cached.frame.height == direct.frame.height &&
                std::ranges::equal(
                    cached.frame.primary_stream, direct.frame.primary_stream
                ),
            "actual non-palette 24-bit magic descriptors must not be rejected as an 8-bit allocation request"
        );
    }

    LegacyTswRuntime many{root};
    many.set_cache_limit(0x00400000U);
    const auto physical_last = many.load_direct(6137U, 130U);
    test.expect_equal(
        physical_last.status,
        LegacyTswRuntimeStatus::ready,
        "the real 131-frame magic record has a readable final descriptor"
    );
    if (physical_last.status == LegacyTswRuntimeStatus::ready) {
        const auto prepared_last = many.query_cached(6137U, 130U);
        test.expect_true(
            prepared_last.status == LegacyTswRuntimeStatus::ready &&
                prepared_last.frame_owner != nullptr &&
                prepared_last.frame.width == physical_last.frame.width &&
                prepared_last.frame.height == physical_last.frame.height &&
                std::ranges::equal(
                    prepared_last.frame.primary_stream,
                    physical_last.frame.primary_stream
                ) &&
                many.magic_prepared_stream_position(0U, 130U) == 0x04A0B1FFU,
            "descriptor 130 uses the actual seek reply after all 131 original-order header/descriptor reads"
        );
    }
}

void test_real_magic_slot_rotation_and_cached_node(
    openswd3::test::Context& test
) {
    LegacyTswRuntime runtime{std::filesystem::path{OPENSWD3_REAL_TSW_ROOT}};
    runtime.set_cache_limit(0x00400000U);
    const auto first = runtime.query_cached(6001U, 0U);
    test.expect_true(
        first.status == LegacyTswRuntimeStatus::ready &&
            first.frame_owner != nullptr,
        "real magic resource one has a resident frame before slot rotation"
    );
    if (first.status != LegacyTswRuntimeStatus::ready) {
        return;
    }

    for (u32 resource = 6002U; resource <= 6006U; ++resource) {
        const auto direct = runtime.load_direct(resource, 0U);
        const auto cached = runtime.query_cached(resource, 0U);
        test.expect_true(
            direct.status == LegacyTswRuntimeStatus::ready &&
                cached.status == LegacyTswRuntimeStatus::ready &&
                cached.frame_owner != nullptr &&
                std::ranges::equal(
                    cached.frame.primary_stream, direct.frame.primary_stream
                ),
            "five real magic descriptors and the sixth replacement retain the physical first-frame stream"
        );
    }

    const auto rotated = runtime.magic_preparation_slots();
    test.expect_true(
        rotated[0U].key == 6U && rotated[0U].age == 0U &&
            rotated[1U].key == 2U && rotated[1U].age == 4U &&
            rotated[2U].key == 3U && rotated[2U].age == 3U &&
            rotated[3U].key == 4U && rotated[3U].age == 2U &&
            rotated[4U].key == 5U && rotated[4U].age == 1U &&
            runtime.magic_prepared_stream_position(0U, 0U) == 0x0003658BU,
        "sixth valid magic record replaces the oldest descriptor slot without changing other ages"
    );

    const auto hit = runtime.query_cached(6001U, 0U);
    const auto after_hit = runtime.magic_preparation_slots();
    test.expect_true(
        hit.cache_hit && hit.frame_owner == first.frame_owner &&
            hit.status == LegacyTswRuntimeStatus::ready &&
            std::ranges::equal(
                hit.frame.primary_stream, first.frame.primary_stream
            ) &&
            after_hit[0U].key == rotated[0U].key &&
            after_hit[0U].age == rotated[0U].age &&
            after_hit[1U].age == rotated[1U].age,
        "published frame-node hit bypasses the evicted five-slot descriptor without re-aging it"
    );

    const auto direct_second = runtime.load_direct(6001U, 1U);
    const auto second = runtime.query_cached(6001U, 1U);
    const auto reloaded = runtime.magic_preparation_slots();
    test.expect_true(
        direct_second.status == LegacyTswRuntimeStatus::ready &&
            second.status == LegacyTswRuntimeStatus::ready &&
            second.frame_owner != nullptr &&
            std::ranges::equal(
                second.frame.primary_stream, direct_second.frame.primary_stream
            ) &&
            reloaded[0U].key == 6U && reloaded[0U].age == 1U &&
            reloaded[1U].key == 1U && reloaded[1U].age == 0U &&
            reloaded[2U].key == 3U && reloaded[2U].age == 4U &&
            reloaded[3U].key == 4U && reloaded[3U].age == 3U &&
            reloaded[4U].key == 5U && reloaded[4U].age == 2U &&
            runtime.magic_prepared_stream_position(1U, 1U) == 0x00023306U,
        "uncached second variant reloads its resource into the oldest remaining slot and reads that slot's stream"
    );
}

void test_real_tsw_frame_piece_provider(openswd3::test::Context& test) {
    LegacyTswRuntime runtime{std::filesystem::path{OPENSWD3_REAL_TSW_ROOT}};
    openswd3::asset_runtime::LegacyTswFramePieceProvider provider{runtime};
    openswd3::rendering::LegacyFramePiece piece;
    const bool loaded = provider.load_frame_piece(6001U, 0U, piece);
    const auto physical = runtime.load_direct(6001U, 0U);
    test.expect_true(
        loaded && physical.status == LegacyTswRuntimeStatus::ready &&
            piece.width == physical.frame.width &&
            piece.height == physical.frame.height &&
            piece.legacy_source_token != 0U &&
            std::ranges::equal(
                piece.source.bytes, physical.frame.primary_stream
            ) &&
            !provider.host_exception(),
        "real TSW frame provider publishes the physical image bytes and dimensions"
    );
    if (!loaded || piece.source.bytes.empty()) {
        return;
    }

    const u8 first_byte = piece.source.bytes.front();
    runtime.close();
    test.expect_true(
        piece.source.bytes.front() == first_byte,
        "real frame bytes remain available to the drawing call after cache closure"
    );
}

#endif

void test_special_resource_and_failures(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    LegacyTswRuntime unavailable{tree.root()};
    test.expect_equal(
        unavailable.query_cached(0xFFFFU, 13U).status,
        LegacyTswRuntimeStatus::special_loader_unavailable,
        "resource FFFF reaches the dedicated loader port"
    );
    unavailable.close();

    FakeSpecialLoader loader;
    LegacyTswRuntime runtime{tree.root(), {}, &loader};
    runtime.set_cache_limit(0x1000U);
    const LegacyTswQueryResult first = runtime.query_cached(0xFFFFU, 13U);
    test.expect_equal(
        first.status,
        LegacyTswRuntimeStatus::ready,
        "special frame loads through port"
    );
    test.expect_equal(loader.calls, std::size_t{1U}, "special loader called");
    test.expect_equal(
        runtime.bucket_entry_count(3U),
        std::size_t{1U},
        "FFFF cache bucket is variant modulo ten"
    );
    test.expect_equal(first.frame.width, u16{14U}, "special frame is borrowed");

    const LegacyTswQueryResult repeated =
        runtime.query_cached(0x1FFFFU, 0x1000DU);
    test.expect_true(
        repeated.cache_hit, "special key also truncates both slots"
    );
    test.expect_equal(
        loader.calls, std::size_t{1U}, "special cache hit skips loader"
    );
    const LegacyTswQueryResult newer = runtime.query_cached(0xFFFFU, 23U);
    const LegacyTswQueryResult moved = runtime.query_cached(0xFFFFU, 13U);
    test.expect_true(
        newer.status == LegacyTswRuntimeStatus::ready && moved.cache_hit &&
            moved.frame_owner == first.frame_owner &&
            moved.lookup_return_ecx ==
                newer.frame_owner->record_token - 8U &&
            moved.lookup_return_edx == 0x004CF8ACU,
        "non-head hit reports the old bucket head in ECX and moves the node"
    );

    loader.fail = true;
    test.expect_equal(
        runtime.load_direct(0xFFFFU, 14U).status,
        LegacyTswRuntimeStatus::special_frame_load_failed,
        "special loader failure remains explicit"
    );
    runtime.close();

    LegacyTswRuntime missing{tree.root() / "missing"};
    test.expect_equal(
        missing.query_cached(1U, 0U).status,
        LegacyTswRuntimeStatus::archive_open_failed,
        "missing six-file set fails initialization"
    );
    test.expect_false(
        missing.is_initialized(), "failed initialization closes partial state"
    );
}

class CachePublicationProbe final : public LegacyTswSpecialFrameLoader {
public:
    [[nodiscard]] bool load_special_frame(
        const u16 variant_index, LegacyTswRuntimeFrame& frame
    ) override {
        const auto before = runtime->find_cached(0xFFFFU, variant_index);
        observed = observed && runtime->cache_entry_count() == calls + 1U &&
            runtime->bucket_entry_count(3U) == calls + 1U &&
            runtime->cached_primary_bytes() == calls * 4U && before.cache_hit &&
            before.frame_owner != nullptr &&
            before.frame_owner->record_token != 0U &&
            before.frame_owner->primary_stream_token == 0U;
        if (before.frame_owner != nullptr) {
            record_tokens[calls] = before.frame_owner->record_token;
        }

        ++calls;
        frame.primary_stream = {0xFFU, 0xFFU, 0U, 0U};
        frame.width = 1U;
        frame.height = 2U;
        return true;
    }

    LegacyTswRuntime* runtime{};
    std::array<u32, 2U> record_tokens{};
    std::size_t calls{};
    bool observed{true};
};

void test_cache_publication_before_load(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    CachePublicationProbe loader;
    LegacyTswRuntime runtime{tree.root(), {}, &loader};
    loader.runtime = &runtime;
    const auto first = runtime.query_cached(0xFFFFU, 13U);
    const auto second = runtime.query_cached(0xFFFFU, 23U);
    test.expect_true(
        loader.observed && loader.calls == 2U &&
            first.status == LegacyTswRuntimeStatus::ready &&
            second.status == LegacyTswRuntimeStatus::ready &&
            first.frame_owner != nullptr && second.frame_owner != nullptr &&
            first.frame_owner->record_token == loader.record_tokens[0U] &&
            second.frame_owner->record_token == loader.record_tokens[1U] &&
            loader.record_tokens[0U] != loader.record_tokens[1U] &&
            runtime.cached_primary_bytes() == 8U,
        "TSW publishes each cache key and record identity before invoking the frame loader, and counts stream bytes only after load"
    );

    LegacyTswRuntime failed{tree.root()};
    const auto miss = failed.query_cached(1U, 0xFFFFU);
    const auto resident = failed.find_cached(1U, 0xFFFFU);
    const auto repeated = failed.query_cached(1U, 0xFFFFU);
    test.expect_true(
        miss.status == LegacyTswRuntimeStatus::physical_frame_failed &&
            resident.cache_hit && resident.frame_owner != nullptr &&
            resident.frame_owner->record_token != 0U && repeated.cache_hit &&
            repeated.frame_owner == resident.frame_owner &&
            repeated.status == miss.status &&
            failed.cache_entry_count() == 1U &&
            failed.bucket_entry_count(1U) == 1U &&
            failed.cached_primary_bytes() == 0U &&
            resident.lookup_return_ecx == 0xFFFF0001U &&
            resident.lookup_return_edx == 0x004CF86CU,
        "TSW load failure retains the published cache node and later packed-key hits do not retry loading or invent image data"
    );

    LegacyTswRuntime unresolved{tree.root()};
    const auto pending = unresolved.query_cached(0xFFFFU, 0U);
    const auto known = unresolved.query_cached(1U, 0U);
    const u32 known_bytes = unresolved.cached_primary_bytes();
    unresolved.set_cache_limit(0xFFFFFFFFU);
    const auto stopped = unresolved.query_cached(2U, 0U);
    const auto retained = unresolved.find_cached(0xFFFFU, 0U);
    test.expect_true(
        pending.status == LegacyTswRuntimeStatus::special_loader_unavailable &&
            known.status == LegacyTswRuntimeStatus::ready &&
            known_bytes != 0U && stopped.status == pending.status &&
            !stopped.cache_hit && stopped.frame_owner == nullptr &&
            unresolved.cache_entry_count() == 2U &&
            unresolved.cached_primary_bytes() == known_bytes &&
            retained.cache_hit && retained.frame_owner == pending.frame_owner,
        "TSW stops before evicting an unresolved payload instead of guessing zero length or free arguments"
    );
}

class NestedCursorLoader final : public LegacyTswSpecialFrameLoader {
public:
    enum class Operation { load, hit, miss, clear };

    [[nodiscard]] bool load_special_frame(
        const u16 variant_index, LegacyTswRuntimeFrame& frame
    ) override {
        ++calls;
        frame.primary_stream = {0xFFU, 0xFFU, 0U, 0U};
        frame.width = static_cast<u16>(variant_index + 1U);
        frame.height = 2U;
        if (variant_index != 0U) {
            frame.primary_stream.insert(
                frame.primary_stream.end(), {0xFFU, 0xFFU, 0U, 0U}
            );
            return true;
        }

        if (operation == Operation::clear) {
            runtime->clear_cache();
        } else if (operation == Operation::load) {
            nested = runtime->query_cached(0xFFFFU, 10U);
        } else {
            nested = runtime->find_cached(
                0xFFFFU, operation == Operation::hit ? 10U : 20U
            );
        }

        return true;
    }

    LegacyTswRuntime* runtime{};
    Operation operation{Operation::load};
    LegacyTswQueryResult nested;
    std::size_t calls{};
};

void test_shared_cursor_after_nested_lookup(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    for (const auto operation : {NestedCursorLoader::Operation::load,
                                 NestedCursorLoader::Operation::hit,
                                 NestedCursorLoader::Operation::miss,
                                 NestedCursorLoader::Operation::clear}) {
        NestedCursorLoader loader;
        loader.operation = operation;
        LegacyTswRuntime runtime{tree.root(), {}, &loader};
        loader.runtime = &runtime;
        runtime.set_cache_limit(0x7FFFFFFFU);
        if (operation != NestedCursorLoader::Operation::load) {
            const auto seed = runtime.query_cached(0xFFFFU, 10U);
            test.expect_true(
                seed.status == LegacyTswRuntimeStatus::ready &&
                    runtime.cached_primary_bytes() == 8U,
                "nested cursor fixture starts with an eight-byte inner frame"
            );
        }

        const auto outer = runtime.query_cached(0xFFFFU, 0U);
        const auto outer_record = runtime.find_cached(0xFFFFU, 0U);
        if (operation == NestedCursorLoader::Operation::clear) {
            test.expect_true(
                outer.status ==
                        LegacyTswRuntimeStatus::cache_cursor_unavailable &&
                    !outer.cache_hit && !outer_record.cache_hit &&
                    runtime.cache_entry_count() == 0U &&
                    runtime.cached_primary_bytes() == 8U &&
                    !runtime.cached_primary_bytes_known() && loader.calls == 2U,
                "a callback-removed destination is not guest-readable; its pending length leaves the prior eight-byte balance unavailable instead of zero"
            );
            continue;
        }

        const bool payloads_preserved = outer_record.cache_hit &&
            outer_record.status == LegacyTswRuntimeStatus::ready &&
            outer_record.frame.primary_stream.size() == 4U &&
            outer_record.frame.width == 1U && loader.calls == 2U &&
            runtime.cache_entry_count() == 2U;
        if (operation == NestedCursorLoader::Operation::miss) {
            test.expect_true(
                payloads_preserved && !loader.nested.cache_hit &&
                    outer.status != LegacyTswRuntimeStatus::ready &&
                    runtime.cached_primary_bytes() == 8U,
                "TSW does not substitute its original node after a nested miss clears the shared cursor; loaded data and prior byte total remain committed"
            );
            continue;
        }

        test.expect_true(
            payloads_preserved &&
                loader.nested.status == LegacyTswRuntimeStatus::ready &&
                outer.status == LegacyTswRuntimeStatus::ready &&
                !outer.cache_hit && outer.frame_owner != nullptr &&
                outer.frame_owner == loader.nested.frame_owner &&
                outer.frame_owner != outer_record.frame_owner &&
                outer.frame.primary_stream.size() == 8U &&
                outer.frame.width == 11U && outer.lookup_return_ecx == 8U &&
                outer.lookup_return_edx == 16U &&
                runtime.cached_primary_bytes() == 16U,
            "TSW reloads the shared cursor after nested load or hit, returns the inner record, and counts the inner length again instead of the original payload"
        );
    }
}

void test_cache_cleanup_balance(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    for (const bool close_files : {false, true}) {
        NestedCursorLoader loader;
        LegacyTswRuntime runtime{tree.root(), {}, &loader};
        loader.runtime = &runtime;
        runtime.set_cache_limit(0x7FFFFFFFU);
        const auto outer = runtime.query_cached(0xFFFFU, 0U);
        test.expect_true(
            outer.status == LegacyTswRuntimeStatus::ready &&
                runtime.cached_primary_bytes() == 16U &&
                runtime.cache_entry_count() == 2U,
            "cleanup balance fixture has eight and four byte nodes counted as sixteen"
        );
        if (close_files) {
            runtime.close();
        } else {
            runtime.clear_cache();
        }

        test.expect_true(
            runtime.cached_primary_bytes() == 4U &&
                runtime.cached_primary_bytes_known() &&
                runtime.cache_entry_count() == 0U &&
                runtime.is_initialized() == !close_files,
            "clearing or closing TSW subtracts each known payload once and preserves the four-byte residual instead of forcing zero"
        );
        const auto reopened = runtime.query_cached(0xFFFFU, 10U);
        test.expect_true(
            reopened.status == LegacyTswRuntimeStatus::ready &&
                !reopened.cache_hit && reopened.lookup_return_ecx == 8U &&
                reopened.lookup_return_edx == 12U &&
                runtime.cached_primary_bytes() == 12U &&
                runtime.cached_primary_bytes_known() &&
                runtime.cache_entry_count() == 1U,
            "TSW next load keeps the residual after cache clear or file reopen because the DF-zero REP ranges do not clear the byte total"
        );
    }

    NestedCursorLoader missed_loader;
    missed_loader.operation = NestedCursorLoader::Operation::miss;
    LegacyTswRuntime underflow{tree.root(), {}, &missed_loader};
    missed_loader.runtime = &underflow;
    underflow.set_cache_limit(0x7FFFFFFFU);
    const auto seed = underflow.query_cached(0xFFFFU, 10U);
    const auto missed = underflow.query_cached(0xFFFFU, 0U);
    test.expect_true(
        seed.status == LegacyTswRuntimeStatus::ready &&
            missed.status == LegacyTswRuntimeStatus::cache_cursor_unavailable &&
            underflow.cached_primary_bytes() == 8U,
        "cleanup underflow fixture preserves a four-byte uncounted payload after the nested cache-only miss"
    );
    underflow.clear_cache();
    test.expect_true(
        underflow.cached_primary_bytes() == 0xFFFFFFFCU &&
            underflow.cached_primary_bytes_known() &&
            underflow.cache_entry_count() == 0U,
        "known cleanup subtracts all twelve payload bytes from eight modulo 32 bits instead of clamping the negative residual to zero"
    );
    const auto after_wrap = underflow.query_cached(0xFFFFU, 30U);
    test.expect_true(
        after_wrap.status == LegacyTswRuntimeStatus::ready &&
            after_wrap.lookup_return_ecx == 8U &&
            after_wrap.lookup_return_edx == 4U &&
            underflow.cached_primary_bytes() == 4U &&
            underflow.cached_primary_bytes_known(),
        "signed capacity admits the negative residual and the next eight-byte load wraps the cache total back to four"
    );

    FakeSpecialLoader loader;
    LegacyTswRuntime unknown{tree.root(), {}, &loader};
    unknown.set_cache_limit(0x7FFFFFFFU);
    const auto first = unknown.query_cached(0xFFFFU, 0U);
    const auto later = unknown.query_cached(0xFFFFU, 3U);
    const auto pending = unknown.query_cached(1U, 0xFFFFU);
    test.expect_true(
        first.status == LegacyTswRuntimeStatus::ready &&
            later.status == LegacyTswRuntimeStatus::ready &&
            pending.status == LegacyTswRuntimeStatus::physical_frame_failed &&
            unknown.cached_primary_bytes() == 8U && loader.calls == 2U,
        "cleanup unknown-length fixture has a known first bucket, unknown second bucket and a later known payload"
    );
    unknown.clear_cache();
    test.expect_true(
        unknown.cached_primary_bytes() == 4U &&
            !unknown.cached_primary_bytes_known() &&
            unknown.cache_entry_count() == 0U,
        "host invalidation retains only the confirmed four-byte prefix before unknown cleanup length and does not assert total zero"
    );
    const auto blocked = unknown.query_cached(0xFFFFU, 20U);
    test.expect_true(
        blocked.status == LegacyTswRuntimeStatus::cache_balance_unavailable &&
            blocked.frame_owner == nullptr && !blocked.cache_hit &&
            unknown.cached_primary_bytes() == 4U &&
            unknown.cache_entry_count() == 0U && loader.calls == 2U,
        "cached lookup cannot use the retained cleanup prefix as a known capacity balance or call another loader"
    );
    unknown.close();
    const auto blocked_reopen = unknown.query_cached(0xFFFFU, 20U);
    test.expect_true(
        blocked_reopen.status ==
                LegacyTswRuntimeStatus::cache_balance_unavailable &&
            blocked_reopen.frame_owner == nullptr &&
            unknown.cached_primary_bytes() == 4U && loader.calls == 2U,
        "close and DF-zero initialization do not repair an unavailable cache balance with a fabricated zero"
    );
    const auto miss = unknown.find_cached(0xFFFFU, 20U);
    const auto direct = unknown.load_direct(0xFFFFU, 30U);
    test.expect_true(
        miss.status == LegacyTswRuntimeStatus::cache_miss && !miss.cache_hit &&
            direct.status == LegacyTswRuntimeStatus::ready &&
            unknown.cached_primary_bytes() == 4U &&
            !unknown.cached_primary_bytes_known() &&
            unknown.cache_entry_count() == 0U && loader.calls == 3U,
        "cache-only and direct requests do not need the unavailable balance and do not repair or increment it"
    );
}

void populate_eviction_shape(
    LegacyTswRuntime& runtime,
    FakeSpecialLoader& loader,
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 7> kVariants{0U, 10U, 20U, 1U, 11U, 2U, 12U};
    for (const u16 variant : kVariants) {
        const auto loaded = runtime.query_cached(0xFFFFU, variant);
        test.expect_equal(
            loaded.status,
            LegacyTswRuntimeStatus::ready,
            "eviction fixture frame loads"
        );
    }
    test.expect_equal(
        loader.calls, std::size_t{7U}, "fixture has seven distinct keys"
    );
    test.expect_equal(
        runtime.cached_primary_bytes(),
        28U,
        "fixture counts four bytes per special frame"
    );
    test.expect_equal(
        runtime.bucket_entry_count(0U),
        std::size_t{3U},
        "bucket zero starts longest"
    );
    test.expect_equal(
        runtime.bucket_entry_count(1U), std::size_t{2U}, "bucket one count"
    );
    test.expect_equal(
        runtime.bucket_entry_count(2U), std::size_t{2U}, "bucket two count"
    );
}

void test_cached_frame_lease_outlives_eviction(
    openswd3::test::Context& test
) {
    const TestTree tree;
    write_six_archives(tree);

    FakeSpecialLoader loader;
    LegacyTswRuntime runtime{tree.root(), {}, &loader};
    runtime.set_cache_limit(8U);
    const LegacyTswQueryResult original = runtime.query_cached(0xFFFFU, 0U);
    const u8* const pinned_bytes = original.frame.primary_stream.data();
    const LegacyTswQueryResult other = runtime.query_cached(0xFFFFU, 10U);
    test.expect_true(
        original.status == LegacyTswRuntimeStatus::ready &&
            other.status == LegacyTswRuntimeStatus::ready &&
            original.frame_owner != nullptr &&
            other.frame_owner != nullptr &&
            runtime.cached_primary_bytes() == 8U,
        "two special frames occupy the same cache bucket"
    );
    if (original.frame.primary_stream.size() < 4U ||
        other.frame.primary_stream.size() < 4U) {
        return;
    }

    const LegacyTswQueryResult replacement =
        runtime.query_cached(0xFFFFU, 0U);
    test.expect_true(
        replacement.status == LegacyTswRuntimeStatus::ready &&
            !replacement.cache_hit && loader.calls == 3U &&
            replacement.frame_owner != original.frame_owner &&
            replacement.frame_owner->record_token !=
                original.frame_owner->record_token &&
            replacement.frame_owner->primary_stream_token !=
                original.frame_owner->primary_stream_token &&
            replacement.frame.primary_stream.data() != pinned_bytes &&
            original.frame.primary_stream.data() == pinned_bytes &&
            original.frame.primary_stream[0U] == 0U &&
            original.frame.primary_stream[2U] == 0xAAU &&
            runtime.cached_primary_bytes() == 8U,
        "evicted cache frame remains readable while its lease is held"
    );

    runtime.close();
    test.expect_true(
        runtime.cache_entry_count() == 0U &&
            original.frame.primary_stream.data() == pinned_bytes &&
            original.frame.primary_stream[3U] == 0x55U,
        "closing the cache does not invalidate an outstanding lease"
    );
}

void test_tsw_frame_piece_provider(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    FakeSpecialLoader loader;
    LegacyTswRuntime runtime{tree.root(), {}, &loader};
    runtime.set_cache_limit(8U);
    openswd3::asset_runtime::LegacyTswFramePieceProvider provider{runtime};
    openswd3::rendering::LegacyFramePiece first;
    openswd3::rendering::LegacyFramePiece second;
    openswd3::rendering::LegacyFramePiece replacement;
    const bool first_loaded =
        provider.load_frame_piece(0x1FFFFU, 0x10000U, first);
    if (!first_loaded) {
        test.expect_true(false, "TSW frame provider loads a low-16-bit key");
        return;
    }
    const u8* const first_bytes = first.source.bytes.data();
    const bool second_loaded = provider.load_frame_piece(0xFFFFU, 10U, second);
    const bool replacement_loaded =
        provider.load_frame_piece(0xFFFFU, 0U, replacement);
    test.expect_true(
        first.width == 1U && first.height == 2U &&
            first.source.layout ==
                openswd3::rendering::LegacyBlitSourceLayout::direct_16 &&
            first.legacy_source_token != 0U &&
            std::ranges::equal(
                first.source.bytes, std::array<u8, 4U>{0U, 0U, 0xAAU, 0x55U}
            ),
        "TSW frame provider keeps the physical dimensions, source token, converted bytes and low-16-bit key"
    );
    test.expect_true(
        second_loaded && replacement_loaded && loader.calls == 3U &&
            replacement.source.bytes.data() != first_bytes &&
            first.source.bytes.data() == first_bytes &&
            first.source.bytes.size() == 4U &&
            first.source.bytes[2U] == 0xAAU && !provider.host_exception(),
        "later cache eviction does not invalidate a prior frame-piece lease"
    );
    runtime.close();
    test.expect_true(
        first.source.bytes.data() == first_bytes &&
            first.source.bytes[3U] == 0x55U,
        "provider lease keeps a frame piece readable after cache closure"
    );

    FakeSpecialLoader failing_loader;
    failing_loader.fail = true;
    LegacyTswRuntime unavailable{tree.root(), {}, &failing_loader};
    openswd3::asset_runtime::LegacyTswFramePieceProvider failed{unavailable};
    openswd3::rendering::LegacyFramePiece missing;
    test.expect_true(
        !failed.load_frame_piece(0xFFFFU, 0U, missing) &&
            missing.source.bytes.empty() && !failed.host_exception(),
        "unavailable TSW frame reports a failed load without a host exception"
    );

    class ThrowingFrameLoader final : public LegacyTswSpecialFrameLoader {
    public:
        bool load_special_frame(u16, LegacyTswRuntimeFrame&) override {
            throw std::runtime_error{"frame host failure"};
        }
    } throwing_loader;
    LegacyTswRuntime exceptional{tree.root(), {}, &throwing_loader};
    openswd3::asset_runtime::LegacyTswFramePieceProvider exceptional_provider{
        exceptional
    };
    openswd3::rendering::LegacyFramePiece interrupted;
    test.expect_true(
        !exceptional_provider.load_frame_piece(0xFFFFU, 0U, interrupted) &&
            interrupted.source.bytes.empty() &&
            exceptional_provider.host_exception(),
        "host loader exception stops the noexcept frame provider without returning a frame"
    );
}

void test_count_before_node_allocation(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    enum class CountOperation { retained_prefix, wrapped_nodes, clear_node };

    struct CountCase {
        u32 attempts;
        bool clear;
        bool close;
        CountOperation operation{CountOperation::retained_prefix};
    };

    constexpr std::array<CountCase, 8U> cases{{
        {1U, false, false},
        {1U, true, false},
        {1U, false, true},
        {0xFFFFU, false, false},
        {0x10000U, false, false},
        {0x10001U, false, false},
        {0xFFFFU, false, false, CountOperation::wrapped_nodes},
        {1U, false, false, CountOperation::clear_node},
    }};
    for (const auto candidate : cases) {
        FakeSpecialLoader loader;
        LegacyTswRuntime runtime{tree.root(), {}, &loader};
        runtime.set_cache_limit(0x7FFFFFFFU);
        const auto probe =
            openswd3::asset_runtime::reserve_legacy_guest_bytes(1U);
        test.expect_true(
            probe.has_value(), "count fixture reserves its identity probe"
        );
        if (!probe) {
            return;
        }

        const u32 begin = *probe + 16U;
        auto barrier = openswd3::asset_runtime::
            register_legacy_external_guest_bytes(
                begin, static_cast<std::size_t>(0x70000000U - begin)
            );
        test.expect_true(
            barrier != nullptr,
            "count fixture temporarily blocks remaining guest identities"
        );
        if (barrier == nullptr) {
            return;
        }

        bool prefix_stopped = true;
        for (u32 attempt = 0U; attempt < candidate.attempts; ++attempt) {
            const auto failed = runtime.query_cached(0xFFFFU, 5U);
            prefix_stopped = prefix_stopped &&
                failed.status == LegacyTswRuntimeStatus::allocation_failed &&
                failed.frame_owner == nullptr && !failed.cache_hit;
        }

        test.expect_true(
            prefix_stopped && runtime.cache_entry_count() == 0U &&
                runtime.cached_primary_bytes() == 0U && loader.calls == 0U,
            "controlled identity allocation stops before publishing any node or calling the image loader; not an original malloc reply"
        );
        barrier.reset();
        if (candidate.clear) {
            runtime.clear_cache();
        }

        if (candidate.close) {
            runtime.close();
        }

        if (candidate.operation == CountOperation::wrapped_nodes) {
            const auto oldest = runtime.query_cached(0xFFFFU, 5U);
            const auto newest = runtime.query_cached(0xFFFFU, 15U);
            test.expect_true(
                oldest.status == LegacyTswRuntimeStatus::ready &&
                    newest.status == LegacyTswRuntimeStatus::ready &&
                    runtime.cached_primary_bytes() == 8U &&
                    runtime.cache_entry_count() == 2U && loader.calls == 2U,
                "65535 retained prefixes plus two real publications wrap the word to one while the bucket has two nodes"
            );
            runtime.set_cache_limit(4U);
            const auto hit = runtime.query_cached(0xFFFFU, 15U);
            test.expect_true(
                hit.status == LegacyTswRuntimeStatus::ready && hit.cache_hit &&
                    hit.frame_owner == newest.frame_owner &&
                    runtime.cached_primary_bytes() == 4U &&
                    runtime.cache_entry_count() == 1U && loader.calls == 2U &&
                    runtime.find_cached(0xFFFFU, 5U).status ==
                        LegacyTswRuntimeStatus::cache_miss,
                "431F67 returns when the decremented word is zero even with one host node remaining and total equal to capacity"
            );
            continue;
        }

        if (candidate.operation == CountOperation::clear_node) {
            const auto published = runtime.query_cached(0xFFFFU, 5U);
            test.expect_true(
                published.status == LegacyTswRuntimeStatus::ready,
                "count cleanup fixture publishes a node after the retained prefix"
            );
            runtime.clear_cache();
        }

        const u32 seed_calls =
            candidate.operation == CountOperation::clear_node ? 2U : 1U;
        const auto seed = runtime.query_cached(0xFFFFU, 6U);
        test.expect_true(
            seed.status == LegacyTswRuntimeStatus::ready &&
                runtime.cached_primary_bytes() == 4U && loader.calls == seed_calls,
            "count fixture restores identity allocation and loads one node in bucket six"
        );
        runtime.set_cache_limit(4U);
        const auto queried = runtime.query_cached(0xFFFFU, 16U);
        if (static_cast<u16>(candidate.attempts) == 0U) {
            test.expect_true(
                queried.status == LegacyTswRuntimeStatus::ready &&
                    runtime.cached_primary_bytes() == 4U &&
                    runtime.cache_entry_count() == 1U && loader.calls == 2U &&
                    runtime.find_cached(0xFFFFU, 6U).status ==
                        LegacyTswRuntimeStatus::cache_miss,
                "sixteen-bit count wraps after 65536 prefixes so bucket six is selected and its normal eviction completes"
            );
            continue;
        }

        test.expect_true(
            queried.status ==
                    LegacyTswRuntimeStatus::cache_bucket_payload_unavailable &&
                queried.frame_owner == nullptr && !queried.cache_hit &&
                runtime.cached_primary_bytes() == 4U &&
                runtime.cache_entry_count() == 1U && loader.calls == seed_calls &&
                runtime.find_cached(0xFFFFU, 6U).frame_owner == seed.frame_owner,
            "word count committed before allocation keeps empty bucket five selected, including after head-zero clear/close; host list size must not select bucket six"
        );
    }
}

void test_initial_empty_bucket_sentinel(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);
    for (const u32 limit : {0U, 0x80000000U, 0xFFFFFFFFU}) {
        FakeSpecialLoader loader;
        LegacyTswRuntime runtime{tree.root(), {}, &loader};
        runtime.set_cache_limit(limit);
        const auto query = runtime.query_cached(0xFFFFU, 0U);
        test.expect_true(
            query.status ==
                    LegacyTswRuntimeStatus::cache_bucket_payload_unavailable &&
                query.frame_owner == nullptr && !query.cache_hit &&
                runtime.cache_entry_count() == 0U &&
                runtime.cached_primary_bytes() == 0U &&
                runtime.cached_primary_bytes_known() && loader.calls == 0U,
            "TSW initial empty bucket cannot be skipped as successful eviction when its sentinel length and free arguments are unavailable"
        );
    }

    for (const u32 limit : {0U, 4U, 5U}) {
        NestedCursorLoader loader;
        LegacyTswRuntime residual{tree.root(), {}, &loader};
        loader.runtime = &residual;
        residual.set_cache_limit(0x7FFFFFFFU);
        const auto counted = residual.query_cached(0xFFFFU, 0U);
        test.expect_true(
            counted.status == LegacyTswRuntimeStatus::ready &&
                residual.cached_primary_bytes() == 16U,
            "empty sentinel fixture creates the repeated sixteen-byte total"
        );
        residual.clear_cache();
        residual.set_cache_limit(limit);
        const auto query = residual.query_cached(0xFFFFU, 30U);
        if (limit == 5U) {
            test.expect_true(
                query.status == LegacyTswRuntimeStatus::ready &&
                    query.lookup_return_ecx == 8U &&
                    query.lookup_return_edx == 12U &&
                    residual.cached_primary_bytes() == 12U &&
                    residual.cache_entry_count() == 1U && loader.calls == 3U,
                "signed capacity below the threshold bypasses sentinel processing and loads into an empty cache normally"
            );
            continue;
        }

        test.expect_true(
            query.status ==
                    LegacyTswRuntimeStatus::cache_bucket_payload_unavailable &&
                query.frame_owner == nullptr && !query.cache_hit &&
                residual.cached_primary_bytes() == 4U &&
                residual.cached_primary_bytes_known() &&
                residual.cache_entry_count() == 0U && loader.calls == 2U,
            "TSW retained byte residual at or above the limit enters the empty bucket sentinel boundary instead of loading another frame"
        );
    }
}

void test_signed_cache_capacity(openswd3::test::Context& test) {
    // Independent LST: .data 4A6020=600000h, setter 4315C0..4315C9,
    // signed JL at 431723/431EDA and signed JGE at 431F76.
    // All query fixtures below have nonempty buckets before eviction;
    // they do not assert the invalid empty-bucket sentinel free ABI.
    const TestTree tree;
    write_six_archives(tree);
    FakeSpecialLoader default_loader;
    LegacyTswRuntime defaults{tree.root(), {}, &default_loader};
    const auto first = defaults.query_cached(0xFFFFU, 0U);
    const auto repeated = defaults.query_cached(0xFFFFU, 0U);
    test.expect_true(
        defaults.cache_limit() == 0x00600000U &&
            first.status == LegacyTswRuntimeStatus::ready &&
            repeated.status == LegacyTswRuntimeStatus::ready &&
            repeated.cache_hit && repeated.frame_owner == first.frame_owner &&
            default_loader.calls == 1U,
        "default TSW capacity is the original six MiB and retains a small frame"
    );

    struct Expected {
        u32 limit;
        std::size_t removed;
        u32 bytes_after_query;
    };

    constexpr std::array<Expected, 15U> cases{{
        {0U, 2U, 8U},
        {1U, 2U, 8U},
        {4U, 2U, 8U},
        {7U, 2U, 8U},
        {8U, 2U, 8U},
        {9U, 1U, 12U},
        {11U, 1U, 12U},
        {12U, 1U, 12U},
        {13U, 0U, 12U},
        {0x7FFFFFFEU, 0U, 12U},
        {0x7FFFFFFFU, 0U, 12U},
        {0x80000000U, 2U, 8U},
        {0x80000001U, 2U, 8U},
        {0xFFFFFFFEU, 2U, 8U},
        {0xFFFFFFFFU, 2U, 8U},
    }};
    bool fixtures_exact = true;
    bool thresholds_exact = true;
    for (const auto expected : cases) {
        FakeSpecialLoader loader;
        LegacyTswRuntime runtime{tree.root(), {}, &loader};
        runtime.set_cache_limit(0x7FFFFFFFU);
        const auto tail = runtime.query_cached(0xFFFFU, 0U);
        const auto other = runtime.query_cached(0xFFFFU, 1U);
        const auto head = runtime.query_cached(0xFFFFU, 10U);
        const bool fixture = tail.status == LegacyTswRuntimeStatus::ready &&
            other.status == LegacyTswRuntimeStatus::ready &&
            head.status == LegacyTswRuntimeStatus::ready &&
            tail.frame_owner != nullptr && head.frame_owner != nullptr &&
            tail.frame.primary_stream.size() == 4U &&
            runtime.cached_primary_bytes() == 12U &&
            runtime.bucket_entry_count(0U) == 2U &&
            runtime.bucket_entry_count(1U) == 1U && loader.calls == 3U;
        fixtures_exact = fixtures_exact && fixture;
        if (!fixture) {
            thresholds_exact = false;
            continue;
        }

        runtime.set_cache_limit(expected.limit);
        const auto query = runtime.query_cached(0xFFFFU, 0U);
        const bool hit = expected.removed == 0U;
        thresholds_exact = thresholds_exact &&
            runtime.cache_limit() == expected.limit &&
            query.status == LegacyTswRuntimeStatus::ready &&
            query.frame_owner != nullptr && query.cache_hit == hit &&
            (query.frame_owner == tail.frame_owner) == hit &&
            runtime.cached_primary_bytes() == expected.bytes_after_query &&
            runtime.cache_entry_count() ==
                3U - expected.removed + (hit ? 0U : 1U) &&
            runtime.bucket_entry_count(0U) ==
                2U - expected.removed + (hit ? 0U : 1U) &&
            runtime.bucket_entry_count(1U) == 1U &&
            loader.calls == (hit ? 3U : 4U) &&
            query.lookup_return_ecx ==
                (hit ? head.frame_owner->record_token - 8U : 4U) &&
            query.lookup_return_edx ==
                (hit ? 0x004CF84CU : expected.bytes_after_query) &&
            tail.frame.primary_stream[2U] == 0xAAU;
    }

    test.expect_true(
        fixtures_exact,
        "signed-capacity fixtures start with twelve bytes and a nonempty longest bucket"
    );
    test.expect_true(
        thresholds_exact,
        "TSW eviction follows fifteen independent signed JL/JGE limits before lookup and within the selected bucket"
    );
}

void test_original_bucket_eviction(openswd3::test::Context& test) {
    const TestTree tree;
    write_six_archives(tree);

    FakeSpecialLoader first_loader;
    LegacyTswRuntime before_hit{tree.root(), {}, &first_loader};
    before_hit.set_cache_limit(0x1000U);
    populate_eviction_shape(before_hit, first_loader, test);
    before_hit.set_cache_limit(28U);
    const auto reloaded_tail = before_hit.query_cached(0xFFFFU, 0U);
    test.expect_false(
        reloaded_tail.cache_hit,
        "eviction runs before lookup and removes the LRU tail"
    );
    test.expect_equal(
        first_loader.calls,
        std::size_t{8U},
        "evicted requested frame is loaded again"
    );
    test.expect_equal(
        before_hit.cached_primary_bytes(),
        28U,
        "reload returns total to the limit"
    );
    before_hit.close();

    FakeSpecialLoader second_loader;
    LegacyTswRuntime one_bucket{tree.root(), {}, &second_loader};
    one_bucket.set_cache_limit(0x1000U);
    populate_eviction_shape(one_bucket, second_loader, test);
    one_bucket.set_cache_limit(12U);
    const auto surviving_hit = one_bucket.query_cached(0xFFFFU, 1U);
    test.expect_true(
        surviving_hit.cache_hit,
        "query hits a non-selected bucket after eviction"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(0U),
        std::size_t{0U},
        "selected longest bucket is drained continuously"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(1U),
        std::size_t{2U},
        "eviction does not recompute a new longest bucket"
    );
    test.expect_equal(
        one_bucket.bucket_entry_count(2U),
        std::size_t{2U},
        "second non-selected bucket also survives"
    );
    test.expect_equal(
        one_bucket.cached_primary_bytes(),
        16U,
        "empty selected bucket can leave total above limit"
    );
}

}  // namespace

int main() {
    openswd3::test::Context test;
    test_owned_frames(test);
    test_lazy_open_route_and_conversion(test);
    test_physical_variant_ignores_declared_count(test);
    test_magic_preparation_before_physical_load(test);
    test_prepared_magic_host_failure_status(test);
#ifndef _WIN32
    test_magic_prepared_stream_short_read(test);
#endif
#ifdef OPENSWD3_REAL_TSW_ROOT
    test_real_owned_frames(test);
    test_real_magic_first_frame_requires_preparation(test);
    test_real_magic_slot_rotation_and_cached_node(test);
    test_real_tsw_frame_piece_provider(test);
#endif
    test_special_resource_and_failures(test);
    test_cache_publication_before_load(test);
    test_shared_cursor_after_nested_lookup(test);
    test_cache_cleanup_balance(test);
    test_original_bucket_eviction(test);
    test_cached_frame_lease_outlives_eviction(test);
    test_tsw_frame_piece_provider(test);
    test_count_before_node_allocation(test);
    test_initial_empty_bucket_sentinel(test);
    test_signed_cache_capacity(test);
    return test.exit_code();
}
