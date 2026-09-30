#!/usr/bin/env python3
"""Recorder tests with synthetic events. Never launch or attach to a process."""

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


def frame(phase, size=0x2B00):
    return {
        "type": "frame", "sequence": 1, "phase": phase,
        "actor_bytes": size, "problem": None,
        "registers": {"flags_available": False},
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

    def test_pair_requires_real_blocks(self):
        self.recorder.record(frame("enter"), bytes(0x2B00))
        self.recorder.record(frame("leave"), bytes(0x2B00))
        self.recorder.record({
            "type": "blocks", "sequence": 1, "blocks": [],
        }, None)
        self.assertEqual(self.recorder.summary()["snapshot_pairs_with_blocks"], [])
        # A delayed drain after leave is accepted without losing correlation.
        self.recorder.record({
            "type": "blocks", "sequence": 1,
            "blocks": [["0x479850", "0x479867"]],
        }, None)
        summary = self.recorder.summary()
        self.assertEqual(summary["snapshot_pairs_with_blocks"], [1])
        self.assertEqual(summary["flags_unavailable_sequences"], [1])
        self.assertFalse(summary["original_diff_verified"])
        self.assertFalse(summary["trace_drain_completion_verified"])

    def test_missing_actor_is_incomplete(self):
        self.recorder.record(frame("enter", 0), None)
        self.assertEqual(self.recorder.summary()["incomplete_sequences"], [1])

    def test_mismatched_size_is_failure(self):
        self.recorder.record(frame("enter"), bytes(4))
        self.assertIn("invalid snapshot size", self.recorder.failure)

    def test_duplicate_never_overwrites_snapshot(self):
        self.recorder.record(frame("enter"), bytes(0x2B00))
        self.recorder.record(frame("enter"), bytes([1]) * 0x2B00)
        self.assertIn("duplicate snapshot", self.recorder.failure)
        self.assertEqual((self.output / "actor-000001-enter.bin").read_bytes(), bytes(0x2B00))

    def test_exception_snapshot_is_not_normal_leave(self):
        self.recorder.record(frame("enter"), bytes(0x2B00))
        self.recorder.record(frame("exception-1"), bytes(0x2B00))
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


if __name__ == "__main__":
    unittest.main()
