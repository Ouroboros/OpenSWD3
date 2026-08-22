# 剧情 VM 低 14 位 opcode 与一级分派表

## 证据边界

本证据块只回答 `sub_427920` 如何从当前命令字选择第一个处理入口，以及未映射值如何处理。汇编是唯一真实依据；IDA 的 switch 注释只用于核对，不参与生成结论。

机器目录由 `tools/build_story_vm_dispatch_inventory.py` 只从完整 LST 机械抽取。生成器要求：

- `swd3.exe.lst` SHA-256 为 `701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`；
- `0x0042D4F4` 的 98 个 LST `dd offset` 逐项解析到同一 LST label；
- `0x0042D67C` 的 94 个 LST `dd offset` 逐项解析到同一 LST label；
- 每个可见表行的首 dword 与该行 little-endian 机器字节一致；
- 两个共享入口内部使用的 6/9 项跳表和 157/73 字节 selector 表均从 LST byte column 重建。

生成器不再读取已失效 hash 的 `swd3.exe.asm`，也不以 PE 作为第二行为来源。重跑后 opcode dispatch、entry groups 和 internal switches 三表与旧基线逐字节相同，证明 192 个表项在切换到唯一 LST 真值后没有漂移。

## 取指与 opcode 域

当前命令指针在 `EBX`。`0x00427B40` 读取 `word [ebx]`，随后：

```text
raw_word = u16([EBX])
effective_opcode = raw_word & 0x3FFF
```

关键指令是 `0x00427B4F: and esi,3FFFh`。所以一级分派只有 `0..16383` 的 14 位域；对一级路线而言，每个有效 opcode 都有四个原始命令字别名：

```text
opcode | 0x0000
opcode | 0x4000
opcode | 0x8000
opcode | 0xC000
```

这只证明高两位不参与一级入口选择。具体 handler 是否重新读取命令或操作数、是否把其他字的高位当 flags，必须在逐 opcode 语义阶段分别确认，不能把“四个入口别名”扩大成“四条指令完整语义永远相同”。

## 完整一级分派边界

汇编形成两张表和五个表外专用值：

| 有效 opcode | 一级路线 | 证据 |
| --- | --- | --- |
| `0` | 默认错误入口 `0x0042D230` | `(opcode-1)` 无符号越界后 `ja` |
| `1..98` | `jpt_427B88[opcode-1]` | `0x00427B7C–0x00427B88`，98 项 |
| `99` | `0x0042AD75` | `cmp edx,63h` 后 `jz` |
| `100..193` | `jpt_42ADB0[opcode-100]` | `0x0042ADA4–0x0042ADB0`，94 项 |
| `194..1023` | 默认错误入口 | 次表索引大于 `0x5D` |
| `1024` | `0x0042D200` | `cmp edx,400h` 后 `jz` |
| `1025` | `0x0042D49F` | `sub edx,401h` 后为零 |
| `1026` | `0x0042D1EA` | 再 `dec edx` 后为零 |
| `1027..16382` | 默认错误入口 | 后续比较均不相等 |
| `16383` | `0x0042D24E` | 再减 `0x3BFD` 后为零 |

最后一个边界由原算术顺序直接得到：进入 `0x0042D219` 时先减 `0x401`，再减一，最后减 `0x3BFD`，合计正好是 `0x3FFF`。由于取指前已与 `0x3FFF`，不存在更大的有效值。

两张表共覆盖 192 个 opcode；加入 `99` 和四个高值专用入口后，非默认显式值为 197 个。剩余 16,187 个有效 opcode 全部进入同一个默认入口。`inventory/story-vm-opcode-dispatch.tsv` 还单列了 opcode 0，因此共有 198 行显式记录，而没有把两个巨大默认范围展开成上万行。

## 表外专用入口的首段行为

这些结论只描述进入后的首个共同控制效果，不代替后续完整语义表：

- `99`：`0x0042AD75` 读取无符号 `word [ebx+2]`，比较全局 `0x004B7AC8`。当全局值小于等于操作数时不推进指针并让出；大于时跳到 `0x0042D182`，推进四字节并继续解释。
- `1024`：`0x0042D200` 推进两字节，设置局部 continue 标志，在本次 `sub_427920` 调用内继续取指。
- `1025`：`0x0042D49F` 推进两字节，清除两个 continue 来源，经 `_AIL_serve` 后返回一。
- `1026`：`0x0042D1EA` 推进两字节，设置 `ESI=1`，在本次调用内继续取指。
- `16383`：`0x0042D24E` 进入带 `TalkEnd` 诊断字符串的大型结束清理路径；角色、剧情和全局状态副作用留给逐 opcode 语义恢复。

这些差异说明 `1024/1025/1026` 不能合并成一个“空操作”。三者虽然只占两字节，却分别控制继续、让出和局部标志。共同join在`var_28|ESI==0`时还固定调用`0x0042D4D7 _AIL_serve`一次；普通yield不能只建模为状态返回。完整校正见`story-vm-common-join-audio-0042b0ae.md`。

## 未映射值不是可忽略 NOP

默认入口 `0x0042D230` 的可见顺序是：

1. `MessageBeep(0)`；
2. 以有效 opcode 和全局 `0x004CF6D8` 组织诊断调用；
3. 不修改当前剧情指令偏移；
4. `ESI` 与局部 continue 标志仍为零，经 `0x0042B0AE` 到 `_AIL_serve`；
5. 从 `sub_427920` 返回一。

因此未知 opcode 会在后续帧再次遇到同一命令，表现为不前进的错误/让出循环。初步 1:1 重写不得把它改成“跳过两字节”“抛异常后终止脚本”或静默 NOP；这类容错属于游戏逻辑修复，不是新系统启动兼容修复。

## 入口共享不等于 opcode 同义

192 个表项只落到 140 个不同入口；加入五个表外专用入口和默认入口后，总计 146 个一级入口目标。表内有 25 个目标被多个 opcode 共享，单个目标最多承接八个 opcode。主表自身有 76 个目标，次表有 67 个目标，其中三个跨表复用：

- `0x0042ADB7`：opcode `20` 与 `169`；
- `0x0042B1F1`：opcode `58` 与 `153`；
- `0x0042B287`：opcode `91` 与 `162`。

共享只证明首个基本块相同。handler 可以检查保存于栈上的有效 opcode、选择不同参数宽度或进入二级分支。例如：

- `0x0042B074/0x0042B070` 最终在 `0x0042B0E4` 按 opcode `29..33` 与 `181..185` 二次选择；中间 `34..180` 走内部默认项。其选择数据是 `0x0042D7F4` 的 6 项跳表和 `0x0042D80C` 的 157 字节表。
- `0x0042C567` 在 `0x0042C57B` 对 opcode `102,103,117,136,140,145,146,174` 选择八个不同 flag，其余 `104..173` 对应值走内部默认项。其数据是 `0x0042D8AC` 的 9 项跳表和 `0x0042D8D0` 的 73 字节表。

这两张表属于共享 handler 内部的 opcode 细分，不是第三、第四张一级分派表。机器目录把它们独立列出，防止后续按入口地址粗暴合并 opcode。

## 机器目录

- `inventory/story-vm-opcode-dispatch.tsv`：198 个显式 opcode 记录，含四个 raw-word 别名、表槽地址、首入口和首条指令。
- `inventory/story-vm-dispatch-ranges.tsv`：完整 14 位域的 11 条解码/边界规则。
- `inventory/story-vm-entry-target-groups.tsv`：146 个入口目标及其 opcode 集；默认入口用三个区间压缩表示。
- `inventory/story-vm-internal-opcode-switches.tsv`：两个共享 handler 内部的 opcode 细分表。
- `inventory/story-vm-handler-workpack.tsv`：P1 的 146 个唯一 handler 审计行，全部从 `pending_audit` 开始；P2 以 evidence/proof override 逐行推进。
- `inventory/story-vm-runtime-paths.tsv`：默认、特殊值、窗口、公共 join/yield 和返回路径的 17 行范围表。
- [`story-vm-handler-workpack-p1.md`](story-vm-handler-workpack-p1.md)：LST 单源生成、导航覆盖和 P2 停止线。

## 当前结论与下一步

P1 的完整 scope 已锁定，P2 当前闭环 21/146。默认非法入口、共享对话入口、bit31/bit30 清除入口、lifetime 暂存入口、role base/variant action、role position、role step、role action wait、same-file jump、unprepared/prepared role-path conditional jump、单角色/全角色 role-path release、共享 opcode20/169 role-path schedule、共享opcode21/22 global-bit conditional jump、opcode23/24 all/any-global-bits conditional jump，以及opcode25/26 global-bit set/clear入口已关闭；下一停止点为`0x004286C5`的opcode27 handler。后续仍必须从每个一级入口的真实 LST 控制流提取参数读取宽度、推进长度、状态读写、同步/异步/等待条件和错误路径，再用全部 `TALK*.DAT` 的真实命令流反向验证；任何按入口地址粗暴合并变体或继承既有 C++/文档完成状态的做法都会丢失内部细分。
