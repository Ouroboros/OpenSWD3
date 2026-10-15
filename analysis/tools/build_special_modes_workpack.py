#!/usr/bin/env python3
"""Build the B9 special-modes audit workpack from the frozen ownership inventory."""

from __future__ import annotations

import csv
from pathlib import Path

TOOL_ROOT = Path(__file__).resolve().parent
RESEARCH_ROOT = TOOL_ROOT.parent
INVENTORY_ROOT = RESEARCH_ROOT / "04-reverse-engineering" / "inventory"
OWNERSHIP_INPUT = INVENTORY_ROOT / "module-function-ownership.tsv"
OUTPUT = INVENTORY_ROOT / "special-modes-function-workpack.tsv"
EXPECTED_CANDIDATE_COUNT = 227
CLOSURES = {
    "0x00411700": (
        "pending",
        "evidence/object-label-panel-00411700.md",
    ),
    "0x0040F890": (
        "pending",
        "evidence/party-dialog-main-0040f890.md",
    ),
    "0x00410730": (
        "pending",
        "evidence/party-dialog-page-00410730.md",
    ),
    "0x00410490": (
        "pending",
        "evidence/party-dialog-row-replace-00410490.md",
    ),
    "0x00410600": (
        "pending",
        "evidence/party-dialog-row-00410600.md",
    ),
    "0x004103C0": (
        "pending",
        "evidence/party-dialog-columns-004103c0.md",
    ),
    "0x00411030": (
        "pending",
        "evidence/party-member-field-setter-00411030.md",
    ),
    "0x004112B0": (
        "pending",
        "evidence/party-member-field-getter-004112b0.md",
    ),
    "0x00406F70": (
        "pending",
        "evidence/high-priority-story-flags-00406f70.md",
    ),
    "0x00406EB0": (
        "pending",
        "evidence/high-priority-common-input-00406eb0.md",
    ),
    "0x00406E30": (
        "pending",
        "evidence/high-priority-menu-frame-00406e30.md",
    ),
    "0x00406E00": (
        "pending",
        "evidence/save-preview-cleanup-00406e00.md",
    ),
    "0x00406D30": (
        "pending",
        "evidence/input-menu-save-preview-reset-00406d30.md",
    ),
    "0x0044EA60": (
        "pending",
        "evidence/special-mode-mode-one-frame-0044ea60.md",
    ),
    "0x0044DBC0": (
        "pending",
        "evidence/special-mode-mode-one-pointer-input-0044dbc0.md",
    ),
    "0x0044F920": (
        "pending",
        "evidence/special-mode-mode-one-alternate-input-0044f920.md",
    ),
    "0x0044E4A0": (
        "pending",
        "evidence/special-mode-mode-one-confirm-0044e4a0.md",
    ),
    "0x0044E330": (
        "pending",
        "evidence/special-mode-mode-one-increase-0044e330.md",
    ),
    "0x0044E260": (
        "pending",
        "evidence/special-mode-mode-one-decrease-0044e260.md",
    ),
    "0x0044E1B0": (
        "pending",
        "evidence/special-mode-mode-one-page-retreat-0044e1b0.md",
    ),
    "0x0044E0C0": (
        "pending",
        "evidence/special-mode-mode-one-page-advance-0044e0c0.md",
    ),
    "0x0044DFF0": (
        "pending",
        "evidence/special-mode-mode-one-retreat-0044dff0.md",
    ),
    "0x0044DF00": (
        "pending",
        "evidence/special-mode-mode-one-advance-0044df00.md",
    ),
    "0x0044FD30": (
        "pending",
        "evidence/special-mode-attribute-delta-render-0044fd30.md",
    ),
    "0x0044E9D0": (
        "pending",
        "evidence/special-mode-level-exit-0044e9d0.md",
    ),
    "0x0044FAF0": (
        "pending",
        "evidence/special-mode-candidate-attribute-comparison-0044faf0.md",
    ),
    "0x0044F800": (
        "pending",
        "evidence/special-mode-workspace-build-0044f800.md",
    ),
    "0x0044FA40": (
        "pending",
        "evidence/special-mode-equipment-contribution-0044fa40.md",
    ),
    "0x0044F8E0": (
        "pending",
        "evidence/special-mode-workspace-record-release-0044f8e0.md",
    ),
    "0x0044F7D0": (
        "pending",
        "evidence/special-mode-visible-count-0044f7d0.md",
    ),
    "0x0044F770": (
        "pending",
        "evidence/special-mode-record-weight-0044f770.md",
    ),
    "0x0044FF90": (
        "pending",
        "evidence/special-mode-packed-value-0044ff90.md",
    ),
    "0x0044DA40": (
        "pending",
        "evidence/special-mode-runtime-cleanup-0044da40.md",
    ),
    "0x0044D920": (
        "pending",
        "evidence/special-mode-runtime-initialization-0044d920.md",
    ),
    "0x0044DAA0": (
        "pending",
        "evidence/special-mode-action-set-initialization-0044daa0.md",
    ),
    "0x0044D6E0": (
        "pending",
        "evidence/guardian-attribute-application-0044d6e0.md",
    ),
    "0x0044D6B0": (
        "pending",
        "evidence/player-item-index-0044d6b0.md",
    ),
    "0x0044D680": (
        "pending",
        "evidence/player-item-masked-lookup-0044d680.md",
    ),
    "0x0044D650": (
        "pending",
        "evidence/fixed-item-slot-lookup-0044d650.md",
    ),
    "0x0044D620": (
        "pending",
        "evidence/item-record-detach-0044d620.md",
    ),
    "0x0044D5D0": (
        "pending",
        "evidence/missing-item-record-create-0044d5d0.md",
    ),
    "0x0044D5A0": (
        "pending",
        "evidence/player-item-chain-release-0044d5a0.md",
    ),
    "0x0044D520": (
        "pending",
        "evidence/player-item-chain-deep-clone-0044d520.md",
    ),
    "0x0044D4E0": (
        "pending",
        "evidence/player-item-quantity-merge-0044d4e0.md",
    ),
    "0x0044D2D0": (
        "pending",
        "evidence/player-item-dual-quantity-0044d2d0.md",
    ),
    "0x0044D0F0": (
        "pending",
        "evidence/shared-item-quantity-0044d0f0.md",
    ),
    "0x0044D070": (
        "pending",
        "evidence/system-menu-record-row-0044d070.md",
    ),
    "0x0044D050": (
        "pending",
        "evidence/system-menu-record-pointer-0044d050.md",
    ),
    "0x0044D010": (
        "pending",
        "evidence/system-menu-record-visible-count-0044d010.md",
    ),
    "0x0044C160": (
        "pending",
        "evidence/system-menu-frame-update-0044c160.md",
    ),
    "0x0044C0E0": (
        "pending",
        "evidence/system-menu-right-click-return-0044c0e0.md",
    ),
    "0x0044BDA0": (
        "pending",
        "evidence/system-menu-confirm-0044bda0.md",
    ),
    "0x0044BBD0": (
        "pending",
        "evidence/system-menu-down-arrow-0044bbd0.md",
    ),
    "0x0044BA20": (
        "pending",
        "evidence/system-menu-up-arrow-0044ba20.md",
    ),
    "0x0044B930": (
        "pending",
        "evidence/system-menu-page-up-0044b930.md",
    ),
    "0x0044B840": (
        "pending",
        "evidence/special-mode-system-menu-page-down-0044b840.md",
    ),
    "0x0044B6E0": (
        "pending",
        "evidence/system-menu-left-arrow-0044b6e0.md",
    ),
    "0x0044B560": (
        "pending",
        "evidence/system-menu-right-arrow-0044b560.md",
    ),
    "0x0044B070": (
        "pending",
        "evidence/system-menu-input-0044b070.md",
    ),
    "0x0044B010": (
        "pending",
        "evidence/system-menu-record-list-close-0044b010.md",
    ),
    "0x0044AF30": (
        "pending",
        "evidence/system-menu-record-list-open-0044af30.md",
    ),
    "0x0044AE70": (
        "pending",
        "evidence/character-attributes-difference-color-0044ae70.md",
    ),
    "0x0044AB00": (
        "pending",
        "evidence/character-attributes-recalculate-0044ab00.md",
    ),
    "0x0044A280": (
        "pending",
        "evidence/character-attributes-render-0044a280.md",
    ),
    "0x0044A250": (
        "pending",
        "evidence/character-attributes-close-0044a250.md",
    ),
    "0x0044A1D0": (
        "pending",
        "evidence/character-attributes-left-wrap-0044a1d0.md",
    ),
    "0x0044A160": (
        "pending",
        "evidence/character-attributes-previous-member-0044a160.md",
    ),
    "0x0044A0D0": (
        "pending",
        "evidence/character-attributes-next-member-0044a0d0.md",
    ),
    "0x0044A050": (
        "pending",
        "evidence/character-attributes-input-0044a050.md",
    ),
    "0x0044A030": (
        "pending",
        "evidence/character-attributes-close-records-0044a030.md",
    ),
    "0x00449FF0": (
        "pending",
        "evidence/character-attributes-open-00449ff0.md",
    ),
    "0x00449D80": (
        "pending",
        "evidence/game-settings-character-profile-00449d80.md",
    ),
    "0x00449C30": (
        "pending",
        "evidence/title-menu-sliding-panels-00449c30.md",
    ),
    "0x004490C0": (
        "pending",
        "evidence/title-menu-frame-render-004490c0.md",
    ),
    "0x00449050": (
        "pending",
        "evidence/game-settings-save-00449050.md",
    ),
    "0x00448EE0": (
        "pending",
        "evidence/title-menu-confirm-00448ee0.md",
    ),
    "0x00448EB0": (
        "pending",
        "evidence/load-progress-next-item-00448eb0.md",
    ),
    "0x00448DA0": (
        "pending",
        "evidence/game-settings-increase-00448da0.md",
    ),
    "0x00448CA0": (
        "pending",
        "evidence/game-settings-decrease-00448ca0.md",
    ),
    "0x00448C70": (
        "pending",
        "evidence/title-menu-first-item-00448c70.md",
    ),
    "0x00448C40": (
        "pending",
        "evidence/title-menu-last-item-00448c40.md",
    ),
    "0x00448C00": (
        "pending",
        "evidence/title-menu-previous-item-00448c00.md",
    ),
    "0x00448BB0": (
        "pending",
        "evidence/title-menu-next-item-00448bb0.md",
    ),
    "0x00448840": (
        "pending",
        "evidence/title-menu-input-00448840.md",
    ),
    "0x00448700": (
        "pending",
        "evidence/title-menu-animation-open-00448700.md",
    ),
    "0x00448650": (
        "pending",
        "evidence/special-mode-special-world-return-00448650.md",
    ),
    "0x004485F0": (
        "pending",
        "evidence/special-mode-special-world-transition-004485f0.md",
    ),
    "0x00448360": (
        "pending",
        "evidence/special-mode-resource-commit-00448360.md",
    ),
    "0x004482E0": (
        "pending",
        "evidence/special-mode-selection-record-cleanup-004482e0.md",
    ),
    "0x00448230": (
        "pending",
        "evidence/special-mode-selection-record-initialization-00448230.md",
    ),
    "0x00448020": (
        "pending",
        "evidence/special-mode-selection-record-clone-00448020.md",
    ),
    "0x00447100": (
        "pending",
        "evidence/game-menu-current-page-render-00447100.md",
    ),
    "0x00446FE0": (
        "pending",
        "evidence/special-mode-interaction-exit-00446fe0.md",
    ),
    "0x00446700": (
        "pending",
        "evidence/special-mode-multiphase-commit-00446700.md",
    ),
    "0x004466A0": (
        "pending",
        "evidence/special-mode-selection-cycle-runtime-advance-004466a0.md",
    ),
    "0x00446680": (
        "pending",
        "evidence/special-mode-selection-publish-runtime-advance-00446680.md",
    ),
    "0x00446550": (
        "pending",
        "evidence/game-menu-next-category-00446550.md",
    ),
    "0x00446420": (
        "pending",
        "evidence/game-menu-previous-category-00446420.md",
    ),
    "0x00446260": (
        "pending",
        "evidence/game-menu-previous-page-00446260.md",
    ),
    "0x00446090": (
        "pending",
        "evidence/game-menu-next-page-00446090.md",
    ),
    "0x00445E90": (
        "pending",
        "evidence/game-menu-up-control-00445e90.md",
    ),
    "0x00445C90": (
        "pending",
        "evidence/game-menu-down-control-00445c90.md",
    ),
    "0x004455E0": (
        "pending",
        "evidence/game-menu-page-input-004455e0.md",
    ),
    "0x004455A0": (
        "pending",
        "evidence/game-menu-list-close-004455a0.md",
    ),
    "0x00445430": (
        "pending",
        "evidence/game-menu-first-list-open-00445430.md",
    ),
    "0x00445420": (
        "pending",
        "evidence/game-menu-page-draw-00445420.md",
    ),
    "0x004453F0": (
        "pending",
        "evidence/game-menu-close-004453f0.md",
    ),
    "0x00445360": (
        "pending",
        "evidence/game-menu-confirm-selection-00445360.md",
    ),
    "0x004452B0": (
        "pending",
        "evidence/game-menu-next-item-004452b0.md",
    ),
    "0x00445210": (
        "pending",
        "evidence/game-menu-previous-item-00445210.md",
    ),
    "0x004450E0": (
        "pending",
        "evidence/game-menu-input-004450e0.md",
    ),
    "0x00444FC0": (
        "pending",
        "evidence/special-mode-callback-table-installation-00444fc0.md",
    ),
    "0x00444FB0": (
        "pending",
        "evidence/special-mode-equipment-final-action-count-00444fb0.md",
    ),
    "0x00444F60": (
        "pending",
        "evidence/special-mode-equipment-action-count-00444f60.md",
    ),
    "0x00444F00": (
        "pending",
        "evidence/special-mode-equipment-record-list-cleanup-00444f00.md",
    ),
    "0x00444E80": (
        "pending",
        "evidence/special-mode-equipment-record-list-rebuild-00444e80.md",
    ),
    "0x00444E50": (
        "pending",
        "evidence/special-mode-equipment-visible-count-00444e50.md",
    ),
    "0x00444DB0": (
        "pending",
        "evidence/special-mode-equipment-record-sort-00444db0.md",
    ),
    "0x004442B0": (
        "pending",
        "evidence/special-mode-equipment-render-004442b0.md",
    ),
    "0x004441A0": (
        "pending",
        "evidence/special-mode-equipment-exit-004441a0.md",
    ),
    "0x00443BD0": (
        "pending",
        "evidence/special-mode-equipment-commit-00443bd0.md",
    ),
    "0x00443B70": (
        "pending",
        "evidence/special-mode-equipment-list-kind-cycle-00443b70.md",
    ),
    "0x00443A60": (
        "pending",
        "evidence/special-mode-equipment-party-cycle-00443a60.md",
    ),
    "0x004439A0": (
        "pending",
        "evidence/special-mode-equipment-column-advance-004439a0.md",
    ),
    "0x004438E0": (
        "pending",
        "evidence/special-mode-equipment-column-toggle-004438e0.md",
    ),
    "0x004437C0": (
        "pending",
        "evidence/special-mode-equipment-page-retreat-004437c0.md",
    ),
    "0x00443670": (
        "pending",
        "evidence/special-mode-equipment-page-advance-00443670.md",
    ),
    "0x00443570": (
        "pending",
        "evidence/special-mode-equipment-retreat-00443570.md",
    ),
    "0x00443450": (
        "pending",
        "evidence/special-mode-equipment-advance-00443450.md",
    ),
    "0x00442F40": (
        "pending",
        "evidence/special-mode-equipment-input-dispatch-00442f40.md",
    ),
    "0x00442F10": (
        "pending",
        "evidence/special-mode-equipment-cleanup-00442f10.md",
    ),
    "0x00442E40": (
        "pending",
        "evidence/special-mode-equipment-initialization-00442e40.md",
    ),
    "0x00442D70": (
        "pending",
        "evidence/special-mode-guardian-record-exchange-attributes-00442d70.md",
    ),
    "0x00442CA0": (
        "pending",
        "evidence/special-mode-guardian-attribute-summary-00442ca0.md",
    ),
    "0x00442BC0": (
        "pending",
        "evidence/special-mode-guardian-party-attribute-finalize-00442bc0.md",
    ),
    "0x00442B10": (
        "pending",
        "evidence/special-mode-guardian-selected-attributes-00442b10.md",
    ),
    "0x00442AA0": (
        "pending",
        "evidence/special-mode-guardian-party-attributes-00442aa0.md",
    ),
    "0x00442A40": (
        "pending",
        "evidence/special-mode-guardian-attribute-seed-00442a40.md",
    ),
    "0x004429B0": (
        "pending",
        "evidence/special-mode-guardian-attribute-cache-refresh-004429b0.md",
    ),
    "0x00442960": (
        "pending",
        "evidence/special-mode-guardian-category-icon-00442960.md",
    ),
    "0x004425C0": (
        "pending",
        "evidence/special-mode-guardian-slot-panel-004425c0.md",
    ),
    "0x00442130": (
        "pending",
        "evidence/special-mode-guardian-attribute-panel-00442130.md",
    ),
    "0x004420F0": (
        "pending",
        "evidence/special-mode-guardian-record-list-drain-004420f0.md",
    ),
    "0x00442050": (
        "pending",
        "evidence/special-mode-guardian-record-list-refresh-00442050.md",
    ),
    "0x00441F70": (
        "pending",
        "evidence/special-mode-guardian-record-filter-00441f70.md",
    ),
    "0x00441680": (
        "pending",
        "evidence/special-mode-guardian-render-00441680.md",
    ),
    "0x00441590": (
        "pending",
        "evidence/special-mode-guardian-interaction-commit-00441590.md",
    ),
    "0x00441160": (
        "pending",
        "evidence/special-mode-guardian-interaction-switch-00441160.md",
    ),
    "0x00441060": (
        "pending",
        "evidence/special-mode-guardian-party-cycle-00441060.md",
    ),
    "0x00440FB0": (
        "pending",
        "evidence/special-mode-guardian-repeat-advance-00440fb0.md",
    ),
    "0x00440F00": (
        "pending",
        "evidence/special-mode-guardian-repeat-retreat-00440f00.md",
    ),
    "0x00440E10": (
        "pending",
        "evidence/special-mode-guardian-page-retreat-00440e10.md",
    ),
    "0x00440D20": (
        "pending",
        "evidence/special-mode-guardian-page-advance-00440d20.md",
    ),
    "0x00440C20": (
        "pending",
        "evidence/special-mode-guardian-selection-retreat-00440c20.md",
    ),
    "0x00440B20": (
        "pending",
        "evidence/special-mode-guardian-selection-00440b20.md",
    ),
    "0x004407F0": (
        "pending",
        "evidence/special-mode-guardian-input-dispatch-004407f0.md",
    ),
    "0x00440630": (
        "pending",
        "evidence/special-mode-guardian-initialization-00440630.md",
    ),
    "0x004405C0": (
        "pending",
        "evidence/special-mode-altar-surface-release-004405c0.md",
    ),
    "0x004404D0": (
        "pending",
        "evidence/special-mode-altar-attributes-004404d0.md",
    ),
    "0x004400A0": (
        "pending",
        "evidence/special-mode-altar-animation-004400a0.md",
    ),
    "0x0043FDE0": (
        "pending",
        "evidence/special-mode-original-surface-preparation-0043fde0.md",
    ),
    "0x0043FA70": (
        "pending",
        "evidence/special-mode-altar-record-panel-0043fa70.md",
    ),
    "0x0043F940": (
        "pending",
        "evidence/special-mode-synthesis-inline-record-refresh-0043f940.md",
    ),
    "0x0043F880": (
        "pending",
        "evidence/special-mode-database-window-refresh-0043f880.md",
    ),
    "0x0043F7C0": (
        "pending",
        "evidence/special-mode-database-record-filter-0043f7c0.md",
    ),
    "0x0043F1E0": (
        "pending",
        "evidence/special-mode-database-runtime-record-refresh-0043f1e0.md",
    ),
    "0x0043F160": (
        "pending",
        "evidence/special-mode-database-forward-sort-0043f160.md",
    ),
    "0x0043F0D0": (
        "pending",
        "evidence/special-mode-database-forward-build-0043f0d0.md",
    ),
    "0x0043F080": (
        "pending",
        "evidence/special-mode-database-forward-release-0043f080.md",
    ),
    "0x0043F000": (
        "pending",
        "evidence/special-mode-database-forward-refresh-0043f000.md",
    ),
    "0x0043E800": (
        "pending",
        "evidence/special-mode-database-render-0043e800.md",
    ),
    "0x0043E770": (
        "pending",
        "evidence/special-mode-database-interaction-exit-0043e770.md",
    ),
    "0x0043E3D0": (
        "pending",
        "evidence/special-mode-database-interaction-commit-0043e3d0.md",
    ),
    "0x0043E310": (
        "pending",
        "evidence/special-mode-database-primary-direction-0043e310.md",
    ),
    "0x0043E250": (
        "pending",
        "evidence/special-mode-database-direction-cycle-0043e250.md",
    ),
    "0x0043E170": (
        "pending",
        "evidence/special-mode-database-page-source-advance-0043e170.md",
    ),
    "0x0043E080": (
        "pending",
        "evidence/special-mode-database-page-cycle-0043e080.md",
    ),
    "0x0043DFA0": (
        "pending",
        "evidence/special-mode-database-page-retreat-0043dfa0.md",
    ),
    "0x0043DED0": (
        "pending",
        "evidence/special-mode-database-page-advance-0043ded0.md",
    ),
    "0x0043DDF0": (
        "pending",
        "evidence/special-mode-database-forward-retreat-0043ddf0.md",
    ),
    "0x0043DD20": (
        "pending",
        "evidence/special-mode-database-forward-advance-0043dd20.md",
    ),
    "0x0043DA30": (
        "pending",
        "evidence/special-mode-database-input-dispatch-0043da30.md",
    ),
    "0x0043D880": (
        "pending",
        "evidence/special-mode-database-cleanup-0043d880.md",
    ),
    "0x0043D530": (
        "pending",
        "evidence/special-mode-database-initialization-0043d530.md",
    ),
    "0x0043D470": (
        "pending",
        "evidence/special-mode-mode-strip-0043d470.md",
    ),
    "0x0043D370": (
        "pending",
        "evidence/special-mode-derived-text-0043d370.md",
    ),
    "0x0043D050": (
        "pending",
        "evidence/special-mode-selected-record-display-0043d050.md",
    ),
    "0x0043CEF0": (
        "pending",
        "evidence/special-mode-entry-consumption-0043cef0.md",
    ),
    "0x0043CC20": (
        "pending",
        "evidence/special-mode-entry-render-0043cc20.md",
    ),
    "0x0043CC00": (
        "pending",
        "evidence/special-mode-entry-alias-0043cc00.md",
    ),
    "0x0043CBD0": (
        "pending",
        "evidence/special-mode-page-refresh-0043cbd0.md",
    ),
    "0x0043C9C0": (
        "pending",
        "evidence/special-mode-entry-initialization-0043c9c0.md",
    ),
    "0x0043C820": (
        "pending",
        "evidence/special-mode-runtime-render-0043c820.md",
    ),
    "0x0043C760": (
        "pending",
        "evidence/special-mode-runtime-mode-advance-0043c760.md",
    ),
    "0x0043C670": (
        "pending",
        "evidence/special-mode-runtime-page-retreat-0043c670.md",
    ),
    "0x0043C590": (
        "pending",
        "evidence/special-mode-runtime-cursor-retreat-0043c590.md",
    ),
    "0x0043C520": (
        "pending",
        "evidence/special-mode-runtime-cursor-advance-0043c520.md",
    ),
    "0x0043C3C0": (
        "pending",
        "evidence/special-mode-runtime-input-dispatch-0043c3c0.md",
    ),
    "0x0043C0D0": (
        "pending",
        "evidence/special-mode-runtime-initialization-0043c0d0.md",
    ),
    "0x0043C090": (
        "pending",
        "evidence/special-mode-availability-0043c090.md",
    ),
    "0x0043BFC0": (
        "pending",
        "evidence/special-mode-dialog-setup-0043bfc0.md",
    ),
    "0x0043BE90": (
        "pending",
        "evidence/special-mode-filtered-records-0043be90.md",
    ),
    "0x0043BE40": (
        "pending",
        "evidence/special-mode-value-group-0043be40.md",
    ),
    "0x0043BD70": (
        "pending",
        "evidence/special-mode-animated-panel-0043bd70.md",
    ),
    "0x0043BCC0": (
        "pending",
        "evidence/special-mode-window-selection-0043bcc0.md",
    ),
    "0x0043BC90": (
        "pending",
        "evidence/special-mode-bounded-forward-count-0043bc90.md",
    ),
    "0x0043BC60": (
        "pending",
        "evidence/special-mode-window-page-retreat-0043bc60.md",
    ),
    "0x0043BBE0": (
        "pending",
        "evidence/special-mode-window-page-advance-0043bbe0.md",
    ),
    "0x0043BBC0": (
        "pending",
        "evidence/special-mode-window-cursor-retreat-0043bbc0.md",
    ),
    "0x0043BB80": (
        "pending",
        "evidence/special-mode-window-cursor-advance-0043bb80.md",
    ),
    "0x0043BB40": (
        "pending",
        "evidence/special-mode-window-cursor-0043bb40.md",
    ),
    "0x0043BA40": (
        "pending",
        "evidence/special-mode-input-status-0043ba40.md",
    ),
    "0x0043B9E0": (
        "pending",
        "evidence/special-mode-shared-text-0043b9e0.md",
    ),
    "0x0043B9C0": (
        "pending",
        "evidence/special-mode-forward-index-0043b9c0.md",
    ),
    "0x0043B9A0": (
        "pending",
        "evidence/special-mode-forward-advance-0043b9a0.md",
    ),
    "0x0043B980": (
        "pending",
        "evidence/special-mode-forward-count-0043b980.md",
    ),
    "0x0043B480": (
        "pending",
        "evidence/special-mode-callback-binding-0043b480.md",
    ),
    "0x0043B080": (
        "pending",
        "evidence/special-mode-ghost-action-0043b080.md",
    ),
    "0x0043AE40": (
        "pending",
        "evidence/special-mode-split-bar-0043ae40.md",
    ),
    "0x0043AAA0": (
        "pending",
        "evidence/game-menu-four-entry-animation-0043aaa0.md",
    ),
    "0x0043A880": (
        "pending",
        "evidence/special-mode-panel-preparation-0043a880.md",
    ),
    "0x0043A610": (
        "pending",
        "evidence/special-mode-frame-render-0043a610.md",
    ),
    "0x0043A470": (
        "pending",
        "evidence/special-mode-input-dispatch-0043a470.md",
    ),
    "0x0043A380": (
        "pending",
        "evidence/special-mode-item-availability-0043a380.md",
    ),
    "0x0043A2A0": (
        "pending",
        "evidence/special-mode-selector-initialization-0043a2a0.md",
    ),
    "0x00439DE0": (
        "pending",
        "evidence/special-mode-global-initialization-00439de0.md",
    ),
    "0x00439FD0": (
        "pending",
        "evidence/special-mode-standard-dispatch-00439fd0.md",
    ),
}


def address_value(value: str) -> int:
    return int(value, 16)


def family(address: int) -> str:
    if 0x00406D30 <= address <= 0x004070A0:
        return "high_priority_dispatch"
    if 0x0040F890 <= address <= 0x00411700:
        return "shared_dialog_and_party_helpers"
    if 0x00439DE0 <= address < 0x0044EA60:
        return "standard_modes_1_3_4_5_6"
    if 0x0044EA60 <= address <= 0x0044FF90:
        return "shop_mode_2"
    return "ownership_review_required"


def main() -> None:
    with OWNERSHIP_INPUT.open(encoding="utf-8", newline="") as source:
        rows = [
            row
            for row in csv.DictReader(source, delimiter="\t")
            if row["module_candidate"] == "special_modes"
        ]
    if len(rows) != EXPECTED_CANDIDATE_COUNT:
        raise SystemExit(
            f"expected {EXPECTED_CANDIDATE_COUNT} special-mode candidates, "
            f"got {len(rows)}"
        )

    rows.sort(key=lambda row: address_value(row["address"]))
    output_rows = []
    for audit_order, row in enumerate(rows, start=1):
        address = address_value(row["address"])
        closure_status, closure_evidence = CLOSURES.get(
            row["address"], ("pending", "")
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
                "ownership and call-graph fields are navigation only; close from full LST body",
            )
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
    print(f"wrote {OUTPUT.relative_to(RESEARCH_ROOT)} ({len(output_rows)} rows)")
    closed_count = sum(row[8] not in {"pending", "pending_audit"} for row in output_rows)
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
