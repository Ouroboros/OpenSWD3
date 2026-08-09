#pragma once

#include "openswd3/compat/types.hpp"
#include "openswd3/rendering/legacy_pixel_conversion.hpp"

#include <optional>
#include <span>
#include <string_view>

namespace openswd3::rendering {

inline constexpr compat::u32 kLegacyBmpHeaderBytes = 54U;

enum class LegacyBmpWriteStatus {
    completed,
    invalid_request,
    open_failed,
    seek_failed,
    write_failed,
    position_failed,
};

struct LegacyBmpWriteResult {
    LegacyBmpWriteStatus status{LegacyBmpWriteStatus::invalid_request};
    compat::u32 row_stride{};
    compat::u32 logical_file_size{};
};

class LegacyBmpWriterPorts {
public:
    virtual ~LegacyBmpWriterPorts() = default;

    // Mirrors Win32 OPEN_ALWAYS: create a missing file, but do not truncate an
    // existing one before the writer overwrites it from offset zero.
    [[nodiscard]] virtual bool open_or_create_without_truncation(
        std::string_view filename
    ) = 0;
    [[nodiscard]] virtual bool seek_absolute(compat::u32 offset) = 0;
    [[nodiscard]] virtual bool write_bytes(
        std::span<const compat::u8> bytes
    ) = 0;
    [[nodiscard]] virtual std::optional<compat::u32> current_position() = 0;
    virtual void close() = 0;
    virtual void maintain_audio() = 0;
};

[[nodiscard]] LegacyBmpWriteResult write_legacy_16bit_framebuffer_bmp(
    std::span<const compat::u16> pixels,
    compat::i32 width,
    compat::i32 height,
    std::string_view filename,
    const LegacyPixelConversionState& pixel_conversion,
    LegacyBmpWriterPorts& ports
);

}  // namespace openswd3::rendering
