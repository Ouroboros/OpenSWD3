#!/usr/bin/env python3
"""Build the static capture map for SWD3 P4 dynamic oracles.

This tool does not launch or modify the original executable.  It locks the
binary/runtime inputs and verifies that every proposed probe still points at
the same assembly instruction.  Assembly remains the sole behavioral truth;
future dynamic captures are corroborating and regression evidence.
"""

from __future__ import annotations

import csv
import hashlib
import re
import struct
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
ASM_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.asm"
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"

CAPTURE_OUTPUT = INVENTORY_ROOT / "p4-oracle-capture-points.tsv"
ARTIFACT_OUTPUT = INVENTORY_ROOT / "p4-oracle-artifacts.tsv"
BASELINE_OUTPUT = INVENTORY_ROOT / "p4-oracle-runtime-baseline.tsv"

LOCKED_FILES = {
    "swd3.exe": "0bac897a7557735b22607d8c8f0a79a3e7ae7729deb56593fd91c21e10baee0c",
    "swd3.exe_export_for_ai/swd3.exe.asm": "d902f6dfd47d7033bf8a971c4ccc3a4d8d037b5b577035041113329363cab052",
    "binkw32.dll": "545f402be018bc905c03305322072e194d77b1a5ec20b62c34cdbc4c81c961fa",
    "Mss32.dll": "9e41e1bd3b38d7ac43354bb83310210dd9e105b228b57e07af305cd160b7bf8f",
    "Mp3dec.asi": "f059d7d8b2bc6f732a6e44938726311415c1e1e0cf4fb17a6137403841085e89",
    "Env.dat": "2c55dddc9a6808afda5d69688f2c27ac268caf2b9155ae82b18596ed593ed9a4",
    "Video/opening.bik": "8fd6e53923ca235cbc7b780780106c87caac319d021c7559a10a495de95a60ee",
}

ASM_LINE_RE = re.compile(r"^([0-9A-F]{8})\s+(.+?)\s*$")

EXPECTED_SNIPPETS = {
    # P screenshot gate and exact source arguments.
    0x0040A285: "cmp ebx, 50h",
    0x0040A314: "mov eax, dword_4CD76C",
    0x0040A325: "call sub_4303D0",
    # Accepted-frame time and input sampling.
    0x0040A5A3: "call ds:timeGetTime",
    0x0040A5CC: "mov dword_4CC2B0, eax",
    0x0040A5D1: "test cl, 1",
    0x0040A744: "call sub_4372B0",
    0x0040A749: "call sub_4050E0",
    # Representative branch-local commits.
    0x00412716: "call dword ptr [ecx+14h]",
    0x0043A854: "call dword ptr [ecx+14h]",
    0x0045350A: "call dword ptr [edx+14h]",
    # Screenshot's untouched packed copy and reverse conversion.
    0x00430481: "rep movsd",
    0x0043048A: "rep movsb",
    0x00430490: "call sub_4238D0",
    0x00430560: "lea esi, [eax-1]",
    0x00430693: "mov eax, 1",
    # GDI-generated glyph cache miss writer.
    0x004368D0: "mov eax, [esp+lpString]",
    0x004368F2: "call sub_436840",
    0x00436934: "cmp word ptr [ebp+eax*2+0], 0",
    0x0043694A: "or [eax], dl",
    0x00436974: "retn 8",
    # Raw mouse DirectInput state and normalized input completion.
    0x00437331: "call dword ptr [ecx+24h]",
    0x00437334: "mov ebx, [esp+34h+var_1C]",
    # Bink destination write followed by the 639x479 DirectDraw commit.
    0x00484680: "call ds:_BinkCopyToBuffer@28",
    0x00484686: "test eax, eax",
    0x004849E1: "mov [esp+18h+var_8], 27Fh",
    0x004849E9: "mov [esp+18h+var_4], 1DFh",
    0x00484A11: "call dword ptr [edx+14h]",
    0x00484A14: "test eax, eax",
    # The other two timeGetTime sources.
    0x00411958: "call ds:timeGetTime",
    0x00411C10: "call ds:timeGetTime",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def normalize(text: str) -> str:
    return " ".join(text.split())


def load_assembly() -> dict[int, str]:
    by_address: dict[int, str] = {}
    for raw in ASM_PATH.read_text(encoding="utf-8", errors="replace").splitlines():
        match = ASM_LINE_RE.match(raw)
        if not match:
            continue
        text = match.group(2).split(";", 1)[0].rstrip()
        if not text or text.endswith(":"):
            continue
        if " = " in text or re.search(r"\bproc near\b|\bendp\b", text):
            continue
        by_address.setdefault(int(match.group(1), 16), normalize(text))
    return by_address


def verify_pe() -> tuple[int, int, int, int, int]:
    data = (WORKSPACE_ROOT / "swd3.exe").read_bytes()
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise SystemExit("swd3.exe no longer has a PE signature")
    coff_characteristics = struct.unpack_from("<H", data, pe_offset + 22)[0]
    optional = pe_offset + 24
    magic = struct.unpack_from("<H", data, optional)[0]
    if magic != 0x10B:
        raise SystemExit(f"expected PE32 optional header, got 0x{magic:04X}")
    image_base = struct.unpack_from("<I", data, optional + 28)[0]
    size_of_image = struct.unpack_from("<I", data, optional + 56)[0]
    dll_characteristics = struct.unpack_from("<H", data, optional + 70)[0]
    reloc_rva, reloc_size = struct.unpack_from("<II", data, optional + 96 + 5 * 8)
    expected = (0x00400000, 0x001A7000, 0, 0, 0)
    actual = (image_base, size_of_image, dll_characteristics, reloc_rva, reloc_size)
    if actual != expected or not (coff_characteristics & 0x0001):
        raise SystemExit(
            "PE fixed-address contract changed: "
            f"actual={actual}, COFF characteristics=0x{coff_characteristics:04X}"
        )
    return actual


def verify_inputs() -> tuple[int, int, int, int, int]:
    for relative, expected in LOCKED_FILES.items():
        path = WORKSPACE_ROOT / relative
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"locked input changed: {relative}: expected {expected}, got {actual}"
            )
    return verify_pe()


def verify_assembly(by_address: dict[int, str]) -> None:
    for address, expected in EXPECTED_SNIPPETS.items():
        actual = by_address.get(address)
        if actual != expected:
            raise SystemExit(
                f"instruction mismatch at 0x{address:08X}: expected {expected!r}, got {actual!r}"
            )


def build_capture_rows() -> list[tuple[object, ...]]:
    return [
        ("accepted_frame_clock", "0x0040A5A3", "before/after call", "timeGetTime return; then 0x004AAECC and interval gate", "log u32 return and callsite; do not substitute host floating time"),
        ("accepted_frame_start", "0x0040A5D1", "after accepted-time store", "accepted u32 time in 0x004CC2B0/0x004AAECC", "frame sequence number begins only after the interval gate accepts"),
        ("keyboard_snapshot", "0x0040A749", "before mouse/normalizer call", "0x004B8748, 256 bytes", "captures the successful DirectInput scan-code snapshot before mouse sampling"),
        ("mouse_raw_state", "0x00437334", "after virtual GetDeviceState returns", "stack-local 0x1C-byte mouse state", "record X/Y and button bytes before baseline/scaling logic; failed HRESULT is not checked"),
        ("normalized_input", "0x0040A74E", "after sub_4050E0 returns", "0x004B7CB0, 20*16 bytes plus logical mouse globals", "pairs one accepted frame with the exact post-normalization state"),
        ("world_precommit", "0x00412716", "before DirectDraw Blt", "0x004CD76C plus pitch/masks", "representative ordinary-world final software framebuffer"),
        ("special_mode_precommit", "0x0043A854", "before DirectDraw Blt", "0x004CD76C plus pitch/masks", "representative menu/special-mode final software framebuffer"),
        ("battle_precommit", "0x0045350A", "before DirectDraw Blt", "0x004CD76C plus pitch/masks", "representative battle final software framebuffer"),
        ("p_screenshot_gate", "0x0040A285", "conditional branch", "WM_KEYDOWN wParam 0x50 and surrounding state", "documents whether the built-in P capture was actually eligible"),
        ("screenshot_preconvert_copy", "0x00430490", "before sub_4238D0 call", "top stack pointer to width*height*2 heap copy; next stack dword is pixel count", "exact contiguous packed-16 bytes before screenshot reverse conversion"),
        ("screenshot_rgb555_copy", "0x00430495", "after sub_4238D0 returns", "heap copy saved in local var_AC", "canonical screenshot intermediate; may have lost narrowed surface bits"),
        ("screenshot_complete", "0x00430693", "success return", "written 24-bit bottom-up BMP", "visual corroboration only; preserve file hash and byte length"),
        ("glyph_mask_entry", "0x004368D0", "function entry", "ECX renderer; [ESP+4] string; [ESP+8] destination mask slot", "save face/size/object fields and destination address before GDI rasterization"),
        ("glyph_mask_complete", "0x00436974", "before retn 8", "destination captured at entry; size=(object+0xFD4)*(object+0x1C)", "raw MSB-first 1-bit glyph mask after nonzero thresholding"),
        ("bink_copy_complete", "0x00484686", "after BinkCopyToBuffer", "0x004CD76C physical surface rows and Bink frame number", "decoder output before any DirectDraw RECT interpretation"),
        ("bink_precommit", "0x00484A11", "before DirectDraw Blt", "source and destination RECT {0,0,639,479}; software framebuffer", "locks the exact inclusive-looking values passed to legacy DirectDraw"),
        ("bink_postcommit", "0x00484A14", "after DirectDraw Blt", "visible primary pixels plus HRESULT", "only dynamic comparison can settle the last-column/last-row visible result"),
        ("cd_poll_clock", "0x00411958", "before/after call", "timeGetTime return in CD/file poll loop", "log separately from accepted-frame clock"),
        ("cd_hold_clock", "0x00411C10", "before/after call", "timeGetTime return in 500ms hold loop", "log separately from accepted-frame clock"),
    ]


def build_artifact_rows() -> list[tuple[object, ...]]:
    return [
        ("run-manifest.tsv", "one row per run", "EXE/DLL/asset/font/OS/display hashes and versions", "required", "without it a pixel mismatch cannot be attributed"),
        ("frame-events.tsv", "one row per accepted frame", "sequence,u32 clock,interval,mode,input hashes,artifact hashes", "required", "authoritative join key for deterministic replay"),
        ("framebuffer-physical.bin", "pitch*480 bytes", "raw bytes from 0x004CD76C including row padding", "required per selected frame", "preserves actual DirectDraw surface layout"),
        ("framebuffer-logical.bin", "480 rows of 640*2 bytes", "canonical packed-16 visible rows plus masks", "required per selected frame", "primary pixel oracle before platform upload"),
        ("builtin-preconvert.bin", "width*height*2 bytes", "sub_4303D0 private copy before sub_4238D0", "required when P capture is used", "proves what the original screenshot writer consumed"),
        ("builtin-rgb555.bin", "width*height*2 bytes", "private copy after reverse conversion", "recommended", "separates conversion differences from BMP serialization"),
        ("builtin-screenshot.bmp", "original file bytes", "24-bit bottom-up BMP", "recommended", "visual and original-writer corroboration; not the sole pixel oracle"),
        ("glyph-mask.bin", "font_height*mask_row_bytes per cache miss", "raw MSB-first 1-bit cache slot", "required for selected CP950 glyph set", "freezes host GDI/font-dependent input to software footprint writers"),
        ("bink-source.bin", "pitch*480 bytes", "software framebuffer immediately after BinkCopyToBuffer", "required for Bink case", "separates decoder output from DirectDraw RECT behavior"),
        ("bink-primary.bin", "640*480 visible packed pixels", "primary surface after the 639x479 Blt", "required for Bink case", "settles right-column/bottom-row behavior"),
        ("time-input-trace.tsv", "one row per sampled call/frame", "three timeGetTime callsites; raw keyboard/mouse; normalized records", "required for replay", "replay must inject recorded u32 values and device samples in original order"),
    ]


def build_baseline_rows(pe: tuple[int, int, int, int, int]) -> list[tuple[object, ...]]:
    rows = []
    roles = {
        "swd3.exe": "sole behavioral binary",
        "swd3.exe_export_for_ai/swd3.exe.asm": "sole static behavioral truth",
        "binkw32.dll": "legacy Bink decoder used by the Bink oracle",
        "Mss32.dll": "legacy Miles runtime; screenshot loop calls AIL_serve",
        "Mp3dec.asi": "Miles MP3 provider in the captured runtime set",
        "Env.dat": "current key-binding/environment input",
        "Video/opening.bik": "selected deterministic Bink asset candidate",
    }
    for relative, expected in LOCKED_FILES.items():
        path = WORKSPACE_ROOT / relative
        rows.append((relative, path.stat().st_size, expected, roles[relative]))
    image_base, size_image, dll_chars, reloc_rva, reloc_size = pe
    rows.extend([
        ("PE.ImageBase", f"0x{image_base:08X}", "n/a", "fixed probe-address base"),
        ("PE.SizeOfImage", f"0x{size_image:08X}", "n/a", "fixed image span"),
        ("PE.DllCharacteristics", f"0x{dll_chars:04X}", "n/a", "zero; no dynamic-base flag"),
        ("PE.BaseRelocationDirectory", f"rva=0x{reloc_rva:08X},size=0x{reloc_size:X}", "n/a", "empty and COFF relocations-stripped bit is set"),
    ])
    return rows


def write_tsv(path: Path, header: tuple[str, ...], rows: list[tuple[object, ...]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as output:
        writer = csv.writer(output, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)


def main() -> None:
    pe = verify_inputs()
    by_address = load_assembly()
    verify_assembly(by_address)
    capture_rows = build_capture_rows()
    artifact_rows = build_artifact_rows()
    baseline_rows = build_baseline_rows(pe)
    if (len(capture_rows), len(artifact_rows), len(baseline_rows)) != (19, 11, 11):
        raise SystemExit("unexpected P4 oracle inventory row count")
    write_tsv(
        CAPTURE_OUTPUT,
        ("probe", "address", "timing", "capture", "purpose"),
        capture_rows,
    )
    write_tsv(
        ARTIFACT_OUTPUT,
        ("artifact", "physical_extent", "content", "requirement", "purpose"),
        artifact_rows,
    )
    write_tsv(
        BASELINE_OUTPUT,
        ("component", "size_or_value", "sha256", "oracle_role"),
        baseline_rows,
    )
    print(f"wrote {CAPTURE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(capture_rows)} rows)")
    print(f"wrote {ARTIFACT_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(artifact_rows)} rows)")
    print(f"wrote {BASELINE_OUTPUT.relative_to(RESEARCH_ROOT)} ({len(baseline_rows)} rows)")


if __name__ == "__main__":
    main()
