'use strict';
// Classifies the stack jump tables of the blend blitters (0x0041B9F0, 0x0041D340, 0x0041E5C0, ...).
// For every table built with `lea edi, [ebp+var_X]` followed by `lea esi/eax, loc_Y` pairs, prints each
// entry index with a signature of the code at loc_Y: the operand of each of the four shifted terms
// (S = source, D = destination), the store width, or COPY / SKIP.
const fs = require('fs');
const path = require('path');

const addr = process.argv[2];
const lines = fs
  .readFileSync(path.resolve(__dirname, '..', 'casm', `${addr}.s`), 'utf8')
  .split('\n')
  .map((l) => l.trim())
  .filter((l) => l && !l.startsWith(';') && l !== 'nop');

const labelIndex = new Map();
lines.forEach((l, i) => {
  const m = /^(loc_[0-9A-F]+):$/.exec(l);
  if (m) labelIndex.set(m[1], i);
});

const tables = [];
let current = null;
for (const l of lines) {
  let m = /^lea edi, \[ebp\+(var_[0-9A-F]+)\]$/.exec(l);
  if (m) {
    current = { name: m[1], entries: [] };
    tables.push(current);
    continue;
  }
  if (!current) continue;
  m = /^lea (esi|eax), (loc_[0-9A-F]+)$/.exec(l);
  if (m) {
    current.entries.push(m[2]);
    continue;
  }
  if (/^mov \[edi(\+4)?\], (esi|eax)$/.test(l) || l === 'add edi, 8') continue;
  current = null;
}

function operand(text) {
  if (/\[esi\]/.test(text)) return 'S';
  if (/\[edi(-2)?\]/.test(text)) return 'D';
  return '?' + text;
}

function signature(label) {
  let i = labelIndex.get(label) + 1;
  const terms = [];
  let store = '';
  let copy = false;
  for (; i < lines.length; i++) {
    const l = lines[i];
    if (/^loc_/.test(l) || /^jmp /.test(l)) break;
    let m = /^mov (eax|ebx), (.*)$/.exec(l);
    if (m && /\[e[sd]i/.test(m[2])) terms.push(operand(m[2]));
    m = /^mov (\[edi(-2)?\]|word ptr \[edi\]), (ax|eax)$/.exec(l);
    if (m) store = `${m[1]}<-${m[3]}`;
  }
  if (terms.length === 1 && terms[0] === 'S') copy = true;
  if (terms.length === 0) return 'SKIP';
  if (copy) return `COPY ${store}`;
  return `${terms.join('')} ${store}`;
}

for (const t of tables) {
  console.log(`table ${t.name} (${t.entries.length} entries)`);
  t.entries.forEach((e, idx) => console.log(`  [${idx}] ${e} ${signature(e)}`));
}
