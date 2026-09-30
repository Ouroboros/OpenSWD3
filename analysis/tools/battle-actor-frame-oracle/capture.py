#!/usr/bin/env python3
"""Read-only spawn oracle for the four physical sub_479850 callers."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import re
import sys
import threading
from datetime import datetime, timezone
from pathlib import Path

EXPECTED_SHA256 = "4c4c226876fd2f3169bfe62c58ede86bba59e0036b7cef4cfaf7d49475c03f2a"
TARGET = "swd32.exe"


def digest(path: Path) -> str:
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            result.update(chunk)
    return result.hexdigest()


class Recorder:
    def __init__(self, output: Path) -> None:
        self.output = output
        self.events = (output / "events.jsonl").open("w", encoding="utf-8", newline="\n")
        self.lock = threading.Lock()
        self.entries = 0
        self.leaves = 0
        self.blocks = 0
        self.exceptions = 0
        self.failure: str | None = None
        self.entered: set[int] = set()
        self.returned: set[int] = set()
        self.traced: set[int] = set()
        self.incomplete: set[int] = set()
        self.flags_missing: set[int] = set()
        self.snapshot_files: set[str] = set()

    def close(self) -> None:
        self.events.close()

    def summary(self) -> dict:
        paired = self.entered & self.returned & self.traced
        usable = paired - self.incomplete
        return {
            "entries": self.entries,
            "leaves": self.leaves,
            "target_blocks": self.blocks,
            "exceptions": self.exceptions,
            "paired_sequences": sorted(paired),
            "incomplete_sequences": sorted(self.incomplete),
            "flags_unavailable_sequences": sorted(self.flags_missing),
            "snapshot_pairs_with_blocks": sorted(usable),
            "unreturned_sequences": sorted(self.entered - self.returned),
            "failure": self.failure,
            "original_diff_verified": False,
            "trace_drain_completion_verified": False,
        }

    def record(self, payload: dict, data: bytes | None) -> None:
        with self.lock:
            payload = dict(payload)
            kind = payload.get("type")
            sequence = payload.get("sequence")
            phase = payload.get("phase")
            if kind in ("frame", "memory", "blocks", "exception", "trace-end"):
                if type(sequence) is not int or sequence <= 0:
                    self.failure = f"invalid event sequence: {sequence}"
                    return

            if kind in ("frame", "memory"):
                if not isinstance(phase, str) or not re.fullmatch(
                    r"enter|leave|exception-[1-9][0-9]*", phase
                ):
                    self.failure = f"invalid snapshot phase: {phase}"
                    return

                count_field = "actor_bytes" if kind == "frame" else "byte_count"
                count = payload.get(count_field)
                if type(count) is not int or count != len(data or b""):
                    self.failure = f"invalid snapshot size: {sequence}/{phase}"
                elif kind == "frame" and (count != 0x2B00 or payload.get("problem")):
                    self.incomplete.add(sequence)
                elif data:
                    region = "actor" if kind == "frame" else payload.get("region")
                    if region not in ("actor", "stack", "frame-header", "list-head-word"):
                        self.failure = f"invalid snapshot region: {region}"
                        return

                    name = f"{region}-{sequence:06d}-{phase}.bin"
                    if name in self.snapshot_files:
                        self.failure = f"duplicate snapshot: {name}"
                        return

                    (self.output / name).write_bytes(data)
                    self.snapshot_files.add(name)
                    payload["snapshot_file"] = name
                    payload["snapshot_sha256"] = hashlib.sha256(data).hexdigest()
                elif kind == "memory" and payload.get("region") == "stack":
                    self.incomplete.add(sequence)

                if kind == "frame":
                    if not payload.get("registers", {}).get("flags_available", False):
                        self.flags_missing.add(sequence)
                    if phase == "enter":
                        self.entries += 1
                        self.entered.add(sequence)
                    elif phase == "leave":
                        self.leaves += 1
                        self.returned.add(sequence)
            elif kind == "blocks":
                blocks = payload.get("blocks")
                if not isinstance(blocks, list):
                    self.failure = "invalid block packet"
                    return

                self.blocks += len(blocks)
                if blocks:
                    self.traced.add(sequence)
                if payload.get("dropped_total", 0) > 0:
                    self.incomplete.add(sequence)
            elif kind == "exception":
                self.exceptions += 1
            elif kind == "capture-error":
                self.failure = str(payload.get("message", payload))
            self.events.write(json.dumps(payload, ensure_ascii=False, sort_keys=True) + "\n")
            self.events.flush()
            if kind in ("warning", "capture-error", "exception"):
                print(f"[{kind}] {payload}", file=sys.stderr, flush=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game-dir", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        import frida
    except ImportError as error:
        raise RuntimeError("缺少 frida Python 包") from error

    agent = Path(__file__).resolve().with_name("agent.js")
    if not agent.is_file():
        raise RuntimeError(f"找不到 Frida agent：{agent}")
    device = frida.get_local_device()
    if args.self_test:
        if not callable(getattr(device, "spawn", None)) or not callable(
            getattr(device, "resume", None)
        ):
            raise RuntimeError("Frida device 不支持 spawn/resume")
        print(f"Frida {frida.__version__}; device={device.id}; agent={digest(agent)}; self-test=ok")
        return 0

    if args.game_dir is None:
        if not getattr(sys, "frozen", False):
            raise RuntimeError("源码运行必须提供 --game-dir")
        binary_directory = Path(sys.executable).resolve().parent
        game = binary_directory if (binary_directory / TARGET).is_file() else binary_directory.parent
    else:
        game = args.game_dir.resolve()
    executable = game / TARGET
    if not executable.is_file():
        raise RuntimeError(f"找不到原版 EXE：{executable}")
    actual_hash = digest(executable)
    if actual_hash != EXPECTED_SHA256:
        raise RuntimeError(f"原版 EXE SHA-256 不匹配：{actual_hash}")

    output = args.output.resolve() if args.output else (
        game / "battle-actor-frame-oracle-output" /
        f"run-{datetime.now():%Y%m%d-%H%M%S}-{os.getpid()}"
    )
    if output.exists() and any(output.iterdir()):
        raise RuntimeError(f"输出目录不是空目录：{output}")
    output.mkdir(parents=True, exist_ok=True)
    manifest = {
        "started_utc": datetime.now(timezone.utc).isoformat(timespec="milliseconds"),
        "game_directory": str(game),
        "host": platform.platform(),
        "frida_version": frida.__version__,
        "target": TARGET,
        "target_sha256": actual_hash,
        "agent_sha256": digest(agent),
        "capture_method": "frida_spawn_attach_hooks_resume",
        "entry": "0x00479850",
        "validation_scope": "diagnostic_capture_not_original_diff",
        "timing_perturbed_by_instrumentation": True,
    }
    recorder = Recorder(output)
    ready = threading.Event()
    detached = threading.Event()
    resumed = False
    process_id = None
    session = None

    def on_message(message, data) -> None:
        if message.get("type") == "error":
            recorder.record({"type": "capture-error", "message": str(message.get("stack", message))}, None)
            return
        payload = message.get("payload")
        if message.get("type") != "send" or not isinstance(payload, dict):
            recorder.record({"type": "capture-error", "message": str(message)}, None)
            return
        if payload.get("type") == "ready":
            ready.set()
            print(f"[就绪] {payload}", flush=True)
        else:
            recorder.record(payload, data)

    def on_detached(reason, crash) -> None:
        if crash is not None:
            recorder.record({"type": "capture-error", "message": f"target crash: {reason}: {crash}"}, None)
        detached.set()

    try:
        process_id = device.spawn(str(executable), argv=[str(executable)], cwd=str(game))
        manifest["target_pid"] = process_id
        (output / "run.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        session = device.attach(process_id)
        session.on("detached", on_detached)
        script = session.create_script(agent.read_text(encoding="utf-8"))
        script.on("message", on_message)
        script.load()
        if not ready.wait(timeout=10):
            raise RuntimeError("Frida agent 未在10秒内报告就绪")
        device.resume(process_id)
        resumed = True
        print(f"[启动] PID={process_id}；请进入战斗并触发角色逐帧动作，结束后正常退出游戏。", flush=True)
        while not detached.wait(timeout=0.25):
            if recorder.failure is not None:
                raise RuntimeError(recorder.failure)
    except KeyboardInterrupt:
        print("[停止] 已 detach；请手动关闭原版。", flush=True)
    finally:
        if session is not None and not detached.is_set():
            session.detach()
        if process_id is not None and not resumed:
            try:
                device.kill(process_id)
            except frida.ProcessNotFoundError:
                pass
        summary = recorder.summary()
        (output / "summary.json").write_text(
            json.dumps(summary, ensure_ascii=False, indent=2) + "\n",
            encoding="utf-8",
        )
        recorder.close()

    print(f"[结果] entry={recorder.entries}, leave={recorder.leaves}, blocks={recorder.blocks}, exceptions={recorder.exceptions}; output={output}", flush=True)
    if recorder.failure is not None:
        return 1
    return 0 if summary["snapshot_pairs_with_blocks"] else 2


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"错误：{error}", file=sys.stderr, flush=True)
        if getattr(sys, "frozen", False) and sys.platform == "win32":
            import ctypes
            ctypes.windll.user32.MessageBoxW(None, str(error), "OpenSWD3 角色帧采集", 0x10)
        raise SystemExit(1) from None
