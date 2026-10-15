'use strict';
// Builds reverse/analysis/data_auto.tsv: one field per IDA data head in the image plus one byte field for
// every code-referenced image address that is not covered by a head. Feeds gen_globals.js.
const fs = require('fs');
const path = require('path');

const dir = path.resolve(__dirname, '..');
const IMAGE_BASE = 0x00499000;
const IMAGE_END = 0x0053f000;
const KIND_TYPES = { byte: 'std::uint8_t', word: 'std::int16_t', dword: 'std::int32_t', qword: 'std::int64_t', float: 'float', double: 'double' };

const items = fs.readFileSync(path.join(dir, 'ida', 'data_items.tsv'), 'utf8').trim().split('\n').slice(1).map((l) => l.split('\t'));
const heads = items.map((r) => ({ addr: parseInt(r[0], 16), size: +r[1], kind: r[2], name: r[3], elem: +r[4] }))
  .filter((h) => h.addr >= IMAGE_BASE && h.addr < IMAGE_END);
heads.sort((a, b) => a.addr - b.addr);

const refs = new Set();
for (const f of fs.readdirSync(path.join(dir, 'asm'))) {
  const text = fs.readFileSync(path.join(dir, 'asm', f), 'utf8');
  for (const m of text.matchAll(/; -> ([0-9A-F,]+)/g)) {
    for (const t of m[1].split(',')) {
      const v = parseInt(t, 16);
      if (v >= IMAGE_BASE && v < IMAGE_END) refs.add(v);
    }
  }
}

function covering(addr) {
  let lo = 0; let hi = heads.length - 1; let best = -1;
  while (lo <= hi) {
    const mid = (lo + hi) >> 1;
    if (heads[mid].addr <= addr) { best = mid; lo = mid + 1; } else hi = mid - 1;
  }
  return best >= 0 && addr < heads[best].addr + Math.max(1, heads[best].size) ? heads[best] : null;
}

const fields = [];
const sanitize = (n, a) => (/^[A-Za-z_][A-Za-z0-9_]*$/.test(n) ? n : `g_${a.toString(16).toUpperCase()}`);
for (const h of heads) {
  if (h.kind === 'align') continue;
  let type;
  if (KIND_TYPES[h.kind] && h.elem > 0) {
    const count = Math.floor(h.size / h.elem);
    type = count > 1 ? `${KIND_TYPES[h.kind]}[${count}]` : KIND_TYPES[h.kind];
  } else if (h.kind === 'str') {
    type = `char[${h.size}]`;
  } else {
    type = `std::uint8_t[${Math.max(1, h.size)}]`;
  }
  fields.push({ addr: h.addr, type, name: sanitize(h.name || `g_${h.addr.toString(16)}`, h.addr) });
}
for (const r of refs) {
  if (covering(r)) continue;
  fields.push({ addr: r, type: 'std::uint8_t', name: `unk_${r.toString(16).toUpperCase()}` });
}
fields.sort((a, b) => a.addr - b.addr);
let out = 'addr\ttype\tname\n';
for (const f of fields) out += `0x${f.addr.toString(16).toUpperCase().padStart(8, '0')}\t${f.type}\t${f.name}\n`;
fs.writeFileSync(path.join(dir, 'data_auto.tsv'), out);
console.log(`auto fields: ${fields.length} (heads ${heads.length}, uncovered refs ${fields.length - heads.filter((h) => h.kind !== 'align').length})`);
