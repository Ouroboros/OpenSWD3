import os

import ida_auto
import ida_bytes
import idautils
import idc

OUT_DIR = idc.ARGV[1]
IMAGE_START = 0x00499000
IMAGE_END = 0x0053F000


def kind_of(flags):
    if not ida_bytes.is_data(flags):
        return "undef"
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
    if ida_bytes.is_tbyte(flags):
        return "tbyte"
    if ida_bytes.is_qword(flags):
        return "qword"
    if ida_bytes.is_dword(flags):
        return "dword"
    if ida_bytes.is_word(flags):
        return "word"
    if ida_bytes.is_byte(flags):
        return "byte"
    return "data"


def main():
    ida_auto.auto_wait()
    with open(os.path.join(OUT_DIR, "data_names.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("addr\tname\tkind\tsize\telem\n")
        for ea, name in idautils.Names():
            if not (IMAGE_START <= ea < IMAGE_END):
                continue
            flags = ida_bytes.get_full_flags(ea)
            kind = kind_of(flags)
            if kind == "undef":
                size, elem = 1, 1
            else:
                size = ida_bytes.get_item_size(ea)
                elem = size if kind in ("str", "struct", "align") else ida_bytes.get_data_elsize(ea, flags)
            fh.write("0x%08X\t%s\t%s\t%d\t%d\n" % (ea, name, kind, size, elem))
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    with open(os.path.join(OUT_DIR, "export_names.err"), "w", encoding="utf-8") as fh:
        fh.write(traceback.format_exc())
    idc.qexit(3)
