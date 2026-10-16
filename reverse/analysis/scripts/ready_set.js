'use strict';
// Lists pending non-CRT functions whose game callees are all implemented (from impl_order.tsv callees and the
// current status in reverse/functions.tsv), grouped by module, in impl_order rank.
const fs = require('fs');
const path = require('path');

const root = path.resolve(__dirname, '..', '..', '..');
const readTsv = (file) => {
  const [header, ...rows] = fs.readFileSync(file, 'utf8').trim().split('\n');
  const cols = header.split('\t');
  return rows.map((r) => Object.fromEntries(r.split('\t').map((v, i) => [cols[i], v])));
};

const functions = readTsv(path.join(root, 'reverse', 'functions.tsv'));
const status = new Map(functions.map((f) => [f.address, f.status]));
const order = readTsv(path.join(root, 'reverse', 'analysis', 'impl_order.tsv'));
const done = (s) => s && s !== 'pending_implementation';

const ready = [];
for (const row of order) {
  if (done(status.get(row.address))) continue;
  const callees = (row.game_callees || '').split(',').filter(Boolean);
  if (callees.every((c) => done(status.get(c)))) ready.push(row);
}
const limit = Number(process.argv[2] || 60);
console.log(`ready: ${ready.length}`);
for (const r of ready.slice(0, limit)) console.log(`${r.rank}\t${r.address}\t${r.module}\t${r.size}`);
