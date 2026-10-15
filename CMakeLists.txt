cmake_minimum_required(VERSION 3.25)

project(swd3_rebuild LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

if(SWD3_SOURCES)
    find_package(SDL3 CONFIG REQUIRED COMPONENTS SDL3-static)
    include(cmake/FFmpegStatic.cmake)

    add_executable(swd3 ${SWD3_SOURCES})
    target_include_directories(swd3 PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(swd3 PRIVATE SDL3::SDL3-static swd3::ffmpeg)
    if(WIN32)
        set_target_properties(swd3 PROPERTIES WIN32_EXECUTABLE ON)
    endif()
else()
    message(STATUS "src/ has no translation units yet; no executable target defined")
endif()

enable_testing()
file(GLOB_RECURSE SWD3_TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
if(SWD3_TEST_SOURCES)
    add_executable(swd3_tests ${SWD3_TEST_SOURCES})
    target_include_directories(swd3_tests PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    add_test(NAME swd3_tests COMMAND swd3_tests)
endif()
