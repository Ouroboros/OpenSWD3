# 战斗角色动作主分派 `0x004539B0`

状态：`pending`。已废弃GPT重建实现，本轮逆向尚未开始。

以下保留历史分析与证据；旧结论、完成记录和测试结果不作为本轮验收。
原版采集事实保留，所有解释和实现结论须重新核实。

## 历史资料（重新核实前不生效）


历史状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

B11动作里的颜色写入、primary抑制门与刷新调用已借实际画面控制存储。
动作4/404和405在刷新返回后检查当前三色，配置与历史快照保持独立。
原写集合、上游零/一来源与验证见
[双门与实时颜色](battle-frame-effect-00453580.md#20-b11双抑制门与当前颜色共用实际存储)。

## 召唤共享全局镜像设置

召唤读取脚本共用的`startup.mirror_mode`，删除独立summon_gate副本。
保留完整DWORD零值比较、前继后实时读取与分配失败前缀。
验证见[共享镜像设置](battle-shared-mirror-setting-0053c000.md)。行动父协议仍待迁移。

## 灵契读取实际敌人参数

灵契在查询次数前、刷新之后和完成激活之后分别读取实际敌人的无符号WORD。
删除三处通用查询、查询编号和模拟回复；参数改变时后继读取当前值，不能复用首读值。
数量20门槛、参数001C绘制抑制、完成累计1及失败前缀保持原顺序。
89组新增执行及原激活/抑制组合的迁移见
[灵契参数读取](battle-spirit-contract-profile-0047f910.md)。该行动父协议继续pending。

## 动作直接消耗实际队员资源

5ad2a9aa回收453F5B/45544C资源消费，删除脚本兼容回复、操作编号、子寄存器结果和计数。
动作27原4728E0完整返回1才消费；初稿误记0已由入口EBX=1纠正，生产比较1保持。
最终WORD来自同一party链及workspace，正常零继续，真实失败阻断未执行后缀。
40次新动作向量、相关菜单/列表、两core/两ASan和SDL通过，见
[实际资源消费](battle-actor-resource-consumption-00470e20.md)。
两动作的其它callee、数组、寄存器请求回复和父Port仍待迁移。

## 1. 完整LST范围

权威主体为`0x004539B0..0x00455CCF`，完整3836行、202个静态call站点、137个`loc_`标签。函数另有不可遗漏的外部FUNCTION CHUNK：

```text
0x00498350..0x00498364
size = 0x15 bytes
18 LST lines
1 call
loc_498350 + SEH_4539B0
```

完整函数因此包含203个call站点。外部chunk在构造deformation对象异常时
释放新分配的本体，再跳MSVC异常展开。当前实现直接拥有实际对象；
构造异常回收本体后继续抛出，不清除已经写入的活动标记。
分配/释放端口及伪指针已删除，详见
[变形对象所有权](battle-action-deformation-owned-storage.md)。

ABI为两个参数cdecl/plain `retn`：第一参数选择角色组A槽，第二参数按动作语义选择角色组B或另一角色槽。唯一caller位于尚未关闭的`0x00456680`；当前caller不提前计数。

## 2. 入口与次级动作号

入口按低32位乘法建立角色组A物理token：

```text
0x005029D0 + group_a_index * 0x2F34
```

首次对象访问直接借队员实际动作WORD，查询仅返回状态及可选值，已删除编号、token解析、寄存器协议、栈轨迹和计数。随后调用角色终止查询；完整EAX等于1时立即返回1，第二参数完全不访问。动作字段或RET失败保留父级原索引算术值或已替换低WORD，阻断终态、显示查询和全部动作后缀。当前证据与门禁见[动作种类查询](battle-actor-action-kind-004786b0.md)；父级其余协议和资源仍待迁移。

主动作号为0且终态完整值不等于1时，直接读取队员实际显示WORD，查询仅返回状态与可选观察值。显示零时返回1；非零时原`00453A42 DEC EDI`把仍为0的动作种类减为FFFFFFFF，随后无符号JA走默认尾并返回0。显示值不替换动作编号，旧“主0、显示5必须进入case5”的实现和测试结论撤销。字段/RET失败保留动作查询、终态及其副作用；父级失败EAX分别保留完整前驱或已替换AX。查询的寄存器请求回复、token解析、栈轨迹和计数已删除；当前依据见[显示种类查询](battle-actor-display-kind-004786c0.md)。

角色组A索引越界只在首次typed动作种类word查询点停止。第二参数不做入口预验；每个case只在首次实际组A/组B对象、表或状态槽访问时检查。

## 3. 动作99终态

动作`0x63`不走jump table。先执行画面模式调用，再直接清保存敌人的镜像DWORD，随后：

- stored group-B index写`0xFFFF`；
- stored group-A index写`0xFFFF`；
- current actor写`0xFFFF`；
- result mode写1；
- battle submode写2；
- 返回1。

## 4. 大动作号前分派

### 4.1 动作100、200、300

- 100：直接返回1；
- 200：按side mode选择组A或组B对象，调用单对象更新，返回0；
- 300：同样按side mode选择对象，退出callee返回1时清current actor并返回1，否则返回0。

### 4.2 动作400与402

400调用普通目标准备；402先把红绿蓝factor都写-12、primary suppression写1、alternate surface mode写1，再调用另一目标准备。成功公共前缀：

- action pending写1；
- packed action只覆盖高word为目标索引；
- target identity写目标索引；
- action pending auxiliary清零；
- actor effect score低32位加2。

blocking effect为0且视觉commit完整EAX等于1时：选择目标、发布目标identity、accumulator写全1、frame refresh写1、清framebuffer、延迟300、effect score再加5。公共尾清accumulator、清pending action、延迟300并返回0。

### 4.3 动作404三相状态

共享packed dword拆为typed低word phase与画面效果高word split extent，组合bit pattern仍等价。

- phase低15位0：低word写1；遍历全部组A，除当前角色外只对“suspended query不等于1”的角色按4、64顺序push；再遍历全部组B按64、4顺序push；
- phase低15位1：packed值形成`0x00010002`，即phase 2、split extent 1，同时split suppression写1；
- phase低15位2：目标准备成功后执行动作400同形commit前缀与score；随后清accumulator、phase、extent、split suppression，清pending action、延迟300；组A除当前角色外按4、64顺序pop，组B按4、64顺序pop。

同一次调用可按原独立`if`连续穿过0→1→2；不改成每帧只前进一步。循环计数超过组A 10槽或组B 8槽时，在首次对象token处typed-stop并保留此前push/pop。

### 4.4 动作405、406、409、500

- 405/406：分别调用两个目标准备，成功后走视觉commit、清屏、score 2+5、pending清理与延迟，返回0；
- 409：独立目标准备，视觉成功时除常规清屏外还写全1 accumulator、调用screen mode 1并把scan push低word写`0x8000`；尾部清current actor并返回1；
- 500：目标准备后只执行pending/目标/score+2，明确不走视觉commit；清pending、延迟并返回0。

其他大于99但未列出的值返回0。

## 5. 1–36稀疏jump table

LST jump table固定36项。有效普通case共27项：

```text
1,2,3,4,5,6,7,11,12,13,14,15,17,
22,23,24,25,26,27,28,29,31,32,33,34,35,36
```

case 8–10、16、18–21、30以及大于36的值走default并返回0；只保留入口动作查询与终止查询，第二参数不访问。

## 6. case 1：普通攻击

按side mode选择目标对象，准备成功后发布pending、packed高word、target identity与auxiliary清零。blocking为0且视觉commit成功时先发布目标与refresh，再清framebuffer并延迟300。

之后查询当前角色class：

- class不等于8：跳过比例修正；
- class等于8：取百分比，side A公式为`10% base + percent*full/100`；side B公式为`10% base + percent*(full-base)/100`，随后低32位取负、发布signed值、再做一次视觉commit及两个模式callee。

两侧pair action调用时点不同，保持LST顺序。公共尾清accumulator、selection high word、selection word，延迟300并返回0。

## 7. case 2/3：选择初始化与完成

首次进入以action runtime低word bit15作初始化门：

- 设置bit15；
- 保存选择snapshot；
- 18项target identity填全1；
- input mode低word写1；
- 18项selection workspace清零；
- available count只在大于当前side角色数时下调。

case 2还可直接构造`640×480, origin 0,0, field 200×200`变形对象。
实际对象本体分配失败时发布空所有者，保持之前的活动标记写入。
runtime低word bit0未置位时返回0。

case 2完成阶段先清共享pair值和两个selection word；若battle flags bit`0x20`置位，
保留deformation并返回，否则直接析构对象并释放owner。
case 3先消费队员实际资源，最终WORD写computed值，再清三项选择状态。
失败保留已发布head或erase，不写computed、选择、fade及撤退后缀。
两者随后置fade active、延迟300；case 3再直连撤退提交，保留原角色就绪与mode门。
叶协议、两个动作及全部六处消费的当前证据见
[实际队员资源](battle-actor-resource-consumption-00470e20.md)。
完整动作父其余协议继续pending，旧撤退返回仍按原caller忽略。

## 8. case 4/5/7/11/12/17

- 4：目标准备成功后走动作400公共commit与清理尾；
- 5：清current actor并返回1；
- 7：目标完成后延迟0、更新目标，目标索引小于4时packed actor低byte加1，返回1；
- 11/12：使用两个不同查询callee，完整EAX精确返回1后分别直接写实际队员绘制抑制0/1，再写步长8、overlay gate 1、current actor全1并返回1；
- 17：独立result mode非零时清current actor并返回1，否则返回0。

11/12已删除步长操作编号及通用调用，借用startup实际队员基础字段。
绘制抑制也已直接写startup.group_a_runtime_reset的同索引元素。
实际存储缺失在第一次抑制DWORD写入点报告，阻断步长和完成发布。
当前字段、所有者和失败前缀见[队员绘制抑制](battle-party-drawing-suppression-0047d870.md)。
全11处步长调用映射与验证见[队员绘制步长](battle-party-presentation-step-0047d860.md)。

## 9. case 6：多帧目标阶段

phase counter低word大于1时先减一；只有减后等于2才发消息并结束。消息ID/text token由phase condition选择。完成尾清画面factor、suppression、当前角色、目标、phase和辅助，置fade active；packed actor低byte减第三byte的u32结果不小于group-B count时还清terminal并把message state写`0x63`。

phase为0时先读目标code与距离，距离至少20或目标phase查询成功都会置condition auxiliary；两者都不满足则condition清零、phase写40并返回。满足时启动目标phase、置condition、设置三个factor=-12、phase=1、selected target、primary suppression=1，并执行动画初始化。

开始和完成时分别于004546B5、0045472E激活实际敌方目标，不再错误激活队员。
开始段多出的模式7调用已删除；刷新后仅类型低WORD为1C才直接写敌方绘制抑制1。
队员行动父链的47D870调用及编号均已删除；顺序、所有权与失败诊断见
[目标绘制抑制](battle-target-drawing-suppression-0047d870.md)。

随后角色phase完成时更新目标记录、packed actor低byte、selected target、phase=30及目标status发布。所有u16/u8操作保留各自回绕宽度。

## 10. case 13/14/15

### 10.1 case 13

phase零时建立factor=-12、primary suppression和动画。目标动作完成后清40字节临时record、只置offset `0x19`的bit`0x20`并commit；随后清视觉phase并置fade。最后扫描当前组A角色的前8个事件word，首个0写`target+1`。前8项全非零仍返回1，不访问第9、10项。前四个角色行直接复用startup reset的唯一40-word控制表，供已关闭消息99过渡控制选择按row-major消费；后六行继续由相邻物理tail承接，不建立前40项平行副本。缺少startup owner只在前四行首次真实事件槽访问typed-stop。

### 10.2 case 14

初始化同case 13，并额外把画面stage写1。目标动作完成后清factor/suppression/phase，置fade、message state `0x62`、current actor全1并返回1。

### 10.3 case 15

phase零时按battle flags bit2与packed high word<2决定是否增加召唤计数和组A数量；从每角色summon word取索引，依序执行选中、模式、首条动作记录清零、构造、动画与召唤记录建立。`0x0045529A`以`ECX=0x005029D0+index*0x2F34`直接组合已关闭`0x004786F0` typed leaf，真实返回地址为`0x0045529F`；只清零Group-A canonical第0动作记录的38个dword，生产路径不再调用原raw地址。每帧保持factor=-12和suppression；坐标完成后更新目标`index+8`、清battle bit2、summon runtime、packed lowword和角色summon word，发布终态并调用两个后续stage，返回1。

004552C2已改为直接写实际召唤队员的绘制抑制1，位于帧源清除之后、镜像及分配之前。
缺startup或残余状态数组时保留帧源清除，不执行分配；后继分配失败保留抑制写入。
实际所有者、首末队员和前后失败测试见[队员绘制抑制](battle-party-drawing-suppression-0047d870.md)。
004552F5也直接写召唤队员的实际镜像字段，仅全DWORD召唤门为0时写1。
动作25、99的敌方镜像、保存索引FFFF的物理别名及前后失败验证见
[行动镜像写入](battle-action-mirror-0047f900.md)。其他父协议继续待迁移。

## 11. case 22：状态指示器与对手选择

action runtime bit15未置位时，直接调用已关闭`0x00450F90`。其返回1后保存snapshot 6，并根据独立side selection word：

- 统计组A或组B中terminal query不等于1的对象；
- side A路径写side mode 1；side B路径不合理化清零旧side mode；
- 找到首个非terminal对象并发布索引；
- 写scene value 1，调用scene与selection finalize；
- action runtime低word置bit15，返回0。

00454BAE在scene发布之后直接把当前队员共享目标标记写为1。
不再使用置位寄存器协议、请求数组、轨迹、计数或转发函数。
字段写故障保留原值；返回故障保留1；两者保留scene前缀，
阻断bit15及正常收尾，失败值沿用scene实际返回。
见[共享目标标记置位](battle-actor-target-selection-latch-set-00478b30.md)。
动作其他协议仍待迁移。

bit15已置时，bit0仍为0则返回0；bit0为1则置fade active并返回1。

## 12. case 23–27

- 23：目标动作完成后查询消息code；`1..0x61A7`走名称构建/格式化与详细消息，0、`0x61A8`及更大值走两个固定文本；最终延迟300；
- 24：目标callee返回低word；bit15置位时遍历全部组B非terminal对象，计算signed值、上夹9999、累加、刷新、发布与可选清屏；保存低15位，低word等于2才清current actor并返回1，否则清低word返回0；
- 25：首次选择按目标准备结果显示成功/失败文本；成功保存双方索引、准备与更新目标。再次进入时更新目标status word；第一choice门非零时在原调用点直接组合已关闭组B行动对象资料，成功后才清该门；第二choice门非零时调用剩余choice操作。随后OR入`choice_cursor-1`低word并直接组合已关闭攻击顺序登记，以类型2把已选组B索引写入共享18槽首个全1记录，再清current actor并返回1。资料组合typed-stop保留choice/status前缀并阻断门清理、第二choice、攻击顺序与成功尾；
- 26：直接调用已关闭`0x00451100`。保留scan runtime、push state、dialog word三组低word门；完成早退或目标动作成功后的共同尾按pop64、延迟300、清四项低word执行。视觉成功先发布全1 accumulator、screen mode 1、清屏与scan push `0x8000`；
- 27：目标动作完成后以固定第一参数4计算选择；可选视觉成功发布目标并清屏，随后清accumulator/current actor并返回1。

## 13. case 28/29/32

28与32要求当前角色动作`0x1965`；29要求目标动作`0x1791`。成功后清40字节临时record，percent按u16读取，mode byte为`4*percent/100+2`：

- 28 flags `0x10000000`；
- 29 flags `0x08000000`；
- 32 flags `0x02000000`。

三者都直接commit已建record；29随后额外刷新目标。公共尾清current actor并返回1。

## 14. case 31：倒计时与内部bit

首次进入：message gate写`0x80000000`，直接初始化mode 1的5秒倒计时，即secondary ticks 150；按当前Group-B行动者token直连typed坐标查询，再直接清已关闭0x98动作record。坐标owner缺失或X/Y真实访问失败时停在查询点，保留倒计时与message gate，阻断动作record清理和Escape后缀。

随后直接读取内部bit 75。置位时按原callee清bit75、清message gate/aux、current actor写全1并返回1；第二参数仍不访问。

bit75未置且message gate bit0为1时，播放固定消息、再次清动作record，重建gate/aux，计算并发布目标值。视觉commit成功才清内部bit74、发布目标、清屏、清gate/current actor并返回1；失败清accumulator后返回0。内部bit span缺失只在对应byte真实访问点typed-stop。

## 15. case 33–36

- 33：先直连已关闭目标动作就绪状态机，复用当前group-A行动者owner、group-B目标token和未读取的`0x1791`；只有状态机返回一才清current actor并解析目标对象。状态机返回零或typed-stop都阻断后缀。目标零token仍在随后首次flags读取点typed-stop；flags bit`0x20`置位直接返回1，否则查询percent和属性，成功时mode 7、presentation 1、发布目标并清屏，最终返回1；
- 34：计算signed值，发布该值、视觉commit`(value,0,0)`，清signed槽；
- 35：发布selection word、视觉commit`(0,word,0)`，清selection word；
- 36：发布selection high word、视觉commit`(0,0,high)`，清high word。

34–36都清current actor并返回1。

## 16. closed callee回收与平台端口

已关闭callee直接复用：

- case 6目标阶段开始前的固定键计数查询，以及完成后的固定键计数链累加；
- mode 1五秒倒计时初始化；
- 两次0x98动作record清零；
- 状态指示器动画；
- 三档刻度扫描；
- 内部bit查询/清除；
- deformation typed对象构造/析构；
- framebuffer全1清屏；
- phase-six与action-twenty-three两处玩家道具双数量步进；
- 四处攻击顺序首匹配移除与完整记录左移；
- case 33目标动作就绪、双阶段粒子与双记录清理。

相邻双对象数值转场关闭后，两处调用均删除旧callee token并直接组合typed实现。五处文字发布也在原调用位置直连共享文字消息入链，复用启动状态的唯一链头和动态节点owner；原调用槽只保留reserved数值。动作累计值与效果协调器主反馈值收敛为唯一共享port；端口reply仍可在每个剩余callee后显式发布该物理值与两个selection word，caller随后按LST重读。其余尚未关闭的角色、AI、数值、消息、选择和呈现callee继续以单一窄端口表达，旧对象地址仅作为`compat::u32` token，不转换为主机指针。

## 17. typed故障点

- 组A/B对象索引：首次对应对象callee调用；
- actor map/effect score：首次表读写；
- case 11/12与15残余状态缺失：首次抑制字段实际写入点报告，分别阻断步长/完成发布和镜像/分配；
- target identity/status/event/summon表：首次元素访问；
- resolved target零token：首次flags读取；
- framebuffer尺寸过大：先发布frame refresh并写满owned前缀，再在首个越界像素停止；
- 状态指示器、刻度扫描与内部bit：各closed helper真实故障点；
- 玩家道具链：未知token在首次节点访问停止，零分配先发布零head再停止；
- 攻击顺序登记：在case25已写目标status后，于首个实际记录读取处停止，阻断current actor清理与成功尾；
- 攻击顺序移除：四处均在原call位置直连；相邻强度效果记录缺失时保留对象/延迟/计数前缀，阻断call后的目标、状态或成功发布；
- case 15首条动作记录清零：PUSH EDI、38次独立STOSD、POP EDI与RET均保留真实停止点；写fault保留此前已提交的零写入，阻断清模式、召唤构造、动画与全部后缀；
- case 31坐标查询：按Group-B索引解析canonical lifecycle owner；任一真实访问typed-stop保留SUB flags与查询前缀，并阻断动作record清理和Escape后缀；
- case 33目标动作就绪：行动者、frame返回记录、共享source owner和目标坐标分别在原始首次访问点停止，并阻断current actor清理与后续目标解析。

不对callee约定外普通返回值、百分比、action code或计数增加现代范围预验。

## 18. 验证与动态差分

定向测试覆盖：

- 入口组A越界、terminal早退与主0/次5动作替换；
- 动作99终态；
- case 1普通清屏、class 8比例/二补数路径；
- case 2 deformation分配、构造、完成时析构与owner释放；
- action404同调用0→1→2、双组push/pop顺序；
- action409独有screen mode与scan push发布；
- case 6减到2消息终态、目标阶段前固定键计数查询阈值及typed-stop后缀阻断、完成时固定键计数链直连、缺键唯一分配及selector-one道具数量直连；
- case 13事件槽；
- case 23消息路径selector-one道具数量直连；
- case 22已完成门与真实状态指示器完成路径；
- case 24全组扫描；
- case 31倒计时、bit75清除及目标访问延后；
- case 33目标动作就绪直接typed组合、双阶段/陈旧sequence及零target真实解引用停点；
- case 34–36三分量尾；
- 超尺寸清屏已写满前缀与refresh时机；
- 27个有效普通case、10个有效特殊动作逐项smoke；
- 9个稀疏switch槽和越界action只执行两个入口callee；
- 95个原始唯一callee中80个窄端口边界全部存在，另外15个已关闭callee全部直连；
- case 3撤退提交成功、固定警告分支和旧地址调用清零；
- case 25攻击顺序登记直连、旧地址调用清零及记录typed-stop前缀；
- case 7攻击顺序移除直连、一过尾源读取stop传播及旧地址调用清零；
- battle聚合目标零warning，普通定向通过。

当前没有原版18个角色对象、剩余callee共享副作用、目标动作就绪的真实ACT/TSW/粒子/音频/绘制联合状态、攻击顺序与相邻强度效果动态记录、全部数值/AI表、消息文本、输入bit、DirectDraw framebuffer、deformation allocator与SEH联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 19. 动作目标word查询两处caller直连

工作包296关闭case 22中的`0x00454A3D`和`0x00454AE6`两处物理call，真实返回地址分别为`0x00454A42`和`0x00454AEB`。两处均以固定Group-A首角色token读取canonical `actor+0x29A2`动作目标，leaf只替换AX；入口flags来自side word与零的word比较，返回后caller才符号扩展AX、定位Group-B对象并执行目标准备、动态角色扫描、首个存活目标发布和场景尾。

实现直接组合`0x004786E0` typed leaf，不增加generic端口调用。动作目标字段或RET停止保留已完成的状态指示器与case前缀，抑制目标准备、扫描、发布和公共场景后缀。首个存活目标经待审`0x00478A70`发布时，同步写Group-A首角色唯一canonical动作目标；全局选择值保持独立状态。定向测试覆盖两处返回地址、EAX高word、ECX/EDX、比较flags、目标符号扩展、发布写入和typed-stop后缀抑制；生产`0x004786E0` raw调用为零。

当前缺少原版完整Group-A/Group-B actor、状态指示器与case 22剩余callee共享副作用，以及两处caller联合寄存器、flags和SEH捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。

## 20. Workpack 316 caller 的 case7 FLAGS 局部审查

`0x004554F6 CALL sub_479850` 正常返回后，LST 的
`0x004554FB CMP EAX,EBX`（此路径 EBX=1）定义包括 AF 在内的
减法 FLAGS；`0x004554FD JNZ` 不改 FLAGS。成功路径传入下一
`sub_478710` 的参数首读障时必须保持 ZF=1、CF=0、AF 已定义且为0，
不能把 CMP 替换成 `TEST`/逻辑零标志（AF 未定义）。
`legacy_battle_action_dispatch_cases_low.cpp` 在该物理 caller
使用 `subtract_flags(reply.eax,1)`；定向宿主用例绑定合成父栈、
组B角色与子端口，由typed caller正常返回后抵达动作模式参数读障，
并核返回地址和FLAGS。未绑定快照时直接抛`NOTIMPLEMENTED`；
受控回放不作为原版动作7证据。
此结论只覆盖该 caller 的局部后缀，不替代
`sub_479850` 的完整 REVIEW 或原版差分。

## 21. 共享目标数量递增协议移除

动作公共上下文的计数请求数组改为单一访问描述；公共结果仅保留
可选的实际递增结果。该操作的owner/view/resolver、寄存器回复、
模拟栈、地址/轨迹、调用计数及执行转发删除。三处实际调用均在组A帧，
借双方同一WORD存储；本动作函数不新增递增调用或私有计数副本。

phase先于计数，后继选择沿用真实地址计算与DEC前驱。成功、失败、
实时选择、部分扫描及字回绕的范围与本批门禁见
[共享目标数量递增](battle-actor-target-selection-count-increment-00478aa0.md)。
本父链其他callee编号、参数数组、寄存器请求/回复、轨迹、计数和Port
仍待逐项迁移；本切片不升级动作、角色帧316或原版差分的验收状态。
