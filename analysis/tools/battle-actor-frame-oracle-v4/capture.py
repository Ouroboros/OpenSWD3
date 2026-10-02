#!/usr/bin/env python3
"""Read-only spawn oracle for sub_479850 and its action-stream call boundaries."""

from __future__ import annotations

import argparse
from collections import Counter
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
ACTOR_BYTES = 0x2B28
ACTION_RECORD_BYTES = 0x98
NESTED_KINDS = {"action_update", "direct_act_stream", "cached_act_stream"}
CALLER_SITES = {
    "action_group_b", "opponent_group_a", "final_group_a", "final_group_b"
}


def agent_source(agent: Path, options: dict) -> str:
    bridge = agent.with_name("flags_bridge.c")
    if not bridge.is_file():
        raise RuntimeError(f"找不到原生FLAGS桥：{bridge}")
    return (
        "const FRAME_NATIVE_BRIDGE_SOURCE="
        + json.dumps(bridge.read_text(encoding="utf-8")) + ";\n"
        + "const FRAME_CAPTURE_OPTIONS=" + json.dumps(options) + ";\n"
        + agent.read_text(encoding="utf-8")
    )


def positive_int(value: str) -> int:
    result = int(value)
    if result <= 0:
        raise argparse.ArgumentTypeError("采集参数必须为正整数")
    return result


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
        self.calls_by_site = Counter()
        self.returns_by_site = Counter()
        self.samples_by_site = Counter()
        self.skips = Counter()
        self.call_ids: set[int] = set()
        self.return_ids: set[int] = set()
        self.trace_complete: set[int] = set()
        self.block_counts = Counter()
        self.limit_sites: set[str] = set()
        self.nested_calls: dict[int, dict] = {}
        self.nested_returns: set[int] = set()
        self.nested_incomplete: set[int] = set()
        self.nested_regions: dict[int, set[tuple[str, str]]] = {}

    def close(self) -> None:
        self.events.close()

    def summary(self) -> dict:
        paired_without_trace = self.entered & self.returned
        paired = paired_without_trace & self.traced
        usable = paired - self.incomplete
        nested_complete = []
        for nested_id, call in self.nested_calls.items():
            expected = {("enter", "stack"), ("leave", "stack")}
            if call["kind"] == "action_update":
                expected |= {("enter", "action-record"), ("leave", "action-record")}
            if (nested_id in self.nested_returns and
                nested_id not in self.nested_incomplete and
                expected <= self.nested_regions.get(nested_id, set())):
                nested_complete.append(nested_id)
        return {
            "nested_calls": len(self.nested_calls),
            "nested_returns": len(self.nested_returns),
            "nested_complete_ids": sorted(nested_complete),
            "nested_incomplete_ids": sorted(self.nested_incomplete),
            "unreturned_nested_ids": sorted(set(self.nested_calls) - self.nested_returns),
            "entries": self.entries,
            "leaves": self.leaves,
            "target_blocks": self.blocks,
            "exceptions": self.exceptions,
            "paired_sequences": sorted(paired_without_trace),
            "incomplete_sequences": sorted(self.incomplete),
            "flags_unavailable_sequences": sorted(self.flags_missing),
            "snapshot_pairs_with_blocks": sorted(usable),
            "snapshot_pairs_with_flags_without_trace": sorted(
                paired_without_trace - self.incomplete - self.flags_missing
            ),
            "unreturned_sequences": sorted(self.entered - self.returned),
            "failure": self.failure,
            "calls_by_site": dict(self.calls_by_site),
            "returns_by_site": dict(self.returns_by_site),
            "sampled_pairs_by_site": dict(self.samples_by_site),
            "skipped_calls": dict(self.skips),
            "sample_limit_sites": sorted(self.limit_sites),
            "unreturned_call_ids": sorted(self.call_ids - self.return_ids),
            "four_callers_observed": CALLER_SITES <= set(self.calls_by_site),
            "trace_completed_sequences": sorted(self.trace_complete),
            "normal_pairs_with_flags_and_trace": sorted(
                usable & self.trace_complete - self.flags_missing
            ),
            "original_diff_verified": False,
            "trace_drain_completion_verified": bool(usable) and (
                usable <= self.trace_complete and not self.entered - self.returned
            ),
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

            if kind in ("nested-call", "nested-return", "nested-memory"):
                nested_id = payload.get("nested_id")
                parent_sequence = payload.get("parent_sequence")
                site = payload.get("site")
                name = payload.get("kind")
                if (type(nested_id) is not int or nested_id <= 0 or
                    type(parent_sequence) is not int or parent_sequence not in self.entered or
                    site not in CALLER_SITES or name not in NESTED_KINDS):
                    self.failure = "invalid nested event identity"
                    return
                if kind != "nested-memory":
                    nested_registers = payload.get("registers")
                    if (not isinstance(nested_registers, dict) or
                        nested_registers.get("flags_available") is not True or
                        not isinstance(nested_registers.get("eflags"), str) or
                        not re.fullmatch(r"0x[0-9a-fA-F]+", nested_registers["eflags"])):
                        self.nested_incomplete.add(nested_id)
                if kind == "nested-call":
                    if nested_id in self.nested_calls:
                        self.failure = f"duplicate nested id: {nested_id}"
                        return
                    self.nested_calls[nested_id] = {
                        "parent_sequence": parent_sequence, "site": site, "kind": name,
                        "thread_id": payload.get("thread_id"),
                    }
                    self.nested_regions[nested_id] = set()
                    if (name == "action_update" and
                        payload.get("record_address") != payload.get("expected_record_address")):
                        self.nested_incomplete.add(nested_id)
                else:
                    call = self.nested_calls.get(nested_id)
                    if (call is None or
                        any(call[field] != payload.get(field) for field in
                            ("parent_sequence", "site", "kind"))):
                        self.failure = f"unmatched nested event: {nested_id}"
                        return
                    if kind == "nested-return":
                        if (nested_id in self.nested_returns or
                            call["thread_id"] != payload.get("thread_id")):
                            self.failure = f"duplicate/mismatched nested return: {nested_id}"
                            return
                        self.nested_returns.add(nested_id)
                    else:
                        phase = payload.get("phase")
                        region = payload.get("region")
                        count = payload.get("byte_count")
                        if (phase not in ("enter", "leave") or region not in
                            ("stack", "action-record") or
                            (region == "action-record" and name != "action_update") or
                            type(count) is not int or count != len(data or b"") or
                            (count not in (0, 128) if region == "stack" else
                             count not in (0, ACTION_RECORD_BYTES))):
                            self.failure = f"invalid nested memory: {nested_id}/{phase}/{region}"
                            return
                        key = (phase, region)
                        if key in self.nested_regions[nested_id]:
                            self.failure = f"duplicate nested memory: {nested_id}/{phase}/{region}"
                            return
                        if count == 0:
                            self.nested_incomplete.add(nested_id)
                        else:
                            self.nested_regions[nested_id].add(key)
                            filename = f"nested-{region}-{nested_id:06d}-{phase}.bin"
                            (self.output / filename).write_bytes(data)
                            payload["snapshot_file"] = filename
                            payload["snapshot_sha256"] = hashlib.sha256(data).hexdigest()

            if kind in ("call", "call-return"):
                call_id = payload.get("call_id")
                site = payload.get("site")
                if type(call_id) is not int or call_id <= 0 or site not in CALLER_SITES:
                    self.failure = "invalid call ledger event"
                    return
                if kind == "call":
                    if call_id in self.call_ids:
                        self.failure = f"duplicate call id: {call_id}"
                        return
                    self.call_ids.add(call_id)
                    self.calls_by_site[site] += 1
                    if not payload.get("sampled", False):
                        self.skips[payload.get("skip_reason")] += 1
                        if payload.get("skip_reason") == "sample_limit":
                            self.limit_sites.add(site)
                else:
                    if call_id not in self.call_ids or call_id in self.return_ids:
                        self.failure = f"unmatched call return: {call_id}"
                        return
                    self.return_ids.add(call_id)
                    self.returns_by_site[site] += 1
            elif kind in ("frame", "memory"):
                if not isinstance(phase, str) or not re.fullmatch(
                    r"enter|leave|exception-[1-9][0-9]*", phase
                ):
                    self.failure = f"invalid snapshot phase: {phase}"
                    return

                count_field = "actor_bytes" if kind == "frame" else "byte_count"
                count = payload.get(count_field)
                if type(count) is not int or count != len(data or b""):
                    self.failure = f"invalid snapshot size: {sequence}/{phase}"
                elif kind == "frame" and (count != ACTOR_BYTES or payload.get("problem")):
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
                        self.samples_by_site[payload.get("site")] += 1
            elif kind == "blocks":
                blocks = payload.get("blocks")
                if not isinstance(blocks, list):
                    self.failure = "invalid block packet"
                    return

                self.blocks += len(blocks)
                self.block_counts[sequence] += len(blocks)
                if blocks:
                    self.traced.add(sequence)
                if payload.get("dropped_total", 0) > 0:
                    self.incomplete.add(sequence)
            elif kind == "trace-end":
                if (payload.get("capture_method") == "synchronous_callouts"
                    and payload.get("drain_completion_verified") is True
                    and payload.get("dropped_total") == 0
                    and payload.get("block_count") == self.block_counts[sequence]):
                    self.trace_complete.add(sequence)
                else:
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
    parser.add_argument("--max-samples-per-site", type=positive_int, default=4096)
    parser.add_argument("--repeat-interval", type=positive_int, default=64)
    args = parser.parse_args()
    try:
        import frida
    except ImportError as error:
        raise RuntimeError("缺少 frida Python 包") from error

    agent = Path(__file__).resolve().with_name("agent.js")
    if not agent.is_file():
        raise RuntimeError(f"找不到 Frida agent：{agent}")
    options = {
        "max_samples_per_site": args.max_samples_per_site,
        "repeat_interval": args.repeat_interval,
        "trace_enabled": False,
    }
    source = agent_source(agent, options)
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
        game / "battle-actor-frame-oracle-v4-output" /
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
        "bridge_sha256": digest(agent.with_name("flags_bridge.c")),
        "injected_source_sha256": hashlib.sha256(source.encode("utf-8")).hexdigest(),
        "schema_version": 4,
        "actor_bytes": ACTOR_BYTES,
        "nested_action_record_bytes": ACTION_RECORD_BYTES,
        "nested_callsites": ["0x00479921", "0x00432421", "0x00432434"],
        "nested_scope": "sampled_parent_only_at_function_entry_and_leave",
        "trace_scope": "disabled_to_isolate_nested_cpu_capture; v3 retains parent_block_trace",
        "entry_leave_extra_globals": [
            "0x004A0E78", "0x004A0E7C", "0x004AB784", "0x004FB308",
        ],
        "extra_globals_timing_scope": "parent_and_nested_function_boundaries_not_inner_reads",
        "sampling": options,
        "full_snapshots_filtered": True,
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
        script = session.create_script(source)
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
    return 0 if summary["snapshot_pairs_with_flags_without_trace"] else 2


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"错误：{error}", file=sys.stderr, flush=True)
        if getattr(sys, "frozen", False) and sys.platform == "win32":
            import ctypes
            ctypes.windll.user32.MessageBoxW(None, str(error), "OpenSWD3 角色帧采集", 0x10)
        raise SystemExit(1) from None
