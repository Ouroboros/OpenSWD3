#!/usr/bin/env bash
# Checks the tools and libraries of the Linux build, offers to install what is missing (nothing is installed without a
# yes), builds the third-party libraries when absent and then the program.
# Usage: build_linux.sh [--i386] [--test]
#   --i386  the 32-bit linux-i386 preset (default: the x64 linux preset)
#   --test  run the unit tests after the build
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/deps/common.sh"

LLVM_VERSION="22.1.8"
LLVM_ARCHIVE="LLVM-${LLVM_VERSION}-Linux-X64.tar.xz"
LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/${LLVM_ARCHIVE}"
CMAKE_LEAST="3.25"

preset=linux
run_tests=0

for arg in "$@"; do
    case "${arg}" in
    --i386) preset=linux-i386 ;;
    --test) run_tests=1 ;;
    -h | --help)
        sed -n '2,6p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
        exit 0
        ;;
    *)
        printf 'unknown option: %s (see --help)\n' "${arg}" >&2
        exit 2
        ;;
    esac
done

ask() {
    local answer
    if [ ! -t 0 ]; then
        return 1
    fi
    read -r -p "$1 [y/N] " answer
    [ "${answer}" = y ] || [ "${answer}" = Y ]
}

as_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    else
        sudo "$@"
    fi
}

installed() {
    dpkg-query -W -f='${Status}' "$1" 2>/dev/null | grep -q 'install ok installed'
}

# The system tools and libraries (apt packages).
missing=()
for pair in cmake:cmake ninja:ninja-build git:git python3:python3 curl:curl xz:xz-utils; do
    command -v "${pair%%:*}" >/dev/null || missing+=("${pair##*:}")
done
[ -x "${SWD3_NASM}" ] || missing+=(nasm)
for package in libc6-dev libstdc++-13-dev; do
    installed "${package}" || missing+=("${package}")
done

if [ "${#missing[@]}" -gt 0 ]; then
    printf 'missing packages: %s\n' "${missing[*]}"
    if ! ask "install them with apt-get?"; then
        printf 'not installed, stopping\n'
        exit 1
    fi
    as_root apt-get update
    as_root apt-get install -y "${missing[@]}"
    SWD3_NASM="$(command -v nasm)"
fi

cmake_version="$(cmake --version | sed -n 's/^cmake version \([0-9.]*\).*/\1/p')"
if [ "$(printf '%s\n%s\n' "${CMAKE_LEAST}" "${cmake_version}" | sort -V | head -1)" != "${CMAKE_LEAST}" ]; then
    printf 'cmake %s found, %s or later needed\n' "${cmake_version}" "${CMAKE_LEAST}" >&2
    exit 1
fi

# LLVM (clang and the llvm tools of the toolchain files).
llvm_ready() {
    local tool
    for tool in clang clang++ llvm-ar llvm-ranlib llvm-nm; do
        [ -x "$1/bin/${tool}" ] || return 1
    done
}

if ! llvm_ready "${SWD3_LLVM_ROOT}"; then
    if [ ! -t 0 ]; then
        printf 'LLVM not found in %s (set SWD3_LLVM_ROOT), stopping\n' "${SWD3_LLVM_ROOT}" >&2
        exit 1
    fi
    printf 'LLVM not found in %s\n' "${SWD3_LLVM_ROOT}"
    printf 'install LLVM %s to:\n  1) /opt/llvm\n  2) %s\n  3) another folder\n  0) do not install, stop\n' \
        "${LLVM_VERSION}" "${SWD3_TP}/llvm"
    read -r -p "choice: " choice
    case "${choice}" in
    1) SWD3_LLVM_ROOT=/opt/llvm ;;
    2) SWD3_LLVM_ROOT="${SWD3_TP}/llvm" ;;
    3) read -r -p "folder: " SWD3_LLVM_ROOT ;;
    *)
        printf 'not installed, stopping\n'
        exit 1
        ;;
    esac

    download="${SWD3_TP_BUILD}/${LLVM_ARCHIVE}"
    mkdir -p "${SWD3_TP_BUILD}"
    curl -L -f -o "${download}" "${LLVM_URL}"
    if mkdir -p "${SWD3_LLVM_ROOT}" 2>/dev/null && [ -w "${SWD3_LLVM_ROOT}" ]; then
        tar -xJf "${download}" --strip-components=1 -C "${SWD3_LLVM_ROOT}"
    else
        as_root mkdir -p "${SWD3_LLVM_ROOT}"
        as_root tar -xJf "${download}" --strip-components=1 -C "${SWD3_LLVM_ROOT}"
    fi
    rm -f "${download}"
    if ! llvm_ready "${SWD3_LLVM_ROOT}"; then
        printf 'LLVM tools still missing in %s\n' "${SWD3_LLVM_ROOT}" >&2
        exit 1
    fi
fi
export SWD3_LLVM_ROOT SWD3_NASM

# The i386 sysroot of the 32-bit preset.
if [ "${preset}" = linux-i386 ] && [ ! -f "${SWD3_I386_SYSROOT}/usr/lib32/libc.a" ]; then
    printf 'i386 sysroot not found in %s\n' "${SWD3_I386_SYSROOT}"
    if ! ask "download the Ubuntu i386 packages and unpack them there?"; then
        printf 'not installed, stopping\n'
        exit 1
    fi
    bash "${SWD3_ROOT}/scripts/deps/setup_i386_sysroot.sh"
fi

# The third-party sources and libraries (inside third_party, built when absent).
prefix="${SWD3_TP_INSTALL}/${preset}"
if [ ! -f "${SWD3_TP_SRC}/tomlplusplus/toml.hpp" ] || [ ! -f "${SWD3_TP_SRC}/SDL3/CMakeLists.txt" ] ||
    [ ! -f "${SWD3_TP_SRC}/ffmpeg/configure" ]; then
    bash "${SWD3_ROOT}/scripts/deps/fetch.sh"
fi
if [ ! -f "${prefix}/lib/libSDL3.a" ]; then
    bash "${SWD3_ROOT}/scripts/deps/build_sdl3.sh" "${preset}"
fi
for library in avcodec avformat avutil swresample swscale; do
    if [ ! -f "${prefix}/lib/lib${library}.a" ]; then
        bash "${SWD3_ROOT}/scripts/deps/build_ffmpeg.sh" "${preset}"
        break
    fi
done

if [ ! -f "${SWD3_TP}/asset/font.bin" ]; then
    printf 'note: third_party/asset/font.bin missing, the program builds but shows no text (see docs/building.md)\n'
fi

cd "${SWD3_ROOT}"
cmake --preset "${preset}" -DSWD3_LLVM_ROOT="${SWD3_LLVM_ROOT}"
cmake --build --preset "${preset}"

if [ "${run_tests}" -eq 1 ]; then
    "${SWD3_ROOT}/build/${preset}/swd3_tests"
fi

printf 'built %s\n' "${SWD3_ROOT}/build/${preset}"
