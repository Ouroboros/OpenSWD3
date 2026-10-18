# 音频泵、作弊对话框与角色属性

## 0x0040CF10 Game::AudioPump

依次调用 0x004854B0（0x4BA6E8，MusicQueue::Update）、0x00486900（0x4C9288，StreamPlayer::Update）、0x00485180（0x4B7EB8，MidiPlayer::Update）、0x00486080（0x4C8450，SoundEffects::Update），返回 al = 1。重建顺序相同；返回值无调用方使用（各调用点之后没有 test al），重建为 void。exact。

## 0x0040F890 CheatProcedure（作弊对话框过程）

- WM_NOTIFY：TCN_SELCHANGE（0xFFFFFDD9）时 tab = TCM_GETCURSEL，删除第 3..0 栏，AddColumns，Fill，0x3F4／0x3EB／0x3EE／0x3EC 先启用，tab ≥ 5 时停用；返回 0。一致。
- WM_INITDIALOG：标签 0 为 0x4A020C「物品」，1..4 为四个角色名（"%s"），5..8 为「%s屬性」（0x4A01FC），9 为 0x4A01F4「變數」；tab = 0；AddColumns；LVM_SETEXTENDEDLISTVIEWSTYLE(0, 0x20)；Fill；EM_LIMITTEXT：0x3ED 4、0x3F5 8、0x3F4 4；返回 1。一致。
- 0x3E8：dword_4CC2AC = 1，EndDialog(1)，ShowCursor(0)。一致。
- 0x3EB 补满：tab ≥ 5 返回；LVM_GETSELECTIONMARK < 0 返回；tab 1..4 返回；SkipItems 取节点；pending(+0xA) + count(+8) < 0x5A 时 pending = 0x5A，再大于 0x5A 时 count = 0x5A − pending；SetRow(name, count + pending, id, 0, 0)。一致。
- 0x3EC 加入：tab ≥ 5 返回；号码栏为空返回；技能页号码须在 0x5DD..0x7CF；数量栏为空或技能页取 "1"，附加值栏为空或技能页取 "0"；AddItem(列表, id, 数量, 0)；返回空时记「物品插入發生錯誤」（0x4A0214）；物品页按 0x49E344[0]、[7]、[9] 的种类与 id 1..0x1F4 设计数（SetCounterProgress 0x4ACBA8 用 +0x50 的上限、SetProficiency 0x4B8A00、SetCounterCapped 0x4B9F00）；Fill；清空三栏。一致。
- 0x3EE 删除：AddItem(列表, 节点 id, −0x400, 0) 非空时记「物品刪除發生錯誤」（0x4A023C）；Fill。一致。
- 0x3F6 设定：变量页号码 ≤ 0x3F 时写 dword_4ACBD0[号码]；物品与技能页一律在物品表找节点（0x44D680），数量栏非空时 count = 0、pending = min(数量, 0x5A)，物品页设计数；属性页号码 ≤ 0x10 时 SetRoleStat(号码, 数量, tab − 5)；号码大于上限时直接返回（不刷新）。一致。
- 已登记的偏差（deviations.md 0x0040F890）：负数号码不写变量表；加入失败、选中行没有节点、技能表为空时不读空指针；空栏按 0（原版为栈上残留值）；宽字符 API 显示同一资源模板。

## 0x004103C0 AddColumns、0x00410490 InsertRow、0x00410600 SetRow

- AddColumns：mask 0xF，fmt 0，宽 0x78／0x40／0x40／0x60，标题 0x4A020C「物品」、0x4A0260「數量」、0x4A0258「號碼」、0x4A0250「附加值」，cchTextMax 4；tab ≥ 5 时只有前 3 栏。一致。
- InsertRow：LVM_INSERTITEM，mask 0xD，iItem = 行，再按 SetRow 写四栏。一致。
- 第 4 栏：值为 −1 时为空；上限为 0 时 "%d%%" 被随后的 "%d" 覆盖，只显示数字（原版 BUG，保留）；上限大于 1 为 "%d/%d"；上限为 1 时缓冲区仍是上一栏的号码文字；其余为 "%d"。CheatDialog::ValueText 相同。

## 0x00410730 Fill

- LVM_DELETEALLITEMS；属性页 17 行（名称表 0x49FC14，每项 8 字节，到 0x49FC94），值为 RoleStat(行, tab − 5)，id 为行号，附加值 (0, 0)；变量页 64 行（dword_4ACBD0..0x4ACCD0），附加值 (−1, 0)；物品与技能页逐节点，物品页按种类取计数。
- 变量页名称：栈上 64 项 × 8 字节，第 0 项为 word_4A0288 加 byte_4A028A，第 11、12 项为 "Cmpy1"、"Cmpy2"，其余各项首字节取自 0x4CC2D8（只作空字符串使用，没有写入，未初始化区为 0），即空名。
- 差异（diff 轮）：0x4A0288 的 Big5 为「錢」（A9 FA），重建写成「金」。已改为 kMoneyLabel = u8"錢"。修正后复核一致（exact 轮）。
- 新增测试 tests/cheat_dialog_test.cpp：CheatDialog::AttributeNames 17 项与 0x49FC14 起逐项比对。

## 0x00411030 Party::SetRoleStat、0x004112B0 Party::RoleStat

角色记录 0x4AB790 起每个 0x38 字节：0..0xD 为 +4 起的 word（读取时 0..5 movsx，6..0xD 零扩展），0xE 为 +0x20 的 dword（经验），0xF 为 +0 的 dword，0x10 为 +0x2C 的字节（等级；设定时再调 0x00477290 LoadLevelEntry(角色 + 1, 等级 + 1)，成功则写经验）；其余号码返回 0。与 RoleRecord 偏移及重建实现一致。exact。

## 补正：0x0040F890 CheatProcedure（字符串截断复查）

WM_INITDIALOG 的第 1..4 页标签为 wsprintfA(0x004A0204 "%s法術", 名字)（注释显示 "%s"），重建只用名字（diff 轮）；改为名字加 "法術"。修正后通过（exact 轮）。
