#!/usr/bin/env python3
"""Read-only independent audit of the one Workpack316 v5 run."""
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import sys

repo = Path(__file__).resolve().parents[4]
run = repo / 'build/vm/battle-actor-frame-oracle-v5-output/run-20261003-101109-6380'
source = repo / 'analysis/tools/battle-actor-frame-oracle-v5'
sys.path.insert(0, str(source))
import capture


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


manifest = json.loads((run / 'run.json').read_text(encoding='utf-8'))
summary = json.loads((run / 'summary.json').read_text(encoding='utf-8'))
events = [json.loads(s) for s in (run / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
assert manifest['schema_version'] == 5
assert manifest['target_sha256'] == capture.EXPECTED_SHA256
assert manifest['agent_sha256'] == sha(source / 'agent.js')
assert manifest['bridge_sha256'] == sha(source / 'flags_bridge.c')
assert manifest['injected_source_sha256'] == hashlib.sha256(
    capture.agent_source(source / 'agent.js', manifest['sampling']).encode('utf-8')
).hexdigest()
assert summary['failure'] is None
files = {p.name for p in run.iterdir() if p.is_file()}
assert {'run.json', 'events.jsonl', 'summary.json'} <= files
references = {}
counts = Counter()
action_words = Counter()
calls = {}
returns = {}
frames = defaultdict(dict)
regions = defaultdict(dict)
nodes = defaultdict(list)
scans = {}
for event in events:
    kind = event['type']
    counts[kind] += 1
    filename = event.get('snapshot_file')
    if filename:
        assert Path(filename).name == filename and filename not in references, filename
        path = run / filename
        assert path.is_file(), f'missing {filename}'
        blob = path.read_bytes()
        expected = event['actor_bytes'] if kind == 'frame' else event['byte_count']
        assert len(blob) == expected, f'size {filename}: {len(blob)} != {expected}'
        assert hashlib.sha256(blob).hexdigest() == event['snapshot_sha256'], filename
        references[filename] = len(blob)
        if kind == 'list-node':
            assert event['value_flags'] == int.from_bytes(blob[0x4c:0x4e], 'little')
            assert event['next'].lower() == hex(int.from_bytes(blob[0:4], 'little'))
        elif kind == 'frame':
            action_words[(event['phase'],
                          int.from_bytes(blob[0x2a6c:0x2a6e], 'little'),
                          int.from_bytes(blob[0x2a70:0x2a72], 'little'))] += 1
    if kind == 'call':
        key = event['call_id']
        assert key not in calls
        calls[key] = event
    elif kind == 'call-return':
        key = event['call_id']
        assert key not in returns and key in calls
        returns[key] = event
    elif kind == 'frame':
        key = (event['sequence'], event['phase'])
        assert event['phase'] not in frames[event['sequence']]
        frames[event['sequence']][event['phase']] = event
        assert event['registers']['flags_available'] is True
    elif kind == 'memory':
        key = (event['sequence'], event['phase'], event['region'])
        assert key not in regions[event['sequence']]
        regions[event['sequence']][key] = event
    elif kind == 'list-node':
        nodes[event['list_id']].append(event)
    elif kind == 'list-scan':
        assert event['list_id'] not in scans
        scans[event['list_id']] = event
    else:
        raise AssertionError(f'unexpected event kind: {kind}')
assert files == set(references) | {'run.json', 'events.jsonl', 'summary.json'}, (len(files), len(references))
assert set(calls) == set(returns) == set(frames) == set(regions)
assert len(calls) == summary['entries'] == summary['leaves'] == 527
for ident, call in calls.items():
    ret = returns[ident]
    assert ret['site'] == call['site'] and ret['thread_id'] == call['thread_id']
    assert set(frames[ident]) == {'enter', 'leave'}
    assert frames[ident]['enter']['site'] == frames[ident]['leave']['site'] == call['site']
    assert len(regions[ident]) == 6
    for phase in ('enter', 'leave'):
        for region in ('stack', 'frame-header', 'list-head-word'):
            assert (ident, phase, region) in regions[ident]
assert {s['site'] for s in calls.values()} == {'final_group_a'}
assert len(nodes) == 1 and len(scans) == 2
for ident, scan in scans.items():
    these = sorted(nodes.get(ident, []), key=lambda e: e['index'])
    assert scan['complete'] is True and len(these) == scan['count']
    assert [e['index'] for e in these] == list(range(scan['count']))
    if these:
        for before, after in zip(these, these[1:]):
            assert before['next'].lower() == after['node'].lower()
        assert these[-1]['next'] == '0x0'
assert summary['list_complete_ids'] == sorted(scans)
assert summary['list_nodes'] == counts['list-node'] == 2
assert summary['list_nodes_with_low_word_seven'] == []
assert len(summary['snapshot_pairs_with_flags_without_trace']) == len(calls)
assert summary['four_callers_observed'] is False
assert summary['profile_calls'] == summary['action_seven_calls'] == summary['nested_calls'] == 0
report = {
    'run': run.name, 'run_manifest_target_sha256_claim': manifest['target_sha256'],
    'run_sha256': sha(run / 'run.json'), 'events_sha256': sha(run / 'events.jsonl'),
    'summary_sha256': sha(run / 'summary.json'),
    'events': len(events), 'event_types': dict(counts),
    'unique_referenced_snapshots': len(references), 'total_files': len(files),
    'sites': dict(Counter(event['site'] for event in calls.values())),
    'actor_action_words_by_phase': [
        {'phase': phase, 'main_2a6c': main, 'fallback_2a70': fallback,
         'frames': count}
        for (phase, main, fallback), count in sorted(action_words.items())
    ],
    'list_scans': [{
        'id': ident, 'actor': scan['actor'], 'root': scan['root'],
        'complete': scan['complete'], 'count': scan['count'],
        'values': [entry['value_flags'] for entry in nodes.get(ident, [])],
        'addresses': [entry['node'] for entry in nodes.get(ident, [])],
    } for ident, scan in sorted(scans.items())],
    'asset_identity_note': 'VM EXE hash taken from manifest, not independently rehashed',
}
print(json.dumps(report, ensure_ascii=False, indent=2))
