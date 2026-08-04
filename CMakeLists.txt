cmake_minimum_required(VERSION 3.25)

project(OpenSWD3 VERSION 0.1.0 LANGUAGES CXX)

option(OPENSWD3_BUILD_APP "Build the SDL3 OpenSWD3 executable" ON)
option(
    OPENSWD3_FETCH_SDL3
    "Fetch the pinned SDL3 source when no installed SDL3 package is available"
    ON
)
option(
    OPENSWD3_FETCH_TOMLPLUSPLUS
    "Fetch the pinned toml++ source when no installed package is available"
    ON
)

find_package(tomlplusplus 3.4 CONFIG QUIET)

if(NOT TARGET tomlplusplus::tomlplusplus)
    if(NOT OPENSWD3_FETCH_TOMLPLUSPLUS)
        message(
            FATAL_ERROR
            "toml++ 3.4 or newer was not found and "
            "OPENSWD3_FETCH_TOMLPLUSPLUS is OFF"
        )
    endif()

    include(FetchContent)

    set(TOMLPLUSPLUS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(TOMLPLUSPLUS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG 30172438cee64926dc41fdd9c11fb3ba5b2ba9de
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(tomlplusplus)
endif()

add_library(openswd3_project_options INTERFACE)
target_compile_features(openswd3_project_options INTERFACE cxx_std_20)

add_library(openswd3_project_warnings INTERFACE)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(openswd3_project_warnings INTERFACE /W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(openswd3_project_warnings INTERFACE /W4 /permissive-)
    else()
        target_compile_options(
            openswd3_project_warnings
            INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
        )
    endif()
else()
    message(FATAL_ERROR "OpenSWD3 supports MSVC and LLVM Clang compilers")
endif()

add_subdirectory(src/diagnostics)
add_subdirectory(src/compat)
add_subdirectory(src/resource_io)
add_subdirectory(src/input_time_rng)
add_subdirectory(src/rendering)
add_subdirectory(src/app)
add_subdirectory(analysis/tools)

if(OPENSWD3_BUILD_APP)
    find_package(SDL3 3.4 CONFIG QUIET)

    if(NOT TARGET SDL3::SDL3)
        if(NOT OPENSWD3_FETCH_SDL3)
            message(
                FATAL_ERROR
                "SDL3 3.4 or newer was not found and OPENSWD3_FETCH_SDL3 is OFF"
            )
        endif()

        include(FetchContent)

        # Keep the fallback self-contained: no SDL runtime DLL, tests, examples,
        # or install rules are needed for the OpenSWD3 application target.
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.12
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(SDL3)
    endif()

    add_subdirectory(src/platform/sdl3)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
