'use strict';

// Simulated Frida callbacks only. No process attachment or original execution.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(`${__dirname}/agent.js`, 'utf8');
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

        readU32() { return 0; },

        readPointer() { return pointer(0); },

        readByteArray(size) { return new ArrayBuffer(size); },
    };
}

function fixture() {
    const messages = [];
    const follows = [];
    const unfollows = [];
    let hook;
    let exceptionHandler;
    const sandbox = {
        ptr: pointer,
        send(payload, data) { messages.push({payload, data}); },

        Process: {
            arch: 'ia32', mainModule: {base: pointer(0x400000)},

            findRangeByAddress() {
                return {base: pointer(0), size: 0x10000000, protection: 'rw-'};
            },

            getCurrentThreadId() { return 7; },

            setExceptionHandler(callback) { exceptionHandler = callback; },
        },

        Interceptor: {
            attach(address, callbacks) { hook = callbacks; },
        },

        Stalker: {
            follow(threadId, options) { follows.push(options); },

            parse(events, options) {
                assert.equal(options.annotate, true);
                return events;
            },

            unfollow(threadId) { unfollows.push(threadId); },

            flush() {},

            garbageCollect() {},
        },
    };

    vm.runInNewContext(source, sandbox);
    return {messages, follows, unfollows, hook,
        exception(details) { return exceptionHandler(details); }};
}

function invocation(address = 0x4554fb) {
    return {
        threadId: 7,
        returnAddress: pointer(address),
        context: {
            eax: pointer(1), ecx: pointer(0x500000), esp: pointer(0x120000),
            eip: pointer(0x479850),
        },
    };
}

const f = fixture();
const call = invocation();
f.hook.onEnter.call(call);
assert.equal(f.follows.length, 1);
const entered = f.messages.find(m => m.payload.phase === 'enter' &&
    m.payload.type === 'frame');
assert.equal(entered.payload.registers.flags_available, false);
assert.equal(entered.data.byteLength, 0x2b00);
f.follows[0].onReceive([
    ['block', '0x479850', '0x479867'],
    ['call', '0x4798f7', '0x478850'],
    ['block', '0x478850', '0x478856'],
]);
f.hook.onLeave.call(call);
// Drain arriving after onLeave must remain in the event stream.
f.follows[0].onReceive([['block', '0x47991f', '0x479920']]);
const chunks = f.messages.filter(m => m.payload.type === 'blocks');
assert.equal(chunks.length, 2);
assert.equal(chunks[0].payload.blocks.length, 1);
assert.equal(chunks[1].payload.blocks[0][0], '0x47991f');
assert.equal(chunks[1].payload.sequence, entered.payload.sequence);
assert.equal(f.unfollows.length, 1);

const unknown = invocation(0x123456);
f.hook.onEnter.call(unknown);
f.hook.onLeave.call(unknown);
assert.equal(f.follows.length, 1);
assert.ok(f.messages.some(m => m.payload.type === 'warning'));

const parent = invocation(0x45aa38);
f.hook.onEnter.call(parent);
const recursive = invocation(0x45aa38);
f.hook.onEnter.call(recursive);
f.hook.onLeave.call(recursive);
assert.equal(f.unfollows.length, 1);
assert.equal(f.exception({
    type: 'access-violation', address: pointer(0),
    memory: {operation: 'read', address: pointer(0)}, context: parent.context,
}), false);
assert.ok(f.messages.some(m => m.payload.phase === 'exception-1'));
f.hook.onLeave.call(parent);
assert.equal(f.unfollows.length, 2);

const wrongArch = {
    ptr: pointer, Process: {arch: 'x64', mainModule: {base: pointer(0x400000)}},
};

assert.throws(() => vm.runInNewContext(source, wrongArch), /unexpected architecture/);
console.log('agent simulated callback tests passed: labels, delayed chunks, missing FLAGS, recursion, exceptions, architecture');
