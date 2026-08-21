# 剧情 VM 完整闭环追加 PLAN

状态：执行中，P0/P1 已完成，当前步骤 P2；当前 shared handler `0x0042B287`（opcodes 91/162）

优先级：高于 [`execution-plan-pi.md`](execution-plan-pi.md) 的当前执行队列

Pi 执行框架：继承 [`execution-plan-pi.md`](execution-plan-pi.md) 顶部规定的主 Agent 主导、
子 Agent 使用干净 Context、单写者、独立审查和阶段性 TG 汇报约束。

## 1. 与原 PLAN 的关系

本文件由 [`execution-plan-pi.md`](execution-plan-pi.md) 挂载并优先执行，不替代原 PLAN。
原 PLAN 中未被本文件调整的目标、约束、模块边界、验证规则和完成条件继续有效。
本文件完成后停止优先级覆盖，恢复原 PLAN 的后续队列。

## 2. 固定事实

- 原版剧情 VM 有 198 个显式 opcode，对应 146 个唯一汇编 handler 入口。
- 其中 25 个入口由多个 opcode 共享；共享入口不代表各 opcode 语义相同。
- 当前 C++ 接入 95 个显式 opcode。
- 当前资产静态控制流观察到 143 个 opcode，其中仍有 68 个尚未实现。
- 另有 55 个 opcode 未在当前资产静态控制流中观察到；未观察不等于不可达或可以删除。
- `0..124` 已有人工汇编语义；`125..193` 目前只有分派、长度和保守 CFG，尚不能直接翻译为 C++。
- 当前已实现的 95 个 opcode 不继承完成状态，必须随所属 handler 组重新审计和验证。

## 3. 固定决策

剧情 VM 的完整范围一次锁定为 `sub_427920` 的全部 198 个显式 opcode、默认非法分支、
表外特殊值、窗口切换和公共解释循环。停止按单条剧情运行轨迹“遇到一个补一个”。

实现和验证单位是 146 个唯一汇编 handler 入口及其共享 opcode 组，不是孤立 opcode，
也不是机械编号段。每处理一个入口，必须把该入口下已经实现和尚未实现的全部 opcode
变体一起逆向、实现和验证。

“完整范围一次锁定”不等于连续写完 198 个 opcode 后统一测试，也不等于先把全部语义
研究完再开始编码。执行方式固定为按 handler 组边逆向、边实现、边验证，当前组完全
收敛后再进入下一组。

P0、P1、P2、P3各自都是独立大阶段；每个阶段完成时都必须执行并通过一次Windows LLVM
`app`完整门禁，前一阶段或当前小工作包的结果不得替代后一阶段。没有对应Windows通过证据时
不得宣告该阶段完成；门禁发现的问题必须修复并重跑至通过。

## 4. 执行顺序

### P0 · 有限收口 B7

在正式进入剧情 VM 模块前，只进行一次有限的 B7 收口：

1. 以原 PLAN 和 `analysis/04-reverse-engineering/modules/world-map.md` 锁定的 114 个
   world-map 地址为全集，建立逐项闭环结果。
2. 每项只能归为已实现、明确跨模块转交或有汇编依据的不可达；不得仅凭既有叙述视为完成。
3. 补齐 B7 自身的真实缺口，包括 PATH VM 尚未恢复的分支。
4. 达到原 PLAN 的 B7 模块移交条件后立即结束 P0；不继续扩大世界模块，不继续首场战斗，
   不再以 `TALK100` 的下一条未实现 opcode 作为工作边界。

P0 已完成：world-map 锁定全集 114/114 已逐项归档为 `44 assembly_exact + 70
platform_adapted + 0 pending_audit`；Linux core 186/186、Linux app 192/192、Windows
LLVM app 192/192 完整门禁通过。P1 从此边界开始，不继承任何 VM handler 的完成状态。

### P1 · 建立完整剧情 VM 工作包

1. 锁定 198 个显式 opcode、146 个唯一 handler 入口、25 个共享入口组、默认非法分支、
   表外特殊值、窗口切换和公共继续/让出路径。
2. 保留现有分派、长度、静态控制流和 `0..124` 人工语义成果作为审计导航；任何既有结论
   在所属 handler 组实施时仍须重新对照 LST。
3. 为 `125..193` 及相关特殊值补齐人工汇编语义，但只随即将实现的 handler 组推进，
   不先进行脱离实现的全量研究。
4. 明确每个 handler 对 world-map、story-scene、special-modes、battle、rendering、
   asset-runtime 和 audio-video 的端口依赖。

P1 已完成：dispatch 生成器已改为锁定完整 LST SHA-256 并从 LST label/机器字节重建两张
一级表、两张内部跳表和 157/73 byte selector；重跑后 198 行 dispatch、146 个入口组和
2 个 internal switch 与旧基线逐字节一致。新增 `story-vm-handler-workpack.tsv` 的 146
行全部从 `pending_audit` 开始，25 个共享入口、50 个现代 case label、初始125行旧语义、
143/55 资产观察及候选端口仅作导航；`story-vm-runtime-paths.tsv` 另锁定17条默认、特殊
值、窗口、公共 join/yield 与返回路径。P1边界提交`a24145a`已在隔离worktree补跑Windows
LLVM app完整门，192/192以exit0通过且未启动游戏EXE。P2当前人工语义已随审计增至128行。前73行已独立
关闭：默认非法与共享对话两组、opcode7/9的bit31/bit30 clear、opcode8 lifetime、opcode10/11
 action、opcode12 position、opcode13 role step、opcode14 action wait、opcode15 same-file jump、
opcode16/17两种role-path conditional jump、opcode18/19 path release、共享opcode20/169批量path
schedule、共享opcode21/22全局bit条件跳转、opcode23/24全局bit列表all/any跳转、
opcode25/26全局bit set/clear、opcode27世界session同步重载、opcode28角色Path id修改与对象协调、
共享opcodes29–33全局整数set/add/sub与无符号条件跳转、opcode34有界脚本时钟设置、
opcode35脚本时钟低位字节条件跳转、opcode36脚本时钟相对快照条件跳转、opcode37脚本时钟快照、
opcode38角色场景清除与MAPS fallback、opcode39角色bit15标记/surface清除/one-shot复位、
opcode40角色重定位/路径完成/MAPS坐标fallback、opcode41共享selector索引目标并重载TALK窗口、
共享opcodes42/43设置/清除dialog counter共用的interaction lock并由42重置受控角色base variant、
opcode44设置角色action wait override并清wait remaining、opcode45切换角色action id、合并紧邻
同角色action指令refresh并为missing角色patch MAPS action/flags、共享opcodes46–49恢复pending
角色action字段/条件应用base/delta override/设置wait override sentinel，共享opcodes50/70/73
按relative target、absolute target或role-centered viewport启动镜头移动，opcode51按四个
movement/step状态等待镜头移动完成并在两路发布previous，opcode52按机器阶段顺序初始化
三通道画面渐变、复现x87除法/零时长位型并同调用继续，opcode53按signed countdown等待
三通道渐变完成并在两路发布previous，opcode54对角色动作先初始化刷新、再按signed repeat
重复刷新且在每轮刷新后清辅助字段，共享opcodes55–57保存旧空间分组、写入新分组并完成角色
空间链解链重插，以及共享opcodes58/153按分配、清零、初始化、四项operand写入顺序分别前插主/次
图片动作链并发布previous后让出，opcode59按当前混音等级提交居中单次音效、忽略后端结果并发布
previous后让出，共享opcodes60/61共同先清场景clear-only bit、由61清零完整framebuffer后重新
置bit、由60直接恢复世界场景并在两路发布previous后让出，以及opcode62先清理旧运行角色、按八项
独立继承规则写MAPS角色源，仅对当前map即时替换/追加运行角色、恢复空间链并保留四粒子槽全填bug，
最后发布previous并同调用继续，以及opcode63按FF00终止扫描写入64-word选择滚动表、以CFCF清空尾项、
同步prefix到interval/remaining并快照left/top；成功同调用继续，超56项原地yield重试，以及opcode64
只把64-word选择滚动表清为CFCF，保留interval、remaining、cursor与视口快照后同调用继续，以及
opcode65按raw selector把命中角色经共享helper转入队伍、同步post/live队伍状态，缺失角色静默消费，
两路均发布previous并yield，以及opcode66零扩展七参数：缺失角色清MAPS bit7，命中角色更新运行时/
MAPS/surface并移除物理party槽，按原版忽略已完成诊断后yield，以及opcode67以脚本bit15保存两阶段、
按accepted-frame u32回绕时钟严格`elapsed>duration`完成，并在初始化/等待/完成三路发布previous，
以及opcode68先解析FFF0，命中角色时清flags bit0400，缺失时以替换后的GUID提交MAPS AND FBFF请求，
opcode69则独立置flags bit0400并提交MAPS OR 0400请求，以及opcode71按raw selector命中后写HeadSgn
slot token，missing不读slot且两路yield；opcode72则以同样raw lookup清field3C，missing静默且两路yield。
opcode74按RGB step与countdown顺序清零，保留current/target并same-call继续。对外进度为
opcode75复用`sub_42E5A0`等价helper取得角色路径所有权，missing index -1越界收敛为typed失败。
opcode76按LST分阶段双lookup，完成朝向action刷新后再挂起；两处index -1越界收敛为typed失败。
shared opcodes77/78按selector命中后分流固定6/4字节，更新等待覆盖并刷新；missing陈旧宽度收敛为typed失败。
opcode84控制首匹配packed-row效果，保持staged读取、op0/1/2、缺失推进；真实3/8陈旧局部typed-stop。
opcode85恢复clear/present/audio/CD preflight/`%Q`解析/begin顺序、preflight不消费与两路previous/yield，
修正合法terminator精确结束在`0x8000`误判失败；SDL以配置root和typed video backend适配CD/Bink。
opcode86按完整32位头像节点key与零扩展u16旧键匹配，只改首个exact；保留空链/ID miss不读后项、
new ID先写再读new variant的staged unsafe点、+10、previous86与same-call。
opcode87按FF00FF00表、secondary two-raw rejection RNG选择同文件target，恢复audio/IP0/窗口替换与
same-call；空表原unsigned DIV0、缺sentinel、owner和load失败均按阶段typed-stop。
opcode88按packed-row→role-head→signed operand顺序清两链并提交战斗request，保持移动链、previous与yield；
释放owner、operand和request owner失败均保留此前已完成副作用。shared opcodes91/162按显式u16或变量11/12
完整u32取得MAPS姓名record index，共享u32目录回绕、32-byte copy、首个`%Q`终止、固定buffer姓名替换、
previous与same-call；非法/zero dynamic selector只消费，目录和terminator unsafe点按阶段typed-stop。
opcode92按u16零扩展后的u32 dec/+30定位保留全局bit，保留selector0回绕与invalid-safe继续写；
0x400-byte owner外裸写在selector8163起按原访问点typed-stop。opcode93独立以相同索引公式调用clear
helper，按`FF-mask`只清单bit；selector0、invalid-safe和owner边界均独立锁定。opcode94按原dword
OR2、+2、previous与yield修复旧combined case漏发布，并映射到已集成u8低位scene-runtime owner。
已实现107/198、已验收98/198；内部workpack为73/146，即
`8 assembly_exact + 65 platform_adapted + 73 pending_audit`。下一行只审计`0x0042A7EE`下的opcode95。

### P2 · 按 handler 组逆向、实现和验证

每个 handler 组必须完整执行以下循环：

```text
不看现有 C++，按 LST 独立恢复入口及全部 opcode 变体
→ 从汇编独立推导参数、分支、边界和状态测试
→ 重新审计该入口下已有 C++ 实现
→ 一次补齐该入口下全部 opcode 变体
→ 连同 helper、全局状态和相邻 opcode 合同验证
→ 汇编到 C++、C++ 到汇编双向逐基本块追溯
→ 单指令、组合指令和真实 TALK 数据回归
→ 零差异、零未决后关闭该 handler 组
```

每组必须验证：参数宽度与符号扩展、IP 推进或改写、自修改指令、同帧继续、跨帧让出、
等待条件、调用顺序、状态副作用、异常出口和原始 BUG。发现差异时按照原 PLAN 的收敛
规则从入口重新验证，不能只修当前实机命中的分支。

依赖菜单、商店、战斗、音视频等尚未完成模块的 opcode，必须完整实现 VM 侧的参数解析、
状态修改、请求和等待合同，并通过窄端口和测试替身验证。外部模块未完成只能阻塞端到端
运行验证，不能免除 handler 实现，不能作为关闭 handler 组的替代证据，也不能伪造成功。

### P3 · 全 VM 验收

全部 handler 组关闭后统一验证：

1. 全部 198 个显式 opcode 均已完成 VM 侧实现；当前资产未观察到的 opcode 也不得省略。
2. 默认非法分支、表外特殊值、窗口切换和公共解释循环均已完成实现和验证。
3. 全部 TALK 数据可按真实窗口和跳转规则遍历，不存在未知 opcode、错误长度或意外越界。
4. 跨 opcode 状态、自修改、等待、同帧继续、跨帧让出和窗口切换组合测试通过。
5. 真实剧情长序列不再因 `unsupported_opcode` 停止。
6. 需要原程序动态值时准备 Frida 工具并等待用户运行；未经用户许可不启动原版。

## 5. 明确禁止

- 不再按当前剧情命中顺序逐个补 opcode。
- 不按 198 个编号机械堆完代码后统一测试。
- 不先无限研究全部 handler，再推迟所有实现。
- 不把保守 CFG、IDA 名称、伪码或测试通过单独视为汇编语义完成。
- 不把当前资产未观察到等同于不可达、无须实现或可以删除。
- 不把外部模块阻塞当成 VM handler 未实现的理由。
- 不为推进剧情而伪造菜单、商店、战斗、音视频等外部 owner 的成功结果。
- 本 PLAN 执行期间不并行扩大首场战斗或回到延期的 `libffmpeg` 后端。
- opcode 在 C++ 控制流中不得裸写数字 case 或跨指令比较：语义已独立收敛时使用 `OP_<编号>_<语义>`；尚未审计或语义不明时只使用 `OP_<编号>`，待该 handler 闭环后再重命名。

## 6. 完成与退出

本 PLAN 仅在以下条件全部满足后完成：

- B7 已按原 PLAN 的模块移交条件有限收口；
- 剧情 VM 全部 146 个 handler 组已经逐组重新审计并关闭；
- 全部 198 个显式 opcode 已完成 VM 侧实现；
- 默认非法分支、表外特殊值、窗口切换和公共解释循环完成全 VM 验收；
- 不存在 VM 自身未恢复、未验证、按剧情临时绕过或以外部阻塞替代实现的行为。

原程序动态差分或尚未完成外部模块可以保留为原 PLAN 允许的已登记阻塞，但不得降低上述
VM 实现和静态/离线验证完成条件。满足全部条件后，本文件停止优先级覆盖，返回
[`execution-plan-pi.md`](execution-plan-pi.md) 的后续模块队列。
