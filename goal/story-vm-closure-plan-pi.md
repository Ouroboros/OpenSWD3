# 剧情 VM 完整闭环追加 PLAN

状态：执行中，P0/P1 已完成，当前步骤 P2；下一handler `0x0042CDED`（opcode175）

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
- `0..144`、`147..152`、`155..168`及`170..173`已有人工汇编语义；其余opcode目前只有分派、长度和保守 CFG，尚不能直接翻译为 C++。
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
LLVM app完整门，192/192以exit0通过且未启动游戏EXE。P2当前人工语义已随审计增至151行。前112行已独立
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
opcode95独立按`AND FFFFFFFD`只清bit1，补齐previous95并保持同一owner合同与yield。
opcode96按机器先写interval70，再解析可选`%/*`与32-byte临时copy域内首个`%Q`；消费后保留
一次audio/preflight result2直接返回，或第二次audio/actual start/common previous96/yield。SDL以配置
root、case-insensitive `Video/`和真实`LegacyAniActivity` start适配CD/Win32文件/DirectDraw；最终复审
确认scene live owner属于异步ANI更新，不向VM插入staged读取。16条真实记录及ANI archive/activity
backend均通过；world-frame ANI stage由opcode97包接通。
opcode97只查询同一activity active extent：active原地发布previous97/yield，inactive推进2、恢复interval35、
发布previous97并same-call。SDL composition现执行实际activity update，映射三blocker、同步scene/process、
执行ending RGB和finalize；normal ending以start前world snapshot适配递归scene redraw。14条真实记录、四alias、
active/inactive、精确尾和normal same-call均通过。
opcode98不读取名义payload，只推进4、发布previous98并yield；四alias、完整精确尾和仅剩opcode两字节
仍完成的未读payload尾均锁定。3条真实记录/3 probes全部raw0062，payload`0190/006C/0001`不参与行为。
opcode99把live ANI phase按signed i32与zero-extended u16 threshold严格比较；等待yield、完成same-call两路
发布previous99，且不增加active检查。139条真实记录、负启动相位、u16最大阈值、staged operand及精确尾通过。
opcode100在lookup前staged读取selector与Talk值；FFF0映射current source、FFFE沿用controlled helper。live路径只写
`talk_script_id`，missing路径通过typed MAPS database port提交Talk-only patch，并保持OR0/AND FFFF、+6、
previous100与same-call。192条真实记录、四alias、FFFF值、双operand截断及精确尾通过。
opcode101只读取selector；FFF0映射current source、FFFE沿用controlled helper。命中live role只OR flags bit26，
missing静默；两路保持+4、previous101与same-call。126条真实记录、四alias、bit28 skip首匹配、截断及精确尾通过。
共享opcodes102/103/117/136/140/145/146/174按内部跳表选择八个mask；FFF0把controlled index作GUID key，
live路径clear/set flags后依次clear/mark surface，missing路径提交布尔AND/OR MAPS patch。683条记录/685 probes；
145/174零记录由asset absence、全alias/boolean synthetic与精确尾锁定。
opcode104先清text bit28，再分阶段sign-extend写两项i16布局值；修复旧case一次性预检与漏发previous104。
125条真实记录、四alias、i16边界、两级截断、精确尾及dialog消费链通过。
opcode105只清text control bit27；806条真实记录、四alias、其他位保留、same-call及精确尾通过。
共享opcodes106/154按normalized opcode选择主/副picture-action链，空链完成，非空读取首节点typed action
packed word高字节并严格等待其大于u16 threshold。106锁定60条记录/63 probes；154以asset absence、
双变体全alias、链选择、严格边界、operand/runtime顺序及精确尾锁定；opcode107按selector→lookup→threshold→
action顺序读取角色动作packed word，threshold高于低字节上限或lookup失败时消费，合法且高字节低于threshold
时发布previous107并等待，其余发布previous107并同帧继续。107锁定294条记录/296 probes，四alias、FFF0/FFFE、
无符号边界、两阶段截断及精确尾通过。opcode108分阶段写one-shot dialog anchor X/Y，只有两项均已写入
后才按u16独立把`X>639`/`Y>479`替换为16；成功+6、previous108、same-call。108零资产，以四alias、
边界、两阶段截断、精确尾和下一dialog消费/重置锁定。opcode109按count逐项lookup并调用既有角色路径
步进owner，missing静默，成功发布previous109、audio service并yield；67条真实记录及分阶段失败通过。
共享opcodes110/111从角色1起扫描bit30，并按互反条件选择同文件target重载或不读target的顺序消费；
opcode110零资产，opcode111的24条记录与双路径回放通过。opcode112按packed-row→role-head顺序短路
检查两条action链；等待与完成都发布previous、service audio并yield，moving-action链不参与。9条真实
记录及等待/完成双路径回放通过。opcode113只读取u16 sound id并固定消费未读padding，复用sample
wrapper且播放结果不参与流控；线性资产零记录，以asset absence和synthetic锁定。opcode114按request、
双stream ID、既有transition、control bit23、flags派生的分阶段顺序提交场景音乐请求，并接入实际
stream manager；157条真实记录/159 probes及same-call合同通过。opcode115按u16零扩展后只夹上限11，
复用实际stream100音量wrapper并忽略返回后yield；线性资产零记录/零probes，以asset absence和synthetic锁定；
Linux core 186/186和app 192/192通过。opcode116冻结入口count逐项调度角色位置，末尾重读count并
same-call继续；30条真实记录/30 probes、117个子记录及代表回放通过，Story VM 3/3及Linux
core186/186、app192/192通过。opcode118按消息raw role index映射GUID并删除全部匹配对话，
按机器顺序清角色gate、释放双owner/节点并递减低15位计数；1669条真实记录/1669 probes及
连续GUID 0→1回放通过，Story VM 3/3及Linux core186/186、app192/192通过。
shared opcodes119、139共享selector与首匹配消息扫描，并分别等待flags bit0/bit15；850条真实
记录/850 probes及双variant跨帧回放、Story VM 3/3、Linux core 186/186、app 192/192均通过。
opcode120按lookup结果分流：live角色分阶段写三个动作字段后刷新并置状态位，missing角色提交
raw MAPS patch；800条真实记录/808 probes及双路径回放、Story VM 3/3、Linux core 186/186、
app 192/192均通过。opcode121只清text-control bit26并same-call继续；815条真实记录/815 probes、
四库精确尾、Story VM 3/3、Linux core 186/186和app 192/192通过。opcode122清零world/player/dialog
共享的进程级速度模式并same-call继续；7条真实记录/7 probes、Story VM 3/3、Linux core 186/186和
app 192/192通过。opcode123按三层相对链首匹配更新Scene_Music表项，成功保留未读`+8`与分阶段
部分提交；当前MAPS表314项，71条真实记录/71 probes及Story VM 3/3通过。已实现138/198、
已验收134/198；内部workpack为99/146，即`14 assembly_exact + 85 platform_adapted + 47 pending_audit`。
opcode123 Linux core 186/186与app 192/192通过。opcode124只清text-control bit25并same-call继续；
完整线性TALK目录零记录，以asset absence和synthetic锁定。已实现139/198、已验收135/198；内部
workpack为100/146，即`15 assembly_exact + 85 platform_adapted + 46 pending_audit`。opcode124 Linux
core 186/186与app 192/192通过。opcode125恢复进程期文本分配链尾插、`%Q`动态记录、IP先于suffix
提交和audio-yield；零资产以asset absence与synthetic锁定。已实现140/198、已验收136/198；
Linux core 186/186与app 192/192完整门通过。内部workpack为101/146，即`15 assembly_exact +
86 platform_adapted + 45 pending_audit`。shared opcodes126/127按完整u32角色base variant与u16
expected执行互反条件；taken-only target重载、live-miss MAPS临时物化、previous和same-call均已锁定。
opcode126有236条真实记录，127零资产。Story VM 3/3、Linux core 186/186和app 192/192通过；opcode128
恢复玩家库存mode0的signed delta、双数量搬运/删除、定义节点前插及固定yield，397条真实记录通过。
MON定义以窄端口转交B9，SDL不伪造成功。Story VM 3/3、Linux core 186/186和app 192/192通过；shared
opcodes129/130/167/168恢复玩家普通库存与64槽角色root的四种存在谓词、taken-only重载和+8
same-call；48条真实记录与四variant回放通过。Story VM 3/3、Linux core 186/186和app 192/192通过；opcode131
恢复四队伍哨兵链的masked预查、mode1 upsert、definition flag与资格位门，以及不合格时的两段mode0减一；
MON仍以窄端口转交B9，零资产以synthetic锁定。Story VM 3/3、Linux core 186/186及app 192/192通过；opcode132
恢复玩家item与四组角色前12槽的root交换、双快照和两次mode0库存更新；raw next交叉别名及说明泄漏以明确
RAII/list平台隔离承接，零资产以synthetic锁定。Story VM 3/3、Linux core 186/186与app 192/192完整门
均以exit 0通过；opcode133恢复零终止u16商品ID列表owner替换与模式2请求；SDL接实际special-mode owner，
B9只负责后续商品链物化。22条真实记录与代表回放、Story VM 3/3、Linux core 186/186与app
192/192完整门均以exit 0通过；opcode134恢复四项角色资源的三段u16回绕加法、i16夹值、first耗尽
自修改下一word为opcode144、transient清理与same-call；B10/B11剩余字段加载不伪造，opcode144独立
保持pending。47条真实记录、四项恢复链和损伤自修改链回放、Story VM 3/3、Linux core 186/186与
app 192/192完整门均以exit 0通过；已实现151/198、已验收147/198；内部workpack为108/146，即`15 assembly_exact + 93 platform_adapted +
38 pending_audit`。opcode135恢复四项special/high-priority状态写、`sub_406D30`输入菜单与存档预览
重置调用、IP/audio/previous/yield顺序；四项实际owner已接线，跨B9/B11 helper以可失败窄port明确延期，
SDL不伪造成功。零资产以asset absence与synthetic锁定，Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过；
已实现152/198、已验收148/198；内部workpack为109/146，即`15 assembly_exact + 94 platform_adapted +
37 pending_audit`。opcode137恢复bit23双路stream transition、双三dword音乐槽组切换、flags mask与无audio
的previous/same-call合同；VM补齐world三槽，SDL transition复用实际stream manager，Win32空诊断scratch以
平台适配省略。60条真实记录、四库代表回放与synthetic通过，Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过；
已实现153/198、已验收149/198；内部workpack为110/146，即`15 assembly_exact + 95 platform_adapted +
36 pending_audit`。opcode138恢复selector-first角色解析、i32回绕平方/x87距离、strict radius谓词与taken-only
同文件重载；FFF0/FFFE语义、分阶段尾和load失败均已锁定。117条真实记录及四库代表回放、synthetic与
Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过；已实现154/198、已验收150/198；内部workpack为111/146，
即`15 assembly_exact + 96 platform_adapted + 35 pending_audit`。opcode141恢复mode/pending两项分阶段
u16配置、current fade保留、previous与same-call，并修复旧裸case整段预检；124条真实记录、四库代表
回放与synthetic通过，Story VM 3/3、Linux core186/186与app192/192完整门均通过。已实现154/198、已验收151/198；
内部workpack为112/146，即`15 assembly_exact + 97 platform_adapted + 34 pending_audit`。opcode142复用已闭环
倒计时初始化器并直连普通世界实际owner，恢复u32回绕、双aux清零、双flag顺序及previous/audio/yield；
2条真实记录与synthetic通过，Story VM 3/3、Linux core186/186与app192/192完整门均通过。已实现155/198、已验收152/198；
内部workpack为113/146，即`15 assembly_exact + 98 platform_adapted + 33 pending_audit`。opcode143恢复
primary ticks停用、16→76→18三flag清除及previous/audio/yield；4条真实记录与synthetic通过，Story VM 3/3
通过，Linux core186/186与app192/192完整门均通过。已实现156/198、已验收153/198；内部workpack为114/146，即
`15 assembly_exact + 99 platform_adapted + 32 pending_audit`。opcode144独立恢复mode4/5低字节选择、
helper前后五项分阶段状态、未读padding及previous/audio/yield；1条真实记录与synthetic、opcode134
自修改组合链通过，Story VM 3/3、Linux core186/186与app192/192通过。已实现157/198、已验收154/198；
内部workpack为115/146，即`15 assembly_exact + 100 platform_adapted + 31 pending_audit`。opcode147固定
置共享剧情flag70并恢复previous/audio/yield；32条真实记录、四alias、幂等位隔离和精确尾通过，Story VM
3/3、Linux core186/186与app192/192通过。已实现158/198、已验收155/198；内部workpack为116/146，即
`16 assembly_exact + 100 platform_adapted + 30 pending_audit`。opcode148固定置共享剧情flag19并恢复
previous/audio/yield；零资产以asset absence和synthetic锁定，Story VM 3/3、Linux core186/186与app192/192通过。
已实现159/198、已验收156/198；内部workpack为117/146，即`17 assembly_exact + 100 platform_adapted +
29 pending_audit`。opcode149固定清共享剧情flag19并恢复previous/audio/yield；零资产以asset absence和
synthetic锁定，Story VM 3/3、Linux core186/186与app192/192通过。已实现160/198、已验收157/198；内部workpack
为118/146，即`18 assembly_exact + 100 platform_adapted + 28 pending_audit`。opcode150复用普通世界
ANI follower实际owner，恢复signed坐标缩放夹值、不可达sentinel原始bug、targetX/targetY差异及双速度
清零；零资产以asset absence和synthetic锁定，Story VM 3/3、Linux core186/186与app192/192通过。
已实现161/198、已验收158/198；内部workpack为119/146，即`18 assembly_exact + 101 platform_adapted + 27 pending_audit`。
opcode151复用同一ANI follower owner，恢复两项目标坐标缩放和两项signed速度的四阶段提交；零资产
以asset absence和synthetic锁定，Story VM 3/3、Linux core186/186与app192/192通过。已实现162/198、已验收159/198；
内部workpack为120/146，即`18 assembly_exact + 102 platform_adapted + 26 pending_audit`。下一行只审计
opcode152复用同一ANI follower owner，恢复X优先短路、两级等待、完成推进及三路previous/audio/yield；
零资产以asset absence和synthetic锁定，Story VM 3/3、Linux core186/186与app192/192通过。已实现163/198、
已验收160/198；内部workpack为121/146，即`18 assembly_exact + 103 platform_adapted + 25 pending_audit`。
opcode155恢复map22 no-op、deferred三写和固定参数同步reload，并补齐共享loader完整dword tile、低16持久
字段及陈旧空间绑定合同；零资产以asset absence和synthetic锁定，Story VM 3/3、runtime-session 2/2、
Linux core186/186和app192/192通过。已实现164/198、已验收161/198；内部workpack为122/146，即`18 assembly_exact +
104 platform_adapted + 24 pending_audit`。opcode156恢复deferred map signed门、完整dword tile同步reload、
成功后三项清理及失败保留；唯一线性TALK记录、synthetic、Story VM 3/3、Linux core186/186和app192/192通过。
已实现165/198、已验收162/198；内部workpack为123/146，即`18 assembly_exact + 105 platform_adapted +
23 pending_audit`。opcode157恢复map22未读X/Y特例、non22三项i16 staged写和固定+8 same-call；
零资产以asset absence和synthetic锁定，Story VM 3/3、Linux core186/186、app192/192通过。已实现166/198、
已验收163/198；内部workpack为124/146，即`19 assembly_exact + 105 platform_adapted + 22 pending_audit`。
shared opcodes158/159恢复自身prefix未读、`%Q`扫描、下一指令operand目录lookahead、host-frame门、
copy/delete双路径及文件API失败仍previous/same-call；SDL以配置data/launch roots和显式后台运行例外承接
CD/Win32路径及激活门。两opcode零资产，以asset absence和synthetic锁定，Story VM 3/3、Linux
core186/186及app192/192通过。已实现168/198、已验收165/198；内部workpack为125/146，即`19 assembly_exact +
106 platform_adapted + 21 pending_audit`。opcode160前置审计另发现共享对话handler误把`dword_4CF73C`
建模为水平居中bool；独立fix已恢复非1置record bit18、等1抑制、成功清零与失败保留，删除无LST依据的
半宽扣减。Story VM 3/3、Linux core186/186及app192/192通过，完整门lifecycle exit 0；该fix不改变
opcode/workpack计数。opcode160恢复完整下一对话控制dword写1、+2、previous与same-call，无audio；
下一成功对话抑制record bit18并清零，失败保留。558条真实记录/558 probes及三类对话后继链已锁定，
两条代表链与synthetic通过。Story VM 3/3、Linux core186/186及app192/192完整门均以exit0通过。
已实现169/198、已验收166/198；内部workpack为126/146，即`20 assembly_exact + 106 platform_adapted + 20 pending_audit`。
opcode161恢复signed story ID、四次audio、TALK文件/目录/窗口切换、IP0、previous与same-call；现代RAII
资源port保持未覆盖窗口尾，并以typed load failure承接Win32同步退出后的陈旧物理指针失败域。90条真实
记录/90 probes、89个目标ID及TALK2/3/4的87/2/1分布已锁定，全部目标首opcode为1026；真实story2037
同调用进入TALK2并提交sound193。Story VM 3/3、Linux core186/186和app192/192通过。已实现169/198、
已验收167/198；内部workpack为127/146，即`20 assembly_exact + 107 platform_adapted + 19 pending_audit`。
opcodes163–164前置审计发现共享current logical map owner及session load request被错误收窄为u16，
并暴露已提交opcode155把固定map22/tile59重载误译为current map/deferred tile。独立fix已恢复完整u32
map owner、SDL无截断绑定、opcode155固定22/59/59、opcode156完整positive deferred map转发，以及
opcode62明确低16 MAPS边界。Story VM 3/3、相关依赖8/8、Linux core186/186及app192/192
完整门均通过；该fix不改变opcode/workpack计数。
shared opcodes163/164恢复signed map operand与完整current map dword的反向谓词；taken-only读取
u32 target并audio、同文件IP0重载、previous/same-call，not-taken不读target且固定+8 same-call。
27条真实记录/27 probes全部基础raw、长度8，163/164为3/24，所有target首opcode均为1026；两种
谓词代表回放和synthetic通过。Story VM 3/3、Linux core186/186及app192/192完整门均通过。
已实现171/198、
已验收169/198；内部workpack为128/146，即`20 assembly_exact + 108 platform_adapted + 18 pending_audit`。
shared opcodes165/166恢复玩家masked链与64个角色完整ID root的signed数量总和；非零总和
分别按`>=`/`<=`阈值，零总和保留165固定sequential、166固定reload特例。taken-only读取target
并同文件IP0重载，not-taken固定+10 same-call。真实资产锁定TALK4三条opcode165、零条opcode166，
三target均有效，代表等号taken回放与synthetic通过。Story VM 3/3、Linux core186/186及app192/192完整门均通过。
已实现173/198、已验收171/198；内部workpack为129/146，即`20 assembly_exact + 109 platform_adapted + 17 pending_audit`。
shared opcodes170–173恢复mode17/18两个nullable 52字节文本owner：偶数变体清owner/剧情位，
奇数变体扫描`%Q`、按首NUL复制并置位；四路均previous、audio一次并yield。缺terminator和
第52字节复制危险点保留已提交副作用后typed-stop。真实资产21条/21 probes，四opcode分布
9/3/5/4，四库分布8/4/5/4；四种代表回放与synthetic通过。Story VM 3/3、Linux core186/186
及app192/192完整门均通过。已实现177/198、已验收175/198；内部workpack为130/146，即
`20 assembly_exact + 110 platform_adapted + 16 pending_audit`。
下一行只审计`0x0042CDED`下的opcode175。

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
