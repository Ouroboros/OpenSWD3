#!/usr/bin/env bash
# Static SDL3 build. Usage: build_sdl3.sh linux|windows|linux-i386|windows-x86 (linux and windows are x64).
# linux = static build used for compilation and unit tests only (dummy/offscreen video, dummy audio).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

platform="${1:?usage: build_sdl3.sh linux|windows|linux-i386|windows-x86}"
src="${SWD3_TP_SRC}/SDL3"
build="${SWD3_TP_BUILD}/sdl3-${platform}"
prefix="${SWD3_TP_INSTALL}/${platform}"
test -f "${src}/CMakeLists.txt"

args=(
    -S "${src}" -B "${build}" -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="${prefix}"
    -DSWD3_LLVM_ROOT="${SWD3_LLVM_ROOT}"
    -DSDL_STATIC=ON -DSDL_SHARED=OFF
    -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
)

case "${platform}" in
linux | linux-i386)
    toolchain=toolchain-linux-x64-clang.cmake

    if [ "${platform}" = linux-i386 ]; then
        toolchain=toolchain-linux-i386-clang.cmake
    fi

    args+=(
        -DCMAKE_TOOLCHAIN_FILE="${SWD3_ROOT}/cmake/${toolchain}"
        -DSDL_UNIX_CONSOLE_BUILD=ON
        -DSDL_X11=OFF -DSDL_WAYLAND=OFF -DSDL_KMSDRM=OFF -DSDL_RPI=OFF -DSDL_VIVANTE=OFF
        -DSDL_OPENGL=OFF -DSDL_OPENGLES=OFF -DSDL_VULKAN=OFF -DSDL_GPU=OFF
        -DSDL_ALSA=OFF -DSDL_PULSEAUDIO=OFF -DSDL_PIPEWIRE=OFF -DSDL_JACK=OFF -DSDL_SNDIO=OFF -DSDL_OSS=OFF
        -DSDL_HIDAPI=OFF -DSDL_LIBUDEV=OFF -DSDL_DBUS=OFF -DSDL_IBUS=OFF -DSDL_FRIBIDI=OFF -DSDL_LIBTHAI=OFF
        -DSDL_DUMMYVIDEO=ON -DSDL_OFFSCREEN=ON -DSDL_DUMMYAUDIO=ON -DSDL_DISKAUDIO=ON
    )
    ;;
windows)
    args+=(-DCMAKE_TOOLCHAIN_FILE="${SWD3_ROOT}/cmake/toolchain-windows-x64-clang-cl.cmake")
    ;;
windows-x86)
    args+=(-DCMAKE_TOOLCHAIN_FILE="${SWD3_ROOT}/cmake/toolchain-windows-x86-clang-cl.cmake")
    ;;
*)
    echo "unknown platform ${platform}" >&2
    exit 2
    ;;
esac

rm -rf "${build}"
cmake "${args[@]}"
cmake --build "${build}" -j"$(swd3_jobs)"
cmake --install "${build}"
printf 'SDL3 %s installed to %s\n' "${platform}" "${prefix}"
