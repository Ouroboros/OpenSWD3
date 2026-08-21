# 剧情 VM opcode 50–74 汇编语义批次

## 证据口径

本批次逐条回查 `sub_427920` 完整汇编，以及只为解释直接调用所必需的 helper。汇编是唯一真实依据；IDA 伪码只作定位。操作名保持业务中性，字段名只使用已由数据流或既有结构证据支持的含义。

`inventory/story-vm-opcode-semantics-050-074.tsv` 是 25 行逐值规格。生成器同时读取已锁定的一级分派与物理长度表，并核对完整汇编 SHA-256，避免人工语义与入口、长度脱节。

## 操作摘要

| opcode | 当前中性操作名 | 汇编行为摘要 |
| ---: | --- | --- |
| `50/70/73` | 启动视口移动 | 相对坐标、绝对坐标或角色坐标三种目标，共用余量、步长、整除检查和地图边界裁剪 |
| `51` | 等待视口移动结束 | 四个移动状态量任一非零就不推进并让出 |
| `52/53/74` | 三分量浮点插值 | 设置起点、终点、每拍增量和倒计时；等待或取消只按原状态字段操作 |
| `54` | 重复 action refresh | 始终先刷新一次，正重复数再额外刷新指定次数 |
| `55..57` | 修改角色状态低两位 | 分别写成 `1/0/2`，并用旧低两位调用对象放置 helper |
| `58` | 入队临时 action 节点 | 分配 `0xA4` 字节节点，内嵌 `0x98` 字节 action 后挂到链表头 |
| `59` | 请求音效播放 | 经 Miles 包装层提交声音编号，忽略返回值 |
| `60/61` | 场景提交位与 framebuffer | 清 bit0，或清零 `0x96000` 字节 framebuffer 后置 bit0 |
| `62` | 新增或更新地图角色记录 | 多种 `FFFF/FFF0` 继承、旧对象清理、当前地图角色替换或追加 |
| `63/64` | 设置/清空 `u16` 选择表 | `FF00` 终止的变长表；超 56 项原地重复，清空不清关联快照 |
| `65/66` | 角色地图转移记录 | 调用两个角色/地图 bookkeeping helper，消费后跨帧让出 |
| `67` | 自修改帧时钟等待 | 用脚本参数 bit15 保存阶段，完成条件是无符号 elapsed 严格大于 duration |
| `68/69` | 清/置角色状态 bit `0x400` | 找到角色时直接改位，未找到时走地图记录 fallback |
| `71/72` | 设置/清除角色外部 action 指针 | 指向静态 `0x98` 字节槽或写零；selector 和槽位均无额外正规化 |

当前四个 TALK 资产的全分支候选图观察到了本批次全部 25 个 opcode。这里的“观察到”只证明某个候选解码位置存在，不证明所有分支、参数和异常在实际游戏状态下都会触发。

## opcode 50、70、73：共用视口移动初始化器

三个opcode共享入口，但参数与长度独立：50把`+2/+4`当signed relative tile位移，70把它们当signed absolute target tile并减当前viewport left/top的算术右移tile值，两者长度10；73用`+2`查角色、经`sub_40D160`构造居中且受地图限制的640×480目标viewport，长度8。73不替换`FFF0`，lookup miss在viewport copy之后形成index -1的role coordinate访问；modern只在该unsafe点checked-stop且不增加MAPS fallback。

共同路径严格按X后Y写raw u16 step，把tile位移左移4成wrapping pixel i32，再用signed `IDIV`检查整除。非整除step先改为正4，两轴除法完成后才按位移符号定方向；非零位移配zero step会在pixel remaining和两项raw step已写后触发CPU整数除零，modern以专用status隔离并保留此前effects。随后按viewport left/right/top/bottom与`map_width/height<<4`的signed wrapping endpoint夹取remaining；夹取发生在step确定后，不重新计算step，zero map也不提前失败。

73还有共享diagnostic的原版bug：发生共同endpoint clamp时，固定diagnostic参数读取会把`current+8`（下一opcode word）当作本指令参数。modern只在该条件下保留额外window边界；无clamp的`0x7FF8`完整记录可先完成IP/previous再由下一fetch失败，有clamp的同位置记录则保留movement/clamp效果后在diagnostic overread处停止。

资产锁定17/62/34条物理记录与同数entry probe，合计113/113，全部为基础raw且长度分别10/10/8；三条代表性TALK1真实回放通过。完整LST、分阶段unsafe、资产与测试证据见 [`story-vm-camera-move-00429066.md`](story-vm-camera-move-00429066.md)。

## opcode 51：四字段联合等待

51 按X/Y剩余位移、X/Y步长的顺序短路检查四个dword。任一字段非零时不推进 IP、保持 `ESI=0` 并让出；四者全零才推进两字节、设置 `ESI=1` 并在当前调用继续。两路都会经过共同出口并发布归一化previous，不能只在完成时发布。

因此重写不能只看“剩余距离是否为零”，也不能在启动指令后直接阻塞到移动结束。初始化、每帧更新和脚本等待是三个独立时序点。资产锁定108条物理记录和108个entry probe，全部raw `0x0033`、长度2；真实`TALK1.DAT@0x000046C2`等待→完成回放通过。完整证据见 [`story-vm-camera-wait-00429362.md`](story-vm-camera-wait-00429362.md)。

## opcode 52、53、74：三分量插值协议

52 的物理长度为 16 字节：六个 `s16` 分别作为三项起点和三项目标，最后一个 `u16` 是 duration。三个current按operand逐读逐写；三个target必须全部读完后才依次写入。duration零扩展为dword countdown，再计算三项 `(target-current)/duration` 浮点增量，推进16字节、发布previous并同调用继续。

duration 为零时没有前置保护，直接进入 x87 除法。modern已按delta符号固定`+Inf/-Inf/0xFFC00000`，并对资产54种唯一差值/时长及零时长/极值共59组与宿主x87逐位比较零差异。资产锁定1361条物理记录/1361个entry probe，全部raw `0x0034`、长度16、duration `1..46`；`TALK1.DAT@0x000043B8`真实回放通过。完整证据见 [`story-vm-frame-color-transition-004293ac.md`](story-vm-frame-color-transition-004293ac.md)。

53 在倒计时按有符号值 `>0` 时原地等待；`<=0` 才推进。每帧更新器 `sub_4146F0` 负责减少倒计时并累加三项。74 只把三个增量和倒计时写零，不覆盖当前值或目标值，所以它是取消后续推进，不是把插值状态整体清空。

## opcode 54–57：刷新次数和查找失败行为

54 把 `FFF0` 替换为当前状态 `+0x24`，但随后没有检查查找结果。它总会先清 action `+0x44/+0x42` 并调用一次 `sub_4321E0`；只有 `s16` repeat count 大于零时，才再循环指定次数，每次重复清 `+0x44`、刷新并清角色 `+0x98` 低字。因此总刷新次数是 `1 + max(repeat, 0)`，不是脚本字段本身。

55、56、57 共用同一 handler，分别把角色状态 `+0x10` 的低两位写成 `1`、`0`、`2`，同时保留旧低两位用于 `sub_411530`。三者都支持 `FFF0`，但同样不检查 lookup 返回；缺失角色会在角色数组基址前形成访问。正常路径推进四字节后让出，不同帧继续取下一条。

## opcode 58–61：临时 action、音频和 framebuffer

58 分配并清零 `0xA4` 字节节点，在 `+8` 放置一个由 `sub_40DC00` 初始化的 `0x98` 字节 action，再写入四个 `u16` 参数并前插到 `dword_4B7C70` 链表。`malloc` 返回没有空值检查；指令消费后让出。

59 把 `u16(+2)` 和全局缩放值传入 `sub_485610`，再由 `sub_485CE0` 进入 Miles 音频对象。返回值不参与剧情分支；handler 推进四字节后让出。平台音频替换可以改变后端，不能改变这条请求在剧情帧中的消费和让出位置。

60 清 `dword_4C9A18` bit0。61 对 `dword_4CD76C` 指向的 framebuffer 执行 `rep stosd`，固定清零 `0x25800` 个 dword，即 `0x96000` 字节，然后置 bit0。61 不检查 framebuffer 指针；两条指令都推进两字节并让出。

`0x96000 = 640 × 480 × 2` 进一步固定了这里操作的是完整 16 位逻辑 framebuffer，而不是 SDL3 或 DirectDraw 的展示 surface。未来 SDL3 平台层必须保留这块软件缓冲及原始时序。

## opcode 62：多重继承的地图角色 upsert

62 长 18 字节，八个 `u16` 参数并非使用统一 sentinel 规则：

- selector `FFF0` 继承当前状态 `+0x24`；
- map id `FFFF` 继承当前 ArgList map；
- X/Y 的 `FFFF` 分别继承受控角色坐标右移四位；
- 其余字段原样零扩展。

若角色已存在，handler 扫描 72 个对象槽并重置关联对象，保存一部分旧角色状态，清状态 bit14/15，刷新角色，再置 bit28。随后用精确解析后的字段调用 `sub_40D460`。目标 map 正是当前活动 map 时，还会建立临时角色记录，按 GUID 替换现有 `0xD8` 字节角色或追加新记录，并更新辅助对象表。

当前 handler 没有证明角色数组容量或辅助表空槽受到保护；helper 失败也只是诊断。初步重写不得把各 sentinel 合并成统一参数模板，也不得擅自增加会改变正常脚本分支的“安全失败”语义。

## opcode 63、64：变长 `u16` 表的原始边界

63 的布局是：

```text
+0   u16 opcode
+2   u16 prefix
+4   u16 item[0]
...  u16 item[n-1]
     u16 0xFF00 terminator
```

终止符扫描没有外部边界。`count <= 56` 时，handler 先以 `CFCF` 填充 64 项目标表，再复制项目但不复制终止符，同时保存 prefix 和当前视口原点；推进量为 `6 + 2*count`。`count > 56` 时只诊断，不推进 IP，并跨帧重复同一条指令。

64 只把 64 个 `u16` 目标项恢复为 `CFCF`，不会清 prefix 或视口快照。目的数组虽有 64 项，接受门槛却是 56；两者都必须原样保留。

## opcode 65、66：消费后让出的转移记录操作

65 不替换 `FFF0`。找到角色时调用 `sub_40D610` 协调 Path、地图对象、角色状态和转移 bookkeeping；找不到时静默消费。66 把七个 `u16` 全部零扩展后交给 `sub_40D790`，由 helper 更新活动角色或 pending map-role 记录，返回值不被观察。

两条指令成功消费后都保持 `ESI=0` 并让出。这里的跨帧边界属于剧情逻辑规格，不是平台性能策略。

## opcode 67：脚本内存就是等待状态

67 把 `+2` 的低 15 位作为 duration，把 bit15 当阶段标记：

1. bit15 未置时，保存 duration 和当前 accepted-frame clock，并直接在脚本字节中置 bit15；IP 不推进并让出。
2. bit15 已置时，以 32 位无符号回绕减法计算 elapsed。`elapsed <= duration` 继续原地等待；只有严格 `elapsed > duration` 才清脚本 bit15、推进四字节并同帧继续。

这条指令会自修改已载入的剧情窗口。重写若把脚本资源映射为只读内存，必须在 VM 层提供等价的可写窗口状态；不能把严格 `>` 改成 `>=`，也不能把无符号回绕改成无限精度时间差。

## opcode 68、69、71、72：相近角色指令仍有不同 selector 合同

68/69 会先把 `FFF0` 替换为当前 selector。找到角色时分别清或置 `role+0x10` 的 `0x400` 位；找不到时调用 `sub_40D460` 记录 fallback 请求。两者推进后让出。

71/72 不替换 `FFF0`。71 找到角色时写 `role+0x3C = 0x004B9F68 + slot*0x98`，slot 没有范围检查；72 把该字段写零。找不到角色时两者都静默消费。即使四条指令都操作角色状态，也不能共用一个自动处理 sentinel、查找失败和范围检查的现代化包装器。

## 产物与下一批

- `inventory/story-vm-opcode-semantics-050-074.tsv`：本批次 25 行人工汇编语义。
- `tools/build_story_vm_opcode_semantics_050_074.py`：汇编哈希锁定生成器。

下一批从 opcode 75 开始，继续记录参数宽度和符号、直接/间接状态效果、脚本 IP、同帧继续、跨帧让出、等待条件及原始异常。不会把候选 CFG、诊断字符串或 IDA 伪码直接冒充最终语义。
