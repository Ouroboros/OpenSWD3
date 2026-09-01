#!/usr/bin/env python3
from __future__ import annotations

import csv
from collections import Counter
from pathlib import Path

RESEARCH_ROOT = Path(__file__).resolve().parents[1]
REVERSE_ENGINEERING_ROOT = RESEARCH_ROOT / "04-reverse-engineering"
INVENTORY_ROOT = REVERSE_ENGINEERING_ROOT / "inventory"
OWNERSHIP_INPUT = INVENTORY_ROOT / "module-function-ownership.tsv"
OUTPUT = INVENTORY_ROOT / "battle-function-workpack.tsv"
EXPECTED_CANDIDATE_COUNT = 422
EXPECTED_FIRST_ADDRESS = "0x00433AA0"
EXPECTED_LAST_ADDRESS = "0x00484500"
EXPECTED_FAMILY_COUNTS = {
    "transferred_action_and_asset_helpers": 15,
    "battle_record_leaves": 2,
    "setup_frame_input_and_resolution": 93,
    "script_dispatch_ai_and_targeting": 77,
    "actor_actions_effects_and_rendering": 194,
    "shared_battle_object_services": 41,
}
EXPECTED_CONFIDENCE_COUNTS = {
    "confirmed_boundary": 61,
    "medium": 361,
}
ALLOWED_CLOSURE_STATUSES = {
    "pending_audit",
    "assembly_exact",
    "platform_adapted",
    "unreachable_current_assets",
    "blocked_runtime_oracle",
}

# Add an entry only after the complete authoritative LST body and every external
# FUNCTION CHUNK have been audited, implemented, tested, evidenced and committed.
CLOSURES: dict[str, tuple[str, str]] = {
    "0x00433AA0": (
        "platform_adapted",
        "evidence/battle-image-point-query-00433aa0.md",
    ),
    "0x00433C40": (
        "platform_adapted",
        "evidence/battle-render-geometry-initialization-00433c40.md",
    ),
    "0x00433D70": (
        "platform_adapted",
        "evidence/battle-render-resource-cleanup-00433d70.md",
    ),
    "0x00433DC0": (
        "platform_adapted",
        "evidence/battle-render-surface-rebuild-00433dc0-00437e90.md",
    ),
    "0x00433E20": (
        "platform_adapted",
        "evidence/battle-primary-row-offsets-00433e20.md",
    ),
    "0x00433E90": (
        "platform_adapted",
        "evidence/battle-surface-row-offsets-00433e90.md",
    ),
    "0x00433F00": (
        "platform_adapted",
        "evidence/battle-render-auxiliary-buffer-release-00433f00.md",
    ),
    "0x00433F30": (
        "assembly_exact",
        "evidence/battle-host-surface-setup-00433f30.md",
    ),
    "0x00433F70": (
        "platform_adapted",
        "evidence/battle-literal-image-cyclic-rotation-00433f70.md",
    ),
    "0x004342E0": (
        "assembly_exact",
        "evidence/battle-render-rectangle-004342e0.md",
    ),
    "0x00434350": (
        "assembly_exact",
        "evidence/battle-line-raster-step-00434350.md",
    ),
    "0x00434420": (
        "platform_adapted",
        "evidence/battle-direction-vector-raster-step-00434420.md",
    ),
    "0x004344E0": (
        "platform_adapted",
        "evidence/battle-directional-surface-scan-004344e0.md",
    ),
    "0x00434790": (
        "platform_adapted",
        "evidence/battle-image-particle-frame-00434790.md",
    ),
    "0x00434DD0": (
        "platform_adapted",
        "evidence/battle-image-particle-spawn-00434dd0.md",
    ),
    "0x0044FFC0": (
        "assembly_exact",
        "evidence/battle-action-threshold-0044ffc0.md",
    ),
    "0x0044FFE0": (
        "platform_adapted",
        "evidence/battle-border-panel-0044ffe0.md",
    ),
    "0x00450270": (
        "platform_adapted",
        "evidence/battle-frame-zero-draw-00450270.md",
    ),
    "0x004502B0": (
        "platform_adapted",
        "evidence/battle-action-frame-draw-004502b0.md",
    ),
    "0x00450400": (
        "platform_adapted",
        "evidence/battle-indexed-action-frame-draw-00450400.md",
    ),
    "0x00450490": (
        "platform_adapted",
        "evidence/battle-explicit-width-frame-draw-00450490.md",
    ),
    "0x004504E0": (
        "platform_adapted",
        "evidence/battle-selected-resource-frame-draw-004504e0.md",
    ),
    "0x00450530": (
        "platform_adapted",
        "evidence/battle-layered-resource-frame-draw-00450530.md",
    ),
    "0x004505B0": (
        "platform_adapted",
        "evidence/battle-layered-frame-two-draw-004505b0.md",
    ),
    "0x00450630": (
        "platform_adapted",
        "evidence/battle-layered-low-word-width-00450630.md",
    ),
    "0x004506B0": (
        "platform_adapted",
        "evidence/battle-decimal-frame-draw-004506b0.md",
    ),
    "0x004507A0": (
        "platform_adapted",
        "evidence/battle-ten-place-decimal-coordinator-004507a0.md",
    ),
    "0x00450900": (
        "platform_adapted",
        "evidence/battle-decimal-place-draw-00450900.md",
    ),
    "0x004509D0": (
        "platform_adapted",
        "evidence/battle-prepared-action-frame-draw-004509d0.md",
    ),
    "0x00450A50": (
        "platform_adapted",
        "evidence/battle-color-vertical-fade-00450a50.md",
    ),
    "0x00450A80": (
        "platform_adapted",
        "evidence/battle-offset-action-frame-draw-00450a80.md",
    ),
    "0x00450B40": (
        "assembly_exact",
        "evidence/battle-action-record-clear-00450b40.md",
    ),
    "0x00450B60": (
        "platform_adapted",
        "evidence/battle-standalone-action-frame-draw-00450b60.md",
    ),
    "0x00450BD0": (
        "platform_adapted",
        "evidence/battle-selected-cached-frame-draw-00450bd0.md",
    ),
    "0x00450C50": (
        "platform_adapted",
        "evidence/battle-vertical-status-panel-00450c50.md",
    ),
    "0x00450F90": (
        "platform_adapted",
        "evidence/battle-status-indicator-animation-00450f90.md",
    ),
    "0x00451100": (
        "platform_adapted",
        "evidence/battle-scale-scan-animation-00451100.md",
    ),
    "0x004512B0": (
        "platform_adapted",
        "evidence/battle-scale-fill-panel-004512b0.md",
    ),
    "0x00451420": (
        "platform_adapted",
        "evidence/battle-action-rotation-cache-00451420.md",
    ),
    "0x00451540": (
        "platform_adapted",
        "evidence/battle-action-rotation-frame-draw-00451540.md",
    ),
    "0x004515E0": (
        "platform_adapted",
        "evidence/battle-action-rotation-playback-004515e0.md",
    ),
    "0x00451730": (
        "platform_adapted",
        "evidence/battle-action-rotation-cache-release-00451730.md",
    ),
    "0x004517A0": (
        "platform_adapted",
        "evidence/battle-actor-group-a-static-lifecycle-004517a0.md",
    ),
    "0x004517B0": (
        "platform_adapted",
        "evidence/battle-actor-group-a-vector-construction-004517b0.md",
    ),
    "0x004517E0": (
        "platform_adapted",
        "evidence/battle-actor-group-a-vector-destruction-004517e0.md",
    ),
    "0x00451800": (
        "platform_adapted",
        "evidence/battle-actor-group-b-static-lifecycle-00451800.md",
    ),
    "0x00451810": (
        "platform_adapted",
        "evidence/battle-actor-group-b-vector-construction-00451810.md",
    ),
    "0x00451840": (
        "platform_adapted",
        "evidence/battle-actor-group-b-vector-destruction-00451840.md",
    ),
    "0x00451860": (
        "platform_adapted",
        "evidence/battle-actor-singleton-static-lifecycle-00451860.md",
    ),
    "0x004518A0": (
        "platform_adapted",
        "evidence/battle-render-geometry-static-lifecycle-004518a0.md",
    ),
    "0x004518E0": (
        "platform_adapted",
        "evidence/battle-render-geometry-binding-static-thunk-004518e0.md",
    ),
    "0x004518F0": (
        "platform_adapted",
        "evidence/battle-render-geometry-binding-initialization-004518f0.md",
    ),
    "0x00451900": (
        "platform_adapted",
        "evidence/battle-file-static-lifecycle-00451900.md",
    ),
    "0x00451940": (
        "platform_adapted",
        "evidence/battle-background-initialization-00451940.md",
    ),
    "0x00451A20": (
        "platform_adapted",
        "evidence/battle-object-reset-00451a20.md",
    ),
    "0x00451B10": (
        "platform_adapted",
        "evidence/battle-startup-coordinator-00451b10.md",
    ),
    "0x004527E0": (
        "platform_adapted",
        "evidence/battle-visual-transition-004527e0.md",
    ),
    "0x004530A0": (
        "platform_adapted",
        "evidence/battle-surface-blend-004530a0.md",
    ),
    "0x00453200": (
        "platform_adapted",
        "evidence/battle-frame-coordinator-00453200.md",
    ),
    "0x00453580": (
        "platform_adapted",
        "evidence/battle-frame-effect-00453580.md",
    ),
    "0x004539B0": (
        "platform_adapted",
        "evidence/battle-action-dispatch-004539b0.md",
    ),
    "0x00455D60": (
        "platform_adapted",
        "evidence/battle-opponent-action-dispatch-00455d60.md",
    ),
    "0x00456680": (
        "platform_adapted",
        "evidence/battle-group-a-frame-00456680.md",
    ),
    "0x004576A0": (
        "platform_adapted",
        "evidence/battle-group-b-frame-004576a0.md",
    ),
    "0x004582B0": (
        "platform_adapted",
        "evidence/battle-effect-frame-004582b0.md",
    ),
    "0x00458DE0": (
        "platform_adapted",
        "evidence/battle-group-effect-frame-00458de0.md",
    ),
    "0x004599B0": (
        "platform_adapted",
        "evidence/battle-single-effect-frame-004599b0.md",
    ),
    "0x00459BF0": (
        "platform_adapted",
        "evidence/battle-intensity-effect-frame-00459bf0.md",
    ),
    "0x00459D10": (
        "platform_adapted",
        "evidence/battle-hud-frame-00459d10.md",
    ),
    "0x0045A980": (
        "platform_adapted",
        "evidence/battle-actor-ready-0045a980.md",
    ),
    "0x0045AA00": (
        "platform_adapted",
        "evidence/battle-final-actor-step-0045aa00.md",
    ),
    "0x0045ADF0": (
        "platform_adapted",
        "evidence/battle-post-action-0045adf0.md",
    ),
    "0x0045AF90": (
        "platform_adapted",
        "evidence/battle-frame-refresh-0045af90.md",
    ),
    "0x0045B0E0": (
        "platform_adapted",
        "evidence/battle-actor-metrics-0045b0e0.md",
    ),
    "0x0045B190": (
        "platform_adapted",
        "evidence/battle-actor-order-0045b190.md",
    ),
    "0x0045B280": (
        "platform_adapted",
        "evidence/battle-actor-priority-0045b280.md",
    ),
    "0x0045B5A0": (
        "platform_adapted",
        "evidence/battle-group-b-order-0045b5a0.md",
    ),
    "0x0045B5E0": (
        "platform_adapted",
        "evidence/battle-actor-frame-sequence-0045b5e0.md",
    ),
    "0x0045B630": (
        "platform_adapted",
        "evidence/battle-global-reset-0045b630.md",
    ),
    "0x0045BD10": (
        "platform_adapted",
        "evidence/battle-full-frame-darkening-0045bd10.md",
    ),
    "0x0045BD90": (
        "platform_adapted",
        "evidence/battle-effect-actor-shift-0045bd90.md",
    ),
    "0x0045C010": (
        "platform_adapted",
        "evidence/battle-effect-coordinator-0045c010.md",
    ),
    "0x0045D180": (
        "platform_adapted",
        "evidence/battle-player-item-quantity-0045d180.md",
    ),
    "0x0045D250": (
        "platform_adapted",
        "evidence/battle-player-item-order-0045d250.md",
    ),
    "0x0045D2A0": (
        "platform_adapted",
        "evidence/battle-party-item-order-0045d2a0.md",
    ),
    "0x0045D2F0": (
        "platform_adapted",
        "evidence/battle-color-accumulation-0045d2f0.md",
    ),
    "0x0045D3E0": (
        "platform_adapted",
        "evidence/battle-color-initialization-0045d3e0.md",
    ),
    "0x0045D490": (
        "platform_adapted",
        "evidence/battle-pre-frame-0045d490.md",
    ),
    "0x0045D690": (
        "platform_adapted",
        "evidence/battle-pair-transition-0045d690.md",
    ),
    "0x0045D810": (
        "platform_adapted",
        "evidence/battle-animation-collision-0045d810.md",
    ),
    "0x0045D8F0": (
        "platform_adapted",
        "evidence/battle-debug-hotkeys-0045d8f0.md",
    ),
    "0x0045DEE0": (
        "platform_adapted",
        "evidence/battle-debug-overlay-0045dee0.md",
    ),
    "0x0045E580": (
        "platform_adapted",
        "evidence/battle-outcome-resolution-0045e580.md",
    ),
    "0x0045E660": (
        "platform_adapted",
        "evidence/battle-context-prompt-0045e660.md",
    ),
    "0x0045E7D0": (
        "platform_adapted",
        "evidence/battle-vertical-shift-0045e7d0.md",
    ),
    "0x0045E9C0": (
        "platform_adapted",
        "evidence/battle-outcome-finalization-0045e9c0.md",
    ),
    "0x0045EA30": (
        "platform_adapted",
        "evidence/battle-runtime-shutdown-0045ea30.md",
    ),
    "0x0045EA80": (
        "platform_adapted",
        "evidence/battle-retreat-commit-0045ea80.md",
    ),
    "0x0045EB40": (
        "platform_adapted",
        "evidence/battle-pending-action-commit-0045eb40.md",
    ),
    "0x0045EC60": (
        "platform_adapted",
        "evidence/battle-reward-item-slot-0045ec60.md",
    ),
    "0x0045EC80": (
        "platform_adapted",
        "evidence/battle-frame-completion-0045ec80.md",
    ),
    "0x0045EDF0": (
        "platform_adapted",
        "evidence/battle-attack-order-entry-0045edf0.md",
    ),
    "0x0045EE70": (
        "platform_adapted",
        "evidence/battle-attack-order-insert-0045ee70.md",
    ),
    "0x0045EFB0": (
        "platform_adapted",
        "evidence/battle-attack-order-remove-0045efb0.md",
    ),
    "0x0045F020": (
        "platform_adapted",
        "evidence/battle-attack-order-dequeue-0045f020.md",
    ),
    "0x0045F0F0": (
        "platform_adapted",
        "evidence/battle-render-geometry-binding-object-initialization-0045f0f0.md",
    ),
    "0x0045F130": (
        "platform_adapted",
        "evidence/battle-definition-archive-header-load-0045f130.md",
    ),
    "0x0045F1B0": (
        "platform_adapted",
        "evidence/battle-definition-archive-record-load-0045f1b0.md",
    ),
    "0x0045F2A0": (
        "platform_adapted",
        "evidence/battle-input-dispatch-0045f2a0.md",
    ),
    "0x0045FC60": (
        "platform_adapted",
        "evidence/battle-frame-input-resolution-0045fc60.md",
    ),
    "0x00460C40": (
        "platform_adapted",
        "evidence/battle-menu-selection-retreat-00460c40.md",
    ),
    "0x00461240": (
        "platform_adapted",
        "evidence/battle-menu-selection-advance-00461240.md",
    ),
    "0x00461900": (
        "platform_adapted",
        "evidence/battle-menu-page-retreat-00461900.md",
    ),
    "0x00461A30": (
        "platform_adapted",
        "evidence/battle-menu-page-advance-00461a30.md",
    ),
    "0x00461C10": (
        "platform_adapted",
        "evidence/battle-menu-input-finalize-00461c10.md",
    ),
    "0x004620D0": (
        "platform_adapted",
        "evidence/battle-target-selection-entry-004620d0.md",
    ),
    "0x00462320": (
        "platform_adapted",
        "evidence/battle-actor-action-cycle-00462320.md",
    ),
    "0x004623A0": (
        "platform_adapted",
        "evidence/battle-actor-action-reverse-cycle-004623a0.md",
    ),
    "0x00462420": (
        "platform_adapted",
        "evidence/battle-actor-action-commit-00462420.md",
    ),
    "0x004624C0": (
        "platform_adapted",
        "evidence/battle-actor-action-candidate-availability-004624c0.md",
    ),
    "0x00462510": (
        "platform_adapted",
        "evidence/battle-menu-context-advance-00462510.md",
    ),
    "0x00462630": (
        "platform_adapted",
        "evidence/battle-menu-context-retreat-00462630.md",
    ),
    "0x00462740": (
        "platform_adapted",
        "evidence/battle-target-selection-refresh-00462740.md",
    ),
    "0x00464270": (
        "platform_adapted",
        "evidence/battle-selection-frame-00464270.md",
    ),
    "0x00464CC0": (
        "platform_adapted",
        "evidence/battle-actor-target-preparation-00464cc0.md",
    ),
    "0x00464DA0": (
        "platform_adapted",
        "evidence/battle-input-record-priming-00464da0.md",
    ),
    "0x00464DD0": (
        "platform_adapted",
        "evidence/battle-available-actor-cycle-00464dd0.md",
    ),
    "0x00464E40": (
        "platform_adapted",
        "evidence/battle-available-actor-reverse-cycle-00464e40.md",
    ),
    "0x00464E90": (
        "platform_adapted",
        "evidence/battle-action-mode-refresh-00464e90.md",
    ),
    "0x00465090": (
        "platform_adapted",
        "evidence/battle-group-b-target-cycle-00465090.md",
    ),
    "0x00465170": (
        "platform_adapted",
        "evidence/battle-group-a-target-cycle-00465170.md",
    ),
    "0x004651D0": (
        "platform_adapted",
        "evidence/battle-action-summary-004651d0.md",
    ),
    "0x00465480": (
        "platform_adapted",
        "evidence/battle-list-frame-00465480.md",
    ),
    "0x004655B0": (
        "platform_adapted",
        "evidence/battle-list-contents-004655b0.md",
    ),
    "0x004659C0": (
        "platform_adapted",
        "evidence/battle-grid-frame-004659c0.md",
    ),
    "0x00465E50": (
        "platform_adapted",
        "evidence/battle-alternate-grid-frame-00465e50.md",
    ),
    "0x00466190": (
        "platform_adapted",
        "evidence/battle-mode-grid-frame-00466190.md",
    ),
    "0x00466500": (
        "platform_adapted",
        "evidence/battle-narrow-grid-frame-00466500.md",
    ),
    "0x004667B0": (
        "platform_adapted",
        "evidence/battle-guard-panel-frame-004667b0.md",
    ),
    "0x00466950": (
        "platform_adapted",
        "evidence/battle-selection-hint-frame-00466950.md",
    ),
    "0x00466C00": (
        "platform_adapted",
        "evidence/battle-control-panel-frame-00466c00.md",
    ),
    "0x00466F70": (
        "platform_adapted",
        "evidence/battle-message-phase-00466f70.md",
    ),
    "0x00467710": (
        "platform_adapted",
        "evidence/battle-victory-rewards-00467710.md",
    ),
    "0x00467AC0": (
        "platform_adapted",
        "evidence/battle-level-up-panel-00467ac0.md",
    ),
    "0x00467C50": (
        "platform_adapted",
        "evidence/battle-level-advancement-00467c50.md",
    ),
    "0x00467F00": (
        "platform_adapted",
        "evidence/battle-level-growth-panel-00467f00.md",
    ),
    "0x00468930": (
        "platform_adapted",
        "evidence/battle-growth-caption-00468930.md",
    ),
    "0x00468AD0": (
        "platform_adapted",
        "evidence/battle-growth-completion-caption-00468ad0.md",
    ),
    "0x00468C80": (
        "platform_adapted",
        "evidence/battle-growth-actor-selection-00468c80.md",
    ),
    "0x00468ED0": (
        "platform_adapted",
        "evidence/battle-growth-item-completion-panel-00468ed0.md",
    ),
    "0x00468FF0": (
        "platform_adapted",
        "evidence/battle-growth-item-result-selection-00468ff0.md",
    ),
    "0x00469080": (
        "platform_adapted",
        "evidence/battle-victory-item-list-panel-00469080.md",
    ),
    "0x00469220": (
        "platform_adapted",
        "evidence/battle-defeat-panel-00469220.md",
    ),
    "0x00469340": (
        "platform_adapted",
        "evidence/battle-talisman-result-panel-00469340.md",
    ),
    "0x004694E0": (
        "assembly_exact",
        "evidence/battle-transition-control-selection-004694e0.md",
    ),
    "0x00469550": (
        "platform_adapted",
        "evidence/battle-text-panel-00469550.md",
    ),
    "0x00469620": (
        "assembly_exact",
        "evidence/battle-transition-stage-advance-00469620.md",
    ),
    "0x00469650": (
        "platform_adapted",
        "evidence/battle-debug-status-panel-00469650.md",
    ),
    "0x004698E0": (
        "platform_adapted",
        "evidence/battle-text-message-004698e0.md",
    ),
    "0x00469960": (
        "platform_adapted",
        "evidence/battle-text-message-frame-00469960.md",
    ),
    "0x00469D20": (
        "platform_adapted",
        "evidence/battle-script-dispatch-00469d20.md",
    ),
    "0x0046E090": (
        "assembly_exact",
        "evidence/battle-group-a-primary-skip-getter-0046e090.md",
    ),
    "0x0046E0A0": (
        "assembly_exact",
        "evidence/battle-group-a-secondary-skip-getter-0046e0a0.md",
    ),
    "0x0046E0B0": (
        "platform_adapted",
        "evidence/battle-script-window-load-0046e0b0.md",
    ),
    "0x0046E1E0": (
        "platform_adapted",
        "evidence/battle-script-page-load-0046e1e0.md",
    ),
    "0x0046E260": (
        "platform_adapted",
        "evidence/battle-script-shutdown-0046e260.md",
    ),
    "0x0046E290": (
        "platform_adapted",
        "evidence/battle-script-curve-sample-0046e290.md",
    ),
    "0x0046E490": (
        "platform_adapted",
        "evidence/battle-group-a-element-construction-0046e490.md",
    ),
    "0x0046E4D0": (
        "platform_adapted",
        "evidence/battle-group-a-element-destruction-0046e4d0.md",
    ),
    "0x0046E520": (
        "platform_adapted",
        "evidence/battle-actor-progress-0046e520.md",
    ),
    "0x0046E6A0": (
        "platform_adapted",
        "evidence/battle-group-a-workspace-reset-0046e6a0.md",
    ),
    "0x0046E730": (
        "platform_adapted",
        "evidence/battle-group-a-configuration-0046e730.md",
    ),
    "0x0046E850": (
        "platform_adapted",
        "evidence/battle-group-a-resource-pair-0046e850.md",
    ),
    "0x0046E870": (
        "platform_adapted",
        "evidence/battle-group-a-value-pair-0046e870.md",
    ),
    "0x0046E890": (
        "platform_adapted",
        "evidence/battle-group-a-summon-materialization-0046e890.md",
    ),
    "0x0046E9C0": (
        "platform_adapted",
        "evidence/battle-group-a-npc-materialization-0046e9c0.md",
    ),
    "0x0046EBB0": (
        "platform_adapted",
        "evidence/battle-group-a-attribute-aggregation-0046ebb0.md",
    ),
    "0x0046EE60": (
        "platform_adapted",
        "evidence/battle-group-a-attribute-effect-0046ee60.md",
    ),
    "0x0046F030": (
        "platform_adapted",
        "evidence/battle-group-a-embedded-profile-application-0046f030.md",
    ),
    "0x0046F1F0": (
        "platform_adapted",
        "evidence/battle-group-a-item-effect-application-0046f1f0.md",
    ),
    "0x0046F5B0": (
        "platform_adapted",
        "evidence/battle-group-a-reward-profile-application-0046f5b0.md",
    ),
    "0x0046F6E0": (
        "platform_adapted",
        "evidence/battle-group-a-effect-reward-application-0046f6e0.md",
    ),
    "0x0046F850": (
        "platform_adapted",
        "evidence/battle-group-a-growth-reward-selection-0046f850.md",
    ),
    "0x0046F8C0": (
        "platform_adapted",
        "evidence/battle-group-a-action-execution-0046f8c0.md",
    ),
    "0x0046FF00": (
        "platform_adapted",
        "evidence/battle-group-a-profile-mode-selection-0046ff00.md",
    ),
    "0x0046FFE0": (
        "platform_adapted",
        "evidence/battle-actor-list-index-commit-0046ffe0.md",
    ),
    "0x0046FFF0": (
        "platform_adapted",
        "evidence/battle-group-a-final-processing-0046fff0.md",
    ),
    "0x00470180": (
        "platform_adapted",
        "evidence/battle-actor-list-query-00470180.md",
    ),
    "0x004702E0": (
        "platform_adapted",
        "evidence/battle-actor-list-count-004702e0.md",
    ),
    "0x00470380": (
        "platform_adapted",
        "evidence/battle-actor-list-state-processing-00470380.md",
    ),
    "0x004705C0": (
        "platform_adapted",
        "evidence/battle-actor-list-profile-application-004705c0.md",
    ),
    "0x004707B0": (
        "platform_adapted",
        "evidence/battle-actor-profile-preparation-004707b0.md",
    ),
    "0x00470820": (
        "platform_adapted",
        "evidence/battle-actor-list-action-00470820.md",
    ),
    "0x00470890": (
        "platform_adapted",
        "evidence/battle-actor-list-action-refresh-00470890.md",
    ),
    "0x004708C0": (
        "platform_adapted",
        "evidence/battle-actor-mode-four-finalization-004708c0.md",
    ),
    "0x00470900": (
        "platform_adapted",
        "evidence/battle-actor-resource-list-commit-00470900.md",
    ),
    "0x00470910": (
        "platform_adapted",
        "evidence/battle-actor-resource-list-query-00470910.md",
    ),
    "0x00470A10": (
        "platform_adapted",
        "evidence/battle-actor-resource-list-count-00470a10.md",
    ),
    "0x00470AC0": (
        "platform_adapted",
        "evidence/battle-actor-resource-selection-00470ac0.md",
    ),
    "0x00470E20": (
        "platform_adapted",
        "evidence/battle-actor-resource-release-00470e20.md",
    ),
    "0x00470F70": (
        "platform_adapted",
        "evidence/battle-actor-flagged-resource-query-00470f70.md",
    ),
    "0x00470FE0": (
        "platform_adapted",
        "evidence/battle-actor-mode-resource-query-00470fe0.md",
    ),
    "0x00471080": (
        "platform_adapted",
        "evidence/battle-actor-mode-resource-count-00471080.md",
    ),
    "0x004710D0": (
        "platform_adapted",
        "evidence/battle-target-phase-start-004710d0.md",
    ),
    "0x00471270": (
        "platform_adapted",
        "evidence/battle-target-phase-advance-00471270.md",
    ),
    "0x004714B0": (
        "platform_adapted",
        "evidence/battle-turn-commit-chance-004714b0.md",
    ),
    "0x00471540": (
        "platform_adapted",
        "evidence/battle-turn-gate-00471540.md",
    ),
    "0x004717D0": (
        "platform_adapted",
        "evidence/battle-actor-action-thirty-override-004717d0.md",
    ),
    "0x004717E0": (
        "platform_adapted",
        "evidence/battle-actor-action-four-override-004717e0.md",
    ),
    "0x004717F0": (
        "platform_adapted",
        "evidence/battle-action-thirteen-frame-004717f0.md",
    ),
    "0x00471AD0": (
        "platform_adapted",
        "evidence/battle-action-fourteen-frame-00471ad0.md",
    ),
    "0x00471D60": (
        "platform_adapted",
        "evidence/battle-summon-frame-00471d60.md",
    ),
    "0x00471FC0": (
        "platform_adapted",
        "evidence/battle-target-phase-spawn-frame-00471fc0.md",
    ),
    "0x004721F0": (
        "platform_adapted",
        "evidence/battle-action-twenty-three-frame-004721f0.md",
    ),
    "0x00472430": (
        "platform_adapted",
        "evidence/battle-action-twenty-three-message-00472430.md",
    ),
    "0x004724D0": (
        "platform_adapted",
        "evidence/battle-action-twenty-four-frame-004724d0.md",
    ),
    "0x00472710": (
        "platform_adapted",
        "evidence/battle-action-twenty-five-ready-00472710.md",
    ),
    "0x00472730": (
        "platform_adapted",
        "evidence/battle-target-phase-check-00472730.md",
    ),
    "0x004728D0": (
        "platform_adapted",
        "evidence/battle-actor-retreat-ready-004728d0.md",
    ),
    "0x004728E0": (
        "platform_adapted",
        "evidence/battle-action-twenty-seven-frame-004728e0.md",
    ),
    "0x00472C70": (
        "platform_adapted",
        "evidence/battle-reward-scale-00472c70.md",
    ),
    "0x00472CE0": (
        "platform_adapted",
        "evidence/battle-dual-record-action-00472ce0.md",
    ),
    "0x00473010": (
        "platform_adapted",
        "evidence/battle-special-five-hundred-00473010.md",
    ),
    "0x004731A0": (
        "platform_adapted",
        "evidence/battle-special-four-oh-five-004731a0.md",
    ),
    "0x004735B0": (
        "platform_adapted",
        "evidence/battle-special-four-oh-six-004735b0.md",
    ),
    "0x00473C10": (
        "platform_adapted",
        "evidence/battle-special-four-hundred-00473c10.md",
    ),
    "0x004745B0": (
        "platform_adapted",
        "evidence/battle-action-four-effect-004745b0.md",
    ),
    "0x00474B60": (
        "platform_adapted",
        "evidence/battle-target-property-chance-00474b60.md",
    ),
    "0x00474BA0": (
        "platform_adapted",
        "evidence/battle-action-four-oh-two-particles-00474ba0.md",
    ),
    "0x00474E60": (
        "platform_adapted",
        "evidence/battle-special-four-oh-nine-00474e60.md",
    ),
    "0x00474FC0": (
        "platform_adapted",
        "evidence/battle-target-effect-00474fc0.md",
    ),
    "0x004750C0": (
        "platform_adapted",
        "evidence/battle-group-a-actor-cleanup-004750c0.md",
    ),
    "0x00475160": (
        "platform_adapted",
        "evidence/battle-actor-message-percent-refresh-00475160.md",
    ),
    "0x00475180": (
        "platform_adapted",
        "evidence/battle-group-a-resource-cleanup-00475180.md",
    ),
    "0x004751C0": (
        "platform_adapted",
        "evidence/battle-target-ready-004751c0.md",
    ),
    "0x00475560": (
        "platform_adapted",
        "evidence/battle-group-b-element-construction-00475560.md",
    ),
    "0x00475590": (
        "platform_adapted",
        "evidence/battle-group-b-element-destruction-00475590.md",
    ),
    "0x004755E0": (
        "platform_adapted",
        "evidence/battle-group-b-actor-progress-004755e0.md",
    ),
    "0x00475720": (
        "platform_adapted",
        "evidence/battle-group-b-action-configuration-00475720.md",
    ),
    "0x00475820": (
        "platform_adapted",
        "evidence/battle-group-b-action-reconfiguration-00475820.md",
    ),
    "0x00475870": (
        "platform_adapted",
        "evidence/battle-group-b-coordinate-offsets-00475870.md",
    ),
    "0x004758A0": (
        "platform_adapted",
        "evidence/battle-group-b-action-execution-004758a0.md",
    ),
    "0x00476080": (
        "platform_adapted",
        "evidence/battle-group-b-opponent-mode-00476080.md",
    ),
    "0x00476140": (
        "platform_adapted",
        "evidence/battle-group-b-action-profile-flag-00476140.md",
    ),
    "0x00476160": (
        "platform_adapted",
        "evidence/battle-group-b-action-composition-00476160.md",
    ),
    "0x004761D0": (
        "platform_adapted",
        "evidence/battle-group-b-action-profile-mode-004761d0.md",
    ),
    "0x00476250": (
        "platform_adapted",
        "evidence/battle-group-b-action-profile-selection-00476250.md",
    ),
    "0x004762F0": (
        "platform_adapted",
        "evidence/battle-group-b-reward-item-selection-004762f0.md",
    ),
    "0x00476330": (
        "platform_adapted",
        "evidence/battle-group-b-status-action-00476330.md",
    ),
    "0x004763D0": (
        "platform_adapted",
        "evidence/battle-group-b-action-seventeen-frame-004763d0.md",
    ),
    "0x00476600": (
        "platform_adapted",
        "evidence/battle-group-b-action-item-selection-00476600.md",
    ),
    "0x00476780": (
        "platform_adapted",
        "evidence/battle-group-b-action-item-option-00476780.md",
    ),
    "0x00476860": (
        "platform_adapted",
        "evidence/battle-group-b-action-item-special-option-00476860.md",
    ),
    "0x004768D0": (
        "platform_adapted",
        "evidence/battle-group-b-action-six-target-availability-004768d0.md",
    ),
    "0x00476900": (
        "platform_adapted",
        "evidence/battle-group-b-opponent-wave-parameters-00476900.md",
    ),
    "0x00476920": (
        "platform_adapted",
        "evidence/battle-group-b-script-resource-parameters-00476920.md",
    ),
    "0x004769A0": (
        "platform_adapted",
        "evidence/battle-group-b-script-action-item-parameters-004769a0.md",
    ),
    "0x00476A10": (
        "platform_adapted",
        "evidence/battle-group-b-script-special-action-item-parameters-00476a10.md",
    ),
    "0x00476A60": (
        "platform_adapted",
        "evidence/battle-group-b-resource-cleanup-00476a60.md",
    ),
    "0x00476A80": (
        "platform_adapted",
        "evidence/battle-mon-profile-load-00476a80.md",
    ),
    "0x00476DB0": (
        "platform_adapted",
        "evidence/battle-mon-definition-load-00476db0.md",
    ),
    "0x00477290": (
        "platform_adapted",
        "evidence/battle-level-requirement-load-00477290.md",
    ),
    "0x00477400": (
        "platform_adapted",
        "evidence/battle-level-profile-load-00477400.md",
    ),
    "0x004776A0": (
        "platform_adapted",
        "evidence/battle-database-shutdown-004776a0.md",
    ),
    "0x004776F0": (
        "assembly_exact",
        "evidence/battle-fixed-object-reset-004776f0.md",
    ),
    "0x00477710": (
        "platform_adapted",
        "evidence/battle-fixed-count-chain-00477710.md",
    ),
}


def address_value(value: str) -> int:
    return int(value, 16)


def family(address: int) -> str:
    if 0x00433AA0 <= address <= 0x00434DD0:
        return "transferred_action_and_asset_helpers"
    if 0x0044FFC0 <= address <= 0x0044FFE0:
        return "battle_record_leaves"
    if 0x00450270 <= address <= 0x0045FC60:
        return "setup_frame_input_and_resolution"
    if 0x00460C40 <= address <= 0x0046FFF0:
        return "script_dispatch_ai_and_targeting"
    if 0x00470180 <= address <= 0x0047FC40:
        return "actor_actions_effects_and_rendering"
    if 0x004800F0 <= address <= 0x00484500:
        return "shared_battle_object_services"
    return "ownership_review_required"


def main() -> None:
    with OWNERSHIP_INPUT.open(encoding="utf-8", newline="") as source:
        rows = [
            row
            for row in csv.DictReader(source, delimiter="\t")
            if row["module_candidate"] == "battle"
            and row["code_origin"] == "game"
        ]
    if len(rows) != EXPECTED_CANDIDATE_COUNT:
        raise SystemExit(
            f"expected {EXPECTED_CANDIDATE_COUNT} battle candidates, "
            f"got {len(rows)}"
        )

    rows.sort(key=lambda row: address_value(row["address"]))
    addresses = {row["address"] for row in rows}
    if len(addresses) != len(rows):
        raise SystemExit("duplicate battle candidate address")
    if rows[0]["address"] != EXPECTED_FIRST_ADDRESS:
        raise SystemExit(f"unexpected first battle address: {rows[0]['address']}")
    if rows[-1]["address"] != EXPECTED_LAST_ADDRESS:
        raise SystemExit(f"unexpected last battle address: {rows[-1]['address']}")

    unknown_closures = sorted(set(CLOSURES) - addresses)
    if unknown_closures:
        raise SystemExit(f"closure address is outside battle scope: {unknown_closures}")

    output_rows = []
    for audit_order, row in enumerate(rows, start=1):
        address = address_value(row["address"])
        closure_status, closure_evidence = CLOSURES.get(
            row["address"], ("pending_audit", "")
        )
        if closure_status not in ALLOWED_CLOSURE_STATUSES:
            raise SystemExit(
                f"unsupported closure status for {row['address']}: "
                f"{closure_status}"
            )
        if closure_status == "pending_audit" and closure_evidence:
            raise SystemExit(
                f"pending closure unexpectedly has evidence: {row['address']}"
            )
        if closure_status != "pending_audit" and not closure_evidence:
            raise SystemExit(f"closed entry lacks evidence: {row['address']}")
        if closure_evidence:
            evidence_path = (REVERSE_ENGINEERING_ROOT / closure_evidence).resolve()
            if REVERSE_ENGINEERING_ROOT.resolve() not in evidence_path.parents:
                raise SystemExit(
                    f"closure evidence escapes reverse-engineering root: "
                    f"{row['address']}"
                )
            if not evidence_path.is_file():
                raise SystemExit(
                    f"closure evidence does not exist for {row['address']}: "
                    f"{closure_evidence}"
                )
        output_rows.append(
            (
                audit_order,
                row["address"],
                row["ida_name_navigation_only"],
                family(address),
                row["candidate_confidence"],
                row["assembly_direct_callers_address_count"],
                row["assembly_direct_callees_address_count"],
                row["review_status"],
                closure_status,
                closure_evidence,
                "ownership and call-graph fields are navigation only; close from full LST body and external chunks",
            )
        )

    invalid_families = [
        row[1] for row in output_rows if row[3] == "ownership_review_required"
    ]
    if invalid_families:
        raise SystemExit(f"battle family is unresolved: {invalid_families}")

    family_counts = Counter(row[3] for row in output_rows)
    if family_counts != EXPECTED_FAMILY_COUNTS:
        raise SystemExit(f"unexpected battle family counts: {dict(family_counts)}")
    confidence_counts = Counter(row[4] for row in output_rows)
    if confidence_counts != EXPECTED_CONFIDENCE_COUNTS:
        raise SystemExit(
            f"unexpected battle confidence counts: {dict(confidence_counts)}"
        )

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as destination:
        writer = csv.writer(destination, delimiter="\t", lineterminator="\n")
        writer.writerow(
            (
                "audit_order",
                "address",
                "navigation_name",
                "entry_family",
                "candidate_confidence_navigation",
                "direct_callers_navigation",
                "direct_callees_navigation",
                "prior_review_navigation",
                "closure_status",
                "closure_evidence",
                "closure_rule",
            )
        )
        writer.writerows(output_rows)

    closed_count = sum(row[8] != "pending_audit" for row in output_rows)
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(output_rows)} rows)")
    if closed_count == EXPECTED_CANDIDATE_COUNT:
        print(
            f"closure {closed_count}/{EXPECTED_CANDIDATE_COUNT}; "
            "all candidates independently closed from full LST bodies"
        )
    else:
        print(
            f"closure {closed_count}/{EXPECTED_CANDIDATE_COUNT}; "
            "remaining candidates require independent LST audit"
        )


if __name__ == "__main__":
    main()
