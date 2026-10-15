'use strict';
// Writes reverse/analysis/casm/<addr>.s: label lines plus instruction text from asm/<addr>.asm
// (addresses, machine code and stack depth dropped; cross-reference targets kept only for calls).
const fs = require('fs');
const path = require('path');

const dir = path.resolve(__dirname, '..');
const outDir = path.join(dir, 'casm');
fs.mkdirSync(outDir, { recursive: true });
for (const f of fs.readdirSync(path.join(dir, 'asm'))) {
  const lines = fs.readFileSync(path.join(dir, 'asm', f), 'utf8').split('\n');
  const out = [];
  for (const line of lines) {
    if (!line) continue;
    if (line.startsWith('; function')) { out.push(line.replace(/ frsize=.*$/, '')); continue; }
    if (line.startsWith('; chunk')) { out.push(line); continue; }
    if (line.endsWith(':')) { out.push(line); continue; }
    const m = /^[0-9A-F]{8} [0-9a-f]+\s+sp=\S*\s+(.*)$/.exec(line);
    if (!m) { out.push(line); continue; }
    let text = m[1].replace(/\s+; -> [0-9A-F,]+$/, '').replace(/\s{2,}/g, ' ');
    out.push(' ' + text);
  }
  fs.writeFileSync(path.join(outDir, f.replace('.asm', '.s')), out.join('\n') + '\n');
}
console.log('done');
