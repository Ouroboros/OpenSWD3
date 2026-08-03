#!/usr/bin/env python3
"""Write the assembly-derived ACT command to ActionRecord field-effect matrix."""

from __future__ import annotations

import csv
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
OUTPUT = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "act-command-field-effects.tsv"
)


@dataclass(frozen=True)
class Effect:
    command: int
    action_offset: int
    width: int
    parameter_ordinal: str
    transform: str
    instruction_addresses: str
    condition_or_note: str = ""


EFFECTS = (
    Effect(0x4145, 0x24, 4, "1", "zero_extend_u16", "0043256B"),
    Effect(0x4148, 0x18, 4, "", "(old & 0x8000000B) | 0x08", "0043250C-00432518"),
    Effect(0x414D, 0x18, 4, "", "(old & 0x80000007) | 0x04", "00432520-0043252C"),
    Effect(0x414E, 0x18, 4, "", "(old & 0x8000002F) | 0x2C", "00432534-00432540"),
    Effect(0x4154, 0x5A, 2, "1", "copy_u16", "00432552"),
    Effect(0x4158, 0x76, 2, "1", "copy_u16", "00432581"),
    Effect(0x4159, 0x78, 2, "1", "copy_u16", "00432598"),
    Effect(0x4342, 0x68, 2, "1", "copy_u16", "004325C0"),
    Effect(0x4342, 0x74, 2, "2", "copy_u16", "004325C8", "parameter 2 is reprocessed next loop"),
    Effect(0x4347, 0x66, 2, "1", "copy_u16", "0043264D"),
    Effect(0x4347, 0x72, 2, "2", "copy_u16", "00432655", "parameter 2 is reprocessed next loop"),
    Effect(0x434C, 0x70, 2, "", "constant_0", "00432621"),
    Effect(0x434C, 0x72, 2, "", "constant_0", "00432625"),
    Effect(0x434C, 0x74, 2, "", "constant_0", "00432629"),
    Effect(0x4352, 0x64, 2, "1", "copy_u16", "00432610"),
    Effect(0x4352, 0x70, 2, "2", "copy_u16", "00432618", "parameter 2 is reprocessed next loop"),
    Effect(0x4544, 0x42, 2, "", "rewind_one_word", "004329BC-004329C2", "only when action+0x90 == 1"),
    Effect(0x464C, 0x7A, 2, "1", "copy_u16", "00432671"),
    Effect(0x464C, 0x7C, 2, "2", "copy_u16", "0043267A"),
    Effect(0x464C, 0x7E, 2, "3", "copy_u16", "00432683"),
    Effect(0x464C, 0x80, 2, "4", "copy_u16", "0043268C"),
    Effect(0x464C, 0x82, 2, "5", "copy_u16", "00432698"),
    Effect(0x464C, 0x84, 2, "6", "copy_u16", "004326A4"),
    Effect(0x464C, 0x86, 2, "7", "copy_u16", "004326B0"),
    Effect(0x4753, 0x18, 4, "", "(old & 0x80000017) | 0x14", "00432713-00432725"),
    Effect(0x4753, 0x8A, 1, "1", "copy_low_u8", "00432728-0043272B"),
    Effect(0x4C44, 0x18, 4, "", "(old & 0x80000013) | 0x10", "004326E5-004326F5"),
    Effect(0x4C44, 0x62, 2, "1", "zero_extend_low_u8", "00432701-00432706"),
    Effect(0x4E4F, 0x18, 4, "", "old & ~0x01", "004326DC"),
    Effect(0x4F32, 0x42, 2, "", "rewind_one_word", "004329ED-00432A00", "always"),
    Effect(0x4F32, 0x8C, 4, "", "constant_1", "004329FA", "only when action+0x44 == 0"),
    Effect(0x4F41, 0x50, 2, "1", "copy_u16", "0043274F"),
    Effect(0x4F56, 0x42, 2, "", "rewind_one_word_or_zero", "004329D0-004329DC", "rewind when action+0x90 == 1; otherwise zero"),
    Effect(0x4F58, 0x5E, 2, "1", "copy_u16", "00432810"),
    Effect(0x4F59, 0x60, 2, "1", "copy_u16", "004327F2"),
    Effect(0x5041, 0x40, 2, "", "increment_high_u8_wrap_to_1_after_low_u8", "00432793-004327D4"),
    Effect(0x5041, 0x4C, 2, "1", "copy_u16", "004327AC"),
    Effect(0x5145, 0x28, 4, "1", "zero_extend_u16", "00432825"),
    Effect(0x5246, 0x4A, 2, "1", "copy_u16", "004328AF"),
    Effect(0x524F, 0x4E, 2, "1", "copy_u16", "00432891"),
    Effect(0x5344, 0x44, 2, "1", "copy_u16_or_action_0x48_low15_override", "0043285C-00432873", "override when action+0x48 bit 15 is set"),
    Effect(0x5344, 0x46, 2, "1", "copy_u16", "00432854"),
    Effect(0x534D, 0x94, 4, "", "constant_1", "004328B8"),
    Effect(0x544E, 0x40, 2, "1", "copy_u16", "0043292A"),
    Effect(0x5457, 0x88, 1, "1", "copy_low_u8", "004328FF-00432906"),
    Effect(0x5649, 0x18, 4, "", "old | 0x01", "004328EA"),
    Effect(0x5748, 0x2C, 4, "1", "zero_extend_u16", "0043293F"),
    Effect(0x5748, 0x30, 4, "2", "zero_extend_u16", "00432956", "parameter 2 is reprocessed next loop"),
    Effect(0x5756, 0x58, 2, "1", "copy_u16", "004329A7"),
    Effect(0x5859, 0x10, 4, "1", "zero_extend_u16", "0043297E"),
    Effect(0x5859, 0x14, 4, "2", "zero_extend_u16", "00432995", "parameter 2 is reprocessed next loop"),
)

EXPECTED_EFFECT_COUNT = 51


def file_ascii(word: int) -> str:
    raw = bytes((word & 0xFF, word >> 8))
    return "".join(chr(byte) if 0x20 <= byte <= 0x7E else "." for byte in raw)


def main() -> None:
    if len(EFFECTS) != EXPECTED_EFFECT_COUNT:
        raise SystemExit(f"unexpected ACT command field-effect count: {len(EFFECTS)}")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "command_word_hex",
                "file_bytes_ascii",
                "action_offset_hex",
                "write_width_bytes",
                "parameter_ordinal",
                "write_transform",
                "producer_instruction_addresses",
                "condition_or_note",
            )
        )
        for effect in sorted(EFFECTS, key=lambda item: (item.command, item.action_offset)):
            writer.writerow(
                (
                    f"{effect.command:04X}",
                    file_ascii(effect.command),
                    f"{effect.action_offset:02X}",
                    effect.width,
                    effect.parameter_ordinal,
                    effect.transform,
                    effect.instruction_addresses,
                    effect.condition_or_note,
                )
            )

    fields = {effect.action_offset for effect in EFFECTS}
    relative_output = OUTPUT.relative_to(WORKSPACE_ROOT)
    print(
        f"wrote {len(EFFECTS)} ACT command effects across {len(fields)} action fields "
        f"to {relative_output}"
    )


if __name__ == "__main__":
    main()
