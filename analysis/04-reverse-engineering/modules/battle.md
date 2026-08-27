# 模块10：战斗状态机、AI与数值系统

状态：`module_in_progress`

当前关闭进度：`88/422`。现有资产读取与建场代码只是此前恢复的有限切片，不提前计入完整函数关闭。

## 1. 唯一真值与模块目标

`swd3.exe.lst`的完整函数体、机器码和外部`FUNCTION CHUNK`是本模块唯一行为真值。IDA名称、反编译C、旧ASM、所有权目录、调用图和现有C++只用于导航，不得替代逐基本块审计。

模块10负责：

- 消费战斗编号后的战斗实例建立与销毁；
- 战斗逐帧状态机、脚本分派、等待与退出；
- 敌我角色对象、行动、技能、效果和战斗内列表生命周期；
- AI决策、目标选择、路径与行动调度；
- 命中、伤害、恢复、状态、奖励和其他战斗数值规则；
- 战斗分支内画面组织与最终呈现时机；
- 向顶层发布原四值战斗结果合同；
- 完成I5：世界进入战斗，完成一次完整战斗，再按原返回路径恢复世界。

模块10不取得通用文件、输入、RNG、动作记录、音频设备、软件绘图原语、世界角色、道具链、剧情变量或存档容器的所有权。它只能通过已关闭的typed接口借用这些模块能力。

## 2. 机械函数范围

生成器：

```text
analysis/tools/build_battle_workpack.py
```

唯一工作包：

```text
analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv
```

范围从冻结的`module-function-ownership.tsv`中机械选择：

```text
module_candidate == battle
code_origin == game
```

当前固定结果：

- 游戏函数：`422`；
- 外部CRT或第三方函数：`0`；
- 唯一地址：`422`；
- 首地址：`0x00433AA0`；
- 尾地址：`0x00484500`；
- `confirmed_boundary`：`61`；
- `medium`导航候选：`361`；
- `pending_audit`：`358`；
- `assembly_exact`：`5`；
- `platform_adapted`：`59`；
- 已关闭：`64`。

六个稳定导航分组为：

- `transferred_action_and_asset_helpers`：`15`项，范围`0x00433AA0..0x00434DD0`；
- `battle_record_leaves`：`2`项，范围`0x0044FFC0..0x0044FFE0`；
- `setup_frame_input_and_resolution`：`93`项，范围`0x00450270..0x0045FC60`；
- `script_dispatch_ai_and_targeting`：`77`项，范围`0x00460C40..0x0046FFF0`；
- `actor_actions_effects_and_rendering`：`194`项，范围`0x00470180..0x0047FC40`；
- `shared_battle_object_services`：`41`项，范围`0x004800F0..0x00484500`。

`audit_order`只是稳定的地址顺序，不证明函数语义，也不强制违背callee优先的实现顺序。每次关闭函数后必须立即回收已关闭callee在caller中的opaque边界。

生成器固定断言候选数、首尾地址、地址唯一性、六组计数、置信度计数、关闭状态集合、证据非空、证据路径不越出逆向根目录且文件实际存在。连续两次生成的工作包必须逐字节一致。

## 3. 现有切片不提前计数

当前`src/battle/`只有两类历史切片：

- `legacy_battle_assets`：FIGTALK固定窗口和`battle.ffd`头、索引、记录读取；
- `legacy_battle_setup`：初始队伍筛选、固定阵型、镜像坐标和敌方记录布局。

它们覆盖了`0x0046E0B0`、`0x0045F130`、`0x0045F1B0`与`0x00451B10`的部分有效资产路径或部分指令区间，但尚未证明所属函数的完整LST函数体、全部外部chunk、全部错误/循环/异常域、caller回收和完整战斗生命周期。因此这些历史切片继续不计数；当前`88/422`只来自本文件逐项登记且完成全部关闭门的函数，不得把其他测试通过、局部有效路径或真实battle 98样本当作函数关闭。

`app::battle_transition`和`frame_runtime`只实现顶层请求与返回编排，不属于422项战斗内部函数关闭计数。

## 4. 顶层接口与I5合同

世界或剧情生产带最高位的战斗请求。顶层在高优先级模式未占用时按原顺序：

1. 关闭当前地图映射；
2. 以请求低16位调用战斗初始化入口；
3. 设置战斗活动门；
4. 后续有效帧每帧调用战斗主入口一次；
5. 战斗入口自行决定等待、继续、呈现或退出。

战斗逐帧返回值保持四值合同：

- `1`：保持战斗活动，并立即结束当前帧；
- `0`：清战斗活动，恢复或重载地图，并立即结束当前帧；
- `2`：清战斗活动，请求原特殊模式返回路径，并立即结束当前帧；
- `3`：清战斗活动，重映射并恢复地图，并立即结束当前帧。

四条出口都不能在同帧继续普通世界或剧情。等待必须由原状态和指令指针决定，不得把一整场战斗压进一次现代调用。

I5最终必须锁定：

- 世界侧请求前状态快照；
- 战斗编号、入口状态、RNG种子与调用序列；
- 至少一场真实战斗的逐帧关键状态；
- 战斗结果与四值出口；
- 世界恢复后的地图、角色、剧情、音乐和输入状态；
- 关键帧framebuffer哈希。

## 5. 状态所有权与跨模块边界

当前状态目录显式记录七项战斗owner：

- `battle_instruction_pointer`；
- `battle_external_result_state`；
- `battle_multipurpose_temporary`；
- `battle_actor_group_a_array`；
- `battle_actor_group_b_array`；
- `battle_status_action_nodes`；
- `battle_effect_action_nodes`。

当前目录另有十项战斗借用状态，所有权保持在原模块：

- `runtime_platform`：战斗请求值、战斗活动门；
- `rendering`：公共source surface；
- `asset_runtime`：公共action记录、ACT stream cache节点；
- `audio_video`：Miles管理对象；
- `special_modes`：道具节点；
- `story_scene`：64项全局整数；
- `world_map`：256项角色记录、四个队伍/道具哨兵头。

另有三类跨模块服务边界不建立战斗owner副本：

- `resource_io`提供文件、内存和资源字节访问；
- `input_time_rng`提供输入快照、帧时钟、等待和两套RNG；
- `persistence`运输战斗持久字段，但不拥有战斗业务语义。

`battle`可依赖已关闭的`asset_runtime`、`audio_video`、`input_time_rng`、`rendering`、`resource_io`、`runtime_platform`、`special_modes`、`story_scene`和`world_map`接口。`runtime_platform`、`world_map`、`story_scene`、`special_modes`与`persistence`可以消费战斗入口或状态合同，但不得反向取得战斗内部对象所有权。

## 6. 真实资产锁

当前战斗真实资产：

- `battle.ffd`：`586004`字节，SHA256 `3f04da197c35bbecc1f274b26e28d418e673e9d6a2d26a5d765aa8dbeeabc31e`；
- `figtalk.dat`：`20612`字节，SHA256 `1ace07e58ed09f6875f5d9cacaa5a7be0bf8b83af4e0668d2a26cc421d873d1c`。

现有真实样本battle id 98只证明：资源定位、记录选择、一个初始玩家槽和一个敌方记录切片可读取。它不证明背景、对象构造、脚本、AI、行动、数值、胜负、奖励、清理或世界恢复。

## 7. 单函数关闭规则

每个工作包项必须依次完成：

1. 锁定完整LST函数范围、ABI、全部入口、返回和外部`FUNCTION CHUNK`；
2. 不看现有C++，记录基本块、条件、宽度、符号扩展、回绕、调用顺序和副作用；
3. 识别战斗owner与跨模块借用状态，禁止复制owner；
4. 从LST独立推导跳转两侧、相等、零、正负、哨兵、截断、回绕和循环测试；
5. 形成中文证据文档；
6. 用C++20 typed接口实现，平台适配保持最小且隔离；
7. 执行LST到C++和C++到LST双向追溯；
8. 运行定向、适用真实资产及完整Linux门；
9. callee关闭后立即回收caller中的opaque调用；
10. 证据、实现、测试和caller回收全部完成后才写入`CLOSURES`。

动态表、链、selector、资源或snapshot越界只能在原始读取/写入点typed-stop，并保留此前副作用。原始无限循环和内存破坏域必须在完整原始域检查后停止，不伪造成功状态。原始BUG、RNG调用次数、帧内时序和战斗结果不得现代化。

允许的工作包状态为：

- `pending_audit`；
- `assembly_exact`；
- `platform_adapted`；
- `unreachable_current_assets`；
- `blocked_runtime_oracle`。

除`pending_audit`外都必须提供证据路径。仅有现有测试、反编译或局部切片不得改变状态。

## 8. 验证门

范围锁门：

- 两次生成逐字节一致；
- `422`行、`422`唯一地址、六组计数正确；
- 初始范围锁为`closure 0/422`，后续进度只能由`CLOSURES`逐项增加；
- 所有行的名称、调用者、被调者、置信度和旧审查状态均明确标为导航信息。

逐函数门：

- LST独立定向测试；
- 必要的ASan小门；
- Linux core与app完整门；
- 大阶段关闭时Windows LLVM app完整门；
- 真实资产或固定状态样本；
- 可用时与原版动态差分。

原版framebuffer、音频、粒子、文字和部分战斗handler的动态捕获仍可能受`blocked_runtime_oracle`限制。该状态只允许在静态、实现、测试和现有资产均闭环后登记，不能替代未完成审计。

助手不得启动原版或OpenSWD3游戏EXE。Windows实机I5由用户执行。

## 9. 非范围

以下内容不计入422项战斗函数：

- 顶层消息泵、战斗请求门和世界/特殊模式最终分派；
- 剧情opcode产生战斗请求的语义；
- 通用文件、内存、ACT/ANI/SND、音频设备和渲染原语；
- 菜单中的战斗速度配置与道具业务；
- 存档容器读写与最终I6兼容验收；
- CRT和第三方库实现。

跨模块caller在其owner模块关闭，战斗只实现并证明自身callee合同。

## 10. 当前执行边界

`audit_order=1`的`0x00433AA0`已关闭为`platform_adapted`。它按原包含上界和组合高标记顺序查询TSW命令流的透明或literal区间，保留负行坐标越过目录及不可达子标记分支，并只在原读取点隔离短源。唯一caller尚未进入现代实现，因此当前没有opaque callback可回收；关闭caller时必须直接调用typed接口。

`audit_order=2`的`0x00433C40`已关闭为`platform_adapted`。它先遗失两张旧行表指针，使已关闭重建callee无法释放旧表，再以固定1280×768和640×480参数建立主表、surface表与全surface矩形；普通申请失败仍继续，只有原行表写点typed-stop阻止后缀。两张360项方向表由精确x87常量与向零转换结果冻结，保留第二、第四象限仅逆序复制基础索引88..0而跳过89的非对称，以及四个正交方向的十万幅值。唯一尾跳caller尚未现代实现。

`audit_order=3`的`0x00433D70`已关闭为`platform_adapted`。它直接调用已关闭附属缓冲释放，再按surface行表、主行表顺序执行两个独立非空释放分支，均在释放返回后清零；其他尺寸、矩形和方向表状态保持不变。两个主动caller与一个CRT退出回调均不消费旧EAX残值。

`audit_order=4`的`0x00433DC0`已关闭为`platform_adapted`。它先直接调用渲染模块`0x00437E90`的typed pitch/高度getter，再以pitch向零除二和高度重建surface行表；矩形故意保留宽取高度、高取未除二pitch的旧参数非对称，最后固定重建1280×768主行表。普通申请失败继续，两个原行表写点分别typed-stop。

`audit_order=7`的`0x00433F00`已关闭为`platform_adapted`。它对附属缓冲空token直接返回；非空时以入口snapshot调用释放端口，回调期间owner仍保留旧token，只有回调返回后才清零。唯一caller尚未现代实现，关闭时必须直接组合该typed入口。

`audit_order=10`的`0x004342E0`已关闭为`assembly_exact`。它按left/top/width/height处理负起点缩短尺寸，右/下达到surface边界时反向移动起点而不裁短，最后按原顺序发布四条绝对边并让EAX携带bottom。三个caller均未现代实现，因此当前没有opaque callback可回收；关闭它们时必须直接调用typed接口。

`audit_order=5`的`0x00433E20`已关闭为`platform_adapted`。它先释放旧主行表，按行数乘4的32位回绕值申请新表；申请失败只清指针并保留旧元数据，成功后才发布步长和行数。正行数逐项写回绕偏移；申请大小回绕导致的越界只在原写入点停止并保留此前前缀。两个caller均未现代实现，当前没有opaque callback可回收。

`audit_order=6`的`0x00433E90`已关闭为`platform_adapted`。它独立释放并重建surface行表，成功后发布被绘制矩形直接消费的surface宽高；控制流与主行表同构但owner严格分离。分配失败和乘法回绕写入点保留相同前缀规则，主行表不受影响。三个caller均未现代实现，当前没有opaque callback可回收。

`audit_order=8`的`0x00433F30`已关闭为`assembly_exact`。它在行表callee前预发布宿主surface宽高，以保存的入参snapshot直接调用surface行表与全surface矩形typed入口；分配失败仍继续发布矩形，原行表越界写typed-stop则不伪造后续副作用。两个callee已直接回收，五个上层调用点尚未现代实现。

`audit_order=9`的`0x00433F70`已关闭为`platform_adapted`。它对固定literal字图像执行四方向循环平移：模式0/1移动完整行记录，模式2/3只移动每行像素负载。入口先清首行头bit15，再检查剩余高flag和模式；每分支只申请一个临时块，按原`rep movsd; rep movsb`顺序保存、原位搬移、回写并释放。申请失败和宽高/移位非法域不提前夹值，而在首次真实图像或临时读写点typed-stop，并保留此前flag、临时和图像写前缀。五个caller尚待现代实现。

`audit_order=11`的`0x00434350`已关闭为`assembly_exact`。它按起终点回绕差值确定轴向符号，以严格半误差阈值推进X/Y主轴坐标，并在更新后判定双轴终点。零长度线段仍先把Y加一，`INT_MIN`取反与误差累加均保留32位异常域；函数无callee，九个caller均忽略其返回值。

`audit_order=12`的`0x00434420`已关闭为`platform_adapted`。它按记录索引读取战斗对象内两张360项方向表，保留符号、严格半误差阈值、零向量Y加一、`INT_MIN`与坐标/误差回绕；越界索引只在原首次水平表读取点停止且不修改记录。该叶子无callee，唯一caller的两个调用点都忽略指针残值。

`audit_order=13`的`0x004344E0`已关闭为`platform_adapted`。它直接回收两处方向向量推进与单像素通道合成callee，按signed固定点商组织反向Y外层和正向X内层扫描；保留三项共享发布、镜像源字节公式、目标边界先行、双透明色、直接/合成写入、除零和源/行表/目标原访问点typed-stop。唯一caller不消费EAX残值。

`audit_order=15`的`0x00434DD0`已关闭为`platform_adapted`。它用固定CRT序列从源图像随机选择非透明像素，建立56字节粒子节点，保存并清除原2×2块，再额外链接空后继；保留bit6/bit7选择优先级、bit0镜像不减一、bit7沿用入口次数与旧Y局部的BUG、零尾发布、批次回绕，以及全部除法、源读、节点写和两次分配失败前缀。唯一caller `0x00434790`现已直接回收typed入口。

`audit_order=16`的`0x0044FFC0`已关闭为`assembly_exact`。它把32位战斗速度设置按`20-setting`、两次乘5和左移2的原指令链换算为行动计时阈值，同时发布typed状态并返回相同bit pattern。运算全程按低32位回绕，不夹值或拒绝极值；原数据默认900及五处下游读取合同已锁定。唯一caller位于战斗初始化入口，尚待现代实现。

`audit_order=30`的`0x00450A50`按callee优先关闭为`platform_adapted`。它把第五个入口dword的栈槽地址发布为共享源，再以`x,y,width,height,8,0`直接调用已关闭软件blitter；现代实现以完整四字节小端snapshot替代悬空栈指针，并在调用期间保留其余入口共享状态。模式8实际选择raw常量色垂直渐变而非普通实色copy；调用期间保留共享opacity，固定尾参数0只表示空palette/辅助；正常返回后按blitter公共后缀清零目标高度、水平位移、纵向phase和opacity而保留放大位，typed-stop不清。低word为`0xFFFF`时仍进入RLE误分类和既有callee typed-stop。12个callsite尚待各caller现代实现。

`audit_order=17`的`0x0044FFE0`已关闭为`platform_adapted`。它先预取但不绘制4号帧，以回绕后的`(横向次数+2)*16 × (纵向次数+2)*16`绘制渐变底板，再按0/1/2、3/5、6/7/8顺序绘制九块边框。1号与7号帧在零重复时仍查询，3号与5号帧每轮重取；右边X只取3号宽乘横向次数加一，行Y只取5号高推进。实现直接组合已关闭帧查询、软件blitter和渐变包装，保留共享记录/source发布、blitter正常后缀、帧缺失和draw typed-stop前缀。

`audit_order=18`的`0x00450270`已关闭为`platform_adapted`。它固定查询资源0号帧，先发布帧记录与源，再以入口X/Y、u16宽高和两个固定0调用软件blitter。typed调用保留查询发布的layout，但按第六参数0清空实际palette与辅助；indexed帧只在首次palette读取点typed-stop。帧缺失保留入口旧源，正常blitter公共后缀清四项单次状态并保留放大位。

`audit_order=19`的`0x004502B0`已关闭为`platform_adapted`。它以完整0x98零记录、固定动作号和低16位variant调用已关闭动作更新器，使用产出的资源/帧号与YX偏移；状态表字节为1时先用固定绿色槽执行四向描边，再主绘制，入口selector完整等于1时以模式flags加0x10及绿蓝-10复绘。实现保留帧/source发布、描边逐遍公共后缀、短状态表原读点停止和三类draw故障前缀。

`audit_order=20`的`0x00450400`已关闭为`platform_adapted`。它按0x98步长选择持久动作槽，只依次覆盖动作号2392与base variant 0后调用更新器；成功后以槽内资源/帧号、YX偏移和mode flags绘制一次。负数或超容量索引只在首个槽写点停止，更新失败保留两项覆盖，帧/source和blitter故障前缀不回滚。

`audit_order=21`的`0x00450490`已关闭为`platform_adapted`。它先查询并发布入口资源/帧，之后才检查显式宽度；宽度非正仍保留record/source发布且不绘制。正宽度路径完全忽略帧宽，以入口宽度和帧u16高度绘制，正常公共后缀清单次请求、RGB和跳行状态并保留放大位。

`audit_order=22`的`0x004504E0`已关闭为`platform_adapted`。它查询并发布入口资源/帧，再以记录u16宽高、入口坐标、flags 0和记录tail绘制一次；indexed帧保留记录palette，失败只在首个记录解引用点停止。

`audit_order=23`的`0x00450530`已关闭为`platform_adapted`。它先完整绘制同资源0号帧，再必查并发布1号帧；第四参数严格为0才跳过第二层，负宽度仍进入blitter。第一层公共后缀先于第二次查询生效，首层或次层typed-stop均保留所属真实前缀。

`audit_order=24`的`0x004505B0`已关闭为`platform_adapted`。它复用同一双层顺序，但第二层固定帧2，严格零门之前仍发布record/source，正常公共尾恒定返回1；八个caller虽立即覆盖EAX，callee合同仍保留。

`audit_order=25`的`0x00450630`已关闭为`platform_adapted`。它完整绘制0号帧后必查并绘制1号帧，第二层宽度严格为入口第四参数低16位加2，范围2..65537；高16位丢弃，不读帧1宽，也没有零宽门。

`audit_order=26`的`0x004506B0`已关闭为`platform_adapted`。它固定四轮按1000/100/10/1做signed向零商余量分解，首个非零后保留内部零，全零强制一帧0；再从单位商向高位以商低16位查帧，按每个当前帧宽回退X。负值不取绝对值，在真实非法帧查询点停止。

`audit_order=27`的`0x004507A0`已关闭为`platform_adapted`。它只覆盖packed颜色槽高16位，发布值/X/Y并清leading，固定按十亿到一调用`0x00450900`十次；每次重读共享X并加EAX低16位，最后一位调用前强制leading=1，正常返回最后推进低字。callee已关闭并直连，opaque端口已删除。

`audit_order=28`的`0x00450900`已关闭为`platform_adapted`。它对共享余数执行u32除法，只以商低16位决定前导零跳过；资源号高16位继承商或leading高字，帧号保留完整商。两种模式固定X减16与空tail，正常后只以商低16位更新余数、置leading，并返回“新余数高16位+帧宽低16位”。

`audit_order=29`的`0x004509D0`已关闭为`platform_adapted`。它按0x98低32位回绕选择持久动作槽，依次写入口动作号和base variant 0，再调用已关闭动作更新器；失败保留前缀并停止。成功路径显式接收更新后ECX snapshot，以其高16位和记录资源低字查询帧，只发布source，不发布帧record，再以记录flags、记录宽高、入口坐标和固定空tail绘制。正常公共后缀照常执行，indexed帧在首次palette读取点typed-stop。

`audit_order=31`的`0x00450A80`已关闭为`platform_adapted`。它只覆盖单个持久动作记录的动作号与base variant，更新失败保留前缀；成功后以零扩展资源号和继承更新后EDX高字的帧号查询。selector精确1只翻转本次flags bit0，并把u16帧宽减X偏移解释为signed修正；其他值使用X偏移低字signed修正，Y始终减完整32位偏移。绘制固定空tail，正常后读取独立陈旧latch并精确返回0或1，不以blitter结果伪造返回值。

`audit_order=32`的`0x00450B40`已关闭为`assembly_exact`。它无参数、无callee，以EAX=0和ECX=38执行`rep stosd`，精确清零单个0x98字节持久动作记录，恢复EDI后plain返回；两个caller均不消费返回值。typed helper以固定152字节全零和EAX零返回直接映射，前后canary锁定不越界。

`audit_order=33`的`0x00450B60`已关闭为`platform_adapted`。它只覆盖独立持久动作记录的动作号与base variant 0，更新失败保留前缀。成功后帧号继承更新后ECX高字、资源号继承EDX高字；帧可用时只发布source，以记录flags、记录宽高、入口坐标和固定空tail绘制。正常公共后缀照常执行，indexed帧在palette读取点typed-stop。

`audit_order=34`的`0x00450BD0`已关闭为`platform_adapted`。selector完整值0和1分别查询固定资源2359和2358，其他值忽略帧参数并复用缓存frame与旧共享source；查询失败先发布空frame record，旧source不变。独立共享word精确等于4000时选择flags 20，所有路径固定空tail。正常blitter公共后缀后只覆写EAX低16位为帧宽，高16位保留显式post-blit snapshot；indexed帧在palette读取点typed-stop并阻断宽度后缀。

`audit_order=35`的`0x00450C50`已关闭为`platform_adapted`。它以同一动作号的四组变体绘制纵向状态面板：顶部保留旧record，中段、填充和底部逐阶段完整清零；四次更新后资源/帧号分别保留LST指定的EAX、ECX或EDX高字。中段按signed数量平铺，填充以x87 `7/max`向零结果和signed商计算局部clip，按帧高do-while推进，随后才恢复640×480逻辑clip并绘制底部。固定空tail、公共后缀、零除/溢出和填充步长非终止域均在原发生点停止，正常返回末次blitter完整EAX snapshot。

`audit_order=36`的`0x00450F90`已关闭为`platform_adapted`。tick零时，完成hold非零会清两个状态word，否则只消费一次`random(2)`；状态低word精确1选择右侧变体3，其他值选择左侧变体2。资源号保留更新后EAX高字，帧0以X=260+50×状态、Y=200和固定空tail绘制。正常后tick按signed除25判周期，周期点把packed亮度dword低字加倍并同时写两个word；亮度达64时复位、可增加右侧hold、完整清record并返回1，否则衰减高word，归零时重载、翻转状态、播放固定提示音并返回0。

`audit_order=37`的`0x00451100`已关闭为`platform_adapted`。它先查询资源234F帧0并以record tail绘制底板，再按三个u16阈值设置1像素竖clip，固定三轮查询/绘制帧1；第一轮clip高度来自帧0，后两轮来自帧1。每轮以counter低word半速值命中三格窗，更新选择marker或只清目标低word。最后按递增前counter设置扫描竖条并再画帧1，正常后才恢复全屏clip、递增counter低word；递增后半速值精确62时低word写8000并返回1。

`audit_order=38`的`0x004512B0`已关闭为`platform_adapted`。它先查资源241A帧2，以u16高度除6并与signed等级做低32位乘法；帧0绘制后推进内容Y，等级小于6才按帧0宽和scaled高度发布局部clip。随后帧2在X+4绘制，帧3查询/source发布后才把共享opacity写8并以模式14在X+11/Y+31绘制；正常后恢复全屏，再把帧1画在内容Y+scaled高度。四次draw均保留record tail，负等级不夹值，typed-stop不提前恢复clip或清opacity。

`audit_order=39`的`0x00451420`已关闭为`platform_adapted`。它映射扩展动作record，入口写动作号和两个dword后进入动作更新循环；每轮精确保留`mov al/mov ax`后的EAX高字和`mov dx`后的EDX高字，以三项FFFF局部槽去重帧查询。未缓存帧按查询→owner缓存→局部槽→640/低word除数→指针解引用顺序，直接调用已关闭literal图像模式3右移；command cursor为0时只清前0x98字节record，否则重置动作号/base variant继续更新。只有record、槽、owner和端口完整token全部重复才判非终止。

`audit_order=40`的`0x00451540`已关闭为`platform_adapted`。存储动作号低word为0时直接返回0；非零时重写动作号/base variant并调用更新器，但精确忽略更新EAX。更新后的u16帧索引访问扩展状态的六owner/frame缓存；上一项的三槽局部表只填充前三槽，先发布source再写共享水平位移；绘制坐标为入口扩展坐标减record偏移，使用record flags和固定空tail。正常公共后缀后显式再次清水平位移并返回`field_8c`，indexed或非法缓存typed-stop不得提前清理或发布返回。

`audit_order=41`的`0x004515E0`已关闭为`platform_adapted`。存储动作非零时先完整清0x98 record再更新；三个FFFF局部槽只让每个u16帧索引旋转和绘制一次。signed旋转量正值直连模式3，负值低32位取负后直连模式2，零跳过；随后从同一owner/frame缓存以偏移坐标、record flags和固定空tail绘制。每轮draw正常或缓存跳过后无条件清两个等待word，cursor为0再清record返回1，否则继续更新；更新失败返回0，完整状态重复才判非终止。

`audit_order=42`的`0x00451730`已关闭为`platform_adapted`。函数固定遍历扩展状态从`+0x9C`开始的六个owner槽：owner非空时先读取并释放嵌套image，回调返回后清内部指针，再释放owner并清外部槽；owner空则连孤立image payload也不触及。六轮后只清存储动作低word、`field_bc`与0x98 record，保留`field_b4/field_b8`并返回EAX 0。该完整LST同时把单帧绘制的typed owner边界从三槽修正为六槽，初始化/播放的三个局部FFFF槽仍只触及前三槽。

`audit_order=43`的`0x004517A0`已关闭为`platform_adapted`。完整范围含13行主体与`0x004517D0..0x004517DB`十行外部FUNCTION CHUNK：先调用组A构造包装器，再无条件跳入chunk，把退出清理函数注册给CRT `_atexit`，最后原样返回注册EAX。typed lifecycle port保留构造、退出注册的严格顺序和完整32位注册结果；构造/析构callee分别留给紧邻工作包回收。

`audit_order=44`的`0x004517B0`已关闭为`platform_adapted`。19行包装器按逆序压入组A析构回调、构造回调、数量10、元素尺寸`0x2F34`和全局基址，再调用MSVC向量构造迭代器并保留callee EAX。typed request锁定五个物理参数；上一项静态初始化器已删除opaque `construct_group()`并直接调用本helper，构造EAX只记录而由后续`_atexit` EAX覆盖最终返回。

`audit_order=45`的`0x004517E0`已关闭为`platform_adapted`。17行退出包装器按逆序压入组A析构回调、数量10、元素尺寸`0x2F34`和全局基址，调用MSVC向量析构迭代器并保留callee EAX。静态生命周期CRT端口现显式接收本关闭函数token，注册目标已typed闭合；编译器迭代器与元素析构回调继续由后续工作包回收。

`audit_order=46`的`0x00451800`已关闭为`platform_adapted`。完整范围含13行主体与`0x00451830..0x0045183B`十行外部FUNCTION CHUNK：组B独立静态入口先调用自己的构造包装器，再把组B退出函数token注册给CRT `_atexit`并原样返回注册EAX。组B construction entry暂时opaque，未偷用组A数组参数；组B构造返回仅记录且由注册EAX覆盖。

`audit_order=47`的`0x00451810`已关闭为`platform_adapted`。19行包装器按逆序压入组B析构回调、构造回调、数量8、元素尺寸`0x2B28`和全局基址，调用MSVC向量构造迭代器并保留callee EAX。typed request与组A同形但常量完全独立；组B静态caller已删除临时`construct_group()`并直接调用本helper。

`audit_order=48`的`0x00451840`已关闭为`platform_adapted`。17行退出包装器按逆序压入组B析构回调、数量8、元素尺寸`0x2B28`和全局基址，调用MSVC向量析构迭代器并保留callee EAX。typed request与组A析构同形但使用独立组B常量；静态注册token对应本helper，组B静态生命周期三件套闭合。

`audit_order=49`的`0x00451860`已关闭为`platform_adapted`。完整范围含13行主体与`0x00451880..0x0045188B`十行外部FUNCTION CHUNK：调用单例构造附件并注册独立退出token，最后原样返回`_atexit` EAX。工作包未单列的`0x00451870/0x00451890`两个9行附件也已完整审计：均加载固定对象token后分别尾跳元素构造/析构；静态caller已直连typed constructor，退出注册目标对应typed destructor。

`audit_order=50`的`0x004518A0`已关闭为`platform_adapted`。完整范围含13行主体与`0x004518C0..0x004518CB`十行外部FUNCTION CHUNK；工作包未单列的`0x004518B0/0x004518D0`两个9行附件加载同一固定owner后分别尾跳已关闭几何初始化与资源清理。modern静态caller直接调用closed初始化，退出wrapper直接调用closed清理，仅CRT退出注册保留平台端口；最终返回仍为`_atexit`完整EAX。

`audit_order=51`的`0x004518E0`已关闭为`platform_adapted`。完整9行、无chunk，唯一指令尾跳相邻`0x004518F0`；typed thunk只转发一次同一绑定对象，不附加参数、状态或返回后处理。相邻两层callee现均已关闭，thunk不再持有opaque边界。

`audit_order=52`的`0x004518F0`已关闭为`platform_adapted`。完整14行、无chunk；按LST先压固定几何owner，再将固定绑定对象写入ECX，单次直连已关闭`audit_order=106`深层initializer并返回绑定对象token EAX。typed wrapper显式发布两个32位token，深层callee端口已删除。

`audit_order=53`的`0x00451900`已关闭为`platform_adapted`。完整范围含13行主体与`0x00451920..0x0045192B`十行外部FUNCTION CHUNK；工作包未单列的`0x00451910/0x00451930`两个9行附件加载同一固定文件owner后分别尾跳已关闭文件构造与析构。modern以`std::optional<LegacyFile>`在原时点真实建立和销毁typed文件对象，仅CRT退出注册保留平台端口；最终返回为`_atexit`完整EAX。

`audit_order=54`的`0x00451940`已关闭为`platform_adapted`。完整108行、无chunk；严格恢复`all_map2.tsw`路径、旋转缓存与旧背景释放、固定variant零加载、命令流转换、signed `640/divisor`、mode3循环右移、双word门下的三帧动作缓存初始化及三个完成word高地址到低地址发布。load失败唯一返回0；成功与正常跳过路径返回全1；除零和closed callee故障域按原访问点typed-stop。真实物理槽1完成640×400转换与shift160旋转。

`audit_order=55`的`0x00451A20`已关闭为`platform_adapted`。完整58行、无chunk；固定执行一次全局reset、三个对象token reset、96个dword正向清零、角色组B八槽与组A十槽遍历。两个对象reset callee仍属于后续工作包，以typed端口隔离；最终返回保留末个组A callee完整EAX。定向测试锁定22次调用顺序、384字节表在角色循环前清零、全部18个物理token及步长。

`audit_order=56`的`0x00451B10`已关闭为`platform_adapted`。完整1351行、60个静态call站点、48个标签、无chunk；邻接显示surface释放/创建附件一并typed闭合但只计主函数。实现覆盖固定零/全1块、陈旧队伍总数与补位word、窗口与几何、`battle.ffd`零敌人早退、已关闭背景初始化直连、组B敌人与组A队伍物化、1–4人固定坐标、缺席槽陈旧坐标、三组x87比率、随机/顺序补位、敌方随机动作及最终u32回绕判定。第九敌人在首次对象访问typed-stop；镜像敌方保留callee后陈旧ECX高word。

`audit_order=57`的`0x004527E0`已关闭为`platform_adapted`。完整1002行、74个静态call站点、37个标签、无chunk；严格恢复640×480双raw快照、每块`0x96000`字节、第二display槽先复制、mode 0/1/2三类34帧进入转场、mode 1另33帧退出、四token条件释放、完整clip恢复、三个独立音乐区间及两条低概率角色事件。mode 1的34项x87正弦整数表由原常量与截断控制字生成后冻结；mode 0两次直连已关闭固定帧绘制并回收共享source状态。raw/surface越界只在首个真实行访问typed-stop，保留此前分配与像素副作用。

`audit_order=58`的`0x004530A0`已关闭为`platform_adapted`。完整171行、12个静态call站点、6个标签、无chunk；系统指标按1后0查询后创建screen surface，先写480项永不读取的`random(20)+15`表，再固定执行两轮479→0逐行rectangle混合。零偏移首轮即把完成计数累加到480，但signed `<=480`条件强制第二轮并达到960；正常总计1440次随机、960次行操作、两次secondary捕获和两次临时surface copy。六个入口参数只有前两个被读，caller的`random(3)`结果保持未读；最终只释放screen surface并返回完整release EAX。

`audit_order=59`的`0x00453200`已关闭为`platform_adapted`。完整412行、44个静态call站点、18个标签、无chunk；恢复活动发布、音乐门、六阶段零早退、target surface锁定/解锁、渲染中止、选择延迟刷新、UI低word、固定帧、选中角色面板、三类陈旧ECX snapshot、跨模块效果/头像/对话、双倒计时、内部bit17返回3、overlay/surface尾与截图回绕。已关闭固定帧、画面效果、动作更新、九宫格、独立帧、效果链、头像链、对话、倒计时和BMP helper全部直接组合；角色映射和内部bit缺失只在原首次真实访问typed-stop，临时surface零token只在立即虚调用点停止。

`audit_order=60`的`0x00453580`已关闭为`platform_adapted`。完整508行、21个静态call站点、22个标签、无chunk；恢复source先发布、全屏clip、双抑制门、零/正/负rotation、上下split带、三通道颜色byte循环、遭遇ID门、标准三surface阶段、alternate framebuffer RGB阶段、cadence、signed stage clamp和fade终态。六次blitter、三次旋转缓存、两次literal旋转和四次颜色调整全部直连已关闭typed实现；仅两次DirectDraw虚操作保留窄端口。三个caller均已回收：逐帧协调器一次，画面转场首阶段及mode 0第二阶段各一次。

`audit_order=61`的`0x004539B0`已关闭为`platform_adapted`。权威主体`0x004539B0..0x00455CCF`完整3836行、202个静态call站点与137个局部标签，另纳入`0x00498350..0x00498364`的21字节异常FUNCTION CHUNK。实现恢复主/次动作号、terminal早退、动作99终态、10个特殊动作、36项稀疏jump table中的27个普通case、三相404 push/pop、deformation异常owner释放、内部bit、倒计时、状态指示、刻度扫描、低宽度packed状态、signed值和清屏前缀。8类已关闭callee直接组合，其余87个唯一callee边界保留单一typed token端口；组A/B对象、状态表、target、内部bit与framebuffer都只在原实际访问点typed-stop。

`audit_order=62`的`0x00455D60`已关闭为`platform_adapted`。权威主体`0x00455D60..0x0045662F`完整993行、51个静态call站点与33个局部标签，另纳入`0x00498370..0x00498384`的21字节异常FUNCTION CHUNK。实现恢复动作100/200/300、17项稀疏jump table的9个有效case、双side攻击尾差异、deformation生命周期、目标phase、低byte回绕、组B wave记录、220/350坐标、镜像、callee陈旧EAX高word、504字节workspace及18个间隔全1头。两个已关闭deformation callee和framebuffer清屏直接组合，其余35个唯一callee边界复用共享typed token端口。

`audit_order=63`的`0x00456680`已关闭为`platform_adapted`。完整权威LST主体`0x00456680..0x0045769B`共1795行、101个静态call站点、84个局部标签且无外部chunk。实现恢复组A效果门、AI terminal统计与无上限随机组B选择、十槽actor queue、idle角色frame启动、两类completed选择循环、动作准备与清理、u32回合阈值、resolved word54最大值、陈旧commit参数、bit`0x4000→0x8000`同调用穿透和固定最终尾。已关闭`0x004539B0`动作主分派caller直接组合，其余45个唯一callee保留共享typed token端口；相邻actor-start guard、target guard highword和turn word保持独立。

`audit_order=64`的`0x004576A0`已关闭为`platform_adapted`。完整权威LST主体`0x004576A0..0x004582AB`共1356行、86个静态call站点、66个局部标签且无外部chunk。实现恢复组B live update与直接EAX早退、phase mode、三类无上限随机选择、packed signed/bit状态、陈旧EDX低byte覆盖、固定八/九边界扫描、动作完成后组A槽清理、陈旧EAX/ECX高word、battle bit、completion资源与surface全1前缀及完整最终EAX。已关闭`0x00455D60`对手动作分派caller直接组合，其余45个唯一callee保留共享typed token端口；idle失败和动作未完成保留路径相关陈旧EBX，而非反编译合理化的恒1。

`audit_order=65`的`0x004582B0`已关闭为`platform_adapted`。完整权威LST主体`0x004582B0..0x00458DDA`共1257行、65个静态call站点、64个局部标签且无外部chunk。实现恢复八槽主/备用双152字节效果记录、两类animation counter、主资源镜像宽度的参数token陈旧高word、主EAX/ECX与备用EDX/EAX声像高word、resource value/owner释放差异、signed status位、三行奖励、pending和final gate。备用零owner严格在play/set-pan及pan清零后首个解引用处typed-stop；最终EDX按alternate-active、末callee、active槽物理地址或辅助奖励signed扩展逐路径更新。32个唯一callee统一由typed token端口发布完整寄存器与输出快照。

`audit_order=66`的`0x00458DE0`已关闭为`platform_adapted`。完整权威LST主体`0x00458DE0..0x004599AF`共1350行、71个静态call站点、80个局部标签且无外部chunk。实现恢复第二套八槽主/备用记录、主sample先于owner解引用、双offset AND门、四类镜像坐标、备用清主pan而保留自身pan、群体A/B状态发布、三套奖励路径与两次final gate。组A双guard、奖励gate、special mode和汇总陈旧EAX/ECX高word均独立保留；组B奖励offset跨角色累计，auxiliary和packed-high发布后清零再写数组。24个唯一callee统一由typed token端口发布完整寄存器与输出快照。

`audit_order=67`的`0x004599B0`已关闭为`platform_adapted`。完整权威LST主体`0x004599B0..0x00459BE9`共269行、10个静态call站点、14个局部标签且无外部chunk。实现恢复单条主152字节效果记录、signed status前缀、初始化失败清备用record、owner首次解引用、offset双低word AND门、镜像base offset、X完整减法与Y低word减法、sample参数坐标高word、左右pan保留play ECX/EDX高word、data token绘制、条件value释放、owner内部value清零和complete尾。唯一caller组B帧已删除opaque token并直接组合typed状态；pending只在子返回1时清全1，typed-stop与port call计数直接传播。

`audit_order=68`的`0x00459BF0`已关闭为`platform_adapted`。完整权威LST主体`0x00459BF0..0x00459D04`从proc到endp共130行、4个静态call站点、4个局部标签且无外部chunk。实现恢复source-zero先于slot访问、i8小于等于-32清零、坐标先于record写、global mode snapshot、初始化失败完整EAX/EDX、lookup分别保留初始化EAX/EDX高word、owner首读、三项signed强度发布、signed16坐标减完整dword offset、u16宽高绘制、无resource释放、强度u8减4和render EDX返回。唯一caller八槽效果协调器已删除opaque token并直接组合；子返回EDX继续供父final gate使用，返回1才清pending。

`audit_order=69`的`0x00459D10`已关闭为`platform_adapted`。完整权威LST主体`0x00459D10..0x0045A97C`从proc到endp共1363行、51个静态call站点、62个局部标签且无外部chunk。实现恢复字体前缀、顶部active队伍条、两类signed byte脉冲、十槽过滤、三次及条件第四次blocked查询、名字叠绘、actor值非对称平滑、primary/secondary/tertiary三组数值差异、bit27绝对值BUG、x87扩展/float两种向零转换、零分母整数不定低dword、底部signed三分步进和路径相关返回。四处x87 helper直接闭合，其余23类callee保留统一HUD端口。画面转场两处和逐帧协调器一处caller均已删除伪边界并直连typed HUD，父级typed-stop即时传播。

`audit_order=70`的`0x0045A980`已关闭为`platform_adapted`。完整权威LST主体`0x0045A980..0x0045A9F4`从proc到endp共64行、1个静态call站点、4个局部标签且无外部chunk。实现恢复group完整值等于1才选组A、其他值选组B、两组token低32位回绕、global-zero与global-nonzero组A路径不同陈旧EAX、组A caller EDX保留、组B陈旧EAX/EDX以及callee完整EAX严格等于1才返回1。两个静态caller都在尚未关闭的`0x0045EB40`，未提前回收。

`audit_order=71`的`0x0045AA00`已关闭为`platform_adapted`。完整权威LST主体`0x0045AA00..0x0045ADEC`从proc到endp共465行、20个静态call站点、26个局部标签且无外部chunk。实现恢复完整group选择、组A第二byte完成计数、阈值控制的32字节逆向清零、三刷新、双组对象重置、角色顺序固定左移、removed unsigned终止、继续配置和五dword记录清零，以及组B全1早退、坐标低word累加、描述符bit5延迟、动作发布、闭区间计数、signed完成比较和末位重置。组A/组B帧两个caller均删除opaque token并直接组合typed实现，子typed-stop阻止父清理。

`audit_order=72`的`0x0045ADF0`已关闭为`platform_adapted`。完整权威LST主体`0x0045ADF0..0x0045AF8E`从proc到endp共186行、11个静态call站点、7个局部标签且无外部chunk。实现恢复完整target入口门、组B先重置、unsigned组A扫描、source跳过、i16目标过滤、动态重载group B上界、首个非terminal候选发布、packed低byte终止条件、五callee清理、十项角色顺序和126 dword选择工作区固定清零。角色顺序与组A帧actor queue、前一最终角色步进保持同一物理数组。唯一组A帧caller已删除opaque token并直连typed实现。

`audit_order=73`的`0x0045AF90`已关闭为`platform_adapted`。完整权威LST主体`0x0045AF90..0x0045B0D7`从proc到endp共138行、9个静态call站点、2个局部标签且无外部chunk。实现恢复三项snapshot逐项低word比较与陈旧高word零调用早退、固定双surface循环、Miles serve、lock/unlock、640×480 viewport、三项i16算术右移半值与factor 1/2颜色调用、三snapshot更新、最终surface发布及末callee完整EAX。角色动作分派四处、对手动作分派一处、八槽效果帧一处和群体效果帧一处共七个已关闭caller全部删除opaque token并直连统一typed刷新器；刷新快照与surface状态只保存在动作/效果端口共同虚继承的单一typed存储，三个后续未关闭caller不提前修改。

`audit_order=74`的`0x0045B0E0`已关闭为`platform_adapted`。完整权威LST主体`0x0045B0E0..0x0045B18B`从proc到endp共92行、2个静态call站点、4个局部标签且无外部chunk。实现恢复metric表与角色顺序表两张18 dword物理表固定清零、caller ECX保存槽与byte/word局部物理别名、组B从索引0扫描、组A从索引8扫描、两组动态数量重载、固定角色基址与步长、i16符号扩展、组A数量加8的u32回绕门、路径相关EAX/EDX及ECX恢复。启动、逐帧协调、角色动作、对手动作两处和最终角色两处共七个已关闭caller全部直连；两张表只保存在三类端口共同虚继承的单一typed存储。

`audit_order=75`的`0x0045B190`已关闭为`platform_adapted`。完整权威LST主体`0x0045B190..0x0045B27A`从proc到endp共126行、0个静态call、11个局部标签且无外部chunk。实现恢复两组数量u32加法回绕、18槽初始非零候选与mask精确值1过滤、候选metric重复读取、组B候选后缀和组A索引8起点的signed严格最小值选择、后续零metric非对称、相等稳定顺序、每轮mask先写与顺序表后写，以及正常尾部18 dword mask清零。metric表、mask和前项角色顺序表只保存在单一共享typed状态；异常数量按原访问顺序在metric、mask或顺序表首次真实越界点typed-stop且不执行最终清mask。战斗启动、画面转场两处、逐帧协调、角色动作、对手动作两处和最终角色两处共九个已关闭caller全部删除opaque边界并直连，两个后续caller未提前修改。

`audit_order=76`的`0x0045B280`已关闭为`platform_adapted`。完整权威LST主体`0x0045B280..0x0045B59C`从proc到endp共384行、3个静态call、32个局部标签且无外部chunk。实现恢复byte及双mode四项入口早退、双组固定token与步长、callee入口陈旧EAX、AX的i16符号扩展和组相关加8、当前同组低metric稳定升序前缀、插入时额外复制陈旧尾槽的原始BUG、18槽剩余候选过滤、异组优先最小值、后续零metric非对称、配对角色后接当前角色双发布、固定总数精确相等循环、尾部18 dword mask清零、ready发布、值18哨兵扫描及已关闭顺序重建直连。入口控制、metric、mask、顺序表和ready共用单一typed角色状态；异常只在真实metric、mask或顺序表访问点停止。唯一逐帧协调caller已删除opaque边界并直连，子typed-stop阻止后续帧阶段。

`audit_order=77`的`0x0045B5A0`已关闭为`platform_adapted`。完整权威LST主体`0x0045B5A0..0x0045B5D2`从proc到endp共33行、0个静态call、3个局部标签且无外部chunk。实现固定读取一次group B完整u32数量、扫描18槽角色顺序、按signed `< 8`稳定压缩、每次成功复制后才做精确数量相等早退、数量0不早退的原始BUG、第九次真实八槽输出store停点、请求过大时输入耗尽正常返回、输出尾不清零及路径相关EAX/ECX/EDX。八槽组B顺序与metric、mask、18槽顺序统一在单一typed角色状态，既有动作状态重复数组已移除。战斗启动、对手动作两处和最终角色两处共五个已关闭caller全部删除opaque边界并直连，一个后续caller未提前修改。

`audit_order=78`的`0x0045B5E0`已关闭为`platform_adapted`。完整权威LST主体`0x0045B5E0..0x0045B62F`从proc到endp共52行、2个静态call、4个局部标签且无外部chunk。实现恢复两组u32数量低32位求和与signed非正早退、固定初始和次迭代、18槽角色顺序逐槽实时读取、signed `< 8`组B分派、完整dword减8组A分派、两组动态数量按槽重载、比较失败仍推进输入、callee最终陈旧EAX及第19次真实顺序表读取停点。typed组合上下文直接调用已关闭组B与组A帧并强制复用同一角色metric物理状态；画面转场两处与逐帧协调一处共三个已关闭caller全部删除opaque边界并直连，子typed-stop阻止caller后续surface或帧阶段。

`audit_order=79`的`0x0045B630`已关闭为`platform_adapted`。完整权威LST主体`0x0045B630..0x0045BD0C`从proc到endp共637行、9个静态call、1个条件跳转、1个局部标签且无外部chunk。实现恢复显示surface、六槽旋转缓存与渲染owner三项已关闭资源释放直连、非零分配token条件释放、234项固定写操作、3300次物理写、13106字节、全部byte/word/dword宽度、重复写序、两段callee之间的分界及最终固定EAX零。完整写元组散列固定；已映射渲染、启动、metric、角色顺序、mask、数量和优先状态继续使用唯一typed存储，未映射全局使用单一按地址字节像，八槽group B顺序表按原始不触碰。停止全部sample直接组合已关闭命令，四个未关闭资源、stream和后置初始化callee保留窄端口。三个caller都在尚未关闭函数中，不提前回收。

`audit_order=80`的`0x0045BD10`已关闭为`platform_adapted`。完整权威LST主体`0x0045BD10..0x0045BD8B`从proc到endp共58行、3个静态call、1个条件跳转、1个局部标签且无外部chunk。实现按原顺序把共享channel delta发布到红绿蓝三个完整dword偏移，直接组合三个已关闭渲染颜色helper，对唯一owned framebuffer完整`0x4B000`像素依次执行红单像素、绿双lane与蓝双lane signed偏移，并保留红通道末像素dword guard读取。三callee后共享delta按低32位减2，再按signed与-32比较；严格大于时返回0，小于等于时清零并返回1。两个caller都在尚未关闭的同一函数中，不提前回收。

`audit_order=81`的`0x0045BD90`已关闭为`platform_adapted`。完整权威LST主体`0x0045BD90..0x0045C00C`从proc到endp共299行、8个静态call、19个分支、17个局部标签且无外部chunk。实现恢复入口ECX scratch、u16调用计数、正phase半分、累计word、方向双路径非对称、共享actor delta、组A后组B、固定角色token和步长、getter/setter可变arg与scratch、setter后动态数量重载、完成threshold signed域、每次最多30累计消费、completion latch、精确mode一重装、固定返回0/1及路径相关ECX/EDX。组计数、packed reward和final gate状态收敛为单一typed存储，全局重置同址字段也回收；单体效果一处与群体效果两处caller全部删除opaque边界并直连，角色typed-stop阻止父级后续收束。

`audit_order=82`的`0x0045C010`已关闭为`platform_adapted`。完整权威LST主体`0x0045C010..0x0045D17A`从proc到endp共1915行、68个静态call、114个分支、59个局部标签且无外部chunk。实现恢复双UI入口门、当前角色组别、两组global/effect/side组合、单目标、group-wide和staged扫描、u16 scan/delay回绕、signed动态数量、18槽处理与三组反馈数组、两种完成门、反馈actor、发起计数、奖励序列、完整framebuffer填充和各路径差异化清理。单体八处与群体五处已关闭callee全部直连，剩余十类callee保留窄端口；单体/群体公共状态纠正为同一18槽虚共享存储，actor发布数组与战斗初始化重置收敛为同一18槽虚共享端口，全局重置同址字段回收。唯一主帧caller删除opaque完成门并直连，子typed-stop阻止固定帧。

`audit_order=83`的`0x0045D180`已关闭为`platform_adapted`。完整权威LST主体`0x0045D180..0x0045D242`从proc到endp共109行、2个静态call、7个分支、6个局部标签且无外部chunk。实现恢复低word零入口、全局head地址伪节点、32位next token链遍历、精确item id、两项i16数量signed总和99门、selector精确1分流、u16回绕、固定`0xB0`头插分配、44 dword清零、完整item参数初始化、offset`0x2C` bit15和payload token返回。玩家道具链复用唯一世界道具状态；零分配先发布零head再在首个清零访问typed-stop。动作分派两处与组B帧两处已关闭caller删除opaque地址并直连，其余九处保留后续回收。

`audit_order=84`的`0x0045D250`已关闭为`platform_adapted`。完整权威LST主体`0x0045D250..0x0045D292`从proc到endp共42行、0个静态call、6个分支、3个局部标签且无外部chunk。实现恢复head与单节点早退、u16 item id稳定升序、每次比较前清当前selected count、相邻三link交换、交换后从head重扫和路径相关EAX。玩家道具排序复用唯一世界道具状态并同步物理token与host list顺序；未知head在首次next读取处typed-stop，未知next在已清当前selected count后停止。唯一startup caller删除第一全局阶段opaque枚举并直连，子typed-stop阻断第二阶段与后续启动流程。

`audit_order=85`的`0x0045D2A0`已关闭为`platform_adapted`。完整权威LST主体`0x0045D2A0..0x0045D2E9`从proc到endp共50行、0个静态call、7个分支、4个局部标签且无外部chunk。实现恢复固定四根循环、前一阶段陈旧EAX、单根空链与单节点跳转、u16 item id稳定升序、相邻三link交换和交换后仅从当前sentinel重扫；四队伍链完全不清selected count。四根复用唯一世界道具状态并同步物理token与host list顺序；缺根、未知head与未知next均在原首次访问处typed-stop并保留跨根前缀。唯一startup caller删除第二全局阶段opaque枚举并直连，子typed-stop阻断资料绑定与后续启动流程。

`audit_order=86`的`0x0045D2F0`已关闭为`platform_adapted`。完整权威LST主体`0x0045D2F0..0x0045D3DE`从proc到endp共109行、4个静态call、7个分支、5个局部标签且无外部chunk。实现恢复三step的x87 C3零/NaN门、参数非零时signed计数先判负再递减、计数重读、非负三current加step、负值三step按dword替换为target、蓝绿红三次向零qword转换及低dword、固定`0x3C000`像素RGB调整。三个转换和颜色callee均已关闭并直连。九float与计数由逐帧协调和全局重置虚共享唯一typed状态；协调器在overlay最终化后直连本入口，子framebuffer失败阻断surface与截图尾；全局重置十个同址dword从未映射字节像回收。

`audit_order=87`的`0x0045D3E0`已关闭为`platform_adapted`。完整权威LST主体`0x0045D3E0..0x0045D48E`从proc到endp共69行、3个静态call、0个分支、0个局部标签且无外部chunk。实现恢复七项signed参数按countdown、三current、三target的发布顺序，current float舍入后向零qword转换，target减低dword的32位回绕，单一x87 countdown分母下红绿蓝step除法，零分母无穷与indefinite NaN、负分母，以及蓝通道EAX/ECX/EDX尾寄存器。三个转换callee已关闭并直连。逐帧协调器与单体/群体效果三个已关闭caller删除opaque边界；效果record七个u16按i16扩展，单体保留EDX、群体恢复完整尾寄存器。初始化门与九float和计数由同一端口虚共享，删除逐帧和效果状态中的两个副本；全局重置只清颜色值并保留该门。

`audit_order=88`的`0x0045D490`已关闭为`platform_adapted`。完整权威LST主体`0x0045D490..0x0045D685`从proc到endp共218行、7个静态call、9个分支、6个局部标签且无外部chunk。实现恢复terminal/active/message三门、action execution先写、`active+2`工作区真实store、source全1短路、组A建立与状态查询、五dword记录末store、1-based组B预查询、零基动态数量扫描、首个不可用发布和全部可用收束。active与线性索引均保留u32回绕；callee后重读可能动态改写的source、secondary与组B数量，不加现代上限。最终角色、动作工作区和metric数量复用既有typed状态，terminal/message进一步与startup、动作和效果路径收敛为唯一共享phase端口，只新增source与双门；全局重置按原写集合清标量及工作区`0..9/16..95`，保留其余槽和角色记录。唯一逐帧caller删除第三前置opaque stage并直连，子typed-stop阻断metric及后续全帧。

`audit_order=89`的`0x0045D690`已关闭为`platform_adapted`。完整权威LST主体`0x0045D690..0x0045D801`从proc到endp共191行、141条实际指令、21个静态call、6个分支、5个局部标签且无外部chunk。实现保留共享primary入口snapshot、零值陈旧寄存器早退、只比较kind低word、kind 1的32位取负与三参数提交、kind 2/4的双零局部输出、i16符号扩展、回绕delta signed门、路径相关secondary发布，以及不同动作标识和提交参数顺序。kind 2尾写辅助奖励并清primary；kind 4先清primary再只替换打包奖励高word。动作累计/主反馈、辅助奖励/次反馈和打包奖励高word分别回收到唯一typed存储；全局重置只清原写集合中的primary。玩家动作双caller、对手动作单caller和效果协调器三caller全部删除旧token并直连，普通返回寄存器继续不被父路径消费。

`audit_order=90`的`0x0045D810`已关闭为`platform_adapted`。完整权威LST主体`0x0045D810..0x0045D8E0`从proc到endp共107行、73条实际指令、1个静态call、5个分支、4个局部标签且无外部chunk。实现恢复八dword局部线段、八槽u16计数先增与回绕、计数乘完整步数的低32位`signed imul`门、无现代上限的重复光栅推进、每轮动态计数重读、只比较X的原BUG、XY低32位输出、命中后清当前槽，以及路径相关EAX/ECX/EDX。唯一线段推进callee已关闭并直连；第九槽只在原始首次计数访问处typed-stop。八槽计数与共享XY由单体和群体效果唯一共用；单体两处与群体一处caller均删除旧token并直连，单体子typed-stop阻断后续音效、效果绑定、完成写入与帧逻辑。

`audit_order=91`的`0x0045D8F0`已关闭为`platform_adapted`。完整权威LST主体`0x0045D8F0..0x0045DED6`从proc到endp共731行、436条实际指令、48个call、53个分支、44个局部标签且无外部chunk。实现恢复调试总门、左右Control短路、Control块14键和无Control H/J/P尾键的固定查询顺序；19次raw DIK查询直连typed快照，8次Sleep保留阻塞延迟端口，其余21次音频、文字、角色与数值调用保留12类窄端口。F1/X/K/F9/F2/P精确切换、Z/D/F/V/H/J动态两组循环、AI跳过、E键返回0、C键七dword清理与陈旧索引、W键18槽发布及126 dword记录重置均按原写序实现。组数量、优先索引、最终角色门、发布数组、效果计数、actor delta、消息、速度、截图和启动记录复用既有typed owner；完整`0x1C`启动记录不再只建模部分字段。唯一逐帧caller删除第六前置opaque门并直连；零返回或typed-stop保留前置角色预处理、metric和顺序副作用并阻断surface及后续帧。

`audit_order=92`的`0x0045DEE0`已关闭为`platform_adapted`。完整权威LST主体`0x0045DEE0..0x0045E576`从proc到endp共736行、460条实际指令、45个call、15个分支、15个局部标签且无外部chunk。实现恢复调试总门、字体前后生命周期、组B生命/锁定/命令/等级、组A锁定/命令、攻击/等待/选择三组动态顺序、十项固定状态文字、255字节格式缓冲和全部CP950字面值。15次`wsprintfA`本地等价格式化；18次文字绘制与12次字体/角色查询保留typed端口。组B双行标记按signed坐标、唯一framebuffer几何、低32位乘法、u16宽度和每列上行后下行顺序直接写唯一framebuffer；第一行成功、第二行越界保留前缀。三类顺序分别在第19/11/19次真实读取处typed-stop；frame divisor零在最后signed `idiv`处停止且不执行字体恢复。两组数量、优先索引、启动记录、最终角色当前/发布值/顺序/预帧门/frame gate、动作packed counter、共享message state、反馈actor、调试开关和framebuffer复用既有owner；新增叠加调用门与其余显示源按全局重置原写集合同步。唯一逐帧caller删除条件反向的opaque边界，改为门精确等于1时直连；子typed-stop阻断结果判定、上下文提示、颜色、surface和截图。

`audit_order=93`的`0x0045E580`已关闭为`platform_adapted`。完整权威LST主体`0x0045E580..0x0045E651`从proc到endp共93行、51条实际指令、5个call、9个分支、4个局部标签且无外部chunk。组A按动态数量减打包高word和已结算低word的u32回绕结果，与完成byte作unsigned门；组B以同一packed dword的低byte减byte2，再与动态数量作signed比较，强制值只接受精确1。两侧均保持先读暗化门、再写结果latch、最后比较门snapshot；组A终态依次暂停音频、调用结果整理、读取共享message并把逐帧active发布为2，再由message 104和battle mode bit3清零；组B终态不暂停音频，清逐帧active并返回整理完整EAX。两个全帧暗化caller全部直连已关闭typed实现；两次尚未审计结果整理复用单一窄端口并登记到第96项。组A结果整理后重新读取组B packed/数量、暗化门与delta，允许同帧两侧分别暗化和整理。两组数量、最终角色排除/完成数、动作phase/packed counter、message、battle mode、逐帧active、framebuffer与颜色偏移复用既有owner；结果latch进一步与组A/B帧和调试快捷键回收到唯一状态端口，仅暗化门与强制值为其余新增状态。全局重置按原宽度清既有owner和三项结果状态，phase高word、packed byte2及暗化delta保持。唯一逐帧caller删除第一个post-input opaque stage并直连；子typed-stop阻断上下文提示、颜色、surface和截图。

`audit_order=94`的`0x0045E660`已关闭为`platform_adapted`。完整权威LST主体`0x0045E660..0x0045E7A3`从proc到endp共169行、92条实际指令、6个call、11个跳转、10个局部标签且无外部chunk；尾后3项直接跳转表与30字节间接switch表一并审计。入口消息门先取snapshot，逐帧计数u32递增后按signed与300比较；达到门且消息门bit31清零时返回递增计数，bit31置位继续，回绕为0也继续。30项switch精确恢复七个通用鼠标提示、case 3门及23个默认case，范围外值走默认；默认路径再按消息门bit31、active actor和镜像模式选择四类动作、鼠标或i16坐标、base variant及offset selector。六个call全部直连已关闭偏移动作帧绘制并复用唯一持久状态端口；只有消息actor路径在callee精确返回1后清消息辅助值，typed-stop保留此前前缀。共享message、动作消息门/辅助值/坐标、最终角色active/pre-frame门、启动镜像和逐帧渲染依赖均复用既有owner，仅新增计数和静态资源选择。全局重置同步清原写集合中的消息门与镜像，保持辅助值、坐标、计数和静态选择。唯一逐帧caller删除最后post-input opaque stage并直连；提示typed-stop保留结果判定前缀并阻断颜色、surface和截图。

`audit_order=95`的`0x0045E7D0`已关闭为`platform_adapted`。完整权威LST主体`0x0045E7D0..0x0045E9B3`从proc到endp共227行、144条实际指令、4个call、14个跳转、13个局部标签且无外部chunk。实现以live phase无符号索引固定16项signed偏移表，按原signed余2构造第一组640×480奇偶矩形并提交等待Blt；第一Blt后重读phase，按`offset×1280`以dword粒度清唯一framebuffer前缀，再次重读phase构造第二组暴露带矩形并提交。两个直接surface选择callee已关闭，两个矩形Blt保留完整destination/source、source token、等待标志和零尾typed端口；第一/第二surface零token、三次表越界和清零dword边界均在原实际访问点typed-stop并保留前缀。第二Blt后动态重读节拍计数/上限/mode，按signed门推进；mode bit使phase在0/1间按原余数切换，否则u32递增，phase到10时返回10后清完成门与phase。phase/节拍上限回收到唯一state port并由全局重置清零，节拍计数保持。逐帧caller恢复完成门任意非零而非精确1的原比较：非零且mode bit清零走整surface提交，否则直连纵向位移；旧opaque枚举保留reserved槽。位移typed-stop保留颜色累加前缀并阻断截图。

`audit_order=96`的`0x0045E9C0`已关闭为`platform_adapted`。完整权威LST主体`0x0045E9C0..0x0045EA26`从proc到endp共53行、35条实际指令、2个call、4个跳转、4个局部标签且无外部chunk。实现精确扫描两个live u16奖励ID；`mov ax`只覆盖低word，入口或前一玩家道具callee返回的EAX高word继续作为完整item参数。两项之后按signed正数门，以固定`0x0300`和selector零调用已关闭玩家道具双数量步进；每轮callee后重新读取组B数量，再以signed索引门决定继续，不增加现代上限。全部完成后依次清结果完成高word、唯一组B metric数量和两项奖励ID，返回清零前最后读取的完整数量。任何子typed-stop均保留已完成奖励与玩家道具链副作用，并阻断三项尾store。奖励ID/完成双word由唯一结果整理state port持有；全局重置清完成双word但保留奖励ID。启动期重复组B数量副本删除，启动和全局重置均直写actor metric唯一owner。结果判定两处旧窄端口保留reserved枚举槽并直接组合typed整理：组A传入暂停音频完整EAX，组B传入暗化返回1；组A整理清数量后，同帧组B门动态重读观察零值。整理typed-stop阻断frame active发布、另一侧判定及全部后续逐帧阶段。

`audit_order=97`的`0x0045EA30`已关闭为`platform_adapted`。完整权威LST主体`0x0045EA30..0x0045EA70`从proc到endp共33行、21条实际指令、3个call、2个跳转、2个局部标签且无外部chunk。首call直接组合已关闭战斗渲染资源清理，按辅助buffer、surface行表、primary行表顺序释放唯一startup render owner。随后以固定组A基址和`0x2F34`步长调用10个对象析构，再以固定组B基址和`0x2B28`步长调用8个对象析构；不读取动态数量、不消费callee返回、不增加现代上限。两类析构仍属后续工作包，以两类窄端口保留完整对象token、索引与三寄存器返回；函数尾完整返回第八个组B析构寄存器。已关闭总销毁caller把旧通用地址枚举保留为reserved数值槽，在原字体/前一资源和后一资源之间直接调用typed战斗运行时销毁。SDL持有唯一战斗启动/渲染状态并接入同一入口；对象析构宿主后端未关闭时仍固定发出18次调用。

`audit_order=98`的`0x0045EA80`已关闭为`platform_adapted`。完整权威LST主体`0x0045EA80..0x0045EB3C`从proc到endp共82行、50条实际指令、4个call、3个跳转、1个局部标签且无外部chunk。入口以u32回绕计算`组A基址+index*0x2F34`，不作十槽预验；选中对象callee只有精确返回1才继续，否则透传完整三寄存器。随后查询固定首对象，callee后读取battle mode bit`0x200`；查询零或mode置位时，依次显示固定CP950“無法撤退!!”五参数文字并以live混音等级播放`0x8C`样本，完整尾返回来自样本callee且不写成功状态。成功路径在查询/mode之后读取组B数量低byte，按原写序发布两个完成门1、结果latch0、辅助latch0、调试重置门0、调试叠加门0、选择token全1、message0、packed counter仅低byte替换和暗化门1；返回EAX0、ECX只替换CL、EDX保留查询值。调试快捷键state抽为无循环依赖共享port，叠加门移到动作分派/逐帧/reset共用的独立gate port；撤退四项新状态唯一持有，全局重置清两完成门与选择token并保留辅助latch。已关闭动作分派case3删除旧地址call，在选择计算、fade和300帧延迟后直接组合typed撤退提交，仍忽略子返回。

`audit_order=99`的`0x0045EB40`已关闭为`platform_adapted`。完整权威LST主体`0x0045EB40..0x0045EC5A`从proc到endp共133行、94条实际指令、7个call、7个跳转、7个局部标签且无外部chunk。入口只各读一次组B/组A数量并低32位相加，signed非正时以总数、组A数量和caller EDX直接返回；正数按入口总数固定遍历18槽物理角色顺序，不重读上界、不加现代上限。每轮首次顺序值以signed `<8`固定本轮组别，但前置对象、ready参数、ready标记、提交对象和发布索引均在各自原指令点重读live槽；两类对象token保持u32回绕。前置callee入口锁定组B`0x565/0x159`与组A`0x3EF/0xBCD`陈旧乘积；已关闭ready查询扩展为完整三寄存器结果并直接组合，只有精确1才先置唯一activation latch、再向启动期共享ready槽写全1。提交callee只有精确1才向共享actor publication写归一索引并调用待关闭动作记录移除；ready/publication越界分别只在原store停止，第19次角色顺序读取停止且保留前18轮。双方数量、角色顺序、activation latch统一归actor metric，ready槽复用启动reset块，publication复用既有效果/启动state；组A最终步进清同一latch，全局重置同步清零。逐帧caller保留前一stage并把其EDX作为早退快照，删除旧第二followup调用后直接组合typed提交；子typed-stop阻断效果协调及后续绘制。

`audit_order=100`的`0x0045EC60`已关闭为`platform_adapted`。完整权威LST主体`0x0045EC60..0x0045EC71`从proc到endp共13行、4条实际指令、0个call、0个跳转、0个局部标签且无外部chunk。两参数cdecl叶函数先把完整u32索引读入ECX，再只读取第二栈槽低16位覆盖AX，最后向`0x004FF2EA+index*2`写一个word并返回；EAX高16位与EDX保持入口值。LST数据目录证明索引0是相邻前缀word，唯一caller三个callsite只传1、1、2，索引1/2正好别名结果奖励整理的两项u16奖励ID；三word已收敛到唯一结果整理state，不建立平行数组。目标地址乘加按u32回绕，owner只覆盖索引0..2；其他索引只在唯一原store处停止，保留此前ECX、AX和地址计算且不修改owner。已关闭结果奖励整理直接消费索引1/2写入，发放后只清两项ID；全局重置保留前缀与奖励ID。唯一caller`0x00462740`尚未关闭，不提前回收其三个callsite。

`audit_order=101`的`0x0045EC80`已关闭为`platform_adapted`。完整权威LST主体`0x0045EC80..0x0045EDEF`从proc到endp共174行、108条实际指令、2个call、16个跳转、7个局部标签且无外部chunk。入口当前角色只有完整全1才扫描；组A按unsigned动态数量读取每个对象偏移`0x2B00/0x2B04`双dword，任一精确1跳过，否则以mask4查询并只对完整EAX 1累加u8 ready，callee后重读数量。组A阈值精确保留数量低byte减phase byte2和排除低byte的u8回绕，再与removed+ready完整和作signed比较；暗化门为0才回绕累加removed并发布message103。回退组B先过独立完成门，按动态完整数量无条件查询每个对象；packed低byte+ready的完整和与组B完整数量作signed比较，暗化门为0才只替换packed低byte、置组B完成门、清terminal并发布message99。成功/失败路径保留LST指定ECX高24位、CL替换和EDX ready或最终callee值。两组数量、当前角色、最终角色计数/terminal、动作phase/packed、暗化门、启动门与message全部复用既有owner，只新增十组对象双跳过字段；第11次组A真实字段读取停止。逐帧caller删除第一followup opaque调用并直接组合，显式post-actor-frame ECX/EDX snapshot进入，正常尾EDX继续传给待执行动作；子typed-stop阻断后续全部阶段。

`audit_order=102`的`0x0045EDF0`已关闭为`platform_adapted`。完整权威LST主体`0x0045EDF0..0x0045EE69`从proc到endp共71行、37条实际指令、0个call、6个跳转、5个局部标签且无外部chunk。两参数cdecl叶函数只接受完整类型1/2；其他值在EAX连续递减两次后立即返回并保持ECX/EDX。有效类型固定扫描`0x00524788..0x00524980`的18条`0x1C`记录，以`+0x00`完整全1判断首空槽；满表静默返回一过尾物理地址和ECX18。空槽先写完整值再只写`+0x08`类型word，其余字段不改。类型2返回offset EAX、值ECX和入口EDX；类型1返回offset EAX、槽索引ECX和值EDX。记录直接复用战斗启动/全局重置唯一18条owner；owner短缺只在下一条真实`+0x00`读取处停止。画面转场低概率敌方事件、角色动作case25和组B逐帧更新三个已关闭caller全部删除旧opaque调用并直连；后两类子stop保留已完成前缀并阻断后续路径。

`audit_order=103`的`0x0045EE70`已关闭为`platform_adapted`。完整权威LST主体`0x0045EE70..0x0045EFAE`从proc到endp共156行、97条实际指令、0个call、10个跳转、9个局部标签且无外部chunk。三参数cdecl叶函数先固定扫描18条`0x1C`记录首空；满表时漏写EAX索引而错误保留0的原BUG不修复。只有完整类型1走特殊位置逻辑：全1位置取首空；指定位置大于首空时EBX保持0并回落槽0。其他类型始终使用指定位置。需要右移时按`first_empty-position+1`回绕次数复制完整七dword，并特意把空槽复制到下一条；第18槽为空时只在首个一过尾目标写停止。目标记录先写值再写类型低word。类型1随后按`(value-8)*20`从启动期十组五dword暂存区依原读写序搬入记录`+14/+0C/+18/+0A/+04`，保留`+10`，再清五项源和两项尾门；正常返回EAX0、ECX源末值、EDX源物理token。记录、暂存区与尾门全部收敛到startup reset唯一owner，全局重置同步清尾门。八个caller中已关闭组A帧删除旧启动token并在frame started/active actor发布后直连；子stop阻断后续AI和最终角色尾，其余七个未关闭caller不提前修改。

`audit_order=104`的`0x0045EFB0`已关闭为`platform_adapted`。完整权威LST主体`0x0045EFB0..0x0045F01D`从proc到endp共65行、36条实际指令、0个call、5个跳转、5个局部标签且无外部chunk。单参数cdecl叶函数固定扫描18条攻击顺序记录`+0x00`的首个完整匹配；无匹配返回一过尾地址、ECX18和入口EDX。命中后从下一条开始固定七dword左移，直至最后一次必读`0x00524980..0x0052499B`：这是相邻`0x98`字节强度效果记录0的前28字节，不是现代哨兵。强度record补齐精确物理布局并与移除函数共用效果协调器唯一owner；缺失时只在原`rep movsd`首次真实源读取处停止。左移完成后固定把第17槽先写七个零dword再写七个全1dword，正常返回EAX全1、ECX0、EDX入口值。角色动作四处、对手动作两处、最终角色两处和待执行动作一处共九个已关闭caller全部删除旧opaque token并直连；待执行动作旧端口槽保留reserved数值但不调用，且继续传播移除后的完整返回寄存器。全局重置同步清全部八条强度record。

`audit_order=14`的`0x00434790`已关闭为`platform_adapted`。它只在首次调用以显式time seed CRT、发布三项共享值并扫描源图，随后直接组合已关闭粒子生成、线段推进与单像素颜色合成；剩余批次回放保留镜像检查X、源索引和实际写入X错位，粒子2×2绘制保留只跳第一透明色、只检查右像素及合成模式右上先合成后被原值覆盖。生命刷新、距离与目标矩形摘除、唯一/首/尾/中间四类双向链释放及其计数不对称均已闭环。三个上层caller都显式消费返回1作为阶段完成信号，尚待各自进入现代实现。

`audit_order=105`的`0x0045F020`已关闭为`platform_adapted`。完整权威LST主体`0x0045F020..0x0045F0E9`从proc到endp共110行、69条实际指令、1个call、11个跳转、9个局部标签且无外部chunk。单参数cdecl函数以EBX从0、ESI从18条攻击顺序首地址开始执行没有18槽上限的28字节扫描：首dword按signed小于7时直接选中，否则以`value-8`低32位按组A步长构造对象token，角色查询完整EAX精确1才继续。18条都跳过后真实读取进入八条精确`0x98`强度效果记录owner，并仍按28字节步长继续；已知相邻owner用尽时只在下一次真实dword读取停止。选中物理位置固定向调用方复制七dword；输出首项与角色metric优先索引同址，后六项也收敛到metric唯一owner。选中全1时不改顺序；非空且索引小于17时把后继完整记录左移到第17槽，再固定扫描18槽首空。未找到首空时故意保留原选中索引，导致满表从该索引起清尾的原行为不修复。尾部每条严格先写七个零dword再把首项写全1；正常清完返回EAX0、ECX0和一过尾EDX。唯一逐帧caller删除旧刷新选择token并在原五门与16帧延迟后直连；内部角色查询保留窄端口，子stop阻断主frame stage。全局重置同步清七dword输出记录。

`audit_order=106`的`0x0045F0F0`已关闭为`platform_adapted`。完整权威LST主体`0x0045F0F0..0x0045F128`从proc到endp共34行、27条实际指令、0个call、1个跳转、1个局部标签且无外部chunk。单参数thiscall先把唯一栈参数几何owner token写到绑定对象`+0x0000`，再以EBX 0、EDI 0和ESI `this+0x3108`执行30轮固定循环：每轮先写ordinal，再按原`cdq/and/add/sar`序列写`floor(index*5/4)`，EDI加5后按signed小于150继续。typed绑定对象保持精确`0x31F4`物理布局：`+0x0004`起`0x2714`字节后续资源头部和`+0x2718..+0x3103`保留区均保持入口字节，30条8字节索引记录从`+0x3104`开始。正常返回EAX/ECX均为绑定对象token、EDX 0。唯一固定参数wrapper和相邻静态thunk删除深层opaque端口，直接复用同一对象owner；wrapper返回固定绑定对象token。

`audit_order=107`的`0x0045F130`已关闭为`platform_adapted`。完整权威LST主体`0x0045F130..0x0045F1A2`从proc到endp共62行、52条实际指令、4个call、1个跳转、1个局部标签且无外部chunk。双参数thiscall以绑定对象token、ANSI文件名和输出地址执行固定只读独占`CreateFileA`；全1handle失败时仍调用`CloseHandle`并返回EAX0、恢复ECX this及关闭EDX，不读对象也不发布输出。成功时固定把`0x2714`字节读入第106项同一对象`+4`，完全忽略`ReadFile`返回和实际长度，短读仅覆盖前缀；随后发布`this+0x1F48`索引token，关闭handle并强制返回EAX1、ECX this及关闭EDX。Windows open/read/close保留三项窄平台端口。唯一启动caller删除旧高层archive-open伪边界并直连；打开失败、短读或读失败都不阻断后续定义记录读取。

`audit_order=108`的`0x0045F1B0`已关闭为`platform_adapted`。完整权威LST主体`0x0045F1B0..0x0045F29D`从proc到endp共128行、108条实际指令、7个call、5个跳转、4个局部标签且无外部chunk。四参数thiscall以同一绑定对象、文件名、`0x10C`目标、battle ID和variant低byte重新打开并读取`0x2714`头部；全1handle仍关闭后返回0。battle ID只保留低16位，`this+0x1F48+id`的count按i8 signed非正拒绝，variant也按i8与count作strict-greater拒绝；随后把索引1到id-1的byte逐项i8符号扩展并u32累计，加signed variant后从`this+8+index*4`读取偏移dword。文件位置按`0x2714 + value*0x10C`低32位计算，seek返回忽略，再固定读`0x10C`记录并关闭；读取返回和短读均忽略。count和offset table只在首次真实越界访问typed-stop且故障路径不关闭。唯一启动caller删除最后高层definition load端口，直接持有raw记录唯一owner并按原offset投影背景、数量和八名敌人字段；普通0返回仍读取入口陈旧record，typed-stop阻断后续启动。

`audit_order=109`的`0x0045F2A0`已关闭为`platform_adapted`。完整权威LST主体`0x0045F2A0..0x0045FC5F`从proc到endp共1176行、868条实际指令、54个call、129个跳转、71个局部标签且无外部chunk。函数先清菜单动作并以渲染中止精确1早退；随后按固定顺序处理DIK 2..9与启动permission byte、20条输入记录中的15个固定槽、signed重复节拍、撤退20/50ms双路径、角色工作区、selected option、鼠标开区间、热点链清理/计数、左右回绕和最终尾清零。8次DIK与热点语义直接复用已关闭typed helper，17类后续战斗callee归单一typed输入端口。组A对象token严格以`active-8`的u32回绕索引计算，两处不同workspace基址均折叠为唯一共享数组的`active+2`物理槽；输入记录与workspace只在首次真实访问typed-stop。其余共享值复用启动、最终角色、动作、metric、消息/terminal、调试、prompt、对话和热点唯一owner，新增未命名值统一归输入分派state port并接入全局重置原写集合。唯一逐帧caller删除第二前置opaque stage，第一stage完整ECX/EDX进入本函数，本函数普通返回继续传入角色预处理，typed-stop阻断所有后续帧阶段。

`audit_order=110`的`0x0045FC60`已关闭为`platform_adapted`。完整权威LST主体`0x0045FC60..0x00460BF7`从proc到endp共1917行、1165条实际指令、31个call、189个跳转、118个局部标签、27个返回点且无外部chunk。函数以战斗前帧鼠标和pre-frame gate形成入口门，直接复用热点vector首命中；31项消息switch只执行0/1/2/3/4/5/8/27/30九个case，严格恢复party hover、permission网格、signed行限制、菜单按钮、三列网格和全部strict/closed矩形边界。case3按启动模式选择组B或组A逆向目标扫描，复用启动party映射/offset、最终角色order/完成槽、metric数量和已关闭TSW像素查询；组A同active不可选时仍执行完整64点call，组B selection 6保留目标动作模式查询。party映射、offset、permission、启动模式、actor order、完成槽、marker和图像源仅在首次原始访问typed-stop。唯一逐帧caller删除最后前置opaque stage，音乐callee完整寄存器进入本函数，本函数完整ECX/EDX进入相邻输入分派；typed-stop阻断全部后续帧。全局重置只同步原写集合。

`audit_order=111`的`0x00460C40`已关闭为`platform_adapted`。完整权威LST主体`0x00460C40..0x004611F6`从proc到endp共699行、410条实际指令、18个call、43个跳转、38个局部/默认标签、10个返回点且无外部chunk；函数后的十项跳表和30-byte间接索引也已审计。入口以`message-1`分派消息1/2/3/4/5/7/8/27/30，恢复权限前缀回绕、列表/网格/scroll后退、equipment双数组写、组B行、alternate/narrow选择和消息27/30非对称sample路径。消息3按启动模式选择组B order或组A order/action cursor；组A固定配置全部十对象并逐byte清marker，selected配置刻意保留一基action kind不减1。权限、启动模式、组Border、actor order、完成槽、角色对象、marker和双数组只在首次真实访问typed-stop。输入分派三处旧调用均直连，本函数完整寄存器进入相邻确认/方向callee，typed-stop阻断调用点后续输入。交叉审计同时把pointer activity与mouse action从错误折叠中拆回两个物理owner；全局reset只同步原写集合。

`audit_order=112`的`0x00461240`已关闭为`platform_adapted`。完整权威LST主体`0x00461240..0x004618A8`从proc到endp共785行、460条实际指令、19个call、47个跳转、44个局部/默认标签、14个返回点且无外部chunk；函数后的十项跳表和30-byte间接索引也已审计。函数以`message-1`分派与后退函数相同的九个消息case，恢复权限上界循环、signed byte列表限制、grid/scroll前进、equipment双数组写后置gate、行/alternate/narrow回绕及消息27无样本夹值。消息3按启动模式执行组B正向order或组A正向order/action cursor；callee拒绝后严格从共享cursor重装ECX/EDX并按live bound继续，不能误用callee返回寄存器。四类角色call完整恢复原乘法中间寄存器和物理token；组Bcount 9在第九次配置thiscall停止。组A完成槽/候选使用一基code地址，code 0在完成槽停止；大组原点使用另一原始公式并让code 10在一过尾thiscall停止。组A随后固定配置十对象、逐byte清marker，一基selected code 10则精确映射第十物理对象。权限、启动模式、order、完成槽、对象、marker和双数组仅在首次真实访问typed-stop。输入分派三处旧调用已直连，普通完整寄存器进入相邻确认/方向callee，停止阻断调用点后续输入。

`audit_order=113`的`0x00461900`已关闭为`platform_adapted`。完整权威LST主体`0x00461900..0x00461A28`从proc到endp共148行、81条实际指令、3个call、12个跳转、8个局部标签、6个返回点且无外部chunk。函数只处理消息2、4、27，各播放一次样本；其他消息清pre-frame gate后返回`message-27`。消息2处理网格与panel B，消息27处理列表与panel A；两者先把selection归一到1，否则把对应scroll减7并作signed零夹，刻意让ECX保留负减法结果。消息4在同型grid分页后从共享scroll重装EDX，先置mouse action gate，再按current equipment索引写两份唯一缓存；typed-stop保留样本、夹值、gate及写前缀。输入分派interaction mode 3与record7两处旧调用均已直连，停止阻断后续鼠标发布、mode 4和最终输入提交。

`audit_order=114`的`0x00461A30`已关闭为`platform_adapted`。完整权威LST主体`0x00461A30..0x00461C00`从proc到endp共210行、112条实际指令、3个call、17个跳转、12个局部标签、5个返回点且无外部chunk。函数入口清pre-frame gate，只处理消息2、4、27并各以sample mix装EAX后播放一次样本。消息2按i8行限制先执行小于7早退，再把列表selection归一到7或让panel A前进7；CL局部写、高24位、signed byte、边界EDX和绕回负EAX全部保留。消息4和27处理网格与panel B；两者在menu action为0且grid不为7时共享`min(7,u16 limit)`归一和双equipment缓存副作用。普通消息4前进后按current equipment发布grid/page缓存；消息27只更新page，短网格时夹0并保留负ECX和夹取前EDX。双数组仅在真实store typed-stop。输入分派interaction mode 4与record8两处旧调用已直连；第113项分页后退的sample预调用EAX同期修正为原sample mix。

`audit_order=115`的`0x00461C10`已关闭为`platform_adapted`。完整权威LST主体`0x00461C10..0x00462085`从proc到endp共504行、258条实际指令、7个静态call、20个跳转、25个局部/默认标签、8个返回点且无外部chunk；函数后的十项跳表和三十byte间接索引也已审计。入口清pre-frame gate并置mouse gate；selected group-B特殊清理按one-based code重置对象及固定缓存。message 1/2/4/5/7/8/27/30分别关闭、重建或切换菜单状态并按两套原始寄存器公式重置active group-A对象，默认只清动画帧。message 3先重置active group-A，再按live signed count重置group-B并固定重置十个group-A、逐byte清marker；count 9在第九次真实对象call停止。随后按action kind映射message或回退fallback kind，匹配路径清十项selection与五dword攻击缓存。所有物理状态复用input/frame/startup/final/metric唯一owner；全局reset补齐新增owner并把action kind恢复权威值1。逐帧输入record0旧调用已直连，typed-stop阻断caller两项尾清理。

`audit_order=116`的`0x004620D0`已关闭为`platform_adapted`。完整权威LST主体`0x004620D0..0x0046231F`从proc到endp共269行、157条实际指令、7个静态call、23个跳转、12个局部标签、7个返回点且无外部chunk。函数依次执行entry word、input、outcome与signed message门；suppression为0时按group-B count减packed低byte决定是否清message；message 110保留u16小于30写29、等于30写100的非对称。dialog非空发布one-shot交互；ready未置、queued角色为0或非零message缺少option时刷新选择状态。真正进入时先清option cache，查询queued group-A角色；未完成则播放样本、重载queued角色、发布mouse/target/phase并配置选择。group-B index缺失或group-A index不匹配时发布message/action 1刷新；匹配时发布message 7、limit 2，并对同一signed group-B对象执行三次primary和两次secondary扫描，callee返回1才增加limit，最后清transition output。active/group-B越界只在首次真实对象call typed-stop并保留声音、gate、message与扫描前缀。逐帧输入十个静态caller已从五条typed业务路径直连；AI/action三处caller尚未关闭。交叉审计同时统一queued/published final-actor owner，拆开group-B index、group-A index与逐帧option cache，target-ready复用actor-frame共享owner，one-shot复用世界player-control owner。

`audit_order=117`的`0x00462320`已关闭为`platform_adapted`。完整权威LST主体`0x00462320..0x00462381`从proc到endp共67行、30条实际指令、8个静态call、2个跳转、5个局部/默认标签、4个返回点且无外部chunk，函数后的四项跳表也已审计。入口从共享queued角色装载EAX并先清pre-frame gate B，再按u32计算`code-8`；unsigned结果大于3时返回转换后EAX并保留入口ECX/EDX。code 8/9/10/11分别把动作起点11/8/9/10交给可用动作轮转callee，再把其EAX作为参数交给动作提交callee，完整透传两级callee寄存器。逐帧输入记录2、记录4菜单后退和记录5菜单前进三个caller已直连；旧动作确认槽保留reserved数值且不再调用。记录2短按把option写0，长按除3余1路径保留原EBP除数3并在两次callee期间写3；记录4前置菜单typed-stop继续阻断动作轮转与菜单动作尾写。当前角色和清零gate均复用final-actor既有物理owner。

下一项回收`audit_order=118`的`0x004623A0`，从完整权威LST主体和所有外部FUNCTION CHUNK独立审计相邻战斗函数。

模块10只有在`422/422`均有实现映射、不可达证据或合规阻塞，完整战斗生命周期和I5通过后才能移交模块11。
