import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_ida
import ida_lines
import ida_loader
import idautils
import idc

OUT_DIR = idc.ARGV[1]
LOG = []

REATTACH = [
    (0x00490A10, 0x00490A00, 0x00490A05, "shared epilogue: pop ebx without push, reached only from 0x490A2D inside sub_490A10"),
]


def hx(ea):
    return "0x%08X" % ea


def dis(ea):
    return idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)


def check(cond, msg):
    LOG.append(("OK   " if cond else "FAIL ") + msg)
    return cond


def reattach():
    for owner, s, e, why in REATTACH:
        fn = ida_funcs.get_func(s)
        if fn and fn.start_ea == s:
            ida_funcs.del_func(s)
        pfn = ida_funcs.get_func(owner)
        ok = ida_funcs.append_func_tail(pfn, s, e)
        ida_auto.auto_wait()
        f2 = ida_funcs.get_func(s)
        check(ok and f2 is not None and f2.start_ea == owner, "reattach %s-%s to %s (%s)" % (hx(s), hx(e), hx(owner), why))


def sp_audit():
    rows = []
    for fea in idautils.Functions():
        fn = ida_funcs.get_func(fea)
        if fn.flags & ida_funcs.FUNC_THUNK:
            continue
        for item in idautils.FuncItems(fea):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(item)):
                continue
            mnem = idc.print_insn_mnem(item)
            if mnem in ("retn", "ret"):
                spd = idc.get_spd(item)
                if spd is not None and spd != 0:
                    rows.append((fea, item, spd, dis(item)))
    with open(os.path.join(OUT_DIR, "sp_audit.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("func\tret_ea\tspd\tdisasm\n")
        for r in rows:
            fh.write("%s\t%s\t%d\t%s\n" % (hx(r[0]), hx(r[1]), r[2], r[3]))
    LOG.append("sp_audit nonzero-ret functions=%d sites=%d" % (len(set(r[0] for r in rows)), len(rows)))


def main():
    ida_auto.auto_wait()
    if ida_ida.inf_get_af() & ida_ida.AF_FLIRT:
        LOG.append("FAIL AF_FLIRT on")
        finish(2)
    reattach()
    sp_audit()
    LOG.append("functions %d" % sum(1 for _ in idautils.Functions()))
    ida_loader.save_database(ida_loader.get_path(ida_loader.PATH_TYPE_IDB), 0)
    LOG.append("saved")
    finish(0 if not any(l.startswith("FAIL") for l in LOG) else 4)


def finish(code):
    with open(os.path.join(OUT_DIR, "fix_boundaries2.log"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(LOG) + "\n")
    idc.qexit(code)


try:
    main()
except Exception:
    import traceback
    LOG.append("FAIL " + traceback.format_exc())
    finish(3)
