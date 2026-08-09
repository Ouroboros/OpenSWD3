add_library(
    openswd3_platform_sdl3_audio
    STATIC
        legacy_sample_backend_sdl3.cpp
)
target_include_directories(
    openswd3_platform_sdl3_audio
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    openswd3_platform_sdl3_audio
    PUBLIC
        openswd3_audio_video
        SDL3::SDL3
)

add_executable(
    openswd3
    event_translation.cpp
    external_launch_sdl3.cpp
    external_uri.cpp
    keyboard_snapshot_sdl3.cpp
    legacy_command_line.cpp
    main.cpp
    mouse_sdl3.cpp
    single_instance.cpp
    startup_dialog_sdl3.cpp
)
target_link_libraries(
    openswd3
    PRIVATE
        openswd3_app
        openswd3_platform_sdl3_audio
        openswd3_input_time_rng
        openswd3_rendering
        openswd3_resource_io
        SDL3::SDL3
)

add_custom_command(
    TARGET openswd3
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets/ui/startup"
        "$<TARGET_FILE_DIR:openswd3>/assets/ui/startup"
    COMMAND
        ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets/fonts"
        "$<TARGET_FILE_DIR:openswd3>/assets/fonts"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/config/openswd3.example.toml"
        "$<TARGET_FILE_DIR:openswd3>/openswd3.example.toml"
)
