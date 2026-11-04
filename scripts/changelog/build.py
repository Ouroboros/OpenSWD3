"""scripts/changelog/build.py <CHANGELOG.md> <out.html> [move.wav]: the changelog page (docs/changelog.html), every
day on one page, the newest first.

CHANGELOG.md: "## 日期" starts a day, "### 分類" a heading, "- " a change; other lines are paragraphs. The days keep
their order. move.wav is the game's cursor sound (all.snd sound 46), played by the sound switch; the page has none
without it.
"""
import base64
import html
import json
import sys


def sound_url(path):
    """The cursor sound as a data address; none without one."""
    if not path:
        return None
    with open(path, 'rb') as sound:
        return 'data:audio/wav;base64,' + base64.b64encode(sound.read()).decode('ascii')


def main():
    source, out_path, *rest = sys.argv[1:]
    days = []
    for line in open(source, encoding='utf-8'):
        line = line.rstrip('\n')
        if line.startswith('## '):
            days.append(dict(date=line[3:].strip(), blocks=[]))
        elif not days or not line.strip():
            continue
        elif line.startswith('### '):
            days[-1]['blocks'].append(['h', html.escape(line[4:].strip())])
        elif line.startswith('- '):
            days[-1]['blocks'].append(['li', html.escape(line[2:].strip())])
        else:
            days[-1]['blocks'].append(['p', html.escape(line.strip())])

    body = ''.join(
        f'<section id="{html.escape(day["date"])}"><h2>{html.escape(day["date"])}</h2>{blocks(day["blocks"])}</section>'
        for day in days
    )
    page = TEMPLATE.replace('<!--BODY-->', body)
    page = page.replace('/*SOUND*/null', json.dumps(sound_url(rest[0] if rest else '')))
    with open(out_path, 'w', encoding='utf-8', newline='\n') as out:
        out.write(page)
    print(f'{len(days)} days, {sum(len(day["blocks"]) for day in days)} lines, {len(page)} bytes')


def blocks(rows):
    """The lines as HTML: the changes of a heading in one list."""
    out = []
    listing = False
    for kind, text in rows:
        if listing and kind != 'li':
            out.append('</ul>')
            listing = False
        if kind == 'li':
            if not listing:
                out.append('<ul>')
                listing = True
            out.append(f'<li>{text}</li>')
        else:
            out.append(f'<h3>{text}</h3>' if kind == 'h' else f'<p>{text}</p>')
    if listing:
        out.append('</ul>')
    return ''.join(out)


TEMPLATE = r'''<!DOCTYPE html>
<html lang="zh-Hant">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>更新記錄</title>
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

.sound {
  margin: 0 auto 6px 6px;
  height: 34px;
  padding: 0 12px;
  font-family: var(--kai);
  font-size: 18px;
  color: #3b2a12;
  background: linear-gradient(180deg, #e8d9b0, #c9b285);
  border: 1px solid rgba(122, 60, 20, 0.6);
  border-radius: 3px;
  cursor: pointer;
}

.sound.off { color: #8a7a5c; background: rgba(255, 248, 228, 0.5); }

.frame {
  background: var(--panel);
  border: 3px double var(--gold);
  box-shadow: inset 0 0 0 1px var(--gold-dark), 0 0 0 1px #3a2a10, 2px 3px 8px rgba(40, 25, 5, 0.55);
}

.page {
  display: flex;
  flex-direction: column;
  flex: 1;
  min-height: 0;
  margin-top: 8px;
  padding: 14px 8px 12px 24px;
}

.page .body { flex: 1; min-height: 0; overflow-y: auto; scrollbar-gutter: stable; padding: 4px 14px 10px 0;
  scrollbar-color: var(--gold-dark) #120e06; font-size: 18px; line-height: 31px; }
.page h2 { font-family: var(--kai); font-size: 32px; line-height: 44px; font-weight: normal; color: #fff2c8;
  margin: 22px 16px 4px 0; padding-bottom: 4px; border-bottom: 1px solid var(--gold-dark); }
.page section:first-child h2 { margin-top: 0; }
.page h3 { font-family: var(--kai); font-size: 23px; font-weight: normal; color: var(--gold-light); margin: 14px 0 2px; }
.page p { margin: 2px 0 6px; }
.page ul { margin: 0 0 6px; padding-left: 26px; }
.page li { margin: 0; }
.page li::marker { color: var(--gold); }

.hint { flex: none; margin-top: 6px; font-size: 13px; color: #5a4524; text-align: right; }
</style>
</head>
<body>
<div class="book">
  <div class="top">
    <button class="sound" id="sound" type="button"></button>
    <h1>更新記錄</h1>
  </div>
  <div class="frame page"><div class="body" id="body"><!--BODY--></div></div>
  <div class="hint">↑↓ PageUp / PageDown 捲動　Home / End 首尾</div>
</div>
<script>
const $ = id => document.getElementById(id);

// The game's cursor sound, played by the switch unless the address says ?sound=off. The switch keeps its state in
// the address (and in the home page's, which hands it to every page it shows).
const MOVE_SOUND = /*SOUND*/null;
const MOVE_VOLUME = 0.6;
let soundOn = new URLSearchParams(location.search).get('sound') !== 'off';

function beep() {
  if (MOVE_SOUND && soundOn) {
    const sound = new Audio(MOVE_SOUND);
    sound.volume = MOVE_VOLUME;
    sound.play().catch(() => {});
  }
}

function setSound(on, tell) {
  soundOn = on;
  const params = new URLSearchParams(location.search);

  if (on) {
    params.delete('sound');
  } else {
    params.set('sound', 'off');
  }

  const search = params.toString();
  history.replaceState(null, '', `${location.pathname}${search ? `?${search}` : ''}${location.hash}`);
  $('sound').textContent = on ? '音效　開' : '音效　關';
  $('sound').classList.toggle('off', !on);

  if (tell && window.parent !== window) {
    window.parent.postMessage({ sound: on }, '*');
  }
}

$('sound').addEventListener('click', event => {
  setSound(!soundOn, true);
  beep();
  event.currentTarget.blur();
});

addEventListener('message', event => {
  if (typeof (event.data || {}).sound === 'boolean' && event.source === window.parent) {
    setSound(event.data.sound, false);
  }
});

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

// A day named in the address (changelog.html#2026-11-05) is scrolled to.
function showNamed(text = addressed()) {
  const section = text ? document.getElementById(text) : null;
  $('body').scrollTop = section ? section.offsetTop - $('body').offsetTop : 0;
}

setSound(soundOn, false);
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
