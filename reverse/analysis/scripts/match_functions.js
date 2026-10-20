// Matches the functions of one build (a) with those of another (b) from their fingerprints (fingerprints.py), and
// derives the maps of global addresses and of structure offsets from the matched functions.
//
// Usage: node match_functions.js <a.jsonl> <a tokens> <b.jsonl> <b tokens> <out dir>
//   (the token dirs are the FINGERPRINT_TOKENS output of fingerprints.py; "-" leaves out the offset map)
//
// The matching, each step only between functions still unmatched:
//   1. hash: the same normalised code, unique on both sides;
//   2. shape: the same instruction shapes (other numbers allowed), unique on both sides;
//   3. anchor: the same strings, or the same imports, unique on both sides and similar enough;
//   4. calls: the callees of a matched pair taken in call order (when both call as many), and the callers of a
//      matched pair that are each other's best;
//   5. similarity: each other's best by similarity, above a threshold;
//   6. neighbours: the unmatched functions between two matched neighbours, in order, as many on both sides.
// The similarity: the share of shape blocks in common, the ratio of instruction counts, the strings and imports in
// common. A pair is reusable when its code is the same (hash) and every callee and global it uses maps to the
// counterpart the other matches give.
//
// Out: functions.tsv, globals.tsv, offsets.tsv, summary.txt.

'use strict';

const fs = require('fs');
const path = require('path');

const [aPath, aTokens, bPath, bTokens, outDir] = process.argv.slice(2);
const SIMILAR = { anchor: 0.3, calls: 0.35, callers: 0.5, best: 0.6, gap: 0.3 };
const CONFIDENCE = { hash: 1, shape: 0.97 };

function load(file) {
    return fs.readFileSync(file, 'utf8').trim().split('\n').map((line) => JSON.parse(line));
}

function counts(list) {
    const map = new Map();
    for (const item of list) {
        map.set(item, (map.get(item) || 0) + 1);
    }
    return map;
}

function prepare(records) {
    for (const record of records) {
        record.blockCounts = counts(record.shape_blocks);
        record.anchorSet = new Set([...record.strings, ...record.imports]);
        record.calls = [...record.callees, ...record.tailcalls].filter((callee) => callee.startsWith('0x'));
        record.callers = [];
    }
    const byAddress = new Map(records.map((record) => [record.address, record]));
    for (const record of records) {
        for (const callee of new Set(record.calls)) {
            const target = byAddress.get(callee);
            if (target) {
                target.callers.push(record.address);
            }
        }
    }
    return byAddress;
}

const a = load(aPath);
const b = load(bPath);
const aBy = prepare(a);
const bBy = prepare(b);

function similarity(x, y) {
    let common = 0;
    for (const [hash, count] of x.blockCounts) {
        common += Math.min(count, y.blockCounts.get(hash) || 0);
    }
    const blocks = common / Math.max(x.shape_blocks.length, y.shape_blocks.length, 1);
    const size = Math.min(x.instructions, y.instructions) / Math.max(x.instructions, y.instructions, 1);
    let anchors = 1;
    if (x.anchorSet.size || y.anchorSet.size) {
        let shared = 0;
        for (const item of x.anchorSet) {
            if (y.anchorSet.has(item)) {
                shared += 1;
            }
        }
        anchors = shared / (x.anchorSet.size + y.anchorSet.size - shared);
    }
    return 0.6 * blocks + 0.2 * size + 0.2 * anchors;
}

const aToB = new Map();
const bToA = new Map();
const pairs = [];

function pair(x, y, method, confidence) {
    if (aToB.has(x.address) || bToA.has(y.address)) {
        return false;
    }
    aToB.set(x.address, y.address);
    bToA.set(y.address, x.address);
    pairs.push({ a: x, b: y, method, confidence });
    return true;
}

function uniqueByKey(key, method, check) {
    const left = new Map();
    const right = new Map();
    for (const x of a) {
        const k = !aToB.has(x.address) && key(x);
        if (k) {
            left.set(k, left.has(k) ? null : x);
        }
    }
    for (const y of b) {
        const k = !bToA.has(y.address) && key(y);
        if (k) {
            right.set(k, right.has(k) ? null : y);
        }
    }
    let made = 0;
    for (const [k, x] of left) {
        const y = right.get(k);
        if (x && y) {
            const score = check ? similarity(x, y) : CONFIDENCE[method];
            if (!check || score >= check) {
                made += pair(x, y, method, check ? score : CONFIDENCE[method]) ? 1 : 0;
            }
        }
    }
    return made;
}

uniqueByKey((record) => record.hash, 'hash', 0);
uniqueByKey((record) => record.shape, 'shape', 0);
uniqueByKey((record) => (record.strings.length ? 'S' + record.strings.join('\u0001') : null), 'anchor', SIMILAR.anchor);
uniqueByKey((record) => {
    const imports = [...new Set(record.imports)].sort();
    return imports.length ? 'I' + imports.join('\u0001') : null;
}, 'anchor', SIMILAR.anchor);

// 4: the call graph, until nothing changes.
for (let changed = true; changed;) {
    changed = false;
    for (const { a: x, b: y } of [...pairs]) {
        if (x.calls.length === y.calls.length) {
            for (let i = 0; i < x.calls.length; ++i) {
                const p = aBy.get(x.calls[i]);
                const q = bBy.get(y.calls[i]);
                if (p && q && !aToB.has(p.address) && !bToA.has(q.address)) {
                    const score = similarity(p, q);
                    if (score >= SIMILAR.calls && pair(p, q, 'calls', score)) {
                        changed = true;
                    }
                }
            }
        }

        const left = x.callers.map((address) => aBy.get(address)).filter((p) => !aToB.has(p.address));
        const right = y.callers.map((address) => bBy.get(address)).filter((q) => !bToA.has(q.address));
        for (const p of left) {
            let best = null;
            for (const q of right) {
                const score = similarity(p, q);
                if (!best || score > best.score) {
                    best = { q, score };
                }
            }
            if (!best || best.score < SIMILAR.callers) {
                continue;
            }
            let back = null;
            for (const r of left) {
                const score = similarity(r, best.q);
                if (!back || score > back.score) {
                    back = { r, score };
                }
            }
            if (back.r === p && pair(p, best.q, 'callers', best.score)) {
                changed = true;
            }
        }
    }
}

// 5: each other's best among the rest.
{
    const left = a.filter((x) => !aToB.has(x.address));
    const right = b.filter((y) => !bToA.has(y.address));
    const bestOf = (x, candidates) => {
        let best = null;
        for (const y of candidates) {
            const ratio = x.instructions / Math.max(y.instructions, 1);
            if (ratio < 0.5 || ratio > 2) {
                continue;
            }
            const score = similarity(x, y);
            if (!best || score > best.score) {
                best = { y, score };
            }
        }
        return best;
    };
    for (const x of left) {
        const best = bestOf(x, right);
        if (best && best.score >= SIMILAR.best && !bToA.has(best.y.address)) {
            const back = bestOf(best.y, left.filter((p) => !aToB.has(p.address)));
            if (back && back.y === x) {
                pair(x, best.y, 'similarity', best.score);
            }
        }
    }
}

// 6: the gaps between neighbours. Functions keep their order within a source file, so the unmatched functions
// between two matched neighbours whose counterparts are neighbours too are taken in order, when as many lie on both
// sides and each pair is similar enough.
for (let changed = true; changed;) {
    changed = false;
    const aOrder = [...a].sort((p, q) => parseInt(p.address, 16) - parseInt(q.address, 16));
    const bOrder = [...b].sort((p, q) => parseInt(p.address, 16) - parseInt(q.address, 16));
    const bIndex = new Map(bOrder.map((record, i) => [record.address, i]));
    let last = -1;
    for (let i = 0; i < aOrder.length; ++i) {
        const mate = aToB.get(aOrder[i].address);
        if (mate === undefined) {
            continue;
        }
        if (last >= 0) {
            const from = bIndex.get(aToB.get(aOrder[last].address));
            const to = bIndex.get(mate);
            const left = aOrder.slice(last + 1, i).filter((x) => !aToB.has(x.address));
            const right = to > from ? bOrder.slice(from + 1, to).filter((y) => !bToA.has(y.address)) : [];
            if (left.length && left.length === right.length && left.length === i - last - 1 && right.length === to - from - 1) {
                const scores = left.map((x, k) => similarity(x, right[k]));
                if (scores.every((score) => score >= SIMILAR.gap)) {
                    left.forEach((x, k) => pair(x, right[k], 'neighbours', scores[k]));
                    changed = true;
                }
            }
        }
        last = i;
    }
}

// The maps of globals and structure offsets, from the pairs of the same shape.
const globalEvidence = new Map();
const offsetEvidence = new Map();
const OPERAND = /\[([a-z]{2,3})([+-])([0-9A-F]+)h?\]|\[([a-z]{2,3})([+-])(\d+)\]/g;

function readTokens(dir, address) {
    const file = path.join(dir, address.slice(2) + '.txt');
    return fs.existsSync(file) ? fs.readFileSync(file, 'utf8').trimEnd().split('\n').map((line) => line.split('\t')) : [];
}

for (const { a: x, b: y } of pairs) {
    if (x.shape !== y.shape) {
        continue;
    }
    if (x.globals.length === y.globals.length) {
        x.globals.forEach((global, i) => {
            const key = global + '\t' + y.globals[i];
            globalEvidence.set(key, (globalEvidence.get(key) || 0) + 1);
        });
    }
    if (aTokens === '-' || bTokens === '-') {
        continue;
    }
    const left = readTokens(aTokens, x.address);
    const right = readTokens(bTokens, y.address);
    left.forEach((line, i) => {
        const other = right[i];
        if (!other || line[2] === other[2]) {
            return;
        }
        const leftOperands = [...line[2].matchAll(OPERAND)];
        const rightOperands = [...other[2].matchAll(OPERAND)];
        leftOperands.forEach((match, j) => {
            const counterpart = rightOperands[j];
            if (!counterpart) {
                return;
            }
            const value = (m) => (m[3] !== undefined ? parseInt(m[3], 16) : parseInt(m[6], 10)) * ((m[2] || m[5]) === '-' ? -1 : 1);
            const base = match[1] || match[4];
            const from = value(match);
            const to = value(counterpart);
            if (from !== to && from > 0 && base !== 'ebp' && base !== 'esp') {
                const key = from + '\t' + to;
                const entry = offsetEvidence.get(key) || { count: 0, functions: new Set() };
                entry.count += 1;
                entry.functions.add(x.address);
                offsetEvidence.set(key, entry);
            }
        });
    });
}

// One mapping per global: the counterpart most of the evidence gives, and whether all of it agrees.
const globalMap = new Map();
for (const [key, count] of globalEvidence) {
    const [from, to] = key.split('\t');
    const entry = globalMap.get(from) || { to: new Map(), total: 0 };
    entry.to.set(to, count);
    entry.total += count;
    globalMap.set(from, entry);
}

function mappedGlobal(from) {
    const entry = globalMap.get(from);
    if (!entry) {
        return null;
    }
    return [...entry.to.entries()].sort((p, q) => q[1] - p[1])[0][0];
}

function reusable(x, y, method) {
    if (method !== 'hash') {
        return 'no: the code differs';
    }
    if (x.calls.length !== y.calls.length) {
        return 'no: other calls';
    }
    for (let i = 0; i < x.calls.length; ++i) {
        if (aToB.get(x.calls[i]) !== y.calls[i]) {
            return 'no: callee ' + x.calls[i] + ' unmatched or matched elsewhere';
        }
    }
    for (let i = 0; i < x.globals.length; ++i) {
        const entry = globalMap.get(x.globals[i]);
        if (!entry || entry.to.size !== 1 || mappedGlobal(x.globals[i]) !== y.globals[i]) {
            return 'no: global ' + x.globals[i] + ' maps ambiguously';
        }
    }
    return 'yes';
}

fs.mkdirSync(outDir, { recursive: true });
const functionLines = ['a_address\ta_name\tb_address\tb_name\tmethod\tconfidence\treusable\ta_instructions\tb_instructions'];
for (const { a: x, b: y, method, confidence } of pairs.sort((p, q) => p.a.address.localeCompare(q.a.address))) {
    functionLines.push([x.address, x.name, y.address, y.name, method, confidence.toFixed(3), reusable(x, y, method),
        x.instructions, y.instructions].join('\t'));
}
for (const x of a.filter((record) => !aToB.has(record.address))) {
    functionLines.push([x.address, x.name, '', '', 'unmatched', '', 'no: unmatched', x.instructions, ''].join('\t'));
}
fs.writeFileSync(path.join(outDir, 'functions.tsv'), functionLines.join('\n') + '\n');

const globalLines = ['a_address\tb_address\tevidence\tconsistent'];
for (const [from, entry] of [...globalMap.entries()].sort()) {
    globalLines.push([from, mappedGlobal(from), entry.total, entry.to.size === 1 ? 'yes' : 'no: ' + [...entry.to.keys()].join(',')].join('\t'));
}
fs.writeFileSync(path.join(outDir, 'globals.tsv'), globalLines.join('\n') + '\n');

const offsetLines = ['a_offset\tb_offset\tevidence\tfunctions'];
for (const [key, entry] of [...offsetEvidence.entries()].sort((p, q) => q[1].count - p[1].count)) {
    const [from, to] = key.split('\t').map((value) => '0x' + Number(value).toString(16).toUpperCase());
    offsetLines.push([from, to, entry.count, [...entry.functions].slice(0, 8).join(',')].join('\t'));
}
fs.writeFileSync(path.join(outDir, 'offsets.tsv'), offsetLines.join('\n') + '\n');

const methods = counts(pairs.map((p) => p.method));
const aInstructions = a.reduce((sum, record) => sum + record.instructions, 0);
const matchedInstructions = pairs.reduce((sum, p) => sum + p.a.instructions, 0);
const reusableCount = pairs.filter((p) => reusable(p.a, p.b, p.method) === 'yes').length;
const summary = [
    `a functions ${a.length}, b functions ${b.length}`,
    `matched ${pairs.length} (${(100 * matchedInstructions / aInstructions).toFixed(1)}% of a's instructions), unmatched ${a.length - pairs.length}`,
    'by method: ' + [...methods.entries()].map(([method, count]) => `${method} ${count}`).join(', '),
    `reusable ${reusableCount}`,
    `globals mapped ${globalMap.size}, consistent ${[...globalMap.values()].filter((entry) => entry.to.size === 1).length}`,
    `offset pairs ${offsetEvidence.size}`,
];
fs.writeFileSync(path.join(outDir, 'summary.txt'), summary.join('\n') + '\n');
console.log(summary.join('\n'));
