#!/usr/bin/env python3
"""Own ia32 PE fixture: verify saved FLAGS; never open or launch the game."""

from __future__ import annotations

import collections
import ctypes
import json
import os
import struct
import sys
import threading
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BASE = 0x400000
TEXT_RVA = 0x1000
ACTOR = 0x520000
FRAME = 0x525000
CALLERS = [0x4554F6, 0x45650F, 0x45AA33, 0x45ACBF]
ENTRY_FLAGS = [0x603, 0xA82, 0x247, 0xED7]
LEAVE_FLAGS = 0xED7
LOOPS = 8
ENTRY_GLOBAL_VALUES = {
    0x004A0E78: 640,
    0x004A0E7C: 480,
    0x004AB784: 0x12345678,
    0x004FB308: 1,
}


def create_fixture(destination: Path, list_probe: bool = False) -> None:
    """Test-only machine code and PE import metadata, not copied game bytes."""
    text = bytearray(0x13F000)

    def put(address: int, data: bytes) -> None:
        start = address - BASE - TEXT_RVA
        text[start:start + len(data)] = data

    def dword(value: int) -> bytes:
        return struct.pack("<I", value)

    bootstrap = bytearray(b"\xb9" + dword(ACTOR))
    bootstrap += b"\xc7\x81\x48\x25\x00\x00" + dword(FRAME)
    if list_probe:
        root = 0x521000
        node = 0x522000
        put(ACTOR + 0x2EC0, dword(root))
        put(root, dword(node))
        node_bytes = bytearray(0x60)
        struct.pack_into("<H", node_bytes, 0x4C, 7)
        struct.pack_into("<H", node_bytes, 0x5E, 0x1F)
        put(node, node_bytes)
        for destination_address, argument in (
            (0x466684, None), (0x4705C0, (0, 1, 1, 0x500000)),
            (0x478710, (7,)),
        ):
            if argument is not None:
                for value in reversed(argument):
                    bootstrap += b"\x68" + dword(value)
            next_ip = BASE + TEXT_RVA + len(bootstrap) + 5
            bootstrap += b"\xe8" + struct.pack("<i", destination_address - next_ip)
            if destination_address == 0x4705C0:
                bootstrap += b"\x83\xc4\x10"  # Own-PE cdecl stub only.
        put(0x466684, b"\xe8" + struct.pack("<i", 0x470180 - 0x466689) + b"\xc3")
        put(0x470180, b"\x83\xec\x28\x83\xc4\x28\xc3")
        put(0x4705C0, b"\x51\x53\x8b\xf1\xc7\x86\xc0\x2e\x00\x00" +
            dword(node) + b"\xe9" + struct.pack("<i", 0x4706EA - 0x4705D3))
        put(0x4706EA, b"\x8b\x96\xc0\x2e\x00\x00\xe9" +
            struct.pack("<i", 0x470706 - 0x4706F5))
        put(0x470706, b"\x66\x8b\x42\x4c\x66\x89\x86\x18\x2f\x00\x00" +
            b"\x5b\x59\xb8" + dword(1) + b"\xc3")
        put(0x478710, b"\x8b\x44\x24\x04\x66\x89\x81\x6c\x2a\x00\x00" +
            b"\xb8" + dword(1) + b"\xc2\x04\x00")
    bootstrap += b"\xbe" + dword(LOOPS)
    loop = len(bootstrap)
    for caller, flags in zip(CALLERS, ENTRY_FLAGS):
        bootstrap += b"\x68" + dword(flags) + b"\x9d"
        next_ip = BASE + TEXT_RVA + len(bootstrap) + 5
        bootstrap += b"\xe8" + struct.pack("<i", caller - next_ip)
    bootstrap += b"\x4e\x75"
    bootstrap += struct.pack("b", loop - (len(bootstrap) + 1))
    # Give asynchronous Frida send() callbacks time to reach the host before exit.
    bootstrap += b"\xbb" + dword(50_000_000)
    drain_loop = len(bootstrap)
    bootstrap += b"\x4b\x75" + struct.pack("b", drain_loop - (len(bootstrap) + 3))
    # CLD before the Win32 exit call; all assertions happen before it.
    bootstrap += b"\xfc\x6a\x00\xff\x15" + dword(BASE + 0x1150)
    put(BASE + TEXT_RVA, bytes(bootstrap))
    for caller in CALLERS:
        put(caller, b"\xe8" + struct.pack("<i", 0x479850 - caller - 5) + b"\xc3")
    # Own PE only: three nested hooks within each sampled parent invocation.
    # The CALL IPs and return addresses match the LST; fixture stubs do not
    # impersonate original child function bodies or their real outputs.
    put(0x479850, b"\x83\xec\x14\x53\x55\xe9" +
        struct.pack("<i", 0x47990A - 0x47985A))
    put(0x47990A, b"\x8d\x81\xd0\x03\x00\x00" + b"\x90" * 10)
    put(0x47991A, b"\x68" + dword(0xED7) + b"\x9d\x50\xe8" +
        struct.pack("<i", 0x4321E0 - 0x479926) +
        b"\x83\xc4\x04\xfe\x81\x58\x29\x00\x00"
        b"\x5d\x5b\x8d\x64\x24\x14\x68" + dword(LEAVE_FLAGS) +
        b"\x9d\xb8\x01\x00\x00\x00\xc3")
    put(0x4321E0, b"\x53\x55\x56\x8b\x74\x24\x10\xe9" +
        struct.pack("<i", 0x432414 - 0x4321EC))
    put(0x432414, b"\xb8" + dword(407) + b"\x6a\x24\x50" +
        b"\x90" * 5 + b"\xe8" + struct.pack("<i", 0x432A50 - 0x432426) +
        b"\x83\xc4\x08\xb8" + dword(407) + b"\x6a\x24\x50" +
        b"\x90" * 3 + b"\xe8" + struct.pack("<i", 0x432BC0 - 0x432439) +
        b"\x83\xc4\x08\x5e\x5d\x5b\xb8\x01\x00\x00\x00\xc3")
    put(0x432A50, b"\xa1\x0c\xb3\x4f\x00\x68" + dword(0x247) +
        b"\x9d\xb8\x01\x00\x00\x00\xc3")
    put(0x432BC0, b"\x51\x68" + dword(0xED7) +
        b"\x9d\xb8\x01\x00\x00\x00\x59\xc3")
    for address, value in ENTRY_GLOBAL_VALUES.items():
        put(address, dword(value))

    put(BASE + 0x1100, struct.pack("<IIIII", 0x1140, 0, 0, 0x1180, 0x1150))
    put(BASE + 0x1140, dword(0x1160) + dword(0))
    put(BASE + 0x1150, dword(0x1160) + dword(0))
    put(BASE + 0x1160, b"\x00\x00ExitProcess\x00")
    put(BASE + 0x1180, b"kernel32.dll\x00")

    header = bytearray(0x200)
    header[:2] = b"MZ"
    struct.pack_into("<I", header, 0x3C, 0x80)
    header[0x80:0x84] = b"PE\x00\x00"
    struct.pack_into("<HHIIIHH", header, 0x84, 0x14C, 1, 0, 0, 0, 0xE0, 0x102)
    opt = 0x98
    struct.pack_into("<H", header, opt, 0x10B)
    for offset, value in {
        4: len(text), 16: TEXT_RVA, 20: TEXT_RVA, 24: TEXT_RVA,
        28: BASE, 32: 0x1000, 36: 0x200, 56: 0x140000, 60: 0x200,
        72: 0x100000, 76: 0x1000, 80: 0x100000, 84: 0x1000, 92: 16,
        104: 0x1100, 108: 40, 192: 0x1150, 196: 8,
    }.items():
        struct.pack_into("<I", header, opt + offset, value)
    struct.pack_into("<H", header, opt + 40, 6)
    struct.pack_into("<H", header, opt + 48, 6)
    struct.pack_into("<H", header, opt + 68, 3)
    section = opt + 0xE0
    struct.pack_into("<8sIIIIIIHHI", header, section, b".text\x00\x00\x00",
                     len(text), TEXT_RVA, len(text), 0x200, 0, 0, 0, 0, 0xE0000020)
    destination.write_bytes(header + text)


def main() -> None:
    output = ROOT / "build/tmp/runtime/oracle-native-probe" / uuid.uuid4().hex
    output.mkdir(parents=True)
    for name in ("TMPDIR", "TMP", "TEMP"):
        os.environ[name] = str(output)
    import frida

    executable = output / "battle-frame-flags-probe.exe"
    list_probe = "--list-probe" in sys.argv
    if list_probe and "--full-agent" not in sys.argv:
        raise RuntimeError("list probe requires --full-agent")
    create_fixture(executable, list_probe)
    bridge = Path(__file__).with_name("flags_bridge.c").read_text(encoding="utf-8")
    javascript = "const BRIDGE=" + json.dumps(bridge) + ";\n" + r"""
if (Process.arch !== 'ia32' || Frida.version !== '16.5.1') {
    throw new Error('probe requires Frida16.5.1 ia32');
}

let sequence = 0;
const enter = new NativeCallback(function(cpu, flags, threadId, returnAddress) {
    send({type:'enter', sequence:++sequence, flags, thread_id:threadId,
          return_address:returnAddress.toString(), eax:cpu.add(32).readU32()});
    return sequence;
}, 'uint', ['pointer','uint','uint','pointer']);
const leave = new NativeCallback(function(cpu, flags, threadId, seq) {
    send({type:'leave', sequence:seq, flags, thread_id:threadId,
          eax:cpu.add(32).readU32()});
}, 'void', ['pointer','uint','uint','uint']);
const nestedEnter = new NativeCallback(function() { return 0; },
    'uint', ['pointer','uint','uint','pointer']);
const nestedLeave = new NativeCallback(function() {},
    'void', ['pointer','uint','uint','uint']);
const native = new CModule(BRIDGE, {frame_enter:enter, frame_leave:leave,
    nested_enter:nestedEnter, nested_leave:nestedLeave});
Interceptor.attach(ptr('0x479850'), {onEnter:native.on_enter, onLeave:native.on_leave});
send({type:'ready'});
"""
    full_agent = "--full-agent" in sys.argv
    recorder = None
    if full_agent:
        import capture
        javascript = capture.agent_source(Path(__file__).with_name("agent.js"), {
            "max_samples_per_site": 2, "repeat_interval": 64,
            "trace_enabled": False, "list_probe_enabled": list_probe,
        })
        recorder = capture.Recorder(output)
    events = []
    ready = threading.Event()
    detached = threading.Event()
    device = frida.get_local_device()
    pid = device.spawn(str(executable), argv=[str(executable)], cwd=str(output))
    session = device.attach(pid)
    handle = ctypes.windll.kernel32.OpenProcess(0x1000, False, pid)
    def on_detached(reason, crash):
        exit_code = ctypes.c_ulong()
        status = ctypes.windll.kernel32.GetExitCodeProcess(
            handle, ctypes.byref(exit_code)) if handle else 0
        events.append({"type": "detached", "reason": str(reason),
                       "crash": str(crash) if crash is not None else None,
                       "exit_code": hex(exit_code.value) if status else None})
        detached.set()

    session.on("detached", on_detached)

    def message(item, data):
        if item.get("type") == "send":
            payload = item["payload"]
            events.append(payload)
            if payload.get("type") == "ready":
                ready.set()
            elif recorder is not None:
                recorder.record(payload, data)
        else:
            events.append({"type": "error", "message": str(item)})
            ready.set()

    script = session.create_script(javascript)
    script.on("message", message)
    resumed = False
    try:
        script.load()
        ready.wait()
        if any(e["type"] == "error" for e in events):
            raise RuntimeError(str(events))
        device.resume(pid)
        resumed = True
        detached.wait()
    finally:
        if not resumed:
            device.kill(pid)
        if not detached.is_set():
            session.detach()
        (output / "events.json").write_text(json.dumps(events, indent=2) + "\n")
        if recorder is not None:
            recorder.close()
        if handle:
            ctypes.windll.kernel32.CloseHandle(handle)

    if full_agent:
        summary = recorder.summary()
        assert summary["failure"] is None, summary
        assert summary["four_callers_observed"], summary
        assert not summary["unreturned_call_ids"], summary
        assert len(summary["snapshot_pairs_with_flags_without_trace"]) == 8, summary
        assert not summary["trace_drain_completion_verified"], summary
        assert len(summary["sample_limit_sites"]) == 4, summary
        assert set(summary["calls_by_site"].values()) == {LOOPS}, summary
        assert summary["nested_calls"] == summary["nested_returns"] == 24, summary
        assert len(summary["nested_complete_ids"]) == 24, summary
        assert not summary["unreturned_nested_ids"], summary
        if list_probe:
            assert summary["list_complete_ids"] == [1], summary
            assert summary["list_nodes"] == 1, summary
            assert summary["list_nodes_with_low_word_seven"] == [
                {"list_id": 1, "index": 0,
                 "snapshot_file": "list-node-000001-000.bin"},
            ], summary
            assert summary["profile_calls"] == summary["profile_returns"] == 1, summary
            assert summary["profile_exit_cursors"] == 1, summary
            assert summary["profile_exit_cursor_snapshots"] == 1, summary
            assert summary["action_seven_calls"] == summary["action_seven_returns"] == 1, summary
            assert (output / "list-node-000001-000.bin").read_bytes()[0x4C:0x4E] == b"\x07\x00"
        site_flags = dict(zip([
            "action_group_b", "opponent_group_a", "final_group_a", "final_group_b"
        ], ENTRY_FLAGS))
        nested_pairs = {}
        for event in events:
            if event.get("type") == "nested-call":
                nested_pairs[event["nested_id"]] = event
                assert event["registers"]["flags_available"], event
                # Parent SUB and child ADD modify FLAGS; an explicit POPFD at
                # 0x47991F fixes the update/direct input, while the ADD after
                # the direct return fixes the cached input to 0x212.
                expected_entry = 0x212 if event["kind"] == "cached_act_stream" else 0xED7
                assert int(event["registers"]["eflags"], 16) & 0xCD7 == expected_entry & 0xCD7, event
                if event["kind"] == "action_update":
                    assert event["record_address"] == event["expected_record_address"], event
            elif event.get("type") == "nested-return":
                entry = nested_pairs[event["nested_id"]]
                assert event["parent_sequence"] == entry["parent_sequence"], event
                assert event["kind"] == entry["kind"], event
                expected = (0x247 if event["kind"] == "direct_act_stream" else
                            0xED7 if event["kind"] == "cached_act_stream" else
                            0x612)
                assert int(event["registers"]["eflags"], 16) & 0xCD7 == expected & 0xCD7, event
        assert len(nested_pairs) == 24, len(nested_pairs)
        entry_contexts = {}
        for event in events:
            if event.get("type") == "call-return":
                assert int(event["eax"], 16) == 1, event
            if event.get("type") == "frame":
                flags = int(event["registers"]["eflags"], 16)
                expected_flags = site_flags[event["site"]] if event["phase"] == "enter" else LEAVE_FLAGS
                assert flags & 0xCD7 == expected_flags & 0xCD7, event
                assert event["actor_bytes"] == capture.ACTOR_BYTES, event
                for address, value in ENTRY_GLOBAL_VALUES.items():
                    assert event["globals"][f"0x{address:08X}"] == f"{value:08x}", event

                seq = event["sequence"]
                if event["phase"] == "enter":
                    entry_contexts[seq] = event["registers"]
                elif event["phase"] == "leave":
                    for name in ("edi", "esi", "ebp", "esp", "ebx", "edx", "ecx"):
                        assert event["registers"][name] == entry_contexts[seq][name], (name, event)
                    before = (output / f"actor-{seq:06d}-enter.bin").read_bytes()
                    after = (output / f"actor-{seq:06d}-leave.bin").read_bytes()
                    expected = bytearray(before)
                    expected[0x2958] = (expected[0x2958] + 1) & 0xFF
                    assert after == expected, seq
        print("full collector native probe passed: 32 parent pairs, 8 complete snapshots without Stalker, 24 paired nested calls, 4 site limits; FLAGS/actor delta preserved")
        print("summary:", json.dumps({k:v for k,v in summary.items() if not isinstance(v,list)}))
        print("artifacts:", output.relative_to(ROOT))
        return

    entries = [e for e in events if e["type"] == "enter"]
    leaves = [e for e in events if e["type"] == "leave"]
    assert not any(e["type"] == "error" for e in events), events
    assert len(entries) == len(leaves) == LOOPS * len(CALLERS), (len(entries), len(leaves))
    expected = {hex(c + 5): f for c, f in zip(CALLERS, ENTRY_FLAGS)}
    for event in entries:
        assert event["flags"] & 0xCD7 == expected[event["return_address"]] & 0xCD7, event
    for event in leaves:
        assert event["flags"] & 0xCD7 == LEAVE_FLAGS & 0xCD7, event
        assert event["eax"] == 1, event
    assert {e["sequence"] for e in entries} == {e["sequence"] for e in leaves}
    print("native saved-FLAGS probe passed:", len(entries), "ia32 pairs; CF/PF/AF/ZF/SF/DF/OF observed")
    print("caller counts:", dict(collections.Counter(e["return_address"] for e in entries)))
    print("artifacts:", output.relative_to(ROOT))


if __name__ == "__main__":
    main()
