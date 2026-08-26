# 战斗角色动作主分派 `0x004539B0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 完整LST范围

权威主体为`0x004539B0..0x00455CCF`，完整3836行、202个静态call站点、137个`loc_`标签。函数另有不可遗漏的外部FUNCTION CHUNK：

```text
0x00498350..0x00498364
size = 0x15 bytes
18 LST lines
1 call
loc_498350 + SEH_4539B0
```

完整函数因此包含203个call站点。外部chunk负责构造deformation对象异常时释放刚分配的owner，再跳MSVC异常展开。modern以`std::make_unique`承担主机对象异常安全，并在catch路径调用legacy owner释放端口后继续抛出，不吞异常。

ABI为两个参数cdecl/plain `retn`：第一参数选择角色组A槽，第二参数按动作语义选择角色组B或另一角色槽。唯一caller位于尚未关闭的`0x00456680`；当前caller不提前计数。

## 2. 入口与次级动作号

入口按低32位乘法建立角色组A物理token：

```text
0x005029D0 + group_a_index * 0x2F34
```

首次对象访问即调用动作号查询，结果只取AX。随后调用角色终止查询；完整EAX等于1时立即返回1，第二参数完全不访问。

主动作号为0时，再调用次级动作号查询；该返回值mask为u16后真正进入后续分派。次级仍为0才返回1。modern测试锁定“主0、次5”必须进入case 5，而不是错误地继续使用陈旧主动作0。

角色组A索引越界只在首次对象查询点typed-stop。第二参数不做入口预验；每个case只在首次实际组A/组B对象、表或状态槽访问时检查。

## 3. 动作99终态

动作`0x63`不走jump table。固定执行两个模式callee，随后：

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

case 2还可直接构造`640×480, origin 0,0, field 200×200` deformation对象；allocator返回0时owner保持空。runtime低word bit0未置位时返回0。

完成阶段先清accumulator和两个selection word。case 2若battle flags bit`0x20`置位则保留deformation并返回；否则直接析构对象并释放owner。case 3调用选择计算callee。两者随后置fade active、延迟300、调用角色finalize并返回0。

## 8. case 4/5/7/11/12/17

- 4：目标准备成功后走动作400公共commit与清理尾；
- 5：清current actor并返回1；
- 7：目标完成后延迟0、更新目标，目标索引小于4时packed actor低byte加1，返回1；
- 11/12：使用两个不同查询callee，成功后分别发布mode 0/1，共用finalize mode 8、overlay gate 1、current actor全1与返回1；
- 17：独立result mode非零时清current actor并返回1，否则返回0。

## 9. case 6：多帧目标阶段

phase counter低word大于1时先减一；只有减后等于2才发消息并结束。消息ID/text token由phase condition选择。完成尾清画面factor、suppression、当前角色、目标、phase和辅助，置fade active；packed actor低byte减第三byte的u32结果不小于group-B count时还清terminal并把message state写`0x63`。

phase为0时先读目标code与距离，距离至少20或目标phase查询成功都会置condition auxiliary；两者都不满足则condition清零、phase写40并返回。满足时启动目标phase、置condition、设置三个factor=-12、phase=1、selected target、primary suppression=1，并执行动画初始化。

随后角色phase完成时更新目标记录、packed actor低byte、selected target、phase=30及目标status发布。所有u16/u8操作保留各自回绕宽度。

## 10. case 13/14/15

### 10.1 case 13

phase零时建立factor=-12、primary suppression和动画。目标动作完成后清40字节临时record、只置offset `0x19`的bit`0x20`并commit；随后清视觉phase并置fade。最后扫描当前组A角色的前8个事件word，首个0写`target+1`。前8项全非零仍返回1，不访问第9、10项。

### 10.2 case 14

初始化同case 13，并额外把画面stage写1。目标动作完成后清factor/suppression/phase，置fade、message state `0x62`、current actor全1并返回1。

### 10.3 case 15

phase零时按battle flags bit2与packed high word<2决定是否增加召唤计数和组A数量；从每角色summon word取索引，依序执行选中、模式、构造、动画与召唤记录建立。每帧保持factor=-12和suppression；坐标完成后更新目标`index+8`、清battle bit2、summon runtime、packed lowword和角色summon word，发布终态并调用两个后续stage，返回1。

## 11. case 22：状态指示器与对手选择

action runtime bit15未置位时，直接调用已关闭`0x00450F90`。其返回1后保存snapshot 6，并根据独立side selection word：

- 统计组A或组B中terminal query不等于1的对象；
- side A路径写side mode 1；side B路径不合理化清零旧side mode；
- 找到首个非terminal对象并发布索引；
- 写scene value 1，调用scene与selection finalize；
- action runtime低word置bit15，返回0。

bit15已置时，bit0仍为0则返回0；bit0为1则置fade active并返回1。

## 12. case 23–27

- 23：目标动作完成后查询消息code；`1..0x61A7`走名称构建/格式化与详细消息，0、`0x61A8`及更大值走两个固定文本；最终延迟300；
- 24：目标callee返回低word；bit15置位时遍历全部组B非terminal对象，计算signed值、上夹9999、累加、刷新、发布与可选清屏；保存低15位，低word等于2才清current actor并返回1，否则清低word返回0；
- 25：首次选择按目标准备结果显示成功/失败文本；成功保存双方索引、准备与更新目标。再次进入时更新目标status word，按两个独立门调用choice操作，OR入`choice_cursor-1`低word后commit并返回1；
- 26：直接调用已关闭`0x00451100`。保留scan runtime、push state、dialog word三组低word门；完成早退或目标动作成功后的共同尾按pop64、延迟300、清四项低word执行。视觉成功先发布全1 accumulator、screen mode 1、清屏与scan push `0x8000`；
- 27：目标动作完成后以固定第一参数4计算选择；可选视觉成功发布目标并清屏，随后清accumulator/current actor并返回1。

## 13. case 28/29/32

28与32要求当前角色动作`0x1965`；29要求目标动作`0x1791`。成功后清40字节临时record，percent按u16读取，mode byte为`4*percent/100+2`：

- 28 flags `0x10000000`；
- 29 flags `0x08000000`；
- 32 flags `0x02000000`。

三者都直接commit已建record；29随后额外刷新目标。公共尾清current actor并返回1。

## 14. case 31：倒计时与内部bit

首次进入：message gate写`0x80000000`，直接初始化mode 1的5秒倒计时，即secondary ticks 150；调用坐标callee并直接清已关闭0x98动作record。

随后直接读取内部bit 75。置位时按原callee清bit75、清message gate/aux、current actor写全1并返回1；第二参数仍不访问。

bit75未置且message gate bit0为1时，播放固定消息、再次清动作record，重建gate/aux，计算并发布目标值。视觉commit成功才清内部bit74、发布目标、清屏、清gate/current actor并返回1；失败清accumulator后返回0。内部bit span缺失只在对应byte真实访问点typed-stop。

## 15. case 33–36

- 33：目标动作`0x1791`完成后先清current actor，再解析目标对象；零token在首次flags读取点typed-stop。flags bit`0x20`置位直接返回1。否则查询percent和属性，成功时mode 7、presentation 1、发布目标并清屏；最终返回1；
- 34：计算signed值，发布该值、视觉commit`(value,0,0)`，清signed槽；
- 35：发布selection word、视觉commit`(0,word,0)`，清selection word；
- 36：发布selection high word、视觉commit`(0,0,high)`，清high word。

34–36都清current actor并返回1。

## 16. closed callee回收与平台端口

已关闭callee直接复用：

- mode 1五秒倒计时初始化；
- 两次0x98动作record清零；
- 状态指示器动画；
- 三档刻度扫描；
- 内部bit查询/清除；
- deformation typed对象构造/析构；
- framebuffer全1清屏。

其余尚未关闭的角色、AI、数值、消息、选择和呈现callee以单一窄端口表达，callee地址仅作为`compat::u32` token，不转换为主机指针。端口reply可显式发布callee实际修改的accumulator与两个selection word；caller随后按LST重读。

## 17. typed故障点

- 组A/B对象索引：首次对应对象callee调用；
- actor map/effect score：首次表读写；
- target identity/status/event/summon表：首次元素访问；
- resolved target零token：首次flags读取；
- framebuffer尺寸过大：先发布frame refresh并写满owned前缀，再在首个越界像素停止；
- 状态指示器、刻度扫描与内部bit：各closed helper真实故障点。

不对callee约定外普通返回值、百分比、action code或计数增加现代范围预验。

## 18. 验证与动态差分

定向测试覆盖：

- 入口组A越界、terminal早退与主0/次5动作替换；
- 动作99终态；
- case 1普通清屏、class 8比例/二补数路径；
- case 2 deformation分配、构造、完成时析构与owner释放；
- action404同调用0→1→2、双组push/pop顺序；
- action409独有screen mode与scan push发布；
- case 6减到2消息终态；
- case 13事件槽；
- case 22已完成门与真实状态指示器完成路径；
- case 24全组扫描；
- case 31倒计时、bit75清除及目标访问延后；
- case 33零target真实解引用停点；
- case 34–36三分量尾；
- 超尺寸清屏已写满前缀与refresh时机；
- 27个有效普通case、10个有效特殊动作逐项smoke；
- 9个稀疏switch槽和越界action只执行两个入口callee；
- 95个原始唯一callee中87个端口边界全部存在，另外8个已关闭callee全部直连；
- battle聚合目标零warning，普通定向通过。

当前没有原版18个角色对象、95类callee共享副作用、全部数值/AI表、消息文本、输入bit、DirectDraw framebuffer、deformation allocator与SEH联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
