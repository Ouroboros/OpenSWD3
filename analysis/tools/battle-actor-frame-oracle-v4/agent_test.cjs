'use strict';

// Synthetic runtime only. Real native ABI coverage is in native_probe_test.py.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(`${__dirname}/agent.js`, 'utf8');
const bridge = fs.readFileSync(`${__dirname}/flags_bridge.c`, 'utf8');
const memory = new Map();
const code = new Map();
for (const [address, bytes] of [
    [0x479850, [0x83, 0xec, 0x14, 0x53, 0x55]],
    [0x4554f6, [0xe8, 0x55, 0x43, 0x02, 0x00]],
    [0x45650f, [0xe8, 0x3c, 0x33, 0x02, 0x00]],
    [0x45aa33, [0xe8, 0x18, 0xee, 0x01, 0x00]],
    [0x45acbf, [0xe8, 0x8c, 0xeb, 0x01, 0x00]],
    [0x479921, [0xe8, 0xba, 0x88, 0xfb, 0xff]],
    [0x432421, [0xe8, 0x2a, 0x06, 0x00, 0x00]],
    [0x432434, [0xe8, 0x87, 0x07, 0x00, 0x00]],
    [0x4321e0, [0x53, 0x55, 0x56]],
    [0x432a50, [0xa1, 0x0c, 0xb3, 0x4f, 0x00]],
    [0x432bc0, [0x51]],
]) {
    bytes.forEach((value, index) => code.set(address + index, value));
}

function pointer(value) {
    const n = typeof value === 'string' ? parseInt(value, 16) : value;
    return {
        add(offset) { return pointer(n + offset); },

        compare(other) { return Math.sign(n - Number(other.toString())); },

        equals(other) { return n === Number(other.toString()); },

        isNull() { return n === 0; },

        toString() { return `0x${n.toString(16)}`; },

        readU8() { return code.get(n); },

        readU32() { return memory.get(n) || 0; },

        readPointer() { return pointer(this.readU32()); },

        readByteArray(size) { return new ArrayBuffer(size); },
    };
}

const cpu = pointer(0x130000);
[0x479850, 1, 2, 0x120100, 0x120000, 3, 4, 0x500000, 1].forEach(
    (value, i) => memory.set(0x130000 + i * 4, value));
memory.set(0x120004, 0x5003d0);

function fixture(overrides = {}) {
    const messages = [];
    const follows = [];
    const unfollows = [];
    const hooks = new Map();
    let handler;
    const sandbox = {
        ptr: pointer, Frida: {version: '16.5.1'},

        FRAME_NATIVE_BRIDGE_SOURCE: bridge,
        FRAME_CAPTURE_OPTIONS: {max_samples_per_site: 2, repeat_interval: 3},

        send(payload, data) { messages.push({payload, data}); },

        NativeCallback: function (callback) { return callback; },

        CModule: function (text, symbols) {
            return {on_enter: symbols.frame_enter, on_leave: symbols.frame_leave,
                on_nested_enter: symbols.nested_enter, on_nested_leave: symbols.nested_leave};
        },

        Process: {
            arch: 'ia32', platform: 'windows', mainModule: {base: pointer(0x400000)},

            findRangeByAddress() {
                return {base: pointer(0), size: 0x10000000, protection: 'rw-'};
            },

            getCurrentThreadId() { return 7; },

            setExceptionHandler(callback) { handler = callback; },
        },

        Interceptor: {
            attach(address, callbacks) { hooks.set(address.toString(), callbacks); },
        },

        Stalker: {
            follow(threadId, options) { follows.push(options); },

            unfollow(threadId) { unfollows.push(threadId); },

            garbageCollect() {},
        },

        ...overrides,
    };

    vm.runInNewContext(source, sandbox);
    return {messages, follows, unfollows,
        enter(address = 0x45aa38, flags = 0x603) {
            return hooks.get('0x479850').onEnter(cpu, flags, 7, pointer(address));
        },

        leave(sequence) { hooks.get('0x479850').onLeave(cpu, 0xed7, 7, sequence); },

        nestedEnter(entry, ret, flags = 0x603) {
            return hooks.get(entry).onEnter(cpu, flags, 7, pointer(ret));
        },

        nestedLeave(entry, sequence, flags = 0xed7) {
            hooks.get(entry).onLeave(cpu, flags, 7, sequence);
        },

        block(index) {
            const instructions = [0x479858, 0x47985b].map(
                address => ({address: pointer(address), size: 3}));
            const callbacks = [];
            let cursor = 0;
            follows[index].transform({
                next() { return instructions[cursor++] || null; },

                keep() {},

                putCallout(callback) { callbacks.push(callback); },
            });

            callbacks.forEach(callback => callback());
        },

        exception(details) { return handler(details); }};
}

// Entry/leave globals are observations, not values at nested CALLs.
const newGlobals = [
    [0x004a0e78, 640], [0x004a0e7c, 480],
    [0x004ab784, 0x319], [0x004fb308, 1],
];
for (const [address, value] of newGlobals) {
    memory.set(address, value);
}

const globalFixture = fixture();
const globalFirst = globalFixture.enter();
globalFixture.block(0);
globalFixture.leave(globalFirst);
const globalEntry = globalFixture.messages.find(m => m.payload.type === 'frame' && m.payload.phase === 'enter');
for (const [address, value] of newGlobals) {
    assert.equal(globalEntry.payload.globals[`0x${address.toString(16).toUpperCase().padStart(8, '0')}`],
        value.toString(16).padStart(8, '0'));
}

assert.equal(globalFixture.messages.find(m => m.payload.type === 'ready').payload.schema_version, 4);
memory.set(0x004a0e78, 639);
const changedGlobal = globalFixture.enter();
globalFixture.block(1);
globalFixture.leave(changedGlobal);
assert.equal(globalFixture.follows.length, 2); // Global changes retain the next snapshot.
for (const [address] of newGlobals) {
    memory.delete(address);
}

const unreadableFixture = fixture({ptr(value) {
    const address = pointer(value);
    if (Number(address.toString()) === 0x004a0e78) {
        address.readU32 = () => { throw new Error('unreadable global'); };
    }

    return address;
}});
const unreadableId = unreadableFixture.enter();
assert.match(unreadableFixture.messages.find(m => m.payload.type === 'frame' &&
    m.payload.phase === 'enter').payload.globals['0x004A0E78'], /^unreadable:/);
unreadableFixture.block(0);
unreadableFixture.leave(unreadableId);

const noTrace = fixture({FRAME_CAPTURE_OPTIONS: {
    max_samples_per_site: 2, repeat_interval: 3, trace_enabled: false,
}});
const noTraceParent = noTrace.enter();
const noTraceChild = noTrace.nestedEnter('0x4321e0', 0x479926);
noTrace.nestedLeave('0x4321e0', noTraceChild);
noTrace.leave(noTraceParent);
assert.equal(noTrace.follows.length, 0);
assert.equal(noTrace.unfollows.length, 0);
assert.equal(noTrace.messages.filter(m => m.payload.type === 'trace-end' ||
    m.payload.type === 'blocks').length, 0);
assert.equal(noTrace.messages.find(m => m.payload.type === 'ready').payload.trace_method,
    'disabled_for_nested_flags_probe');

const f = fixture();
assert.equal(f.nestedEnter('0x4321e0', 0x479926), 0); // Unsampled child is ignored.
const first = f.enter();
const update = f.nestedEnter('0x4321e0', 0x479926, 0x603);
const cached = f.nestedEnter('0x432bc0', 0x432439, 0xa82);
f.nestedLeave('0x432bc0', cached, 0x247);
f.nestedLeave('0x4321e0', update, 0xed7);
const direct = f.nestedEnter('0x432a50', 0x432426);
f.nestedLeave('0x432a50', direct);
assert.equal(f.nestedEnter('0x432bc0', 0x123456), 0);
const nestedCalls = f.messages.filter(m => m.payload.type === 'nested-call');
const nestedReturns = f.messages.filter(m => m.payload.type === 'nested-return');
assert.equal(nestedCalls.length, 3);
assert.equal(nestedReturns.length, 3);
assert.equal(nestedCalls[0].payload.parent_sequence, first);
assert.equal(nestedCalls[0].payload.kind, 'action_update');
assert.equal(nestedCalls[0].payload.record_address, '0x5003d0');
assert.equal(nestedCalls[0].payload.expected_record_address, '0x5003d0');
assert.equal(nestedCalls[1].payload.registers.eflags, '0xa82');
assert.equal(nestedReturns[1].payload.registers.eflags, '0xed7');
assert.equal(f.messages.filter(m => m.payload.type === 'nested-memory').length, 8);
assert.equal(f.messages.filter(m => m.payload.type === 'nested-memory' &&
    m.payload.region === 'action-record').length, 2);
f.block(0);
f.leave(first);
const entered = f.messages.find(m => m.payload.phase === 'enter');
assert.equal(entered.payload.registers.eflags, '0x603');
assert.equal(entered.payload.registers.flags_available, true);
assert.equal(entered.data.byteLength, 0x2b28);
assert.equal(f.messages.find(m => m.payload.type === 'trace-end').payload.block_count, 1);
assert.equal(f.messages.find(m => m.payload.type === 'trace-end').payload.drain_completion_verified, true);
for (let i = 0; i < 2; i++) {
    f.leave(f.enter());
}

assert.equal(f.follows.length, 1);
assert.ok(f.messages.some(m => m.payload.skip_reason === 'unchanged_captured_input'));
const repeated = f.enter();
f.block(1);
f.leave(repeated);
assert.equal(f.follows.length, 2);
f.leave(f.enter(0x45aa38, 0x247));
assert.ok(f.messages.some(m => m.payload.skip_reason === 'sample_limit'));
const otherSite = f.enter(0x4554fb);
f.block(2);
f.leave(otherSite);
assert.equal(f.follows.length, 3); // One site's budget never blocks another site.

const parent = f.enter(0x45acc4);
const recursive = f.enter(0x4554fb);
f.leave(recursive);
const before = f.unfollows.length;
const originalContext = {eip: pointer(0x479850), esp: pointer(0x120000)};

memory.set(0x140000, 0x10001); // SDK ia32 CONTEXT_CONTROL includes FLAGS.
memory.set(0x140000 + 0xb8, 0x479850);
memory.set(0x140000 + 0xc0, 0x603);
assert.equal(f.exception({
    type: 'access-violation', address: pointer(0), context: originalContext,
    nativeContext: pointer(0x140000),
}), false);
assert.equal(originalContext.eflags, undefined); // Never modify the real CpuContext.
const exception = f.messages.find(m => m.payload.type === 'exception');
assert.equal(exception.payload.registers.eflags, '0x603');
assert.equal(exception.payload.registers.flags_source, 'win32_ia32_context');

// SDK winnt.h: only CONTROL declares EIP/FLAGS valid, not the architecture bit.
const contextFixture = fixture();
const contextId = contextFixture.enter();
for (const spec of [
    {name: 'CONTROL', flags: 0x10001, available: true, reads: [0, 0xb8, 0xc0]},
    {name: 'FULL', flags: 0x10007, available: true, reads: [0, 0xb8, 0xc0]},
    {name: 'architecture only', flags: 0x10000, available: false, reads: [0]},
    {name: 'INTEGER without CONTROL', flags: 0x10002, available: false, reads: [0]},
    {name: 'CONTROL without ia32', flags: 1, available: false, reads: [0]},
    {name: 'no flags', flags: 0, available: false, reads: [0]},
    {name: 'wrong EIP', flags: 0x10001, eip: 0x479851,
        available: false, reads: [0, 0xb8]},
    {name: 'unreadable ContextFlags', flags: 0x10001, unreadable: true,
        available: false, reads: [0]},
]) {
    const reads = [];
    const nativeContext = {
        isNull() { return false; },

        readU32() {
            reads.push(0);
            if (spec.unreadable) {
                throw new Error('unreadable native ContextFlags');
            }

            return spec.flags;
        },

        add(offset) {
            assert.ok(offset === 0xb8 || offset === 0xc0);
            return {
                readU32() {
                    reads.push(offset);
                    return offset === 0xb8 ? (spec.eip ?? 0x479850) : 0xed7;
                },
            };
        },
    };

    const context = {eip: pointer(0x479850), esp: pointer(0x120000)};
    assert.equal(contextFixture.exception({
        type: 'access-violation', address: pointer(0), context, nativeContext,
    }), false, spec.name);
    const event = contextFixture.messages.filter(m => m.payload.type === 'exception').at(-1);
    assert.equal(event.payload.registers.flags_available, spec.available, spec.name);
    assert.equal(event.payload.registers.eflags,
        spec.available ? '0xed7' : undefined, spec.name);
    assert.equal(event.payload.registers.flags_source,
        spec.available ? 'win32_ia32_context' : 'unavailable', spec.name);
    assert.deepEqual(reads, spec.reads, spec.name);
    assert.equal(context.eflags, undefined, spec.name);
    const snapshot = contextFixture.messages.filter(m => m.payload.type === 'frame' &&
        m.payload.phase.startsWith('exception-')).at(-1);
    assert.equal(snapshot.payload.registers.flags_available, spec.available, spec.name);
}

contextFixture.block(0);
contextFixture.leave(contextId);
f.block(3);
f.leave(parent);
assert.equal(f.unfollows.length, before + 1);
assert.equal(f.enter(0x123456), 0);
assert.throws(() => f.enter(0x4554fb, 0), /invalid saved FLAGS/);
assert.throws(() => fixture({Frida: {version: '17.0.0'}}), /requires Frida16.5.1/);
function rangeProcess(gap) {
    return {
        arch: 'ia32', platform: 'windows', mainModule: {base: pointer(0x400000)},

        getCurrentThreadId() { return 7; },

        setExceptionHandler() {},

        findRangeByAddress(address) {
            const n = Number(address.toString());
            if (n >= 0x500000 && n < 0x502000) {
                return {base: pointer(0x500000), size: 0x2000, protection: 'r--'};
            }

            if (n >= 0x502000 && n < 0x503000) {
                return gap ? null : {base: pointer(0x502000), size: 0x1000, protection: 'rw-'};
            }

            return {base: pointer(0), size: 0x10000000, protection: 'rw-'};
        },
    };
}

const contiguous = fixture({Process: rangeProcess(false)});

const contiguousId = contiguous.enter();
contiguous.block(0);
contiguous.leave(contiguousId);
assert.equal(contiguous.messages.find(m => m.payload.type === 'frame').payload.actor_bytes, 0x2b28);
const gap = fixture({Process: rangeProcess(true)});

const gapId = gap.enter();
gap.block(0);
gap.leave(gapId);
assert.equal(gap.messages.find(m => m.payload.type === 'frame').payload.actor_bytes, 0);
console.log('agent synthetic tests passed: trace-disabled nested capture, parent-linked action/direct/cached boundaries, paired record/stack bytes, FLAGS, globals, quotas, recursion, ia32 CONTROL, mappings');
