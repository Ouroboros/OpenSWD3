'use strict';
// Prints casm/<addr>.s with labels renamed L0, L1, ... in order of first appearance, so that two routines
// generated from the same macro can be compared with diff.
const fs = require('fs');
const path = require('path');

const file = path.resolve(__dirname, '..', 'casm', `${process.argv[2]}.s`);
const text = fs.readFileSync(file, 'utf8');
const names = new Map();
const rename = (name) => {
  if (!names.has(name)) names.set(name, `L${names.size}`);
  return names.get(name);
};
const out = [];
for (const line of text.split('\n')) {
  if (line.startsWith(';')) continue;
  out.push(line.replace(/\b(loc|locret|sub)_[0-9A-F]{6}\b/g, (m) => rename(m)).replace(/\$\+2/g, 'NEXT'));
}
process.stdout.write(out.join('\n'));
