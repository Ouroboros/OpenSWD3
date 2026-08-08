#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import locale
import os
import platform
import sys
import threading
from datetime import datetime, timezone
from pathlib import Path

EXPECTED_EXE_SHA256 = (
    "78ddd0acf752dde32bbc4ea5a12256954878342899309c33516efd6dace0508a"
)
RUNTIME_FILES = (
    "swd3_nodvd.exe",
    "binkw32.dll",
    "Mss32.dll",
    "Mp3dec.asi",
    "Env.dat",
)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)

    return digest.hexdigest()


def tsv_value(value: object) -> str:
    return str(value).replace("\t", "\\t").replace("\r", "\\r").replace(
        "\n", "\\n"
    )


def require_fresh_output(path: Path) -> None:
    if path.exists() and any(path.iterdir()):
        raise RuntimeError(f"输出目录不是空目录：{path}")

    path.mkdir(parents=True, exist_ok=True)


def write_run_manifest(
    output_directory: Path,
    game_directory: Path,
    agent_path: Path,
    frida_version: str,
    process_id: int,
) -> None:
    rows = [
        ("field", "value"),
        ("started_utc", datetime.now(timezone.utc).isoformat(timespec="milliseconds")),
        ("game_directory", game_directory),
        ("host_platform", platform.platform()),
        ("host_machine", platform.machine()),
        ("python_version", platform.python_version()),
        ("python_encoding", locale.getpreferredencoding(False)),
        ("frida_version", frida_version),
        ("target_pid", process_id),
        ("agent_sha256", sha256_file(agent_path)),
        ("glyph_entry", "0x004368D0"),
        ("glyph_return", "0x00436974"),
        ("capture_method", "frida_attach_interceptor_on_enter_on_leave"),
    ]

    for file_name in RUNTIME_FILES:
        path = game_directory / file_name
        rows.append((f"runtime_file.{file_name}.present", path.is_file()))
        if path.is_file():
            rows.append((f"runtime_file.{file_name}.size", path.stat().st_size))
            rows.append((f"runtime_file.{file_name}.sha256", sha256_file(path)))

    windows_directory = os.environ.get("WINDIR")
    if windows_directory:
        for file_name in ("mingliu.ttc", "mingliub.ttc"):
            path = Path(windows_directory) / "Fonts" / file_name
            if path.is_file():
                rows.append((f"font_candidate.{file_name}.size", path.stat().st_size))
                rows.append((f"font_candidate.{file_name}.sha256", sha256_file(path)))

    with (output_directory / "run.tsv").open(
        "w", encoding="utf-8", newline=""
    ) as stream:
        for key, value in rows:
            stream.write(f"{tsv_value(key)}\t{tsv_value(value)}\n")


def resolve_process_id(device, requested_process_id: int | None) -> int:
    if requested_process_id is not None:
        if requested_process_id <= 0:
            raise RuntimeError(f"PID 必须为正整数：{requested_process_id}")

        return requested_process_id

    matches = [
        process.pid
        for process in device.enumerate_processes()
        if process.name.casefold() == "swd3_nodvd.exe"
    ]
    if not matches:
        raise RuntimeError("没有找到正在运行的 swd3_nodvd.exe")
    if len(matches) != 1:
        raise RuntimeError(
            f"找到 {len(matches)} 个 swd3_nodvd.exe，请只保留一个"
        )

    return matches[0]


class CaptureWriter:
    def __init__(self, output_directory: Path) -> None:
        self._mask_directory = output_directory / "masks"
        self._mask_directory.mkdir()
        self._manifest = (output_directory / "glyph-masks.tsv").open(
            "w", encoding="utf-8", newline=""
        )
        self._manifest.write(
            "index\trenderer\tcache_key\traw_bytes\tconsumed_bytes\t"
            "width\theight\trow_bytes\tmask_bytes\tsha256\tmask_file\n"
        )
        self._manifest.flush()
        self._lock = threading.Lock()
        self.capture_count = 0
        self.error_message: str | None = None

    def close(self) -> None:
        self._manifest.close()

    def record_error(self, message: str) -> None:
        with self._lock:
            if self.error_message is None:
                self.error_message = message

        print(f"[错误] {message}", file=sys.stderr, flush=True)

    def record_mask(self, payload: dict[str, object], data: bytes | None) -> None:
        if data is None:
            self.record_error("Frida glyph-mask 消息没有二进制 mask")
            return

        required = (
            "renderer",
            "cache_key",
            "raw_bytes",
            "consumed_bytes",
            "width",
            "height",
            "row_bytes",
            "mask_bytes",
        )
        if any(field not in payload for field in required):
            self.record_error("Frida glyph-mask 消息缺少字段")
            return

        width = int(payload["width"])
        height = int(payload["height"])
        expected_size = int(payload["mask_bytes"])
        if len(data) != expected_size:
            self.record_error(
                f"mask 长度错误：收到 {len(data)}，预期 {expected_size}"
            )
            return

        with self._lock:
            self.capture_count += 1
            index = self.capture_count
            key = str(payload["cache_key"])
            if key.startswith("0x"):
                key = key[2:]

            key = key.upper()
            file_name = (
                f"glyph-{index:06d}-{width}x{height}-key-{key}.bin"
            )
            relative_path = Path("masks") / file_name
            (self._mask_directory / file_name).write_bytes(data)
            values = (
                index,
                payload["renderer"],
                payload["cache_key"],
                payload["raw_bytes"],
                payload["consumed_bytes"],
                width,
                height,
                payload["row_bytes"],
                expected_size,
                hashlib.sha256(data).hexdigest(),
                relative_path.as_posix(),
            )
            self._manifest.write("\t".join(tsv_value(value) for value in values))
            self._manifest.write("\n")
            self._manifest.flush()

        print(
            f"[捕获] #{index} key=0x{key} size={width}x{height}",
            flush=True,
        )


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="捕获 swd3_nodvd.exe 在 sub_4368D0 生成的字形 mask"
    )
    parser.add_argument("--game-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--pid", type=int)
    parser.add_argument("--self-test", action="store_true")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    if arguments.self_test:
        try:
            import frida
        except ImportError as error:
            raise RuntimeError("缺少 frida Python 包") from error

        agent_path = Path(__file__).with_name("agent.js")
        if not agent_path.is_file():
            raise RuntimeError(f"找不到 Frida agent：{agent_path}")

        device = frida.get_local_device()
        print(f"Frida {frida.__version__}; device={device.id}; self-test=ok")
        return 0

    if arguments.game_dir is None:
        if not getattr(sys, "frozen", False):
            raise RuntimeError("源码运行时必须传入 --game-dir")

        executable_directory = Path(sys.executable).resolve().parent
        if (executable_directory / "swd3_nodvd.exe").is_file():
            game_directory = executable_directory
        else:
            game_directory = executable_directory.parent
    else:
        game_directory = arguments.game_dir.resolve()

    if arguments.output is None:
        run_name = datetime.now().strftime("run-%Y%m%d-%H%M%S")
        run_name += f"-{os.getpid()}"
        output_directory = game_directory / "glyph-oracle-output" / run_name
    else:
        output_directory = arguments.output.resolve()

    executable = game_directory / "swd3_nodvd.exe"
    agent_path = Path(__file__).with_name("agent.js")
    if not executable.is_file():
        raise RuntimeError(f"找不到原版 EXE：{executable}")
    if not agent_path.is_file():
        raise RuntimeError(f"找不到 Frida agent：{agent_path}")

    actual_exe_sha256 = sha256_file(executable)
    if actual_exe_sha256 != EXPECTED_EXE_SHA256:
        raise RuntimeError(
            "swd3_nodvd.exe SHA-256 不匹配："
            f"{actual_exe_sha256} != {EXPECTED_EXE_SHA256}"
        )

    try:
        import frida
    except ImportError as error:
        raise RuntimeError("缺少 frida Python 包") from error

    device = frida.get_local_device()
    process_id = resolve_process_id(device, arguments.pid)
    require_fresh_output(output_directory)

    write_run_manifest(
        output_directory,
        game_directory,
        agent_path,
        frida.__version__,
        process_id,
    )
    writer = CaptureWriter(output_directory)
    detached = threading.Event()
    ready = threading.Event()

    def on_message(message, data) -> None:
        if message.get("type") == "error":
            writer.record_error(str(message.get("stack", message)))
            return

        payload = message.get("payload")
        if message.get("type") != "send" or not isinstance(payload, dict):
            writer.record_error(f"未知 Frida 消息：{message}")
            return

        message_type = payload.get("type")
        if message_type == "ready":
            print(
                f"[就绪] hook={payload.get('glyph_entry')} "
                f"image={payload.get('image_base')}",
                flush=True,
            )
            ready.set()
        elif message_type == "glyph-mask":
            writer.record_mask(payload, data)
        elif message_type == "capture-error":
            writer.record_error(str(payload))
        else:
            writer.record_error(f"未知 agent 消息：{payload}")

    def on_detached(reason, crash) -> None:
        if crash is None:
            print(f"[结束] Frida session detached: {reason}", flush=True)
        else:
            writer.record_error(f"目标进程异常结束：{reason} {crash}")

        detached.set()

    session = None
    try:
        session = device.attach(process_id)
        session.on("detached", on_detached)
        script = session.create_script(agent_path.read_text(encoding="utf-8"))
        script.on("message", on_message)
        script.load()
        if not ready.wait(timeout=10.0):
            raise RuntimeError("Frida agent 在 10 秒内没有报告就绪")

        print(
            f"[已附加] PID={process_id}；请继续操作原版。",
            flush=True,
        )
        while not detached.wait(timeout=0.25):
            if writer.error_message is not None:
                raise RuntimeError(writer.error_message)
    except KeyboardInterrupt:
        print("\n[停止] 已停止捕获；原版进程不会被本工具强制结束。", flush=True)
    finally:
        if session is not None and not detached.is_set():
            session.detach()

        writer.close()

    print(f"[完成] 共捕获 {writer.capture_count} 个 glyph mask", flush=True)
    return 1 if writer.error_message is not None else 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"错误：{error}", file=sys.stderr)
        if getattr(sys, "frozen", False) and sys.platform == "win32":
            import ctypes

            ctypes.windll.user32.MessageBoxW(
                None,
                str(error),
                "OpenSWD3 Glyph Oracle",
                0x10,
            )

        raise SystemExit(1) from None
