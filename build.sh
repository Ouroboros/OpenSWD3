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
clang_executable="${CC:-clang}"
clangxx_executable="${CXX:-clang++}"
test_jobs="${OPENSWD3_TEST_JOBS:-8}"

if [[ ! "$test_jobs" =~ ^[1-9][0-9]*$ ]]; then
    echo "[OpenSWD3] OPENSWD3_TEST_JOBS must be a positive integer." >&2
    exit 2
fi

for tool in \
    "$cmake_executable" \
    "$ctest_executable" \
    "$clangxx_executable" \
    make; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "[OpenSWD3] Required tool not found: $tool" >&2
        exit 2
    fi
done

configure_arguments=(
    -S .
    -B "$build_directory"
    -G "Unix Makefiles"
    -DCMAKE_BUILD_TYPE:STRING=Debug
    -DCMAKE_CXX_COMPILER:FILEPATH="$clangxx_executable"
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

echo "[OpenSWD3] Configure: linux-$target"
"$cmake_executable" "${configure_arguments[@]}"

echo "[OpenSWD3] Build: linux-$target-debug"
"$cmake_executable" --build "$build_directory" --parallel

echo "[OpenSWD3] Test: Debug (parallel jobs: $test_jobs)"
"$ctest_executable" \
    --test-dir "$build_directory" \
    --parallel "$test_jobs" \
    --output-on-failure

echo "[OpenSWD3] Build and tests completed successfully."
