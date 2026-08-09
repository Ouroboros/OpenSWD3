#pragma once

#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/compat/types.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace openswd3::asset_runtime {

inline constexpr std::size_t kLegacyActionRecordSize = 0x98U;

struct LegacyActionRecord {
    compat::u32 action_id;
    compat::u32 cached_action_id;
    compat::u32 base_variant;
    compat::u32 cached_base_variant;
    compat::u32 draw_offset_x;
    compat::u32 draw_offset_y;
    compat::u32 mode_flags;
    compat::u32 field_1c;
    compat::u32 one_shot_base_variant;
    compat::u32 field_24;
    compat::u32 field_28;
    compat::u32 field_2c;
    compat::u32 field_30;
    compat::u32 variant_delta;
    compat::u32 cached_variant_delta;
    compat::u32 one_shot_variant_delta;
    compat::u16 packed_ap_state;
    compat::u16 command_cursor;
    compat::u16 wait_remaining;
    compat::u16 wait_default;
    compat::u16 wait_override;
    compat::u16 field_4a;
    compat::u16 field_4c;
    compat::u16 field_4e;
    compat::u16 field_50;
    compat::u16 field_52;
    compat::u32 stream_pointer_32;
    compat::u16 field_58;
    compat::u16 field_5a;
    compat::u16 field_5c;
    compat::u16 field_5e;
    compat::u16 field_60;
    compat::u16 field_62;
    compat::u16 field_64;
    compat::u16 field_66;
    compat::u16 field_68;
    compat::u16 field_6a;
    compat::u16 field_6c;
    compat::u16 field_6e;
    compat::u16 field_70;
    compat::u16 field_72;
    compat::u16 field_74;
    compat::u16 field_76;
    compat::u16 field_78;
    compat::u16 field_7a;
    compat::u16 field_7c;
    compat::u16 field_7e;
    compat::u16 field_80;
    compat::u16 field_82;
    compat::u16 field_84;
    compat::u16 field_86;
    compat::u8 field_88;
    compat::u8 field_89;
    compat::u8 field_8a;
    compat::u8 field_8b;
    compat::u32 field_8c;
    compat::u32 external_mode;
    compat::u32 field_94;
};

static_assert(sizeof(LegacyActionRecord) == kLegacyActionRecordSize);
static_assert(offsetof(LegacyActionRecord, action_id) == 0x00U);
static_assert(offsetof(LegacyActionRecord, mode_flags) == 0x18U);
static_assert(offsetof(LegacyActionRecord, variant_delta) == 0x34U);
static_assert(offsetof(LegacyActionRecord, packed_ap_state) == 0x40U);
static_assert(offsetof(LegacyActionRecord, command_cursor) == 0x42U);
static_assert(offsetof(LegacyActionRecord, wait_remaining) == 0x44U);
static_assert(offsetof(LegacyActionRecord, stream_pointer_32) == 0x54U);
static_assert(offsetof(LegacyActionRecord, field_88) == 0x88U);
static_assert(offsetof(LegacyActionRecord, field_8c) == 0x8CU);
static_assert(offsetof(LegacyActionRecord, external_mode) == 0x90U);
static_assert(offsetof(LegacyActionRecord, field_94) == 0x94U);

void initialize_legacy_action_record(LegacyActionRecord& record) noexcept;

enum class LegacyActionStreamStatus {
    ready,
    load_failed,
};

struct LegacyActionStreamLoadResult {
    LegacyActionStreamStatus status{LegacyActionStreamStatus::load_failed};
    std::span<const compat::u8> stream;
    bool cache_hit{};
};

class LegacyActionStreamProvider {
public:
    virtual ~LegacyActionStreamProvider() = default;

    [[nodiscard]] virtual LegacyActionStreamLoadResult
    load_action_stream(compat::u32 action_id, compat::u32 variant_index,
                       bool cached) = 0;
};

class LegacyActActionStreamProvider final : public LegacyActionStreamProvider {
public:
    explicit LegacyActActionStreamProvider(LegacyActRuntime& runtime) noexcept;

    [[nodiscard]] LegacyActionStreamLoadResult
    load_action_stream(compat::u32 action_id, compat::u32 variant_index,
                       bool cached) override;

private:
    LegacyActRuntime& runtime_;
    std::vector<compat::u8> direct_stream_;
};

enum class LegacyActionUpdateStatus {
    completed,
    stream_load_failed,
    malformed_stream,
};

struct LegacyActionUpdateResult {
    LegacyActionUpdateStatus status{LegacyActionUpdateStatus::completed};
    compat::u32 return_value{1U};
    bool key_changed{};
    bool cache_hit{};
};

class LegacyActionUpdater final {
public:
    explicit LegacyActionUpdater(LegacyActionStreamProvider& provider) noexcept;

    void set_stream_cache_mode(compat::u32 value) noexcept;
    [[nodiscard]] compat::u32 stream_cache_mode() const noexcept;

    [[nodiscard]] LegacyActionUpdateResult update(LegacyActionRecord& record);

private:
    LegacyActionStreamProvider& provider_;
    compat::u32 stream_cache_mode_{};
};

}  // namespace openswd3::asset_runtime
