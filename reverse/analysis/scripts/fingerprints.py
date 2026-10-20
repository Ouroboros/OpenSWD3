"""Function fingerprints computed from the listings of export_funcs.py, for matching the functions of two builds.

Usage: python3 -I fingerprints.py <asm dir> <exe> <out.jsonl> <funclets.tsv>

Each instruction is normalised from its machine code:
  1. call rel32: the displacement becomes CALL; the callee is listed apart, followed through import stubs (a function
     that only jumps on) to DLL!name.
  2. jmp/jcc: a target inside the function (any of its chunks) becomes the label L<k>, numbered in order of first
     use; a target in another function becomes TAILCALL (listed apart).
  3. A little-endian dword of the instruction equal to one of its reference targets is masked by the kind of the
     target: a label L<k> (inside the function), FUNC (a function start), CODE (other code), IMP (the import address
     table), STR (a string in a data section, its text listed apart) or MEM (other data, listed apart; constants of
     .rdata with their 8 bytes).
  3b. Every remaining dword whose value lies in a loaded section of the program is masked by its section and marked
     NOREF (IDA gives no reference for some indexed and based operands and some pushed immediates).
  4. All other bytes are kept (structure offsets, immediates, stack offsets).
  5. A jump through a table adds the labels of the table's entries, in table order.
The function hash is the SHA-1 of its normalised instructions in order; a basic block ends at a label and after a
transfer or a return. Exception funclets the listings give as functions of their own (the unwind funclets and the
__ehhandler stub after them) are appended to their parent, ordered by address, as the parent's tail chunks; the
parent is the one function that refers to the stub.
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
TRANSFERS = {"jmp", "ja", "jae", "jb", "jbe", "jc", "jcxz", "jecxz", "je", "jg", "jge", "jl", "jle", "jna", "jnae",
             "jnb", "jnbe", "jnc", "jne", "jng", "jnge", "jnl", "jnle", "jno", "jnp", "jns", "jnz", "jo", "jp",
             "jpe", "jpo", "js", "jz", "loop", "loope", "loopne", "loopnz", "loopz"}
ENDS = {"ret", "retn", "retf", "iret", "int3", "hlt"}
# The shape of an instruction: its mnemonic and operands with every word but a register or a size made N (numbers,
# and names whatever a database calls them), so that a build whose structures or arrays changed size (other field
# offsets, other counts) still gives the same shape.
SHAPE_WORD = re.compile(r"[A-Za-z_?$@.][A-Za-z_0-9?$@.]*|[0-9][0-9A-Fa-f]*h?")
SHAPE_KEPT = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp", "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
    "al", "ah", "bl", "bh", "cl", "ch", "dl", "dh", "cs", "ds", "es", "fs", "gs", "ss", "st", "byte", "word",
    "dword", "qword", "tbyte", "ptr", "large",
}


def shape_of(text):
    text = text.split(";", 1)[0].strip()
    mnemonic, _, operands = text.partition(" ")
    operands = SHAPE_WORD.sub(lambda match: match.group(0) if match.group(0) in SHAPE_KEPT else "N", operands)
    return (mnemonic + " " + re.sub(r"\s+", "", operands)).strip()


class Image:
    def __init__(self, path):
        with open(path, "rb") as fh:
            self.data = fh.read()
        data = self.data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        directories = pe + 24 + 96
        import_rva = struct.unpack_from("<I", data, directories + 1 * 8)[0]
        iat_rva, iat_size = struct.unpack_from("<II", data, directories + 12 * 8)
        self.iat = (self.base + iat_rva, self.base + iat_rva + iat_size)
        self.sections = []
        at = pe + 24 + optional
        for _ in range(count):
            name = data[at:at + 8].rstrip(b"\0").decode("latin-1")
            vsize, rva, rsize, raw = struct.unpack_from("<IIII", data, at + 8)
            self.sections.append((name, self.base + rva, self.base + rva + max(vsize, rsize), raw, rsize))
            at += 40
        self.imports = self.read_imports(import_rva)

    def offset_of(self, address):
        for name, start, end, raw, rsize in self.sections:
            if start <= address < end and address - start < rsize:
                return raw + address - start
        return None

    def section_of(self, address):
        for name, start, end, raw, rsize in self.sections:
            if start <= address < end:
                return name
        return None

    def loaded(self, value):
        return any(start <= value < end for _, start, end, _, _ in self.sections)

    def dword(self, address):
        at = self.offset_of(address)
        return None if at is None or at + 4 > len(self.data) else struct.unpack_from("<I", self.data, at)[0]

    def cstring(self, address):
        at = self.offset_of(address)
        if at is None:
            return None
        end = self.data.find(b"\0", at)
        return None if end < 0 else self.data[at:end]

    def read_imports(self, import_rva):
        """The IAT slots by address: DLL!name (DLL!#ordinal by ordinal)."""
        slots = {}
        descriptor = self.base + import_rva
        while True:
            at = self.offset_of(descriptor)
            if at is None:
                break
            lookup, _, _, name_rva, first_thunk = struct.unpack_from("<IIIII", self.data, at)
            if name_rva == 0:
                break
            dll = (self.cstring(self.base + name_rva) or b"?").decode("latin-1")
            names = lookup or first_thunk
            index = 0
            while True:
                entry = self.dword(self.base + names + 4 * index)
                if not entry:
                    break
                if entry & 0x80000000:
                    label = "%s!#%d" % (dll, entry & 0xFFFF)
                else:
                    label = "%s!%s" % (dll, (self.cstring(self.base + entry + 2) or b"?").decode("latin-1"))
                slots[self.base + first_thunk + 4 * index] = label
                index += 1
            descriptor += 20
        return slots

    def string_at(self, address):
        section = self.section_of(address)
        if section is None or section.startswith(".text") or section == ".rsrc":
            return None
        text = self.cstring(address)
        if text is None or len(text) < 2:
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


def code_of(instructions):
    return [item for item in instructions if item[0] == "insn"]


def mnemonic_of(text):
    return text.split()[0] if text else ""


def is_handler_stub(instructions):
    code = code_of(instructions)
    return len(code) == 2 and code[0][2][:1] == b"\xb8" and code[1][2][:1] == b"\xe9"


def main():
    asm_dir, exe, out_path, funclet_path = sys.argv[1:5]
    image = Image(exe)
    listings = {}

    for name in sorted(os.listdir(asm_dir)):
        if name.endswith(".asm"):
            function, chunks, instructions = read_listing(os.path.join(asm_dir, name))
            listings[function[0]] = (function[1], chunks, instructions)

    starts = sorted(listings)
    referenced = {}

    for address, (_, _, instructions) in listings.items():
        for item in code_of(instructions):
            for target in item[4]:
                referenced.setdefault(target, set()).add(address)

    # The exception funclets: a stub (mov eax, offset FuncInfo; jmp) referred to by one function, and the
    # unreferenced functions just before it.
    parent_of = {}
    pending = []

    for address in starts:
        _, _, instructions = listings[address]
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

    def import_through(target, seen=()):
        """DLL!name of an IAT slot, or of a function that only jumps on to one (an import stub)."""
        if target in image.imports:
            return image.imports[target]
        if target in listings and target not in seen:
            code = code_of(listings[target][2])
            if len(code) == 1 and mnemonic_of(code[0][3]) == "jmp":
                for next_target in code[0][4]:
                    found = import_through(next_target, seen + (target,))
                    if found:
                        return found
        return None

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
            shapes = []
            texts = []
            blocks = [[]]
            shape_blocks = [[]]
            record = {"callees": [], "tailcalls": [], "strings": [], "imports": [], "globals": [], "constants": [],
                      "noref": []}
            edges = 0

            def label(target):
                if target not in labels:
                    labels[target] = "L%d" % len(labels)
                return labels[target]

            def classify(target):
                """The kind of a reference target and what it adds to the record."""
                if image.iat[0] <= target < image.iat[1] or target in image.imports:
                    record["imports"].append(image.imports.get(target, "0x%08X" % target))
                    return "IMP"
                if inside(target):
                    return label(target)
                if target in listings:
                    imported = import_through(target)
                    record["callees"].append(imported or "0x%08X" % target)
                    return "FUNC"
                section = image.section_of(target)
                if section is not None and section.startswith(".text"):
                    return "CODE"
                text = image.string_at(target)
                if text is not None:
                    record["strings"].append(text)
                    return "STR"
                record["globals"].append("0x%08X" % target)
                if section == ".rdata":
                    at = image.offset_of(target)
                    if at is not None:
                        record["constants"].append(image.data[at:at + 8].hex())
                return "MEM"

            for item in items:
                if item[0] == "label":
                    if blocks[-1]:
                        blocks.append([])
                        shape_blocks.append([])
                    continue

                _, ea, code, text, refs = item
                mnemonic = mnemonic_of(text)
                mnemonics.append(mnemonic)
                masked = bytearray(code)
                notes = []
                transfer = mnemonic in TRANSFERS
                handled = set()

                if code[:1] == b"\xe8" and len(code) == 5:
                    masked[1:5] = b"CALL"
                    notes.append("CALL")
                    for target in refs:
                        if inside(target):
                            notes.append(label(target))
                        else:
                            imported = import_through(target)
                            record["callees"].append(imported or "0x%08X" % target)
                        handled.add(target)
                elif transfer and code[:1] in (b"\xe9", b"\xeb") or (transfer and code[:1] != b"\xff"):
                    if refs and all(inside(target) for target in refs):
                        masked = bytearray(code[:2] if code[:1] == b"\x0f" else code[:1])
                        notes.append(label(refs[0]))
                        edges += 1
                    elif refs:
                        masked = bytearray(code[:1])
                        notes.append("TAILCALL")
                        for target in refs:
                            imported = import_through(target)
                            record["tailcalls"].append(imported or "0x%08X" % target)
                    handled.update(refs)
                elif transfer:
                    edges += 1

                # 3: the dwords that are reference targets.
                windows = set()
                for target in refs:
                    if target in handled:
                        continue
                    at = bytes(masked).find(struct.pack("<I", target))
                    if at < 0:
                        continue
                    kind = classify(target)
                    masked[at:at + 4] = kind.encode().ljust(4, b"_")[:4]
                    notes.append(kind)
                    windows.update(range(at, at + 4))

                # 5: a jump through a table (jmp [table + index * 4]; IDA lists its cases, not the table): the table
                # masked and its entries in table order.
                if transfer and code[:2] == b"\xff\x24" and len(code) == 7:
                    table = struct.unpack_from("<I", code, 3)[0]
                    cases = []
                    entry = table
                    while True:
                        value = image.dword(entry)
                        if value is None or value not in refs or not inside(value):
                            break
                        cases.append(label(value))
                        entry += 4
                    if cases:
                        masked[3:7] = b"JTAB"
                        notes.append("SW:" + " ".join(cases))
                        windows.update(range(3, 7))

                # 3b: the other dwords that are addresses of the program.
                at = 0
                while at + 4 <= len(masked):
                    if any(index in windows for index in range(at, at + 4)):
                        at += 1
                        continue
                    value = struct.unpack_from("<I", masked, at)[0]
                    if image.loaded(value) and bytes(masked[at:at + 4]) == code[at:at + 4]:
                        section = image.section_of(value) or "?"
                        masked[at:at + 4] = b"NREF"
                        notes.append("NOREF" + section)
                        record["noref"].append("0x%08X" % value)
                        if not section.startswith(".text"):
                            record["globals"].append("0x%08X" % value)
                        windows.update(range(at, at + 4))
                        at += 4
                        continue
                    at += 1

                # Calls through the IAT keep their form (FF15) apart from direct calls (E8).
                token = bytes(masked).hex() + ("|" + ",".join(notes) if notes else "")
                tokens.append(token)
                blocks[-1].append(token)
                shape = shape_of(text)
                shapes.append(shape)
                texts.append(text.split(";", 1)[0].strip())
                shape_blocks[-1].append(shape)

                if transfer or mnemonic in ENDS:
                    blocks.append([])
                    shape_blocks.append([])

            blocks = [block for block in blocks if block]
            shape_blocks = [block for block in shape_blocks if block]

            # FINGERPRINT_TOKENS=<dir>: the normalised instructions of each function (token, shape, text), for
            # comparing two of them and for aligning the operands of matched ones (match_functions.js).
            token_dir = os.environ.get("FINGERPRINT_TOKENS")
            if token_dir:
                os.makedirs(token_dir, exist_ok=True)
                with open(os.path.join(token_dir, "%08X.txt" % address), "w", encoding="utf-8", newline="\n") as fh:
                    fh.write("\n".join("\t".join(line) for line in zip(tokens, shapes, texts)) + "\n")

            result = {
                "address": "0x%08X" % address,
                "name": name,
                "funclets": ["0x%08X" % funclet for funclet in sorted(owned.get(address, []))],
                "hash": hashlib.sha1("\n".join(tokens).encode()).hexdigest(),
                "blocks": [hashlib.sha1("\n".join(block).encode()).hexdigest() for block in blocks],
                "mnemonics": hashlib.sha1(" ".join(mnemonics).encode()).hexdigest(),
                "shape": hashlib.sha1("\n".join(shapes).encode()).hexdigest(),
                "shape_blocks": [hashlib.sha1("\n".join(block).encode()).hexdigest() for block in shape_blocks],
                "instructions": len(tokens),
                "block_count": len(blocks),
                "edges": edges,
            }
            result.update(record)
            out.write(json.dumps(result, ensure_ascii=False) + "\n")


main()
