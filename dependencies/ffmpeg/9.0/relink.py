#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

COMPONENTS = ("avformat", "avcodec", "swresample", "swscale", "avutil")


def require_file(path: Path) -> Path:
    resolved = path.resolve()
    if not resolved.is_file():
        raise RuntimeError(f"缺少重链接输入：{resolved}")
    return resolved


def windows_path(path: Path) -> str:
    resolved = path.resolve()
    if sys.platform == "win32":
        return str(resolved)
    wslpath = shutil.which("wslpath")
    if wslpath is None:
        raise RuntimeError("从非Windows环境调用Windows编译器需要wslpath")
    return subprocess.check_output(
        [wslpath, "-w", str(resolved)], text=True
    ).strip()


def linux_command(
    compiler: str,
    output: Path,
    objects: list[Path],
    ffmpeg_root: Path,
    sdl_library: Path,
) -> list[str]:
    libraries = [require_file(ffmpeg_root / "lib" / f"lib{name}.a") for name in COMPONENTS]
    return [
        compiler,
        "-shared",
        "-o",
        str(output),
        *(str(path) for path in objects),
        "-Wl,--start-group",
        *(str(path) for path in libraries),
        "-Wl,--end-group",
        str(sdl_library),
        "-pthread",
        "-lm",
        "-latomic",
        "-Wl,--gc-sections",
        "-Wl,--exclude-libs,ALL",
        "-Wl,-rpath,$ORIGIN",
    ]


def windows_command(
    compiler: str,
    output: Path,
    objects: list[Path],
    ffmpeg_root: Path,
    sdl_library: Path,
    configuration: str,
) -> list[str]:
    libraries = [require_file(ffmpeg_root / "lib" / f"{name}.lib") for name in COMPONENTS]
    debug = configuration.lower() == "debug"
    compile_runtime = "msvcrtd" if debug else "msvcrt"
    mode_flags = (
        ["-O0", "-D_DEBUG", "-g", "-Xclang", "-gcodeview"]
        if debug
        else ["-O3", "-DNDEBUG"]
    )
    return [
        compiler,
        "--target=x86_64-pc-windows-msvc",
        "-nostartfiles",
        "-nostdlib",
        *mode_flags,
        "-D_DLL",
        "-D_MT",
        "-Xclang",
        f"--dependent-lib={compile_runtime}",
        "-shared",
        "-fuse-ld=lld-link",
        "-o",
        windows_path(output),
        *(windows_path(path) for path in objects),
        *(windows_path(path) for path in libraries),
        windows_path(sdl_library),
        "-lbcrypt.lib",
        "-lole32.lib",
        "-luser32.lib",
        "-lkernel32",
        "-lgdi32",
        "-lwinspool",
        "-lshell32",
        "-loleaut32",
        "-luuid",
        "-lcomdlg32",
        "-ladvapi32",
        "-loldnames",
        "-Wl,/OPT:REF,/OPT:ICF,/Brepro",
    ]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="用可修改的FFmpeg 9.0静态库重新链接openswd3_ffmpeg"
    )
    parser.add_argument("--platform", choices=("linux", "windows"), required=True)
    parser.add_argument("--kit-root", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--ffmpeg-root", type=Path)
    parser.add_argument("--sdl-library", type=Path)
    parser.add_argument("--compiler")
    parser.add_argument("--configuration", choices=("Debug", "Release"))
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()

    kit_root = arguments.kit_root.resolve()
    ffmpeg_root = (
        arguments.ffmpeg_root.resolve()
        if arguments.ffmpeg_root is not None
        else kit_root / "ffmpeg"
    )
    objects = sorted(
        path.resolve()
        for path in (kit_root / "objects").iterdir()
        if path.suffix.lower() in (".o", ".obj")
    )
    if not objects:
        raise RuntimeError(f"重链接包中没有媒体对象文件：{kit_root / 'objects'}")

    if arguments.sdl_library is not None:
        sdl_library = require_file(arguments.sdl_library)
    else:
        candidates = sorted((kit_root / "third-party").glob("*SDL3*"))
        if len(candidates) != 1:
            raise RuntimeError("请用--sdl-library指定唯一的SDL3链接库")
        sdl_library = require_file(candidates[0])

    if arguments.platform == "windows":
        compiler = arguments.compiler or "clang++.exe"
        configuration = arguments.configuration or (
            "Debug" if "Debug" in kit_root.parts else "Release"
        )
        output = (arguments.output or kit_root / "relinked/openswd3_ffmpeg.dll").resolve()
        command = windows_command(
            compiler,
            output,
            objects,
            ffmpeg_root,
            sdl_library,
            configuration,
        )
    else:
        compiler = arguments.compiler or "c++"
        output = (
            arguments.output or kit_root / "relinked/libopenswd3_ffmpeg.so"
        ).resolve()
        command = linux_command(compiler, output, objects, ffmpeg_root, sdl_library)

    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(command, check=True, cwd=kit_root)
    if not output.is_file():
        raise RuntimeError(f"编译器没有生成预期媒体库：{output}")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
