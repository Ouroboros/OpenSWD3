import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_nalt
import ida_segment
import idautils
import idc

OUT_DIR = idc.ARGV[1]


def hx(ea):
    return "0x%08X" % ea


def import_names():
    names = {}
    for i in range(ida_nalt.get_import_module_qty()):
        mod = ida_nalt.get_import_module_name(i)

        def cb(ea, name, ordinal, mod=mod):
            names[ea] = "%s!%s" % (mod, name or "#%d" % ordinal)
            return True

        ida_nalt.enum_import_names(i, cb)
    return names


def thunk_target(ea, imps):
    fn = ida_funcs.get_func(ea)
    if fn and fn.flags & ida_funcs.FUNC_THUNK:
        for tgt in idautils.DataRefsFrom(ea):
            if tgt in imps:
                return imps[tgt]
    return None


def main():
    ida_auto.auto_wait()
    imps = import_names()
    with open(os.path.join(OUT_DIR, "func_refs.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("func\tname\tapis\tstrings\tglobals_written\tglobals_read\n")
        for fea in idautils.Functions():
            apis = set()
            strs = []
            gw = set()
            gr = set()
            for item in idautils.FuncItems(fea):
                for tgt in idautils.CodeRefsFrom(item, 0):
                    if tgt in imps:
                        apis.add(imps[tgt])
                    else:
                        t = thunk_target(tgt, imps)
                        if t:
                            apis.add(t)
                for xr in idautils.XrefsFrom(item, 0):
                    tgt = xr.to
                    if tgt in imps:
                        apis.add(imps[tgt])
                        continue
                    seg = ida_segment.getseg(tgt)
                    if not seg or ida_segment.get_segm_name(seg) == ".text":
                        continue
                    flags = ida_bytes.get_full_flags(tgt)
                    if ida_bytes.is_strlit(flags):
                        s = idc.get_strlit_contents(tgt, -1, idc.get_str_type(tgt))
                        if s is not None:
                            txt = s.decode("cp936", "replace").replace("\t", " ").replace("\n", "\\n").replace("\r", "\\r")
                            strs.append(txt[:60])
                    elif xr.type == idautils.ida_xref.dr_W:
                        gw.add(tgt)
                    elif xr.type in (idautils.ida_xref.dr_R, idautils.ida_xref.dr_O):
                        gr.add(tgt)
            fh.write("%s\t%s\t%s\t%s\t%s\t%s\n" % (
                hx(fea), ida_funcs.get_func_name(fea), ",".join(sorted(apis)),
                " | ".join(dict.fromkeys(strs)), ",".join(hx(x) for x in sorted(gw)), ",".join(hx(x) for x in sorted(gr))))
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(os.path.join(OUT_DIR, "func_refs.err"), "w", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
