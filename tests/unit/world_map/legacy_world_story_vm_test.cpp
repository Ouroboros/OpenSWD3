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
#include <filesystem>
#include <fstream>
#include <functional>
#include <span>
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
using openswd3::world_map::LegacyWorldRoleRecord;
using openswd3::world_map::LegacyWorldObjectSlot;
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
using openswd3::world_map::OP_70_START_ABSOLUTE_CAMERA_MOVE;
using openswd3::world_map::OP_73_START_CAMERA_MOVE_TO_ROLE;
using openswd3::world_map::OP_1025;
using openswd3::world_map::OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION;
using openswd3::world_map::OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS;

void write_u16(
    const std::span<u8> bytes, const std::size_t offset, const u16 value
) noexcept {
    bytes[offset] = static_cast<u8>(value);
    bytes[offset + 1U] = static_cast<u8>(value >> 8U);
}

[[nodiscard]] u16
read_u16(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u16>(
        static_cast<u16>(bytes[offset]) |
        static_cast<u16>(static_cast<u16>(bytes[offset + 1U]) << 8U)
    );
}

[[nodiscard]] u32
read_u32(const std::span<const u8> bytes, const std::size_t offset) noexcept {
    return static_cast<u32>(bytes[offset]) |
        (static_cast<u32>(bytes[offset + 1U]) << 8U) |
        (static_cast<u32>(bytes[offset + 2U]) << 16U) |
        (static_cast<u32>(bytes[offset + 3U]) << 24U);
}

void write_u32(
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

[[nodiscard]] openswd3::rendering::LegacyPixelConversionState
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

    void play_sound_effect(const u16 sound_id) noexcept override {
        sound_effect_requests.push_back(sound_id);
    }

    void clear_story_framebuffer() noexcept override {
        ++framebuffer_clear_count;
    }

    void present_story_framebuffer() noexcept override {
        ++framebuffer_present_count;
    }

    void begin_story_video(const std::span<const u8> filename) override {
        ++video_begin_count;
        last_video_filename.assign(filename.begin(), filename.end());
    }

    i32 query_story_video_progress() override {
        ++video_progress_query_count;
        return video_progress;
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
    u32 framebuffer_clear_count{};
    u32 framebuffer_present_count{};
    u32 video_begin_count{};
    u32 video_progress_query_count{};
    u32 beep_count{};
    u32 direct_audio_service_count{};
    u32 dialog_text_prepare_count{};
    u32 role_path_payload_release_count{};
    u32 released_role_path_index{0xFFFFFFFFU};
    u32 world_session_reload_begin_count{};
    u32 world_session_reload_count{};
    bool last_data_clear_before_read{};
    bool dialog_text_prepare_success{};
    bool world_session_reload_success{true};
    bool throw_on_dialog_text_prepare{};
    LegacyTalkWindowStatus data_load_status{LegacyTalkWindowStatus::ready};
    i32 last_story_id{};
    i32 video_progress{-1};
    std::vector<u8> last_video_filename;
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

    void play_sound_effect(const u16 sound_id) noexcept override {
        sound_effect_requests.push_back(sound_id);
    }

    void clear_story_framebuffer() noexcept override {
        ++framebuffer_clear_count;
    }

    void present_story_framebuffer() noexcept override {
        ++framebuffer_present_count;
    }

    void begin_story_video(const std::span<const u8> filename) override {
        ++video_begin_count;
        last_video_filename.assign(filename.begin(), filename.end());
    }

    i32 query_story_video_progress() override {
        ++video_progress_query_count;
        return -1;
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
    u32 video_begin_count{};
    u32 video_progress_query_count{};
    u32 role_path_payload_release_count{};
    u32 released_role_path_index{0xFFFFFFFFU};
    u32 world_session_reload_begin_count{};
    u32 world_session_reload_count{};
    openswd3::world_map::LegacyWorldLoadRequest last_world_load_request{};
    std::vector<u8> last_video_filename;
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

struct Fixture {
    LegacyWorldTalkContext context{};
    LegacyWorldStoryVmState state{};
    std::array<LegacyWorldRoleRecord, 3U> roles{};
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
    u32 indexed_target_selector{};
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{};
    RecordingPorts ports{};

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
        runtime.camera = &camera;
        runtime.indexed_target_selector = &indexed_target_selector;
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

void prime_loaded_instruction(Fixture& fixture, const u16 raw_word) {
    fixture.context.talk_data_offset = 0x1111U;
    fixture.context.instruction_offset = 0U;
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x1111U;
    fixture.state.window_loaded = true;
    write_u16(fixture.state.window, 0U, raw_word);
}

void prime_long_camera_move(
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
    write_u16(fixture.state.window, 10U, OP_1025);
}

void prime_role_camera_move(
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
    write_u16(fixture.state.window, 8U, OP_1025);
}

void prime_frame_color_transition(
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
    write_u16(fixture.state.window, 16U, OP_1025);
}

std::size_t write_dialog_instruction(
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

void test_shared_dialog_handler_variants(openswd3::test::Context& test) {
    constexpr std::array<u16, 8U> opcodes{1U, 2U, 3U, 4U, 5U, 6U, 89U, 90U};
    constexpr std::array<u8, 3U> text{'A', '%', 'Q'};
    for (const u16 opcode : opcodes) {
        Fixture fixture;
        const std::size_t end =
            write_dialog_instruction(fixture, opcode, 0x00F8U, text);
        fixture.state.previous_opcode = 0x1234U;
        const auto result = fixture.step();
        const auto& message = fixture.dialogs.messages.front();
        const auto& record = message.record;
        const bool odd_variant = (opcode & 1U) != 0U;
        const u32 expected_flags = (opcode == 5U || opcode == 6U ? 0x40U : 0U) |
            (odd_variant ? 0x10U : 0U);
        const u16 expected_width = opcode <= 2U ? 176U : 22U;
        const u16 expected_height = opcode <= 2U ? 66U : 33U;
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.dialog_enqueue_count == 1U &&
                result.dialog_text_prepare_count == 1U &&
                result.dialog_text_prepare_success_count == 0U &&
                result.direct_audio_service_count == 2U &&
                result.action_update_count == 1U &&
                fixture.context.instruction_offset == end &&
                fixture.roles[1].interaction_gate == (odd_variant ? 1U : 2U) &&
                fixture.dialogs.close.flagged_dialog_counter ==
                    (odd_variant ? 1U : 0U) &&
                record.role_index == 1U && record.flags == expected_flags &&
                record.width == expected_width &&
                record.height == expected_height &&
                record.character_delay == 4U &&
                message.text == std::vector<u8>(text.begin(), text.end()) &&
                fixture.state.previous_opcode == opcode &&
                fixture.state.dialog_anchor_left == 0x8000U &&
                fixture.state.dialog_anchor_top == 0x8000U &&
                !fixture.state.dialog_center_pending &&
                fixture.state.text_control_flags == 0xFFFFFFFFU &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 3U, 4U, 2U},
            "all eight shared dialog opcodes preserve their variant contract"
        );
        if (opcode >= 3U && opcode <= 6U) {
            test.expect_true(
                record.left == 100U && record.top == 120U,
                "mode one retains its explicit left and top words"
            );
        }
    }
}

void test_shared_dialog_raw_aliases(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        90U,
        static_cast<u16>(90U | 0x4000U),
        static_cast<u16>(90U | 0x8000U),
        static_cast<u16>(90U | 0xC000U)
    };
    constexpr std::array<u8, 2U> text{'%', 'Q'};
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        const std::size_t end =
            write_dialog_instruction(fixture, raw_word, 0xFFF0U, text);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == raw_word && result.opcode == 90U &&
                fixture.context.instruction_offset == end &&
                read_u16(fixture.state.window, 2U) == 0x00F8U &&
                fixture.dialogs.messages.front().record.role_index == 1U &&
                fixture.roles[1].interaction_gate == 2U,
            "raw aliases share mode-two semantics and rewrite FFF0 in place"
        );
    }

    Fixture chained;
    const std::size_t end =
        write_dialog_instruction(chained, 90U, 0x00F8U, text);
    write_u16(chained.state.window, end, 194U);
    const auto dialog = chained.step();
    const auto invalid = chained.step();
    test.expect_true(
        dialog.status == LegacyWorldStoryVmStatus::yielded &&
            invalid.status == LegacyWorldStoryVmStatus::yielded &&
            invalid.invalid_opcode_previous == 90U &&
            invalid.invalid_opcode_current == 194U &&
            chained.state.previous_opcode == 194U,
        "dialog common join publishes its opcode before a later default"
    );
}

void test_clear_dialog_control_flag(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0007U, 0x4007U, 0x8007U, 0xC007U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        fixture.state.text_control_flags = 0x92345678U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0x12345678U &&
                fixture.state.previous_opcode == 7U &&
                result.direct_audio_service_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 7 aliases clear bit 31 and continue without audio"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 7U);
    write_u16(chained.state.window, 2U, 194U);
    const auto result = chained.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 2U &&
            result.invalid_opcode_previous == 7U &&
            result.invalid_opcode_current == 194U && result.beep_count == 1U &&
            result.direct_audio_service_count == 1U &&
            chained.context.instruction_offset == 2U &&
            chained.state.previous_opcode == 194U,
        "opcode 7 publishes previous before the same-call next fetch"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 7U);
    write_u16(dialog.state.window, 2U, 2U);
    write_u16(dialog.state.window, 4U, 0x00F8U);
    write_u16(dialog.state.window, 6U, 0x232DU);
    dialog.state.window[8U] = '%';
    dialog.state.window[9U] = 'Q';
    const auto dialog_result = dialog.step();
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog_result.dialog_enqueue_count == 1U &&
            dialog.context.instruction_offset == 10U &&
            (dialog.dialogs.messages.front().record.flags & 0x20U) != 0U &&
            dialog.state.text_control_flags == 0xFFFFFFFFU &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog observes opcode 7 bit 31 clear before resetting it"
    );
}

void test_clear_dialog_control_flag_bit30(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0009U, 0x4009U, 0x8009U, 0xC009U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        fixture.state.text_control_flags = 0xD2345678U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.text_control_flags == 0x92345678U &&
                fixture.state.previous_opcode == 9U &&
                result.direct_audio_service_count == 0U,
            "opcode 9 aliases clear only bit 30 and continue without audio"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 9U);
    write_u16(chained.state.window, 2U, 194U);
    const auto result = chained.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.executed_instruction_count == 2U &&
            result.invalid_opcode_previous == 9U &&
            result.invalid_opcode_current == 194U &&
            chained.context.instruction_offset == 2U,
        "opcode 9 publishes previous before the same-call next fetch"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 9U);
    write_u16(dialog.state.window, 2U, 2U);
    write_u16(dialog.state.window, 4U, 0x00F8U);
    write_u16(dialog.state.window, 6U, 0x232DU);
    dialog.state.window[8U] = '%';
    dialog.state.window[9U] = 'Q';
    const auto dialog_result = dialog.step();
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog.context.instruction_offset == 10U &&
            (dialog.dialogs.messages.front().record.flags & 0x400U) != 0U &&
            dialog.state.text_control_flags == 0xFFFFFFFFU &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog observes opcode 9 bit 30 clear before resetting it"
    );
}

void test_stage_dialog_lifetime(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x0008U, 0x4008U, 0x8008U, 0xC008U
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFFFU);
        write_u16(fixture.state.window, 4U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 4U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.next_text_aux_pending &&
                fixture.state.next_text_aux_value == 0xFFFFU &&
                fixture.state.previous_opcode == 8U &&
                result.direct_audio_service_count == 0U &&
                fixture.ports.story_protocol_events.empty(),
            "opcode 8 aliases stage an unsigned word and continue without audio"
        );
    }

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, 8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            !truncated.state.next_text_aux_pending &&
            truncated.state.next_text_aux_value == 60U &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.direct_audio_service_count == 0U,
        "opcode 8 short operand fails before one-shot state and join effects"
    );

    Fixture dialog;
    prime_loaded_instruction(dialog, 8U);
    write_u16(dialog.state.window, 2U, 37U);
    write_u16(dialog.state.window, 4U, 2U);
    write_u16(dialog.state.window, 6U, 0x00F8U);
    write_u16(dialog.state.window, 8U, 0x232DU);
    dialog.state.window[10U] = '%';
    dialog.state.window[11U] = 'Q';
    const auto dialog_result = dialog.step();
    const auto& record = dialog.dialogs.messages.front().record;
    test.expect_true(
        dialog_result.status == LegacyWorldStoryVmStatus::yielded &&
            dialog_result.executed_instruction_count == 2U &&
            dialog.context.instruction_offset == 12U &&
            (record.flags & 0x08U) != 0U && record.lifetime_limit == 37U &&
            !dialog.state.next_text_aux_pending &&
            dialog.state.next_text_aux_value == 60U &&
            dialog.state.previous_opcode == 2U,
        "same-call dialog consumes and resets opcode 8 one-shot lifetime"
    );
}

void test_dialog_text_preparation_and_mode_zero_metrics(
    openswd3::test::Context& test
) {
    constexpr std::array<u8, 6U> source{'%', 'T', '1', '.', '%', 'Q'};
    const std::array<std::vector<u8>, 2U> prepared{
        std::vector<u8>{'%', 'N', '%', 'N', '%', 'N', '%', 'Q'},
        std::vector<u8>{'%', 'N', '%', 'N', '%', 'N', '%', 'N', '%', 'Q'},
    };
    constexpr std::array<u16, 2U> widths{198U, 220U};
    constexpr std::array<u16, 2U> heights{88U, 110U};
    for (std::size_t index = 0U; index < prepared.size(); ++index) {
        Fixture fixture;
        write_dialog_instruction(fixture, 1U, 0x00F8U, source);
        fixture.ports.dialog_text_prepare_success = true;
        fixture.ports.prepared_dialog_text = prepared[index];
        const auto result = fixture.step();
        const auto& message = fixture.dialogs.messages.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.dialog_text_prepare_success_count == 1U &&
                fixture.ports.last_dialog_text ==
                    std::vector<u8>(source.begin(), source.end()) &&
                message.text == prepared[index] &&
                message.record.width == widths[index] &&
                message.record.height == heights[index],
            "prepared mode-zero text selects the original line-count bucket"
        );
    }

    Fixture measured;
    constexpr std::array<u8, 7U> measured_text{
        'A', 'B', 'C', 'D', 'E', '%', 'Q'
    };
    write_dialog_instruction(measured, 2U, 0x00F8U, measured_text);
    measured.state.text_control_flags &= 0xF7FFFFFFU;
    const auto result = measured.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            measured.dialogs.messages.front().record.width == 55U &&
            measured.dialogs.messages.front().record.height == 22U,
        "bit-27 clear uses measured visible bytes and a two-unit height"
    );
}

void test_dialog_anchor_center_delay_and_reset(openswd3::test::Context& test) {
    Fixture fixture;
    constexpr std::array<u8, 2U> text{'%', 'Q'};
    write_dialog_instruction(
        fixture, 4U, 0x00F8U, text, 0x232DU, 200U, 120U, 4U, 3U
    );
    fixture.state.dialog_anchor_left = 10U;
    fixture.state.dialog_anchor_top = 20U;
    fixture.state.dialog_center_pending = true;
    fixture.state.dialog_character_delay_base = 3U;
    fixture.state.text_layout_first = 7;
    fixture.state.text_layout_second = -9;
    fixture.state.speaker_name[0] = 'N';
    fixture.state.speaker_name[1] = 0U;
    fixture.state.speaker_name[2] = 0xAAU;
    const auto result = fixture.step(16, 32);
    const auto& message = fixture.dialogs.messages.front();
    const auto& record = message.record;
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            record.anchor_left == 26U && record.anchor_top == 52U &&
            record.left == 178U && record.top == 120U && record.width == 44U &&
            record.height == 33U && record.character_delay == 6U &&
            message.caption.size() == 1U &&
            fixture.state.speaker_name[0] == 0U &&
            fixture.state.speaker_name[2] == 0xAAU &&
            fixture.state.dialog_anchor_left == 0x8000U &&
            fixture.state.dialog_anchor_top == 0x8000U &&
            !fixture.state.dialog_center_pending &&
            fixture.state.text_layout_first == 0 &&
            fixture.state.text_layout_second == 0 &&
            fixture.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U, 4U, 4U, 2U},
        "anchor, center, configured delay and one-byte name reset are exact"
    );

    Fixture detached;
    write_dialog_instruction(detached, 2U, 0xFFFDU, text);
    detached.context.world_x = 64U;
    detached.context.world_y = 80U;
    const auto detached_result = detached.step();
    const auto& detached_record = detached.dialogs.messages.front().record;
    test.expect_true(
        detached_result.status == LegacyWorldStoryVmStatus::yielded &&
            detached_record.role_index == 0xFFFDU &&
            detached_record.anchor_left == 64U &&
            detached_record.anchor_top == 80U &&
            detached.context.field_26 == 2U &&
            detached.roles[1].interaction_gate == 0U,
        "FFFD uses the detached context anchor and interaction gate"
    );

    Fixture index_zero;
    write_dialog_instruction(index_zero, 90U, 0U, text);
    index_zero.roles[0].guid = 0U;
    const auto zero_result = index_zero.step();
    const auto& zero_record = index_zero.dialogs.messages.front().record;
    test.expect_true(
        zero_result.status == LegacyWorldStoryVmStatus::yielded &&
            zero_record.role_index == 0U && zero_record.left == 30U &&
            zero_record.top == 99U &&
            index_zero.roles[0].interaction_gate == 2U,
        "role index zero preserves the original skipped auto-center branch"
    );
}

void test_dialog_checked_failure_order(openswd3::test::Context& test) {
    constexpr std::array<u8, 3U> text{'A', '%', 'Q'};
    Fixture missing_role;
    write_dialog_instruction(missing_role, 90U, 0x7777U, text);
    missing_role.state.previous_opcode = 0x55U;
    missing_role.state.text_control_flags = 0x12345678U;
    missing_role.state.speaker_name[0] = 'X';
    const auto role_result = missing_role.step();
    test.expect_true(
        role_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            role_result.direct_audio_service_count == 1U &&
            role_result.dialog_text_prepare_count == 1U &&
            missing_role.dialogs.messages.empty() &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.state.text_control_flags == 0x12345678U &&
            missing_role.state.speaker_name[0] == 'X' &&
            missing_role.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U},
        "missing role stops at the caller gate write after audio and prepare"
    );

    Fixture missing_terminator;
    prime_loaded_instruction(missing_terminator, 90U);
    write_u16(missing_terminator.state.window, 2U, 0x00F8U);
    write_u16(missing_terminator.state.window, 4U, 0x232DU);
    write_u16(missing_terminator.state.window, 6U, 2U);
    write_u16(missing_terminator.state.window, 8U, 3U);
    missing_terminator.state.window[10U] = 'A';
    const auto terminator_result = missing_terminator.step();
    test.expect_true(
        terminator_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            terminator_result.direct_audio_service_count == 1U &&
            terminator_result.dialog_text_prepare_count == 0U &&
            missing_terminator.dialogs.messages.empty() &&
            missing_terminator.ports.story_protocol_events ==
                std::vector<u32>{2U},
        "missing percent-Q stops after the first original audio service"
    );

    Fixture allocation_failure;
    write_dialog_instruction(allocation_failure, 90U, 0x00F8U, text);
    allocation_failure.ports.throw_on_dialog_text_prepare = true;
    const auto allocation_result = allocation_failure.step();
    test.expect_true(
        allocation_result.status ==
                LegacyWorldStoryVmStatus::dialog_allocation_failed &&
            allocation_result.direct_audio_service_count == 1U &&
            allocation_result.dialog_text_prepare_count == 1U &&
            allocation_failure.dialogs.messages.empty() &&
            allocation_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 3U},
        "text preparation allocation failure preserves prior effects"
    );
}

void test_default_invalid_opcode_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> opcode_zero_aliases{
        0x0000U, 0x4000U, 0x8000U, 0xC000U
    };
    for (const u16 raw_word : opcode_zero_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        fixture.state.previous_opcode = 0x1234U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.raw_word == raw_word && result.opcode == 0U &&
                result.instruction_offset == 0U &&
                fixture.context.instruction_offset == 0U &&
                result.executed_instruction_count == 1U &&
                result.invalid_opcode_diagnostic_count == 1U &&
                result.invalid_opcode_current == 0U &&
                result.invalid_opcode_previous == 0x1234U &&
                result.beep_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.state.previous_opcode == 0U &&
                fixture.ports.beep_count == 1U &&
                fixture.ports.direct_audio_service_count == 1U &&
                fixture.ports.default_protocol_events ==
                    std::vector<u32>{1U, 2U},
            "opcode zero raw aliases beep, diagnose, publish and service"
        );
    }

    constexpr std::array<u16, 4U> default_boundaries{
        194U, 1023U, 1027U, 16382U
    };
    for (const u16 opcode : default_boundaries) {
        Fixture fixture;
        prime_loaded_instruction(fixture, opcode);
        fixture.state.previous_opcode = 0x55AAU;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                fixture.context.instruction_offset == 0U &&
                result.invalid_opcode_current == opcode &&
                result.invalid_opcode_previous == 0x55AAU &&
                fixture.state.previous_opcode == opcode &&
                fixture.ports.default_protocol_events ==
                    std::vector<u32>{1U, 2U},
            "both original default ranges retain the no-advance protocol"
        );
    }

    Fixture chained;
    prime_loaded_instruction(chained, 194U);
    chained.state.previous_opcode = 0x55U;
    const auto first = chained.step();
    write_u16(chained.state.window, 0U, 1023U);
    const auto second = chained.step();
    test.expect_true(
        first.invalid_opcode_previous == 0x55U &&
            first.invalid_opcode_current == 194U &&
            second.invalid_opcode_previous == 194U &&
            second.invalid_opcode_current == 1023U &&
            chained.state.previous_opcode == 1023U &&
            chained.ports.default_protocol_events ==
                std::vector<u32>{1U, 2U, 1U, 2U},
        "default diagnostics observe the prior join value before publishing"
    );

    constexpr std::array<u16, 3U> explicit_unimplemented{OP_1025, 1024U, 1025U};
    for (const u16 opcode : explicit_unimplemented) {
        Fixture fixture;
        prime_loaded_instruction(fixture, opcode);
        fixture.state.previous_opcode = 0xA5A5U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == 0xA5A5U &&
                fixture.ports.beep_count == 0U &&
                fixture.ports.direct_audio_service_count == 0U &&
                fixture.ports.default_protocol_events.empty(),
            "explicit unimplemented opcodes do not masquerade as defaults"
        );
    }
}

void test_initial_flags_and_alignment_gate(openswd3::test::Context& test) {
    Fixture fixture;
    const auto initialized = fixture.state;
    fixture.roles[0].world_x = 17U;
    const auto blocked = fixture.step();
    test.expect_true(
        openswd3::world_map::query_legacy_world_story_flag(initialized, 1U) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 3U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 4U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 10U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 30U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                initialized, 70U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                initialized, 2U
            ) &&
            initialized.script_variables[0] == 100U &&
            initialized.deferred_map_tile_x == -1 &&
            initialized.deferred_map_tile_y == -1 &&
            initialized.deferred_map_id == 0 &&
            std::ranges::all_of(
                initialized.script_variables.begin() + 1U,
                initialized.script_variables.end(),
                [](const u32 value) { return value == 0U; }
            ) &&
            blocked.status == LegacyWorldStoryVmStatus::yielded &&
            fixture.ports.story_load_count == 0U &&
            fixture.context.talk_data_offset == 0U,
        "sub_40E0B0 flags, initial money and first-load alignment gate are exact"
    );
}

void test_reinitialization_writes_only_owned_vm_fields(
    openswd3::test::Context& test
) {
    LegacyWorldStoryVmState state;
    state.flags.fill(0xFFU);
    state.script_variables.fill(0x12345678U);
    state.window[0] = 0xA5U;
    state.speaker_name[0] = 0x5AU;
    state.text_control_flags = 0x11223344U;
    state.wait_duration = 9U;
    state.wait_started_at = 10U;
    state.loaded_file_number = 11U;
    state.loaded_data_offset = 12U;
    state.window_loaded = true;
    state.music_request = 13U;
    state.music_first_stream = 14U;
    state.music_second_stream = 15U;
    state.music_control_flags = 16U;
    state.current_first_stream = 17U;
    state.current_second_stream = 18U;
    state.previous_opcode = 0x1234U;
    state.guid_one_action_override = 0x5678U;
    state.dialog_scale = 13U;
    state.dialog_character_delay_base = 3U;
    state.dialog_anchor_left = 21U;
    state.dialog_anchor_top = 22U;
    state.dialog_center_pending = true;

    openswd3::world_map::initialize_legacy_world_story_vm(state);

    test.expect_true(
        state.script_variables[0] == 100U &&
            state.script_variables[1] == 0x12345678U &&
            state.window[0] == 0xA5U && state.speaker_name[0] == 0x5AU &&
            state.text_control_flags == 0x11223344U &&
            state.wait_duration == 9U && state.wait_started_at == 10U &&
            state.loaded_file_number == 11U &&
            state.loaded_data_offset == 12U && state.window_loaded &&
            state.music_request == 0U && state.music_first_stream == 0U &&
            state.music_second_stream == 0U &&
            state.music_control_flags == 0U &&
            state.current_first_stream == 1U &&
            state.current_second_stream == 0U &&
            state.previous_opcode == 0x1234U &&
            state.guid_one_action_override == 0U && state.dialog_scale == 13U &&
            state.dialog_character_delay_base == 3U &&
            state.dialog_anchor_left == 21U && state.dialog_anchor_top == 22U &&
            state.dialog_center_pending && state.deferred_map_tile_x == -1 &&
            state.deferred_map_tile_y == -1 && state.deferred_map_id == 0 &&
            openswd3::world_map::query_legacy_world_story_flag(state, 70U) &&
            !openswd3::world_map::query_legacy_world_story_flag(state, 2U),
        "sub_40E0B0 rewrites only its VM globals on repeated initialization"
    );
}

void test_dialog_enqueue_and_wait_protocol(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 89U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x232DU);
    write_u16(script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 8U, 8U);
    script[10U] = 'A';
    script[11U] = '%';
    script[12U] = 'Q';
    write_u16(script, 13U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 15U, 0x00F8U);
    write_u16(script, 17U, 0xFFFFU);
    fixture.state.speaker_name[0] = 'N';
    fixture.state.speaker_name[1] = 0U;

    const auto enqueued = fixture.step(16, 32);
    const auto& message = fixture.dialogs.messages.front();
    test.expect_equal(
        enqueued.status, LegacyWorldStoryVmStatus::yielded, "opcode 89 yields"
    );
    test.expect_equal(
        enqueued.executed_instruction_count,
        1U,
        "opcode 89 counts one instruction"
    );
    test.expect_equal(
        enqueued.dialog_enqueue_count, 1U, "opcode 89 enqueues one dialog"
    );
    test.expect_equal(
        enqueued.action_update_count,
        3U,
        "initial load, frame and caption update three actions"
    );
    test.expect_equal(
        fixture.context.instruction_offset,
        u16{13U},
        "opcode 89 advances behind %Q"
    );
    test.expect_equal(
        fixture.roles[1].interaction_gate,
        u16{1U},
        "opcode 89 leaves the owner gate at one"
    );
    test.expect_true(
        (fixture.roles[1].flags & 0x00080000U) != 0U,
        "initial load marks the source role"
    );
    test.expect_equal(
        message.record.width,
        u16{154U},
        "dialog width is column count times eleven"
    );
    test.expect_equal(
        message.record.height,
        u16{88U},
        "dialog height is row count times eleven"
    );
    test.expect_equal(
        message.record.left,
        u16{227U},
        "dialog left uses role, camera and facing offset"
    );
    test.expect_equal(
        message.record.top,
        u16{260U},
        "dialog top uses role, camera and facing offset"
    );
    test.expect_equal(
        message.record.flags,
        u32{0x10U},
        "opcode 89 adds only its odd-variant flag"
    );
    test.expect_equal(
        message.record.character_delay,
        u16{4U},
        "dialog delay is twice the initialized base delay"
    );
    test.expect_true(
        message.record.saved_foreground_index == 0U &&
            message.record.saved_secondary_index == 0U &&
            message.record.text_style == 4U &&
            message.record.saved_text_style == 0U,
        "calloc leaves saved text attributes zero while sub_40AFF0 sets style four"
    );
    test.expect_equal(
        message.caption.size(),
        std::size_t{1U},
        "speaker name becomes the caption"
    );
    test.expect_equal(
        message.text.size(),
        std::size_t{3U},
        "dialog text includes its %Q terminator"
    );
    test.expect_equal(
        fixture.state.speaker_name[0],
        u8{0U},
        "speaker buffer is cleared after enqueue"
    );
    test.expect_equal(
        fixture.dialogs.close.flagged_dialog_counter,
        u32{0x8001U},
        "dialog count increments without losing story lock"
    );

    const auto waiting = fixture.step();
    const u16 waiting_instruction_offset = fixture.context.instruction_offset;
    fixture.roles[1].interaction_gate = 0U;
    const auto released = fixture.step();
    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.executed_instruction_count == 1U &&
            waiting_instruction_offset == 13U &&
            released.status == LegacyWorldStoryVmStatus::yielded &&
            released.executed_instruction_count == 1U &&
            fixture.context.instruction_offset == 17U,
        "opcode 14 stalls on gate one and advances then yields at zero"
    );
}

void test_dialog_role_overlap_avoidance(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 89U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x232DU);
    write_u16(script, 6U, 8U);
    write_u16(script, 8U, 4U);
    script[10U] = '%';
    script[11U] = 'Q';
    fixture.roles[1].world_x = 320U;
    fixture.roles[1].world_y = 30U;
    fixture.roles[1].action.variant_delta = 1U;

    const auto result = fixture.step();
    const auto& record = fixture.dialogs.messages.front().record;
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            record.left == 276U && record.top == 32U,
        "sub_40AFF0 repeats the facing offset when the first panel still overlaps its role"
    );
}

void test_dialog_explicit_layout_pair(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 104U);
    write_u16(script, 2U, 5U);
    write_u16(script, 4U, static_cast<u16>(-7));
    write_u16(script, 6U, 89U);
    write_u16(script, 8U, 0x00F8U);
    write_u16(script, 10U, 0x232DU);
    write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 14U, 8U);
    script[16U] = '%';
    script[17U] = 'Q';

    const auto result = fixture.step();
    const auto& record = fixture.dialogs.messages.front().record;

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 89U && result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 18U && record.left == 248U &&
            record.top == 189U,
        "opcode 104 replaces the second role-facing offset with its signed pair"
    );
    test.expect_true(
        fixture.state.text_control_flags == 0xFFFFFFFFU &&
            fixture.state.text_layout_first == 0 &&
            fixture.state.text_layout_second == 0,
        "dialog enqueue resets opcode 104 text globals to their legacy defaults"
    );
}

void test_transfer_flags_and_terminal_cleanup(openswd3::test::Context& test) {
    Fixture fixture;
    auto first = std::span<u8>{fixture.ports.initial_window};
    write_u16(first, 0U, 161U);
    write_u16(first, 2U, 2042U);
    auto second = std::span<u8>{fixture.ports.transferred_window};
    write_u16(second, 0U, 25U);
    write_u16(second, 2U, 123U);
    write_u16(second, 4U, 26U);
    write_u16(second, 6U, 3U);
    write_u16(second, 8U, 0xFFFFU);
    fixture.roles[1].flags |= 0x00000800U;
    fixture.roles[1].action.base_variant = 1U;
    fixture.roles[1].action.variant_delta = 2U;
    fixture.roles[1].action.one_shot_base_variant = 7U;
    fixture.roles[1].action.one_shot_variant_delta = 6U;
    fixture.roles[2].path_data_id = 9U;
    fixture.roles[2].path_word_index = 17U;
    fixture.roles[2].action.one_shot_base_variant = 8U;
    fixture.roles[2].action.one_shot_variant_delta = 5U;
    fixture.active_object_slots[0].bytes[0] = 2U;
    fixture.active_object_slots[0].bytes[1] = 0U;
    fixture.active_object_slots[0].bytes[0x1BU] = 2U;

    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::terminated &&
            result.executed_instruction_count == 4U &&
            fixture.ports.story_load_count == 2U &&
            fixture.ports.last_story_id == 2042 &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 123U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 3U
            ) &&
            fixture.context.source_guid == 0xFFFFU &&
            fixture.context.talk_data_offset == 0xFFFFFFFFU &&
            !fixture.state.window_loaded &&
            (fixture.dialogs.close.flagged_dialog_counter & 0x8000U) == 0U &&
            (fixture.roles[1].flags & 0x00080000U) == 0U &&
            fixture.roles[1].action.base_variant == 7U &&
            fixture.roles[1].action.variant_delta == 6U &&
            fixture.roles[1].action.one_shot_base_variant == 0xFFFFFFFFU &&
            fixture.roles[1].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            fixture.roles[2].action.one_shot_base_variant == 0xFFFFFFFFU &&
            fixture.roles[2].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            fixture.roles[2].path_data_id == 0U &&
            fixture.roles[2].path_word_index == 0U &&
            std::ranges::all_of(
                fixture.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            result.role_one_shot_clear_count == fixture.roles.size() &&
            result.active_object_reset_count == 1U &&
            result.action_update_count == 2U,
        "opcode 161 replaces the window, 25/26 mutate flags, and FFFF restores and releases the source role"
    );
}

void test_same_file_branch(openswd3::test::Context& test) {
    Fixture fixture;
    auto first = std::span<u8>{fixture.ports.initial_window};
    write_u16(first, 0U, 21U);
    write_u16(first, 2U, 1U);
    write_u32(first, 4U, 0x3333U);
    auto branch = std::span<u8>{fixture.ports.transferred_window};
    write_u16(branch, 0U, 0xFFFFU);

    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::terminated &&
            result.executed_instruction_count == 2U &&
            fixture.ports.data_load_count == 1U &&
            result.action_update_count == 2U,
        "opcode 21 branches within the current TALK file before TalkEnd"
    );
}

void test_role_action_operand_extension(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 120U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x8000U);
    write_u16(script, 6U, 0xFFFEU);
    write_u16(script, 8U, 0x8000U);
    write_u16(script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 12U, 0x00F8U);

    const auto result = fixture.step();

    Fixture missing;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, 120U);
    write_u16(missing_script, 2U, 0x7777U);
    write_u16(missing_script, 4U, 0x8000U);
    write_u16(missing_script, 6U, 0xFFFEU);
    write_u16(missing_script, 8U, 0x8123U);
    write_u16(missing_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing_script, 12U, 0x00F8U);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();

    Fixture preserved;
    preserved.roles[1].action.action_id = 0x12345678U;
    preserved.roles[1].action.base_variant = 0x87654321U;
    preserved.roles[1].action.variant_delta = 0x10203040U;
    preserved.roles[1].action.wait_remaining = 7U;
    auto preserved_script = std::span<u8>{preserved.ports.initial_window};
    write_u16(preserved_script, 0U, 120U);
    write_u16(preserved_script, 2U, 0x00F8U);
    write_u16(preserved_script, 4U, 0xFFFFU);
    write_u16(preserved_script, 6U, 0xFFFFU);
    write_u16(preserved_script, 8U, 0xFFFFU);
    write_u16(preserved_script, 10U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(preserved_script, 12U, 0x00F8U);
    const auto preserved_result = preserved.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            fixture.roles[1].action.action_id == 0xFFFF8000U &&
            fixture.roles[1].action.base_variant == 0xFFFFFFFEU &&
            fixture.roles[1].action.variant_delta == 0x00008000U,
        "opcode 120 sign-extends action and base while zero-extending variant"
    );
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            missing_result.executed_instruction_count == 2U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.action_id == 0x8000U &&
            patch.base_variant == 0xFFFEU && patch.variant_delta == 0x8123U &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 120 patches the MAPS role source and consumes ten bytes when " "the runtime role is absent"
    );
    test.expect_true(
        preserved_result.status == LegacyWorldStoryVmStatus::yielded &&
            preserved_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            preserved.roles[1].action.action_id == 0x12345678U &&
            preserved.roles[1].action.base_variant == 0x87654321U &&
            preserved.roles[1].action.variant_delta == 0x10203040U &&
            preserved.roles[1].action.wait_remaining == 0U &&
            (preserved.roles[1].flags & 0x1000U) != 0U &&
            preserved.ports.action_update_count == 2U,
        "opcode 120 preserves FFFF action operands while refreshing and " "marking the resolved role"
    );
}

void test_missing_role_position_patch(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 40U);
    write_u16(script, 2U, 0x7777U);
    write_u16(script, 4U, 0x8123U);
    write_u16(script, 6U, 0xFEDCU);
    write_u16(script, 8U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 10U, 0x00F8U);

    const auto result = fixture.step();
    const auto patch = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 12U &&
            fixture.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.tile_x == 0x8123U &&
            patch.tile_y == 0xFEDCU && patch.flags_or_mask == 0U &&
            patch.flags_and_mask == 0xFFFFU && patch.action_id == 0xFFFFU &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU,
        "opcode 40 preserves raw tile words in the MAPS fallback and consumes " "eight bytes when the role is absent"
    );
}

void test_change_role_base_variant_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x000AU, 0x400AU, 0x800AU, 0xC00AU
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFF0U);
        write_u16(fixture.state.window, 4U, 0x1234U);
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.roles[1].action.wait_remaining = 9U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.instruction_offset == 6U &&
                fixture.context.instruction_offset == 6U &&
                fixture.roles[1].action.base_variant == 0x1234U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                result.action_update_count == 1U &&
                fixture.state.previous_opcode == 10U &&
                fixture.ports.role_patch_requests.empty(),
            "opcode 10 aliases update the live role and continue"
        );
    }

    Fixture missing;
    prime_loaded_instruction(missing, 10U);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x0333U);
    write_u16(missing.state.window, 6U, OP_1025);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == 10U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.base_variant == 0x0333U &&
            patch.action_id == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 10 patches the MAPS source when the live role is absent"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, 10U);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x1234U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled_result.action_update_count == 0U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 10 stops at an invalid controlled-role live access"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, 10U);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.action_update_count == 0U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 10 short payload fails before role or MAPS mutation"
    );
}

void test_change_role_variant_delta_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        0x000BU, 0x400BU, 0x800BU, 0xC00BU
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFFF0U);
        write_u16(fixture.state.window, 4U, 0x8123U);
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.roles[1].flags = 0xA5000001U;
        fixture.roles[1].action.wait_remaining = 9U;
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.roles[1].action.variant_delta == 0x8123U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[1].flags == 0xA5001001U &&
                result.action_update_count == 1U &&
                fixture.state.previous_opcode == 11U &&
                fixture.ports.role_patch_requests.empty(),
            "opcode 11 aliases update the live role and continue"
        );
    }

    Fixture missing;
    prime_loaded_instruction(missing, 11U);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x8123U);
    write_u16(missing.state.window, 6U, OP_1025);
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == 11U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.variant_delta == 0x8123U &&
            patch.action_id == 0xFFFFU && patch.base_variant == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU,
        "opcode 11 patches the MAPS source when the live role is absent"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, 11U);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8123U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled_result.action_update_count == 0U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 11 stops at an invalid controlled-role live access"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, 11U);
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated_result.action_update_count == 0U &&
            truncated.ports.role_patch_requests.empty(),
        "opcode 11 short payload fails before role or MAPS mutation"
    );

    Fixture chained;
    prime_loaded_instruction(chained, 11U);
    write_u16(chained.state.window, 2U, 0x00F8U);
    write_u16(chained.state.window, 4U, 3U);
    write_u16(chained.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 8U, 0x00F8U);
    write_u16(chained.state.window, 10U, 0x0222U);
    write_u16(chained.state.window, 12U, OP_1025);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            chained_result.executed_instruction_count == 3U &&
            chained_result.action_update_count == 1U &&
            chained.roles[1].action.variant_delta == 3U &&
            chained.roles[1].action.action_id == 0x0222U &&
            (chained.roles[1].flags & 0x00001000U) != 0U,
        "opcode 11 defers its action update across a same-role opcode 45"
    );
}

void test_set_role_position_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_12_SET_ROLE_POSITION,
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0x4000U),
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0x8000U),
        static_cast<u16>(OP_12_SET_ROLE_POSITION | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0x02080001U;
        fixture.roles[1].action.cached_base_variant = 7U;
        fixture.roles[1].action.cached_variant_delta = 8U;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x1015U);
        write_u16(fixture.state.window, 6U, 0x100FU);
        write_u16(fixture.state.window, 8U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        const auto slot =
            std::span<const u8>{fixture.active_object_slots[0].bytes};
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 0U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode == OP_12_SET_ROLE_POSITION &&
                fixture.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
                fixture.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
                (fixture.roles[1].flags & 0x02080000U) == 0U &&
                read_u16(slot, 0x00U) == 1U && read_u16(slot, 0x04U) == 336U &&
                read_u16(slot, 0x06U) == 240U && (slot[0x1BU] & 0x0FU) == 2U &&
                fixture.dialogs.close.flagged_dialog_counter == 0U,
            "opcode 12 aliases schedule wrapped coordinates and continue"
        );
    }

    Fixture current_alias;
    current_alias.roles[1].flags = 0x00080000U;
    current_alias.roles[1].action.cached_base_variant = 7U;
    current_alias.roles[1].action.cached_variant_delta = 8U;
    StoryPathHarness alias_paths{current_alias};
    prime_loaded_instruction(current_alias, OP_12_SET_ROLE_POSITION);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    write_u16(current_alias.state.window, 4U, 22U);
    write_u16(current_alias.state.window, 6U, 15U);
    write_u16(current_alias.state.window, 8U, OP_1025);
    const auto alias_result = current_alias.step();
    test.expect_true(
        alias_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            current_alias.roles[1].action.cached_base_variant == 7U &&
            current_alias.roles[1].action.cached_variant_delta == 8U &&
            (current_alias.roles[1].flags & 0x00080000U) != 0U,
        "opcode 12 substitutes FFF0 for lookup but not for raw cache reset"
    );

    Fixture controlled;
    StoryPathHarness controlled_paths{controlled, 1U};
    prime_loaded_instruction(controlled, OP_12_SET_ROLE_POSITION);
    write_u16(controlled.state.window, 2U, 0x00F8U);
    write_u16(controlled.state.window, 4U, 21U);
    write_u16(controlled.state.window, 6U, 15U);
    write_u16(controlled.state.window, 8U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.dialogs.close.flagged_dialog_counter == 0x8000U &&
            controlled.context.instruction_offset == 8U &&
            controlled.state.previous_opcode == OP_12_SET_ROLE_POSITION,
        "opcode 12 marks dialog state when the target is controlled"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_12_SET_ROLE_POSITION);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 21U);
    write_u16(missing.state.window, 6U, 15U);
    write_u16(missing.state.window, 8U, OP_1025);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            missing_result.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 8U &&
            missing.state.previous_opcode == OP_12_SET_ROLE_POSITION &&
            missing.dialogs.close.flagged_dialog_counter == 0U,
        "opcode 12 consumes an ordinary missing role without scheduling"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_12_SET_ROLE_POSITION);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 21U);
    write_u16(invalid_controlled.state.window, 6U, 15U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 12 stops before an invalid controlled-role schedule"
    );

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_12_SET_ROLE_POSITION);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    write_u16(unavailable.state.window, 4U, 21U);
    write_u16(unavailable.state.window, 6U, 15U);
    unavailable.roles[1].flags = 0x00080000U;
    unavailable.roles[1].action.cached_base_variant = 7U;
    unavailable.roles[1].action.cached_variant_delta = 8U;
    unavailable.state.previous_opcode = 0x55U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x55U &&
            unavailable.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
            unavailable.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
            (unavailable.roles[1].flags & 0x00080000U) == 0U,
        "opcode 12 preserves cache reset before a missing path runtime"
    );

    Fixture found_truncated;
    found_truncated.context.instruction_offset = 0x7FFCU;
    found_truncated.context.talk_data_offset = 0x1111U;
    found_truncated.state.loaded_file_number = 1U;
    found_truncated.state.loaded_data_offset = 0x1111U;
    found_truncated.state.window_loaded = true;
    write_u16(found_truncated.state.window, 0x7FFCU, OP_12_SET_ROLE_POSITION);
    write_u16(found_truncated.state.window, 0x7FFEU, 0x00F8U);
    found_truncated.roles[1].flags = 0x00080000U;
    found_truncated.roles[1].action.cached_base_variant = 7U;
    found_truncated.state.previous_opcode = 0x55U;
    const auto found_truncated_result = found_truncated.step();
    test.expect_true(
        found_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_truncated.context.instruction_offset == 0x7FFCU &&
            found_truncated.state.previous_opcode == 0x55U &&
            found_truncated.roles[1].action.cached_base_variant ==
                0xFFFFFFFFU &&
            (found_truncated.roles[1].flags & 0x00080000U) == 0U,
        "opcode 12 checks found-role operands after the original cache reset"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    write_u16(missing_truncated.state.window, 0x7FFCU, OP_12_SET_ROLE_POSITION);
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    missing_truncated.state.previous_opcode = 0x55U;
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
            LegacyWorldStoryVmStatus::operand_out_of_range,
        "opcode 12 missing short status"
    );
    test.expect_true(
        missing_truncated.context.instruction_offset == 0x8004U,
        "opcode 12 missing short instruction offset"
    );
    test.expect_true(
        missing_truncated.state.previous_opcode == OP_12_SET_ROLE_POSITION,
        "opcode 12 missing short previous"
    );
}

void test_step_role_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_13_STEP_ROLE,
        static_cast<u16>(OP_13_STEP_ROLE | 0x4000U),
        static_cast<u16>(OP_13_STEP_ROLE | 0x8000U),
        static_cast<u16>(OP_13_STEP_ROLE | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        const auto scheduled =
            openswd3::world_map::schedule_legacy_world_story_path(
                paths.runtime,
                openswd3::world_map::LegacyWorldStoryPathRequest{
                    .role_index = 1U,
                    .destination_x = 336U,
                    .destination_y = 240U,
                }
            );
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status ==
                    openswd3::world_map::LegacyWorldStoryPathStatus::
                        completed &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_13_STEP_ROLE &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_13_STEP_ROLE &&
                read_u16(slot, 0x02U) == 0U && read_u16(slot, 0x16U) == 4U &&
                read_u16(slot, 0x18U) == 0U &&
                (fixture.roles[1].flags & 0x40000000U) != 0U,
            "opcode 13 aliases arm one path step, service audio, and yield"
        );
    }

    Fixture current_alias;
    current_alias.roles[1].flags = 0x02000000U;
    prime_loaded_instruction(current_alias, OP_13_STEP_ROLE);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    const auto current_alias_result = current_alias.step();
    test.expect_true(
        current_alias_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_alias_result.direct_audio_service_count == 1U &&
            current_alias.context.instruction_offset == 4U &&
            current_alias.state.previous_opcode == OP_13_STEP_ROLE &&
            current_alias.roles[1].flags == 0x02000000U,
        "opcode 13 substitutes FFF0 and skips an already stepped role"
    );

    Fixture no_slot;
    StoryPathHarness no_slot_paths{no_slot};
    prime_loaded_instruction(no_slot, OP_13_STEP_ROLE);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::yielded &&
            no_slot_result.direct_audio_service_count == 1U &&
            no_slot.context.instruction_offset == 4U &&
            no_slot.state.previous_opcode == OP_13_STEP_ROLE &&
            std::ranges::all_of(
                no_slot.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ),
        "opcode 13 ignores the helper's no matching slot return"
    );

    Fixture arrived;
    StoryPathHarness arrived_paths{arrived};
    auto& arrived_slot = arrived.active_object_slots[0].bytes;
    arrived_slot.fill(0xFFU);
    arrived_slot[0x00U] = 1U;
    arrived_slot[0x01U] = 0U;
    arrived_slot[0x02U] = 0U;
    arrived_slot[0x03U] = 0U;
    arrived_slot[0x1BU] = 2U;
    arrived_slot[0x1CU] = 0xFFU;
    arrived.roles[1].path_wait_remaining = 7U;
    arrived.roles[1].flags = 0x44000000U;
    prime_loaded_instruction(arrived, OP_13_STEP_ROLE);
    write_u16(arrived.state.window, 2U, 0x00F8U);
    const auto arrived_result = arrived.step();
    test.expect_true(
        arrived_result.status == LegacyWorldStoryVmStatus::yielded &&
            arrived_result.direct_audio_service_count == 1U &&
            arrived.roles[1].path_wait_remaining == 0U &&
            arrived.roles[1].flags == 0U &&
            arrived.context.instruction_offset == 4U &&
            arrived.state.previous_opcode == OP_13_STEP_ROLE,
        "opcode 13 ignores the helper's arrived return after its side effects"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_13_STEP_ROLE);
    write_u16(missing.state.window, 2U, 0x7777U);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.direct_audio_service_count == 1U &&
            missing.context.instruction_offset == 4U &&
            missing.state.previous_opcode == OP_13_STEP_ROLE,
        "opcode 13 consumes an ordinary missing role and yields"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_13_STEP_ROLE);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.direct_audio_service_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 13 stops before an invalid controlled-role flag read"
    );

    Fixture unavailable;
    prime_loaded_instruction(unavailable, OP_13_STEP_ROLE);
    write_u16(unavailable.state.window, 2U, 0x00F8U);
    unavailable.state.previous_opcode = 0x55U;
    const auto unavailable_result = unavailable.step();
    test.expect_true(
        unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            unavailable_result.direct_audio_service_count == 0U &&
            unavailable.context.instruction_offset == 0U &&
            unavailable.state.previous_opcode == 0x55U,
        "opcode 13 stops at the helper call when path runtime is unavailable"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, OP_13_STEP_ROLE);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U,
        "opcode 13 checks its selector before any side effect"
    );
}

void test_wait_role_action_status_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_14_WAIT_ROLE_ACTION_STATUS,
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0x4000U),
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0x8000U),
        static_cast<u16>(OP_14_WAIT_ROLE_ACTION_STATUS | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        fixture.roles[1].interaction_gate = 2U;
        fixture.state.previous_opcode = 0x55U;
        const auto waiting = fixture.step();
        const u16 waiting_offset = fixture.context.instruction_offset;
        fixture.roles[1].interaction_gate = 0U;
        const auto completed = fixture.step();
        test.expect_true(
            waiting.status == LegacyWorldStoryVmStatus::yielded &&
                waiting.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
                waiting.executed_instruction_count == 1U &&
                waiting.direct_audio_service_count == 1U &&
                waiting_offset == 0U &&
                completed.status == LegacyWorldStoryVmStatus::yielded &&
                completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
                completed.executed_instruction_count == 1U &&
                completed.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
            "opcode 14 aliases yield until role action status clears"
        );
    }

    Fixture current_alias;
    prime_loaded_instruction(current_alias, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(current_alias.state.window, 2U, 0xFFF0U);
    current_alias.roles[1].interaction_gate = 0U;
    const auto current_alias_result = current_alias.step();
    test.expect_true(
        current_alias_result.status == LegacyWorldStoryVmStatus::yielded &&
            current_alias_result.direct_audio_service_count == 1U &&
            current_alias.context.instruction_offset == 4U &&
            current_alias.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 resolves FFF0 before reading role action status"
    );

    Fixture context_alias;
    context_alias.context.source_guid = 0xFFFDU;
    context_alias.context.field_26 = 1U;
    prime_loaded_instruction(context_alias, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(context_alias.state.window, 2U, 0xFFF0U);
    const auto context_waiting = context_alias.step();
    const u16 context_waiting_offset = context_alias.context.instruction_offset;
    context_alias.context.field_26 = 0U;
    const auto context_completed = context_alias.step();
    test.expect_true(
        context_waiting.status == LegacyWorldStoryVmStatus::yielded &&
            context_waiting.direct_audio_service_count == 1U &&
            context_waiting_offset == 0U &&
            context_completed.status == LegacyWorldStoryVmStatus::yielded &&
            context_completed.direct_audio_service_count == 1U &&
            context_alias.context.instruction_offset == 4U &&
            context_alias.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 recognizes FFFD after FFF0 source substitution"
    );

    Fixture direct_context;
    direct_context.context.field_26 = 0U;
    prime_loaded_instruction(direct_context, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(direct_context.state.window, 2U, 0xFFFDU);
    const auto direct_context_result = direct_context.step();
    test.expect_true(
        direct_context_result.status == LegacyWorldStoryVmStatus::yielded &&
            direct_context_result.direct_audio_service_count == 1U &&
            direct_context.context.instruction_offset == 4U &&
            direct_context.state.previous_opcode ==
                OP_14_WAIT_ROLE_ACTION_STATUS,
        "opcode 14 reads direct FFFD context action status"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing.state.window, 2U, 0x7777U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.direct_audio_service_count == 0U &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 14 stops at an ordinary missing-role status read"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.direct_audio_service_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 14 stops before an invalid controlled-role status read"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFEU, OP_14_WAIT_ROLE_ACTION_STATUS);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U,
        "opcode 14 checks its selector before any side effect"
    );
}

void test_jump_same_file_offset_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_15_JUMP_SAME_FILE_OFFSET,
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0x4000U),
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0x8000U),
        static_cast<u16>(OP_15_JUMP_SAME_FILE_OFFSET | 0xC000U)
    };
    constexpr u32 target = 0x12345678U;
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u32(fixture.state.window, 2U, target);
        write_u16(fixture.ports.transferred_window, 0U, 59U);
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 59U &&
                result.executed_instruction_count == 2U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == target &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.loaded_file_number == 1U &&
                fixture.state.loaded_data_offset == target &&
                fixture.state.window_loaded &&
                fixture.state.previous_opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.last_data_file_number == 1U &&
                fixture.ports.last_data_offset == target &&
                !fixture.ports.last_data_clear_before_read &&
                fixture.ports.sound_effect_requests ==
                    std::vector<u16>{0x1234U} &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
            "opcode 15 aliases service audio, load, publish, and same-call fetch"
        );
    }

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u32(load_failure.state.window, 2U, 0x87654321U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x87654321U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 15 checked I/O failure preserves earlier legacy side effects"
    );

    Fixture boundary;
    boundary.context.instruction_offset = 0x7FFAU;
    boundary.context.talk_data_offset = 0x1111U;
    boundary.state.loaded_file_number = 1U;
    boundary.state.loaded_data_offset = 0x1111U;
    boundary.state.window_loaded = true;
    write_u16(boundary.state.window, 0x7FFAU, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u32(boundary.state.window, 0x7FFCU, 0x33445566U);
    write_u16(boundary.ports.transferred_window, 0U, 59U);
    write_u16(boundary.ports.transferred_window, 2U, 0x4321U);
    const auto boundary_result = boundary.step();
    test.expect_true(
        boundary_result.status == LegacyWorldStoryVmStatus::yielded &&
            boundary_result.opcode == 59U &&
            boundary_result.executed_instruction_count == 2U &&
            boundary.context.talk_data_offset == 0x33445566U &&
            boundary.context.instruction_offset == 4U,
        "opcode 15 accepts an exact six-byte window-tail payload"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(truncated.state.window, 0x7FFCU, OP_15_JUMP_SAME_FILE_OFFSET);
    write_u16(truncated.state.window, 0x7FFEU, 0x5566U);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.direct_audio_service_count == 0U &&
            truncated.context.talk_data_offset == 0x1111U &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U,
        "opcode 15 checks its u32 target before helper side effects"
    );
}

void test_jump_if_role_path_unprepared_protocol(openswd3::test::Context& test) {
    const auto configure_slot = [](Fixture& fixture, const u16 role_index) {
        write_u16(fixture.active_object_slots[0].bytes, 0U, role_index);
        fixture.active_object_slots[0].bytes[0x1BU] = 2U;
    };
    const auto configure_jump = [](Fixture& fixture, const u32 target) {
        write_u32(fixture.state.window, 4U, target);
        write_u16(fixture.ports.transferred_window, 0U, 59U);
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
    };

    constexpr std::array<u16, 4U> raw_aliases{
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED,
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0x4000U),
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0x8000U),
        static_cast<u16>(OP_16_JUMP_IF_ROLE_PATH_UNPREPARED | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        configure_jump(fixture, 0x12345678U);
        configure_slot(fixture, 1U);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 59U &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
            "opcode 16 aliases jump when a type-2 role path is unprepared"
        );
    }

    Fixture no_slot;
    prime_loaded_instruction(no_slot, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    write_u32(no_slot.state.window, 4U, 0x12345678U);
    write_u16(no_slot.state.window, 8U, 59U);
    write_u16(no_slot.state.window, 10U, 0x2345U);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::yielded &&
            no_slot_result.opcode == 59U &&
            no_slot_result.executed_instruction_count == 2U &&
            no_slot_result.direct_audio_service_count == 0U &&
            no_slot.context.talk_data_offset == 0x1111U &&
            no_slot.context.instruction_offset == 12U &&
            no_slot.state.previous_opcode ==
                OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            no_slot.ports.data_load_count == 0U,
        "opcode 16 advances eight bytes when no type-2 role path exists"
    );

    Fixture prepared;
    prepared.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(prepared, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(prepared.state.window, 2U, 0x00F8U);
    write_u32(prepared.state.window, 4U, 0x12345678U);
    write_u16(prepared.state.window, 8U, 59U);
    write_u16(prepared.state.window, 10U, 0x3456U);
    configure_slot(prepared, 1U);
    const auto prepared_result = prepared.step();
    test.expect_true(
        prepared_result.status == LegacyWorldStoryVmStatus::yielded &&
            prepared_result.opcode == 59U &&
            prepared_result.direct_audio_service_count == 0U &&
            prepared.context.instruction_offset == 12U &&
            prepared.ports.data_load_count == 0U,
        "opcode 16 does not jump after the role path step is prepared"
    );

    Fixture ordinary_missing;
    prime_loaded_instruction(
        ordinary_missing, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(ordinary_missing.state.window, 2U, 0x7777U);
    write_u32(ordinary_missing.state.window, 4U, 0x22223333U);
    write_u16(ordinary_missing.state.window, 8U, 59U);
    write_u16(ordinary_missing.state.window, 10U, 0x2345U);
    configure_slot(ordinary_missing, 0U);
    const auto ordinary_missing_result = ordinary_missing.step();
    test.expect_true(
        ordinary_missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            ordinary_missing_result.opcode == 59U &&
            ordinary_missing.context.talk_data_offset == 0x1111U &&
            ordinary_missing.context.instruction_offset == 12U &&
            ordinary_missing.ports.data_load_count == 0U,
        "opcode 16 preserves resolver miss output FFFFFFFF"
    );

    Fixture raw_current_token;
    prime_loaded_instruction(
        raw_current_token, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u32(raw_current_token.state.window, 4U, 0x33334444U);
    write_u16(raw_current_token.state.window, 8U, 59U);
    write_u16(raw_current_token.state.window, 10U, 0x3456U);
    configure_slot(raw_current_token, 1U);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status == LegacyWorldStoryVmStatus::yielded &&
            raw_current_token_result.opcode == 59U &&
            raw_current_token.context.talk_data_offset == 0x1111U &&
            raw_current_token.context.instruction_offset == 12U &&
            raw_current_token.ports.data_load_count == 0U,
        "opcode 16 passes FFF0 raw to the resolver without source substitution"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u32(invalid_controlled.state.window, 4U, 0x44445555U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.data_load_count == 0U,
        "opcode 16 obeys the VM controlled-role entry safety boundary"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_16_JUMP_IF_ROLE_PATH_UNPREPARED);
    write_u16(load_failure.state.window, 2U, 0x00F8U);
    configure_jump(load_failure, 0x66667777U);
    configure_slot(load_failure, 1U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_seek_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x66667777U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            !load_failure.state.window_loaded,
        "opcode 16 branch preserves helper effects before checked I/O failure"
    );

    Fixture branch_truncated;
    branch_truncated.context.instruction_offset = 0x7FFCU;
    branch_truncated.context.talk_data_offset = 0x1111U;
    branch_truncated.state.loaded_file_number = 1U;
    branch_truncated.state.loaded_data_offset = 0x1111U;
    branch_truncated.state.window_loaded = true;
    write_u16(
        branch_truncated.state.window,
        0x7FFCU,
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(branch_truncated, 1U);
    branch_truncated.state.previous_opcode = 0x55U;
    const auto branch_truncated_result = branch_truncated.step();
    test.expect_true(
        branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            branch_truncated.context.instruction_offset == 0x7FFCU &&
            branch_truncated.state.previous_opcode == 0x55U &&
            branch_truncated.ports.data_load_count == 0U,
        "opcode 16 branch reads the u32 target only after its slot predicate"
    );

    Fixture no_branch_truncated;
    no_branch_truncated.context.instruction_offset = 0x7FFCU;
    no_branch_truncated.context.talk_data_offset = 0x1111U;
    no_branch_truncated.state.loaded_file_number = 1U;
    no_branch_truncated.state.loaded_data_offset = 0x1111U;
    no_branch_truncated.state.window_loaded = true;
    write_u16(
        no_branch_truncated.state.window,
        0x7FFCU,
        OP_16_JUMP_IF_ROLE_PATH_UNPREPARED
    );
    write_u16(no_branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto no_branch_truncated_result = no_branch_truncated.step();
    test.expect_true(
        no_branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_branch_truncated_result.executed_instruction_count == 1U &&
            no_branch_truncated.context.instruction_offset == 0x8004U &&
            no_branch_truncated.state.previous_opcode ==
                OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            no_branch_truncated.ports.data_load_count == 0U,
        "opcode 16 no-branch path advances without reading an absent target"
    );
}

void test_jump_if_role_path_prepared_protocol(openswd3::test::Context& test) {
    const auto configure_slot = [](Fixture& fixture) {
        write_u16(fixture.active_object_slots[0].bytes, 0U, 1U);
        fixture.active_object_slots[0].bytes[0x1BU] = 2U;
    };
    constexpr std::array<u16, 4U> raw_aliases{
        OP_17_JUMP_IF_ROLE_PATH_PREPARED,
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0x4000U),
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0x8000U),
        static_cast<u16>(OP_17_JUMP_IF_ROLE_PATH_PREPARED | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0x40000000U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u32(fixture.state.window, 4U, 0x12345678U);
        write_u16(fixture.ports.transferred_window, 0U, 59U);
        write_u16(fixture.ports.transferred_window, 2U, 0x1234U);
        configure_slot(fixture);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == 59U &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
                fixture.ports.data_load_count == 1U,
            "opcode 17 aliases jump when a type-2 role path is prepared"
        );
    }

    Fixture unprepared;
    prime_loaded_instruction(unprepared, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(unprepared.state.window, 2U, 0x00F8U);
    write_u32(unprepared.state.window, 4U, 0x22223333U);
    write_u16(unprepared.state.window, 8U, 59U);
    write_u16(unprepared.state.window, 10U, 0x2345U);
    configure_slot(unprepared);
    const auto unprepared_result = unprepared.step();
    test.expect_true(
        unprepared_result.status == LegacyWorldStoryVmStatus::yielded &&
            unprepared_result.opcode == 59U &&
            unprepared_result.direct_audio_service_count == 0U &&
            unprepared.context.instruction_offset == 12U &&
            unprepared.state.previous_opcode ==
                OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            unprepared.ports.data_load_count == 0U,
        "opcode 17 advances when the matching role path is unprepared"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u32(missing.state.window, 4U, 0x33334444U);
    write_u16(missing.state.window, 8U, 59U);
    write_u16(missing.state.window, 10U, 0x3456U);
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing_result.opcode == 59U &&
            missing.context.instruction_offset == 12U &&
            missing.ports.data_load_count == 0U,
        "opcode 17 resolver miss output FFFFFFFF takes no-branch"
    );

    Fixture raw_current_token;
    raw_current_token.context.source_guid = 0x00F8U;
    raw_current_token.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(
        raw_current_token, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u32(raw_current_token.state.window, 4U, 0x33334444U);
    write_u16(raw_current_token.state.window, 8U, 59U);
    write_u16(raw_current_token.state.window, 10U, 0x3456U);
    configure_slot(raw_current_token);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status == LegacyWorldStoryVmStatus::yielded &&
            raw_current_token_result.opcode == 59U &&
            raw_current_token.context.instruction_offset == 12U &&
            raw_current_token.ports.data_load_count == 0U,
        "opcode 17 passes FFF0 raw without source substitution"
    );

    Fixture wrong_type;
    wrong_type.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(wrong_type, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(wrong_type.state.window, 2U, 0x00F8U);
    write_u32(wrong_type.state.window, 4U, 0x33334444U);
    write_u16(wrong_type.state.window, 8U, 59U);
    write_u16(wrong_type.state.window, 10U, 0x3456U);
    configure_slot(wrong_type);
    wrong_type.active_object_slots[0].bytes[0x1BU] = 3U;
    const auto wrong_type_result = wrong_type.step();
    test.expect_true(
        wrong_type_result.status == LegacyWorldStoryVmStatus::yielded &&
            wrong_type_result.opcode == 59U &&
            wrong_type.context.instruction_offset == 12U &&
            wrong_type.ports.data_load_count == 0U,
        "opcode 17 requires active-slot type low nibble two"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.data_load_count == 0U,
        "opcode 17 obeys the VM controlled-role entry safety boundary"
    );

    Fixture load_failure;
    load_failure.roles[1].flags = 0x40000000U;
    prime_loaded_instruction(load_failure, OP_17_JUMP_IF_ROLE_PATH_PREPARED);
    write_u16(load_failure.state.window, 2U, 0x00F8U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    configure_slot(load_failure);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.state.previous_opcode ==
                OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            !load_failure.state.window_loaded,
        "opcode 17 preserves branch helper effects before I/O failure"
    );

    Fixture truncated;
    truncated.roles[1].flags = 0x40000000U;
    truncated.context.instruction_offset = 0x7FFCU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    write_u16(
        truncated.state.window, 0x7FFCU, OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(truncated);
    truncated.state.previous_opcode = 0x55U;
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFCU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U,
        "opcode 17 prepared branch reads the target at its original danger point"
    );

    Fixture no_branch_truncated;
    no_branch_truncated.context.instruction_offset = 0x7FFCU;
    no_branch_truncated.context.talk_data_offset = 0x1111U;
    no_branch_truncated.state.loaded_file_number = 1U;
    no_branch_truncated.state.loaded_data_offset = 0x1111U;
    no_branch_truncated.state.window_loaded = true;
    write_u16(
        no_branch_truncated.state.window,
        0x7FFCU,
        OP_17_JUMP_IF_ROLE_PATH_PREPARED
    );
    write_u16(no_branch_truncated.state.window, 0x7FFEU, 0x00F8U);
    configure_slot(no_branch_truncated);
    no_branch_truncated.state.previous_opcode = 0x55U;
    const auto no_branch_truncated_result = no_branch_truncated.step();
    test.expect_true(
        no_branch_truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_branch_truncated_result.executed_instruction_count == 1U &&
            no_branch_truncated.context.instruction_offset == 0x8004U &&
            no_branch_truncated.state.previous_opcode ==
                OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            no_branch_truncated.ports.data_load_count == 0U,
        "opcode 17 unprepared no-branch advances before the next fetch fails"
    );
}

void test_role_action_chain_update_gate(openswd3::test::Context& test) {
    const auto run_chain = [](const u16 second_opcode) {
        Fixture fixture;
        auto script = std::span<u8>{fixture.ports.initial_window};
        write_u16(script, 0U, 10U);
        write_u16(script, 2U, 0x00F8U);
        write_u16(script, 4U, 2U);
        write_u16(script, 6U, second_opcode);
        write_u16(script, 8U, 0x00F8U);
        write_u16(script, 10U, 3U);
        write_u16(script, 12U, OP_14_WAIT_ROLE_ACTION_STATUS);
        write_u16(script, 14U, 0x00F8U);
        const auto result = fixture.step();
        return std::tuple{result, fixture.roles[1]};
    };

    const auto [plain_result, plain_role] = run_chain(11U);
    const auto [flagged_result, flagged_role] = run_chain(0x400BU);
    test.expect_true(
        plain_result.status == LegacyWorldStoryVmStatus::yielded &&
            plain_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            plain_result.action_update_count == 2U &&
            plain_role.action.base_variant == 2U &&
            plain_role.action.variant_delta == 3U &&
            (plain_role.flags & 0x00001000U) != 0U,
        "opcodes 10 and 11 coalesce a same-role raw action chain"
    );
    test.expect_true(
        flagged_result.status == LegacyWorldStoryVmStatus::yielded &&
            flagged_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            flagged_result.action_update_count == 3U &&
            flagged_role.action.base_variant == 2U &&
            flagged_role.action.variant_delta == 3U,
        "sub_42E740 compares the next raw opcode without masking flag bits"
    );

    Fixture next_opcode_truncated;
    next_opcode_truncated.context.instruction_offset = 0x7FFAU;
    next_opcode_truncated.context.talk_data_offset = 0x1111U;
    next_opcode_truncated.state.loaded_file_number = 1U;
    next_opcode_truncated.state.loaded_data_offset = 0x1111U;
    next_opcode_truncated.state.window_loaded = true;
    next_opcode_truncated.roles[1].action.base_variant = 9U;
    next_opcode_truncated.roles[1].action.wait_remaining = 7U;
    write_u16(next_opcode_truncated.state.window, 0x7FFAU, 10U);
    write_u16(next_opcode_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(next_opcode_truncated.state.window, 0x7FFEU, 2U);
    next_opcode_truncated.state.previous_opcode = 0x55U;
    const auto next_opcode_truncated_result = next_opcode_truncated.step();
    test.expect_true(
        next_opcode_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_opcode_truncated_result.executed_instruction_count == 1U &&
            next_opcode_truncated_result.action_update_count == 0U &&
            next_opcode_truncated.roles[1].action.base_variant == 2U &&
            next_opcode_truncated.roles[1].action.wait_remaining == 0U &&
            next_opcode_truncated.context.instruction_offset == 0x7FFAU &&
            next_opcode_truncated.state.previous_opcode == 0x55U,
        "opcode 10 writes action fields before mandatory lookahead opcode access"
    );

    Fixture next_selector_truncated;
    next_selector_truncated.context.instruction_offset = 0x7FF8U;
    next_selector_truncated.context.talk_data_offset = 0x1111U;
    next_selector_truncated.state.loaded_file_number = 1U;
    next_selector_truncated.state.loaded_data_offset = 0x1111U;
    next_selector_truncated.state.window_loaded = true;
    next_selector_truncated.roles[1].action.variant_delta = 9U;
    next_selector_truncated.roles[1].action.wait_remaining = 7U;
    next_selector_truncated.roles[1].flags = 0x20U;
    write_u16(next_selector_truncated.state.window, 0x7FF8U, 11U);
    write_u16(next_selector_truncated.state.window, 0x7FFAU, 0x00F8U);
    write_u16(next_selector_truncated.state.window, 0x7FFCU, 3U);
    write_u16(
        next_selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    next_selector_truncated.state.previous_opcode = 0x55U;
    const auto next_selector_truncated_result = next_selector_truncated.step();
    test.expect_true(
        next_selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_selector_truncated_result.action_update_count == 0U &&
            next_selector_truncated.roles[1].action.variant_delta == 3U &&
            next_selector_truncated.roles[1].action.wait_remaining == 0U &&
            next_selector_truncated.roles[1].flags == 0x20U &&
            next_selector_truncated.context.instruction_offset == 0x7FF8U &&
            next_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 11 reads a recognized next selector before refresh and flags"
    );
}

void test_change_requested_action_id(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> aliases{
        OP_45_SET_ROLE_ACTION_ID,
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x4000U),
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x8000U),
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0xC000U),
    };
    for (const u16 raw_word : aliases) {
        Fixture fixture;
        fixture.roles[1].flags = 0xA4A50020U;
        fixture.roles[1].action.action_id = 0xDEADBEEFU;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8001U);
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.roles[1].action.action_id == 0x00008001U &&
                fixture.roles[1].flags == 0xA4A51020U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 45 aliases write a zero-extended action id and set bit 12"
        );
    }

    Fixture current_source;
    current_source.roles[1].action.action_id = 0xDEADBEEFU;
    prime_loaded_instruction(current_source, OP_45_SET_ROLE_ACTION_ID);
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 0xFFFFU);
    write_u16(current_source.state.window, 6U, OP_1025);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            current_source.roles[1].action.action_id == 0x0000FFFFU &&
            current_source.roles[0].action.action_id == 0U &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 45 translates FFF0 only for the current role lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.action_id = 0x1111U;
    controlled.roles[2].action.action_id = 0x2222U;
    prime_loaded_instruction(controlled, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 0x8123U);
    write_u16(controlled.state.window, 6U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.roles[2].action.action_id == 0x8123U &&
            controlled.roles[1].action.action_id == 0x1111U &&
            (controlled.roles[2].flags & 0x1000U) != 0U,
        "opcode 45 passes FFFE through for controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[1].action.action_id = 0x1111U;
    first_clear_match.roles[2].action.action_id = 0x2222U;
    prime_loaded_instruction(first_clear_match, OP_45_SET_ROLE_ACTION_ID);
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, 0x8004U);
    write_u16(first_clear_match.state.window, 6U, OP_1025);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            first_clear_match.roles[0].action.action_id == 0U &&
            first_clear_match.roles[1].action.action_id == 0x8004U &&
            first_clear_match.roles[2].action.action_id == 0x2222U,
        "opcode 45 skips bit-28 roles and uses the first clear GUID match"
    );

    Fixture chained;
    prime_loaded_instruction(chained, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 2U, 0x00F8U);
    write_u16(chained.state.window, 4U, 0x0222U);
    write_u16(chained.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(chained.state.window, 8U, 0x00F8U);
    write_u16(chained.state.window, 10U, 0U);
    write_u16(chained.state.window, 12U, OP_1025);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            chained_result.executed_instruction_count == 3U &&
            chained_result.action_update_count == 1U &&
            chained.roles[1].action.action_id == 0U &&
            (chained.roles[1].flags & 0x1000U) != 0U &&
            chained.context.instruction_offset == 12U &&
            chained.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 coalesces an exact raw same-role chain and writes zero"
    );

    const auto run_field_chain = [](const u16 next_opcode) {
        Fixture fixture;
        prime_loaded_instruction(fixture, OP_45_SET_ROLE_ACTION_ID);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x0111U);
        write_u16(fixture.state.window, 6U, next_opcode);
        write_u16(fixture.state.window, 8U, 0x00F8U);
        write_u16(fixture.state.window, 10U, 3U);
        write_u16(fixture.state.window, 12U, OP_1025);
        const auto result = fixture.step();
        return std::tuple{result, fixture.roles[1]};
    };
    const auto [base_chain_result, base_chain_role] =
        run_field_chain(OP_10_SET_ROLE_BASE_VARIANT);
    const auto [delta_chain_result, delta_chain_role] = run_field_chain(11U);
    test.expect_true(
        base_chain_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            base_chain_result.executed_instruction_count == 3U &&
            base_chain_result.action_update_count == 1U &&
            base_chain_role.action.action_id == 0x0111U &&
            base_chain_role.action.base_variant == 3U,
        "opcode 45 coalesces a same-role raw opcode 10 successor"
    );
    test.expect_true(
        delta_chain_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            delta_chain_result.executed_instruction_count == 3U &&
            delta_chain_result.action_update_count == 1U &&
            delta_chain_role.action.action_id == 0x0111U &&
            delta_chain_role.action.variant_delta == 3U,
        "opcode 45 coalesces a same-role raw opcode 11 successor"
    );

    Fixture aliased_next;
    prime_loaded_instruction(aliased_next, OP_45_SET_ROLE_ACTION_ID);
    write_u16(aliased_next.state.window, 2U, 0x00F8U);
    write_u16(aliased_next.state.window, 4U, 0x0111U);
    write_u16(
        aliased_next.state.window,
        6U,
        static_cast<u16>(OP_45_SET_ROLE_ACTION_ID | 0x4000U)
    );
    write_u16(aliased_next.state.window, 8U, 0x00F8U);
    write_u16(aliased_next.state.window, 10U, 0x0222U);
    write_u16(aliased_next.state.window, 12U, OP_1025);
    const auto aliased_next_result = aliased_next.step();
    test.expect_true(
        aliased_next_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            aliased_next_result.action_update_count == 2U &&
            aliased_next.roles[1].action.action_id == 0x0222U,
        "opcode 45 lookahead compares the next raw opcode without alias masking"
    );

    Fixture untranslated_next;
    prime_loaded_instruction(untranslated_next, OP_45_SET_ROLE_ACTION_ID);
    write_u16(untranslated_next.state.window, 2U, 0x00F8U);
    write_u16(untranslated_next.state.window, 4U, 0x0111U);
    write_u16(untranslated_next.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(untranslated_next.state.window, 8U, 0xFFF0U);
    write_u16(untranslated_next.state.window, 10U, 0x0222U);
    write_u16(untranslated_next.state.window, 12U, OP_1025);
    const auto untranslated_next_result = untranslated_next.step();
    test.expect_true(
        untranslated_next_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            untranslated_next_result.action_update_count == 2U &&
            untranslated_next.roles[1].action.action_id == 0x0222U,
        "opcode 45 lookahead does not translate the next FFF0 selector"
    );

    Fixture controlled_chain;
    prime_loaded_instruction(controlled_chain, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled_chain.state.window, 2U, 0xFFFEU);
    write_u16(controlled_chain.state.window, 4U, 0x0111U);
    write_u16(controlled_chain.state.window, 6U, OP_45_SET_ROLE_ACTION_ID);
    write_u16(controlled_chain.state.window, 8U, 0xFFFEU);
    write_u16(controlled_chain.state.window, 10U, 0x0222U);
    write_u16(controlled_chain.state.window, 12U, OP_1025);
    const auto controlled_chain_result = controlled_chain.step();
    test.expect_true(
        controlled_chain_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled_chain_result.action_update_count == 1U &&
            controlled_chain.roles[0].action.action_id == 0x0222U,
        "opcode 45 lookahead preserves FFFE controlled-role selection"
    );

    Fixture update_failure;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(update_failure, OP_45_SET_ROLE_ACTION_ID);
    write_u16(update_failure.state.window, 2U, 0x00F8U);
    write_u16(update_failure.state.window, 4U, 0x8005U);
    write_u16(update_failure.state.window, 6U, OP_1025);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.roles[1].action.action_id == 0x8005U &&
            (update_failure.roles[1].flags & 0x1000U) != 0U &&
            update_failure.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 refresh failure is diagnostic-only before bit 12 is set"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_45_SET_ROLE_ACTION_ID);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x0333U);
    write_u16(missing.state.window, 6U, OP_1025);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    const auto patch = missing.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            missing_result.executed_instruction_count == 2U &&
            missing_result.action_update_count == 0U &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x7777U && patch.action_id == 0x0333U &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.tile_x == 0xFFFFU && patch.tile_y == 0xFFFFU &&
            patch.talk_script_id == 0xFFFFU && patch.path_data_id == 0xFFFFU &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 missing role uses the exact MAPS action-and-flag patch"
    );
}

void test_change_requested_action_id_failure_ordering(
    openswd3::test::Context& test
) {
    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    selector_truncated.state.previous_opcode = 0x55U;
    const auto selector_truncated_result = selector_truncated.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated_result.executed_instruction_count == 1U &&
            selector_truncated_result.action_update_count == 0U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x55U,
        "opcode 45 stops at the first unsafe selector access"
    );

    Fixture found_value_truncated;
    found_value_truncated.context.instruction_offset = 0x7FFCU;
    found_value_truncated.context.talk_data_offset = 0x1111U;
    found_value_truncated.state.loaded_file_number = 1U;
    found_value_truncated.state.loaded_data_offset = 0x1111U;
    found_value_truncated.state.window_loaded = true;
    found_value_truncated.roles[1].action.action_id = 0xDEADBEEFU;
    write_u16(
        found_value_truncated.state.window, 0x7FFCU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(found_value_truncated.state.window, 0x7FFEU, 0x00F8U);
    found_value_truncated.state.previous_opcode = 0x55U;
    const auto found_value_truncated_result = found_value_truncated.step();
    test.expect_true(
        found_value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_value_truncated.roles[1].action.action_id == 0xDEADBEEFU &&
            found_value_truncated_result.action_update_count == 0U &&
            found_value_truncated.context.instruction_offset == 0x7FFCU &&
            found_value_truncated.state.previous_opcode == 0x55U,
        "opcode 45 finds the role before the unsafe action-id access"
    );

    Fixture missing_value_truncated;
    missing_value_truncated.context.instruction_offset = 0x7FFCU;
    missing_value_truncated.context.talk_data_offset = 0x1111U;
    missing_value_truncated.state.loaded_file_number = 1U;
    missing_value_truncated.state.loaded_data_offset = 0x1111U;
    missing_value_truncated.state.window_loaded = true;
    write_u16(
        missing_value_truncated.state.window, 0x7FFCU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(missing_value_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto missing_value_truncated_result = missing_value_truncated.step();
    test.expect_true(
        missing_value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_value_truncated.ports.role_patch_requests.empty() &&
            missing_value_truncated.context.instruction_offset == 0x7FFCU,
        "opcode 45 missing-role patch waits for the action-id read"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].flags = 0x20U;
    exact_tail.roles[1].action.action_id = 0xDEADBEEFU;
    write_u16(exact_tail.state.window, 0x7FFAU, OP_45_SET_ROLE_ACTION_ID);
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x8008U);
    exact_tail.state.previous_opcode = 0x55U;
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.action_update_count == 0U &&
            exact_tail.roles[1].action.action_id == 0x8008U &&
            exact_tail.roles[1].flags == 0x20U &&
            exact_tail.context.instruction_offset == 0x7FFAU &&
            exact_tail.state.previous_opcode == 0x55U,
        "opcode 45 writes action id before the mandatory next-opcode access"
    );

    Fixture next_selector_truncated;
    next_selector_truncated.context.instruction_offset = 0x7FF8U;
    next_selector_truncated.context.talk_data_offset = 0x1111U;
    next_selector_truncated.state.loaded_file_number = 1U;
    next_selector_truncated.state.loaded_data_offset = 0x1111U;
    next_selector_truncated.state.window_loaded = true;
    next_selector_truncated.roles[1].flags = 0x20U;
    write_u16(
        next_selector_truncated.state.window, 0x7FF8U, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(next_selector_truncated.state.window, 0x7FFAU, 0x00F8U);
    write_u16(next_selector_truncated.state.window, 0x7FFCU, 0x8009U);
    write_u16(
        next_selector_truncated.state.window, 0x7FFEU, OP_45_SET_ROLE_ACTION_ID
    );
    next_selector_truncated.state.previous_opcode = 0x55U;
    const auto next_selector_truncated_result = next_selector_truncated.step();
    test.expect_true(
        next_selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            next_selector_truncated_result.action_update_count == 0U &&
            next_selector_truncated.roles[1].action.action_id == 0x8009U &&
            next_selector_truncated.roles[1].flags == 0x20U &&
            next_selector_truncated.context.instruction_offset == 0x7FF8U &&
            next_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 45 reads a recognized next selector before refresh and bit 12"
    );

    Fixture unrecognized_tail;
    unrecognized_tail.context.instruction_offset = 0x7FF8U;
    unrecognized_tail.context.talk_data_offset = 0x1111U;
    unrecognized_tail.state.loaded_file_number = 1U;
    unrecognized_tail.state.loaded_data_offset = 0x1111U;
    unrecognized_tail.state.window_loaded = true;
    write_u16(
        unrecognized_tail.state.window, 0x7FF8U, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(unrecognized_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(unrecognized_tail.state.window, 0x7FFCU, 0x8010U);
    write_u16(unrecognized_tail.state.window, 0x7FFEU, OP_1025);
    const auto unrecognized_tail_result = unrecognized_tail.step();
    test.expect_true(
        unrecognized_tail_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            unrecognized_tail_result.executed_instruction_count == 2U &&
            unrecognized_tail_result.action_update_count == 1U &&
            unrecognized_tail.roles[1].action.action_id == 0x8010U &&
            (unrecognized_tail.roles[1].flags & 0x1000U) != 0U &&
            unrecognized_tail.context.instruction_offset == 0x7FFEU &&
            unrecognized_tail.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 unrecognized lookahead needs no following selector"
    );

    Fixture missing_exact_tail;
    missing_exact_tail.context.instruction_offset = 0x7FFAU;
    missing_exact_tail.context.talk_data_offset = 0x1111U;
    missing_exact_tail.state.loaded_file_number = 1U;
    missing_exact_tail.state.loaded_data_offset = 0x1111U;
    missing_exact_tail.state.window_loaded = true;
    write_u16(
        missing_exact_tail.state.window, 0x7FFAU, OP_45_SET_ROLE_ACTION_ID
    );
    write_u16(missing_exact_tail.state.window, 0x7FFCU, 0x7777U);
    write_u16(missing_exact_tail.state.window, 0x7FFEU, 0x0333U);
    const auto missing_exact_tail_result = missing_exact_tail.step();
    test.expect_true(
        missing_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            missing_exact_tail_result.executed_instruction_count == 1U &&
            missing_exact_tail_result.action_update_count == 0U &&
            missing_exact_tail.ports.role_patch_requests.size() == 1U &&
            missing_exact_tail.context.instruction_offset == 0x8000U &&
            missing_exact_tail.state.previous_opcode ==
                OP_45_SET_ROLE_ACTION_ID,
        "opcode 45 missing-role exact tail patches before the next fetch"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_45_SET_ROLE_ACTION_ID);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8011U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 45 invalid controlled owner stops at the VM session boundary"
    );
}

void test_restore_role_action_overrides_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime_role_instruction = [](Fixture& fixture,
                                           const u16 raw_word) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
    };

    for (const u16 mask : alias_masks) {
        Fixture restore_all;
        auto& action = restore_all.roles[1].action;
        action.action_id = 0xA0A0A0A0U;
        action.cached_action_id = 0x41414141U;
        action.base_variant = 0xB0B0B0B0U;
        action.cached_base_variant = 0x42424242U;
        action.variant_delta = 0xC0C0C0C0U;
        action.cached_variant_delta = 0x43434343U;
        action.mode_flags = 0x44444444U;
        action.field_1c = 0x11111111U;
        action.one_shot_base_variant = 0x22222222U;
        action.one_shot_variant_delta = 0x33333333U;
        action.packed_ap_state = 0x4545U;
        action.command_cursor = 0x4646U;
        action.wait_remaining = 0x4747U;
        action.wait_default = 0x4848U;
        action.wait_override = 0x4949U;
        action.field_4a = 0x4A4AU;
        action.field_8c = 0x4B4B4B4BU;
        action.external_mode = 0x4C4C4C4CU;
        prime_role_instruction(
            restore_all,
            static_cast<u16>(OP_46_RESTORE_ROLE_ACTION_OVERRIDES | mask)
        );
        const auto result = restore_all.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                action.action_id == 0x11111111U &&
                action.cached_action_id == 0x41414141U &&
                action.base_variant == 0x22222222U &&
                action.cached_base_variant == 0x42424242U &&
                action.variant_delta == 0x33333333U &&
                action.cached_variant_delta == 0x43434343U &&
                action.mode_flags == 0x44444444U &&
                action.field_1c == 0xFFFFFFFFU &&
                action.one_shot_base_variant == 0xFFFFFFFFU &&
                action.one_shot_variant_delta == 0xFFFFFFFFU &&
                action.packed_ap_state == 0x4545U &&
                action.command_cursor == 0U && action.wait_remaining == 0U &&
                action.wait_default == 0U && action.wait_override == 0U &&
                action.field_4a == 0x1111U && action.field_8c == 0x4B4B4B4BU &&
                action.external_mode == 0U &&
                restore_all.context.instruction_offset == 4U &&
                restore_all.state.previous_opcode ==
                    OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
                restore_all.ports.direct_audio_service_count == 0U,
            "opcode 46 aliases restore all targets then reset exact action fields"
        );

        Fixture base_override;
        auto& base_action = base_override.roles[1].action;
        base_action.base_variant = 0x11111111U;
        base_action.one_shot_base_variant = 0x89ABCDEFU;
        base_action.one_shot_variant_delta = 0x76543210U;
        base_action.wait_remaining = 0x1111U;
        base_action.wait_default = 0x2222U;
        base_action.wait_override = 0x3333U;
        prime_role_instruction(
            base_override,
            static_cast<u16>(OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE | mask)
        );
        const auto base_result = base_override.step();
        test.expect_true(
            base_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                base_result.executed_instruction_count == 2U &&
                base_result.action_update_count == 1U &&
                base_action.base_variant == 0x89ABCDEFU &&
                base_action.one_shot_base_variant == 0xFFFFFFFFU &&
                base_action.one_shot_variant_delta == 0x76543210U &&
                base_action.wait_remaining == 0x1111U &&
                base_action.wait_default == 0x2222U &&
                base_action.wait_override == 0x3333U &&
                base_override.context.instruction_offset == 4U &&
                base_override.state.previous_opcode ==
                    OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
                base_override.ports.direct_audio_service_count == 0U,
            "opcode 47 aliases apply the full pending base variant only"
        );

        Fixture delta_override;
        auto& delta_action = delta_override.roles[1].action;
        delta_action.variant_delta = 0x11111111U;
        delta_action.one_shot_base_variant = 0x76543210U;
        delta_action.one_shot_variant_delta = 0x89ABCDEFU;
        delta_action.wait_remaining = 0x1111U;
        delta_action.wait_default = 0x2222U;
        delta_action.wait_override = 0x3333U;
        prime_role_instruction(
            delta_override,
            static_cast<u16>(OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE | mask)
        );
        const auto delta_result = delta_override.step();
        test.expect_true(
            delta_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                delta_result.executed_instruction_count == 2U &&
                delta_result.action_update_count == 1U &&
                delta_action.variant_delta == 0x89ABCDEFU &&
                delta_action.one_shot_variant_delta == 0xFFFFFFFFU &&
                delta_action.one_shot_base_variant == 0x76543210U &&
                delta_action.wait_remaining == 0x1111U &&
                delta_action.wait_default == 0x2222U &&
                delta_action.wait_override == 0x3333U &&
                delta_override.context.instruction_offset == 4U &&
                delta_override.state.previous_opcode ==
                    OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
                delta_override.ports.direct_audio_service_count == 0U,
            "opcode 48 aliases apply the full pending variant delta only"
        );

        Fixture wait_override;
        auto& wait_action = wait_override.roles[1].action;
        wait_action.one_shot_base_variant = 0x11111111U;
        wait_action.one_shot_variant_delta = 0x22222222U;
        wait_action.wait_remaining = 0x3333U;
        wait_action.wait_default = 0x4444U;
        wait_action.wait_override = 0x5555U;
        prime_role_instruction(
            wait_override,
            static_cast<u16>(OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF | mask)
        );
        const auto wait_result = wait_override.step();
        test.expect_true(
            wait_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                wait_result.executed_instruction_count == 2U &&
                wait_result.action_update_count == 1U &&
                wait_action.one_shot_base_variant == 0x11111111U &&
                wait_action.one_shot_variant_delta == 0x22222222U &&
                wait_action.wait_remaining == 0x3333U &&
                wait_action.wait_default == 0x4444U &&
                wait_action.wait_override == 0xFFFFU &&
                wait_override.context.instruction_offset == 4U &&
                wait_override.state.previous_opcode ==
                    OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
                wait_override.ports.direct_audio_service_count == 0U,
            "opcode 49 aliases write only the wait-override word"
        );
    }

    constexpr std::array<u16, 3U> conditional_opcodes{
        OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE,
        OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE,
        OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF,
    };
    for (const u16 opcode : conditional_opcodes) {
        Fixture no_pending_value;
        auto& action = no_pending_value.roles[1].action;
        action.base_variant = 0x11111111U;
        action.variant_delta = 0x22222222U;
        action.one_shot_base_variant = 0xFFFFFFFFU;
        action.one_shot_variant_delta = 0xFFFFFFFFU;
        action.wait_remaining = 0x3333U;
        action.wait_default = 0x4444U;
        action.wait_override = 0xFFFFU;
        prime_role_instruction(no_pending_value, opcode);
        const auto result = no_pending_value.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.action_update_count == 1U &&
                action.base_variant == 0x11111111U &&
                action.variant_delta == 0x22222222U &&
                action.one_shot_base_variant == 0xFFFFFFFFU &&
                action.one_shot_variant_delta == 0xFFFFFFFFU &&
                action.wait_remaining == 0x3333U &&
                action.wait_default == 0x4444U &&
                action.wait_override == 0xFFFFU &&
                no_pending_value.context.instruction_offset == 4U &&
                no_pending_value.state.previous_opcode == opcode,
            "opcodes 47-49 refresh once even when their conditional write skips"
        );
    }

    Fixture restore_absent_values;
    auto& absent_action = restore_absent_values.roles[1].action;
    absent_action.action_id = 0x11111111U;
    absent_action.base_variant = 0x22222222U;
    absent_action.variant_delta = 0x33333333U;
    absent_action.field_1c = 0xFFFFFFFFU;
    absent_action.one_shot_base_variant = 0xFFFFFFFFU;
    absent_action.one_shot_variant_delta = 0xFFFFFFFFU;
    prime_role_instruction(
        restore_absent_values, OP_46_RESTORE_ROLE_ACTION_OVERRIDES
    );
    const auto restore_absent_result = restore_absent_values.step();
    test.expect_true(
        restore_absent_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            restore_absent_result.action_update_count == 1U &&
            absent_action.action_id == 0xFFFFFFFFU &&
            absent_action.base_variant == 0xFFFFFFFFU &&
            absent_action.variant_delta == 0xFFFFFFFFU,
        "opcode 46 unconditionally copies absent pending values"
    );

    Fixture literal_fff0;
    literal_fff0.roles[1].action.base_variant = 0x11111111U;
    literal_fff0.roles[1].action.one_shot_base_variant = 0x22222222U;
    literal_fff0.roles[2].guid = 0xFFF0U;
    literal_fff0.roles[2].action.base_variant = 0x33333333U;
    literal_fff0.roles[2].action.one_shot_base_variant = 0x44444444U;
    prime_loaded_instruction(
        literal_fff0, OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE
    );
    write_u16(literal_fff0.state.window, 2U, 0xFFF0U);
    write_u16(literal_fff0.state.window, 4U, OP_1025);
    const auto literal_fff0_result = literal_fff0.step();
    test.expect_true(
        literal_fff0_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            literal_fff0.roles[1].action.base_variant == 0x11111111U &&
            literal_fff0.roles[1].action.one_shot_base_variant == 0x22222222U &&
            literal_fff0.roles[2].action.base_variant == 0x44444444U &&
            literal_fff0.roles[2].action.one_shot_base_variant == 0xFFFFFFFFU,
        "opcodes 46-49 treat FFF0 as a literal GUID"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.one_shot_variant_delta = 0x11111111U;
    controlled.roles[2].action.one_shot_variant_delta = 0x22222222U;
    prime_loaded_instruction(
        controlled, OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE
    );
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.roles[1].action.variant_delta == 0U &&
            controlled.roles[1].action.one_shot_variant_delta == 0x11111111U &&
            controlled.roles[2].action.variant_delta == 0x22222222U &&
            controlled.roles[2].action.one_shot_variant_delta == 0xFFFFFFFFU,
        "opcodes 46-49 pass FFFE through for controlled-role selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[0].action.wait_override = 0x1111U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].action.wait_override = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].action.wait_override = 0x3333U;
    prime_loaded_instruction(
        first_clear_match, OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF
    );
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, OP_1025);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            first_clear_match.roles[0].action.wait_override == 0x1111U &&
            first_clear_match.roles[1].action.wait_override == 0xFFFFU &&
            first_clear_match.roles[2].action.wait_override == 0x3333U,
        "opcodes 46-49 skip bit-28 roles and use the first clear GUID match"
    );

    constexpr std::array<u16, 4U> opcodes{
        OP_46_RESTORE_ROLE_ACTION_OVERRIDES,
        OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE,
        OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE,
        OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF,
    };
    for (const u16 opcode : opcodes) {
        Fixture missing;
        prime_loaded_instruction(missing, opcode);
        write_u16(missing.state.window, 2U, 0x7777U);
        missing.state.previous_opcode = 0x55U;
        const auto result = missing.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::role_not_found &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.action_update_count == 0U &&
                missing.context.instruction_offset == 0U &&
                missing.state.previous_opcode == 0x55U &&
                missing.ports.role_patch_requests.empty(),
            "opcodes 46-49 stop at the first unsafe missing-role action access"
        );

        Fixture selector_truncated;
        selector_truncated.context.instruction_offset = 0x7FFEU;
        selector_truncated.context.talk_data_offset = 0x1111U;
        selector_truncated.state.loaded_file_number = 1U;
        selector_truncated.state.loaded_data_offset = 0x1111U;
        selector_truncated.state.window_loaded = true;
        write_u16(selector_truncated.state.window, 0x7FFEU, opcode);
        selector_truncated.state.previous_opcode = 0x55U;
        const auto truncated_result = selector_truncated.step();
        test.expect_true(
            truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                truncated_result.opcode == opcode &&
                truncated_result.executed_instruction_count == 1U &&
                truncated_result.action_update_count == 0U &&
                selector_truncated.context.instruction_offset == 0x7FFEU &&
                selector_truncated.state.previous_opcode == 0x55U,
            "opcodes 46-49 stop at the unsafe selector-word access"
        );

        Fixture exact_tail;
        auto& tail_action = exact_tail.roles[1].action;
        tail_action.action_id = 0x10U;
        tail_action.field_1c = 0x101U;
        tail_action.base_variant = 0x20U;
        tail_action.one_shot_base_variant = 0x202U;
        tail_action.variant_delta = 0x30U;
        tail_action.one_shot_variant_delta = 0x303U;
        tail_action.wait_override = 0x404U;
        exact_tail.context.instruction_offset = 0x7FFCU;
        exact_tail.context.talk_data_offset = 0x1111U;
        exact_tail.state.loaded_file_number = 1U;
        exact_tail.state.loaded_data_offset = 0x1111U;
        exact_tail.state.window_loaded = true;
        write_u16(exact_tail.state.window, 0x7FFCU, opcode);
        write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto tail_result = exact_tail.step();
        const bool tail_effect =
            (opcode == OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
             tail_action.action_id == 0x101U &&
             tail_action.base_variant == 0x202U &&
             tail_action.variant_delta == 0x303U &&
             tail_action.wait_override == 0U) ||
            (opcode == OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
             tail_action.base_variant == 0x202U &&
             tail_action.one_shot_base_variant == 0xFFFFFFFFU) ||
            (opcode == OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
             tail_action.variant_delta == 0x303U &&
             tail_action.one_shot_variant_delta == 0xFFFFFFFFU) ||
            (opcode == OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
             tail_action.wait_override == 0xFFFFU);
        test.expect_true(
            tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                tail_result.opcode == opcode &&
                tail_result.executed_instruction_count == 1U &&
                tail_result.action_update_count == 1U && tail_effect &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == opcode &&
                exact_tail.ports.direct_audio_service_count == 0U,
            "opcodes 46-49 exact-tail records complete before next fetch"
        );

        Fixture update_failure;
        auto& failed_action = update_failure.roles[1].action;
        failed_action.action_id = 0x10U;
        failed_action.field_1c = 0x101U;
        failed_action.base_variant = 0x20U;
        failed_action.one_shot_base_variant = 0x202U;
        failed_action.variant_delta = 0x30U;
        failed_action.one_shot_variant_delta = 0x303U;
        failed_action.wait_override = 0x404U;
        update_failure.ports.action_update_result = 0U;
        prime_role_instruction(update_failure, opcode);
        const auto failed_result = update_failure.step();
        const bool failed_effect =
            (opcode == OP_46_RESTORE_ROLE_ACTION_OVERRIDES &&
             failed_action.action_id == 0x101U &&
             failed_action.base_variant == 0x202U &&
             failed_action.variant_delta == 0x303U) ||
            (opcode == OP_47_APPLY_ROLE_BASE_VARIANT_OVERRIDE &&
             failed_action.base_variant == 0x202U) ||
            (opcode == OP_48_APPLY_ROLE_VARIANT_DELTA_OVERRIDE &&
             failed_action.variant_delta == 0x303U) ||
            (opcode == OP_49_SET_ROLE_ACTION_WAIT_OVERRIDE_FFFF &&
             failed_action.wait_override == 0xFFFFU);
        test.expect_true(
            failed_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                failed_result.action_update_count == 1U &&
                failed_result.action_update_failure_count == 1U &&
                failed_effect &&
                update_failure.context.instruction_offset == 4U &&
                update_failure.state.previous_opcode == opcode,
            "opcodes 46-49 refresh failure is diagnostic-only after effects"
        );

        Fixture invalid_controlled;
        prime_loaded_instruction(invalid_controlled, opcode);
        write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
        invalid_controlled.state.previous_opcode = 0x55U;
        const auto invalid_result = invalid_controlled.step(
            0, 0, static_cast<u32>(invalid_controlled.roles.size())
        );
        test.expect_true(
            invalid_result.status == LegacyWorldStoryVmStatus::role_not_found &&
                invalid_result.opcode == 0U &&
                invalid_result.executed_instruction_count == 0U &&
                invalid_result.action_update_count == 0U &&
                invalid_controlled.context.instruction_offset == 0U &&
                invalid_controlled.state.previous_opcode == 0x55U,
            "opcodes 46-49 invalid controlled owner stops before opcode fetch"
        );
    }
}

void test_start_camera_move_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        CameraMoveFixture relative;
        relative.camera.right = 800U;
        relative.camera.bottom = 800U;
        relative.camera_pan.remaining_x = 0x11111111;
        relative.camera_pan.remaining_y = 0x22222222;
        relative.camera_pan.step_x = 0x33333333;
        relative.camera_pan.step_y = 0x44444444;
        relative.state.previous_opcode = 0x55U;
        prime_long_camera_move(
            relative,
            static_cast<u16>(OP_50_START_RELATIVE_CAMERA_MOVE | mask),
            -3,
            2,
            8U,
            7U
        );
        const auto relative_result = relative.step(160, 320);
        test.expect_true(
            relative_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                relative_result.opcode == OP_1025 &&
                relative_result.executed_instruction_count == 2U &&
                relative.camera_pan.remaining_x == -48 &&
                relative.camera_pan.remaining_y == 32 &&
                relative.camera_pan.step_x == -8 &&
                relative.camera_pan.step_y == 4 &&
                relative.context.instruction_offset == 10U &&
                relative.state.previous_opcode ==
                    OP_50_START_RELATIVE_CAMERA_MOVE &&
                relative.ports.direct_audio_service_count == 0U,
            "opcode 50 aliases replace active motion and derive signed steps"
        );

        CameraMoveFixture absolute;
        absolute.camera.right = 800U;
        absolute.camera.bottom = 560U;
        absolute.camera_pan.remaining_x = 0x11111111;
        absolute.camera_pan.remaining_y = 0x22222222;
        absolute.camera_pan.step_x = 0x33333333;
        absolute.camera_pan.step_y = 0x44444444;
        absolute.state.previous_opcode = 0x55U;
        prime_long_camera_move(
            absolute,
            static_cast<u16>(OP_70_START_ABSOLUTE_CAMERA_MOVE | mask),
            7,
            9,
            6U,
            8U
        );
        const auto absolute_result = absolute.step(160, 80);
        test.expect_true(
            absolute_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                absolute_result.opcode == OP_1025 &&
                absolute_result.executed_instruction_count == 2U &&
                absolute.camera_pan.remaining_x == -48 &&
                absolute.camera_pan.remaining_y == 64 &&
                absolute.camera_pan.step_x == -6 &&
                absolute.camera_pan.step_y == 8 &&
                absolute.context.instruction_offset == 10U &&
                absolute.state.previous_opcode ==
                    OP_70_START_ABSOLUTE_CAMERA_MOVE &&
                absolute.ports.direct_audio_service_count == 0U,
            "opcode 70 aliases derive displacement from viewport tile origin"
        );

        CameraMoveFixture role_target;
        role_target.roles[1].world_x = 800U;
        role_target.roles[1].world_y = 640U;
        role_target.camera_pan.remaining_x = 0x11111111;
        role_target.camera_pan.remaining_y = 0x22222222;
        role_target.camera_pan.step_x = 0x33333333;
        role_target.camera_pan.step_y = 0x44444444;
        role_target.state.previous_opcode = 0x55U;
        prime_role_camera_move(
            role_target,
            static_cast<u16>(OP_73_START_CAMERA_MOVE_TO_ROLE | mask),
            0x00F8U,
            16U,
            6U
        );
        const auto role_result = role_target.step();
        test.expect_true(
            role_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                role_result.opcode == OP_1025 &&
                role_result.executed_instruction_count == 2U &&
                role_target.camera_pan.remaining_x == 480 &&
                role_target.camera_pan.remaining_y == 400 &&
                role_target.camera_pan.step_x == 16 &&
                role_target.camera_pan.step_y == 4 &&
                role_target.context.instruction_offset == 8U &&
                role_target.state.previous_opcode ==
                    OP_73_START_CAMERA_MOVE_TO_ROLE &&
                role_target.ports.direct_audio_service_count == 0U,
            "opcode 73 aliases center a clamped viewport on the selected role"
        );
    }

    CameraMoveFixture zero_axes;
    zero_axes.camera.right = 640U;
    zero_axes.camera.bottom = 480U;
    zero_axes.camera_pan.remaining_x = 11;
    zero_axes.camera_pan.remaining_y = 22;
    zero_axes.camera_pan.step_x = 33;
    zero_axes.camera_pan.step_y = 44;
    prime_long_camera_move(
        zero_axes, OP_50_START_RELATIVE_CAMERA_MOVE, 0, 0, 0U, 0U
    );
    const auto zero_axes_result = zero_axes.step();
    test.expect_true(
        zero_axes_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            zero_axes.camera_pan.remaining_x == 0 &&
            zero_axes.camera_pan.remaining_y == 0 &&
            zero_axes.camera_pan.step_x == 0 &&
            zero_axes.camera_pan.step_y == 0 &&
            zero_axes.context.instruction_offset == 10U,
        "zero camera displacement accepts zero requested steps"
    );

    CameraMoveFixture literal_fff0;
    literal_fff0.roles[1].world_x = 320U;
    literal_fff0.roles[1].world_y = 240U;
    literal_fff0.roles[2].guid = 0xFFF0U;
    literal_fff0.roles[2].world_x = 960U;
    literal_fff0.roles[2].world_y = 720U;
    prime_role_camera_move(
        literal_fff0, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFF0U, 16U, 16U
    );
    const auto literal_fff0_result = literal_fff0.step();
    test.expect_true(
        literal_fff0_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            literal_fff0.camera_pan.remaining_x == 640 &&
            literal_fff0.camera_pan.remaining_y == 480 &&
            literal_fff0.camera_pan.step_x == 16 &&
            literal_fff0.camera_pan.step_y == 16,
        "opcode 73 treats FFF0 as a literal role GUID"
    );

    CameraMoveFixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].world_x = 320U;
    controlled.roles[1].world_y = 240U;
    controlled.roles[2].world_x = 960U;
    controlled.roles[2].world_y = 720U;
    prime_role_camera_move(
        controlled, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFFEU, 16U, 16U
    );
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.camera_pan.remaining_x == 640 &&
            controlled.camera_pan.remaining_y == 480,
        "opcode 73 passes FFFE through for controlled-role selection"
    );

    CameraMoveFixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[0].world_x = 320U;
    first_clear_match.roles[0].world_y = 240U;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[1].world_x = 960U;
    first_clear_match.roles[1].world_y = 720U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[2].world_x = 1200U;
    first_clear_match.roles[2].world_y = 960U;
    prime_role_camera_move(
        first_clear_match, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x2222U, 16U, 16U
    );
    const auto first_clear_result = first_clear_match.step();
    test.expect_true(
        first_clear_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            first_clear_match.camera_pan.remaining_x == 640 &&
            first_clear_match.camera_pan.remaining_y == 480,
        "opcode 73 skips bit-28 roles and uses the first clear GUID match"
    );

    CameraMoveFixture far_role;
    far_role.roles[1].world_x = 2000U;
    far_role.roles[1].world_y = 1600U;
    prime_role_camera_move(
        far_role, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x00F8U, 16U, 16U
    );
    const auto far_role_result = far_role.step();
    test.expect_true(
        far_role_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            far_role.camera_pan.remaining_x == 960 &&
            far_role.camera_pan.remaining_y == 800 &&
            far_role.camera_pan.step_x == 16 &&
            far_role.camera_pan.step_y == 16,
        "opcode 73 centers then clamps the target viewport to map bounds"
    );

    CameraMoveFixture zero_map;
    zero_map.runtime.role_surface.map_width = 0U;
    zero_map.runtime.map_height = 0U;
    zero_map.roles[1].world_x = 320U;
    zero_map.roles[1].world_y = 240U;
    prime_role_camera_move(
        zero_map, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x00F8U, 16U, 16U
    );
    const auto zero_map_result = zero_map.step();
    test.expect_true(
        zero_map_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            zero_map.camera_pan.remaining_x == -640 &&
            zero_map.camera_pan.remaining_y == -480 &&
            zero_map.camera_pan.step_x == -16 &&
            zero_map.camera_pan.step_y == -16 &&
            zero_map.context.instruction_offset == 8U,
        "zero-sized maps preserve original wrapping camera clamping"
    );

    CameraMoveFixture high_clamp;
    high_clamp.runtime.role_surface.map_width = 50U;
    high_clamp.runtime.map_height = 40U;
    high_clamp.camera.right = 800U;
    high_clamp.camera.bottom = 640U;
    prime_long_camera_move(
        high_clamp, OP_50_START_RELATIVE_CAMERA_MOVE, 10, 10, 16U, 16U
    );
    const auto high_clamp_result = high_clamp.step();
    test.expect_true(
        high_clamp_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            high_clamp.camera_pan.remaining_x == 0 &&
            high_clamp.camera_pan.remaining_y == 0 &&
            high_clamp.camera_pan.step_x == 16 &&
            high_clamp.camera_pan.step_y == 16,
        "camera max clamping occurs after step derivation without recomputing steps"
    );

    CameraMoveFixture low_clamp;
    low_clamp.camera.right = 800U;
    low_clamp.camera.bottom = 800U;
    prime_long_camera_move(
        low_clamp, OP_50_START_RELATIVE_CAMERA_MOVE, -20, -20, 16U, 16U
    );
    const auto low_clamp_result = low_clamp.step(160, 160);
    test.expect_true(
        low_clamp_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            low_clamp.camera_pan.remaining_x == -160 &&
            low_clamp.camera_pan.remaining_y == -160 &&
            low_clamp.camera_pan.step_x == -16 &&
            low_clamp.camera_pan.step_y == -16,
        "camera minimum clamping preserves already-signed step magnitudes"
    );
}

void test_start_camera_move_failure_ordering(openswd3::test::Context& test) {
    CameraMoveFixture x_divide_by_zero;
    x_divide_by_zero.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        x_divide_by_zero, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 2, 0U, 3U
    );
    const auto x_divide_result = x_divide_by_zero.step();
    test.expect_true(
        x_divide_result.status ==
                LegacyWorldStoryVmStatus::camera_step_divide_by_zero &&
            x_divide_result.opcode == OP_50_START_RELATIVE_CAMERA_MOVE &&
            x_divide_result.executed_instruction_count == 1U &&
            x_divide_by_zero.camera_pan.remaining_x == 16 &&
            x_divide_by_zero.camera_pan.remaining_y == 32 &&
            x_divide_by_zero.camera_pan.step_x == 0 &&
            x_divide_by_zero.camera_pan.step_y == 3 &&
            x_divide_by_zero.context.instruction_offset == 0U &&
            x_divide_by_zero.state.previous_opcode == 0x55U &&
            x_divide_by_zero.ports.direct_audio_service_count == 0U,
        "X step divide-by-zero preserves shifted displacements and raw steps"
    );

    CameraMoveFixture y_divide_by_zero;
    y_divide_by_zero.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        y_divide_by_zero, OP_50_START_RELATIVE_CAMERA_MOVE, -1, -2, 3U, 0U
    );
    const auto y_divide_result = y_divide_by_zero.step();
    test.expect_true(
        y_divide_result.status ==
                LegacyWorldStoryVmStatus::camera_step_divide_by_zero &&
            y_divide_by_zero.camera_pan.remaining_x == -16 &&
            y_divide_by_zero.camera_pan.remaining_y == -32 &&
            y_divide_by_zero.camera_pan.step_x == 4 &&
            y_divide_by_zero.camera_pan.step_y == 0 &&
            y_divide_by_zero.context.instruction_offset == 0U &&
            y_divide_by_zero.state.previous_opcode == 0x55U,
        "Y step divide-by-zero retains X fallback before sign application"
    );

    CameraMoveFixture relative_without_camera;
    relative_without_camera.runtime.camera = nullptr;
    relative_without_camera.state.previous_opcode = 0x55U;
    prime_long_camera_move(
        relative_without_camera, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 0, 1U, 0U
    );
    const auto relative_without_camera_result = relative_without_camera.step();
    test.expect_true(
        relative_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            relative_without_camera.camera_pan.remaining_x == 16 &&
            relative_without_camera.camera_pan.remaining_y == 0 &&
            relative_without_camera.camera_pan.step_x == 1 &&
            relative_without_camera.camera_pan.step_y == 0 &&
            relative_without_camera.context.instruction_offset == 0U &&
            relative_without_camera.state.previous_opcode == 0x55U,
        "opcode 50 reaches the camera owner only after step preparation"
    );

    CameraMoveFixture absolute_without_camera;
    absolute_without_camera.runtime.camera = nullptr;
    absolute_without_camera.camera_pan.remaining_x = 0x11111111;
    absolute_without_camera.camera_pan.remaining_y = 0x22222222;
    absolute_without_camera.camera_pan.step_x = 0x33333333;
    absolute_without_camera.camera_pan.step_y = 0x44444444;
    prime_long_camera_move(
        absolute_without_camera, OP_70_START_ABSOLUTE_CAMERA_MOVE, 7, 9, 4U, 4U
    );
    const auto absolute_without_camera_result = absolute_without_camera.step();
    test.expect_true(
        absolute_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            absolute_without_camera.camera_pan.remaining_x == 0x11111111 &&
            absolute_without_camera.camera_pan.remaining_y == 0x22222222 &&
            absolute_without_camera.camera_pan.step_x == 0x33333333 &&
            absolute_without_camera.camera_pan.step_y == 0x44444444 &&
            absolute_without_camera.context.instruction_offset == 0U,
        "opcode 70 accesses the camera after its first target operand"
    );

    CameraMoveFixture missing_role;
    missing_role.camera_pan.remaining_x = 11;
    missing_role.camera_pan.remaining_y = 22;
    missing_role.camera_pan.step_x = 33;
    missing_role.camera_pan.step_y = 44;
    missing_role.state.previous_opcode = 0x55U;
    prime_role_camera_move(
        missing_role, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7777U, 4U, 4U
    );
    const auto missing_role_result = missing_role.step();
    test.expect_true(
        missing_role_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            missing_role_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            missing_role_result.executed_instruction_count == 1U &&
            missing_role.camera_pan.remaining_x == 11 &&
            missing_role.camera_pan.remaining_y == 22 &&
            missing_role.camera_pan.step_x == 33 &&
            missing_role.camera_pan.step_y == 44 &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.ports.role_patch_requests.empty(),
        "opcode 73 missing role stops at the unchecked coordinate access"
    );

    CameraMoveFixture missing_role_without_camera;
    missing_role_without_camera.runtime.camera = nullptr;
    prime_role_camera_move(
        missing_role_without_camera,
        OP_73_START_CAMERA_MOVE_TO_ROLE,
        0x7777U,
        4U,
        4U
    );
    const auto missing_without_camera_result =
        missing_role_without_camera.step();
    test.expect_true(
        missing_without_camera_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_role_without_camera.context.instruction_offset == 0U,
        "opcode 73 copies the camera owner before its unsafe missing-role read"
    );

    Fixture missing_pan;
    prime_long_camera_move(
        missing_pan, OP_50_START_RELATIVE_CAMERA_MOVE, 1, 1, 1U, 1U
    );
    const auto missing_pan_result = missing_pan.step();
    test.expect_true(
        missing_pan_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_pan_result.opcode == OP_50_START_RELATIVE_CAMERA_MOVE &&
            missing_pan_result.executed_instruction_count == 1U &&
            missing_pan.context.instruction_offset == 0U,
        "camera move handlers require the process camera-pan owner first"
    );

    CameraMoveFixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime_role_camera_move(
        invalid_controlled, OP_73_START_CAMERA_MOVE_TO_ROLE, 0xFFFEU, 4U, 4U
    );
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.camera_pan.remaining_x == 0 &&
            invalid_controlled.camera_pan.remaining_y == 0 &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 73 invalid controlled owner stops before opcode fetch"
    );
}

void test_start_camera_move_window_boundaries(openswd3::test::Context& test) {
    const auto prime_at =
        [](Fixture& fixture, const u16 raw_word, const u16 offset) {
            fixture.context.talk_data_offset = 0x1111U;
            fixture.context.instruction_offset = offset;
            fixture.state.loaded_file_number = 1U;
            fixture.state.loaded_data_offset = 0x1111U;
            fixture.state.window_loaded = true;
            fixture.state.previous_opcode = 0x55U;
            write_u16(fixture.state.window, offset, raw_word);
        };

    constexpr std::array<u16, 2U> long_opcodes{
        OP_50_START_RELATIVE_CAMERA_MOVE,
        OP_70_START_ABSOLUTE_CAMERA_MOVE,
    };
    for (const u16 opcode : long_opcodes) {
        const i16 first = opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? -2 : 5;
        const i16 second = opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? 3 : 7;
        const i32 expected_tile_x =
            opcode == OP_50_START_RELATIVE_CAMERA_MOVE ? -2 : 1;
        constexpr i32 expected_tile_y = 3;

        CameraMoveFixture first_operand_truncated;
        first_operand_truncated.camera_pan.remaining_x = 11;
        first_operand_truncated.camera_pan.remaining_y = 22;
        first_operand_truncated.camera_pan.step_x = 33;
        first_operand_truncated.camera_pan.step_y = 44;
        prime_at(first_operand_truncated, opcode, 0x7FFEU);
        const auto first_truncated_result =
            first_operand_truncated.step(64, 64);
        test.expect_true(
            first_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                first_truncated_result.opcode == opcode &&
                first_truncated_result.executed_instruction_count == 1U &&
                first_operand_truncated.camera_pan.remaining_x == 11 &&
                first_operand_truncated.camera_pan.remaining_y == 22 &&
                first_operand_truncated.camera_pan.step_x == 33 &&
                first_operand_truncated.camera_pan.step_y == 44 &&
                first_operand_truncated.context.instruction_offset == 0x7FFEU &&
                first_operand_truncated.state.previous_opcode == 0x55U,
            "long camera moves stop before the first operand word"
        );

        CameraMoveFixture second_operand_truncated;
        second_operand_truncated.camera_pan.remaining_x = 11;
        second_operand_truncated.camera_pan.remaining_y = 22;
        second_operand_truncated.camera_pan.step_x = 33;
        second_operand_truncated.camera_pan.step_y = 44;
        prime_at(second_operand_truncated, opcode, 0x7FFCU);
        write_u16(
            second_operand_truncated.state.window,
            0x7FFEU,
            static_cast<u16>(first)
        );
        const auto second_truncated_result =
            second_operand_truncated.step(64, 64);
        test.expect_true(
            second_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                second_operand_truncated.camera_pan.remaining_x ==
                    (opcode == OP_50_START_RELATIVE_CAMERA_MOVE
                         ? expected_tile_x
                         : 11) &&
                second_operand_truncated.camera_pan.remaining_y == 22 &&
                second_operand_truncated.camera_pan.step_x == 33 &&
                second_operand_truncated.camera_pan.step_y == 44 &&
                second_operand_truncated.context.instruction_offset ==
                    0x7FFCU &&
                second_operand_truncated.state.previous_opcode == 0x55U,
            "camera move variants preserve their distinct Y-target truncation order"
        );

        CameraMoveFixture x_step_truncated;
        x_step_truncated.camera_pan.step_x = 33;
        x_step_truncated.camera_pan.step_y = 44;
        prime_at(x_step_truncated, opcode, 0x7FFAU);
        write_u16(
            x_step_truncated.state.window, 0x7FFCU, static_cast<u16>(first)
        );
        write_u16(
            x_step_truncated.state.window, 0x7FFEU, static_cast<u16>(second)
        );
        const auto x_step_truncated_result = x_step_truncated.step(64, 64);
        test.expect_true(
            x_step_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                x_step_truncated.camera_pan.remaining_x == expected_tile_x &&
                x_step_truncated.camera_pan.remaining_y == expected_tile_y &&
                x_step_truncated.camera_pan.step_x == 33 &&
                x_step_truncated.camera_pan.step_y == 44 &&
                x_step_truncated.context.instruction_offset == 0x7FFAU &&
                x_step_truncated.state.previous_opcode == 0x55U,
            "long camera moves retain both tile displacements before X-step truncation"
        );

        CameraMoveFixture y_step_truncated;
        y_step_truncated.camera_pan.step_x = 33;
        y_step_truncated.camera_pan.step_y = 44;
        prime_at(y_step_truncated, opcode, 0x7FF8U);
        write_u16(
            y_step_truncated.state.window, 0x7FFAU, static_cast<u16>(first)
        );
        write_u16(
            y_step_truncated.state.window, 0x7FFCU, static_cast<u16>(second)
        );
        write_u16(y_step_truncated.state.window, 0x7FFEU, 8U);
        const auto y_step_truncated_result = y_step_truncated.step(64, 64);
        test.expect_true(
            y_step_truncated_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                y_step_truncated.camera_pan.remaining_x == expected_tile_x &&
                y_step_truncated.camera_pan.remaining_y == expected_tile_y &&
                y_step_truncated.camera_pan.step_x == 8 &&
                y_step_truncated.camera_pan.step_y == 44 &&
                y_step_truncated.context.instruction_offset == 0x7FF8U &&
                y_step_truncated.state.previous_opcode == 0x55U,
            "long camera moves retain raw X step before Y-step truncation"
        );

        CameraMoveFixture exact_tail;
        exact_tail.camera.right = 704U;
        exact_tail.camera.bottom = 544U;
        prime_at(exact_tail, opcode, 0x7FF6U);
        write_u16(exact_tail.state.window, 0x7FF8U, static_cast<u16>(first));
        write_u16(exact_tail.state.window, 0x7FFAU, static_cast<u16>(second));
        write_u16(exact_tail.state.window, 0x7FFCU, 8U);
        write_u16(exact_tail.state.window, 0x7FFEU, 4U);
        const auto exact_tail_result = exact_tail.step(64, 64);
        const i32 expected_pixel_x = expected_tile_x * 16;
        test.expect_true(
            exact_tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                exact_tail_result.opcode == opcode &&
                exact_tail_result.executed_instruction_count == 1U &&
                exact_tail.camera_pan.remaining_x == expected_pixel_x &&
                exact_tail.camera_pan.remaining_y == 48 &&
                exact_tail.camera_pan.step_x ==
                    (expected_pixel_x < 0 ? -8 : 8) &&
                exact_tail.camera_pan.step_y == 4 &&
                exact_tail.context.instruction_offset == 0x8000U &&
                exact_tail.state.previous_opcode == opcode,
            "complete long camera records finish effects before next fetch"
        );
    }

    CameraMoveFixture role_selector_truncated;
    role_selector_truncated.camera_pan.remaining_x = 11;
    role_selector_truncated.camera_pan.remaining_y = 22;
    role_selector_truncated.camera_pan.step_x = 33;
    role_selector_truncated.camera_pan.step_y = 44;
    prime_at(role_selector_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFEU);
    const auto role_selector_result = role_selector_truncated.step();
    test.expect_true(
        role_selector_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_selector_truncated.camera_pan.remaining_x == 11 &&
            role_selector_truncated.camera_pan.remaining_y == 22 &&
            role_selector_truncated.camera_pan.step_x == 33 &&
            role_selector_truncated.camera_pan.step_y == 44 &&
            role_selector_truncated.context.instruction_offset == 0x7FFEU &&
            role_selector_truncated.state.previous_opcode == 0x55U,
        "opcode 73 stops before the selector word"
    );

    CameraMoveFixture role_x_step_truncated;
    role_x_step_truncated.roles[1].world_x = 800U;
    role_x_step_truncated.roles[1].world_y = 640U;
    role_x_step_truncated.camera_pan.step_x = 33;
    role_x_step_truncated.camera_pan.step_y = 44;
    prime_at(role_x_step_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFCU);
    write_u16(role_x_step_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto role_x_step_result = role_x_step_truncated.step();
    test.expect_true(
        role_x_step_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_x_step_truncated.camera_pan.remaining_x == 30 &&
            role_x_step_truncated.camera_pan.remaining_y == 25 &&
            role_x_step_truncated.camera_pan.step_x == 33 &&
            role_x_step_truncated.camera_pan.step_y == 44 &&
            role_x_step_truncated.context.instruction_offset == 0x7FFCU &&
            role_x_step_truncated.state.previous_opcode == 0x55U,
        "opcode 73 retains role-derived tile displacements before X-step truncation"
    );

    CameraMoveFixture role_y_step_truncated;
    role_y_step_truncated.roles[1].world_x = 800U;
    role_y_step_truncated.roles[1].world_y = 640U;
    role_y_step_truncated.camera_pan.step_x = 33;
    role_y_step_truncated.camera_pan.step_y = 44;
    prime_at(role_y_step_truncated, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FFAU);
    write_u16(role_y_step_truncated.state.window, 0x7FFCU, 0x00F8U);
    write_u16(role_y_step_truncated.state.window, 0x7FFEU, 8U);
    const auto role_y_step_result = role_y_step_truncated.step();
    test.expect_true(
        role_y_step_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            role_y_step_truncated.camera_pan.remaining_x == 30 &&
            role_y_step_truncated.camera_pan.remaining_y == 25 &&
            role_y_step_truncated.camera_pan.step_x == 8 &&
            role_y_step_truncated.camera_pan.step_y == 44 &&
            role_y_step_truncated.context.instruction_offset == 0x7FFAU &&
            role_y_step_truncated.state.previous_opcode == 0x55U,
        "opcode 73 retains raw X step before Y-step truncation"
    );

    CameraMoveFixture role_exact_tail;
    role_exact_tail.roles[1].world_x = 800U;
    role_exact_tail.roles[1].world_y = 640U;
    prime_at(role_exact_tail, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FF8U);
    write_u16(role_exact_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(role_exact_tail.state.window, 0x7FFCU, 8U);
    write_u16(role_exact_tail.state.window, 0x7FFEU, 8U);
    const auto role_exact_tail_result = role_exact_tail.step();
    test.expect_true(
        role_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            role_exact_tail_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            role_exact_tail_result.executed_instruction_count == 1U &&
            role_exact_tail.camera_pan.remaining_x == 480 &&
            role_exact_tail.camera_pan.remaining_y == 400 &&
            role_exact_tail.camera_pan.step_x == 8 &&
            role_exact_tail.camera_pan.step_y == 8 &&
            role_exact_tail.context.instruction_offset == 0x8000U &&
            role_exact_tail.state.previous_opcode ==
                OP_73_START_CAMERA_MOVE_TO_ROLE,
        "opcode 73 exact tail completes when clamp diagnostics are skipped"
    );

    CameraMoveFixture clamped_diagnostic_tail;
    clamped_diagnostic_tail.runtime.role_surface.map_width = 50U;
    clamped_diagnostic_tail.camera.right = 900U;
    clamped_diagnostic_tail.roles[1].world_x = 320U;
    clamped_diagnostic_tail.roles[1].world_y = 240U;
    prime_at(clamped_diagnostic_tail, OP_73_START_CAMERA_MOVE_TO_ROLE, 0x7FF8U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFAU, 0x00F8U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFCU, 4U);
    write_u16(clamped_diagnostic_tail.state.window, 0x7FFEU, 4U);
    const auto clamped_tail_result = clamped_diagnostic_tail.step();
    test.expect_true(
        clamped_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            clamped_tail_result.opcode == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            clamped_tail_result.executed_instruction_count == 1U &&
            clamped_diagnostic_tail.camera_pan.remaining_x == -100 &&
            clamped_diagnostic_tail.camera_pan.remaining_y == 0 &&
            clamped_diagnostic_tail.camera_pan.step_x == 0 &&
            clamped_diagnostic_tail.camera_pan.step_y == 0 &&
            clamped_diagnostic_tail.context.instruction_offset == 0x7FF8U &&
            clamped_diagnostic_tail.state.previous_opcode == 0x55U,
        "opcode 73 clamp diagnostics preserve the original next-word overread"
    );

    CameraMoveFixture clamped_diagnostic_available;
    clamped_diagnostic_available.runtime.role_surface.map_width = 50U;
    clamped_diagnostic_available.camera.right = 900U;
    clamped_diagnostic_available.roles[1].world_x = 320U;
    clamped_diagnostic_available.roles[1].world_y = 240U;
    prime_role_camera_move(
        clamped_diagnostic_available,
        OP_73_START_CAMERA_MOVE_TO_ROLE,
        0x00F8U,
        4U,
        4U
    );
    const auto clamped_available_result = clamped_diagnostic_available.step();
    test.expect_true(
        clamped_available_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            clamped_available_result.opcode == OP_1025 &&
            clamped_available_result.executed_instruction_count == 2U &&
            clamped_diagnostic_available.camera_pan.remaining_x == -100 &&
            clamped_diagnostic_available.camera_pan.remaining_y == 0 &&
            clamped_diagnostic_available.context.instruction_offset == 8U &&
            clamped_diagnostic_available.state.previous_opcode ==
                OP_73_START_CAMERA_MOVE_TO_ROLE,
        "opcode 73 clamp diagnostics consume an available next word only diagnostically"
    );
}

void test_wait_for_camera_move_protocol(openswd3::test::Context& test) {
    struct Variant {
        i32 remaining_x;
        i32 remaining_y;
        i32 step_x;
        i32 step_y;
    };
    constexpr std::array<Variant, 4U> variants{
        Variant{16, 0, 0, 0},
        Variant{0, -16, 0, 0},
        Variant{0, 0, 4, 0},
        Variant{0, 0, 0, -4},
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        for (const Variant variant : variants) {
            CameraMoveFixture waiting;
            waiting.camera_pan.remaining_x = variant.remaining_x;
            waiting.camera_pan.remaining_y = variant.remaining_y;
            waiting.camera_pan.step_x = variant.step_x;
            waiting.camera_pan.step_y = variant.step_y;
            waiting.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                waiting,
                static_cast<u16>(OP_51_WAIT_CAMERA_MOVE_COMPLETE | mask)
            );
            write_u16(waiting.state.window, 2U, OP_1025);

            const auto result = waiting.step();

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                    result.executed_instruction_count == 1U &&
                    waiting.camera_pan.remaining_x == variant.remaining_x &&
                    waiting.camera_pan.remaining_y == variant.remaining_y &&
                    waiting.camera_pan.step_x == variant.step_x &&
                    waiting.camera_pan.step_y == variant.step_y &&
                    waiting.context.instruction_offset == 0U &&
                    waiting.state.previous_opcode ==
                        OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                    waiting.ports.direct_audio_service_count == 0U,
                "opcode 51 aliases wait on each camera movement field"
            );
        }

        CameraMoveFixture completed;
        completed.state.previous_opcode = 0x55U;
        prime_loaded_instruction(
            completed, static_cast<u16>(OP_51_WAIT_CAMERA_MOVE_COMPLETE | mask)
        );
        write_u16(completed.state.window, 2U, OP_1025);

        const auto result = completed.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                completed.context.instruction_offset == 2U &&
                completed.state.previous_opcode ==
                    OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
                completed.ports.direct_audio_service_count == 0U,
            "opcode 51 aliases advance and continue when all movement fields are zero"
        );
    }

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(missing_owner, OP_51_WAIT_CAMERA_MOVE_COMPLETE);
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 51 requires the camera-pan owner before reading movement state"
    );

    CameraMoveFixture completed_tail;
    completed_tail.context.talk_data_offset = 0x1111U;
    completed_tail.context.instruction_offset = 0x7FFEU;
    completed_tail.state.loaded_file_number = 1U;
    completed_tail.state.loaded_data_offset = 0x1111U;
    completed_tail.state.window_loaded = true;
    completed_tail.state.previous_opcode = 0x55U;
    write_u16(
        completed_tail.state.window, 0x7FFEU, OP_51_WAIT_CAMERA_MOVE_COMPLETE
    );
    const auto completed_tail_result = completed_tail.step();
    test.expect_true(
        completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_51_WAIT_CAMERA_MOVE_COMPLETE,
        "opcode 51 exact tail publishes completion before the next fetch fails"
    );

    CameraMoveFixture waiting_tail;
    waiting_tail.context.talk_data_offset = 0x1111U;
    waiting_tail.context.instruction_offset = 0x7FFEU;
    waiting_tail.state.loaded_file_number = 1U;
    waiting_tail.state.loaded_data_offset = 0x1111U;
    waiting_tail.state.window_loaded = true;
    waiting_tail.state.previous_opcode = 0x55U;
    waiting_tail.camera_pan.step_y = -4;
    write_u16(
        waiting_tail.state.window, 0x7FFEU, OP_51_WAIT_CAMERA_MOVE_COMPLETE
    );
    const auto waiting_tail_result = waiting_tail.step();
    test.expect_true(
        waiting_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_tail_result.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_tail_result.executed_instruction_count == 1U &&
            waiting_tail.context.instruction_offset == 0x7FFEU &&
            waiting_tail.state.previous_opcode ==
                OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_tail.camera_pan.step_y == -4,
        "opcode 51 exact-tail wait publishes without advancing"
    );
}

void test_start_frame_color_transition_protocol(openswd3::test::Context& test) {
    constexpr std::array<i16, 6U> components{-30, 5, 17, 0, -25, 2};
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        openswd3::rendering::LegacyFrameColorTransitionState color{};
        fixture.runtime.frame_color = &color;
        fixture.state.previous_opcode = 0x55U;
        prime_frame_color_transition(
            fixture,
            static_cast<u16>(OP_52_START_FRAME_COLOR_TRANSITION | mask),
            components,
            6U
        );

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                color.current_red == -30.0F && color.current_green == 5.0F &&
                color.current_blue == 17.0F && color.target_red == 0.0F &&
                color.target_green == -25.0F && color.target_blue == 2.0F &&
                color.countdown == 6 && color.step_red == 5.0F &&
                color.step_green == -5.0F && color.step_blue == -2.5F &&
                fixture.context.instruction_offset == 16U &&
                fixture.state.previous_opcode ==
                    OP_52_START_FRAME_COLOR_TRANSITION &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 52 aliases initialize all color-transition values and continue"
        );
    }

    Fixture maximum_duration;
    openswd3::rendering::LegacyFrameColorTransitionState maximum_color{};
    maximum_duration.runtime.frame_color = &maximum_color;
    prime_frame_color_transition(
        maximum_duration,
        OP_52_START_FRAME_COLOR_TRANSITION,
        std::array<i16, 6U>{-32768, -32768, -32768, 32767, 32767, 32767},
        0xFFFFU
    );
    const auto maximum_result = maximum_duration.step();
    test.expect_true(
        maximum_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            maximum_color.countdown == 65535 &&
            maximum_color.step_red == 1.0F &&
            maximum_color.step_green == 1.0F && maximum_color.step_blue == 1.0F,
        "opcode 52 zero-extends duration and preserves full signed component range"
    );

    Fixture zero_duration;
    openswd3::rendering::LegacyFrameColorTransitionState zero_color{};
    zero_duration.runtime.frame_color = &zero_color;
    prime_frame_color_transition(
        zero_duration,
        OP_52_START_FRAME_COLOR_TRANSITION,
        std::array<i16, 6U>{0, 0, 5, 1, -1, 5},
        0U
    );
    const auto zero_result = zero_duration.step();
    test.expect_true(
        zero_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            zero_color.countdown == 0 &&
            std::bit_cast<u32>(zero_color.step_red) == 0x7F800000U &&
            std::bit_cast<u32>(zero_color.step_green) == 0xFF800000U &&
            std::bit_cast<u32>(zero_color.step_blue) == 0xFFC00000U &&
            zero_duration.context.instruction_offset == 16U &&
            zero_duration.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION,
        "opcode 52 zero duration reproduces x87 infinity and indefinite-NaN bits"
    );

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_frame_color_transition(
        missing_owner, OP_52_START_FRAME_COLOR_TRANSITION, components, 6U
    );
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 52 accesses the color owner after reading the first component"
    );
}

void test_start_frame_color_transition_window_boundaries(
    openswd3::test::Context& test
) {
    constexpr std::array<i16, 6U> components{-30, 5, 17, 0, -25, 2};

    for (std::size_t available = 0U; available < 7U; ++available) {
        Fixture fixture;
        openswd3::rendering::LegacyFrameColorTransitionState color{
            .countdown = 77,
            .current_red = 101.0F,
            .current_green = 102.0F,
            .current_blue = 103.0F,
            .target_red = 201.0F,
            .target_green = 202.0F,
            .target_blue = 203.0F,
            .step_red = 301.0F,
            .step_green = 302.0F,
            .step_blue = 303.0F,
        };
        fixture.runtime.frame_color = &color;
        const u16 offset = static_cast<u16>(0x7FFEU - available * 2U);
        fixture.context.talk_data_offset = 0x1111U;
        fixture.context.instruction_offset = offset;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x55U;
        write_u16(
            fixture.state.window, offset, OP_52_START_FRAME_COLOR_TRANSITION
        );
        for (std::size_t index = 0U;
             index < available && index < components.size();
             ++index) {
            write_u16(
                fixture.state.window,
                offset + 2U + index * 2U,
                static_cast<u16>(components[index])
            );
        }

        const auto result = fixture.step();
        const bool all_targets_available = available >= components.size();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
                result.executed_instruction_count == 1U &&
                color.current_red == (available >= 1U ? -30.0F : 101.0F) &&
                color.current_green == (available >= 2U ? 5.0F : 102.0F) &&
                color.current_blue == (available >= 3U ? 17.0F : 103.0F) &&
                color.target_red == (all_targets_available ? 0.0F : 201.0F) &&
                color.target_green ==
                    (all_targets_available ? -25.0F : 202.0F) &&
                color.target_blue == (all_targets_available ? 2.0F : 203.0F) &&
                color.countdown == 77 && color.step_red == 301.0F &&
                color.step_green == 302.0F && color.step_blue == 303.0F &&
                fixture.context.instruction_offset == offset &&
                fixture.state.previous_opcode == 0x55U,
            "opcode 52 truncations preserve staged current and grouped target writes"
        );
    }

    Fixture exact_tail;
    openswd3::rendering::LegacyFrameColorTransitionState exact_color{};
    exact_tail.runtime.frame_color = &exact_color;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.context.instruction_offset = 0x7FF0U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    write_u16(
        exact_tail.state.window, 0x7FF0U, OP_52_START_FRAME_COLOR_TRANSITION
    );
    for (std::size_t index = 0U; index < components.size(); ++index) {
        write_u16(
            exact_tail.state.window,
            0x7FF2U + index * 2U,
            static_cast<u16>(components[index])
        );
    }
    write_u16(exact_tail.state.window, 0x7FFEU, 6U);

    const auto exact_result = exact_tail.step();

    test.expect_true(
        exact_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_result.opcode == OP_52_START_FRAME_COLOR_TRANSITION &&
            exact_result.executed_instruction_count == 1U &&
            exact_color.countdown == 6 && exact_color.step_red == 5.0F &&
            exact_color.step_green == -5.0F && exact_color.step_blue == -2.5F &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION,
        "opcode 52 exact tail completes all effects before the next fetch fails"
    );
}

void test_wait_for_frame_color_transition_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<i32, 2U> waiting_values{1, 0x7FFFFFFF};
    constexpr std::array<i32, 3U> completed_values{
        0,
        -1,
        -2147483647 - 1,
    };

    for (const u16 mask : alias_masks) {
        for (const i32 countdown : waiting_values) {
            Fixture fixture;
            openswd3::rendering::LegacyFrameColorTransitionState color{
                .countdown = countdown,
                .current_red = 101.0F,
                .current_green = 102.0F,
                .current_blue = 103.0F,
                .target_red = 201.0F,
                .target_green = 202.0F,
                .target_blue = 203.0F,
                .step_red = 301.0F,
                .step_green = 302.0F,
                .step_blue = 303.0F,
            };
            fixture.runtime.frame_color = &color;
            fixture.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_53_WAIT_FRAME_COLOR_TRANSITION | mask)
            );

            const auto result = fixture.step();

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    result.executed_instruction_count == 1U &&
                    color.countdown == countdown &&
                    color.current_red == 101.0F &&
                    color.current_green == 102.0F &&
                    color.current_blue == 103.0F &&
                    color.target_red == 201.0F &&
                    color.target_green == 202.0F &&
                    color.target_blue == 203.0F && color.step_red == 301.0F &&
                    color.step_green == 302.0F && color.step_blue == 303.0F &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode ==
                        OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcode 53 aliases wait in place for every positive signed countdown"
            );
        }

        for (const i32 countdown : completed_values) {
            Fixture fixture;
            openswd3::rendering::LegacyFrameColorTransitionState color{
                .countdown = countdown,
                .current_red = 101.0F,
                .current_green = 102.0F,
                .current_blue = 103.0F,
                .target_red = 201.0F,
                .target_green = 202.0F,
                .target_blue = 203.0F,
                .step_red = 301.0F,
                .step_green = 302.0F,
                .step_blue = 303.0F,
            };
            fixture.runtime.frame_color = &color;
            fixture.state.previous_opcode = 0x55U;
            prime_loaded_instruction(
                fixture,
                static_cast<u16>(OP_53_WAIT_FRAME_COLOR_TRANSITION | mask)
            );
            write_u16(fixture.state.window, 2U, OP_1025);

            const auto result = fixture.step();

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                    result.opcode == OP_1025 &&
                    result.executed_instruction_count == 2U &&
                    color.countdown == countdown &&
                    color.current_red == 101.0F &&
                    color.current_green == 102.0F &&
                    color.current_blue == 103.0F &&
                    color.target_red == 201.0F &&
                    color.target_green == 202.0F &&
                    color.target_blue == 203.0F && color.step_red == 301.0F &&
                    color.step_green == 302.0F && color.step_blue == 303.0F &&
                    fixture.context.instruction_offset == 2U &&
                    fixture.state.previous_opcode ==
                        OP_53_WAIT_FRAME_COLOR_TRANSITION &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcode 53 aliases complete and continue for every non-positive signed countdown"
            );
        }
    }

    Fixture missing_owner;
    missing_owner.state.previous_opcode = 0x55U;
    prime_loaded_instruction(missing_owner, OP_53_WAIT_FRAME_COLOR_TRANSITION);
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U,
        "opcode 53 missing color owner stops at the original countdown access"
    );

    Fixture waiting_tail;
    openswd3::rendering::LegacyFrameColorTransitionState waiting_color{};
    waiting_color.countdown = 1;
    waiting_tail.runtime.frame_color = &waiting_color;
    waiting_tail.context.talk_data_offset = 0x1111U;
    waiting_tail.context.instruction_offset = 0x7FFEU;
    waiting_tail.state.loaded_file_number = 1U;
    waiting_tail.state.loaded_data_offset = 0x1111U;
    waiting_tail.state.window_loaded = true;
    waiting_tail.state.previous_opcode = 0x55U;
    write_u16(
        waiting_tail.state.window, 0x7FFEU, OP_53_WAIT_FRAME_COLOR_TRANSITION
    );
    const auto waiting_tail_result = waiting_tail.step();
    test.expect_true(
        waiting_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            waiting_tail_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            waiting_tail_result.executed_instruction_count == 1U &&
            waiting_tail.context.instruction_offset == 0x7FFEU &&
            waiting_tail.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION,
        "opcode 53 waiting tail publishes previous and remains at 0x7FFE"
    );

    Fixture completed_tail;
    openswd3::rendering::LegacyFrameColorTransitionState completed_color{};
    completed_tail.runtime.frame_color = &completed_color;
    completed_tail.context.talk_data_offset = 0x1111U;
    completed_tail.context.instruction_offset = 0x7FFEU;
    completed_tail.state.loaded_file_number = 1U;
    completed_tail.state.loaded_data_offset = 0x1111U;
    completed_tail.state.window_loaded = true;
    completed_tail.state.previous_opcode = 0x55U;
    write_u16(
        completed_tail.state.window, 0x7FFEU, OP_53_WAIT_FRAME_COLOR_TRANSITION
    );
    const auto completed_tail_result = completed_tail.step();
    test.expect_true(
        completed_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            completed_tail_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            completed_tail_result.executed_instruction_count == 1U &&
            completed_tail.context.instruction_offset == 0x8000U &&
            completed_tail.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION,
        "opcode 53 completed tail publishes previous before the next fetch fails"
    );
}

void test_repeat_role_action_refresh_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime_instruction = [](Fixture& fixture,
                                      const u16 raw_word,
                                      const u16 selector,
                                      const i16 repeat_count) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, static_cast<u16>(repeat_count));
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        auto& action = fixture.roles[1].action;
        action.command_cursor = 0x1111U;
        action.wait_remaining = 0x2222U;
        action.field_58 = 0x3333U;
        std::vector<std::tuple<u16, u16, u16>> snapshots;
        fixture.ports.action_update_callback =
            [&snapshots](
                openswd3::asset_runtime::LegacyActionRecord& updated,
                const u32 update_index
            ) {
                snapshots.emplace_back(
                    updated.wait_remaining,
                    updated.command_cursor,
                    updated.field_58
                );
                updated.wait_remaining =
                    static_cast<u16>(0x4000U + update_index);
                updated.command_cursor =
                    static_cast<u16>(0x5000U + update_index);
                updated.field_58 = static_cast<u16>(0x6000U + update_index);
            };
        prime_instruction(
            fixture,
            static_cast<u16>(OP_54_REPEAT_ROLE_ACTION_REFRESH | mask),
            0x00F8U,
            2
        );

        const auto result = fixture.step();

        const std::vector<std::tuple<u16, u16, u16>> expected{
            {0U, 0U, 0x3333U},
            {0U, 0x5001U, 0x6001U},
            {0U, 0x5002U, 0U},
        };
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 3U &&
                result.action_update_failure_count == 0U &&
                snapshots == expected && action.wait_remaining == 0x4003U &&
                action.command_cursor == 0x5003U && action.field_58 == 0U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_54_REPEAT_ROLE_ACTION_REFRESH &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 54 aliases preserve initial and repeated refresh ordering"
        );
    }

    constexpr std::array<i16, 3U> non_positive_repeats{
        static_cast<i16>(0x8000U),
        -1,
        0,
    };
    for (const i16 repeat_count : non_positive_repeats) {
        Fixture fixture;
        auto& action = fixture.roles[1].action;
        action.command_cursor = 0x1111U;
        action.wait_remaining = 0x2222U;
        action.field_58 = 0x3333U;
        std::tuple<u16, u16, u16> snapshot{};
        fixture.ports.action_update_callback =
            [&snapshot](
                openswd3::asset_runtime::LegacyActionRecord& updated, const u32
            ) {
                snapshot = {
                    updated.wait_remaining,
                    updated.command_cursor,
                    updated.field_58,
                };
                updated.wait_remaining = 0x4444U;
                updated.command_cursor = 0x5555U;
                updated.field_58 = 0x6666U;
            };
        prime_instruction(
            fixture, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x00F8U, repeat_count
        );

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                snapshot == std::tuple<u16, u16, u16>{0U, 0U, 0x3333U} &&
                action.command_cursor == 0x5555U &&
                action.wait_remaining == 0x4444U &&
                action.field_58 == 0x6666U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_54_REPEAT_ROLE_ACTION_REFRESH,
            "opcode 54 non-positive signed repeat keeps the initial refresh writeback"
        );
    }

    Fixture update_failure;
    auto& failed_action = update_failure.roles[1].action;
    failed_action.command_cursor = 0x1111U;
    failed_action.wait_remaining = 0x2222U;
    failed_action.field_58 = 0x3333U;
    update_failure.ports.action_update_result = 0U;
    update_failure.ports.action_update_callback =
        [](openswd3::asset_runtime::LegacyActionRecord& updated,
           const u32 update_index) {
            updated.wait_remaining = static_cast<u16>(0x7000U + update_index);
            updated.command_cursor = static_cast<u16>(0x7100U + update_index);
            updated.field_58 = static_cast<u16>(0x7200U + update_index);
        };
    prime_instruction(
        update_failure, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x00F8U, 2
    );
    const auto failure_result = update_failure.step();
    test.expect_true(
        failure_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            failure_result.action_update_count == 3U &&
            failure_result.action_update_failure_count == 3U &&
            failed_action.command_cursor == 0x7103U &&
            failed_action.wait_remaining == 0x7003U &&
            failed_action.field_58 == 0U &&
            update_failure.context.instruction_offset == 6U &&
            update_failure.state.previous_opcode ==
                OP_54_REPEAT_ROLE_ACTION_REFRESH,
        "opcode 54 refresh failures are diagnostic-only across every iteration"
    );

    Fixture source_selector;
    source_selector.roles[1].action.command_cursor = 0x1111U;
    source_selector.roles[2].guid = 0xFFF0U;
    source_selector.roles[2].action.command_cursor = 0x2222U;
    prime_instruction(
        source_selector, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0xFFF0U, 0
    );
    const auto source_result = source_selector.step();
    test.expect_true(
        source_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            source_result.action_update_count == 1U &&
            source_selector.roles[1].action.command_cursor == 0U &&
            source_selector.roles[2].action.command_cursor == 0x2222U,
        "opcode 54 translates FFF0 to the talk source before lookup"
    );

    Fixture controlled_selector;
    controlled_selector.roles[1].guid = 0xFFFEU;
    controlled_selector.roles[1].action.command_cursor = 0x1111U;
    controlled_selector.roles[2].action.command_cursor = 0x2222U;
    prime_instruction(
        controlled_selector, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0xFFFEU, 0
    );
    const auto controlled_result = controlled_selector.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled_result.action_update_count == 1U &&
            controlled_selector.roles[1].action.command_cursor == 0x1111U &&
            controlled_selector.roles[2].action.command_cursor == 0U,
        "opcode 54 passes FFFE through for controlled-role selection"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    selector_truncated.state.previous_opcode = 0x55U;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    const auto selector_truncated_result = selector_truncated.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated_result.action_update_count == 0U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x55U,
        "opcode 54 stops at the unsafe selector-word access"
    );

    Fixture repeat_truncated;
    repeat_truncated.context.instruction_offset = 0x7FFCU;
    repeat_truncated.context.talk_data_offset = 0x1111U;
    repeat_truncated.state.loaded_file_number = 1U;
    repeat_truncated.state.loaded_data_offset = 0x1111U;
    repeat_truncated.state.window_loaded = true;
    repeat_truncated.state.previous_opcode = 0x55U;
    write_u16(
        repeat_truncated.state.window, 0x7FFCU, OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    write_u16(repeat_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto repeat_truncated_result = repeat_truncated.step();
    test.expect_true(
        repeat_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            repeat_truncated_result.action_update_count == 0U &&
            repeat_truncated.context.instruction_offset == 0x7FFCU &&
            repeat_truncated.state.previous_opcode == 0x55U,
        "opcode 54 performs missing-role lookup before the unsafe repeat read"
    );

    Fixture missing_role;
    missing_role.roles[0].action.command_cursor = 0x1111U;
    prime_instruction(
        missing_role, OP_54_REPEAT_ROLE_ACTION_REFRESH, 0x7777U, 2
    );
    const auto missing_result = missing_role.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            missing_result.executed_instruction_count == 1U &&
            missing_result.action_update_count == 0U &&
            missing_role.roles[0].action.command_cursor == 0x1111U &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U,
        "opcode 54 missing role stops at the first unsafe action-field write"
    );

    Fixture exact_tail;
    auto& tail_action = exact_tail.roles[1].action;
    tail_action.command_cursor = 0x1111U;
    tail_action.wait_remaining = 0x2222U;
    tail_action.field_58 = 0x3333U;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x55U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_54_REPEAT_ROLE_ACTION_REFRESH
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 1U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            tail_result.opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            tail_result.executed_instruction_count == 1U &&
            tail_result.action_update_count == 2U &&
            tail_action.command_cursor == 0U &&
            tail_action.wait_remaining == 0U && tail_action.field_58 == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_54_REPEAT_ROLE_ACTION_REFRESH,
        "opcode 54 exact tail completes every refresh before next-fetch failure"
    );
}

void test_shared_role_spatial_group_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    struct GroupCase {
        u16 opcode;
        u32 old_group;
        u32 target_group;
    };
    constexpr std::array<GroupCase, 3U> group_cases{
        GroupCase{OP_55_SET_ROLE_SPATIAL_GROUP_1, 0U, 1U},
        GroupCase{OP_56_SET_ROLE_SPATIAL_GROUP_0, 2U, 0U},
        GroupCase{OP_57_SET_ROLE_SPATIAL_GROUP_2, 1U, 2U},
    };
    constexpr std::size_t role_row =
        openswd3::world_map::kLegacySpatialRowPadding + 2U;
    const auto reset_spatial =
        [](openswd3::world_map::LegacyRoleSpatialIndex& spatial) {
            spatial.map_height = 4U;
            for (auto& group : spatial.row_heads) {
                group.assign(44U, 0U);
            }
        };
    const auto prime_instruction =
        [](Fixture& fixture, const u16 raw_word, const u16 selector) {
            prime_loaded_instruction(fixture, raw_word);
            write_u16(fixture.state.window, 2U, selector);
            fixture.state.previous_opcode = 0x66U;
        };

    for (const GroupCase group_case : group_cases) {
        for (const u16 mask : alias_masks) {
            Fixture fixture;
            openswd3::world_map::LegacyRoleSpatialIndex spatial;
            reset_spatial(spatial);
            auto& role = fixture.roles[1];
            role.world_y = 32U;
            role.flags = 0xA5A50000U | group_case.old_group;
            const bool inserted =
                openswd3::world_map::insert_legacy_role_spatially(
                    spatial, fixture.roles, 1U, group_case.old_group
                );
            fixture.runtime.spatial_index = &spatial;
            prime_instruction(
                fixture, static_cast<u16>(group_case.opcode | mask), 0x00F8U
            );

            const auto result = fixture.step();

            test.expect_true(
                inserted &&
                    result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == group_case.opcode &&
                    result.executed_instruction_count == 1U &&
                    role.flags == (0xA5A50000U | group_case.target_group) &&
                    spatial.row_heads[group_case.old_group][role_row] == 0U &&
                    spatial.row_heads[group_case.target_group][role_row] ==
                        1U &&
                    role.spatial_next_link_32 == 0U &&
                    fixture.context.instruction_offset == 4U &&
                    fixture.state.previous_opcode == group_case.opcode &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcodes 55-57 aliases move the role from its old group to the requested group"
            );
        }
    }

    Fixture source_selector;
    openswd3::world_map::LegacyRoleSpatialIndex source_spatial;
    reset_spatial(source_spatial);
    source_selector.roles[1].world_y = 32U;
    source_selector.roles[1].flags = 0U;
    source_selector.roles[2].guid = 0xFFF0U;
    source_selector.roles[2].world_y = 32U;
    source_selector.roles[2].flags = 0U;
    const bool source_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            source_spatial, source_selector.roles, 1U, 0U
        );
    source_selector.runtime.spatial_index = &source_spatial;
    prime_instruction(source_selector, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0xFFF0U);
    const auto source_result = source_selector.step();
    test.expect_true(
        source_inserted &&
            source_result.status == LegacyWorldStoryVmStatus::yielded &&
            (source_selector.roles[1].flags & 3U) == 1U &&
            (source_selector.roles[2].flags & 3U) == 0U &&
            source_spatial.row_heads[1U][role_row] == 1U,
        "shared role group handler translates FFF0 to the talk source"
    );

    Fixture controlled_selector;
    openswd3::world_map::LegacyRoleSpatialIndex controlled_spatial;
    reset_spatial(controlled_spatial);
    controlled_selector.roles[1].guid = 0xFFFEU;
    controlled_selector.roles[1].world_y = 32U;
    controlled_selector.roles[1].flags = 0U;
    controlled_selector.roles[2].world_y = 32U;
    controlled_selector.roles[2].flags = 0U;
    const bool controlled_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            controlled_spatial, controlled_selector.roles, 2U, 0U
        );
    controlled_selector.runtime.spatial_index = &controlled_spatial;
    prime_instruction(
        controlled_selector, OP_57_SET_ROLE_SPATIAL_GROUP_2, 0xFFFEU
    );
    const auto controlled_result = controlled_selector.step(0, 0, 2U);
    test.expect_true(
        controlled_inserted &&
            controlled_result.status == LegacyWorldStoryVmStatus::yielded &&
            (controlled_selector.roles[1].flags & 3U) == 0U &&
            (controlled_selector.roles[2].flags & 3U) == 2U &&
            controlled_spatial.row_heads[2U][role_row] == 2U,
        "shared role group handler passes FFFE through for controlled-role selection"
    );

    Fixture absent_from_chain;
    openswd3::world_map::LegacyRoleSpatialIndex empty_spatial;
    reset_spatial(empty_spatial);
    absent_from_chain.roles[1].world_y = 32U;
    absent_from_chain.roles[1].flags = 0U;
    absent_from_chain.runtime.spatial_index = &empty_spatial;
    prime_instruction(
        absent_from_chain, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto absent_result = absent_from_chain.step();
    test.expect_true(
        absent_result.status == LegacyWorldStoryVmStatus::yielded &&
            (absent_from_chain.roles[1].flags & 3U) == 1U &&
            empty_spatial.row_heads[0U][role_row] == 0U &&
            empty_spatial.row_heads[1U][role_row] == 0U &&
            absent_from_chain.context.instruction_offset == 4U &&
            absent_from_chain.state.previous_opcode ==
                OP_55_SET_ROLE_SPATIAL_GROUP_1,
        "shared role group handler treats a missing spatial-chain node as diagnostic-only"
    );

    Fixture logical_y;
    openswd3::world_map::LegacyRoleSpatialIndex logical_spatial;
    reset_spatial(logical_spatial);
    logical_y.roles[1].world_y = 0xFFFFFFFFU;
    logical_y.roles[1].flags = 0U;
    const bool logical_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            logical_spatial, logical_y.roles, 1U, 0U
        );
    logical_y.runtime.spatial_index = &logical_spatial;
    prime_instruction(logical_y, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U);
    const auto logical_result = logical_y.step();
    const std::size_t zero_row = openswd3::world_map::kLegacySpatialRowPadding;
    test.expect_true(
        logical_inserted &&
            logical_result.status == LegacyWorldStoryVmStatus::yielded &&
            (logical_y.roles[1].flags & 3U) == 1U &&
            logical_spatial.row_heads[0U][zero_row] == 1U &&
            logical_spatial.row_heads[1U][zero_row] == 0U,
        "shared role group handler preserves logical Y shift before the diagnostic-only miss"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    selector_truncated.state.previous_opcode = 0x66U;
    selector_truncated.roles[1].flags = 0x12345678U;
    write_u16(
        selector_truncated.state.window, 0x7FFEU, OP_55_SET_ROLE_SPATIAL_GROUP_1
    );
    const auto truncated_result = selector_truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated.roles[1].flags == 0x12345678U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x66U,
        "shared role group handler stops at the unsafe selector read"
    );

    Fixture missing_role;
    missing_role.context.instruction_offset = 0x7FFCU;
    missing_role.context.talk_data_offset = 0x1111U;
    missing_role.state.loaded_file_number = 1U;
    missing_role.state.loaded_data_offset = 0x1111U;
    missing_role.state.window_loaded = true;
    missing_role.state.previous_opcode = 0x66U;
    missing_role.roles[0].flags = 0x12345678U;
    write_u16(
        missing_role.state.window, 0x7FFCU, OP_55_SET_ROLE_SPATIAL_GROUP_1
    );
    write_u16(missing_role.state.window, 0x7FFEU, 0x7777U);
    const auto missing_result = missing_role.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_55_SET_ROLE_SPATIAL_GROUP_1 &&
            missing_result.executed_instruction_count == 1U &&
            missing_role.roles[0].flags == 0x12345678U &&
            missing_role.context.instruction_offset == 0x7FFCU &&
            missing_role.state.previous_opcode == 0x66U,
        "shared role group handler isolates the original negative role index before flags access"
    );

    Fixture missing_owner;
    missing_owner.roles[1].world_y = 32U;
    missing_owner.roles[1].flags = 0xA5A50000U;
    prime_instruction(missing_owner, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U);
    const auto owner_result = missing_owner.step();
    test.expect_true(
        owner_result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner.roles[1].flags == 0xA5A50001U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x66U,
        "shared role group handler updates flags before the missing spatial owner boundary"
    );

    Fixture invalid_old_group;
    openswd3::world_map::LegacyRoleSpatialIndex invalid_group_spatial;
    reset_spatial(invalid_group_spatial);
    invalid_old_group.roles[1].world_y = 32U;
    invalid_old_group.roles[1].flags = 0xA5A50003U;
    invalid_old_group.runtime.spatial_index = &invalid_group_spatial;
    prime_instruction(
        invalid_old_group, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto invalid_group_result = invalid_old_group.step();
    test.expect_true(
        invalid_group_result.status ==
                LegacyWorldStoryVmStatus::role_spatial_relocation_failed &&
            invalid_old_group.roles[1].flags == 0xA5A50001U &&
            invalid_old_group.context.instruction_offset == 0U &&
            invalid_old_group.state.previous_opcode == 0x66U,
        "shared role group handler preserves the flags write before invalid old-group isolation"
    );

    Fixture reinsertion_failure;
    openswd3::world_map::LegacyRoleSpatialIndex broken_spatial;
    reset_spatial(broken_spatial);
    reinsertion_failure.roles[1].world_y = 32U;
    reinsertion_failure.roles[1].flags = 0U;
    const bool broken_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            broken_spatial, reinsertion_failure.roles, 1U, 0U
        );
    broken_spatial.row_heads[1U].clear();
    reinsertion_failure.runtime.spatial_index = &broken_spatial;
    prime_instruction(
        reinsertion_failure, OP_55_SET_ROLE_SPATIAL_GROUP_1, 0x00F8U
    );
    const auto reinsertion_result = reinsertion_failure.step();
    test.expect_true(
        broken_inserted &&
            reinsertion_result.status ==
                LegacyWorldStoryVmStatus::role_spatial_relocation_failed &&
            (reinsertion_failure.roles[1].flags & 3U) == 1U &&
            broken_spatial.row_heads[0U][role_row] == 0U &&
            reinsertion_failure.roles[1].spatial_next_link_32 == 0U &&
            reinsertion_failure.context.instruction_offset == 0U &&
            reinsertion_failure.state.previous_opcode == 0x66U,
        "shared role group handler keeps completed unlink effects when reinsertion is isolated"
    );

    Fixture exact_tail;
    openswd3::world_map::LegacyRoleSpatialIndex tail_spatial;
    reset_spatial(tail_spatial);
    exact_tail.roles[1].world_y = 32U;
    exact_tail.roles[1].flags = 0U;
    const bool tail_inserted =
        openswd3::world_map::insert_legacy_role_spatially(
            tail_spatial, exact_tail.roles, 1U, 0U
        );
    exact_tail.runtime.spatial_index = &tail_spatial;
    exact_tail.context.instruction_offset = 0x7FFCU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(exact_tail.state.window, 0x7FFCU, OP_57_SET_ROLE_SPATIAL_GROUP_2);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x00F8U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_inserted &&
            tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            tail_result.opcode == OP_57_SET_ROLE_SPATIAL_GROUP_2 &&
            tail_result.executed_instruction_count == 1U &&
            (exact_tail.roles[1].flags & 3U) == 2U &&
            tail_spatial.row_heads[2U][role_row] == 1U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode == OP_57_SET_ROLE_SPATIAL_GROUP_2,
        "shared role group handler exact tail completes relocation and yields at the window end"
    );
}

void test_wait_for_role_action_position(openswd3::test::Context& test) {
    Fixture waiting;
    auto waiting_script = std::span<u8>{waiting.ports.initial_window};
    write_u16(waiting_script, 0U, 107U);
    write_u16(waiting_script, 2U, 0xFFF0U);
    write_u16(waiting_script, 4U, 5U);
    write_u16(waiting_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(waiting_script, 8U, 0x00F8U);
    waiting.roles[1].action.packed_ap_state = 0x0405U;
    const auto stalled = waiting.step();
    const u16 stalled_offset = waiting.context.instruction_offset;
    waiting.roles[1].action.packed_ap_state = 0x0505U;
    const auto completed = waiting.step();

    Fixture invalid_threshold;
    auto invalid_script = std::span<u8>{invalid_threshold.ports.initial_window};
    write_u16(invalid_script, 0U, 107U);
    write_u16(invalid_script, 2U, 0x00F8U);
    write_u16(invalid_script, 4U, 5U);
    write_u16(invalid_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(invalid_script, 8U, 0x00F8U);
    invalid_threshold.roles[1].action.packed_ap_state = 0x0104U;
    const auto invalid = invalid_threshold.step();

    Fixture missing;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, 107U);
    write_u16(missing_script, 2U, 0x7777U);
    write_u16(missing_script, 4U, 5U);
    write_u16(missing_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing_script, 8U, 0x00F8U);
    const auto absent = missing.step();

    test.expect_true(
        stalled.status == LegacyWorldStoryVmStatus::yielded &&
            stalled.opcode == 107U &&
            stalled.executed_instruction_count == 1U && stalled_offset == 0U &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            completed.executed_instruction_count == 2U &&
            waiting.context.instruction_offset == 10U,
        "opcode 107 waits until the packed AP one-based index reaches its threshold"
    );
    test.expect_true(
        invalid.status == LegacyWorldStoryVmStatus::yielded &&
            invalid.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            invalid.executed_instruction_count == 2U &&
            invalid_threshold.context.instruction_offset == 10U &&
            absent.status == LegacyWorldStoryVmStatus::yielded &&
            absent.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            absent.executed_instruction_count == 2U &&
            missing.context.instruction_offset == 10U,
        "opcode 107 consumes invalid thresholds and missing roles without waiting"
    );
}

void test_release_role_path_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_18_RELEASE_ROLE_PATH,
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0x4000U),
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0x8000U),
        static_cast<u16>(OP_18_RELEASE_ROLE_PATH | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        for (auto& object : fixture.active_object_slots) {
            object.bytes.fill(0xFFU);
        }
        fixture.roles[1].flags = 0xA0000000U;
        fixture.roles[1].action.wait_remaining = 7U;
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0x00U, 1U);
        slot.bytes[0x1BU] = 2U;
        openswd3::world_map::LegacyWorldStoryPathRuntime paths{};
        paths.roles = fixture.roles;
        paths.active_object_slots = fixture.active_object_slots;
        fixture.runtime.story_paths = &paths;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_18_RELEASE_ROLE_PATH &&
                fixture.roles[1].flags == 0x20000000U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 18 aliases complete a type>1 slot and continue in-call"
        );
    }

    Fixture already_released;
    already_released.roles[1].flags = 0x20000000U;
    already_released.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(already_released, OP_18_RELEASE_ROLE_PATH);
    write_u16(already_released.state.window, 2U, 0x00F8U);
    write_u16(already_released.state.window, 4U, OP_1025);
    const auto already_released_result = already_released.step();
    test.expect_true(
        already_released_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            already_released_result.executed_instruction_count == 2U &&
            already_released.context.instruction_offset == 4U &&
            already_released.roles[1].flags == 0x20000000U &&
            already_released.roles[1].action.wait_remaining == 0U,
        "opcode 18 advances without a story-path runtime after bit31 is clear"
    );

    Fixture no_slot;
    no_slot.roles[1].flags = 0xA0000000U;
    no_slot.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(no_slot, OP_18_RELEASE_ROLE_PATH);
    write_u16(no_slot.state.window, 2U, 0x00F8U);
    write_u16(no_slot.state.window, 4U, OP_1025);
    const auto no_slot_result = no_slot.step();
    test.expect_true(
        no_slot_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            no_slot_result.executed_instruction_count == 3U &&
            no_slot.context.instruction_offset == 4U &&
            no_slot.state.previous_opcode == OP_18_RELEASE_ROLE_PATH &&
            no_slot.roles[1].flags == 0x20000000U &&
            no_slot.roles[1].action.wait_remaining == 0U,
        "opcode 18 clears bit31 then retries once in-call after no slot"
    );

    Fixture type_one;
    type_one.roles[1].flags = 0x80000000U;
    prime_loaded_instruction(type_one, OP_18_RELEASE_ROLE_PATH);
    write_u16(type_one.state.window, 2U, 0x00F8U);
    write_u16(type_one.state.window, 4U, OP_1025);
    write_u16(type_one.active_object_slots[0].bytes, 0x00U, 1U);
    type_one.active_object_slots[0].bytes[0x1BU] = 1U;
    const auto type_one_result = type_one.step();
    test.expect_true(
        type_one_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            type_one_result.executed_instruction_count == 3U &&
            type_one.context.instruction_offset == 4U &&
            type_one.active_object_slots[0].bytes[0x1BU] == 1U,
        "opcode 18 requires slot type low nibble greater than one"
    );

    Fixture missing;
    prime_loaded_instruction(missing, OP_18_RELEASE_ROLE_PATH);
    write_u16(missing.state.window, 2U, 0x7777U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 18 stops at the original helper role dereference after a miss"
    );

    Fixture raw_current_token;
    prime_loaded_instruction(raw_current_token, OP_18_RELEASE_ROLE_PATH);
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            raw_current_token.context.instruction_offset == 0U,
        "opcode 18 passes FFF0 raw without source substitution"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(invalid_controlled, OP_18_RELEASE_ROLE_PATH);
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 18 obeys the VM controlled-role entry safety boundary"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x80000000U;
    runtime_unavailable.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(runtime_unavailable, OP_18_RELEASE_ROLE_PATH);
    write_u16(runtime_unavailable.state.window, 2U, 0x00F8U);
    write_u16(runtime_unavailable.active_object_slots[0].bytes, 0x00U, 1U);
    runtime_unavailable.active_object_slots[0].bytes[0x1BU] = 2U;
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.roles[1].flags == 0x80000000U &&
            runtime_unavailable.roles[1].action.wait_remaining == 7U,
        "opcode 18 requires the typed owner only after a matching slot"
    );

    Fixture helper_failure;
    helper_failure.roles[1].flags = 0x80000000U;
    helper_failure.roles[1].action.wait_remaining = 7U;
    prime_loaded_instruction(helper_failure, OP_18_RELEASE_ROLE_PATH);
    write_u16(helper_failure.state.window, 2U, 0x00F8U);
    auto& failed_slot = helper_failure.active_object_slots[0];
    write_u16(failed_slot.bytes, 0x00U, 1U);
    write_u16(failed_slot.bytes, 0x08U, 2U);
    failed_slot.bytes[0x1BU] = 2U;
    openswd3::world_map::LegacyWorldStoryPathRuntime failed_paths{};
    failed_paths.roles = helper_failure.roles;
    failed_paths.active_object_slots = helper_failure.active_object_slots;
    helper_failure.runtime.story_paths = &failed_paths;
    const auto helper_failure_result = helper_failure.step();
    test.expect_true(
        helper_failure_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            helper_failure.context.instruction_offset == 0U &&
            helper_failure.roles[1].flags == 0x80000000U &&
            helper_failure.roles[1].action.wait_remaining == 7U,
        "opcode 18 checked-stops when chained-path ownership is unavailable"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.roles[1].flags = 0x80000000U;
    truncated.roles[1].action.wait_remaining = 7U;
    write_u16(truncated.state.window, 0x7FFEU, OP_18_RELEASE_ROLE_PATH);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.roles[1].flags == 0x80000000U &&
            truncated.roles[1].action.wait_remaining == 7U,
        "opcode 18 reads the selector before any helper or role side effect"
    );
}

void test_release_all_role_paths_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_19_RELEASE_ROLE_PATHS,
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0x4000U),
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0x8000U),
        static_cast<u16>(OP_19_RELEASE_ROLE_PATHS | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        for (auto& object : fixture.active_object_slots) {
            object.bytes.fill(0xFFU);
        }
        fixture.roles[0].flags = 0xA0000000U;
        fixture.roles[0].action.wait_remaining = 5U;
        fixture.roles[1].flags = 0xA0000000U;
        fixture.roles[1].action.wait_remaining = 7U;
        fixture.roles[2].flags = 0xA0000000U;
        fixture.roles[2].action.wait_remaining = 9U;
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0x00U, 2U);
        slot.bytes[0x1BU] = 2U;
        openswd3::world_map::LegacyWorldStoryPathRuntime paths{};
        paths.roles = fixture.roles;
        paths.active_object_slots = fixture.active_object_slots;
        fixture.runtime.story_paths = &paths;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS &&
                fixture.roles[0].flags == 0xA0000000U &&
                fixture.roles[0].action.wait_remaining == 5U &&
                fixture.roles[1].flags == 0x20000000U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[2].flags == 0x20000000U &&
                fixture.roles[2].action.wait_remaining == 0U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 19 aliases release flagged roles except role zero"
        );
    }

    Fixture already_released;
    already_released.roles[1].flags = 0x20000000U;
    already_released.roles[1].action.wait_remaining = 7U;
    already_released.roles[2].action.wait_remaining = 9U;
    prime_loaded_instruction(already_released, OP_19_RELEASE_ROLE_PATHS);
    write_u16(already_released.state.window, 2U, OP_1025);
    const auto already_released_result = already_released.step();
    test.expect_true(
        already_released_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            already_released_result.executed_instruction_count == 2U &&
            already_released.context.instruction_offset == 2U &&
            already_released.roles[1].action.wait_remaining == 7U &&
            already_released.roles[2].action.wait_remaining == 9U,
        "opcode 19 leaves bit31-clear role action waits untouched"
    );

    Fixture type_one;
    type_one.roles[1].flags = 0x80000000U;
    type_one.roles[1].action.wait_remaining = 7U;
    write_u16(type_one.active_object_slots[0].bytes, 0x00U, 1U);
    type_one.active_object_slots[0].bytes[0x1BU] = 1U;
    prime_loaded_instruction(type_one, OP_19_RELEASE_ROLE_PATHS);
    write_u16(type_one.state.window, 2U, OP_1025);
    const auto type_one_result = type_one.step();
    test.expect_true(
        type_one_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            type_one_result.executed_instruction_count == 2U &&
            type_one.roles[1].flags == 0U &&
            type_one.roles[1].action.wait_remaining == 0U &&
            type_one.active_object_slots[0].bytes[0x1BU] == 1U,
        "opcode 19 ignores helper zero return for a type-one slot"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x80000000U;
    runtime_unavailable.roles[1].action.wait_remaining = 7U;
    runtime_unavailable.roles[2].flags = 0x80000000U;
    runtime_unavailable.roles[2].action.wait_remaining = 9U;
    write_u16(runtime_unavailable.active_object_slots[0].bytes, 0x00U, 1U);
    runtime_unavailable.active_object_slots[0].bytes[0x1BU] = 2U;
    prime_loaded_instruction(runtime_unavailable, OP_19_RELEASE_ROLE_PATHS);
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.roles[1].flags == 0x80000000U &&
            runtime_unavailable.roles[1].action.wait_remaining == 7U &&
            runtime_unavailable.roles[2].flags == 0x80000000U &&
            runtime_unavailable.roles[2].action.wait_remaining == 9U,
        "opcode 19 stops in role order when a matching slot lacks its owner"
    );

    Fixture one_role;
    one_role.roles[0].flags = 0x80000000U;
    one_role.roles[0].action.wait_remaining = 5U;
    prime_loaded_instruction(one_role, OP_19_RELEASE_ROLE_PATHS);
    write_u16(one_role.state.window, 2U, OP_1025);
    auto one_role_span =
        std::span<LegacyWorldRoleRecord>{one_role.roles}.first(1U);
    const auto one_role_result =
        openswd3::world_map::step_legacy_world_story_vm(
            one_role.context,
            one_role.state,
            one_role_span,
            0U,
            one_role.active_object_slots,
            one_role.maps_payload,
            one_role.dialogs,
            one_role.dialog_resources,
            one_role.first_name,
            one_role.second_name,
            one_role.runtime,
            one_role.ports
        );
    test.expect_true(
        one_role_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            one_role_result.executed_instruction_count == 2U &&
            one_role.context.instruction_offset == 2U &&
            one_role.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS &&
            one_role.roles[0].flags == 0x80000000U &&
            one_role.roles[0].action.wait_remaining == 5U,
        "opcode 19 count-one path skips role zero and consumes two bytes"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFEU, OP_19_RELEASE_ROLE_PATHS);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.context.instruction_offset == 0x8000U &&
            truncated.state.previous_opcode == OP_19_RELEASE_ROLE_PATHS,
        "opcode 19 consumes at the window tail before the next fetch fails"
    );
}

void test_schedule_role_paths_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> opcode20_aliases{
        OP_20_SCHEDULE_ROLE_PATHS,
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0x4000U),
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0x8000U),
        static_cast<u16>(OP_20_SCHEDULE_ROLE_PATHS | 0xC000U)
    };
    for (const u16 raw_word : opcode20_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        fixture.roles[1].flags = 0x00080000U;
        fixture.roles[1].action.cached_base_variant = 11U;
        fixture.roles[1].action.cached_variant_delta = 22U;
        fixture.roles[1].action.one_shot_base_variant = 33U;
        fixture.roles[1].action.one_shot_variant_delta = 44U;
        fixture.roles[1].action.wait_override = 0U;
        fixture.roles[1].interaction_gate = 0x1234U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 1U);
        write_u16(fixture.state.window, 4U, 0x00F8U);
        write_u16(fixture.state.window, 6U, 21U);
        write_u16(fixture.state.window, 8U, 15U);
        write_u16(fixture.state.window, 10U, OP_1025);

        const auto scheduled = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status == LegacyWorldStoryVmStatus::yielded &&
                scheduled.opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                scheduled.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                read_u16(fixture.state.window, 2U) == 0x4001U &&
                (fixture.roles[1].flags & 0x00080000U) == 0U &&
                fixture.roles[1].action.cached_base_variant ==
                    std::numeric_limits<u32>::max() &&
                fixture.roles[1].action.cached_variant_delta ==
                    std::numeric_limits<u32>::max() &&
                fixture.roles[1].action.one_shot_base_variant == 33U &&
                fixture.roles[1].action.one_shot_variant_delta == 44U &&
                fixture.roles[1].action.wait_override == 0x8001U &&
                fixture.roles[1].interaction_gate == 0x1234U &&
                read_u16(slot, 0x04U) == 336U &&
                read_u16(slot, 0x06U) == 240U &&
                read_u16(slot, 0x10U) == 0xFFFFU &&
                read_u16(slot, 0x12U) == 0xFFFFU &&
                read_u16(slot, 0x14U) == 0xFFFFU,
            "opcode 20 aliases schedule six-byte records and publish phase one"
        );

        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();
        test.expect_true(
            completed.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                completed.opcode == OP_1025 &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
                read_u16(fixture.state.window, 2U) == 1U,
            "opcode 20 ready phase clears high bits and continues in-call"
        );
    }

    constexpr std::array<u16, 4U> opcode169_aliases{
        OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS,
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0x4000U),
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0x8000U),
        static_cast<u16>(OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS | 0xC000U)
    };
    for (const u16 raw_word : opcode169_aliases) {
        Fixture fixture;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 1U);
        write_u16(fixture.state.window, 4U, 0x00F8U);
        write_u16(fixture.state.window, 6U, 21U);
        write_u16(fixture.state.window, 8U, 15U);
        write_u16(fixture.state.window, 10U, 0x1234U);
        write_u16(fixture.state.window, 12U, 0xFFFEU);
        write_u16(fixture.state.window, 14U, 7U);
        write_u16(fixture.state.window, 16U, OP_1025);

        const auto scheduled = fixture.step();
        const auto& slot = fixture.active_object_slots[0].bytes;
        test.expect_true(
            scheduled.status == LegacyWorldStoryVmStatus::yielded &&
                scheduled.opcode == OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                scheduled.executed_instruction_count == 1U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                read_u16(fixture.state.window, 2U) == 0x4001U &&
                read_u16(slot, 0x10U) == 0x1234U &&
                read_u16(slot, 0x12U) == 0xFFFEU && read_u16(slot, 0x14U) == 7U,
            "opcode 169 aliases forward twelve-byte action-bearing records"
        );

        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();
        test.expect_true(
            completed.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                completed.opcode == OP_1025 &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 16U &&
                fixture.state.previous_opcode ==
                    OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS &&
                read_u16(fixture.state.window, 2U) == 1U,
            "opcode 169 ready phase advances by twelve bytes per record"
        );
    }

    struct Variant {
        u16 opcode;
        std::size_t record_size;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_20_SCHEDULE_ROLE_PATHS, 6U},
        Variant{OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS, 12U},
    };
    for (const auto variant : variants) {
        Fixture empty;
        prime_loaded_instruction(empty, variant.opcode);
        write_u16(empty.state.window, 2U, 0U);
        write_u16(empty.state.window, 4U, OP_1025);
        const auto staged = empty.step();
        const u16 staged_count = read_u16(empty.state.window, 2U);
        const auto completed = empty.step();
        test.expect_true(
            staged.status == LegacyWorldStoryVmStatus::yielded &&
                staged.opcode == variant.opcode &&
                staged.executed_instruction_count == 1U &&
                staged_count == 0x4000U &&
                read_u16(empty.state.window, 2U) == 0U &&
                completed.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                completed.opcode == OP_1025 &&
                completed.executed_instruction_count == 2U &&
                empty.context.instruction_offset == 4U &&
                empty.state.previous_opcode == variant.opcode,
            "shared path handler count-zero stages then completes without runtime"
        );
    }

    Fixture selected_fallback;
    selected_fallback.roles[1].world_x = 352U;
    selected_fallback.roles[1].world_y = 256U;
    StoryPathHarness selected_paths{selected_fallback, 1U};
    prime_loaded_instruction(selected_fallback, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(selected_fallback.state.window, 2U, 0x8001U);
    write_u16(selected_fallback.state.window, 4U, 0xFFFEU);
    write_u16(selected_fallback.state.window, 6U, 0xFFFFU);
    write_u16(selected_fallback.state.window, 8U, 0xFFFFU);
    const auto selected_staged = selected_fallback.step(0, 0, 1U);
    const auto& selected_slot = selected_fallback.active_object_slots[0].bytes;
    test.expect_true(
        selected_staged.status == LegacyWorldStoryVmStatus::yielded &&
            read_u16(selected_fallback.state.window, 2U) == 0xC001U &&
            read_u16(selected_slot, 0x00U) == 1U &&
            read_u16(selected_slot, 0x04U) == 352U &&
            read_u16(selected_slot, 0x06U) == 256U &&
            (selected_slot[0x1BU] & 0x80U) == 0U &&
            (selected_fallback.dialogs.close.flagged_dialog_counter &
             0x8000U) != 0U,
        "opcode 20 forwards bit15 and uses selected-role FFFF coordinates"
    );
    selected_fallback.roles[1].flags |= 0x02000000U;
    write_u16(selected_fallback.state.window, 10U, OP_1025);
    const auto selected_completed = selected_fallback.step(0, 0, 1U);
    test.expect_true(
        selected_completed.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            selected_fallback.context.instruction_offset == 10U &&
            read_u16(selected_fallback.state.window, 2U) == 1U,
        "ready phase clears both bit14 and forwarded bit15"
    );

    for (const auto variant : variants) {
        Fixture missing_tail;
        missing_tail.context.instruction_offset = 0x7FFAU;
        missing_tail.context.talk_data_offset = 0x1111U;
        missing_tail.state.loaded_file_number = 1U;
        missing_tail.state.loaded_data_offset = 0x1111U;
        missing_tail.state.window_loaded = true;
        missing_tail.state.previous_opcode = 0x55U;
        write_u16(missing_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(missing_tail.state.window, 0x7FFCU, 1U);
        write_u16(missing_tail.state.window, 0x7FFEU, 0xFFF0U);
        const auto missing_tail_result = missing_tail.step();
        test.expect_true(
            missing_tail_result.status == LegacyWorldStoryVmStatus::yielded &&
                missing_tail_result.executed_instruction_count == 1U &&
                missing_tail.context.instruction_offset == 0x7FFAU &&
                missing_tail.state.previous_opcode == variant.opcode &&
                read_u16(missing_tail.state.window, 0x7FFCU) == 0x4001U,
            "missing selector skips its record tail before phase publication"
        );

        Fixture valid_tail;
        valid_tail.context.instruction_offset = 0x7FFAU;
        valid_tail.context.talk_data_offset = 0x1111U;
        valid_tail.state.loaded_file_number = 1U;
        valid_tail.state.loaded_data_offset = 0x1111U;
        valid_tail.state.window_loaded = true;
        valid_tail.state.previous_opcode = 0x55U;
        valid_tail.roles[1].flags = 0x00080000U;
        valid_tail.roles[1].action.cached_base_variant = 11U;
        valid_tail.roles[1].action.cached_variant_delta = 22U;
        write_u16(valid_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(valid_tail.state.window, 0x7FFCU, 1U);
        write_u16(valid_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto valid_tail_result = valid_tail.step();
        test.expect_true(
            valid_tail_result.status ==
                    LegacyWorldStoryVmStatus::operand_out_of_range &&
                valid_tail.context.instruction_offset == 0x7FFAU &&
                valid_tail.state.previous_opcode == 0x55U &&
                (valid_tail.roles[1].flags & 0x00080000U) == 0U &&
                valid_tail.roles[1].action.cached_base_variant ==
                    std::numeric_limits<u32>::max() &&
                valid_tail.roles[1].action.cached_variant_delta ==
                    std::numeric_limits<u32>::max() &&
                valid_tail.roles[1].action.wait_override == 0x8001U,
            "valid selector applies role effects before truncated coordinates"
        );

        Fixture wait_tail;
        wait_tail.context.instruction_offset = 0x7FFAU;
        wait_tail.context.talk_data_offset = 0x1111U;
        wait_tail.state.loaded_file_number = 1U;
        wait_tail.state.loaded_data_offset = 0x1111U;
        wait_tail.state.window_loaded = true;
        wait_tail.roles[1].flags = 0x02000000U;
        write_u16(wait_tail.state.window, 0x7FFAU, variant.opcode);
        write_u16(wait_tail.state.window, 0x7FFCU, 0x4001U);
        write_u16(wait_tail.state.window, 0x7FFEU, 0x00F8U);
        const auto wait_tail_result = wait_tail.step();
        const u16 expected_ip =
            static_cast<u16>(0x7FFAU + 4U + variant.record_size);
        test.expect_true(
            wait_tail_result.status ==
                    LegacyWorldStoryVmStatus::instruction_out_of_range &&
                wait_tail_result.executed_instruction_count == 1U &&
                wait_tail.context.instruction_offset == expected_ip &&
                wait_tail.state.previous_opcode == variant.opcode &&
                read_u16(wait_tail.state.window, 0x7FFCU) == 1U,
            "ready phase reads only selectors before advancing past the window"
        );
    }

    Fixture action_tail;
    action_tail.context.instruction_offset = 0x7FF4U;
    action_tail.context.talk_data_offset = 0x1111U;
    action_tail.state.loaded_file_number = 1U;
    action_tail.state.loaded_data_offset = 0x1111U;
    action_tail.state.window_loaded = true;
    action_tail.state.previous_opcode = 0x55U;
    action_tail.roles[1].flags = 0x00080000U;
    action_tail.roles[1].action.cached_base_variant = 11U;
    action_tail.roles[1].action.cached_variant_delta = 22U;
    write_u16(
        action_tail.state.window,
        0x7FF4U,
        OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS
    );
    write_u16(action_tail.state.window, 0x7FF6U, 1U);
    write_u16(action_tail.state.window, 0x7FF8U, 0x00F8U);
    write_u16(action_tail.state.window, 0x7FFAU, 21U);
    write_u16(action_tail.state.window, 0x7FFCU, 15U);
    write_u16(action_tail.state.window, 0x7FFEU, 0x1234U);
    const auto action_tail_result = action_tail.step();
    test.expect_true(
        action_tail_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            action_tail.context.instruction_offset == 0x7FF4U &&
            action_tail.state.previous_opcode == 0x55U &&
            (action_tail.roles[1].flags & 0x00080000U) == 0U &&
            action_tail.roles[1].action.cached_base_variant ==
                std::numeric_limits<u32>::max() &&
            action_tail.roles[1].action.cached_variant_delta ==
                std::numeric_limits<u32>::max() &&
            action_tail.roles[1].action.wait_override == 0x8001U &&
            read_u16(action_tail.state.window, 0x7FF6U) == 1U,
        "opcode 169 applies role effects before a truncated action tail"
    );

    Fixture wait_missing;
    prime_loaded_instruction(wait_missing, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(wait_missing.state.window, 2U, 0x4001U);
    write_u16(wait_missing.state.window, 4U, 0x7777U);
    wait_missing.state.previous_opcode = 0x55U;
    const auto wait_missing_result = wait_missing.step();
    test.expect_true(
        wait_missing_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            wait_missing.context.instruction_offset == 0U &&
            wait_missing.state.previous_opcode == 0x55U &&
            read_u16(wait_missing.state.window, 2U) == 0x4001U,
        "wait phase stops at the original role dereference after resolver miss"
    );

    Fixture not_ready;
    StoryPathHarness not_ready_paths{not_ready};
    prime_loaded_instruction(not_ready, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(not_ready.state.window, 2U, 0x4001U);
    write_u16(not_ready.state.window, 4U, 0x00F8U);
    const auto not_ready_result = not_ready.step();
    test.expect_true(
        not_ready_result.status == LegacyWorldStoryVmStatus::yielded &&
            not_ready_result.executed_instruction_count == 1U &&
            not_ready.context.instruction_offset == 0U &&
            not_ready.state.previous_opcode == OP_20_SCHEDULE_ROLE_PATHS &&
            read_u16(not_ready.state.window, 2U) == 0x4001U,
        "wait phase keeps the instruction staged while a role is not ready"
    );

    Fixture runtime_unavailable;
    runtime_unavailable.roles[1].flags = 0x00080000U;
    runtime_unavailable.roles[1].action.cached_base_variant = 11U;
    runtime_unavailable.roles[1].action.cached_variant_delta = 22U;
    prime_loaded_instruction(runtime_unavailable, OP_20_SCHEDULE_ROLE_PATHS);
    write_u16(runtime_unavailable.state.window, 2U, 1U);
    write_u16(runtime_unavailable.state.window, 4U, 0x00F8U);
    write_u16(runtime_unavailable.state.window, 6U, 21U);
    write_u16(runtime_unavailable.state.window, 8U, 15U);
    const auto runtime_unavailable_result = runtime_unavailable.step();
    test.expect_true(
        runtime_unavailable_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            runtime_unavailable.context.instruction_offset == 0U &&
            runtime_unavailable.state.previous_opcode == 0U &&
            (runtime_unavailable.roles[1].flags & 0x00080000U) == 0U &&
            runtime_unavailable.roles[1].action.cached_base_variant ==
                std::numeric_limits<u32>::max() &&
            runtime_unavailable.roles[1].action.cached_variant_delta ==
                std::numeric_limits<u32>::max() &&
            runtime_unavailable.roles[1].action.wait_override == 0x8001U &&
            read_u16(runtime_unavailable.state.window, 2U) == 1U,
        "initial phase preserves role effects before missing path runtime"
    );
}

void test_jump_if_global_bit_protocol(openswd3::test::Context& test) {
    struct Variant {
        u16 opcode;
        bool branch_when_set;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_21_JUMP_IF_GLOBAL_BIT_SET, true},
        Variant{OP_22_JUMP_IF_GLOBAL_BIT_CLEAR, false},
    };

    for (const auto variant : variants) {
        constexpr std::array<u16, 4U> modifiers{0U, 0x4000U, 0x8000U, 0xC000U};
        for (const u16 modifier : modifiers) {
            Fixture fixture;
            prime_loaded_instruction(
                fixture, static_cast<u16>(variant.opcode | modifier)
            );
            write_u16(fixture.state.window, 2U, 0x0123U);
            write_u32(fixture.state.window, 4U, 0x12345678U);
            if (variant.branch_when_set) {
                openswd3::world_map::set_legacy_world_story_flag(
                    fixture.state, 0x0123U
                );
            }
            write_u16(fixture.ports.transferred_window, 0U, OP_1025);
            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                    result.opcode == OP_1025 &&
                    result.executed_instruction_count == 2U &&
                    result.direct_audio_service_count == 1U &&
                    fixture.context.talk_data_offset == 0x12345678U &&
                    fixture.context.instruction_offset == 0U &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.data_load_count == 1U,
                "opcode 21/22 aliases branch on the XOR-selected flag state"
            );
        }

        Fixture sequential;
        prime_loaded_instruction(sequential, variant.opcode);
        write_u16(sequential.state.window, 2U, 0x0123U);
        write_u32(sequential.state.window, 4U, 0x12345678U);
        if (!variant.branch_when_set) {
            openswd3::world_map::set_legacy_world_story_flag(
                sequential.state, 0x0123U
            );
        }
        write_u16(sequential.state.window, 8U, OP_1025);
        const auto sequential_result = sequential.step();
        test.expect_true(
            sequential_result.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                sequential_result.opcode == OP_1025 &&
                sequential_result.executed_instruction_count == 2U &&
                sequential_result.direct_audio_service_count == 0U &&
                sequential.context.instruction_offset == 8U &&
                sequential.state.previous_opcode == variant.opcode &&
                sequential.ports.data_load_count == 0U,
            "opcode 21/22 advance eight bytes on the opposite flag state"
        );
    }

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_21_JUMP_IF_GLOBAL_BIT_SET);
    write_u16(load_failure.state.window, 2U, 0x0123U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    openswd3::world_map::set_legacy_world_story_flag(
        load_failure.state, 0x0123U
    );
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_21_JUMP_IF_GLOBAL_BIT_SET &&
            !load_failure.state.window_loaded,
        "opcode 21 preserves same-file loader effects before I/O failure"
    );

    Fixture missing_bit;
    missing_bit.context.instruction_offset = 0x7FFEU;
    missing_bit.context.talk_data_offset = 0x1111U;
    missing_bit.state.loaded_file_number = 1U;
    missing_bit.state.loaded_data_offset = 0x1111U;
    missing_bit.state.window_loaded = true;
    missing_bit.state.previous_opcode = 0x55U;
    write_u16(missing_bit.state.window, 0x7FFEU, OP_21_JUMP_IF_GLOBAL_BIT_SET);
    const auto missing_bit_result = missing_bit.step();
    test.expect_true(
        missing_bit_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_bit.context.instruction_offset == 0x7FFEU &&
            missing_bit.state.previous_opcode == 0x55U,
        "opcode 21 reads the bit selector before any control decision"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FFCU;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FFCU, OP_21_JUMP_IF_GLOBAL_BIT_SET
    );
    write_u16(missing_target.state.window, 0x7FFEU, 0x0123U);
    openswd3::world_map::set_legacy_world_story_flag(
        missing_target.state, 0x0123U
    );
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FFCU &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 21 branch reads the target at its original danger point"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FFCU;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FFCU, OP_21_JUMP_IF_GLOBAL_BIT_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFEU, 0x0123U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8004U &&
            sequential_tail.state.previous_opcode ==
                OP_21_JUMP_IF_GLOBAL_BIT_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 21 no-branch advances before the next fetch fails"
    );
}

void test_jump_if_all_global_bits_set_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET,
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0x4000U),
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0x8000U),
        static_cast<u16>(OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0456U);
        write_u16(fixture.state.window, 6U, 0xFF00U);
        write_u32(fixture.state.window, 8U, 0x12345678U);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0123U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0456U
        );
        write_u16(fixture.ports.transferred_window, 0U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
                fixture.ports.data_load_count == 1U,
            "opcode 23 aliases jump when every listed global bit is set"
        );
    }

    Fixture one_clear;
    prime_loaded_instruction(one_clear, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(one_clear.state.window, 2U, 0x0123U);
    write_u16(one_clear.state.window, 4U, 0x0456U);
    write_u16(one_clear.state.window, 6U, 0xFF00U);
    write_u32(one_clear.state.window, 8U, 0x12345678U);
    write_u16(one_clear.state.window, 12U, OP_1025);
    openswd3::world_map::set_legacy_world_story_flag(one_clear.state, 0x0123U);
    const auto one_clear_result = one_clear.step();
    test.expect_true(
        one_clear_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            one_clear_result.opcode == OP_1025 &&
            one_clear_result.executed_instruction_count == 2U &&
            one_clear_result.direct_audio_service_count == 0U &&
            one_clear.context.instruction_offset == 12U &&
            one_clear.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            one_clear.ports.data_load_count == 0U,
        "opcode 23 advances over the full list when any bit is clear"
    );

    Fixture empty;
    prime_loaded_instruction(empty, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(empty.state.window, 2U, 0xFF00U);
    write_u32(empty.state.window, 4U, 0x22223333U);
    write_u16(empty.ports.transferred_window, 0U, OP_1025);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            empty_result.opcode == OP_1025 &&
            empty_result.direct_audio_service_count == 1U &&
            empty.context.talk_data_offset == 0x22223333U &&
            empty.state.previous_opcode == OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET,
        "opcode 23 treats an empty bit list as unconditionally true"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET);
    write_u16(load_failure.state.window, 2U, 0xFF00U);
    write_u32(load_failure.state.window, 4U, 0x44445555U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            !load_failure.state.window_loaded,
        "opcode 23 preserves loader effects before taken-branch I/O failure"
    );

    Fixture unterminated;
    unterminated.context.instruction_offset = 0x7FFCU;
    unterminated.context.talk_data_offset = 0x1111U;
    unterminated.state.loaded_file_number = 1U;
    unterminated.state.loaded_data_offset = 0x1111U;
    unterminated.state.window_loaded = true;
    unterminated.state.previous_opcode = 0x55U;
    write_u16(
        unterminated.state.window, 0x7FFCU, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(unterminated.state.window, 0x7FFEU, 0x0123U);
    const auto unterminated_result = unterminated.step();
    test.expect_true(
        unterminated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            unterminated.context.instruction_offset == 0x7FFCU &&
            unterminated.state.previous_opcode == 0x55U,
        "opcode 23 scans for FF00 until the original list danger point"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FFCU;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FFCU, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(missing_target.state.window, 0x7FFEU, 0xFF00U);
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FFCU &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 23 taken empty-list branch reads the target after FF00"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FF8U;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FF8U, OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFAU, 0x0123U);
    write_u16(sequential_tail.state.window, 0x7FFCU, 0xFF00U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8002U &&
            sequential_tail.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 23 no-branch skips an unavailable target before fetch fails"
    );
}

void test_jump_if_any_global_bit_set_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET,
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0x4000U),
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0x8000U),
        static_cast<u16>(OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0456U);
        write_u16(fixture.state.window, 6U, 0xFF00U);
        write_u32(fixture.state.window, 8U, 0x12345678U);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0456U
        );
        write_u16(fixture.ports.transferred_window, 0U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.talk_data_offset == 0x12345678U &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.previous_opcode ==
                    OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
                fixture.ports.data_load_count == 1U,
            "opcode 24 aliases jump when any listed global bit is set"
        );
    }

    Fixture all_clear;
    prime_loaded_instruction(all_clear, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(all_clear.state.window, 2U, 0x0123U);
    write_u16(all_clear.state.window, 4U, 0x0456U);
    write_u16(all_clear.state.window, 6U, 0xFF00U);
    write_u32(all_clear.state.window, 8U, 0x12345678U);
    write_u16(all_clear.state.window, 12U, OP_1025);
    const auto all_clear_result = all_clear.step();
    test.expect_true(
        all_clear_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            all_clear_result.opcode == OP_1025 &&
            all_clear_result.executed_instruction_count == 2U &&
            all_clear_result.direct_audio_service_count == 0U &&
            all_clear.context.instruction_offset == 12U &&
            all_clear.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            all_clear.ports.data_load_count == 0U,
        "opcode 24 advances over the full list when every bit is clear"
    );

    Fixture empty;
    prime_loaded_instruction(empty, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(empty.state.window, 2U, 0xFF00U);
    write_u32(empty.state.window, 4U, 0x22223333U);
    write_u16(empty.state.window, 8U, OP_1025);
    const auto empty_result = empty.step();
    test.expect_true(
        empty_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            empty_result.opcode == OP_1025 &&
            empty_result.direct_audio_service_count == 0U &&
            empty.context.instruction_offset == 8U &&
            empty.state.previous_opcode == OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            empty.ports.data_load_count == 0U,
        "opcode 24 treats an empty bit list as false"
    );

    Fixture load_failure;
    prime_loaded_instruction(load_failure, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET);
    write_u16(load_failure.state.window, 2U, 0x0123U);
    write_u16(load_failure.state.window, 4U, 0xFF00U);
    write_u32(load_failure.state.window, 6U, 0x44445555U);
    openswd3::world_map::set_legacy_world_story_flag(
        load_failure.state, 0x0123U
    );
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.context.talk_data_offset == 0x44445555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            !load_failure.state.window_loaded,
        "opcode 24 preserves loader effects before taken-branch I/O failure"
    );

    Fixture missing_target;
    missing_target.context.instruction_offset = 0x7FF8U;
    missing_target.context.talk_data_offset = 0x1111U;
    missing_target.state.loaded_file_number = 1U;
    missing_target.state.loaded_data_offset = 0x1111U;
    missing_target.state.window_loaded = true;
    missing_target.state.previous_opcode = 0x55U;
    write_u16(
        missing_target.state.window, 0x7FF8U, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET
    );
    write_u16(missing_target.state.window, 0x7FFAU, 0x0123U);
    write_u16(missing_target.state.window, 0x7FFCU, 0xFF00U);
    openswd3::world_map::set_legacy_world_story_flag(
        missing_target.state, 0x0123U
    );
    const auto missing_target_result = missing_target.step();
    test.expect_true(
        missing_target_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_target.context.instruction_offset == 0x7FF8U &&
            missing_target.state.previous_opcode == 0x55U &&
            missing_target.ports.data_load_count == 0U,
        "opcode 24 taken branch reads the target only after FF00"
    );

    Fixture sequential_tail;
    sequential_tail.context.instruction_offset = 0x7FF8U;
    sequential_tail.context.talk_data_offset = 0x1111U;
    sequential_tail.state.loaded_file_number = 1U;
    sequential_tail.state.loaded_data_offset = 0x1111U;
    sequential_tail.state.window_loaded = true;
    sequential_tail.state.previous_opcode = 0x55U;
    write_u16(
        sequential_tail.state.window, 0x7FF8U, OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET
    );
    write_u16(sequential_tail.state.window, 0x7FFAU, 0x0123U);
    write_u16(sequential_tail.state.window, 0x7FFCU, 0xFF00U);
    const auto sequential_tail_result = sequential_tail.step();
    test.expect_true(
        sequential_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            sequential_tail_result.executed_instruction_count == 1U &&
            sequential_tail.context.instruction_offset == 0x8002U &&
            sequential_tail.state.previous_opcode ==
                OP_24_JUMP_IF_ANY_GLOBAL_BIT_SET &&
            sequential_tail.ports.data_load_count == 0U,
        "opcode 24 no-branch skips an unavailable target before fetch fails"
    );
}

void test_set_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_25_SET_GLOBAL_BIT,
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0x4000U),
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0x8000U),
        static_cast<u16>(OP_25_SET_GLOBAL_BIT | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0123U
                ) &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_25_SET_GLOBAL_BIT,
            "opcode 25 aliases set the addressed global bit then continue"
        );
    }

    Fixture already_set;
    prime_loaded_instruction(already_set, OP_25_SET_GLOBAL_BIT);
    write_u16(already_set.state.window, 2U, 0x0123U);
    write_u16(already_set.state.window, 4U, OP_1025);
    openswd3::world_map::set_legacy_world_story_flag(
        already_set.state, 0x0123U
    );
    const auto already_set_result = already_set.step();
    test.expect_true(
        already_set_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            openswd3::world_map::query_legacy_world_story_flag(
                already_set.state, 0x0123U
            ) &&
            already_set.state.previous_opcode == OP_25_SET_GLOBAL_BIT,
        "opcode 25 is idempotent when the bit is already set"
    );

    constexpr u16 last_valid_bit = static_cast<u16>(
        openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U - 1U
    );
    Fixture last_bit;
    prime_loaded_instruction(last_bit, OP_25_SET_GLOBAL_BIT);
    write_u16(last_bit.state.window, 2U, last_valid_bit);
    write_u16(last_bit.state.window, 4U, OP_1025);
    const auto last_bit_result = last_bit.step();
    test.expect_true(
        last_bit_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            openswd3::world_map::query_legacy_world_story_flag(
                last_bit.state, last_valid_bit
            ),
        "opcode 25 sets the final bit owned by the typed global flag array"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFEU, OP_25_SET_GLOBAL_BIT);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            !openswd3::world_map::query_legacy_world_story_flag(
                truncated.state, 0U
            ),
        "opcode 25 reads the bit operand before mutating global flags"
    );
}

void test_clear_global_bit_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_26_CLEAR_GLOBAL_BIT,
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0x4000U),
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0x8000U),
        static_cast<u16>(OP_26_CLEAR_GLOBAL_BIT | 0xC000U)
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, OP_1025);
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0120U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0123U
        );
        openswd3::world_map::set_legacy_world_story_flag(
            fixture.state, 0x0127U
        );
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0120U
                ) &&
                !openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0123U
                ) &&
                openswd3::world_map::query_legacy_world_story_flag(
                    fixture.state, 0x0127U
                ) &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_26_CLEAR_GLOBAL_BIT,
            "opcode 26 aliases clear only the addressed global bit"
        );
    }

    Fixture already_clear;
    prime_loaded_instruction(already_clear, OP_26_CLEAR_GLOBAL_BIT);
    write_u16(already_clear.state.window, 2U, 0x0123U);
    write_u16(already_clear.state.window, 4U, OP_1025);
    const auto already_clear_result = already_clear.step();
    test.expect_true(
        already_clear_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            !openswd3::world_map::query_legacy_world_story_flag(
                already_clear.state, 0x0123U
            ) &&
            already_clear.state.previous_opcode == OP_26_CLEAR_GLOBAL_BIT,
        "opcode 26 is idempotent when the bit is already clear"
    );

    constexpr u16 last_valid_bit = static_cast<u16>(
        openswd3::world_map::kLegacyWorldStoryFlagBytes * 8U - 1U
    );
    Fixture last_bit;
    prime_loaded_instruction(last_bit, OP_26_CLEAR_GLOBAL_BIT);
    write_u16(last_bit.state.window, 2U, last_valid_bit);
    write_u16(last_bit.state.window, 4U, OP_1025);
    openswd3::world_map::set_legacy_world_story_flag(
        last_bit.state, last_valid_bit
    );
    const auto last_bit_result = last_bit.step();
    test.expect_true(
        last_bit_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            !openswd3::world_map::query_legacy_world_story_flag(
                last_bit.state, last_valid_bit
            ),
        "opcode 26 clears the final bit owned by the typed global flag array"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    openswd3::world_map::set_legacy_world_story_flag(truncated.state, 0U);
    write_u16(truncated.state.window, 0x7FFEU, OP_26_CLEAR_GLOBAL_BIT);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            openswd3::world_map::query_legacy_world_story_flag(
                truncated.state, 0U
            ),
        "opcode 26 reads the bit operand before mutating global flags"
    );
}

void test_reload_world_session_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_27_RELOAD_WORLD_SESSION,
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0x4000U),
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0x8000U),
        static_cast<u16>(OP_27_RELOAD_WORLD_SESSION | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        prime_loaded_instruction(fixture, raw_word);
        fixture.runtime.role_surface.selected_guid = 0x00F8U;
        write_u16(fixture.state.window, 2U, 0x0123U);
        write_u16(fixture.state.window, 4U, 0x0045U);
        write_u16(fixture.state.window, 6U, 0x0067U);
        write_u16(fixture.state.window, 8U, 0x0089U);
        write_u16(fixture.state.window, 10U, 0x00ABU);
        write_u16(fixture.state.window, 12U, 0x00CDU);
        write_u16(fixture.state.window, 14U, OP_1025);
        const auto result = fixture.step();
        const auto& request = fixture.ports.last_world_load_request;
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 14U &&
                fixture.state.previous_opcode == OP_27_RELOAD_WORLD_SESSION &&
                fixture.ports.world_session_reload_begin_count == 1U &&
                fixture.ports.world_session_reload_count == 1U &&
                request.logical_map_id == 0x0123U &&
                request.tile_x == 0x0045U && request.tile_y == 0x0067U &&
                request.action_id == 0x0089U &&
                request.base_variant == 0x00ABU &&
                request.variant_delta == 0x00CDU &&
                request.selected_guid == 0x00F8U && request.load_flags == 1U,
            "opcode 27 aliases synchronously submit all six load operands"
        );
    }

    Fixture inherited;
    std::array<LegacyWorldRoleRecord, 2U> replacement_roles{};
    replacement_roles[1].guid = 0x0BEEU;
    prime_loaded_instruction(inherited, OP_27_RELOAD_WORLD_SESSION);
    inherited.runtime.role_surface.selected_guid = 0x00F8U;
    inherited.roles[1].action.action_id = 0x12345U;
    inherited.roles[1].action.base_variant = 0x23456U;
    inherited.roles[1].action.variant_delta = 0x34567U;
    inherited.ports.replacement_roles = replacement_roles;
    inherited.ports.replacement_controlled_role_index = 1U;
    inherited.ports.replacement_selected_guid = 0x0BEEU;
    write_u16(inherited.state.window, 2U, 120U);
    write_u16(inherited.state.window, 4U, 31U);
    write_u16(inherited.state.window, 6U, 29U);
    write_u16(inherited.state.window, 8U, 0xFFFFU);
    write_u16(inherited.state.window, 10U, 0xFFFFU);
    write_u16(inherited.state.window, 12U, 0xFFFFU);
    write_u16(inherited.state.window, 14U, OP_10_SET_ROLE_BASE_VARIANT);
    write_u16(inherited.state.window, 16U, 0xFFFEU);
    write_u16(inherited.state.window, 18U, 0x2222U);
    write_u16(inherited.state.window, 20U, OP_1025);
    const auto inherited_result = inherited.step(0, 0, 1U);
    const auto& inherited_request = inherited.ports.last_world_load_request;
    test.expect_true(
        inherited_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            inherited_result.opcode == OP_1025 &&
            inherited_result.executed_instruction_count == 3U &&
            inherited.context.instruction_offset == 20U &&
            inherited.state.previous_opcode == OP_10_SET_ROLE_BASE_VARIANT &&
            inherited_request.action_id == 0x2345U &&
            inherited_request.base_variant == 0x3456U &&
            inherited_request.variant_delta == 0x4567U &&
            inherited_request.selected_guid == 0x00F8U &&
            replacement_roles[1].action.base_variant == 0x2222U &&
            inherited.roles[1].action.base_variant == 0x23456U &&
            inherited.ports.story_protocol_events ==
                std::vector<u32>{6U, 7U, 4U},
        "opcode 27 resolves FFFF through low16 action fields then continues " "against the synchronously rebound world"
    );

    Fixture load_failed;
    prime_loaded_instruction(load_failed, OP_27_RELOAD_WORLD_SESSION);
    load_failed.state.previous_opcode = 0x55U;
    load_failed.ports.world_session_reload_success = false;
    write_u16(load_failed.state.window, 2U, 1U);
    write_u16(load_failed.state.window, 4U, 2U);
    write_u16(load_failed.state.window, 6U, 3U);
    write_u16(load_failed.state.window, 8U, 4U);
    write_u16(load_failed.state.window, 10U, 5U);
    write_u16(load_failed.state.window, 12U, 6U);
    const auto load_failed_result = load_failed.step();
    test.expect_true(
        load_failed_result.status ==
                LegacyWorldStoryVmStatus::world_session_load_failed &&
            load_failed.context.instruction_offset == 0U &&
            load_failed.state.previous_opcode == 0x55U &&
            load_failed.ports.world_session_reload_begin_count == 1U &&
            load_failed.ports.world_session_reload_count == 1U,
        "opcode 27 publishes no IP or previous opcode after reload failure"
    );

    Fixture missing_role;
    prime_loaded_instruction(missing_role, OP_27_RELOAD_WORLD_SESSION);
    missing_role.state.previous_opcode = 0x55U;
    write_u16(missing_role.state.window, 2U, 1U);
    write_u16(missing_role.state.window, 4U, 2U);
    write_u16(missing_role.state.window, 6U, 3U);
    write_u16(missing_role.state.window, 8U, 0xFFFFU);
    write_u16(missing_role.state.window, 10U, 5U);
    write_u16(missing_role.state.window, 12U, 6U);
    const auto missing_role_result = missing_role.step(0, 0, 99U);
    test.expect_true(
        missing_role_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            missing_role.context.instruction_offset == 0U &&
            missing_role.state.previous_opcode == 0x55U &&
            missing_role.ports.world_session_reload_begin_count == 0U &&
            missing_role.ports.world_session_reload_count == 0U &&
            missing_role.ports.story_protocol_events.empty(),
        "the existing VM entry guard rejects an invalid controlled role " "before opcode 27 dispatch"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FF4U;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FF4U, OP_27_RELOAD_WORLD_SESSION);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FF4U &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.world_session_reload_begin_count == 0U &&
            truncated.ports.world_session_reload_count == 0U,
        "opcode 27 reads the complete fourteen-byte record before transition " "effects"
    );
}

void test_change_role_path_id_protocol(openswd3::test::Context& test) {
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 selector,
                          const u16 path_id) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, selector);
        write_u16(fixture.state.window, 4U, path_id);
    };

    constexpr std::array<u16, 4U> raw_aliases{
        OP_28_CHANGE_ROLE_PATH_ID,
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0x4000U),
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0x8000U),
        static_cast<u16>(OP_28_CHANGE_ROLE_PATH_ID | 0xC000U),
    };
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.roles[1].path_word_index = 7U;
        fixture.roles[1].path_data_id = 0x1111U;
        prime(fixture, raw_word, 0x00F8U, 0x2468U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
                fixture.roles[1].path_data_id == 0x2468U &&
                fixture.roles[1].path_word_index == 0U &&
                (fixture.roles[1].flags & 0x1000U) != 0U &&
                fixture.ports.direct_audio_service_count == 1U,
            "opcode 28 aliases update the live role then yield after audio service"
        );
    }

    Fixture missing;
    prime(missing, OP_28_CHANGE_ROLE_PATH_ID, 0x4321U, 0x1357U);
    const auto missing_result = missing.step();
    const auto& patch = missing.ports.role_patch_requests.front();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::yielded &&
            missing.ports.role_patch_requests.size() == 1U &&
            patch.guid == 0x4321U && patch.action_id == 0xFFFFU &&
            patch.base_variant == 0xFFFFU && patch.variant_delta == 0xFFFFU &&
            patch.tile_x == 0xFFFFU && patch.tile_y == 0xFFFFU &&
            patch.talk_script_id == 0xFFFFU && patch.path_data_id == 0x1357U &&
            patch.flags_or_mask == 0x1000U && patch.flags_and_mask == 0xFFFFU &&
            patch.logical_map_id == 0xFFFFU &&
            missing.context.instruction_offset == 6U &&
            missing.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            missing.ports.direct_audio_service_count == 1U,
        "opcode 28 preserves all other MAPS fields on a missing live role"
    );

    Fixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime(invalid_controlled, OP_28_CHANGE_ROLE_PATH_ID, 0xFFFEU, 0x1357U);
    const auto invalid_controlled_result = invalid_controlled.step(0, 0, 99U);
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.role_patch_requests.empty() &&
            invalid_controlled.ports.direct_audio_service_count == 0U,
        "opcode 28 isolates an invalid controlled-role selector before MAPS patching"
    );

    Fixture type_two;
    type_two.roles[1].path_payload_relation = 0x11111111U;
    type_two.roles[1].path_payload_pointer_32 = 0x22222222U;
    auto& type_two_slot = type_two.active_object_slots[0];
    type_two_slot.bytes.fill(0x5AU);
    write_u16(type_two_slot.bytes, 0U, 1U);
    type_two_slot.bytes[0x1BU] = 2U;
    prime(type_two, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x3456U);
    const auto type_two_result = type_two.step();
    test.expect_true(
        type_two_result.status == LegacyWorldStoryVmStatus::yielded &&
            type_two.ports.role_path_payload_release_count == 1U &&
            type_two.ports.released_role_path_index == 1U &&
            type_two.roles[1].path_payload_relation == 0U &&
            type_two.roles[1].path_payload_pointer_32 == 0U &&
            std::ranges::all_of(
                type_two_slot.bytes.begin() + 8,
                type_two_slot.bytes.begin() + 16,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            type_two_slot.bytes[0x10U] == 0x5AU &&
            type_two_result.active_object_reset_count == 0U,
        "opcode 28 releases role payload then clears only four type-2 link words"
    );

    Fixture aligned;
    auto& aligned_slot = aligned.active_object_slots[0];
    aligned_slot.bytes.fill(0x33U);
    write_u16(aligned_slot.bytes, 0U, 1U);
    aligned_slot.bytes[0x1BU] = 1U;
    prime(aligned, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x4567U);
    const auto aligned_result = aligned.step();
    test.expect_true(
        aligned_result.status == LegacyWorldStoryVmStatus::yielded &&
            aligned_result.active_object_reset_count == 1U &&
            std::ranges::all_of(
                aligned_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ),
        "opcode 28 resets an aligned matching type-1 object without runtime owners"
    );

    const auto test_alignment = [&](const u32 start_x,
                                    const u32 start_y,
                                    const u8 direction,
                                    const u32 flags,
                                    const bool provide_surface) {
        Fixture fixture;
        fixture.roles[1].world_x = start_x;
        fixture.roles[1].world_y = start_y;
        fixture.roles[1].flags = flags;
        StoryPathHarness harness(fixture);
        fixture.runtime.spatial_index = &harness.spatial;
        fixture.runtime.role_surface = provide_surface
            ? harness.runtime.role_surface
            : openswd3::world_map::LegacyWorldRoleSurfaceContext{};
        auto& slot = fixture.active_object_slots[0];
        write_u16(slot.bytes, 0U, 1U);
        write_u16(slot.bytes, 2U, 0U);
        slot.bytes[0x1BU] = 1U;
        slot.bytes[0x1CU] = direction;
        prime(fixture, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x5678U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                fixture.roles[1].world_x == 320U &&
                fixture.roles[1].world_y == 240U &&
                result.active_object_reset_count == 1U &&
                std::ranges::all_of(
                    slot.bytes, [](const u8 value) { return value == 0xFFU; }
                ),
            "opcode 28 aligns type-1 movement backward before spatial reinsertion"
        );
    };
    test_alignment(324U, 244U, 0U, 0U, true);
    test_alignment(316U, 236U, 4U, 0U, true);
    test_alignment(324U, 244U, 0U, 0x4000U, false);

    Fixture invalid_direction;
    invalid_direction.state.previous_opcode = 0x55U;
    invalid_direction.roles[1].world_x = 324U;
    invalid_direction.roles[1].flags = 0x4000U;
    auto& invalid_slot = invalid_direction.active_object_slots[0];
    write_u16(invalid_slot.bytes, 0U, 1U);
    write_u16(invalid_slot.bytes, 2U, 0U);
    invalid_slot.bytes[0x1BU] = 1U;
    invalid_slot.bytes[0x1CU] = 8U;
    prime(invalid_direction, OP_28_CHANGE_ROLE_PATH_ID, 0x00F8U, 0x6789U);
    const auto invalid_direction_result = invalid_direction.step();
    test.expect_true(
        invalid_direction_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            invalid_direction.context.instruction_offset == 0U &&
            invalid_direction.state.previous_opcode == 0x55U &&
            invalid_direction.roles[1].path_data_id == 0U &&
            invalid_direction.ports.direct_audio_service_count == 0U &&
            invalid_slot.bytes[0x1BU] == 1U,
        "opcode 28 isolates an invalid type-1 direction without publishing the instruction"
    );

    Fixture live_truncated;
    live_truncated.context.instruction_offset = 0x7FFCU;
    live_truncated.context.talk_data_offset = 0x1111U;
    live_truncated.state.loaded_file_number = 1U;
    live_truncated.state.loaded_data_offset = 0x1111U;
    live_truncated.state.window_loaded = true;
    live_truncated.state.previous_opcode = 0x55U;
    live_truncated.roles[1].path_payload_relation = 0x11111111U;
    live_truncated.roles[1].path_payload_pointer_32 = 0x22222222U;
    write_u16(live_truncated.state.window, 0x7FFCU, OP_28_CHANGE_ROLE_PATH_ID);
    write_u16(live_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto live_truncated_result = live_truncated.step();
    test.expect_true(
        live_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            live_truncated.context.instruction_offset == 0x7FFCU &&
            live_truncated.state.previous_opcode == 0x55U &&
            live_truncated.ports.role_path_payload_release_count == 1U &&
            live_truncated.roles[1].path_payload_relation == 0U &&
            live_truncated.roles[1].path_payload_pointer_32 == 0U &&
            live_truncated.roles[1].path_word_index == 0U &&
            live_truncated.roles[1].path_data_id == 0U,
        "opcode 28 reads path id only after live-role payload and object effects"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    missing_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_truncated.state.window, 0x7FFCU, OP_28_CHANGE_ROLE_PATH_ID
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x4321U);
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U &&
            missing_truncated.ports.role_patch_requests.empty() &&
            missing_truncated.ports.direct_audio_service_count == 0U,
        "opcode 28 missing-role fallback reads path id before MAPS patching"
    );
}

void test_global_integer_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    const auto prime = [](Fixture& fixture,
                          const u16 raw_word,
                          const u16 index,
                          const u16 value) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, index);
        write_u16(fixture.state.window, 4U, value);
    };

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000001U;
        prime(
            fixture,
            static_cast<u16>(OP_29_SET_GLOBAL_INTEGER | mask),
            2U,
            0xFFFFU
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_29_SET_GLOBAL_INTEGER &&
                fixture.state.script_variables[0] == 0U &&
                fixture.state.script_variables[2] == 0xFFFFFFFFU,
            "opcode 29 aliases sign-extend the value and clamp variable zero"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[2] = 0xFFFFFFF0U;
        prime(
            fixture, static_cast<u16>(OP_30_ADD_GLOBAL_INTEGER | mask), 2U, 32U
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode == OP_30_ADD_GLOBAL_INTEGER &&
                fixture.state.script_variables[2] == 0x10U,
            "opcode 30 aliases add a sign-extended value with u32 wrapping"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[2] = 5U;
        prime(
            fixture,
            static_cast<u16>(OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO | mask),
            2U,
            10U
        );
        write_u16(fixture.state.window, 6U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO &&
                fixture.state.script_variables[2] == 0U,
            "opcode 31 aliases clamp a sign-bit subtraction result to zero"
        );
    }

    Fixture add_above_path_limit;
    add_above_path_limit.state.script_variables[2] = 990U;
    prime(add_above_path_limit, OP_30_ADD_GLOBAL_INTEGER, 2U, 50U);
    write_u16(add_above_path_limit.state.window, 6U, OP_1025);
    const auto add_above_path_limit_result = add_above_path_limit.step();
    test.expect_true(
        add_above_path_limit_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            add_above_path_limit.state.script_variables[2] == 1040U,
        "story opcode 30 does not inherit the PATH VM 1000-value cap"
    );

    Fixture subtract_negative;
    subtract_negative.state.script_variables[2] = 3U;
    prime(
        subtract_negative, OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO, 2U, 0xFFFBU
    );
    write_u16(subtract_negative.state.window, 6U, OP_1025);
    const auto subtract_negative_result = subtract_negative.step();
    test.expect_true(
        subtract_negative_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            subtract_negative.state.script_variables[2] == 8U,
        "opcode 31 subtracts a negative s16 as its wrapped u32 bit pattern"
    );

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000000U;
        fixture.state.script_variables[2] = 0U;
        prime(
            fixture,
            static_cast<u16>(OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE | mask),
            2U,
            0xFFFFU
        );
        write_u32(fixture.state.window, 6U, 0x12345678U);
        write_u16(fixture.state.window, 10U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
                fixture.state.script_variables[0] == 0U &&
                fixture.ports.data_load_count == 0U,
            "opcode 32 aliases compare against sign-extended threshold bits as unsigned"
        );
    }

    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_variables[0] = 0x80000000U;
        fixture.state.script_variables[2] = 0xFFFFFFFFU;
        prime(
            fixture,
            static_cast<u16>(OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE | mask),
            2U,
            0U
        );
        write_u32(fixture.state.window, 6U, 0x12345678U);
        write_u16(fixture.state.window, 10U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 10U &&
                fixture.state.previous_opcode ==
                    OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
                fixture.state.script_variables[0] == 0U &&
                fixture.ports.data_load_count == 0U,
            "opcode 33 aliases use an unsigned less-or-equal comparison"
        );
    }

    Fixture ge_taken;
    ge_taken.state.script_variables[0] = 0x80000000U;
    ge_taken.state.script_variables[2] = 0xFFFFFFFFU;
    prime(ge_taken, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 2U, 0xFFFFU);
    write_u32(ge_taken.state.window, 6U, 0x2222U);
    write_u16(ge_taken.ports.transferred_window, 0U, OP_1025);
    const auto ge_taken_result = ge_taken.step();
    test.expect_true(
        ge_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            ge_taken_result.executed_instruction_count == 2U &&
            ge_taken.context.talk_data_offset == 0x2222U &&
            ge_taken.context.instruction_offset == 0U &&
            ge_taken.state.previous_opcode ==
                OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            ge_taken.state.script_variables[0] == 0U &&
            ge_taken.ports.data_load_count == 1U &&
            ge_taken.ports.last_data_offset == 0x2222U &&
            ge_taken.ports.direct_audio_service_count == 1U,
        "opcode 32 taken path reloads then clamps variable zero before continuation"
    );

    Fixture le_taken;
    le_taken.state.script_variables[2] = 0U;
    prime(le_taken, OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE, 2U, 0xFFFFU);
    write_u32(le_taken.state.window, 6U, 0x3333U);
    write_u16(le_taken.ports.transferred_window, 0U, OP_1025);
    const auto le_taken_result = le_taken.step();
    test.expect_true(
        le_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            le_taken_result.executed_instruction_count == 2U &&
            le_taken.context.talk_data_offset == 0x3333U &&
            le_taken.state.previous_opcode ==
                OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
            le_taken.ports.data_load_count == 1U &&
            le_taken.ports.direct_audio_service_count == 1U,
        "opcode 33 taken path accepts zero below a sign-extended negative threshold"
    );

    constexpr std::array<u16, 5U> opcodes{
        OP_29_SET_GLOBAL_INTEGER,
        OP_30_ADD_GLOBAL_INTEGER,
        OP_31_SUBTRACT_GLOBAL_INTEGER_CLAMP_ZERO,
        OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE,
        OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE,
    };
    for (const u16 opcode : opcodes) {
        Fixture fixture;
        fixture.context.instruction_offset = 0x7FFAU;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x55U;
        fixture.state.script_variables[0] = 0x80000000U;
        write_u16(fixture.state.window, 0x7FFAU, opcode);
        write_u16(fixture.state.window, 0x7FFCU, 64U);
        write_u16(fixture.state.window, 0x7FFEU, 1U);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == opcode &&
                result.executed_instruction_count == 1U &&
                result.direct_audio_service_count == 1U &&
                fixture.context.instruction_offset == 0x7FFAU &&
                fixture.state.previous_opcode == opcode &&
                fixture.state.script_variables[0] == 0x80000000U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 1U,
            "opcodes 29-33 high index yields without target read or shared clamp"
        );
    }

    Fixture negative_update;
    negative_update.state.previous_opcode = 0x55U;
    prime(negative_update, OP_29_SET_GLOBAL_INTEGER, 0xFFFFU, 1U);
    const auto negative_update_result = negative_update.step();
    test.expect_true(
        negative_update_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_update.context.instruction_offset == 0U &&
            negative_update.state.previous_opcode == 0x55U &&
            negative_update.ports.direct_audio_service_count == 0U,
        "negative story-variable index stops at the original write unsafe point"
    );

    Fixture negative_branch;
    negative_branch.state.previous_opcode = 0x55U;
    prime(
        negative_branch, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 0xFFFFU, 0U
    );
    write_u32(negative_branch.state.window, 6U, 0x4444U);
    const auto negative_branch_result = negative_branch.step();
    test.expect_true(
        negative_branch_result.status ==
                LegacyWorldStoryVmStatus::script_variable_index_out_of_range &&
            negative_branch.context.instruction_offset == 0U &&
            negative_branch.state.previous_opcode == 0x55U &&
            negative_branch.ports.data_load_count == 0U,
        "negative conditional index reads target before the original read unsafe point"
    );

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFCU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    write_u16(value_truncated.state.window, 0x7FFCU, OP_29_SET_GLOBAL_INTEGER);
    write_u16(value_truncated.state.window, 0x7FFEU, 2U);
    const auto value_truncated_result = value_truncated.step();
    test.expect_true(
        value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFCU,
        "shared numeric entry requires index and value before dispatch"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFAU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    write_u16(
        target_truncated.state.window,
        0x7FFAU,
        OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE
    );
    write_u16(target_truncated.state.window, 0x7FFCU, 0xFFFFU);
    write_u16(target_truncated.state.window, 0x7FFEU, 0U);
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFAU,
        "negative conditional index still performs the earlier target read"
    );

    Fixture load_failure;
    load_failure.state.script_variables[0] = 0x80000000U;
    load_failure.state.script_variables[2] = 1U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime(load_failure, OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE, 2U, 1U);
    write_u32(load_failure.state.window, 6U, 0x5555U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x5555U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            load_failure.state.script_variables[0] == 0U &&
            load_failure.ports.direct_audio_service_count == 1U,
        "taken numeric branch preserves loader then shared-tail failure order"
    );
}

void test_set_bounded_script_clock_protocol(openswd3::test::Context& test) {
    struct Sample {
        u16 value;
        u32 expected;
    };
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    constexpr std::array<Sample, 4U> samples{
        Sample{0U, 0U},
        Sample{1000U, 1000U},
        Sample{1001U, 0U},
        Sample{0xFFFFU, 0U},
    };

    for (const u16 mask : alias_masks) {
        for (const auto sample : samples) {
            Fixture fixture;
            fixture.state.script_clock = 77U;
            fixture.state.script_clock_frame_counter = 9U;
            fixture.state.script_clock_origin = 88U;
            prime_loaded_instruction(
                fixture, static_cast<u16>(OP_34_SET_BOUNDED_SCRIPT_CLOCK | mask)
            );
            write_u16(fixture.state.window, 2U, sample.value);
            write_u16(fixture.state.window, 4U, OP_1025);
            const auto result = fixture.step();
            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                    result.opcode == OP_1025 &&
                    result.executed_instruction_count == 2U &&
                    fixture.context.instruction_offset == 4U &&
                    fixture.state.previous_opcode ==
                        OP_34_SET_BOUNDED_SCRIPT_CLOCK &&
                    fixture.state.script_clock == sample.expected &&
                    fixture.state.script_clock_frame_counter == 9U &&
                    fixture.state.script_clock_origin == 88U &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcode 34 aliases set or reset the bounded script clock"
            );
        }
    }

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFEU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.state.previous_opcode = 0x55U;
    truncated.state.script_clock = 77U;
    write_u16(truncated.state.window, 0x7FFEU, OP_34_SET_BOUNDED_SCRIPT_CLOCK);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated.context.instruction_offset == 0x7FFEU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.state.script_clock == 77U,
        "opcode 34 rejects a missing u16 before writing the script clock"
    );
}

void test_jump_if_byte_le_script_clock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock = 0x12340001U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK | mask)
        );
        fixture.state.window[2] = 2U;
        fixture.state.window[3] = 0xA5U;
        write_u32(fixture.state.window, 4U, 0x2222U);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
                fixture.state.script_clock == 0x12340001U &&
                fixture.state.window[3] == 0xA5U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 35 aliases compare the byte against only clock low16"
        );
    }

    Fixture equality_taken;
    equality_taken.state.script_clock = 0x123400FFU;
    prime_loaded_instruction(
        equality_taken, OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    equality_taken.state.window[2] = 0xFFU;
    equality_taken.state.window[3] = 0xA5U;
    write_u32(equality_taken.state.window, 4U, 0x3333U);
    write_u16(equality_taken.ports.transferred_window, 0U, OP_1025);
    const auto equality_taken_result = equality_taken.step();
    test.expect_true(
        equality_taken_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            equality_taken_result.executed_instruction_count == 2U &&
            equality_taken.context.talk_data_offset == 0x3333U &&
            equality_taken.context.instruction_offset == 0U &&
            equality_taken.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            equality_taken.ports.data_load_count == 1U &&
            equality_taken.ports.last_data_offset == 0x3333U &&
            equality_taken.ports.direct_audio_service_count == 1U,
        "opcode 35 equality takes the same-file branch and continues"
    );

    Fixture no_target_needed;
    no_target_needed.context.instruction_offset = 0x7FFDU;
    no_target_needed.context.talk_data_offset = 0x1111U;
    no_target_needed.state.loaded_file_number = 1U;
    no_target_needed.state.loaded_data_offset = 0x1111U;
    no_target_needed.state.window_loaded = true;
    no_target_needed.state.script_clock = 0U;
    write_u16(
        no_target_needed.state.window,
        0x7FFDU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    no_target_needed.state.window[0x7FFFU] = 0xFFU;
    const auto no_target_needed_result = no_target_needed.step();
    test.expect_true(
        no_target_needed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_needed.context.instruction_offset == 0x8005U &&
            no_target_needed.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            no_target_needed.ports.data_load_count == 0U &&
            no_target_needed.ports.direct_audio_service_count == 0U,
        "opcode 35 not-taken path neither reads padding nor requires target"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFDU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.previous_opcode = 0x55U;
    target_truncated.state.script_clock = 0xFFU;
    write_u16(
        target_truncated.state.window,
        0x7FFDU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    target_truncated.state.window[0x7FFFU] = 0xFFU;
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFDU &&
            target_truncated.state.previous_opcode == 0x55U &&
            target_truncated.ports.data_load_count == 0U,
        "opcode 35 taken path reads target only after the comparison"
    );

    Fixture value_truncated;
    value_truncated.context.instruction_offset = 0x7FFEU;
    value_truncated.context.talk_data_offset = 0x1111U;
    value_truncated.state.loaded_file_number = 1U;
    value_truncated.state.loaded_data_offset = 0x1111U;
    value_truncated.state.window_loaded = true;
    value_truncated.state.previous_opcode = 0x55U;
    write_u16(
        value_truncated.state.window,
        0x7FFEU,
        OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK
    );
    const auto value_truncated_result = value_truncated.step();
    test.expect_true(
        value_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            value_truncated.context.instruction_offset == 0x7FFEU &&
            value_truncated.state.previous_opcode == 0x55U,
        "opcode 35 requires the value byte before comparing"
    );

    Fixture load_failure;
    load_failure.state.script_clock = 1U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime_loaded_instruction(load_failure, OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK);
    load_failure.state.window[2] = 1U;
    load_failure.state.window[3] = 0xA5U;
    write_u32(load_failure.state.window, 4U, 0x4444U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x4444U &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode ==
                OP_35_JUMP_IF_BYTE_LE_SCRIPT_CLOCK &&
            load_failure.ports.direct_audio_service_count == 1U,
        "opcode 35 taken path preserves the checked loader failure order"
    );
}

void test_jump_if_script_clock_exceeds_origin_delta_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock_origin = 0xFFFFFFF0U;
        fixture.state.script_clock = 0x10U;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA | mask
            )
        );
        write_u16(fixture.state.window, 2U, 0x20U);
        write_u32(fixture.state.window, 4U, 0x2222U);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
                fixture.state.script_clock_origin == 0xFFFFFFF0U &&
                fixture.state.script_clock == 0x10U &&
                fixture.ports.data_load_count == 0U &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 36 aliases use wrapped threshold and strict comparison"
        );
    }

    Fixture taken;
    taken.state.script_clock_origin = 0xFFFFFFF0U;
    taken.state.script_clock = 0x11U;
    prime_loaded_instruction(
        taken, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(taken.state.window, 2U, 0x20U);
    write_u32(taken.state.window, 4U, 0x3333U);
    write_u16(taken.ports.transferred_window, 8U, OP_1025);
    const auto taken_result = taken.step();
    test.expect_true(
        taken_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            taken_result.executed_instruction_count == 2U &&
            taken.context.talk_data_offset == 0x3333U &&
            taken.context.instruction_offset == 8U &&
            taken.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            taken.ports.data_load_count == 1U &&
            taken.ports.last_data_offset == 0x3333U &&
            taken.ports.direct_audio_service_count == 1U &&
            taken.ports.beep_count == 0U,
        "opcode 36 taken path reloads then continues at new-window offset 8"
    );

    Fixture full_width_clock;
    full_width_clock.state.script_clock_origin = 0U;
    full_width_clock.state.script_clock = 0x10000U;
    prime_loaded_instruction(
        full_width_clock, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(full_width_clock.state.window, 2U, 1U);
    write_u32(full_width_clock.state.window, 4U, 0x3535U);
    write_u16(full_width_clock.ports.transferred_window, 8U, OP_1025);
    const auto full_width_clock_result = full_width_clock.step();
    test.expect_true(
        full_width_clock_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            full_width_clock.context.talk_data_offset == 0x3535U &&
            full_width_clock.context.instruction_offset == 8U &&
            full_width_clock.ports.data_load_count == 1U,
        "opcode 36 compares the full 32-bit script clock"
    );

    Fixture no_target_needed;
    no_target_needed.context.instruction_offset = 0x7FFCU;
    no_target_needed.context.talk_data_offset = 0x1111U;
    no_target_needed.state.loaded_file_number = 1U;
    no_target_needed.state.loaded_data_offset = 0x1111U;
    no_target_needed.state.window_loaded = true;
    no_target_needed.state.script_clock_origin = 0U;
    no_target_needed.state.script_clock = 1U;
    write_u16(
        no_target_needed.state.window,
        0x7FFCU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(no_target_needed.state.window, 0x7FFEU, 1U);
    const auto no_target_needed_result = no_target_needed.step();
    test.expect_true(
        no_target_needed_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_target_needed.context.instruction_offset == 0x8004U &&
            no_target_needed.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            no_target_needed.ports.data_load_count == 0U,
        "opcode 36 not-taken path does not require the target"
    );

    Fixture target_truncated;
    target_truncated.context.instruction_offset = 0x7FFCU;
    target_truncated.context.talk_data_offset = 0x1111U;
    target_truncated.state.loaded_file_number = 1U;
    target_truncated.state.loaded_data_offset = 0x1111U;
    target_truncated.state.window_loaded = true;
    target_truncated.state.previous_opcode = 0x55U;
    target_truncated.state.script_clock_origin = 0U;
    target_truncated.state.script_clock = 2U;
    write_u16(
        target_truncated.state.window,
        0x7FFCU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(target_truncated.state.window, 0x7FFEU, 1U);
    const auto target_truncated_result = target_truncated.step();
    test.expect_true(
        target_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            target_truncated.context.instruction_offset == 0x7FFCU &&
            target_truncated.state.previous_opcode == 0x55U &&
            target_truncated.ports.data_load_count == 0U,
        "opcode 36 taken path reads target only after comparison"
    );

    Fixture delta_truncated;
    delta_truncated.context.instruction_offset = 0x7FFEU;
    delta_truncated.context.talk_data_offset = 0x1111U;
    delta_truncated.state.loaded_file_number = 1U;
    delta_truncated.state.loaded_data_offset = 0x1111U;
    delta_truncated.state.window_loaded = true;
    delta_truncated.state.previous_opcode = 0x55U;
    write_u16(
        delta_truncated.state.window,
        0x7FFEU,
        OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    const auto delta_truncated_result = delta_truncated.step();
    test.expect_true(
        delta_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            delta_truncated.context.instruction_offset == 0x7FFEU &&
            delta_truncated.state.previous_opcode == 0x55U,
        "opcode 36 requires the complete u16 delta before comparison"
    );

    Fixture load_failure;
    load_failure.state.script_clock_origin = 0U;
    load_failure.state.script_clock = 2U;
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    prime_loaded_instruction(
        load_failure, OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA
    );
    write_u16(load_failure.state.window, 2U, 1U);
    write_u32(load_failure.state.window, 4U, 0x4444U);
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure.context.talk_data_offset == 0x4444U &&
            load_failure.context.instruction_offset == 8U &&
            load_failure.state.previous_opcode ==
                OP_36_JUMP_IF_SCRIPT_CLOCK_GT_ORIGIN_PLUS_DELTA &&
            load_failure.ports.direct_audio_service_count == 1U,
        "opcode 36 failure preserves loader then post-load +8 ordering"
    );
}

void test_snapshot_script_clock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        fixture.state.script_clock = 0x89ABCDEFU;
        fixture.state.script_clock_origin = 0x01234567U;
        fixture.state.script_clock_frame_counter = 20U;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_37_SNAPSHOT_SCRIPT_CLOCK | mask)
        );
        write_u16(fixture.state.window, 2U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.script_clock_origin == 0x89ABCDEFU &&
                fixture.state.script_clock == 0x89ABCDEFU &&
                fixture.state.script_clock_frame_counter == 20U &&
                fixture.state.previous_opcode == OP_37_SNAPSHOT_SCRIPT_CLOCK &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 37 aliases snapshot the full clock and continue"
        );
    }

    Fixture window_tail;
    window_tail.context.instruction_offset = 0x7FFEU;
    window_tail.context.talk_data_offset = 0x1111U;
    window_tail.state.loaded_file_number = 1U;
    window_tail.state.loaded_data_offset = 0x1111U;
    window_tail.state.window_loaded = true;
    window_tail.state.script_clock = 0xFEDCBA98U;
    window_tail.state.script_clock_origin = 0x01234567U;
    write_u16(window_tail.state.window, 0x7FFEU, OP_37_SNAPSHOT_SCRIPT_CLOCK);
    const auto window_tail_result = window_tail.step();
    test.expect_true(
        window_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            window_tail.context.instruction_offset == 0x8000U &&
            window_tail.state.script_clock_origin == 0xFEDCBA98U &&
            window_tail.state.script_clock == 0xFEDCBA98U &&
            window_tail.state.previous_opcode == OP_37_SNAPSHOT_SCRIPT_CLOCK,
        "opcode 37 needs no bytes beyond the two-byte opcode"
    );
}

void test_clear_role_from_scene_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture, static_cast<u16>(OP_38_CLEAR_ROLE_FROM_SCENE | mask)
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0xFFFFU &&
                request.tile_y == 0xFFFFU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0U &&
                request.flags_and_mask == 0x7FFFU &&
                request.logical_map_id == 0xFFFFU &&
                result.active_object_reset_count == 0U,
            "opcode 38 aliases patch only MAPS role flags on ordinary miss"
        );
    }

    Fixture raw_current_source_fallback;
    raw_current_source_fallback.context.source_guid = 0x4321U;
    prime_loaded_instruction(
        raw_current_source_fallback, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(raw_current_source_fallback.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_source_fallback.state.window, 4U, OP_1025);
    const auto raw_current_source_result = raw_current_source_fallback.step();
    test.expect_true(
        raw_current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_source_fallback.ports.role_patch_requests.size() ==
                1U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .guid == 0xFFF0U &&
            raw_current_source_fallback.context.instruction_offset == 4U &&
            raw_current_source_fallback.state.previous_opcode ==
                OP_38_CLEAR_ROLE_FROM_SCENE,
        "opcode 38 missing FFF0 lookup patches the original raw selector"
    );

    Fixture controlled_out_of_range;
    controlled_out_of_range.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        controlled_out_of_range, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(controlled_out_of_range.state.window, 2U, 0xFFFEU);
    const auto controlled_out_of_range_result =
        controlled_out_of_range.step(0, 0, 99U);
    test.expect_true(
        controlled_out_of_range_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            controlled_out_of_range_result.opcode == 0U &&
            controlled_out_of_range_result.executed_instruction_count == 0U &&
            controlled_out_of_range.context.instruction_offset == 0U &&
            controlled_out_of_range.state.previous_opcode == 0x55U &&
            controlled_out_of_range.ports.role_patch_requests.empty(),
        "opcode 38 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live_current_source_without_surface;
    live_current_source_without_surface.state.previous_opcode = 0x55U;
    live_current_source_without_surface.roles[1].flags = 0xE0009234U;
    prime_loaded_instruction(
        live_current_source_without_surface, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(live_current_source_without_surface.state.window, 2U, 0xFFF0U);
    const auto live_current_source_without_surface_result =
        live_current_source_without_surface.step();
    test.expect_true(
        live_current_source_without_surface_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            live_current_source_without_surface.context.instruction_offset ==
                0U &&
            live_current_source_without_surface.roles[1].flags == 0x1234U &&
            live_current_source_without_surface.state.previous_opcode ==
                0x55U &&
            live_current_source_without_surface.ports.role_patch_requests
                .empty(),
        "opcode 38 clears live FFF0 role flags before the surface unsafe point"
    );

    Fixture live;
    live.roles[0].guid = 0x00F8U;
    live.roles[0].flags = 0U;
    live.roles[1].guid = 0x00F8U;
    live.roles[1].flags = 0xE0009234U;
    live.roles[1].map_cell_pointer_32 = 0U;
    live.roles[1].action.field_2c = 1U;
    live.roles[1].action.field_30 = 1U;
    std::array<u8, 16U> surface{};
    surface.fill(0xFFU);
    live.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = surface,
        };
    live.active_object_slots[0].bytes.fill(0x22U);
    write_u16(live.active_object_slots[0].bytes, 0U, 0U);
    live.active_object_slots[1].bytes.fill(0x33U);
    write_u16(live.active_object_slots[1].bytes, 0U, 1U);
    live.active_object_slots.back().bytes.fill(0x44U);
    write_u16(live.active_object_slots.back().bytes, 0U, 0U);
    prime_loaded_instruction(live, OP_38_CLEAR_ROLE_FROM_SCENE);
    write_u16(live.state.window, 2U, 0xFFFEU);
    write_u16(live.state.window, 4U, OP_1025);
    const auto live_result = live.step(0, 0, 1U);
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 4U &&
            live.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
            live.roles[1].flags == 0x1234U &&
            read_u32(surface, 0U) == 0xCF7FFFFFU &&
            live_result.active_object_reset_count == 2U &&
            std::ranges::all_of(
                live.active_object_slots[0].bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.active_object_slots[1].bytes[2U] == 0x33U &&
            std::ranges::all_of(
                live.active_object_slots.back().bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.ports.role_patch_requests.empty(),
        "opcode 38 relooks up the first same-GUID role and scans all 72 slots"
    );

    Fixture wide_role_index;
    std::vector<LegacyWorldRoleRecord> wide_roles(0x10001U);
    auto& last_wide_role = wide_roles.back();
    last_wide_role.guid = 0x00F8U;
    last_wide_role.flags = 0xE0009234U;
    last_wide_role.map_cell_pointer_32 = 0U;
    last_wide_role.action.field_2c = 1U;
    last_wide_role.action.field_30 = 1U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        wide_slots{};
    wide_slots[0].bytes.fill(0x22U);
    write_u16(wide_slots[0].bytes, 0U, 0U);
    std::array<u8, 16U> wide_surface{};
    wide_surface.fill(0xFFU);
    wide_role_index.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = wide_surface,
        };
    prime_loaded_instruction(wide_role_index, OP_38_CLEAR_ROLE_FROM_SCENE);
    write_u16(wide_role_index.state.window, 2U, 0xFFFEU);
    write_u16(wide_role_index.state.window, 4U, OP_1025);
    const auto wide_role_index_result =
        openswd3::world_map::step_legacy_world_story_vm(
            wide_role_index.context,
            wide_role_index.state,
            wide_roles,
            0x10000U,
            wide_slots,
            wide_role_index.maps_payload,
            wide_role_index.dialogs,
            wide_role_index.dialog_resources,
            wide_role_index.first_name,
            wide_role_index.second_name,
            wide_role_index.runtime,
            wide_role_index.ports
        );
    test.expect_true(
        wide_role_index_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            wide_role_index_result.executed_instruction_count == 2U &&
            wide_role_index_result.active_object_reset_count == 0U &&
            last_wide_role.flags == 0x1234U &&
            wide_slots[0].bytes[2U] == 0x22U &&
            wide_role_index.ports.role_patch_requests.empty(),
        "opcode 38 compares object u16 index with full replacement u32"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FFCU;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window, 0x7FFCU, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    write_u16(no_following_bytes.state.window, 0x7FFEU, 0x1234U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_38_CLEAR_ROLE_FROM_SCENE &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 38 requires no bytes after its four-byte record"
    );

    Fixture operand_truncated;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x55U;
    write_u16(
        operand_truncated.state.window, 0x7FFEU, OP_38_CLEAR_ROLE_FROM_SCENE
    );
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x55U &&
            operand_truncated.ports.role_patch_requests.empty(),
        "opcode 38 requires the complete u16 selector before side effects"
    );
}

void test_set_role_flag_8000_and_clear_one_shots_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS | mask
            )
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode ==
                    OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0xFFFFU &&
                request.tile_y == 0xFFFFU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0x8000U &&
                request.flags_and_mask == 0xFFFFU &&
                request.logical_map_id == 0xFFFFU &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 39 aliases patch only the MAPS role flag on ordinary miss"
        );
    }

    Fixture raw_current_source_fallback;
    raw_current_source_fallback.context.source_guid = 0x4321U;
    prime_loaded_instruction(
        raw_current_source_fallback,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(raw_current_source_fallback.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_source_fallback.state.window, 4U, OP_1025);
    const auto raw_current_source_result = raw_current_source_fallback.step();
    test.expect_true(
        raw_current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_source_fallback.ports.role_patch_requests.size() ==
                1U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .guid == 0xFFF0U &&
            raw_current_source_fallback.ports.role_patch_requests.front()
                    .flags_or_mask == 0x8000U &&
            raw_current_source_fallback.context.instruction_offset == 4U &&
            raw_current_source_fallback.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS,
        "opcode 39 missing FFF0 lookup patches the original raw selector"
    );

    Fixture controlled_out_of_range;
    controlled_out_of_range.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        controlled_out_of_range, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(controlled_out_of_range.state.window, 2U, 0xFFFEU);
    const auto controlled_out_of_range_result =
        controlled_out_of_range.step(0, 0, 99U);
    test.expect_true(
        controlled_out_of_range_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            controlled_out_of_range_result.opcode == 0U &&
            controlled_out_of_range_result.executed_instruction_count == 0U &&
            controlled_out_of_range.context.instruction_offset == 0U &&
            controlled_out_of_range.state.previous_opcode == 0x55U &&
            controlled_out_of_range.ports.role_patch_requests.empty(),
        "opcode 39 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live_without_surface;
    live_without_surface.state.previous_opcode = 0x55U;
    live_without_surface.roles[1].flags = 0xA5A50001U;
    live_without_surface.roles[1].action.one_shot_base_variant = 0x11111111U;
    live_without_surface.roles[1].action.one_shot_variant_delta = 0x22222222U;
    prime_loaded_instruction(
        live_without_surface, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(live_without_surface.state.window, 2U, 0xFFF0U);
    const auto live_without_surface_result = live_without_surface.step();
    test.expect_true(
        live_without_surface_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            live_without_surface.context.instruction_offset == 0U &&
            live_without_surface.roles[1].flags == 0xA5A58001U &&
            live_without_surface.roles[1].action.one_shot_base_variant ==
                0x11111111U &&
            live_without_surface.roles[1].action.one_shot_variant_delta ==
                0x22222222U &&
            live_without_surface.state.previous_opcode == 0x55U &&
            live_without_surface.ports.role_patch_requests.empty(),
        "opcode 39 sets the live flag before the surface unsafe point"
    );

    Fixture partial_surface_failure;
    partial_surface_failure.state.previous_opcode = 0x55U;
    partial_surface_failure.roles[1].flags = 0xA5A50001U;
    partial_surface_failure.roles[1].map_cell_pointer_32 = 3U;
    partial_surface_failure.roles[1].action.field_2c = 2U;
    partial_surface_failure.roles[1].action.field_30 = 1U;
    partial_surface_failure.roles[1].action.one_shot_base_variant = 0x11111111U;
    partial_surface_failure.roles[1].action.one_shot_variant_delta =
        0x22222222U;
    std::array<u8, 16U> partial_surface{};
    partial_surface.fill(0xFFU);
    partial_surface_failure.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = partial_surface,
        };
    prime_loaded_instruction(
        partial_surface_failure, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(partial_surface_failure.state.window, 2U, 0xFFF0U);
    const auto partial_surface_failure_result = partial_surface_failure.step();
    test.expect_true(
        partial_surface_failure_result.status ==
                LegacyWorldStoryVmStatus::role_surface_failed &&
            partial_surface_failure.context.instruction_offset == 0U &&
            partial_surface_failure.roles[1].flags == 0xA5A58001U &&
            read_u32(partial_surface, 12U) == 0xCF7FFFFFU &&
            partial_surface_failure.roles[1].action.one_shot_base_variant ==
                0x11111111U &&
            partial_surface_failure.roles[1].action.one_shot_variant_delta ==
                0x22222222U &&
            partial_surface_failure.state.previous_opcode == 0x55U,
        "opcode 39 preserves partial surface effects before checked failure"
    );

    Fixture live;
    live.roles[1].flags = 0xA5A50001U;
    live.roles[1].map_cell_pointer_32 = 0U;
    live.roles[1].action.field_2c = 1U;
    live.roles[1].action.field_30 = 1U;
    live.roles[1].action.one_shot_base_variant = 0x11111111U;
    live.roles[1].action.one_shot_variant_delta = 0x22222222U;
    std::array<u8, 16U> surface{};
    surface.fill(0xFFU);
    live.runtime.role_surface =
        openswd3::world_map::LegacyWorldRoleSurfaceContext{
            .map_width = 2U,
            .selected_guid = 0U,
            .surface_grid = surface,
        };
    prime_loaded_instruction(
        live, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(live.state.window, 2U, 0xFFFEU);
    write_u16(live.state.window, 4U, OP_1025);
    const auto live_result = live.step(0, 0, 1U);
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 4U &&
            live.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            live.roles[1].flags == 0xA5A58001U &&
            read_u32(surface, 0U) == 0xCF7FFFFFU &&
            live.roles[1].action.one_shot_base_variant == 0xFFFFFFFFU &&
            live.roles[1].action.one_shot_variant_delta == 0xFFFFFFFFU &&
            live.ports.role_patch_requests.empty() &&
            live.ports.direct_audio_service_count == 0U,
        "opcode 39 clears surface before setting both one-shot fields to -1"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FFCU;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window,
        0x7FFCU,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    write_u16(no_following_bytes.state.window, 0x7FFEU, 0x1234U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 39 requires no bytes after its four-byte record"
    );

    Fixture operand_truncated;
    operand_truncated.context.instruction_offset = 0x7FFEU;
    operand_truncated.context.talk_data_offset = 0x1111U;
    operand_truncated.state.loaded_file_number = 1U;
    operand_truncated.state.loaded_data_offset = 0x1111U;
    operand_truncated.state.window_loaded = true;
    operand_truncated.state.previous_opcode = 0x55U;
    write_u16(
        operand_truncated.state.window,
        0x7FFEU,
        OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    const auto operand_truncated_result = operand_truncated.step();
    test.expect_true(
        operand_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            operand_truncated.context.instruction_offset == 0x7FFEU &&
            operand_truncated.state.previous_opcode == 0x55U &&
            operand_truncated.ports.role_patch_requests.empty(),
        "opcode 39 requires the complete u16 selector before side effects"
    );
}

void test_relocate_role_and_complete_path_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    for (const u16 mask : alias_masks) {
        Fixture fixture;
        prime_loaded_instruction(
            fixture,
            static_cast<u16>(OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH | mask)
        );
        write_u16(fixture.state.window, 2U, 0x1234U);
        write_u16(fixture.state.window, 4U, 0x1015U);
        write_u16(fixture.state.window, 6U, 0x100FU);
        write_u16(fixture.state.window, 8U, OP_1025);
        const auto result = fixture.step();
        const auto request = fixture.ports.role_patch_requests.empty()
            ? openswd3::world_map::LegacyMapsRolePatchRequest{}
            : fixture.ports.role_patch_requests.front();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == 8U &&
                fixture.state.previous_opcode ==
                    OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
                fixture.ports.role_patch_requests.size() == 1U &&
                request.guid == 0x1234U && request.action_id == 0xFFFFU &&
                request.base_variant == 0xFFFFU &&
                request.variant_delta == 0xFFFFU && request.tile_x == 0x1015U &&
                request.tile_y == 0x100FU &&
                request.talk_script_id == 0xFFFFU &&
                request.path_data_id == 0xFFFFU &&
                request.flags_or_mask == 0U &&
                request.flags_and_mask == 0xFFFFU &&
                request.logical_map_id == 0xFFFFU &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 40 aliases patch raw MAPS coordinates on ordinary miss"
        );
    }

    Fixture raw_current_token;
    prime_loaded_instruction(
        raw_current_token, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(raw_current_token.state.window, 2U, 0xFFF0U);
    write_u16(raw_current_token.state.window, 4U, 21U);
    write_u16(raw_current_token.state.window, 6U, 15U);
    write_u16(raw_current_token.state.window, 8U, OP_1025);
    const auto raw_current_token_result = raw_current_token.step();
    test.expect_true(
        raw_current_token_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            raw_current_token.ports.role_patch_requests.size() == 1U &&
            raw_current_token.ports.role_patch_requests.front().guid ==
                0xFFF0U &&
            raw_current_token.roles[1].world_x == 320U &&
            raw_current_token.roles[1].world_y == 240U,
        "opcode 40 treats FFF0 as a literal GUID rather than current source"
    );

    Fixture literal_current_token;
    literal_current_token.roles[2].guid = 0xFFF0U;
    prime_loaded_instruction(
        literal_current_token, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(literal_current_token.state.window, 2U, 0xFFF0U);
    write_u16(literal_current_token.state.window, 4U, 21U);
    write_u16(literal_current_token.state.window, 6U, 15U);
    const auto literal_current_token_result = literal_current_token.step();
    test.expect_true(
        literal_current_token_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            literal_current_token.context.instruction_offset == 0U &&
            literal_current_token.ports.role_patch_requests.empty(),
        "opcode 40 resolves a real FFF0 GUID without source substitution"
    );

    Fixture invalid_controlled;
    invalid_controlled.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        invalid_controlled, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 21U);
    write_u16(invalid_controlled.state.window, 6U, 15U);
    const auto invalid_controlled_result = invalid_controlled.step(0, 0, 99U);
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U &&
            invalid_controlled.ports.role_patch_requests.empty(),
        "opcode 40 invalid controlled owner stops at the VM session boundary"
    );

    Fixture live;
    live.roles[1].flags = 0x82000001U;
    live.roles[1].action.cached_base_variant = 7U;
    live.roles[1].action.cached_variant_delta = 8U;
    live.roles[1].action.wait_remaining = 9U;
    StoryPathHarness live_paths{live};
    auto& completed_slot = live.active_object_slots[0];
    write_u16(completed_slot.bytes, 0x00U, 1U);
    completed_slot.bytes[0x1BU] = 2U;
    prime_loaded_instruction(live, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    write_u16(live.state.window, 2U, 0x00F8U);
    write_u16(live.state.window, 4U, 0x1015U);
    write_u16(live.state.window, 6U, 0x100FU);
    write_u16(live.state.window, 8U, OP_1025);
    const auto live_result = live.step();
    test.expect_true(
        live_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            live_result.opcode == OP_1025 &&
            live_result.executed_instruction_count == 2U &&
            live.context.instruction_offset == 8U &&
            live.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            live.roles[1].world_x == 336U && live.roles[1].world_y == 240U &&
            live.roles[1].map_cell_pointer_32 == 771U &&
            live.roles[1].flags == 1U &&
            live.roles[1].action.cached_base_variant == 0xFFFFFFFFU &&
            live.roles[1].action.cached_variant_delta == 0xFFFFFFFFU &&
            live.roles[1].action.wait_remaining == 9U &&
            std::ranges::all_of(
                completed_slot.bytes,
                [](const u8 value) { return value == 0xFFU; }
            ) &&
            live.ports.role_patch_requests.empty() &&
            live.ports.direct_audio_service_count == 0U,
        "opcode 40 schedules, completes type2 ownership, then clears caller state"
    );

    Fixture controlled;
    controlled.roles[1].action.cached_base_variant = 7U;
    controlled.roles[1].action.cached_variant_delta = 8U;
    StoryPathHarness controlled_paths{controlled, 1U};
    prime_loaded_instruction(controlled, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 21U);
    write_u16(controlled.state.window, 6U, 15U);
    write_u16(controlled.state.window, 8U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 1U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.context.instruction_offset == 8U &&
            controlled.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            controlled.roles[1].action.cached_base_variant == 7U &&
            controlled.roles[1].action.cached_variant_delta == 8U &&
            controlled.ports.role_patch_requests.empty(),
        "opcode 40 compares the raw selector to source GUID for cache reset"
    );

    Fixture helper_failure;
    openswd3::world_map::LegacyWorldStoryPathRuntime incomplete_paths{};
    incomplete_paths.roles = helper_failure.roles;
    incomplete_paths.active_object_slots = helper_failure.active_object_slots;
    helper_failure.runtime.story_paths = &incomplete_paths;
    helper_failure.state.previous_opcode = 0x55U;
    prime_loaded_instruction(
        helper_failure, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(helper_failure.state.window, 2U, 0x00F8U);
    write_u16(helper_failure.state.window, 4U, 21U);
    write_u16(helper_failure.state.window, 6U, 15U);
    const auto helper_failure_result = helper_failure.step();
    test.expect_true(
        helper_failure_result.status ==
                LegacyWorldStoryVmStatus::role_path_failed &&
            helper_failure.context.instruction_offset == 0U &&
            helper_failure.state.previous_opcode == 0x55U &&
            helper_failure.roles[1].world_x == 320U &&
            helper_failure.roles[1].world_y == 240U,
        "opcode 40 stops at a checked sub_42DAF0 runtime failure"
    );

    Fixture found_truncated;
    found_truncated.context.instruction_offset = 0x7FFCU;
    found_truncated.context.talk_data_offset = 0x1111U;
    found_truncated.state.loaded_file_number = 1U;
    found_truncated.state.loaded_data_offset = 0x1111U;
    found_truncated.state.window_loaded = true;
    found_truncated.state.previous_opcode = 0x55U;
    write_u16(
        found_truncated.state.window,
        0x7FFCU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(found_truncated.state.window, 0x7FFEU, 0x00F8U);
    const auto found_truncated_result = found_truncated.step();
    test.expect_true(
        found_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            found_truncated_result.executed_instruction_count == 1U &&
            found_truncated.context.instruction_offset == 0x7FFCU &&
            found_truncated.state.previous_opcode == 0x55U &&
            found_truncated.ports.role_patch_requests.empty(),
        "opcode 40 reads destination operands only after a successful lookup"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    missing_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_truncated.state.window,
        0x7FFCU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated_result.executed_instruction_count == 1U &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U &&
            missing_truncated.ports.role_patch_requests.empty(),
        "opcode 40 missing-role MAPS operands are read after lookup failure"
    );

    Fixture missing_y_truncated;
    missing_y_truncated.context.instruction_offset = 0x7FFAU;
    missing_y_truncated.context.talk_data_offset = 0x1111U;
    missing_y_truncated.state.loaded_file_number = 1U;
    missing_y_truncated.state.loaded_data_offset = 0x1111U;
    missing_y_truncated.state.window_loaded = true;
    missing_y_truncated.state.previous_opcode = 0x55U;
    write_u16(
        missing_y_truncated.state.window,
        0x7FFAU,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(missing_y_truncated.state.window, 0x7FFCU, 0x7777U);
    write_u16(missing_y_truncated.state.window, 0x7FFEU, 21U);
    const auto missing_y_truncated_result = missing_y_truncated.step();
    test.expect_true(
        missing_y_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_y_truncated.context.instruction_offset == 0x7FFAU &&
            missing_y_truncated.state.previous_opcode == 0x55U &&
            missing_y_truncated.ports.role_patch_requests.empty(),
        "opcode 40 reads the +6 coordinate before using the available +4 word"
    );

    Fixture no_following_bytes;
    no_following_bytes.context.instruction_offset = 0x7FF8U;
    no_following_bytes.context.talk_data_offset = 0x1111U;
    no_following_bytes.state.loaded_file_number = 1U;
    no_following_bytes.state.loaded_data_offset = 0x1111U;
    no_following_bytes.state.window_loaded = true;
    write_u16(
        no_following_bytes.state.window,
        0x7FF8U,
        OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH
    );
    write_u16(no_following_bytes.state.window, 0x7FFAU, 0x7777U);
    write_u16(no_following_bytes.state.window, 0x7FFCU, 21U);
    write_u16(no_following_bytes.state.window, 0x7FFEU, 15U);
    const auto no_following_bytes_result = no_following_bytes.step();
    test.expect_true(
        no_following_bytes_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            no_following_bytes.context.instruction_offset == 0x8000U &&
            no_following_bytes.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            no_following_bytes.ports.role_patch_requests.size() == 1U,
        "opcode 40 requires no bytes after its eight-byte record"
    );
}

void test_reload_indexed_target_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> raw_aliases{
        OP_41_RELOAD_INDEXED_TARGET,
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0x4000U),
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0x8000U),
        static_cast<u16>(OP_41_RELOAD_INDEXED_TARGET | 0xC000U),
    };
    constexpr u32 first_target = 0x11112222U;
    constexpr u32 second_target = 0x33334444U;
    for (const u16 raw_word : raw_aliases) {
        Fixture fixture;
        fixture.indexed_target_selector = 1U;
        prime_loaded_instruction(fixture, raw_word);
        write_u32(fixture.state.window, 2U, first_target);
        write_u32(fixture.state.window, 6U, second_target);
        write_u32(fixture.state.window, 10U, 0xFF00FF00U);
        write_u16(fixture.ports.transferred_window, 0U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.indexed_target_selector == 0U &&
                fixture.context.talk_data_offset == second_target &&
                fixture.context.instruction_offset == 0U &&
                fixture.state.loaded_file_number == 1U &&
                fixture.state.loaded_data_offset == second_target &&
                fixture.state.window_loaded &&
                fixture.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET &&
                fixture.ports.data_load_count == 1U &&
                fixture.ports.last_data_file_number == 1U &&
                fixture.ports.last_data_offset == second_target &&
                !fixture.ports.last_data_clear_before_read &&
                fixture.ports.story_protocol_events == std::vector<u32>{2U, 5U},
            "opcode 41 aliases select one dword target and reload in same call"
        );
    }

    Fixture out_of_range;
    out_of_range.indexed_target_selector = 0x00010002U;
    prime_loaded_instruction(out_of_range, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(out_of_range.state.window, 2U, first_target);
    write_u32(out_of_range.state.window, 6U, second_target);
    write_u32(out_of_range.state.window, 10U, 0xFF00FF00U);
    write_u16(out_of_range.ports.transferred_window, 0U, OP_1025);
    const auto out_of_range_result = out_of_range.step();
    test.expect_true(
        out_of_range_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            out_of_range.ports.last_data_offset == first_target &&
            out_of_range.indexed_target_selector == 0U &&
            out_of_range.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "opcode 41 compares the full u32 selector and falls back to index zero"
    );

    Fixture sentinel_selected;
    sentinel_selected.indexed_target_selector = 2U;
    prime_loaded_instruction(sentinel_selected, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(sentinel_selected.state.window, 2U, first_target);
    write_u32(sentinel_selected.state.window, 6U, second_target);
    write_u32(sentinel_selected.state.window, 10U, 0xFF00FF00U);
    write_u16(sentinel_selected.ports.transferred_window, 0U, OP_1025);
    const auto sentinel_selected_result = sentinel_selected.step();
    test.expect_true(
        sentinel_selected_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            sentinel_selected.ports.last_data_offset == 0xFF00FF00U &&
            sentinel_selected.context.talk_data_offset == 0xFF00FF00U &&
            sentinel_selected.indexed_target_selector == 0U,
        "opcode 41 preserves the selector-equals-count sentinel target bug"
    );

    Fixture load_failure;
    load_failure.indexed_target_selector = 1U;
    prime_loaded_instruction(load_failure, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(load_failure.state.window, 2U, first_target);
    write_u32(load_failure.state.window, 6U, second_target);
    write_u32(load_failure.state.window, 10U, 0xFF00FF00U);
    load_failure.ports.data_load_status =
        LegacyTalkWindowStatus::data_read_failed;
    load_failure.state.previous_opcode = 0x55U;
    const auto load_failure_result = load_failure.step();
    test.expect_true(
        load_failure_result.status == LegacyWorldStoryVmStatus::load_failed &&
            load_failure_result.executed_instruction_count == 1U &&
            load_failure_result.load_status ==
                LegacyTalkWindowStatus::data_read_failed &&
            load_failure_result.direct_audio_service_count == 1U &&
            load_failure.indexed_target_selector == 0U &&
            load_failure.context.talk_data_offset == second_target &&
            load_failure.context.instruction_offset == 0U &&
            load_failure.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET &&
            !load_failure.state.window_loaded &&
            load_failure.ports.story_protocol_events ==
                std::vector<u32>{2U, 5U},
        "opcode 41 checked load failure preserves audio, target, reset, and previous"
    );

    Fixture missing_owner;
    missing_owner.indexed_target_selector = 9U;
    missing_owner.runtime.indexed_target_selector = nullptr;
    prime_loaded_instruction(missing_owner, OP_41_RELOAD_INDEXED_TARGET);
    missing_owner.state.previous_opcode = 0x55U;
    const auto missing_owner_result = missing_owner.step();
    test.expect_true(
        missing_owner_result.status ==
                LegacyWorldStoryVmStatus::runtime_unavailable &&
            missing_owner_result.executed_instruction_count == 1U &&
            missing_owner.indexed_target_selector == 9U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x55U &&
            missing_owner.ports.data_load_count == 0U &&
            missing_owner.ports.direct_audio_service_count == 0U,
        "opcode 41 stops at the missing indexed-selector owner"
    );

    Fixture truncated;
    truncated.context.instruction_offset = 0x7FFAU;
    truncated.context.talk_data_offset = 0x1111U;
    truncated.state.loaded_file_number = 1U;
    truncated.state.loaded_data_offset = 0x1111U;
    truncated.state.window_loaded = true;
    truncated.indexed_target_selector = 1U;
    truncated.state.previous_opcode = 0x55U;
    write_u16(truncated.state.window, 0x7FFAU, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(truncated.state.window, 0x7FFCU, first_target);
    const auto truncated_result = truncated.step();
    test.expect_true(
        truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            truncated_result.executed_instruction_count == 1U &&
            truncated.indexed_target_selector == 1U &&
            truncated.context.instruction_offset == 0x7FFAU &&
            truncated.state.previous_opcode == 0x55U &&
            truncated.ports.data_load_count == 0U &&
            truncated.ports.direct_audio_service_count == 0U,
        "opcode 41 checked scan stops when the FF00FF00 terminator is absent"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.indexed_target_selector = 0U;
    write_u16(exact_tail.state.window, 0x7FF6U, OP_41_RELOAD_INDEXED_TARGET);
    write_u32(exact_tail.state.window, 0x7FF8U, first_target);
    write_u32(exact_tail.state.window, 0x7FFCU, 0xFF00FF00U);
    write_u16(exact_tail.ports.transferred_window, 0U, OP_1025);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            exact_tail_result.executed_instruction_count == 2U &&
            exact_tail.ports.last_data_offset == first_target &&
            exact_tail.indexed_target_selector == 0U &&
            exact_tail.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "opcode 41 accepts a target table ending at the window boundary"
    );
}

void test_interaction_lock_protocol(openswd3::test::Context& test) {
    constexpr std::array<u16, 4U> set_aliases{
        OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0x4000U
        ),
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0x8000U
        ),
        static_cast<u16>(
            OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT | 0xC000U
        ),
    };
    for (const u16 raw_word : set_aliases) {
        Fixture fixture;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE0005U;
        fixture.roles[0].action.base_variant = 7U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.ports.action_update_count == 1U &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE8005U &&
                fixture.roles[0].action.base_variant == 0U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode ==
                    OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 42 aliases set shared lock, reset base variant, and continue"
        );
    }

    Fixture update_failure;
    update_failure.dialogs.close.flagged_dialog_counter = 0x12340002U;
    update_failure.roles[0].action.base_variant = 9U;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(
        update_failure, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    write_u16(update_failure.state.window, 2U, OP_1025);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.dialogs.close.flagged_dialog_counter ==
                0x12348002U &&
            update_failure.roles[0].action.base_variant == 0U &&
            update_failure.context.instruction_offset == 2U &&
            update_failure.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        "opcode 42 action-update failure is diagnostic-only after state writes"
    );

    constexpr std::array<u16, 4U> clear_aliases{
        OP_43_CLEAR_INTERACTION_LOCK,
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0x4000U),
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0x8000U),
        static_cast<u16>(OP_43_CLEAR_INTERACTION_LOCK | 0xC000U),
    };
    for (const u16 raw_word : clear_aliases) {
        Fixture fixture;
        fixture.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
        fixture.roles[0].action.base_variant = 7U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, OP_1025);
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 0U &&
                fixture.ports.action_update_count == 0U &&
                fixture.dialogs.close.flagged_dialog_counter == 0xCAFE0005U &&
                fixture.roles[0].action.base_variant == 7U &&
                fixture.context.instruction_offset == 2U &&
                fixture.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 43 aliases clear only shared interaction-lock bit fifteen"
        );
    }

    Fixture chained;
    chained.dialogs.close.flagged_dialog_counter = 0x12340003U;
    chained.roles[0].action.base_variant = 11U;
    prime_loaded_instruction(
        chained, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    write_u16(chained.state.window, 2U, OP_43_CLEAR_INTERACTION_LOCK);
    write_u16(chained.state.window, 4U, OP_1025);
    const auto chained_result = chained.step();
    test.expect_true(
        chained_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            chained_result.executed_instruction_count == 3U &&
            chained.dialogs.close.flagged_dialog_counter == 0x12340003U &&
            chained.roles[0].action.base_variant == 0U &&
            chained.ports.action_update_count == 1U &&
            chained.context.instruction_offset == 4U &&
            chained.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK,
        "opcodes 42 and 43 share one lock owner across same-call continuation"
    );

    Fixture invalid_controlled;
    invalid_controlled.dialogs.close.flagged_dialog_counter = 0x1234U;
    prime_loaded_instruction(
        invalid_controlled, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.dialogs.close.flagged_dialog_counter ==
                0x1234U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 42 invalid controlled owner stops at the VM session boundary"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFEU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.dialogs.close.flagged_dialog_counter = 0x1234U;
    exact_tail.roles[0].action.base_variant = 7U;
    write_u16(
        exact_tail.state.window,
        0x7FFEU,
        OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail.dialogs.close.flagged_dialog_counter == 0x9234U &&
            exact_tail.roles[0].action.base_variant == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT,
        "opcode 42 needs no following byte before its effects and previous publish"
    );

    Fixture clear_invalid_controlled;
    clear_invalid_controlled.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
    prime_loaded_instruction(
        clear_invalid_controlled, OP_43_CLEAR_INTERACTION_LOCK
    );
    clear_invalid_controlled.state.previous_opcode = 0x55U;
    const auto clear_invalid_controlled_result = clear_invalid_controlled.step(
        0, 0, static_cast<u32>(clear_invalid_controlled.roles.size())
    );
    test.expect_true(
        clear_invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            clear_invalid_controlled_result.opcode == 0U &&
            clear_invalid_controlled_result.executed_instruction_count == 0U &&
            clear_invalid_controlled.dialogs.close.flagged_dialog_counter ==
                0xCAFE8005U &&
            clear_invalid_controlled.context.instruction_offset == 0U &&
            clear_invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 43 invalid controlled owner stops at the VM session boundary"
    );

    Fixture clear_exact_tail;
    clear_exact_tail.context.instruction_offset = 0x7FFEU;
    clear_exact_tail.context.talk_data_offset = 0x1111U;
    clear_exact_tail.state.loaded_file_number = 1U;
    clear_exact_tail.state.loaded_data_offset = 0x1111U;
    clear_exact_tail.state.window_loaded = true;
    clear_exact_tail.dialogs.close.flagged_dialog_counter = 0xCAFE8005U;
    write_u16(
        clear_exact_tail.state.window, 0x7FFEU, OP_43_CLEAR_INTERACTION_LOCK
    );
    const auto clear_exact_tail_result = clear_exact_tail.step();
    test.expect_true(
        clear_exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            clear_exact_tail_result.executed_instruction_count == 1U &&
            clear_exact_tail_result.action_update_count == 0U &&
            clear_exact_tail.dialogs.close.flagged_dialog_counter ==
                0xCAFE0005U &&
            clear_exact_tail.context.instruction_offset == 0x8000U &&
            clear_exact_tail.state.previous_opcode ==
                OP_43_CLEAR_INTERACTION_LOCK,
        "opcode 43 needs no following byte before clear and previous publish"
    );
}

void test_set_role_action_wait_override_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> aliases{
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0x4000U),
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0x8000U),
        static_cast<u16>(OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE | 0xC000U),
    };
    for (const u16 raw_word : aliases) {
        Fixture fixture;
        fixture.roles[1].action.wait_remaining = 0xCAFEU;
        fixture.roles[1].action.wait_default = 0xBEEFU;
        fixture.roles[1].action.wait_override = 0x1234U;
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0x00F8U);
        write_u16(fixture.state.window, 4U, 0x8003U);
        write_u16(fixture.state.window, 6U, OP_1025);
        fixture.state.previous_opcode = 0x55U;
        const auto result = fixture.step();
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
                result.opcode == OP_1025 &&
                result.executed_instruction_count == 2U &&
                result.action_update_count == 1U &&
                result.action_update_failure_count == 0U &&
                fixture.ports.action_update_count == 1U &&
                fixture.roles[1].action.wait_remaining == 0U &&
                fixture.roles[1].action.wait_default == 0xBEEFU &&
                fixture.roles[1].action.wait_override == 0x8003U &&
                fixture.context.instruction_offset == 6U &&
                fixture.state.previous_opcode ==
                    OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
                fixture.ports.direct_audio_service_count == 0U,
            "opcode 44 aliases write wait override, clear remaining, and continue"
        );
    }

    Fixture current_source;
    current_source.roles[1].action.wait_remaining = 9U;
    prime_loaded_instruction(
        current_source, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(current_source.state.window, 2U, 0xFFF0U);
    write_u16(current_source.state.window, 4U, 0xFFFFU);
    write_u16(current_source.state.window, 6U, OP_1025);
    const auto current_source_result = current_source.step();
    test.expect_true(
        current_source_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            current_source.roles[1].action.wait_override == 0xFFFFU &&
            current_source.roles[1].action.wait_remaining == 0U &&
            current_source.roles[0].action.wait_override == 0U &&
            read_u16(current_source.state.window, 2U) == 0xFFF0U,
        "opcode 44 translates FFF0 to the context GUID before lookup"
    );

    Fixture controlled;
    controlled.roles[1].guid = 0xFFFEU;
    controlled.roles[1].action.wait_override = 0x1111U;
    controlled.roles[2].action.wait_remaining = 7U;
    prime_loaded_instruction(controlled, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    write_u16(controlled.state.window, 2U, 0xFFFEU);
    write_u16(controlled.state.window, 4U, 0x8123U);
    write_u16(controlled.state.window, 6U, OP_1025);
    const auto controlled_result = controlled.step(0, 0, 2U);
    test.expect_true(
        controlled_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            controlled.roles[2].action.wait_override == 0x8123U &&
            controlled.roles[2].action.wait_remaining == 0U &&
            controlled.roles[1].action.wait_override == 0x1111U,
        "opcode 44 passes FFFE to the helper for direct controlled-index selection"
    );

    Fixture first_clear_match;
    first_clear_match.roles[0].guid = 0x2222U;
    first_clear_match.roles[0].flags =
        openswd3::world_map::kLegacyWorldGuidLookupSkipBit;
    first_clear_match.roles[1].guid = 0x2222U;
    first_clear_match.roles[2].guid = 0x2222U;
    first_clear_match.roles[1].action.wait_remaining = 5U;
    first_clear_match.roles[2].action.wait_remaining = 7U;
    prime_loaded_instruction(
        first_clear_match, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(first_clear_match.state.window, 2U, 0x2222U);
    write_u16(first_clear_match.state.window, 4U, 0x8004U);
    write_u16(first_clear_match.state.window, 6U, OP_1025);
    const auto first_clear_match_result = first_clear_match.step();
    test.expect_true(
        first_clear_match_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            first_clear_match.roles[0].action.wait_override == 0U &&
            first_clear_match.roles[1].action.wait_override == 0x8004U &&
            first_clear_match.roles[1].action.wait_remaining == 0U &&
            first_clear_match.roles[2].action.wait_override == 0U &&
            first_clear_match.roles[2].action.wait_remaining == 7U,
        "opcode 44 lookup skips bit-28 roles and uses the first clear GUID match"
    );

    Fixture update_failure;
    update_failure.roles[1].action.wait_remaining = 9U;
    update_failure.ports.action_update_result = 0U;
    prime_loaded_instruction(
        update_failure, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(update_failure.state.window, 2U, 0x00F8U);
    write_u16(update_failure.state.window, 4U, 0x8005U);
    write_u16(update_failure.state.window, 6U, OP_1025);
    const auto update_failure_result = update_failure.step();
    test.expect_true(
        update_failure_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            update_failure_result.action_update_count == 1U &&
            update_failure_result.action_update_failure_count == 1U &&
            update_failure.roles[1].action.wait_override == 0x8005U &&
            update_failure.roles[1].action.wait_remaining == 0U &&
            update_failure.context.instruction_offset == 6U &&
            update_failure.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        "opcode 44 refresh failure is diagnostic-only after both word writes"
    );

    Fixture missing;
    missing.roles[0].action.wait_override = 0x1111U;
    missing.roles[1].action.wait_override = 0x2222U;
    missing.roles[2].action.wait_override = 0x3333U;
    prime_loaded_instruction(missing, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    write_u16(missing.state.window, 2U, 0x7777U);
    write_u16(missing.state.window, 4U, 0x8006U);
    missing.state.previous_opcode = 0x55U;
    const auto missing_result = missing.step();
    test.expect_true(
        missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.opcode == OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            missing_result.executed_instruction_count == 1U &&
            missing_result.action_update_count == 0U &&
            missing.roles[0].action.wait_override == 0x1111U &&
            missing.roles[1].action.wait_override == 0x2222U &&
            missing.roles[2].action.wait_override == 0x3333U &&
            missing.context.instruction_offset == 0U &&
            missing.state.previous_opcode == 0x55U,
        "opcode 44 missing role stops at the first unsafe action access"
    );

    Fixture selector_truncated;
    selector_truncated.context.instruction_offset = 0x7FFEU;
    selector_truncated.context.talk_data_offset = 0x1111U;
    selector_truncated.state.loaded_file_number = 1U;
    selector_truncated.state.loaded_data_offset = 0x1111U;
    selector_truncated.state.window_loaded = true;
    write_u16(
        selector_truncated.state.window,
        0x7FFEU,
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    selector_truncated.state.previous_opcode = 0x55U;
    const auto selector_truncated_result = selector_truncated.step();
    test.expect_true(
        selector_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            selector_truncated_result.executed_instruction_count == 1U &&
            selector_truncated_result.action_update_count == 0U &&
            selector_truncated.context.instruction_offset == 0x7FFEU &&
            selector_truncated.state.previous_opcode == 0x55U,
        "opcode 44 stops at the first unsafe selector-word access"
    );

    Fixture missing_truncated;
    missing_truncated.context.instruction_offset = 0x7FFCU;
    missing_truncated.context.talk_data_offset = 0x1111U;
    missing_truncated.state.loaded_file_number = 1U;
    missing_truncated.state.loaded_data_offset = 0x1111U;
    missing_truncated.state.window_loaded = true;
    write_u16(
        missing_truncated.state.window,
        0x7FFCU,
        OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(missing_truncated.state.window, 0x7FFEU, 0x7777U);
    missing_truncated.state.previous_opcode = 0x55U;
    const auto missing_truncated_result = missing_truncated.step();
    test.expect_true(
        missing_truncated_result.status ==
                LegacyWorldStoryVmStatus::operand_out_of_range &&
            missing_truncated_result.executed_instruction_count == 1U &&
            missing_truncated_result.action_update_count == 0U &&
            missing_truncated.context.instruction_offset == 0x7FFCU &&
            missing_truncated.state.previous_opcode == 0x55U,
        "opcode 44 reads the value after lookup and before unsafe role access"
    );

    Fixture invalid_controlled;
    prime_loaded_instruction(
        invalid_controlled, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(invalid_controlled.state.window, 2U, 0xFFFEU);
    write_u16(invalid_controlled.state.window, 4U, 0x8007U);
    invalid_controlled.state.previous_opcode = 0x55U;
    const auto invalid_controlled_result = invalid_controlled.step(
        0, 0, static_cast<u32>(invalid_controlled.roles.size())
    );
    test.expect_true(
        invalid_controlled_result.status ==
                LegacyWorldStoryVmStatus::role_not_found &&
            invalid_controlled_result.opcode == 0U &&
            invalid_controlled_result.executed_instruction_count == 0U &&
            invalid_controlled.context.instruction_offset == 0U &&
            invalid_controlled.state.previous_opcode == 0x55U,
        "opcode 44 invalid controlled owner stops at the VM session boundary"
    );

    Fixture exact_tail;
    exact_tail.context.instruction_offset = 0x7FFAU;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.roles[1].action.wait_remaining = 9U;
    write_u16(
        exact_tail.state.window, 0x7FFAU, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE
    );
    write_u16(exact_tail.state.window, 0x7FFCU, 0x00F8U);
    write_u16(exact_tail.state.window, 0x7FFEU, 0x8008U);
    const auto exact_tail_result = exact_tail.step();
    test.expect_true(
        exact_tail_result.status ==
                LegacyWorldStoryVmStatus::instruction_out_of_range &&
            exact_tail_result.executed_instruction_count == 1U &&
            exact_tail_result.action_update_count == 1U &&
            exact_tail.roles[1].action.wait_override == 0x8008U &&
            exact_tail.roles[1].action.wait_remaining == 0U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE,
        "opcode 44 exact-tail record completes before the next fetch fails"
    );
}

void test_shared_picture_action_enqueue_protocol(
    openswd3::test::Context& test
) {
    constexpr std::array<u16, 4U> alias_masks{
        0U,
        0x4000U,
        0x8000U,
        0xC000U,
    };
    struct Variant {
        u16 opcode;
        bool primary;
    };
    constexpr std::array<Variant, 2U> variants{
        Variant{OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION, true},
        Variant{OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION, false},
    };
    const auto prime_instruction = [](Fixture& fixture, const u16 raw_word) {
        prime_loaded_instruction(fixture, raw_word);
        write_u16(fixture.state.window, 2U, 0xFEDCU);
        write_u16(fixture.state.window, 4U, 0x8001U);
        write_u16(fixture.state.window, 6U, 0xFFFFU);
        write_u16(fixture.state.window, 8U, 0x8000U);
        fixture.state.previous_opcode = 0x66U;
    };

    for (const Variant variant : variants) {
        for (const u16 mask : alias_masks) {
            Fixture fixture;
            openswd3::world_map::LegacyPictureActionLists picture_actions;
            picture_actions.primary.emplace_back();
            picture_actions.primary.back().screen_x = 0x1111U;
            picture_actions.secondary.emplace_back();
            picture_actions.secondary.back().screen_x = 0x2222U;
            fixture.runtime.picture_actions = &picture_actions;
            prime_instruction(fixture, static_cast<u16>(variant.opcode | mask));

            const auto result = fixture.step();
            const auto& destination = variant.primary
                ? picture_actions.primary
                : picture_actions.secondary;
            const auto& other = variant.primary ? picture_actions.secondary
                                                : picture_actions.primary;
            const auto& node = destination.front();
            const u16 prior_x = variant.primary ? 0x1111U : 0x2222U;
            const u16 other_x = variant.primary ? 0x2222U : 0x1111U;

            test.expect_true(
                result.status == LegacyWorldStoryVmStatus::yielded &&
                    result.opcode == variant.opcode &&
                    result.executed_instruction_count == 1U &&
                    destination.size() == 2U && other.size() == 1U &&
                    std::next(destination.begin())->screen_x == prior_x &&
                    other.front().screen_x == other_x &&
                    node.screen_x == 0xFEDCU && node.screen_y == 0x8001U &&
                    node.field_04 == 0U && node.field_06 == 0U &&
                    node.action.action_id == 0xFFFFU &&
                    node.action.base_variant == 0x8000U &&
                    node.action.field_1c == 0xFFFFFFFFU &&
                    node.action.one_shot_base_variant == 0xFFFFFFFFU &&
                    node.action.one_shot_variant_delta == 0xFFFFFFFFU &&
                    node.action.wait_override == 0U &&
                    node.action.wait_default == 0U &&
                    node.action.wait_remaining == 0U &&
                    node.action.command_cursor == 0U &&
                    node.action.external_mode == 0U &&
                    node.next_pointer_32 == 0U &&
                    fixture.context.instruction_offset == 10U &&
                    fixture.state.previous_opcode == variant.opcode &&
                    fixture.ports.direct_audio_service_count == 0U,
                "opcodes 58 and 153 aliases initialize and prepend the selected picture-action list"
            );
        }
    }

    struct BoundaryCase {
        u16 instruction_offset;
        u32 available_operands;
    };
    constexpr std::array<BoundaryCase, 4U> boundaries{
        BoundaryCase{0x7FFEU, 0U},
        BoundaryCase{0x7FFCU, 1U},
        BoundaryCase{0x7FFAU, 2U},
        BoundaryCase{0x7FF8U, 3U},
    };
    for (const BoundaryCase boundary : boundaries) {
        Fixture fixture;
        openswd3::world_map::LegacyPictureActionLists picture_actions;
        picture_actions.primary.emplace_back();
        picture_actions.secondary.emplace_back();
        fixture.runtime.picture_actions = &picture_actions;
        fixture.context.instruction_offset = boundary.instruction_offset;
        fixture.context.talk_data_offset = 0x1111U;
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset = 0x1111U;
        fixture.state.window_loaded = true;
        fixture.state.previous_opcode = 0x66U;
        write_u16(
            fixture.state.window,
            boundary.instruction_offset,
            OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION
        );
        for (u32 operand = 0U; operand < boundary.available_operands;
             ++operand) {
            write_u16(
                fixture.state.window,
                static_cast<std::size_t>(boundary.instruction_offset) + 2U +
                    2U * operand,
                static_cast<u16>(0x1000U + operand)
            );
        }

        const auto result = fixture.step();

        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::operand_out_of_range &&
                result.opcode == OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION &&
                result.executed_instruction_count == 1U &&
                picture_actions.primary.size() == 1U &&
                picture_actions.secondary.size() == 1U &&
                fixture.context.instruction_offset ==
                    boundary.instruction_offset &&
                fixture.state.previous_opcode == 0x66U,
            "shared picture-action handler keeps an incomplete staged node unlinked"
        );
    }

    Fixture missing_owner;
    prime_instruction(missing_owner, OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION);
    const auto owner_result = missing_owner.step();
    test.expect_true(
        owner_result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            owner_result.opcode == OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            owner_result.executed_instruction_count == 1U &&
            missing_owner.context.instruction_offset == 0U &&
            missing_owner.state.previous_opcode == 0x66U,
        "shared picture-action handler reaches the list owner only after all operands"
    );

    Fixture exact_tail;
    openswd3::world_map::LegacyPictureActionLists tail_actions;
    exact_tail.runtime.picture_actions = &tail_actions;
    exact_tail.context.instruction_offset = 0x7FF6U;
    exact_tail.context.talk_data_offset = 0x1111U;
    exact_tail.state.loaded_file_number = 1U;
    exact_tail.state.loaded_data_offset = 0x1111U;
    exact_tail.state.window_loaded = true;
    exact_tail.state.previous_opcode = 0x66U;
    write_u16(
        exact_tail.state.window,
        0x7FF6U,
        OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION
    );
    write_u16(exact_tail.state.window, 0x7FF8U, 1U);
    write_u16(exact_tail.state.window, 0x7FFAU, 2U);
    write_u16(exact_tail.state.window, 0x7FFCU, 3U);
    write_u16(exact_tail.state.window, 0x7FFEU, 4U);
    const auto tail_result = exact_tail.step();
    test.expect_true(
        tail_result.status == LegacyWorldStoryVmStatus::yielded &&
            tail_result.opcode == OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            tail_result.executed_instruction_count == 1U &&
            tail_actions.primary.empty() &&
            tail_actions.secondary.size() == 1U &&
            tail_actions.secondary.front().screen_x == 1U &&
            tail_actions.secondary.front().screen_y == 2U &&
            tail_actions.secondary.front().action.action_id == 3U &&
            tail_actions.secondary.front().action.base_variant == 4U &&
            exact_tail.context.instruction_offset == 0x8000U &&
            exact_tail.state.previous_opcode ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION,
        "shared picture-action handler exact tail links the node and yields at the window end"
    );
}

void test_request_battle_after_clearing_overlay_lists(
    openswd3::test::Context& test
) {
    Fixture fixture;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_rows(2U);
    openswd3::world_map::LegacyRoleHeadActionList role_heads(3U);
    u32 battle_request{};
    fixture.runtime.packed_row_effects = &packed_rows;
    fixture.runtime.role_head_actions = &role_heads;
    fixture.runtime.battle_request_value = &battle_request;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 88U);
    write_u16(script, 2U, 0xFFFEU);

    const auto result = fixture.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 88U && result.executed_instruction_count == 1U &&
            fixture.context.instruction_offset == 4U,
        "opcode 88 consumes four bytes and yields immediately"
    );
    test.expect_true(
        packed_rows.empty() && role_heads.empty() &&
            battle_request == 0xFFFFFFFEU,
        "opcode 88 clears only its two overlay owners and tags a sign-extended battle id"
    );
}

void test_play_sound_effect_request(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 59U);
    write_u16(script, 2U, 0x1234U);
    write_u16(script, 4U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 6U, 0x00F8U);

    const auto requested = fixture.step();
    const auto continued = fixture.step();

    test.expect_true(
        requested.status == LegacyWorldStoryVmStatus::yielded &&
            requested.opcode == 59U &&
            requested.executed_instruction_count == 1U &&
            fixture.ports.sound_effect_requests.size() == 1U &&
            fixture.ports.sound_effect_requests.front() == 0x1234U &&
            continued.status == LegacyWorldStoryVmStatus::yielded &&
            continued.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            fixture.context.instruction_offset == 8U,
        "opcode 59 submits the u16 sound id, advances four bytes and yields"
    );
}

void test_wait_for_frame_color_transition(openswd3::test::Context& test) {
    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{
        .countdown = 1,
    };
    fixture.runtime.frame_color = &frame_color;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 53U);
    write_u16(script, 2U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 4U, 0x00F8U);

    const auto waiting = fixture.step();
    frame_color.countdown = 0;
    const auto completed = fixture.step();

    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == 53U && waiting.executed_instruction_count == 1U &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            completed.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U,
        "opcode 53 waits in place only while the signed color countdown is positive"
    );

    Fixture cancel_fixture;
    openswd3::rendering::LegacyFrameColorTransitionState cancel_color{
        .countdown = 7,
        .current_red = 1.0F,
        .current_green = 2.0F,
        .current_blue = 3.0F,
        .target_red = 4.0F,
        .target_green = 5.0F,
        .target_blue = 6.0F,
        .step_red = 7.0F,
        .step_green = 8.0F,
        .step_blue = 9.0F,
    };
    cancel_fixture.runtime.frame_color = &cancel_color;
    auto cancel_script = std::span<u8>{cancel_fixture.ports.initial_window};
    write_u16(cancel_script, 0U, 74U);
    write_u16(cancel_script, 2U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(cancel_script, 4U, 0x00F8U);

    const auto cancelled = cancel_fixture.step();

    test.expect_true(
        cancelled.status == LegacyWorldStoryVmStatus::yielded &&
            cancelled.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            cancelled.executed_instruction_count == 2U &&
            cancel_fixture.context.instruction_offset == 6U &&
            cancel_color.countdown == 0 && cancel_color.step_red == 0.0F &&
            cancel_color.step_green == 0.0F && cancel_color.step_blue == 0.0F &&
            cancel_color.current_red == 1.0F &&
            cancel_color.current_green == 2.0F &&
            cancel_color.current_blue == 3.0F &&
            cancel_color.target_red == 4.0F &&
            cancel_color.target_green == 5.0F &&
            cancel_color.target_blue == 6.0F,
        "opcode 74 clears only the three color steps and countdown then continues"
    );
}

void test_turn_role_toward_role(openswd3::test::Context& test) {
    Fixture fixture;
    fixture.roles[1].action.base_variant = 7U;
    fixture.roles[1].action.variant_delta = 6U;
    fixture.roles[1].action.wait_remaining = 9U;
    fixture.roles[1].action.field_2c = 0U;
    fixture.roles[1].action.field_30 = 0U;
    fixture.roles[1].world_x = 100U;
    fixture.roles[1].world_y = 100U;
    fixture.roles[2].guid = 0x00F9U;
    fixture.roles[2].flags = 0U;
    fixture.roles[2].world_x = 200U;
    fixture.roles[2].world_y = 100U;

    openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
    std::vector<u8> surface_grid(16U * 16U * sizeof(u32), 0U);
    openswd3::world_map::LegacyWorldPathNodePool node_pool;
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    u8 scene_render_flags{};
    std::array<u8, openswd3::world_map::kLegacyWorldGuidOneArrivalByteCount>
        selected_arrival_bytes{};
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = fixture.roles,
        .active_object_slots = fixture.active_object_slots,
        .spatial_index = &spatial_index,
        .role_surface =
            {
                .map_width = 16U,
                .selected_guid = 0U,
                .surface_grid = surface_grid,
            },
        .node_pool = &node_pool,
        .movement = &movement,
        .camera = &fixture.camera,
        .selected_arrival_bytes = selected_arrival_bytes,
        .selected_role_index = 0U,
        .map_height = 16U,
        .scene_render_flags = &scene_render_flags,
    };
    fixture.runtime.story_paths = &story_paths;

    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 76U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 0x00F9U);
    write_u16(script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 8U, 0x00F8U);
    const auto result = fixture.step();

    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 2U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.variant_delta == 3U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            (fixture.roles[1].flags & 0x80000000U) != 0U &&
            fixture.context.instruction_offset == 10U,
        "opcode 76 turns the first role toward the second and suspends it"
    );
}

void test_set_role_head_sign_action(openswd3::test::Context& test) {
    Fixture fixture;
    auto script = std::span<u8>{fixture.ports.initial_window};
    write_u16(script, 0U, 71U);
    write_u16(script, 2U, 0x00F8U);
    write_u16(script, 4U, 3U);
    write_u16(script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(script, 8U, 0x00F8U);
    const auto assigned = fixture.step();

    Fixture missing;
    missing.roles[1].field_3c = 0x12345678U;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, 71U);
    write_u16(missing_script, 2U, 0xFFF0U);
    write_u16(missing_script, 4U, 0U);
    write_u16(missing_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(missing_script, 8U, 0x00F8U);
    const auto ignored = missing.step();

    Fixture cleared;
    cleared.roles[1].field_3c =
        openswd3::world_map::legacy_world_head_sign_action_token(2U);
    auto clear_script = std::span<u8>{cleared.ports.initial_window};
    write_u16(clear_script, 0U, 72U);
    write_u16(clear_script, 2U, 0x00F8U);
    write_u16(clear_script, 4U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(clear_script, 6U, 0x00F8U);
    const auto removed = cleared.step();

    test.expect_true(
        assigned.status == LegacyWorldStoryVmStatus::yielded &&
            assigned.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            assigned.executed_instruction_count == 2U &&
            fixture.roles[1].field_3c ==
                openswd3::world_map::legacy_world_head_sign_action_token(3U) &&
            ignored.status == LegacyWorldStoryVmStatus::yielded &&
            ignored.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            ignored.executed_instruction_count == 2U &&
            missing.roles[1].field_3c == 0x12345678U &&
            removed.status == LegacyWorldStoryVmStatus::yielded &&
            removed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            removed.executed_instruction_count == 2U &&
            cleared.roles[1].field_3c == 0U,
        "opcodes 71 and 72 assign and clear the head sign while unresolved " "selectors are consumed without substituting FFF0"
    );
}

void test_set_and_clear_role_wait_override(openswd3::test::Context& test) {
    Fixture assigned;
    assigned.roles[1].action.wait_remaining = 9U;
    auto assigned_script = std::span<u8>{assigned.ports.initial_window};
    write_u16(assigned_script, 0U, 77U);
    write_u16(assigned_script, 2U, 0xFFF0U);
    write_u16(assigned_script, 4U, 3U);
    write_u16(assigned_script, 6U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(assigned_script, 8U, 0x00F8U);
    const auto assigned_result = assigned.step();

    Fixture cleared;
    cleared.roles[1].action.wait_override = 0x8123U;
    cleared.roles[1].action.wait_remaining = 9U;
    auto cleared_script = std::span<u8>{cleared.ports.initial_window};
    write_u16(cleared_script, 0U, 78U);
    write_u16(cleared_script, 2U, 0x00F8U);
    write_u16(cleared_script, 4U, OP_14_WAIT_ROLE_ACTION_STATUS);
    write_u16(cleared_script, 6U, 0x00F8U);
    const auto cleared_result = cleared.step();

    Fixture missing;
    auto missing_script = std::span<u8>{missing.ports.initial_window};
    write_u16(missing_script, 0U, 77U);
    write_u16(missing_script, 2U, 0x7777U);
    write_u16(missing_script, 4U, 5U);
    const auto missing_result = missing.step();

    test.expect_true(
        assigned_result.status == LegacyWorldStoryVmStatus::yielded &&
            assigned_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            assigned_result.executed_instruction_count == 2U &&
            assigned_result.action_update_count == 2U &&
            assigned.roles[1].action.wait_override == 0x8003U &&
            assigned.roles[1].action.wait_remaining == 0U &&
            assigned.context.instruction_offset == 10U &&
            cleared_result.status == LegacyWorldStoryVmStatus::yielded &&
            cleared_result.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            cleared_result.executed_instruction_count == 2U &&
            cleared_result.action_update_count == 2U &&
            cleared.roles[1].action.wait_override == 0U &&
            cleared.roles[1].action.wait_remaining == 0U &&
            cleared.context.instruction_offset == 8U &&
            missing_result.status == LegacyWorldStoryVmStatus::role_not_found &&
            missing_result.instruction_offset == 0U &&
            missing_result.first_operand_available &&
            missing_result.first_operand_word == 0x7777U &&
            missing.context.instruction_offset == 0U,
        "opcodes 77 and 78 refresh the role wait override while an unresolved " "selector preserves the undefined-width instruction boundary"
    );
}

void test_real_clear_dialog_control_flag_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004518);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 7U,
        "real opcode 7 physical record is a two-byte instruction"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 7U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, OP_1025);
    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.text_control_flags == 0x7FFFFFFFU &&
            fixture.state.previous_opcode == 7U,
        "real opcode 7 record replays the clear-and-continue contract"
    );
}

void test_real_change_role_base_variant_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A24);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 10U &&
            read_u16(instruction, 2U) == 1U && read_u16(instruction, 4U) == 0U,
        "real opcode 10 physical record has the expected six-byte payload"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 10U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    fixture.roles[1].guid = 1U;
    fixture.roles[1].action.base_variant = 77U;
    fixture.roles[1].action.wait_remaining = 9U;
    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U &&
            fixture.roles[1].action.base_variant == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            result.action_update_count == 1U &&
            fixture.state.previous_opcode == 10U,
        "real opcode 10 record replays the live-role update contract"
    );
}

void test_real_change_role_variant_delta_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004A2E);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 11U &&
            read_u16(instruction, 2U) == 1U && read_u16(instruction, 4U) == 0U,
        "real opcode 11 physical record has the expected six-byte payload"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 11U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    fixture.roles[1].guid = 1U;
    fixture.roles[1].flags = 1U;
    fixture.roles[1].action.variant_delta = 77U;
    fixture.roles[1].action.wait_remaining = 9U;
    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 6U &&
            fixture.roles[1].action.variant_delta == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            fixture.roles[1].flags == 0x00001001U &&
            result.action_update_count == 1U &&
            fixture.state.previous_opcode == 11U,
        "real opcode 11 record replays the live-role update contract"
    );
}

void test_real_clear_dialog_control_flag_bit30_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000451E);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 9U,
        "real opcode 9 physical record is a two-byte instruction"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 9U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, OP_1025);
    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.text_control_flags == 0xBFFFFFFFU &&
            fixture.state.previous_opcode == 9U,
        "real opcode 9 record replays the bit-30 clear contract"
    );
}

void test_real_stage_dialog_lifetime_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000451A);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) && read_u16(instruction, 0U) == 8U &&
            read_u16(instruction, 2U) == 0xFFFFU,
        "real opcode 8 physical record carries an unsigned FFFF word"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, 8U);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    const auto result = fixture.step();
    test.expect_true(
        result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.next_text_aux_pending &&
            fixture.state.next_text_aux_value == 0xFFFFU &&
            fixture.state.previous_opcode == 8U,
        "real opcode 8 record replays the staged-lifetime contract"
    );
}

void test_real_wait_role_action_status_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000471F);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    test.expect_true(
        static_cast<bool>(input) &&
            read_u16(instruction, 0U) == OP_14_WAIT_ROLE_ACTION_STATUS &&
            read_u16(instruction, 2U) == 0x00F8U,
        "real opcode 14 record is a four-byte wait for role F8"
    );
    if (!input) {
        return;
    }

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_14_WAIT_ROLE_ACTION_STATUS);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].interaction_gate = 1U;
    const auto waiting = fixture.step();
    const u16 waiting_offset = fixture.context.instruction_offset;
    fixture.roles[1].interaction_gate = 0U;
    const auto completed = fixture.step();
    test.expect_true(
        waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            waiting.direct_audio_service_count == 1U && waiting_offset == 0U &&
            completed.status == LegacyWorldStoryVmStatus::yielded &&
            completed.opcode == OP_14_WAIT_ROLE_ACTION_STATUS &&
            completed.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_14_WAIT_ROLE_ACTION_STATUS,
        "real opcode 14 record replays wait, audio service, and completion"
    );
}

void test_real_jump_same_file_offset_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00008A85);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 1U;
    state.loaded_data_offset = 0x00008885U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00F8U;
    context.talk_script_id = 248U;
    context.talk_data_offset = 0x00008885U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_15_JUMP_SAME_FILE_OFFSET &&
            read_u32(instruction, 2U) == 0x000088CFU &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 59U && result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 1U &&
            context.talk_data_offset == 0x000088CFU &&
            context.instruction_offset == 4U &&
            state.loaded_file_number == 1U &&
            state.loaded_data_offset == 0x000088CFU &&
            state.previous_opcode == OP_15_JUMP_SAME_FILE_OFFSET &&
            ports.sound_effect_requests == std::vector<u16>{0x003AU},
        "real opcode 15 jumps within TALK1 and same-call executes opcode 59"
    );
}

void test_real_jump_if_role_path_unprepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000F963);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 2U;
    state.loaded_data_offset = 0x0000F763U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00E4U;
    context.talk_script_id = 2200U;
    context.talk_data_offset = 0x0000F763U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x00E4U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    write_u16(active_object_slots[0].bytes, 0U, 1U);
    active_object_slots[0].bytes[0x1BU] = 2U;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {.current_tick = 0U},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            read_u16(instruction, 2U) == 0x00E4U &&
            read_u32(instruction, 4U) == 0x0000F787U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 67U && result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 1U &&
            context.talk_data_offset == 0x0000F787U &&
            context.instruction_offset == 0U &&
            state.loaded_file_number == 2U &&
            state.loaded_data_offset == 0x0000F787U &&
            state.previous_opcode == OP_16_JUMP_IF_ROLE_PATH_UNPREPARED &&
            state.wait_duration == 0x012CU &&
            read_u16(state.window, 2U) == 0x812CU,
        "real opcode 16 jumps within TALK2 and same-call starts opcode 67 wait"
    );
}

void test_real_jump_if_role_path_prepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000074A6);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    std::ranges::copy(instruction, state.window.begin());
    state.loaded_file_number = 2U;
    state.loaded_data_offset = 0x000072A6U;
    state.window_loaded = true;
    LegacyWorldTalkContext context{};
    context.source_guid = 0x00DAU;
    context.talk_script_id = 2200U;
    context.talk_data_offset = 0x000072A6U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    roles[1].guid = 0x00DAU;
    roles[1].flags = 0x40000000U;
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    write_u16(active_object_slots[0].bytes, 0U, 1U);
    active_object_slots[0].bytes[0x1BU] = 2U;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        {},
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        {},
        ports
    );
    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_17_JUMP_IF_ROLE_PATH_PREPARED &&
            read_u32(instruction, 4U) == 0x00007296U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == 109U && result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 1U &&
            context.talk_data_offset == 0x00007296U &&
            state.previous_opcode == OP_17_JUMP_IF_ROLE_PATH_PREPARED,
        "real opcode 17 jumps within TALK2 and same-call fetches opcode 109"
    );
}

void test_real_release_role_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00054136);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_18_RELEASE_ROLE_PATH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].guid = 0x000BU;
    fixture.roles[1].flags = 0x20000000U;
    fixture.roles[1].action.wait_remaining = 9U;
    const auto result = fixture.step();
    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_18_RELEASE_ROLE_PATH &&
            read_u16(instruction, 2U) == 0x000BU &&
            read_u16(instruction, 4U) == 111U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == 111U && result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_18_RELEASE_ROLE_PATH &&
            fixture.roles[1].flags == 0x20000000U &&
            fixture.roles[1].action.wait_remaining == 0U,
        "real opcode 18 releases role 11 then same-call fetches opcode 111"
    );
}

void test_real_release_all_role_paths_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00010C93);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_19_RELEASE_ROLE_PATHS);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].flags = 0x20000000U;
    fixture.roles[1].action.wait_remaining = 7U;
    const auto result = fixture.step();
    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_19_RELEASE_ROLE_PATHS &&
            read_u16(instruction, 2U) == OP_28_CHANGE_ROLE_PATH_ID &&
            read_u16(instruction, 4U) == 102U &&
            read_u16(instruction, 6U) == 102U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            fixture.roles[1].flags == 0x20000000U &&
            fixture.roles[1].action.wait_remaining == 7U &&
            fixture.ports.role_patch_requests.size() == 1U &&
            fixture.ports.role_patch_requests.front().guid == 102U &&
            fixture.ports.role_patch_requests.front().path_data_id == 102U &&
            fixture.ports.role_patch_requests.front().flags_or_mask ==
                0x1000U &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 19 skips released roles then opcode 28 patches MAPS and yields"
    );
}

void test_real_schedule_role_path_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        std::size_t instruction_size;
        u16 opcode;
        u16 expected_action_id;
        u16 expected_base_variant;
        u16 expected_variant_delta;
    };
    constexpr std::array<Sample, 2U> samples{
        Sample{
            0x000049F6,
            10U,
            OP_20_SCHEDULE_ROLE_PATHS,
            0xFFFFU,
            0xFFFFU,
            0xFFFFU,
        },
        Sample{
            0x0002A56D,
            16U,
            OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS,
            0xFFFFU,
            0U,
            7U,
        },
    };

    for (const auto sample : samples) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(sample.file_offset);
        std::vector<u8> instruction(sample.instruction_size + 2U);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        fixture.context.source_guid = 1U;
        fixture.roles[1].guid = 1U;
        StoryPathHarness paths{fixture};
        prime_loaded_instruction(fixture, sample.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        write_u16(fixture.state.window, sample.instruction_size, OP_1025);
        const auto staged = fixture.step();
        const u16 staged_count = read_u16(fixture.state.window, 2U);
        const auto& slot = fixture.active_object_slots[0].bytes;
        fixture.roles[1].flags |= 0x02000000U;
        const auto completed = fixture.step();

        test.expect_true(
            instruction_read && read_u16(instruction, 0U) == sample.opcode &&
                (read_u16(instruction, 2U) & 0x4000U) == 0U &&
                staged.status == LegacyWorldStoryVmStatus::yielded &&
                staged.opcode == sample.opcode && staged_count == 0x4001U &&
                read_u16(slot, 0x10U) == sample.expected_action_id &&
                read_u16(slot, 0x12U) == sample.expected_base_variant &&
                read_u16(slot, 0x14U) == sample.expected_variant_delta &&
                completed.status ==
                    LegacyWorldStoryVmStatus::unsupported_opcode &&
                completed.opcode == OP_1025 &&
                completed.executed_instruction_count == 2U &&
                fixture.context.instruction_offset == sample.instruction_size &&
                fixture.state.previous_opcode == sample.opcode &&
                read_u16(fixture.state.window, 2U) == 1U,
            "real opcode 20/169 records stage paths then complete in-call"
        );
    }
}

void test_real_jump_if_global_bit_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        std::streamoff file_offset;
        u16 opcode;
        u16 bit_index;
        u32 target;
        bool set_bit;
    };
    constexpr std::array<Sample, 2U> samples{
        Sample{
            0x000014CE,
            OP_21_JUMP_IF_GLOBAL_BIT_SET,
            0x01BDU,
            0x000012F0U,
            true,
        },
        Sample{
            0x00002560,
            OP_22_JUMP_IF_GLOBAL_BIT_CLEAR,
            0x0064U,
            0x00002386U,
            false,
        },
    };

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    for (const auto sample : samples) {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(sample.file_offset);
        std::array<u8, 8U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.loaded_file_number = 1U;
        fixture.state.loaded_data_offset =
            static_cast<u32>(sample.file_offset) - 0x200U;
        fixture.state.window_loaded = true;
        fixture.context.talk_data_offset = fixture.state.loaded_data_offset;
        if (sample.set_bit) {
            openswd3::world_map::set_legacy_world_story_flag(
                fixture.state, sample.bit_index
            );
        }
        RealPorts ports{databases};
        const auto result = openswd3::world_map::step_legacy_world_story_vm(
            fixture.context,
            fixture.state,
            fixture.roles,
            0U,
            fixture.active_object_slots,
            fixture.maps_payload,
            fixture.dialogs,
            fixture.dialog_resources,
            fixture.first_name,
            fixture.second_name,
            fixture.runtime,
            ports
        );

        const bool opcode21_tail =
            result.status == LegacyWorldStoryVmStatus::runtime_unavailable &&
            result.opcode == 76U && result.executed_instruction_count == 3U &&
            fixture.context.talk_data_offset == sample.target &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.loaded_file_number == 1U &&
            fixture.state.loaded_data_offset == sample.target;
        const bool opcode22_tail =
            result.status == LegacyWorldStoryVmStatus::terminated &&
            result.opcode == 0x3FFFU &&
            result.executed_instruction_count == 3U &&
            fixture.context.talk_data_offset == 0xFFFFFFFFU &&
            fixture.context.instruction_offset == 0xFFFFU;
        test.expect_true(
            instruction_read &&
                initialized.status ==
                    openswd3::resource_io::LegacyResourceDatabaseStatus::
                        ready &&
                read_u16(instruction, 0U) == sample.opcode &&
                read_u16(instruction, 2U) == sample.bit_index &&
                read_u32(instruction, 4U) == sample.target &&
                result.load_status == LegacyTalkWindowStatus::ready &&
                result.direct_audio_service_count == 1U &&
                fixture.state.previous_opcode == sample.opcode,
            "real opcode 21/22 records execute the audited first branch"
        );
        if (sample.opcode == OP_21_JUMP_IF_GLOBAL_BIT_SET) {
            test.expect_true(
                opcode21_tail,
                "real opcode 21 target follows the expected prefix chain"
            );
        } else {
            test.expect_true(
                opcode22_tail,
                "real opcode 22 target follows the expected prefix chain"
            );
        }
    }
}

void test_real_jump_if_all_global_bits_set_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00008AD3);
    std::array<u8, 12U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);
    input.close();

    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    Fixture fixture;
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_file_number = 1U;
    fixture.state.loaded_data_offset = 0x000088D3U;
    fixture.state.window_loaded = true;
    fixture.context.talk_data_offset = 0x000088D3U;
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 295U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 264U);
    RealPorts ports{databases};
    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        fixture.context,
        fixture.state,
        fixture.roles,
        0U,
        fixture.active_object_slots,
        fixture.maps_payload,
        fixture.dialogs,
        fixture.dialog_resources,
        fixture.first_name,
        fixture.second_name,
        fixture.runtime,
        ports
    );

    test.expect_true(
        instruction_read &&
            initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            read_u16(instruction, 0U) == OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            read_u16(instruction, 2U) == 295U &&
            read_u16(instruction, 4U) == 264U &&
            read_u16(instruction, 6U) == 0xFF00U &&
            read_u32(instruction, 8U) == 0x000088E1U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 59U && result.executed_instruction_count == 2U &&
            result.load_status == LegacyTalkWindowStatus::ready &&
            result.direct_audio_service_count == 1U &&
            fixture.context.talk_data_offset == 0x000088E1U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.loaded_file_number == 1U &&
            fixture.state.loaded_data_offset == 0x000088E1U &&
            fixture.state.previous_opcode ==
                OP_23_JUMP_IF_ALL_GLOBAL_BITS_SET &&
            ports.sound_effect_requests == std::vector<u16>{0x0039U},
        "real opcode 23 branches then same-call opcode 59 plays sound and yields"
    );
}

void test_real_set_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000074C1);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_25_SET_GLOBAL_BIT);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_data_offset = 0x000072C1U;
    fixture.context.talk_data_offset = 0x000072C1U;
    const auto result = fixture.step();

    test.expect_true(
        instruction_read && read_u16(instruction, 0U) == OP_25_SET_GLOBAL_BIT &&
            read_u16(instruction, 2U) == 7080U &&
            read_u16(instruction, 4U) == 59U &&
            read_u16(instruction, 6U) == 0x00ABU &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 59U && result.executed_instruction_count == 2U &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 7080U
            ) &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_25_SET_GLOBAL_BIT &&
            fixture.ports.sound_effect_requests == std::vector<u16>{0x00ABU},
        "real opcode 25 sets bit 7080 then same-call opcode 59 plays sound"
    );
}

void test_real_clear_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000265FE);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_26_CLEAR_GLOBAL_BIT);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.state.loaded_data_offset = 0x000263FEU;
    fixture.context.talk_data_offset = 0x000263FEU;
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 614U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 615U);
    openswd3::world_map::set_legacy_world_story_flag(fixture.state, 616U);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_26_CLEAR_GLOBAL_BIT &&
            read_u16(instruction, 2U) == 615U &&
            read_u16(instruction, 4U) == 59U &&
            read_u16(instruction, 6U) == 0x0038U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 59U && result.executed_instruction_count == 2U &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 614U
            ) &&
            !openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 615U
            ) &&
            openswd3::world_map::query_legacy_world_story_flag(
                fixture.state, 616U
            ) &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode == OP_26_CLEAR_GLOBAL_BIT &&
            fixture.ports.sound_effect_requests == std::vector<u16>{0x0038U},
        "real opcode 26 clears bit 615 then same-call opcode 59 plays sound"
    );
}

void test_real_reload_world_session_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK3.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00016095);
    std::array<u8, 14U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_27_RELOAD_WORLD_SESSION);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 14U, OP_1025);
    fixture.runtime.role_surface.selected_guid = 0x00F8U;
    fixture.roles[1].action.action_id = 0x12345U;
    fixture.roles[1].action.base_variant = 0x23456U;
    fixture.roles[1].action.variant_delta = 0x34567U;
    const auto result = fixture.step(0, 0, 1U);
    const auto& request = fixture.ports.last_world_load_request;

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_27_RELOAD_WORLD_SESSION &&
            read_u16(instruction, 2U) == 161U &&
            read_u16(instruction, 4U) == 23U &&
            read_u16(instruction, 6U) == 22U &&
            read_u16(instruction, 8U) == 0xFFFFU &&
            read_u16(instruction, 10U) == 0xFFFFU &&
            read_u16(instruction, 12U) == 0xFFFFU &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 14U &&
            fixture.state.previous_opcode == OP_27_RELOAD_WORLD_SESSION &&
            request.logical_map_id == 161U && request.tile_x == 23U &&
            request.tile_y == 22U && request.action_id == 0x2345U &&
            request.base_variant == 0x3456U &&
            request.variant_delta == 0x4567U &&
            request.selected_guid == 0x00F8U && request.load_flags == 1U,
        "real opcode 27 record inherits all three controlled-role action " "fields before synchronous world reload"
    );
}

void test_real_change_role_path_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK2.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0001938D);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_28_CHANGE_ROLE_PATH_ID);
    std::ranges::copy(instruction, fixture.state.window.begin());
    fixture.roles[1].guid = 2U;
    fixture.roles[1].path_data_id = 0x1111U;
    fixture.roles[1].path_word_index = 7U;
    fixture.roles[1].flags = 0x20U;
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_28_CHANGE_ROLE_PATH_ID &&
            read_u16(instruction, 2U) == 2U &&
            read_u16(instruction, 4U) == 30U &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            result.executed_instruction_count == 1U &&
            result.direct_audio_service_count == 1U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_28_CHANGE_ROLE_PATH_ID &&
            fixture.roles[1].path_data_id == 30U &&
            fixture.roles[1].path_word_index == 0U &&
            fixture.roles[1].flags == 0x1020U &&
            fixture.ports.direct_audio_service_count == 1U,
        "real opcode 28 record replaces live role path id then yields"
    );
}

void test_real_global_integer_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream add_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    add_input.seekg(0x00007FAF);
    std::array<u8, 6U> add_instruction{};
    add_input.read(
        reinterpret_cast<char*>(add_instruction.data()),
        static_cast<std::streamsize>(add_instruction.size())
    );
    const bool add_read = static_cast<bool>(add_input);

    Fixture add_fixture;
    prime_loaded_instruction(add_fixture, OP_30_ADD_GLOBAL_INTEGER);
    std::ranges::copy(add_instruction, add_fixture.state.window.begin());
    write_u16(add_fixture.state.window, 6U, OP_1025);
    const auto add_result = add_fixture.step();
    test.expect_true(
        add_read && read_u16(add_instruction, 0U) == OP_30_ADD_GLOBAL_INTEGER &&
            read_u16(add_instruction, 2U) == 0U &&
            read_u16(add_instruction, 4U) == 50U &&
            add_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            add_result.executed_instruction_count == 2U &&
            add_fixture.context.instruction_offset == 6U &&
            add_fixture.state.previous_opcode == OP_30_ADD_GLOBAL_INTEGER &&
            add_fixture.state.script_variables[0] == 150U,
        "real opcode 30 record adds 50 to initialized variable zero"
    );

    std::ifstream chain_input{
        root / "TALK1.DAT", std::ios::binary | std::ios::in
    };
    chain_input.seekg(0x0000FF97);
    std::array<u8, 28U> chain{};
    chain_input.read(
        reinterpret_cast<char*>(chain.data()),
        static_cast<std::streamsize>(chain.size())
    );
    const bool chain_read = static_cast<bool>(chain_input);

    Fixture chain_fixture;
    prime_loaded_instruction(
        chain_fixture, OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE
    );
    std::ranges::copy(chain, chain_fixture.state.window.begin());
    chain_fixture.state.script_variables[62] = 3U;
    const auto chain_result = chain_fixture.step();
    test.expect_true(
        chain_read &&
            read_u16(chain, 0U) == OP_33_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_LE &&
            read_u16(chain, 2U) == 62U && read_u16(chain, 4U) == 2U &&
            read_u16(chain, 10U) == OP_32_JUMP_IF_GLOBAL_INTEGER_UNSIGNED_GE &&
            read_u16(chain, 12U) == 62U && read_u16(chain, 14U) == 4U &&
            read_u16(chain, 20U) == OP_29_SET_GLOBAL_INTEGER &&
            read_u16(chain, 22U) == 62U && read_u16(chain, 24U) == 4U &&
            read_u16(chain, 26U) == 0xFFFFU &&
            chain_result.status == LegacyWorldStoryVmStatus::terminated &&
            chain_result.executed_instruction_count == 4U &&
            chain_fixture.context.instruction_offset == 0xFFFFU &&
            chain_fixture.state.previous_opcode == OP_29_SET_GLOBAL_INTEGER &&
            chain_fixture.state.script_variables[62] == 4U &&
            chain_fixture.ports.data_load_count == 0U,
        "real opcode 33 to 32 to 29 chain falls through then terminates"
    );
}

void test_real_relocate_role_and_complete_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x0000464E);
    std::array<u8, 8U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 8U, OP_1025);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            read_u16(instruction, 2U) == 1U &&
            read_u16(instruction, 4U) == 0x24U &&
            read_u16(instruction, 6U) == 0x21U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 8U &&
            fixture.state.previous_opcode ==
                OP_40_RELOCATE_ROLE_AND_COMPLETE_PATH &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.tile_x == 0x24U &&
            request.tile_y == 0x21U && request.flags_or_mask == 0U &&
            request.flags_and_mask == 0xFFFFU,
        "real opcode 40 record patches missing role coordinates then continues"
    );
}

void test_real_reload_indexed_target_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000042E6);
    std::array<u8, 26U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.indexed_target_selector = 3U;
    prime_loaded_instruction(fixture, OP_41_RELOAD_INDEXED_TARGET);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.ports.transferred_window, 0U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_41_RELOAD_INDEXED_TARGET &&
            read_u32(instruction, 2U) == 0x00004100U &&
            read_u32(instruction, 6U) == 0x00004118U &&
            read_u32(instruction, 10U) == 0x00004124U &&
            read_u32(instruction, 14U) == 0x0000410CU &&
            read_u32(instruction, 18U) == 0x00004130U &&
            read_u32(instruction, 22U) == 0xFF00FF00U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.direct_audio_service_count == 1U &&
            fixture.ports.last_data_file_number == 1U &&
            fixture.ports.last_data_offset == 0x0000410CU &&
            fixture.context.talk_data_offset == 0x0000410CU &&
            fixture.indexed_target_selector == 0U &&
            fixture.state.previous_opcode == OP_41_RELOAD_INDEXED_TARGET,
        "real opcode 41 record selects target three, resets selector, and reloads"
    );
}

void test_real_interaction_lock_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000046EE);
    std::array<u8, 2U> set_instruction{};
    input.read(
        reinterpret_cast<char*>(set_instruction.data()),
        static_cast<std::streamsize>(set_instruction.size())
    );
    input.seekg(0x0000A164);
    std::array<u8, 2U> clear_instruction{};
    input.read(
        reinterpret_cast<char*>(clear_instruction.data()),
        static_cast<std::streamsize>(clear_instruction.size())
    );
    const bool instructions_read = static_cast<bool>(input);

    Fixture set_fixture;
    set_fixture.dialogs.close.flagged_dialog_counter = 0x12340005U;
    set_fixture.roles[0].action.base_variant = 7U;
    prime_loaded_instruction(
        set_fixture, OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT
    );
    std::ranges::copy(set_instruction, set_fixture.state.window.begin());
    write_u16(set_fixture.state.window, 2U, OP_1025);
    const auto set_result = set_fixture.step();

    Fixture clear_fixture;
    clear_fixture.dialogs.close.flagged_dialog_counter = 0x12348005U;
    prime_loaded_instruction(clear_fixture, OP_43_CLEAR_INTERACTION_LOCK);
    std::ranges::copy(clear_instruction, clear_fixture.state.window.begin());
    write_u16(clear_fixture.state.window, 2U, OP_1025);
    const auto clear_result = clear_fixture.step();

    test.expect_true(
        instructions_read &&
            read_u16(set_instruction, 0U) ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
            read_u16(clear_instruction, 0U) == OP_43_CLEAR_INTERACTION_LOCK &&
            set_result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            set_result.executed_instruction_count == 2U &&
            set_fixture.dialogs.close.flagged_dialog_counter == 0x12348005U &&
            set_fixture.roles[0].action.base_variant == 0U &&
            set_fixture.ports.action_update_count == 1U &&
            set_fixture.state.previous_opcode ==
                OP_42_SET_INTERACTION_LOCK_AND_RESET_BASE_VARIANT &&
            clear_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            clear_result.executed_instruction_count == 2U &&
            clear_fixture.dialogs.close.flagged_dialog_counter == 0x12340005U &&
            clear_fixture.ports.action_update_count == 0U &&
            clear_fixture.state.previous_opcode == OP_43_CLEAR_INTERACTION_LOCK,
        "real opcodes 42 and 43 set and clear the shared interaction lock"
    );
}

void test_real_set_role_action_wait_override_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00041D04);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[1].guid = 0x027FU;
    fixture.roles[1].action.wait_remaining = 9U;
    fixture.roles[1].action.wait_override = 0x8123U;
    prime_loaded_instruction(fixture, OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            read_u16(instruction, 2U) == 0x027FU &&
            read_u16(instruction, 4U) == 0U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 1U &&
            fixture.roles[1].action.wait_override == 0U &&
            fixture.roles[1].action.wait_remaining == 0U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode ==
                OP_44_SET_ROLE_ACTION_WAIT_OVERRIDE &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 44 writes the role wait override and clears remaining wait"
    );
}

void test_real_set_role_action_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000051C9);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[1].action.action_id = 0xDEADBEEFU;
    prime_loaded_instruction(fixture, OP_45_SET_ROLE_ACTION_ID);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_45_SET_ROLE_ACTION_ID &&
            read_u16(instruction, 2U) == 0x00F8U &&
            read_u16(instruction, 4U) == 0x0223U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 1U &&
            fixture.roles[1].action.action_id == 0x0223U &&
            (fixture.roles[1].flags & 0x1000U) != 0U &&
            fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_45_SET_ROLE_ACTION_ID &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 45 changes the requested role action and continues"
    );
}

void test_real_camera_move_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    std::array<u8, 10U> relative_instruction{};
    input.seekg(0x0000A18C);
    input.read(
        reinterpret_cast<char*>(relative_instruction.data()),
        static_cast<std::streamsize>(relative_instruction.size())
    );
    const bool relative_read = static_cast<bool>(input);

    input.clear();
    std::array<u8, 10U> absolute_instruction{};
    input.seekg(0x000046B8);
    input.read(
        reinterpret_cast<char*>(absolute_instruction.data()),
        static_cast<std::streamsize>(absolute_instruction.size())
    );
    const bool absolute_read = static_cast<bool>(input);

    input.clear();
    std::array<u8, 8U> role_instruction{};
    input.seekg(0x000096A3);
    input.read(
        reinterpret_cast<char*>(role_instruction.data()),
        static_cast<std::streamsize>(role_instruction.size())
    );
    const bool role_read = static_cast<bool>(input);

    CameraMoveFixture relative;
    relative.camera.right = 640U;
    relative.camera.bottom = 1120U;
    prime_loaded_instruction(relative, OP_50_START_RELATIVE_CAMERA_MOVE);
    std::ranges::copy(relative_instruction, relative.state.window.begin());
    write_u16(relative.state.window, 10U, OP_1025);
    const auto relative_result = relative.step(0, 640);

    CameraMoveFixture absolute;
    absolute.camera.right = 656U;
    absolute.camera.bottom = 512U;
    prime_loaded_instruction(absolute, OP_70_START_ABSOLUTE_CAMERA_MOVE);
    std::ranges::copy(absolute_instruction, absolute.state.window.begin());
    write_u16(absolute.state.window, 10U, OP_1025);
    const auto absolute_result = absolute.step(16, 32);

    CameraMoveFixture role;
    role.roles[1].guid = 1U;
    role.roles[1].world_x = 800U;
    role.roles[1].world_y = 640U;
    prime_loaded_instruction(role, OP_73_START_CAMERA_MOVE_TO_ROLE);
    std::ranges::copy(role_instruction, role.state.window.begin());
    write_u16(role.state.window, 8U, OP_1025);
    const auto role_result = role.step();

    test.expect_true(
        relative_read &&
            read_u16(relative_instruction, 0U) ==
                OP_50_START_RELATIVE_CAMERA_MOVE &&
            static_cast<i16>(read_u16(relative_instruction, 2U)) == 0 &&
            static_cast<i16>(read_u16(relative_instruction, 4U)) == -32 &&
            read_u16(relative_instruction, 6U) == 8U &&
            read_u16(relative_instruction, 8U) == 4U &&
            relative_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            relative_result.executed_instruction_count == 2U &&
            relative.camera_pan.remaining_x == 0 &&
            relative.camera_pan.remaining_y == -512 &&
            relative.camera_pan.step_x == 0 &&
            relative.camera_pan.step_y == -4 &&
            relative.context.instruction_offset == 10U &&
            relative.state.previous_opcode == OP_50_START_RELATIVE_CAMERA_MOVE,
        "real opcode 50 starts the requested relative camera move"
    );
    test.expect_true(
        absolute_read &&
            read_u16(absolute_instruction, 0U) ==
                OP_70_START_ABSOLUTE_CAMERA_MOVE &&
            read_u16(absolute_instruction, 2U) == 5U &&
            read_u16(absolute_instruction, 4U) == 8U &&
            read_u16(absolute_instruction, 6U) == 4U &&
            read_u16(absolute_instruction, 8U) == 16U &&
            absolute_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            absolute_result.executed_instruction_count == 2U &&
            absolute.camera_pan.remaining_x == 64 &&
            absolute.camera_pan.remaining_y == 96 &&
            absolute.camera_pan.step_x == 4 &&
            absolute.camera_pan.step_y == 16 &&
            absolute.context.instruction_offset == 10U &&
            absolute.state.previous_opcode == OP_70_START_ABSOLUTE_CAMERA_MOVE,
        "real opcode 70 starts the requested absolute camera move"
    );
    test.expect_true(
        role_read &&
            read_u16(role_instruction, 0U) == OP_73_START_CAMERA_MOVE_TO_ROLE &&
            read_u16(role_instruction, 2U) == 1U &&
            read_u16(role_instruction, 4U) == 8U &&
            read_u16(role_instruction, 6U) == 8U &&
            role_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            role_result.executed_instruction_count == 2U &&
            role.camera_pan.remaining_x == 480 &&
            role.camera_pan.remaining_y == 400 && role.camera_pan.step_x == 8 &&
            role.camera_pan.step_y == 8 &&
            role.context.instruction_offset == 8U &&
            role.state.previous_opcode == OP_73_START_CAMERA_MOVE_TO_ROLE,
        "real opcode 73 starts a camera move toward role 1"
    );
}

void test_real_start_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000043B8);
    std::array<u8, 16U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState color{};
    fixture.runtime.frame_color = &color;
    prime_loaded_instruction(fixture, OP_52_START_FRAME_COLOR_TRANSITION);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 16U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_52_START_FRAME_COLOR_TRANSITION &&
            static_cast<i16>(read_u16(instruction, 2U)) == -30 &&
            static_cast<i16>(read_u16(instruction, 4U)) == -30 &&
            static_cast<i16>(read_u16(instruction, 6U)) == -30 &&
            read_u16(instruction, 8U) == 0U &&
            read_u16(instruction, 10U) == 0U &&
            read_u16(instruction, 12U) == 0U &&
            read_u16(instruction, 14U) == 6U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            color.current_red == -30.0F && color.current_green == -30.0F &&
            color.current_blue == -30.0F && color.target_red == 0.0F &&
            color.target_green == 0.0F && color.target_blue == 0.0F &&
            color.countdown == 6 && color.step_red == 5.0F &&
            color.step_green == 5.0F && color.step_blue == 5.0F &&
            fixture.context.instruction_offset == 16U &&
            fixture.state.previous_opcode ==
                OP_52_START_FRAME_COLOR_TRANSITION &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 52 initializes the scripted frame-color transition"
    );
}

void test_real_shared_picture_action_enqueue_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    const auto read_record =
        [&root](const std::streamoff file_offset) -> std::array<u8, 10U> {
        std::ifstream input{
            root / "TALK1.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(file_offset);
        std::array<u8, 10U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        return instruction;
    };
    const auto primary = read_record(0x0000549F);
    const auto secondary_first = read_record(0x0000468A);
    const auto secondary_second = read_record(0x00004698);

    Fixture fixture;
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    fixture.runtime.picture_actions = &picture_actions;
    const auto execute = [&fixture](const std::array<u8, 10U>& instruction) {
        prime_loaded_instruction(fixture, read_u16(instruction, 0U));
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;
        return fixture.step();
    };

    const auto primary_result = execute(primary);
    const auto first_secondary_result = execute(secondary_first);
    const auto second_secondary_result = execute(secondary_second);
    const auto secondary_tail = picture_actions.secondary.size() >= 2U
        ? std::next(picture_actions.secondary.begin())
        : picture_actions.secondary.end();

    test.expect_true(
        read_u16(primary, 0U) == OP_58_ENQUEUE_PRIMARY_PICTURE_ACTION &&
            read_u16(secondary_first, 0U) ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            read_u16(secondary_second, 0U) ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            primary_result.status == LegacyWorldStoryVmStatus::yielded &&
            first_secondary_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            second_secondary_result.status ==
                LegacyWorldStoryVmStatus::yielded &&
            picture_actions.primary.size() == 1U &&
            picture_actions.secondary.size() == 2U &&
            picture_actions.primary.front().screen_x == 82U &&
            picture_actions.primary.front().screen_y == 344U &&
            picture_actions.primary.front().action.action_id == 9006U &&
            picture_actions.primary.front().action.base_variant == 2U &&
            picture_actions.secondary.front().screen_x == 480U &&
            picture_actions.secondary.front().screen_y == 400U &&
            picture_actions.secondary.front().action.action_id == 9050U &&
            picture_actions.secondary.front().action.base_variant == 1U &&
            secondary_tail->screen_x == 360U &&
            secondary_tail->screen_y == 400U &&
            secondary_tail->action.action_id == 9050U &&
            secondary_tail->action.base_variant == 0U &&
            fixture.context.instruction_offset == 10U &&
            fixture.state.previous_opcode ==
                OP_153_ENQUEUE_SECONDARY_PICTURE_ACTION &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcodes 58 and 153 prepend primary and secondary picture actions"
    );
}

void test_real_shared_role_spatial_group_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct RealRecord {
        std::streamoff file_offset;
        u16 opcode;
        u16 selector;
        u32 old_group;
        u32 target_group;
    };
    constexpr std::array<RealRecord, 4U> records{
        RealRecord{0x000121DA, OP_55_SET_ROLE_SPATIAL_GROUP_1, 322U, 0U, 1U},
        RealRecord{0x00012B12, OP_56_SET_ROLE_SPATIAL_GROUP_0, 322U, 2U, 0U},
        RealRecord{0x00005084, OP_57_SET_ROLE_SPATIAL_GROUP_2, 701U, 1U, 2U},
        RealRecord{0x0000526B, OP_57_SET_ROLE_SPATIAL_GROUP_2, 702U, 1U, 2U},
    };
    constexpr std::size_t role_row =
        openswd3::world_map::kLegacySpatialRowPadding + 2U;

    for (const RealRecord record : records) {
        std::ifstream input{
            root / "TALK4.DAT", std::ios::binary | std::ios::in
        };
        input.seekg(record.file_offset);
        std::array<u8, 4U> instruction{};
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        const bool instruction_read = static_cast<bool>(input);

        Fixture fixture;
        openswd3::world_map::LegacyRoleSpatialIndex spatial;
        spatial.map_height = 4U;
        for (auto& group : spatial.row_heads) {
            group.assign(44U, 0U);
        }
        fixture.roles[1].guid = record.selector;
        fixture.roles[1].world_y = 32U;
        fixture.roles[1].flags = record.old_group;
        const bool inserted = openswd3::world_map::insert_legacy_role_spatially(
            spatial, fixture.roles, 1U, record.old_group
        );
        fixture.runtime.spatial_index = &spatial;
        prime_loaded_instruction(fixture, record.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.state.previous_opcode = 0x66U;

        const auto result = fixture.step();

        test.expect_true(
            instruction_read && inserted &&
                read_u16(instruction, 0U) == record.opcode &&
                read_u16(instruction, 2U) == record.selector &&
                result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == record.opcode &&
                result.executed_instruction_count == 1U &&
                (fixture.roles[1].flags & 3U) == record.target_group &&
                spatial.row_heads[record.old_group][role_row] == 0U &&
                spatial.row_heads[record.target_group][role_row] == 1U &&
                fixture.context.instruction_offset == 4U &&
                fixture.state.previous_opcode == record.opcode &&
                fixture.ports.direct_audio_service_count == 0U,
            "real opcodes 55-57 move the selected role between spatial groups"
        );
    }
}

void test_real_repeat_role_action_refresh_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00005A6B);
    std::array<u8, 6U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    fixture.roles[2].guid = 1U;
    auto& action = fixture.roles[2].action;
    action.command_cursor = 0x1111U;
    action.wait_remaining = 0x2222U;
    action.field_58 = 0x3333U;
    prime_loaded_instruction(fixture, OP_54_REPEAT_ROLE_ACTION_REFRESH);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 6U, OP_1025);
    const auto result = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            read_u16(instruction, 2U) == 1U &&
            static_cast<i16>(read_u16(instruction, 4U)) == 1 &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            result.action_update_count == 2U &&
            result.action_update_failure_count == 0U &&
            action.command_cursor == 0U && action.wait_remaining == 0U &&
            action.field_58 == 0U && fixture.context.instruction_offset == 6U &&
            fixture.state.previous_opcode == OP_54_REPEAT_ROLE_ACTION_REFRESH &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 54 performs the initial and requested repeated refresh"
    );
}

void test_real_wait_for_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000043B8);
    std::array<u8, 18U> instructions{};
    input.read(
        reinterpret_cast<char*>(instructions.data()),
        static_cast<std::streamsize>(instructions.size())
    );
    const bool instructions_read = static_cast<bool>(input);

    Fixture fixture;
    openswd3::rendering::LegacyFrameColorTransitionState color{};
    fixture.runtime.frame_color = &color;
    prime_loaded_instruction(fixture, OP_52_START_FRAME_COLOR_TRANSITION);
    std::ranges::copy(instructions, fixture.state.window.begin());
    write_u16(fixture.state.window, 18U, OP_1025);

    const auto waiting_result = fixture.step();
    const bool waiting_state =
        waiting_result.status == LegacyWorldStoryVmStatus::yielded &&
        waiting_result.opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
        waiting_result.executed_instruction_count == 2U &&
        color.countdown == 6 && color.step_red == 5.0F &&
        color.step_green == 5.0F && color.step_blue == 5.0F &&
        fixture.context.instruction_offset == 16U &&
        fixture.state.previous_opcode == OP_53_WAIT_FRAME_COLOR_TRANSITION;

    color.countdown = 0;
    const auto completed_result = fixture.step();

    test.expect_true(
        instructions_read &&
            read_u16(instructions, 0U) == OP_52_START_FRAME_COLOR_TRANSITION &&
            read_u16(instructions, 16U) == OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            waiting_state &&
            completed_result.status ==
                LegacyWorldStoryVmStatus::unsupported_opcode &&
            completed_result.opcode == OP_1025 &&
            completed_result.executed_instruction_count == 2U &&
            color.countdown == 0 && fixture.context.instruction_offset == 18U &&
            fixture.state.previous_opcode ==
                OP_53_WAIT_FRAME_COLOR_TRANSITION &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 53 waits after transition start and continues after completion"
    );
}

void test_real_wait_for_camera_move_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000046C2);
    std::array<u8, 2U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    CameraMoveFixture fixture;
    fixture.state.previous_opcode = 0x55U;
    prime_loaded_instruction(fixture, OP_51_WAIT_CAMERA_MOVE_COMPLETE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 2U, OP_1025);
    fixture.camera_pan.remaining_x = 64;
    fixture.camera_pan.step_x = 4;

    const auto waiting = fixture.step();
    const u16 waiting_offset = fixture.context.instruction_offset;
    const u32 waiting_previous = fixture.state.previous_opcode;
    const i32 waiting_remaining_x = fixture.camera_pan.remaining_x;
    const i32 waiting_step_x = fixture.camera_pan.step_x;
    fixture.camera_pan.remaining_x = 0;
    fixture.camera_pan.step_x = 0;
    const auto completed = fixture.step();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting.status == LegacyWorldStoryVmStatus::yielded &&
            waiting.opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting.executed_instruction_count == 1U && waiting_offset == 0U &&
            waiting_previous == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            waiting_remaining_x == 64 && waiting_step_x == 4 &&
            completed.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            completed.opcode == OP_1025 &&
            completed.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 2U &&
            fixture.state.previous_opcode == OP_51_WAIT_CAMERA_MOVE_COMPLETE &&
            fixture.ports.direct_audio_service_count == 0U,
        "real opcode 51 waits for camera motion then continues"
    );
}

void test_real_set_role_flag_8000_and_clear_one_shots_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x000049F0);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(
        fixture, OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS
    );
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode ==
                OP_39_SET_ROLE_FLAG_8000_AND_CLEAR_ONE_SHOTS &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.flags_or_mask == 0x8000U &&
            request.flags_and_mask == 0xFFFFU,
        "real opcode 39 record patches missing role flag then continues"
    );
}

void test_real_clear_role_from_scene_record(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    std::ifstream input{root / "TALK1.DAT", std::ios::binary | std::ios::in};
    input.seekg(0x00004656);
    std::array<u8, 4U> instruction{};
    input.read(
        reinterpret_cast<char*>(instruction.data()),
        static_cast<std::streamsize>(instruction.size())
    );
    const bool instruction_read = static_cast<bool>(input);

    Fixture fixture;
    prime_loaded_instruction(fixture, OP_38_CLEAR_ROLE_FROM_SCENE);
    std::ranges::copy(instruction, fixture.state.window.begin());
    write_u16(fixture.state.window, 4U, OP_1025);
    const auto result = fixture.step();
    const auto request = fixture.ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : fixture.ports.role_patch_requests.front();

    test.expect_true(
        instruction_read &&
            read_u16(instruction, 0U) == OP_38_CLEAR_ROLE_FROM_SCENE &&
            read_u16(instruction, 2U) == 1U &&
            result.status == LegacyWorldStoryVmStatus::unsupported_opcode &&
            result.opcode == OP_1025 &&
            result.executed_instruction_count == 2U &&
            fixture.context.instruction_offset == 4U &&
            fixture.state.previous_opcode == OP_38_CLEAR_ROLE_FROM_SCENE &&
            fixture.ports.role_patch_requests.size() == 1U &&
            request.guid == 1U && request.flags_or_mask == 0U &&
            request.flags_and_mask == 0x7FFFU,
        "real opcode 38 record patches missing role flags then continues"
    );
}

void test_real_shared_dialog_handler_records(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    struct Sample {
        const char* filename;
        std::streamoff file_offset;
        std::size_t size;
        u16 opcode;
    };
    constexpr std::array<Sample, 8U> samples{
        Sample{"TALK4.DAT", 0x000304C5, 36U, 1U},
        Sample{"TALK1.DAT", 0x00007533, 23U, 2U},
        Sample{"TALK1.DAT", 0x00004295, 77U, 3U},
        Sample{"TALK1.DAT", 0x00004422, 185U, 4U},
        Sample{"TALK1.DAT", 0x00020CA4, 51U, 5U},
        Sample{"TALK1.DAT", 0x000046CC, 34U, 6U},
        Sample{"TALK1.DAT", 0x00002634, 56U, 89U},
        Sample{"TALK1.DAT", 0x000027A2, 32U, 90U},
    };
    constexpr std::array<u8, 2U> percent_t{'%', 'T'};

    for (const auto& sample : samples) {
        std::ifstream input{
            root / sample.filename, std::ios::binary | std::ios::in
        };
        std::vector<u8> instruction(sample.size);
        input.seekg(sample.file_offset);
        input.read(
            reinterpret_cast<char*>(instruction.data()),
            static_cast<std::streamsize>(instruction.size())
        );
        if (!input) {
            test.expect_true(false, "real shared-dialog sample is readable");
            continue;
        }

        Fixture fixture;
        prime_loaded_instruction(fixture, sample.opcode);
        std::ranges::copy(instruction, fixture.state.window.begin());
        fixture.roles[0].guid = 0xEEEEU;
        const u16 selector = read_u16(instruction, 2U);
        fixture.roles[1].guid =
            selector == 0xFFF0U ? fixture.context.source_guid : selector;
        const auto result = fixture.step();
        const u32 mode =
            sample.opcode <= 2U ? 0U : (sample.opcode <= 6U ? 1U : 2U);
        const std::size_t text_offset =
            mode == 0U ? 6U : (mode == 1U ? 14U : 10U);
        const auto& message = fixture.dialogs.messages.front();
        const bool odd_variant = (sample.opcode & 1U) != 0U;
        const auto percent_t_match =
            std::ranges::search(message.text, percent_t);
        test.expect_true(
            result.status == LegacyWorldStoryVmStatus::yielded &&
                result.opcode == sample.opcode &&
                result.executed_instruction_count == 1U &&
                result.dialog_enqueue_count == 1U &&
                result.dialog_text_prepare_count == 1U &&
                result.dialog_text_prepare_success_count == 0U &&
                result.direct_audio_service_count == 2U &&
                fixture.context.instruction_offset == sample.size &&
                message.text.size() == sample.size - text_offset &&
                fixture.roles[1].interaction_gate == (odd_variant ? 1U : 2U) &&
                fixture.ports.story_protocol_events ==
                    std::vector<u32>{2U, 3U, 4U, 2U} &&
                (sample.opcode != 1U ||
                 percent_t_match.begin() != message.text.end()),
            "one real physical record closes each shared dialog variant"
        );
    }
}

void test_real_story_248_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = 248U;
    context.talk_script_id = 248U;
    std::array<LegacyWorldRoleRecord, 2U> roles{};
    std::array<
        LegacyWorldObjectSlot,
        openswd3::world_map::kLegacyWorldActiveObjectSlotCount>
        active_object_slots{};
    roles[0].world_x = 16U;
    roles[0].world_y = 16U;
    roles[1].guid = 248U;
    roles[1].flags = 0U;
    roles[1].world_x = 320U;
    roles[1].world_y = 240U;
    roles[1].action.variant_delta = 4U;
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U, 0x2331U};
    constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU, 0x233BU};
    for (std::size_t index = 0U; index < kFrames.size(); ++index) {
        dialog_resources.frame_actions[index].action_id = kFrames[index];
        dialog_resources.caption_actions[index].action_id = kCaptions[index];
    }
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    openswd3::world_map::LegacyWorldCameraRect camera{};
    const openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .camera = &camera,
    };

    const auto result = openswd3::world_map::step_legacy_world_story_vm(
        context,
        state,
        roles,
        0U,
        active_object_slots,
        databases.maps_payload_bytes(),
        dialogs,
        dialog_resources,
        first_name,
        second_name,
        runtime,
        ports
    );
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            result.status == LegacyWorldStoryVmStatus::yielded &&
            result.opcode == 89U && result.executed_instruction_count == 3U &&
            result.dialog_enqueue_count == 1U &&
            dialogs.messages.size() == 1U &&
            !dialogs.messages.front().caption.empty() &&
            dialogs.messages.front().record.width == 154U &&
            dialogs.messages.front().record.height == 88U &&
            roles[1].interaction_gate == 1U,
        "real story 248 executes 0x402, 91 and 89 into its first dialog"
    );
}

void test_real_new_game_story_patches_unloaded_role(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    auto payload = databases.mutable_maps_payload_bytes();
    const auto decoded =
        openswd3::world_map::decode_legacy_maps_world_database(payload);
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            decoded.status ==
                openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready,
        "real new-game patch test decodes the current MAPS database"
    );
    if (initialized.status !=
            openswd3::resource_io::LegacyResourceDatabaseStatus::ready ||
        maps.status != openswd3::resource_io::LegacyMapsPayloadStatus::ready ||
        decoded.status !=
            openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready) {
        return;
    }

    StoryTestTree tree;
    openswd3::asset_runtime::LegacyActRuntime act_runtime{root};
    openswd3::asset_runtime::LegacyActActionStreamProvider action_provider{
        act_runtime
    };
    openswd3::asset_runtime::LegacyActionUpdater action_updater{
        action_provider
    };
    openswd3::world_map::LegacyWorldActionUpdaterInitializer action_initializer{
        action_updater
    };
    auto loaded = openswd3::world_map::load_legacy_world_runtime_session(
        payload,
        openswd3::world_map::LegacyWorldRuntimeSessionRequest{
            .archive_path = root / "huge.lmf",
            .cache_directory = tree.root() / "cache" / "maps",
            .load = decoded.database.initial_load,
            .cache_limit_megabytes = 60U,
            .pixel_conversion = rgb565_conversion(),
        },
        action_initializer
    );
    test.expect_equal(
        loaded.status,
        openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready,
        "real new-game patch test creates the exact initial world session"
    );
    if (loaded.status !=
        openswd3::world_map::LegacyWorldRuntimeSessionStatus::ready) {
        return;
    }

    auto& world = loaded.session;
    auto& map = world.render.map_load.session;
    auto& roles = map.business.state.roles;
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = decoded.database.initial_load.selected_guid;
    context.talk_script_id = 100U;
    openswd3::world_map::LegacyWorldMapRolePathState map_role_paths{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    openswd3::world_map::LegacyWorldCameraPanState camera_pan{};
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_row_effects;
    openswd3::world_map::LegacyRoleHeadActionList role_head_actions;
    u32 battle_request_value{};
    u32 indexed_target_selector{};
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{};
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldPathNodePool path_node_pool;
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = roles,
        .active_object_slots = map_role_paths.active_object_slots,
        .spatial_index = &map.business.state.spatial_index,
        .role_surface =
            {
                .map_width = map.header.width,
                .selected_guid = roles[world.selected_role_index].guid,
                .surface_grid = map.surface_grid.surface_grid,
            },
        .node_pool = &path_node_pool,
        .movement = &movement,
        .camera = &world.camera,
        .selected_arrival_bytes = map_role_paths.guid_one_arrival_bytes,
        .selected_role_index = world.selected_role_index,
        .map_height = map.header.height,
        .scene_render_flags = &scene_render_flags,
    };
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .spatial_index = &map.business.state.spatial_index,
        .role_surface =
            {
                .map_width = map.header.width,
                .selected_guid = roles[world.selected_role_index].guid,
                .surface_grid = map.surface_grid.surface_grid,
            },
        .camera = &world.camera,
        .camera_pan = &camera_pan,
        .movement = &movement,
        .picture_actions = &picture_actions,
        .packed_row_effects = &packed_row_effects,
        .role_head_actions = &role_head_actions,
        .battle_request_value = &battle_request_value,
        .frame_color = &frame_color,
        .story_paths = &story_paths,
        .indexed_target_selector = &indexed_target_selector,
        .scene_render_flags = &scene_render_flags,
        .map_height = map.header.height,
    };
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto step = [&] {
        return openswd3::world_map::step_legacy_world_story_vm(
            context,
            state,
            roles,
            world.selected_role_index,
            map_role_paths.active_object_slots,
            databases.maps_payload_bytes(),
            dialogs,
            dialog_resources,
            first_name,
            second_name,
            runtime,
            ports
        );
    };

    const auto first_clear = step();
    const auto opening_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto opening_video = step();
    auto boundary = opening_video;
    std::size_t boundary_count{};
    for (; boundary_count < 64U && ports.role_patch_requests.size() < 3U;
         ++boundary_count) {
        if (boundary.opcode == 67U) {
            runtime.current_tick = state.wait_started_at +
                static_cast<u32>(state.wait_duration) + 1U;
        } else if (boundary.opcode == 51U) {
            while (openswd3::world_map::advance_legacy_world_camera_pan(
                world.camera, camera_pan
            )) {
            }
        } else if (
            boundary.opcode == 89U ||
            boundary.status != LegacyWorldStoryVmStatus::yielded
        ) {
            break;
        }
        boundary = step();
    }
    const auto missing_patch = ports.role_patch_requests.empty()
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests.front();
    const auto second_missing_patch = ports.role_patch_requests.size() < 2U
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests[1U];
    const auto position_patch = ports.role_patch_requests.size() < 3U
        ? openswd3::world_map::LegacyMapsRolePatchRequest{}
        : ports.role_patch_requests[2U];
    const auto runtime_role = std::ranges::find(
        roles,
        missing_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto second_runtime_role = std::ranges::find(
        roles,
        second_missing_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto position_runtime_role = std::ranges::find(
        roles,
        position_patch.guid,
        &openswd3::world_map::LegacyWorldRoleRecord::guid
    );
    const auto source_role = std::ranges::find(
        decoded.database.role_sources,
        missing_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );
    const auto second_source_role = std::ranges::find(
        decoded.database.role_sources,
        second_missing_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );
    const auto position_source_role = std::ranges::find(
        decoded.database.role_sources,
        position_patch.guid,
        &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
    );

    test.expect_true(
        first_clear.status == LegacyWorldStoryVmStatus::yielded &&
            first_clear.opcode == 61U &&
            opening_wait.status == LegacyWorldStoryVmStatus::yielded &&
            opening_wait.opcode == 67U &&
            opening_video.status == LegacyWorldStoryVmStatus::yielded &&
            opening_video.opcode == 85U,
        "real initial roles execute the opening story through its video boundary"
    );
    test.expect_equal(
        roles.size(), std::size_t{33U}, "real initial world role count"
    );
    test.expect_equal(
        ports.role_patch_requests.size(),
        std::size_t{3U},
        "real opening TALK100 MAPS patch count"
    );
    test.expect_true(
        boundary_count < 64U,
        "real opening TALK100 reaches its MAPS patch boundary"
    );
    test.expect_true(
        boundary.status == LegacyWorldStoryVmStatus::yielded &&
            boundary.opcode == 61U,
        "real opening TALK100 continues past opcode 40"
    );
    test.expect_true(
        runtime_role == roles.end(),
        "first patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        second_runtime_role == roles.end(),
        "second patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        position_runtime_role == roles.end(),
        "position-patched TALK100 role is absent from runtime"
    );
    test.expect_true(
        source_role != decoded.database.role_sources.end(),
        "first patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        second_source_role != decoded.database.role_sources.end(),
        "second patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        position_source_role != decoded.database.role_sources.end(),
        "position-patched TALK100 role exists in MAPS sources"
    );
    test.expect_true(
        missing_patch.guid == 123U && missing_patch.action_id == 561U &&
            missing_patch.base_variant == 8U &&
            missing_patch.variant_delta == 0U &&
            second_missing_patch.guid == 240U &&
            second_missing_patch.action_id == 561U &&
            second_missing_patch.base_variant == 0U &&
            second_missing_patch.variant_delta == 1U,
        "real TALK100 preserves both unloaded-role action patch operands"
    );
    test.expect_equal(
        missing_patch.flags_or_mask,
        u16{0x1000U},
        "real opening TALK100 role patch OR mask"
    );
    test.expect_equal(
        missing_patch.flags_and_mask,
        u16{0xFFFFU},
        "real opening TALK100 role patch AND mask"
    );
    test.expect_equal(
        missing_patch.logical_map_id,
        u16{0xFFFFU},
        "real opening TALK100 role patch preserves map id"
    );
    test.expect_true(
        second_missing_patch.flags_or_mask == 0x1000U &&
            second_missing_patch.flags_and_mask == 0xFFFFU &&
            second_missing_patch.logical_map_id == 0xFFFFU,
        "second real TALK100 role patch preserves masks and map id"
    );
    test.expect_true(
        position_patch.guid == 195U && position_patch.tile_x == 16U &&
            position_patch.tile_y == 36U &&
            position_patch.flags_or_mask == 0U &&
            position_patch.flags_and_mask == 0xFFFFU &&
            position_patch.logical_map_id == 0xFFFFU,
        "real TALK100 preserves the missing GUID 195 opcode-40 patch"
    );
}

void test_real_new_game_story_reaches_first_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
) {
    openswd3::resource_io::LegacyResourceDatabases databases;
    const auto initialized = databases.initialize(root);
    const auto maps = databases.reload_maps_payload();
    const auto maps_world =
        openswd3::world_map::decode_legacy_maps_world_database(
            databases.maps_payload_bytes()
        );
    LegacyWorldStoryVmState state{};
    openswd3::world_map::initialize_legacy_world_story_vm(state);
    LegacyWorldTalkContext context{};
    context.source_guid = 1U;
    context.talk_script_id = 100U;

    std::vector<LegacyWorldRoleRecord> roles(11U);
    const auto initialize_role = [&](const std::size_t index,
                                     const u16 guid,
                                     const u32 tile_x,
                                     const u32 tile_y) {
        auto& role = roles[index];
        role.guid = guid;
        role.flags = 0U;
        const auto source = std::ranges::find(
            maps_world.database.role_sources,
            guid,
            &openswd3::world_map::LegacyMapsRoleSourceRecord::guid
        );
        if (source != maps_world.database.role_sources.end()) {
            role.flags |= source->flags;
        }
        role.world_x = tile_x << 4U;
        role.world_y = tile_y << 4U;
        role.map_cell_pointer_32 = tile_y * 80U + tile_x;
        role.action.field_2c = 1U;
        role.action.field_30 = 1U;
    };
    initialize_role(1U, 1U, 2U, 2U);
    initialize_role(2U, 1U, 3U, 3U);
    initialize_role(3U, 123U, 4U, 4U);
    initialize_role(4U, 240U, 5U, 5U);
    initialize_role(5U, 195U, 6U, 6U);
    initialize_role(6U, 248U, 7U, 7U);
    initialize_role(7U, 249U, 8U, 8U);
    initialize_role(8U, 191U, 9U, 9U);
    initialize_role(9U, 250U, 10U, 10U);
    initialize_role(10U, 251U, 11U, 11U);

    openswd3::world_map::LegacyRoleSpatialIndex spatial_index;
    spatial_index.map_height = 80U;
    for (auto& row_heads : spatial_index.row_heads) {
        row_heads.assign(120U, openswd3::world_map::kLegacySpatialNoRole);
    }
    const bool inserted_role_one =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 1U, roles[1U].flags & 3U
        );
    const bool inserted_role_195 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 5U, roles[5U].flags & 3U
        );
    const bool inserted_role_248 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 6U, roles[6U].flags & 3U
        );
    const bool inserted_role_249 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 7U, roles[7U].flags & 3U
        );
    const bool inserted_role_250 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 9U, roles[9U].flags & 3U
        );
    const bool inserted_role_251 =
        openswd3::world_map::insert_legacy_role_spatially(
            spatial_index, roles, 10U, roles[10U].flags & 3U
        );

    std::vector<u8> surface_grid(80U * 80U * sizeof(u32), 0U);
    openswd3::world_map::LegacyWorldMapRolePathState map_role_paths{};
    openswd3::story_scene::LegacyDialogRuntimeState dialogs;
    openswd3::world_map::LegacyWorldDialogRuntimeState dialog_resources;
    constexpr std::array<u16, 4U> kFrames{0x232DU, 0x232FU, 0x2330U, 0x2331U};
    constexpr std::array<u16, 4U> kCaptions{0x2337U, 0x2339U, 0x233AU, 0x233BU};
    for (std::size_t index = 0U; index < kFrames.size(); ++index) {
        dialog_resources.frame_actions[index].action_id = kFrames[index];
        dialog_resources.caption_actions[index].action_id = kCaptions[index];
    }

    openswd3::world_map::LegacyWorldCameraRect camera{};
    camera.right = 640U;
    camera.bottom = 480U;
    openswd3::world_map::LegacyWorldCameraPanState camera_pan{};
    openswd3::world_map::LegacyWorldMovementRuntimeState movement{};
    openswd3::world_map::LegacyPictureActionLists picture_actions;
    std::list<openswd3::rendering::LegacyPackedRowEffect> packed_row_effects;
    openswd3::world_map::LegacyRoleHeadActionList role_head_actions;
    u32 battle_request_value{};
    u32 indexed_target_selector{};
    openswd3::rendering::LegacyFrameColorTransitionState frame_color{};
    openswd3::rendering::LegacyFramebuffer frame_color_framebuffer;
    openswd3::rendering::LegacyPixelConversionState frame_color_format;
    u8 scene_render_flags{};
    openswd3::world_map::LegacyWorldPathNodePool path_node_pool;
    openswd3::world_map::LegacyWorldStoryPathRuntime story_paths{
        .roles = roles,
        .active_object_slots = map_role_paths.active_object_slots,
        .spatial_index = &spatial_index,
        .role_surface =
            {
                .map_width = 80U,
                .selected_guid = 1U,
                .surface_grid = surface_grid,
            },
        .node_pool = &path_node_pool,
        .movement = &movement,
        .camera = &camera,
        .selected_arrival_bytes = map_role_paths.guid_one_arrival_bytes,
        .selected_role_index = 1U,
        .map_height = 80U,
        .scene_render_flags = &scene_render_flags,
    };
    openswd3::world_map::LegacyWorldStoryVmRuntime runtime{
        .spatial_index = &spatial_index,
        .role_surface =
            {
                .map_width = 80U,
                .selected_guid = 1U,
                .surface_grid = surface_grid,
            },
        .camera = &camera,
        .camera_pan = &camera_pan,
        .movement = &movement,
        .picture_actions = &picture_actions,
        .packed_row_effects = &packed_row_effects,
        .role_head_actions = &role_head_actions,
        .battle_request_value = &battle_request_value,
        .frame_color = &frame_color,
        .story_paths = &story_paths,
        .indexed_target_selector = &indexed_target_selector,
        .scene_render_flags = &scene_render_flags,
        .map_height = 80U,
    };
    std::array<u8, 16U> first_name{};
    std::array<u8, 16U> second_name{};
    RealPorts ports{databases};
    const auto step = [&] {
        return openswd3::world_map::step_legacy_world_story_vm(
            context,
            state,
            roles,
            1U,
            map_role_paths.active_object_slots,
            databases.maps_payload_bytes(),
            dialogs,
            dialog_resources,
            first_name,
            second_name,
            runtime,
            ports
        );
    };
    StoryFrameActionPorts frame_actions;
    StoryPathCompletionPorts path_completion{story_paths};
    const auto advance_path_frame = [&] {
        return openswd3::world_map::advance_legacy_world_map_role_paths(
            roles,
            spatial_index,
            runtime.role_surface,
            1U,
            scene_render_flags,
            movement,
            camera,
            map_role_paths,
            frame_actions,
            path_completion
        );
    };

    const auto first_clear = step();
    const auto opening_wait = step();
    runtime.current_tick = 2001U;
    const auto opening_video = step();
    const auto branch_clear = step();
    const auto first_scene_wait = step();
    runtime.current_tick += 4501U;
    const auto first_picture = step();
    const auto second_scene_wait = step();
    runtime.current_tick += 7501U;
    const auto second_picture = step();
    const auto third_scene_wait = step();
    runtime.current_tick += 7501U;
    const auto transition_clear = step();
    const auto camera_wait = step();
    while (
        openswd3::world_map::advance_legacy_world_camera_pan(camera, camera_pan)
    ) {
    }
    const auto title = step();
    const auto title_record = dialogs.messages.back().record;
    roles[2].interaction_gate = 0U;
    const auto first_dialog = step();
    const u16 first_dialog_gate = roles[6].interaction_gate;
    const bool first_dialog_has_caption =
        !dialogs.messages.back().caption.empty();
    auto last_dialog = first_dialog;
    for (std::size_t dialog_index = 1U; dialog_index < 10U; ++dialog_index) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        test.expect_equal(
            released_dialog.opcode,
            u16{14U},
            "story 100 dialog release boundary"
        );
        last_dialog = step();
    }
    for (auto& role : roles) {
        role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_last_dialog = step();
    const auto first_path_scene = step();
    const auto first_path_schedule = step();
    auto first_path_wait = first_path_schedule;
    bool first_path_frames_completed = true;
    std::size_t first_path_frame_count{};
    for (; first_path_frame_count < 512U; ++first_path_frame_count) {
        first_path_wait = step();
        if (first_path_wait.opcode == 67U) {
            break;
        }
        const auto advanced = advance_path_frame();
        if (first_path_wait.opcode != 20U ||
            advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
            first_path_frames_completed = false;
            break;
        }
    }
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto second_path_schedule = step();
    auto second_path_wait = second_path_schedule;
    bool second_path_frames_completed = true;
    std::size_t second_path_frame_count{};
    for (; second_path_frame_count < 512U; ++second_path_frame_count) {
        second_path_wait = step();
        if (second_path_wait.opcode == 95U) {
            break;
        }
        const auto advanced = advance_path_frame();
        if (second_path_wait.opcode != 20U ||
            advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
            second_path_frames_completed = false;
            break;
        }
    }
    const auto hidden_scene_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto first_facing_wait = step();
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto next_dialog = step();
    const u32 path_final_facing = roles[1].action.variant_delta;
    auto later_dialog = next_dialog;
    bool later_dialog_chain_completed = true;
    for (std::size_t dialog_index = 0U; dialog_index < 5U; ++dialog_index) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS ||
            released_dialog.status != LegacyWorldStoryVmStatus::yielded) {
            later_dialog_chain_completed = false;
            break;
        }
        later_dialog = step();
        if (later_dialog.opcode != 89U ||
            later_dialog.status != LegacyWorldStoryVmStatus::yielded) {
            later_dialog_chain_completed = false;
            break;
        }
    }
    for (auto& role : roles) {
        role.interaction_gate = 0U;
    }
    context.field_26 = 0U;
    const auto released_later_dialog = step();
    const auto head_sign_wait = step();
    const u32 head_sign_token = roles[8].field_3c;
    runtime.current_tick =
        state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
    const auto post_head_sign_dialog = step();
    auto next_unsupported = post_head_sign_dialog;
    std::size_t post_head_sign_dialog_count{};
    bool post_head_sign_dialog_releases_completed = true;
    while (post_head_sign_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            post_head_sign_dialog_releases_completed = false;
            break;
        }
        next_unsupported = step();
        ++post_head_sign_dialog_count;
    }
    std::size_t third_path_frame_count{};
    bool third_path_frames_completed = true;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 20U) {
        for (; third_path_frame_count < 512U; ++third_path_frame_count) {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                third_path_frames_completed = false;
                break;
            }
            next_unsupported = step();
            if (next_unsupported.opcode != 20U) {
                break;
            }
        }
    }
    bool third_path_wait_completed = false;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 67U) {
        runtime.current_tick =
            state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
        next_unsupported = step();
        third_path_wait_completed = true;
    }
    std::size_t third_path_dialog_count{};
    bool third_path_dialog_releases_completed = true;
    while (third_path_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            third_path_dialog_releases_completed = false;
            break;
        }
        next_unsupported = step();
        ++third_path_dialog_count;
    }
    std::size_t final_dialog_count{};
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 67U) {
        runtime.current_tick =
            state.wait_started_at + static_cast<u32>(state.wait_duration) + 1U;
        next_unsupported = step();
    }
    while (final_dialog_count < 32U &&
           next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
           next_unsupported.opcode == 89U) {
        for (auto& role : roles) {
            role.interaction_gate = 0U;
        }
        context.field_26 = 0U;
        const auto released_dialog = step();
        if (released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
            released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
            break;
        }
        next_unsupported = step();
        ++final_dialog_count;
    }
    std::size_t fourth_path_frame_count{};
    bool fourth_path_frames_completed = true;
    if (next_unsupported.status == LegacyWorldStoryVmStatus::yielded &&
        next_unsupported.opcode == 20U) {
        for (; fourth_path_frame_count < 512U; ++fourth_path_frame_count) {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                fourth_path_frames_completed = false;
                break;
            }
            next_unsupported = step();
            if (next_unsupported.opcode != 20U) {
                break;
            }
        }
    }
    std::size_t post_opcode_45_wait_count{};
    std::size_t post_opcode_45_dialog_count{};
    std::size_t post_opcode_45_path_frame_count{};
    std::size_t post_opcode_45_color_wait_count{};
    bool post_opcode_45_progression_completed = true;
    bool battle_request_submitted = false;
    for (std::size_t boundary_count = 0U; boundary_count < 2048U &&
         next_unsupported.status == LegacyWorldStoryVmStatus::yielded;
         ++boundary_count) {
        switch (next_unsupported.opcode) {
        case 20U: {
            const auto advanced = advance_path_frame();
            if (advanced.status !=
                openswd3::world_map::LegacyWorldMapRolePathStatus::completed) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_path_frame_count;
            next_unsupported = step();
            break;
        }
        case 51U:
            while (openswd3::world_map::advance_legacy_world_camera_pan(
                camera, camera_pan
            )) {
            }
            next_unsupported = step();
            break;
        case 53U:
            if (const auto advanced =
                    openswd3::rendering::update_legacy_frame_color_transition(
                        frame_color,
                        true,
                        frame_color_framebuffer,
                        frame_color_format
                    );
                advanced.status !=
                openswd3::rendering::LegacyFrameColorTransitionStatus::
                    completed) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_color_wait_count;
            next_unsupported = step();
            break;
        case 67U:
            runtime.current_tick = state.wait_started_at +
                static_cast<u32>(state.wait_duration) + 1U;
            ++post_opcode_45_wait_count;
            next_unsupported = step();
            break;
        case 89U:
            for (auto& role : roles) {
                role.interaction_gate = 0U;
            }
            context.field_26 = 0U;
            if (const auto released_dialog = step();
                released_dialog.status != LegacyWorldStoryVmStatus::yielded ||
                released_dialog.opcode != OP_14_WAIT_ROLE_ACTION_STATUS) {
                post_opcode_45_progression_completed = false;
                break;
            }
            ++post_opcode_45_dialog_count;
            next_unsupported = step();
            break;
        case 88U:
            battle_request_submitted = true;
            break;
        default:
            next_unsupported = step();
            break;
        }
        if (!post_opcode_45_progression_completed || battle_request_submitted) {
            break;
        }
    }
    const std::string video_filename{
        ports.last_video_filename.begin(), ports.last_video_filename.end()
    };
    test.expect_equal(
        first_clear.opcode, u16{61U}, "story 100 first clear boundary"
    );
    test.expect_equal(
        opening_wait.opcode, u16{67U}, "story 100 opening wait boundary"
    );
    test.expect_equal(
        opening_video.opcode, u16{85U}, "story 100 video boundary"
    );
    test.expect_equal(
        branch_clear.opcode, u16{61U}, "story 100 branch clear boundary"
    );
    test.expect_equal(
        first_scene_wait.opcode, u16{67U}, "story 100 first scene wait boundary"
    );
    test.expect_equal(
        first_picture.opcode, u16{153U}, "story 100 first picture boundary"
    );
    test.expect_equal(
        second_scene_wait.opcode,
        u16{67U},
        "story 100 second scene wait boundary"
    );
    test.expect_equal(
        second_picture.opcode, u16{153U}, "story 100 second picture boundary"
    );
    test.expect_equal(
        third_scene_wait.opcode, u16{67U}, "story 100 third scene wait boundary"
    );
    test.expect_equal(
        transition_clear.opcode, u16{60U}, "story 100 transition clear boundary"
    );
    test.expect_equal(
        camera_wait.opcode, u16{51U}, "story 100 camera wait boundary"
    );
    test.expect_equal(title.opcode, u16{6U}, "story 100 title boundary");
    test.expect_equal(
        first_dialog.opcode, u16{89U}, "story 100 first spoken dialog boundary"
    );
    test.expect_equal(
        last_dialog.opcode, u16{89U}, "story 100 tenth spoken dialog boundary"
    );
    test.expect_equal(
        released_last_dialog.opcode,
        u16{14U},
        "story 100 last dialog release boundary"
    );
    test.expect_equal(
        first_path_scene.opcode, u16{94U}, "story 100 first path scene boundary"
    );
    test.expect_equal(
        first_path_schedule.opcode,
        u16{20U},
        "story 100 first path schedule boundary"
    );
    test.expect_equal(
        first_path_wait.opcode,
        u16{67U},
        "story 100 first path completion boundary"
    );
    test.expect_equal(
        second_path_schedule.opcode,
        u16{20U},
        "story 100 second path schedule boundary"
    );
    test.expect_equal(
        second_path_wait.opcode,
        u16{95U},
        "story 100 second path completion boundary"
    );
    test.expect_equal(
        hidden_scene_wait.opcode,
        u16{67U},
        "story 100 hidden-scene wait boundary"
    );
    test.expect_equal(
        first_facing_wait.opcode,
        u16{67U},
        "story 100 first facing wait boundary"
    );
    test.expect_equal(
        next_dialog.opcode, u16{89U}, "story 100 next dialog boundary"
    );
    test.expect_equal(
        later_dialog.opcode,
        u16{89U},
        "story 100 fifth post-path dialog boundary"
    );
    test.expect_equal(
        released_later_dialog.opcode,
        u16{14U},
        "story 100 fifth post-path dialog release boundary"
    );
    test.expect_equal(
        head_sign_wait.opcode, u16{67U}, "story 100 head-sign wait boundary"
    );
    test.expect_equal(
        post_head_sign_dialog.opcode,
        u16{89U},
        "story 100 post-head-sign dialog boundary"
    );
    test.expect_equal(
        post_head_sign_dialog_count,
        std::size_t{11U},
        "story 100 post-head-sign dialog count"
    );
    test.expect_equal(
        third_path_frame_count,
        std::size_t{46U},
        "story 100 third path frame count"
    );
    test.expect_equal(
        third_path_dialog_count,
        std::size_t{5U},
        "story 100 third path dialog count"
    );
    test.expect_equal(
        final_dialog_count, std::size_t{7U}, "story 100 final dialog count"
    );
    test.expect_equal(
        fourth_path_frame_count,
        std::size_t{40U},
        "story 100 fourth path frame count"
    );
    test.expect_equal(
        post_opcode_45_wait_count,
        std::size_t{8U},
        "story 100 post-opcode-45 wait count"
    );
    test.expect_equal(
        post_opcode_45_dialog_count,
        std::size_t{8U},
        "story 100 post-opcode-45 dialog count"
    );
    test.expect_equal(
        post_opcode_45_path_frame_count,
        std::size_t{65U},
        "story 100 post-opcode-45 path frame count"
    );
    test.expect_equal(
        post_opcode_45_color_wait_count,
        std::size_t{1U},
        "story 100 post-opcode-45 color wait count"
    );
    test.expect_equal(
        next_unsupported.opcode, u16{88U}, "story 100 battle request boundary"
    );
    test.expect_equal(
        next_unsupported.status,
        LegacyWorldStoryVmStatus::yielded,
        "story 100 yields after consuming opcode 88"
    );
    test.expect_equal(
        state.loaded_data_offset,
        u32{17476U},
        "story 100 branched window base through both paths"
    );
    test.expect_equal(
        context.instruction_offset,
        u16{3847U},
        "story 100 instruction boundary after opcode 88"
    );
    test.expect_equal(
        battle_request_value,
        u32{0x80000062U},
        "story 100 submits battle id 98 with the request tag"
    );
    test.expect_true(
        initialized.status ==
                openswd3::resource_io::LegacyResourceDatabaseStatus::ready &&
            maps.status ==
                openswd3::resource_io::LegacyMapsPayloadStatus::ready &&
            maps_world.status ==
                openswd3::world_map::LegacyMapsWorldDatabaseStatus::ready &&
            inserted_role_one && inserted_role_195 && inserted_role_248 &&
            inserted_role_249 && inserted_role_250 && inserted_role_251,
        "real story 100 fixture uses the real databases and valid spatial roles"
    );
    test.expect_true(
        first_clear.opcode == 61U && opening_wait.opcode == 67U &&
            opening_video.opcode == 85U && branch_clear.opcode == 61U &&
            first_scene_wait.opcode == 67U && first_picture.opcode == 153U &&
            second_scene_wait.opcode == 67U && second_picture.opcode == 153U &&
            third_scene_wait.opcode == 67U && transition_clear.opcode == 60U &&
            camera_wait.opcode == 51U && title.opcode == 6U &&
            title.status == LegacyWorldStoryVmStatus::yielded &&
            title_record.flags == 0x468U &&
            title_record.lifetime_limit == 20U && title_record.left == 20U &&
            title_record.top == 20U && title_record.width == 132U &&
            title_record.height == 22U && first_dialog.opcode == 89U &&
            first_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            first_dialog_has_caption && first_dialog_gate == 1U &&
            first_path_frames_completed && second_path_frames_completed &&
            first_path_frame_count < 512U && second_path_frame_count < 512U &&
            next_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            later_dialog_chain_completed &&
            head_sign_wait.status == LegacyWorldStoryVmStatus::yielded &&
            head_sign_token ==
                openswd3::world_map::legacy_world_head_sign_action_token(0U) &&
            post_head_sign_dialog.status == LegacyWorldStoryVmStatus::yielded &&
            roles[8].field_3c == 0U &&
            post_head_sign_dialog_releases_completed &&
            third_path_frames_completed && third_path_wait_completed &&
            third_path_dialog_releases_completed &&
            fourth_path_frames_completed &&
            post_opcode_45_progression_completed && battle_request_submitted &&
            dialogs.messages.size() == 48U &&
            (roles[9].flags & 0x00008000U) != 0U &&
            (roles[10].flags & 0x00008000U) != 0U &&
            roles[9].action.base_variant == 33U &&
            roles[10].action.base_variant == 0U &&
            roles[9].action.wait_override == 0x8002U &&
            roles[10].action.wait_override == 0x8000U &&
            roles[9].action.wait_remaining == 0U &&
            roles[10].action.wait_remaining == 0U && roles[10].field_3c == 0U,
        "real story 100 crosses opcode 45 and all subsequent restored waits, " "dialogs, paths, color transition control, role-path release, primary " "picture enqueue and explicit text layout through the opcode 88 battle " "request"
    );
    test.expect_true(
        ports.sound_effect_requests == std::vector<u16>{0x73U, 0x3CU},
        "real story 100 submits both scripted opcode 59 sound ids"
    );
    test.expect_equal(
        roles[6].world_x, u32{37U * 16U}, "real story 100 relocates role 248 x"
    );
    test.expect_equal(
        roles[6].world_y, u32{33U * 16U}, "real story 100 relocates role 248 y"
    );
    test.expect_equal(
        roles[7].world_x, u32{39U * 16U}, "real story 100 relocates role 249 x"
    );
    test.expect_equal(
        roles[7].world_y, u32{33U * 16U}, "real story 100 relocates role 249 y"
    );
    test.expect_equal(
        roles[1].world_x,
        u32{13U * 16U},
        "real story 100 completes first clear GUID 1 path x"
    );
    test.expect_equal(
        roles[1].world_y,
        u32{28U * 16U},
        "real story 100 completes first clear GUID 1 path y"
    );
    test.expect_equal(
        roles[1].action.base_variant,
        u32{68U},
        "real story 100 applies the later opcode 10 base variant"
    );
    test.expect_equal(
        path_final_facing,
        u32{7U},
        "real story 100 applies the final opcode 11 facing"
    );
    test.expect_equal(
        roles[5].world_x, u32{16U * 16U}, "real story 100 relocates role 195 x"
    );
    test.expect_equal(
        roles[5].world_y, u32{36U * 16U}, "real story 100 relocates role 195 y"
    );
    test.expect_true(
        picture_actions.primary.size() == 1U &&
            picture_actions.secondary.size() == 2U &&
            frame_color.current_red == 10.0F &&
            frame_color.current_green == 10.0F &&
            frame_color.current_blue == 10.0F &&
            frame_color.target_red == 10.0F &&
            frame_color.target_green == 10.0F &&
            frame_color.target_blue == 10.0F && frame_color.step_red == 0.0F &&
            frame_color.step_green == 0.0F && frame_color.step_blue == 0.0F &&
            frame_color.countdown == 0 && scene_render_flags == 0U &&
            (roles[1].flags & 0x00001000U) != 0U,
        "real story 100 creates two pictures then completes and cancels the " "later color transition"
    );
    test.expect_true(
        ports.framebuffer_clear_count == 3U &&
            ports.framebuffer_present_count == 1U &&
            ports.video_begin_count == 1U &&
            ports.video_progress_query_count == 1U &&
            video_filename == "OPENING.bik",
        "real story 100 preserves its framebuffer and video side effects"
    );
}

}  // namespace

int main(const int argument_count, char** arguments) {
    openswd3::test::Context test;
    test_default_invalid_opcode_protocol(test);
    test_shared_dialog_handler_variants(test);
    test_shared_dialog_raw_aliases(test);
    test_clear_dialog_control_flag(test);
    test_clear_dialog_control_flag_bit30(test);
    test_stage_dialog_lifetime(test);
    test_dialog_text_preparation_and_mode_zero_metrics(test);
    test_dialog_anchor_center_delay_and_reset(test);
    test_dialog_checked_failure_order(test);
    test_initial_flags_and_alignment_gate(test);
    test_reinitialization_writes_only_owned_vm_fields(test);
    test_dialog_enqueue_and_wait_protocol(test);
    test_dialog_role_overlap_avoidance(test);
    test_dialog_explicit_layout_pair(test);
    test_transfer_flags_and_terminal_cleanup(test);
    test_same_file_branch(test);
    test_role_action_operand_extension(test);
    test_missing_role_position_patch(test);
    test_change_role_base_variant_protocol(test);
    test_change_role_variant_delta_protocol(test);
    test_set_role_position_protocol(test);
    test_step_role_protocol(test);
    test_wait_role_action_status_protocol(test);
    test_jump_same_file_offset_protocol(test);
    test_jump_if_role_path_unprepared_protocol(test);
    test_jump_if_role_path_prepared_protocol(test);
    test_role_action_chain_update_gate(test);
    test_change_requested_action_id(test);
    test_change_requested_action_id_failure_ordering(test);
    test_restore_role_action_overrides_protocol(test);
    test_start_camera_move_protocol(test);
    test_start_camera_move_failure_ordering(test);
    test_start_camera_move_window_boundaries(test);
    test_wait_for_camera_move_protocol(test);
    test_start_frame_color_transition_protocol(test);
    test_start_frame_color_transition_window_boundaries(test);
    test_wait_for_frame_color_transition_protocol(test);
    test_repeat_role_action_refresh_protocol(test);
    test_shared_role_spatial_group_protocol(test);
    test_wait_for_role_action_position(test);
    test_release_role_path_protocol(test);
    test_release_all_role_paths_protocol(test);
    test_schedule_role_paths_protocol(test);
    test_jump_if_global_bit_protocol(test);
    test_jump_if_all_global_bits_set_protocol(test);
    test_jump_if_any_global_bit_set_protocol(test);
    test_set_global_bit_protocol(test);
    test_clear_global_bit_protocol(test);
    test_reload_world_session_protocol(test);
    test_change_role_path_id_protocol(test);
    test_global_integer_protocol(test);
    test_set_bounded_script_clock_protocol(test);
    test_jump_if_byte_le_script_clock_protocol(test);
    test_jump_if_script_clock_exceeds_origin_delta_protocol(test);
    test_snapshot_script_clock_protocol(test);
    test_clear_role_from_scene_protocol(test);
    test_set_role_flag_8000_and_clear_one_shots_protocol(test);
    test_relocate_role_and_complete_path_protocol(test);
    test_reload_indexed_target_protocol(test);
    test_interaction_lock_protocol(test);
    test_set_role_action_wait_override_protocol(test);
    test_shared_picture_action_enqueue_protocol(test);
    test_request_battle_after_clearing_overlay_lists(test);
    test_play_sound_effect_request(test);
    test_wait_for_frame_color_transition(test);
    test_turn_role_toward_role(test);
    test_set_role_head_sign_action(test);
    test_set_and_clear_role_wait_override(test);
    if (argument_count == 3 &&
        std::string_view{arguments[2]} == "initial-session") {
        test_real_new_game_story_patches_unloaded_role(
            test, std::filesystem::path{arguments[1]}
        );
    } else if (argument_count == 2) {
        const std::filesystem::path root{arguments[1]};
        test_real_clear_dialog_control_flag_record(test, root);
        test_real_change_role_base_variant_record(test, root);
        test_real_change_role_variant_delta_record(test, root);
        test_real_clear_dialog_control_flag_bit30_record(test, root);
        test_real_stage_dialog_lifetime_record(test, root);
        test_real_wait_role_action_status_record(test, root);
        test_real_jump_same_file_offset_record(test, root);
        test_real_jump_if_role_path_unprepared_record(test, root);
        test_real_jump_if_role_path_prepared_record(test, root);
        test_real_release_role_path_record(test, root);
        test_real_release_all_role_paths_record(test, root);
        test_real_schedule_role_path_records(test, root);
        test_real_jump_if_global_bit_records(test, root);
        test_real_jump_if_all_global_bits_set_record(test, root);
        test_real_set_global_bit_record(test, root);
        test_real_clear_global_bit_record(test, root);
        test_real_reload_world_session_record(test, root);
        test_real_change_role_path_id_record(test, root);
        test_real_global_integer_records(test, root);
        test_real_relocate_role_and_complete_path_record(test, root);
        test_real_reload_indexed_target_record(test, root);
        test_real_interaction_lock_records(test, root);
        test_real_set_role_action_wait_override_record(test, root);
        test_real_set_role_action_id_record(test, root);
        test_real_camera_move_records(test, root);
        test_real_wait_for_camera_move_record(test, root);
        test_real_start_frame_color_transition_record(test, root);
        test_real_wait_for_frame_color_transition_record(test, root);
        test_real_repeat_role_action_refresh_record(test, root);
        test_real_shared_role_spatial_group_records(test, root);
        test_real_shared_picture_action_enqueue_records(test, root);
        test_real_set_role_flag_8000_and_clear_one_shots_record(test, root);
        test_real_clear_role_from_scene_record(test, root);
        test_real_shared_dialog_handler_records(test, root);
        test_real_story_248_dialog(test, root);
        test_real_new_game_story_reaches_first_dialog(test, root);
    }
    return test.exit_code();
}
