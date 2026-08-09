#include "test.hpp"

#include "openswd3/audio_video/legacy_snd_archive.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::audio_video::kLegacySndRuntimeSizeMask;
using openswd3::audio_video::LegacySndArchive;
using openswd3::audio_video::LegacySndOpenStatus;
using openswd3::audio_video::LegacySndSampleStatus;
using openswd3::compat::u32;
using openswd3::compat::u8;

constexpr std::size_t kIndexOffset = 0x1CU;
constexpr std::size_t kDiskRecordSize = 0x2CU;
constexpr std::size_t kSlotCount = 3000U;
constexpr std::size_t kPayloadOffset =
    kIndexOffset + kSlotCount * kDiskRecordSize;
constexpr std::size_t kSyntheticPayloadSize = 48U;

void write_u32(const std::span<u8> bytes, const std::size_t offset,
               const u32 value) {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

[[nodiscard]] u32 read_u32(const std::span<const u8> bytes,
                           const std::size_t offset) {
    return static_cast<u32>(bytes[offset]) |
           (static_cast<u32>(bytes[offset + 1U]) << 8U) |
           (static_cast<u32>(bytes[offset + 2U]) << 16U) |
           (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

class TestTree {
public:
    TestTree() {
        const auto unique_value =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
                ("legacy-snd-archive-" + std::to_string(unique_value));
        std::filesystem::create_directories(root_);
    }

    ~TestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] std::filesystem::path path(const char* name) const {
        return root_ / name;
    }

    void write(const char* name, const std::span<const u8> bytes) const {
        std::ofstream output{path(name), std::ios::binary | std::ios::trunc};
        for (const u8 byte : bytes) {
            output.put(static_cast<char>(byte));
        }
    }

private:
    std::filesystem::path root_;
};

void write_record(const std::span<u8> archive, const std::size_t slot,
                  const u32 raw_size, const u32 payload_offset,
                  const u32 raw_type) {
    const std::size_t record = kIndexOffset + slot * kDiskRecordSize;
    write_u32(archive, record + 0x14U, raw_size);
    write_u32(archive, record + 0x18U, payload_offset);
    write_u32(archive, record + 0x1CU, 0xDEADBEEFU);
    write_u32(archive, record + 0x20U, raw_type);
}

[[nodiscard]] std::vector<u8> synthetic_archive() {
    std::vector<u8> archive(kPayloadOffset + 4U * kSyntheticPayloadSize, 0U);
    write_u32(archive, 0x18U, 1U);

    for (u32 type = 0U; type < 4U; ++type) {
        const u32 payload_offset =
            static_cast<u32>(kPayloadOffset + type * kSyntheticPayloadSize);
        write_record(archive, type,
                     0xC0000000U | static_cast<u32>(kSyntheticPayloadSize),
                     payload_offset, 0xFFFFFFFCU | type);
        for (std::size_t index = 0U; index < kSyntheticPayloadSize; ++index) {
            archive[payload_offset + index] =
                static_cast<u8>(0x20U * type + static_cast<u32>(index));
        }
    }

    write_record(archive, 4U, 48U, static_cast<u32>(archive.size() + 1U), 0U);
    write_record(archive, 5U, 4U, static_cast<u32>(kPayloadOffset), 1U);
    return archive;
}

[[nodiscard]] bool bytes_equal(const std::span<const u8> left,
                               const std::span<const u8> right) {
    return std::ranges::equal(left, right);
}

void test_open_and_runtime_index(openswd3::test::Context& test) {
    const TestTree tree;
    const std::vector<u8> bytes = synthetic_archive();
    tree.write("all.snd", bytes);

    LegacySndArchive archive;
    test.expect_equal(archive.open(tree.path("all.snd")),
                      LegacySndOpenStatus::ready, "fixed SND index opens");
    test.expect_true(archive.is_open(), "successful open records state");
    test.expect_equal(archive.entries().size(), kSlotCount,
                      "runtime index always contains 3000 slots");
    test.expect_true(archive.entry(0U) == nullptr, "sound ID zero is unsafe");
    test.expect_true(archive.entry(3001U) == nullptr,
                     "sound ID above the fixed table is unsafe");

    for (u32 type = 0U; type < 4U; ++type) {
        const auto* const entry = archive.entry(type + 1U);
        test.expect_true(entry != nullptr, "one-based entry exists");
        if (entry == nullptr) {
            continue;
        }
        test.expect_equal(
            entry->file_offset,
            static_cast<u32>(kPayloadOffset + type * kSyntheticPayloadSize),
            "disk +0x18 becomes runtime file offset");
        test.expect_equal(
            entry->packed_size_and_type,
            static_cast<u32>(kSyntheticPayloadSize) | (type << 26U),
            "disk size and type are masked into the runtime word");
        test.expect_equal(entry->reference_count, 0U, "reference starts zero");
        test.expect_equal(entry->buffer_token, 0U, "buffer token starts zero");
    }
    test.expect_equal(
        archive.entry(7U)->file_offset, 0U,
        "declared count does not shorten the fixed physical index");

    archive.close();
    test.expect_false(archive.is_open(), "close clears open state");
    test.expect_equal(archive.entries()[0].file_offset, 0U,
                      "close clears runtime entries");
}

void test_sample_reconstruction(openswd3::test::Context& test) {
    const TestTree tree;
    const std::vector<u8> bytes = synthetic_archive();
    tree.write("all.snd", bytes);

    LegacySndArchive archive;
    static_cast<void>(archive.open(tree.path("all.snd")));

    const auto type_zero = archive.load_sample(1U);
    test.expect_equal(type_zero.status, LegacySndSampleStatus::ready,
                      "type zero sample loads");
    test.expect_equal(type_zero.bytes.size(), kSyntheticPayloadSize + 24U,
                      "type zero adds the original 24-byte prefix");
    constexpr std::array<u8, 4> kRiff{'R', 'I', 'F', 'F'};
    test.expect_true(bytes_equal(std::span{type_zero.bytes}.first<4>(), kRiff),
                     "type zero starts with RIFF");
    test.expect_equal(read_u32(type_zero.bytes, 4U),
                      static_cast<u32>(kSyntheticPayloadSize),
                      "RIFF size keeps the source payload size");
    test.expect_equal(read_u32(type_zero.bytes, 0x10U), 16U, "fmt size");
    test.expect_true(
        bytes_equal(std::span{type_zero.bytes}.subspan(0x14U, 0x12U),
                    std::span{bytes}.subspan(kPayloadOffset, 0x12U)),
        "forward overlapping copy preserves source bytes zero through 17");
    test.expect_equal(type_zero.bytes[0x26U], u8{'t'}, "chunk tag t");
    test.expect_equal(type_zero.bytes[0x27U], u8{'a'}, "chunk tag a");
    test.expect_equal(read_u32(type_zero.bytes, 0x28U),
                      static_cast<u32>(kSyntheticPayloadSize - 24U),
                      "data size keeps the original subtraction bug");
    test.expect_true(
        bytes_equal(std::span{type_zero.bytes}.subspan(0x2CU),
                    std::span{bytes}.subspan(kPayloadOffset + 20U,
                                             kSyntheticPayloadSize - 20U)),
        "type zero tail starts at source byte 20");

    const auto type_one = archive.load_sample(2U);
    test.expect_equal(type_one.status, LegacySndSampleStatus::ready,
                      "type one sample loads");
    test.expect_true(
        bytes_equal(type_one.bytes, std::span{bytes}.subspan(
                                        kPayloadOffset + kSyntheticPayloadSize,
                                        kSyntheticPayloadSize)),
        "type one returns the raw payload without rewrite");

    for (u32 type = 2U; type <= 3U; ++type) {
        const auto sample = archive.load_sample(type + 1U);
        const std::size_t source_offset =
            kPayloadOffset + type * kSyntheticPayloadSize;
        test.expect_equal(sample.status, LegacySndSampleStatus::ready,
                          "type two/three sample loads");
        test.expect_equal(sample.bytes.size(), kSyntheticPayloadSize,
                          "type two/three does not add a prefix");
        test.expect_true(
            bytes_equal(std::span{sample.bytes}.first<4>(),
                        std::span{bytes}.subspan(source_offset, 4U)),
            "type two/three keeps the raw first four bytes");
        test.expect_equal(read_u32(sample.bytes, 4U),
                          static_cast<u32>(kSyntheticPayloadSize),
                          "type two/three rewrites size");
        test.expect_true(
            bytes_equal(std::span{sample.bytes}.subspan(0x14U, 0x12U),
                        std::span{bytes}.subspan(source_offset + 0x18U, 0x12U)),
            "type two/three uses the same forward overlap result");
        test.expect_equal(sample.bytes[0x26U], u8{'t'}, "type rewrite t");
        test.expect_equal(sample.bytes[0x27U], u8{'a'}, "type rewrite a");
    }
}

void test_safety_boundaries(openswd3::test::Context& test) {
    const TestTree tree;
    LegacySndArchive archive;
    test.expect_equal(archive.load_sample(1U).status,
                      LegacySndSampleStatus::archive_not_open,
                      "sample lookup before open is rejected");
    test.expect_equal(archive.open(tree.path("missing.snd")),
                      LegacySndOpenStatus::file_open_failed,
                      "missing SND does not create an empty archive");

    constexpr std::array<u8, 32> kShortArchive{};
    tree.write("short.snd", kShortArchive);
    test.expect_equal(archive.open(tree.path("short.snd")),
                      LegacySndOpenStatus::index_read_failed,
                      "truncated fixed index is rejected");

    const std::vector<u8> bytes = synthetic_archive();
    tree.write("all.snd", bytes);
    static_cast<void>(archive.open(tree.path("all.snd")));
    test.expect_equal(archive.load_sample(0U).status,
                      LegacySndSampleStatus::invalid_sound_id,
                      "zero ID is isolated before original table underflow");
    test.expect_equal(archive.load_sample(3001U).status,
                      LegacySndSampleStatus::invalid_sound_id,
                      "large ID is isolated before original table overflow");
    test.expect_equal(archive.load_sample(7U).status,
                      LegacySndSampleStatus::empty_entry,
                      "zero file offset remains an empty lookup");
    test.expect_equal(archive.load_sample(5U).status,
                      LegacySndSampleStatus::sample_out_of_file_range,
                      "corrupt file window is isolated");
    test.expect_equal(archive.load_sample(6U).status,
                      LegacySndSampleStatus::unsafe_original_allocation,
                      "original template overflow is isolated");
}

class Sha256 {
public:
    void update(const std::span<const u8> bytes) {
        total_bytes_ += bytes.size();
        for (const u8 byte : bytes) {
            block_[block_size_++] = byte;
            if (block_size_ == block_.size()) {
                process_block();
                block_size_ = 0U;
            }
        }
    }

    [[nodiscard]] std::string finish() {
        const std::uint64_t bit_count = total_bytes_ * 8U;
        block_[block_size_++] = 0x80U;
        if (block_size_ > 56U) {
            std::fill(block_.begin() + static_cast<std::ptrdiff_t>(block_size_),
                      block_.end(), 0U);
            process_block();
            block_size_ = 0U;
        }
        std::fill(block_.begin() + static_cast<std::ptrdiff_t>(block_size_),
                  block_.begin() + 56, 0U);
        for (std::size_t index = 0U; index < 8U; ++index) {
            block_[63U - index] = static_cast<u8>(bit_count >> (index * 8U));
        }
        process_block();

        std::ostringstream output;
        output << std::hex << std::setfill('0');
        for (const u32 word : state_) {
            output << std::setw(8) << word;
        }
        return output.str();
    }

private:
    [[nodiscard]] static constexpr u32 rotate_right(const u32 value,
                                                    const u32 count) noexcept {
        return std::rotr(value, static_cast<int>(count));
    }

    void process_block() noexcept {
        std::array<u32, 64> words{};
        for (std::size_t index = 0U; index < 16U; ++index) {
            const std::size_t offset = index * 4U;
            words[index] = (static_cast<u32>(block_[offset]) << 24U) |
                           (static_cast<u32>(block_[offset + 1U]) << 16U) |
                           (static_cast<u32>(block_[offset + 2U]) << 8U) |
                           static_cast<u32>(block_[offset + 3U]);
        }
        for (std::size_t index = 16U; index < words.size(); ++index) {
            const u32 s0 = rotate_right(words[index - 15U], 7U) ^
                           rotate_right(words[index - 15U], 18U) ^
                           (words[index - 15U] >> 3U);
            const u32 s1 = rotate_right(words[index - 2U], 17U) ^
                           rotate_right(words[index - 2U], 19U) ^
                           (words[index - 2U] >> 10U);
            words[index] = words[index - 16U] + s0 + words[index - 7U] + s1;
        }

        u32 a = state_[0];
        u32 b = state_[1];
        u32 c = state_[2];
        u32 d = state_[3];
        u32 e = state_[4];
        u32 f = state_[5];
        u32 g = state_[6];
        u32 h = state_[7];
        for (std::size_t index = 0U; index < words.size(); ++index) {
            const u32 sum_one = rotate_right(e, 6U) ^ rotate_right(e, 11U) ^
                                rotate_right(e, 25U);
            const u32 choice = (e & f) ^ ((~e) & g);
            const u32 temporary_one =
                h + sum_one + choice + kRoundConstants[index] + words[index];
            const u32 sum_zero = rotate_right(a, 2U) ^ rotate_right(a, 13U) ^
                                 rotate_right(a, 22U);
            const u32 majority = (a & b) ^ (a & c) ^ (b & c);
            const u32 temporary_two = sum_zero + majority;
            h = g;
            g = f;
            f = e;
            e = d + temporary_one;
            d = c;
            c = b;
            b = a;
            a = temporary_one + temporary_two;
        }
        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    static constexpr std::array<u32, 64> kRoundConstants{
        0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U, 0x3956C25BU,
        0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U, 0xD807AA98U, 0x12835B01U,
        0x243185BEU, 0x550C7DC3U, 0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U,
        0xC19BF174U, 0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
        0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU, 0x983E5152U,
        0xA831C66DU, 0xB00327C8U, 0xBF597FC7U, 0xC6E00BF3U, 0xD5A79147U,
        0x06CA6351U, 0x14292967U, 0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU,
        0x53380D13U, 0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
        0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U, 0xD192E819U,
        0xD6990624U, 0xF40E3585U, 0x106AA070U, 0x19A4C116U, 0x1E376C08U,
        0x2748774CU, 0x34B0BCB5U, 0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU,
        0x682E6FF3U, 0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
        0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U,
    };

    std::array<u32, 8> state_{
        0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
        0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U,
    };
    std::array<u8, 64> block_{};
    std::size_t block_size_{};
    std::uint64_t total_bytes_{};
};

void test_real_archive(openswd3::test::Context& test,
                       const std::filesystem::path& path) {
    LegacySndArchive archive;
    test.expect_equal(archive.open(path), LegacySndOpenStatus::ready,
                      "real all.snd opens");
    if (!archive.is_open()) {
        return;
    }

    std::size_t nonempty_count{};
    std::uint64_t total_view_bytes{};
    std::set<u32> unique_offsets;
    for (const auto& entry : archive.entries()) {
        if (entry.file_offset == 0U) {
            continue;
        }
        ++nonempty_count;
        unique_offsets.insert(entry.file_offset);
        total_view_bytes +=
            entry.packed_size_and_type & kLegacySndRuntimeSizeMask;
        test.expect_equal(entry.packed_size_and_type >> 26U, 0U,
                          "current all.snd runtime type is zero");
    }
    test.expect_equal(nonempty_count, std::size_t{664U}, "real nonempty count");
    test.expect_equal(unique_offsets.size(), std::size_t{662U},
                      "real unique payload offsets");
    test.expect_equal(total_view_bytes, std::uint64_t{117128703U},
                      "real runtime view byte total");

    test.expect_equal(archive.entry(270U)->packed_size_and_type &
                          kLegacySndRuntimeSizeMask,
                      0x01DE1747U, "ID 270 keeps its spanning alias length");
    test.expect_equal(archive.entry(270U)->file_offset, 0x0110B1E8U,
                      "ID 270 keeps its alias offset");
    test.expect_equal(archive.entry(277U)->packed_size_and_type &
                          kLegacySndRuntimeSizeMask,
                      0x01DE1747U, "ID 277 keeps its spanning alias length");
    test.expect_equal(archive.entry(277U)->file_offset, 0x011D943EU,
                      "ID 277 keeps its alias offset");

    Sha256 digest;
    std::uint64_t total_allocation_bytes{};
    for (u32 sound_id = 1U; sound_id <= 3000U; ++sound_id) {
        const auto* const entry = archive.entry(sound_id);
        if (entry->file_offset == 0U) {
            continue;
        }
        const auto sample = archive.load_sample(sound_id);
        test.expect_equal(sample.status, LegacySndSampleStatus::ready,
                          "real sample loads");
        if (sample.status != LegacySndSampleStatus::ready) {
            continue;
        }
        total_allocation_bytes += sample.bytes.size();
        digest.update(sample.bytes);
        if (sound_id == 506U || sound_id == 507U) {
            constexpr std::array<u8, 4> kMalformedTag{0U, 0U, 't', 'a'};
            test.expect_true(
                bytes_equal(std::span{sample.bytes}.subspan(0x24U, 4U),
                            kMalformedTag),
                "malformed original chunk tag remains unchanged");
        }
    }
    test.expect_equal(total_allocation_bytes, std::uint64_t{117144639U},
                      "real loader allocation byte total");
    test.expect_equal(
        digest.finish(),
        std::string{
            "38ecbbcbb7473ab8c8ec116837b8c472fa7ed274ad42e4a0e0f04aaef6241e27"},
        "all returned sample buffers match the evidence SHA-256");
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_open_and_runtime_index(test);
    test_sample_reconstruction(test);
    test_safety_boundaries(test);
    if (argument_count == 2) {
        test_real_archive(test, std::filesystem::path{arguments[1]});
    }
    return test.exit_code();
}
