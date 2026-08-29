add_executable(
    openswd3_diagnostics_log_tests
    unit/diagnostics/log_test.cpp
)
target_compile_definitions(
    openswd3_diagnostics_log_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_diagnostics_log_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_diagnostics_log_tests
    PRIVATE openswd3_diagnostics
)

add_test(
    NAME diagnostics.log
    COMMAND openswd3_diagnostics_log_tests
)

add_executable(
    openswd3_story_scene_legacy_dialog_geometry_tests
    unit/story_scene/legacy_dialog_geometry_test.cpp
)
target_include_directories(
    openswd3_story_scene_legacy_dialog_geometry_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_story_scene_legacy_dialog_geometry_tests
    PRIVATE openswd3_story_scene
)

add_test(
    NAME story_scene.legacy_dialog_geometry
    COMMAND openswd3_story_scene_legacy_dialog_geometry_tests
)

add_executable(
    openswd3_story_scene_legacy_dialog_text_tests
    unit/story_scene/legacy_dialog_text_test.cpp
)
target_include_directories(
    openswd3_story_scene_legacy_dialog_text_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_story_scene_legacy_dialog_text_tests
    PRIVATE openswd3_story_scene
)

add_test(
    NAME story_scene.legacy_dialog_text
    COMMAND openswd3_story_scene_legacy_dialog_text_tests
)

add_executable(
    openswd3_story_scene_legacy_dialog_control_tests
    unit/story_scene/legacy_dialog_control_test.cpp
)
target_include_directories(
    openswd3_story_scene_legacy_dialog_control_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_story_scene_legacy_dialog_control_tests
    PRIVATE openswd3_story_scene
)

add_test(
    NAME story_scene.legacy_dialog_control
    COMMAND openswd3_story_scene_legacy_dialog_control_tests
)

add_executable(
    openswd3_story_scene_legacy_dialog_runtime_tests
    unit/story_scene/legacy_dialog_runtime_test.cpp
)
target_include_directories(
    openswd3_story_scene_legacy_dialog_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_story_scene_legacy_dialog_runtime_tests
    PRIVATE openswd3_story_scene
)

add_test(
    NAME story_scene.legacy_dialog_runtime
    COMMAND openswd3_story_scene_legacy_dialog_runtime_tests
)

add_executable(
    openswd3_special_modes_legacy_initial_menu_tests
    unit/special_modes/legacy_initial_menu_test.cpp
)
target_include_directories(
    openswd3_special_modes_legacy_initial_menu_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_special_modes_legacy_initial_menu_tests
    PRIVATE openswd3_special_modes
)
if(WIN32)
    # 该聚合测试包含多组大型原始状态快照；仅提高测试进程的栈保留量。
    target_link_options(
        openswd3_special_modes_legacy_initial_menu_tests
        PRIVATE "LINKER:/stack:16777216"
    )
endif()
if(EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT")
    target_compile_definitions(
        openswd3_special_modes_legacy_initial_menu_tests
        PRIVATE OPENSWD3_GAME_DATA_ROOT="${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_test(
    NAME special_modes.legacy_initial_menu
    COMMAND openswd3_special_modes_legacy_initial_menu_tests
)

add_executable(
    openswd3_app_frame_dispatch_tests
    unit/app/frame_dispatch_test.cpp
)
target_include_directories(
    openswd3_app_frame_dispatch_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_frame_dispatch_tests PRIVATE openswd3_app)

add_test(
    NAME app.frame_dispatch
    COMMAND openswd3_app_frame_dispatch_tests
)

add_executable(
    openswd3_app_frame_preparation_tests
    unit/app/frame_preparation_test.cpp
)
target_include_directories(
    openswd3_app_frame_preparation_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_app_frame_preparation_tests
    PRIVATE openswd3_app
)

add_test(
    NAME app.frame_preparation
    COMMAND openswd3_app_frame_preparation_tests
)

add_executable(
    openswd3_app_display_refresh_tests
    unit/app/display_refresh_test.cpp
)
target_include_directories(
    openswd3_app_display_refresh_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_display_refresh_tests PRIVATE openswd3_app)

add_test(
    NAME app.display_refresh
    COMMAND openswd3_app_display_refresh_tests
)

add_executable(
    openswd3_app_idle_runtime_tests
    unit/app/idle_runtime_test.cpp
)
target_include_directories(
    openswd3_app_idle_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_idle_runtime_tests PRIVATE openswd3_app)

add_test(
    NAME app.idle_runtime
    COMMAND openswd3_app_idle_runtime_tests
)

add_executable(
    openswd3_app_display_lifecycle_tests
    unit/app/display_lifecycle_test.cpp
)
target_include_directories(
    openswd3_app_display_lifecycle_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_display_lifecycle_tests PRIVATE openswd3_app)

add_test(
    NAME app.display_lifecycle
    COMMAND openswd3_app_display_lifecycle_tests
)

add_executable(
    openswd3_app_battle_transition_tests
    unit/app/battle_transition_test.cpp
)
target_include_directories(
    openswd3_app_battle_transition_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_battle_transition_tests PRIVATE openswd3_app)

add_test(
    NAME app.battle_transition
    COMMAND openswd3_app_battle_transition_tests
)

add_executable(
    openswd3_battle_legacy_battle_assets_tests
    unit/battle/legacy_battle_assets_test.cpp
)
target_compile_definitions(
    openswd3_battle_legacy_battle_assets_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../FIGTALK.dat" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../battle.ffd"
)
    target_compile_definitions(
        openswd3_battle_legacy_battle_assets_tests
        PRIVATE OPENSWD3_GAME_DATA_ROOT="${PROJECT_SOURCE_DIR}/.."
    )
endif()
target_include_directories(
    openswd3_battle_legacy_battle_assets_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_battle_legacy_battle_assets_tests
    PRIVATE openswd3_battle
)

add_test(
    NAME battle.legacy_battle_assets
    COMMAND openswd3_battle_legacy_battle_assets_tests
)

add_executable(
    openswd3_battle_legacy_battle_setup_tests
    unit/battle/legacy_battle_action_mode_refresh_test.cpp
    unit/battle/legacy_battle_action_summary_test.cpp
    unit/battle/legacy_battle_alternate_grid_frame_test.cpp
    unit/battle/legacy_battle_action_rotation_cache_test.cpp
    unit/battle/legacy_battle_actor_action_candidate_availability_test.cpp
    unit/battle/legacy_battle_actor_action_commit_test.cpp
    unit/battle/legacy_battle_actor_action_cycle_test.cpp
    unit/battle/legacy_battle_actor_action_reverse_cycle_test.cpp
    unit/battle/legacy_battle_actor_target_preparation_test.cpp
    unit/battle/legacy_battle_available_actor_cycle_test.cpp
    unit/battle/legacy_battle_available_actor_reverse_cycle_test.cpp
    unit/battle/legacy_battle_action_dispatch_test.cpp
    unit/battle/legacy_battle_attack_order_dequeue_test.cpp
    unit/battle/legacy_battle_attack_order_entry_test.cpp
    unit/battle/legacy_battle_attack_order_insert_test.cpp
    unit/battle/legacy_battle_attack_order_remove_test.cpp
    unit/battle/legacy_battle_retreat_commit_test.cpp
    unit/battle/legacy_battle_actor_frame_sequence_test.cpp
    unit/battle/legacy_battle_actor_lifecycle_test.cpp
    unit/battle/legacy_battle_actor_list_index_commit_test.cpp
    unit/battle/legacy_battle_actor_list_query_test.cpp
    unit/battle/legacy_battle_actor_metrics_test.cpp
    unit/battle/legacy_battle_actor_priority_test.cpp
    unit/battle/legacy_battle_actor_progress_test.cpp
    unit/battle/legacy_battle_actor_ready_test.cpp
    unit/battle/legacy_battle_group_a_action_execution_test.cpp
    unit/battle/legacy_battle_group_a_attribute_aggregation_test.cpp
    unit/battle/legacy_battle_group_a_attribute_effect_test.cpp
    unit/battle/legacy_battle_group_a_effect_reward_application_test.cpp
    unit/battle/legacy_battle_group_a_embedded_profile_application_test.cpp
    unit/battle/legacy_battle_group_a_final_processing_test.cpp
    unit/battle/legacy_battle_group_a_configuration_test.cpp
    unit/battle/legacy_battle_group_a_growth_reward_selection_test.cpp
    unit/battle/legacy_battle_group_a_item_effect_application_test.cpp
    unit/battle/legacy_battle_group_a_npc_materialization_test.cpp
    unit/battle/legacy_battle_group_a_profile_mode_selection_test.cpp
    unit/battle/legacy_battle_group_a_reward_profile_application_test.cpp
    unit/battle/legacy_battle_group_a_summon_materialization_test.cpp
    unit/battle/legacy_battle_group_a_resource_pair_test.cpp
    unit/battle/legacy_battle_group_a_value_pair_test.cpp
    unit/battle/legacy_battle_group_a_workspace_reset_test.cpp
    unit/battle/legacy_battle_animation_collision_test.cpp
    unit/battle/legacy_battle_opponent_action_dispatch_test.cpp
    unit/battle/legacy_battle_background_initialization_test.cpp
    unit/battle/legacy_battle_color_accumulation_test.cpp
    unit/battle/legacy_battle_context_prompt_test.cpp
    unit/battle/legacy_battle_control_panel_frame_test.cpp
    unit/battle/legacy_battle_debug_hotkeys_test.cpp
    unit/battle/legacy_battle_debug_overlay_test.cpp
    unit/battle/legacy_battle_debug_status_panel_test.cpp
    unit/battle/legacy_battle_defeat_panel_test.cpp
    unit/battle/legacy_battle_definition_archive_test.cpp
    unit/battle/legacy_battle_effect_coordinator_test.cpp
    unit/battle/legacy_battle_effect_frame_test.cpp
    unit/battle/legacy_battle_effect_shift_test.cpp
    unit/battle/legacy_battle_frame_completion_test.cpp
    unit/battle/legacy_battle_frame_coordinator_test.cpp
    unit/battle/legacy_battle_frame_effect_test.cpp
    unit/battle/legacy_battle_frame_refresh_test.cpp
    unit/battle/legacy_battle_full_frame_darkening_test.cpp
    unit/battle/legacy_battle_final_actor_step_test.cpp
    unit/battle/legacy_battle_group_a_frame_test.cpp
    unit/battle/legacy_battle_group_a_target_cycle_test.cpp
    unit/battle/legacy_battle_group_b_frame_test.cpp
    unit/battle/legacy_battle_global_reset_test.cpp
    unit/battle/legacy_battle_group_b_order_test.cpp
    unit/battle/legacy_battle_group_b_target_cycle_test.cpp
    unit/battle/legacy_battle_group_effect_frame_test.cpp
    unit/battle/legacy_battle_grid_frame_test.cpp
    unit/battle/legacy_battle_growth_actor_selection_test.cpp
    unit/battle/legacy_battle_growth_caption_test.cpp
    unit/battle/legacy_battle_growth_item_completion_panel_test.cpp
    unit/battle/legacy_battle_growth_item_result_selection_test.cpp
    unit/battle/legacy_battle_guard_panel_frame_test.cpp
    unit/battle/legacy_battle_hud_frame_test.cpp
    unit/battle/legacy_battle_input_dispatch_test.cpp
    unit/battle/legacy_battle_input_record_priming_test.cpp
    unit/battle/legacy_battle_frame_input_resolution_test.cpp
    unit/battle/legacy_battle_list_frame_test.cpp
    unit/battle/legacy_battle_level_advancement_test.cpp
    unit/battle/legacy_battle_level_growth_panel_test.cpp
    unit/battle/legacy_battle_level_up_panel_test.cpp
    unit/battle/legacy_battle_list_contents_test.cpp
    unit/battle/legacy_battle_menu_context_advance_test.cpp
    unit/battle/legacy_battle_menu_context_retreat_test.cpp
    unit/battle/legacy_battle_menu_input_finalize_test.cpp
    unit/battle/legacy_battle_menu_page_advance_test.cpp
    unit/battle/legacy_battle_menu_page_retreat_test.cpp
    unit/battle/legacy_battle_menu_selection_advance_test.cpp
    unit/battle/legacy_battle_menu_selection_retreat_test.cpp
    unit/battle/legacy_battle_message_phase_test.cpp
    unit/battle/legacy_battle_mode_grid_frame_test.cpp
    unit/battle/legacy_battle_narrow_grid_frame_test.cpp
    unit/battle/legacy_battle_intensity_effect_frame_test.cpp
    unit/battle/legacy_battle_object_reset_test.cpp
    unit/battle/legacy_battle_outcome_finalization_test.cpp
    unit/battle/legacy_battle_outcome_resolution_test.cpp
    unit/battle/legacy_battle_pair_transition_test.cpp
    unit/battle/legacy_battle_reward_item_slot_test.cpp
    unit/battle/legacy_battle_runtime_shutdown_test.cpp
    unit/battle/legacy_battle_party_item_order_test.cpp
    unit/battle/legacy_battle_pending_action_commit_test.cpp
    unit/battle/legacy_battle_player_item_order_test.cpp
    unit/battle/legacy_battle_player_item_quantity_test.cpp
    unit/battle/legacy_battle_post_action_test.cpp
    unit/battle/legacy_battle_pre_frame_test.cpp
    unit/battle/legacy_battle_scale_fill_panel_test.cpp
    unit/battle/legacy_battle_scale_scan_test.cpp
    unit/battle/legacy_battle_script_curve_test.cpp
    unit/battle/legacy_battle_script_dispatch_test.cpp
    unit/battle/legacy_battle_setup_test.cpp
    unit/battle/legacy_battle_single_effect_frame_test.cpp
    unit/battle/legacy_battle_status_indicator_test.cpp
    unit/battle/legacy_battle_startup_test.cpp
    unit/battle/legacy_battle_surface_blend_test.cpp
    unit/battle/legacy_battle_talisman_result_panel_test.cpp
    unit/battle/legacy_battle_text_message_test.cpp
    unit/battle/legacy_battle_text_panel_test.cpp
    unit/battle/legacy_battle_selection_frame_test.cpp
    unit/battle/legacy_battle_selection_hint_frame_test.cpp
    unit/battle/legacy_battle_target_selection_entry_test.cpp
    unit/battle/legacy_battle_target_selection_refresh_test.cpp
    unit/battle/legacy_battle_transition_test.cpp
    unit/battle/legacy_battle_transition_control_selection_test.cpp
    unit/battle/legacy_battle_transition_stage_advance_test.cpp
    unit/battle/legacy_battle_vertical_panel_test.cpp
    unit/battle/legacy_battle_vertical_shift_test.cpp
    unit/battle/legacy_battle_victory_item_list_panel_test.cpp
    unit/battle/legacy_battle_victory_rewards_test.cpp
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../FIGTALK.dat" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../battle.ffd"
)
    target_compile_definitions(
        openswd3_battle_legacy_battle_setup_tests
        PRIVATE OPENSWD3_GAME_DATA_ROOT="${PROJECT_SOURCE_DIR}/.."
    )
endif()
target_include_directories(
    openswd3_battle_legacy_battle_setup_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_battle_legacy_battle_setup_tests
    PRIVATE openswd3_battle
)
if(WIN32)
    target_link_options(
        openswd3_battle_legacy_battle_setup_tests
        PRIVATE "LINKER:/STACK:8388608"
    )
endif()

add_test(
    NAME battle.legacy_battle_setup
    COMMAND openswd3_battle_legacy_battle_setup_tests
)

add_executable(
    openswd3_app_startup_tests
    unit/app/startup_test.cpp
)
target_include_directories(
    openswd3_app_startup_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_startup_tests PRIVATE openswd3_app)

add_test(
    NAME app.startup
    COMMAND openswd3_app_startup_tests
)

add_executable(
    openswd3_app_initialization_tests
    unit/app/initialization_test.cpp
)
target_include_directories(
    openswd3_app_initialization_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_initialization_tests PRIVATE openswd3_app)

add_test(
    NAME app.initialization
    COMMAND openswd3_app_initialization_tests
)

add_executable(
    openswd3_app_platform_backend_initialization_tests
    unit/app/platform_backend_initialization_test.cpp
)
target_include_directories(
    openswd3_app_platform_backend_initialization_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_app_platform_backend_initialization_tests
    PRIVATE openswd3_app
)

add_test(
    NAME app.platform_backend_initialization
    COMMAND openswd3_app_platform_backend_initialization_tests
)

add_executable(
    openswd3_app_frame_runtime_tests
    unit/app/frame_runtime_test.cpp
)
target_include_directories(
    openswd3_app_frame_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_frame_runtime_tests PRIVATE openswd3_app)

add_test(
    NAME app.frame_runtime
    COMMAND openswd3_app_frame_runtime_tests
)

add_executable(
    openswd3_app_legacy_new_game_transition_tests
    unit/app/legacy_new_game_transition_test.cpp
)
target_include_directories(
    openswd3_app_legacy_new_game_transition_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_app_legacy_new_game_transition_tests
    PRIVATE openswd3_app
)

add_test(
    NAME app.legacy_new_game_transition
    COMMAND openswd3_app_legacy_new_game_transition_tests
)

add_executable(
    openswd3_compat_legacy_stubs_tests
    unit/compat/legacy_stubs_test.cpp
)
target_include_directories(
    openswd3_compat_legacy_stubs_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_compat_legacy_stubs_tests PRIVATE openswd3_compat)

add_test(
    NAME compat.legacy_stubs
    COMMAND openswd3_compat_legacy_stubs_tests
)

add_executable(
    openswd3_compat_legacy_decimal_tests
    unit/compat/legacy_decimal_test.cpp
)
target_include_directories(
    openswd3_compat_legacy_decimal_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_compat_legacy_decimal_tests
    PRIVATE openswd3_compat
)

add_test(
    NAME compat.legacy_decimal
    COMMAND openswd3_compat_legacy_decimal_tests
)

add_executable(
    openswd3_app_command_line_tests
    unit/app/command_line_test.cpp
)
target_include_directories(
    openswd3_app_command_line_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_command_line_tests PRIVATE openswd3_app)

add_test(
    NAME app.command_line
    COMMAND openswd3_app_command_line_tests
)

add_executable(
    openswd3_app_window_events_tests
    unit/app/window_events_test.cpp
)
target_include_directories(
    openswd3_app_window_events_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_window_events_tests PRIVATE openswd3_app)

add_test(
    NAME app.window_events
    COMMAND openswd3_app_window_events_tests
)

add_executable(
    openswd3_app_host_window_event_tests
    unit/app/host_window_event_test.cpp
)
target_include_directories(
    openswd3_app_host_window_event_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_host_window_event_tests PRIVATE openswd3_app)

add_test(
    NAME app.host_window_event
    COMMAND openswd3_app_host_window_event_tests
)

add_executable(
    openswd3_app_screenshot_tests
    unit/app/screenshot_test.cpp
)
target_include_directories(
    openswd3_app_screenshot_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_screenshot_tests PRIVATE openswd3_app)

add_test(
    NAME app.screenshot
    COMMAND openswd3_app_screenshot_tests
)

add_executable(
    openswd3_app_startup_dialog_tests
    unit/app/startup_dialog_test.cpp
)
target_include_directories(
    openswd3_app_startup_dialog_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_startup_dialog_tests PRIVATE openswd3_app)

add_test(
    NAME app.startup_dialog
    COMMAND openswd3_app_startup_dialog_tests
)

add_executable(
    openswd3_startup_ui_assets_tests
    unit/app/startup_ui_assets_test.cpp
)
target_include_directories(
    openswd3_startup_ui_assets_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_startup_ui_assets_tests
    PRIVATE openswd3_app
)

add_test(
    NAME assets.startup_ui
    COMMAND
        openswd3_startup_ui_assets_tests
        "${PROJECT_SOURCE_DIR}/assets/ui/startup"
)

add_executable(
    openswd3_app_external_launch_tests
    unit/app/external_launch_test.cpp
)
target_include_directories(
    openswd3_app_external_launch_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_external_launch_tests PRIVATE openswd3_app)

add_test(
    NAME app.external_launch
    COMMAND openswd3_app_external_launch_tests
)

add_executable(
    openswd3_platform_external_uri_tests
    unit/app/external_uri_test.cpp
    ${PROJECT_SOURCE_DIR}/src/platform/sdl3/external_uri.cpp
)
target_include_directories(
    openswd3_platform_external_uri_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${PROJECT_SOURCE_DIR}/src/platform/sdl3
)
target_link_libraries(
    openswd3_platform_external_uri_tests
    PRIVATE openswd3_app
)

add_test(
    NAME platform.external_uri
    COMMAND openswd3_platform_external_uri_tests
)

add_executable(
    openswd3_app_auxiliary_text_dialog_tests
    unit/app/auxiliary_text_dialog_test.cpp
)
target_include_directories(
    openswd3_app_auxiliary_text_dialog_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_app_auxiliary_text_dialog_tests
    PRIVATE openswd3_app
)

add_test(
    NAME app.auxiliary_text_dialog
    COMMAND openswd3_app_auxiliary_text_dialog_tests
)

add_executable(
    openswd3_app_process_startup_tests
    unit/app/process_startup_test.cpp
)
target_include_directories(
    openswd3_app_process_startup_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(openswd3_app_process_startup_tests PRIVATE openswd3_app)

add_test(
    NAME app.process_startup
    COMMAND openswd3_app_process_startup_tests
)

add_executable(
    openswd3_platform_startup_adapters_tests
    unit/app/platform_startup_adapters_test.cpp
    ${PROJECT_SOURCE_DIR}/src/platform/sdl3/legacy_command_line.cpp
    ${PROJECT_SOURCE_DIR}/src/platform/sdl3/single_instance.cpp
)
target_include_directories(
    openswd3_platform_startup_adapters_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
        ${PROJECT_SOURCE_DIR}/src/platform/sdl3
)
target_link_libraries(
    openswd3_platform_startup_adapters_tests
    PRIVATE openswd3_app
)

add_test(
    NAME platform.startup_adapters
    COMMAND openswd3_platform_startup_adapters_tests
)

add_executable(
    openswd3_runtime_platform_integration_tests
    unit/app/runtime_platform_integration_test.cpp
)
target_include_directories(
    openswd3_runtime_platform_integration_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_runtime_platform_integration_tests
    PRIVATE openswd3_app
)

add_test(
    NAME app.runtime_platform_integration
    COMMAND openswd3_runtime_platform_integration_tests
)

add_executable(
    openswd3_resource_io_legacy_lzo1x_tests
    unit/resource_io/legacy_lzo1x_test.cpp
)
target_include_directories(
    openswd3_resource_io_legacy_lzo1x_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_lzo1x_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_lzo1x
    COMMAND openswd3_resource_io_legacy_lzo1x_tests
)

add_executable(
    openswd3_resource_io_legacy_lzo1x_compression_tests
    unit/resource_io/legacy_lzo1x_compression_test.cpp
)
target_include_directories(
    openswd3_resource_io_legacy_lzo1x_compression_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_lzo1x_compression_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_lzo1x_compression
    COMMAND openswd3_resource_io_legacy_lzo1x_compression_tests
)

add_executable(
    openswd3_resource_io_data_directory_tests
    unit/resource_io/data_directory_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_data_directory_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_data_directory_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_data_directory_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.data_directory
    COMMAND openswd3_resource_io_data_directory_tests
)

add_executable(
    openswd3_resource_io_legacy_file_tests
    unit/resource_io/legacy_file_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_legacy_file_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_legacy_file_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_file_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_file
    COMMAND openswd3_resource_io_legacy_file_tests
)

add_executable(
    openswd3_resource_io_legacy_cm_cache_tests
    unit/resource_io/legacy_cm_cache_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_legacy_cm_cache_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_legacy_cm_cache_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_cm_cache_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_cm_cache
    COMMAND openswd3_resource_io_legacy_cm_cache_tests
)

add_executable(
    openswd3_resource_io_legacy_lmf_archive_tests
    unit/resource_io/legacy_lmf_archive_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_legacy_lmf_archive_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_legacy_lmf_archive_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_lmf_archive_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_lmf_archive
    COMMAND openswd3_resource_io_legacy_lmf_archive_tests
)

add_executable(
    openswd3_resource_io_legacy_memory_manager_tests
    unit/resource_io/legacy_memory_manager_test.cpp
)
target_include_directories(
    openswd3_resource_io_legacy_memory_manager_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_memory_manager_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_memory_manager
    COMMAND openswd3_resource_io_legacy_memory_manager_tests
)

add_executable(
    openswd3_resource_io_legacy_dbcs_text_buffer_tests
    unit/resource_io/legacy_dbcs_text_buffer_test.cpp
)
target_include_directories(
    openswd3_resource_io_legacy_dbcs_text_buffer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_dbcs_text_buffer_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_dbcs_text_buffer
    COMMAND openswd3_resource_io_legacy_dbcs_text_buffer_tests
)

add_executable(
    openswd3_resource_io_legacy_environment_tests
    unit/resource_io/legacy_environment_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_legacy_environment_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_legacy_environment_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_environment_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_environment
    COMMAND openswd3_resource_io_legacy_environment_tests
)

add_executable(
    openswd3_resource_io_legacy_resource_databases_tests
    unit/resource_io/legacy_resource_databases_test.cpp
)
target_compile_definitions(
    openswd3_resource_io_legacy_resource_databases_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_resource_io_legacy_resource_databases_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_resource_io_legacy_resource_databases_tests
    PRIVATE openswd3_resource_io
)

add_test(
    NAME resource_io.legacy_resource_databases
    COMMAND openswd3_resource_io_legacy_resource_databases_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../PATH.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../TALK1.DAT"
)
    add_test(
        NAME resource_io.legacy_resource_databases_real
        COMMAND
            openswd3_resource_io_legacy_resource_databases_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_input_time_rng_legacy_secondary_rng_tests
    unit/input_time_rng/legacy_secondary_rng_test.cpp
)
target_include_directories(
    openswd3_input_time_rng_legacy_secondary_rng_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_input_time_rng_legacy_secondary_rng_tests
    PRIVATE openswd3_input_time_rng
)

add_test(
    NAME input_time_rng.legacy_secondary_rng
    COMMAND openswd3_input_time_rng_legacy_secondary_rng_tests
)

add_executable(
    openswd3_input_time_rng_legacy_crt_rng_tests
    unit/input_time_rng/legacy_crt_rng_test.cpp
)
target_include_directories(
    openswd3_input_time_rng_legacy_crt_rng_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_input_time_rng_legacy_crt_rng_tests
    PRIVATE openswd3_input_time_rng
)

add_test(
    NAME input_time_rng.legacy_crt_rng
    COMMAND openswd3_input_time_rng_legacy_crt_rng_tests
)

add_executable(
    openswd3_input_time_rng_legacy_frame_clock_tests
    unit/input_time_rng/legacy_frame_clock_test.cpp
)
target_include_directories(
    openswd3_input_time_rng_legacy_frame_clock_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_input_time_rng_legacy_frame_clock_tests
    PRIVATE openswd3_input_time_rng
)

add_test(
    NAME input_time_rng.legacy_frame_clock
    COMMAND openswd3_input_time_rng_legacy_frame_clock_tests
)

add_executable(
    openswd3_input_time_rng_legacy_input_tests
    unit/input_time_rng/legacy_input_test.cpp
)
target_include_directories(
    openswd3_input_time_rng_legacy_input_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_input_time_rng_legacy_input_tests
    PRIVATE openswd3_input_time_rng
)

add_test(
    NAME input_time_rng.legacy_input
    COMMAND openswd3_input_time_rng_legacy_input_tests
)

add_executable(
    openswd3_input_time_rng_legacy_text_input_tests
    unit/input_time_rng/legacy_text_input_test.cpp
)
target_include_directories(
    openswd3_input_time_rng_legacy_text_input_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_input_time_rng_legacy_text_input_tests
    PRIVATE openswd3_input_time_rng openswd3_resource_io
)

add_test(
    NAME input_time_rng.legacy_text_input
    COMMAND openswd3_input_time_rng_legacy_text_input_tests
)

add_executable(
    openswd3_rendering_legacy_pixel_conversion_tests
    unit/rendering/legacy_pixel_conversion_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_pixel_conversion_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_pixel_conversion_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_pixel_conversion
    COMMAND openswd3_rendering_legacy_pixel_conversion_tests
)

add_executable(
    openswd3_rendering_legacy_image_command_stream_tests
    unit/rendering/legacy_image_command_stream_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_image_command_stream_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_image_command_stream_tests
    PRIVATE openswd3_rendering openswd3_resource_io
)

add_test(
    NAME rendering.legacy_image_command_stream
    COMMAND openswd3_rendering_legacy_image_command_stream_tests
)

if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME rendering.legacy_image_command_stream_asset
        COMMAND
            openswd3_rendering_legacy_image_command_stream_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_rendering_legacy_drawing_helpers_tests
    unit/rendering/legacy_drawing_helpers_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_drawing_helpers_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_drawing_helpers_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_drawing_helpers
    COMMAND openswd3_rendering_legacy_drawing_helpers_tests
)

add_executable(
    openswd3_rendering_legacy_action_renderers_tests
    unit/rendering/legacy_action_renderers_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_action_renderers_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_action_renderers_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_action_renderers
    COMMAND openswd3_rendering_legacy_action_renderers_tests
)

add_executable(
    openswd3_rendering_legacy_software_helpers_tests
    unit/rendering/legacy_software_helpers_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_software_helpers_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_software_helpers_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_software_helpers
    COMMAND openswd3_rendering_legacy_software_helpers_tests
)

add_executable(
    openswd3_rendering_legacy_frame_color_tests
    unit/rendering/legacy_frame_color_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_frame_color_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_frame_color_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_frame_color
    COMMAND openswd3_rendering_legacy_frame_color_tests
)

add_executable(
    openswd3_rendering_legacy_scaled_rle_writer_tests
    unit/rendering/legacy_scaled_rle_writer_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_scaled_rle_writer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_scaled_rle_writer_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_scaled_rle_writer
    COMMAND openswd3_rendering_legacy_scaled_rle_writer_tests
)

add_executable(
    openswd3_rendering_legacy_pause_overlay_tests
    unit/rendering/legacy_pause_overlay_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_pause_overlay_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_pause_overlay_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_pause_overlay
    COMMAND openswd3_rendering_legacy_pause_overlay_tests
)

add_executable(
    openswd3_rendering_legacy_timed_messages_tests
    unit/rendering/legacy_timed_messages_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_timed_messages_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_timed_messages_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_timed_messages
    COMMAND openswd3_rendering_legacy_timed_messages_tests
)

add_executable(
    openswd3_rendering_legacy_framebuffer_tests
    unit/rendering/legacy_framebuffer_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_framebuffer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_framebuffer_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_framebuffer
    COMMAND openswd3_rendering_legacy_framebuffer_tests
)

add_executable(
    openswd3_rendering_legacy_bmp_writer_tests
    unit/rendering/legacy_bmp_writer_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_bmp_writer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_bmp_writer_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_bmp_writer
    COMMAND openswd3_rendering_legacy_bmp_writer_tests
)

add_executable(
    openswd3_rendering_legacy_countdown_tests
    unit/rendering/legacy_countdown_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_countdown_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_countdown_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_countdown
    COMMAND openswd3_rendering_legacy_countdown_tests
)

add_executable(
    openswd3_rendering_legacy_rectangle_effect_tests
    unit/rendering/legacy_rectangle_effect_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_rectangle_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_rectangle_effect_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_rectangle_effect
    COMMAND openswd3_rendering_legacy_rectangle_effect_tests
)

add_executable(
    openswd3_rendering_legacy_tiled_frame_tests
    unit/rendering/legacy_tiled_frame_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_tiled_frame_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_tiled_frame_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_tiled_frame
    COMMAND openswd3_rendering_legacy_tiled_frame_tests
)

add_executable(
    openswd3_rendering_legacy_effect_panel_tests
    unit/rendering/legacy_effect_panel_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_effect_panel_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_effect_panel_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_effect_panel
    COMMAND openswd3_rendering_legacy_effect_panel_tests
)

add_executable(
    openswd3_rendering_legacy_presentation_tests
    unit/rendering/legacy_presentation_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_presentation_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_presentation_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_presentation
    COMMAND openswd3_rendering_legacy_presentation_tests
)

add_executable(
    openswd3_rendering_legacy_glyph_cache_tests
    unit/rendering/legacy_glyph_cache_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_glyph_cache_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_glyph_cache_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_glyph_cache
    COMMAND openswd3_rendering_legacy_glyph_cache_tests
)

add_executable(
    openswd3_rendering_legacy_glyph_atlas_tests
    unit/rendering/legacy_glyph_atlas_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_glyph_atlas_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_glyph_atlas_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_glyph_atlas
    COMMAND openswd3_rendering_legacy_glyph_atlas_tests
)

add_test(
    NAME rendering.legacy_glyph_atlas_asset
    COMMAND
        openswd3_rendering_legacy_glyph_atlas_tests
        "${PROJECT_SOURCE_DIR}/assets/fonts/legacy-glyph-atlas.bin"
)

add_executable(
    openswd3_rendering_legacy_glyph_writer_tests
    unit/rendering/legacy_glyph_writer_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_glyph_writer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_glyph_writer_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_glyph_writer
    COMMAND openswd3_rendering_legacy_glyph_writer_tests
)

add_executable(
    openswd3_rendering_legacy_text_renderer_tests
    unit/rendering/legacy_text_renderer_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_text_renderer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_text_renderer_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_text_renderer
    COMMAND openswd3_rendering_legacy_text_renderer_tests
)

add_executable(
    openswd3_rendering_legacy_text_renderer_runtime_tests
    unit/rendering/legacy_text_renderer_runtime_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_text_renderer_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_text_renderer_runtime_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_text_renderer_runtime
    COMMAND openswd3_rendering_legacy_text_renderer_runtime_tests
)

add_executable(
    openswd3_audio_video_legacy_audio_parameters_tests
    unit/audio_video/legacy_audio_parameters_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_audio_parameters_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_audio_parameters_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_audio_parameters
    COMMAND openswd3_audio_video_legacy_audio_parameters_tests
)

add_executable(
    openswd3_audio_video_legacy_audio_output_tests
    unit/audio_video/legacy_audio_output_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_audio_output_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_audio_output_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_audio_output
    COMMAND openswd3_audio_video_legacy_audio_output_tests
)

add_executable(
    openswd3_audio_video_legacy_snd_archive_tests
    unit/audio_video/legacy_snd_archive_test.cpp
)
target_compile_definitions(
    openswd3_audio_video_legacy_snd_archive_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_audio_video_legacy_snd_archive_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_snd_archive_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_snd_archive
    COMMAND openswd3_audio_video_legacy_snd_archive_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../all.snd")
    add_test(
        NAME audio_video.legacy_snd_archive_real
        COMMAND
            openswd3_audio_video_legacy_snd_archive_tests
            "${PROJECT_SOURCE_DIR}/../all.snd"
    )
endif()

add_executable(
    openswd3_audio_video_legacy_sample_manager_tests
    unit/audio_video/legacy_sample_manager_test.cpp
)
target_compile_definitions(
    openswd3_audio_video_legacy_sample_manager_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_audio_video_legacy_sample_manager_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_sample_manager_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_sample_manager
    COMMAND openswd3_audio_video_legacy_sample_manager_tests
)

add_executable(
    openswd3_audio_video_legacy_sample_commands_tests
    unit/audio_video/legacy_sample_commands_test.cpp
)
target_compile_definitions(
    openswd3_audio_video_legacy_sample_commands_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_audio_video_legacy_sample_commands_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_sample_commands_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_sample_commands
    COMMAND openswd3_audio_video_legacy_sample_commands_tests
)

add_executable(
    openswd3_audio_video_legacy_stream_manager_tests
    unit/audio_video/legacy_stream_manager_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_stream_manager_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_stream_manager_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_stream_manager
    COMMAND openswd3_audio_video_legacy_stream_manager_tests
)

add_executable(
    openswd3_audio_video_legacy_sequence_manager_tests
    unit/audio_video/legacy_sequence_manager_test.cpp
)
target_compile_definitions(
    openswd3_audio_video_legacy_sequence_manager_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_audio_video_legacy_sequence_manager_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_sequence_manager_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_sequence_manager
    COMMAND openswd3_audio_video_legacy_sequence_manager_tests
)

add_executable(
    openswd3_audio_video_legacy_audio_coordination_tests
    unit/audio_video/legacy_audio_coordination_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_audio_coordination_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_audio_coordination_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_audio_coordination
    COMMAND openswd3_audio_video_legacy_audio_coordination_tests
)

add_executable(
    openswd3_audio_video_legacy_world_music_tests
    unit/audio_video/legacy_world_music_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_world_music_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_world_music_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_world_music
    COMMAND openswd3_audio_video_legacy_world_music_tests
)

add_executable(
    openswd3_audio_video_legacy_media_acquisition_tests
    unit/audio_video/legacy_media_acquisition_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_media_acquisition_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_media_acquisition_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_media_acquisition
    COMMAND openswd3_audio_video_legacy_media_acquisition_tests
)

add_executable(
    openswd3_audio_video_legacy_video_tests
    unit/audio_video/legacy_video_test.cpp
)
target_include_directories(
    openswd3_audio_video_legacy_video_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_audio_video_legacy_video_tests
    PRIVATE openswd3_audio_video
)

add_test(
    NAME audio_video.legacy_video
    COMMAND openswd3_audio_video_legacy_video_tests
)

add_executable(
    openswd3_asset_runtime_legacy_asset_cache_limits_tests
    unit/asset_runtime/legacy_asset_cache_limits_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_asset_cache_limits_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_asset_cache_limits_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_asset_cache_limits
    COMMAND openswd3_asset_runtime_legacy_asset_cache_limits_tests
)

add_executable(
    openswd3_asset_runtime_legacy_tsw_archive_tests
    unit/asset_runtime/legacy_tsw_archive_test.cpp
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_tsw_archive_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_asset_runtime_legacy_tsw_archive_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_tsw_archive_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_tsw_archive
    COMMAND openswd3_asset_runtime_legacy_tsw_archive_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME asset_runtime.legacy_tsw_archive_real
        COMMAND
            openswd3_asset_runtime_legacy_tsw_archive_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_asset_runtime_legacy_tsw_runtime_tests
    unit/asset_runtime/legacy_tsw_runtime_test.cpp
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_tsw_runtime_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_asset_runtime_legacy_tsw_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_tsw_runtime_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_tsw_runtime
    COMMAND openswd3_asset_runtime_legacy_tsw_runtime_tests
)

add_executable(
    openswd3_asset_runtime_legacy_act_archive_tests
    unit/asset_runtime/legacy_act_archive_test.cpp
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_act_archive_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_asset_runtime_legacy_act_archive_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_act_archive_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_act_archive
    COMMAND openswd3_asset_runtime_legacy_act_archive_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act"
)
    add_test(
        NAME asset_runtime.legacy_act_archive_real
        COMMAND
            openswd3_asset_runtime_legacy_act_archive_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_asset_runtime_legacy_ani_archive_tests
    unit/asset_runtime/legacy_ani_archive_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_archive_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_ani_archive_tests
    PRIVATE
        OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_BINARY_DIR}/test-artifacts"
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_archive_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_archive
    COMMAND openswd3_asset_runtime_legacy_ani_archive_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../Video/Bd2Dh2.Ani")
    add_test(
        NAME asset_runtime.legacy_ani_archive_real
        COMMAND
            openswd3_asset_runtime_legacy_ani_archive_tests
            "${PROJECT_SOURCE_DIR}/../Video"
    )
endif()

add_executable(
    openswd3_asset_runtime_legacy_ani_activity_tests
    unit/asset_runtime/legacy_ani_activity_test.cpp
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_ani_activity_tests
    PRIVATE
        OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_BINARY_DIR}/test-artifacts"
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_activity_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_activity_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_activity
    COMMAND openswd3_asset_runtime_legacy_ani_activity_tests
)

add_executable(
    openswd3_asset_runtime_legacy_frame_deformation_tests
    unit/asset_runtime/legacy_frame_deformation_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_frame_deformation_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_frame_deformation_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_frame_deformation
    COMMAND openswd3_asset_runtime_legacy_frame_deformation_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_row_copy_effect_tests
    unit/asset_runtime/legacy_ani_row_copy_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_row_copy_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_row_copy_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_row_copy_effect
    COMMAND openswd3_asset_runtime_legacy_ani_row_copy_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_spark_effect_tests
    unit/asset_runtime/legacy_ani_spark_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_spark_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_spark_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_spark_effect
    COMMAND openswd3_asset_runtime_legacy_ani_spark_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_drift_effect_tests
    unit/asset_runtime/legacy_ani_drift_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_drift_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_drift_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_drift_effect
    COMMAND openswd3_asset_runtime_legacy_ani_drift_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_directional_effect_tests
    unit/asset_runtime/legacy_ani_directional_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_directional_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_directional_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_directional_effect
    COMMAND openswd3_asset_runtime_legacy_ani_directional_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_role_particle_effect_tests
    unit/asset_runtime/legacy_ani_role_particle_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_role_particle_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_role_particle_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_role_particle_effect
    COMMAND openswd3_asset_runtime_legacy_ani_role_particle_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_streak_effect_tests
    unit/asset_runtime/legacy_ani_streak_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_streak_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_streak_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_streak_effect
    COMMAND openswd3_asset_runtime_legacy_ani_streak_effect_tests
)

add_executable(
    openswd3_asset_runtime_legacy_ani_follower_effect_tests
    unit/asset_runtime/legacy_ani_follower_effect_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_ani_follower_effect_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_ani_follower_effect_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_ani_follower_effect
    COMMAND openswd3_asset_runtime_legacy_ani_follower_effect_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME asset_runtime.legacy_ani_follower_effect_real
        COMMAND
            openswd3_asset_runtime_legacy_ani_follower_effect_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
    add_test(
        NAME asset_runtime.legacy_ani_drift_effect_real
        COMMAND
            openswd3_asset_runtime_legacy_ani_drift_effect_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
    add_test(
        NAME asset_runtime.legacy_ani_directional_effect_real
        COMMAND
            openswd3_asset_runtime_legacy_ani_directional_effect_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
    add_test(
        NAME asset_runtime.legacy_ani_role_particle_effect_real
        COMMAND
            openswd3_asset_runtime_legacy_ani_role_particle_effect_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_asset_runtime_legacy_act_runtime_tests
    unit/asset_runtime/legacy_act_runtime_test.cpp
)
target_compile_definitions(
    openswd3_asset_runtime_legacy_act_runtime_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_asset_runtime_legacy_act_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_act_runtime_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_act_runtime
    COMMAND openswd3_asset_runtime_legacy_act_runtime_tests
)

add_executable(
    openswd3_asset_runtime_legacy_action_record_tests
    unit/asset_runtime/legacy_action_record_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_action_record_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_action_record_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_action_record
    COMMAND openswd3_asset_runtime_legacy_action_record_tests
)

add_executable(
    openswd3_world_map_legacy_role_spatial_query_tests
    unit/world_map/legacy_role_spatial_query_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_role_spatial_query_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_role_spatial_query_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_role_spatial_query
    COMMAND openswd3_world_map_legacy_role_spatial_query_tests
)

add_executable(
    openswd3_world_map_legacy_world_facing_role_query_tests
    unit/world_map/legacy_world_facing_role_query_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_facing_role_query_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_facing_role_query_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_facing_role_query
    COMMAND openswd3_world_map_legacy_world_facing_role_query_tests
)

add_executable(
    openswd3_world_map_legacy_world_facing_talk_tests
    unit/world_map/legacy_world_facing_talk_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_facing_talk_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_facing_talk_tests
    PRIVATE openswd3_world_map
)
add_test(
    NAME world_map.legacy_world_facing_talk
    COMMAND openswd3_world_map_legacy_world_facing_talk_tests
)

add_executable(
    openswd3_world_map_legacy_world_debug_hotkeys_tests
    unit/world_map/legacy_world_debug_hotkeys_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_debug_hotkeys_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_debug_hotkeys_tests
    PRIVATE openswd3_world_map
)
add_test(
    NAME world_map.legacy_world_debug_hotkeys
    COMMAND openswd3_world_map_legacy_world_debug_hotkeys_tests
)

add_executable(
    openswd3_world_map_legacy_world_player_control_tests
    unit/world_map/legacy_world_player_control_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_player_control_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_player_control_tests
    PRIVATE openswd3_world_map
)
add_test(
    NAME world_map.legacy_world_player_control
    COMMAND openswd3_world_map_legacy_world_player_control_tests
)

add_executable(
    openswd3_world_map_legacy_movement_collision_tests
    unit/world_map/legacy_movement_collision_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_movement_collision_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_movement_collision_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_movement_collision
    COMMAND openswd3_world_map_legacy_movement_collision_tests
)

add_executable(
    openswd3_world_map_legacy_world_map_business_tests
    unit/world_map/legacy_world_map_business_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_map_business_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_map_business_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_map_business
    COMMAND openswd3_world_map_legacy_world_map_business_tests
)

add_executable(
    openswd3_world_map_legacy_world_direction_input_tests
    unit/world_map/legacy_world_direction_input_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_direction_input_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_direction_input_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_direction_input
    COMMAND openswd3_world_map_legacy_world_direction_input_tests
)

add_executable(
    openswd3_world_map_legacy_world_direction_adjustment_tests
    unit/world_map/legacy_world_direction_adjustment_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_direction_adjustment_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_direction_adjustment_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_direction_adjustment
    COMMAND openswd3_world_map_legacy_world_direction_adjustment_tests
)

add_executable(
    openswd3_world_map_legacy_world_pathfinding_tests
    unit/world_map/legacy_world_pathfinding_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_pathfinding_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_pathfinding_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_pathfinding
    COMMAND openswd3_world_map_legacy_world_pathfinding_tests
)

add_executable(
    openswd3_world_map_legacy_world_path_requests_tests
    unit/world_map/legacy_world_path_requests_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_path_requests_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_path_requests_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_path_requests
    COMMAND openswd3_world_map_legacy_world_path_requests_tests
)

add_executable(
    openswd3_world_map_legacy_world_path_script_tests
    unit/world_map/legacy_world_path_script_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_path_script_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_path_script_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_path_script
    COMMAND openswd3_world_map_legacy_world_path_script_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../PATH.DAT"
)
    add_test(
        NAME world_map.legacy_world_path_script_real
        COMMAND
            openswd3_world_map_legacy_world_path_script_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
            "${PROJECT_SOURCE_DIR}/../PATH.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_facing_tests
    unit/world_map/legacy_world_facing_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_facing_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_facing_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_facing
    COMMAND openswd3_world_map_legacy_world_facing_tests
)

add_executable(
    openswd3_world_map_legacy_world_collision_talk_tests
    unit/world_map/legacy_world_collision_talk_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_collision_talk_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_collision_talk_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_collision_talk
    COMMAND openswd3_world_map_legacy_world_collision_talk_tests
)

add_executable(
    openswd3_world_map_legacy_world_interaction_tests
    unit/world_map/legacy_world_interaction_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_interaction_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_interaction_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_interaction
    COMMAND openswd3_world_map_legacy_world_interaction_tests
)

add_executable(
    openswd3_world_map_legacy_world_player_motion_tests
    unit/world_map/legacy_world_player_motion_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_player_motion_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_player_motion_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_player_motion
    COMMAND openswd3_world_map_legacy_world_player_motion_tests
)

add_executable(
    openswd3_world_map_legacy_world_map_role_paths_tests
    unit/world_map/legacy_world_map_role_paths_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_map_role_paths_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_map_role_paths_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_map_role_paths
    COMMAND openswd3_world_map_legacy_world_map_role_paths_tests
)

add_executable(
    openswd3_world_map_legacy_world_party_role_actions_tests
    unit/world_map/legacy_world_party_role_actions_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_party_role_actions_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_party_role_actions_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_party_role_actions
    COMMAND openswd3_world_map_legacy_world_party_role_actions_tests
)

add_executable(
    openswd3_world_map_legacy_world_player_post_frame_tests
    unit/world_map/legacy_world_player_post_frame_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_player_post_frame_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_player_post_frame_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_player_post_frame
    COMMAND openswd3_world_map_legacy_world_player_post_frame_tests
)

add_executable(
    openswd3_world_map_legacy_random_encounter_tests
    unit/world_map/legacy_random_encounter_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_random_encounter_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_random_encounter_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_random_encounter
    COMMAND openswd3_world_map_legacy_random_encounter_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT")
    add_test(
        NAME world_map.legacy_random_encounter_real
        COMMAND
            openswd3_world_map_legacy_random_encounter_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_role_post_materialization_tests
    unit/world_map/legacy_world_role_post_materialization_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_post_materialization_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_post_materialization_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_post_materialization
    COMMAND openswd3_world_map_legacy_world_role_post_materialization_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT")
    add_test(
        NAME world_map.legacy_world_role_post_materialization_real
        COMMAND
            openswd3_world_map_legacy_world_role_post_materialization_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_role_transfer_tests
    unit/world_map/legacy_world_role_transfer_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_transfer_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_transfer_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_transfer
    COMMAND openswd3_world_map_legacy_world_role_transfer_tests
)

add_executable(
    openswd3_world_map_legacy_world_role_map_update_tests
    unit/world_map/legacy_world_role_map_update_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_map_update_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_map_update_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_map_update
    COMMAND openswd3_world_map_legacy_world_role_map_update_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT")
    add_test(
        NAME world_map.legacy_world_role_map_update_real
        COMMAND
            openswd3_world_map_legacy_world_role_map_update_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_role_surface_occupancy_tests
    unit/world_map/legacy_world_role_surface_occupancy_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_surface_occupancy_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_surface_occupancy_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_surface_occupancy
    COMMAND openswd3_world_map_legacy_world_role_surface_occupancy_tests
)

add_executable(
    openswd3_world_map_legacy_maps_world_database_tests
    unit/world_map/legacy_maps_world_database_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_maps_world_database_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_maps_world_database_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_maps_world_database
    COMMAND openswd3_world_map_legacy_maps_world_database_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT")
    add_test(
        NAME world_map.legacy_maps_world_database_real
        COMMAND
            openswd3_world_map_legacy_maps_world_database_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_role_preload_tests
    unit/world_map/legacy_world_role_preload_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_preload_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_preload_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_preload
    COMMAND openswd3_world_map_legacy_world_role_preload_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../PATH.DAT"
)
    add_test(
        NAME world_map.legacy_world_role_preload_real
        COMMAND
            openswd3_world_map_legacy_world_role_preload_tests
            "${PROJECT_SOURCE_DIR}/../MAPS.DAT"
            "${PROJECT_SOURCE_DIR}/../PATH.DAT"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_runtime_session_tests
    unit/world_map/legacy_world_runtime_session_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_runtime_session_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_runtime_session_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_runtime_session_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_runtime_session
    COMMAND openswd3_world_map_legacy_world_runtime_session_tests
)

add_executable(
    openswd3_world_map_legacy_world_load_progress_tests
    unit/world_map/legacy_world_load_progress_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_load_progress_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_load_progress_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_load_progress
    COMMAND openswd3_world_map_legacy_world_load_progress_tests
)

add_executable(
    openswd3_world_map_legacy_world_special_frame_loader_tests
    unit/world_map/legacy_world_special_frame_loader_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_special_frame_loader_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_special_frame_loader_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_special_frame_loader_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_special_frame_loader
    COMMAND openswd3_world_map_legacy_world_special_frame_loader_tests
)

if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME world_map.legacy_world_runtime_session_real
        COMMAND
            openswd3_world_map_legacy_world_runtime_session_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_background_tests
    unit/world_map/legacy_world_background_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_background_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_background_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_background_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_background
    COMMAND openswd3_world_map_legacy_world_background_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_world_background_real
        COMMAND
            openswd3_world_map_legacy_world_background_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_flagged_roles_tests
    unit/world_map/legacy_world_flagged_roles_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_flagged_roles_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_flagged_roles_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_flagged_roles
    COMMAND openswd3_world_map_legacy_world_flagged_roles_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME world_map.legacy_world_flagged_roles_real
        COMMAND
            openswd3_world_map_legacy_world_flagged_roles_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_spatial_audio_tests
    unit/world_map/legacy_world_spatial_audio_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_spatial_audio_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_spatial_audio_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_spatial_audio
    COMMAND openswd3_world_map_legacy_world_spatial_audio_tests
)

add_executable(
    openswd3_world_map_legacy_world_role_lookup_tests
    unit/world_map/legacy_world_role_lookup_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_lookup_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_lookup_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_lookup
    COMMAND openswd3_world_map_legacy_world_role_lookup_tests
)

add_executable(
    openswd3_world_map_legacy_world_role_lifecycle_tests
    unit/world_map/legacy_world_role_lifecycle_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_role_lifecycle_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_role_lifecycle_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_role_lifecycle
    COMMAND openswd3_world_map_legacy_world_role_lifecycle_tests
)

add_executable(
    openswd3_world_map_legacy_world_item_lifecycle_tests
    unit/world_map/legacy_world_item_lifecycle_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_item_lifecycle_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_item_lifecycle_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_item_lifecycle
    COMMAND openswd3_world_map_legacy_world_item_lifecycle_tests
)

add_executable(
    openswd3_world_map_legacy_world_roles_tests
    unit/world_map/legacy_world_roles_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_roles_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_roles_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_roles
    COMMAND openswd3_world_map_legacy_world_roles_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME world_map.legacy_world_roles_real
        COMMAND
            openswd3_world_map_legacy_world_roles_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

if(TARGET openswd3_platform_sdl3_world_role)
    add_executable(
        openswd3_platform_sdl3_world_role_runtime_adapter_tests
        unit/app/world_role_runtime_adapter_test.cpp
    )
    target_compile_definitions(
        openswd3_platform_sdl3_world_role_runtime_adapter_tests
        PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
    )
    target_include_directories(
        openswd3_platform_sdl3_world_role_runtime_adapter_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
    )
    target_link_libraries(
        openswd3_platform_sdl3_world_role_runtime_adapter_tests
        PRIVATE openswd3_platform_sdl3_world_role
    )
    add_test(
        NAME platform_sdl3.world_role_runtime_adapter
        COMMAND openswd3_platform_sdl3_world_role_runtime_adapter_tests
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_frame_composition_tests
    unit/world_map/legacy_world_frame_composition_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_frame_composition_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_frame_composition_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_frame_composition_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_frame_composition
    COMMAND openswd3_world_map_legacy_world_frame_composition_tests
)

add_executable(
    openswd3_world_map_legacy_world_frame_coordinator_tests
    unit/world_map/legacy_world_frame_coordinator_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_frame_coordinator_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_frame_coordinator_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_frame_coordinator
    COMMAND openswd3_world_map_legacy_world_frame_coordinator_tests
)

add_executable(
    openswd3_world_map_legacy_world_camera_pan_tests
    unit/world_map/legacy_world_camera_pan_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_camera_pan_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_camera_pan_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_camera_pan
    COMMAND openswd3_world_map_legacy_world_camera_pan_tests
)

add_executable(
    openswd3_world_map_legacy_world_debug_overlay_tests
    unit/world_map/legacy_world_debug_overlay_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_debug_overlay_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_debug_overlay_tests
    PRIVATE openswd3_world_map
)
add_test(
    NAME world_map.legacy_world_debug_overlay
    COMMAND openswd3_world_map_legacy_world_debug_overlay_tests
)

add_executable(
    openswd3_world_map_legacy_picture_actions_tests
    unit/world_map/legacy_picture_actions_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_picture_actions_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_picture_actions_tests
    PRIVATE openswd3_world_map
)
add_test(
    NAME world_map.legacy_picture_actions
    COMMAND openswd3_world_map_legacy_picture_actions_tests
)

add_executable(
    openswd3_world_map_legacy_moving_actions_tests
    unit/world_map/legacy_moving_actions_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_moving_actions_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_moving_actions_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_moving_actions
    COMMAND openswd3_world_map_legacy_moving_actions_tests
)

add_executable(
    openswd3_world_map_legacy_role_head_actions_tests
    unit/world_map/legacy_role_head_actions_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_role_head_actions_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_role_head_actions_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_role_head_actions
    COMMAND openswd3_world_map_legacy_role_head_actions_tests
)

add_executable(
    openswd3_world_map_legacy_world_head_sign_actions_tests
    unit/world_map/legacy_world_head_sign_actions_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_head_sign_actions_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_head_sign_actions_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_head_sign_actions
    COMMAND openswd3_world_map_legacy_world_head_sign_actions_tests
)

if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_world_frame_composition_real
        COMMAND
            openswd3_world_map_legacy_world_frame_composition_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_interpolation_tests
    unit/world_map/legacy_world_interpolation_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_interpolation_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_interpolation_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_interpolation
    COMMAND openswd3_world_map_legacy_world_interpolation_tests
)

add_executable(
    openswd3_world_map_legacy_world_frame_runtime_tests
    unit/world_map/legacy_world_frame_runtime_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_frame_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_frame_runtime_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_frame_runtime
    COMMAND openswd3_world_map_legacy_world_frame_runtime_tests
)

add_executable(
    openswd3_world_map_legacy_world_transient_reset_tests
    unit/world_map/legacy_world_transient_reset_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_transient_reset_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_transient_reset_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_transient_reset
    COMMAND openswd3_world_map_legacy_world_transient_reset_tests
)

add_executable(
    openswd3_world_map_legacy_world_dialog_runtime_tests
    unit/world_map/legacy_world_dialog_runtime_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_dialog_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_dialog_runtime_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_dialog_runtime
    COMMAND openswd3_world_map_legacy_world_dialog_runtime_tests
)

add_executable(
    openswd3_world_map_legacy_world_story_vm_tests
    unit/world_map/legacy_world_story_vm_test.cpp
    unit/world_map/legacy_world_story_vm_test_01.cpp
    unit/world_map/legacy_world_story_vm_test_02.cpp
    unit/world_map/legacy_world_story_vm_test_03.cpp
    unit/world_map/legacy_world_story_vm_test_04.cpp
    unit/world_map/legacy_world_story_vm_test_05.cpp
    unit/world_map/legacy_world_story_vm_test_06.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_story_vm_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_story_vm_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_story_vm_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_story_vm
    COMMAND openswd3_world_map_legacy_world_story_vm_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../TALK1.DAT"
)
    add_test(
        NAME world_map.legacy_world_story_vm_real
        COMMAND
            openswd3_world_map_legacy_world_story_vm_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../MAPS.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../TALK1.DAT" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act"
)
    add_test(
        NAME world_map.legacy_world_story_vm_initial_session_real
        COMMAND
            openswd3_world_map_legacy_world_story_vm_tests
            "${PROJECT_SOURCE_DIR}/.."
            initial-session
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_cursor_tests
    unit/world_map/legacy_world_cursor_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_cursor_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_cursor_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_cursor
    COMMAND openswd3_world_map_legacy_world_cursor_tests
)

add_executable(
    openswd3_world_map_legacy_world_indexed_objects_tests
    unit/world_map/legacy_world_indexed_objects_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_indexed_objects_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_indexed_objects_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_indexed_objects
    COMMAND openswd3_world_map_legacy_world_indexed_objects_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME world_map.legacy_world_frame_runtime_real
        COMMAND
            openswd3_world_map_legacy_world_frame_runtime_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_frame_tail_tests
    unit/world_map/legacy_world_frame_tail_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_frame_tail_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_frame_tail_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_frame_tail
    COMMAND openswd3_world_map_legacy_world_frame_tail_tests
)

add_executable(
    openswd3_world_map_legacy_world_selection_scroll_tests
    unit/world_map/legacy_world_selection_scroll_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_selection_scroll_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_selection_scroll_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_selection_scroll
    COMMAND openswd3_world_map_legacy_world_selection_scroll_tests
)

add_executable(
    openswd3_world_map_legacy_cm_cache_runtime_tests
    unit/world_map/legacy_cm_cache_runtime_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_cm_cache_runtime_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_cm_cache_runtime_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_cm_cache_runtime
    COMMAND openswd3_world_map_legacy_cm_cache_runtime_tests
)

add_executable(
    openswd3_world_map_legacy_cm_cache_generator_tests
    unit/world_map/legacy_cm_cache_generator_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_cm_cache_generator_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_cm_cache_generator_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_cm_cache_generator_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_cm_cache_generator
    COMMAND openswd3_world_map_legacy_cm_cache_generator_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_cm_cache_generator_real
        COMMAND
            openswd3_world_map_legacy_cm_cache_generator_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_world_map_legacy_cm_cache_loader_tests
    unit/world_map/legacy_cm_cache_loader_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_cm_cache_loader_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_cm_cache_loader_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_cm_cache_loader_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_cm_cache_loader
    COMMAND openswd3_world_map_legacy_cm_cache_loader_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_cm_cache_loader_real
        COMMAND
            openswd3_world_map_legacy_cm_cache_loader_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_map_session_tests
    unit/world_map/legacy_world_map_session_test.cpp
)
target_include_directories(
    openswd3_world_map_legacy_world_map_session_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_map_session_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_map_session
    COMMAND openswd3_world_map_legacy_world_map_session_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_world_map_session_real
        COMMAND
            openswd3_world_map_legacy_world_map_session_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_world_map_legacy_world_render_session_tests
    unit/world_map/legacy_world_render_session_test.cpp
)
target_compile_definitions(
    openswd3_world_map_legacy_world_render_session_tests
    PRIVATE OPENSWD3_TEST_ARTIFACT_ROOT="${CMAKE_CURRENT_BINARY_DIR}/runtime"
)
target_include_directories(
    openswd3_world_map_legacy_world_render_session_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_world_map_legacy_world_render_session_tests
    PRIVATE openswd3_world_map
)

add_test(
    NAME world_map.legacy_world_render_session
    COMMAND openswd3_world_map_legacy_world_render_session_tests
)
if(EXISTS "${PROJECT_SOURCE_DIR}/../huge.lmf")
    add_test(
        NAME world_map.legacy_world_render_session_real
        COMMAND
            openswd3_world_map_legacy_world_render_session_tests
            "${PROJECT_SOURCE_DIR}/../huge.lmf"
    )
endif()

add_executable(
    openswd3_asset_runtime_legacy_action_draw_bridge_tests
    unit/asset_runtime/legacy_action_draw_bridge_test.cpp
)
target_include_directories(
    openswd3_asset_runtime_legacy_action_draw_bridge_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_asset_runtime_legacy_action_draw_bridge_tests
    PRIVATE openswd3_asset_runtime
)

add_test(
    NAME asset_runtime.legacy_action_draw_bridge
    COMMAND openswd3_asset_runtime_legacy_action_draw_bridge_tests
)
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.tsw" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.tsw"
)
    add_test(
        NAME asset_runtime.legacy_action_draw_bridge_real
        COMMAND
            openswd3_asset_runtime_legacy_action_draw_bridge_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()
if(
    EXISTS "${PROJECT_SOURCE_DIR}/../all_char.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_item.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_magic.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_sys.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map1.act" AND
    EXISTS "${PROJECT_SOURCE_DIR}/../all_map2.act"
)
    add_test(
        NAME asset_runtime.legacy_action_record_real
        COMMAND
            openswd3_asset_runtime_legacy_action_record_tests
            "${PROJECT_SOURCE_DIR}/.."
    )
endif()

add_executable(
    openswd3_rendering_legacy_formatted_text_tests
    unit/rendering/legacy_formatted_text_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_formatted_text_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_formatted_text_tests
    PRIVATE openswd3_rendering
)

add_test(
    NAME rendering.legacy_formatted_text
    COMMAND openswd3_rendering_legacy_formatted_text_tests
)

add_executable(
    openswd3_rendering_legacy_blitter_tests
    unit/rendering/legacy_blitter_test.cpp
)
target_include_directories(
    openswd3_rendering_legacy_blitter_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
)
target_link_libraries(
    openswd3_rendering_legacy_blitter_tests
    PRIVATE openswd3_rendering openswd3_resource_io
)

add_test(
    NAME rendering.legacy_blitter
    COMMAND openswd3_rendering_legacy_blitter_tests
)

if(TARGET SDL3::SDL3)
    add_executable(
        openswd3_platform_legacy_sample_backend_sdl3_tests
        unit/audio_video/legacy_sample_backend_sdl3_test.cpp
    )
    target_include_directories(
        openswd3_platform_legacy_sample_backend_sdl3_tests
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support
    )
    target_link_libraries(
        openswd3_platform_legacy_sample_backend_sdl3_tests
        PRIVATE
            openswd3_media_ffmpeg
            openswd3_platform_sdl3_audio
    )
    get_property(
        OPENSWD3_FFMPEG_RUNTIME_FILES
        GLOBAL PROPERTY OPENSWD3_FFMPEG_RUNTIME_FILES
    )
    get_property(
        OPENSWD3_SDL_RUNTIME_FILES
        GLOBAL PROPERTY OPENSWD3_SDL_RUNTIME_FILES
    )
    get_property(
        OPENSWD3_FFMPEG_LICENSE_FILE
        GLOBAL PROPERTY OPENSWD3_FFMPEG_LICENSE_FILE
    )
    add_custom_command(
        TARGET openswd3_platform_legacy_sample_backend_sdl3_tests
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:openswd3_media_ffmpeg>"
            ${OPENSWD3_FFMPEG_RUNTIME_FILES}
            ${OPENSWD3_SDL_RUNTIME_FILES}
            "${OPENSWD3_FFMPEG_LICENSE_FILE}"
            "$<TARGET_FILE_DIR:openswd3_platform_legacy_sample_backend_sdl3_tests>"
    )
    openswd3_remove_split_ffmpeg_runtime_files(
        openswd3_platform_legacy_sample_backend_sdl3_tests
    )

    add_test(
        NAME platform.legacy_sample_backend_sdl3
        COMMAND openswd3_platform_legacy_sample_backend_sdl3_tests
    )
    if(EXISTS "${PROJECT_SOURCE_DIR}/../all.snd")
        add_test(
            NAME platform.legacy_sample_backend_sdl3_real
            COMMAND
                openswd3_platform_legacy_sample_backend_sdl3_tests
                "${PROJECT_SOURCE_DIR}/../all.snd"
                "${PROJECT_SOURCE_DIR}/.."
        )
    endif()

    add_executable(
        openswd3_platform_keyboard_snapshot_sdl3_tests
        unit/app/keyboard_snapshot_sdl3_test.cpp
        ${PROJECT_SOURCE_DIR}/src/platform/sdl3/keyboard_snapshot_sdl3.cpp
    )
    target_include_directories(
        openswd3_platform_keyboard_snapshot_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/support
            ${PROJECT_SOURCE_DIR}/src/platform/sdl3
    )
    target_link_libraries(
        openswd3_platform_keyboard_snapshot_sdl3_tests
        PRIVATE openswd3_input_time_rng SDL3::SDL3
    )

    add_test(
        NAME platform.keyboard_snapshot_sdl3
        COMMAND openswd3_platform_keyboard_snapshot_sdl3_tests
    )

    add_executable(
        openswd3_platform_event_translation_sdl3_tests
        unit/app/event_translation_sdl3_test.cpp
        ${PROJECT_SOURCE_DIR}/src/platform/sdl3/event_translation.cpp
    )
    target_include_directories(
        openswd3_platform_event_translation_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/support
            ${PROJECT_SOURCE_DIR}/src/platform/sdl3
    )
    target_link_libraries(
        openswd3_platform_event_translation_sdl3_tests
        PRIVATE openswd3_app SDL3::SDL3
    )

    add_test(
        NAME platform.event_translation_sdl3
        COMMAND openswd3_platform_event_translation_sdl3_tests
    )

    add_executable(
        openswd3_platform_mouse_sdl3_tests
        unit/app/mouse_sdl3_test.cpp
        ${PROJECT_SOURCE_DIR}/src/platform/sdl3/mouse_sdl3.cpp
    )
    target_include_directories(
        openswd3_platform_mouse_sdl3_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/support
            ${PROJECT_SOURCE_DIR}/src/platform/sdl3
    )
    target_link_libraries(
        openswd3_platform_mouse_sdl3_tests
        PRIVATE openswd3_input_time_rng SDL3::SDL3
    )

    add_test(
        NAME platform.mouse_sdl3
        COMMAND openswd3_platform_mouse_sdl3_tests
    )
endif()

# CTest may launch the synthetic and real-asset invocations of one executable
# in the same clock tick. Their per-process TestTree names intentionally share
# a prefix, so serialize only invocations of the same binary while preserving
# parallel execution across unrelated test executables.
function(openswd3_lock_shared_test_executable lock_name)
    set(existing_tests)
    foreach(test_name IN LISTS ARGN)
        if(TEST "${test_name}")
            list(APPEND existing_tests "${test_name}")
        endif()
    endforeach()
    if(existing_tests)
        set_tests_properties(
            ${existing_tests}
            PROPERTIES RESOURCE_LOCK "${lock_name}"
        )
    endif()
endfunction()

openswd3_lock_shared_test_executable(
    act_archive
    asset_runtime.legacy_act_archive
    asset_runtime.legacy_act_archive_real
)
openswd3_lock_shared_test_executable(
    action_draw_bridge
    asset_runtime.legacy_action_draw_bridge
    asset_runtime.legacy_action_draw_bridge_real
)
openswd3_lock_shared_test_executable(
    action_record
    asset_runtime.legacy_action_record
    asset_runtime.legacy_action_record_real
)
openswd3_lock_shared_test_executable(
    ani_archive
    asset_runtime.legacy_ani_archive
    asset_runtime.legacy_ani_archive_real
)
openswd3_lock_shared_test_executable(
    ani_directional_effect
    asset_runtime.legacy_ani_directional_effect
    asset_runtime.legacy_ani_directional_effect_real
)
openswd3_lock_shared_test_executable(
    ani_drift_effect
    asset_runtime.legacy_ani_drift_effect
    asset_runtime.legacy_ani_drift_effect_real
)
openswd3_lock_shared_test_executable(
    ani_follower_effect
    asset_runtime.legacy_ani_follower_effect
    asset_runtime.legacy_ani_follower_effect_real
)
openswd3_lock_shared_test_executable(
    ani_role_particle_effect
    asset_runtime.legacy_ani_role_particle_effect
    asset_runtime.legacy_ani_role_particle_effect_real
)
openswd3_lock_shared_test_executable(
    tsw_archive
    asset_runtime.legacy_tsw_archive
    asset_runtime.legacy_tsw_archive_real
)
openswd3_lock_shared_test_executable(
    snd_archive
    audio_video.legacy_snd_archive
    audio_video.legacy_snd_archive_real
)
openswd3_lock_shared_test_executable(
    sample_backend_sdl3
    platform.legacy_sample_backend_sdl3
    platform.legacy_sample_backend_sdl3_real
)
openswd3_lock_shared_test_executable(
    glyph_atlas
    rendering.legacy_glyph_atlas
    rendering.legacy_glyph_atlas_asset
)
openswd3_lock_shared_test_executable(
    image_command_stream
    rendering.legacy_image_command_stream
    rendering.legacy_image_command_stream_asset
)
openswd3_lock_shared_test_executable(
    resource_databases
    resource_io.legacy_resource_databases
    resource_io.legacy_resource_databases_real
)
openswd3_lock_shared_test_executable(
    cm_cache_generator
    world_map.legacy_cm_cache_generator
    world_map.legacy_cm_cache_generator_real
)
openswd3_lock_shared_test_executable(
    cm_cache_loader
    world_map.legacy_cm_cache_loader
    world_map.legacy_cm_cache_loader_real
)
openswd3_lock_shared_test_executable(
    maps_world_database
    world_map.legacy_maps_world_database
    world_map.legacy_maps_world_database_real
)
openswd3_lock_shared_test_executable(
    random_encounter
    world_map.legacy_random_encounter
    world_map.legacy_random_encounter_real
)
openswd3_lock_shared_test_executable(
    world_background
    world_map.legacy_world_background
    world_map.legacy_world_background_real
)
openswd3_lock_shared_test_executable(
    world_flagged_roles
    world_map.legacy_world_flagged_roles
    world_map.legacy_world_flagged_roles_real
)
openswd3_lock_shared_test_executable(
    world_frame_composition
    world_map.legacy_world_frame_composition
    world_map.legacy_world_frame_composition_real
)
openswd3_lock_shared_test_executable(
    world_frame_runtime
    world_map.legacy_world_frame_runtime
    world_map.legacy_world_frame_runtime_real
)
openswd3_lock_shared_test_executable(
    world_map_session
    world_map.legacy_world_map_session
    world_map.legacy_world_map_session_real
)
openswd3_lock_shared_test_executable(
    world_path_script
    world_map.legacy_world_path_script
    world_map.legacy_world_path_script_real
)
openswd3_lock_shared_test_executable(
    world_render_session
    world_map.legacy_world_render_session
    world_map.legacy_world_render_session_real
)
openswd3_lock_shared_test_executable(
    world_role_map_update
    world_map.legacy_world_role_map_update
    world_map.legacy_world_role_map_update_real
)
openswd3_lock_shared_test_executable(
    world_role_post_materialization
    world_map.legacy_world_role_post_materialization
    world_map.legacy_world_role_post_materialization_real
)
openswd3_lock_shared_test_executable(
    world_role_preload
    world_map.legacy_world_role_preload
    world_map.legacy_world_role_preload_real
)
openswd3_lock_shared_test_executable(
    world_roles
    world_map.legacy_world_roles
    world_map.legacy_world_roles_real
)
openswd3_lock_shared_test_executable(
    world_runtime_session
    world_map.legacy_world_runtime_session
    world_map.legacy_world_runtime_session_real
)
openswd3_lock_shared_test_executable(
    world_story_vm
    world_map.legacy_world_story_vm
    world_map.legacy_world_story_vm_real
    world_map.legacy_world_story_vm_initial_session_real
)

# Legacy read-only databases and several archives intentionally retain the
# original Win32 exclusive sharing mode. Real-asset tests therefore share one
# additional lock; synthetic tests and unrelated binaries remain parallel.
set(openswd3_shared_real_asset_tests
    assets.startup_ui
    battle.legacy_battle_setup
    special_modes.legacy_initial_menu
    resource_io.legacy_resource_databases_real
    rendering.legacy_image_command_stream_asset
    rendering.legacy_glyph_atlas_asset
    audio_video.legacy_snd_archive_real
    asset_runtime.legacy_tsw_archive_real
    asset_runtime.legacy_act_archive_real
    asset_runtime.legacy_ani_archive_real
    asset_runtime.legacy_ani_follower_effect_real
    asset_runtime.legacy_ani_drift_effect_real
    asset_runtime.legacy_ani_directional_effect_real
    asset_runtime.legacy_ani_role_particle_effect_real
    world_map.legacy_world_path_script_real
    world_map.legacy_random_encounter_real
    world_map.legacy_world_role_post_materialization_real
    world_map.legacy_world_role_map_update_real
    world_map.legacy_maps_world_database_real
    world_map.legacy_world_role_preload_real
    world_map.legacy_world_runtime_session_real
    world_map.legacy_world_background_real
    world_map.legacy_world_flagged_roles_real
    world_map.legacy_world_roles_real
    world_map.legacy_world_frame_composition_real
    world_map.legacy_world_story_vm_real
    world_map.legacy_world_story_vm_initial_session_real
    world_map.legacy_world_frame_runtime_real
    world_map.legacy_cm_cache_generator_real
    world_map.legacy_cm_cache_loader_real
    world_map.legacy_world_map_session_real
    world_map.legacy_world_render_session_real
    asset_runtime.legacy_action_draw_bridge_real
    asset_runtime.legacy_action_record_real
    platform.legacy_sample_backend_sdl3_real
)
foreach(test_name IN LISTS openswd3_shared_real_asset_tests)
    if(TEST "${test_name}")
        set_property(
            TEST "${test_name}"
            APPEND PROPERTY RESOURCE_LOCK legacy_real_assets
        )
    endif()
endforeach()
