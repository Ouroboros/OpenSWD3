import os
import re
import sys

root = sys.argv[1]
analysis = root + '/reverse/analysis'
starts = set()
ranges = []
for line in open(root + '/reverse/functions.tsv', encoding='utf-8'):
    cells = line.rstrip('\n').split('\t')
    if len(cells) > 2 and cells[2].startswith('0x'):
        starts.add(int(cells[2], 16))
text_low, text_high = 0x401000, 0x499000
# function ranges from the raw export (start, end)
bounds = {}
for line in open(analysis + '/ida/functions_raw.tsv', encoding='utf-8'):
    cells = line.rstrip('\n').split('\t')
    try:
        start = int(cells[0], 16)
        end = int(cells[1], 16)
    except (ValueError, IndexError):
        continue
    bounds[start] = end
ordered = sorted(bounds)


def owner(address):
    import bisect
    index = bisect.bisect_right(ordered, address) - 1
    if index >= 0 and ordered[index] <= address < bounds[ordered[index]]:
        return ordered[index]
    return None


# 1. code immediates into .text
immediate = {}
pattern = re.compile(r'^([0-9A-F]{8}) .*?\b(push|mov|lea|cmp)\b[^;]*\boffset (?:sub|loc|j_sub|nullsub|unk|byte|dword|word|off|stru)_([0-9A-F]+)')
for name in sorted(os.listdir(analysis + '/asm')):
    for line in open(os.path.join(analysis + '/asm', name), encoding='utf-8', errors='replace'):
        found = pattern.match(line)
        if not found:
            continue
        target = int(found.group(3), 16)
        if text_low <= target < text_high:
            immediate.setdefault(target, []).append(int(found.group(1), 16))
non_start = {t: s for t, s in immediate.items() if t not in starts}
# 2. unresolved indirect sites
unresolved = []
for line in open(analysis + '/ida/indirect_sites.tsv', encoding='utf-8'):
    cells = line.rstrip('\n').split('\t')
    if len(cells) < 7 or cells[0] == 'ea':
        continue
    if cells[3] == 'switch' or cells[6]:
        continue
    unresolved.append(cells)
kinds = {}
for cells in unresolved:
    text = cells[5]
    if re.search(r'call\s+(?:dword ptr )?\[e[a-z]{2}\+[0-9A-Fa-fh]*\]', text) or re.search(r'call\s+dword ptr \[e..\]', text):
        kind = 'vtable/member pointer call'
    elif re.search(r'call\s+e[a-z]{2}\b', text):
        kind = 'register call'
    elif re.search(r'call\s+(?:dword ptr )?(?:ds:)?[a-z_]*?off_|call\s+ds:', text):
        kind = 'memory pointer call'
    elif cells[1] == 'jmp':
        kind = 'jmp'
    else:
        kind = 'other'
    kinds.setdefault(kind, []).append(cells)
out = open(root + '/tmp/scope_audit.txt', 'w', encoding='utf-8')
out.write('immediate .text targets: %d, not a function start: %d\n' % (len(immediate), len(non_start)))
for target in sorted(non_start):
    out.write('  %08X owner=%s from %s\n' % (target, '%08X' % owner(target) if owner(target) else '-', ' '.join('%08X' % s for s in non_start[target][:4])))
out.write('unresolved indirect sites: %d\n' % len(unresolved))
for kind, cells in sorted(kinds.items()):
    out.write('%s: %d\n' % (kind, len(cells)))
    for c in cells:
        out.write('  %s func=%s %s\n' % (c[0], c[4], c[5][:90]))
out.close()
print(len(immediate), len(non_start), len(unresolved))
