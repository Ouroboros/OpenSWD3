#pragma once

#include "test.hpp"

#include "openswd3/asset_runtime/legacy_act_runtime.hpp"
#include "openswd3/world_map/legacy_world_head_sign_actions.hpp"
#include "openswd3/world_map/legacy_world_map_role_paths.hpp"
#include "openswd3/world_map/legacy_world_role_lookup.hpp"
#include "openswd3/world_map/legacy_world_runtime_session.hpp"
#include "openswd3/world_map/legacy_world_story_vm.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#ifndef OPENSWD3_TEST_ARTIFACT_ROOT
#error OPENSWD3_TEST_ARTIFACT_ROOT must name a build-tree directory
#endif

namespace {

using openswd3::compat::i16;
using openswd3::compat::i32;
using openswd3::compat::u8;
using openswd3::compat::u16;
using openswd3::compat::u32;
using openswd3::resource_io::LegacyTalkWindowLoadResult;
using openswd3::resource_io::LegacyTalkWindowStatus;
using openswd3::world_map::LegacyWorldItemListState;
using openswd3::world_map::LegacyWorldItemNode;
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldObjectSlot;
using openswd3::world_map::LegacyWorldStoryFileDirectory;
using openswd3::world_map::LegacyWorldStoryFileOperation;
using openswd3::world_map::LegacyWorldStoryVmPorts;
using openswd3::world_map::LegacyWorldStoryVmState;
using openswd3::world_map::LegacyWorldStoryVmStatus;
using openswd3::world_map::LegacyWorldTalkContext;
using openswd3::world_map::OP_10_SET_ROLE_BASE_VARIANT;
using openswd3::world_map::OP_12_SET_ROLE_POSITION;
using openswd3::world_map::OP_13_STEP_ROLE;
using openswd3::world_map::OP_14_WAIT_ROLE_ACTION_STATUS;
using openswd3::world_map::OP_15_JUMP_SAME_FILE_OFFSET;
using openswd3::world_map::OP_16_JUMP_IF_ROLE_PATH_UNPREPARED;
using openswd3::world_map::OP_17_JUMP_IF_ROLE_PATH_PREPARED;
using openswd3::world_map::OP_18_RELEASE_ROLE_PATH;
using openswd3::world_map::OP_19_RELEASE_ROLE_PATHS;
using openswd3::world_map::OP_20_SCHEDULE_ROLE_PATHS;
using openswd3::world_map::OP_21_JUMP_IF_GLOBAL_BIT_SET;
using openswd3::world_map::OP_22_JUMP_IF_GLOBAL_BIT_CLEAR;
using openswd3::world_map::OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET;
using openswd3::world_map::OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET;
using openswd3::world_map::OP_25_SET_GLOBAL_BIT;
using openswd3::world_map::OP_26_CLEAR_GLOBAL_BIT;
using openswd3::world_map::OP_27_RELOAD_WORLD_SESSION;
using openswd3::world_map::OP_28_CHANGE_ROLE_PATH_ID;
using openswd3::world_map::OP_29_SET_GLOBAL_INTEGER;
using openswd3::world_map::OP_30_ADD_GLOBAL_INTEGER;
using openswd3::world_map::OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO;
using openswd3::world_map::OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE;
using openswd3::world_map::OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE;
using openswd3::world_map::OP_34_SET_BOUNDED_SCRIPT_CLOCK;
using openswd3::world_map::OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK;
using openswd3::world_map::OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA;
using openswd3::world_map::OP_37_SNAPSHOT_SCRIPT_CLOCK;
using openswd3::world_map::OP_38_CLEAR_ROLE_FROM_SCENE;
using openswd3::world_map::OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS;
using openswd3::world_map::OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH;
using openswd3::world_map::OP_41_RELOAD_INDEXED_TARGET;
using openswd3::world_map::OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT;
using openswd3::world_map::OP_43_CLEAR_INTERACTION_LOCK;
using openswd3::world_map::OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE;
using openswd3::world_map::OP_45_SET_ROLE_ACTION_ID;
using openswd3::world_map::OP_46_RESTORE_ROLE_ACTION_OVERRIDES;
using openswd3::world_map::OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE;
using openswd3::world_map::OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE;
using openswd3::world_map::OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF;
using openswd3::world_map::OP_50_START_RELATIVE_CAMERA_MOVE;
using openswd3::world_map::OP_51_WAIT_CAMERA_MOVE_COMPLETE;
using openswd3::world_map::OP_52_START_FRAME_COLOR_TRANSITION;
using openswd3::world_map::OP_53_WAIT_FRAME_COLOR_TRANSITION;
using openswd3::world_map::OP_54_REPEAT_ROLE_ACTION_REFRESH;
using openswd3::world_map::OP_55_SET_ROLE_SPATIAL_GROUP_1;
using openswd3::world_map::OP_56_SET_ROLE_SPATIAL_GROUP_0;
using openswd3::world_map::OP_57_SET_ROLE_SPATIAL_GROUP_2;
using openswd3::world_map::OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION;
using openswd3::world_map::OP_59_PLAY_SOUND_EFFECT;
using openswd3::world_map::OP_60_RESUME_WORLD_SCENE_RENDERING;
using openswd3::world_map::OP_61_CLEAR_AND_SUSPEND_WORLD_SCENE_RENDERING;
using openswd3::world_map::OP_62_WRITE_MAP_ROLE;
using openswd3::world_map::OP_63_SET_SELECTION_SCROLL;
using openswd3::world_map::OP_64_CLEAR_SELECTION_SCROLL;
using openswd3::world_map::OP_65_TRANSFER_ROLE_TO_PARTY;
using openswd3::world_map::OP_66_UPDATE_ROLE_MAP_STATE;
using openswd3::world_map::OP_67_WAIT_FRAME_CLOCK;
using openswd3::world_map::OP_68_CLEAR_ROLE_FLAG_0400;
using openswd3::world_map::OP_69_SET_ROLE_FLAG_0400;
using openswd3::world_map::OP_70_START_ABSOLUTE_CAMERA_MOVE;
using openswd3::world_map::OP_71_SET_ROLE_HEAD_SIGN;
using openswd3::world_map::OP_72_CLEAR_ROLE_HEAD_SIGN;
using openswd3::world_map::OP_73_START_CAMERA_MOVE_TO_ROLE;
using openswd3::world_map::OP_74_CANCEL_FRAME_COLOR_TRANSITION;
using openswd3::world_map::OP_75_SUSPEND_STORY_ROLE;
using openswd3::world_map::OP_76_TURN_AND_SUSPEND_STORY_ROLE;
using openswd3::world_map::OP_77_SET_ROLE_WAIT_OVERRIDE;
using openswd3::world_map::OP_78_CLEAR_ROLE_WAIT_OVERRIDE;
using openswd3::world_map::OP_79_ENQUEUE_MOVING_ACTION;
using openswd3::world_map::OP_80_CLEAR_TEXT_CONTROL_BIT29;
using openswd3::world_map::OP_81_ENQUEUE_ROLE_HEAD_ACTION;
using openswd3::world_map::OP_82_DISMISS_ROLE_HEAD_ACTION;
using openswd3::world_map::OP_83_UPSERT_PACKED_ROW_EFFECT;
using openswd3::world_map::OP_84_CONTROL_PACKED_ROW_EFFECT;
using openswd3::world_map::OP_85_BEGIN_STORY_VIDEO;
using openswd3::world_map::OP_86_REWRITE_ROLE_HEAD_ACTION_KEY;
using openswd3::world_map::OP_87_RELOAD_RANDOM_TARGET;
using openswd3::world_map::OP_88_REQUEST_BATTLE;
using openswd3::world_map::OP_91_LOAD_NAME_RECORD;
using openswd3::world_map::OP_92_SET_RESERVED_GLOBAL_BIT;
using openswd3::world_map::OP_93_CLEAR_RESERVED_GLOBAL_BIT;
using openswd3::world_map::OP_94_SET_SCENE_RENDER_BIT1;
using openswd3::world_map::OP_95_CLEAR_SCENE_RENDER_BIT1;
using openswd3::world_map::OP_96_BEGIN_CUSTOM_ANI;
using openswd3::world_map::OP_97_WAIT_CUSTOM_ANI_COMPLETE;
using openswd3::world_map::OP_98_CONSUME_FOUR_BYTE_NOOP;
using openswd3::world_map::OP_99_WAIT_CUSTOM_ANI_PHASE;
using openswd3::world_map::OP_100_SET_ROLE_TALK_SCRIPT;
using openswd3::world_map::OP_101_SET_ROLE_STATUS_BIT26;
using openswd3::world_map::OP_102_SET_ROLE_STATUS_BIT6;
using openswd3::world_map::OP_103_SET_ROLE_STATUS_BIT5;
using openswd3::world_map::OP_104_SET_TEXT_LAYOUT_PAIR;
using openswd3::world_map::OP_105_CLEAR_TEXT_CONTROL_BIT27;
using openswd3::world_map::OP_106_WAIT_PRIMARY_PICTURE_ACTION_BYTE;
using openswd3::world_map::OP_107_WAIT_ROLE_ACTION_INDEX;
using openswd3::world_map::OP_108_SET_NEXT_DIALOG_ANCHOR;
using openswd3::world_map::OP_109_STEP_ROLES;
using openswd3::world_map::OP_110_RELOAD_IF_NO_SECONDARY_ROLE_BIT30;
using openswd3::world_map::OP_111_RELOAD_IF_ANY_SECONDARY_ROLE_BIT30;
using openswd3::world_map::OP_112_WAIT_PACKED_ROW_AND_ROLE_HEAD_ACTIONS;
using openswd3::world_map::OP_113_PLAY_SOUND_EFFECT_WITH_UNREAD_PADDING;
using openswd3::world_map::OP_114_STAGE_SCENE_MUSIC_STREAM_REQUEST;
using openswd3::world_map::OP_115_SET_MUSIC_STREAM_VOLUME;
using openswd3::world_map::OP_116_BATCH_SET_ROLE_POSITIONS;
using openswd3::world_map::OP_117_SET_ROLE_STATUS_BIT4;
using openswd3::world_map::OP_118_REMOVE_DIALOGS_FOR_ROLE_GUID;
using openswd3::world_map::OP_119_WAIT_DIALOG_FLAG_BIT0;
using openswd3::world_map::OP_120_UPDATE_ROLE_ACTION_FIELDS;
using openswd3::world_map::OP_121_CLEAR_TEXT_CONTROL_BIT26;
using openswd3::world_map::OP_122_CLEAR_SPEED_MODE;
using openswd3::world_map::OP_123_UPDATE_SCENE_MUSIC_TABLE_ENTRY;
using openswd3::world_map::OP_124_CLEAR_TEXT_CONTROL_BIT25;
using openswd3::world_map::OP_125_APPEND_TEXT_ALLOCATION;
using openswd3::world_map::OP_126_RELOAD_IF_ROLE_BASE_VARIANT_EQUAL;
using openswd3::world_map::OP_127_RELOAD_IF_ROLE_BASE_VARIANT_NOT_EQUAL;
using openswd3::world_map::OP_128_ADJUST_PLAYER_ITEM_QUANTITY;
using openswd3::world_map::OP_129_RELOAD_IF_ANY_ITEM_OWNER_HAS_ITEM;
using openswd3::world_map::OP_130_RELOAD_IF_NO_ITEM_OWNER_HAS_ITEM;
using openswd3::world_map::OP_131_ADD_PARTY_ITEM_IF_ALLOWED;
using openswd3::world_map::OP_132_SWAP_PLAYER_ITEM_INTO_ROLE_SLOT;
using openswd3::world_map::OP_133_REQUEST_SHOP;
using openswd3::world_map::OP_134_ADJUST_PARTY_MEMBER_RESOURCES;
using openswd3::world_map::OP_135_RESET_INPUT_MENU_STATE;
using openswd3::world_map::OP_136_SET_ROLE_STATUS_BIT12;
using openswd3::world_map::OP_137_STOP_SCENE_MUSIC_STREAM;
using openswd3::world_map::OP_138_RELOAD_IF_ROLE_OUTSIDE_RADIUS;
using openswd3::world_map::OP_139_WAIT_DIALOG_FLAG_BIT15;
using openswd3::world_map::OP_140_SET_ROLE_STATUS_BIT11;
using openswd3::world_map::OP_141_CONFIGURE_MUSIC_STREAM_TRANSITION;
using openswd3::world_map::OP_142_INITIALIZE_PRIMARY_COUNTDOWN;
using openswd3::world_map::OP_143_DISABLE_PRIMARY_COUNTDOWN;
using openswd3::world_map::OP_144_REQUEST_SPECIAL_MODE_4_OR_5;
using openswd3::world_map::OP_145_SET_ROLE_STATUS_BIT13;
using openswd3::world_map::OP_146_SET_ROLE_STATUS_BIT8;
using openswd3::world_map::OP_147_SET_STORY_FLAG_70;
using openswd3::world_map::OP_148_SET_STORY_FLAG_19;
using openswd3::world_map::OP_149_CLEAR_STORY_FLAG_19;
using openswd3::world_map::OP_150_CONFIGURE_ANI_FOLLOWER_POSITION;
using openswd3::world_map::OP_151_CONFIGURE_ANI_FOLLOWER_TARGET;
using openswd3::world_map::OP_152_WAIT_ANI_FOLLOWER_TARGET;
using openswd3::world_map::OP_155_RELOAD_CURRENT_WORLD_SESSION;
using openswd3::world_map::OP_156_RELOAD_DEFERRED_WORLD_SESSION;
using openswd3::world_map::OP_157_CONFIGURE_DEFERRED_WORLD_SESSION;
using openswd3::world_map::OP_158_COPY_STORY_FILE;
using openswd3::world_map::OP_159_DELETE_STORY_FILE;
using openswd3::world_map::OP_160_SUPPRESS_NEXT_DIALOG_FLAG18;
using openswd3::world_map::OP_161_TRANSFER_STORY;
using openswd3::world_map::OP_162_LOAD_DYNAMIC_NAME_RECORD;
using openswd3::world_map::OP_163_RELOAD_IF_CURRENT_MAP_NOT_EQUAL;
using openswd3::world_map::OP_164_RELOAD_IF_CURRENT_MAP_EQUAL;
using openswd3::world_map::OP_165_RELOAD_IF_ITEM_TOTAL_AT_LEAST;
using openswd3::world_map::OP_166_RELOAD_IF_ITEM_TOTAL_AT_MOST;
using openswd3::world_map::OP_167_RELOAD_IF_ANY_ROLE_ITEM_ROOT_HAS_ITEM;
using openswd3::world_map::OP_168_RELOAD_IF_NO_ROLE_ITEM_ROOT_HAS_ITEM;
using openswd3::world_map::OP_174_SET_ROLE_STATUS_BIT14;
using openswd3::world_map::OP_1024_LATCH_COMMON_JOIN_SAME_CALL;
using openswd3::world_map::OP_1025_CLEAR_COMMON_JOIN_LATCH_AND_YIELD;
using openswd3::world_map::OP_1026_CONTINUE_COMMON_JOIN_SAME_CALL;
using openswd3::world_map::OP_16383_FINISH_TALK;
using openswd3::world_map::OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION;
using openswd3::world_map::OP_154_WAIT_SECONDARY_PICTURE_ACTION_BYTE;
using openswd3::world_map::OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS;
using openswd3::world_map::OP_170_CLEAR_MODE17_TEXT;
using openswd3::world_map::OP_171_SET_MODE17_TEXT;
using openswd3::world_map::OP_172_CLEAR_MODE18_TEXT;
using openswd3::world_map::OP_173_SET_MODE18_TEXT;
using openswd3::world_map::OP_175_SUSPEND_STORY_ANI;
using openswd3::world_map::OP_176_RESUME_STORY_ANI;
using openswd3::world_map::OP_177_GATHER_PARTY_AT_PLAYER;
using openswd3::world_map::OP_178_SET_ROLE_COLLISION_BYPASS;
using openswd3::world_map::OP_179_ENQUEUE_FRAME_DEFORMATION;
using openswd3::world_map::OP_180_CLEAR_FRAME_EXECUTION_GATE;
using openswd3::world_map::OP_181_SET_GLOBAL_INTEGER_WIDE;
using openswd3::world_map::OP_182_ADD_GLOBAL_INTEGER_WIDE;
using openswd3::world_map::OP_183_SUBTRACT_GLOBAL_INTEGER_WIDE_CLAMP_ZERO;
using openswd3::world_map::OP_184_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_GE;
using openswd3::world_map::OP_185_JUMP_IF_GLOBAL_INTEGER_WIDE_UNSIGNED_LE;
using openswd3::world_map::OP_186_RELOAD_IF_PARTY_MEMBER_FIELD_GE;
using openswd3::world_map::OP_187_RELOAD_IF_PARTY_MEMBER_FIELD_LE;
using openswd3::world_map::OP_188_SET_PARTY_MEMBER_FIELD;
using openswd3::world_map::OP_189_ADD_PARTY_MEMBER_FIELD;
using openswd3::world_map::OP_190_SUBTRACT_PARTY_MEMBER_FIELD;
using openswd3::world_map::OP_191_WAIT_CAMERA_TOP_WHILE_MOVING;
using openswd3::world_map::OP_192_WAIT_MUSIC_STREAM_TRANSITION;
using openswd3::world_map::OP_193_WAIT_STORY_VIDEO;

[[maybe_unused]] void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

struct StoryVmTypedStopOpcode {
    [[nodiscard]] constexpr operator u16() const noexcept {
        return OP_29_SET_GLOBAL_INTEGER;
    }
};

constexpr StoryVmTypedStopOpcode kStoryVmTypedStop{};

[[maybe_unused]] void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const StoryVmTypedStopOpcode
) noexcept {
    write_u16(bytes, offset, static_cast<u16>(kStoryVmTypedStop));
    if (offset + 6U > bytes.size()) {
        return;
    }

    write_u16(bytes, offset + 2U, 0xFFFFU);
    write_u16(bytes, offset + 4U, 0U);
}

struct StoryVmLookaheadTypedStopOpcode {
    [[nodiscard]] constexpr operator u16() const noexcept {
        return OP_179_ENQUEUE_FRAME_DEFORMATION;
    }
};

constexpr StoryVmLookaheadTypedStopOpcode kStoryVmLookaheadTypedStop{};

[[maybe_unused]] void write_u16(
    const std::span<u8> bytes,
    const std::size_t offset,
    const StoryVmLookaheadTypedStopOpcode
) noexcept {
    write_u16(bytes, offset, static_cast<u16>(kStoryVmLookaheadTypedStop));
    if (offset + 10U > bytes.size()) {
        return;
    }

    write_u16(bytes, offset + 4U, 0U);
    write_u16(bytes, offset + 6U, 0U);
    write_u16(bytes, offset + 8U, 0U);
}

[[nodiscard, maybe_unused]] u16
read_u16(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard, maybe_unused]] u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

[[maybe_unused]] void write_u32(
    const std::span<u8> bytes, const std::size_t offset, const u32 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
    bytes[offset + 2U] = static_cast<u8>(value >> 16U);
    bytes[offset + 3U] = static_cast<u8>(value >> 24U);
}

class StoryTestTree final {
public:
    StoryTestTree() {
        const auto unique =
            std::chrono::steady_clock::now().time_since_epoch().count();
        root_ = std::filesystem::path{OPENSWD3_TEST_ARTIFACT_ROOT} /
            ("legacy-world-story-" + std::to_string(unique));
        std::filesystem::create_directories(root_);
    }

    ~StoryTestTree() {
        std::error_code ignored;
        std::filesystem::remove_all(root_, ignored);
    }

    [[nodiscard]] const std::filesystem::path& root() const noexcept {
        return root_;
    }

private:
    std::filesystem::path root_;
};

[[nodiscard, maybe_unused]] openswd3::rendering::LegacyPixelConversionState
rgb565_conversion() {
    openswd3::rendering::LegacyPixelConversionState conversion;
    openswd3::rendering::select_legacy_pixel_conversion(
        conversion,
        openswd3::rendering::LegacyPixelMasks{
            .red = 0xF800U,
            .green = 0x07E0U,
            .blue = 0x001FU,
        }
    );
    return conversion;
}

class RecordingPorts final : public LegacyWorldStoryVmPorts {
public:
    LegacyTalkWindowLoadResult load_story_window(
        const i32 story_id,
        const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
            destination,
        const bool clear_before_read
    ) override {
        ++story_load_count;
        last_story_id = story_id;
        last_story_clear_before_read = clear_before_read;
        if (story_load_callback) {
            story_load_callback();
        }
        if (story_load_status != LegacyTalkWindowStatus::ready) {
            return LegacyTalkWindowLoadResult{.status = story_load_status};
        }
        copy_window(
            story_id == 2042 ? transferred_window : initial_window,
            destination,
            clear_before_read
        );
        return LegacyTalkWindowLoadResult{
            .status = LegacyTalkWindowStatus::ready,
            .file_number = story_id == 2042 ? 2U : 1U,
            .entry_index = story_id == 2042 ? 42U : 248U,
            .data_offset = story_id == 2042 ? 0x2222U : 0x1111U,
            .actual_size = static_cast<u32>(
                story_id == 2042 ? transferred_window.size()
                                 : initial_window.size()
            ),
        };
    }

    LegacyTalkWindowLoadResult load_data_window(
        const u32 file_number,
        const u32 data_offset,
        const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
            destination,
        const bool clear_before_read
    ) override {
        ++data_load_count;
        last_data_file_number = file_number;
        last_data_offset = data_offset;
        last_data_clear_before_read = clear_before_read;
        story_protocol_events.push_back(5U);
        if (data_load_status == LegacyTalkWindowStatus::ready) {
            copy_window(transferred_window, destination, clear_before_read);
        }
        return LegacyTalkWindowLoadResult{
            .status = data_load_status,
            .file_number = file_number,
            .data_offset = data_offset,
            .actual_size = data_load_status == LegacyTalkWindowStatus::ready
                ? static_cast<u32>(transferred_window.size())
                : 0U,
        };
    }

    u32 update_action(
        openswd3::asset_runtime::LegacyActionRecord& action
    ) override {
        ++action_update_count;
        story_protocol_events.push_back(4U);
        action.field_4a = static_cast<u16>(action.action_id);
        if (action_update_callback) {
            action_update_callback(action, action_update_count);
        }
        return action_update_result;
    }

    void release_role_path_payload(const u32 role_index) noexcept override {
        ++role_path_payload_release_count;
        released_role_path_index = role_index;
    }

    void begin_world_session_reload() noexcept override {
        ++world_session_reload_begin_count;
        story_protocol_events.push_back(6U);
    }

    bool reload_world_session(
        const openswd3::world_map::LegacyWorldLoadRequest& request,
        std::span<openswd3::world_map::LegacyWorldRoleRecord>& roles,
        u32& controlled_role_index,
        openswd3::world_map::LegacyWorldStoryVmRuntime& runtime
    ) override {
        ++world_session_reload_count;
        last_world_load_request = request;
        story_protocol_events.push_back(7U);
        if (world_session_reload_callback) {
            world_session_reload_callback();
        }
        if (!world_session_reload_success) {
            return false;
        }
        if (!replacement_roles.empty()) {
            roles = replacement_roles;
            controlled_role_index = replacement_controlled_role_index;
            runtime.role_surface.selected_guid = replacement_selected_guid;
        }
        return true;
    }

    void patch_role_source(
        const openswd3::world_map::LegacyMapsRolePatchRequest& request
    ) noexcept override {
        role_patch_requests.push_back(request);
    }

    [[nodiscard]] bool load_story_item_definition(
        const u16 item_id,
        const std::
            span<u8, openswd3::world_map::kLegacyItemDefinitionSnapshotBytes>
                definition_snapshot,
        std::vector<u8>& description
    ) override {
        ++item_definition_load_count;
        last_item_definition_id = item_id;
        story_protocol_events.push_back(13U);
        if (!item_definition_load_success) {
            return false;
        }
        std::ranges::copy(
            prepared_item_definition, definition_snapshot.begin()
        );
        description = prepared_item_description;
        return true;
    }

    void play_sound_effect(const u16 sound_id) noexcept override {
        sound_effect_requests.push_back(sound_id);
    }

    void apply_music_stream_transition(
        u32& transition_mode,
        u32& current_fade_divisor,
        const u32 pending_fade_divisor
    ) noexcept override {
        ++music_transition_apply_count;
        last_music_transition_mode = transition_mode;
        last_music_current_fade_divisor = current_fade_divisor;
        last_music_pending_fade_divisor = pending_fade_divisor;
        story_protocol_events.push_back(11U);
        if (music_transition_callback) {
            music_transition_callback();
        }
        if (transition_mode == 1U) {
            transition_mode = 0U;
            current_fade_divisor = 0U;
        } else if (transition_mode == 2U) {
            current_fade_divisor = pending_fade_divisor;
        }
    }

    void set_music_stream_volume(const u32 level) noexcept override {
        ++music_volume_write_count;
        last_music_volume_level = level;
        story_protocol_events.push_back(12U);
        if (music_volume_callback) {
            music_volume_callback();
        }
    }

    void clear_story_framebuffer() noexcept override {
        ++framebuffer_clear_count;
        if (framebuffer_clear_callback) {
            framebuffer_clear_callback();
        }
    }

    void present_story_framebuffer() noexcept override {
        ++framebuffer_present_count;
        if (framebuffer_present_callback) {
            framebuffer_present_callback();
        }
    }

    [[nodiscard]] bool prepare_story_video() noexcept override {
        ++video_prepare_count;
        if (story_video_prepare_callback) {
            story_video_prepare_callback();
        }
        return video_prepare_success;
    }

    void begin_story_video(const std::span<const u8> filename) override {
        ++video_begin_count;
        last_video_filename.assign(filename.begin(), filename.end());
        if (story_video_begin_callback) {
            story_video_begin_callback();
        }
    }

    i32 query_story_video_progress() override {
        ++video_progress_query_count;
        if (video_progress_callback) {
            video_progress_callback();
        }

        return video_progress;
    }

    void set_story_frame_interval(const u32 milliseconds) noexcept override {
        ++ani_frame_interval_write_count;
        last_ani_frame_interval = milliseconds;
        story_protocol_events.push_back(8U);
    }

    [[nodiscard]] bool prepare_story_ani() noexcept override {
        ++ani_prepare_count;
        story_protocol_events.push_back(9U);
        return ani_prepare_success;
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyAniActivityStartResult
    begin_story_ani(
        const std::span<const u8> filename, const u8 flags
    ) override {
        ++ani_begin_count;
        last_ani_filename.assign(filename.begin(), filename.end());
        last_ani_flags = flags;
        story_protocol_events.push_back(10U);
        return ani_start_result;
    }

    [[nodiscard]] bool is_story_ani_active() const noexcept override {
        ++ani_active_query_count;
        return ani_active;
    }

    [[nodiscard]] i32 query_story_ani_phase() const noexcept override {
        ++ani_phase_query_count;
        return ani_phase;
    }

    void set_story_ani_suspended(const bool suspended) noexcept override {
        ++ani_suspend_write_count;
        if (suspended) {
            ani_control_flags |= openswd3::asset_runtime::kLegacyAniSuspendFlag;
        } else {
            ani_control_flags &= ~static_cast<u32>(
                openswd3::asset_runtime::kLegacyAniSuspendFlag
            );
        }
        if (story_ani_suspend_callback) {
            story_ani_suspend_callback();
        }
    }

    void suspend_story_host_frame_execution() noexcept override {
        ++story_host_frame_suspend_count;
        if (story_host_frame_suspend_callback) {
            story_host_frame_suspend_callback();
        }
    }

    [[nodiscard]] bool perform_story_file_operation(
        const LegacyWorldStoryFileOperation operation,
        const LegacyWorldStoryFileDirectory directory,
        const std::span<const u8> filename
    ) override {
        ++story_file_operation_count;
        last_story_file_operation = operation;
        last_story_file_directory = directory;
        last_story_file_name.assign(filename.begin(), filename.end());
        if (story_file_operation_callback) {
            story_file_operation_callback();
        }
        return story_file_operation_success;
    }

    [[nodiscard]] bool reset_input_menu_and_save_previews() override {
        ++input_menu_reset_count;
        story_protocol_events.push_back(14U);
        if (input_menu_reset_callback) {
            input_menu_reset_callback();
        }
        return input_menu_reset_success;
    }

    [[nodiscard]] bool load_party_member_level_field(
        const u32 group, const u32 level, u32& output
    ) override {
        ++party_member_level_load_count;
        last_party_member_level_group = group;
        last_party_member_level = level;
        if (!party_member_level_load_success) {
            return false;
        }

        output = party_member_level_output;
        return true;
    }

    void beep() noexcept override {
        ++beep_count;
        default_protocol_events.push_back(1U);
        story_protocol_events.push_back(1U);
    }

    void service_audio() override {
        ++direct_audio_service_count;
        default_protocol_events.push_back(2U);
        story_protocol_events.push_back(2U);
        if (audio_service_callback) {
            audio_service_callback();
        }
    }

    bool prepare_dialog_text(
        const std::span<const u8> source, std::vector<u8>& destination
    ) override {
        ++dialog_text_prepare_count;
        story_protocol_events.push_back(3U);
        if (throw_on_dialog_text_prepare) {
            throw std::bad_alloc{};
        }
        last_dialog_text.assign(source.begin(), source.end());
        if (!dialog_text_prepare_success) {
            return false;
        }
        destination = prepared_dialog_text;
        return true;
    }

    std::array<u8, 256U> initial_window{};
    std::array<u8, 256U> transferred_window{};
    u32 story_load_count{};
    u32 data_load_count{};
    u32 last_data_file_number{};
    u32 last_data_offset{};
    u32 action_update_count{};
    u32 action_update_result{1U};
    std::function<void(openswd3::asset_runtime::LegacyActionRecord&, u32)>
        action_update_callback;
    std::function<void()> story_load_callback;
    std::function<void()> framebuffer_clear_callback;
    std::function<void()> framebuffer_present_callback;
    std::function<void()> story_video_prepare_callback;
    std::function<void()> story_video_begin_callback;
    std::function<void()> audio_service_callback;
    std::function<void()> world_session_reload_callback;
    std::function<void()> music_transition_callback;
    std::function<void()> music_volume_callback;
    std::function<void()> input_menu_reset_callback;
    std::function<void()> video_progress_callback;
    std::function<void()> story_ani_suspend_callback;
    std::function<void()> story_host_frame_suspend_callback;
    std::function<void()> story_file_operation_callback;
    u32 framebuffer_clear_count{};
    u32 framebuffer_present_count{};
    u32 video_prepare_count{};
    u32 video_begin_count{};
    u32 video_progress_query_count{};
    u32 ani_frame_interval_write_count{};
    u32 ani_prepare_count{};
    u32 ani_begin_count{};
    mutable u32 ani_active_query_count{};
    mutable u32 ani_phase_query_count{};
    u32 ani_suspend_write_count{};
    u32 ani_control_flags{};
    u32 last_ani_frame_interval{};
    u32 beep_count{};
    u32 direct_audio_service_count{};
    u32 music_transition_apply_count{};
    u32 last_music_transition_mode{};
    u32 last_music_current_fade_divisor{};
    u32 last_music_pending_fade_divisor{};
    u32 music_volume_write_count{};
    u32 last_music_volume_level{};
    u32 dialog_text_prepare_count{};
    u32 item_definition_load_count{};
    u32 input_menu_reset_count{};
    u32 party_member_level_load_count{};
    u32 last_party_member_level_group{};
    u32 last_party_member_level{};
    u32 party_member_level_output{};
    u32 role_path_payload_release_count{};
    u32 released_role_path_index{0xFFFFFFFFU};
    u32 world_session_reload_begin_count{};
    u32 world_session_reload_count{};
    u32 story_host_frame_suspend_count{};
    u32 story_file_operation_count{};
    bool last_story_clear_before_read{};
    bool last_data_clear_before_read{};
    bool dialog_text_prepare_success{};
    bool item_definition_load_success{true};
    bool world_session_reload_success{true};
    bool input_menu_reset_success{true};
    bool party_member_level_load_success{};
    bool story_file_operation_success{true};
    bool video_prepare_success{true};
    bool ani_prepare_success{true};
    bool ani_active{};
    bool throw_on_dialog_text_prepare{};
    LegacyTalkWindowStatus story_load_status{LegacyTalkWindowStatus::ready};
    LegacyTalkWindowStatus data_load_status{LegacyTalkWindowStatus::ready};
    i32 last_story_id{};
    i32 video_progress{-1};
    i32 ani_phase{};
    u8 last_ani_flags{};
    u16 last_item_definition_id{};
    std::array<u8, openswd3::world_map::kLegacyItemDefinitionSnapshotBytes>
        prepared_item_definition{};
    std::vector<u8> prepared_item_description;
    std::vector<u8> last_video_filename;
    std::vector<u8> last_ani_filename;
    std::vector<u8> last_story_file_name;
    LegacyWorldStoryFileOperation last_story_file_operation{
        LegacyWorldStoryFileOperation::copy
    };
    LegacyWorldStoryFileDirectory last_story_file_directory{
        LegacyWorldStoryFileDirectory::root
    };
    openswd3::asset_runtime::LegacyAniActivityStartResult ani_start_result{
        .status = openswd3::asset_runtime::LegacyAniActivityStartStatus::ready,
        .open_status = openswd3::asset_runtime::LegacyAniOpenStatus::ready,
        .frame_status =
            openswd3::asset_runtime::LegacyAniFrameLoadStatus::ready,
    };
    std::vector<openswd3::world_map::LegacyMapsRolePatchRequest>
        role_patch_requests;
    std::vector<u16> sound_effect_requests;
    std::vector<u32> default_protocol_events;
    std::vector<u32> story_protocol_events;
    std::vector<u8> last_dialog_text;
    std::vector<u8> prepared_dialog_text;
    openswd3::world_map::LegacyWorldLoadRequest last_world_load_request{};
    std::span<openswd3::world_map::LegacyWorldRoleRecord> replacement_roles{};
    u32 replacement_controlled_role_index{};
    u32 replacement_selected_guid{};

private:
    template <std::size_t Size>
    static void copy_window(
        const std::array<u8, Size>& source,
        const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
            destination,
        const bool clear_before_read
    ) noexcept {
        if (clear_before_read) {
            std::ranges::fill(destination, u8{0x0CU});
        }
        std::ranges::copy(source, destination.begin());
    }
};

class RealPorts final : public LegacyWorldStoryVmPorts {
public:
    explicit RealPorts(
        openswd3::resource_io::LegacyResourceDatabases& databases
    ) noexcept
        : databases_(databases) {}

    LegacyTalkWindowLoadResult load_story_window(
        const i32 story_id,
        const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
            destination,
        const bool clear_before_read
    ) override {
        return databases_.load_talk_story_window(
            story_id, destination, clear_before_read
        );
    }

    LegacyTalkWindowLoadResult load_data_window(
        const u32 file_number,
        const u32 data_offset,
        const std::span<u8, openswd3::resource_io::kLegacyTalkWindowSize>
            destination,
        const bool clear_before_read
    ) override {
        return databases_.load_talk_data_window(
            file_number, data_offset, destination, clear_before_read
        );
    }

    u32 update_action(openswd3::asset_runtime::LegacyActionRecord&) override {
        ++action_update_count;
        return 1U;
    }

    void release_role_path_payload(const u32 role_index) noexcept override {
        ++role_path_payload_release_count;
        released_role_path_index = role_index;
    }

    void begin_world_session_reload() noexcept override {
        ++world_session_reload_begin_count;
    }

    bool reload_world_session(
        const openswd3::world_map::LegacyWorldLoadRequest& request,
        std::span<openswd3::world_map::LegacyWorldRoleRecord>&,
        u32&,
        openswd3::world_map::LegacyWorldStoryVmRuntime&
    ) override {
        ++world_session_reload_count;
        last_world_load_request = request;
        return true;
    }

    void patch_role_source(
        const openswd3::world_map::LegacyMapsRolePatchRequest& request
    ) noexcept override {
        role_patch_requests.push_back(request);
    }

    [[nodiscard]] bool load_story_item_definition(
        const u16 item_id,
        const std::
            span<u8, openswd3::world_map::kLegacyItemDefinitionSnapshotBytes>
                definition_snapshot,
        std::vector<u8>& description
    ) override {
        definition_snapshot[0U] = static_cast<u8>(item_id);
        definition_snapshot[1U] = static_cast<u8>(item_id >> 8U);
        description = {static_cast<u8>('I'), 0U};
        return true;
    }

    void play_sound_effect(const u16 sound_id) noexcept override {
        sound_effect_requests.push_back(sound_id);
    }

    void apply_music_stream_transition(
        u32& transition_mode,
        u32& current_fade_divisor,
        const u32 pending_fade_divisor
    ) noexcept override {
        if (transition_mode == 1U) {
            transition_mode = 0U;
            current_fade_divisor = 0U;
        } else if (transition_mode == 2U) {
            current_fade_divisor = pending_fade_divisor;
        }
    }

    void set_music_stream_volume(const u32) noexcept override {}

    void clear_story_framebuffer() noexcept override {
        ++framebuffer_clear_count;
    }

    void present_story_framebuffer() noexcept override {
        ++framebuffer_present_count;
    }

    [[nodiscard]] bool prepare_story_video() noexcept override {
        ++video_prepare_count;
        return true;
    }

    void begin_story_video(const std::span<const u8> filename) override {
        ++video_begin_count;
        last_video_filename.assign(filename.begin(), filename.end());
    }

    i32 query_story_video_progress() override {
        ++video_progress_query_count;
        return -1;
    }

    void set_story_frame_interval(const u32 milliseconds) noexcept override {
        last_ani_frame_interval = milliseconds;
    }

    [[nodiscard]] bool prepare_story_ani() noexcept override {
        return true;
    }

    [[nodiscard]] openswd3::asset_runtime::LegacyAniActivityStartResult
    begin_story_ani(
        const std::span<const u8> filename, const u8 flags
    ) override {
        last_ani_filename.assign(filename.begin(), filename.end());
        last_ani_flags = flags;
        return openswd3::asset_runtime::LegacyAniActivityStartResult{
            .status =
                openswd3::asset_runtime::LegacyAniActivityStartStatus::ready,
            .open_status = openswd3::asset_runtime::LegacyAniOpenStatus::ready,
            .frame_status =
                openswd3::asset_runtime::LegacyAniFrameLoadStatus::ready,
        };
    }

    [[nodiscard]] bool is_story_ani_active() const noexcept override {
        return false;
    }

    [[nodiscard]] i32 query_story_ani_phase() const noexcept override {
        return 0;
    }

    void set_story_ani_suspended(const bool suspended) noexcept override {
        if (suspended) {
            ani_control_flags |= openswd3::asset_runtime::kLegacyAniSuspendFlag;
        } else {
            ani_control_flags &= ~static_cast<u32>(
                openswd3::asset_runtime::kLegacyAniSuspendFlag
            );
        }
    }

    void suspend_story_host_frame_execution() noexcept override {}

    [[nodiscard]] bool perform_story_file_operation(
        const LegacyWorldStoryFileOperation,
        const LegacyWorldStoryFileDirectory,
        const std::span<const u8>
    ) override {
        return true;
    }

    [[nodiscard]] bool reset_input_menu_and_save_previews() override {
        return true;
    }

    [[nodiscard]] bool
    load_party_member_level_field(const u32, const u32, u32&) override {
        return false;
    }

    void beep() noexcept override {}
    void service_audio() override {}
    bool
    prepare_dialog_text(const std::span<const u8>, std::vector<u8>&) override {
        return false;
    }

    u32 action_update_count{};
    u32 framebuffer_clear_count{};
    u32 framebuffer_present_count{};
    u32 video_prepare_count{};
    u32 video_begin_count{};
    u32 video_progress_query_count{};
    u32 last_ani_frame_interval{};
    u32 ani_control_flags{};
    u8 last_ani_flags{};
    u32 role_path_payload_release_count{};
    u32 released_role_path_index{0xFFFFFFFFU};
    u32 world_session_reload_begin_count{};
    u32 world_session_reload_count{};
    openswd3::world_map::LegacyWorldLoadRequest last_world_load_request{};
    std::vector<u8> last_video_filename;
    std::vector<u8> last_ani_filename;
    std::vector<openswd3::world_map::LegacyMapsRolePatchRequest>
        role_patch_requests;
    std::vector<u16> sound_effect_requests;

private:
    openswd3::resource_io::LegacyResourceDatabases& databases_;
};

class StoryFrameActionPorts final
    : public openswd3::asset_runtime::LegacyActionDrawPorts {
public:
    openswd3::asset_runtime::LegacyActionUpdateStatus update_action_record(
        openswd3::asset_runtime::LegacyActionRecord&
    ) override {
        return openswd3::asset_runtime::LegacyActionUpdateStatus::completed;
    }

    bool load_frame_piece(
        u16, u16, openswd3::rendering::LegacyFramePiece&
    ) override {
        return false;
    }

    openswd3::rendering::LegacyBlitExecutionStatus draw_frame_piece(
        const openswd3::rendering::LegacyFramePiece&, i32, i32, u32, i32
    ) noexcept override {
        return openswd3::rendering::LegacyBlitExecutionStatus::completed;
    }
};

class StoryPathCompletionPorts final
    : public openswd3::world_map::LegacyWorldMapRolePathPorts {
public:
    explicit StoryPathCompletionPorts(
        openswd3::world_map::LegacyWorldStoryPathRuntime& runtime
    ) noexcept
        : runtime_(runtime) {}

    bool complete_role_path(const u32 role_index) noexcept override {
        const auto result =
            openswd3::world_map::complete_legacy_world_story_path(
                runtime_, role_index
            );
        return result.status ==
            openswd3::world_map::LegacyWorldStoryPathStatus::completed;
    }

private:
    openswd3::world_map::LegacyWorldStoryPathRuntime& runtime_;
};

struct FixtureStorage {
    LegacyWorldTalkContext context{};
    LegacyWorldStoryVmState state{};
    std::vector<LegacyWorldRoleRecord> roles =
        std::vector<LegacyWorldRoleRecord>(3U);
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    std::array<u8, 0x100U> maps_payload{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs{};
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources{};
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    openswd3::world_map::LegacyWorldCameraRect camera{};
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::asset_runtime::LegacyAniFollowerState ani_follower{};
    std::array<i16, openswd3::world_map::kLegacyWorldSelectionWordCount>
        selection_words{};
    openswd3::world_map::LegacyWorldSelectionScrollState selection_scroll{};
    openswd3::world_map::LegacyWorldRoleTransferState role_transfer_state{};
    u32 live_party_role_count{1U};
    openswd3::world_map::LegacyWorldPlayerPostFrameState player_post_frame{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldPartySlotCount>
        live_party_object_slots{};
    u32 indexed_target_selector{};
    openswd3::input_time_rng::LegacySecondaryRng secondary_rng{};
    openswd3::input_time_rng::LegacyCrtRng crt_rng{};
    openswd3::asset_runtime::LegacyDeformationList frame_deformations;
    u32 frame_execution_gate{1U};
    u32 speed_mode{};
    u32 special_mode_state{};
    u32 special_input_mode{};
    u32 high_priority_state{};
    u32 high_priority_submode{};
    u32 high_priority_auxiliary{};
    LegacyWorldItemListState item_lists;
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{};
    RecordingPorts ports{};
};

struct Fixture {
    std::unique_ptr<FixtureStorage> storage =
        std::make_unique<FixtureStorage>();
    LegacyWorldTalkContext& context = storage->context;
    LegacyWorldStoryVmState& state = storage->state;
    std::vector<LegacyWorldRoleRecord>& roles = storage->roles;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>&
        active_object_slots = storage->active_object_slots;
    std::array<u8, 0x100U>& maps_payload = storage->maps_payload;
    openswd3::story_scene::LegacyDialogRuntimeState& dialogs = storage->dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState& dialog_resources =
        storage->dialog_resources;
    std::array<u8, 16U>& first_name = storage->first_name;
    std::array<u8, 16U>& second_name = storage->second_name;
    openswd3::world_map::LegacyWorldCameraRect& camera = storage->camera;
    openswd3::world_map::LegacyWorldMovementRuntimeState& movement =
        storage->movement;
    openswd3::asset_runtime::LegacyAniFollowerState& ani_follower =
        storage->ani_follower;
    std::array<i16, openswd3::world_map::kLegacyWorldSelectionWordCount>&
        selection_words = storage->selection_words;
    openswd3::world_map::LegacyWorldSelectionScrollState& selection_scroll =
        storage->selection_scroll;
    openswd3::world_map::LegacyWorldRoleTransferState& role_transfer_state =
        storage->role_transfer_state;
    u32& live_party_role_count = storage->live_party_role_count;
    openswd3::world_map::LegacyWorldPlayerPostFrameState& player_post_frame =
        storage->player_post_frame;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldPartySlotCount>&
        live_party_object_slots = storage->live_party_object_slots;
    u32& indexed_target_selector = storage->indexed_target_selector;
    openswd3::input_time_rng::LegacySecondaryRng& secondary_rng =
        storage->secondary_rng;
    openswd3::input_time_rng::LegacyCrtRng& crt_rng = storage->crt_rng;
    openswd3::asset_runtime::LegacyDeformationList& frame_deformations =
        storage->frame_deformations;
    u32& frame_execution_gate = storage->frame_execution_gate;
    u32& speed_mode = storage->speed_mode;
    u32& special_mode_state = storage->special_mode_state;
    u32& special_input_mode = storage->special_input_mode;
    u32& high_priority_state = storage->high_priority_state;
    u32& high_priority_submode = storage->high_priority_submode;
    u32& high_priority_auxiliary = storage->high_priority_auxiliary;
    LegacyWorldItemListState& item_lists = storage->item_lists;
    std::list<LegacyWorldItemNode>& player_inventory =
        item_lists.player_inventory;
    openswd3::world_map::LegacyWorldStoryVmRuntime& runtime = storage->runtime;
    RecordingPorts& ports = storage->ports;

    Fixture() {
        openswd3::world_map::initialize_legacy_world_story_vm(state);
        context.source_guid = 0x00F8U;
        context.talk_script_id = 248U;
        roles[0].world_x = 16U;
        roles[0].world_y = 16U;
        roles[1].guid = 0x00F8U;
        roles[1].flags = 0U;
        roles[1].world_x = 320U;
        roles[1].world_y = 240U;
        roles[1].action.variant_delta = 0U;
        selection_words.fill(
            std::bit_cast<i16>(
                openswd3::world_map::kLegacyWorldSelectionSentinel
            )
        );
        runtime.role_storage = &roles;
        runtime.role_transfer_state = &role_transfer_state;
        runtime.live_party_role_count = &live_party_role_count;
        runtime.player_post_frame = &player_post_frame;
        runtime.live_party_object_slots = &live_party_object_slots;
        runtime.selection_words = &selection_words;
        runtime.selection_scroll = &selection_scroll;
        runtime.camera = &camera;
        runtime.movement = &movement;
        runtime.ani_follower = &ani_follower;
        runtime.indexed_target_selector = &indexed_target_selector;
        runtime.secondary_rng = &secondary_rng;
        runtime.frame_deformations = &frame_deformations;
        runtime.crt_rng = &crt_rng;
        runtime.frame_execution_gate = &frame_execution_gate;
        runtime.speed_mode = &speed_mode;
        runtime.special_mode_state = &special_mode_state;
        runtime.special_input_mode = &special_input_mode;
        runtime.high_priority_state = &high_priority_state;
        runtime.high_priority_submode = &high_priority_submode;
        runtime.high_priority_auxiliary = &high_priority_auxiliary;
        runtime.player_inventory = &player_inventory;
        runtime.party_item_lists = &item_lists.party_item_lists;
        runtime.role_item_lists = &item_lists.role_item_lists;
        dialog_resources.frame_actions[0].action_id = 0x232DU;
        dialog_resources.caption_actions[0].action_id = 0x2337U;
    }

    [[nodiscard]] auto step(
        const i32 camera_left = 0,
        const i32 camera_top = 0,
        const u32 controlled_role_index = 0U
    ) {
        camera.left = std::bit_cast<u32>(camera_left);
        camera.top = std::bit_cast<u32>(camera_top);
        return openswd3::world_map::step_legacy_world_story_vm(
            context,
            state,
            roles,
            controlled_role_index,
            active_object_slots,
            maps_payload,
            dialogs,
            dialog_resources,
            first_name,
            second_name,
            runtime,
            ports
        );
    }
};

struct CameraMoveFixture : Fixture {
    openswd3::world_map::LegacyWorldCameraPanState camera_pan{};

    CameraMoveFixture() {
        runtime.camera_pan = &camera_pan;
        runtime.role_surface.map_width = 100U;
        runtime.map_height = 80U;
        camera.right = 640U;
        camera.bottom = 480U;
    }
};

struct StoryPathHarness {
    static constexpr u32 kMapWidth = 50U;
    static constexpr u32 kMapHeight = 40U;

    openswd3::world_map::LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface = std::vector<u8>(kMapWidth * kMapHeight * 4U, 0U);
    openswd3::world_map::LegacyWorldPathNodePool node_pool;
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    std::array<u8, 0x200U> selected_arrival_bytes{};
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldStoryPathRuntime runtime;

    explicit StoryPathHarness(
        Fixture& fixture, const u32 selected_role_index = 0U
    ) {
        for (auto& slot : fixture.active_object_slots) {
            slot.bytes.fill(0xFFU);
        }
        spatial.map_height = kMapHeight;
        const std::size_t row_count = static_cast<std::size_t>(kMapHeight) +
            2U * openswd3::world_map::kLegacySpatialRowPadding;
        for (auto& rows : spatial.row_heads) {
            rows.assign(row_count, openswd3::world_map::kLegacySpatialNoRole);
        }
        const openswd3::world_map::LegacyWorldRoleSurfaceContext role_surface{
            .map_width = kMapWidth,
            .selected_guid = fixture.roles[selected_role_index].guid,
            .surface_grid = surface,
        };
        for (u32 role_index = 0U; role_index < 2U; ++role_index) {
            auto& role = fixture.roles[role_index];
            role.action.field_2c = 1U;
            role.action.field_30 = 1U;
            role.map_cell_pointer_32 =
                (role.world_y >> 4U) * kMapWidth + (role.world_x >> 4U);
            static_cast<void>(
                openswd3::world_map::mark_legacy_world_role_surface_occupancy(
                    role, role_surface
                )
            );
            static_cast<void>(openswd3::world_map::insert_legacy_role_spatially(
                spatial, fixture.roles, role_index, role.flags & 3U
            ));
        }
        fixture.camera.right = 640U;
        fixture.camera.bottom = 480U;
        runtime = {
            .roles = fixture.roles,
            .active_object_slots = fixture.active_object_slots,
            .spatial_index = &spatial,
            .role_surface = role_surface,
            .node_pool = &node_pool,
            .movement = &movement,
            .camera = &fixture.camera,
            .selected_arrival_bytes = selected_arrival_bytes,
            .selected_role_index = selected_role_index,
            .map_height = kMapHeight,
            .scene_render_flags = &scene_render_flags,
        };
        fixture.runtime.story_paths = &runtime;
    }
};

void prime_loaded_instruction(Fixture& fixture, u16 raw_word);

struct MapRoleWriteHarness {
    static constexpr u32 kMapWidth = 32U;
    static constexpr u32 kMapHeight = 32U;

    Fixture& fixture;
    openswd3::world_map::LegacyMapsWorldDatabase database;
    std::vector<u8> payload = std::vector<u8>(0x200U, 0U);
    openswd3::world_map::LegacyRoleSpatialIndex spatial;
    std::vector<u8> surface =
        std::vector<u8>(kMapWidth * kMapHeight * sizeof(u32), 0U);
    openswd3::asset_runtime::LegacyAniRoleParticleEffect particles;

    explicit MapRoleWriteHarness(
        Fixture& source_fixture, const u32 current_map_id = 5U
    )
        : fixture(source_fixture) {
        fixture.roles.reserve(16U);
        fixture.roles[0].guid = 1U;
        fixture.roles[0].world_x = 16U;
        fixture.roles[0].world_y = 16U;
        for (auto& slot : fixture.active_object_slots) {
            slot.bytes.fill(0xFFU);
        }
        spatial.map_height = kMapHeight;
        const std::size_t row_count = static_cast<std::size_t>(kMapHeight) +
            2U * openswd3::world_map::kLegacySpatialRowPadding;
        for (auto& rows : spatial.row_heads) {
            rows.assign(row_count, openswd3::world_map::kLegacySpatialNoRole);
        }
        fixture.runtime.spatial_index = &spatial;
        fixture.runtime.role_surface = {
            .map_width = kMapWidth,
            .selected_guid = fixture.roles[0].guid,
            .surface_grid = surface,
        };
        fixture.runtime.mutable_maps_payload = payload;
        fixture.runtime.maps_database = &database;
        fixture.runtime.role_storage = &fixture.roles;
        fixture.runtime.role_particles = &particles;
        fixture.runtime.current_logical_map_id = current_map_id;
        fixture.runtime.map_height = kMapHeight;
    }

    void add_source(openswd3::world_map::LegacyMapsRoleSourceRecord source) {
        source.payload_offset = static_cast<u32>(
            0x40U +
            database.role_sources.size() *
                openswd3::world_map::kLegacyMapsRoleSourceRecordSize
        );
        database.role_sources.push_back(source);
        static_cast<void>(
            openswd3::world_map::write_legacy_maps_role_source_record(
                payload, database.role_sources.back()
            )
        );
    }

    void insert_runtime_role(const u32 role_index) {
        auto& role = fixture.roles[role_index];
        role.map_cell_pointer_32 =
            (role.world_y >> 4U) * kMapWidth + (role.world_x >> 4U);
        static_cast<void>(openswd3::world_map::insert_legacy_role_spatially(
            spatial, fixture.roles, role_index, role.flags & 3U
        ));
    }

    void prime(
        const u16 raw_word,
        const u16 selector,
        const u16 map_id,
        const u16 path_data_id,
        const u16 tile_x,
        const u16 tile_y,
        const u16 action_id,
        const u16 base_variant,
        const u16 variant_delta
    ) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, map_id);
        write_u16(fixture.state.window, 6U, path_data_id);
        write_u16(fixture.state.window, 8U, tile_x);
        write_u16(fixture.state.window, 10U, tile_y);
        write_u16(fixture.state.window, 12U, action_id);
        write_u16(fixture.state.window, 14U, base_variant);
        write_u16(fixture.state.window, 16U, variant_delta);
        write_u16(fixture.state.window, 18U, 67U);
        write_u16(fixture.state.window, 20U, 0U);
    }
};

[[maybe_unused]] void
prime_loaded_instruction(Fixture& fixture, const u16 raw_word) {
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0U;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    write_u16(fixture.state.window, 0U, raw_word);
}

[[maybe_unused]] void prime_long_camera_move(
    Fixture& fixture,
    const u16 raw_word,
    const i16 first,
    const i16 second,
    const u16 step_x,
    const u16 step_y
) {
    prime_loaded_instruction(fixture, raw_word);
    write_u16(fixture.state.window, 2U, static_cast<u16>(first));
    write_u16(fixture.state.window, 4U, static_cast<u16>(second));
    write_u16(fixture.state.window, 6U, step_x);
    write_u16(fixture.state.window, 8U, step_y);
    write_u16(fixture.state.window, 10U, kStoryVmTypedStop);
}

[[maybe_unused]] void prime_role_camera_move(
    Fixture& fixture,
    const u16 raw_word,
    const u16 selector,
    const u16 step_x,
    const u16 step_y
) {
    prime_loaded_instruction(fixture, raw_word);
    write_u16(fixture.state.window, 2U, selector);
    write_u16(fixture.state.window, 4U, step_x);
    write_u16(fixture.state.window, 6U, step_y);
    write_u16(fixture.state.window, 8U, kStoryVmTypedStop);
}

[[maybe_unused]] void prime_frame_color_transition(
    Fixture& fixture,
    const u16 raw_word,
    const std::array<i16, 6U>& components,
    const u16 duration
) {
    prime_loaded_instruction(fixture, raw_word);
    for (std::size_t index = 0U; index < components.size(); ++index) {
        write_u16(
            fixture.state.window,
            2U + index * 2U,
            static_cast<u16>(components[index])
        );
    }
    write_u16(fixture.state.window, 14U, duration);
    write_u16(fixture.state.window, 16U, kStoryVmTypedStop);
}

[[maybe_unused]] std::size_t write_dialog_instruction(
    Fixture& fixture,
    const u16 raw_word,
    const u16 selector,
    const std::span<const u8> text,
    const u16 frame_action_id = 0x232DU,
    const u16 left = 100U,
    const u16 top = 120U,
    const u16 columns = 2U,
    const u16 rows = 3U
) {
    prime_loaded_instruction(fixture, raw_word);
    auto window = std::span<u8>{fixture.state.window};
    write_u16(window, 2U, selector);
    write_u16(window, 4U, frame_action_id);
    const u16 opcode = static_cast<u16>(raw_word & 0x3FFFU);
    std::size_t text_offset{};
    if (opcode <= 2U) {
        text_offset = 6U;
    } else if (opcode <= 6U) {
        write_u16(window, 6U, left);
        write_u16(window, 8U, top);
        write_u16(window, 10U, columns);
        write_u16(window, 12U, rows);
        text_offset = 14U;
    } else {
        write_u16(window, 6U, columns);
        write_u16(window, 8U, rows);
        text_offset = 10U;
    }
    std::ranges::copy(
        text, window.begin() + static_cast<std::ptrdiff_t>(text_offset)
    );
    return text_offset + text.size();
}

}  // namespace
