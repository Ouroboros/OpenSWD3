import ida_auto
import ida_bytes
import ida_funcs
import ida_lines
import ida_xref
import idautils
import idc

OUT = idc.ARGV[1]
RANGES = []
for part in idc.ARGV[2].split(","):
    s, e = part.split("-")
    RANGES.append((int(s, 16), int(e, 16)))


def main():
    ida_auto.auto_wait()
    with open(OUT, "w", encoding="utf-8") as fh:
        for (s, e) in RANGES:
            fh.write("==== %08X-%08X\n" % (s, e))
            ea = s
            while ea < e and ea != idc.BADADDR:
                fn = ida_funcs.get_func(ea)
                owner = "%08X" % fn.start_ea if fn else "--------"
                flags = ida_bytes.get_full_flags(ea)
                kind = "C" if ida_bytes.is_code(flags) else ("D" if ida_bytes.is_data(flags) else "U")
                refs = ",".join("%08X:%s" % (x.frm, idautils.XrefTypeName(x.type)[:6]) for x in idautils.XrefsTo(ea, 0) if x.type != ida_xref.fl_F)
                name = idc.get_name(ea)
                fh.write("%08X %s %s %-24s %-60s %s\n" % (ea, owner, kind, name, idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)[:60], refs[:120]))
                nxt = idc.next_head(ea, e + 1)
                ea = nxt if nxt != idc.BADADDR else e
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(OUT, "a", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
