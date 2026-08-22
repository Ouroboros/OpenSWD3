#pragma once

#include "test.hpp"

#include <filesystem>

void test_shared_dialog_handler_variants(openswd3::test::Context& test);
void test_shared_dialog_raw_aliases(openswd3::test::Context& test);
void test_dialog_flag18_suppression_protocol(openswd3::test::Context& test);
void test_clear_dialog_control_flag(openswd3::test::Context& test);
void test_clear_dialog_control_flag_bit30(openswd3::test::Context& test);
void test_stage_dialog_lifetime(openswd3::test::Context& test);
void test_dialog_text_preparation_and_mode_zero_metrics(
    openswd3::test::Context& test
);
void test_dialog_anchor_delay_flag_and_reset(openswd3::test::Context& test);
void test_dialog_checked_failure_order(openswd3::test::Context& test);
void test_default_invalid_opcode_protocol(openswd3::test::Context& test);
void test_initial_flags_and_alignment_gate(openswd3::test::Context& test);
void test_reinitialization_writes_only_owned_vm_fields(
    openswd3::test::Context& test
);
void test_dialog_enqueue_and_wait_protocol(openswd3::test::Context& test);
void test_dialog_role_overlap_avoidance(openswd3::test::Context& test);
void test_dialog_explicit_layout_pair(openswd3::test::Context& test);
void test_story_transfer_protocol(openswd3::test::Context& test);
void test_current_map_reload_protocol(openswd3::test::Context& test);
void test_item_total_reload_protocol(openswd3::test::Context& test);
void test_mode_text_protocol(openswd3::test::Context& test);
void test_suspend_story_ani_protocol(openswd3::test::Context& test);
void test_resume_story_ani_protocol(openswd3::test::Context& test);
void test_gather_party_at_player_protocol(openswd3::test::Context& test);
void test_set_role_collision_bypass_protocol(openswd3::test::Context& test);
void test_enqueue_frame_deformation_protocol(openswd3::test::Context& test);
void test_clear_frame_execution_gate_protocol(openswd3::test::Context& test);
void test_party_member_field_reload_protocol(openswd3::test::Context& test);
void test_party_member_field_write_protocol(openswd3::test::Context& test);
void test_transfer_flags_and_terminal_cleanup(openswd3::test::Context& test);
void test_same_file_branch(openswd3::test::Context& test);
void test_role_action_operand_extension(openswd3::test::Context& test);
void test_update_role_action_fields_protocol(openswd3::test::Context& test);
void test_missing_role_position_patch(openswd3::test::Context& test);
void test_change_role_base_variant_protocol(openswd3::test::Context& test);
void test_change_role_variant_delta_protocol(openswd3::test::Context& test);
void test_set_role_position_protocol(openswd3::test::Context& test);
void test_step_role_protocol(openswd3::test::Context& test);
void test_wait_role_action_status_protocol(openswd3::test::Context& test);
void test_jump_same_file_offset_protocol(openswd3::test::Context& test);
void test_jump_if_role_path_unprepared_protocol(openswd3::test::Context& test);
void test_jump_if_role_path_prepared_protocol(openswd3::test::Context& test);
void test_role_action_chain_update_gate(openswd3::test::Context& test);
void test_change_requested_action_id(openswd3::test::Context& test);
void test_change_requested_action_id_failure_ordering(
    openswd3::test::Context& test
);
void test_restore_role_action_overrides_protocol(openswd3::test::Context& test);
void test_start_camera_move_protocol(openswd3::test::Context& test);
void test_start_camera_move_failure_ordering(openswd3::test::Context& test);
void test_start_camera_move_window_boundaries(openswd3::test::Context& test);
void test_wait_for_camera_move_protocol(openswd3::test::Context& test);
void test_wait_for_camera_top_while_moving_protocol(
    openswd3::test::Context& test
);
void test_wait_for_music_stream_transition_protocol(
    openswd3::test::Context& test
);
void test_wait_for_story_video_protocol(openswd3::test::Context& test);
void test_latch_common_join_same_call_protocol(openswd3::test::Context& test);
void test_clear_common_join_latch_and_yield_protocol(
    openswd3::test::Context& test
);
void test_start_frame_color_transition_protocol(openswd3::test::Context& test);
void test_start_frame_color_transition_window_boundaries(
    openswd3::test::Context& test
);
void test_wait_for_frame_color_transition_protocol(
    openswd3::test::Context& test
);
void test_repeat_role_action_refresh_protocol(openswd3::test::Context& test);
void test_shared_role_spatial_group_protocol(openswd3::test::Context& test);
void test_wait_for_role_action_index_threshold(openswd3::test::Context& test);
void test_set_next_dialog_anchor_protocol(openswd3::test::Context& test);
void test_step_role_list_protocol(openswd3::test::Context& test);
void test_secondary_role_bit30_reload_protocol(openswd3::test::Context& test);
void test_wait_overlay_action_lists_protocol(openswd3::test::Context& test);
void test_play_sound_effect_with_unread_padding_protocol(
    openswd3::test::Context& test
);
void test_stage_scene_music_stream_request_protocol(
    openswd3::test::Context& test
);
void test_set_music_stream_volume_protocol(openswd3::test::Context& test);
void test_batch_set_role_positions_protocol(openswd3::test::Context& test);
void test_remove_dialogs_for_role_guid_protocol(openswd3::test::Context& test);
void test_wait_dialog_flag_protocol(openswd3::test::Context& test);
void test_release_role_path_protocol(openswd3::test::Context& test);
void test_release_all_role_paths_protocol(openswd3::test::Context& test);
void test_schedule_role_paths_protocol(openswd3::test::Context& test);
void test_jump_if_global_bit_protocol(openswd3::test::Context& test);
void test_jump_if_all_global_bits_set_protocol(openswd3::test::Context& test);
void test_jump_if_any_global_bit_set_protocol(openswd3::test::Context& test);
void test_set_global_bit_protocol(openswd3::test::Context& test);
void test_clear_global_bit_protocol(openswd3::test::Context& test);
void test_reload_world_session_protocol(openswd3::test::Context& test);
void test_change_role_path_id_protocol(openswd3::test::Context& test);
void test_global_integer_protocol(openswd3::test::Context& test);
void test_wide_global_integer_protocol(openswd3::test::Context& test);
void test_set_bounded_script_clock_protocol(openswd3::test::Context& test);
void test_jump_if_byte_le_script_clock_protocol(openswd3::test::Context& test);
void test_jump_if_script_clock_exceeds_origin_delta_protocol(
    openswd3::test::Context& test
);
void test_snapshot_script_clock_protocol(openswd3::test::Context& test);
void test_clear_role_from_scene_protocol(openswd3::test::Context& test);
void test_set_role_flag_8000_and_clear_one_shots_protocol(
    openswd3::test::Context& test
);
void test_relocate_role_and_complete_path_protocol(
    openswd3::test::Context& test
);
void test_reload_indexed_target_protocol(openswd3::test::Context& test);
void test_interaction_lock_protocol(openswd3::test::Context& test);
void test_set_role_action_wait_override_protocol(openswd3::test::Context& test);
void test_shared_picture_action_enqueue_protocol(openswd3::test::Context& test);
void test_request_battle_after_clearing_overlay_lists(
    openswd3::test::Context& test
);
void test_play_sound_effect_protocol(openswd3::test::Context& test);
void test_shared_scene_render_control_protocol(openswd3::test::Context& test);
void test_write_map_role_patch_protocol(openswd3::test::Context& test);
void test_write_map_role_materialization_protocol(
    openswd3::test::Context& test
);
void test_write_map_role_failure_ordering(openswd3::test::Context& test);
void test_set_selection_scroll_protocol(openswd3::test::Context& test);
void test_clear_selection_scroll_protocol(openswd3::test::Context& test);
void test_transfer_role_to_party_protocol(openswd3::test::Context& test);
void test_update_role_map_state_protocol(openswd3::test::Context& test);
void test_frame_clock_wait_protocol(openswd3::test::Context& test);
void test_clear_role_flag_0400_protocol(openswd3::test::Context& test);
void test_set_role_flag_0400_protocol(openswd3::test::Context& test);
void test_wait_for_frame_color_transition(openswd3::test::Context& test);
void test_suspend_story_role_protocol(openswd3::test::Context& test);
void test_turn_role_toward_role(openswd3::test::Context& test);
void test_turn_role_toward_role_lookup_boundaries(
    openswd3::test::Context& test
);
void test_turn_role_toward_role_owner_and_exact_tail(
    openswd3::test::Context& test
);
void test_set_role_head_sign_action(openswd3::test::Context& test);
void test_set_and_clear_role_wait_override(openswd3::test::Context& test);
void test_role_wait_override_lookup_boundaries(openswd3::test::Context& test);
void test_role_wait_override_exact_tails(openswd3::test::Context& test);
void test_clear_text_control_bit29(openswd3::test::Context& test);
void test_enqueue_role_head_action_protocol(openswd3::test::Context& test);
void test_enqueue_role_head_action_boundaries(openswd3::test::Context& test);
void test_dismiss_role_head_action_protocol(openswd3::test::Context& test);
void test_dismiss_role_head_action_boundaries(openswd3::test::Context& test);
void test_upsert_packed_row_effect_protocol(openswd3::test::Context& test);
void test_upsert_packed_row_effect_boundaries(openswd3::test::Context& test);
void test_control_packed_row_effect_protocol(openswd3::test::Context& test);
void test_control_packed_row_effect_boundaries(openswd3::test::Context& test);
void test_begin_story_video_protocol(openswd3::test::Context& test);
void test_rewrite_role_head_action_key_protocol(openswd3::test::Context& test);
void test_reload_random_target_protocol(openswd3::test::Context& test);
void test_request_battle_protocol(openswd3::test::Context& test);
void test_load_name_record_protocol(openswd3::test::Context& test);
void test_set_reserved_global_bit_protocol(openswd3::test::Context& test);
void test_clear_reserved_global_bit_protocol(openswd3::test::Context& test);
void test_set_scene_render_bit1_protocol(openswd3::test::Context& test);
void test_clear_scene_render_bit1_protocol(openswd3::test::Context& test);
void test_begin_custom_ani_protocol(openswd3::test::Context& test);
void test_wait_custom_ani_complete_protocol(openswd3::test::Context& test);
void test_consume_four_byte_noop_protocol(openswd3::test::Context& test);
void test_wait_custom_ani_phase_protocol(openswd3::test::Context& test);
void test_set_role_talk_script_protocol(openswd3::test::Context& test);
void test_set_role_status_bit26_protocol(openswd3::test::Context& test);
void test_set_role_status_from_boolean_protocol(openswd3::test::Context& test);
void test_set_text_layout_pair_protocol(openswd3::test::Context& test);
void test_clear_text_control_bit27_protocol(openswd3::test::Context& test);
void test_clear_text_control_bit26_protocol(openswd3::test::Context& test);
void test_clear_speed_mode_protocol(openswd3::test::Context& test);
void test_update_scene_music_table_entry_protocol(
    openswd3::test::Context& test
);
void test_clear_text_control_bit25_protocol(openswd3::test::Context& test);
void test_append_text_allocation_protocol(openswd3::test::Context& test);
void test_role_base_variant_reload_protocol(openswd3::test::Context& test);
void test_adjust_player_item_quantity_protocol(openswd3::test::Context& test);
void test_item_presence_reload_protocol(openswd3::test::Context& test);
void test_add_party_item_if_allowed_protocol(openswd3::test::Context& test);
void test_swap_player_item_into_role_slot_protocol(
    openswd3::test::Context& test
);
void test_request_shop_protocol(openswd3::test::Context& test);
void test_adjust_party_member_resources_protocol(openswd3::test::Context& test);
void test_reset_input_menu_state_protocol(openswd3::test::Context& test);
void test_stop_scene_music_stream_protocol(openswd3::test::Context& test);
void test_role_distance_reload_protocol(openswd3::test::Context& test);
void test_configure_music_stream_transition_protocol(
    openswd3::test::Context& test
);
void test_initialize_primary_countdown_protocol(openswd3::test::Context& test);
void test_disable_primary_countdown_protocol(openswd3::test::Context& test);
void test_request_special_mode_four_or_five_protocol(
    openswd3::test::Context& test
);
void test_set_story_flag_70_protocol(openswd3::test::Context& test);
void test_set_story_flag_19_protocol(openswd3::test::Context& test);
void test_clear_story_flag_19_protocol(openswd3::test::Context& test);
void test_configure_ani_follower_position_protocol(
    openswd3::test::Context& test
);
void test_configure_ani_follower_target_protocol(openswd3::test::Context& test);
void test_wait_ani_follower_target_protocol(openswd3::test::Context& test);
void test_reload_current_world_session_protocol(openswd3::test::Context& test);
void test_reload_deferred_world_session_protocol(openswd3::test::Context& test);
void test_configure_deferred_world_session_protocol(
    openswd3::test::Context& test
);
void test_story_file_operations_protocol(openswd3::test::Context& test);
void test_suppress_next_dialog_flag18_protocol(openswd3::test::Context& test);
void test_wait_picture_action_byte_protocol(openswd3::test::Context& test);
void test_enqueue_moving_action_protocol(openswd3::test::Context& test);
void test_enqueue_moving_action_boundaries(openswd3::test::Context& test);
void test_real_clear_dialog_control_flag_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_change_role_base_variant_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_change_role_variant_delta_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_dialog_control_flag_bit30_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_stage_dialog_lifetime_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_role_action_status_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_story_transfer_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_current_map_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_item_total_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_mode_text_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_gather_party_at_player_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_collision_bypass_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_jump_same_file_offset_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_jump_if_role_path_unprepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_jump_if_role_path_prepared_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_release_role_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_release_all_role_paths_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_schedule_role_path_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_jump_if_global_bit_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_jump_if_all_global_bits_set_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_global_bit_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_reload_world_session_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_reload_deferred_world_session_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_change_role_path_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_global_integer_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_relocate_role_and_complete_path_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_reload_indexed_target_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_interaction_lock_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_action_wait_override_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_action_id_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_camera_move_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_start_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_play_sound_effect_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_write_map_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_selection_scroll_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_selection_scroll_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_transfer_role_to_party_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_update_role_map_state_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_frame_clock_wait_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_role_flag_0400_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_flag_0400_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_head_sign_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_role_head_sign_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_cancel_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_suspend_story_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_turn_and_suspend_story_role_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_role_wait_override_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_begin_story_video_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_for_story_video_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_rewrite_role_head_action_key_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_reload_random_target_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_request_battle_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_scene_render_bit1_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_scene_render_bit1_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_begin_custom_ani_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_custom_ani_complete_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_consume_four_byte_noop_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_custom_ani_phase_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_talk_script_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_status_bit26_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_status_from_boolean_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_text_layout_pair_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_text_control_bit27_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_text_control_bit26_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_speed_mode_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_update_scene_music_table_entry_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_role_base_variant_reload_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_adjust_player_item_quantity_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_request_shop_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_adjust_party_member_resources_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_item_presence_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_primary_picture_action_byte_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_role_action_index_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_step_role_list_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_secondary_role_bit30_reload_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_overlay_action_lists_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_stage_scene_music_stream_request_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_stop_scene_music_stream_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_role_distance_reload_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_configure_music_stream_transition_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_initialize_primary_countdown_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_disable_primary_countdown_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_request_special_mode_four_or_five_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_story_flag_70_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_batch_set_role_positions_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_remove_dialogs_for_role_guid_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_dialog_flag_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_update_role_action_fields_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_load_name_record_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_control_packed_row_effect_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_upsert_packed_row_effect_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_dismiss_role_head_action_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_enqueue_role_head_action_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_text_control_bit29_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_shared_scene_render_control_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_shared_picture_action_enqueue_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_shared_role_spatial_group_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_repeat_role_action_refresh_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_for_frame_color_transition_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_for_camera_move_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_wait_for_camera_top_while_moving_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_set_role_flag_8000_and_clear_one_shots_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_clear_role_from_scene_record(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_shared_dialog_handler_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_suppress_next_dialog_flag18_records(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_story_248_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_new_game_story_patches_unloaded_role(
    openswd3::test::Context& test, const std::filesystem::path& root
);
void test_real_new_game_story_reaches_first_dialog(
    openswd3::test::Context& test, const std::filesystem::path& root
);
