function(openswd3_configure_ffmpeg)
    if(TARGET OpenSWD3::FFmpeg)
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
        "${PROJECT_SOURCE_DIR}/build/dependencies/ffmpeg/9.0/self-built/${_platform_directory}"
        CACHE PATH
        "Self-built FFmpeg 9.0 minimal LGPL static archive package root"
    )

    set(_include_directory "${OPENSWD3_FFMPEG_ROOT}/include")
    if(NOT EXISTS "${_include_directory}/libavformat/avformat.h")
        message(
            FATAL_ERROR
            "FFmpeg 9.0 static headers were not found at "
            "${OPENSWD3_FFMPEG_ROOT}. See dependencies/ffmpeg/9.0/SOURCE.md"
        )
    endif()

    set(_components avformat avcodec avutil swresample swscale)
    foreach(_component IN LISTS _components)
        set(_target "openswd3_ffmpeg_${_component}")
        if(WIN32)
            set(_archive "${OPENSWD3_FFMPEG_ROOT}/lib/${_component}.lib")
        else()
            set(_archive "${OPENSWD3_FFMPEG_ROOT}/lib/lib${_component}.a")
        endif()
        if(NOT EXISTS "${_archive}")
            message(FATAL_ERROR "Incomplete static FFmpeg component: ${_component}")
        endif()

        list(APPEND _static_archives "${_archive}")
        add_library(${_target} STATIC IMPORTED GLOBAL)
        set_target_properties(
            ${_target}
            PROPERTIES
                IMPORTED_LOCATION "${_archive}"
                INTERFACE_INCLUDE_DIRECTORIES "${_include_directory}"
        )
    endforeach()

    add_library(OpenSWD3::avformat ALIAS openswd3_ffmpeg_avformat)
    add_library(OpenSWD3::avcodec ALIAS openswd3_ffmpeg_avcodec)
    add_library(OpenSWD3::avutil ALIAS openswd3_ffmpeg_avutil)
    add_library(OpenSWD3::swresample ALIAS openswd3_ffmpeg_swresample)
    add_library(OpenSWD3::swscale ALIAS openswd3_ffmpeg_swscale)
    set_property(
        GLOBAL PROPERTY OPENSWD3_FFMPEG_STATIC_ARCHIVES
        "${_static_archives}"
    )

    add_library(openswd3_ffmpeg_static_bundle INTERFACE)
    if(WIN32)
        target_link_libraries(
            openswd3_ffmpeg_static_bundle
            INTERFACE
                OpenSWD3::avformat
                OpenSWD3::avcodec
                OpenSWD3::swresample
                OpenSWD3::swscale
                OpenSWD3::avutil
                bcrypt
                ole32
                user32
        )
    else()
        find_package(Threads REQUIRED)
        target_link_libraries(
            openswd3_ffmpeg_static_bundle
            INTERFACE
                "$<LINK_GROUP:RESCAN,OpenSWD3::avformat,OpenSWD3::avcodec,OpenSWD3::swresample,OpenSWD3::swscale,OpenSWD3::avutil>"
                Threads::Threads
                m
                atomic
        )
    endif()
    add_library(OpenSWD3::FFmpeg ALIAS openswd3_ffmpeg_static_bundle)

    set_property(GLOBAL PROPERTY OPENSWD3_FFMPEG_RUNTIME_FILES "")
    set_property(
        GLOBAL PROPERTY OPENSWD3_FFMPEG_LICENSE_FILE
        "${OPENSWD3_FFMPEG_ROOT}/LICENSE.txt"
    )
endfunction()

function(openswd3_remove_split_ffmpeg_runtime_files target)
    set(
        _obsolete_names
        avformat-63.dll
        avcodec-63.dll
        avutil-61.dll
        swresample-7.dll
        swscale-10.dll
        libavformat.so.63
        libavcodec.so.63
        libavutil.so.61
        libswresample.so.7
        libswscale.so.10
        libavformat.so.63.1.100
        libavcodec.so.63.1.100
        libavutil.so.61.1.100
        libswresample.so.7.1.100
        libswscale.so.10.1.100
    )
    set(_obsolete_paths)
    foreach(_name IN LISTS _obsolete_names)
        list(APPEND _obsolete_paths "$<TARGET_FILE_DIR:${target}>/${_name}")
    endforeach()
    add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rm -f ${_obsolete_paths}
        VERBATIM
    )
endfunction()
