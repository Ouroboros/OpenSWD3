# 剧情 VM opcode 75–99 汇编语义批次

## 证据口径

本批次逐条回查 `sub_427920` 完整汇编，并继续下钻角色占位、三类画面 action 链、Bink 包装层、Ani 播放更新器和 PRNG 等直接 helper。汇编是唯一真实依据；IDA 伪码只作导航。操作名只用于区分当前已证明的数据流。

`inventory/story-vm-opcode-semantics-075-099.tsv` 是 25 行逐值规格。生成器读取已锁定的分派与长度表，并核对完整汇编 SHA-256。

## 操作摘要

| opcode | 当前中性操作名 | 汇编行为摘要 |
| ---: | --- | --- |
| `75` | 标记角色 bit31 并协调占位 | lookup 未检查，`sub_42E5A0` 对角色和地图对象执行对齐/占位处理 |
| `76` | 第一角色转向第二角色后标记 bit31 | 计算中心点距离和量化方向，刷新 action，再进入与 75 相同 helper |
| `77/78` | 设置/清除角色 `+0x88` | 找到角色时固定推进；找不到时推进量读取未赋值的局部变量 |
| `79` | 建立移动 action 对象 | 以两点和移动量计算浮点增量，挂到 `dword_4AD3E8` 链 |
| `80` | 清文本控制 bit29 | `dword_4A1360 &= 0xDFFFFFFF` |
| `81/82/86` | 建立、遣出、改键角色头像 action | 共用 `dword_4BA6E0` 的 `0xB4` 字节 action 链 |
| `83/84` | 建立/控制 framebuffer 区域效果 | 逐行 span 数组、四种更新模式及原始未检查边界 |
| `85` | 启动 Bink 视频 | 清屏并提交后解析 `%Q` 文件名，`.avi/.mpg` 在内部改为 `.bik` |
| `87` | 随机 TALK 窗口转移 | `u32` 目标表以 `FF00FF00` 结束；空表会随机除零 |
| `88` | 请求战斗 | 清两类画面 action 链并写带 bit31 的战斗请求 |
| `89/90` | mode 2 文本 action | 复用 1–6 的混合 selector/text 编码；89 具有奇数变体副作用 |
| `91` | 装入文本名称缓冲 | 从相对偏移表复制固定 32 字节，再寻找 `%Q` 并生成两个文本缓冲 |
| `92/93` | 置/清保留全局 bit | 一基 `1..4` 映射到 bit `30..33`；非法值诊断后仍执行 |
| `94/95` | 置/清场景 bit1 | 两者都消费后让出 |
| `96/97/99` | 启动、等待 Ani 与等待相位 | 恢复 Ani 初始化、异步更新门控和严格相位比较 |
| `98` | 四字节空操作 | `+2` 载荷未读取，消费后让出 |

TALK 全分支候选图观察到本批次 24/25 个值；唯一未观察的是 opcode 93。观察计数从 opcode 79 的 2 个节点到 opcode 80 的 6291 个节点不等。候选解码位置不等于实际游戏状态轨迹；样本中没有 93 也不允许删除其明确 handler。

## opcode 75、76：bit31 角色协议和未检查查找

75 不替换 `FFF0`，直接以 `sub_40C0D0` 的输出调用 `sub_42E5A0`。helper 对受控角色还会把坐标低四位按世界移动分量逐步归整、清四个移动分量并重做对象占位；对其他角色会按地图对象的步长表归整。最后无条件置 `role+0x10` bit31。

lookup 失败时 `sub_40C0D0` 把输出写成 `0xFFFFFFFF`，75 不检查返回值。`sub_42E5A0` 随即用 index `-1` 构造角色指针，在数组前读写。helper 自带的 `FFFE` 当前角色规则仍有效，但这不能被误写为 75 支持 `FFF0`。

75现已独立闭环：现代复用`suspend_legacy_world_story_role`，合法域保持helper副作用、+4、previous75和same-call；index -1越界与固定全局owner收敛为typed失败。82条真实记录/82 probes全部raw`0x004B`、长度4，TALK1/2/3/4分布`19/43/18/2`；完整证据见[`story-vm-role-suspend-00429d70.md`](story-vm-role-suspend-00429d70.md)。

76 的第一个 selector 支持 `FFF0`，第二个不支持。两次 lookup 失败都只输出诊断并继续。正常路径以两个角色的坐标和范围字段组成中心点，`sub_411E20` 得到整数距离和量化方向，`sub_411F00` 再映射 facing 值：

- 先把第一角色 action `+0x08/+0x34` 清零；
- 中心距离至少为 4 时才把 facing 写入 `+0x34`；
- 清 action `+0x44` 并刷新；
- 最后对第一角色调用 `sub_42E5A0`。

任一 lookup 失败都会让中心点计算在角色数组前继续执行。这是原始异常，不得把 76 现代化成“任一角色不存在就跳过”。

76现已独立闭环：两个lookup按LST分阶段，第一参数独占`FFF0`替换；合法域保持中心点/朝向、action刷新、挂起、+6、previous76和same-call，两处index -1越界与固定owner收敛为typed失败。449条真实记录/450 probes全部raw`0x004C`、长度6，TALK1/2/3/4分布`181/140/56/72`；完整证据见[`story-vm-role-turn-suspend-00429da6.md`](story-vm-role-turn-suspend-00429da6.md)。

## opcode 77、78：成功宽度固定，失败宽度来自旧栈值

77/78 共用 `0x00429F7B`。找到角色时：

- 77 把 `u16(+4) | 0x8000` 写到 `role+0x88`，长度设为 6；
- 78 把 `role+0x88` 写零，长度设为 4；
- 两者都清 action `+0x44` 并调用 `sub_4321E0`。

找不到角色时会执行格式化和 `sub_40C060/sub_40AF70` fallback，但该分支从未给 `var_40` 赋值。公共尾部仍读取 `var_40` 并把它同时加到脚本指针和 16 位 IP。`var_40` 在函数入口和每次分派前都没有初始化，因此推进量取决于本次 `sub_427920` 调用中该栈槽此前留下的值，或者新调用的原栈内容。

这也解释了物理长度目录只能确认“成功 lookup 时 77 为 6、78 为 4”。未来兼容器不能悄悄把失败路径定成固定长度；必须先用原程序 oracle 捕获可触发状态，或明确把此未定义机器状态作为隔离兼容例外。

77/78现已独立闭环：selector命中后77才读取payload；成功域保持word写入、刷新、固定宽度、previous与same-call，missing陈旧`var_40`推进收敛为typed stop。77有442条记录/447 probes，TALK分布`137/109/113/83`；78有4条/4 probes，分布`1/3/0/0`。完整证据见[`story-vm-role-wait-override-00429f7b.md`](story-vm-role-wait-override-00429f7b.md)。

## opcode 79：两点移动 action 的零距离除法

79 分配、清零并初始化一个 `0xB4` 字节 action 对象。四个坐标参数左移四位成为起点和终点；`s16(+14)` 作为每拍移动量。handler 用 32 位整数计算：

```text
dx = target_x - start_x
dy = target_y - start_y
distance = sqrt(dx*dx + dy*dy)
step_x = dx * movement / distance
step_y = dy * movement / distance
```

两个 step 以浮点保存，起点也转换成浮点，节点前插到 `dword_4AD3E8`。`sub_414B60` 每帧先绘制 action，再累加坐标；进入目标点横纵各 `±32` 的范围时释放节点。

坐标平方和先以32位整数完成，可能在`fsqrt`前溢出；起点和终点相同时还会直接执行x87零除。`malloc`也没有空值检查。这些边界不得由C++容器或向量库隐式改变。

79现已独立闭环：现代0xB4节点与既有moving list生命周期接通，保留分配/初始化时点、7个staged operand、16/32位wrapping、x87中间精度、前插、+16、previous79和same-call；裸分配/指针改为typed容器。TALK线性目录为0条/0 probes，使用`asset_absence_verified`，不把2个候选CFG节点冒充真实记录。完整证据见[`story-vm-moving-action-enqueue-0042a0a6.md`](story-vm-moving-action-enqueue-0042a0a6.md)。

## opcode 80：清文本控制 bit29

80对32位`dword_4A1360`执行`&= 0xDFFFFFFF`，经共享尾写回，再+2、发布previous80并same-call继续。它没有operand、helper、callback或yield；其他31位必须保持。

80现已独立闭环：四raw alias精确尾与`TALK1.DAT@0x00004520`真实记录通过；资产锁为2256条/2256 probes，分布`609/453/507/687`，全部raw `0x0050`、长度2。完整证据见[`story-vm-text-control-bit29-clear-0042a1ef.md`](story-vm-text-control-bit29-clear-0042a1ef.md)。

## opcode 81、82、86：角色头像 action 链

81 建立另一类 `0xB4` 字节 action 对象并前插到 `dword_4BA6E0`。`+2/+4` 是 action id/variant，`+6` 是目标 X；`+8` 的低 15 位是目标 Y，bit15 控制特殊初始/运动路径。普通路径按目标 X 是否大于 320，从 `-120` 或 `760` 开始；每帧向目标 X 收敛并绘制。

82 用 action id/variant 查找第一个节点并发出离场运动：原运动 word bit15 已置时写 `10000`，否则当前 X 在 320 左侧写 `-1`，右侧写 `+1`。86 同样只找第一个匹配项，然后把 key 改成 `+6/+8` 的新 id/variant。找不到都静默消费。

三个 handler 没有节点容量或分配保护。它们共享链表不代表可以改成按 key 唯一的 map；重复 key 存在时只改第一个节点的顺序行为必须保留。

81现已独立闭环：现代精确0xB4节点接入既有role-head list，保留分配/初始化、四个staged operand、signed X边界、low15 Y、bit15特殊motion、前插、+10、previous81与same-call。资产锁1888条/1888 probes，分布`488/340/398/662`，全部raw `0x0051`、长度10；3条bit15特殊记录均在TALK2。普通与特殊真实回放通过。完整证据见[`story-vm-role-head-action-enqueue-0042a200.md`](story-vm-role-head-action-enqueue-0042a200.md)。

82现已独立闭环：保留首匹配、motion bit15→10000、signed X左右遣出、空链不读operand、ID miss不读variant、所有缺失路径静默+6、previous82与same-call。资产锁1889条/1889 probes，分布`489/340/398/662`，全部raw `0x0052`、长度6；TALK1真实精确尾回放通过。完整证据见[`story-vm-role-head-action-dismiss-0042a2c6.md`](story-vm-role-head-action-dismiss-0042a2c6.md)。

## opcode 83、84：framebuffer 区域效果与两个原始越界面

83 先按 id 低字节删除所有同 id 节点，再建立 24 字节记录和两条逐行 `s16` 数组。参数 `X/Y/width/height` 都先清 bit0；合法门控只有：

```text
X >= 0
Y >= 0
X + width  <= 640
Y + height <= 480
```

它没有要求 width/height 为正。数组分配尺寸使用 `2*height+8`，所以负 height 可以形成回绕后的分配参数。`+4` 在消费者中还会符号扩展后索引颜色表，没有范围检查。mode 1、2 和其他值分别建立 `0x4000`、`0x0800`、`0x8000` 更新状态；每帧消费者随机改变各行 span 并直接写 16 位 framebuffer。

84 找第一个 id 相同的节点：操作 0 OR `0x2000`，操作 1 OR `0x1000`，操作 2 释放节点。若操作值不是 0、1、2，代码仍在公共尾部把 `var_44` OR 进节点状态；该局部变量在这条路径上没有赋值，来源同样是旧栈内容。非法 id `>=256` 只诊断，缺失 id 静默消费。

83现已独立闭环：保留invalid-ID早消费、删除全部同ID先于分配/后续读取、七个staged截断、四矩形门、无正尺寸附加门、两级数组、三mode初始化、前插、+16、previous83与same-call。资产锁1879条/1879 probes，分布`485/337/396/661`，全部raw `0x0053`、长度16；mode为1/0/11共`935/938/6`，TALK1真实110行mode1回放通过。完整证据见[`story-vm-packed-row-effect-upsert-0042a341.md`](story-vm-packed-row-effect-upsert-0042a341.md)。

84现已独立闭环：保留invalid-ID/空链/ID miss不读operation、首匹配、op0/1高mode替换、op2释放、+6、previous84与same-call。资产锁1879条/1879 probes，分布`485/337/396/661`，operation0/1/3/8为`910/963/3/3`；真实op0/op1回放成功，6条真实3/8记录因原版陈旧`var_44`而modern typed-stop。完整证据见[`story-vm-packed-row-effect-control-0042a54c.md`](story-vm-packed-row-effect-control-0042a54c.md)。

## opcode 85：清屏提交后启动 Bink

85 先以运行时逻辑宽高计算字节数，清零软件 framebuffer，并把软件源 surface 整体 Blt 到 primary。随后`AIL_serve`并由`sub_484730`从`+2`开始消费到`%Q`：

- CD checker失败时设置close-request、不消费文件名并yield；
- 成功时构造`video\\swd3\\`路径；
- 若文件名含case-sensitive `.avi`或`.mpg`，首匹配原地改成`.bik`；
- 建立Bink包装对象并调用打开入口；
- 设置视频活动位。

85现已独立闭环：修正旧C++把合法`%Q`恰好结束在`0x8000`误判为失败的问题，恢复clear→present→audio→preflight→parse→begin、previous85与yield；SDL以配置data root和typed video backend替代CD/固定路径/Bink裸owner。资产锁11条/11 probes，分布`6/2/1/2`，全部raw `0x0055`和`%Q`终止；真实`OPENING.bik`与`Demo.mpg`精确尾回放通过。完整证据见[`story-vm-video-start-0042a611.md`](story-vm-video-start-0042a611.md)。

正常解析后推进到 `%Q` 之后并跨帧让出。若 CD/path helper 返回 2，helper 在扫描字符串前返回零，既不推进 IP，也不释放刚分配的 `0x400` 字节临时区；外层不检查返回值，因此下一帧会清屏、提交并重试同一 opcode。

平台层将来可以用 FFmpeg 或其他解码后端替换 Bink DLL，但必须保留“先清屏并提交、再尝试打开”、扩展名改写、失败重试位置和脚本消费时序。

## opcode 86：改写头像动作键

86遍历固定0xB4头像动作链，以完整32位node action ID/base variant和脚本零扩展u16旧键比较；只把首个exact match的`+0x00/+0x08`依次改为new action ID/new variant。访问严格分阶段：空链不读任何operand，ID miss不读variant/new key，new ID写后才读取new variant，因此末operand截断会保留ID部分写。所有路径固定+10、发布previous86并same-call继续。

86现已独立闭环：typed list owner替代固定裸全局/哨兵，保留线性顺序、完整dword比较、首匹配和unsafe副作用。资产锁34条/34 probes，分布`4/22/5/3`，全部raw `0x0056`、长度10；真实`TALK1 10002/18→10002/24`与`TALK4 10001/22→10001/54`精确尾回放通过。完整证据见[`story-vm-role-head-action-key-rewrite-0042a673.md`](story-vm-role-head-action-key-rewrite-0042a673.md)。

## opcode 87：空随机目标表直接整数除零

87 从 `+2` 开始扫描 `u32` 目标直到 `0xFF00FF00`，调用 `sub_439070(count)` 取得 `[0,count)` 的无偏随机索引，再由 `sub_42E430` 重载 TALK 窗口。成功路径不按表的物理尾继续，而是把当前脚本指针替换为新窗口基址。

终止符扫描无边界。首项就是 sentinel 时 `count=0`；`sub_439070` 的第一步是 `0xFFFF / count`，没有零保护，因此空表触发整数除法错误。随机数算法、拒绝采样次数和调用顺序也属于 1:1 规格，不能替换为 `std::uniform_int_distribution` 后假设等价。

87现已独立闭环：复用assembly-exact 250-word secondary RNG，保持每次尝试先丢弃1个raw、再以candidate做`0xFFFF` acceptance/remainder；空表以明确DIV0 typed-stop隔离，不伪造目标。`sub_42E430`的audio、同文件target、IP0、0x8000窗口与same-call已恢复。资产锁5条/5 probes，分布`3/1/0/1`，四条3-target、一条6-target；固定seed真实回放选择`TALK1 index1=0x28995`与`TALK4 index4=0x2FEB3`。完整证据见[`story-vm-random-target-reload-0042a6cb.md`](story-vm-random-target-reload-0042a6cb.md)。

## opcode 88：战斗请求之前只清两条链

88 调用 `sub_40F500` 释放 framebuffer 区域效果链，调用 `sub_40F570` 释放角色头像链，然后写：

```text
dword_4A94AC = sign_extend(s16(+2)) | 0x80000000
```

这就是主帧消费的战斗请求。它不清 79 使用的 `dword_4AD3E8` 移动 action 链。推进四字节后 `ESI=0`，所以请求提交后立即让出，不继续执行下一条剧情指令。

88现已独立闭环：修正旧C++在释放前whole-record/三owner预检及漏发previous88，恢复packed-row→role-head→signed operand→request→IP/previous→yield顺序；每个typed失败保留此前释放。资产锁52条/52 probes，分布`20/7/8/17`，全部raw `0x0058`、长度4；真实battle98/290精确尾回放通过，移动链保持。完整证据见[`story-vm-battle-request-0042a727.md`](story-vm-battle-request-0042a727.md)。

## opcode 89、90 与 1–6 的共享文本入口

89/90 完整复用 opcode 1–6 的 `0x00427B8F`：首个 `u16` 同时是 selector 和文本头两个字节，`FFF0` 会被改写进脚本窗口，载荷逐字节扫描到 `%Q`，分配 `0x4C` 记录并加入文本/action 队列。

两者把内部 mode 固定为 2。89 是奇数变体，还置记录 `0x10` flag、把选中上下文的 action 状态写一并递增 `dword_4A9920`；90 不做这三项。两者消费后都让出。

## opcode 91–95：名称缓冲、bit 映射和场景位

91 把 `FFF0` 替换为当前状态 selector，然后直接用该值索引 `dword_4C9A10` 的相对偏移表；这里没有 `sub_40C0D0`，所以 `FFFE` 不会获得 helper 的当前角色语义。它固定复制 32 字节到 `word_4CF6B8`，向后寻找 `%Q`，把 `%` 写零，再调用两次 `sub_40BAA0` 生成两个全局文本缓冲。索引和终止扫描都没有边界检查。

shared 91/162现已独立闭环：91使用u16显式index并把FFF0替换为source GUID；162只接受变量11/12，读取完整u32动态index，非法selector或zero只消费。共享路径保持u32目录回绕、32-byte copy、首个`%Q`、固定buffer前缀替换、+4、previous与same-call；缺terminator在copy后typed-stop。资产锁1184条/1203 probes：91为1180/1199、162为4/4，四条162均type11；真实TALK1显式782与variable11→782得到相同MAPS姓名。完整证据见[`story-vm-name-record-load-0042b287.md`](story-vm-name-record-load-0042b287.md)。

92/93 计算 `u32(selector)-1+30`，分别调用无边界全局 bit 置位/清位 helper。有效 selector 1–4 映射 bit30–33；零会经 32 位回绕映射到 bit29，5 以上则继续向后。非法值虽然诊断，仍然执行访问。当前候选资产没有 opcode 93，但 handler 完整存在。

opcode92现已独立闭环：保留u16零扩展、u32 dec/+30回绕、selector0→bit29、invalid-safe继续写、+4、previous与same-call；0x400-byte owner最后安全selector8162，8163起在原始裸写点typed-stop，避免u16 helper错误截断。线性资产0条/0 probes，四raw全文件102处双字节候选均非指令入口。完整证据见[`story-vm-reserved-global-bit-set-0042a756.md`](story-vm-reserved-global-bit-set-0042a756.md)。

opcode93另行独立闭环：相同索引公式在clear helper中按`FF-mask`只清单bit；selector0、invalid-safe、8162/8163边界、+4、previous和same-call独立锁定。线性资产同为0条/0 probes，四raw全文件41处候选均非入口。完整证据见[`story-vm-reserved-global-bit-clear-0042a792.md`](story-vm-reserved-global-bit-clear-0042a792.md)。

94/95 分别置、清 `dword_4C9A18` bit1。两者都推进两字节但保持 `ESI=0`，因此消费后跨帧让出。

opcode94现已独立闭环：修复旧combined modern case漏发previous94，保持OR2、其他低位、+2与yield；原dword owner映射到已集成u8低位scene-runtime owner，缺失在原读取点typed-stop。资产锁39条/39 probes，分布`14/7/3/15`；真实TALK1 `0x49F4`精确尾回放`A5→A7`。完整证据见[`story-vm-scene-render-bit1-set-0042a7ce.md`](story-vm-scene-render-bit1-set-0042a7ce.md)。

opcode95另行独立闭环：按`AND FFFFFFFD`只清bit1，补齐旧numeric case漏发previous95，保持+2与yield；同一低位owner缺失在原读取点typed-stop。资产锁25条/25 probes，分布`9/5/2/9`；真实TALK1 `0x4A22`精确尾回放`A7→A5`。完整证据见[`story-vm-scene-render-bit1-clear-0042a7ee.md`](story-vm-scene-render-bit1-clear-0042a7ee.md)。

## opcode 96：自定义 Ani 打开与启动

96 是当前自定义 Ani 的剧情启动入口。它先把全局帧间隔从通常的 35 ms 改为 70 ms，然后解析：

```text
+2  optional byte '%': flags |= 2
    optional byte '*': flags |= 1
    filename bytes
    terminator 25 51 (%Q)
```

两个前缀按顺序独立判断。handler 建立 `Video\...` / `swd3\Video\...` 路径，打开文件，清旧帧节点，固定读取 `0x24` 字节头；随后写入首帧链字段，分配两个 `0x9C400` 缓冲。storage bpp 为 8 时，从文件尾读取初始 `0x300` RGB 调色板并转换成 256 个 16 位颜色。最后把相位值暂设为一，调用 `sub_4158C0` 预载首帧，再改成 `-13` 进入每帧 Ani 更新器 `sub_4154A0`。

原始错误行为也已固定：

- 文件名和路径复制、`%Q` 扫描都无边界；
- CD helper 返回 2 时，文件名已经消费，函数设置全局错误位并直接以 `EAX=0` 退出整个 `sub_427920`；
- Ani 文件打开失败时，指令已经消费，两个临时 `0x400` 缓冲没有释放，随后让出；
- 头读取长度不是 `0x24` 时只记录错误、Sleep 250 ms，仍继续解释当前缓冲内容；
- 大小分配都不检查空指针。

已有 `ani-container-and-lzo-boundary.md` 已全量固定 19 文件、5312 帧的容器和解压边界；本批次把它接回了实际剧情启动和异步状态机，但还没有因此宣布全部 Ani 时间、淡入淡出和中途调色板事件完成。

## opcode 97、99：两个不同的 Ani 等待条件

97 只检查 `dword_4CAE8C`。非零时不推进并跨帧等待；Ani 更新器完成资源释放时把它写零。变零后 97 推进两字节，把帧间隔恢复为 35 ms，并同帧继续。它不检查文件对象、相位计数或错误码。

99 则把有符号 `dword_4B7AC8` 与零扩展 `u16(+2)` 作有符号比较。只有 counter 严格大于 threshold 才推进四字节；`<=` 时原地跨帧等待。96 成功后把 counter 写成 `-13`，因此启动负相位不会满足任何非负 threshold。97 是“整个 Ani 活动结束”门控，99 是“内部时间线超过指定点”门控，不能合并。

## opcode 98：带未读载荷的跨帧空操作

98 只把脚本指针和 16 位 IP 各加四，完全不读取 `+2` 的物理 `u16`，也不设置 `ESI=1`。所以它不是普通同帧 no-op，而是固定消费四字节后让出一次。

## 产物与下一批

- `inventory/story-vm-opcode-semantics-075-099.tsv`：本批次 25 行人工汇编语义。
- `tools/build_story_vm_opcode_semantics_075_099.py`：汇编哈希锁定生成器。

下一批从 opcode 100 开始。100–193 属于第二张一级表，但仍由同一个 `sub_427920` 解释器和同一套继续/让出协议执行；会继续逐值回查，不能因编号跨表就改用另一套抽象。
