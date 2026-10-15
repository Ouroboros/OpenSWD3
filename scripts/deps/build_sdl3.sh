#!/usr/bin/env bash
# Static SDL3 build. Usage: build_sdl3.sh linux|windows
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

platform="${1:?usage: build_sdl3.sh linux|windows}"
src="${SWD3_TP_SRC}/SDL3"
build="${SWD3_TP_BUILD}/sdl3-${platform}"
prefix="${SWD3_TP_INSTALL}/${platform}"
test -f "${src}/CMakeLists.txt"

args=(
    -S "${src}" -B "${build}" -G Ninja
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="${prefix}"
    -DSDL_STATIC=ON -DSDL_SHARED=OFF
    -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
)

linux_optional_backends() {
    local entry pkg opt
    for entry in \
        xcursor:SDL_X11_XCURSOR xi:SDL_X11_XINPUT xfixes:SDL_X11_XFIXES xrandr:SDL_X11_XRANDR \
        xscrnsaver:SDL_X11_XSCRNSAVER xtst:SDL_X11_XTEST wayland-client:SDL_WAYLAND alsa:SDL_ALSA libpulse:SDL_PULSEAUDIO \
        libpipewire-0.3:SDL_PIPEWIRE jack:SDL_JACK sndio:SDL_SNDIO gbm:SDL_KMSDRM libudev:SDL_LIBUDEV \
        dbus-1:SDL_DBUS ibus-1.0:SDL_IBUS libusb-1.0:SDL_HIDAPI_LIBUSB; do
        pkg="${entry%%:*}"
        opt="${entry##*:}"
        if pkg-config --exists "${pkg}"; then
            echo "SDL3 backend ${opt}: ${pkg} found" >&2
        else
            echo "SDL3 backend ${opt}: ${pkg} missing, disabled" >&2
            printf '%s\n' "-D${opt}=OFF"
        fi
    done
}

case "${platform}" in
linux)
    args+=(-DCMAKE_C_COMPILER="${SWD3_LLVM_ROOT}/bin/clang" -DCMAKE_CXX_COMPILER="${SWD3_LLVM_ROOT}/bin/clang++")
    mapfile -t disabled < <(linux_optional_backends)
    args+=("${disabled[@]}")
    ;;
windows)
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
