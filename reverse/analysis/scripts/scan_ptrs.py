import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_lines
import ida_segment
import idautils
import idc

OUT_DIR = idc.ARGV[1]


def hx(ea):
    return "0x%08X" % ea


def main():
    ida_auto.auto_wait()
    text = None
    for sea in idautils.Segments():
        seg = ida_segment.getseg(sea)
        if ida_segment.get_segm_name(seg) == ".text":
            text = seg
    rows = []
    for sea in idautils.Segments():
        seg = ida_segment.getseg(sea)
        name = ida_segment.get_segm_name(seg)
        ea = seg.start_ea
        while ea + 4 <= seg.end_ea:
            flags = ida_bytes.get_full_flags(ea)
            if name == ".text" and ida_bytes.is_code(flags):
                ea = ida_bytes.get_item_end(ea)
                ea = (ea + 3) & ~3
                continue
            if not ida_bytes.is_loaded(ea):
                ea += 4
                continue
            val = ida_bytes.get_dword(ea)
            if text.start_ea <= val < text.end_ea:
                tflags = ida_bytes.get_full_flags(val)
                fn = ida_funcs.get_func(val)
                if fn and fn.start_ea == val:
                    role = "func_start"
                elif fn and ida_bytes.is_code(tflags) and ida_bytes.is_head(tflags):
                    role = "inside_func"
                elif ida_bytes.is_code(tflags) and ida_bytes.is_head(tflags):
                    role = "unowned_code"
                elif ida_bytes.is_code(tflags) or (fn is not None):
                    role = "mid_instruction"
                else:
                    role = "not_code"
                head = idc.get_item_head(ea)
                rows.append((ea, name, val, role, hx(fn.start_ea) if fn else "",
                             idc.get_name(head) or "", "1" if ida_bytes.is_off0(ida_bytes.get_full_flags(head)) else "0",
                             idc.generate_disasm_line(val, ida_lines.GENDSM_REMOVE_TAGS)[:60] if ida_bytes.is_code(tflags) else ""))
            ea += 4
    with open(os.path.join(OUT_DIR, "text_pointer_scan.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("slot\tsegment\ttarget\trole\ttarget_func\tslot_item_name\tslot_is_offset\ttarget_disasm\n")
        for r in rows:
            fh.write("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n" % (hx(r[0]), r[1], hx(r[2]), r[3], r[4], r[5], r[6], r[7]))
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(os.path.join(OUT_DIR, "text_pointer_scan.err"), "w", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
