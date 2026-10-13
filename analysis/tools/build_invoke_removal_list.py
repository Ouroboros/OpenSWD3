import argparse
import csv
import hashlib
import json
import re
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
INVENTORY = ROOT / 'analysis/04-reverse-engineering/inventory'
EVIDENCE = ROOT / 'analysis/04-reverse-engineering/evidence'
EXTENSIONS = {'.h', '.hpp', '.c', '.cc', '.cpp', '.cxx'}
PATTERNS = {
    'invoke': r'(?<!std::)\b(?:invoke\w*|\w+_invoke)\s*\(',
    'register': r'\b(?:(?:entry_|return_)?(?:eax|ebx|ecx|edx|esi|edi|ebp|esp|eflags|st[0-7])|x87|x87_stack)\b',
    'callee': r'\b(?:callee(?:_token|_address|_id)?|constructor_token|destructor_token|operation_token)\b',
    'operation': r'\b(?:operation|call_kind|call_token|callee_id)\b',
    'arguments': r'\b(?:args|arguments|argument_count|stack_arguments)\b',
    'counter': r'\b(?:\w+_calls|call_count|\w+_call_count)\b',
    'interface': r'\b(?:[A-Z]\w*)?(?:Ports?|Dispatch(?:er)?|Adapters?)\b',
}
PATTERNS = {key: re.compile(value) for key, value in PATTERNS.items()}
LEXICAL = re.compile(r'//[^\n]*|/\*.*?\*/|R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
DEFINITION = re.compile(r'\b(class|struct)\s+([A-Za-z_]\w*)\s*(?:final\s*)?(?::[^;{}]*)?\{')
INTERFACE_NAME = re.compile(r'(?:Ports?|Dispatch(?:er)?|Adapters?)$')


def clean_text(text):
    return LEXICAL.sub(lambda match: ''.join('\n' if char == '\n' else ' ' for char in match.group()), text)


def source_link(path, line=None):
    suffix = f':{line}' if line else ''
    return f'[{path}{suffix}](../../../{path}{suffix})'


def stable_id(prefix, key):
    return prefix + '-' + hashlib.sha256(key.encode()).hexdigest()[:12]


def build():
    parser = argparse.ArgumentParser()
    parser.add_argument('--replace-snapshot', action='store_true')
    options = parser.parse_args()
    destinations = [
        INVENTORY / 'invoke-removal-list.json',
        EVIDENCE / 'invoke-removal-list.md',
        EVIDENCE / 'invoke-removal-ports.md',
    ]
    if not options.replace_snapshot and any(path.exists() for path in destinations):
        raise SystemExit('Refusing to overwrite checklist records; explicit --replace-snapshot required.')

    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    paths = sorted(path for directory in ('include', 'src') for path in (ROOT / directory).rglob('*') if path.suffix in EXTENSIONS)
    tests = sorted(path for path in (ROOT / 'tests').rglob('*') if path.suffix in EXTENSIONS)
    sources = {}
    occurrences = defaultdict(list)
    interfaces = []
    for path in paths + tests:
        relative = path.relative_to(ROOT).as_posix()
        raw = path.read_bytes()
        text = raw.decode('utf-8')
        clean = clean_text(text)
        for number, line in enumerate(clean.splitlines(), 1):
            for symbol in set(re.findall(r'\b[A-Za-z_]\w*\b', line)):
                occurrences[symbol].append({'path': relative, 'line': number})

        if path not in paths:
            continue

        hits = []
        lines = text.splitlines()
        for number, line in enumerate(clean.splitlines(), 1):
            kinds = [key for key, pattern in PATTERNS.items() if pattern.search(line)]
            if kinds:
                hits.append({'line': number, 'kinds': kinds, 'text': lines[number - 1].strip()})

        sources[relative] = {'sha256': hashlib.sha256(raw).hexdigest(), 'hits': hits}
        for match in DEFINITION.finditer(clean):
            name = match.group(2)
            if not INTERFACE_NAME.search(name):
                continue

            opening = match.end() - 1
            closing = opening + 1
            depth = 1
            while depth and closing < len(clean):
                depth += (clean[closing] == '{') - (clean[closing] == '}')
                closing += 1

            if depth:
                raise ValueError(f'Unclosed definition: {relative}:{name}')

            body = clean[opening + 1:closing - 1]
            prefix = clean[match.start():opening]
            identity = relative + '::' + name + '::' + ' '.join(prefix.split())
            entry = {
                'id': stable_id('PORT', identity), 'name': name, 'path': relative,
                'line': text.count('\n', 0, match.start()) + 1,
                'bases': prefix.split(':', 1)[1].strip() if ':' in prefix else '',
                'kind': '抽象接口' if re.search(r'=\s*0\s*;', body) else '实现或转发类',
                'signals': [key for key, pattern in PATTERNS.items() if key != 'interface' and pattern.search(body)],
                'status': 'pending_review',
            }
            interfaces.append(entry)

    seen_ids = Counter(entry['id'] for entry in interfaces)
    if any(count != 1 for count in seen_ids.values()):
        raise ValueError('Duplicate interface identity; add enclosing class scope before publishing')

    for entry in interfaces:
        entry['references'] = occurrences[entry['name']]

    with (INVENTORY / 'invoke-migration-progress.tsv').open() as stream:
        previous = list(csv.DictReader(stream, delimiter='\t'))

    units = []
    covered = set()
    for row in previous:
        files = [path for path in row['source_files'].split(';') if path in sources]
        covered.update(files)
        units.append({
            'id': stable_id('CHAIN', row['unit']), 'unit': row['unit'],
            'files': files, 'historical_status': row['status'],
            'historical_verification_commit': row['verification_commit'],
            'historical_evidence': row['evidence'], 'historical_reason': row['reason'],
            'status': 'pending_review',
        })

    for path in sorted(set(sources) - covered):
        units.append({
            'id': stable_id('FILE', path), 'unit': path, 'files': [path],
            'historical_status': 'not_in_previous_inventory',
            'historical_verification_commit': '', 'historical_evidence': '',
            'historical_reason': '', 'status': 'pending_review',
        })

    current = {'battle/legacy_battle_actor_list_query', 'battle/legacy_battle_grid_frame', 'battle/legacy_battle_selection_frame', 'battle/legacy_battle_frame_coordinator'}
    for unit in units:
        unit['signals'] = sorted({kind for path in unit['files'] for hit in sources[path]['hits'] for kind in hit['kinds']})
        unit['interfaces'] = [entry['id'] for entry in interfaces if entry['path'] in unit['files']]
        unit['priority'] = 0 if unit['unit'] in current else (1 if set(unit['signals']) - {'interface'} else 2)

    units.sort(key=lambda entry: (entry['priority'], entry['unit']))
    summary = {
        'revision': revision, 'snapshot': 'current_worktree',
        'production_files': len(sources), 'test_reference_files': len(tests),
        'historical_units': len(previous), 'checklist_units': len(units),
        'supplemental_file_units': len(units) - len(previous),
        'interface_definitions': len(interfaces),
        'port_definitions': sum(bool(re.search(r'Ports?$', entry['name'])) for entry in interfaces),
        'interface_definitions_with_direct_signals': sum(bool(entry['signals']) for entry in interfaces),
        'historical_statuses': dict(Counter(row['status'] for row in previous)),
        'current_snapshot_verified': 0,
        'scope_exhaustiveness_verified': False,
    }
    artifact = {'summary': summary, 'units': units, 'interfaces': interfaces, 'sources': sources}
    (INVENTORY / 'invoke-removal-list.json').write_text(json.dumps(artifact, ensure_ascii=False, indent=2) + '\n')
    intro = [
        '# INVOKE 逐项迁移清单', '',
        '## 使用规则', '',
        '- 这是当前工作树的逐项执行清单。固定ID不随排序变化。',
        '- 方框只在实际调用链完成迁移、LST复核、相关测试和构建通过、提交推送后勾选。',
        '- 每次只实现一个可独立验证批次。子切片发布只能更新该条断点，不能勾选整条。',
        '- 未命中关键词不等于完成。纯转发、继承协议、资源所有权仍须逐项人工复核。',
        '- 平台边界和合法业务操作不能仅按名字删除；保留必须填写具体理由与证据。',
        '- 旧台账的verified/excluded单独保留。新快照未复验，不抹去历史，也不冒充当前通过。',
        '- 下列文件路径是定义/实现范围。逐个类型的所有生产与测试符号引用在[Port清单](invoke-removal-ports.md)。',
        '- 符号引用是调用方导航，包含声明和同名引用；还需核实虚调用、继承和别名接线。',
        '- 每次发布更新本条：已删协议、剩余协议、测试日志、提交、远端验证。',
        '- 自动生成器只用于建立快照；已开始人工更新后不得直接覆盖勾选和验收记录。', '',
        '## 当前基线', '',
        f'- HEAD：`{revision}`；包含未提交行查询改动，该批未验证。',
        f'- 生产文件：{len(sources)}；每个文件均列入清单及SHA256快照。',
        f'- 旧台账：{len(previous)}族；状态分布：`{summary["historical_statuses"]}`。',
        f'- 本清单：{len(units)}条，其中{len(units) - len(previous)}条补充覆盖旧台账之外的生产文件。',
        f'- Port/Ports定义：{summary["port_definitions"]}；含Dispatch/Adapter共{len(interfaces)}个类型定义。',
        '- 上述数值分别是族、文件和类型的数量，不能相加或作为同一完成率分母。',
        '- 全项目语义覆盖尚未验收。所有新清单方框保持未勾选。',
        '- 当前顺序：先核完清单与遗漏；再收敛已存在的行查询改动，沿角色列表调用链继续。', '',
        '## 逐项 LIST', '',
    ]
    for unit in units:
        intro.extend([
            f'- [ ] **{unit["id"]}** `{unit["unit"]}`',
            f'  - 当前残留信号：{", ".join(unit["signals"]) or "无关键词命中，仍待语义复核"}。',
            f'  - 历史状态：`{unit["historical_status"]}`；本快照：待复核。',
            '  - 文件：' + '；'.join(source_link(path) for path in unit['files']) + '。',
            '  - 类型：' + (', '.join(f'[{identity}](invoke-removal-ports.md#{identity.lower()})' for identity in unit['interfaces']) or '无具名Port/Dispatch/Adapter定义，检查函数与数据协议') + '。',
            '  - 验收：调用链、LST、失败路径、所有权、测试、提交与远端均待记录。',
        ])
        for path in unit['files']:
            locations = defaultdict(list)
            for hit in sources[path]['hits']:
                for kind in hit['kinds']:
                    if kind != 'interface':
                        locations[kind].append(hit['line'])

            for kind, lines in sorted(locations.items()):
                intro.append(f'  - `{kind}` {source_link(path)}：' + ', '.join(str(line) for line in lines))

        intro.append('')
    (EVIDENCE / 'invoke-removal-list.md').write_text('\n'.join(intro))
    details = ['# Port、Dispatch、Adapter 定义与引用清单', '',
        '对应[总清单](invoke-removal-list.md)。全部来自当前生产源码。', '',
        '每项列出定义、直接残留特征、继承和全部符号引用文件/行。',
        '无直接特征的类型仍待检查继承协议及纯转发；不自动判定应删除或可保留。', '',
    ]
    for entry in sorted(interfaces, key=lambda row: (row['path'], row['line'])):
        details.extend([
            f'## {entry["id"]}', '',
            f'- [ ] `{entry["name"]}`：{entry["kind"]}；待复核。',
            f'- 定义：{source_link(entry["path"], entry["line"])}。',
            f'- 继承：`{entry["bases"] or "无"}`。',
            f'- 直接残留：{", ".join(entry["signals"]) or "未命中；须检查纯转发及继承"}。',
            '- 处理与验收记录：待填写。',
            '- 引用位置：',
        ])
        references = defaultdict(list)
        for reference in entry['references']:
            references[reference['path']].append(reference['line'])

        for path, lines in sorted(references.items()):
            details.append(f'  - {source_link(path)}：' + ', '.join(str(line) for line in lines))

        details.append('')
    (EVIDENCE / 'invoke-removal-ports.md').write_text('\n'.join(details))
    assert set(sources) == {path for unit in units for path in unit['files']}
    assert len({unit['id'] for unit in units}) == len(units)
    assert all(entry['references'] for entry in interfaces)
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    print('All production files mapped; IDs unique; all interface references indexed.')


if __name__ == '__main__':
    build()
