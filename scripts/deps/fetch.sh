#!/usr/bin/env bash
# Downloads pinned SDL3, FFmpeg and toml++ sources into third_party/src and records the resolved commits.
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

mkdir -p "${SWD3_TP_SRC}"

fetch_tag() {
    local repo="$1" tag="$2" dest="$3"
    if [ -d "${dest}/.git" ]; then
        git -C "${dest}" fetch --depth 1 origin "refs/tags/${tag}:refs/tags/${tag}"
        git -C "${dest}" checkout --force "refs/tags/${tag}"
    else
        git clone --depth 1 --branch "${tag}" "${repo}" "${dest}"
    fi
    printf '%s\t%s\t%s\n' "$(basename "${dest}")" "${tag}" "$(git -C "${dest}" rev-parse HEAD)"
}

{
    printf 'name\ttag\tcommit\n'
    fetch_tag "${SDL3_REPO}" "${SDL3_TAG}" "${SWD3_TP_SRC}/SDL3"
    fetch_tag "${FFMPEG_REPO}" "${FFMPEG_TAG}" "${SWD3_TP_SRC}/ffmpeg"
    fetch_tag "${TOMLPP_REPO}" "${TOMLPP_TAG}" "${SWD3_TP_SRC}/tomlplusplus"
} > "${SWD3_TP}/SOURCES.tsv.new"
mv "${SWD3_TP}/SOURCES.tsv.new" "${SWD3_TP}/SOURCES.tsv"
cat "${SWD3_TP}/SOURCES.tsv"
