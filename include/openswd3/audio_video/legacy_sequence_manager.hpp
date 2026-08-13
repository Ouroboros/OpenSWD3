#pragma once

#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace openswd3::audio_video {

using LegacyMidiDriverHandle = compat::u32;
using LegacySequenceHandle = compat::u32;

class LegacySequenceBackend {
public:
    virtual ~LegacySequenceBackend() = default;

    [[nodiscard]] virtual bool
    open_midi_output(compat::i32 device_id, LegacyMidiDriverHandle& driver) = 0;
    [[nodiscard]] virtual std::string_view last_error() const = 0;
    virtual void close_midi_output(LegacyMidiDriverHandle driver) = 0;

    [[nodiscard]] virtual LegacySequenceHandle
    allocate_sequence_handle(LegacyMidiDriverHandle driver) = 0;
    virtual void release_sequence_handle(LegacySequenceHandle handle) = 0;
    [[nodiscard]] virtual compat::i32 initialize_sequence(
        LegacySequenceHandle handle,
        std::span<const compat::u8> bytes,
        compat::u32 start_offset
    ) = 0;
    virtual void set_sequence_user_data(
        LegacySequenceHandle handle, compat::u32 slot, compat::i32 value
    ) = 0;
    [[nodiscard]] virtual compat::i32
    sequence_user_data(LegacySequenceHandle handle, compat::u32 slot) = 0;
    virtual void set_sequence_volume(
        LegacySequenceHandle handle,
        compat::i32 volume,
        compat::i32 milliseconds
    ) = 0;
    virtual void set_sequence_loop_count(
        LegacySequenceHandle handle, compat::i32 loop_count
    ) = 0;
    virtual void start_sequence(LegacySequenceHandle handle) = 0;
    [[nodiscard]] virtual compat::u32
    sequence_status(LegacySequenceHandle handle) = 0;
    virtual void end_sequence(LegacySequenceHandle handle) = 0;
};

enum class LegacySequenceManagerInitializeStatus {
    ready,
    midi_output_open_failed,
    node_allocation_failed,
};

class LegacySequenceManager final {
public:
    explicit LegacySequenceManager(LegacySequenceBackend& backend) noexcept;
    ~LegacySequenceManager();

    LegacySequenceManager(const LegacySequenceManager&) = delete;
    LegacySequenceManager& operator=(const LegacySequenceManager&) = delete;
    LegacySequenceManager(LegacySequenceManager&&) = delete;
    LegacySequenceManager& operator=(LegacySequenceManager&&) = delete;

    [[nodiscard]] LegacySequenceManagerInitializeStatus
    initialize_output(compat::u32 ignored_audio_driver);
    [[nodiscard]] bool shutdown();

    [[nodiscard]] bool initialized() const noexcept;
    [[nodiscard]] bool sequence_enabled() const noexcept;
    void set_sequence_enabled(bool enabled) noexcept;
    [[nodiscard]] LegacyMidiDriverHandle midi_driver() const noexcept;
    [[nodiscard]] std::string_view last_error() const noexcept;

    [[nodiscard]] compat::i32 play(
        std::string_view filename,
        compat::i32 sequence_id,
        compat::i32 volume,
        compat::i32 loop_count
    );
    [[nodiscard]] bool service();
    [[nodiscard]] bool sequence_absent(compat::i32 sequence_id);

    [[nodiscard]] std::size_t active_sequence_count() const noexcept;
    [[nodiscard]] std::size_t free_sequence_count() const noexcept;

private:
    static constexpr compat::u32 kNoNode = 0xFFFFFFFFU;
    static constexpr compat::u32 kFadingFlag = 0x80000000U;

    struct SequenceNode {
        LegacySequenceHandle handle{};
        std::vector<compat::u8> bytes;
        compat::i32 fade_step{};
        compat::i32 fixed_volume{};
        compat::u32 state_flags{};
        compat::u32 next{kNoNode};
    };

    void copy_backend_error();
    [[nodiscard]] compat::u32 pop_free_node() noexcept;
    void push_free_node(compat::u32 node) noexcept;
    void push_active_node(compat::u32 node) noexcept;
    [[nodiscard]] compat::u32 find_active_node(compat::i32 sequence_id);
    [[nodiscard]] std::size_t list_size(compat::u32 head) const noexcept;

    LegacySequenceBackend& backend_;
    std::vector<SequenceNode> nodes_;
    compat::u32 active_head_{kNoNode};
    compat::u32 free_head_{kNoNode};
    LegacyMidiDriverHandle midi_driver_{};
    std::string last_error_;
    bool initialized_{};
    bool sequence_enabled_{};
};

}  // namespace openswd3::audio_video
