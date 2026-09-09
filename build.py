#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent


class BuildConfigurationError(RuntimeError):
    pass


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Configure and build OpenSWD3 with CMake and Ninja."
    )
    parser.add_argument(
        "target",
        nargs="?",
        choices=("core", "app"),
        default="core",
        help="build target (default: core)",
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="run unit tests after a successful build",
    )
    return parser.parse_args()


def positive_job_count(name: str, default: int) -> int:
    value = os.environ.get(name, str(default))
    if not value.isdigit() or int(value) <= 0:
        raise BuildConfigurationError(f"{name} must be a positive integer.")

    return int(value)


def resolve_tool(environment_name: str, default: str) -> str:
    configured = os.environ.get(environment_name, default)
    resolved = shutil.which(configured)
    if resolved is None:
        candidate = Path(configured)
        if candidate.is_file():
            resolved = str(candidate.resolve())

    if resolved is None:
        raise BuildConfigurationError(f"Required tool not found: {configured}")

    return resolved


def cache_generator(cache_file: Path) -> str:
    if not cache_file.is_file():
        return ""

    for line in cache_file.read_text(encoding="utf-8", errors="replace").splitlines():
        key, separator, value = line.partition("=")
        if separator and key == "CMAKE_GENERATOR:INTERNAL":
            return value

    return ""


def run(command, environment=None) -> None:
    subprocess.run(command, cwd=ROOT, env=environment, check=True)


def build(arguments: argparse.Namespace) -> None:
    processor_count = os.cpu_count() or 1
    build_jobs = positive_job_count("OPENSWD3_BUILD_JOBS", processor_count)
    test_jobs = None
    if arguments.test:
        test_jobs = positive_job_count("OPENSWD3_TEST_JOBS", processor_count)

    cmake = resolve_tool("OPENSWD3_CMAKE", "cmake")
    ninja = resolve_tool("OPENSWD3_NINJA", "ninja")
    cxx = resolve_tool("CXX", "clang++")
    ctest = None
    if arguments.test:
        ctest = resolve_tool("OPENSWD3_CTEST", "ctest")

    sanitizer = os.environ.get("OPENSWD3_SANITIZER", "none")
    if sanitizer not in ("none", "address"):
        raise BuildConfigurationError(f"Unsupported sanitizer: {sanitizer}")
    if sanitizer == "address" and arguments.target != "core":
        raise BuildConfigurationError(
            "AddressSanitizer is available only for the core target."
        )
    if sanitizer == "address" and os.name == "nt":
        raise BuildConfigurationError(
            "The AddressSanitizer build wrapper is supported only on Linux."
        )

    is_windows = os.name == "nt"
    platform_prefix = "" if is_windows else "linux-"
    build_name = f"{platform_prefix}{arguments.target}"
    build_directory = ROOT / "build" / build_name
    build_application = "ON" if arguments.target == "app" else "OFF"
    expected_generator = "Ninja Multi-Config"
    build_label = build_name

    if sanitizer == "address":
        build_directory = ROOT / "build" / "linux-asan"
        expected_generator = "Ninja"
        build_label = "linux-asan"

    cache_file = build_directory / "CMakeCache.txt"
    current_generator = cache_generator(cache_file)
    if current_generator and current_generator != expected_generator:
        print(
            "[OpenSWD3] Reset generator: "
            f"{current_generator} -> {expected_generator}"
        )
        shutil.rmtree(build_directory)

    configure_arguments = [
        cmake,
        "-S",
        ".",
        "-B",
        str(build_directory.relative_to(ROOT)),
        "-G",
        expected_generator,
        f"-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja}",
        f"-DCMAKE_CXX_COMPILER:FILEPATH={cxx}",
        "-DCMAKE_CXX_EXTENSIONS:BOOL=OFF",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON",
        "-DBUILD_TESTING:BOOL=ON",
        f"-DOPENSWD3_BUILD_APP:BOOL={build_application}",
    ]

    if sanitizer == "address":
        configure_arguments.extend(
            (
                "-DCMAKE_BUILD_TYPE:STRING=Debug",
                "-DCMAKE_CXX_FLAGS:STRING=-fsanitize=address "
                "-fno-omit-frame-pointer",
                "-DCMAKE_EXE_LINKER_FLAGS:STRING=-fsanitize=address",
            )
        )

    if arguments.target == "app":
        cc = resolve_tool("CC", "clang")
        configure_arguments.append(f"-DCMAKE_C_COMPILER:FILEPATH={cc}")
        if not is_windows:
            configure_arguments.extend(
                (
                    "-DSDL_X11_XCURSOR:BOOL=OFF",
                    "-DSDL_X11_XFIXES:BOOL=OFF",
                    "-DSDL_X11_XINPUT:BOOL=OFF",
                    "-DSDL_X11_XRANDR:BOOL=OFF",
                    "-DSDL_X11_XTEST:BOOL=OFF",
                )
            )

    reconfigure = os.environ.get("OPENSWD3_RECONFIGURE", "0") == "1"
    if reconfigure or not cache_file.is_file() or not (
        build_directory / "build.ninja"
    ).is_file():
        print(f"[OpenSWD3] Configure: {build_label}")
        run(configure_arguments)
    else:
        print(f"[OpenSWD3] Configure: {build_label} (reuse Ninja cache)")

    print(
        f"[OpenSWD3] Build: {build_label}-debug "
        f"(parallel jobs: {build_jobs})"
    )
    run(
        (
            cmake,
            "--build",
            str(build_directory.relative_to(ROOT)),
            "--config",
            "Debug",
            "--parallel",
            str(build_jobs),
        )
    )

    if not arguments.test:
        print("[OpenSWD3] Build completed successfully. Unit tests were not run.")
        print("[OpenSWD3] Pass --test to run unit tests.")
        return

    print(f"[OpenSWD3] Test: Debug (parallel jobs: {test_jobs})")
    test_environment = os.environ.copy()
    if sanitizer == "address":
        test_environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"

    run(
        (
            ctest,
            "--test-dir",
            str(build_directory.relative_to(ROOT)),
            "-C",
            "Debug",
            "--parallel",
            str(test_jobs),
            "--output-on-failure",
        ),
        test_environment,
    )
    print("[OpenSWD3] Build and tests completed successfully.")


def main() -> int:
    try:
        build(parse_arguments())
    except BuildConfigurationError as error:
        print(f"[OpenSWD3] {error}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as error:
        print(
            f"[OpenSWD3] Failed with exit code {error.returncode}.",
            file=sys.stderr,
        )
        return error.returncode or 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
