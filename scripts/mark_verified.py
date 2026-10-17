"""Records a round of the per-function assembly verification in reverse/functions.tsv.

Usage: python3 scripts/mark_verified.py <workspace> <exact|diff> <evidence path> <address>...
Each address (0x00401000 or 401000) gets one more verification round and the evidence path; `exact` marks it
assembly_exact, `diff` leaves it implemented_pending_review (differences found and fixed, verify again).
"""

import sys


def main():
    root, result, evidence = sys.argv[1], sys.argv[2], sys.argv[3]
    if result not in ('exact', 'diff'):
        raise SystemExit('result must be exact or diff')
    wanted = {'0x%08X' % int(a, 16) for a in sys.argv[4:]}
    path = root + '/reverse/functions.tsv'
    rows = open(path, encoding='utf-8').read().split('\n')
    found = set()
    for index, line in enumerate(rows):
        cells = line.split('\t')
        if len(cells) < 10 or cells[2] not in wanted:
            continue
        if cells[6] == 'pending_implementation':
            raise SystemExit('%s is not implemented' % cells[2])
        cells[6] = 'assembly_exact' if result == 'exact' else 'implemented_pending_review'
        cells[7] = str(int(cells[7] or '0') + 1)
        cells[8] = evidence
        rows[index] = '\t'.join(cells)
        found.add(cells[2])
    missing = sorted(wanted - found)
    if missing:
        raise SystemExit('not in the table: ' + ' '.join(missing))
    open(path, 'w', encoding='utf-8').write('\n'.join(rows))
    counts = {}
    for line in rows[1:]:
        cells = line.split('\t')
        if len(cells) > 6:
            counts[cells[6]] = counts.get(cells[6], 0) + 1
    print(len(found), 'marked', result, counts)


main()
