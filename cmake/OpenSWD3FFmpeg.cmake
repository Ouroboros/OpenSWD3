function(openswd3_configure_ffmpeg)
    if(TARGET OpenSWD3::avformat)
        return()
    endif()

    if(WIN32)
        set(_platform_directory "windows-x64")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_platform_directory "linux-x64")
    else()
        message(FATAL_ERROR "The OpenSWD3 FFmpeg backend supports Windows and Linux")
    endif()

    set(
        OPENSWD3_FFMPEG_ROOT
        "${PROJECT_SOURCE_DIR}/build/dependencies/ffmpeg/9.0/${_platform_directory}"
        CACHE PATH
        "Extracted BtbN FFmpeg 9.0 lgpl-shared package root"
    )

    set(_include_directory "${OPENSWD3_FFMPEG_ROOT}/include")
    if(NOT EXISTS "${_include_directory}/libavformat/avformat.h")
        message(
            FATAL_ERROR
            "FFmpeg 9.0 lgpl-shared headers were not found at "
            "${OPENSWD3_FFMPEG_ROOT}. See dependencies/ffmpeg/9.0/SOURCE.md"
        )
    endif()

    set(_components avformat avcodec avutil swresample swscale)
    set(_runtime_files)
    foreach(_component IN LISTS _components)
        set(_target "openswd3_ffmpeg_${_component}")
        add_library(${_target} SHARED IMPORTED GLOBAL)
        set_target_properties(
            ${_target}
            PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${_include_directory}"
        )

        if(WIN32)
            if(_component STREQUAL "avformat")
                set(_runtime_name "avformat-63.dll")
            elseif(_component STREQUAL "avcodec")
                set(_runtime_name "avcodec-63.dll")
            elseif(_component STREQUAL "avutil")
                set(_runtime_name "avutil-61.dll")
            elseif(_component STREQUAL "swresample")
                set(_runtime_name "swresample-7.dll")
            elseif(_component STREQUAL "swscale")
                set(_runtime_name "swscale-10.dll")
            endif()
            set(_runtime "${OPENSWD3_FFMPEG_ROOT}/bin/${_runtime_name}")
            set(_import "${OPENSWD3_FFMPEG_ROOT}/lib/${_component}.lib")
            if(NOT EXISTS "${_runtime}" OR NOT EXISTS "${_import}")
                message(FATAL_ERROR "Incomplete FFmpeg component: ${_component}")
            endif()
            set_target_properties(
                ${_target}
                PROPERTIES
                    IMPORTED_IMPLIB "${_import}"
                    IMPORTED_LOCATION "${_runtime}"
            )
        else()
            if(_component STREQUAL "avformat")
                set(_runtime_name "libavformat.so.63")
            elseif(_component STREQUAL "avcodec")
                set(_runtime_name "libavcodec.so.63")
            elseif(_component STREQUAL "avutil")
                set(_runtime_name "libavutil.so.61")
            elseif(_component STREQUAL "swresample")
                set(_runtime_name "libswresample.so.7")
            elseif(_component STREQUAL "swscale")
                set(_runtime_name "libswscale.so.10")
            endif()
            set(_runtime "${OPENSWD3_FFMPEG_ROOT}/lib/${_runtime_name}")
            if(NOT EXISTS "${_runtime}")
                message(FATAL_ERROR "Incomplete FFmpeg component: ${_component}")
            endif()
            set_target_properties(
                ${_target}
                PROPERTIES IMPORTED_LOCATION "${_runtime}"
            )
        endif()
        list(APPEND _runtime_files "${_runtime}")
    endforeach()

    add_library(OpenSWD3::avformat ALIAS openswd3_ffmpeg_avformat)
    add_library(OpenSWD3::avcodec ALIAS openswd3_ffmpeg_avcodec)
    add_library(OpenSWD3::avutil ALIAS openswd3_ffmpeg_avutil)
    add_library(OpenSWD3::swresample ALIAS openswd3_ffmpeg_swresample)
    add_library(OpenSWD3::swscale ALIAS openswd3_ffmpeg_swscale)

    set_property(GLOBAL PROPERTY OPENSWD3_FFMPEG_RUNTIME_FILES "${_runtime_files}")
    set_property(
        GLOBAL PROPERTY OPENSWD3_FFMPEG_LICENSE_FILE
        "${OPENSWD3_FFMPEG_ROOT}/LICENSE.txt"
    )
endfunction()
