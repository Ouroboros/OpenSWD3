set(_swd3_ff_libs avformat avcodec swscale swresample avutil)
set(_swd3_ff_targets)
foreach(_lib IN LISTS _swd3_ff_libs)
    find_library(SWD3_FF_${_lib}
        NAMES ${_lib} lib${_lib} lib${_lib}.a
        PATHS "${SWD3_DEPS_PREFIX}/lib"
        NO_DEFAULT_PATH REQUIRED)
    list(APPEND _swd3_ff_targets "${SWD3_FF_${_lib}}")
endforeach()

add_library(swd3_ffmpeg INTERFACE)
add_library(swd3::ffmpeg ALIAS swd3_ffmpeg)
target_include_directories(swd3_ffmpeg INTERFACE "${SWD3_DEPS_PREFIX}/include")
target_link_libraries(swd3_ffmpeg INTERFACE ${_swd3_ff_targets})
if(WIN32)
    target_link_libraries(swd3_ffmpeg INTERFACE bcrypt)
else()
    target_link_libraries(swd3_ffmpeg INTERFACE m pthread)
endif()
