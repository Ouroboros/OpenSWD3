"""scripts/mechanics/build.py <mechanics.md> <out.html>: the game mechanics page (docs/mechanics.html), silent.

mechanics.md: "## 標題 {#id}" starts a section (its link name), "### " a heading, "- " a list item, "| … |" table rows
(the first row the header, the "|---|" row skipped); other lines are paragraphs. `code` spans are kept.
"""
import html
import re
import sys


def inline(text):
    """A line as HTML: escaped, `code` spans kept."""
    return re.sub(r'`([^`]+)`', r'<code>\1</code>', html.escape(text))


def cells(line):
    return [inline(cell.strip()) for cell in line.strip().strip('|').split('|')]


def main():
    source, out_path = sys.argv[1:]
    sections = []
    for line in open(source, encoding='utf-8'):
        line = line.rstrip('\n')
        if line.startswith('## '):
            match = re.fullmatch(r'## (.+?) \{#([a-z]+)\}', line)
            if not match:
                sys.exit(f'a section without its link name: {line}')
            sections.append(dict(title=match.group(1), id=match.group(2), blocks=[]))
        elif not sections or not line.strip():
            continue
        elif line.startswith('### '):
            sections[-1]['blocks'].append(['h', inline(line[4:].strip())])
        elif line.startswith('- '):
            sections[-1]['blocks'].append(['li', inline(line[2:].strip())])
        elif line.startswith('|'):
            if re.fullmatch(r'\|(\s*-+\s*\|)+', line.strip()):
                continue
            sections[-1]['blocks'].append(['tr', cells(line)])
        else:
            sections[-1]['blocks'].append(['p', inline(line.strip())])

    body = ''.join(
        f'<section id="{section["id"]}"><h2>{html.escape(section["title"])}</h2>{blocks(section["blocks"])}</section>'
        for section in sections
    )
    tabs = ''.join(
        f'<div class="tab" data-id="{section["id"]}">{html.escape(section["title"])}</div>' for section in sections
    )
    page = TEMPLATE.replace('<!--BODY-->', body).replace('<!--TABS-->', tabs)
    with open(out_path, 'w', encoding='utf-8') as out:
        out.write(page)
    print(f'{len(sections)} sections, {sum(len(s["blocks"]) for s in sections)} lines, {len(page)} bytes')


WIDE_COLUMNS = 8  # a table of more columns is drawn smaller and tighter (the altar's category table)


def blocks(rows):
    """The lines as HTML: the items of a run in one list, the rows of a run in one table (the first its header)."""
    out = []
    open_kind = None
    for kind, content in rows:
        if open_kind and kind != open_kind:
            out.append('</ul>' if open_kind == 'li' else '</table>')
            open_kind = None
        if kind == 'li':
            if not open_kind:
                out.append('<ul>')
                open_kind = 'li'
            out.append(f'<li>{content}</li>')
        elif kind == 'tr':
            if not open_kind:
                out.append('<table class="wide">' if len(content) > WIDE_COLUMNS else '<table>')
                open_kind = 'tr'
                out.append('<tr>' + ''.join(f'<th>{cell}</th>' for cell in content) + '</tr>')
            else:
                out.append('<tr>' + ''.join(f'<td>{cell}</td>' for cell in content) + '</tr>')
        else:
            out.append(f'<h3>{content}</h3>' if kind == 'h' else f'<p>{content}</p>')
    if open_kind:
        out.append('</ul>' if open_kind == 'li' else '</table>')
    return ''.join(out)


TEMPLATE = r'''<!DOCTYPE html>
<html lang="zh-Hant">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>遊戲機制</title>
<style>
:root {
  --gold: #a89048;
  --gold-dark: #6a5420;
  --gold-light: #f0e848;
  --text: #c8b888;
  --dim: #988a62;
  --panel: #100f08;
  --panel-list: #181010;
  --ink: #3a2008;
  --jade-light: #a8d890;
  --kai: "BiauKai", "DFKai-SB", "標楷體", "KaiTi", "STKaiti", "Kaiti TC", serif;
  --ming: "PMingLiU", "MingLiU", "新細明體", "Songti TC", "Noto Serif TC", "Noto Serif CJK TC", serif;
}

* { box-sizing: border-box; }

html, body { margin: 0; height: 100%; overflow: hidden; }

body {
  font-family: var(--ming);
  color: var(--text);
  background:
    radial-gradient(ellipse at 15% 20%, rgba(255, 220, 150, 0.45), transparent 50%),
    radial-gradient(ellipse at 85% 80%, rgba(110, 50, 10, 0.4), transparent 55%),
    radial-gradient(ellipse at 50% 55%, rgba(150, 80, 20, 0.25), transparent 45%),
    radial-gradient(ellipse at 5% 90%, rgba(90, 40, 10, 0.35), transparent 40%),
    repeating-linear-gradient(90deg, rgba(90, 40, 10, 0.12) 0 1px, transparent 1px 80px),
    linear-gradient(160deg, #e0b070 0%, #cc9450 45%, #b07834 100%);
}

.book {
  position: absolute;
  left: 0;
  top: 0;
  width: 1180px;
  height: 870px;
  padding: 12px 12px 8px;
  display: flex;
  flex-direction: column;
  transform-origin: 0 0;
}

.top {
  display: flex;
  align-items: flex-end;
  justify-content: space-between;
  border-top: 2px solid rgba(255, 230, 180, 0.5);
  background: repeating-linear-gradient(90deg, rgba(255, 230, 180, 0.3) 0 1px, transparent 1px 80px);
  height: 64px;
  flex: none;
  padding: 0 12px 4px 6px;
}

h1 {
  margin: 0;
  font-family: var(--kai);
  font-size: 44px;
  letter-spacing: 6px;
  color: var(--ink);
  text-shadow: 2px 2px 0 rgba(255, 230, 180, 0.45), 0 0 8px rgba(255, 230, 180, 0.35);
  font-weight: bold;
}

.tools { display: flex; align-items: flex-end; gap: 10px; }

.layout {
  display: grid;
  grid-template-columns: 116px 1fr;
  gap: 0 14px;
  margin-top: 8px;
  flex: 1;
  min-height: 0;
}

/* The sections as the bestiary's tabs, one shown at a time. */
.tabs {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding-top: 10px;
  margin-right: -20px;
  z-index: 1;
}

.tab {
  font-family: var(--kai);
  font-size: 19px;
  padding: 6px 19px 6px 12px;
  color: #2c3410;
  background: linear-gradient(90deg, #5c7034, #889c58);
  --edge: #b89838;
  border: 1px solid #000;
  border-right: none;
  border-radius: 10px 0 0 10px;
  box-shadow: inset 0 0 0 2px var(--edge);
  cursor: pointer;
  user-select: none;
  text-align: center;
  white-space: nowrap;
}

.tab.chosen {
  color: #181800;
  background: linear-gradient(90deg, #b8d050, #eef488);
  --edge: #b07818;
  font-weight: bold;
}

.layout .page { z-index: 2; }
.page .body > section { display: none; }
.page .body > section.chosen { display: block; }

/* The bright corner marks of the game's frames, inside the frame line. */
.frame {
  --corner: linear-gradient(var(--gold-light), var(--gold-light));
  --fill: var(--panel);
  background:
    var(--corner) left top / 9px 3px no-repeat,
    var(--corner) left top / 3px 9px no-repeat,
    var(--corner) right top / 9px 3px no-repeat,
    var(--corner) right top / 3px 9px no-repeat,
    var(--corner) left bottom / 9px 3px no-repeat,
    var(--corner) left bottom / 3px 9px no-repeat,
    var(--corner) right bottom / 9px 3px no-repeat,
    var(--corner) right bottom / 3px 9px no-repeat,
    var(--fill);
  background-origin: padding-box;
  border: 2px solid var(--gold);
  box-shadow: inset 0 0 0 1px #000, 0 0 0 1px #000;
  position: relative;
}






.page { display: flex; flex-direction: column; min-height: 0; padding: 14px 8px 12px 24px; }

.page .title {
  flex: none;
  font-family: var(--kai);
  font-size: 32px;
  line-height: 44px;
  color: #f0e0a8;
  border-bottom: 1px solid var(--gold-dark);
  margin-right: 16px;
  padding-bottom: 4px;
}

.page .title small { font-size: 18px; color: var(--dim); margin-left: 14px; }

.page .body {
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  scrollbar-gutter: stable;
  padding: 4px 14px 10px 0;
  scrollbar-color: var(--gold) #000;
  font-size: 18px;
  line-height: 31px;
}

.page h2 {
  font-family: var(--kai);
  font-size: 30px;
  font-weight: normal;
  color: #f0e0a8;
  margin: 22px 16px 4px 0;
  padding-bottom: 4px;
  border-bottom: 1px solid var(--gold-dark);
}

.page .body > section > h2 { margin-top: 4px; }

.page h2 .date { color: inherit; text-decoration: none; cursor: pointer; }
.page h2 .date:hover { color: var(--gold-light); }
.page h2 .date:hover::after { content: " #"; color: var(--gold); }

.page h3 { font-family: var(--kai); font-size: 23px; font-weight: normal; color: var(--gold-light); margin: 14px 0 2px; }
.page p { margin: 2px 0 6px; }
.page ul, .page ol { margin: 0 0 6px; padding-left: 26px; }
.page li > ul, .page li > ol { margin: 0 0 2px; padding-left: 24px; }
.page ol li::marker { font-size: 16px; }
.page li::marker { color: var(--gold); }





.page table { width: calc(100% - 16px); border-collapse: collapse; margin: 6px 0 12px; font-size: 17px; line-height: 27px; }
.page th, .page td { text-align: left; vertical-align: top; padding: 6px 10px; border-bottom: 1px solid rgba(168, 144, 72, 0.35); }
.page th { font-family: var(--kai); font-size: 19px; font-weight: normal; color: var(--gold-light); background: rgba(168, 144, 72, 0.18); white-space: nowrap; }
.page td:first-child { white-space: nowrap; color: #f0e0a8; }
.page td:first-child:nth-last-child(2) { width: 8em; }  /* two columns: the same first column, the tables line up */
.page table.wide { font-size: 15px; line-height: 22px; }
.page table.wide th, .page table.wide td { padding: 4px 3px; text-align: center; white-space: nowrap; }
.page table.wide th { font-size: 15px; }
.page table.wide td:first-child, .page table.wide th:first-child { text-align: left; padding-left: 6px; }

.hint { flex: none; margin-top: 6px; font-size: 13px; color: #3a2008; text-align: right; }
</style>
</head>
<body>
<div class="book">
  <div class="top">
    <div class="tools"></div>
    <h1>遊戲機制</h1>
  </div>
  <div class="layout">
    <div class="tabs" id="tabs"><!--TABS--></div>
    <div class="frame page"><div class="body" id="body"><!--BODY--></div></div>
  </div>
  <div class="hint">←→ 分頁　↑↓ PgUp / PgDn 捲動　Home / End 首尾</div>
</div>
<script>
const $ = id => document.getElementById(id);

document.addEventListener('keydown', event => {
  const body = $('body');
  const page = body.clientHeight - 40;
  const keys = {
    ArrowUp: () => { body.scrollTop -= 40; },
    ArrowDown: () => { body.scrollTop += 40; },
    PageUp: () => { body.scrollTop -= page; },
    PageDown: () => { body.scrollTop += page; },
    Home: () => { body.scrollTop = 0; },
    End: () => { body.scrollTop = body.scrollHeight; },
    ArrowLeft: () => turn(-1),
    ArrowRight: () => turn(1),
  };

  if (keys[event.key]) {
    keys[event.key]();
    event.preventDefault();
  }
});

// The book keeps its size: scaled whole to fit the window, centred.
function fit() {
  const book = document.querySelector('.book');
  const scale = Math.min(innerWidth / book.offsetWidth, innerHeight / book.offsetHeight);
  const left = (innerWidth - book.offsetWidth * scale) / 2;
  const top = (innerHeight - book.offsetHeight * scale) / 2;
  book.style.transform = `translate(${left}px, ${top}px) scale(${scale})`;
}

function addressed() {
  try {
    return decodeURIComponent(location.hash.slice(1));
  } catch {
    return '';
  }
}

const tabs = [...document.querySelectorAll('.tab')];
let shownTab = 0;

// The tab of a section shown alone, from its top.
function show(at) {
  shownTab = at;
  tabs.forEach((tab, index) => tab.classList.toggle('chosen', index === at));
  document.querySelectorAll('.body > section').forEach((section, index) => {
    section.classList.toggle('chosen', index === at);
  });
  $('body').scrollTop = 0;
}

// A section named in the address (mechanics.html#refine) is shown; the first one otherwise.
function showNamed(text = addressed()) {
  const at = tabs.findIndex(tab => tab.dataset.id === text);
  show(at < 0 ? 0 : at);
}

// The tab chosen kept in the address (and in the home page's, which opens it there), so that its link can be copied.
function keepPlace(detail) {
  history.replaceState(null, '', `${location.search}#${encodeURIComponent(detail)}`);

  if (window.parent !== window) {
    window.parent.postMessage({ shown: 'mechanics', detail }, '*');
  }
}

function choose(at) {
  if (at === shownTab) {
    return;
  }

  show(at);
  keepPlace(tabs[at].dataset.id);
}

// ←→ step through the tabs, wrapping round as the bestiary's do.
function turn(step) {
  choose((shownTab + step + tabs.length) % tabs.length);
}

$('tabs').addEventListener('click', event => {
  const tab = event.target.closest('.tab');

  if (tab) {
    choose(tabs.indexOf(tab));
  }
});

addEventListener('resize', fit);
addEventListener('hashchange', () => showNamed());
fit();
showNamed();
</script>
</body>
</html>
'''

if __name__ == '__main__':
    main()
