#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIR}/../../.." && pwd)"
CACHE_ROOT="${PROJECT_ROOT}/build/dependencies/ffmpeg/9.0"
SOURCE_CACHE="${CACHE_ROOT}/source"
ARCHIVE="${SOURCE_CACHE}/ffmpeg-9.0.tar.xz"
SIGNATURE="${SOURCE_CACHE}/ffmpeg-9.0.tar.xz.asc"
KEY_FILE="${SCRIPT_DIR}/ffmpeg-devel.asc"
PROJECT_TMP_ROOT="${PROJECT_ROOT}/build/tmp/runtime"
WORK_ROOT="${OPENSWD3_FFMPEG_WORK_ROOT:-${PROJECT_TMP_ROOT}/openswd3-ffmpeg-9.0}"
SOURCE_DIR="${WORK_ROOT}/source"
WINDOWS_LLVM_BIN="${OPENSWD3_WINDOWS_LLVM_BIN:-/mnt/d/Dev/Compiler/LLVM/x64/bin}"
JOBS="${OPENSWD3_FFMPEG_JOBS:-$(nproc)}"
PLATFORM="${1:-all}"

SOURCE_URL="https://ffmpeg.org/releases/ffmpeg-9.0.tar.xz"
SIGNATURE_URL="https://ffmpeg.org/releases/ffmpeg-9.0.tar.xz.asc"
SOURCE_BYTES="12032020"
SOURCE_SHA256="7f607a00dd0d28a729d5a4811205812eef01cf6ef6155025febb6f36a9062d52"
SIGNING_FINGERPRINT="FCF986EA15E6E293A5644F10B4322F04D67658D8"
SOURCE_DATE_EPOCH="1785795290"

case "${PLATFORM}" in
    all|linux-x64|windows-x64) ;;
    *)
        printf 'usage: %s [all|linux-x64|windows-x64]\n' "$0" >&2
        exit 2
        ;;
esac

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'required command is unavailable: %s\n' "$1" >&2
        exit 1
    fi
}

require_command curl
require_command gpg
require_command make
require_command realpath
require_command sha256sum
require_command tar

PROJECT_TMP_ROOT="$(realpath -m -- "${PROJECT_TMP_ROOT}")"
WORK_ROOT="$(realpath -m -- "${WORK_ROOT}")"
case "${WORK_ROOT}/" in
    "${PROJECT_ROOT}/"*) ;;
    *)
        printf 'FFmpeg work directory must stay inside the repository: %s\n' \
            "${WORK_ROOT}" >&2
        exit 1
        ;;
esac
SOURCE_DIR="${WORK_ROOT}/source"
mkdir -p "${SOURCE_CACHE}" "${PROJECT_TMP_ROOT}" "${WORK_ROOT}"
export TMPDIR="${PROJECT_TMP_ROOT}"
export TMP="${PROJECT_TMP_ROOT}"
export TEMP="${PROJECT_TMP_ROOT}"
if [[ ! -f "${ARCHIVE}" ]]; then
    curl --fail --location --retry 3 --output "${ARCHIVE}" "${SOURCE_URL}"
fi
if [[ ! -f "${SIGNATURE}" ]]; then
    curl --fail --location --retry 3 --output "${SIGNATURE}" "${SIGNATURE_URL}"
fi

actual_bytes="$(stat --format='%s' "${ARCHIVE}")"
actual_sha256="$(sha256sum "${ARCHIVE}" | cut -d ' ' -f 1)"
if [[ "${actual_bytes}" != "${SOURCE_BYTES}" ]]; then
    printf 'FFmpeg source size mismatch: expected %s, got %s\n' \
        "${SOURCE_BYTES}" "${actual_bytes}" >&2
    exit 1
fi
if [[ "${actual_sha256}" != "${SOURCE_SHA256}" ]]; then
    printf 'FFmpeg source SHA256 mismatch: expected %s, got %s\n' \
        "${SOURCE_SHA256}" "${actual_sha256}" >&2
    exit 1
fi

gnupg_home="$(mktemp -d "${PROJECT_TMP_ROOT}/openswd3-ffmpeg-gpg.XXXXXX")"
cleanup_gnupg() {
    rm -rf "${gnupg_home}"
}
trap cleanup_gnupg EXIT
chmod 700 "${gnupg_home}"
gpg --homedir "${gnupg_home}" --batch --no-autostart --quiet \
    --import "${KEY_FILE}"
actual_fingerprint="$(
    gpg --homedir "${gnupg_home}" --batch --no-autostart --with-colons \
        --fingerprint ffmpeg-devel@ffmpeg.org |
        awk -F: '$1 == "fpr" { print $10; exit }'
)"
if [[ "${actual_fingerprint}" != "${SIGNING_FINGERPRINT}" ]]; then
    printf 'FFmpeg signing fingerprint mismatch: expected %s, got %s\n' \
        "${SIGNING_FINGERPRINT}" "${actual_fingerprint}" >&2
    exit 1
fi
gpg --homedir "${gnupg_home}" --batch --no-autostart --quiet \
    --verify "${SIGNATURE}" "${ARCHIVE}"

rm -rf "${SOURCE_DIR}"
mkdir -p "${SOURCE_DIR}"
tar --extract --file "${ARCHIVE}" --strip-components=1 \
    --directory "${SOURCE_DIR}"

export LC_ALL=C
export TZ=UTC
export SOURCE_DATE_EPOCH

COMMON_CONFIGURE=(
    --disable-everything
    --disable-autodetect
    --disable-network
    --disable-programs
    --disable-doc
    --disable-avdevice
    --disable-avfilter
    --disable-debug
    --disable-x86asm
    --disable-iconv
    --enable-small
    --disable-shared
    --enable-static
    --enable-pic
    --enable-avcodec
    --enable-avformat
    --enable-avutil
    --enable-swresample
    --enable-swscale
    --enable-demuxer=bink,mp3
    --enable-decoder=bink,binkaudio_dct,binkaudio_rdft,mp3float
    --enable-parser=mpegaudio
    --enable-protocol=file
    --pkg-config=false
)

write_build_info() {
    local prefix="$1"
    local platform="$2"
    local compiler="$3"
    local linker="$4"
    local configure_line="$5"
    cat >"${prefix}/BUILDINFO.txt" <<EOF
FFmpeg release: 9.0 (tag n9.0)
Source URL: ${SOURCE_URL}
Source bytes: ${SOURCE_BYTES}
Source SHA256: ${SOURCE_SHA256}
Source signature fingerprint: ${SIGNING_FINGERPRINT}
SOURCE_DATE_EPOCH: ${SOURCE_DATE_EPOCH}
Platform: ${platform}
Compiler: ${compiler}
Linker: ${linker}
License configuration: LGPL static archives for openswd3_ffmpeg; GPL and nonfree components disabled
Enabled demuxers: bink, mp3
Enabled decoders: bink, binkaudio_dct, binkaudio_rdft, mp3float
Enabled parser: mpegaudio
Enabled protocol: file
Enabled conversion libraries: swresample, swscale
Configure: ${configure_line}
EOF
    cp "${SOURCE_DIR}/COPYING.LGPLv2.1" "${prefix}/LICENSE.txt"
}

build_linux() {
    require_command gcc
    local build_dir="${WORK_ROOT}/build-linux-x64"
    local prefix="${CACHE_ROOT}/self-built/linux-x64"
    rm -rf "${build_dir}" "${prefix}"
    mkdir -p "${build_dir}" "${prefix}"
    local configure_args=(
        "${COMMON_CONFIGURE[@]}"
        --prefix=/
        --cc=gcc
        --extra-cflags=-Os\ -ffunction-sections\ -fdata-sections\ -fno-ident
    )
    (
        cd "${build_dir}"
        "${SOURCE_DIR}/configure" "${configure_args[@]}"
        make -j"${JOBS}"
        make DESTDIR="${prefix}" install
    )
    write_build_info \
        "${prefix}" \
        linux-x64 \
        "$(gcc --version | head -n 1)" \
        "$(ld --version | head -n 1)" \
        "${configure_args[*]}"
}

build_windows() {
    if [[ ! -d "${WINDOWS_LLVM_BIN}" ]]; then
        printf 'Windows LLVM tool directory is unavailable: %s\n' \
            "${WINDOWS_LLVM_BIN}" >&2
        exit 1
    fi

    local PATH="${WINDOWS_LLVM_BIN}:${PATH}"
    export PATH
    require_command clang-cl.exe
    require_command lld-link.exe
    require_command llvm-ar.exe
    require_command llvm-nm.exe
    require_command llvm-strip.exe

    local work_dir="${CACHE_ROOT}/work-windows-x64"
    local source_dir="${work_dir}/source"
    local prefix="${CACHE_ROOT}/self-built/windows-x64"
    rm -rf "${work_dir}" "${prefix}"
    mkdir -p "${source_dir}" "${prefix}"
    tar --extract --file "${ARCHIVE}" --strip-components=1 \
        --directory "${source_dir}"

    local configure_args=(
        "${COMMON_CONFIGURE[@]}"
        --prefix=/
        --target-os=win32
        --arch=x86_64
        --enable-cross-compile
        --toolchain=msvc
        --cc=clang-cl.exe
        --cxx=clang-cl.exe
        --ld=lld-link.exe
        --ar=llvm-ar.exe
        --nm=llvm-nm.exe
        --strip=llvm-strip.exe
        --disable-pthreads
        --enable-w32threads
        --extra-cflags=/Brepro\ /Gy\ /Gw
    )
    (
        cd "${source_dir}"
        ./configure "${configure_args[@]}"
        make -j"${JOBS}"
        make DESTDIR="${prefix}" install
    )
    write_build_info \
        "${prefix}" \
        windows-x64 \
        "$(clang-cl.exe --version | head -n 1)" \
        "$(lld-link.exe --version | head -n 1)" \
        "${configure_args[*]}"
}

if [[ "${PLATFORM}" == all || "${PLATFORM}" == linux-x64 ]]; then
    build_linux
fi
if [[ "${PLATFORM}" == all || "${PLATFORM}" == windows-x64 ]]; then
    build_windows
fi

printf 'FFmpeg 9.0 minimal static archive build completed: %s\n' "${PLATFORM}"
