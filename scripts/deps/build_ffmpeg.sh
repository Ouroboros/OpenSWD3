#!/usr/bin/env bash
# Minimal LGPL static FFmpeg n9.0: Bink video/audio and MP3 demux+decode, file protocol,
# swresample and swscale, with the x86 assembly (nasm). Usage: build_ffmpeg.sh linux|windows|linux-i386|windows-x86
# (linux and windows are x64).
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

platform="${1:?usage: build_ffmpeg.sh linux|windows|linux-i386|windows-x86}"
src="${SWD3_TP_SRC}/ffmpeg"
build="${SWD3_TP_BUILD}/ffmpeg-${platform}"
prefix="${SWD3_TP_INSTALL}/${platform}"
test -f "${src}/configure"
test -x "${SWD3_NASM}"

common=(
    --prefix="${prefix}"
    --enable-static --disable-shared
    --disable-programs --disable-doc --disable-network --disable-autodetect
    --disable-avdevice --disable-avfilter
    --enable-x86asm --x86asmexe="${SWD3_NASM}"
    --disable-everything
    --enable-avcodec --enable-avformat --enable-avutil --enable-swresample --enable-swscale
    --enable-protocol=file
    --enable-demuxer=bink,mp3
    --enable-decoder=bink,binkaudio_rdft,binkaudio_dct,mp3,mp3float
    --enable-parser=mpegaudio
)

case "${platform}" in
linux)
    extra=(
        --target-os=linux --arch=x86_64
        --cc="${SWD3_LLVM_ROOT}/bin/clang"
        --cxx="${SWD3_LLVM_ROOT}/bin/clang++"
        --ar="${SWD3_LLVM_ROOT}/bin/llvm-ar"
        --ranlib="${SWD3_LLVM_ROOT}/bin/llvm-ranlib"
        --nm="${SWD3_LLVM_ROOT}/bin/llvm-nm"
        --host-cc="${SWD3_LLVM_ROOT}/bin/clang"
        --extra-ldflags="-static"
    )
    ;;
linux-i386)
    i386="--target=x86_64-linux-gnu -m32 --sysroot=${SWD3_I386_SYSROOT}"
    extra=(
        --target-os=linux --arch=x86 --enable-cross-compile
        --cc="${SWD3_LLVM_ROOT}/bin/clang"
        --cxx="${SWD3_LLVM_ROOT}/bin/clang++"
        --ar="${SWD3_LLVM_ROOT}/bin/llvm-ar"
        --ranlib="${SWD3_LLVM_ROOT}/bin/llvm-ranlib"
        --nm="${SWD3_LLVM_ROOT}/bin/llvm-nm"
        --host-cc="${SWD3_LLVM_ROOT}/bin/clang"
        --extra-cflags="${i386}"
        --extra-ldflags="${i386} -static"
    )
    ;;
windows | windows-x86)
    if [ "${platform}" = windows ]; then
        target=x86_64-pc-windows-msvc
        libarch=x64
        arch=x86_64
    else
        target=i686-pc-windows-msvc
        libarch=x86
        arch=x86
    fi
    links="$(swd3_winsdk_links)"
    sdkinc="${links}/sdk/Include/${SWD3_WINSDK_VERSION}"
    sdklib="${links}/sdk/Lib/${SWD3_WINSDK_VERSION}"
    cflags="--target=${target} -MT -imsvc${links}/msvc/include -imsvc${sdkinc}/ucrt -imsvc${sdkinc}/um -imsvc${sdkinc}/shared"
    ldflags="-libpath:${links}/msvc/lib/${libarch} -libpath:${sdklib}/ucrt/${libarch} -libpath:${sdklib}/um/${libarch}"
    extra=(
        --toolchain=msvc
        --target-os=win32 --arch="${arch}" --enable-cross-compile
        --cc="${SWD3_LLVM_ROOT}/bin/clang-cl"
        --cxx="${SWD3_LLVM_ROOT}/bin/clang-cl"
        --ld="${SWD3_LLVM_ROOT}/bin/lld-link"
        --ar="${SWD3_LLVM_ROOT}/bin/llvm-ar"
        --nm="${SWD3_LLVM_ROOT}/bin/llvm-nm"
        --host-cc="${SWD3_LLVM_ROOT}/bin/clang"
        --extra-cflags="${cflags}"
        --extra-ldflags="${ldflags}"
    )
    ;;
*)
    echo "unknown platform ${platform}" >&2
    exit 2
    ;;
esac

rm -rf "${build}"
mkdir -p "${build}" "${prefix}"
cd "${build}"

if [[ "${platform}" = windows* ]] && command -v wslpath >/dev/null; then
    # configure pipes clang-cl /showIncludes output through `wslpath -u`, which mangles
    # paths that are already Linux paths; pass those through unchanged.
    real_wslpath="$(command -v wslpath)"
    mkdir -p "${build}/shim"
    cat > "${build}/shim/wslpath" <<EOF
#!/usr/bin/env bash
if [ "\$1" = -u ] && [ "\${2:0:1}" = / ]; then printf '%s\n' "\$2"; exit 0; fi
exec "${real_wslpath}" "\$@"
EOF
    chmod +x "${build}/shim/wslpath"
    export PATH="${build}/shim:${PATH}"
fi
"${src}/configure" "${common[@]}" "${extra[@]}"
make -j"$(swd3_jobs)"
make install
printf 'ffmpeg %s installed to %s\n' "${platform}" "${prefix}"
