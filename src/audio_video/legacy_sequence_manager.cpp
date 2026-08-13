#include "openswd3/audio_video/legacy_sequence_manager.hpp"

#include "openswd3/audio_video/legacy_audio_parameters.hpp"
#include "openswd3/resource_io/legacy_file.hpp"

#include <bit>
#include <filesystem>
#include <limits>
#include <new>
#include <utility>

namespace openswd3::audio_video {
namespace {

[[nodiscard]] constexpr compat::i32
from_bits(const compat::u32 value) noexcept {
    return std::bit_cast<compat::i32>(value);
}

[[nodiscard]] constexpr compat::i32
wrapping_subtract(const compat::i32 left, const compat::i32 right) noexcept {
    return from_bits(
        static_cast<compat::u32>(left) - static_cast<compat::u32>(right)
    );
}

[[nodiscard]] compat::i32 fixed_volume(const compat::i32 volume) noexcept {
    return from_bits(
        static_cast<compat::u32>(legacy_audio_volume_parameter(volume)) << 4U
    );
}

[[nodiscard]] bool status_is_retained(const compat::u32 status) noexcept {
    return status == 4U || status == 8U || status == 16U;
}

}  // namespace

LegacySequenceManager::LegacySequenceManager(
    LegacySequenceBackend& backend
) noexcept
    : backend_(backend) {}

LegacySequenceManager::~LegacySequenceManager() {
    static_cast<void>(shutdown());
}

LegacySequenceManagerInitializeStatus
LegacySequenceManager::initialize_output(const compat::u32) {
    if (!backend_.open_midi_output(-1, midi_driver_)) {
        copy_backend_error();
        if (!backend_.open_midi_output(0, midi_driver_)) {
            const std::string first_error = last_error_;
            copy_backend_error();
            try {
                last_error_ = first_error + " //" + last_error_;
            } catch (const std::bad_alloc&) {
                last_error_ = first_error;
            }
            midi_driver_ = 0U;
            sequence_enabled_ = false;
            return LegacySequenceManagerInitializeStatus::
                midi_output_open_failed;
        }
    }

    initialized_ = true;
    sequence_enabled_ = true;
    try {
        nodes_.reserve(nodes_.size() + 1U);
        const compat::u32 node = static_cast<compat::u32>(nodes_.size());
        SequenceNode sequence;
        sequence.handle = backend_.allocate_sequence_handle(midi_driver_);
        nodes_.push_back(std::move(sequence));
        push_free_node(node);
    } catch (const std::bad_alloc&) {
        return LegacySequenceManagerInitializeStatus::node_allocation_failed;
    }
    return LegacySequenceManagerInitializeStatus::ready;
}

bool LegacySequenceManager::shutdown() {
    if (!initialized_) {
        return true;
    }

    compat::u32 node = active_head_;
    while (node != kNoNode) {
        SequenceNode& sequence = nodes_[node];
        const compat::u32 next = sequence.next;
        backend_.end_sequence(sequence.handle);
        backend_.release_sequence_handle(sequence.handle);
        std::vector<compat::u8>{}.swap(sequence.bytes);
        node = next;
    }

    node = free_head_;
    while (node != kNoNode) {
        SequenceNode& sequence = nodes_[node];
        const compat::u32 next = sequence.next;
        backend_.release_sequence_handle(sequence.handle);
        node = next;
    }

    backend_.close_midi_output(midi_driver_);
    nodes_.clear();
    active_head_ = kNoNode;
    free_head_ = kNoNode;
    initialized_ = false;
    sequence_enabled_ = false;
    return true;
}

bool LegacySequenceManager::initialized() const noexcept {
    return initialized_;
}

bool LegacySequenceManager::sequence_enabled() const noexcept {
    return sequence_enabled_;
}

void LegacySequenceManager::set_sequence_enabled(const bool enabled) noexcept {
    sequence_enabled_ = enabled;
}

LegacyMidiDriverHandle LegacySequenceManager::midi_driver() const noexcept {
    return midi_driver_;
}

std::string_view LegacySequenceManager::last_error() const noexcept {
    return last_error_;
}

compat::i32 LegacySequenceManager::play(
    const std::string_view filename,
    const compat::i32 sequence_id,
    const compat::i32 volume,
    const compat::i32 loop_count
) {
    if (!initialized_ || !sequence_enabled_ || sequence_id == 0) {
        return 0;
    }

    const compat::u32 node = pop_free_node();
    if (node == kNoNode) {
        return 0;
    }
    SequenceNode& sequence = nodes_[node];
    sequence.bytes.clear();
    sequence.fade_step = 0;

    resource_io::LegacyFile file;
    if (!file.open(
            std::filesystem::path{std::string{filename}},
            resource_io::LegacyFileCreation::open_always,
            resource_io::LegacyFileAccess::read
        )) {
        copy_backend_error();
        push_free_node(node);
        return 0;
    }

    const compat::u32 file_size = file.size();
    try {
        sequence.bytes.resize(file_size);
    } catch (const std::bad_alloc&) {
        copy_backend_error();
        push_free_node(node);
        return 0;
    }
    if (file_size != 0U) {
        compat::u32 read_size = file_size;
        static_cast<void>(file.read(sequence.bytes, read_size));
    }
    static_cast<void>(file.close());

    const compat::i32 initialize_result =
        backend_.initialize_sequence(sequence.handle, sequence.bytes, 0U);
    if (initialize_result == 0) {
        copy_backend_error();
        std::vector<compat::u8>{}.swap(sequence.bytes);
        push_free_node(node);
        return 0;
    }
    if (initialize_result == -1) {
        copy_backend_error();
    }

    sequence.fixed_volume = fixed_volume(volume);
    backend_.set_sequence_user_data(sequence.handle, 0U, sequence_id);
    backend_.set_sequence_volume(
        sequence.handle,
        legacy_audio_volume_parameter(sequence.fixed_volume / 16),
        0
    );
    backend_.set_sequence_loop_count(sequence.handle, loop_count);
    backend_.start_sequence(sequence.handle);
    push_active_node(node);
    return sequence_id;
}

bool LegacySequenceManager::service() {
    if (!initialized_ || !sequence_enabled_) {
        return false;
    }

    compat::u32 previous = kNoNode;
    compat::u32 node = active_head_;
    bool previous_was_removed = false;
    while (node != kNoNode) {
        SequenceNode& sequence = nodes_[node];
        bool remove = false;

        if ((sequence.state_flags & kFadingFlag) != 0U) {
            sequence.fixed_volume =
                wrapping_subtract(sequence.fixed_volume, sequence.fade_step);
            const compat::i32 converted =
                legacy_audio_volume_parameter(sequence.fixed_volume / 16);
            if (converted != 0) {
                backend_.set_sequence_volume(sequence.handle, converted, 0);
                previous_was_removed = false;
                previous = node;
                node = sequence.next;
                continue;
            }
            remove = true;
        } else {
            const compat::u32 status =
                backend_.sequence_status(sequence.handle);
            if (status == 1U || status == 2U) {
                remove = true;
            } else if (status_is_retained(status)) {
                previous_was_removed = false;
                previous = node;
                node = sequence.next;
                continue;
            } else if (previous_was_removed) {
                remove = true;
            } else {
                previous = node;
                node = sequence.next;
                continue;
            }
        }

        if (remove) {
            previous_was_removed = true;
            const compat::u32 next = sequence.next;
            backend_.end_sequence(sequence.handle);
            std::vector<compat::u8>{}.swap(sequence.bytes);
            sequence.fade_step = 0;
            sequence.state_flags = 0U;

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

bool LegacySequenceManager::sequence_absent(const compat::i32 sequence_id) {
    if (sequence_id == 0) {
        return active_head_ == kNoNode;
    }
    return find_active_node(sequence_id) == kNoNode;
}

std::size_t LegacySequenceManager::active_sequence_count() const noexcept {
    return list_size(active_head_);
}

std::size_t LegacySequenceManager::free_sequence_count() const noexcept {
    return list_size(free_head_);
}

void LegacySequenceManager::copy_backend_error() {
    try {
        last_error_.assign(backend_.last_error());
    } catch (const std::bad_alloc&) {
        last_error_ = "sequence backend failure";
    }
}

compat::u32 LegacySequenceManager::pop_free_node() noexcept {
    const compat::u32 node = free_head_;
    free_head_ = kNoNode;
    if (node != kNoNode) {
        free_head_ = nodes_[node].next;
    }
    return node;
}

void LegacySequenceManager::push_free_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = free_head_;
    free_head_ = node;
}

void LegacySequenceManager::push_active_node(const compat::u32 node) noexcept {
    if (node == kNoNode) {
        return;
    }
    nodes_[node].next = active_head_;
    active_head_ = node;
}

compat::u32
LegacySequenceManager::find_active_node(const compat::i32 sequence_id) {
    compat::u32 node = active_head_;
    while (node != kNoNode) {
        static_cast<void>(backend_.sequence_user_data(nodes_[node].handle, 0U));
        if (backend_.sequence_user_data(nodes_[node].handle, 0U) ==
            sequence_id) {
            return node;
        }
        node = nodes_[node].next;
    }
    return kNoNode;
}

std::size_t LegacySequenceManager::list_size(compat::u32 head) const noexcept {
    std::size_t count{};
    while (head != kNoNode) {
        ++count;
        head = nodes_[head].next;
    }
    return count;
}

}  // namespace openswd3::audio_video
