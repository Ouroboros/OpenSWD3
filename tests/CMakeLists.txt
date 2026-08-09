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
        PRIVATE openswd3_platform_sdl3_audio
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
