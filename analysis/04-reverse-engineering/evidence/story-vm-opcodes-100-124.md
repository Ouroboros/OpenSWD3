# 剧情 VM opcode 100–124 汇编语义批次

## 证据口径

本批次逐条回查 `sub_427920` 完整汇编，并下钻角色 GUID/index 转换、角色/地图协调、TALK 窗口转移、Miles stream 包装和文字 action 更新器。**完整汇编是唯一真实依据**；IDA 伪码只作定位和提出待核假设，任何冲突都以汇编为准。

`inventory/story-vm-opcode-semantics-100-124.tsv` 是 25 行逐值规格。生成器读取已锁定的分派表和长度表，并核对完整汇编 SHA-256；当前操作名仍是中性工作名，不替代机器行为。

## 操作摘要

| opcode | 当前中性操作名 | 汇编行为摘要 |
| ---: | --- | --- |
| `100` | 设置角色 Talk 脚本编号 | 找到角色时写 `role+0x1E`，否则走 pending-role helper |
| `101` | 置角色状态 bit26 | 找不到角色时静默消费 |
| `102/103/117` | 按布尔值置/清角色 bit6/5/4 | 清位后按非零参数置位，并刷新两类角色派生状态 |
| `104` | 写文本布局参数并清 bit28 | 两个 `s16` 分别写入 `dword_4CF730/4CF734` |
| `105/121/124` | 清文本控制 bit27/26/25 | 三条独立两字节指令 |
| `106` | 等待 stream 字节严格大于阈值 | 指针为空直接完成；否则 `byte > u16` 才完成 |
| `107` | 等待角色 action 当前下标达到阈值 | 低字节为上限，高字节为当前一基下标 |
| `108` | 设置下一文本坐标 | 超出 `639/479` 的分量分别替换为 16，不是 clamp |
| `109` | 批量执行角色/地图协调 | 变长 selector 表；消费后让出 |
| `110/111` | 按次要角色 bit30 条件转移 | 跳转条件互为反相，角色 0 不参与扫描 |
| `112` | 等待两条 overlay action 链为空 | 成功消费后仍让出，不检查移动 action 链 |
| `113` | 播放音效 | 固定六字节，但 `+4` padding 从未读取 |
| `114` | 提交场景音乐 stream 请求 | 写 pending 值、同步旧 stream，并精确改写控制位 |
| `115` | 设置 stream 100 音量档 | `u16` 只做上限 11 截断，消费后让出 |
| `116` | 批量设置角色位置 | 每项 6 字节；lookup 失败仍把 `-1` 交给位置 helper |
| `118` | 删除指定角色的 dialog 记录 | 删除所有匹配项并维护低 15 位计数 |
| `119` | 等待 dialog bit0 置位或记录消失 | 匹配记录 bit0 为零时才等待 |
| `120` | 选择性更新角色 action 三字段 | 三个 `FFFF` 分别表示保留，成功后置角色 bit12 |
| `122` | 清文字快进开关 | `dword_4CAEB8=0`；同一全局由 C 键切换 |
| `123` | 更新 Scene_Music 表项 | 成功路径只复制 `+2..+7`，`+8` 只供错误诊断 |

TALK 全分支候选图观察到本批次 20/25 个值；未观察的是 `108/110/113/115/124`。已观察值从 opcode 112 的 27 个节点到 opcode 118 的 3458 个节点不等。候选图只证明可解码位置存在，不证明分支或异常在具体游戏状态下实际触发；未观察值仍必须按完整 handler 保留。

## opcode 100、101：角色 Talk 字段与 bit26

100 的 `+2` 是角色 selector，`FFF0` 替换成当前状态块 `+0x24`；`sub_40C0D0` 自带的 `FFFE` 规则仍能直接得到受控角色 index。找到角色后，地址计算严格按 `index * 0xD8`，把 `+4` 的 `u16` 写到 `role+0x1E`。主循环的角色碰撞/交互路径会把该字段复制到 `word_4B72BE`，再据此选择 Talk 脚本，因此这里可确认它是 Talk 文件/脚本编号，而非动作编号。

找不到角色时，100 不丢弃写入意图，而是以大量 `FFFF` 保留值调用 `sub_40D460`，把 `+4` 放进对应 pending-role 字段。指令始终推进六字节并同帧继续；没有脚本编号范围检查。

opcode100现已独立闭环：两个operand在selector替换/lookup前完整staged；live路径只写`talk_script_id`，missing路径提交精确Talk-only MAPS patch，并保持`FFF0` current source、helper-native `FFFE`、+6、previous100与same-call。四raw alias、`FFFF`值、双operand截断和精确尾通过。资产锁192条/192 probes，全部raw`0064`、长度6，分布`49/14/47/82`；Talk范围0..6909，TALK1/2/3/4四条代表记录在missing路径真实回放。完整证据见[`story-vm-role-talk-script-write-0042b3b0.md`](story-vm-role-talk-script-write-0042b3b0.md)。

101 同样处理 `FFF0`，但只在 lookup 成功时执行：

```text
role[index].status |= 0x04000000
```

失败路径不诊断、不调用 pending helper，直接消费四字节。这种差异不能被统一的现代 selector 包装器抹平。

opcode101现已独立闭环：单一selector在lookup前staged，命中时只OR bit26，missing静默，并保持`FFF0` current source、helper-native `FFFE`、+4、previous101与same-call。四raw alias、bit28 skip首匹配、selector截断和精确尾通过。资产锁126条/126 probes，全部raw`0065`、长度4，分布`39/38/11/38`；40种selector范围0..1061，TALK1/2/3/4四条代表记录命中live角色真实回放。完整证据见[`story-vm-role-status-bit26-set-0042b43b.md`](story-vm-role-status-bit26-set-0042b43b.md)。

## opcode 102、103、117：共享布尔状态位 handler

三条指令在 `0x0042C567` 二次分派，只改变掩码：

| opcode | 掩码 | 角色状态位 |
| ---: | ---: | ---: |
| `102` | `0x40` | bit6 |
| `103` | `0x20` | bit5 |
| `117` | `0x10` | bit4 |

成功路径先无条件清所选位，再在 `+4 != 0` 时置回；随后对 `role+0x00` 派生对象依次调用 `sub_40AE20`、`sub_40AEC0`。因此参数不是只接受 0/1 的枚举，而是“零为假、任意非零为真”。

这里的 `FFF0` 规则与大多数角色 opcode 不同：汇编把它替换成 `dword_4AB378` 的受控角色 **index**，然后仍把这个数交给按角色 GUID 搜索的 `sub_40C0D0`，没有读取状态块 `+0x24`。重写必须复现这个看似不协调的两步。

lookup 失败会输出 `(Sar_**)` 诊断，然后构造“清该位/按布尔值置该位”的掩码参数调用 `sub_40D460`。成功和失败都固定推进六字节、置 `ESI=1` 并同帧继续。

共享入口现已按全部八个变体独立闭环：除102/103/117外，内部跳表还映射136/140/145/146/174到`1000/0800/2000/0100/4000`。live路径保留clear flags→任意非零set→clear surface→mark surface；missing路径按布尔提交精确MAPS AND/OR mask。`FFF0`继续按受控index作为GUID key，而非current source或直接controlled role。683条记录/685 probes覆盖六个变体；145/174零记录由asset absence与synthetic锁定。完整证据见[`story-vm-role-status-boolean-flags-0042c567.md`](story-vm-role-status-boolean-flags-0042c567.md)。

## opcode 104、105、108、121、122、124：文字全局状态

104 先执行：

```text
dword_4A1360 &= 0xEFFFFFFF
dword_4CF730 = sign_extend(s16(+2))
dword_4CF734 = sign_extend(s16(+4))
```

后续文字 action 创建会把这两个 dword 原样传给 `sub_40AFF0`。没有坐标、间距或枚举范围门控。

opcode104现已独立闭环：bit28清除先于第一operand，第一signed i16写入先于第二operand；两个原unsafe读取点分别typed-stop并保留此前副作用。成功路径补齐previous104与same-call。四raw alias、i16边界、两级截断、精确尾及125条资产/125 probes通过；完整证据见[`story-vm-text-layout-pair-0042b47e.md`](story-vm-text-layout-pair-0042b47e.md)。

105、121、124 分别清 `dword_4A1360` 的 bit27、bit26、bit25，均为两字节、同帧继续。它们与此前的 bit31..28 控制指令属于独立 opcode，不能合并成会自动正规化整个 flag word 的接口。

opcode105现已独立闭环：只执行u32 `AND F7FFFFFF`、+2、previous105与same-call。四raw alias、精确尾及806条资产/806 probes通过；完整证据见[`story-vm-text-control-bit27-clear-0042b4b9.md`](story-vm-text-control-bit27-clear-0042b4b9.md)。

108 把 `+2/+4` 写入 `dword_4A135C` 的低/高 word，供下一次文字 action 覆盖默认位置。随后分别执行无符号边界检查：

- `X > 639` 时把 X 改成 16；
- `Y > 479` 时把 Y 改成 16。

这不是把值夹到画面边缘。以 `u16` 编码的负数同样大于上界，因此也被替换为 16。当前 TALK 候选图没有观察到 opcode 108，但 handler 明确存在。该入口现已按零资产闭环；分阶段写入、one-shot消费/重置与精确尾见[`story-vm-next-dialog-anchor-0042b5f2.md`](story-vm-next-dialog-anchor-0042b5f2.md)。

122 只写 `dword_4CAEB8=0`。同一全局在输入路径中由按键 `0x43`（C）在 0/1 间切换，并在文字 action 更新器中强制推进字符位置和等待状态，因此当前中性名记为“清文字快进开关”。这条指令不清任何其他文字 flag，消费两字节后同帧继续。

## opcode 106、107、112：三种等待合同

106 从 `unk_4B7BD0+0xA0` 取指针。为空时直接完成；非空时零扩展 `[pointer+0x49]` 的单字节，并与 `+2` 的 `u16` 作无符号比较：

```text
byte <= threshold  -> 不推进，ESI=0，让出
byte >  threshold  -> 推进 4，ESI=1，同帧继续
```

比较是严格大于。指针持续非空时，阈值 255 或更大永远无法由单字节满足。

共享入口现已按106/154独立闭环：154选择副图片动作链，106选择主链；`node+0x49`精确映射为typed action `packed_ap_state`高字节。两个变体各四raw alias、链选择、空链、严格边界、threshold 256、operand/runtime访问顺序与精确尾通过。opcode106锁定60条资产/63 probes；154以asset absence和synthetic锁定。完整证据见[`story-vm-picture-action-byte-wait-0042b4ca.md`](story-vm-picture-action-byte-wait-0042b4ca.md)。

107 找到角色后读取其 action `+0x40` 的 packed word：低字节是 AP 项数量/上限，高字节是一基当前下标。精确分支为：

- `threshold > low_byte`：诊断并消费；
- `threshold <= low_byte && high_byte < threshold`：不推进并让出；
- `high_byte >= threshold`：消费并同帧继续。

角色 lookup 失败也诊断并消费。因此 opcode 107 的等待条件不是简单的 `current < threshold`；非法阈值路径必须先于等待判断。该入口现已独立闭环，完整访问顺序、common join、精确尾与资产锁见[`story-vm-role-action-index-wait-0042b50f.md`](story-vm-role-action-index-wait-0042b50f.md)。

112 只检查 `dword_4BAB9C` framebuffer 区域效果链和 `dword_4BA6E0` 角色头像 action 链。任一非空时不推进；两者都为空时推进两字节。两条路径的 `ESI` 都保持零，所以即便成功也立即让出。`dword_4AD3E8` 移动 action 链不在谓词内。

opcode112现已独立闭环：packed-row链先读且非空时短路第二链，role-head链只在首链为空时读取；等待与完成均发布previous112、service audio并yield，完成不same-call。线性资产锁定9条记录/9 probes，代表记录先等待role-head链、清空后完成，同时保留非空moving-action链。完整证据见[`story-vm-overlay-action-lists-wait-0042b70c.md`](story-vm-overlay-action-lists-wait-0042b70c.md)。

## opcode 109、110、111、116：批量角色处理与条件转移

109 的物理格式是 `u16 count` 后跟 `count` 个 `u16` selector。每项 lookup 成功才调用 `sub_42E280`；该 helper 协调角色与地图对象，并按关联结果设置或清理角色 bit30 等状态。失败项静默跳过，helper 返回值全部忽略。handler 自身不把 `FFF0` 换成当前状态，但 `sub_40C0D0` 的 `FFFE` 特例仍存在。

处理完成后，真实脚本指针按完整 `4 + 2*count` 移动，状态块中的 16 位 IP 只加该长度低 16 位；`ESI` 不置一，因此消费后让出。`count=0` 合法。大 count 会让 16 位 IP 与完整指针产生原始回绕差异。

opcode109现已独立闭环：逐项selector读取、lookup、游标推进和helper调用顺序，post-loop count重读、previous/audio/yield、分阶段失败及真实count1/count18记录均已锁定。完整证据见[`story-vm-role-step-list-0042b63c.md`](story-vm-role-step-list-0042b63c.md)。

110/111 从角色 index 1 开始扫描到 `dword_49E0C4-1`，只测试 `role+0x10` bit30；index 0 永远跳过。随后按 `+2` 的 `u32` TALK 目标决定顺序推进或调用 `sub_42E430` 转移窗口：

| opcode | 转移条件 | 顺序消费条件 |
| ---: | --- | --- |
| `110` | 没有次要角色带 bit30 | 至少一个带 bit30 |
| `111` | 至少一个次要角色带 bit30 | 一个都没有 |

角色总数不大于一时，110 必然转移，111 必然顺序消费。转移后当前指针改成 `dword_4B8860`，不是简单地把同一缓冲区 IP 改为绝对偏移。

共享入口现已独立闭环：角色1起的bit30首命中扫描、两opcode互反条件、只在转移分支读取u32 target、same-file窗口重载、previous与same-call均已锁定。opcode110零资产；opcode111有24条记录/24 probes，代表记录同时回放顺序进入opcode67及重载到count18 opcode109。完整证据见[`story-vm-secondary-role-bit30-reload-0042b6a5.md`](story-vm-secondary-role-bit30-reload-0042b6a5.md)。

116 的每项为 `{selector, X, Y}`。`FFF0` 在 handler 中替换成状态块 `+0x24`；lookup 的成功返回没有被测试，输出 index 无论是否为 `-1` 都交给：

```text
sub_42DAF0(index, (X << 4) & 0xFFFF, (Y << 4) & 0xFFFF,
            0, -1, -1, -1)
```

若输出 index 等于 `dword_4AB378`，再置 `dword_4A9920` bit15。X/Y 的移位在 16 位寄存器中进行，溢出直接回绕。与 109 一样，物理指针按完整 `4 + 6*count` 前进，状态 IP 只加低 16 位；但 116 最终置 `ESI=1`，允许同帧继续。

opcode116现已独立闭环：锁定入口count冻结、末尾count重读、selector→lookup→Y→X→位置owner→受控比较顺序、missing index原危险点、16位坐标和same-call合同。30条真实记录/30 probes、117个子记录及代表count1回放通过；完整证据见[`story-vm-batch-role-position-0042b83a.md`](story-vm-batch-role-position-0042b83a.md)。

## opcode 113–115：音效与场景音乐 stream

113 只读取 `+2` 的音效 ID，与全局 `dword_4AB784` 一起调用 `sub_485610`。wrapper 把全局档位缩放后提交给 Miles sample 播放层。指令却固定推进六字节：`+4` 的 word 从未读取。播放结果不观察，消费后让出。

opcode113现已独立闭环：只要求opcode与sound word可读，固定消费未读padding；复用已审计sample wrapper，保持play→IP+6→previous113→正常common audio→yield。线性资产零记录/零probes，55处raw字样均非证明入口，以asset absence及四alias、operand截断、未读padding尾和精确尾synthetic锁定。完整证据见[`story-vm-sound-effect-unread-padding-0042b723.md`](story-vm-sound-effect-unread-padding-0042b723.md)。

114 写入 pending stream 状态：

```text
dword_4B7C80 = 0x80000001
dword_4B7C84 = u16(+2)
dword_4B7C88 = u16(+4)
```

若 `dword_4B7380` 为零先改成一，再调用 `sub_485880` 同步当前 stream。随后在 `dword_4ACDBC` 中置 bit23、清 bit16/17，并按 `+6` 的高位重新派生：

- bit15 置位：不设置 bit16/17；
- 否则 bit14 置位：OR `0x30000`；
- 否则/同时 bit13 置位：OR `0x20000`。

公共尾部的 `and al,0` 会清整个 dword 的低八位，而不是只清一个局部 flag。其余上 24 位保留。指令推进八字节并同帧继续。

opcode114现已独立闭环：恢复request→双ID→既有transition同步→control bit23→flags派生的分阶段顺序，并把`0x004B7378` current fade divisor接入实际stream manager端口；成功发布previous114且无audio、same-call继续。157条真实记录/159 probes、四alias、三类资产flags、三阶段截断和精确尾均锁定。完整证据见[`story-vm-scene-music-stream-request-0042b739.md`](story-vm-scene-music-stream-request-0042b739.md)。

115 将零扩展的 `u16` level 与 11 比较，超过 11 就强制写成 11。后面的“若小于零则置零”分支在该数据流上不可达。`sub_485850` 把 0..11 档缩放后，为 Miles stream 编号 100 调用 `AIL_set_stream_volume`；stream 不存在等错误返回全部忽略。指令推进四字节后让出。

opcode115现已独立闭环：新增窄port复用实际stream manager，锁定u16零扩展、上限11、不可达负夹分支、wrapper返回忽略、previous/audio/yield及精确尾。线性资产为零记录/零probes，109处raw字样均非证明入口，以asset absence和四alias synthetic锁定。完整证据见[`story-vm-music-stream-volume-0042b7fc.md`](story-vm-music-stream-volume-0042b7fc.md)。

## opcode 118、119、120：dialog 链与角色 action

118 的参数是角色 GUID selector，`FFF0` 换成当前状态块 `+0x24`。它遍历 `dword_4ACF48` dialog 链，把记录 `+0x16` 的角色 index 交给 `sub_40C060` 取 GUID，再与参数比较。每个匹配项都会：

1. 通过平行 predecessor 链重接 `+0x48` next；
2. 非 `FFFD` 记录按原始 `record+0x16` 直接定位角色并清 `role+0x26`；
3. 释放记录 `+0x38`、`+0x44` 两个指针和记录本体；
4. 把 `dword_4A9920` 低 15 位减一并在负数时夹到零，bit15 原样保留；
5. 继续删除后续所有匹配项。

opcode118现已独立闭环：锁定selector局部`FFF0`替换、raw role index→GUID匹配、predecessor保持、解链→gate→text/caption/node释放→counter顺序、低15位零夹与高位清除、无效index分阶段停止及same-call合同。1669条真实记录/1669 probes和TALK1连续`0→1`记录回放通过；完整证据见[`story-vm-dialog-role-remove-0042b8e6.md`](story-vm-dialog-role-remove-0042b8e6.md)。

119 接受普通角色 selector，也接受特殊值 `FFFD`。普通值经 `sub_40C0D0` 解析成 index 后，与第一条匹配 dialog 记录的 `+0x16` 比较。对 opcode 119，汇编的真实方向是：

```text
找到记录且 record+0x08 bit0 == 0 -> 等待，不推进
bit0 == 1                         -> 消费
没有记录或角色 lookup 失败       -> 消费
```

`FFFD` 绕过 lookup，直接匹配特殊记录。若理论上有多个匹配项，只有扫描到的第一项决定等待，不继续检查其余项。共享opcode139执行相同selector和首匹配扫描，但等待位改为`record+0x08` bit15；bit15清零等待、置位完成。

共享入口现已独立闭环：两opcode均锁定四raw alias、`FFF0→FFFD`判断顺序、`FFFE`完整u32 index、lookup失败/空链/miss消费、首匹配优先、各自bit极性、wait audio/yield与完成same-call。119/139共850条真实记录/850 probes及两条variant回放通过；完整证据见[`story-vm-dialog-flag-wait-0042b9c2.md`](story-vm-dialog-flag-wait-0042b9c2.md)。

120 的三个可选字段分别写 action `+0x00/+0x08/+0x34`。每个原始 word 等于 `FFFF` 时保留旧值；前两个非 sentinel 值按 `s16` 符号扩展，第三个按 `u16` 零扩展。随后清 action `+0x44`、调用刷新 helper，并置 `role+0x10` bit12。刷新失败只诊断，不撤销已写字段。角色不存在时改走 `sub_40D460`，仍消费十字节。

真实首图会稳定触发角色不存在分支：初始运行时角色表有 33 项但不含 GUID `123/240`，`TALK100` 在首段剧情视频边界前依次提交 `123,561,8,0` 和 `240,561,0,1` 两个 source patch，二者均以 `0x1000` 作为 OR mask，其他 mask/map 字段保持 `FFFF`。真实初始世界回归固定验证角色缺失、MAPS source 存在、两组原始操作数和继续执行结果，防止人工补齐角色的测试夹具再次掩盖此分支。

## opcode 123：Scene_Music 表项的非对称复制

123 从 `dword_4C9A10` 的相对偏移链取得 8 字节一项、以 key 零结束的 Scene_Music 表。`+2` 为查找 key；若是 `FFF0`，只在比较时以 `ArgList` 低 16 位替代。找到匹配项后实际复制：

```text
entry[0..3] = raw dword at instruction +2
entry[4..5] = raw word  at instruction +6
entry[6..7] = 保持不变
```

因此 `+8` 在成功路径完全未读；找不到 key 时，错误诊断才读取并打印 `+2/+4/+6/+8` 四个 word。`FFF0` 也只影响查找：成功复制回表的 dword 仍以字面值 `FFF0` 开头。handler 不追加新条目，不改终止项，最后固定推进十字节。

## 1:1 还原约束

- `119` 必须保留“bit0 为零等待、bit0 置位完成”的真实方向。
- `109/116` 必须区分完整内存指针推进与 16 位 IP 的低位回绕。
- `102/103/117` 的 `FFF0 -> controlled index -> GUID lookup` 两步不得改成普通当前角色替换。
- `112/113/115` 的成功/消费路径仍会让出；不能因操作已经完成就自动同帧执行下一条。
- `113` 的未读 `+4` 和 `123` 成功路径未读 `+8` 都是物理格式的一部分。
- lookup 失败后的 `-1` 使用、静默消费、pending helper、诊断消费四类行为必须逐 opcode 保留。
- 当前阶段只记录这些原始合同，不修复游戏逻辑 BUG；未来只有启动或新系统兼容所必需的最小改动可以隔离在平台层。
