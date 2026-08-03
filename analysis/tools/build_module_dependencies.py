#!/usr/bin/env python3
"""Build the Stage A4 original module dependency inventory."""

from __future__ import annotations

import csv
import hashlib
from collections import defaultdict
from pathlib import Path


RESEARCH_ROOT = Path(__file__).resolve().parents[1]
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
CROSS_CALL_PATH = INVENTORY_ROOT / "module-cross-calls.tsv"
STATE_PATH = INVENTORY_ROOT / "module-state-ownership.tsv"
OUTPUT_PATH = INVENTORY_ROOT / "module-dependencies.tsv"

EXPECTED_SHA256 = {
    CROSS_CALL_PATH: "a8b10704bdb61fdb4aa4fa51102c9b4772dd1048b6100a9e0f8f3950a5c88c39",
    STATE_PATH: "4aada7d5ebd719e32533b64722369261b4b1ae807b2ac200c2f0e26c0b559da3",
}

GAME_MODULES = (
    "runtime_platform",
    "resource_io",
    "input_time_rng",
    "rendering",
    "audio_video",
    "asset_runtime",
    "story_scene",
    "world_map",
    "special_modes",
    "battle",
    "persistence",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_tsv(path: Path) -> list[dict[str, str]]:
    with path.open("r", encoding="utf-8", newline="") as source:
        return list(csv.DictReader(source, delimiter="\t"))


def strongly_connected_components(
    nodes: tuple[str, ...], edges: set[tuple[str, str]]
) -> list[tuple[str, ...]]:
    adjacency = {node: [] for node in nodes}
    reverse = {node: [] for node in nodes}
    for source, target in sorted(edges):
        adjacency[source].append(target)
        reverse[target].append(source)

    visited: set[str] = set()
    finish_order: list[str] = []

    def visit(node: str) -> None:
        if node in visited:
            return
        visited.add(node)
        for target in adjacency[node]:
            visit(target)
        finish_order.append(node)

    for node in nodes:
        visit(node)

    visited.clear()
    components: list[tuple[str, ...]] = []

    def collect(node: str, component: list[str]) -> None:
        if node in visited:
            return
        visited.add(node)
        component.append(node)
        for target in reverse[node]:
            collect(target, component)

    for node in reversed(finish_order):
        if node in visited:
            continue
        component: list[str] = []
        collect(node, component)
        components.append(tuple(sorted(component)))
    return sorted(components)


def main() -> None:
    for path, expected in EXPECTED_SHA256.items():
        actual = sha256(path)
        if actual != expected:
            raise SystemExit(
                f"input hash mismatch for {path.name}: {actual} != {expected}"
            )

    direct_pairs: dict[tuple[str, str], int] = defaultdict(int)
    direct_callsites: dict[tuple[str, str], int] = defaultdict(int)
    for row in read_tsv(CROSS_CALL_PATH):
        if row["boundary_kind"] != "game_cross_module":
            continue
        edge = (
            row["caller_module_candidate"],
            row["callee_module_candidate"],
        )
        if edge[0] not in GAME_MODULES or edge[1] not in GAME_MODULES:
            raise SystemExit(f"unexpected game dependency edge: {edge}")
        direct_pairs[edge] += 1
        direct_callsites[edge] += int(row["callsite_count"])

    written_states: dict[tuple[str, str], set[str]] = defaultdict(set)
    borrowed_states: dict[tuple[str, str], set[str]] = defaultdict(set)
    for row in read_tsv(STATE_PATH):
        owner = row["owner_module"]
        if owner not in GAME_MODULES:
            raise SystemExit(f"unexpected state owner: {owner}")
        for writer in filter(None, row["writer_modules"].split(";")):
            if writer in GAME_MODULES and writer != owner:
                written_states[(writer, owner)].add(row["state_id"])
        for reader in filter(None, row["reader_modules"].split(";")):
            if reader in GAME_MODULES and reader != owner:
                borrowed_states[(reader, owner)].add(row["state_id"])

    edges = set(direct_pairs) | set(written_states) | set(borrowed_states)
    components = strongly_connected_components(GAME_MODULES, edges)
    component_by_module: dict[str, str] = {}
    cycle_number = 0
    for component in components:
        if len(component) == 1:
            component_by_module[component[0]] = "none"
            continue
        cycle_number += 1
        component_id = f"cycle_{cycle_number:02d}"
        for module in component:
            component_by_module[module] = component_id

    with OUTPUT_PATH.open("w", encoding="utf-8", newline="") as output_file:
        writer = csv.writer(output_file, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "dependent_module",
                "provider_or_owner_module",
                "direct_function_pairs",
                "static_callsite_count",
                "written_owner_state_count",
                "borrowed_owner_state_count",
                "written_owner_states",
                "borrowed_owner_states",
                "relationship_evidence",
                "reverse_dependency_present",
                "cycle_component",
                "review_status",
            )
        )
        for edge in sorted(edges):
            edge_written_states = written_states.get(edge, set())
            edge_borrowed_states = borrowed_states.get(edge, set())
            evidence: list[str] = []
            if edge in direct_pairs:
                evidence.append("direct_caller_to_callee")
            if edge_written_states:
                evidence.append("state_writer_to_owner")
            if edge_borrowed_states:
                evidence.append("state_reader_to_owner")
            writer.writerow(
                (
                    edge[0],
                    edge[1],
                    direct_pairs[edge],
                    direct_callsites[edge],
                    len(edge_written_states),
                    len(edge_borrowed_states),
                    ";".join(sorted(edge_written_states)),
                    ";".join(sorted(edge_borrowed_states)),
                    ";".join(evidence),
                    "yes" if (edge[1], edge[0]) in edges else "no",
                    component_by_module[edge[0]]
                    if component_by_module[edge[0]] == component_by_module[edge[1]]
                    else "none",
                    "aggregated_from_reviewed_A2_A3_evidence",
                )
            )

    cycle_components = [component for component in components if len(component) > 1]
    print(f"wrote {len(edges)} dependency rows to {OUTPUT_PATH.relative_to(RESEARCH_ROOT)}")
    print(
        f"direct_edges={len(direct_pairs)} direct_function_pairs={sum(direct_pairs.values())} "
        f"static_callsites={sum(direct_callsites.values())}"
    )
    print(
        f"state_write_edges={len(written_states)} state_read_edges={len(borrowed_states)} "
        f"cycle_components={len(cycle_components)}"
    )
    for index, component in enumerate(cycle_components, 1):
        print(f"cycle_{index:02d}={';'.join(component)}")


if __name__ == "__main__":
    main()
