import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_name
import ida_segment
import ida_xref
import idautils
import idc

OUT_DIR = idc.ARGV[1]
ASM_DIR = os.path.join(OUT_DIR, "asm")
DEC_DIR = os.path.join(OUT_DIR, "decomp")
os.makedirs(ASM_DIR, exist_ok=True)
os.makedirs(DEC_DIR, exist_ok=True)
ERRORS = []


def hx(ea):
    return "%08X" % ea


def asm_listing(fea):
    lines = []
    fn = ida_funcs.get_func(fea)
    lines.append("; function %s %s flags=0x%X frsize=%d frregs=%d argsize=%d" % (
        hx(fea), ida_funcs.get_func_name(fea), fn.flags, fn.frsize, fn.frregs, fn.argsize))
    for (cs, ce) in idautils.Chunks(fea):
        lines.append("; chunk %s-%s" % (hx(cs), hx(ce)))
        ea = cs
        while ea < ce and ea != idc.BADADDR:
            flags = ida_bytes.get_full_flags(ea)
            size = ida_bytes.get_item_size(ea)
            name = ida_name.get_name(ea)
            if name and ea != fea and (ida_name.is_public_name(ea) or flags & ida_bytes.FF_NAME or flags & ida_bytes.FF_LABL):
                lines.append("%s:" % name)
            raw = ida_bytes.get_bytes(ea, min(size, 16)) or b""
            text = idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)
            spd = idc.get_spd(ea)
            refs = []
            for xr in idautils.XrefsFrom(ea, 0):
                if xr.type == ida_xref.fl_F:
                    continue
                refs.append("%s" % hx(xr.to))
            lines.append("%s %-32s sp=%-5s %s%s" % (
                hx(ea), raw.hex(), "" if spd is None else str(spd), text,
                ("    ; -> " + ",".join(refs)) if refs else ""))
            ea += max(1, size)
    return "\n".join(lines) + "\n"


def main():
    ida_auto.auto_wait()
    have_hr = ida_hexrays.init_hexrays_plugin()
    count = 0
    for fea in idautils.Functions():
        with open(os.path.join(ASM_DIR, hx(fea) + ".asm"), "w", encoding="utf-8", newline="\n") as fh:
            fh.write(asm_listing(fea))
        if have_hr:
            try:
                cf = ida_hexrays.decompile(fea)
                text = str(cf) if cf else "// decompile returned None\n"
            except Exception as exc:
                text = "// decompile failed: %s\n" % exc
                ERRORS.append("%s %s" % (hx(fea), exc))
            with open(os.path.join(DEC_DIR, hx(fea) + ".c"), "w", encoding="utf-8", newline="\n") as fh:
                fh.write(text)
        count += 1
    with open(os.path.join(OUT_DIR, "export_funcs.log"), "w", encoding="utf-8") as fh:
        fh.write("functions=%d hexrays=%s errors=%d\n" % (count, have_hr, len(ERRORS)))
        fh.write("\n".join(ERRORS) + "\n")
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(os.path.join(OUT_DIR, "export_funcs.log"), "w", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
