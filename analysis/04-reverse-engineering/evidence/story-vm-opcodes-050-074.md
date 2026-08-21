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
| `58` | 入队主图片动作节点 | 与次表153共享入口；完整初始化后分别前插主/次图片动作链 |
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

53 在countdown按有符号值 `>0` 时原地等待，`<=0` 才推进2字节并同调用继续；两路都发布previous，且handler自身不递减countdown或修改颜色状态。资产锁定1360条物理记录/1360个entry probe，全部raw `0x0035`、长度2；真实`TALK1.DAT@0x000043B8` opcode52→53等待/完成序列通过。完整证据见 [`story-vm-frame-color-wait-0042949d.md`](story-vm-frame-color-wait-0042949d.md)。每帧更新器 `sub_4146F0` 负责减少倒计时并累加三项。74 只把三个增量和倒计时写零，不覆盖当前值或目标值，所以它是取消后续推进，不是把插值状态整体清空。

## opcode 54–57：刷新次数和查找失败行为

54 把`FFF0`替换为talk source GUID，`FFFE`保留给lookup helper选择受控角色；lookup发生在signed repeat读取前且原版忽略失败。它总会先清action `+0x44/+0x42`并刷新一次；只有repeat大于零时才再循环指定次数，每轮清`+0x44`、刷新、再清角色`+0x98`低字。因此总刷新次数是`1 + max(repeat, 0)`，刷新失败仅诊断且最后一次回写除`+0x98`低字外均保留。资产锁定256条物理记录/258个entry probe，全部raw `0x0036`、长度6；`TALK1.DAT@0x00005A6B`真实回放通过。完整证据见 [`story-vm-role-action-repeat-refresh-004294c0.md`](story-vm-role-action-repeat-refresh-004294c0.md)。

55、56、57共用同一handler：先保存角色flags低两位旧空间分组，再清低两位并分别写为1、0、2；随后以旧分组解链，并按新flags分组重插。起始行严格使用`(world_y_u32 >> 4) - 1`的logical shift/回绕；helper链中not-found只诊断并继续，missing selector的`-1`角色索引与损坏空间链由modern在原unsafe点隔离。三条都推进4字节、发布normalized previous并跨帧让出。资产仅有`TALK4.DAT`四条物理记录/四个probe，55/56各一条、57两条，全部raw低位形式、长度4。完整证据见 [`story-vm-role-spatial-groups-004295f3.md`](story-vm-role-spatial-groups-004295f3.md)。

## opcode 58（与次表153共享）、59–61：图片动作、音频和 framebuffer

58与次表153共享入口。入口在读取任何operand前分配并清零`0xA4`字节节点，再在`+8`处调用`sub_40DC00`初始化内嵌`0x98`字节动作记录；随后严格按`+2/+4/+6/+8`逐word写入屏幕坐标、action id和base variant。四项完整后才访问链头：58前插主图片动作链`dword_4B7C70`，153前插次图片动作链`dword_4B8968`。两条都推进10字节、发布normalized previous并让出。

原版`malloc`返回没有空值检查，且operand故障会在进程终止前遗留未链接堆块。modern以未链接临时list节点保持分配→初始化→逐项写入→最终链入顺序，只在typed-stop无效域释放临时节点；平台owner也延迟到四项完整后检查。资产锁定58为73条物理记录/77 probes，153为11/11，合计84/88，全部低位raw且长度10；一条主链与连续两条次链TALK1记录回放固定了列表归属和前插顺序。完整证据见[`story-vm-picture-action-enqueue-0042b1f1.md`](story-vm-picture-action-enqueue-0042b1f1.md)。

59先快照当前全局样本混音等级，再读`u16(+2)`一基声音编号并调用`sub_485610`。wrapper按32位回绕执行`level << 7`后作signed `/11`，再向样本管理器提交pan0、loop1请求；下游把音量夹到`0..127`。0编号、资源/handle/Miles失败和成功启动均返回0，VM完全忽略结果，推进4字节、发布normalized previous并让出。

SDL端口每个世界帧使用当前mix level调用已审计`audio_video::play_legacy_sample`；资源上界检查与后端替换只隔离原版裸目录/Miles无效域。资产锁定740条物理记录/740个probe，分布224/155/279/82，全部raw `0x003B`、长度4；93种声音编号范围1..656，高位alias字样为0。完整证据见[`story-vm-sound-effect-0042967b.md`](story-vm-sound-effect-0042967b.md)。

60、61共享入口并先把`dword_4C9A18` bit0清零；normalized delta不是1的60直接进入+2尾。61在bit0已清状态下从`dword_4CD76C`取裸framebuffer指针，以`rep stosd`固定清零`0x25800`个dword，再重新读取flags并只把低字节bit0置1。两条均保留其余flag、推进2字节、发布normalized previous并让出；原版无framebuffer空指针检查，故裸指针故障会保留先前清位且阻止重新置位、推进和发布。

`0x96000 = 640 × 480 × 2`固定了完整16位逻辑framebuffer，而不是SDL或DirectDraw展示surface。modern `kLegacyWorldFrameClearOnly`以低`u8`建模已知flags，SDL端口清零`LegacyFramebuffer::physical_pixels()`这一同尺寸持有型span；owner缺失在任何效果前typed-stop。资产锁定60为21条、61为20条，共41条物理记录/41 probes，全部低位raw且长度2；唯一高位alias字样`0x403D`位于TALK1文件头dword目录，不是指令入口。TALK1各一条真实记录回放锁定了清位→清屏→置位与随后恢复场景顺序。完整证据见[`story-vm-scene-render-control-00429693.md`](story-vm-scene-render-control-00429693.md)。

## opcode 62：多重继承的地图角色 upsert

62 长18字节：selector的`FFF0`继承Talk source GUID；map的`FFFF`继承当前map；X/Y的`FFFF`分别继承受控角色坐标右移四位；path/action/base/variant的`FFFF`由`sub_40D460`逐字段保留。旧运行角色存在时，handler先重置72槽中的全部关联对象，保存flags低16和Talk id，清bits14/15、清地表占用并置bit28；这一清理发生在剩余14字节参数读取前。

MAPS patch成功且目标为当前map时，原版先分配并清零`0xD8`临时角色，由`sub_40D560`复制源字段，再由`sub_40F280`更新动作、映射地表标志并按条件写占用。随后从索引1直接搜索GUID：命中时使用新物化flags低2位摘链、整记录覆盖并重插，未命中时复制到role count槽、插入成功后递增count。最终flags bit9置位时遍历四个16字节粒子emitter；循环故意填满所有空selector槽并保留各槽head链，而不是只占一个槽。

GUID缺失仅诊断；正常、非当前map和缺失三路都推进18、发布previous并同调用继续。443条真实记录全部为raw`0x003E`、长度18、单entry probe，TALK1/2/3/4分布`77/59/128/179`；map sentinel 124条，X/Y sentinel各16条。完整LST、typed owner、失败顺序、四槽bug、资产与测试证据见[`story-vm-map-role-write-004296de.md`](story-vm-map-role-write-004296de.md)。

## opcode 63、64：变长 `u16` 表的原始边界

63 的布局是：

```text
+0   u16 opcode
+2   u16 prefix
+4   u16 item[0]
...  u16 item[n-1]
     u16 0xFF00 terminator
```

终止符扫描没有外部边界。`count <= 56` 时，handler先以`CFCF`填充64项目标表，复制项目但不复制`FF00`，把prefix零扩展写入滚动interval与remaining，并按top后left的读取顺序保存当前视口快照；cursor不重置。推进量为`6 + 2*count`，发布previous并同调用继续。`count > 56`时不访问owner、不推进IP，只诊断、发布previous并跨帧重复同一条指令。

这里`FF00`是脚本terminator，`CFCF`才是运行时空项，不能混用。现代复用既有64-word表和selection-scroll状态，并按原版`rep movsd`→读取top→奇数尾copy→写计时→读取left→写快照的切点保留typed失败部分效果。7条真实记录全部raw`0x003F`、count8、长度22，TALK1/2/3分布`2/1/4`；TALK1代表记录回放通过。完整证据见[`story-vm-selection-scroll-write-00429a1b.md`](story-vm-selection-scroll-write-00429a1b.md)。

64只把64个`u16`目标项恢复为`CFCF`，不会清interval、remaining、cursor或视口快照；随后推进2、发布previous并同调用继续。目的数组虽有64项，63的接受门槛却是56；两者都必须原样保留。资产锁定8条raw`0x0040`、长度2记录，TALK1/2/3分布`3/1/4`；TALK1真实回放通过。完整证据见[`story-vm-selection-scroll-clear-00429ad2.md`](story-vm-selection-scroll-clear-00429ad2.md)。

## opcode 65、66：消费后让出的转移记录操作

65不替换`FFF0`。找到角色时调用已独立闭环的`sub_40D610`：按Path/活动对象条件完成地表与空间对齐、MAPS flags `0x80` patch和对象槽清除，再写post与live party bookkeeping，清Talk、清flag`0x4000`并置`0x80`；找不到时不访问transfer owner并静默消费。现代nullable MAPS owner只在真实patch点检查，helper成功后按新party槽→live count顺序同步SDL帧状态。109条物理记录/110 probes全部raw`0x0041`、长度4，TALK1/2/3/4分布`52/1/21/35`；真实GUID3记录回放通过。完整证据见[`story-vm-role-transfer-00429ae8.md`](story-vm-role-transfer-00429ae8.md)。

66把七个`u16`全部零扩展后交给`sub_40D790`：缺失运行角色只清同GUID MAPS flags bit7；命中角色则按八个物理party槽完成可选对齐/空间摘链、运行角色与MAPS写入、surface标记、party indices/对象槽左移和post/live count同步。原版caller忽略helper返回，因此party未命中diagnostic、missing-source MAPS diagnostic，以及已完成party移除的MAPS/空间diagnostic都仍消费；checked方向/owner/surface等提前失败才typed-stop。100条真实记录全部raw`0x0042`、长度16，TALK1/2/3/4分布`48/0/22/30`；TALK1 selector9代表记录回放通过。完整证据见[`story-vm-role-map-update-00429b14.md`](story-vm-role-map-update-00429b14.md)。

两条指令成功或diagnostic-consumed后都保持`ESI=0`并让出。这里的跨帧边界属于剧情逻辑规格，不是平台性能策略。

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
