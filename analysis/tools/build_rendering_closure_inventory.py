#!/usr/bin/env python3
"""Build the finite B4 rendering closure matrix from the locked LST scope."""

from __future__ import annotations

import csv
import hashlib
from dataclasses import dataclass
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
WORKSPACE_ROOT = RESEARCH_ROOT.parents[1]
LST_PATH = WORKSPACE_ROOT / "swd3.exe_export_for_ai" / "swd3.exe.lst"
OWNERSHIP_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "module-function-ownership.tsv"
)
OUTPUT_PATH = (
    RESEARCH_ROOT
    / "04-reverse-engineering"
    / "inventory"
    / "rendering-closure.tsv"
)

EXPECTED_SHA256 = {
    LST_PATH: "701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b",
    OWNERSHIP_PATH: "a17f41539cc2d740d0eb71561ca19624d10244d8220b7e0dbdc8a603b9b49101",
}


@dataclass(frozen=True)
class Policy:
    disposition: str
    target_owner: str
    implementation_mapping: str
    evidence: str
    remaining_work: str


IMPLEMENTED_STREAM_CONVERSION = {
    0x004014F0,
    0x004019A0,
    0x00401B70,
    0x00401C70,
    0x00401E50,
}
IMPLEMENTED_DRAWING_HELPERS = {
    0x0040DE50,
    0x0040E080,
    0x004117F0,
}
IMPLEMENTED_SCALED_RLE_WRITERS = {
    0x00422C70,
    0x00423020,
}
IMPLEMENTED_SOFTWARE_HELPERS = {
    0x00417050,
    0x004174D0,
    0x00417530,
    0x004175B0,
    0x00417650,
    0x00417DE0,
}
IMPLEMENTED_ACTION_RENDERERS = {
    0x00411FA0,
    0x00414B60,
    0x00414CE0,
    0x00414E50,
    0x004153D0,
}
IMPLEMENTED_FRAME_COLOR = {
    0x00420490,
    0x00420560,
    0x00420600,
    0x004206F0,
    0x004207E0,
    0x00421FB0,
}
IMPLEMENTED_FONT_RUNTIME = {
    0x0040F340,
    0x00435160,
    0x004351F0,
}
INTERNAL_RAW_FADE_STEPS = {
    0x00417FF4,
    0x0041802D,
    0x00418066,
    0x0041809F,
    0x004180D8,
    0x00418111,
    0x0041814A,
    0x00418183,
    0x004181BC,
    0x004181F5,
    0x0041822E,
    0x00418267,
    0x0041829D,
    0x004182D3,
    0x00418309,
    0x0041830E,
}
IMPLEMENTED_BLITTER = {
    0x00416D90,
    0x00416F80,
    0x00416FF0,
    0x004170E0,
    0x004176D0,
    0x004177D0,
    0x00417840,
    0x00417950,
    0x00417E40,
    0x00417EC0,
    0x00418350,
    0x004185C0,
    0x00418840,
    0x00418EB0,
    0x0041B280,
    0x0041B620,
    0x0041B9F0,
    0x0041CCF0,
    0x0041D010,
    0x0041D340,
    0x0041E5C0,
    0x0041F8D0,
    0x0041FEA0,
    0x004208D0,
    0x00420D70,
    0x00421230,
    0x00421540,
    0x00421850,
    0x00421BE0,
    0x00422030,
    0x004223A0,
    0x00422730,
    0x004229C0,
}
UNREACHABLE_BLITTER = {0x00419570, 0x0041A3B0}
IMPLEMENTED_PIXEL_FORMAT = {
    0x00416D30,
    0x004233D0,
    0x00423400,
    0x004238B0,
    0x004238D0,
    0x004238F0,
    0x00423920,
    0x00423950,
    0x00423990,
    0x004239D0,
}
IMPLEMENTED_FEATURES = {
    0x0042E850,
    0x004303D0,
    0x004306C0,
    0x004308C0,
    0x00430B60,
    0x0043B110,
    0x0043BAB0,
}
PLATFORM_FRAMEBUFFER = {0x00416F10, 0x00416F60}
PLATFORM_FONT_PRODUCTION = {
    0x00435240,
    0x004352C0,
    0x004352E0,
    0x004353C0,
    0x00435410,
    0x00435430,
    0x004354D0,
    0x00435500,
    0x004355B0,
    0x00436840,
}
IMPLEMENTED_FONT_CORE = {
    0x00435620,
    0x00435650,
    0x00435660,
    0x00435670,
    0x00435680,
    0x004358C0,
    0x00435AF0,
    0x00435D80,
    0x00436030,
    0x00436410,
    0x004368D0,
    0x00436980,
    0x004369C0,
    0x00436AD0,
    0x00436EA0,
}
PLATFORM_DIRECTDRAW = {
    0x00437570,
    0x00437610,
    0x00437680,
    0x004376D0,
    0x00437720,
    0x004377D0,
    0x00437A50,
    0x00437AD0,
    0x00437AE0,
    0x00437B60,
    0x00437C00,
    0x00437C40,
    0x00437C80,
    0x00437D80,
    0x00437DC0,
    0x00437DF0,
    0x00437E20,
    0x00437E30,
    0x00437E90,
    0x00437EB0,
    0x00437EF0,
    0x00437F40,
    0x00437F90,
}
DEFERRED_BATTLE_SURFACES = {0x00451A90, 0x00451AE0}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def policy_for(address: int) -> Policy:
    if address == 0x004350E0:
        return Policy(
            "platform_adapted",
            "rendering;platform_sdl3",
            "LegacyFramebuffer and LegacyTextRendererRuntime construction",
            "asset-runtime-closure-audit.md;"
            "font-surface-and-glyph-rendering.md",
            "none",
        )
    if address in IMPLEMENTED_STREAM_CONVERSION:
        return Policy(
            "implemented",
            "rendering",
            "legacy image command-stream encode/decode/pixel-conversion family",
            "legacy-image-command-stream-004014f0-00401e50.md",
            "none",
        )
    if address in IMPLEMENTED_DRAWING_HELPERS:
        return Policy(
            "implemented",
            "rendering",
            "legacy animated border/thumbnail/decorated-number helpers",
            "legacy-drawing-helpers-0040de50-004117f0.md",
            "none",
        )
    if address in IMPLEMENTED_SCALED_RLE_WRITERS:
        return Policy(
            "implemented",
            "rendering",
            "LegacyScaledRleSource;LegacyScaledRleTransform;"
            "write_legacy_scaled_rle_forward/reverse",
            "legacy-scaled-rle-writers-00422c70-00423020.md",
            "none",
        )
    if address in IMPLEMENTED_SOFTWARE_HELPERS:
        return Policy(
            "implemented",
            "rendering",
            "legacy outline/fixed-tile/packed-row software helpers",
            "legacy-outline-fixed-tile-and-packed-row-00417050-00417de0.md",
            "none",
        )
    if address == 0x00414B60:
        return Policy(
            "implemented",
            "rendering;story_scene;world_map",
            "LegacyMovingActionNode; update_draw_legacy_moving_actions; "
            "LegacyWorldFrameRuntimePorts",
            "moving-actions-00414b60.md",
            "none",
        )
    if address == 0x00414CE0:
        return Policy(
            "implemented",
            "rendering;story_scene;world_map",
            "LegacyRoleHeadActionNode; update_draw_legacy_role_head_actions; "
            "LegacyWorldFrameRuntimePorts",
            "role-head-actions-00414ce0.md",
            "none",
        )
    if address in IMPLEMENTED_ACTION_RENDERERS:
        return Policy(
            "implemented",
            "rendering;story_scene",
            "legacy pause/action/packed-row/timed-message coordinators",
            "legacy-pause-action-and-message-renderers-00411fa0-004153d0.md",
            "none",
        )
    if address in IMPLEMENTED_FRAME_COLOR:
        return Policy(
            "implemented",
            "rendering",
            "legacy_frame_color signed-offset/pair/combine/grayscale helpers",
            "frame-color-adjustment-and-combine.md",
            "none",
        )
    if address in IMPLEMENTED_FONT_RUNTIME:
        return Policy(
            "implemented",
            "rendering;platform_sdl3",
            "LegacyTextRendererRuntime;SdlSmokeInitializationPorts;"
            "SdlDisplayLifecyclePorts;SmokeShutdownPorts",
            "font-surface-and-glyph-rendering.md",
            "none",
        )
    if address in INTERNAL_RAW_FADE_STEPS:
        return Policy(
            "implemented_internal_branch",
            "rendering",
            "blit_legacy_copy_paths/raw_constant_vertical_fade",
            "legacy-blitter-raw-constant-fade-00417ec0.md",
            "none",
        )
    if address in IMPLEMENTED_BLITTER:
        return Policy(
            "implemented",
            "rendering",
            "select_legacy_blitter;blit_legacy_copy_paths",
            "software-blitter-dispatch-and-pixel-effects.md",
            "none",
        )
    if address in UNREACHABLE_BLITTER:
        return Policy(
            "unreachable_current_assets",
            "rendering",
            "explicit unsupported_routine boundary",
            "software-blitter-dispatch-and-pixel-effects.md",
            "preserve the original dangerous forced-state boundary; no normal-path implementation",
        )
    if address in IMPLEMENTED_PIXEL_FORMAT:
        return Policy(
            "implemented",
            "rendering",
            "LegacyFramebuffer;LegacyPixelConversionState;legacy pixel converters",
            "legacy-framebuffer-geometry-00416d30.md;pixel-format-selection-and-cm-cache.md",
            "none",
        )
    if address in IMPLEMENTED_FEATURES:
        return Policy(
            "implemented",
            "rendering",
            "legacy tiled frame/BMP/formatted text/countdown/rectangle/effect panel units",
            "rendering.md:B4.7",
            "none",
        )
    if address in PLATFORM_FRAMEBUFFER:
        return Policy(
            "platform_adapted",
            "rendering;platform_sdl3",
            "LegacyFramebuffer stable owned storage",
            "legacy-framebuffer-geometry-00416d30.md",
            "none",
        )
    if address in PLATFORM_FONT_PRODUCTION:
        return Policy(
            "platform_adapted",
            "rendering;platform_sdl3",
            "LegacyGlyphAtlasProvider;legacy-glyph-atlas.bin",
            "font-surface-and-glyph-rendering.md",
            "none",
        )
    if address in IMPLEMENTED_FONT_CORE:
        return Policy(
            "implemented",
            "rendering",
            "LegacyTextRendererState;LegacyGlyphCache;draw_legacy_text;legacy glyph writers",
            "font-surface-and-glyph-rendering.md",
            "none",
        )
    if address == 0x00436FA0:
        return Policy(
            "transferred",
            "input_time_rng",
            "legacy input-device release boundary",
            "swd3.exe.lst:0x00436FA0 -> 0x004374E0",
            "B3/runtime integration; remove the stale rendering ownership",
        )
    if address in PLATFORM_DIRECTDRAW:
        return Policy(
            "platform_adapted",
            "rendering;platform_sdl3",
            "SDL backend lifecycle;LegacyFramebuffer;LegacyPrimaryCompositionStatus",
            "framebuffer-and-display-presentation.md;legacy-primary-presentation-requests.md",
            "none",
        )
    if address in DEFERRED_BATTLE_SURFACES:
        return Policy(
            "implemented_with_deferred_binding",
            "rendering;battle",
            "LegacyPresentationSources battle surface roles",
            "legacy-primary-presentation-requests.md",
            "B10 supplies the owned battle surfaces at the existing lifecycle ports",
        )
    raise SystemExit(f"unclassified rendering address: 0x{address:08X}")


def main() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"input hash changed for {path}: expected {expected}, got {actual}"
            )

    with OWNERSHIP_PATH.open("r", encoding="utf-8", newline="") as source:
        rows = [
            row
            for row in csv.DictReader(source, delimiter="\t")
            if row["module_candidate"] == "rendering"
        ]
    if len(rows) != 152:
        raise SystemExit(f"expected 152 rendering entries, got {len(rows)}")

    output_rows = []
    for row in rows:
        address = int(row["address"], 16)
        policy = policy_for(address)
        output_rows.append(
            {
                "address": f"0x{address:08X}",
                "closure_disposition": policy.disposition,
                "target_owner": policy.target_owner,
                "implementation_mapping": policy.implementation_mapping,
                "evidence": policy.evidence,
                "remaining_work": policy.remaining_work,
            }
        )

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.DictWriter(
            destination,
            fieldnames=output_rows[0].keys(),
            delimiter="\t",
            lineterminator="\n",
        )
        writer.writeheader()
        writer.writerows(output_rows)

    print(f"wrote {OUTPUT_PATH}: {len(output_rows)} rows")


if __name__ == "__main__":
    main()
