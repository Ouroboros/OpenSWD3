"""Function fingerprints computed from the listings of export_funcs.py, for matching the functions of two builds.

Usage: python3 -I fingerprints.py <asm dir> <exe> <out.jsonl> <funclets.tsv>

Each instruction is normalised from its machine code:
  - call rel32: the displacement becomes CALL (the callee is listed apart);
  - jmp/jcc: a target inside the function (any of its chunks) becomes the label L<k>, numbered in order of first
    use; a target in another function becomes TAILCALL (listed apart);
  - any little-endian dword of the instruction equal to one of its reference targets is masked by the kind of the
    target: FUNC (a function start), CODE (inside a function: switch tables, labels), IMP (the import address
    table), STR (a string in a data section, its text listed apart) or MEM (other data, listed apart);
  - all other bytes are kept (structure offsets, immediates, stack offsets).
The function hash is the SHA-1 of its normalised instructions in order; a basic block ends at a label and after a
transfer or a return. Exception funclets the listings give as functions of their own (the unwind funclets and the
__ehhandler stub after them) are appended to their parent, ordered by address, as the parent's tail chunks; the
parent is the one function that pushes the stub.
"""

import hashlib
import json
import os
import re
import struct
import sys

LINE = re.compile(r"^([0-9A-F]{8}) ([0-9a-f]*)\s+sp=\S*\s+(.*?)(?:    ; -> ([0-9A-F,]+))?$")
FUNC = re.compile(r"^; function ([0-9A-F]{8}) (\S+)")
CHUNK = re.compile(r"^; chunk ([0-9A-F]{8})-([0-9A-F]{8})")
IMPORT = re.compile(r"ds:(?:__imp_)?([A-Za-z_][A-Za-z_0-9@?$]*)")
TRANSFERS = ("jmp", "ja", "jae", "jb", "jbe", "jc", "jcxz", "jecxz", "je", "jg", "jge", "jl", "jle", "jna", "jnae",
             "jnb", "jnbe", "jnc", "jne", "jng", "jnge", "jnl", "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp",
             "jpe", "jpo", "js", "jz", "loop", "loope", "loopne", "loopnz", "loopz")
ENDS = ("ret", "retn", "retf", "iret", "int3", "hlt")


def read_sections(exe):
    with open(exe, "rb") as fh:
        data = fh.read()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    iat_rva, iat_size = struct.unpack_from("<II", data, pe + 24 + 96 + 12 * 8)  # data directory 12: the IAT
    sections = []
    at = pe + 24 + optional
    for _ in range(count):
        name = data[at:at + 8].rstrip(b"\0").decode("latin-1")
        vsize, rva, rsize, raw = struct.unpack_from("<IIII", data, at + 8)
        sections.append((name, base + rva, base + rva + max(vsize, rsize), raw, rsize))
        at += 40
    return data, sections, (base + iat_rva, base + iat_rva + iat_size)


def section_of(sections, address):
    for name, start, end, raw, rsize in sections:
        if start <= address < end:
            return name, start, raw, rsize
    return None, 0, 0, 0


def string_at(data, sections, address):
    name, start, raw, rsize = section_of(sections, address)
    if name is None or name.startswith(".text") or name == ".rsrc":
        return None
    offset = address - start
    if offset >= rsize:
        return None
    end = data.find(b"\0", raw + offset, raw + rsize)
    if end < 0:
        return None
    text = data[raw + offset:end]
    if len(text) < 2:
        return None
    try:
        decoded = text.decode("cp950")
    except UnicodeDecodeError:
        return None
    if not all(ch.isprintable() or ch in "\r\n\t" for ch in decoded):
        return None
    return decoded


def read_listing(path):
    function = None
    chunks = []
    instructions = []
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            raw = raw.rstrip("\n")
            match = FUNC.match(raw)
            if match:
                function = (int(match.group(1), 16), match.group(2))
                continue
            match = CHUNK.match(raw)
            if match:
                chunks.append((int(match.group(1), 16), int(match.group(2), 16)))
                continue
            if raw.endswith(":") and not raw.startswith(";"):
                instructions.append(("label", raw[:-1]))
                continue
            match = LINE.match(raw)
            if match:
                # IDA also lists references to structure members by their netnode ids (above 32 bits).
                refs = [int(x, 16) for x in match.group(4).split(",")] if match.group(4) else []
                refs = [target for target in refs if target <= 0xFFFFFFFF]
                instructions.append(("insn", int(match.group(1), 16), bytes.fromhex(match.group(2)),
                                     match.group(3), refs))
    return function, chunks, instructions


def mnemonic_of(text):
    return text.split()[0] if text else ""


def is_handler_stub(instructions):
    code = [item for item in instructions if item[0] == "insn"]
    return (len(code) == 2 and code[0][2][:1] == b"\xb8" and code[1][2][:1] == b"\xe9")


def main():
    asm_dir, exe, out_path, funclet_path = sys.argv[1:5]
    data, sections, imports = read_sections(exe)
    listings = {}

    for name in sorted(os.listdir(asm_dir)):
        if name.endswith(".asm"):
            function, chunks, instructions = read_listing(os.path.join(asm_dir, name))
            listings[function[0]] = (function[1], chunks, instructions)

    starts = sorted(listings)
    referenced = {}

    for address, (_, _, instructions) in listings.items():
        for item in instructions:
            if item[0] == "insn":
                for target in item[4]:
                    referenced.setdefault(target, set()).add(address)

    # The exception funclets: a stub (mov eax, offset FuncInfo; jmp) pushed by one function, and the unreferenced
    # functions just before it.
    parent_of = {}
    pending = []

    for address in starts:
        name, chunks, instructions = listings[address]
        users = referenced.get(address, set()) - {address}

        if is_handler_stub(instructions) and len(users) == 1:
            parent = next(iter(users))
            for funclet in pending:
                parent_of[funclet] = parent
            parent_of[address] = parent
            pending = []
        elif not users:
            pending.append(address)
        else:
            pending = []

    owned = {}
    for funclet, parent in parent_of.items():
        owned.setdefault(parent, []).append(funclet)

    with open(funclet_path, "w", encoding="utf-8", newline="\n") as fh:
        fh.write("funclet\tname\tparent\tparent_name\tkind\n")
        for funclet in sorted(parent_of):
            parent = parent_of[funclet]
            kind = "ehhandler" if is_handler_stub(listings[funclet][2]) else "unwind"
            fh.write("0x%08X\t%s\t0x%08X\t%s\t%s\n" % (funclet, listings[funclet][0], parent, listings[parent][0], kind))

    def kind_of(target, inside):
        if imports[0] <= target < imports[1] or section_of(sections, target)[0] == ".idata":
            return "IMP"
        if inside(target):
            return "CODE"
        if target in listings:
            return "FUNC"
        section = section_of(sections, target)[0]
        if section is not None and section.startswith(".text"):
            return "CODE"
        if string_at(data, sections, target) is not None:
            return "STR"
        return "MEM"

    with open(out_path, "w", encoding="utf-8", newline="\n") as out:
        for address in starts:
            if address in parent_of:
                continue

            name, chunks, instructions = listings[address]
            items = list(instructions)
            all_chunks = list(chunks)
            for funclet in sorted(owned.get(address, [])):
                all_chunks += listings[funclet][1]
                items += listings[funclet][2]

            def inside(target):
                return any(start <= target < end for start, end in all_chunks)

            labels = {}
            tokens = []
            mnemonics = []
            blocks = [[]]
            callees = []
            tailcalls = []
            strings = []
            imported = []
            globals_used = []
            edges = 0

            def label(target):
                if target not in labels:
                    labels[target] = "L%d" % len(labels)
                return labels[target]

            for item in items:
                if item[0] == "label":
                    if blocks[-1]:
                        blocks.append([])
                    continue

                _, ea, code, text, refs = item
                mnemonic = mnemonic_of(text)
                mnemonics.append(mnemonic)
                masked = bytearray(code)
                notes = []
                transfer = mnemonic in TRANSFERS

                if code[:1] == b"\xe8" and len(code) == 5:
                    masked[1:5] = b"\0\0\0\0"
                    notes.append("CALL")
                    for target in refs:
                        callees.append("0x%08X" % target)
                elif transfer:
                    targets = [target for target in refs if inside(target)]
                    others = [target for target in refs if not inside(target)]
                    imported_name = IMPORT.search(text)

                    if imported_name and code[:2] == b"\xff\x25":
                        masked[2:6] = b"\0\0\0\0"
                        notes.append("IMP:" + imported_name.group(1))
                        imported.append(imported_name.group(1))
                    elif targets and not others:
                        masked = bytearray(code[:1] if code[:1] != b"\x0f" else code[:2])
                        notes.append(label(targets[0]))
                        edges += 1
                    elif others and len(code) in (2, 5) and code[:1] in (b"\xe9", b"\xeb"):
                        masked = bytearray(code[:1])
                        notes.append("TAILCALL")
                        tailcalls += ["0x%08X" % target for target in others]
                    else:
                        edges += 1

                for target in refs:
                    if code[:1] == b"\xe8" or (transfer and notes and notes[-1].startswith(("L", "TAILCALL"))):
                        continue
                    packed = struct.pack("<I", target)
                    at = bytes(masked).find(packed)
                    if at < 0:
                        continue
                    kind = kind_of(target, inside)
                    masked[at:at + 4] = kind.encode().ljust(4, b"_")
                    notes.append(kind)
                    if kind == "STR":
                        strings.append(string_at(data, sections, target))
                    elif kind == "MEM":
                        globals_used.append("0x%08X" % target)
                    elif kind == "IMP":
                        imported_name = IMPORT.search(text)
                        imported.append(imported_name.group(1) if imported_name else "0x%08X" % target)
                    elif kind == "FUNC":
                        callees.append("0x%08X" % target)

                token = bytes(masked).hex() + ("|" + ",".join(notes) if notes else "")
                tokens.append(token)
                blocks[-1].append(token)

                if transfer or mnemonic in ENDS:
                    blocks.append([])

            blocks = [block for block in blocks if block]
            record = {
                "address": "0x%08X" % address,
                "name": name,
                "funclets": ["0x%08X" % funclet for funclet in sorted(owned.get(address, []))],
                "hash": hashlib.sha1("\n".join(tokens).encode()).hexdigest(),
                "blocks": [hashlib.sha1("\n".join(block).encode()).hexdigest() for block in blocks],
                "mnemonics": hashlib.sha1(" ".join(mnemonics).encode()).hexdigest(),
                "instructions": len(tokens),
                "block_count": len(blocks),
                "edges": edges,
                "callees": callees,
                "tailcalls": tailcalls,
                "strings": strings,
                "imports": imported,
                "globals": globals_used,
            }
            out.write(json.dumps(record, ensure_ascii=False) + "\n")


main()
