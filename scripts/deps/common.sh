#!/usr/bin/env bash
set -euo pipefail

SWD3_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SWD3_TP="${SWD3_ROOT}/third_party"
SWD3_TP_SRC="${SWD3_TP}/src"
SWD3_TP_BUILD="${SWD3_TP}/build"
SWD3_TP_INSTALL="${SWD3_TP}/install"

SDL3_REPO="https://github.com/libsdl-org/SDL.git"
SDL3_TAG="release-3.4.18"
FFMPEG_REPO="https://git.ffmpeg.org/ffmpeg.git"
FFMPEG_TAG="n9.0"

SWD3_LLVM_ROOT="${SWD3_LLVM_ROOT:-/opt/llvm}"
SWD3_I386_SYSROOT="${SWD3_I386_SYSROOT:-/opt/sysroot-i386}"
SWD3_NASM="${SWD3_NASM:-$(command -v nasm || echo /opt/nasm/usr/bin/nasm)}"
SWD3_MSVC_ROOT="${SWD3_MSVC_ROOT:-/mnt/c/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207}"
SWD3_WINSDK_ROOT="${SWD3_WINSDK_ROOT:-/mnt/c/Program Files (x86)/Windows Kits/10}"
SWD3_WINSDK_VERSION="${SWD3_WINSDK_VERSION:-10.0.26100.0}"

swd3_jobs() {
    nproc
}

swd3_winsdk_links() {
    local links="${SWD3_TP}/winsdk"
    mkdir -p "${links}"
    ln -sfn "${SWD3_MSVC_ROOT}" "${links}/msvc"
    ln -sfn "${SWD3_WINSDK_ROOT}" "${links}/sdk"
    test -d "${links}/msvc/include"
    test -d "${links}/sdk/Include/${SWD3_WINSDK_VERSION}/ucrt"
    printf '%s\n' "${links}"
}
