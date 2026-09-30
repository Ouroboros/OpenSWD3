'use strict';

// Check the five hook anchors before resuming the suspended original.
// No business-state writes; Frida still instruments code and changes timing.
const ENTRY = ptr('0x00479850');
const FIRST = 0x00479850;
const LAST = 0x0047B9CA;
const ACTOR_BYTES = 0x2b00;
const MAX_SAMPLES = 512;
const MAX_BLOCKS = 16384;
const RETURNS = new Map([
    ['0x4554fb', 'action_group_b'],
    ['0x456514', 'opponent_group_a'],
    ['0x45aa38', 'final_group_a'],
    ['0x45acc4', 'final_group_b'],
]);
const active = new Map();
let sequence = 0;
let unexpectedCallerReported = false;

function checkBytes(address, bytes) {
    for (let i = 0; i < bytes.length; ++i) {
        if (address.add(i).readU8() !== bytes[i]) {
            throw new Error(`LST instruction mismatch at ${address.add(i)}`);
        }
    }
}

function registers(context) {
    const names = ['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp', 'esp', 'eip', 'eflags'];
    const result = {flags_available: context.eflags !== undefined};

    for (const name of names) {
        const value = context[name];
        if (value !== undefined) {
            result[name] = value.toString();
        }
    }

    return result;
}

function actorBytes(actor) {
    const range = Process.findRangeByAddress(actor);
    if (range === null || !range.protection.includes('r') ||
        range.base.add(range.size).compare(actor.add(ACTOR_BYTES)) < 0) {
        return null;
    }

    return actor.readByteArray(ACTOR_BYTES);
}

function globals() {
    const addresses = [
        '0x004A82F4', '0x004A8360', '0x0053E7B4', '0x0053E7B8',
        '0x00499138', '0x004CD730', '0x004CD71C', '0x004CD30C',
        '0x004CD304',
    ];
    const result = {};

    for (const address of addresses) {
        const p = ptr(address);
        try {
            result[address] = p.readU32().toString(16).padStart(8, '0');
        } catch (error) {
            result[address] = `unreadable:${error}`;
        }
    }

    return result;
}

function emitSnapshot(phase, sample, context) {
    let bytes = null;
    let problem = null;
    try {
        bytes = actorBytes(sample.actor);
        if (bytes === null) {
            problem = 'actor memory range is not fully readable';
        }
    } catch (error) {
        problem = String(error);
    }

    send({
        type: 'frame', phase, sequence: sample.sequence,
        site: sample.site, thread_id: sample.threadId,
        actor: sample.actor.toString(),
        registers: registers(context), globals: globals(),
        actor_bytes: bytes === null ? 0 : ACTOR_BYTES,
        problem,
    }, bytes);
    emitMemory(phase, sample, 'stack', context.esp, 128);
    for (const [name, offset, size] of [
        ['frame-header', 0x2548, 16],
        ['list-head-word', 0x2584, 4],
    ]) {
        try {
            const address = sample.actor.add(offset).readPointer();
            emitMemory(phase, sample, name, address, size);
        } catch (error) {
            send({type: 'memory', phase, sequence: sample.sequence,
                  region: name, byte_count: 0, problem: String(error)});
        }
    }
}

function emitMemory(phase, sample, region, address, size) {
    let bytes = null;
    let problem = null;
    try {
        const range = Process.findRangeByAddress(address);
        if (address.isNull()) {
            problem = 'null_pointer';
        } else if (range === null || !range.protection.includes('r') ||
                   range.base.add(range.size).compare(address.add(size)) < 0) {
            problem = 'range_not_readable';
        } else {
            bytes = address.readByteArray(size);
        }
    } catch (error) {
        problem = String(error);
    }

    send({type: 'memory', phase, sequence: sample.sequence, region,
          address: address.toString(), byte_count: bytes === null ? 0 : size,
          problem}, bytes);
}

if (Process.arch !== 'ia32' || !Process.mainModule.base.equals(ptr('0x00400000'))) {
    throw new Error(`unexpected architecture/image base: ${Process.arch}/${Process.mainModule.base}`);
}

checkBytes(ENTRY, [0x83, 0xec, 0x14, 0x53, 0x55]);
checkBytes(ptr('0x004554F6'), [0xe8, 0x55, 0x43, 0x02, 0x00]);
checkBytes(ptr('0x0045650F'), [0xe8, 0x3c, 0x33, 0x02, 0x00]);
checkBytes(ptr('0x0045AA33'), [0xe8, 0x18, 0xee, 0x01, 0x00]);
checkBytes(ptr('0x0045ACBF'), [0xe8, 0x8c, 0xeb, 0x01, 0x00]);

Process.setExceptionHandler(function (details) {
    const sample = active.get(Process.getCurrentThreadId());
    if (sample !== undefined) {
        sample.exceptions++;
        send({
            type: 'exception', sequence: sample.sequence,
            site: sample.site, thread_id: sample.threadId,
            exception_type: details.type,
            address: details.address.toString(),
            memory: details.memory === undefined ? null : {
                operation: details.memory.operation,
                address: details.memory.address.toString(),
            },

            registers: registers(details.context),
        });

        emitSnapshot(`exception-${sample.exceptions}`, sample, details.context);
    }

    return false; // Let the original Windows SEH/exception behavior proceed.
});

Interceptor.attach(ENTRY, {
    onEnter() {
        if (sequence >= MAX_SAMPLES) {
            this.sample = null;
            return;
        }

        if (active.has(this.threadId)) {
            this.sample = null;
            send({type: 'warning', message: 'recursive frame entry not sampled',
                  thread_id: this.threadId});

            return;
        }

        const returnAddress = this.returnAddress.toString().toLowerCase();
        const site = RETURNS.get(returnAddress);
        if (site === undefined) {
            this.sample = null;
            if (!unexpectedCallerReported) {
                unexpectedCallerReported = true;
                send({type: 'warning', message: `unrecognized frame caller return ${returnAddress}`});
            }

            return;
        }

        const sample = {
            sequence: ++sequence,
            threadId: this.threadId,
            site,
            actor: this.context.ecx,
            blocks: 0,
            dropped: 0,
            chunks: 0,
            exceptions: 0,
        };

        this.sample = sample;
        active.set(this.threadId, sample);
        emitSnapshot('enter', sample, this.context);
        Stalker.follow(this.threadId, {
            events: {call: false, ret: false, exec: false, block: true, compile: false},

            onReceive(events) {
                const blocks = [];
                for (const event of Stalker.parse(events, {stringify: true, annotate: true})) {
                    if (event[0] !== 'block') {
                        continue;
                    }

                    const address = parseInt(event[1], 16);
                    if (address < FIRST || address > LAST) {
                        continue;
                    }

                    if (sample.blocks < MAX_BLOCKS) {
                        blocks.push([event[1], event[2]]);
                        sample.blocks++;
                    } else {
                        sample.dropped++;
                    }
                }

                // Send chunks directly: a delayed drain after onLeave must
                // not disappear from a prematurely serialized final array.
                send({type: 'blocks', sequence: sample.sequence, site: sample.site,
                      chunk: ++sample.chunks, blocks, dropped_total: sample.dropped});
            },
        });
    },

    onLeave() {
        const sample = this.sample;
        if (sample === null) {
            return;
        }

        Stalker.unfollow(this.threadId);
        Stalker.flush();
        emitSnapshot('leave', sample, this.context);
        send({type: 'trace-end', sequence: sample.sequence, site: sample.site,
              drain_completion_verified: false});

        active.delete(this.threadId);
        Stalker.garbageCollect();
    },
});

send({type: 'ready', entry: ENTRY.toString(), caller_count: RETURNS.size,
      max_samples: MAX_SAMPLES, actor_bytes: ACTOR_BYTES});
