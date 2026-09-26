#pragma once

#include "openswd3/battle/legacy_battle_actor_runtime_reset.hpp"
#include "openswd3/compat/types.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <span>

namespace openswd3::asset_runtime {
struct LegacyActionRecord;
}

namespace openswd3::rendering {
struct LegacyRasterGeometryState;
struct LegacyScaledRleTransform;
struct LegacyPixelConversionState;
}

namespace openswd3::battle {
struct LegacyBattleDirectionVectors;
struct LegacyBattleDirectionalSurface;
struct LegacyBattleDirectionalScanSharedState;
}

namespace openswd3::battle {

inline constexpr compat::u32 kLegacyBattleActorFramePresentationAddress =
    0x00479850U;

enum class LegacyBattleActorFrameEntryStatus : compat::u16 {
    update_ready,
    update_frame_read_ready,
    update_post_lookup_ready,
    update_resource_byte_read_ready,
    update_selector_dec_ready,
    update_selector_case_ready,
    case_one_active_ready,
    case_one_audio_call_ready,
    case_one_audio_child_typed_stop,
    case_one_source_token_ready,
    case_one_source_read_ready,
    case_one_global_write_ready,
    case_one_height_ready,
    case_one_draw_args_ready,
    case_one_draw_call_ready,
    case_one_draw_child_typed_stop,
    case_one_draw_arguments_unbacked,
    case_one_draw_return_ready,
    case_one_stack_cleanup_ready,
    case_one_draw_returned,
    case_two_release_ready,
    case_two_particle_init_ready,
    case_two_decoder_prepare_ready,
    case_two_decoder_call_ready,
    case_two_decoder_source_read_typed_stop,
    case_two_decoder_child_typed_stop,
    case_two_decoder_arguments_unbacked,
    case_two_decoder_token_write_ready,
    case_two_post_decoder_frame_read_ready,
    case_two_geometry_ready,
    case_two_emitter_flags_ready,
    case_two_property_call_ready,
    case_two_property_child_read_typed_stop,
    case_two_metrics_iat_read_ready,
    case_two_metrics_iat_read_typed_stop,
    case_two_metrics_child_typed_stop,
    case_two_rectangle_call_ready,
    case_two_rectangle_child_typed_stop,
    case_two_sample_code_write_ready,
    case_two_sample_call_ready,
    case_two_sample_phase_write_ready,
    case_two_audio_child_typed_stop,
    case_two_frame_resource_read_typed_stop,
    case_two_particle_tail_ready,
    case_two_particle_call_ready,
    case_two_particle_child_typed_stop,
    case_two_particle_reply_unbacked,
    case_two_particle_phase_100_write_ready,
    case_two_particle_returned,
    case_two_particle_global_read_typed_stop,
    case_nine_active_ready,
    case_nine_source_ready,
    case_nine_audio_call_ready,
    case_nine_audio_child_typed_stop,
    case_nine_draw_flags_ready,
    case_nine_draw_call_ready,
    case_nine_draw_child_typed_stop,
    case_nine_draw_globals_ready,
    case_nine_draw_arguments_unbacked,
    case_nine_draw_resource_read_typed_stop,
    case_nine_source_resource_read_typed_stop,
    case_eight_particle_init_ready,
    case_eight_decoder_prepare_ready,
    case_eight_decoder_call_ready,
    case_eight_decoder_source_read_typed_stop,
    case_eight_decoder_child_typed_stop,
    case_eight_decoder_arguments_unbacked,
    case_eight_decoder_token_write_ready,
    case_eight_post_decoder_frame_read_ready,
    case_eight_frame_resource_read_typed_stop,
    case_eight_geometry_ready,
    case_eight_geometry_value_ready,
    case_eight_property_call_ready,
    case_eight_property_child_read_typed_stop,
    case_eight_metrics_iat_read_ready,
    case_eight_metrics_iat_read_typed_stop,
    case_eight_metrics_child_typed_stop,
    case_eight_rectangle_call_ready,
    case_eight_rectangle_child_typed_stop,
    case_eight_sample_handle_read_ready,
    case_eight_sample_call_ready,
    case_eight_audio_child_typed_stop,
    case_eight_sample_phase_write_ready,
    case_six_active_ready,
    case_six_audio_call_ready,
    case_six_audio_child_typed_stop,
    case_six_source_ready,
    case_six_source_resource_read_typed_stop,
    case_six_draw_parameters_ready,
    case_six_draw_resource_read_typed_stop,
    case_six_draw_arguments_unbacked,
    case_six_draw_call_ready,
    case_six_draw_child_typed_stop,
    case_six_phase_decrement_ready,
    case_six_returned,
    case_seven_audio_ready,
    case_seven_audio_call_ready,
    case_seven_audio_child_typed_stop,
    case_seven_source_ready,
    case_seven_resource_read_typed_stop,
    case_seven_initial_source_ready,
    case_seven_geometry_resource_read_typed_stop,
    case_seven_rectangle_call_ready,
    case_seven_rectangle_child_typed_stop,
    case_seven_rectangle_return_ready,
    case_seven_post_rectangle_globals_ready,
    case_seven_draw_resource_read_typed_stop,
    case_seven_draw_call_ready,
    case_seven_draw_child_typed_stop,
    case_seven_draw_return_ready,
    case_seven_second_rectangle_call_ready,
    case_seven_second_rectangle_child_typed_stop,
    case_seven_second_rectangle_return_ready,
    case_seven_second_post_rectangle_globals_ready,
    case_seven_second_draw_call_ready,
    case_seven_second_draw_child_typed_stop,
    case_seven_second_draw_return_ready,
    case_seven_third_rectangle_call_ready,
    case_seven_third_rectangle_child_typed_stop,
    case_seven_third_rectangle_return_ready,
    case_seven_third_post_rectangle_globals_ready,
    case_seven_third_draw_call_ready,
    case_seven_third_draw_child_typed_stop,
    case_seven_third_draw_return_ready,
    case_seven_fourth_rectangle_call_ready,
    case_seven_fourth_rectangle_child_typed_stop,
    case_seven_fourth_rectangle_return_ready,
    case_seven_fourth_post_rectangle_globals_ready,
    case_seven_fourth_draw_call_ready,
    case_seven_fourth_draw_child_typed_stop,
    case_seven_fourth_draw_return_ready,
    case_seven_shared_rectangle_call_ready,
    case_seven_shared_rectangle_child_typed_stop,
    case_seven_shared_rectangle_return_ready,
    case_seven_active_returned,
    case_three_post_rectangle_globals_ready,
    case_four_post_rectangle_globals_ready,
    case_three_first_draw_call_ready,
    case_three_first_draw_child_typed_stop,
    case_three_first_draw_return_ready,
    case_four_first_draw_call_ready,
    case_four_first_draw_child_typed_stop,
    case_four_first_draw_return_ready,
    case_three_second_rectangle_call_ready,
    case_three_second_rectangle_child_typed_stop,
    case_three_second_rectangle_return_ready,
    case_four_second_rectangle_call_ready,
    case_four_second_rectangle_child_typed_stop,
    case_four_second_rectangle_return_ready,
    case_three_second_post_rectangle_globals_ready,
    case_four_second_post_rectangle_globals_ready,
    case_three_second_draw_call_ready,
    case_three_second_draw_child_typed_stop,
    case_three_second_draw_return_ready,
    case_four_second_draw_call_ready,
    case_four_second_draw_child_typed_stop,
    case_four_second_draw_return_ready,
    case_three_four_shared_rectangle_call_ready,
    case_three_four_shared_rectangle_child_typed_stop,
    case_three_four_shared_rectangle_return_ready,
    case_three_four_active_returned,
    case_thirteen_audio_ready,
    case_thirteen_source_ready,
    case_thirteen_audio_call_ready,
    case_thirteen_audio_child_typed_stop,
    case_thirteen_initial_source_ready,
    case_thirteen_first_rectangle_call_ready,
    case_thirteen_first_rectangle_child_typed_stop,
    case_thirteen_first_rectangle_return_ready,
    case_thirteen_first_post_rectangle_globals_ready,
    case_thirteen_first_draw_call_ready,
    case_thirteen_first_draw_child_typed_stop,
    case_thirteen_first_draw_return_ready,
    case_thirteen_second_rectangle_call_ready,
    case_thirteen_second_rectangle_child_typed_stop,
    case_thirteen_second_rectangle_return_ready,
    case_thirteen_second_post_rectangle_globals_ready,
    case_thirteen_second_draw_call_ready,
    case_thirteen_second_draw_child_typed_stop,
    case_thirteen_second_draw_return_ready,
    case_thirteen_third_rectangle_call_ready,
    case_thirteen_third_rectangle_child_typed_stop,
    case_thirteen_third_rectangle_return_ready,
    case_thirteen_third_post_rectangle_globals_ready,
    case_thirteen_third_draw_call_ready,
    case_thirteen_third_draw_child_typed_stop,
    case_thirteen_third_draw_return_ready,
    case_thirteen_fourth_rectangle_call_ready,
    case_thirteen_fourth_rectangle_child_typed_stop,
    case_thirteen_fourth_rectangle_return_ready,
    case_thirteen_fourth_post_rectangle_globals_ready,
    case_thirteen_fourth_draw_call_ready,
    case_thirteen_fourth_draw_child_typed_stop,
    case_thirteen_fourth_draw_return_ready,
    case_thirteen_active_returned,
    case_fourteen_audio_ready,
    case_fourteen_audio_call_ready,
    case_fourteen_audio_child_typed_stop,
    case_fourteen_source_ready,
    case_fourteen_early_geometry_ready,
    case_fourteen_late_geometry_ready,
    case_fourteen_early_first_rectangle_call_ready,
    case_fourteen_early_first_rectangle_child_typed_stop,
    case_fourteen_early_first_rectangle_return_ready,
    case_fourteen_early_first_globals_ready,
    case_fourteen_early_first_draw_call_ready,
    case_fourteen_early_first_draw_child_typed_stop,
    case_fourteen_early_first_draw_return_ready,
    case_fourteen_early_second_rectangle_call_ready,
    case_fourteen_early_second_rectangle_child_typed_stop,
    case_fourteen_early_second_rectangle_return_ready,
    case_fourteen_early_second_globals_ready,
    case_fourteen_early_second_draw_call_ready,
    case_fourteen_early_second_draw_child_typed_stop,
    case_fourteen_early_second_draw_return_ready,
    case_fourteen_shared_rectangle_arguments_ready,
    case_fourteen_shared_rectangle_call_ready,
    case_fourteen_shared_rectangle_child_typed_stop,
    case_fourteen_shared_rectangle_return_ready,
    case_fourteen_active_returned,
    case_fourteen_late_first_rectangle_call_ready,
    case_fourteen_late_first_rectangle_child_typed_stop,
    case_fourteen_late_first_rectangle_return_ready,
    case_fourteen_late_first_globals_ready,
    case_fourteen_late_first_draw_call_ready,
    case_fourteen_late_first_draw_child_typed_stop,
    case_fourteen_late_first_draw_return_ready,
    case_fourteen_late_second_rectangle_call_ready,
    case_fourteen_late_second_rectangle_child_typed_stop,
    case_fourteen_late_second_rectangle_return_ready,
    case_fourteen_late_second_globals_ready,
    case_fourteen_late_second_draw_call_ready,
    case_fourteen_late_second_draw_child_typed_stop,
    case_fourteen_late_second_draw_return_ready,
    case_fourteen_progress_reset_ready,
    case_hundred_terminal_reset_ready,
    case_hundred_release_call_ready,
    case_hundred_release_child_typed_stop,
    case_hundred_reset_prefix_ready,
    case_hundred_reset_call_ready,
    case_hundred_reset_returned,
    case_hundred_count_short_reset_ready,
    case_hundred_short_return_ready,
    case_hundred_short_returned,
    case_hundred_audio_ready,
    case_hundred_audio_call_ready,
    case_hundred_audio_child_typed_stop,
    case_hundred_source_ready,
    case_hundred_source_resource_read_typed_stop,
    case_hundred_draw_arguments_ready,
    case_hundred_draw_resource_read_typed_stop,
    case_hundred_draw_call_ready,
    case_hundred_draw_child_typed_stop,
    case_hundred_draw_return_ready,
    case_hundred_count_increment_ready,
    case_hundred_increment_return_ready,
    case_hundred_increment_returned,
    case_hundred_particle_gate_ready,
    case_hundred_particle_init_ready,
    case_hundred_particle_existing_ready,
    case_hundred_particle_decoder_arguments_ready,
    case_hundred_decoder_source_read_typed_stop,
    case_hundred_decoder_call_ready,
    case_hundred_decoder_arguments_unbacked,
    case_hundred_decoder_child_typed_stop,
    case_hundred_decoder_token_write_ready,
    case_hundred_post_decoder_source_ready,
    case_hundred_decoder_resource_read_typed_stop,
    case_hundred_geometry_ready,
    case_hundred_geometry_gate_ready,
    case_hundred_configuration_resource_read_typed_stop,
    case_hundred_attribute_call_ready,
    case_hundred_attribute_child_read_typed_stop,
    case_hundred_metrics_iat_read_ready,
    case_hundred_metrics_iat_read_typed_stop,
    case_hundred_metrics_first_call_ready,
    case_hundred_metrics_child_typed_stop,
    case_hundred_rectangle_call_ready,
    case_hundred_rectangle_child_typed_stop,
    case_hundred_post_rectangle_sample_ready,
    case_hundred_post_rectangle_audio_call_ready,
    case_hundred_phase_write_ready,
    case_hundred_particle_tail_ready,
    case_eleven_active_ready,
    case_eleven_audio_call_ready,
    case_eleven_audio_child_typed_stop,
    case_eleven_source_ready,
    case_eleven_source_resource_read_typed_stop,
    case_eleven_first_draw_arguments_ready,
    case_eleven_first_draw_resource_read_typed_stop,
    case_eleven_first_draw_call_ready,
    case_eleven_first_draw_child_typed_stop,
    case_eleven_first_draw_return_ready,
    case_eleven_second_draw_arguments_ready,
    case_eleven_second_draw_resource_read_typed_stop,
    case_eleven_second_draw_call_ready,
    case_eleven_second_draw_child_typed_stop,
    case_eleven_phase_decrement_ready,
    case_eleven_returned,
    case_fifteen_active_ready,
    case_fifteen_audio_call_ready,
    case_fifteen_audio_child_typed_stop,
    case_fifteen_source_ready,
    case_fifteen_source_resource_read_typed_stop,
    case_fifteen_first_draw_arguments_ready,
    case_fifteen_first_draw_resource_read_typed_stop,
    case_fifteen_first_draw_call_ready,
    case_fifteen_first_draw_child_typed_stop,
    case_fifteen_first_draw_return_ready,
    case_fifteen_second_draw_arguments_ready,
    case_fifteen_second_draw_resource_read_typed_stop,
    case_fifteen_second_draw_call_ready,
    case_fifteen_second_draw_child_typed_stop,
    case_fifteen_phase_decrement_ready,
    case_fifteen_returned,
    case_fifty_active_ready,
    case_fifty_audio_call_ready,
    case_fifty_audio_child_typed_stop,
    case_fifty_source_ready,
    case_fifty_source_resource_read_typed_stop,
    case_fifty_draw_arguments_ready,
    case_fifty_draw_resource_read_typed_stop,
    case_fifty_table_read_typed_stop,
    case_fifty_draw_call_ready,
    case_fifty_draw_child_typed_stop,
    case_fifty_phase_increment_ready,
    case_fifty_returned,
    case_fifty_one_init_ready,
    case_fifty_one_geometry_ready,
    case_fifty_one_resource_read_typed_stop,
    case_fifty_one_geometry_resource_typed_stop,
    case_fifty_one_particle_call_ready,
    case_fifty_one_property_child_read_typed_stop,
    case_fifty_one_decoder_prepare_ready,
    case_fifty_one_decoder_call_ready,
    case_fifty_one_decoder_source_read_typed_stop,
    case_fifty_one_decoder_child_typed_stop,
    case_fifty_one_decoder_arguments_unbacked,
    case_fifty_one_decoder_token_write_ready,
    case_fifty_one_audio_call_ready,
    case_fifty_one_audio_child_typed_stop,
    case_fifty_one_tail_ready,
    case_fifty_one_global_read_typed_stop,
    case_fifty_one_spawn_call_ready,
    case_fifty_one_spawn_child_typed_stop,
    case_fifty_one_spawn_returned,
    case_fifty_one_reset_ready,
    case_fifty_one_reset_call_ready,
    case_fifty_one_reset_returned,
    case_three_audio_ready,
    case_three_audio_call_ready,
    case_three_audio_child_typed_stop,
    case_three_source_ready,
    case_three_initial_source_ready,
    case_three_rectangle_call_ready,
    case_three_rectangle_child_typed_stop,
    case_three_rectangle_return_ready,
    case_three_resource_read_typed_stop,
    case_three_geometry_resource_read_typed_stop,
    case_four_audio_ready,
    case_four_audio_call_ready,
    case_four_audio_child_typed_stop,
    case_four_source_ready,
    case_four_initial_source_ready,
    case_four_rectangle_call_ready,
    case_four_rectangle_child_typed_stop,
    case_four_rectangle_return_ready,
    case_four_resource_read_typed_stop,
    case_four_geometry_resource_read_typed_stop,
    case_five_audio_ready,
    case_five_audio_call_ready,
    case_five_audio_child_typed_stop,
    case_five_source_ready,
    case_five_source_base_ready,
    case_five_draw_call_ready,
    case_five_draw_child_typed_stop,
    case_five_after_first_draw_ready,
    case_five_second_draw_prepare_ready,
    case_five_second_draw_globals_ready,
    case_five_second_draw_call_ready,
    case_five_second_draw_child_typed_stop,
    case_five_clip_arguments_ready,
    case_five_clip_call_ready,
    case_five_clip_child_typed_stop,
    case_five_second_width_resource_read_typed_stop,
    case_five_height_resource_read_typed_stop,
    case_five_draw_resource_read_typed_stop,
    case_five_resource_read_typed_stop,
    case_ten_audio_ready,
    case_ten_audio_call_ready,
    case_ten_audio_child_typed_stop,
    case_ten_source_ready,
    case_ten_source_base_ready,
    case_ten_draw_call_ready,
    case_ten_draw_child_typed_stop,
    case_ten_after_first_draw_ready,
    case_ten_second_draw_prepare_ready,
    case_ten_second_draw_globals_ready,
    case_ten_second_draw_call_ready,
    case_ten_second_draw_child_typed_stop,
    case_ten_clip_arguments_ready,
    case_ten_clip_call_ready,
    case_ten_clip_child_typed_stop,
    case_ten_second_width_resource_read_typed_stop,
    case_ten_height_resource_read_typed_stop,
    case_ten_draw_resource_read_typed_stop,
    case_ten_resource_read_typed_stop,
    case_five_ten_terminal_return_ready,
    case_five_ten_terminal_returned,
    case_five_ten_clip_return_ready,
    case_five_ten_active_returned,
    case_twelve_audio_ready,
    case_twelve_audio_call_ready,
    case_twelve_audio_child_typed_stop,
    case_twelve_globals_ready,
    case_twelve_global_values_ready,
    case_twelve_forward_args_ready,
    case_twelve_reverse_args_ready,
    case_twelve_source_resource_read_typed_stop,
    case_twelve_raster_resource_read_typed_stop,
    case_twelve_raster_call_ready,
    case_twelve_raster_child_typed_stop,
    case_twelve_raster_return_ready,
    case_twelve_stack_cleanup_ready,
    case_twelve_active_returned,
    case_twelve_reset_tail_ready,
    case_twelve_reset_call_ready,
    case_twelve_reset_returned,
    case_reset_progress_write_ready,
    case_two_release_call_ready,
    case_two_emitter_clear_ready,
    case_two_release_child_typed_stop,
    case_two_emitter_reset_ready,
    case_reset_call_ready,
    case_reset_returned,
    update_selector_default_ready,
    selector_table_read_typed_stop,
    update_frame_resource_read_typed_stop,
    frame_resource_read_typed_stop,
    actor_resource_read_typed_stop,
    global_read_typed_stop,
    global_write_typed_stop,
    case_one_source_read_typed_stop,
    case_one_height_resource_read_typed_stop,
    case_one_draw_resource_read_typed_stop,
    update_returned,
    update_child_typed_stop,
    update_frame_lookup_typed_stop,
    reset_call_ready,
    reset_release_call_ready,
    linked_node_read_ready,
    linked_node_read_typed_stop,
    linked_node_release_child_typed_stop,
    reset_returned,
    reset_child_typed_stop,
    default_returned,
    actor_read_typed_stop,
    actor_write_typed_stop,
    nested_record_read_typed_stop,
    nested_record_write_typed_stop,
    stack_write_typed_stop,
    stack_read_typed_stop,
    allocator_debug_break_typed_stop,
    allocator_block_write_typed_stop,
};

enum class LegacyBattleActorFrameEntryAccessKind : compat::u8 {
    actor_read,
    actor_write,
    nested_record_read,
    nested_record_write,
    frame_resource_read,
    actor_resource_read,
    global_read,
    global_write,
    selector_byte_table_read,
    selector_jump_table_read,
    linked_node_read,
    surface_row_read,
    surface_pixel_read,
    surface_pixel_write,
    stack_write,
    stack_read,
    callee_call,
    debug_break,
    allocator_block_write,
};

struct LegacyBattleActorFrameParentArgumentWord;

struct LegacyBattleActorFrameDecoderSource {
    compat::u32 token{};
    std::span<const compat::u8> bytes{};
};

struct LegacyBattleActorFrameEntryRequest {
    compat::u32 actor_token{};
    compat::u32 entry_eax{};
    compat::u32 entry_edx{};
    compat::u32 entry_ebx{};
    compat::u32 entry_ebp{};
    compat::u32 entry_esi{};
    compat::u32 entry_edi{};
    compat::u32 entry_esp{};
    compat::u32 entry_return_address{};
    LegacyBattleActorCoordinateFlags entry_flags{};
    bool entry_flags_known{};
    bool direction_flag{};
    bool actor_readable{true};
    bool actor_writable{true};
    bool nested_record_readable{true};
    bool nested_record_writable{true};
    bool actor_resource_readable{true};
    bool linked_node_readable{true};
    const compat::u32* audio_state_mode_owner{};  // [0x004C8450+0x54]
    bool audio_state_readable{true};
    const compat::u32* audio_state_submode_owner{};  // [0x004C8450+0x58]
    bool audio_state_submode_readable{true};
    bool global_readable{true};
    bool global_writable{true};
    const compat::u32* decoder_header_marker_owner{};  // 0x004CDE74
    std::span<const LegacyBattleActorFrameDecoderSource> decoder_sources{};
    bool decoder_source_readable{true};
    std::array<LegacyBattleActorFrameParentArgumentWord*, 3U>
        decoder_output_owners{};
    const compat::u32* decoder_allocator_global_owner{};      // 0x0053D1B4
    const compat::u32* decoder_heap_debug_flags_owner{};      // 0x004A82F4
    const compat::u32* decoder_heap_request_counter_owner{};  // 0x004A82F8
    compat::u32* decoder_heap_request_counter_write_owner{};  // same address
    const compat::u32* decoder_heap_break_counter_owner{};    // 0x004A82FC
    const compat::u32* decoder_heap_alloc_owner{};            // 0x004A8360
    const compat::u32* decoder_small_block_limit_owner{};     // 0x004A8390
    const compat::u32* decoder_small_pool_index_owner{};      // 0x0053E7B4
    const compat::u32* decoder_small_pool_base_owner{};       // 0x0053E7B8
    bool decoder_small_pool_return_known{};
    compat::u32 decoder_small_pool_return_eax{};
    compat::u32 decoder_small_pool_return_ecx{};
    compat::u32 decoder_small_pool_return_edx{};
    LegacyBattleActorCoordinateFlags decoder_small_pool_return_flags{};
    bool decoder_small_pool_return_flags_known{};
    compat::u32 decoder_heap_block_token{};
    std::span<compat::u8> decoder_heap_block_bytes{};
    bool decoder_heap_block_writable{true};
    const compat::u8* decoder_heap_guard_byte_owner{};
    bool decoder_first_heap_fill_child_stack_backed{};
    bool decoder_first_heap_fill_write_backed{};
    bool decoder_second_heap_fill_child_stack_backed{};
    bool decoder_second_heap_fill_write_backed{};
    const compat::u8* decoder_heap_payload_byte_owner{};  // 0x004A8302
    bool decoder_payload_heap_fill_child_stack_backed{};
    const compat::u32* decoder_heap_stats_size_owner{};
    compat::u32* decoder_heap_stats_size_write_owner{};
    const compat::u32* decoder_heap_stats_live_size_owner{};
    compat::u32* decoder_heap_stats_live_size_write_owner{};
    const compat::u32* decoder_heap_stats_peak_size_owner{};
    compat::u32* decoder_heap_stats_peak_size_write_owner{};
    const compat::u32* decoder_heap_tail_owner{};
    compat::u32* decoder_heap_tail_write_owner{};
    compat::u32* decoder_heap_head_write_owner{};
    compat::u32 decoder_heap_old_tail_block_token{};
    std::span<compat::u8> decoder_heap_old_tail_block_bytes{};
    bool decoder_heap_old_tail_block_writable{true};
    const compat::u32* decoder_win32_heap_owner{};   // 0x0053E7BC
    const compat::u32* decoder_win32_alloc_owner{};  // 0x00499198
    const compat::u32* draw_source_token_owner{};    // 0x004CD730
    const compat::u32* draw_palette_token_owner{};   // 0x004CD764
    const compat::u32* draw_height_third_owner{};    // 0x004CD75C
    LegacyBattleActorFrameParentArgumentWord* draw_argument_10_owner{};
    compat::u32 draw_source_bytes_token{};
    std::span<const compat::u8> draw_source_bytes{};
    bool draw_source_readable{true};
    bool selector_byte_table_readable{true};
    bool selector_jump_table_readable{true};
    bool return_address_readable{true};
    bool release_return_address_readable{true};
    bool call_stack_writable{true};
    bool stack_readable{true};
    bool system_metrics_iat_known{};
    compat::u32 system_metrics_function_token{};
    bool particle_global_4cd76c_known{};
    compat::u32 particle_global_4cd76c{};
    bool reset_random_callable{true};
    compat::u32 reset_random_return_ecx{};
    rendering::LegacyScaledRleTransform* scaled_rle_transform{};
    std::size_t stop_before_access{std::numeric_limits<std::size_t>::max()};
};

struct LegacyBattleActorFrameUpdateReply {
    bool returned{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    compat::u32 stopped_instruction{};
    // The resource loader may publish source record bytes separately from
    // its pointer return. Without this explicit owner, nested reads stop.
    bool resource_header_known{};
    compat::u32 resource_value_00{};
    compat::u32 resource_value_04{};
    compat::u16 resource_value_0c{};
    compat::u16 resource_value_0e{};
};

struct LegacyBattleActorFrameDecodeReply {
    bool returned{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    std::span<compat::u16> source_pixels{};
    bool source_pixels_known{};
};

class LegacyBattleActorFrameDecodePort {
public:
    virtual ~LegacyBattleActorFrameDecodePort() = default;
    // The registers/flags are the current state after the typed callee prefix,
    // not the original CALL entry. returned=false stops at that suffix boundary.
    [[nodiscard]] virtual LegacyBattleActorFrameDecodeReply decode(
        const std::array<compat::u32, 4U>& stack_arguments,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

struct LegacyBattleActorFrameMetricsReply {
    bool returned{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
};

class LegacyBattleActorFrameRectanglePort {
public:
    virtual ~LegacyBattleActorFrameRectanglePort() = default;
    // returned=true is permitted only after the host-surface owner and
    // nested callees completed; returned=false denotes entry-only stop.
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply set_host_surface(
        compat::u32 width,
        compat::u32 height,
        compat::u32 host_token,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameParticlePort {
public:
    virtual ~LegacyBattleActorFrameParticlePort() = default;
    // This narrow boundary requires a canonical emitter owner. A normal
    // completed sub_434790 reply is restricted to EAX 0 or 1; an entry stop
    // must not claim writes from inside the nested particle implementation.
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply update_particles(
        LegacyBattleTargetPhaseState& phase,
        compat::u32 global_argument,
        compat::u32 emitter_token,
        compat::u32 host_token,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameMetricsPort {
public:
    virtual ~LegacyBattleActorFrameMetricsPort() = default;
    [[nodiscard]] virtual LegacyBattleActorFrameMetricsReply get_system_metrics(
        compat::u32 index,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameSoundPort {
public:
    virtual ~LegacyBattleActorFrameSoundPort() = default;
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply play_sample(
        compat::u32 sample_id,
        compat::u32 sample_handle,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

// sub_47E950 node: the first dword is the next pointer. The original
// allocation is 0xAC bytes; fields beyond +0 are not read by this caller.
struct LegacyBattleActorFrameLinkedNode {
    compat::u32 token{};
    compat::u32 next_token{};
};

class LegacyBattleActorFrameLinkedNodeResolverPort {
public:
    virtual ~LegacyBattleActorFrameLinkedNodeResolverPort() = default;
    // Resolve fresh canonical backing before every physical node read.
    // The returned span stays live until the following release call starts.
    [[nodiscard]] virtual std::span<const LegacyBattleActorFrameLinkedNode>
    resolve_linked_nodes(compat::u32 current_token) = 0;
};

class LegacyBattleActorFrameReleasePort {
public:
    virtual ~LegacyBattleActorFrameReleasePort() = default;
    // A false reply denotes an entry-only stop at sub_4885A0, before its
    // first PUSH. Deep allocator/CRT failures require a separate trace and
    // must not be represented as an entry-only reply after side effects.
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply release_emitter(
        compat::u32 token,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameDrawPort {
public:
    virtual ~LegacyBattleActorFrameDrawPort() = default;
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply draw_frame(
        const std::array<compat::u32, 6U>& stack_arguments,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameScaledRlePort {
public:
    virtual ~LegacyBattleActorFrameScaledRlePort() = default;
    // A returned reply requires the canonical source, transform and
    // framebuffer to have completed the selected writer call. A stop may
    // only denote its entry; no deep child state is represented here.
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply draw_scaled_rle(
        bool reverse,
        const std::array<compat::u32, 4U>& stack_arguments,
        compat::u32 source_token,
        const rendering::LegacyScaledRleTransform& transform,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx,
        const LegacyBattleActorCoordinateFlags& entry_flags
    ) = 0;
};

class LegacyBattleActorFrameUpdatePort {
public:
    virtual ~LegacyBattleActorFrameUpdatePort() = default;
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply update(
        asset_runtime::LegacyActionRecord& record,
        compat::u32 record_token,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx
    ) = 0;
    [[nodiscard]] virtual LegacyBattleActorFrameUpdateReply lookup_frame(
        compat::u32 action_value,
        compat::u32 argument_zero,
        compat::u32 entry_eax,
        compat::u32 entry_ecx,
        compat::u32 entry_edx
    ) = 0;
};

struct LegacyBattleActorFrameEntryResult {
    LegacyBattleActorFrameEntryStatus status{
        LegacyBattleActorFrameEntryStatus::update_ready
    };
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    compat::u32 ebx{};
    compat::u32 ebp{};
    compat::u32 esi{};
    compat::u32 edi{};
    compat::u32 esp{};
    compat::u32 eip{kLegacyBattleActorFramePresentationAddress};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
    bool direction_flag{};
    bool returned{};
    std::size_t accesses_completed{};
    LegacyBattleActorFrameEntryAccessKind stopped_access_kind{};
    compat::u32 stopped_instruction{};
    compat::u32 stopped_token{};
    compat::u32 last_pushed_value{};
    compat::u32 draw_auxiliary_value{};
    bool draw_auxiliary_pushed{};
    compat::u32 metric_height_on_stack{};
    compat::u32 metric_width_on_stack{};
    compat::u32 case_three_phase_twice_local{};
    compat::u32 case_four_phase_twice_local{};
    compat::u32 case_thirteen_phase_twice_local{};
    compat::u32 case_thirteen_y_local{};
    compat::u32 case_fourteen_phase_local{};
    bool case_thirteen_shared{};
    compat::u32 particle_global_argument_on_stack{};
    std::array<compat::u32, 5U> draw_argument_pushes{};
    std::size_t draw_argument_count{};
    std::array<compat::u32, 4U> decoder_argument_pushes{};
    std::size_t decoder_argument_count{};
    std::array<compat::u32, 4U> rectangle_argument_pushes{};
    std::size_t rectangle_argument_count{};
    std::array<compat::u32, 4U> scaled_rle_argument_pushes{};
    std::size_t scaled_rle_argument_count{};
    std::size_t reset_calls{};
    std::size_t sample_calls{};
    std::size_t draw_calls{};
    std::size_t decode_calls{};
    std::size_t metrics_calls{};
    std::size_t rectangle_calls{};
    std::size_t particle_calls{};
    std::size_t release_calls{};
    compat::u32 release_saved_ebx{};
    compat::u32 release_saved_ebp{};
    compat::u32 release_saved_esi{};
    compat::u32 release_saved_edi{};
    std::size_t update_calls{};
    std::size_t frame_lookup_calls{};
    LegacyBattleActorFrameUpdateReply update_child{};
    LegacyBattleActorFrameUpdateReply frame_lookup_child{};
    LegacyBattleActorFrameUpdateReply sample_child{};
    LegacyBattleActorFrameUpdateReply draw_child{};
    LegacyBattleActorFrameUpdateReply scaled_rle_child{};
    LegacyBattleActorFrameUpdateReply release_child{};
    LegacyBattleActorFrameDecodeReply decoder_child{};
    LegacyBattleActorFrameMetricsReply metrics_child{};
    LegacyBattleActorFrameUpdateReply rectangle_child{};
    LegacyBattleActorFrameUpdateReply particle_child{};
    LegacyBattleActorRuntimeResetResult reset_child{};
};

// Exact entry/route prefix only: 0x00479850..0x004798F7 or 0x00479920,
// and the default 0x0047A80B..0x0047A814. A nonzero gate yields a pending
// reset call or update continuation, never a completed frame.
[[nodiscard]] LegacyBattleActorFrameEntryResult
enter_legacy_battle_actor_frame_presentation(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request
) noexcept;

struct LegacyBattleActorFrameDirectionalScanOwners;

struct LegacyBattleActorFrameEntryRoutePorts {
    LegacyBattleBoundedRandomPort* random{};
    LegacyBattleActorFrameUpdatePort* updater{};
    LegacyBattleActorFrameLinkedNodeResolverPort* linked_nodes{};
    LegacyBattleActorFrameReleasePort* release{};
    LegacyBattleActorFrameSoundPort* sound{};
    LegacyBattleActorFrameDrawPort* draw{};
    LegacyBattleActorFrameDecodePort* decoder{};
    LegacyBattleActorFrameMetricsPort* metrics{};
    LegacyBattleActorFrameRectanglePort* rectangle{};
    LegacyBattleActorFrameParticlePort* particle{};
    LegacyBattleActorFrameScaledRlePort* scaled_rle{};
    rendering::LegacyRasterGeometryState* clip_raster{};
    const LegacyBattleActorFrameDirectionalScanOwners*
        directional_scan_owners{};
};

enum class LegacyBattleActorFrameCallerSite : compat::u32 {
    action_group_b = 0x004554F6U,
    opponent_group_a = 0x0045650FU,
    final_group_a = 0x0045AA33U,
    final_group_b = 0x0045ACBFU,
};

enum class LegacyBattleActorFrameCallerStatus : compat::u8 {
    call_ready,
    sentinel_skip,
    call_stack_write_typed_stop,
    parent_argument_write_typed_stop,
};

struct LegacyBattleActorFrameCallerAdmission {
    LegacyBattleActorFrameCallerStatus status{
        LegacyBattleActorFrameCallerStatus::call_stack_write_typed_stop
    };
    compat::u32 eip{};
    compat::u32 esp{};
    // Prepared registers/return slot for call_ready or CALL stack-write stop;
    // the return slot is not committed if the CALL write stops.
    LegacyBattleActorFrameEntryRequest child_request{};
};

// Prepare the four physical CALL entries without running a child or a parent
// success tail. caller_snapshot.entry_esp is the parent ESP before CALL;
// unspecified preserved registers and DF come from that snapshot.
[[nodiscard]] LegacyBattleActorFrameCallerAdmission
prepare_legacy_battle_actor_frame_caller(
    LegacyBattleActorFrameCallerSite site,
    compat::u32 index,
    const LegacyBattleActorFrameEntryRequest& caller_snapshot
) noexcept;

enum class LegacyBattleActorFrameCallerRunStatus : compat::u8 {
    sentinel_skip,
    parent_argument_write_typed_stop,
    caller_stack_write_typed_stop,
    child_typed_stop,
    returned,
};

struct LegacyBattleActorFrameCallerRunResult {
    LegacyBattleActorFrameCallerRunStatus status{
        LegacyBattleActorFrameCallerRunStatus::child_typed_stop
    };
    LegacyBattleActorFrameCallerAdmission admission{};
    LegacyBattleActorFrameEntryResult child{};
    compat::u32 eip{};
    compat::u32 esp{};
    compat::u32 eax{};
    compat::u32 edx{};
    bool returned{};
};

struct LegacyBattleActorFrameParentArgumentWord {
    compat::u32 token{};
    compat::u32* word{};
    bool writable{true};
    bool readable{true};
};

struct LegacyBattleActorFrameCallerPhysicalStop {
    compat::u32 eip{};
    compat::u32 esp{};
    compat::u32 token{};
    compat::u32 eax{};
    compat::u32 ecx{};
    compat::u32 edx{};
    LegacyBattleActorCoordinateFlags flags{};
    bool flags_known{};
};

struct LegacyBattleActorFrameCallerBinding {
    // Absence retains the preexisting opaque-port caller for old host tests;
    // it is not evidence that the typed production caller was exercised.
    const LegacyBattleActorFrameEntryRequest* caller_snapshot{};
    const LegacyBattleActorFrameEntryRoutePorts* ports{};
    LegacyBattleActorFrameCallerRunResult* observed{};
    // Required for final_group_a: the physical [parent ESP+0x18] write
    // precedes both the CALL return-slot write and the child entry.
    LegacyBattleActorFrameParentArgumentWord* final_group_a_argument_4{};
    // After final-B child EAX==1: PUSH arg_4 pointer, PUSH arg_0 pointer,
    // then clear arg_0 and arg_4 before sub_475870 enters.
    LegacyBattleActorFrameParentArgumentWord* final_group_b_argument_0{};
    LegacyBattleActorFrameParentArgumentWord* final_group_b_argument_4{};
    bool final_group_b_first_push_writable{true};
    bool final_group_b_second_push_writable{true};
    bool final_group_b_first_output_writable{true};
    bool final_group_b_second_output_writable{true};
    LegacyBattleActorFrameCallerPhysicalStop* final_group_b_stack_stop{};
};

// Requires a real parent ESP/register snapshot; no synthetic stack address
// is inferred. Only a physical child RET to this CALL's suffix may report
// returned=true. Missing canonical owners/ports remain child typed stops.
[[nodiscard]] LegacyBattleActorFrameCallerRunResult
advance_legacy_battle_actor_frame_caller(
    LegacyBattleActorFrameCallerSite site,
    compat::u32 index,
    const LegacyBattleActorRuntimeResetOwners& owners,
    const LegacyBattleActorFrameEntryRequest& caller_snapshot,
    const LegacyBattleActorFrameEntryRoutePorts& ports,
    LegacyBattleActorFrameParentArgumentWord* final_group_a_argument_4 = nullptr
);

// Compose the audited entry, reset, linked-list release, update, lookup,
// default and selector-1/2 routes. Missing callee ports and all other
// selector paths retain their next unexecuted site. A physical RET alone
// sets returned=true.
[[nodiscard]] LegacyBattleActorFrameEntryResult
advance_legacy_battle_actor_frame_entry_route(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    const LegacyBattleActorFrameEntryRoutePorts& ports
);

// From reset_call_ready only: execute the existing typed 0x00478850 child
// at the physical 0x004798F7 CALL, then publish the parent suffix through
// 0x0047990B. Stop before the separately modeled 0x00479911 release CALL.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Executes the arg=1 0x0047E950 child after 0x00479911. A zero linked
// head returns through 0x0047991F; a nonzero head reaches the separate
// linked-node continuation at 0x0047F0DE without claiming a return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_release(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Execute exactly one nonempty 0x0047F0DE list-node step. The borrowed
// descriptors must be live for this step; a release may invalidate them,
// so the caller must resolve fresh backing on each nonterminal continuation.
// The next pointer is captured before release; no descriptor is read after
// that call. Nonterminal release returns linked_node_read_ready, not a RET.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_release_node(
    std::span<const LegacyBattleActorFrameLinkedNode> nodes,
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// From update_ready only: PUSH EDI, call the adapted 0x004321E0 updater,
// and publish the EAX=0 return path. EAX!=0 passes to the separate
// actor+0x41A lookup and selector continuations.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_update(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameUpdatePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// From update_frame_read_ready only: read the slot2 +0x4A word, push both
// arguments, call adapted 0x004315D0, publish +0x2548 and the following
// owner reads/writes. Stop before 0x00479965's next actor read.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_lookup(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameUpdatePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// From update_post_lookup_ready: preserve the optional source-read stop,
// mask +0x2694, and read the actor+0x0C resource token. Stop before the
// unexecuted nested resource+0x20 TEST at 0x0047999F.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_resource_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Execute 0x0047999F's nested resource byte TEST and the two selector
// overrides in exact order. Stop before 0x004799C3 DEC / jump-table decode.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_selector_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Preserve the two separate physical table reads at 0x004799CF/0x004799D5.
// Returned targets are not yet executed; default and case paths stay distinct.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_selector_dispatch(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Complete the shared 0x0047A80B default epilogue only after the jump has
// actually arrived there. Preserve each saved-register/RET stack read.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_default_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case 1: read the signed 16-bit phase and stop before the active draw
// branch, or arrive at the unexecuted shared 0x0047B801 reset tail.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Execute the shared 0x0047B801..0x0047B818 reset prefix, including
// individual REP writes. Stop before CALL sub_478850 at 0x0047B81A.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_common_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Execute CALL sub_478850 at 0x0047B81A and the shared return-1 tail.
// This path does not call the 0x00479911 list release.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_common_reset_return(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case 1 active branch: retain the optional audio arguments or the first
// source-token read and EBX push. Neither nested CALL/source read is executed.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_active_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Physical 0x00479A0E first-dword read from the resource returned by the
// frame loader; stop before publishing global 0x004CD730.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_source_read(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Execute only the phase-zero 0x004799FA sample CALL and ADD ESP,8; a
// stopped sound boundary must retain the two arguments and return slot.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_audio(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// The 0x00479A10 source global and three independent signed arithmetic
// motion publications, ending before actor+0x2548 at 0x00479A77.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_motion_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// 0x00479A77..0x00479AA1: read resource+0x0E and phase for 4CD75C;
// read/mask/write actor+0x2694 before the next draw-argument read.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_height(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Ordered frame token/offset/resource/coordinate/motion reads and five
// argument PUSHes. Stop before executing sub_4170E0 at 0x00479AEC.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// 0x00479AEC CALL, then an independently stoppable 0x00479AF1 word RMW.
// No phase increment occurs when the callee does not return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_phase_increment(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Post-draw caller stack cleanup and 0x00479AFC..0x00479B05 return zero.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_one_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case2 reads phase at 0x00479B06 and, unless exactly CX=100, reads
// actor+0x0E14 before branching to init or the existing-particle tail.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case2 phase==100: two word clears, 38 physical REP writes from slot2,
// then the first emitter token read. Release/22-dword reset is still pending.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_release_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// The zero-emitter branch reaches the 22-dword reset without a release
// call. MOV ECX,0x16 leaves the preceding CMP flags unchanged.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_clear(
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Optional emitter release CALL at 0x00479C94. Only a normal typed reply
// reaches the still-unexecuted shared 22-dword clear at 0x0047B814.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_release_call(
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// Shared 0x0047B814 XOR EAX / 22 REP writes. Only the Group-A emitter's
// 0x0E14..0x0E6B owner is complete; writes outside it stop before commit.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// New-particle REP: case2 0x00479B30 or case8 0x0047A628, distinct from
// terminal clear; each stops before its own stack-output LEAs.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_initial_clear(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case2/8 stack-output LEAs and four ordered arguments, including the
// separately read source first dword; stop before each decoder CALL.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// 0x00479B46 CALL/RET boundary, then 0x00479B4B actor+0x0E14 write.
// A deep decoder failure cannot be represented as a normal return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_call(
    LegacyBattleActorFrameDecodePort& decoder,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_decoder_publish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// 0x00479B51..0x00479B6F: read frame token before cdecl cleanup, then
// independently read width and height from separately reloaded tokens.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_dimensions(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// 0x00479B76..0x00479BC7: X/Y, repeated source width/height and three
// dword emitter-coordinate writes; stop before +0x0E3C flag-byte RMW.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// +0x0E3C byte RMW followed by Y geometry and seven ordered emitter
// field writes; stop at the unexecuted 0x00479C1C sub_47CE70 call.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_emitter_fields(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Physical three-instruction sub_47CE70, CMP eax,1, and the optional
// +0x0E3C byte RMW. Stop before GetSystemMetrics IAT loading.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_property(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// IAT read, two independent GetSystemMetrics(index=1, then EBX) stdcall
// boundaries, and their two result PUSHes; do not enter sub_433F30 here.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_metrics(
    LegacyBattleActorFrameMetricsPort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// Shared case2 existing-emitter/initialized tail: separate global read,
// PUSH actor+0x0E14 and PUSH dword_4CD76C, then stop before sub_434790.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Physical CALL/RET8 of sub_433F30; host owner side effects are delegated
// to the explicit port, not inferred from metric values alone.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_rectangle_call(
    LegacyBattleActorFrameRectanglePort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// +0x0428 aliases slot2 LegacyActionRecord::field_58; after its word
// write read the live sample global and push the two audio arguments.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// +0x2958=1 is written only after normal audio return and ADD ESP,8.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_sample_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Common particle CALL; normal RET8 with EAX 0 reaches default epilogue,
// EAX 1 reaches the still-separate +0x2958=100 write.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_call(
    LegacyBattleActorFrameParticlePort& port,
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// EAX 1 branch only: phase100 write, distinct POP EDI/ESI/EBP/EBX,
// stack locals cleanup and RET; the EAX 0 branch uses default_return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_two_particle_return(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case9: signed phase>=45 writes two independent words and enters the
// common reset at its second write; all other phases stay at 0x0047A763.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case9 phase==BX alone re-reads sample into EDX and pushes EDX/0x31;
// nonzero phase goes straight to the still-unexecuted frame-token read.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// Case9 +0x2548/source head, EBX stack slot, signed one-operand IMUL,
// and sequential +4CD730/+4CD724/+4CC2F0 global writes.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_source_and_opacity(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Physical six draw arguments (including the earlier EBX slot); source
// width/height and actor offsets are read at their own LST instructions.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// A7F8/A7FE sequential global clears then +0x2958 word INC RMW;
// normal control joins the already-typed default POP/RET epilogue.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_nine_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case8 has an in-memory CMP word (no AX write), then a separate E14
// dword CMP. The first two exits join shared case2 release/particle tail.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case8 source-origin x/y then constant target-origin x, with an optional
// overwrite when +0x2B08 == 1; stop at the following geometry arithmetic.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case8 geometry fields use their own height read and OR-byte RMW;
// stop before sub_47CE70 and preserve twelve ordered memory accesses.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_fields(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case8 has no actor +0x428 write; read the global sample handle then
// PUSH handle and code 0x31 before the independent audio CALL.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eight_sample_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case6 compares signed phase against -32 (inclusive terminal), unlike
// case9's signed +45 terminal; both share the physical reset second write.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case11 has the same signed -32 terminal and common physical reset
// writes, but a separate initial read and active successor.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case50 uses signed phase >=15 and jumps to the shared B801 reset
// head, rather than clearing the auxiliary phase at A253.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_initialize(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_property(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_decoder_call(
    LegacyBattleActorFrameDecodePort& decoder,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

struct LegacyBattleActorFrameDirectionalScanOwners {
    const LegacyBattleDirectionVectors* vectors{};
    const LegacyBattleDirectionalSurface* surface{};
    LegacyBattleDirectionalScanSharedState* shared{};
    rendering::LegacyPixelConversionState* pixel_format{};
    compat::u32 surface_token{};
};

// The no-owner entry retains its child-entry typed stop. This overload may
// return only after the canonical decoder pixels and surface have been
// supplied and the real 0x004344E0 callee has completed.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_spawn_call(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameDirectionalScanOwners& owners,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_spawn_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifty_one_reset_return(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleBoundedRandomPort& random,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_post_rectangle_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_third_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_fourth_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_fourth_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_shared_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_seven_shared_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_four_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_four_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_four_shared_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_third_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_third_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_fourth_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_thirteen_fourth_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_three_rectangle_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_progress_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_early_phase_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_shared_rectangle_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_first_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_second_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fourteen_late_phase_tail(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_short_reset(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_short_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_draw_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_count_increment(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_increment_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_particle_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_decoder_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_dimensions(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_geometry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_configuration(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_metrics_prepare(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_metrics_calls(
    LegacyBattleActorFrameMetricsPort& port,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_phase_write(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_particle_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_release_gate(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_release_call(
    LegacyBattleActorFrameReleasePort& release,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_hundred_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_rectangle_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_four_rectangle_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_draw_call(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_after_first_draw(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_clip_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_clip_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_draw_call(
    const LegacyBattleActorRuntimeResetView& actor,
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_after_first_draw(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_clip_arguments(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_ten_clip_entry(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_terminal_writes(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_terminal_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Executes the actual sub_416FF0 memory/stack sequence against the canonical
// raster owner. Never treats the earlier child-entry stop as a normal return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_clip_callee(
    rendering::LegacyRasterGeometryState& raster,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_five_ten_active_return(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_header(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_globals(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_source_route(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_raster_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_raster_entry(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix,
    LegacyBattleActorFrameScaledRlePort* raster = nullptr
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_active_phase(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_active_return(
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_terminal_writes(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_twelve_reset_prefix(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case6 phase0 loads the shared sample into EAX before both PUSHes.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// Case11 shares sound arguments but has separate physical branch,
// sample read, PUSH, CALL and return instructions.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_audio_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_audio_call(
    LegacyBattleActorFrameSoundPort& sound,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_between_draws(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_fifteen_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// First case11 source header writes render mode 15 before the frame head,
// then conditionally replaces it for signed phase <= -16.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_first_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// The first case11 draw keeps all six arguments on the stack for a
// second call; unlike case6 it must not clean them on normal return.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_first_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_between_draws(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// The second case11 six-argument draw uses phase*10 to derive Y and
// leaves the first draw's six arguments below it on the call stack.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_second_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_second_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_eleven_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case6 has three independent signed phase reads; publish three distinct
// draw globals only after the separately faultable frame-head publication.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_source(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// Case6 six arguments include the already-pushed zero from the source
// stage; five later PUSHes follow distinct actor and frame reads.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_draw_arguments(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

// CALL pushes its own return slot. Only normal callee return permits
// caller ADD ESP,0x18 and the subsequent phase-word decrement.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_draw_call(
    LegacyBattleActorFrameDrawPort& draw,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
);

// DEC word is an independent read/write RMW and preserves CF. Its
// return uses the case6 physical POP/RET sequence, not default's EIPs.
[[nodiscard]] LegacyBattleActorFrameEntryResult
continue_legacy_battle_actor_frame_case_six_finish(
    const LegacyBattleActorRuntimeResetView& actor,
    const LegacyBattleActorFrameEntryRequest& request,
    LegacyBattleActorFrameEntryResult prefix
) noexcept;

}  // namespace openswd3::battle
