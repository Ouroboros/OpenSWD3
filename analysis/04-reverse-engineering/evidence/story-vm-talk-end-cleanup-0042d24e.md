# Story VM talk-end cleanup at `0x0042D24E`

## Scope

- Authoritative handler: `swd3.exe.lst:0x0042D24E..0x0042D49E`.
- Effective opcode: `16383` (`0x3FFF`); every raw word with low 14 bits equal to `0x3FFF` reaches the same handler.
- Classification: `platform_adapted`.
- Modern owner: `src/world_map/legacy_world_story_vm.cpp`.
- Synthetic coverage: `tests/unit/world_map/legacy_world_story_vm_test_02.cpp:test_finish_talk_protocol`.
- Real-asset coverage: `tests/unit/world_map/legacy_world_story_vm_test_05.cpp:test_real_finish_talk_records`.

## LST to C++ mapping

### Source role cleanup

- `0x0042D24E..0x0042D264` skips source-role handling for source selectors `0xFFFF` and `0xFFFD`.
- `0x0042D26A..0x0042D27A` resolves every other source through `sub_40C0D0`; a lookup miss only reaches the original diagnostic path and then continues global cleanup.
- `0x0042D284..0x0042D2A6` diagnoses a resolved role index outside the original 256-role table but still reaches the original unsafe indexed access. The port converts that memory-corruption point to `LegacyWorldStoryVmStatus::role_index_out_of_range` before source-role mutation.
- `0x0042D2A9` calls `sub_42D920` for a live bit-31 path. The port uses `complete_legacy_world_story_path`, retains its active-object reset side effect, and then clears bit 31 at the same point as `0x0042D2D0..0x0042D2D5`.
- `0x0042D2D2..0x0042D2F3` tests role flag bit 11, conditionally restores the base variant and variant delta from their one-shot fields, and then writes `0xFFFFFFFF` to both one-shot fields.
- `0x0042D2F6..0x0042D33B` calls the action-update owner. A zero legacy return is diagnostic-only and does not stop cleanup.
- `0x0042D345..0x0042D36C` performs the original bit-19 second path-completion call and then applies the same `0x7FFFFFFF` mask again. Because bit 31 was already cleared, the second completion is a no-op and bit 19 remains set. The port deliberately preserves this quirk instead of clearing bit 19.

A fresh public VM call with `source_guid == 0xFFFF` is rejected by the pre-dispatch inactive-context gate, so the handler-local `0xFFFF` selector branch is statically mapped while the behaviorally identical reachable `0xFFFD` skip branch is dynamically exercised. `0xFFFE` is dynamically exercised through the controlled-role selector path.

### Global role and object cleanup

- `0x0042D3B2..0x0042D3D1` walks the runtime role count and writes `0xFFFFFFFF` to both one-shot action fields of every role.
- The original arrays have capacity 256. If a modern span exposes more than 256 roles, the port commits the first 256 clears and then returns `role_index_out_of_range` at the original first out-of-bounds access.
- `0x0042D3D3..0x0042D43B` scans all 72 active-object slots. A slot with role `0xFFFF`, or with path-state low nibble at most one, is skipped. Every selected slot is reset through the existing object-reset owner and its referenced role path fields are cleared.
- A selected slot whose role index is outside the original role table commits the slot reset first, then returns `role_index_out_of_range` before the original unsafe role write.

### Final cleanup and common join

- `0x0042D43D..0x0042D460` formats and submits a debug-only `TalkEnd` message through `nullsub_1`; the SDL port has no observable debug consumer and does not invent one.
- `0x0042D465..0x0042D475` clears dialog bit 15.
- `0x0042D47A..0x0042D48C` writes `0xFFFF` to the first word of the loaded talk window and fills the talk context with `0xFF`.
- The port also retires the typed window metadata (`window_loaded`, file number, and data offset) which represents the original pointer/window lifetime.
- `0x0042D48E..0x0042D494` clears the two movement counters represented by `LegacyWorldMovementRuntimeState::no_input_frame_count` and `idle_phase`. A missing movement owner becomes an explicit `runtime_unavailable` stop after every earlier cleanup side effect has committed.
- `0x0042D49A` enters the audited common join. Without a call-local opcode-1024 latch, opcode 16383 publishes `previous_opcode`, services audio once after final cleanup, and returns `terminated`. With the latch set, it publishes previous, suppresses common audio, and same-calls the next fetch, which fails against the retired `0xFFFF` context. Exact-tail execution does not read a successor before cleanup.

## Synthetic coverage

`test_finish_talk_protocol` verifies:

- all four raw selector aliases (`0x3FFF`, `0x7FFF`, `0xBFFF`, `0xFFFF`);
- reachable `0xFFFD` map-event skip and `0xFFFE` controlled-role resolution;
- ordinary lookup miss as diagnostic-only continuation;
- bit-31 path completion, one-shot restoration, action-update ordering, zero-return continuation, and the preserved bit-19 quirk;
- role-wide one-shot clearing and all 72 active-object slots;
- commit-before-stop ordering for an invalid selected object role, a role span larger than 256, a missing path owner, and a missing movement owner;
- dialog, window, typed window metadata, context, and movement-counter cleanup;
- common previous publication, final-state-before-audio ordering, latch suppression/same-call behavior, and exact-tail behavior.

Existing same-call chains that terminate through opcode 16383 were updated to require opcode 16383 as `previous_opcode` and the additional common-join audio service where no opcode-1024 latch is active.

## Real TALK asset coverage

The generated linear-record inventory contains 3,743 unique opcode-16383 records and 3,756 entry probes:

- `TALK1.DAT`: 1,035 records.
- `TALK2.DAT`: 673 records.
- `TALK3.DAT`: 964 records.
- `TALK4.DAT`: 1,071 records.

Nine physical locations have more than one entry probe:

- `TALK1.DAT@0x00011FCF` (2).
- `TALK2.DAT@0x00033060` (2).
- `TALK2.DAT@0x0003307A` (2).
- `TALK2.DAT@0x000330D4` (2).
- `TALK3.DAT@0x00011206` (6).
- `TALK3.DAT@0x0001E431` (2).
- `TALK4.DAT@0x000364E6` (2).
- `TALK4.DAT@0x00036500` (2).
- `TALK4.DAT@0x0003655A` (2).

The real-asset test reads and replays 17 records covering each file's first/last selected boundary plus every multi-probe location. Every sample retains raw word `0xFFFF` and executes terminal cleanup, movement reset, previous publication, and common audio.

## Verification snapshot

- Story VM synthetic, real-asset, and initial-session real-asset CTest registrations: `3/3` passed.
- Generated handler workpack: `146/146 = 26 assembly_exact + 120 platform_adapted`.
- `story-vm-handler-workpack.tsv` SHA-256: `f442d46d2d9179ee51c2c26da24e469cfba7ae1d2e9ac94882d612489b8075b5`.
- `story-vm-runtime-paths.tsv` SHA-256 after P3 closure: `e756eb7dc677b1066fc5a2b943db9c2fe3e7caf1c314ab078879efbbb86dded2`.
