from pathlib import Path
from collections import defaultdict, Counter
import csv
import json
import re
import subprocess

output = Path('build/tmp/runtime/invoke-project-audit')
output.mkdir(parents=True, exist_ok=True)
review_path = Path('analysis/04-reverse-engineering/inventory/invoke-migration-review.json')
reviews = json.loads(review_path.read_text())
refs = {'baseline': reviews['baseline'], 'current': 'HEAD'}
patterns = {
    'invoke': r'(?<!std::)\b(?:invoke\w*|\w+_invoke)\s*\(',
    'register': r'\b(?:(?:entry_|return_)?(?:eax|ebx|ecx|edx|esi|edi|ebp|esp|eflags|st[0-7])|x87|x87_stack)\b',
    'callee': r'\b(?:callee(?:_token|_address|_id)?|constructor_token|destructor_token|operation_token)\b',
    'operation': r'\b(?:operation|call_kind|call_token|callee_id)\b',
    'arguments': r'\b(?:args|arguments|argument_count|stack_arguments)\b',
    'counter': r'\b(?:\w+_calls|call_count|\w+_call_count)\b',
    'interface': r'\b(?:[A-Z]\w*)?(?:Ports?|Dispatch(?:er)?|Adapters?)\b',
}
patterns = {name: re.compile(pattern) for name, pattern in patterns.items()}
lexical = re.compile(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')
groups = defaultdict(dict)
metadata = {}
for label, ref in refs.items():
    revision = subprocess.check_output(['git', 'rev-parse', ref], text=True).strip()
    tree = subprocess.check_output(['git', 'ls-tree', '-r', revision], text=True)
    entries = []
    for line in tree.splitlines():
        attributes, name = line.split('\t', 1)
        if name.startswith(('include/', 'src/')) and Path(name).suffix in {'.h', '.hpp', '.c', '.cc', '.cpp', '.cxx'}:
            entries.append((attributes.split()[2], name))
    public_headers = {str(Path(name.removeprefix('include/openswd3/')).with_suffix('')) for _, name in entries if name.startswith('include/openswd3/')}
    request = ''.join(object_id + '\n' for object_id, _ in entries).encode()
    raw = subprocess.run(['git', 'cat-file', '--batch'], input=request, stdout=subprocess.PIPE, check=True).stdout
    cursor = 0
    records = []
    for object_id, name in entries:
        boundary = raw.index(b'\n', cursor)
        header = raw[cursor:boundary].decode().split()
        assert header[0] == object_id and header[1] == 'blob'
        size = int(header[2])
        text = raw[boundary + 1:boundary + 1 + size].decode('utf-8', errors='replace')
        cursor = boundary + size + 2
        clean = lexical.sub(lambda match: ''.join('\n' if char == '\n' else ' ' for char in match.group()), text)
        lines = text.splitlines()
        hits = []
        for number, line in enumerate(clean.splitlines(), 1):
            kinds = [kind for kind, pattern in patterns.items() if pattern.search(line)]
            if kinds:
                hits.append({'line': number, 'kinds': kinds, 'text': lines[number - 1].strip()})
        if not hits:
            continue
        parts = Path(name).parts
        area = parts[2] if parts[0] == 'include' else parts[1]
        key = area + '/' + Path(name).stem
        if name.startswith('src/') and key not in public_headers:
            candidates = [key.removesuffix('_internal')]
            for included in re.findall(r'^#include\s+"([^"]+)"', text, re.M):
                candidate = str(Path(included.removeprefix('openswd3/')).with_suffix(''))
                if '/' not in candidate:
                    candidate = area + '/' + candidate
                candidate = candidate.removesuffix('_internal')
                if key.startswith(candidate + '_'):
                    candidates.append(candidate)
            key = next((candidate for candidate in candidates if candidate in public_headers), key)
        key = reviews.get('unit_aliases', {}).get(key, key)
        groups[key].setdefault(label, []).append({'path': name, 'hits': hits})
        records.append({'path': name, 'hits': hits})
    metadata[label] = {'revision': revision, 'scanned_files': len(entries), 'candidate_files': len(records), 'candidate_lines': sum(len(row['hits']) for row in records)}
    (output / (label + '.json')).write_text(json.dumps(records, ensure_ascii=False, indent=2) + '\n')

current_paths = {name for _, name in entries}
for key, review in reviews['units'].items():
    if review.get('manual_scope'):
        if not review.get('files') or not set(review['files']) <= current_paths:
            raise ValueError('Manual scope requires current production source files: ' + key)
        if not review.get('reason') or not review.get('evidence'):
            raise ValueError('Manual scope requires semantic evidence: ' + key)
        groups.setdefault(key, {})

ordered = dict(sorted(groups.items()))
(output / 'groups.json').write_text(json.dumps(ordered, ensure_ascii=False, indent=2) + '\n')
(output / 'metadata.json').write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + '\n')
with (output / 'groups.tsv').open('w') as stream:
    stream.write('family\tbaseline_kinds\tcurrent_kinds\tclassification\n')
    for key, versions in ordered.items():
        kinds = {}
        for label in refs:
            kinds[label] = sorted({kind for row in versions.get(label, []) for hit in row['hits'] for kind in hit['kinds']})
        status = 'review_required' if kinds['current'] else 'clear_candidate_not_verified'
        stream.write(f"{key}\t{','.join(kinds['baseline'])}\t{','.join(kinds['current'])}\t{status}\n")
with (output / 'clear-candidates.md').open('w') as stream:
    for key, versions in ordered.items():
        if 'baseline' not in versions or 'current' in versions:
            continue
        stream.write(f'## {key}\n\n')
        for row in versions['baseline']:
            stream.write(row['path'] + '\n\n')
            for hit in row['hits']:
                stream.write(f"- {hit['line']}: {hit['text']}\n")
        stream.write('\n')
rows = []
for key, versions in ordered.items():
    review = reviews['units'].get(key, {})
    status = review.get('status', 'pending')
    if status not in {'verified', 'pending', 'excluded'}:
        raise ValueError('Unknown review status: ' + key)
    reason = review.get('reason', '候选协议或接口尚待逐项迁移、语义复核及调用方验收')
    if status in {'verified', 'excluded'}:
        if not review.get('files') or not review.get('evidence'):
            raise ValueError('Reviewed unit requires source files and evidence: ' + key)
        source_check = subprocess.run([
            'git', 'diff', '--quiet', review['verification_commit'],
            metadata['current']['revision'], '--', *review['files'],
        ])
        if source_check.returncode not in {0, 1}:
            raise RuntimeError('Cannot compare reviewed source: ' + key)
        current_candidates = {entry['path'] for entry in versions.get('current', [])}
        if (source_check.returncode or
                not current_candidates <= set(review['files']) or
                (status == 'verified' and versions.get('current'))):
            status = 'pending'
            reason = '已复核源码发生变化或重新出现候选信号，必须重新验收'
    if status == 'verified':
        logs = review.get('validation_logs', [])
        if not logs or not all(
            Path(path).is_file() and
            '100% tests passed, 0 tests failed' in Path(path).read_text()
            for path in logs
        ):
            status = 'pending'
            reason = '历史定向验证日志缺失或未证明通过，必须补充验证依据'
    evidence = review.get('evidence', '')
    if evidence and not Path(evidence).is_file():
        raise RuntimeError('Missing evidence: ' + evidence)
    rows.append({
        'unit': key,
        'status': status,
        'baseline_signals': ','.join(sorted({kind for entry in versions.get('baseline', []) for hit in entry['hits'] for kind in hit['kinds']})),
        'current_signals': ','.join(sorted({kind for entry in versions.get('current', []) for hit in entry['hits'] for kind in hit['kinds']})),
        'source_files': ';'.join(sorted({entry['path'] for entries in versions.values() for entry in entries} | set(review.get('files', [])))),
        'verification_commit': review.get('verification_commit', ''),
        'evidence': evidence,
        'reason': reason,
    })
missing_units = set(reviews['units']) - set(ordered)
if missing_units:
    raise RuntimeError('Review units disappeared; reconcile explicitly: ' + ', '.join(sorted(missing_units)))
with (output / 'invoke-migration-progress.tsv').open('w', newline='') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]), delimiter='\t', lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)
counts = Counter(row['status'] for row in rows)
total = len(rows) - counts['excluded']
summary = {
    'source_revision': metadata['current']['revision'],
    'baseline_revision': metadata['baseline']['revision'],
    'scanned_production_files': metadata['current']['scanned_files'],
    'candidate_units': len(rows),
    'excluded_units': counts['excluded'],
    'scope_units': total,
    'verified_units': counts['verified'],
    'pending_units': total - counts['verified'],
    'ledger_percent': round(100 * counts['verified'] / total, 1) if total else 0,
    'scope_exhaustiveness_verified': False,
}
(output / 'progress-summary.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n')
print(json.dumps(summary, ensure_ascii=False))
print('This is a provisional project-wide review ledger, not proof of complete scope or goal acceptance.')
print('Artifacts:', output)
