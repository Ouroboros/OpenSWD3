import json
import os
import re

import ida_auto
import ida_bytes
import ida_entry
import ida_funcs
import ida_ida
import ida_idaapi
import ida_kernwin
import ida_lines
import ida_loader
import ida_nalt
import ida_name
import ida_segment
import ida_ua
import ida_xref
import idautils
import idc

MODE = idc.ARGV[1] if len(idc.ARGV) > 1 else "export"
OUT_DIR = idc.ARGV[2] if len(idc.ARGV) > 2 else os.path.dirname(ida_loader.get_path(ida_loader.PATH_TYPE_IDB))
os.makedirs(OUT_DIR, exist_ok=True)
LOG_LINES = []


def log(msg):
    LOG_LINES.append(msg)
    ida_kernwin.msg("[analyze] %s\n" % msg)


def fail(msg):
    log("FATAL: " + msg)
    flush_log()
    idc.qexit(2)


def flush_log():
    with open(os.path.join(OUT_DIR, "analyze_%s.log" % MODE), "w", encoding="utf-8") as fh:
        fh.write("\n".join(LOG_LINES) + "\n")


def hx(ea):
    return "0x%08X" % ea


def exec_segments():
    segs = []
    for sea in idautils.Segments():
        seg = ida_segment.getseg(sea)
        if seg.perm & ida_segment.SEGPERM_EXEC or seg.type == ida_segment.SEG_CODE:
            segs.append(seg)
    return segs


def all_segments():
    rows = []
    for sea in idautils.Segments():
        seg = ida_segment.getseg(sea)
        rows.append({
            "name": ida_segment.get_segm_name(seg),
            "start": hx(seg.start_ea),
            "end": hx(seg.end_ea),
            "perm": seg.perm,
            "type": seg.type,
            "bitness": seg.bitness,
            "exec": bool(seg.perm & ida_segment.SEGPERM_EXEC or seg.type == ida_segment.SEG_CODE),
        })
    return rows


def unowned_heads():
    heads = []
    for seg in exec_segments():
        for ea in idautils.Heads(seg.start_ea, seg.end_ea):
            if ida_bytes.is_code(ida_bytes.get_full_flags(ea)) and ida_funcs.get_func(ea) is None:
                heads.append(ea)
    return heads


def group_regions(heads):
    regions = []
    for ea in heads:
        size = ida_bytes.get_item_size(ea)
        if regions and regions[-1][1] == ea:
            regions[-1][1] = ea + size
            regions[-1][2] += 1
        else:
            regions.append([ea, ea + size, 1])
    return regions


def xrefs_to_desc(ea):
    out = []
    for xr in idautils.XrefsTo(ea, 0):
        if xr.type in (ida_xref.fl_F,):
            continue
        out.append("%s:%s" % (hx(xr.frm), idautils.XrefTypeName(xr.type)))
    return out


def func_flags_desc(fn):
    parts = []
    for name, bit in (("LIB", ida_funcs.FUNC_LIB), ("THUNK", ida_funcs.FUNC_THUNK),
                      ("NORET", ida_funcs.FUNC_NORET), ("FRAME", ida_funcs.FUNC_FRAME),
                      ("HIDDEN", ida_funcs.FUNC_HIDDEN), ("STATICDEF", ida_funcs.FUNC_STATICDEF)):
        if fn.flags & bit:
            parts.append(name)
    return "|".join(parts)


def import_map():
    imports = []
    for i in range(ida_nalt.get_import_module_qty()):
        mod = ida_nalt.get_import_module_name(i)

        def cb(ea, name, ordinal, mod=mod):
            imports.append({"module": mod, "ea": hx(ea), "name": name or "", "ordinal": ordinal})
            return True

        ida_nalt.enum_import_names(i, cb)
    return imports


def has_switch(ea):
    si = ida_nalt.switch_info_t()
    res = ida_nalt.get_switch_info(si, ea)
    return bool(res) and res is not None and (not isinstance(res, int) or res > 0)


def export_all(tag):
    import_list = import_map()
    import_eas = {int(x["ea"], 16) for x in import_list}

    af = ida_ida.inf_get_af()
    funcs = list(idautils.Functions())
    libname_re = re.compile(r"^unknown_libname_[0-9]+$")
    bad_names = []
    lib_funcs = []
    with open(os.path.join(OUT_DIR, "functions_raw.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("start\tend\tname\tflags\tsize\tchunks\tuser_named\tcallers\tcallees\n")
        for ea in funcs:
            fn = ida_funcs.get_func(ea)
            name = ida_funcs.get_func_name(ea)
            if libname_re.match(name):
                bad_names.append((ea, name))
            if fn.flags & ida_funcs.FUNC_LIB:
                lib_funcs.append((ea, name))
            chunks = ["%s-%s" % (hx(s), hx(e)) for (s, e) in idautils.Chunks(ea)]
            size = sum(e - s for (s, e) in idautils.Chunks(ea))
            callers = set()
            for xr in idautils.CodeRefsTo(ea, 0):
                cf = ida_funcs.get_func(xr)
                callers.add(cf.start_ea if cf else xr)
            callees = set()
            for item in idautils.FuncItems(ea):
                for tgt in idautils.CodeRefsFrom(item, 0):
                    tf = ida_funcs.get_func(tgt)
                    if tf and tf.start_ea == tgt and tgt != ea:
                        callees.add(tgt)
                    elif tgt in import_eas:
                        callees.add(tgt)
                for tgt in idautils.DataRefsFrom(item):
                    if tgt in import_eas:
                        callees.add(tgt)
            user_named = ida_name.is_public_name(ea) or (ida_bytes.get_full_flags(ea) & ida_bytes.FF_NAME and not ida_name.get_name(ea).startswith(("sub_", "nullsub_", "j_", "unknown_")))
            fh.write("%s\t%s\t%s\t%s\t%d\t%s\t%d\t%s\t%s\n" % (
                hx(fn.start_ea), hx(fn.end_ea), name, func_flags_desc(fn), size, ";".join(chunks),
                1 if user_named else 0,
                ",".join(hx(c) for c in sorted(callers)),
                ",".join(hx(c) for c in sorted(callees))))

    heads = unowned_heads()
    regions = group_regions(heads)
    with open(os.path.join(OUT_DIR, "unowned_heads.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("ea\tsize\tdisasm\txrefs_to\n")
        for ea in heads:
            fh.write("%s\t%d\t%s\t%s\n" % (hx(ea), ida_bytes.get_item_size(ea),
                                         idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS),
                                         ",".join(xrefs_to_desc(ea))))
    with open(os.path.join(OUT_DIR, "unowned_regions.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("start\tend\theads\tfirst_disasm\tlast_disasm\txrefs_to_start\tprev_func\tnext_func\n")
        for (s, e, n) in regions:
            last = idc.prev_head(e, s)
            pf = ida_funcs.get_prev_func(s)
            nf = ida_funcs.get_next_func(s)
            fh.write("%s\t%s\t%d\t%s\t%s\t%s\t%s\t%s\n" % (
                hx(s), hx(e), n,
                idc.generate_disasm_line(s, ida_lines.GENDSM_REMOVE_TAGS),
                idc.generate_disasm_line(last, ida_lines.GENDSM_REMOVE_TAGS),
                ",".join(xrefs_to_desc(s)),
                hx(pf.start_ea) if pf else "",
                hx(nf.start_ea) if nf else ""))

    indirect = []
    for seg in exec_segments():
        for ea in idautils.Heads(seg.start_ea, seg.end_ea):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(ea)):
                continue
            insn = ida_ua.insn_t()
            if ida_ua.decode_insn(insn, ea) == 0:
                continue
            mnem = insn.get_canon_mnem()
            if mnem not in ("call", "jmp", "callni", "jmpni", "callfi", "jmpfi"):
                continue
            op = insn.ops[0]
            if op.type in (ida_ua.o_near, ida_ua.o_far):
                continue
            if op.type == ida_ua.o_mem and op.addr in import_eas:
                continue
            kind = "call" if mnem.startswith("call") else "jmp"
            sw = has_switch(ea) if kind == "jmp" else False
            fn = ida_funcs.get_func(ea)
            indirect.append((ea, kind, op.type, "switch" if sw else "",
                             hx(fn.start_ea) if fn else "",
                             idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS),
                             ",".join(hx(t) for t in idautils.CodeRefsFrom(ea, 0))))
    with open(os.path.join(OUT_DIR, "indirect_sites.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("ea\tkind\toptype\tswitch\tfunc\tdisasm\tresolved_targets\n")
        for row in indirect:
            fh.write("%s\t%s\t%d\t%s\t%s\t%s\t%s\n" % (hx(row[0]), row[1], row[2], row[3], row[4], row[5], row[6]))

    code_drefs = []
    for seg in exec_segments():
        for ea in idautils.Heads(seg.start_ea, seg.end_ea):
            flags = ida_bytes.get_full_flags(ea)
            if not ida_bytes.is_code(flags):
                continue
            refs = [xr for xr in idautils.XrefsTo(ea, 0) if xr.type in (ida_xref.dr_O, ida_xref.dr_R, ida_xref.dr_W, ida_xref.dr_I)]
            if not refs:
                continue
            fn = ida_funcs.get_func(ea)
            role = "func_start" if fn and fn.start_ea == ea else ("inside_func" if fn else "unowned")
            code_drefs.append((ea, role, hx(fn.start_ea) if fn else "",
                               ",".join("%s:%s" % (hx(x.frm), idautils.XrefTypeName(x.type)) for x in refs)))
    with open(os.path.join(OUT_DIR, "code_drefs.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("target\trole\tfunc\tdata_xrefs\n")
        for row in code_drefs:
            fh.write("%s\t%s\t%s\t%s\n" % (hx(row[0]), row[1], row[2], row[3]))

    gaps = []
    for seg in exec_segments():
        owned = []
        for fea in funcs:
            for (s, e) in idautils.Chunks(fea):
                if s >= seg.start_ea and e <= seg.end_ea:
                    owned.append((s, e))
        owned.sort()
        cur = seg.start_ea
        for (s, e) in owned:
            if s > cur:
                gaps.append((cur, s))
            cur = max(cur, e)
        if cur < seg.end_ea:
            gaps.append((cur, seg.end_ea))
    with open(os.path.join(OUT_DIR, "nonfunc_gaps.tsv"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("start\tend\tsize\tcode_heads\tdata_heads\tunknown_bytes\tall_align_or_pad\n")
        for (s, e) in gaps:
            code_n = data_n = unk = 0
            pad = True
            ea = s
            while ea < e:
                flags = ida_bytes.get_full_flags(ea)
                if ida_bytes.is_code(flags):
                    code_n += 1
                    mnem = idc.print_insn_mnem(ea)
                    if mnem not in ("nop", "int3", "align") and not (mnem == "lea" and idc.print_operand(ea, 0) == idc.print_operand(ea, 1).strip("[]")):
                        pad = False
                    ea += max(1, ida_bytes.get_item_size(ea))
                elif ida_bytes.is_data(flags):
                    data_n += 1
                    if not ida_bytes.is_align(flags):
                        pad = False
                    ea += max(1, ida_bytes.get_item_size(ea))
                else:
                    b = ida_bytes.get_byte(ea)
                    unk += 1
                    if b not in (0x00, 0x90, 0xCC):
                        pad = False
                    ea += 1
            fh.write("%s\t%s\t%d\t%d\t%d\t%d\t%d\n" % (hx(s), hx(e), e - s, code_n, data_n, unk, 1 if pad else 0))

    entries = []
    for i in range(ida_entry.get_entry_qty()):
        ordn = ida_entry.get_entry_ordinal(i)
        entries.append({"ordinal": ordn, "ea": hx(ida_entry.get_entry(ordn)), "name": ida_entry.get_entry_name(ordn)})

    sha = ida_nalt.retrieve_input_file_sha256()
    md5 = ida_nalt.retrieve_input_file_md5()
    meta = {
        "tag": tag,
        "idb_path": ida_loader.get_path(ida_loader.PATH_TYPE_IDB),
        "input_path": ida_nalt.get_input_file_path(),
        "input_sha256": sha.hex() if sha else "",
        "input_md5": md5.hex() if md5 else "",
        "input_size": ida_nalt.retrieve_input_file_size(),
        "procname": ida_ida.inf_get_procname(),
        "is_64bit": ida_ida.inf_is_64bit(),
        "is_32bit": ida_ida.inf_is_32bit_exactly(),
        "filetype": ida_ida.inf_get_filetype(),
        "imagebase": hx(ida_nalt.get_imagebase()),
        "min_ea": hx(ida_ida.inf_get_min_ea()),
        "max_ea": hx(ida_ida.inf_get_max_ea()),
        "start_ea": hx(ida_ida.inf_get_start_ea()),
        "af": "0x%08X" % af,
        "af2": "0x%08X" % ida_ida.inf_get_af2(),
        "af_flirt_on": bool(af & ida_ida.AF_FLIRT),
        "compiler_id": ida_ida.inf_get_cc_id(),
        "function_count": len(funcs),
        "unknown_libname_count": len(bad_names),
        "func_lib": [{"ea": hx(e), "name": n} for (e, n) in lib_funcs],
        "unowned_code_heads": len(heads),
        "unowned_code_regions": len(regions),
        "indirect_sites": len(indirect),
        "indirect_sites_switch": sum(1 for r in indirect if r[3]),
        "nonfunc_gaps": len(gaps),
        "segments": all_segments(),
        "entries": entries,
        "imports": import_list,
        "patched_bytes": count_patches(),
    }
    with open(os.path.join(OUT_DIR, "meta_%s.json" % tag), "w", encoding="utf-8") as fh:
        json.dump(meta, fh, indent=1)
    log("functions=%d unknown_libname=%d func_lib=%d unowned_heads=%d regions=%d af=%s" % (
        len(funcs), len(bad_names), len(lib_funcs), len(heads), len(regions), meta["af"]))
    return meta


def count_patches():
    patches = []

    def cb(ea, fpos, org, val):
        patches.append(ea)
        return 0

    ida_bytes.visit_patched_bytes(0, ida_idaapi.BADADDR, cb)
    return len(patches)


def main():
    if MODE == "analyze":
        before = ida_ida.inf_get_af()
        ida_ida.inf_set_af(before & ~ida_ida.AF_FLIRT)
        after = ida_ida.inf_get_af()
        log("AF before=0x%08X after=0x%08X" % (before, after))
        if after & ida_ida.AF_FLIRT:
            fail("AF_FLIRT still set")
        ida_auto.auto_wait()
        if ida_ida.inf_get_af() & ida_ida.AF_FLIRT:
            fail("AF_FLIRT set after auto_wait")
        meta = export_all("analyze")
        if meta["unknown_libname_count"]:
            fail("unknown_libname names present")
        ida_loader.save_database(ida_loader.get_path(ida_loader.PATH_TYPE_IDB), 0)
        log("saved %s" % ida_loader.get_path(ida_loader.PATH_TYPE_IDB))
    elif MODE == "reopen":
        ida_auto.auto_wait()
        meta = export_all("reopen")
        if meta["af_flirt_on"]:
            fail("AF_FLIRT on in reopened database")
        if meta["unknown_libname_count"]:
            fail("unknown_libname names present")
        start = ida_ida.inf_get_start_ea()
        lines = []
        ea = start
        for _ in range(12):
            lines.append("%s  %s" % (hx(ea), idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)))
            ea = idc.next_head(ea, ida_idaapi.BADADDR)
        with open(os.path.join(OUT_DIR, "reopen_entry_disasm.txt"), "w", encoding="utf-8") as fh:
            fh.write("\n".join(lines) + "\n")
        log("entry disasm lines=%d" % len(lines))
    else:
        ida_auto.auto_wait()
        export_all(MODE)
    flush_log()
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    log(traceback.format_exc())
    flush_log()
    idc.qexit(3)
