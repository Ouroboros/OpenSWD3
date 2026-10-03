#!/usr/bin/env node
/** Verify WP316 CALL/RET ledger against physical LST instruction bytes. */
import fs from 'node:fs';

const root = new URL('../../../', import.meta.url);
const lstPath = new URL('../swd3.exe_export_for_ai/swd3.exe.lst', root);
const callLedger = process.argv[2] ?? 'build/workpack316/call-audit.tsv';
const returnLedger = process.argv[3] ?? 'build/workpack316/return-audit.tsv';
if (process.argv.length > 4 || callLedger.startsWith('/') ||
    returnLedger.startsWith('/')) {
    throw new Error(
        'usage: verify_call_bytes.mjs [call-ledger] [return-ledger]'
    );
}

const calls = fs.readFileSync(new URL(callLedger, root), 'utf8')
    .trimEnd().split(/\r?\n/).slice(1).map((line) => line.split('\t'));
const returns = fs.readFileSync(new URL(returnLedger, root), 'utf8')
    .trimEnd().split(/\r?\n/).slice(1).map((line) => line.split('\t'));
const callIps = new Set(calls.map((row) => row[1].slice(2).toUpperCase()));
const returnIps = new Set(returns.map((row) => row[0].slice(2).toUpperCase()));
const relevant = new Set([...callIps, ...returnIps]);
const physical = new Map();

for (const line of fs.readFileSync(lstPath, 'utf8').split(/\r?\n/)) {
    const match = line.match(
        /^\.text:([0-9A-F]{8})\s+([0-9A-F]{2}(?:\s+[0-9A-F]{2})*)(?:\s+(.+))?$/
    );
    if (match === null || !relevant.has(match[1])) {
        continue;
    }

    const bytes = match[2].trim().split(/\s+/)
        .map((byte) => Number.parseInt(byte, 16));
    const instruction = match[3]?.trimStart();
    if (instruction !== undefined && /^(?:call|retn)\b/.test(instruction)) {
        if (physical.has(match[1])) {
            throw new Error(`duplicate instruction 0x${match[1]}`);
        }
        physical.set(match[1], {bytes, text: match[3]});
    } else if (match[3] === undefined && physical.has(match[1])) {
        physical.get(match[1]).bytes.push(...bytes);
    }
}

const errors = [];
let direct = 0;
let indirect = 0;
for (const row of calls) {
    const ip = row[1].slice(2).toUpperCase();
    const instruction = physical.get(ip);
    if (instruction === undefined) {
        errors.push(`CALL ${ip}: no physical instruction`);
        continue;
    }

    const {bytes, text} = instruction;
    if (!text.startsWith(`call    ${row[3]}`)) {
        errors.push(`CALL ${ip}: ledger ${row[3]}, LST ${text}`);
    }

    if (bytes[0] === 0xE8 && bytes.length === 5) {
        ++direct;
        const displacement = (
            bytes[1] | (bytes[2] << 8) | (bytes[3] << 16) | (bytes[4] << 24)
        );
        const target = (Number.parseInt(ip, 16) + 5 + displacement) >>> 0;
        const expected = /^sub_([0-9A-F]+)$/.exec(row[3]);
        if (expected === null || target !== Number.parseInt(expected[1], 16)) {
            errors.push(
                `CALL ${ip}: bytes target 0x${target.toString(16)}, ` +
                `ledger ${row[3]}`
            );
        }
    } else if (bytes[0] === 0xFF && bytes[1] === 0xD7 && bytes.length === 2) {
        ++indirect;
        if (row[3] !== 'edi') {
            errors.push(`CALL ${ip}: FF D7 is indirect EDI, ledger ${row[3]}`);
        }
    } else {
        const encoding = bytes.map((byte) => byte.toString(16)).join(' ');
        errors.push(`CALL ${ip}: unsupported encoding ${encoding}`);
    }
}

function relativeStackValue(value) {
    const match = /^([+-]?)(\d+)$/.exec(value);
    if (match === null) {
        throw new Error(`unparsed RET ESP delta ${value}`);
    }
    return (match[1] === '-' ? -1 : 1) * Number(match[2]);
}

for (const row of returns) {
    const ip = row[0].slice(2).toUpperCase();
    const instruction = physical.get(ip);
    if (instruction === undefined) {
        errors.push(`RET ${ip}: no physical instruction`);
        continue;
    }

    const {bytes, text} = instruction;
    const pop = bytes[0] === 0xC3 && bytes.length === 1 ? 0
        : bytes[0] === 0xC2 && bytes.length === 3
        ? bytes[1] | (bytes[2] << 8)
        : null;
    if (!text.startsWith('retn') || pop === null) {
        errors.push(`RET ${ip}: unsupported encoding or mnemonic ${text}`);
        continue;
    }

    const before = relativeStackValue(row[4]);
    const after = relativeStackValue(row[5]);
    if (after !== before + 4 + pop) {
        errors.push(
            `RET ${ip}: bytes pop ${pop}, ledger ESP ${row[4]} -> ${row[5]}`
        );
    }
}

if (calls.length !== 98 || returns.length !== 22 || direct !== 92 ||
    indirect !== 6 || physical.size !== 120) {
    errors.push(
        `cardinality: calls=${calls.length}, returns=${returns.length}, ` +
        `direct=${direct}, indirect=${indirect}, physical=${physical.size}`
    );
}

console.log(JSON.stringify({
    calls: calls.length, direct, indirect, returns: returns.length,
    physical: physical.size, errors,
}, null, 2));
if (errors.length !== 0) {
    process.exitCode = 1;
}
