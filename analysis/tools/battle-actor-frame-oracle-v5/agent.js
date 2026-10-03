'use strict';

// Frida modifies code/timing. Never write original business data or registers.
// Inject FRAME_NATIVE_BRIDGE_SOURCE and FRAME_CAPTURE_OPTIONS before this file.
const ENTRY = ptr('0x00479850');
const NESTED_ENTRIES = [ptr('0x004321E0'), ptr('0x00432A50'), ptr('0x00432BC0')];
const LIST_QUERY_ENTRY = ptr('0x00470180');
const LIST_PROFILE_ENTRY = ptr('0x004705C0');
const ACTION_KIND_SETTER = ptr('0x00478710');
const NARROW_LIST_RETURN = ptr('0x00466689');
const LIST_NODE_BYTES = 0x60;
const MAX_LIST_NODES = 128;
const ACTION_RECORD_BYTES = 0x98;
const FIRST = 0x00479850;
const LAST = 0x0047B9CA;
// Group B stride is 0x2B28; the target reads the dword at actor+0x2B20.
const ACTOR_BYTES = 0x2b28;
const MAX_BLOCKS = 16384;
const MAX_SAMPLES_PER_SITE = FRAME_CAPTURE_OPTIONS.max_samples_per_site;
const REPEAT_INTERVAL = FRAME_CAPTURE_OPTIONS.repeat_interval;
const TRACE_ENABLED = FRAME_CAPTURE_OPTIONS.trace_enabled !== false;
const LIST_PROBE_ENABLED = FRAME_CAPTURE_OPTIONS.list_probe_enabled === true;
const RETURNS = new Map([
    ['0x4554fb', 'action_group_b'],
    ['0x456514', 'opponent_group_a'],
    ['0x45aa38', 'final_group_a'],
    ['0x45acc4', 'final_group_b'],
]);
const NESTED_RETURNS = new Map([
    ['0x479926', 'action_update'],
    ['0x432426', 'direct_act_stream'],
    ['0x432439', 'cached_act_stream'],
]);
const CPU_FIELDS = ['eip', 'edi', 'esi', 'ebp', 'esp', 'ebx', 'edx', 'ecx', 'eax'];
const active = new Map();
const calls = new Map();
const nestedCalls = new Map();
const seenListHeads = new Set();
const profileByThread = new Map();
let listSequence = 0;
let profileSequence = 0;
let actionKindSequence = 0;
let nestedSequence = 0;
const previousInputs = new Map();
const sampledBySite = new Map();
let callSequence = 0;
let unexpectedCallerReported = false;

function checkBytes(address, bytes) {
    for (let i = 0; i < bytes.length; ++i) {
        if (address.add(i).readU8() !== bytes[i]) {
            throw new Error(`LST instruction mismatch at ${address.add(i)}`);
        }
    }
}

function readCpu(cpu, flags, source) {
    const context = {};

    CPU_FIELDS.forEach((name, index) => {
        context[name] = ptr(cpu.add(index * 4).readU32());
    });

    context.eflags = ptr(flags);
    context.flags_source = source;
    return context;
}

function registers(context) {
    const result = {flags_available: context.eflags !== undefined,
                    flags_source: context.flags_source || 'unavailable'};

    for (const name of [...CPU_FIELDS, 'eflags']) {
        if (context[name] !== undefined) {
            result[name] = context[name].toString();
        }
    }

    return result;
}

function readableBytes(address, size) {
    const end = address.add(size);
    let cursor = address;
    while (cursor.compare(end) < 0) {
        const range = Process.findRangeByAddress(cursor);
        if (range === null || !range.protection.includes('r')) {
            return null;
        }

        const next = range.base.add(range.size);
        if (next.compare(cursor) <= 0) {
            throw new Error('invalid non-advancing Frida memory range');
        }

        cursor = next.compare(end) < 0 ? next : end;
    }

    return address.readByteArray(size);
}

function actorBytes(actor) {
    return readableBytes(actor, ACTOR_BYTES);
}

function globals() {
    const addresses = [
        '0x004A82F4', '0x004A8360', '0x0053E7B4', '0x0053E7B8',
        '0x00499138', '0x004A0E78', '0x004A0E7C', '0x004AB784',
        '0x004FB308', '0x004CD730', '0x004CD71C', '0x004CD30C',
        '0x004CD304',
    ];
    const result = {};

    for (const address of addresses) {
        try {
            result[address] = ptr(address).readU32().toString(16).padStart(8, '0');
        } catch (error) {
            result[address] = `unreadable:${error}`;
        }
    }

    return result;
}

function sameBytes(left, right) {
    if (left === null || right === null || left.byteLength !== right.byteLength) {
        return false;
    }

    const a = new Uint8Array(left);
    const b = new Uint8Array(right);
    for (let i = 0; i < a.length; i++) {
        if (a[i] !== b[i]) {
            return false;
        }
    }

    return true;
}

function chooseSample(site, actor, context, bytes, globalValues) {
    const key = `${site}:${actor}`;
    const tag = JSON.stringify([registers(context), globalValues]);
    const previous = previousInputs.get(key);
    let repeats = 0;
    if (previous !== undefined && previous.tag === tag && sameBytes(previous.bytes, bytes)) {
        repeats = previous.repeats + 1;
    }

    previousInputs.set(key, {tag, bytes, repeats});

    if (repeats !== 0 && repeats % REPEAT_INTERVAL !== 0) {
        return 'unchanged_captured_input';
    }

    if ((sampledBySite.get(site) || 0) >= MAX_SAMPLES_PER_SITE) {
        return 'sample_limit';
    }

    sampledBySite.set(site, (sampledBySite.get(site) || 0) + 1);
    return null;
}

function emitMemory(phase, sample, region, address, size) {
    let bytes = null;
    let problem = null;
    try {
        if (address.isNull()) {
            problem = 'null_pointer';
        } else {
            bytes = readableBytes(address, size);
            if (bytes === null) {
                problem = 'range_not_readable';
            }
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'memory', phase, sequence: sample.sequence, region,
          address: address.toString(), byte_count: bytes === null ? 0 : size,
          problem}, bytes);
}

function emitSnapshot(phase, sample, context, preparedBytes, preparedGlobals) {
    let bytes = null;
    let problem = null;
    try {
        bytes = preparedBytes === undefined ? actorBytes(sample.actor) : preparedBytes;
        if (bytes === null) {
            problem = 'actor memory range is not fully readable';
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'frame', phase, sequence: sample.sequence,
          site: sample.site, thread_id: sample.threadId, actor: sample.actor.toString(),
          registers: registers(context),
          globals: preparedGlobals === undefined ? globals() : preparedGlobals,
          actor_bytes: bytes === null ? 0 : ACTOR_BYTES, problem}, bytes);
    emitMemory(phase, sample, 'stack', context.esp, 128);
    for (const [name, offset, size] of [
        ['frame-header', 0x2548, 16],
        ['list-head-word', 0x2584, 4],
    ]) {
        try {
            emitMemory(phase, sample, name, sample.actor.add(offset).readPointer(), size);
        } catch (error) {
            send({type: 'memory', phase, sequence: sample.sequence,
                  region: name, byte_count: 0, problem: String(error)});
        }
    }
}

function emitNestedMemory(phase, call, region, address, size) {
    let bytes = null;
    let problem = null;
    try {
        if (address === null || address.isNull()) {
            problem = 'null_pointer';
        } else {
            bytes = readableBytes(address, size);
            if (bytes === null) {
                problem = 'range_not_readable';
            }
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'nested-memory', phase, nested_id: call.id,
          parent_sequence: call.parentSequence, site: call.site, kind: call.kind,
          region, address: address === null ? null : address.toString(),
          byte_count: bytes === null ? 0 : size, problem}, bytes);
}

function emitNestedSnapshot(phase, call, context) {
    send({type: phase === 'enter' ? 'nested-call' : 'nested-return',
          nested_id: call.id, parent_sequence: call.parentSequence,
          site: call.site, kind: call.kind, thread_id: call.threadId,
          return_address: call.returnAddress.toString(),
          record_address: call.recordAddress === null ? null : call.recordAddress.toString(),
          expected_record_address: call.expectedRecord.toString(),
          registers: registers(context), globals: globals()});
    emitNestedMemory(phase, call, 'stack', context.esp, 128);
    if (call.kind === 'action_update') {
        emitNestedMemory(phase, call, 'action-record', call.recordAddress, ACTION_RECORD_BYTES);
    }
}

// The narrow '絕招' list calls sub_470180 from 0x466684. Its cursor
// starts at actor+0x2EC0 and advances through each node's first dword.
// This snapshot observes source nodes only; it does not prove selection.
function scanNarrowList() {
    let returnAddress;
    try {
        returnAddress = this.context.esp.readPointer();
    } catch (error) {
        send({type: 'warning', message: `list query return unreadable: ${error}`});
        return;
    }

    if (!returnAddress.equals(NARROW_LIST_RETURN)) {
        return;
    }

    const actor = this.context.ecx;
    const threadId = Process.getCurrentThreadId();
    const id = ++listSequence;
    let root = null;
    let node = null;
    let count = 0;
    let problem = null;
    const visited = new Set();
    try {
        root = actor.add(0x2ec0).readPointer();
        if (root.isNull() || readableBytes(root, 4) === null) {
            throw new Error('unreadable list root');
        }

        node = root.readPointer();
        const headKey = `${actor}:${root}:${node}`;
        if (seenListHeads.has(headKey)) {
            return;
        }

        seenListHeads.add(headKey);
        while (!node.isNull() && count < MAX_LIST_NODES) {
            const token = node.toString();
            if (visited.has(token)) {
                problem = 'list_cycle';
                break;
            }

            visited.add(token);
            const bytes = readableBytes(node, LIST_NODE_BYTES);
            if (bytes === null) {
                problem = 'unreadable_node';
                break;
            }

            const next = node.readPointer();
            send({type: 'list-node', list_id: id, index: count,
                  thread_id: threadId, actor: actor.toString(),
                  root: root.toString(), node: token, next: next.toString(),
                  byte_count: LIST_NODE_BYTES}, bytes);
            node = next;
            count++;
        }

        if (count === MAX_LIST_NODES && !node.isNull()) {
            problem = 'node_limit';
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'list-scan', list_id: id, thread_id: threadId,
          actor: actor.toString(), root: root === null ? null : root.toString(),
          count, complete: problem === null, problem});
}

function profileEnter() {
    const threadId = Process.getCurrentThreadId();
    const call = {id: ++profileSequence, actor: this.context.ecx,
                  threadId};
    const stack = profileByThread.get(threadId) || [];
    stack.push(call);
    profileByThread.set(threadId, stack);
    let argumentsSnapshot = null;
    let problem = null;
    try {
        argumentsSnapshot = [4, 8, 12, 16].map(offset =>
            this.context.esp.add(offset).readU32());
    } catch (error) {
        problem = String(error);
    }

    send({type: 'profile-call', profile_id: call.id, thread_id: threadId,
          actor: call.actor.toString(), arguments: argumentsSnapshot, problem});
}

function profileLeave(retval) {
    const threadId = Process.getCurrentThreadId();
    const stack = profileByThread.get(threadId);
    const call = stack === undefined ? undefined : stack.pop();
    if (call === undefined) {
        send({type: 'warning', message: 'profile return without query entry'});
        return;
    }

    if (stack.length === 0) {
        profileByThread.delete(threadId);
    }

    // Snapshot only the exit cursor. The matching branch may have visited
    // another node or never executed; do not label this a selected node.
    let node = null;
    let bytes = null;
    let appliedWord = null;
    let problem = null;
    try {
        node = call.actor.add(0x2ec0).readPointer();
        appliedWord = call.actor.add(0x2f18).readU16();
        bytes = readableBytes(node, LIST_NODE_BYTES);
        if (bytes === null) {
            problem = 'unreadable profile exit cursor';
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'profile-cursor-exit', profile_id: call.id, thread_id: threadId,
          actor: call.actor.toString(), node: node === null ? null : node.toString(),
          applied_word: appliedWord, byte_count: bytes === null ? 0 : LIST_NODE_BYTES,
          problem}, bytes);
    send({type: 'profile-return', profile_id: call.id, thread_id: threadId,
          actor: call.actor.toString(), eax: retval.toString(), cursor_snapshots: 1});
}

function actionKindEnter() {
    let argument;
    let returnAddress;
    try {
        argument = this.context.esp.add(4).readU32();
        returnAddress = this.context.esp.readPointer();
    } catch (error) {
        send({type: 'warning', message: `action kind argument unreadable: ${error}`});
        return;
    }

    if (argument !== 7) {
        return;
    }

    const actor = this.context.ecx;
    this.actionKindCall = {id: ++actionKindSequence, actor};
    send({type: 'action-seven-call', action_id: this.actionKindCall.id,
          thread_id: Process.getCurrentThreadId(), actor: actor.toString(),
          return_address: returnAddress.toString(), argument});
}

function actionKindLeave(retval) {
    const call = this.actionKindCall;
    if (call === undefined) {
        return;
    }

    let value = null;
    let problem = null;
    try {
        value = call.actor.add(0x2a6c).readU16();
    } catch (error) {
        problem = String(error);
    }

    send({type: 'action-seven-return', action_id: call.id,
          thread_id: Process.getCurrentThreadId(), actor: call.actor.toString(),
          eax: retval.toString(), action_word: value, problem});
}

function recordBlock(start, end) {
    const sample = active.get(Process.getCurrentThreadId());
    if (sample === undefined) {
        return;
    }

    if (sample.blocks.length < MAX_BLOCKS) {
        sample.blocks.push([start, end]);
    } else {
        sample.dropped++;
    }
}

function transformBlocks(iterator) {
    let instruction = iterator.next();
    if (instruction === null) {
        return;
    }

    const start = instruction.address.toString();
    const address = parseInt(start, 16);
    let end = start;
    if (address >= FIRST && address <= LAST) {
        // Executed synchronously before this translated block. No event queue.
        iterator.putCallout(function () { recordBlock(start, end); });
    }

    do {
        end = instruction.address.add(instruction.size).toString();
        iterator.keep();
    } while ((instruction = iterator.next()) !== null);
}

function frameEnter(cpu, flags, threadId, returnAddress) {
    if ((flags & 2) === 0) {
        throw new Error('invalid saved FLAGS reserved bit');
    }

    const site = RETURNS.get(returnAddress.toString().toLowerCase());
    if (site === undefined) {
        if (!unexpectedCallerReported) {
            unexpectedCallerReported = true;
            send({type: 'warning', message: `unrecognized frame caller return ${returnAddress}`});
        }

        return 0;
    }

    const context = readCpu(cpu, flags, 'frida_16_5_1_saved_pushfd');
    const sequence = ++callSequence;
    let bytes = null;
    let skipReason = null;
    const globalValues = globals();
    if (active.has(threadId)) {
        skipReason = 'recursive_call';
    } else {
        try {
            bytes = actorBytes(context.ecx);
        } catch (error) {
            send({type: 'warning', message: `actor pre-read failed: ${error}`});
        }

        skipReason = chooseSample(site, context.ecx, context, bytes, globalValues);
    }

    const call = {sequence, site, threadId, actor: context.ecx, sampled: skipReason === null,
                  blocks: [], dropped: 0, exceptions: 0};

    calls.set(sequence, call);
    send({type: 'call', call_id: sequence, site, thread_id: threadId,
          actor: context.ecx.toString(), sampled: call.sampled, skip_reason: skipReason,
          eflags: context.eflags.toString()});

    if (call.sampled) {
        active.set(threadId, call);
        emitSnapshot('enter', call, context, bytes, globalValues);
        if (TRACE_ENABLED) {
            Stalker.follow(threadId, {
                events: {call: false, ret: false, exec: false, block: false, compile: false},

                transform: transformBlocks,
            });
        }
    }

    return sequence;
}

function frameLeave(cpu, flags, threadId, sequence) {
    const call = calls.get(sequence);
    if (call === undefined) {
        throw new Error(`missing native invocation ${sequence}`);
    }

    const context = readCpu(cpu, flags, 'frida_16_5_1_saved_pushfd');
    if (call.sampled) {
        if (TRACE_ENABLED) {
            Stalker.unfollow(threadId);
        }
        emitSnapshot('leave', call, context);
        if (TRACE_ENABLED) {
            send({type: 'blocks', sequence, site: call.site, chunk: 1,
                  blocks: call.blocks, dropped_total: call.dropped});

            send({type: 'trace-end', sequence, site: call.site,
                  block_count: call.blocks.length, dropped_total: call.dropped,
                  drain_completion_verified: true, capture_method: 'synchronous_callouts'});
            Stalker.garbageCollect();
        }
        active.delete(threadId);
    }

    send({type: 'call-return', call_id: sequence, site: call.site, thread_id: threadId,
          eax: context.eax.toString(), eflags: context.eflags.toString()});

    calls.delete(sequence);
}

function nestedEnter(cpu, flags, threadId, returnAddress) {
    if ((flags & 2) === 0) {
        throw new Error('invalid nested saved FLAGS reserved bit');
    }

    const parent = active.get(threadId);
    const kind = NESTED_RETURNS.get(returnAddress.toString().toLowerCase());
    if (parent === undefined || kind === undefined) {
        return 0;
    }

    const context = readCpu(cpu, flags, 'frida_16_5_1_saved_pushfd');
    let recordAddress = null;
    if (kind === 'action_update') {
        try {
            recordAddress = context.esp.add(4).readPointer();
        } catch (error) {
            send({type: 'warning', message: `nested action argument unreadable: ${error}`});
        }
    }

    const call = {id: ++nestedSequence, parentSequence: parent.sequence,
                  site: parent.site, threadId, kind, returnAddress,
                  recordAddress, expectedRecord: parent.actor.add(0x3d0)};
    nestedCalls.set(call.id, call);
    emitNestedSnapshot('enter', call, context);
    return call.id;
}

function nestedLeave(cpu, flags, threadId, id) {
    const call = nestedCalls.get(id);
    if (call === undefined || call.threadId !== threadId) {
        throw new Error(`unmatched nested invocation ${id}/${threadId}`);
    }

    const context = readCpu(cpu, flags, 'frida_16_5_1_saved_pushfd');
    emitNestedSnapshot('leave', call, context);
    nestedCalls.delete(id);
}

function exceptionFlags(details) {
    // ia32 Windows CONTEXT: ContextFlags+0, Eip+0xB8, EFlags+0xC0.
    const context = {};

    for (const name of CPU_FIELDS) {
        context[name] = details.context[name];
    }

    const native = details.nativeContext;
    if (native !== undefined && !native.isNull()) {
        try {
            // SDK CONTEXT_CONTROL = CONTEXT_i386 | 1; architecture alone
            // does not declare EIP/FLAGS valid.
            if ((native.readU32() & 0x10001) === 0x10001 &&
                ptr(native.add(0xb8).readU32()).equals(details.context.eip)) {
                context.eflags = ptr(native.add(0xc0).readU32());
                context.flags_source = 'win32_ia32_context';
            }
        } catch (error) {
            send({type: 'warning', message: `exception native context unavailable: ${error}`});
        }
    }

    return context;
}

if (Process.arch !== 'ia32' || Process.platform !== 'windows' ||
    Frida.version !== '16.5.1' || !Process.mainModule.base.equals(ptr('0x00400000'))) {
    throw new Error('requires Frida16.5.1 Windows ia32 image base 0x400000');
}

if (!Number.isInteger(MAX_SAMPLES_PER_SITE) || MAX_SAMPLES_PER_SITE <= 0 ||
    !Number.isInteger(REPEAT_INTERVAL) || REPEAT_INTERVAL <= 0) {
    throw new Error('invalid capture limits');
}

checkBytes(ENTRY, [0x83, 0xec, 0x14, 0x53, 0x55]);
checkBytes(ptr('0x004554F6'), [0xe8, 0x55, 0x43, 0x02, 0x00]);
checkBytes(ptr('0x0045650F'), [0xe8, 0x3c, 0x33, 0x02, 0x00]);
checkBytes(ptr('0x0045AA33'), [0xe8, 0x18, 0xee, 0x01, 0x00]);
checkBytes(ptr('0x0045ACBF'), [0xe8, 0x8c, 0xeb, 0x01, 0x00]);
checkBytes(ptr('0x00479921'), [0xe8, 0xba, 0x88, 0xfb, 0xff]);
checkBytes(ptr('0x00432421'), [0xe8, 0x2a, 0x06, 0x00, 0x00]);
checkBytes(ptr('0x00432434'), [0xe8, 0x87, 0x07, 0x00, 0x00]);
checkBytes(NESTED_ENTRIES[0], [0x53, 0x55, 0x56]);
checkBytes(NESTED_ENTRIES[1], [0xa1, 0x0c, 0xb3, 0x4f, 0x00]);
checkBytes(NESTED_ENTRIES[2], [0x51]);
if (LIST_PROBE_ENABLED) {
    checkBytes(LIST_QUERY_ENTRY, [0x83, 0xec, 0x28]);
    checkBytes(LIST_PROFILE_ENTRY, [0x51, 0x53, 0x8b]);
    checkBytes(ACTION_KIND_SETTER, [0x8b, 0x44, 0x24, 0x04]);
}

Process.setExceptionHandler(function (details) {
    const sample = active.get(Process.getCurrentThreadId());
    if (sample !== undefined) {
        sample.exceptions++;
        const context = exceptionFlags(details);
        send({type: 'exception', sequence: sample.sequence, site: sample.site,
              thread_id: sample.threadId, exception_type: details.type,
              address: details.address.toString(),
              memory: details.memory === undefined ? null : {
                  operation: details.memory.operation, address: details.memory.address.toString(),
              },

              registers: registers(context)});

        emitSnapshot(`exception-${sample.exceptions}`, sample, context);
    }

    return false;
});

// Frida16.5.1's inline-cache epilogue restores LAHF/SAHF bits, not OF.
// Disable trusted inline-cache paths; verify all seven FLAGS in the native probe.
Stalker.trustThreshold = -1;

const nativeEnter = new NativeCallback(frameEnter, 'uint', ['pointer', 'uint', 'uint', 'pointer']);
const nativeLeave = new NativeCallback(frameLeave, 'void', ['pointer', 'uint', 'uint', 'uint']);
const nestedNativeEnter = new NativeCallback(nestedEnter, 'uint', ['pointer', 'uint', 'uint', 'pointer']);
const nestedNativeLeave = new NativeCallback(nestedLeave, 'void', ['pointer', 'uint', 'uint', 'uint']);
const nativeBridge = new CModule(FRAME_NATIVE_BRIDGE_SOURCE, {
    frame_enter: nativeEnter, frame_leave: nativeLeave,
    nested_enter: nestedNativeEnter, nested_leave: nestedNativeLeave,
});

Interceptor.attach(ENTRY, {onEnter: nativeBridge.on_enter, onLeave: nativeBridge.on_leave});
for (const entry of NESTED_ENTRIES) {
    Interceptor.attach(entry, {onEnter: nativeBridge.on_nested_enter,
                               onLeave: nativeBridge.on_nested_leave});
}

if (LIST_PROBE_ENABLED) {
    Interceptor.attach(LIST_QUERY_ENTRY, {onEnter: scanNarrowList});
    Interceptor.attach(LIST_PROFILE_ENTRY,
                       {onEnter: profileEnter, onLeave: profileLeave});
    Interceptor.attach(ACTION_KIND_SETTER,
                       {onEnter: actionKindEnter, onLeave: actionKindLeave});
}

send({type: 'ready', schema_version: 5, entry: ENTRY.toString(), caller_count: RETURNS.size,
      nested_hook_count: NESTED_ENTRIES.length,
      list_hook_count: LIST_PROBE_ENABLED ? 3 : 0,
      max_samples_per_site: MAX_SAMPLES_PER_SITE, repeat_interval: REPEAT_INTERVAL,
      actor_bytes: ACTOR_BYTES, flags_method: 'frida_16_5_1_saved_pushfd',
      trace_method: TRACE_ENABLED ? 'synchronous_callouts' : 'disabled_for_nested_flags_probe'});
