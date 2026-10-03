#!/usr/bin/env python3
"""Recorder synthetic tests; never launch or attach to the game."""

import importlib.util
import json
import shutil
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location(
    "frame_capture", Path(__file__).with_name("capture.py")
)
CAPTURE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CAPTURE)
SIZE = CAPTURE.ACTOR_BYTES


def frame(phase, size=SIZE, flags=False):
    return {
        "type": "frame", "sequence": 1, "phase": phase,
        "site": "final_group_a", "actor_bytes": size, "problem": None,
        "registers": {"flags_available": flags},
    }


class RecorderTest(unittest.TestCase):
    def setUp(self):
        directory = ROOT / "build/tmp/runtime/oracle-recorder-tests"
        directory.mkdir(parents=True, exist_ok=True)
        self.output = Path(tempfile.mkdtemp(dir=directory))
        self.recorder = CAPTURE.Recorder(self.output)

    def tearDown(self):
        self.recorder.close()
        shutil.rmtree(self.output)

    def pair(self, flags=False):
        self.recorder.record(frame("enter", flags=flags), bytes(SIZE))
        self.recorder.record(frame("leave", flags=flags), bytes(SIZE))
        self.recorder.record({
            "type": "blocks", "sequence": 1,
            "blocks": [["0x479850", "0x479867"]],
        }, None)

    def test_pair_requires_real_blocks(self):
        self.recorder.record(frame("enter"), bytes(SIZE))
        self.recorder.record(frame("leave"), bytes(SIZE))
        self.recorder.record({"type": "blocks", "sequence": 1, "blocks": []}, None)
        self.assertEqual(self.recorder.summary()["snapshot_pairs_with_blocks"], [])

    def test_missing_actor_is_incomplete(self):
        self.recorder.record(frame("enter", 0), None)
        self.assertEqual(self.recorder.summary()["incomplete_sequences"], [1])

    def test_mismatched_size_is_failure(self):
        self.recorder.record(frame("enter"), bytes(4))
        self.assertIn("invalid snapshot size", self.recorder.failure)

    def test_duplicate_never_overwrites_snapshot(self):
        self.recorder.record(frame("enter"), bytes(SIZE))
        self.recorder.record(frame("enter"), bytes([1]) * SIZE)
        self.assertIn("duplicate snapshot", self.recorder.failure)
        self.assertEqual((self.output / "actor-000001-enter.bin").read_bytes(), bytes(SIZE))

    def test_exception_snapshot_is_not_normal_leave(self):
        self.recorder.record(frame("enter"), bytes(SIZE))
        self.recorder.record(frame("exception-1"), bytes(SIZE))
        self.assertEqual(self.recorder.leaves, 0)
        self.assertEqual(self.recorder.summary()["unreturned_sequences"], [1])

    def test_dropped_blocks_are_incomplete(self):
        self.recorder.record({
            "type": "blocks", "sequence": 1,
            "blocks": [["0x479850", "0x479867"]], "dropped_total": 1,
        }, None)
        self.assertEqual(self.recorder.summary()["incomplete_sequences"], [1])

    def test_invalid_path_is_rejected(self):
        self.recorder.record({
            "type": "memory", "sequence": 1, "phase": "enter",
            "byte_count": 4, "region": "../outside",
        }, bytes(4))
        self.assertIn("invalid snapshot region", self.recorder.failure)

    def test_memory_snapshot_hash_is_recorded(self):
        self.recorder.record({
            "type": "memory", "sequence": 1, "phase": "enter",
            "byte_count": 4, "region": "list-head-word",
        }, bytes(4))
        event = json.loads((self.output / "events.jsonl").read_text())
        self.assertEqual(event["snapshot_sha256"],
                         CAPTURE.hashlib.sha256(bytes(4)).hexdigest())

    def test_synchronous_trace_is_verified_only_with_exact_count(self):
        self.pair(flags=True)
        self.recorder.record({
            "type": "trace-end", "sequence": 1, "block_count": 1,
            "capture_method": "synchronous_callouts", "dropped_total": 0,
            "drain_completion_verified": True,
        }, None)
        summary = self.recorder.summary()
        self.assertTrue(summary["trace_drain_completion_verified"])
        self.assertEqual(summary["normal_pairs_with_flags_and_trace"], [1])
        self.assertFalse(summary["original_diff_verified"])

    def test_trace_count_mismatch_is_incomplete(self):
        self.pair(flags=True)
        self.recorder.record({
            "type": "trace-end", "sequence": 1, "block_count": 2,
            "capture_method": "synchronous_callouts", "dropped_total": 0,
            "drain_completion_verified": True,
        }, None)
        self.assertEqual(self.recorder.summary()["incomplete_sequences"], [1])

    def test_missing_flags_still_prevent_normal_flags_trace_pair(self):
        self.pair()
        self.assertEqual(self.recorder.summary()["normal_pairs_with_flags_and_trace"], [])

    def test_call_limits_are_explicit_and_unreturned_calls_preserved(self):
        self.recorder.record({
            "type": "call", "call_id": 4, "site": "action_group_b",
            "sampled": False, "skip_reason": "sample_limit",
        }, None)
        summary = self.recorder.summary()
        self.assertEqual(summary["sample_limit_sites"], ["action_group_b"])
        self.assertEqual(summary["unreturned_call_ids"], [4])
        self.recorder.record({
            "type": "call-return", "call_id": 4, "site": "action_group_b",
        }, None)
        self.assertEqual(self.recorder.summary()["unreturned_call_ids"], [])

    def test_duplicate_call_ids_are_failure(self):
        event = {"type": "call", "call_id": 1, "site": "final_group_a", "sampled": True}
        self.recorder.record(event, None)
        self.recorder.record(event, None)
        self.assertIn("duplicate call id", self.recorder.failure)

    def test_no_stalker_parent_pair_is_not_mislabeled_as_trace(self):
        self.recorder.record(frame("enter", flags=True), bytes(SIZE))
        self.recorder.record(frame("leave", flags=True), bytes(SIZE))
        summary = self.recorder.summary()
        self.assertEqual(summary["paired_sequences"], [1])
        self.assertEqual(summary["snapshot_pairs_with_flags_without_trace"], [1])
        self.assertEqual(summary["snapshot_pairs_with_blocks"], [])
        self.assertFalse(summary["trace_drain_completion_verified"])

    def test_original_list_nodes_are_hashed_and_not_parent_samples(self):
        first = bytearray(CAPTURE.LIST_NODE_BYTES)
        first[0x4c:0x4e] = (7).to_bytes(2, "little")
        for index, node in enumerate((bytes(first), bytes(CAPTURE.LIST_NODE_BYTES))):
            self.recorder.record({
                "type": "list-node", "list_id": 2, "index": index,
                "thread_id": 7, "actor": "0x580000", "root": "0x610000",
                "node": f"0x{0x620000 + 0x60 * index:x}",
                "next": "0x0" if index else "0x620060",
                "byte_count": CAPTURE.LIST_NODE_BYTES,
            }, node)
        self.recorder.record({
            "type": "list-scan", "list_id": 2, "thread_id": 7,
            "actor": "0x580000", "root": "0x610000", "count": 2,
            "complete": True, "problem": None,
        }, None)
        summary = self.recorder.summary()
        self.assertEqual(summary["list_complete_ids"], [2])
        self.assertEqual(summary["list_nodes"], 2)
        self.assertEqual(summary["list_nodes_with_low_word_seven"], [
            {"list_id": 2, "index": 0,
             "snapshot_file": "list-node-000002-000.bin"},
        ])
        self.assertEqual(summary["paired_sequences"], [])
        self.assertFalse(summary["original_diff_verified"])
        events = [json.loads(row) for row in
                  (self.output / "events.jsonl").read_text().splitlines()]
        self.assertEqual(events[0]["snapshot_sha256"],
                         CAPTURE.hashlib.sha256(first).hexdigest())
        self.assertEqual(events[0]["value_flags"], 7)
        self.assertEqual((self.output / events[0]["snapshot_file"]).read_bytes(), first)
        self.recorder.record({
            "type": "list-node", "list_id": 2, "index": 0,
            "thread_id": 7, "actor": "0x580000", "byte_count": 96,
        }, bytes(96))
        self.assertIn("duplicate list node", self.recorder.failure)

    def test_list_scan_with_gap_is_not_complete(self):
        self.recorder.record({
            "type": "list-scan", "list_id": 3, "thread_id": 7,
            "actor": "0x580000", "count": 0,
            "complete": False, "problem": "unreadable_node",
        }, None)
        self.assertEqual(self.recorder.summary()["list_incomplete_ids"], [3])
        self.assertEqual(self.recorder.summary()["list_complete_ids"], [])

    def test_list_scan_rejects_missing_initial_node(self):
        self.recorder.record({
            "type": "list-node", "list_id": 4, "index": 1,
            "thread_id": 7, "actor": "0x580000", "byte_count": 96,
        }, bytes(96))
        self.recorder.record({
            "type": "list-scan", "list_id": 4, "thread_id": 7,
            "actor": "0x580000", "count": 1,
            "complete": True, "problem": None,
        }, None)
        self.assertIn("invalid list scan", self.recorder.failure)

    def test_profile_and_setter_provenance_stay_separate(self):
        self.recorder.record({
            "type": "profile-call", "profile_id": 1, "thread_id": 7,
            "actor": "0x580000", "arguments": [0, 1, 1, 0x500000],
        }, None)
        raw = bytearray(CAPTURE.LIST_NODE_BYTES)
        raw[0x4c:0x4e] = (7).to_bytes(2, "little")
        self.recorder.record({
            "type": "profile-cursor-exit", "profile_id": 1, "thread_id": 7,
            "actor": "0x580000", "node": "0x620000", "applied_word": 7,
            "byte_count": CAPTURE.LIST_NODE_BYTES,
        }, bytes(raw))
        self.recorder.record({
            "type": "profile-return", "profile_id": 1, "thread_id": 7,
            "actor": "0x580000", "eax": "0x1", "cursor_snapshots": 1,
        }, None)
        self.recorder.record({
            "type": "action-seven-call", "action_id": 1, "thread_id": 7,
            "actor": "0x580000", "argument": 7,
            "return_address": "0x456ba6",
        }, None)
        summary = self.recorder.summary()
        self.assertEqual(summary["profile_exit_cursor_snapshots"], 1)
        self.assertEqual(summary["profile_returns"], 1)
        self.assertEqual(summary["unreturned_action_seven_ids"], [1])
        self.assertEqual(summary["list_nodes_with_low_word_seven"], [])
        self.recorder.record({
            "type": "action-seven-return", "action_id": 1,
            "thread_id": 7, "actor": "0x580000", "eax": "0x1",
            "action_word": 7,
        }, None)
        self.assertEqual(self.recorder.summary()["action_seven_returns"], 1)
        events = [json.loads(row) for row in
                  (self.output / "events.jsonl").read_text().splitlines()]
        self.assertEqual(events[1]["snapshot_sha256"],
                         CAPTURE.hashlib.sha256(raw).hexdigest())

    def test_nested_action_pair_and_files(self):
        self.recorder.record(frame("enter"), bytes(SIZE))
        call = {
            "type": "nested-call", "nested_id": 1, "parent_sequence": 1,
            "site": "final_group_a", "kind": "action_update", "thread_id": 7,
            "record_address": "0x5003d0", "expected_record_address": "0x5003d0",
            "registers": {"flags_available": True, "eflags": "0x603"},
        }
        self.recorder.record(call, None)
        for phase in ("enter", "leave"):
            for region, size in (("stack", 128), ("action-record", 0x98)):
                self.recorder.record({
                    "type": "nested-memory", "nested_id": 1,
                    "parent_sequence": 1, "site": "final_group_a",
                    "kind": "action_update", "phase": phase,
                    "region": region, "byte_count": size,
                }, bytes([1]) * size)
        self.recorder.record({
            **call, "type": "nested-return",
        }, None)
        summary = self.recorder.summary()
        self.assertEqual(summary["nested_complete_ids"], [1])
        self.assertEqual(summary["nested_returns"], 1)
        self.assertEqual(summary["unreturned_nested_ids"], [])
        self.assertEqual((self.output / "nested-action-record-000001-leave.bin").read_bytes(),
                         bytes([1]) * 0x98)
        events = [json.loads(row) for row in
                  (self.output / "events.jsonl").read_text().splitlines()]
        snapshot = next(row for row in events if row.get("region") == "action-record")
        self.assertEqual(snapshot["snapshot_sha256"],
                         CAPTURE.hashlib.sha256(bytes([1]) * 0x98).hexdigest())

    def test_nested_unavailable_flags_do_not_count_as_complete(self):
        self.recorder.record(frame("enter", flags=True), bytes(SIZE))
        call = {
            "type": "nested-call", "nested_id": 1, "parent_sequence": 1,
            "site": "final_group_a", "kind": "direct_act_stream", "thread_id": 7,
            "registers": {"flags_available": False},
        }
        self.recorder.record(call, None)
        for phase in ("enter", "leave"):
            self.recorder.record({
                "type": "nested-memory", "nested_id": 1, "parent_sequence": 1,
                "site": "final_group_a", "kind": "direct_act_stream",
                "phase": phase, "region": "stack", "byte_count": 128,
            }, bytes(128))
        self.recorder.record({
            **call, "type": "nested-return",
            "registers": {"flags_available": True, "eflags": "0x247"},
        }, None)
        self.assertEqual(self.recorder.summary()["nested_complete_ids"], [])
        self.assertEqual(self.recorder.summary()["nested_incomplete_ids"], [1])

    def test_nested_missing_actor_or_unreturned_is_explicit(self):
        self.recorder.record(frame("enter"), bytes(SIZE))
        self.recorder.record({
            "type": "nested-call", "nested_id": 2, "parent_sequence": 1,
            "site": "final_group_a", "kind": "action_update", "thread_id": 7,
            "record_address": "0x1000", "expected_record_address": "0x5003d0",
        }, None)
        self.assertEqual(self.recorder.summary()["unreturned_nested_ids"], [2])
        self.assertEqual(self.recorder.summary()["nested_incomplete_ids"], [2])
        self.recorder.record({
            "type": "nested-return", "nested_id": 2, "parent_sequence": 1,
            "site": "final_group_a", "kind": "action_update", "thread_id": 7,
        }, None)
        self.assertEqual(self.recorder.summary()["nested_complete_ids"], [])

    def test_nested_identity_and_memory_shape_are_checked(self):
        self.recorder.record({
            "type": "nested-call", "nested_id": 1, "parent_sequence": 1,
            "site": "final_group_a", "kind": "cached_act_stream", "thread_id": 7,
        }, None)
        self.assertIn("invalid nested event identity", self.recorder.failure)
        self.recorder.failure = None
        self.recorder.record(frame("enter"), bytes(SIZE))
        self.recorder.record({
            "type": "nested-call", "nested_id": 1, "parent_sequence": 1,
            "site": "final_group_a", "kind": "cached_act_stream", "thread_id": 7,
        }, None)
        self.recorder.record({
            "type": "nested-memory", "nested_id": 1, "parent_sequence": 1,
            "site": "final_group_a", "kind": "cached_act_stream",
            "phase": "enter", "region": "action-record", "byte_count": 152,
        }, bytes(152))
        self.assertIn("invalid nested memory", self.recorder.failure)


if __name__ == "__main__":
    unittest.main()
