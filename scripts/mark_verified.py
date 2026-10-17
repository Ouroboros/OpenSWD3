"""Records the result of the per-function assembly verification in reverse/functions.tsv.

Usage: python3 scripts/mark_verified.py <workspace> <exact|diff> <evidence path> <address>... [--pass N]

verification_count is the number of verification passes the function has passed (pass N is the N-th full sweep of
the table, 1 by default). `exact` marks it assembly_exact with the count N (checking it again within the same pass
leaves the count unchanged); `diff` leaves it implemented_pending_review (differences found, to be fixed and checked
again in this pass) and does not change the count.
"""

import sys


def main():
    args = sys.argv[1:]
    sweep = 1
    if '--pass' in args:
        at = args.index('--pass')
        sweep = int(args[at + 1])
        del args[at:at + 2]
    root, result, evidence = args[0], args[1], args[2]
    if result not in ('exact', 'diff'):
        raise SystemExit('result must be exact or diff')
    if sweep < 1:
        raise SystemExit('the pass starts at 1')
    wanted = {'0x%08X' % int(a, 16) for a in args[3:]}
    path = root + '/reverse/functions.tsv'
    rows = open(path, encoding='utf-8').read().split('\n')
    found = set()
    for index, line in enumerate(rows):
        cells = line.split('\t')
        if len(cells) < 10 or cells[2] not in wanted:
            continue
        if cells[6] == 'pending_implementation':
            raise SystemExit('%s is not implemented' % cells[2])
        passed = int(cells[7] or '0')
        if result == 'exact':
            if passed < sweep - 1:
                raise SystemExit('%s has passed %d passes, it cannot pass pass %d' % (cells[2], passed, sweep))
            cells[6] = 'assembly_exact'
            cells[7] = str(max(passed, sweep))
        else:
            cells[6] = 'implemented_pending_review'
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
    print(len(found), 'marked', result, 'pass', sweep, counts)


main()
