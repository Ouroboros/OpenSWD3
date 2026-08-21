#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

target="${1:-core}"
case "$target" in
core)
    build_directory="build/linux-core"
    build_application="OFF"
    ;;

app)
    build_directory="build/linux-app"
    build_application="ON"
    ;;

*)
    echo "Usage: ./build.sh [core|app]" >&2
    exit 2
    ;;
esac

cmake_executable="${OPENSWD3_CMAKE:-cmake}"
ctest_executable="${OPENSWD3_CTEST:-ctest}"
ninja_executable="${OPENSWD3_NINJA:-ninja}"
clang_executable="${CC:-clang}"
clangxx_executable="${CXX:-clang++}"
detected_processor_count="$(getconf _NPROCESSORS_ONLN)"
build_jobs="${OPENSWD3_BUILD_JOBS:-$detected_processor_count}"
test_jobs="${OPENSWD3_TEST_JOBS:-$detected_processor_count}"
expected_generator="Ninja Multi-Config"

if [[ ! "$build_jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo "[OpenSWD3] OPENSWD3_BUILD_JOBS must be a positive integer." >&2
    exit 2
fi
if [[ ! "$test_jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo "[OpenSWD3] OPENSWD3_TEST_JOBS must be a positive integer." >&2
    exit 2
fi

for tool in \
    "$cmake_executable" \
    "$ctest_executable" \
    "$ninja_executable" \
    "$clangxx_executable" \
    getconf; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "[OpenSWD3] Required tool not found: $tool" >&2
        exit 2
    fi
done

cache_file="$build_directory/CMakeCache.txt"
if [[ -f "$cache_file" ]]; then
    current_generator=""
    while IFS='=' read -r key value; do
        if [[ "$key" == "CMAKE_GENERATOR:INTERNAL" ]]; then
            current_generator="$value"
            break
        fi
    done < "$cache_file"
    if [[ "$current_generator" != "$expected_generator" ]]; then
        echo "[OpenSWD3] Reset generator: ${current_generator:-unknown} -> $expected_generator"
        rm -rf -- "$build_directory"
    fi
fi

configure_arguments=(
    -S .
    -B "$build_directory"
    -G "$expected_generator"
    -DCMAKE_MAKE_PROGRAM:FILEPATH="$ninja_executable"
    -DCMAKE_CXX_COMPILER:FILEPATH="$clangxx_executable"
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON
    -DOPENSWD3_BUILD_APP:BOOL="$build_application"
)

if [[ "$target" == "app" ]]; then
    if ! command -v "$clang_executable" >/dev/null 2>&1; then
        echo "[OpenSWD3] Required tool not found: $clang_executable" >&2
        exit 2
    fi

    configure_arguments+=(
        -DCMAKE_C_COMPILER:FILEPATH="$clang_executable"
        -DSDL_X11_XCURSOR:BOOL=OFF
        -DSDL_X11_XFIXES:BOOL=OFF
        -DSDL_X11_XINPUT:BOOL=OFF
        -DSDL_X11_XRANDR:BOOL=OFF
        -DSDL_X11_XTEST:BOOL=OFF
    )
fi

if [[ "${OPENSWD3_RECONFIGURE:-0}" == "1" || ! -f "$cache_file" || ! -f "$build_directory/build.ninja" ]]; then
    echo "[OpenSWD3] Configure: linux-$target"
    "$cmake_executable" "${configure_arguments[@]}"
else
    echo "[OpenSWD3] Configure: linux-$target (reuse Ninja cache)"
fi

echo "[OpenSWD3] Build: linux-$target-debug (parallel jobs: $build_jobs)"
"$cmake_executable" \
    --build "$build_directory" \
    --config Debug \
    --parallel "$build_jobs"

echo "[OpenSWD3] Test: Debug (parallel jobs: $test_jobs)"
"$ctest_executable" \
    --test-dir "$build_directory" \
    -C Debug \
    --parallel "$test_jobs" \
    --output-on-failure

echo "[OpenSWD3] Build and tests completed successfully."
