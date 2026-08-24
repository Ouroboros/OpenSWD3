# 标准模式数据库绘制 `0x0043E800`

状态：`platform_adapted`、`unit_tested`

## 1. LST范围与边界

唯一行为真值为`swd3.exe.lst`。函数范围`0x0043E800..0x0043EFFE`，910行、29个基本块；无direct caller，由B480和后续标准模式callback表绑定。

完整审计纳入2次4239D0、4次40DC50、5次40EBF0、1次AE40、1次B110、5次BAB0、8次436AD0、4次FA70、1次400A0、2次405C0及resource resolve/blit路径。已关闭AE40和FA70均直接复用typed实现；400A0/405C0等未关闭callee继续保持最小port边界，不提前计数。

新增数据库render typed API。固定文本、索引文本、物品查询、resource resolve及绘制操作均为显式port；操作请求保留坐标、尺寸、flags、比例和文本。资源解析失败在原`[eax]`解引用点typed-stop，之前所有绘制副作用保留。

## 2. owner映射

- `4FCAD4`→`hover_flag`；`4FCADC`→`page_selection`；`4FCAB8`→`direction_selection`。
- `4FCAA4`→`display_flags`；`4FCAD0/4FCBA4/4FCB98/4FCAD8/4FCD10`→window/list/visible/total/current list owner。
- `4FCA88/4FCBA0`→两份runtime record；`4FCBAC/4FCC5C`与其后0xB0字节→missing index及两份inline record。
- `4FCAAC`→`runtime_input_flags`；`4FCA8C`→`interaction_toggle`；`4FCAB4`→`phase_3_countdown`。
- `4FC320`→新增`animation_offset`；`4FB970/4FB978`→`primary_action`。
- forward node补充typed display name；旧导航、数值及release字段不变。

## 3. 公共与phase1/5路径

入口严格生成两色并查询物品`0x1BB0`。存在时按`len*11`绘制`118h/DC`提示并早退。

非早退时：

1. hover为1初始化`232C/0F`动作。
2. phase1/5初始化page动作：variant=`page+14h`、X=`31*page+8`、Y固定`34h`；direction不大于1时初始化variant=`direction+10h`、X=`8*direction+21h`、Y=`216*direction+D0h`。
3. total大于16时，display低/高nibble分别减1并生成overlay bit0/bit1；AE40比例严格为`(offset+visible)/total`与`offset/total`。
4. 从current head开始，Y=`4Fh`每项加`18h`，严格在`1CFh`停止；格式`%-12s%3d`，selection相等时发B110 marker。
5. 两份record以X=`F8h/1E4h`绘制inline名称、数值和附加文本；可选动作X=`108h/1DCh`。detail gated路径使用runtime `+60h`与两inline `+5Ch`均值，资源ID为`2465h/2463h`，blit X=`139h/25Bh`、Y=`1A9h`。

UT覆盖两个display nibble衰减、overlay=3、浮点比例、list marker、所有精确动作坐标、双record格式、物品gate、两个threshold资源和resource typed-stop。

## 4. phase2–5

- phase2/10：第一record受物品`1BA9`门控；两个FA70 flags严格由runtime bit0/bit1移到bit12并叠加toggle相等位，现直接展开东西方祭坛背景、动作、十行文本、禁用矩形及等级不足警告，随后绘制公共panel/text。
- phase3 countdown不大于-35：绘制同组panel，`animation_offset=(-40-countdown)*6`；等于-35时写primary action`232A/46`，再按u32加1返回。
- phase3 countdown大于-35：写`232A/46`并清offset；不大于-30时写`(countdown*3+90)*2`，调用400A0后加1；新值大于140时写200并调用405C0。
- phase4：按toggle选择runtime record `+5Ch` action ID，以variant44、X104h、YB4h初始化。
- phase5：完成前述公共phase1/5绘制后，按`len*12`在X=`140h-width/2`、Y=`E4h`绘制返回提示。
- 其他phase保留interaction phase EAX。

UT覆盖phase2 flags、phase3 -35/-34/140三个边界、phase4 record选择和phase5居中提示。

## 5. 验证

定向`special_modes.legacy_initial_menu`通过。workpack双生成稳定为`57/227`，SHA256均为`2515ab3d64a6961f4ae9389efd852d47b363e4e4818311b788e3de569669cffe`；下一单元为`0x0043F000`。Linux core完整门`188/188`、Linux app完整门`194/194`通过；按阶段门禁未运行Windows BUILD。
