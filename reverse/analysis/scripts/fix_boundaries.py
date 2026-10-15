import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_ida
import ida_lines
import ida_loader
import ida_nalt
import idautils
import idc

OUT_DIR = idc.ARGV[1]
LOG = []

FRAGMENTS_TO_MERGE = [
    (0x00417EC0, [0x00417FEA, 0x00417FEF, 0x00417FF4, 0x0041802D, 0x00418066, 0x0041809F, 0x0041830E], 0x00418346,
     "jmp ebx dispatch blocks of sub_417EC0: block addresses stored by lea in sub_417EC0, blocks jump back to loc_417FD7/loc_417FD1"),
    (0x00489680, [0x004896AD], 0x004896B6, "epilogue after jmp eax matching prologue push ebx/esi/edi"),
    (0x0048A740, [0x0048A87A], 0x0048A897, "__except handler of start using [ebp-18h]/[ebp-68h] frame slots"),
    (0x0048A8D0, [], 0x0048A8FA, "epilogue after ExitProcess"),
    (0x00431150, [], 0x004311B7, "epilogue after int 3"),
]
DATA_STRINGS = [(0x0049091C, 8, "VC20XC0 marker string misdefined as code")]


def hx(ea):
    return "0x%08X" % ea


def dis(ea):
    return idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)


def check(cond, msg):
    LOG.append(("OK   " if cond else "FAIL ") + msg)
    return cond


def merge_fragments():
    for owner, frags, new_end, why in FRAGMENTS_TO_MERGE:
        for f in frags:
            fn = ida_funcs.get_func(f)
            if fn and fn.start_ea == f:
                ida_funcs.del_func(f)
                LOG.append("del_func %s" % hx(f))
        ok = ida_funcs.set_func_end(owner, new_end)
        ida_auto.auto_wait()
        fn = ida_funcs.get_func(owner)
        check(ok and fn is not None and fn.start_ea == owner and fn.end_ea == new_end,
              "merge %s end=%s (%s) got_end=%s" % (hx(owner), hx(new_end), why, hx(fn.end_ea) if fn else "none"))
        ea = owner
        orphans = 0
        while ea < new_end:
            if ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
                f2 = ida_funcs.get_func(ea)
                if f2 is None or f2.start_ea != owner:
                    orphans += 1
            ea = idc.next_head(ea, new_end + 1)
            if ea == idc.BADADDR:
                break
        check(orphans == 0, "merge %s no foreign/unowned code heads in range (%d)" % (hx(owner), orphans))


def fix_data():
    for ea, size, why in DATA_STRINGS:
        fn = ida_funcs.get_func(ea)
        if fn and fn.start_ea == ea:
            ida_funcs.del_func(ea)
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
        ok = ida_bytes.create_strlit(ea, size, ida_nalt.STRTYPE_C)
        check(ok and ida_bytes.is_data(ida_bytes.get_full_flags(ea)), "data %s size %d (%s)" % (hx(ea), size, why))


def aligned_boundary(ea):
    if ida_bytes.is_flow(ida_bytes.get_full_flags(ea)):
        return False
    prev = idc.prev_head(ea, 0)
    pflags = ida_bytes.get_full_flags(prev)
    if ida_bytes.is_align(pflags) or ida_bytes.is_unknown(pflags):
        return True
    if ida_bytes.is_code(pflags) and ea % 16 == 0:
        return idc.print_insn_mnem(prev) in ("retn", "ret", "jmp")
    return False


def split_tails():
    total = 0
    for rnd in range(8):
        tails = {}
        for fea in idautils.Functions():
            for (s, e) in idautils.Chunks(fea):
                if s != fea:
                    tails.setdefault(s, set()).add(fea)
        cands = sorted(s for s in tails if aligned_boundary(s))
        if not cands:
            break
        made = 0
        for s in cands:
            parents = sorted(tails[s])
            for p in parents:
                pfn = ida_funcs.get_func(p)
                if pfn:
                    ida_funcs.remove_func_tail(pfn, s)
            ok = ida_funcs.add_func(s, idc.BADADDR)
            ida_auto.auto_wait()
            fn = ida_funcs.get_func(s)
            good = ok and fn is not None and fn.start_ea == s
            if good:
                last = idc.prev_head(fn.end_ea, s)
                nxt = ida_funcs.get_next_func(s)
                good = idc.print_insn_mnem(last) in ("retn", "ret", "jmp") and (nxt is None or fn.end_ea <= nxt.start_ea)
            check(good, "split tail %s parents=%s -> func end=%s last=%s refs=%s" % (
                hx(s), ",".join(hx(p) for p in parents), hx(fn.end_ea) if fn else "none",
                dis(idc.prev_head(fn.end_ea, s)) if fn else "",
                ",".join("%s" % hx(x.frm) for x in idautils.XrefsTo(s, 0))))
            if good:
                made += 1
        total += made
        LOG.append("split round %d made %d" % (rnd, made))
        if made == 0:
            break
    return total


def main():
    ida_auto.auto_wait()
    if ida_ida.inf_get_af() & ida_ida.AF_FLIRT:
        LOG.append("FAIL AF_FLIRT on")
        finish(2)
    before = sum(1 for _ in idautils.Functions())
    merge_fragments()
    fix_data()
    n = split_tails()
    ida_auto.auto_wait()
    after = sum(1 for _ in idautils.Functions())
    remaining = []
    for fea in idautils.Functions():
        ch = list(idautils.Chunks(fea))
        if len(ch) > 1:
            remaining.append("%s %s" % (hx(fea), ";".join("%s-%s" % (hx(s), hx(e)) for s, e in ch)))
    LOG.append("functions %d -> %d, split %d, remaining multi-chunk %d" % (before, after, n, len(remaining)))
    LOG.extend("multichunk " + r for r in remaining)
    ida_loader.save_database(ida_loader.get_path(ida_loader.PATH_TYPE_IDB), 0)
    LOG.append("saved")
    finish(0 if not any(l.startswith("FAIL") for l in LOG) else 4)


def finish(code):
    with open(os.path.join(OUT_DIR, "fix_boundaries.log"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(LOG) + "\n")
    idc.qexit(code)


try:
    main()
except Exception:
    import traceback
    LOG.append("FAIL " + traceback.format_exc())
    finish(3)
