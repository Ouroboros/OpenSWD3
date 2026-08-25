include(${PROJECT_SOURCE_DIR}/cmake/OpenSWD3FFmpeg.cmake)
openswd3_configure_ffmpeg()

add_library(
    openswd3_media_ffmpeg_objects
    OBJECT
        legacy_ffmpeg_backends.cpp
)
target_compile_definitions(
    openswd3_media_ffmpeg_objects
    PRIVATE OPENSWD3_MEDIA_FFMPEG_BUILD
)
target_include_directories(
    openswd3_media_ffmpeg_objects
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(
    openswd3_media_ffmpeg_objects
    PRIVATE
        OpenSWD3::FFmpeg
        SDL3::SDL3
        openswd3_project_options
        openswd3_project_warnings
)
set_target_properties(
    openswd3_media_ffmpeg_objects
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

add_library(
    openswd3_media_ffmpeg
    SHARED
        $<TARGET_OBJECTS:openswd3_media_ffmpeg_objects>
)
target_include_directories(
    openswd3_media_ffmpeg
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(
    openswd3_media_ffmpeg
    PRIVATE
        OpenSWD3::FFmpeg
        SDL3::SDL3
)
set_target_properties(
    openswd3_media_ffmpeg
    PROPERTIES
        OUTPUT_NAME openswd3_ffmpeg
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
        BUILD_WITH_INSTALL_RPATH TRUE
)
if(WIN32)
    target_link_options(
        openswd3_media_ffmpeg
        PRIVATE "LINKER:/OPT:REF" "LINKER:/OPT:ICF" "LINKER:/Brepro"
    )
else()
    target_link_options(
        openswd3_media_ffmpeg
        PRIVATE "LINKER:--gc-sections" "LINKER:--exclude-libs,ALL"
    )
endif()

get_property(
    OPENSWD3_FFMPEG_STATIC_ARCHIVES
    GLOBAL PROPERTY OPENSWD3_FFMPEG_STATIC_ARCHIVES
)
if(WIN32)
    set(_openswd3_ffmpeg_relink_platform windows)
else()
    set(_openswd3_ffmpeg_relink_platform linux)
endif()
set(
    _openswd3_ffmpeg_relink_root
    "${PROJECT_BINARY_DIR}/compliance/$<CONFIG>/openswd3_ffmpeg-${_openswd3_ffmpeg_relink_platform}"
)
add_custom_command(
    TARGET openswd3_media_ffmpeg
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E rm -rf
        "${_openswd3_ffmpeg_relink_root}"
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${_openswd3_ffmpeg_relink_root}/source"
        "${_openswd3_ffmpeg_relink_root}/ffmpeg/lib"
        "${_openswd3_ffmpeg_relink_root}/objects"
        "${_openswd3_ffmpeg_relink_root}/third-party"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/build/dependencies/ffmpeg/9.0/source/ffmpeg-9.0.tar.xz"
        "${PROJECT_SOURCE_DIR}/build/dependencies/ffmpeg/9.0/source/ffmpeg-9.0.tar.xz.asc"
        "${_openswd3_ffmpeg_relink_root}/source"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${OPENSWD3_FFMPEG_STATIC_ARCHIVES}
        "${_openswd3_ffmpeg_relink_root}/ffmpeg/lib"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${OPENSWD3_FFMPEG_ROOT}/BUILDINFO.txt"
        "${OPENSWD3_FFMPEG_ROOT}/LICENSE.txt"
        "${_openswd3_ffmpeg_relink_root}/ffmpeg"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_OBJECTS:openswd3_media_ffmpeg_objects>
        "${_openswd3_ffmpeg_relink_root}/objects"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_LINKER_FILE:SDL3::SDL3>"
        "${_openswd3_ffmpeg_relink_root}/third-party"
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/ffmpeg-devel.asc"
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/build-minimal.sh"
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/verify-minimal.py"
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/relink.py"
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/SOURCE.md"
        "${PROJECT_SOURCE_DIR}/dependencies/ffmpeg/9.0/RELINKING.md"
        "${_openswd3_ffmpeg_relink_root}"
    COMMAND_EXPAND_LISTS
    VERBATIM
)

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

add_library(
    openswd3_platform_sdl3_world_role
    STATIC
        world_role_runtime_adapter.cpp
)
target_include_directories(
    openswd3_platform_sdl3_world_role
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(
    openswd3_platform_sdl3_world_role
    PUBLIC
        openswd3_audio_video
        openswd3_world_map
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
    text_encoding_sdl3.cpp
)
target_link_libraries(
    openswd3
    PRIVATE
        openswd3_app
        openswd3_battle
        openswd3_media_ffmpeg
        openswd3_platform_sdl3_audio
        openswd3_platform_sdl3_world_role
        openswd3_input_time_rng
        openswd3_rendering
        openswd3_resource_io
        openswd3_special_modes
        openswd3_world_map
        SDL3::SDL3
)

if(NOT WIN32)
    find_package(Iconv REQUIRED)
    target_link_libraries(openswd3 PRIVATE Iconv::Iconv)
endif()

if(WIN32)
    set(OPENSWD3_SDL_RUNTIME_FILES "$<TARGET_FILE:SDL3::SDL3>")
else()
    set(
        OPENSWD3_SDL_RUNTIME_FILES
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_SONAME_FILE:SDL3::SDL3>"
    )
endif()
set_property(
    GLOBAL PROPERTY OPENSWD3_SDL_RUNTIME_FILES
    "${OPENSWD3_SDL_RUNTIME_FILES}"
)

get_property(
    OPENSWD3_FFMPEG_RUNTIME_FILES
    GLOBAL PROPERTY OPENSWD3_FFMPEG_RUNTIME_FILES
)
get_property(
    OPENSWD3_FFMPEG_LICENSE_FILE
    GLOBAL PROPERTY OPENSWD3_FFMPEG_LICENSE_FILE
)

add_custom_command(
    TARGET openswd3
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${OPENSWD3_FFMPEG_RUNTIME_FILES}
        ${OPENSWD3_SDL_RUNTIME_FILES}
        "${OPENSWD3_FFMPEG_LICENSE_FILE}"
        "$<TARGET_FILE_DIR:openswd3>"
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
openswd3_remove_split_ffmpeg_runtime_files(openswd3)
