import collections
import re
import sys

root = sys.argv[1]
rows = []
for line in open(root + '/reverse/analysis/ida/text_pointer_scan.tsv', encoding='utf-8'):
    cells = line.rstrip('\n').split('\t')
    if cells[0] == 'slot':
        continue
    while len(cells) < 8:
        cells.append('')
    rows.append(cells)
groups = collections.Counter()
detail = collections.defaultdict(list)
for slot, segment, target, role, func, name, is_offset, disasm in rows:
    prefix = re.sub(r'_[0-9A-F]+.*$', '', name) if name else '-'
    key = (segment, role, prefix, is_offset)
    groups[key] += 1
    detail[key].append((slot, target, func, name, disasm))
out = open(root + '/tmp/ptrscan.txt', 'w', encoding='utf-8')
for key, count in groups.most_common():
    out.write('%s %s name=%s offset=%s: %d\n' % (key + (count,)))
    if key[1] != 'func_start' and not (key[2] == 'jpt' or 'jumptable' in ' '.join(d[4] for d in detail[key][:1])):
        for slot, target, func, name, disasm in detail[key][:12]:
            out.write('    %s -> %s func=%s name=%s %s\n' % (slot, target, func, name, disasm[:60]))
out.close()
print(len(rows))
