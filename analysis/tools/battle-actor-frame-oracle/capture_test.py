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


if __name__ == "__main__":
    unittest.main()
