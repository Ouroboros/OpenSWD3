"""scripts/encounters/build.py <monsters.jsonl> <maps.dat> <battle.ffd> <figtalk.dat> <out.html> [move.wav]:
the map-encounters page (docs/encounters.html), listing the random encounters and story battles of each place in SWD3
with links to the bestiary (docs/bestiary.html).

Random encounters are read live from maps.dat table 7 (encounter areas) for maps whose table 3 encounter group is
non-zero (Game::RollEncounter, 0x0040DA60), with FIGTALK.dat battle switches (command 0x18, Game::FightTalkNextBattle)
verified against the live file. Formations are read live from battle.ffd (choice 0, BattleFile::ReadFormation).
Story battles come from verified mounted TALK*.DAT events (StartBattle 0x58).
"""

import base64
import json
import os
import struct
import sys
from collections import Counter, defaultdict

MAPS_HEADER = 0x200
SETTINGS_TABLE = 3
SETTINGS_BYTES = 0xE
SETTINGS_ENCOUNTER_GROUP_AT = 0xA
AREA_TABLE = 7
AREA_BYTES = 0xE
NAME_TABLE = 0x14
TABLE_END = 0xFFFF

FFD_HEADER = 0x2714
FFD_COUNTS_AT = 0x1F44
FFD_RECORD_BYTES = 0x10C

BESTIARY_CATS = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 13, 14}
OTHER_CATS = {23, 26}


def sound_url(path):
    if not path:
        return None
    with open(path, 'rb') as f:
        return 'data:audio/wav;base64,' + base64.b64encode(f.read()).decode('ascii')


def load_monsters(path):
    records = {}
    hidden = {}
    with open(path, encoding='utf-8') as f:
        for line in f:
            r = json.loads(line)
            if 'hidden' in r:
                hidden[r['id']] = r['hidden']
            else:
                records[r['id']] = r
    out = {}
    for ident, m in records.items():
        h = hidden.get(ident, {})
        in_bestiary = m['category'] in BESTIARY_CATS or (m['category'] in OTHER_CATS and m['hp'] > 0)
        out[ident] = {
            'id': ident,
            'name': m['name'],
            'level': m['level'],
            'boss': 1 if h.get('boss', 0) else 0,
            'in_bestiary': 1 if in_bestiary else 0,
        }
    return out


def load_formations(path):
    ffd = open(path, 'rb').read()
    header = ffd[:FFD_HEADER]
    counts = header[FFD_COUNTS_AT:FFD_HEADER]
    formations = {}
    skipped = 0
    for b in range(1, len(counts)):
        c = struct.unpack_from('<b', counts, b)[0]
        if c > 0:
            rec = struct.unpack_from('<I', header, 4 + 4 * skipped)[0]
            base = FFD_HEADER + FFD_RECORD_BYTES * rec
            if base + FFD_RECORD_BYTES <= len(ffd):
                ec = struct.unpack_from('<H', ffd, base + 0x98)[0]
                ids = [struct.unpack_from('<I', ffd, base + 0x9C + 4 * k)[0] & 0xFFFF for k in range(min(ec, 8))]
                if ec > 0:
                    formations[b] = ids
        skipped += c
    return formations


def load_maps_dat(path):
    raw = open(path, 'rb').read()[MAPS_HEADER:]
    word = lambda at: struct.unpack_from('<H', raw, at)[0]
    dword = lambda at: struct.unpack_from('<I', raw, at)[0]
    tbl = lambda idx: dword(4 * idx)

    place_names = {}
    at = tbl(NAME_TABLE)
    while at + 1 < len(raw):
        code = word(at)
        if code == TABLE_END:
            break
        end = raw.find(b'%Q', at + 2)
        if end < 0:
            break
        place_names[code] = raw[at + 2:end].decode('big5', 'replace').strip()
        at = end + 2

    encounter_group = {}
    at = tbl(SETTINGS_TABLE)
    while at + 1 < len(raw):
        mid = word(at)
        if mid == TABLE_END:
            break
        if mid not in encounter_group:
            encounter_group[mid] = word(at + SETTINGS_ENCOUNTER_GROUP_AT)
        at += SETTINGS_BYTES

    areas_by_map = defaultdict(list)
    at = tbl(AREA_TABLE)
    while at + 1 < len(raw):
        mid = word(at)
        if mid == TABLE_END:
            break
        left, top, right, bottom = struct.unpack_from('<4H', raw, at + 2)
        toff = dword(at + 0xA)
        cnt = word(toff) if toff + 2 <= len(raw) else 0
        battles = [word(toff + 2 * (k + 1)) for k in range(cnt)]
        areas_by_map[mid].append({
            'left': left, 'top': top, 'right': right, 'bottom': bottom,
            'battles': battles,
        })
        at += AREA_BYTES

    return place_names, encounter_group, areas_by_map


def verify_figtalk_switches(path, expected_switches):
    """Verify every (src_battle, flag, dst_battle) FIGTALK 0x30/0x18 switch against FIGTALK.dat."""
    fig = open(path, 'rb').read()
    starts = {}
    for b in range(2000):
        off = struct.unpack_from('<I', fig, 0x200 + 4 * b)[0]
        if 0 < off < len(fig) - 0x200:
            starts[b] = off
    all_offs = sorted(set(starts.values()))
    next_off = {o: (all_offs[i + 1] if i + 1 < len(all_offs) else len(fig) - 0x200)
                for i, o in enumerate(all_offs)}
    for src, flag, dst in expected_switches:
        off = starts.get(src)
        if not off:
            raise SystemExit(f'FIGTALK missing battle {src}')
        chunk = fig[0x200 + off:0x200 + next_off[off]]
        words = struct.unpack_from(f'<{len(chunk) // 2}H', chunk)
        has_flag = any(words[k] == 0x30 and words[k + 1] == flag for k in range(len(words) - 1))
        has_switch = any(words[k] == 0x18 and words[k + 1] == dst for k in range(len(words) - 1))
        if not (has_flag and has_switch):
            raise SystemExit(f'FIGTALK switch verification failed for battle {src} flag {flag:#x} -> {dst}')


# Verified FIGTALK 0x18 battle switches: (src_battle, flag, dst_battle)
SEA_SWITCH = {63: 60, 64: 61, 65: 62}          # flag 0x1B65 (agree to help Venice in ev 621)
DAOIST_SWITCH = {203: 237, 236: 237, 238: 261, 239: 261, 240: 261}  # flag 0x002B (fought Chi You in ev 4049)
HAN_SWITCH = {
    221: 251, 222: 254, 223: 255, 224: 252,
    225: 253, 226: 256, 227: 257, 228: 258, 229: 259,
}  # flag 0x01EE (after Taiji Palace Satan battle in ev 6542)

EXPECTED_FIGTALK_SWITCHES = (
    [(k, 0x1B65, v) for k, v in SEA_SWITCH.items()]
    + [(k, 0x002B, v) for k, v in DAOIST_SWITCH.items()]
    + [(156, 0x002B, 260)]
    + [(k, 0x01EE, v) for k, v in HAN_SWITCH.items()]
)


def fmt_maps(maps):
    if len(maps) == 1:
        return f'地圖 {maps[0]}'
    return '地圖 ' + '、'.join(str(m) for m in maps)


def aggregate_battles(battle_list):
    """Preserve first-seen order while counting duplicates (e.g. [7, 8, 8] -> [[7,1],[8,2]], total=3)."""
    counts = Counter()
    order = []
    for b in battle_list:
        if b not in counts:
            order.append(b)
        counts[b] += 1
    return [[b, counts[b]] for b in order], len(battle_list)


def get_map_battles(areas_by_map, encounter_group, map_id, area_index=None, switch_map=None):
    """Read battles from maps.dat table 7 for map_id (optionally a specific area index), applying switch_map."""
    if encounter_group.get(map_id, 0) == 0:
        raise SystemExit(f'map {map_id} has encounter_group == 0')
    areas = areas_by_map.get(map_id, [])
    if not areas:
        raise SystemExit(f'map {map_id} has no table 7 areas')
    if area_index is not None:
        battles = list(areas[area_index]['battles'])
    else:
        battles = list(areas[0]['battles'])
        # Confirm every area on this map has the same table if area_index is omitted
        for a in areas[1:]:
            if Counter(a['battles']) != Counter(battles):
                raise SystemExit(f'map {map_id} has differing areas; specify area_index')
    if switch_map:
        battles = [switch_map.get(b, b) for b in battles]
    return battles


def verify_same_table(areas_by_map, encounter_group, map_specs, switch_map=None):
    """Given list of map_id or (map_id, area_index), verify all have identical weighted encounter distribution."""
    ref_list = None
    ref_dist = None
    for spec in map_specs:
        mid, aidx = spec if isinstance(spec, tuple) else (spec, None)
        blist = get_map_battles(areas_by_map, encounter_group, mid, aidx, switch_map)
        # Normalize weights by gcd so [101,102,103,104,101,102,103,104] matches [101,102,103,104]
        c = Counter(blist)
        tot = len(blist)
        dist = tuple(sorted((b, cnt * 10000 // tot) for b, cnt in c.items()))
        if ref_dist is None:
            ref_list = blist
            ref_dist = dist
        elif dist != ref_dist:
            raise SystemExit(f'encounter tables differ in group {map_specs}: {ref_list} vs {blist}')
    return ref_list


# Complete, chronologically ordered list of places in SWD3, verified against maps.dat, HUGE.lmf, TALK*.DAT, FIGTALK.dat.
# Each entry:
#   name: display title in left list
#   aliases: extra search keywords (alternative names players use)
#   note: optional period/stage note shown under overview header
#   random: list of (title, detail_or_None, map_specs, switch_map_or_None)
#     where map_specs is a list of map_id or (map_id, area_index)
#   story: list of (event_label, map_id, battle_id, sub_location)
PLACES = [
    {
        'name': '威尼斯',
        'aliases': ['威尼斯酒屋', '威尼斯大廣場', '火刑場'],
        'note': '',
        'random': [],
        'story': [
            ('100', 81, 98, '威尼斯酒屋（開場戰）'),
            ('2071', 20, 32, '威尼斯大廣場（火刑場，第一場）'),
            ('2071', 20, 33, '威尼斯大廣場（火刑場，第二場）'),
        ],
    },
    {
        'name': '義大利北部（大地圖）',
        'aliases': ['大地圖', '威尼斯周邊', '修道院周邊', '法蘭克關隘', '海洞周邊', '青苔石迷宮周邊'],
        'note': '依所處方位分為三個遇敵區',
        'random': [
            ('東北部（威尼斯、法蘭克關隘周邊）', '地圖 10 東北部', [(10, 0)], None),
            ('東南部（海洞周邊）', '地圖 10 東南部', [(10, 1)], None),
            ('西部（修道院、青苔石迷宮周邊）', '地圖 10 西部', [(10, 2), (10, 3)], None),
        ],
        'story': [],
    },
    {
        'name': '威尼斯下水道',
        'aliases': ['威尼斯 下水道', '墓園地道', '地道'],
        'note': '從威尼斯墓園進入、通往威尼斯教堂外；每段遇敵表各自獨立',
        'random': [
            ('第一段（墓園入口）', None, [37], None),
            ('第二段', None, [38], None),
            ('第三段', None, [34], None),
            ('第四段', None, [36], None),
            ('第五段（中段岔口）', None, [39], None),
            ('中段支室', None, [33], None),
            ('第六段', None, [35], None),
            ('出口段（往教堂外）', None, [32], None),
        ],
        'story': [
            ('471', 32, 16, '出口段（往教堂外）'),
        ],
    },
    {
        'name': '威尼斯教堂',
        'aliases': ['威尼斯 教堂內', '教堂左塔', '教堂右塔', '教堂地牢'],
        'note': '',
        'random': [
            ('教堂內', None, [24], None),
            ('右塔底～中層', None, [28, 29], None),
            ('右塔頂', None, [30], None),
            ('左塔底～中層', None, [25, 26], None),
            ('左塔頂', None, [27], None),
        ],
        'story': [
            ('2041', 40, 29, '教堂地牢'),
        ],
    },
    {
        'name': '修道院',
        'aliases': ['修道院 迴廊', '修道院 密室', '修道院 左通道', '修道院 地下洞天'],
        'note': '',
        'random': [
            ('前段（左通道、入口迴廊與密室）', None, [54, 56, 53, 62], None),
            ('中段迴廊', None, [63, 65, 66], None),
            ('後段迴廊與密室（往地下洞天）', None, [75, 74, 68], None),
        ],
        'story': [
            ('284', 53, 55, '前段密室'),
            ('286', 74, 56, '後段密室（寶箱）'),
            ('2050', 72, 99, '地下洞天'),
        ],
    },
    {
        'name': '潔西卡家地窖',
        'aliases': ['潔西卡家 地窖'],
        'note': '從威尼斯民房進入',
        'random': [
            ('前段', None, [45, 44, 46], None),
            ('中段', None, [47, 79], None),
            ('通道', None, [78], None),
            ('深處與密室', None, [49, 77], None),
        ],
        'story': [],
    },
    {
        'name': '青苔石迷宮',
        'aliases': ['海盜秘密洞穴'],
        'note': '第一、二層遇敵表相同',
        'random': [
            ('第一～二層', None, [117, 118], None),
        ],
        'story': [
            ('2059／2064', 118, 84, '第二層（救阿拉伯人）'),
        ],
    },
    {
        'name': '海戰戰場',
        'aliases': ['海戰'],
        'note': '與威尼斯護民官對話選擇是否幫忙，會影響船上的隨機遇敵；打完康那里士劇情後離開',
        'random': [
            ('不幫威尼斯（雙方皆會遇到）', '地圖 110 · 旗標 0x1B65 未開', [110], None),
            ('答應幫威尼斯（只遇梅羅文加軍人）', '地圖 110 · 旗標 0x1B65 開啟，第 63～65 場改為第 60～62 場', [110], SEA_SWITCH),
        ],
        'story': [],
    },
    {
        'name': '移動島・東北密洞',
        'aliases': ['移動島迷宮', '東北密洞'],
        'note': '由移動島海岸進入（離開移動島後無法再回）',
        'random': [
            ('前段', None, [112], None),
            ('中段', None, [113, 114], None),
            ('深處', None, [115, 116], None),
        ],
        'story': [],
    },
    {
        'name': '移動島城堡',
        'aliases': ['移動島迷宮', '移動島城堡大廳', '移動島城堡頂層', '移動島庭園', '浮島機關', '考驗二'],
        'note': '迷宮前半部（地圖 150）與後半部（地圖 152）共用西側、中段、東側三組遇敵表；離開移動島後無法再回',
        'random': [
            ('外圍小迷宮', None, [151], None),
            ('城堡迷宮・西側區域', '地圖 150、152 西側', [(150, 0), (152, 0)], None),
            ('城堡迷宮・中段區域', '地圖 150、152 中段', [(150, 1), (152, 1)], None),
            ('城堡迷宮・東側深處（往浮島機關）', '地圖 150、152 東側', [(150, 2), (152, 2)], None),
        ],
        'story': [
            ('556', 151, 83, '外圍小迷宮'),
            ('690～692', 152, 78, '城堡迷宮後半部（守衛）'),
            ('2077', 168, 53, '城堡頂層（第一場）'),
            ('2077', 168, 54, '城堡頂層（第二場）'),
            ('547', 148, 82, '城堡大廳（藍人來襲）'),
            ('723／724／726', 148, 81, '城堡大廳（巡邏藍人，單隻）'),
            ('725／727', 148, 82, '城堡大廳（巡邏藍人，兩隻）'),
            ('546', 170, 80, '庭園（冰山出現）'),
            ('19～21', 50, 79, '庭園（考驗二，回到過去的修道院外）'),
            ('575', 170, 92, '庭園（四石像會合）'),
        ],
    },
    {
        'name': '冰山',
        'aliases': ['冰山島'],
        'note': '離開移動島後無法再回',
        'random': [
            ('外圍前段', None, [184, 179, 181, 185, 186], None),
            ('中段', None, [183, 182, 177], None),
            ('深處（往藍魔神）', None, [188, 189, 190, 187], None),
        ],
        'story': [
            ('672', 182, 113, '中段（解救女妖）'),
            ('596', 178, 90, '深處通道（藍人埋伏）'),
            ('581／3', 180, 91, '冰山深處（牢房前）'),
        ],
    },
    {
        'name': '地中海東岸（大地圖）',
        'aliases': ['大地圖', '貝魯特周邊', '大馬士革周邊', '肯迪之家周邊', '塔德莫爾周邊'],
        'note': '由西向東分為三個遇敵區',
        'random': [
            ('西部（貝魯特、大馬士革、敘利亞森林周邊）', '地圖 200 西部', [(200, 0)], None),
            ('中部（肯迪之家周邊）', '地圖 200 中部', [(200, 1)], None),
            ('東部（塔德莫爾周邊）', '地圖 200 東部', [(200, 2)], None),
        ],
        'story': [],
    },
    {
        'name': '煉妖壺（奧斯曼戰）',
        'aliases': ['煉妖壺', '大馬士革', '奧斯曼家'],
        'note': '在大馬士革奧斯曼家觸發劇情進入',
        'random': [],
        'story': [
            ('2405', 22, 165, '煉妖壺內'),
        ],
    },
    {
        'name': '敘利亞森林',
        'aliases': ['森林'],
        'note': '地圖原名「森林」，位於大馬士革南方',
        'random': [
            ('全區', None, [214], None),
        ],
        'story': [],
    },
    {
        'name': '塔德莫爾',
        'aliases': ['塔德莫爾 通道', '塔德莫爾 台階', '塔德莫爾 密室', '火象迷宮', '水象迷宮', '土象迷宮', '風象迷宮'],
        'note': '',
        'random': [
            ('入口側通道與密室', None, [261, 267, 268], None),
            ('台階與深處通道', None, [262, 256, 259], None),
            ('火象迷宮', None, [265], None),
            ('水象迷宮', None, [266], None),
            ('土象迷宮', None, [263], None),
            ('風象迷宮', None, [264], None),
        ],
        'story': [
            ('857', 270, 150, '深處通道出口'),
        ],
    },
    {
        'name': '血腥地獄',
        'aliases': ['地獄通道', '地獄廣場', '地獄深處'],
        'note': '地獄通道（地圖 273、274）全區遇敵表相同',
        'random': [
            ('地獄通道', None, [273, 274], None),
        ],
        'story': [
            ('1018', 275, 140, '地獄廣場（人骨烤肉架）'),
            ('1020', 276, 130, '地獄深處'),
        ],
    },
    {
        'name': '大馬士革皇宮',
        'aliases': ['皇宮通道', '皇宮走廊', '歐馬亞皇宮大殿'],
        'note': '',
        'random': [
            ('皇宮通道', None, [195], None),
        ],
        'story': [
            ('889', 197, 160, '皇宮走廊（第一場）'),
            ('889', 197, 161, '皇宮走廊（第二場）'),
            ('894', 196, 162, '歐馬亞皇宮大殿（第一場）'),
            ('894', 196, 163, '歐馬亞皇宮大殿（第二場）'),
        ],
    },
    {
        'name': '肯迪之家',
        'aliases': ['薇達比武'],
        'note': '大馬士革皇宮事件後，離開阿拉伯前與薇達比武',
        'random': [],
        'story': [
            ('3', 250, 164, '啟程前比武'),
        ],
    },
    {
        'name': '美索不達米亞山區',
        'aliases': ['山洞迷宮'],
        'note': '分為南半部（兩河平原入口側）與北半部（往西域深處）兩區',
        'random': [
            ('南半部（兩河平原入口側）', '地圖 61 南半部', [(61, 0)], None),
            ('北半部（深處，往西域）', '地圖 61 北半部', [(61, 1)], None),
        ],
        'story': [
            ('1177', 61, 145, '山洞中段（守路怪物）'),
        ],
    },
    {
        'name': '西域（大地圖）',
        'aliases': ['大地圖', '石國周邊', '天山山腳周邊', '怛羅斯以東'],
        'note': '僅怛羅斯以東區域會隨機遇敵；兩河平原大地圖無隨機遇敵',
        'random': [
            ('怛羅斯以東（石國、天山山腳周邊）', '地圖 8 東部', [(8, 0)], None),
        ],
        'story': [],
    },
    {
        'name': '石國',
        'aliases': ['石國地下水路', '石國王子營區', '幻影石國'],
        'note': '',
        'random': [
            ('石國地下水路', None, [43], None),
        ],
        'story': [
            ('97／996', 213, 180, '石國王子營區（初見遠恩／守衛）'),
            ('994', 213, 181, '石國王子營區（營帳前）'),
            ('4037', 213, 166, '石國王子營區（幻影石國）'),
            ('2243', 43, 185, '石國地下水路（迷宮內）'),
        ],
    },
    {
        'name': '怛羅斯',
        'aliases': ['怛羅斯戰場'],
        'note': '兩段戰場地圖遇敵表相同',
        'random': [
            ('戰場', None, [159, 160], None),
        ],
        'story': [
            ('4027', 159, 147, '戰場前段'),
            ('4032／4033', 160, 147, '戰場後段'),
            ('4028', 158, 146, '戰場中心（黃雷）'),
        ],
    },
    {
        'name': '天山',
        'aliases': ['天山山洞', '天山山道', '天山山頂', '天山－軒轅劍石室', '蚩尤'],
        'note': '',
        'random': [
            ('山洞前段', None, [300, 301], None),
            ('山洞中段與支室', None, [299, 302, 303], None),
            ('山洞後段（往山頂出口）', None, [304], None),
        ],
        'story': [
            ('4096', 283, 197, '天山山道（遇見黃雷，地圖 283）'),
            ('4097', 286, 175, '天山山道（地圖 286）'),
            ('4099', 307, 177, '天山山道（地圖 307）'),
            ('4015', 194, 178, '天山山頂'),
            ('4049', 240, 176, '天山－軒轅劍石室（隱藏頭目，打過後開啟旗標 0x2B）'),
        ],
    },
    {
        'name': '黃河流域（大地圖）',
        'aliases': ['大地圖', '長安周邊', '漢長安廢墟周邊', '驪山周邊', '洛陽周邊', '少林寺周邊'],
        'note': '以長安、洛陽之間為界分為東西兩區',
        'random': [
            ('西部（長安、漢長安廢墟、驪山周邊）', '地圖 9 西部', [(9, 0)], None),
            ('東部（洛陽、少林寺周邊）', '地圖 9 東部', [(9, 1)], None),
        ],
        'story': [],
    },
    {
        'name': '少林寺木人巷',
        'aliases': ['木人巷', '木人巷大廳'],
        'note': '第一段（地圖 11）為隨機遇敵；其餘各段（地圖 12～15）為觸碰巡邏機關和尚開戰',
        'random': [
            ('第一段（隨機遇敵）', None, [11], None),
        ],
        'story': [
            ('6134', 12, 187, '第二段（觸碰機關和尚）'),
            ('6136', 13, 187, '第三段（觸碰機關和尚）'),
            ('6137', 14, 186, '第四段（觸碰機關和尚）'),
            ('6135', 15, 186, '第五段（觸碰機關和尚，單隻）'),
            ('6142／6143', 15, 187, '第五段（觸碰機關和尚，兩隻）'),
            ('6104', 102, 220, '木人巷大廳（出口）'),
        ],
    },
    {
        'name': '漢長安城（第一次）',
        'aliases': ['漢長安廢墟', '人骨佛堂', '安史之亂前'],
        'note': '安史之亂前（追黃八時）；安史之亂後迷宮全數換為高階神魔，見「漢長安城（復活後）」',
        'random': [
            ('前段', None, [315], None),
            ('後段', None, [316], None),
        ],
        'story': [
            ('6805', 315, 230, '前段（黃八）'),
            ('6806', 316, 204, '後段（黑壇教道士）'),
            ('6810', 317, 202, '人骨佛堂'),
        ],
    },
    {
        'name': '王大善人家',
        'aliases': ['王思月家'],
        'note': '地圖原名「王思月家」，位於長安城內',
        'random': [],
        'story': [
            ('6302', 313, 200, '後院屋內（黃雷）'),
        ],
    },
    {
        'name': '驪山樹林',
        'aliases': ['驪山'],
        'note': '',
        'random': [
            ('前段', None, [192], None),
            ('後段（往祭壇）', None, [193], None),
        ],
        'story': [
            ('6410', 193, 201, '祭壇'),
        ],
    },
    {
        'name': '洛陽道觀（蚩尤戰前）',
        'aliases': ['道觀一樓', '道觀二樓', '道觀三樓', '道觀四樓', '道觀頂樓', '洛陽道觀'],
        'note': '尚未打過天山蚩尤時；打過蚩尤（旗標 0x2B 開啟）後隨機遇敵與四樓解籤戰皆會替換，見「洛陽道觀（蚩尤戰後）」',
        'random': [
            ('一～二樓', None, [157, 156], None),
            ('三樓、頂樓', None, [155, 153], None),
        ],
        'story': [
            ('6075', 154, 156, '四樓解籤（抽中「有兔爰爰」）'),
            ('6072', 153, 205, '頂樓（第一場，王思月與鬼魂）'),
            ('6072', 153, 206, '頂樓（第二場，雨丹子）'),
        ],
    },
    {
        'name': '洛陽道觀（蚩尤戰後）',
        'aliases': ['道觀一樓', '道觀二樓', '道觀三樓', '道觀四樓', '道觀頂樓', '洛陽道觀', '牛魔王', 'DOMO神打功'],
        'note': '打過天山蚩尤（旗標 0x2B 開啟）後，戰鬥腳本會替換道觀隨機遇敵與四樓解籤戰（未持 DOMO神打功時改為牛魔王）',
        'random': [
            ('一～二樓', '地圖 157、156 · 原第 203、236 場改為第 237 場，第 240 場改為第 261 場', [157, 156], DAOIST_SWITCH),
            ('三樓、頂樓', '地圖 155、153 · 原第 238、239 場改為第 261 場', [155, 153], DAOIST_SWITCH),
        ],
        'story': [
            ('6075', 154, 260, '四樓解籤（抽中「有兔爰爰」，原第 156 場替換為牛魔王）'),
            ('6072', 153, 205, '頂樓（若尚未觸發，第一場）'),
            ('6072', 153, 206, '頂樓（若尚未觸發，第二場）'),
        ],
    },
    {
        'name': '東平郡王府（安祿山府）',
        'aliases': ['安祿山府', '王府大廳', '王府東廂房', '王府西廂房', '王府南廂房', '王府北廂房', '王府二樓', '王府三樓', '王府四樓', '王府密道', '皇城－太極宮'],
        'note': '',
        'random': [
            ('大廳、東／西／南廂房與二樓', None, [287, 289, 290, 293, 294], None),
            ('三～四樓與北廂房', None, [291, 292, 295], None),
            ('王府密道', None, [297], None),
        ],
        'story': [
            ('6509', 289, 218, '王府東廂房'),
            ('6545／6546', 291, 219, '王府三樓（解救慧榮、張客後）'),
            ('2288', 297, 241, '王府密道（守衛）'),
            ('6542', 171, 290, '皇城－太極宮（由密道進入，觸發安史之亂）'),
        ],
    },
    {
        'name': '漢長安城（復活後）',
        'aliases': ['漢長安廢墟', '復活後的漢長安廢墟', '人骨佛堂', '安史之亂後', '最終戰'],
        'note': '安史之亂（太極宮撒旦戰，旗標 0x1EE 開啟）後進入，原隨機遇敵全數由戰鬥腳本替換為高階神魔',
        'random': [
            ('前段', '地圖 315 · 原第 221～224 場替換為第 251、254、255、252 場', [315], HAN_SWITCH),
            ('後段', '地圖 316 · 原第 225～229 場替換為第 253、256、257、259、258 場', [316], HAN_SWITCH),
        ],
        'story': [
            ('6910／6810', 318, 291, '廢墟盡頭（最終戰第一場）'),
            ('6910／6810', 337, 292, '人骨佛堂（最終戰第二場）'),
        ],
    },
]


def main():
    monsters_path, maps_path, ffd_path, fig_path, out_path, *rest = sys.argv[1:]
    monsters = load_monsters(monsters_path)
    formations = load_formations(ffd_path)
    place_names, encounter_group, areas_by_map = load_maps_dat(maps_path)
    verify_figtalk_switches(fig_path, EXPECTED_FIGTALK_SWITCHES)

    # Verify 100% coverage of all maps.dat random encounter areas (encounter_group > 0 and non-empty table 7)
    covered_areas = set()
    used_formations = {}
    used_monsters = {}
    out_places = []

    for spec in PLACES:
        place_maps = set()
        place_monsters = set()
        random_out = []

        for title, custom_detail, map_specs, switch_map in spec['random']:
            blist = verify_same_table(areas_by_map, encounter_group, map_specs, switch_map)
            mids = []
            for ms in map_specs:
                if isinstance(ms, tuple):
                    mid, aidx = ms
                    covered_areas.add((mid, aidx))
                else:
                    mid = ms
                    for aidx in range(len(areas_by_map[mid])):
                        covered_areas.add((mid, aidx))
                if mid not in mids:
                    mids.append(mid)
                place_maps.add(mid)

            rows, total = aggregate_battles(blist)
            for b, _ in rows:
                if b not in formations:
                    raise SystemExit(f'missing formation for battle {b}')
                used_formations[str(b)] = formations[b]
                place_monsters.update(formations[b])

            detail = custom_detail if custom_detail is not None else fmt_maps(mids)
            random_out.append({
                'title': title,
                'detail': detail,
                'total': total,
                'rows': rows,
            })

        story_out = []
        for ev_label, mid, bid, sub in spec['story']:
            if bid not in formations:
                raise SystemExit(f'missing formation for story battle {bid}')
            used_formations[str(bid)] = formations[bid]
            place_monsters.update(formations[bid])
            place_maps.add(mid)
            story_out.append([ev_label, mid, bid, sub])

        for ident in place_monsters:
            if ident not in monsters:
                raise SystemExit(f'missing monster record {ident}')
            m = monsters[ident]
            used_monsters[str(ident)] = [m['name'], m['level'], m['boss'], m['in_bestiary']]

        sorted_monsters = sorted(place_monsters, key=lambda i: (monsters[i]['level'], i))
        out_places.append({
            'name': spec['name'],
            'aliases': spec['aliases'],
            'note': spec['note'],
            'maps': sorted(place_maps),
            'monsters': sorted_monsters,
            'random': random_out,
            'story': story_out,
        })

    # Assert every random encounter area in maps.dat is covered
    all_areas = {
        (mid, aidx)
        for mid, areas in areas_by_map.items()
        if encounter_group.get(mid, 0) > 0
        for aidx in range(len(areas))
    }
    uncovered = sorted(all_areas - covered_areas)
    if uncovered:
        raise SystemExit(f'uncovered random encounter areas: {uncovered}')

    non_bestiary = [
        (int(k), v[0]) for k, v in used_monsters.items() if v[3] == 0
    ]

    page_data = {
        'places': out_places,
        'formations': used_formations,
        'monsters': used_monsters,
    }
    page = TEMPLATE.replace('/*DATA*/null', json.dumps(page_data, ensure_ascii=False, separators=(',', ':')))
    page = page.replace('/*SOUND*/null', json.dumps(sound_url(rest[0] if rest else '')))
    with open(out_path, 'w', encoding='utf-8', newline='\n') as out:
        out.write(page)

    print(
        f'{len(out_places)} places, {len(used_formations)} formations, '
        f'{len(used_monsters)} monsters (non-bestiary: {non_bestiary}), {len(page)} bytes'
    )


TEMPLATE = r'''<!DOCTYPE html>
<html lang="zh-Hant">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>地圖遇敵</title>
<style>
:root {
  --gold: #c9a24a;
  --gold-dark: #7a5a1c;
  --gold-light: #f2d27a;
  --text: #e9dcb8;
  --dim: #b9a67a;
  --panel: rgba(14, 10, 4, 0.94);
  --ink: #7a1f12;
  --jade: #4f8a72;
  --jade-dark: #1f3d33;
  --jade-light: #9fd4bb;
  --warn: #ff8a6a;
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

.search {
  width: 360px;
  height: 34px;
  margin: 0 0 6px 6px;
  padding: 0 10px;
  font: inherit;
  font-size: 18px;
  color: #3b2a12;
  background: rgba(255, 248, 228, 0.8);
  border: 1px solid rgba(122, 60, 20, 0.6);
  border-radius: 3px;
  outline: none;
}

.search:focus { border-color: var(--ink); background: #fffaf0; }

.sound {
  margin: 0 auto 6px 12px;
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

h1 {
  margin: 0;
  font-family: var(--kai);
  font-size: 44px;
  letter-spacing: 6px;
  color: var(--ink);
  text-shadow: 2px 2px 0 rgba(255, 240, 210, 0.6), 0 0 6px rgba(122, 31, 18, 0.25);
  font-weight: bold;
}

.layout {
  display: grid;
  grid-template-columns: 326px 1fr;
  gap: 0 14px;
  margin-top: 8px;
  flex: 1;
  min-height: 0;
}

.frame {
  background: var(--panel);
  border: 3px double var(--gold);
  box-shadow: inset 0 0 0 1px var(--gold-dark), 0 0 0 1px #3a2a10, 2px 3px 8px rgba(40, 25, 5, 0.55);
}

.list {
  position: relative;
  min-height: 0;
  overflow-x: hidden;
  overflow-y: auto;
  scroll-padding: 12px 0;
  scrollbar-gutter: stable;
  padding: 12px 6px 12px 10px;
  scrollbar-color: var(--gold-dark) #120e06;
}

.row {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  font-family: var(--kai);
  font-size: 21px;
  line-height: 36px;
  padding: 0 8px 0 6px;
  cursor: pointer;
  white-space: nowrap;
  text-shadow: 1px 1px 0 #000;
}

.row span:first-child { overflow: hidden; text-overflow: ellipsis; }
.row .count { font-family: var(--ming); font-size: 14px; color: var(--dim); flex: none; margin-left: 6px; }
.row.chosen {
  color: #fff6d6;
  background: linear-gradient(90deg, rgba(232, 168, 56, 0.95), rgba(170, 110, 20, 0.75) 45%, rgba(90, 55, 5, 0.1));
}
.row.chosen .count { color: #fff0c0; }
.empty { font-family: var(--kai); font-size: 20px; color: var(--dim); padding: 8px 6px; }

.right {
  display: flex;
  flex-direction: column;
  gap: 10px;
  min-height: 0;
  min-width: 0;
}

.overview {
  flex: none;
  padding: 14px 18px 12px;
  max-height: 290px;
  overflow-y: auto;
  scrollbar-color: var(--gold-dark) #120e06;
}

.place-head {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 12px;
  border-bottom: 1px solid var(--gold-dark);
  padding-bottom: 4px;
  margin-bottom: 8px;
}

.place-title {
  font-family: var(--kai);
  font-size: 28px;
  line-height: 36px;
  color: #fff2c8;
}

.place-meta {
  font-size: 15px;
  color: var(--dim);
  white-space: nowrap;
}

.place-note {
  font-size: 15px;
  line-height: 24px;
  color: var(--dim);
  margin: -2px 0 8px;
}

.chips {
  display: flex;
  flex-wrap: wrap;
  gap: 6px 8px;
}

.chip {
  display: inline-flex;
  align-items: baseline;
  gap: 5px;
  padding: 2px 10px;
  font-size: 16px;
  line-height: 24px;
  color: var(--text);
  text-decoration: none;
  background: rgba(201, 162, 74, 0.12);
  border: 1px solid rgba(201, 162, 74, 0.45);
  border-radius: 4px;
  white-space: nowrap;
}

a.chip:hover {
  color: #fff6d6;
  border-color: var(--gold-light);
  background: rgba(232, 168, 56, 0.25);
}

.chip.no-link {
  border-style: dashed;
  color: var(--dim);
}

.chip.hit {
  color: #fff6d6;
  border-color: var(--gold-light);
  background: rgba(232, 168, 56, 0.34);
}

.chip .lv { font-size: 13px; color: var(--dim); }
.chip.hit .lv { color: #fff0c0; }

.boss {
  display: inline-block;
  padding: 0 5px;
  font-family: var(--ming);
  font-size: 11px;
  line-height: 17px;
  color: #fff2e8;
  background: rgba(168, 42, 24, 0.85);
  border: 1px solid var(--warn);
  border-radius: 3px;
  vertical-align: 1px;
}

.detail {
  position: relative;
  flex: 1;
  min-height: 0;
  overflow-y: auto;
  overflow-x: hidden;
  scrollbar-gutter: stable;
  padding: 12px 12px 12px 18px;
  scrollbar-color: var(--gold-dark) #120e06;
}

.group { margin-bottom: 16px; }
.group:last-child { margin-bottom: 4px; }

.group-head {
  font-family: var(--kai);
  font-size: 21px;
  line-height: 30px;
  color: var(--gold-light);
  border-bottom: 1px solid var(--gold-dark);
  padding-bottom: 2px;
  margin-bottom: 6px;
}

.group-head .sub {
  font-family: var(--ming);
  font-size: 14px;
  color: var(--dim);
  margin-left: 10px;
}

.detail table {
  width: 100%;
  border-collapse: collapse;
  font-size: 16px;
  line-height: 26px;
}

.detail th, .detail td {
  text-align: left;
  vertical-align: top;
  padding: 5px 8px;
  border-bottom: 1px dashed rgba(201, 162, 74, 0.3);
}

.detail th {
  font-family: var(--kai);
  font-weight: normal;
  font-size: 16px;
  color: var(--gold-light);
  background: rgba(201, 162, 74, 0.16);
  border-bottom: 1px solid rgba(201, 162, 74, 0.5);
  white-space: nowrap;
}

.detail td.battle { width: 84px; white-space: nowrap; color: var(--dim); }
.detail td.rate { width: 112px; white-space: nowrap; color: #fff2c8; }
.detail td.event { width: 250px; color: #fff2c8; }
.detail td.enemies { overflow-wrap: anywhere; }

a.mlink {
  color: var(--gold-light);
  text-decoration: none;
  border-bottom: 1px dotted rgba(242, 210, 122, 0.55);
}

a.mlink:hover {
  color: #fff6d6;
  border-bottom-style: solid;
}

a.mlink.hit, span.mplain.hit {
  color: #1e1406;
  background: var(--gold-light);
  border-radius: 3px;
  padding: 0 3px;
  border-bottom: none;
}

span.mplain { color: var(--text); }
.lv-inline { font-size: 13px; color: var(--dim); margin-left: 2px; }
.times { color: var(--jade-light); font-weight: bold; margin-left: 3px; }
.plus { color: var(--dim); margin: 0 5px; }

.hint { flex: none; margin-top: 6px; font-size: 13px; color: #5a4524; text-align: right; }
</style>
</head>
<body>
<div class="book">
  <div class="top">
    <input class="search" id="search" type="search" placeholder="搜尋地點、分區、地圖號、怪物（空白分隔多詞）" autocomplete="off">
    <button class="sound" id="sound" type="button"></button>
    <h1>地圖遇敵</h1>
  </div>
  <div class="layout">
    <div class="frame list" id="list"></div>
    <div class="right">
      <div class="frame overview" id="overview"></div>
      <div class="frame detail" id="detail"></div>
    </div>
  </div>
  <div class="hint">↑↓ PgUp / PgDn Home / End 選地點　點擊怪物名稱可跳轉至神魔異事錄</div>
</div>
<script>
const DATA = /*DATA*/null;
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

const $ = id => document.getElementById(id);

function escape(text) {
  return String(text).replace(/[&<>"]/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]);
}

function gcd(a, b) {
  while (b) {
    [a, b] = [b, a % b];
  }

  return a;
}

function formatRate(count, total) {
  if (count >= total) {
    return '100%';
  }

  const g = gcd(count, total);
  const pct = Math.round((count * 100) / total);
  return `${count / g}/${total / g} (${pct}%)`;
}

function monsterOf(id) {
  const [name, level, boss, inBestiary] = DATA.monsters[String(id)];
  return { id, name, level, boss, inBestiary };
}

function monsterHref(m) {
  return `bestiary.html${location.search}#${encodeURIComponent(`${m.name}@${m.id}`)}`;
}

// Search matches place name, aliases, notes, sub-area titles/details, map numbers, and every monster name encountered.
DATA.places.forEach(place => {
  const names = place.monsters.map(id => DATA.monsters[String(id)][0]);
  const mapTags = place.maps.map(m => `地圖${m} 地圖 ${m}`);
  const groupTags = place.random.map(g => `${g.title} ${g.detail}`);
  const storyTags = place.story.map(s => `${s[3]} 事件${s[0]} 事件 ${s[0]}`);
  place.text = [place.name, ...place.aliases, place.note, ...names, ...mapTags, ...groupTags, ...storyTags]
    .join('\n')
    .toLowerCase();
});

let shown = [];
let chosen = 0;
let currentPlace = null;

function query() {
  return $('search').value.trim().toLowerCase().split(/\s+/).filter(Boolean);
}

function monsterHit(m, words) {
  if (!words.length) {
    return false;
  }

  const lower = m.name.toLowerCase();
  return words.some(word => lower.includes(word));
}

function keepInView(row) {
  const box = $('list');

  if (chosen === 0) {
    box.scrollTop = 0;
    return;
  }

  if (chosen === shown.length - 1) {
    box.scrollTop = box.scrollHeight;
    return;
  }

  const pad = 12;
  const top = row.offsetTop - pad;
  const bottom = row.offsetTop + row.offsetHeight + pad;

  if (top < box.scrollTop) {
    box.scrollTop = Math.max(0, top);
  } else if (bottom > box.scrollTop + box.clientHeight) {
    box.scrollTop = bottom - box.clientHeight;
  }
}

function renderEnemies(battle, words) {
  const ids = DATA.formations[String(battle)] || [];
  const counts = new Map();
  const order = [];

  for (const id of ids) {
    if (!counts.has(id)) {
      counts.set(id, 0);
      order.push(id);
    }

    counts.set(id, counts.get(id) + 1);
  }

  return order.map(id => {
    const m = monsterOf(id);
    const n = counts.get(id);
    const hit = monsterHit(m, words) ? ' hit' : '';
    const boss = m.boss ? ' <span class="boss">BOSS</span>' : '';
    const times = n > 1 ? `<span class="times">×${n}</span>` : '';
    const label = m.inBestiary
      ? `<a class="mlink${hit}" href="${monsterHref(m)}" data-id="${m.id}">${escape(m.name)}</a>`
      : `<span class="mplain${hit}" title="此敵人未收錄於神魔異事錄">${escape(m.name)}</span>`;
    return `${label}<span class="lv-inline">(${m.level}級)</span>${boss}${times}`;
  }).join('<span class="plus">＋</span>');
}

function show() {
  document.querySelectorAll('.row').forEach((row, at) => row.classList.toggle('chosen', at === chosen));
  const row = document.querySelector('.row.chosen');

  if (row) {
    keepInView(row);
  }

  const place = shown[chosen];

  if (!place) {
    $('overview').innerHTML = '';
    $('detail').innerHTML = '';
    return;
  }

  currentPlace = place;
  keepPlace(place.name);
  const words = query();
  const kinds = [];

  if (place.random.length) {
    kinds.push('隨機遇敵');
  }

  if (place.story.length) {
    kinds.push('劇情戰鬥');
  }

  const chips = place.monsters.map(id => {
    const m = monsterOf(id);
    const hit = monsterHit(m, words) ? ' hit' : '';
    const boss = m.boss ? '<span class="boss">BOSS</span>' : '';

    if (m.inBestiary) {
      return `<a class="chip${hit}" href="${monsterHref(m)}" data-id="${m.id}">` +
        `<span>${escape(m.name)}</span><span class="lv">${m.level}級</span>${boss}</a>`;
    }

    return `<span class="chip no-link${hit}" title="此敵人未收錄於神魔異事錄">` +
      `<span>${escape(m.name)}</span><span class="lv">${m.level}級</span>${boss}</span>`;
  }).join('');

  const note = place.note ? `<div class="place-note">${escape(place.note)}</div>` : '';

  $('overview').innerHTML =
    `<div class="place-head"><div class="place-title">${escape(place.name)}</div>` +
    `<div class="place-meta">共 ${place.monsters.length} 種怪物 · ${kinds.join('、')}</div></div>` +
    `${note}<div class="chips">${chips}</div>`;

  const blocks = [];

  for (const group of place.random) {
    const head = `${escape(group.title)}<span class="sub">${escape(group.detail)}</span>`;
    const trs = group.rows.map(([battle, count]) =>
      `<tr><td class="battle">第 ${battle} 場</td>` +
      `<td class="rate">${formatRate(count, group.total)}</td>` +
      `<td class="enemies">${renderEnemies(battle, words)}</td></tr>`).join('');
    blocks.push(
      `<div class="group"><div class="group-head">${head}</div>` +
      `<table><tr><th>戰鬥</th><th>機率</th><th>敵人</th></tr>${trs}</table></div>`
    );
  }

  if (place.story.length) {
    const trs = place.story.map(([ev, mapId, battle, sub]) => {
      const where = sub ? `${sub} · 事件 ${ev}` : `事件 ${ev}（地圖 ${mapId}）`;
      return `<tr><td class="event">${escape(where)}</td>` +
        `<td class="battle">第 ${battle} 場</td>` +
        `<td class="enemies">${renderEnemies(battle, words)}</td></tr>`;
    }).join('');
    blocks.push(
      `<div class="group"><div class="group-head">劇情戰鬥</div>` +
      `<table><tr><th>觸發</th><th>戰鬥</th><th>敵人</th></tr>${trs}</table></div>`
    );
  }

  $('detail').innerHTML = blocks.join('');
  const firstHit = $('detail').querySelector('.hit');

  if (firstHit) {
    const group = firstHit.closest('.group');

    if (group) {
      $('detail').scrollTop = Math.max(0, group.offsetTop - 12);
    }
  }
}

function list(keepCurrent = false) {
  const current = keepCurrent ? currentPlace : null;
  const words = query();
  shown = words.length
    ? DATA.places.filter(place => words.every(word => place.text.includes(word)))
    : DATA.places;
  const kept = current ? shown.indexOf(current) : -1;
  chosen = kept >= 0 ? kept : Math.min(chosen, Math.max(shown.length - 1, 0));

  if (kept < 0 && keepCurrent) {
    chosen = 0;
    $('detail').scrollTop = 0;
  }

  $('list').innerHTML = shown.length
    ? shown.map((place, at) =>
        `<div class="row${at === chosen ? ' chosen' : ''}" data-at="${at}">` +
        `<span>${escape(place.name)}</span><span class="count">${place.monsters.length} 種</span></div>`).join('')
    : '<div class="empty">找不到</div>';
  show();
}

function move(by) {
  if (!shown.length) {
    return;
  }

  const before = chosen;
  chosen = Math.max(0, Math.min(shown.length - 1, chosen + by));

  if (chosen !== before) {
    beep();
    $('detail').scrollTop = 0;
  }

  show();
}

function openMonster(id) {
  const m = monsterOf(id);

  if (!m.inBestiary) {
    return;
  }

  const detail = `${m.name}@${m.id}`;

  if (window.parent !== window) {
    window.parent.postMessage({ open: 'bestiary', detail }, '*');
    return;
  }

  location.href = `bestiary.html${location.search}#${encodeURIComponent(detail)}`;
}

document.addEventListener('click', event => {
  if (event.metaKey || event.ctrlKey || event.shiftKey || event.altKey || event.button !== 0) {
    return;
  }

  const link = event.target.closest('a.chip, a.mlink');

  if (!link) {
    return;
  }

  event.preventDefault();
  openMonster(Number(link.dataset.id));
});

$('list').addEventListener('click', event => {
  const row = event.target.closest('.row');

  if (row) {
    const at = Number(row.dataset.at);

    if (at !== chosen) {
      beep();
      $('detail').scrollTop = 0;
    }

    chosen = at;
    show();
  }
});

$('search').addEventListener('input', () => {
  list(true);
});

document.addEventListener('keydown', event => {
  const typing = event.target === $('search');

  if (typing && event.key === 'Escape') {
    $('search').value = '';
    $('search').dispatchEvent(new Event('input'));
    return;
  }

  if (typing && ['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) {
    return;
  }

  const keys = {
    ArrowUp: () => move(-1),
    ArrowDown: () => move(1),
    PageUp: () => move(-15),
    PageDown: () => move(15),
    Home: () => move(-shown.length),
    End: () => move(shown.length),
  };

  if (keys[event.key]) {
    keys[event.key]();
    event.preventDefault();
  }
});

function fit() {
  const book = document.querySelector('.book');
  const scale = Math.min(innerWidth / book.offsetWidth, innerHeight / book.offsetHeight);
  const left = (innerWidth - book.offsetWidth * scale) / 2;
  const top = (innerHeight - book.offsetHeight * scale) / 2;
  book.style.transform = `translate(${left}px, ${top}px) scale(${scale})`;
}

function keepPlace(detail) {
  history.replaceState(null, '', `${location.search}#${encodeURIComponent(detail)}`);

  if (window.parent !== window) {
    window.parent.postMessage({ shown: 'encounters', detail }, '*');
  }
}

function addressed() {
  try {
    return decodeURIComponent(location.hash.slice(1));
  } catch {
    return '';
  }
}

function chooseNamed(text = addressed()) {
  if (!text) {
    return;
  }

  const at = DATA.places.findIndex(place => place.name === text);

  if (at < 0) {
    return;
  }

  $('search').value = '';
  list();
  chosen = at;
  $('detail').scrollTop = 0;
  show();
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

setSound(soundOn, false);
addEventListener('resize', fit);
addEventListener('hashchange', () => chooseNamed());
const place = addressed();
const initial = place ? DATA.places.findIndex(p => p.name === place) : -1;
if (initial >= 0) {
  chosen = initial;
}
fit();
list();
</script>
</body>
</html>
'''

if __name__ == '__main__':
    main()
