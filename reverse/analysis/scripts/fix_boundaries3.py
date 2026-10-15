import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_ida
import ida_loader
import ida_offset
import idautils
import idc

OUT_DIR = idc.ARGV[1]
LOG = []

DIK_START = 0x00486AC0
DIK_END = 0x00487AC0
DIK_HDR_END = 0x00487AD8
FALSE_FUNCS = [0x004876E4, 0x004876F4, 0x00487764, 0x00487774]


def hx(ea):
    return "0x%08X" % ea


def check(cond, msg):
    LOG.append(("OK   " if cond else "FAIL ") + msg)
    return cond


def main():
    ida_auto.auto_wait()
    if ida_ida.inf_get_af() & ida_ida.AF_FLIRT:
        LOG.append("FAIL AF_FLIRT on")
        finish(2)
    for ea in FALSE_FUNCS:
        fn = ida_funcs.get_func(ea)
        if fn and fn.start_ea == ea:
            ida_funcs.del_func(ea)
            LOG.append("del_func %s (byte inside c_dfDIKeyboard DIOBJECTDATAFORMAT table)" % hx(ea))
    ida_bytes.del_items(DIK_START, ida_bytes.DELIT_SIMPLE, DIK_HDR_END - DIK_START)
    ea = DIK_START
    while ea < DIK_HDR_END:
        ida_bytes.create_dword(ea, 4)
        ea += 4
    ea = DIK_START
    while ea < DIK_END:
        ida_offset.op_plain_offset(ea, 0, 0)
        ea += 16
    ida_offset.op_plain_offset(DIK_END + 20, 0, 0)
    idc.set_cmt(DIK_START, "c_dfDIKeyboard rgodf: 256 x DIOBJECTDATAFORMAT {pguid, dwOfs, dwType, dwFlags}", 0)
    idc.set_cmt(DIK_END, "c_dfDIKeyboard DIDATAFORMAT {dwSize, dwObjSize, dwFlags, dwDataSize, dwNumObjs, rgodf}", 0)
    ida_auto.auto_wait()
    code_in = 0
    funcs_in = 0
    ea = DIK_START
    while ea < DIK_HDR_END:
        if ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
            code_in += 1
        if ida_funcs.get_func(ea):
            funcs_in += 1
        ea += 1
    check(code_in == 0 and funcs_in == 0, "c_dfDIKeyboard %s-%s is data only (code bytes=%d func bytes=%d)" % (hx(DIK_START), hx(DIK_HDR_END), code_in, funcs_in))
    LOG.append("functions %d" % sum(1 for _ in idautils.Functions()))
    ida_loader.save_database(ida_loader.get_path(ida_loader.PATH_TYPE_IDB), 0)
    LOG.append("saved")
    finish(0 if not any(l.startswith("FAIL") for l in LOG) else 4)


def finish(code):
    with open(os.path.join(OUT_DIR, "fix_boundaries3.log"), "w", encoding="utf-8") as fh:
        fh.write("\n".join(LOG) + "\n")
    idc.qexit(code)


try:
    main()
except Exception:
    import traceback
    LOG.append("FAIL " + traceback.format_exc())
    finish(3)
