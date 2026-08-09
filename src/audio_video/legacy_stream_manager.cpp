#include "openswd3/audio_video/legacy_stream_manager.hpp"

#include "openswd3/audio_video/legacy_audio_parameters.hpp"

#include <bit>
#include <csignal>
#include <new>

namespace openswd3::audio_video {
namespace {

[[nodiscard]] constexpr compat::i32 from_bits(
    const compat::u32 value
) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32 wrapping_subtract(
    const compat::i32 left,
    const compat::i32 right
) noexcept {
    return from_bits(
        static_cast<compat::u32>(left) - static_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32 fixed_volume(
    const compat::i32 volume
) noexcept {
    return from_bits(
        static_cast<compat::u32>(legacy_audio_volume_parameter(volume)) << 4U
    );
}

[[nodiscard]] compat::i32 legacy_fade_step(
    const compat::i32 volume,
    const compat::i32 divisor
) {
    // 0x004868E8 raises the x86 integer-divide exception for a zero divisor.
    // Keep this an observable fatal error instead of silently repairing it.
    if (divisor == 0) {
        static_cast<void>(std::raise(SIGFPE));
        return 0;
    }
    const compat::i32 quotient = volume / divisor;
    return quotient == 0 ? 1 : quotient;
}

[[nodiscard]] bool status_is_retained(const compat::u32 status) noexcept {
    return status == 4U || status == 8U || status == 16U;
}

}  // namespace

LegacyStreamManager::LegacyStreamManager(
    LegacyStreamBackend& backend
) noexcept : backend_(backend) {}

LegacyStreamManager::~LegacyStreamManager() {
    static_cast<void>(shutdown());
}

LegacyStreamManagerInitializeStatus LegacyStreamManager::initialize_pool(
    const compat::u32 driver_token
) {
    driver_token_ = driver_token;
    initialized_ = true;
    stream_enabled_ = true;

    try {
        nodes_.reserve(nodes_.size() + 2U);
        for (std::size_t index = 0U; index < 2U; ++index) {
            const compat::u32 node = static_cast<compat::u32>(nodes_.size());
            nodes_.push_back(StreamNode{});
            push_free_node(node);
        }
    } catch (const std::bad_alloc&) {
        return LegacyStreamManagerInitializeStatus::pool_allocation_failed;
    }
    return LegacyStreamManagerInitializeStatus::ready;
}

bool LegacyStreamManager::shutdown() {
    if (!initialized_) {
        return true;
    }

    compat::u32 node = active_head_;
    while (node != kNoNode) {
        StreamNode& stream = nodes_[node];
        const compat::u32 next = stream.next;
        std::string{}.swap(stream.filename);
        backend_.close_stream(stream.handle);
        node = next;
    }

    node = free_head_;
    while (node != kNoNode) {
        StreamNode& stream = nodes_[node];
        const compat::u32 next = stream.next;
        std::string{}.swap(stream.filename);
        backend_.close_stream(stream.handle);
        node = next;
    }

    nodes_.clear();
    active_head_ = kNoNode;
    free_head_ = kNoNode;
    initialized_ = false;
    stream_enabled_ = false;
    return true;
}

bool LegacyStreamManager::initialized() const noexcept {
    return initialized_;
}

bool LegacyStreamManager::stream_enabled() const noexcept {
    return stream_enabled_;
}

void LegacyStreamManager::set_stream_enabled(const bool enabled) noexcept {
    stream_enabled_ = enabled;
}

compat::u32 LegacyStreamManager::driver_token() const noexcept {
    return driver_token_;
}

std::string_view LegacyStreamManager::last_error() const noexcept {
    return last_error_;
}

compat::i32 LegacyStreamManager::set_volume(
    const compat::i32 stream_id,
    const compat::i32 volume
) {
    if (!initialized_) {
        return 0;
    }

    const compat::u32 node = find_active_node(stream_id);
    if (node == kNoNode) {
        return -1;
    }

    StreamNode& stream = nodes_[node];
    stream.fixed_volume = fixed_volume(volume);
    backend_.set_stream_volume(
        stream.handle,
        legacy_audio_volume_parameter(volume)
    );
    return backend_.stream_volume(stream.handle);
}

compat::i32 LegacyStreamManager::play(
    const std::string_view filename,
    const compat::i32 stream_id,
    const compat::i32 volume,
    const compat::i32 loop_count
) {
    if (stream_id == 0 || find_active_node(stream_id) != kNoNode) {
        return 0;
    }

    const compat::u32 node = pop_free_node();
    if (node == kNoNode) {
        return 0;
    }

    StreamNode& stream = nodes_[node];
    stream.filename.clear();
    stream.fade_step = 0;
    stream.handle = backend_.open_stream(driver_token_, filename, 0);
    if (stream.handle == 0U) {
        try {
            last_error_.assign(backend_.last_error());
        } catch (const std::bad_alloc&) {
            last_error_ = "stream open failed";
        }
        push_free_node(node);
        return 0;
    }

    try {
        stream.filename.assign(filename);
    } catch (const std::bad_alloc&) {
        backend_.close_stream(stream.handle);
        stream.handle = 0U;
        last_error_ = "stream filename allocation failed";
        push_free_node(node);
        return 0;
    }

    stream.fixed_volume = fixed_volume(volume);
    backend_.set_stream_user_data(stream.handle, 0U, stream_id);
    backend_.set_stream_volume(
        stream.handle,
        legacy_audio_volume_parameter(volume)
    );
    backend_.set_stream_loop_count(stream.handle, loop_count);
    backend_.start_stream(stream.handle);
    push_active_node(node);
    return stream_id;
}

compat::i32 LegacyStreamManager::begin_fade(
    const compat::i32 stream_id,
    const compat::i32 divisor
) {
    if (!initialized_ || !stream_enabled_ || stream_id == 0) {
        return 0;
    }

    const compat::u32 node = find_active_node(stream_id);
    if (node == kNoNode) {
        return 0;
    }

    StreamNode& stream = nodes_[node];
    compat::i32 total_milliseconds{};
    compat::i32 current_milliseconds{};
    backend_.stream_ms_position(
        stream.handle,
        total_milliseconds,
        current_milliseconds
    );
    stream.state_flags |= kFadingFlag;
    stream.fade_step = legacy_fade_step(stream.fixed_volume, divisor);
    return stream_id;
}

bool LegacyStreamManager::service() {
    compat::u32 previous = kNoNode;
    compat::u32 node = active_head_;
    bool previous_was_removed = false;

    while (node != kNoNode) {
        StreamNode& stream = nodes_[node];
        bool remove = false;

        if ((stream.state_flags & kFadingFlag) != 0U) {
            stream.fixed_volume = wrapping_subtract(
                stream.fixed_volume,
                stream.fade_step
            );
            const compat::i32 converted = legacy_audio_volume_parameter(
                stream.fixed_volume / 16
            );
            if (converted != 0) {
                backend_.set_stream_volume(stream.handle, converted);
                previous_was_removed = false;
                previous = node;
                node = stream.next;
                continue;
            }
            remove = true;
        } else {
            const compat::u32 status = backend_.stream_status(stream.handle);
            if (status == 2U) {
                backend_.set_stream_volume(stream.handle, 0);
                remove = true;
            } else if (status_is_retained(status)) {
                previous_was_removed = false;
                previous = node;
                node = stream.next;
                continue;
            } else if (previous_was_removed) {
                remove = true;
            } else {
                previous = node;
                node = stream.next;
                continue;
            }
        }

        if (remove) {
            previous_was_removed = true;
            const compat::u32 next = stream.next;
            backend_.set_stream_volume(stream.handle, 0);
            backend_.close_stream(stream.handle);
            std::string{}.swap(stream.filename);
            stream.handle = 0U;
            stream.fade_step = 0;
            stream.state_flags = 0U;

            if (previous == kNoNode) {
                active_head_ = next;
            } else {
                nodes_[previous].next = next;
            }
            push_free_node(node);
            node = next;
        }
    }
    return false;
}

bool LegacyStreamManager::stream_absent(const compat::i32 stream_id) {
    return find_active_node(stream_id) == kNoNode;
}

std::size_t LegacyStreamManager::active_stream_count() const noexcept {
    return list_size(active_head_);
}

std::size_t LegacyStreamManager::free_stream_count() const noexcept {
    return list_size(free_head_);
}

compat::u32 LegacyStreamManager::pop_free_node() noexcept {
    const compat::u32 node = free_head_;
    free_head_ = kNoNode;
    if (node != kNoNode) {
        free_head_ = nodes_[node].next;
    }
    return node;
}

void LegacyStreamManager::push_free_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = free_head_;
    free_head_ = node;
}

void LegacyStreamManager::push_active_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = active_head_;
    active_head_ = node;
}

compat::u32 LegacyStreamManager::find_active_node(
    const compat::i32 stream_id
) {
    compat::u32 node = active_head_;
    while (node != kNoNode) {
        if (backend_.stream_user_data(nodes_[node].handle, 0U) == stream_id) {
            return node;
        }
        node = nodes_[node].next;
    }
    return kNoNode;
}

std::size_t LegacyStreamManager::list_size(
    compat::u32 head
) const noexcept {
    std::size_t count{};
    while (head != kNoNode) {
        ++count;
        head = nodes_[head].next;
    }
    return count;
}

}  // namespace openswd3::audio_video
