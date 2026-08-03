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
