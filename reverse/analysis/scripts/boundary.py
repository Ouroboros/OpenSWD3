import os

import ida_auto
import ida_bytes
import ida_funcs
import ida_ida
import ida_kernwin
import ida_lines
import ida_loader
import ida_segment
import ida_ua
import idautils
import idc

OUT_DIR = idc.ARGV[1] if len(idc.ARGV) > 1 else os.path.dirname(ida_loader.get_path(ida_loader.PATH_TYPE_IDB))
PASS = idc.ARGV[2] if len(idc.ARGV) > 2 else "1"
EXTRA = [int(x, 16) for x in idc.ARGV[3].split(",")] if len(idc.ARGV) > 3 and idc.ARGV[3] else []
ROWS = []
LOG = []


def hx(ea):
    return "0x%08X" % ea


def dis(ea):
    return idc.generate_disasm_line(ea, ida_lines.GENDSM_REMOVE_TAGS)


def exec_segments():
    for sea in idautils.Segments():
        seg = ida_segment.getseg(sea)
        if seg.perm & ida_segment.SEGPERM_EXEC or seg.type == ida_segment.SEG_CODE:
            yield seg


def unowned_heads():
    out = []
    for seg in exec_segments():
        for ea in idautils.Heads(seg.start_ea, seg.end_ea):
            if ida_bytes.is_code(ida_bytes.get_full_flags(ea)) and ida_funcs.get_func(ea) is None:
                out.append(ea)
    return out


def regions(heads):
    regs = []
    for ea in heads:
        size = ida_bytes.get_item_size(ea)
        if regs and regs[-1][1] == ea:
            regs[-1][1] = ea + size
        else:
            regs.append([ea, ea + size])
    return regs


def is_pad_before(ea):
    prev = idc.prev_head(ea, 0)
    if prev == idc.BADADDR:
        return True
    flags = ida_bytes.get_full_flags(prev)
    if ida_bytes.is_align(flags):
        return True
    if ida_bytes.is_code(flags):
        mnem = idc.print_insn_mnem(prev)
        return mnem in ("retn", "ret", "jmp", "int3", "nop") and not ida_bytes.is_flow(ida_bytes.get_full_flags(ea))
    if ida_bytes.is_unknown(flags) or ida_bytes.is_data(flags):
        return True
    return False


def try_create(start, reason):
    flags = ida_bytes.get_full_flags(start)
    row = {"start": start, "reason": reason, "result": "", "end": 0, "detail": ""}
    if ida_funcs.get_func(start):
        row["result"] = "skip_owned"
        ROWS.append(row)
        return False
    if ida_bytes.is_flow(flags):
        row["result"] = "skip_fallthrough_midblock"
        row["detail"] = dis(idc.prev_head(start, 0))
        ROWS.append(row)
        return False
    if not is_pad_before(start):
        row["result"] = "skip_no_boundary_before"
        row["detail"] = dis(idc.prev_head(start, 0))
        ROWS.append(row)
        return False
    prev_f = ida_funcs.get_prev_func(start)
    next_f = ida_funcs.get_next_func(start)
    next_start = next_f.start_ea if next_f else idc.BADADDR
    ok = ida_funcs.add_func(start, idc.BADADDR)
    fn = ida_funcs.get_func(start)
    if not ok or fn is None or fn.start_ea != start:
        row["result"] = "add_failed"
        ROWS.append(row)
        return False
    problems = []
    if fn.end_ea > next_start:
        problems.append("end_past_next_func")
    chunks = list(idautils.Chunks(start))
    for (cs, ce) in chunks:
        for (ps, pe) in ([(c[0], c[1]) for c in idautils.Chunks(prev_f.start_ea)] if prev_f else []):
            if cs < pe and ps < ce and prev_f.start_ea != start:
                problems.append("overlap_prev")
    last = idc.prev_head(fn.end_ea, start)
    last_m = idc.print_insn_mnem(last)
    if last_m not in ("retn", "ret", "jmp", "int3") and not (idc.get_func_flags(start) & ida_funcs.FUNC_NORET):
        callee_noret = False
        if last_m == "call":
            tgt = idc.get_operand_value(last, 0)
            tf = ida_funcs.get_func(tgt)
            callee_noret = bool(tf and tf.flags & ida_funcs.FUNC_NORET)
        if not callee_noret:
            problems.append("bad_last_insn:" + last_m)
    if problems:
        ida_funcs.del_func(start)
        row["result"] = "rolled_back"
        row["detail"] = ",".join(problems)
        ROWS.append(row)
        return False
    row["result"] = "created"
    row["end"] = fn.end_ea
    row["detail"] = "chunks=%d last=%s" % (len(chunks), dis(last))
    ROWS.append(row)
    return True


def main():
    ida_auto.auto_wait()
    if ida_ida.inf_get_af() & ida_ida.AF_FLIRT:
        LOG.append("FATAL AF_FLIRT on")
        finish(2)
    before_funcs = sum(1 for _ in idautils.Functions())
    before_heads = len(unowned_heads())
    for rnd in range(6):
        made = 0
        cands = [r[0] for r in regions(unowned_heads())]
        if rnd == 0:
            cands = sorted(set(cands) | set(EXTRA))
        for s in cands:
            if any(r["start"] == s and r["result"] == "created" for r in ROWS):
                continue
            if try_create(s, "unowned_region_start_round%d" % rnd):
                made += 1
                ida_auto.auto_wait()
        LOG.append("round %d created %d" % (rnd, made))
        if made == 0:
            break
    ida_auto.auto_wait()
    after_funcs = sum(1 for _ in idautils.Functions())
    after_heads = len(unowned_heads())
    LOG.append("functions %d -> %d, unowned heads %d -> %d" % (before_funcs, after_funcs, before_heads, after_heads))
    ida_loader.save_database(ida_loader.get_path(ida_loader.PATH_TYPE_IDB), 0)
    LOG.append("saved")
    finish(0)


def finish(code):
    with open(os.path.join(OUT_DIR, "boundary_pass%s.tsv" % PASS), "w", encoding="utf-8", newline="\n") as fh:
        fh.write("start\tend\tresult\treason\tdetail\tfirst_disasm\n")
        last_by_start = {}
        for r in ROWS:
            last_by_start[r["start"]] = r
        for s in sorted(last_by_start):
            r = last_by_start[s]
            fh.write("%s\t%s\t%s\t%s\t%s\t%s\n" % (hx(s), hx(r["end"]) if r["end"] else "", r["result"], r["reason"], r["detail"], dis(s)))
    with open(os.path.join(OUT_DIR, "boundary_pass%s.log" % PASS), "w", encoding="utf-8") as fh:
        fh.write("\n".join(LOG) + "\n")
    idc.qexit(code)


try:
    main()
except Exception:
    import traceback
    LOG.append(traceback.format_exc())
    finish(3)
