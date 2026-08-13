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
    "4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a"
)
TARGET_EXE = "swd32.exe"
RUNTIME_FILES = (
    TARGET_EXE,
    "binkw32.dll",
    "Mss32.dll",
    "Mp3dec.asi",
    "Env.dat",
)
ROLE_FIELDS = (
    "sample_ms",
    "guid",
    "role_address",
    "world_x",
    "world_y",
    "role_offset_x",
    "role_offset_y",
    "flags",
    "talk_script_id",
    "action_id",
    "base_variant",
    "variant_delta",
    "draw_offset_x",
    "draw_offset_y",
    "resource_id",
    "frame_index",
    "mode_flags",
    "camera_left",
    "camera_top",
    "destination_x",
    "destination_y",
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
        ("target_exe", TARGET_EXE),
        ("target_pid", process_id),
        ("agent_sha256", sha256_file(agent_path)),
        ("glyph_entry", "0x004368D0"),
        ("glyph_return", "0x00436974"),
        ("role_draw_entry", "0x00413910"),
        ("capture_method", "frida_spawn_attach_hooks_resume"),
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


class CaptureWriter:
    def __init__(self, output_directory: Path) -> None:
        self._mask_directory = output_directory / "masks"
        self._mask_directory.mkdir()
        self._glyph_manifest = (output_directory / "glyph-masks.tsv").open(
            "w", encoding="utf-8", newline=""
        )
        self._glyph_manifest.write(
            "index\trenderer\tcache_key\traw_bytes\tconsumed_bytes\t"
            "width\theight\trow_bytes\tmask_bytes\tsha256\tmask_file\n"
        )
        self._font_manifest = (output_directory / "font-selections.tsv").open(
            "w", encoding="utf-8", newline=""
        )
        self._font_manifest.write(
            "index\trenderer\twidth\theight\tface_name\ttm_height\t"
            "tm_ascent\ttm_descent\ttm_internal_leading\t"
            "tm_external_leading\ttm_average_char_width\t"
            "tm_maximum_char_width\ttm_weight\ttm_overhang\t"
            "tm_pitch_and_family\ttm_charset\n"
        )
        self._role_manifest = (output_directory / "role-placement.tsv").open(
            "w", encoding="utf-8", newline=""
        )
        self._role_manifest.write("\t".join(ROLE_FIELDS) + "\n")
        self._glyph_manifest.flush()
        self._font_manifest.flush()
        self._role_manifest.flush()
        self._lock = threading.Lock()
        self.glyph_count = 0
        self.font_selection_count = 0
        self.role_sample_count = 0
        self.error_message: str | None = None

    def close(self) -> None:
        self._glyph_manifest.close()
        self._font_manifest.close()
        self._role_manifest.close()

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
            self.glyph_count += 1
            index = self.glyph_count
            key = str(payload["cache_key"])
            if key.startswith("0x"):
                key = key[2:]
            key = key.upper()
            file_name = f"glyph-{index:06d}-{width}x{height}-key-{key}.bin"
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
            self._glyph_manifest.write(
                "\t".join(tsv_value(value) for value in values) + "\n"
            )
            self._glyph_manifest.flush()

        print(f"[字形] #{index} key=0x{key} size={width}x{height}", flush=True)

    def record_font_selection(self, payload: dict[str, object]) -> None:
        fields = (
            "renderer",
            "width",
            "height",
            "face_name",
            "tm_height",
            "tm_ascent",
            "tm_descent",
            "tm_internal_leading",
            "tm_external_leading",
            "tm_average_char_width",
            "tm_maximum_char_width",
            "tm_weight",
            "tm_overhang",
            "tm_pitch_and_family",
            "tm_charset",
        )
        if any(field not in payload for field in fields):
            self.record_error("Frida font-selection 消息缺少字段")
            return

        with self._lock:
            self.font_selection_count += 1
            values = (self.font_selection_count,) + tuple(
                payload[field] for field in fields
            )
            self._font_manifest.write(
                "\t".join(tsv_value(value) for value in values) + "\n"
            )
            self._font_manifest.flush()

        print(
            f"[字体] size={payload['width']}x{payload['height']} "
            f"face={payload['face_name']}",
            flush=True,
        )

    def record_role_placement(self, payload: dict[str, object]) -> None:
        if any(field not in payload for field in ROLE_FIELDS):
            self.record_error("Frida role-placement 消息缺少字段")
            return

        with self._lock:
            self.role_sample_count += 1
            self._role_manifest.write(
                "\t".join(tsv_value(payload[field]) for field in ROLE_FIELDS) + "\n"
            )
            self._role_manifest.flush()

        if self.role_sample_count == 1:
            print("[角色] 已开始记录 GUID 248/249", flush=True)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="同时捕获 swd32.exe 的字形 mask 与角色位置"
    )
    parser.add_argument("--game-dir", type=Path)
    parser.add_argument("--output", type=Path)
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
        if not callable(getattr(device, "spawn", None)) or not callable(
            getattr(device, "resume", None)
        ):
            raise RuntimeError("当前 Frida device 不支持 spawn/resume")
        print(f"Frida {frida.__version__}; device={device.id}; self-test=ok")
        return 0

    if arguments.game_dir is None:
        if not getattr(sys, "frozen", False):
            raise RuntimeError("源码运行时必须传入 --game-dir")

        executable_directory = Path(sys.executable).resolve().parent
        if (executable_directory / TARGET_EXE).is_file():
            game_directory = executable_directory
        else:
            game_directory = executable_directory.parent
    else:
        game_directory = arguments.game_dir.resolve()

    if arguments.output is None:
        run_name = datetime.now().strftime("run-%Y%m%d-%H%M%S")
        run_name += f"-{os.getpid()}"
        output_directory = game_directory / "swd3-oracle-output" / run_name
    else:
        output_directory = arguments.output.resolve()

    executable = game_directory / TARGET_EXE
    agent_path = Path(__file__).with_name("agent.js")
    if not executable.is_file():
        raise RuntimeError(f"找不到原版 EXE：{executable}")
    if not agent_path.is_file():
        raise RuntimeError(f"找不到 Frida agent：{agent_path}")

    actual_exe_sha256 = sha256_file(executable)
    if actual_exe_sha256 != EXPECTED_EXE_SHA256:
        raise RuntimeError(
            f"{TARGET_EXE} SHA-256 不匹配："
            f"{actual_exe_sha256} != {EXPECTED_EXE_SHA256}"
        )

    try:
        import frida
    except ImportError as error:
        raise RuntimeError("缺少 frida Python 包") from error

    device = frida.get_local_device()
    require_fresh_output(output_directory)
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
                f"[就绪] glyph={payload.get('glyph_entry')} "
                f"role={payload.get('role_draw_entry')} "
                f"image={payload.get('image_base')}",
                flush=True,
            )
            ready.set()
        elif message_type == "glyph-mask":
            writer.record_mask(payload, data)
        elif message_type == "font-selection":
            writer.record_font_selection(payload)
        elif message_type == "role-placement":
            writer.record_role_placement(payload)
        elif message_type == "capture-error":
            writer.record_error(str(payload.get("message", payload)))
        else:
            writer.record_error(f"未知 agent 消息：{payload}")

    def on_detached(reason, crash) -> None:
        if crash is None:
            print(f"[结束] Frida session detached: {reason}", flush=True)
        else:
            writer.record_error(f"目标进程异常结束：{reason} {crash}")

        detached.set()

    process_id = None
    process_resumed = False
    session = None
    try:
        process_id = device.spawn(
            str(executable),
            argv=[str(executable)],
            cwd=str(game_directory),
        )
        write_run_manifest(
            output_directory,
            game_directory,
            agent_path,
            frida.__version__,
            process_id,
        )
        session = device.attach(process_id)
        session.on("detached", on_detached)
        script = session.create_script(agent_path.read_text(encoding="utf-8"))
        script.on("message", on_message)
        script.load()
        if not ready.wait(timeout=10.0):
            raise RuntimeError("Frida agent 在 10 秒内没有报告就绪")

        device.resume(process_id)
        process_resumed = True
        print(
            f"[已启动] PID={process_id}；两个 hook 均已装入，请操作原版。",
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

        if process_id is not None and not process_resumed:
            try:
                device.kill(process_id)
            except frida.ProcessNotFoundError:
                pass

        writer.close()

    print(
        f"[完成] glyph={writer.glyph_count}，字体={writer.font_selection_count}，"
        f"角色样本={writer.role_sample_count}",
        flush=True,
    )
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
                "OpenSWD3 Oracle",
                0x10,
            )

        raise SystemExit(1) from None
