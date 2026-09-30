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

function fixture(overrides = {}) {
    const messages = [];
    const follows = [];
    const unfollows = [];
    let hook;
    let handler;
    const sandbox = {
        ptr: pointer, Frida: {version: '16.5.1'},

        FRAME_NATIVE_BRIDGE_SOURCE: bridge,
        FRAME_CAPTURE_OPTIONS: {max_samples_per_site: 2, repeat_interval: 3},

        send(payload, data) { messages.push({payload, data}); },

        NativeCallback: function (callback) { return callback; },

        CModule: function (text, symbols) {
            return {on_enter: symbols.frame_enter, on_leave: symbols.frame_leave};
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
            attach(address, callbacks) { hook = callbacks; },
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
            return hook.onEnter(cpu, flags, 7, pointer(address));
        },

        leave(sequence) { hook.onLeave(cpu, 0xed7, 7, sequence); },

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

const f = fixture();
const first = f.enter();
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

memory.set(0x140000, 0x10000);
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
console.log('agent synthetic tests passed: FLAGS, synchronous trace, repeat sampling, caller quotas, recursion, exception context copy, version guard, contiguous mappings, unreadable gap');
