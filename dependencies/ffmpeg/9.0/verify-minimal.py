#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

SOURCE_BYTES = 12_032_020
SOURCE_SHA256 = "7f607a00dd0d28a729d5a4811205812eef01cf6ef6155025febb6f36a9062d52"
WINDOWS_BASELINE_BYTES = 99_364_864
COMPONENTS = {
    "avformat": ("63", "63.1.100"),
    "avcodec": ("63", "63.1.100"),
    "avutil": ("61", "61.1.100"),
    "swresample": ("7", "7.1.100"),
    "swscale": ("10", "10.1.100"),
}
EXPECTED_CONFIGURE = {
    "--disable-everything",
    "--disable-autodetect",
    "--disable-network",
    "--disable-programs",
    "--disable-avdevice",
    "--disable-avfilter",
    "--enable-small",
    "--enable-shared",
    "--disable-static",
    "--enable-demuxer='bink,mp3'",
    "--enable-decoder='bink,binkaudio_dct,binkaudio_rdft,mp3float'",
    "--enable-parser=mpegaudio",
    "--enable-protocol=file",
}
FORBIDDEN_CONFIGURE = {"--enable-gpl", "--enable-version3", "--enable-nonfree"}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_file(path: Path) -> None:
    if not path.is_file():
        raise RuntimeError(f"required file is unavailable: {path}")


def needed_libraries(command: list[str], pattern: str) -> list[str]:
    output = subprocess.check_output(command, text=True)
    return sorted(set(re.findall(pattern, output)))


def windows_configuration(root: Path) -> str:
    output = subprocess.check_output(
        ["x86_64-w64-mingw32-strings", str(root / "bin/avcodec-63.dll")],
        text=True,
    ).splitlines()
    configuration = next(
        (line for line in output if line.startswith("--disable-everything ")),
        "",
    )
    if not configuration:
        raise RuntimeError("Windows FFmpeg configuration string is unavailable")
    return configuration


def linux_configuration(root: Path) -> tuple[str, str]:
    code = """
import ctypes
codec = ctypes.CDLL('libavcodec.so.63')
codec.avcodec_configuration.restype = ctypes.c_char_p
util = ctypes.CDLL('libavutil.so.61')
util.av_version_info.restype = ctypes.c_char_p
print(util.av_version_info().decode())
print(codec.avcodec_configuration().decode())
"""
    environment = dict(os.environ)
    environment["LD_LIBRARY_PATH"] = str(root / "lib")
    output = subprocess.check_output(
        [sys.executable, "-c", code], text=True, env=environment
    ).splitlines()
    if len(output) != 2:
        raise RuntimeError("unexpected FFmpeg version/configuration output")
    return output[0], output[1]


def verify(project_root: Path) -> dict[str, object]:
    cache_root = project_root / "build/dependencies/ffmpeg/9.0"
    archive = cache_root / "source/ffmpeg-9.0.tar.xz"
    require_file(archive)
    if archive.stat().st_size != SOURCE_BYTES:
        raise RuntimeError("FFmpeg source byte count does not match the lock")
    if sha256(archive) != SOURCE_SHA256:
        raise RuntimeError("FFmpeg source SHA256 does not match the lock")

    linux_root = cache_root / "self-built/linux-x64"
    windows_root = cache_root / "self-built/windows-x64"
    for root in (linux_root, windows_root):
        require_file(root / "LICENSE.txt")
        require_file(root / "BUILDINFO.txt")
        require_file(root / "include/libavformat/avformat.h")
        require_file(root / "include/libavcodec/avcodec.h")
        require_file(root / "include/libavutil/avutil.h")
        require_file(root / "include/libswresample/swresample.h")
        require_file(root / "include/libswscale/swscale.h")

    linux_runtime: list[Path] = []
    windows_runtime: list[Path] = []
    windows_imports: list[Path] = []
    linux_needed: dict[str, list[str]] = {}
    windows_needed: dict[str, list[str]] = {}
    for component, (major, full_version) in COMPONENTS.items():
        linux_library = linux_root / f"lib/lib{component}.so.{full_version}"
        linux_link = linux_root / f"lib/lib{component}.so.{major}"
        windows_library = windows_root / f"bin/{component}-{major}.dll"
        msvc_import = windows_root / f"bin/{component}.lib"
        mingw_import = windows_root / f"lib/lib{component}.dll.a"
        definition = windows_root / f"lib/{component}-{major}.def"
        for path in (
            linux_library,
            linux_link,
            windows_library,
            msvc_import,
            mingw_import,
            definition,
        ):
            require_file(path)
        linux_runtime.append(linux_library)
        windows_runtime.append(windows_library)
        windows_imports.extend((msvc_import, mingw_import, definition))
        linux_needed[component] = needed_libraries(
            ["readelf", "-d", str(linux_library)], r"Shared library: \[([^]]+)\]"
        )
        windows_needed[component] = needed_libraries(
            ["x86_64-w64-mingw32-objdump", "-p", str(windows_library)],
            r"DLL Name: ([^\r\n]+)",
        )

    linux_allowed = {
        "libavcodec.so.63",
        "libavutil.so.61",
        "libc.so.6",
        "libm.so.6",
    }
    windows_allowed = {
        "KERNEL32.dll",
        "bcrypt.dll",
        "msvcrt.dll",
        "avcodec-63.dll",
        "avutil-61.dll",
    }
    unexpected_linux: dict[str, list[str]] = {}
    for component, libraries in linux_needed.items():
        unexpected = sorted(set(libraries) - linux_allowed)
        if unexpected:
            unexpected_linux[component] = unexpected
    unexpected_windows: dict[str, list[str]] = {}
    for component, libraries in windows_needed.items():
        unexpected = sorted(set(libraries) - windows_allowed)
        if unexpected:
            unexpected_windows[component] = unexpected
    if unexpected_linux:
        raise RuntimeError(f"unexpected Linux runtime dependencies: {unexpected_linux}")
    if unexpected_windows:
        raise RuntimeError(f"unexpected Windows runtime dependencies: {unexpected_windows}")

    windows_build_info = (windows_root / "BUILDINFO.txt").read_text(encoding="utf-8")
    windows_configure = windows_configuration(windows_root)
    for option in ("--disable-pthreads", "--enable-w32threads", "-static-libgcc"):
        if option not in windows_build_info or option not in windows_configure:
            raise RuntimeError(f"missing Windows build option: {option}")
    windows_missing_options = sorted(
        option for option in EXPECTED_CONFIGURE if option not in windows_configure
    )
    if windows_missing_options:
        raise RuntimeError(
            f"missing Windows configure options: {windows_missing_options}"
        )
    windows_forbidden_options = sorted(
        option for option in FORBIDDEN_CONFIGURE if option in windows_configure
    )
    if windows_forbidden_options:
        raise RuntimeError(
            f"forbidden Windows configure options: {windows_forbidden_options}"
        )

    version, configuration = linux_configuration(linux_root)
    if not version.startswith("9.0"):
        raise RuntimeError(f"unexpected linked FFmpeg version: {version}")
    missing_options = sorted(
        option for option in EXPECTED_CONFIGURE if option not in configuration
    )
    if missing_options:
        raise RuntimeError(f"missing configure options: {missing_options}")
    forbidden_options = sorted(
        option for option in FORBIDDEN_CONFIGURE if option in configuration
    )
    if forbidden_options:
        raise RuntimeError(f"forbidden configure options: {forbidden_options}")

    linux_bytes = sum(path.stat().st_size for path in linux_runtime)
    windows_bytes = sum(path.stat().st_size for path in windows_runtime)
    reduction = (WINDOWS_BASELINE_BYTES - windows_bytes) * 100.0 / WINDOWS_BASELINE_BYTES
    return {
        "source": {
            "bytes": SOURCE_BYTES,
            "sha256": SOURCE_SHA256,
        },
        "version": version,
        "configuration": configuration,
        "linux": {
            "runtime_bytes": linux_bytes,
            "runtime_mib": round(linux_bytes / (1024 * 1024), 2),
            "sha256": {path.name: sha256(path) for path in linux_runtime},
            "needed": linux_needed,
        },
        "windows": {
            "runtime_bytes": windows_bytes,
            "runtime_mib": round(windows_bytes / (1024 * 1024), 2),
            "baseline_bytes": WINDOWS_BASELINE_BYTES,
            "reduction_percent": round(reduction, 2),
            "configuration": windows_configure,
            "sha256": {path.name: sha256(path) for path in windows_runtime},
            "import_files": [str(path.relative_to(windows_root)) for path in windows_imports],
            "needed": windows_needed,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[3],
    )
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()
    report = verify(arguments.project_root.resolve())
    text = json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if arguments.output is not None:
        arguments.output.parent.mkdir(parents=True, exist_ok=True)
        arguments.output.write_text(text, encoding="utf-8")
    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
