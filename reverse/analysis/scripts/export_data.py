import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_name
import ida_nalt
import ida_offset
import ida_segment
import idautils
import idc

OUT_DIR = idc.ARGV[1]
IMAGE_START = 0x00499000
IMAGE_END = 0x0053F000
CODE_START = 0x00401000
CODE_END = 0x00499000


def hx(ea):
    return "0x%08X" % ea


def kind_of(flags):
    if ida_bytes.is_strlit(flags):
        return "str"
    if ida_bytes.is_align(flags):
        return "align"
    if ida_bytes.is_struct(flags):
        return "struct"
    if ida_bytes.is_float(flags):
        return "float"
    if ida_bytes.is_double(flags):
        return "double"
    if ida_bytes.is_byte(flags):
        return "byte"
    if ida_bytes.is_word(flags):
        return "word"
    if ida_bytes.is_dword(flags):
        return "dword"
    if ida_bytes.is_qword(flags):
        return "qword"
    if ida_bytes.is_tbyte(flags):
        return "tbyte"
    if ida_bytes.is_data(flags):
        return "data"
    return "unknown"


def main():
    ida_auto.auto_wait()
    items = open(os.path.join(OUT_DIR, "data_items.tsv"), "w", encoding="utf-8", newline="\n")
    items.write("addr\tsize\tkind\tname\telem_size\toffset\txrefs_code\txrefs_data\n")
    relocs = open(os.path.join(OUT_DIR, "data_relocs.tsv"), "w", encoding="utf-8", newline="\n")
    relocs.write("slot\ttarget\ttarget_kind\ttarget_func\tslot_item\n")
    ea = IMAGE_START
    while ea < IMAGE_END:
        flags = ida_bytes.get_full_flags(ea)
        if not ida_bytes.is_head(flags):
            ea = idc.next_head(ea, IMAGE_END)
            if ea == idc.BADADDR:
                break
            continue
        size = ida_bytes.get_item_size(ea)
        name = ida_name.get_name(ea) or ""
        kind = kind_of(flags)
        elem = 0
        if ida_bytes.is_data(flags) and not ida_bytes.is_strlit(flags) and not ida_bytes.is_struct(flags):
            elem = ida_bytes.get_data_elsize(ea, flags)
        is_off = ida_bytes.is_off0(flags)
        code_x = 0
        data_x = 0
        for xr in idautils.XrefsTo(ea, 0):
            f = ida_bytes.get_full_flags(xr.frm)
            if ida_bytes.is_code(f):
                code_x += 1
            else:
                data_x += 1
        items.write("%s\t%d\t%s\t%s\t%d\t%d\t%d\t%d\n" % (hx(ea), size, kind, name, elem, 1 if is_off else 0, code_x, data_x))
        if is_off and elem == 4:
            n = size // 4
            for i in range(n):
                slot = ea + 4 * i
                val = ida_bytes.get_dword(slot)
                if CODE_START <= val < CODE_END:
                    fn = ida_funcs.get_func(val)
                    tk = "func" if fn and fn.start_ea == val else "code"
                    relocs.write("%s\t%s\t%s\t%s\t%s\n" % (hx(slot), hx(val), tk, hx(fn.start_ea) if fn else "", name))
                elif IMAGE_START <= val < IMAGE_END:
                    relocs.write("%s\t%s\tdata\t\t%s\n" % (hx(slot), hx(val), name))
        ea += max(1, size)
    items.close()
    relocs.close()
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(os.path.join(OUT_DIR, "export_data.err"), "w", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
