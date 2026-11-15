"""scripts/mechanics/build.py <mechanics.md> <out.html>: the game mechanics page (docs/mechanics.html), silent.

mechanics.md: "## 標題 {#id}" starts a section (its link name), "### " a heading, "- " a list item, "| … |" table rows
(the first row the header, the "|---|" row skipped); other lines are paragraphs. `code` spans are kept;
[text](view) links another page of the site (altar: altar.html), opened beside the home page's menu when shown there.
"""
import html
import re
import sys


def inline(text):
    """A line as HTML: escaped, `code` spans kept, [text](view) a link to the view's page."""
    text = re.sub(r'`([^`]+)`', r'<code>\1</code>', html.escape(text))
    return re.sub(r'\[([^\]]+)\]\(([a-z]+)\)', r'<a class="view" href="\2.html" data-view="\2">\1</a>', text)


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
                wide = len(content) > WIDE_COLUMNS
                out.append('<table class="wide">' if wide else '<table>')
                open_kind = 'tr'
                # A wide table's heads take their number ("天神 0") on a line of its own.
                heads = [cell.replace(' ', '<br>', 1) if wide else cell for cell in content]
                out.append('<tr>' + ''.join(f'<th>{cell}</th>' for cell in heads) + '</tr>')
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
  --gold: #c9a24a;
  --gold-dark: #7a5a1c;
  --gold-light: #f2d27a;
  --text: #e9dcb8;
  --dim: #b9a67a;
  --panel: rgba(14, 10, 4, 0.94);
  --ink: #7a1f12;
  --kai: "BiauKai", "DFKai-SB", "標楷體", "KaiTi", "STKaiti", "Kaiti TC", serif;
  --ming: "PMingLiU", "MingLiU", "新細明體", "Songti TC", "Noto Serif TC", "Noto Serif CJK TC", serif;
}

* { box-sizing: border-box; }

html, body { margin: 0; height: 100%; overflow: hidden; }

body {
  font-family: var(--ming);
  color: var(--text);
  background:
    radial-gradient(ellipse at 20% 10%, rgba(255, 250, 230, 0.55), transparent 55%),
    radial-gradient(ellipse at 80% 85%, rgba(120, 90, 50, 0.35), transparent 60%),
    radial-gradient(ellipse at 60% 40%, rgba(90, 70, 40, 0.25), transparent 50%),
    repeating-linear-gradient(90deg, rgba(110, 60, 20, 0.08) 0 1px, transparent 1px 72px),
    linear-gradient(160deg, #e8dcc0 0%, #d6c39c 45%, #c4ab7d 100%);
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
  border-top: 2px solid rgba(122, 60, 20, 0.45);
  background: repeating-linear-gradient(90deg, rgba(122, 60, 20, 0.35) 0 1px, transparent 1px 54px);
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
  text-shadow: 2px 2px 0 rgba(255, 240, 210, 0.6), 0 0 6px rgba(122, 31, 18, 0.25);
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
  gap: 3px;
  padding-top: 10px;
  margin-right: -14px;
  z-index: 1;
}

.tab {
  font-family: var(--kai);
  font-size: 19px;
  padding: 5px 12px 5px 10px;
  color: #6d5a3c;
  background: linear-gradient(90deg, #b8a27a, #d9c9a3);
  border: 1px solid #8a6d3b;
  border-right: none;
  border-radius: 6px 0 0 6px;
  cursor: pointer;
  user-select: none;
  text-align: center;
  white-space: nowrap;
}

.tab.chosen { color: #1e1406; background: linear-gradient(90deg, #d6b45e, #f6e3a6); border-color: var(--gold-dark);
  font-weight: bold; }

.layout .page { z-index: 2; }
.page .body > section { display: none; }
.page .body > section.chosen { display: block; }

.frame {
  background: var(--panel);
  border: 3px double var(--gold);
  box-shadow: inset 0 0 0 1px var(--gold-dark), 0 0 0 1px #3a2a10, 2px 3px 8px rgba(40, 25, 5, 0.55);
}

.page { display: flex; flex-direction: column; min-height: 0; padding: 14px 8px 12px 24px; }

.page .title {
  flex: none;
  font-family: var(--kai);
  font-size: 32px;
  line-height: 44px;
  color: #fff2c8;
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
  scrollbar-color: var(--gold-dark) #120e06;
  font-size: 18px;
  line-height: 31px;
}

.page h2 {
  font-family: var(--kai);
  font-size: 30px;
  font-weight: normal;
  color: #fff2c8;
  margin: 22px 16px 4px 0;
  padding-bottom: 4px;
  border-bottom: 1px solid var(--gold-dark);
}

.page .body > section > h2 { margin-top: 4px; }

.page h3 { font-family: var(--kai); font-size: 23px; font-weight: normal; color: var(--gold-light); margin: 14px 0 2px; }
.page p { margin: 2px 0 6px; }
.page ul, .page ol { margin: 0 0 6px; padding-left: 26px; }
.page li > ul, .page li > ol { margin: 0 0 2px; padding-left: 24px; }
.page ol li::marker { font-size: 16px; }
.page li::marker { color: var(--gold); }
.page a.view { color: var(--gold-light); }

.page table { width: calc(100% - 16px); border-collapse: collapse; margin: 6px 0 12px; font-size: 17px; line-height: 27px; }
.page th, .page td { text-align: left; vertical-align: top; padding: 6px 10px; border-bottom: 1px solid rgba(201, 162, 74, 0.35); }
.page th { font-family: var(--kai); font-size: 19px; font-weight: normal; color: var(--gold-light); background: rgba(201, 162, 74, 0.18); white-space: nowrap; }
.page td:first-child { white-space: nowrap; color: #fff2c8; }
.page td:first-child:nth-last-child(2) { width: 8em; }  /* two columns: the same first column, the tables line up */
.page table.wide { font-size: 15px; line-height: 22px; }
.page table.wide th, .page table.wide td { padding: 4px 3px; text-align: center; white-space: nowrap; }
.page table.wide th { font-size: 15px; }
.page table.wide td:first-child, .page table.wide th:first-child { text-align: left; padding-left: 6px; }

.hint { flex: none; margin-top: 6px; font-size: 13px; color: #5a4524; text-align: right; }
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

// A link to another page: the home page shows it beside its menu (going back returns here); alone it opens itself.
document.addEventListener('click', event => {
  const link = event.target.closest('a.view');

  if (!link) {
    return;
  }

  event.preventDefault();

  if (window.parent !== window) {
    window.parent.postMessage({ open: link.dataset.view, detail: '' }, '*');
    return;
  }

  location.href = `${link.dataset.view}.html${location.search}`;
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
