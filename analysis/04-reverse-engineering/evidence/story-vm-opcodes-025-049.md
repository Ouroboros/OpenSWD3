# 剧情 VM opcode 25–49 汇编语义批次

## 证据口径

本批次逐条回查 `sub_427920` 完整汇编以及只为解释直接调用所必需的 helper。汇编是唯一真实依据；IDA 伪码只作定位。操作名保持业务中性，字段名只使用已由既有结构证据或本批次数据流证明的含义。

`inventory/story-vm-opcode-semantics-025-049.tsv` 是 25 行逐值规格。生成器读取已锁定的一级分派与长度表，并锁定完整汇编 SHA-256，避免人工语义与入口或物理宽度脱节。

## 操作摘要

| opcode | 当前中性操作名 | 汇编行为摘要 |
| ---: | --- | --- |
| `25/26` | 设置/清除全局 bit | 对 `byte_4AB384` bitset 操作，均输出诊断并同帧继续 |
| `27` | 重置并装入地图/session | 六个 `u16` 参数进入 `sub_42E790`；后三个允许 `FFFF` 继承受控角色动作字段 |
| `28` | 修改角色 Path id | 清理旧 Path 动态载荷、协调地图对象，推进后跨帧让出 |
| `29/30/31` | 设置/加/减通用整数 | `s16` 下标和数值；减法按符号位归零，三者还有共享的第零项归零尾部 |
| `32/33` | 通用整数条件跳转 | 阈值先符号扩展，再按 32 位无符号关系比较 |
| `34..37` | 有界计数器与快照 | 设值、两个条件跳转、复制快照；34 超过 1000 时归零 |
| `38/39` | 清/置角色状态范围 | 38 执行 `& 0x00007FFF`，39 对状态 OR `0x8000` 并清两个 pending 字段 |
| `40` | 重定位角色并完成路径 | tile 坐标按 u16 左移四位，依次调用调度与完成 helper，再清 bit31 |
| `41` | 按共享 selector 重载目标 | `u32` 表以 `FF00FF00` 结束；`>` 回退 0，`==count` 会选择 sentinel |
| `42/43` | 设置/清除共享 interaction lock | 42 还把受控角色 action `+0x08` 归零并尝试刷新；43 只清位 |
| `44` | 修改 action `+0x48` | 原诊断名 `ChangSpd`；同时把 action `+0x44` 归零 |
| `45` | 修改请求动作 id | 写 action `+0x00`；若下一条仍操作同一角色可推迟刷新 |
| `46..49` | 恢复 pending action 字段 | 全部恢复、单独恢复 `+0x20`、单独恢复 `+0x3C`、写 `+0x48=FFFF` |

候选控制流图在当前四个 TALK 资产中观察到本批次 16/25 个值。未观察到 `31`、`34..37`、`46..49`；未出现不代表可以删除，完整汇编已经给出明确 handler。

## opcode 27：世界 session 同步重建边界

物理布局固定为 14 字节：

```text
+0   u16 opcode
+2   u16 logical map id
+4   u16 tile x
+6   u16 tile y
+8   u16 requested action id
+10  u16 requested base variant
+12  u16 requested variant delta
```

handler 在 `0x004286C5..0x00428712` 依机器顺序读取六个 `u16`，并传 literal `1` 给 `sub_42E790`。最后一条机器指令是从 `0x0042870E` 开始的近跳；下一 handler 严格从 `0x00428713` 开始，静态提取器不能把 `0x0042870E` 误作边界。

`sub_42E790` 按固定顺序清 `dword_4B7920`、`0x200` 字节工作区、`dword_4B7518/dword_4A948C/dword_4A9488`，再设置 process bit0。后三项若为 `0xFFFF`，分别取当前受控角色内嵌 action `+0x00/+0x08/+0x34` 的低 16 位；前三项没有继承规则。helper 同步调用 `sub_40C130`，返回前只清 process bit0。

SDL typed owner 保留 `progress -> 旧role preload -> 不可逆owner teardown -> runtime load -> VM span/runtime重绑`。literal bit0 已由显式 preload 消费，runtime loader 使用清位后的 flags，避免重复同步。`dword_4C8BE0` 由持久 VM state 提供，`dword_4A9940` 的 item `0x0192` 查询由 player inventory typed owner 提供。pending-session 双缓冲使下一条 opcode 在同一次 VM 调用中观察新 world，并在旧外层引用离开作用域后再提交；teardown 后的 checked failure 会丢弃失效 session 并停止 frame，不能继续索引已清空 roles。

handler 推进 14 字节、设置 `ESI=1` 并同调用继续。647 条真实物理记录全部 raw `0x001B`、长度 14；唯一全三项继承记录为 `TALK3.DAT@0x00016095`。完整独立证据见 [`story-vm-world-session-reload-004286c5.md`](story-vm-world-session-reload-004286c5.md)。

## opcode 28：消费后让出，而不是同帧继续

角色存在时，handler 先释放角色 `+0x38` 拥有的旧 Path 载荷并清 `+0x34/+0x38`，再扫描固定 72 个地图对象槽：

- type 2 对象只把 `+0x08/+0x0A/+0x0C/+0x0E` 四个 `u16` 写成 `FFFF`；
- type 1 对象在角色坐标未按 16 像素对齐时，按方向步长执行反向减法直到整格，清旧 surface occupancy，并从旧 `(world_y >> 4) - 1` 开始空间重插；
- type 1 匹配对象最终交给 `sub_40DD40` 重置，其他 type 不重置。

之后写 `role+0x1C=path_id`、`role+0x18=0`，并对 `role+0x10` OR `0x1000`。角色不存在时走 `sub_40D460` fallback，只 patch MAPS source 的 Path id 与 `0x1000` flag。

两条路径都汇合到 `0x0042BEE9`，只推进六字节，没有设置 `ESI=1`。common join 发布 previous opcode 后进入 `_AIL_serve` 并返回；因此该指令消费后跨帧让出，下一条要到后续帧执行。这里不能照相邻大多数角色操作统一成同帧继续。

45 条真实记录全部 raw `0x001C`、长度 6，分布为 TALK1/2/3/4=`7/27/2/9`。完整 staged-read、对象槽、反向对齐、重插、typed owner 与测试证据见 [`story-vm-role-path-id-change-00428713.md`](story-vm-role-path-id-change-00428713.md)。

## opcode 29–33：带原始越界行为的 64 项整数区

五个 opcode 共用 `0x0042B074`。下标和阈值/数值都以 `movsx` 从 `s16` 读取；门控却只有：

```text
cmp index, 64
jl  dispatch
```

所以真实边界是：

- `0..63` 正常访问 `dword_4ACBD0[index]`；
- 负下标同样通过 `jl`，在数组基址之前读写；
- `index >= 64` 输出诊断，不推进 IP，且 `ESI=0`，随后跨帧让出并重复同一条指令。

操作 29 直接赋值，30 使用 32 位回绕加法，31 使用 32 位减法并在结果符号位置位时把选中项写零。所有正常分派还经过共享尾部：若 `dword_4ACBD0[0]` 的符号位置位，就把第零项写零。这意味着 29/30 操作非零下标时仍可能附带修改第零项。

32/33 的阈值虽由 `s16` 符号扩展，实际条件跳转使用 `jb/ja`，即比较两个 32 位无符号 bit pattern：

- 32 在 `value >= unsigned(sign_extend(threshold))` 时跳转；
- 33 在 `value <= unsigned(sign_extend(threshold))` 时跳转。

不能在重写中把比较简化为有符号关系。机器负下标会访问数组前内存；typed owner 在真实资产零命中的前提下，必须保留 32/33 先读 target 的顺序，并只在首次数组越界访问点转为显式 checked failure。`index>=64` 则不是 unsafe adaptation，仍严格保留不推进、发布 previous opcode、audio service 与 yield/retry。44 条真实记录分布为 29/30/31/32/33=`9/22/0/8/5`，index 仅 `0,2,41,50,62`。完整证据见 [`story-vm-global-integers-0042b074.md`](story-vm-global-integers-0042b074.md)。

## opcode 34–37：有界值与快照

opcode 34 将零扩展的 `u16` 写入 `dword_4ACDB0`；若值大于 1000，则写零而不是写 1000。opcode 37 把当前值复制到 `dword_4BA42C`。

opcode 35 只读取 `+2` 的一个字节，并与 `dword_4ACDB0` 低 16 位作无符号比较；`+3` 的一个物理字节未读，目标位于 `+4`。opcode 36 计算 `dword_4BA42C + u16(+2)` 的 32 位回绕和，当前值严格大于该无符号和时跳转。

这四个值当前 TALK 候选图都未观察到，但不能因此将 opcode 35 的未读 padding 删除或把指令缩成七字节。opcode34的线性记录与entry probe均为0；四个TALK文件里501处raw `0x0022`字节序列都不是已证明的入口，不能伪造为真实回放。其`u16`零扩展、先写后清零、共享clock owner及same-call合同的完整独立证据见 [`story-vm-script-clock-set-0042890f.md`](story-vm-script-clock-set-0042890f.md)。opcode35同样为0条记录、0个entry probe；其四种raw word共1145处字节候选全非已证明入口。clock低16位、`u8(+2)`、未读`+3`、branch-only target和typed同文件加载证据见 [`story-vm-script-clock-byte-jump-00428934.md`](story-vm-script-clock-byte-jump-00428934.md)。opcode36也为0条记录、0个entry probe；其snapshot+delta按u32回绕、完整clock严格`>`、branch-only target，以及taken重载后仍从新窗口offset8继续的特殊尾部见 [`story-vm-script-clock-origin-delta-jump-0042896c.md`](story-vm-script-clock-origin-delta-jump-0042896c.md)。opcode37同样没有记录或probe；其完整32位clock快照、仅两字节物理长度及窗口尾副作用顺序见 [`story-vm-script-clock-snapshot-004289be.md`](story-vm-script-clock-snapshot-004289be.md)。

## opcode 38–40：相似角色入口并不共享 selector 规则

38 和 39 在 lookup 前把 `0xFFF0` 替换为 `state+0x24`。38 对角色状态执行完整 `& 0x00007FFF`，会清除 bit15 及所有更高位；39 只 OR `0x8000`，然后把角色 `+0x60/+0x7C` 写成 `-1`。

40 没有 `0xFFF0` 替换。它把两个 `u16` 分量左移四位后传给 `sub_42DAF0`，调用 `sub_42D920`，再清角色状态 bit31。只有原始 selector 恰好等于 `state+0x24` 时，才额外把角色 `+0x4C/+0x78` 写成 `-1`。三个 handler 的 selector 规则不能因参数形状相似而共用一个前处理器。

opcode38的独立闭环还证明：live path先掩码并清surface，再按GUID重新取得第一个skip bit已清的完整u32 role index，固定扫描72个object槽并把所有匹配槽完整fill `0xFF`；ordinary miss则用原始`+2`selector向MAPS发送flags `AND 0x7FFF`/`OR 0` patch。786条真实记录、790个entry probe、FFF0/raw fallback及`TALK1.DAT@0x00004656`回放见 [`story-vm-role-scene-clear-004289de.md`](story-vm-role-scene-clear-004289de.md)。

opcode39则不做GUID重查或object扫描：live path先对完整flags OR `0x8000`，surface clear返回后才把role `+0x60/+0x7C`两个one-shot dword写成`0xFFFFFFFF`；ordinary miss使用raw selector向MAPS发送flags `OR 0x8000`/`AND 0xFFFF` patch。553条真实记录、558个entry probe及`TALK1.DAT@0x000049F0`回放见 [`story-vm-role-flag-8000-00428adc.md`](story-vm-role-flag-8000-00428adc.md)。

## opcode 41：必须保留的 selector 等于 count 边界

handler 从 `+2` 开始无边界扫描 `u32` 目标，直到 `0xFF00FF00`，得到实际目标数量 `count`。随后执行的是：

```text
cmp selector, count
jbe use_selector
selector = 0
```

所以：

- `selector < count` 选择正常目标；
- `selector > count` 诊断并回退到目标零；
- `selector == count` 把终止值本身当成目标，重载到 `0xFF00FF00`。

全分支候选图里的 193 条非入队/文件外边全部来自这一相等边。没有证据表明正常游戏状态必然触发它，但 1:1 初步还原不得把 `jbe` 修成严格小于。

## opcodes 42/43：dialog counter 与 interaction lock 是同一 owner

共享入口先比较 effective opcode 42；相等时按完整 u32 对 `dword_4A9920` OR `0x00008000`，再把受控角色 action `+0x08` 的完整 dword 写零并调用一次 `sub_4321E0`。refresh 返回零只进入 `nullsub_1` 的 `Act Err(Talk:Rmlock)` 诊断，然后仍按两字节推进并同调用继续；已经写入的 lock 与 base variant 不回滚。

不等于42的共享分支即opcode43，只对完整 u32 AND `0xFFFF7FFF`，不访问角色且不刷新 action。两者各自都在窗口`0x7FFE`完整记录上先完成副作用、推进和previous发布，下一次fetch才失败。

`dword_4A9920`低15位同时承载dialog counter，bit15承载世界交互锁。原版map-event赋值、鼠标方向门、世界移动门、dialog生命周期及42/43全都读写同一进程全局。SDL runtime现以`world_dialogs_.close.flagged_dialog_counter`为canonical：VM直接使用，interaction map-event写/方向门借用同一指针，移动门也读取同一字段；模块单元测试才使用局部fallback，不维护镜像副本。

资产中opcode42/43分别有84/62条记录和84/62个entry probes，全部为两字节raw `0x002A/0x002B`；文件分布分别为68/15/1/0与52/8/2/0。真实回放及typed session/action port边界见 [`story-vm-interaction-lock-00428d18.md`](story-vm-interaction-lock-00428d18.md)。

## opcode 44：两种特殊 selector 与 staged unsafe 顺序

opcode44先把raw `0xFFF0`替换为context source GUID，但不自修改脚本；随后`sub_40C0D0`仍把`0xFFFE`作为独立特殊值，直接返回受控角色index。ordinary selector按u16 GUID扫描，跳过bit28置位角色并取第一个clear匹配。

lookup返回值被忽略；miss输出`0xFFFFFFFF`。机器仍先读`u16(+4)`，然后才第一次访问越界的`action[-1]`。modern checked边界因此必须保留“selector→lookup→value→action”顺序：missing且value截断先报operand越界，完整value的missing才在action访问点停下。

live path依次把`u16(+4)`写入action `+0x48` wait override、把action `+0x44` wait remaining写零，再调用一次`sub_4321E0`；refresh零返回只诊断，六字节推进与same-call continuation不变。8条真实记录及FFF0/FFFE、bit28、missing、窗口尾的完整证据见 [`story-vm-role-action-wait-override-00428db8.md`](story-vm-role-action-wait-override-00428db8.md)。

## opcode 46–49：查找失败后仍计算数组前地址

`sub_40C0D0` 会先清输出；普通 selector 继续调用 `sub_40C100`。角色不存在时，后者把 lookup 返回的 `0xFFFFFFFF` 写入输出并返回零。

opcode 46、47、48、49 都忽略这个布尔返回值，直接用输出做：

```text
index * 0xD8 + role_array_base + 0x40
```

因此缺失角色会形成角色数组之前的 action 指针并继续读写。46–49 也不把 `0xFFF0` 替换为当前角色。这个异常不能在兼容核心中改成“角色不存在则跳过”；若未来为了现代系统内存安全隔离，必须作为显式兼容例外记录并证明正常资产/状态不触发，而不是静默改语义。

## opcode 45：向后看一条的刷新合并

current selector `0xFFF0`只在本条handler内替换为context source GUID；`0xFFFE`仍由`sub_40C0D0`直接选择受控index。角色存在时，45把`u16(+4)`零扩展并完整写入u32 action `+0x00`，再对完整u32角色flags OR `0x1000`。写零会输出诊断，但零仍然落入字段。

在refresh和flags写之前，`sub_42E740`强制读取尚未消费的下一raw opcode。只有精确`0x000A/0x000B/0x002D`才继续读取下一selector；alias不合并，next `0xFFF0`也不执行本条handler的替换。下一selector解析到同一个role index时跳过`sub_4321E0`，否则立即refresh。缺next opcode或recognized-next selector时保留已完成的action-id写，但不refresh、不置flags、不推进当前IP或发布previous。

missing live role不执行lookahead，而由`sub_40D460`只patch MAPS action id并OR flags `0x1000`，随后正常推进。65条真实记录、9次资产same-role合并、完整窗口尾顺序与typed边界见 [`story-vm-role-action-id-00428e52.md`](story-vm-role-action-id-00428e52.md)。

## opcode 46–49 的 action pending 协议

四条共享4-byte handler，selector不执行`0xFFF0` context替换；`0xFFF0`是ordinary字面GUID，`0xFFFE`仍由helper选择受控index。ordinary miss的lookup返回被忽略并形成`action[-1]`，modern在各分支首次unsafe action访问点checked-stop，不增加MAPS fallback。

- 46无条件把`+0x1C/+0x20/+0x3C`的完整u32复制到`+0x00/+0x08/+0x34`，即使pending值是`0xFFFFFFFF`也照常覆盖；随后精确复用`sub_40DC00`，把三个pending dword写回`FFFFFFFF`，清u16 wait override/default/remaining、u16 command cursor与u32 external mode。
- 47只在`+0x20 != FFFFFFFF`时复制完整u32到`+0x08`并清pending。
- 48只在`+0x3C != FFFFFFFF`时复制完整u32到`+0x34`并清pending。
- 49只对`+0x48`的低16位比较和写入`FFFF`，不改相邻wait字段。

四者无论条件写是否发生，都恰好尝试一次`sub_4321E0(action)`；失败仅诊断，随后推进4字节、发布previous并同调用继续。锁定TALK目录对四条均为0条物理记录/0个entry probe，因此使用`asset_absence_verified`且不伪造real replay。完整证据见 [`story-vm-role-action-override-restore-00428f7b.md`](story-vm-role-action-override-restore-00428f7b.md)。

## 产物与下一批

- `inventory/story-vm-opcode-semantics-025-049.tsv`：本批次 25 行人工汇编语义。
- `tools/build_story_vm_opcode_semantics_025_049.py`：汇编哈希锁定生成器。

下一批从 opcode 50 开始，继续按相同字段记录参数、直接/间接状态效果、IP、同帧继续、跨帧让出及原始异常；不会把 CFG 底稿或字符串名称直接冒充最终业务语义。
