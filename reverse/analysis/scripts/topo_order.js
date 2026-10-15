'use strict';
// Bottom-up implementation order: game functions sorted so that callees come before callers
// (strongly connected components kept together). CRT functions are excluded; they map to host CRT calls.
const fs = require('fs');
const path = require('path');

const dir = path.resolve(__dirname, '..');
const read = (f) => fs.readFileSync(path.join(dir, f), 'utf8').trim().split('\n').slice(1).map((l) => l.split('\t'));
const funcs = read('ida/functions_raw.tsv');
const table = new Map(fs.readFileSync(path.resolve(dir, '..', 'functions.tsv'), 'utf8').trim().split('\n').slice(1)
  .map((l) => l.split('\t')).map((r) => [r[2], r]));
const isCrt = (a) => { const x = parseInt(a, 16); return x >= 0x487af0 && x < 0x497fcc; };
const nodes = funcs.filter((f) => !isCrt(f[0])).map((f) => f[0]);
const nodeSet = new Set(nodes);
const edges = new Map(funcs.map((f) => [f[0], f[8].split(',').filter((c) => nodeSet.has(c) && c !== f[0])]));

let index = 0;
const idx = new Map(); const low = new Map(); const onStack = new Set(); const stack = []; const sccs = [];
function strong(v) {
  idx.set(v, index); low.set(v, index); index++; stack.push(v); onStack.add(v);
  for (const w of edges.get(v) || []) {
    if (!idx.has(w)) { strong(w); low.set(v, Math.min(low.get(v), low.get(w))); }
    else if (onStack.has(w)) low.set(v, Math.min(low.get(v), idx.get(w)));
  }
  if (low.get(v) === idx.get(v)) {
    const comp = []; let w;
    do { w = stack.pop(); onStack.delete(w); comp.push(w); } while (w !== v);
    sccs.push(comp.sort());
  }
}
for (const v of nodes) if (!idx.has(v)) strong(v);

const sizes = new Map(funcs.map((f) => [f[0], +f[4]]));
let out = 'rank\tscc\taddress\tname\tmodule\tsize\tstatus\tgame_callees\n';
let rank = 0;
sccs.forEach((comp, s) => {
  for (const a of comp) {
    const t = table.get(a) || [];
    out += `${++rank}\t${s}\t${a}\t${t[3] || ''}\t${t[4] || ''}\t${sizes.get(a)}\t${t[6] || ''}\t${(edges.get(a) || []).join(',')}\n`;
  }
});
fs.writeFileSync(path.join(dir, 'impl_order.tsv'), out);
const multi = sccs.filter((c) => c.length > 1);
console.log(`functions=${nodes.length} sccs=${sccs.length} recursive_groups=${multi.length} largest=${Math.max(...sccs.map((c) => c.length))}`);
multi.forEach((c) => console.log('group', c.join(',')));
