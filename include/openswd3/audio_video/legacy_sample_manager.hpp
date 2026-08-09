#pragma once

#include "openswd3/audio_video/legacy_snd_archive.hpp"
#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace openswd3::audio_video {

using LegacySampleHandle = compat::u32;

class LegacySampleBackend {
public:
    virtual ~LegacySampleBackend() = default;

    [[nodiscard]] virtual compat::u32 driver_token() const = 0;
    [[nodiscard]] virtual LegacySampleHandle allocate_sample_handle() = 0;
    virtual void initialize_sample(LegacySampleHandle handle) = 0;
    virtual void release_sample_handle(LegacySampleHandle handle) = 0;

    [[nodiscard]] virtual bool set_sample_file(
        LegacySampleHandle handle,
        std::span<const compat::u8> bytes
    ) = 0;
    [[nodiscard]] virtual bool set_named_sample_file(
        LegacySampleHandle handle,
        std::string_view extension,
        std::span<const compat::u8> bytes,
        compat::u32 auxiliary
    ) = 0;

    virtual void set_sample_user_data(
        LegacySampleHandle handle,
        compat::u32 slot,
        compat::u32 value
    ) = 0;
    [[nodiscard]] virtual compat::u32 sample_user_data(
        LegacySampleHandle handle,
        compat::u32 slot
    ) = 0;
    virtual void set_sample_volume(
        LegacySampleHandle handle,
        compat::i32 volume
    ) = 0;
    virtual void set_sample_pan(
        LegacySampleHandle handle,
        compat::i32 pan
    ) = 0;
    virtual void set_sample_loop_count(
        LegacySampleHandle handle,
        compat::i32 loop_count
    ) = 0;
    virtual void start_sample(LegacySampleHandle handle) = 0;
    virtual void end_sample(LegacySampleHandle handle) = 0;
    [[nodiscard]] virtual compat::u32 sample_status(
        LegacySampleHandle handle
    ) = 0;
};

enum class LegacySampleManagerInitializeStatus {
    ready,
    archive_not_open,
    pool_allocation_failed,
};

struct LegacySamplePlayRequest {
    std::optional<std::vector<compat::u8>> existing_buffer;
    compat::u32 sound_id{};
    compat::i32 volume{};
    compat::i32 pan{};
    compat::i32 loop_count{};
    compat::u32 named_file_auxiliary{};
};

class LegacySampleManager final {
public:
    LegacySampleManager(
        LegacySampleBackend& backend,
        LegacySndArchive& archive
    ) noexcept;
    ~LegacySampleManager();

    LegacySampleManager(const LegacySampleManager&) = delete;
    LegacySampleManager& operator=(const LegacySampleManager&) = delete;
    LegacySampleManager(LegacySampleManager&&) = delete;
    LegacySampleManager& operator=(LegacySampleManager&&) = delete;

    [[nodiscard]] LegacySampleManagerInitializeStatus initialize_pool(
        compat::i32 requested_handle_count
    );
    [[nodiscard]] bool shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] bool sample_enabled() const noexcept;
    void set_sample_enabled(bool enabled) noexcept;
    [[nodiscard]] compat::u32 driver_token() const;
    [[nodiscard]] compat::i32 configured_handle_count() const noexcept;

    [[nodiscard]] compat::i32 play(LegacySamplePlayRequest request);
    [[nodiscard]] compat::i32 stop(compat::u32 sound_id);
    [[nodiscard]] bool stop_all();
    [[nodiscard]] compat::i32 set_volume(
        compat::u32 sound_id,
        compat::i32 volume
    );
    [[nodiscard]] compat::i32 set_pan(
        compat::u32 sound_id,
        compat::i32 pan
    );
    [[nodiscard]] bool service_completed_samples();

    [[nodiscard]] std::size_t active_sample_count() const noexcept;
    [[nodiscard]] std::size_t free_sample_count() const noexcept;
    [[nodiscard]] std::size_t live_buffer_count() const noexcept;
    [[nodiscard]] bool buffer_is_live(compat::u32 token) const noexcept;

private:
    static constexpr compat::u32 kNoNode = 0xFFFFFFFFU;

    struct SampleNode {
        LegacySampleHandle handle{};
        compat::u32 next{kNoNode};
    };

    struct OwnedBuffer {
        std::vector<compat::u8> bytes;
        bool live{};
    };

    [[nodiscard]] compat::u32 pop_free_node() noexcept;
    void push_free_node(compat::u32 node) noexcept;
    void push_active_node(compat::u32 node) noexcept;
    [[nodiscard]] compat::u32 find_active_node(compat::u32 sound_id);
    [[nodiscard]] compat::u32 unlink_active_node(compat::u32 sound_id);
    [[nodiscard]] std::size_t list_size(compat::u32 head) const noexcept;

    [[nodiscard]] compat::u32 retain_buffer(std::vector<compat::u8> bytes);
    void release_buffer(compat::u32 token) noexcept;
    [[nodiscard]] std::span<const compat::u8> buffer_bytes(
        compat::u32 token
    ) const noexcept;
    void release_sample_reference(LegacySampleHandle handle);

    LegacySampleBackend& backend_;
    LegacySndArchive& archive_;
    std::vector<SampleNode> nodes_;
    std::vector<OwnedBuffer> buffers_;
    compat::u32 active_head_{kNoNode};
    compat::u32 free_head_{kNoNode};
    compat::i32 configured_handle_count_{};
    bool initialized_{};
    bool sample_enabled_{};
};

}  // namespace openswd3::audio_video
