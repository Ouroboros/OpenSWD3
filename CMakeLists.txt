cmake_minimum_required(VERSION 3.25)

project(swd3_rebuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Scalar SSE2 floating point without contraction: every float/double intermediate is rounded like the original's
# x87 code running at the 53-bit precision set by the MSVC runtime.
if(MSVC)
    add_compile_options(/arch:SSE2 /clang:-ffp-contract=off)
else()
    add_compile_options(-msse2 -mfpmath=sse -ffp-contract=off -fno-strict-aliasing)
endif()

if(WIN32)
    set(SWD3_DEPS_PLATFORM windows)
else()
    set(SWD3_DEPS_PLATFORM linux)
endif()
set(SWD3_DEPS_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/third_party/install/${SWD3_DEPS_PLATFORM}"
    CACHE PATH "Prefix of SDL3/FFmpeg built by scripts/deps")
list(PREPEND CMAKE_PREFIX_PATH "${SWD3_DEPS_PREFIX}")
list(PREPEND CMAKE_FIND_ROOT_PATH "${SWD3_DEPS_PREFIX}")

file(GLOB_RECURSE SWD3_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
if(WIN32)
    list(FILTER SWD3_SOURCES EXCLUDE REGEX "_posix\\.c(pp)?$")
else()
    list(FILTER SWD3_SOURCES EXCLUDE REGEX "_win\\.c(pp)?$")
endif()
set(SWD3_MAIN_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/app/main.cpp")
list(REMOVE_ITEM SWD3_SOURCES "${SWD3_MAIN_SOURCE}")

if(MSVC)
    set(SWD3_WARNINGS /W3)
else()
    set(SWD3_WARNINGS -Wall -Wno-unused-parameter)
endif()

find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-static)
include(cmake/FFmpegStatic.cmake)
add_library(swd3_core STATIC ${SWD3_SOURCES})
target_include_directories(swd3_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(swd3_core PRIVATE ${SWD3_WARNINGS})
target_link_libraries(swd3_core PUBLIC SDL3::SDL3-static swd3::ffmpeg)
if(NOT WIN32)
    # Pull in the fmod replacement of src/platform/libm_posix.cpp before avutil needs it.
    target_link_options(swd3_core PUBLIC -Wl,--undefined=fmod)
endif()
if(WIN32)
    target_compile_definitions(swd3_core PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(swd3_core PUBLIC user32 gdi32 advapi32 ole32 winmm imm32)
endif()

if(EXISTS "${SWD3_MAIN_SOURCE}")
    add_executable(swd3 "${SWD3_MAIN_SOURCE}")
    target_link_libraries(swd3 PRIVATE swd3_core SDL3::SDL3-static swd3::ffmpeg)
    if(WIN32)
        set_target_properties(swd3 PROPERTIES WIN32_EXECUTABLE ON)
    endif()
endif()

enable_testing()
file(GLOB_RECURSE SWD3_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
if(SWD3_TEST_SOURCES)
    add_executable(swd3_tests ${SWD3_TEST_SOURCES})
    target_include_directories(swd3_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(swd3_tests PRIVATE swd3_core)
    add_test(NAME swd3_tests COMMAND swd3_tests WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()
