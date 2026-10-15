'use strict';
const fs = require('fs');
const path = require('path');

const analysisDir = path.resolve(__dirname, '..');
const idaDir = path.join(analysisDir, 'ida');
const outTsv = path.resolve(analysisDir, '..', 'functions.tsv');
const outModules = path.join(analysisDir, 'module_assignment.tsv');

const readTsv = (file) => fs.readFileSync(file, 'utf8').trim().split('\n').slice(1).map((l) => l.split('\t'));
const funcs = readTsv(path.join(idaDir, 'functions_raw.tsv'));
const refs = new Map(readTsv(path.join(idaDir, 'func_refs.tsv')).map((r) => [r[0], r]));
const hex = (s) => parseInt(s, 16);

const CRT_START = 0x00487af0;
const CRT_END = 0x00497fcc;
const EH_START = 0x00497fd0;
const EH_END = 0x00498422;
const STATIC_INIT_RANGES = [[0x00401000, 0x004014f0], [0x004272c0, 0x004272f0], [0x004517a0, 0x00451940]];

const ACT_ERR_MODULE = {
  Path: 'role_path', KeyCole: 'role_event', Misc: 'misc', 'Misc:': 'misc', Dialog: 'dialog', picPaint: 'pic_paint',
  PicPaint: 'pic_paint', Talk: 'talk', DlgMessage: 'dlg_message', Menu: 'menu', 'Menu:': 'menu', Botl: 'menu',
  RefrashCursor: 'pic_paint',
};

const RULES = [
  [/cDINPUT|Direct Input Failure/, 'input_dinput'],
  [/DirectDraw .*Error|DDERR_/, 'ddraw'],
  [/cFile ,CloseView/, 'file'],
  [/MemMgr/, 'memmgr'],
  [/all_char\.(tsw|act)|<<TSW|<<ACT|Mem error::tsw|Act error::actno|^ACT$|^TSW$/, 'res_pack'],
  [/particle_put/, 'particle'],
  [/Env\.dat/, 'config_env'],
  [/save\\%d\.sav|Fame\.dat/, 'saveload'],
  [/SoftstarSwd3|ScrnShot/, 'winmain'],
  [/CheatDialog/, 'cheat_dialog'],
  [/CD checker|GetDiskFreeSpaceExA/, 'cdcheck'],
  [/DrawKernel init|DDraw init Failed|ReleaseGame|RoleDataBase init|Born\.avi/, 'game_init'],
  [/www\.softstar|\\shell\\open\\command|^a:\\$/, 'launcher'],
  [/HUGE\.lmf|mcache\.dat|DecompressMap|GetCacheMapUnit|LoadCity|data\\%d\.cm/, 'map'],
  [/Talk%d\.dat|SWD3-Talk/, 'talk'],
  [/battle\.ffd|Battle_(Europa|Arab|China)|c:\\snap/, 'battle'],
  [/figtalk\.dat/, 'figtalk'],
  [/manrole\.cpp/, 'manrole'],
  [/mon\.dat|level\.dat|org\.cpp|fame (de)?compress/, 'org'],
  [/aStar|CompanyWalk|Pth:rndwlk/, 'role_path'],
  [/Music\\|Music String/, 'music'],
  [/all\.snd|SFX file|RIFF/, 'audio_mss'],
  [/video\\|BinkCopyToBuffer/, 'movie_bink'],
];

const API_RULES = [
  [/^mss32!/, 'audio_mss'],
  [/^binkw32!/, 'movie_bink'],
  [/DirectInputCreateA/, 'input_dinput'],
  [/DirectDrawCreate/, 'ddraw'],
  [/MapViewOfFile|CreateFileMappingA|UnmapViewOfFile|SetEndOfFile|GetFileTime/, 'file'],
  [/HeapCreate|HeapDestroy/, 'memmgr'],
  [/ImmIsIME|CharNextA|CharPrevA/, 'text_dbcs'],
  [/CreateFontIndirectA|TextOutA|SetBkMode/, 'font_gdi'],
  [/DialogBoxParamA|RegOpenKeyExA/, null],
];

function directEvidence(ea, name, r) {
  const a = hex(ea);
  if (a >= CRT_START && a < CRT_END) return ['crt', 'statically linked MSVC debug CRT range 0x487AF0-0x497FCC (CRT source-file assert strings)'];
  if (STATIC_INIT_RANGES.some(([s, e]) => a >= s && a < e)) return ['static_init', 'MSVC global ctor/atexit/dtor thunk pattern'];
  if (name === '_WinMain@16') return ['winmain', 'WinMain'];
  const strings = r ? r[3] : '';
  const apis = r ? r[2].split(',').filter((x) => x && x !== 'mss32!_AIL_serve@0') : [];
  const act = [...strings.matchAll(/Act Err\(([A-Za-z]+)/g)].map((m) => m[1]);
  for (const tag of act) {
    if (ACT_ERR_MODULE[tag]) return [ACT_ERR_MODULE[tag], `string "Act Err(${tag}:...)"`];
  }
  for (const s of strings.split(' | ')) {
    for (const [re, mod] of RULES) {
      if (re.test(s)) return [mod, `string "${s.slice(0, 40)}"`];
    }
  }
  for (const api of apis) {
    for (const [re, mod] of API_RULES) {
      if (mod && re.test(api)) return [mod, `import ${api}`];
    }
  }
  return null;
}

const rows = funcs.map((f) => ({ ea: f[0], name: f[2], flags: f[3], size: +f[4], module: 'unknown', basis: '' }));
for (const row of rows) {
  const ev = directEvidence(row.ea, row.name, refs.get(row.ea));
  if (ev) [row.module, row.basis] = ev;
}

const anchored = rows.map((r) => r.module !== 'unknown');
for (let i = 0; i < rows.length; i++) {
  if (anchored[i]) continue;
  let lo = i - 1;
  while (lo >= 0 && !anchored[lo]) lo--;
  let hi = i + 1;
  while (hi < rows.length && !anchored[hi]) hi++;
  if (lo >= 0 && hi < rows.length && rows[lo].module === rows[hi].module && rows[lo].module !== 'static_init') {
    const a = hex(rows[i].ea);
    if (a >= EH_START && a < EH_END) continue;
    rows[i].module = rows[lo].module;
    rows[i].basis = `inferred: between ${rows[lo].module} anchors ${rows[lo].ea} and ${rows[hi].ea}`;
  }
}

const byEa = new Map(rows.map((r) => [r.ea, r]));
const ehRows = rows.filter((r) => hex(r.ea) >= EH_START && hex(r.ea) < EH_END);
let pending = [];
for (const r of ehRows) {
  pending.push(r);
  const m = /^SEH_([0-9A-F]+)$/.exec(r.name);
  if (m) {
    const parentEa = '0x' + m[1].padStart(8, '0');
    const parent = byEa.get(parentEa);
    for (const p of pending) {
      p.module = parent ? parent.module : 'unknown';
      p.basis = `EH funclet of ${parentEa} (${p === r ? 'handler' : 'unwind action'})`;
    }
    pending = [];
  }
}
for (const p of pending) {
  p.module = 'unknown';
  p.basis = 'EH unwind funclet, parent not yet identified';
}

const header = 'order\tbinary\taddress\tname\tmodule\tsource\tstatus\tverification_count\tevidence\tnotes\n';
let body = '';
rows.forEach((r, idx) => {
  const notes = [r.basis, r.flags ? `ida_flags=${r.flags}` : ''].filter(Boolean).join('; ');
  body += `${idx + 1}\tswd32.exe\t${r.ea}\t${r.name}\t${r.module}\t\tpending_implementation\t0\treverse/analysis/ida/functions_raw.tsv\t${notes}\n`;
});
fs.writeFileSync(outTsv, header + body);

const counts = {};
rows.forEach((r) => { counts[r.module] = (counts[r.module] || 0) + 1; });
const direct = rows.filter((r) => r.basis && !r.basis.startsWith('inferred')).length;
let summary = 'module\tfunctions\n';
Object.entries(counts).sort((x, y) => y[1] - x[1]).forEach(([m, n]) => { summary += `${m}\t${n}\n`; });
fs.writeFileSync(outModules, summary);
console.log(`functions=${rows.length} direct_or_pattern=${direct} inferred=${rows.filter((r) => r.basis.startsWith('inferred')).length} unknown=${counts.unknown || 0}`);
console.log(summary);
