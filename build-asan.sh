#!/usr/bin/env bash

set -euo pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

build_directory="build/linux-asan"
cmake_executable="${OPENSWD3_CMAKE:-cmake}"
ctest_executable="${OPENSWD3_CTEST:-ctest}"
ninja_executable="${OPENSWD3_NINJA:-ninja}"
clangxx_executable="${CXX:-clang++}"
detected_processor_count="$(getconf _NPROCESSORS_ONLN)"
build_jobs="${OPENSWD3_BUILD_JOBS:-$detected_processor_count}"
test_jobs="${OPENSWD3_TEST_JOBS:-$detected_processor_count}"
expected_generator="Ninja"

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
    -DCMAKE_BUILD_TYPE:STRING=Debug
    -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON
    -DOPENSWD3_BUILD_APP:BOOL=OFF
    "-DCMAKE_CXX_FLAGS:STRING=-fsanitize=address -fno-omit-frame-pointer"
    -DCMAKE_EXE_LINKER_FLAGS:STRING=-fsanitize=address
)

if [[ "${OPENSWD3_RECONFIGURE:-0}" == "1" || ! -f "$cache_file" || ! -f "$build_directory/build.ninja" ]]; then
    echo "[OpenSWD3] Configure: linux-asan"
    "$cmake_executable" "${configure_arguments[@]}"
else
    echo "[OpenSWD3] Configure: linux-asan (reuse Ninja cache)"
fi

echo "[OpenSWD3] Build: linux-asan (parallel jobs: $build_jobs)"
"$cmake_executable" \
    --build "$build_directory" \
    --parallel "$build_jobs"

echo "[OpenSWD3] Test: AddressSanitizer (parallel jobs: $test_jobs)"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
    "$ctest_executable" \
    --test-dir "$build_directory" \
    --parallel "$test_jobs" \
    --output-on-failure

echo "[OpenSWD3] AddressSanitizer build and tests completed successfully."
