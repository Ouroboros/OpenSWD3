#include "openswd3/audio_video/legacy_sample_manager.hpp"

#include "openswd3/audio_video/legacy_audio_parameters.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace openswd3::audio_video {
namespace {

constexpr compat::i32 kMaximumSampleHandleCount = 16;

[[nodiscard]] bool is_riff(
    const std::span<const compat::u8> bytes
) noexcept {
    return bytes.size() >= 4U &&
        bytes[0] == static_cast<compat::u8>('R') &&
        bytes[1] == static_cast<compat::u8>('I') &&
        bytes[2] == static_cast<compat::u8>('F') &&
        bytes[3] == static_cast<compat::u8>('F');
}

}  // namespace

LegacySampleManager::LegacySampleManager(
    LegacySampleBackend& backend,
    LegacySndArchive& archive
) noexcept : backend_(backend), archive_(archive) {}

LegacySampleManager::~LegacySampleManager() {
    static_cast<void>(shutdown());
}

LegacySampleManagerInitializeStatus LegacySampleManager::initialize_pool(
    const compat::i32 requested_handle_count
) {
    if (initialized_) {
        return LegacySampleManagerInitializeStatus::ready;
    }
    if (!archive_.is_open()) {
        return LegacySampleManagerInitializeStatus::archive_not_open;
    }

    configured_handle_count_ = std::min(
        requested_handle_count,
        kMaximumSampleHandleCount
    );
    const compat::i32 allocation_count = std::max(
        configured_handle_count_,
        compat::i32{0}
    );

    try {
        nodes_.reserve(static_cast<std::size_t>(allocation_count));
    } catch (const std::bad_alloc&) {
        return LegacySampleManagerInitializeStatus::pool_allocation_failed;
    }

    initialized_ = true;
    sample_enabled_ = true;
    for (compat::i32 index = 0; index < allocation_count; ++index) {
        const LegacySampleHandle handle = backend_.allocate_sample_handle();
        if (handle == 0U) {
            break;
        }

        backend_.initialize_sample(handle);
        const compat::u32 node = static_cast<compat::u32>(nodes_.size());
        nodes_.push_back(SampleNode{handle, free_head_});
        free_head_ = node;
    }
    return LegacySampleManagerInitializeStatus::ready;
}

bool LegacySampleManager::shutdown() {
    if (!initialized_) {
        return true;
    }

    compat::u32 node = active_head_;
    while (node != kNoNode) {
        const SampleNode& sample = nodes_[node];
        backend_.end_sample(sample.handle);
        backend_.release_sample_handle(sample.handle);
        node = sample.next;
    }

    node = free_head_;
    while (node != kNoNode) {
        const SampleNode& sample = nodes_[node];
        backend_.end_sample(sample.handle);
        backend_.release_sample_handle(sample.handle);
        node = sample.next;
    }

    nodes_.clear();
    buffers_.clear();
    active_head_ = kNoNode;
    free_head_ = kNoNode;
    configured_handle_count_ = 0;
    initialized_ = false;
    sample_enabled_ = false;
    archive_.close();
    backend_.close_output();
    return true;
}

bool LegacySampleManager::initialized() const noexcept {
    return initialized_;
}

bool LegacySampleManager::sample_enabled() const noexcept {
    return sample_enabled_;
}

void LegacySampleManager::set_sample_enabled(const bool enabled) noexcept {
    sample_enabled_ = enabled;
}

compat::u32 LegacySampleManager::driver_token() const {
    return backend_.driver_token();
}

compat::i32 LegacySampleManager::configured_handle_count() const noexcept {
    return configured_handle_count_;
}

compat::i32 LegacySampleManager::play(LegacySamplePlayRequest request) {
    if (!initialized_ || !sample_enabled_ || request.sound_id == 0U ||
        request.sound_id > kLegacySndSlotCount) {
        return 0;
    }

    std::vector<compat::u8> sample_bytes;
    if (request.existing_buffer.has_value()) {
        sample_bytes = std::move(*request.existing_buffer);
    } else {
        LegacySndSampleBuffer loaded = archive_.load_sample(request.sound_id);
        if (loaded.status != LegacySndSampleStatus::ready) {
            return 0;
        }
        sample_bytes = std::move(loaded.bytes);
    }

    const compat::u32 buffer_token = retain_buffer(std::move(sample_bytes));
    if (buffer_token == 0U) {
        return 0;
    }

    const compat::u32 node = pop_free_node();
    if (node == kNoNode) {
        // sub_485CE0 does not free a freshly loaded buffer when no sample
        // handle is available.
        return 0;
    }

    SampleNode& sample = nodes_[node];
    backend_.initialize_sample(sample.handle);
    const std::span<const compat::u8> bytes = buffer_bytes(buffer_token);
    const bool configured = is_riff(bytes)
        ? backend_.set_sample_file(sample.handle, bytes)
        : backend_.set_named_sample_file(
              sample.handle,
              ".mp3",
              bytes,
              request.named_file_auxiliary
          );
    if (!configured) {
        release_buffer(buffer_token);
        push_free_node(node);
        return 0;
    }

    LegacySndRuntimeEntry& entry = archive_.entries_[request.sound_id - 1U];
    ++entry.reference_count;
    entry.buffer_token = buffer_token;

    backend_.set_sample_user_data(sample.handle, 0U, request.sound_id);
    backend_.set_sample_volume(
        sample.handle,
        legacy_audio_volume_parameter(request.volume)
    );
    backend_.set_sample_pan(
        sample.handle,
        legacy_audio_pan_parameter(request.pan)
    );
    backend_.set_sample_loop_count(sample.handle, request.loop_count);
    backend_.start_sample(sample.handle);
    push_active_node(node);
    return 0;
}

compat::i32 LegacySampleManager::stop(const compat::u32 sound_id) {
    if (!initialized_ || !sample_enabled_ || sound_id == 0U) {
        return 0;
    }

    const compat::u32 node = unlink_active_node(sound_id);
    if (node == kNoNode) {
        return 0;
    }

    const LegacySampleHandle handle = nodes_[node].handle;
    backend_.end_sample(handle);
    release_sample_reference(handle);
    push_free_node(node);
    return 0;
}

bool LegacySampleManager::stop_all() {
    if (!initialized_ || !sample_enabled_) {
        return false;
    }

    compat::u32 node = active_head_;
    while (node != kNoNode) {
        const compat::u32 next = nodes_[node].next;
        const LegacySampleHandle handle = nodes_[node].handle;
        backend_.end_sample(handle);
        release_sample_reference(handle);
        push_free_node(node);
        node = next;
    }
    active_head_ = kNoNode;
    return true;
}

compat::i32 LegacySampleManager::set_volume(
    const compat::u32 sound_id,
    const compat::i32 volume
) {
    if (!initialized_ || !sample_enabled_) {
        return 0;
    }

    const compat::u32 node = find_active_node(sound_id);
    const compat::i32 converted = legacy_audio_volume_parameter(volume);
    if (node != kNoNode) {
        backend_.set_sample_volume(nodes_[node].handle, converted);
    }
    return 0;
}

compat::i32 LegacySampleManager::set_pan(
    const compat::u32 sound_id,
    const compat::i32 pan
) {
    if (!initialized_ || !sample_enabled_) {
        return 0;
    }

    const compat::u32 node = find_active_node(sound_id);
    const compat::i32 converted = legacy_audio_pan_parameter(pan);
    if (node != kNoNode) {
        backend_.set_sample_pan(nodes_[node].handle, converted);
    }
    return converted;
}

bool LegacySampleManager::service_completed_samples() {
    if (!initialized_ || !sample_enabled_) {
        return false;
    }

    compat::u32 previous = kNoNode;
    compat::u32 node = active_head_;
    while (node != kNoNode) {
        const compat::u32 status = backend_.sample_status(nodes_[node].handle);
        if (status >= 1U && status <= 2U) {
            const compat::u32 next = nodes_[node].next;
            if (previous == kNoNode) {
                active_head_ = next;
            } else {
                nodes_[previous].next = next;
            }

            const LegacySampleHandle handle = nodes_[node].handle;
            push_free_node(node);
            backend_.set_sample_volume(handle, 0);
            backend_.end_sample(handle);
            release_sample_reference(handle);
            node = next;
            continue;
        }

        previous = node;
        node = nodes_[node].next;
    }
    return true;
}

std::size_t LegacySampleManager::active_sample_count() const noexcept {
    return list_size(active_head_);
}

std::size_t LegacySampleManager::free_sample_count() const noexcept {
    return list_size(free_head_);
}

std::size_t LegacySampleManager::live_buffer_count() const noexcept {
    return static_cast<std::size_t>(std::count_if(
        buffers_.cbegin(),
        buffers_.cend(),
        [](const OwnedBuffer& buffer) { return buffer.live; }
    ));
}

bool LegacySampleManager::buffer_is_live(const compat::u32 token) const noexcept {
    return token != 0U && token <= buffers_.size() &&
        buffers_[token - 1U].live;
}

compat::u32 LegacySampleManager::pop_free_node() noexcept {
    const compat::u32 node = free_head_;
    free_head_ = kNoNode;
    if (node != kNoNode) {
        free_head_ = nodes_[node].next;
    }
    return node;
}

void LegacySampleManager::push_free_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = free_head_;
    free_head_ = node;
}

void LegacySampleManager::push_active_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = active_head_;
    active_head_ = node;
}

compat::u32 LegacySampleManager::find_active_node(
    const compat::u32 sound_id
) {
    compat::u32 node = active_head_;
    while (node != kNoNode) {
        if (backend_.sample_user_data(nodes_[node].handle, 0U) == sound_id) {
            return node;
        }
        node = nodes_[node].next;
    }
    return kNoNode;
}

compat::u32 LegacySampleManager::unlink_active_node(
    const compat::u32 sound_id
) {
    compat::u32 previous = kNoNode;
    compat::u32 node = active_head_;
    while (node != kNoNode) {
        if (backend_.sample_user_data(nodes_[node].handle, 0U) == sound_id) {
            if (previous == kNoNode) {
                active_head_ = nodes_[node].next;
            } else {
                nodes_[previous].next = nodes_[node].next;
            }
            return node;
        }
        previous = node;
        node = nodes_[node].next;
    }
    return kNoNode;
}

std::size_t LegacySampleManager::list_size(
    compat::u32 head
) const noexcept {
    std::size_t count{};
    while (head != kNoNode) {
        ++count;
        head = nodes_[head].next;
    }
    return count;
}

compat::u32 LegacySampleManager::retain_buffer(
    std::vector<compat::u8> bytes
) {
    if (buffers_.size() >= std::numeric_limits<compat::u32>::max()) {
        return 0U;
    }
    try {
        buffers_.push_back(OwnedBuffer{std::move(bytes), true});
    } catch (const std::bad_alloc&) {
        return 0U;
    }
    return static_cast<compat::u32>(buffers_.size());
}

void LegacySampleManager::release_buffer(const compat::u32 token) noexcept {
    if (!buffer_is_live(token)) {
        return;
    }
    OwnedBuffer& buffer = buffers_[token - 1U];
    std::vector<compat::u8>{}.swap(buffer.bytes);
    buffer.live = false;
}

std::span<const compat::u8> LegacySampleManager::buffer_bytes(
    const compat::u32 token
) const noexcept {
    if (!buffer_is_live(token)) {
        return {};
    }
    return buffers_[token - 1U].bytes;
}

void LegacySampleManager::release_sample_reference(
    const LegacySampleHandle handle
) {
    const compat::u32 sound_id = backend_.sample_user_data(handle, 0U);
    if (sound_id == 0U || sound_id > kLegacySndSlotCount) {
        return;
    }

    LegacySndRuntimeEntry& entry = archive_.entries_[sound_id - 1U];
    --entry.reference_count;
    if (entry.reference_count == 0U) {
        release_buffer(entry.buffer_token);
        entry.buffer_token = 0U;
    }
}

}  // namespace openswd3::audio_video
