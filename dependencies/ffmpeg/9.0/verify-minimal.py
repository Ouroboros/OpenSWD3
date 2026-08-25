#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

SOURCE_BYTES = 12_032_020
SOURCE_SHA256 = "7f607a00dd0d28a729d5a4811205812eef01cf6ef6155025febb6f36a9062d52"
WINDOWS_BASELINE_BYTES = 99_364_864
COMPONENTS = ("avformat", "avcodec", "avutil", "swresample", "swscale")
EXPECTED_CONFIGURE = {
    "--disable-everything",
    "--disable-autodetect",
    "--disable-network",
    "--disable-programs",
    "--disable-avdevice",
    "--disable-avfilter",
    "--enable-small",
    "--disable-shared",
    "--enable-static",
    "--enable-demuxer='bink,mp3'",
    "--enable-decoder='bink,binkaudio_dct,binkaudio_rdft,mp3float'",
    "--enable-parser=mpegaudio",
    "--enable-protocol=file",
}
FORBIDDEN_CONFIGURE = {
    "--enable-shared",
    "--disable-static",
    "--enable-gpl",
    "--enable-version3",
    "--enable-nonfree",
}
SPLIT_RUNTIME_PATTERNS = (
    "*.dll",
    "*.dll.a",
    "*.so",
    "*.so.*",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_file(path: Path) -> None:
    if not path.is_file():
        raise RuntimeError(f"required file is unavailable: {path}")


def command_output(command: list[str]) -> str:
    return subprocess.check_output(command, text=True, errors="replace")


def archive_members(path: Path) -> list[str]:
    members = [line for line in command_output(["ar", "t", str(path)]).splitlines() if line]
    if not members:
        raise RuntimeError(f"static archive is empty: {path}")
    return members


def archive_strings(path: Path) -> list[str]:
    return command_output(["strings", str(path)]).splitlines()


def archive_configuration(path: Path) -> str:
    configuration = next(
        (line for line in archive_strings(path) if line.startswith("--disable-everything ")),
        "",
    )
    if not configuration:
        raise RuntimeError(f"FFmpeg configuration string is unavailable: {path}")
    return configuration


def archive_version(path: Path) -> str:
    version_line = next(
        (line for line in archive_strings(path) if line.startswith("FFmpeg version ")),
        "",
    )
    if not version_line:
        raise RuntimeError(f"FFmpeg version string is unavailable: {path}")
    return version_line.removeprefix("FFmpeg version ")


def verify_configuration(configuration: str, platform: str) -> None:
    missing = sorted(option for option in EXPECTED_CONFIGURE if option not in configuration)
    if missing:
        raise RuntimeError(f"missing {platform} configure options: {missing}")
    forbidden = sorted(option for option in FORBIDDEN_CONFIGURE if option in configuration)
    if forbidden:
        raise RuntimeError(f"forbidden {platform} configure options: {forbidden}")


def needed_libraries(command: list[str], pattern: str) -> list[str]:
    return sorted(set(re.findall(pattern, command_output(command))))


def verify_wrapper(path: Path, platform: str) -> dict[str, object]:
    require_file(path)
    if platform == "linux":
        needed = needed_libraries(
            ["readelf", "-d", str(path)], r"Shared library: \[([^]]+)\]"
        )
    else:
        needed = needed_libraries(
            ["x86_64-w64-mingw32-objdump", "-p", str(path)],
            r"DLL Name: ([^\r\n]+)",
        )
    split_dependencies = [
        library
        for library in needed
        if re.match(r"(?:lib)?(?:avformat|avcodec|avutil|swresample|swscale)", library)
    ]
    if split_dependencies:
        raise RuntimeError(
            f"{platform} wrapper still depends on split FFmpeg runtimes: "
            f"{split_dependencies}"
        )
    if platform == "windows":
        forbidden_runtime = [
            library
            for library in needed
            if library.lower().startswith(("libgcc", "libatomic", "libwinpthread"))
        ]
        if forbidden_runtime:
            raise RuntimeError(
                f"Windows wrapper depends on extra compiler runtimes: {forbidden_runtime}"
            )
    return {
        "path": str(path),
        "bytes": path.stat().st_size,
        "sha256": sha256(path),
        "needed": needed,
    }


def verify(project_root: Path, linux_wrapper: Path | None, windows_wrapper: Path | None) -> dict[str, object]:
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
        split_runtimes = sorted(
            str(path.relative_to(root))
            for pattern in SPLIT_RUNTIME_PATTERNS
            for path in root.rglob(pattern)
        )
        if split_runtimes:
            raise RuntimeError(f"split FFmpeg runtime remains in {root}: {split_runtimes}")

    linux_archives = [linux_root / f"lib/lib{component}.a" for component in COMPONENTS]
    windows_archives = [windows_root / f"lib/{component}.lib" for component in COMPONENTS]
    archive_member_counts: dict[str, dict[str, int]] = {"linux": {}, "windows": {}}
    for path in linux_archives:
        require_file(path)
        archive_member_counts["linux"][path.name] = len(archive_members(path))
    for path in windows_archives:
        require_file(path)
        archive_member_counts["windows"][path.name] = len(archive_members(path))

    linux_configuration = archive_configuration(linux_root / "lib/libavcodec.a")
    windows_configuration = archive_configuration(windows_root / "lib/avcodec.lib")
    verify_configuration(linux_configuration, "Linux")
    verify_configuration(windows_configuration, "Windows")

    linux_version = archive_version(linux_root / "lib/libavutil.a")
    windows_version = archive_version(windows_root / "lib/avutil.lib")
    if not linux_version.startswith("9.0") or not windows_version.startswith("9.0"):
        raise RuntimeError(
            f"unexpected FFmpeg versions: Linux={linux_version}, Windows={windows_version}"
        )

    windows_build_info = (windows_root / "BUILDINFO.txt").read_text(encoding="utf-8")
    for option in (
        "--enable-cross-compile",
        "--toolchain=msvc",
        "--cc=clang-cl.exe",
        "--ar=llvm-ar.exe",
        "--disable-pthreads",
        "--enable-w32threads",
        "/Brepro",
    ):
        if option not in windows_build_info or option not in windows_configuration:
            raise RuntimeError(f"missing Windows build option: {option}")

    linux_bytes = sum(path.stat().st_size for path in linux_archives)
    windows_bytes = sum(path.stat().st_size for path in windows_archives)
    report: dict[str, object] = {
        "source": {"bytes": SOURCE_BYTES, "sha256": SOURCE_SHA256},
        "version": linux_version,
        "configuration": linux_configuration,
        "linux": {
            "archive_bytes": linux_bytes,
            "archive_mib": round(linux_bytes / (1024 * 1024), 2),
            "sha256": {path.name: sha256(path) for path in linux_archives},
            "member_counts": archive_member_counts["linux"],
        },
        "windows": {
            "archive_bytes": windows_bytes,
            "archive_mib": round(windows_bytes / (1024 * 1024), 2),
            "baseline_bytes": WINDOWS_BASELINE_BYTES,
            "configuration": windows_configuration,
            "sha256": {path.name: sha256(path) for path in windows_archives},
            "member_counts": archive_member_counts["windows"],
        },
    }
    if linux_wrapper is not None:
        report["linux"]["wrapper"] = verify_wrapper(linux_wrapper, "linux")  # type: ignore[index]
    if windows_wrapper is not None:
        wrapper_report = verify_wrapper(windows_wrapper, "windows")
        wrapper_report["reduction_percent"] = round(
            (WINDOWS_BASELINE_BYTES - int(wrapper_report["bytes"]))
            * 100.0
            / WINDOWS_BASELINE_BYTES,
            2,
        )
        report["windows"]["wrapper"] = wrapper_report  # type: ignore[index]
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--project-root", type=Path, default=Path(__file__).resolve().parents[3]
    )
    parser.add_argument("--linux-wrapper", type=Path)
    parser.add_argument("--windows-wrapper", type=Path)
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()
    project_root = arguments.project_root.resolve()
    linux_wrapper = (
        arguments.linux_wrapper.resolve() if arguments.linux_wrapper is not None else None
    )
    windows_wrapper = (
        arguments.windows_wrapper.resolve()
        if arguments.windows_wrapper is not None
        else None
    )
    report = verify(project_root, linux_wrapper, windows_wrapper)
    text = json.dumps(report, ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if arguments.output is not None:
        arguments.output.parent.mkdir(parents=True, exist_ok=True)
        arguments.output.write_text(text, encoding="utf-8")
    sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
