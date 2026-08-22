# Story VM full validation P3

## Scope and authoritative inputs

This evidence closes P3 from `goal/story-vm-closure-plan-pi.md` after every handler work package was independently closed.

Authoritative behavior comes from `swd3.exe_export_for_ai/swd3.exe.lst`. The full-TALK generators additionally lock the four real TALK payload hashes and the authoritative LST SHA-256 `701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`.

The P3 runtime-path inventory has 17 rows, all closed:

- 7 `assembly_exact`.
- 10 `platform_adapted`.
- 0 scope-only, LST-only, or `pending_audit` rows.

## 1. Complete explicit opcode domain

The regenerated dispatcher inventories prove:

- 198 explicit effective opcodes.
- 146 unique handler entries.
- 25 shared entries.
- 11 primary dispatch ranges.
- 2 internal refinement switches.
- 197 modern C++ case labels; opcode 1026 retains the pre-existing shared case and is counted once.

The handler workpack is `146/146 = 26 assembly_exact + 120 platform_adapted`. All 198 explicit opcodes are implemented and closed, including 55 opcodes not observed by the current linear TALK prefix probes.

## 2. Entry, decode, dispatch, defaults, specials, and returns

### Entry activation and initial load

`0x00427920..0x00427B3F` is represented by typed VM state and resource owners:

- inactive source `0xFFFF` exits before loading or audio service;
- the initial alignment gate yields without loading;
- a successful initial load commits file/data/window metadata before fetch;
- checked invalid story, open, seek, read, and directory failures replace Win32 `WM_DESTROY`/zero-return behavior with explicit `load_failed` while preserving every earlier side effect.

Synthetic initial-load, alignment, reinitialization, transfer, truncation, and checked-failure tests lock these paths.

### Fetch and dispatch

`0x00427B40..0x00427B88` reads the raw 16-bit word, retains it for diagnostics/tests, and dispatches on `raw & 0x3FFF`. Raw modifier aliases are exercised throughout the suite, including all four aliases of default opcode zero, opcodes 1024-1026, and opcode 16383.

The two LST jump tables regenerate to exactly:

- 98 primary entries for opcodes 1-98.
- 94 secondary entries for opcodes 100-193.

Opcode 99, the default ranges, and the four special values retain their separate routing.

### Internal refinement switches

The two internal tables are regenerated from LST and matched to closed C++ handlers:

- numeric refinement: six dword targets plus 157 selector bytes, with non-default opcodes 29-33 and 181-185;
- flag refinement: nine dword targets plus 73 selector bytes, with non-default opcodes 102, 103, 117, 136, 140, 145, 146, and 174.

Every owning opcode has signedness, wrapping, comparison, selector, exact-tail, previous-publication, and yield/same-call coverage in its handler evidence and synthetic tests.

### Default and special paths

- Default effective opcodes `0`, `194..1023`, `1027..16382` beep, diagnose, do not advance IP, publish previous, pass the common join, service audio, and yield.
- Opcode 1024 advances two bytes and latches call-local common-join continuation.
- Opcode 1025 advances two bytes, clears continuation, publishes previous, services audio, and yields.
- Opcode 1026 advances two bytes, sets one-shot continuation, publishes previous, suppresses common audio, and same-calls the next fetch.
- Opcode 16383 performs the independently audited terminal cleanup before the common join.

### Return paths

The original integer return values are represented by typed results:

- fatal return zero preserves the close-bit side effect and the handler-selected failure ordering;
- ordinary yield return one services audio, while inactive entry return one skips audio;
- initial-load failure zero becomes checked `load_failed` rather than sending a Win32 destroy message.

No caller-visible behavior depends on the erased raw EAX value.

## 3. Full real TALK traversal

### Strict linear probe

`build_story_vm_talk_linear_probe.py` now supports every current variable encoding, including `mode_text_percent_q`, and rejects any stop reason outside the locked set.

Results:

- 3,992 directory entry probes.
- 58,782 unique linear records.
- 198 opcode coverage rows.
- 143/198 opcodes observed.
- 3,756 probes reach opcode 16383.
- 68 stop at unconditional opcode 15 transfer.
- 90 stop at opcode 161 story transfer.
- 65 stop at indexed opcode 41 transfer.
- 5 stop at random opcode 87 transfer.
- 8 TALK3 directory values are locked invalid index candidates and are not treated as story streams.
- 0 mid-stream decode errors, unknown opcodes, nonpositive lengths, cycles, or step-limit stops.

### Strict control-flow graph

`build_story_vm_talk_control_flow_graph.py` validates all 31 transfer opcodes against the authoritative LST and shares the linear probe's invalid-root exclusion.

Results:

- 3,979 unique valid root windows from 3,984 valid root slots.
- 8 invalid TALK3 root slots excluded.
- 138,988 decoded context nodes.
- 99,092 physical instruction offsets.
- 137,207 edges.
- 0 CFG issues.

The edges include sequential, conditional, indexed, random, same-file transfer, and story-reload paths. The 193 `selector_equal_count_sentinel_bug` edges intentionally preserve opcode 41's original selector-equals-count bug; their `target_outside_file` resolution is explicit and never enqueued as valid code.

## 4. Cross-opcode state and control combinations

The single Story VM test binary retains three CTest registrations and covers:

- state carried across opcode boundaries;
- self-modifying instructions;
- exact-tail and staged truncation behavior;
- same-call continuation and persistent call-local latch interaction;
- cross-frame yield/wait behavior;
- same-file reload and story-window transfer;
- previous publication and common audio ordering;
- external-owner requests for dialog, menu, shop, battle, audio, video, world/session, path, ANI, rendering, and input contracts;
- typed-stop ordering at original unsafe or stale-value points.

The same test binary and real-asset lock are serialized by the existing CTest resource locks; the core no-SDL build remains separately verifiable.

## 5. Real long-sequence execution

The real-asset registration replays the long story-248/new-game path through its first dialog and verifies the expected first-dialog state without `unsupported_opcode`. The initial-session registration independently verifies the unloaded-role patch path. Representative real records for all 143 asset-observed opcodes are additionally replayed by opcode-specific tests.

No VM-side handler, dispatch path, encoding length, or transfer rule remains unimplemented or temporarily bypassed. External module owners and previously registered runtime-oracle limitations remain narrow typed boundaries and do not substitute for VM implementation.

## 6. Reproducible inventory snapshot

- `story-vm-handler-workpack.tsv`: `f442d46d2d9179ee51c2c26da24e469cfba7ae1d2e9ac94882d612489b8075b5`.
- `story-vm-runtime-paths.tsv`: `e756eb7dc677b1066fc5a2b943db9c2fe3e7caf1c314ab078879efbbb86dded2`.
- `story-vm-talk-linear-entry-probes.tsv`: `8bdee28f58fbcc377bd661818ccbf770878bb55b83632a401fff53e712820894`.
- `story-vm-talk-linear-records.tsv`: `cef644815fc6929236ee4cd4ccd342444e6ca509d84449306a2bc0e5b2d989db`.
- `story-vm-talk-opcode-coverage.tsv`: `0151e6977f5985164600f4e38bcfe88196f1abef4d783c5e0ca221105c0cce1c`.
- `story-vm-control-transfer-rules.tsv`: `13984b41f7a358e39580ff52daae4aaf0ff578d705dbb6f4a9b2b456105adf56`.
- `story-vm-talk-cfg-nodes.tsv`: `71597faab113aa502eb67d9df9568c3e1f6c28ba496717ce805defdd725668b1`.
- `story-vm-talk-cfg-edges.tsv`: `f60daf36716ec5833c07b5b8c46c4b2d28b28043a34310561ad4f011ad972129`.
- `story-vm-talk-cfg-issues.tsv`: `4f043bfe60f13f69358aa958db15e9ea4620d0974a48159ef2f1a1d1a1d98102` (header only; zero issue rows).

## 7. Final gates

- All nine generated inventory hashes reproduced identically in two consecutive complete generation rounds.
- Story VM synthetic, real-asset, and initial-session real-asset registrations passed `3/3`.
- Linux core full gate passed `186/186`.
- Linux app full gate passed `192/192`.
- The P2 Windows LLVM app full gate passed `192/192` after its independently committed MAPS asset portability fix.
- The separate P3 Windows LLVM app full gate passed `192/192`.
- No original or OpenSWD3 game executable was launched.
