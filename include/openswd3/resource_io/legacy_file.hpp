#pragma once

#include "openswd3/compat/types.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <string_view>

namespace openswd3::resource_io {

[[nodiscard]] bool legacy_exclusive_file_probe(
    const std::filesystem::path& path,
    std::span<char> error_buffer = {}
) noexcept;

enum class LegacyFileCreation {
    open_existing,
    open_always,
};

enum class LegacyFileAccess {
    read,
    write,
    read_write,
};

struct LegacyFileTime {
    compat::u32 low{};
    compat::u32 high{};
};

class LegacyFile final {
public:
    LegacyFile();
    ~LegacyFile();

    LegacyFile(const LegacyFile&) = delete;
    LegacyFile& operator=(const LegacyFile&) = delete;
    LegacyFile(LegacyFile&&) = delete;
    LegacyFile& operator=(LegacyFile&&) = delete;

    [[nodiscard]] bool open(
        const std::filesystem::path& path,
        LegacyFileCreation creation,
        LegacyFileAccess access
    );
    [[nodiscard]] bool close() noexcept;

    [[nodiscard]] bool create_read_only_mapping() noexcept;
    [[nodiscard]] bool close_mapping() noexcept;
    [[nodiscard]] const compat::u8* map_view(
        compat::u32 offset = 0U,
        compat::u32 size = 0U
    ) noexcept;
    [[nodiscard]] bool close_view(const compat::u8* view) noexcept;

    [[nodiscard]] compat::u32 size() const noexcept;
    [[nodiscard]] bool truncate_at_current_position() noexcept;
    [[nodiscard]] bool read(
        std::span<compat::u8> buffer,
        compat::u32& in_out_size
    ) noexcept;
    [[nodiscard]] bool write(
        std::span<const compat::u8> buffer,
        compat::u32& in_out_size
    ) noexcept;

    [[nodiscard]] compat::u32 seek_current_one_based(
        compat::i32 distance
    ) noexcept;
    [[nodiscard]] compat::u32 seek_begin_one_based(
        compat::i32 distance
    ) noexcept;
    [[nodiscard]] compat::u32 seek_end_one_based(
        compat::i32 distance
    ) noexcept;
    [[nodiscard]] bool current_position(compat::u32& position) noexcept;

    [[nodiscard]] bool write_exact(
        std::span<const compat::u8> buffer
    ) noexcept;
    [[nodiscard]] bool write_u8(compat::u8 value) noexcept;
    [[nodiscard]] bool write_u32(compat::u32 value) noexcept;
    [[nodiscard]] bool read_u8(compat::u8& value) noexcept;
    [[nodiscard]] bool read_u32(compat::u32& value) noexcept;
    [[nodiscard]] bool last_write_time(LegacyFileTime& time) const noexcept;

    [[nodiscard]] std::string_view error_message() const noexcept;

private:
    enum class SeekOrigin {
        begin,
        current,
        end,
    };

    struct State;

    [[nodiscard]] compat::u32 seek_raw(
        SeekOrigin origin,
        compat::i32 distance
    ) noexcept;
    void set_error(std::string_view message) noexcept;
    void set_system_error() noexcept;

    std::unique_ptr<State> state_;
};

}  // namespace openswd3::resource_io
