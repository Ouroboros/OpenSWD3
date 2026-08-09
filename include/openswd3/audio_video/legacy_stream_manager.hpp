#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace openswd3::audio_video {

using LegacyStreamHandle = compat::u32;

class LegacyStreamBackend {
public:
    virtual ~LegacyStreamBackend() = default;

    [[nodiscard]] virtual LegacyStreamHandle open_stream(
        compat::u32 driver_token,
        std::string_view filename,
        compat::i32 file_offset
    ) = 0;
    [[nodiscard]] virtual std::string_view last_error() const = 0;
    virtual void close_stream(LegacyStreamHandle handle) = 0;

    virtual void set_stream_user_data(
        LegacyStreamHandle handle,
        compat::u32 slot,
        compat::i32 value
    ) = 0;
    [[nodiscard]] virtual compat::i32 stream_user_data(
        LegacyStreamHandle handle,
        compat::u32 slot
    ) = 0;
    virtual void set_stream_volume(
        LegacyStreamHandle handle,
        compat::i32 volume
    ) = 0;
    [[nodiscard]] virtual compat::i32 stream_volume(
        LegacyStreamHandle handle
    ) = 0;
    virtual void set_stream_loop_count(
        LegacyStreamHandle handle,
        compat::i32 loop_count
    ) = 0;
    virtual void start_stream(LegacyStreamHandle handle) = 0;
    [[nodiscard]] virtual compat::u32 stream_status(
        LegacyStreamHandle handle
    ) = 0;
    virtual void stream_ms_position(
        LegacyStreamHandle handle,
        compat::i32& total_milliseconds,
        compat::i32& current_milliseconds
    ) = 0;
};

enum class LegacyStreamManagerInitializeStatus {
    ready,
    pool_allocation_failed,
};

class LegacyStreamManager final {
public:
    explicit LegacyStreamManager(LegacyStreamBackend& backend) noexcept;
    ~LegacyStreamManager();

    LegacyStreamManager(const LegacyStreamManager&) = delete;
    LegacyStreamManager& operator=(const LegacyStreamManager&) = delete;
    LegacyStreamManager(LegacyStreamManager&&) = delete;
    LegacyStreamManager& operator=(LegacyStreamManager&&) = delete;

    [[nodiscard]] LegacyStreamManagerInitializeStatus initialize_pool(
        compat::u32 driver_token
    );
    [[nodiscard]] bool shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] bool stream_enabled() const noexcept;
    void set_stream_enabled(bool enabled) noexcept;
    [[nodiscard]] compat::u32 driver_token() const noexcept;
    [[nodiscard]] std::string_view last_error() const noexcept;

    [[nodiscard]] compat::i32 set_volume(
        compat::i32 stream_id,
        compat::i32 volume
    );
    [[nodiscard]] compat::i32 play(
        std::string_view filename,
        compat::i32 stream_id,
        compat::i32 volume,
        compat::i32 loop_count
    );
    [[nodiscard]] compat::i32 begin_fade(
        compat::i32 stream_id,
        compat::i32 divisor
    );
    [[nodiscard]] bool service();
    [[nodiscard]] bool stream_absent(compat::i32 stream_id);

    [[nodiscard]] std::size_t active_stream_count() const noexcept;
    [[nodiscard]] std::size_t free_stream_count() const noexcept;

private:
    static constexpr compat::u32 kNoNode = 0xFFFFFFFFU;
    static constexpr compat::u32 kFadingFlag = 0x80000000U;

    struct StreamNode {
        LegacyStreamHandle handle{};
        std::string filename;
        compat::i32 fade_step{};
        compat::i32 fixed_volume{};
        compat::u32 state_flags{};
        compat::u32 next{kNoNode};
    };

    [[nodiscard]] compat::u32 pop_free_node() noexcept;
    void push_free_node(compat::u32 node) noexcept;
    void push_active_node(compat::u32 node) noexcept;
    [[nodiscard]] compat::u32 find_active_node(compat::i32 stream_id);
    [[nodiscard]] std::size_t list_size(compat::u32 head) const noexcept;

    LegacyStreamBackend& backend_;
    std::vector<StreamNode> nodes_;
    compat::u32 active_head_{kNoNode};
    compat::u32 free_head_{kNoNode};
    compat::u32 driver_token_{};
    std::string last_error_;
    bool initialized_{};
    bool stream_enabled_{};
};

}  // namespace openswd3::audio_video
