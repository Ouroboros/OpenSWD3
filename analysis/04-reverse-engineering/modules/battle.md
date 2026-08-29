# 模块10：战斗状态机、AI与数值系统

状态：`module_in_progress`

当前关闭进度：`159/422`。现有资产读取与建场代码只是此前恢复的有限切片，不提前计入完整函数关闭。

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
- `pending_audit`：`264`；
- `assembly_exact`：`7`；
- `platform_adapted`：`151`；
- 已关闭：`158`。

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

`audit_order=71`的`0x0045AA00`已关闭为`platform_adapted`。完整权威LST主体`0x0045AA00..0x0045ADEC`从proc到endp共465行、20个静态call站点、26个局部标签且无外部chunk。实现恢复完整group选择、组A第二byte完成计数、阈值控制的32字节逆向清零、三刷新、双组对象重置、角色顺序固定左移、removed unsigned终止、继续配置和五dword记录清零，以及组B全1早退、坐标低word累加、描述符bit5延迟、动作发布、闭区间计数、signed完成比较和末位重置。交叉审计锁定该函数对`0x0053BCEC`的1/0x63/0x67均为共享message发布，不建立第二份状态。组A/组B帧两个caller均删除opaque token并直接组合typed实现，子typed-stop阻止父清理。

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

`audit_order=117`的`0x00462320`已关闭为`platform_adapted`。完整权威LST主体`0x00462320..0x00462381`从proc到endp共67行、30条实际指令、8个静态call、2个跳转、5个局部/默认标签、4个返回点且无外部chunk，函数后的四项跳表也已审计。入口从共享queued角色装载EAX并先清pre-frame gate B，再按u32计算`code-8`；unsigned结果大于3时返回转换后EAX并保留入口ECX/EDX。code 8/9/10/11分别把动作起点11/8/9/10交给可用动作轮转callee，再把其EAX直接交给typed队列提交，完整透传两级寄存器并传播队列/角色typed-stop。逐帧输入记录2、记录4菜单后退和记录5菜单前进三个caller已直连；旧动作确认槽保留reserved数值且不再调用。记录2短按把option写0，长按除3余1路径保留原EBP除数3并在两次callee期间写3；记录4前置菜单typed-stop继续阻断动作轮转与菜单动作尾写。当前角色和清零gate均复用final-actor既有物理owner。

`audit_order=118`的`0x004623A0`已关闭为`platform_adapted`。完整权威LST主体`0x004623A0..0x00462401`从proc到endp共67行、30条实际指令、8个静态call、2个跳转、5个局部/默认标签、4个返回点且无外部chunk，函数后的四项跳表也已审计。入口从共享queued角色装载EAX并先清pre-frame gate B，再按u32计算`code-8`；unsigned结果大于3时返回转换后EAX并保留入口ECX/EDX。code 8/9/10/11分别把反向动作起点9/10/11/8交给可用动作轮转callee，再把其EAX直接交给与正向轮转共用的typed队列提交，完整透传两级寄存器并传播队列/角色typed-stop。逐帧输入记录6菜单前进和记录3选择后退两个caller已直连；旧secondary confirmation槽保留reserved数值且不再调用。记录6前置菜单typed-stop阻断反向轮转与菜单动作2尾写；记录3按原顺序执行热点回绕、可选菜单后退、反向轮转和左向动作边界。当前角色与清零gate继续复用final-actor唯一owner。

`audit_order=119`的`0x00462420`已关闭为`platform_adapted`。完整权威LST主体`0x00462420..0x004624BF`从proc到endp共92行、53条实际指令、1个静态call、8个跳转、6个局部/返回标签、1个返回点且无外部chunk。message非零且option全1时立即返回；否则以live group-A count减1，结果恰好0时直接清三个selection dword与一个word。其余路径扫描final-actor十槽队列，每轮动态重读count并以unsigned `index >= count-1`结束；count 12或0不加现代上限，在第十一次真实读取typed-stop。匹配非零actor code时按`code-8`保留乘移寄存器，调用既有group-A角色查询；返回1继续扫描，其他值交换queued角色与当前队列槽。队列、角色停点都在四项缓存清理前，保留全部前缀。逐帧输入selected option一处caller，以及正向/反向轮转八处caller均已直连；三类旧槽保留reserved数值且零调用。record15普通返回后才进入目标选择并恢复option；typed-stop阻断这些尾操作。两种轮转的提交typed-stop继续向逐帧输入传播并阻断各自菜单或方向尾路径。

`audit_order=120`的`0x004624C0`已关闭为`platform_adapted`。完整权威LST主体`0x004624C0..0x0046250A`从proc到endp共47行、30条实际指令、1个静态call、3个跳转、3个局部/返回标签、2个返回点且无外部chunk。入口固定读取group-A count并清EAX；count为0时返回0且保持入口ECX。其他路径从final-actor十槽队列起点逐dword完整比较候选code，不跳过零，也不动态重读count；未命中返回0与一过尾队列token。count大于10不加现代上限，在第十一次真实读取typed-stop。首个匹配code按`code-8`保留乘移寄存器，角色调用前EAX为索引乘`0xBCD`，ECX为group-A对象token，EDX为入口count；物理索引越界在首次真实角色call前停止。角色查询完整EAX为0时归一返回1，任何非零bit pattern返回0，并保留callee ECX/EDX。三个caller属于尚待各自工作包关闭的正向/反向候选轮转，当前既有轮转opaque边界不提前拆分；关闭caller时必须直连本typed实现。

`audit_order=121`的`0x00462510`已关闭为`platform_adapted`。完整权威LST主体`0x00462510..0x0046262D`从proc到endp共128行、70条实际指令、4个静态call、9个跳转、9个局部/返回标签、1个返回点且无外部chunk。入口读取message并无条件清pre-frame gate B；message 1/2/4/30四个条件分别重读共享message，样本回调可令同次调用连续执行多分支。message 1按signed界限把action kind前进4，再读取九byte物理权限域，权限0时回退4；越界只在真实byte读取停止。message 2把共享action category按u32加1并按signed `>=3`把存储回绕0，但保留加1后的EAX，同时重置list selection与panel scroll A。message 4按signed比较轮转四类装备，依次恢复equipment grid和scroll缓存；负signed索引在首次真实四槽读取停止。message 30把grid selection加5并按signed上界把存储夹到10，但样本调用保留夹值前EAX。四条路径均播放样本46并保留各自预调用寄存器。逐帧记录5旧右向槽保留reserved数值并零调用；对话为空且message为3时先完成菜单选择前进，再直连本实现，普通返回后才轮转角色动作，typed-stop阻断轮转。

`audit_order=122`的`0x00462630`已关闭为`platform_adapted`。完整权威LST主体`0x00462630..0x00462738`从proc到endp共122行、66条实际指令、4个静态call、9个跳转、9个局部/返回标签、1个返回点且无外部chunk。入口读取message并无条件清pre-frame gate B；message 1完成或跳过后，强制EAX为2、ECX为live message再判断message 2，后续message 4/30继续独立重读，样本回调仍可触发同次多分支。message 1按signed `action-4 >=1`回退action kind，再读取九byte权限域，权限0时加4恢复；越界在真实byte读取停止。message 2把action category按u32减1并先回写，signed负值只把存储回绕2，随后把list selection写1但不清panel scroll A。message 4把equipment selection减1并先回写，signed负值把EAX与存储回绕3；非负大索引在首次四槽读取停止，有效索引恢复grid与scroll缓存。message 30把grid selection减5并先回写，signed结果小于1时只把存储夹到1，样本保留减法EAX。逐帧记录3旧左向槽保留reserved数值且零调用；热点回绕、可选菜单选择后退和反向角色轮转完成后直连本实现，本实现typed-stop保留已完成轮转并阻断后续记录。

`audit_order=123`的`0x00462740`已关闭为`platform_adapted`。完整权威LST主体`0x00462740..0x004640F1`从proc到endp共2820行、1662条实际指令、95个静态call、140个跳转、110个局部/默认标签、37个返回点且无外部chunk；函数后的200项主message压缩表和36项动作压缩表也已审计。入口仅在target-ready完整dword为1时运行，随后清selection runtime gate，以u32 `message-1`分派1/2/3/4/5/7/8/27/30/98/100–104/110–113/200；表内默认返回selector 20，表外保持caller ECX。message 1在输入gate和animation frame B signed阈值6后读取跨startup物理owner的action word重映射，再按36项动作表进入目标面板、角色提交或message回退。message 2/4/8/27/30完成候选gate、目标校验、三项角色属性、提示样本、默认/alternate回退及四类动作/装备路径；默认组B与alternate组A目标轮转现均直接组合已关闭typed实现。message 3提交动作、按live group-B count和固定四个group-A对象清选择、处理动作30/4和cleanup角色；message 5以行选择、高word偏移和group-A count在共享动作workspace物理word/dword视图写效果记录；message 7现直连已关闭组B目标轮转，恢复对象重置、一基发布、共享message与输入记录预置尾部；message 1公共尾也按live共享message 3组合同一实现。98–113保留`>=20`、`>20`、`>=30`三种signed阈值、AL-only actor byte替换和三条故意不同的状态清理；200保留两组live count清理。新runtime owner只承接未建模状态；`0x0053BCEC`继续复用既有唯一共享message，不建立runtime副本；角色、动作workspace、五dword记录、菜单、计数、镜像与startup尾块继续复用唯一owner；global reset只同步原234项写程序真实覆盖的字节。目标选择进入原刷新槽保留reserved数值且零调用，ready/queued短路直连本实现并传播typed-stop。

`audit_order=124`的`0x00464270`已关闭为`platform_adapted`。完整权威LST主体`0x00464270..0x00464C6F`从proc到endp共1154行、747条实际指令、45个静态call、73个跳转、49个局部/默认标签、12个返回点且无外部chunk；函数后的30项message压缩表也已审计。入口在message 103直接返回，queued为0且target-ready非1时返回0；其他路径先查询queued组A角色，完成时清五项选择指针、message/cache/runtime，按live group-B count重置对象，再扫描`group-A count-1`范围十槽actor order并以首个空闲/未完成槽交换queued。随后无条件查询queued释放状态并置display gate。message 1完成pointer origin、鼠标边界、signed frame/phase夹值、比例填充、陈旧ESI标签居中和动作摘要；message 2直连列表框与最多七行列表内容，并保留i8 row-limit、资源/矩形/共享文字子stop、纵向面板条件和cache A/B/C发布；message 4直连网格列表帧并保留隐藏行不计七行上限、当前行双矩形/共享说明和输入发布顺序；message 27直连替代网格列表帧并保留双九宫格、固定标题、名称先于数值、第八次查询、单矩形与iterator发布；message 30直连模式网格帧并保留未读滚动参数、两列五行、首/次/缺省三段来源、signed组容量和page/group目标修正；message 8直连窄网格帧并保留稀疏查询、零结果不计七行、终止哨兵刷新、显示行重绘和source iterator发布；四条路径正常返回才发布各自auxiliary/cache；message 3从共享动作workspace选择组A/组B，按live count绘制全部角色标记或轮转九项target map，并按one-based组A或group-B当前目标构造prepared动作帧；live输入门为1时直连当前目标提示帧，保留动态名称框、生命阈值10、渐变阈值15和两套镜像判断；message 5直连护驾面板帧并保留固定面板、先选框、组A尾段列名和单项“無”；message 7直连控制面板帧并保留双边框、三项主查询、两项特殊查询、压缩索引和双transition发布，message 6只在双actor gate为0时对物理控制word高byte OR 0x40并关闭选择。比例面板、纵向面板、prepared动作帧、角色目标准备、动作摘要、列表框、列表内容、网格列表帧、替代网格列表帧、模式网格帧、窄网格帧、护驾面板帧、当前目标提示帧和控制面板帧十四类已关闭callee已直连；动作摘要复用启动颜色/profile、九byte权限、三项动作workspace和静态文字表，列表框复用主帧面板记录及偏移动作帧state，列表内容复用启动颜色、说明record/index、输入门和candidate owner，网格列表帧另复用target argument、主帧面板记录和同一偏移动作帧state并仅接收外部MAPS/共享文字span，替代网格列表帧复用主色、target、输入门、row-limit和同一面板记录且没有共享文字owner，模式网格帧继续复用这些owner且仅新增单一文字缓冲和三项栈计数，窄网格帧复用共享byte、candidate及输入门并把输入分派五dword选择工作区作为唯一20-byte文字owner且只新增两个栈计数，护驾面板帧复用组A数量、组B选择和target-effect且不新增物理state，当前目标提示帧复用五dword分组、queued/published、组B数量、选择阻断、mirror和面板owner并只持有共享渐变颜色槽，控制面板帧复用alternate选择、组B索引、transition与同一颜色槽，并把紧邻选择工作区的六dword设为唯一24-byte文字owner，typed-stop按原首次对象、order、workspace、profile、target map、动作更新、共享文字或绘制访问点传播。`0x98`步长动作记录第8项从lower-panel四dword起始，现仅保存前8项独立记录与后两项非重叠尾区，第8项前16字节直接和lower-panel owner互相装载写回。新选择帧owner只承接十项标签、两项pointer origin、display/secondary和已关闭绘制的栈局部状态；原suppression与final-actor pre-frame gate B同址，现直接复用后者唯一owner；五项选择指针直接复用输入分派selection workspace，物理控制word直接复用输入分派retreat control word，两项actor origin也复用输入分派内由target-selection entry配置callee写回的唯一owner，message、queued/published角色、actor order、workspace、数量、其余输入cache、菜单、target map、debug与target runtime也继续复用既有唯一owner。global reset通过输入owner清物理控制word，只清选择帧owner内原234项写程序覆盖的display/secondary两项状态，并通过final-actor owner清同址pre-frame gate B。主帧原frame-stage槽、角色目标准备槽、动作摘要槽、列表框槽、列表内容槽、网格列表帧槽、替代网格列表帧槽、模式网格帧槽、窄网格帧槽、护驾面板帧槽、当前目标提示帧槽和控制面板帧槽均保留reserved数值且零调用，各自直连typed实现；子typed-stop阻断画面效果及全部后续帧。

`audit_order=125`的`0x00464CC0`已关闭为`platform_adapted`。完整权威LST主体`0x00464CC0..0x00464D96`从proc到endp共95行、73条实际指令、4个静态call、5个跳转、3个局部标签、1个返回点且无外部chunk。入口把group-A actor code写入唯一已提交角色owner，写动作1、提交门1及共享动作workspace的`10 + (code-8)`项，再按EAX=`index*0xBCD`、ECX=组Atoken、EDX caller值调用准备callee。workspace地址按u32回绕；code 7先写物理第9项，再在组A基址前一对象call停止。准备后以signed域比较processed低byte与live group-B count；只有正count较大时直连secondary RNG取随机起点并加1发布one-based组B code。每个对象查询前保留EAX=`code*0x565`、EDX=`code*0x345`、ECX=one-before token；完成返回1时循环递增code、按live count回绕1、递增ESI，并在signed `ESI >= count`时停止，否则查询下一对象。无现代循环上限或八对象count夹值，第九个对象在首次真实call停止。`0x0053BD50`删除target runtime副本，debug状态内语义化已提交角色成为目标刷新、撤退、调试与global reset共享唯一owner；`0x0053AF30`前十项也删除debug副本，调试全清和global reset只清动作owner；processed低byte复用动作owner。选择帧完成角色路径与组A逐角色帧queue-mode路径都已直连本实现，原选择帧槽保留reserved数值且零调用，组A旧callee token零调用；主帧进入角色序列前注入唯一action、final actor和target runtime owner。

`audit_order=126`的`0x00464DA0`已关闭为`platform_adapted`。完整权威LST主体`0x00464DA0..0x00464DC0`从proc到endp共16行、7条实际指令、0个call、0个跳转、0个局部标签、1个返回点且无外部chunk。函数先置ECX=1、EAX=2，再按序写输入记录1的rapid-press multiplicity=1、记录1的held sample count=2、记录15的held sample count=2、记录12的held sample count=1，EDX保持caller值。四项地址都映射输入归一化records唯一owner；records不含记录1时零写停止，只含记录0..1时完成前两写后在记录15首次真实访问停止，不预先要求20项也不回滚。目标选择刷新权威LST的14个调用点通过共享分支统一直连本实现，原`refresh_target_display`槽改为reserved且生产代码零调用；主帧records经输入分派、目标选择入口传到目标刷新。子typed-stop保留此前动作、缓存、角色或目标发布并阻断后续调用。函数无分支、callee和不确定外部状态，完整LST与固定状态测试覆盖全部行为，不依赖动态oracle。

`audit_order=127`的`0x00464DD0`已关闭为`platform_adapted`。完整权威LST主体`0x00464DD0..0x00464E33`从proc到endp共68行、43条实际指令、2个静态call、6个跳转、6个局部/返回标签、2个返回点且无外部chunk。主候选表按物理顺序为`10/9/8/11`；合法starting code从命中项开始，失败后只在post-read索引恰等于4时回绕0，最多查询四次并返回首个可用code或0。无效starting code扫描结束后不回绕，故意跨入相邻四dword并固定查询`2/1/0/3`；不增加现代边界或提前过滤小于8的code。两个候选可用性call都直连已关闭实现，复用唯一actor order和live group-A对象；winning code作为EAX继续直连动作提交，ECX/EDX保留callee结果。正向动作轮转四项跳表路径通过共享分支统一直连本实现，原候选轮转槽保留reserved数值且生产代码零调用；子typed-stop保留候选表/actor-order扫描前缀并阻断动作提交。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`127/422 = 122 platform_adapted + 5 assembly_exact + 295 pending_audit`，连续双跑逐字节一致，SHA256为`acf996591f919299b57cd117bf4c83807c3c7d17ac3915a39473d02e01c53313`。

`audit_order=128`的`0x00464E40`已关闭为`platform_adapted`。完整权威LST主体`0x00464E40..0x00464E8D`从proc到endp共56行、35条实际指令、1个静态call、5个跳转、4个局部/返回标签、2个返回点且无外部chunk。函数仍正向扫描共享主候选表`10/9/8/11`寻找starting code，但每次读取候选后先递减索引，signed负值立即回绕3，因此合法起点沿主表反向循环并最多查询四次。无效starting code从主表尾后一dword先读取相邻值2，再回到主表索引3，固定轨迹为`2/11/8/9`；不现代化过滤或改写。唯一候选可用性call直连已关闭实现，复用正向函数登记的单一连续八dword typed常量、唯一actor order和live group-A对象。反向动作轮转四项跳表路径统一直连本实现，原反向候选槽保留reserved数值且生产代码零调用；子typed-stop保留物理候选、索引递减、actor-order扫描和地址计算并阻断动作提交。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`128/422 = 123 platform_adapted + 5 assembly_exact + 294 pending_audit`，连续双跑逐字节一致，SHA256为`14933e3422080906cbb8a67432b800e9ada693d5be9affc512e64beb3ff4827b`。

`audit_order=129`的`0x00464E90`已关闭为`platform_adapted`。完整权威LST主体`0x00464E90..0x0046508E`从proc到endp共215行、128条实际指令、3个静态call、14个跳转、10个局部/返回标签、1个返回点且无外部chunk。函数先把九byte权限域的后八byte写为前五项1、后三项0，清三项动作文字token和动作count，但保留旧动作code尾部；queued角色按u32减8映射到共享四项source，首角色特殊模式可先登记固定动作6。每个source只读取两项对象token及其`+0x54`动作code，仅unsigned `[0x15,0x32)`进入最多三项workspace。21项静态文字token后的8项越界读取严格跨入live action kind、published actor、target cursor、独立相邻值、三项列表选择和selection actor code owner，不现代化为安全表。随后以`0x3EF/0xBCD/0x3EF`三种寄存器形状查询同一group-A角色并裁剪权限2/4；第三次返回1时重建只含权限2/5的稀疏表，按live action kind读取，禁用项回退kind 2。映射、source、对象、角色和最终权限均只在首次真实访问typed-stop并保留前缀。逐帧输入七个callsite、目标选择入口一个callsite与动作摘要一个callsite均已直连；刷新后实时读取permission，reserved旧槽生产代码零调用。`0x004A75C8`起数组收敛为启动状态单一十dword物理owner，启动/input使用前四项，选择帧复用完整相邻视图。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`129/422 = 124 platform_adapted + 5 assembly_exact + 293 pending_audit`，连续双跑逐字节一致，SHA256为`f0bc8129c6aff3c5b88836eb909085558c2a9f2eeffde7bd998016a7f9f0c071`。原版四组option对象、三个角色查询callee共享副作用与EAX/ECX/EDX联合动态捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。

`audit_order=130`的`0x00465090`已关闭为`platform_adapted`。完整权威LST主体`0x00465090..0x00465163`从proc到endp共98行、59条实际指令、3个静态call、6个跳转、6个局部/返回标签、1个返回点且无外部chunk。函数按signed当前目标与live group-B count先归零越界索引，再以EAX=`index*0x159`、ECX=对象token、EDX caller值查询当前对象；完成时每轮重读live cursor和count，cursor一基递增/回绕后读取九dword顺序表，发布零基候选并以EAX=`index*0x565`、EDX=`index*0x159`查询。扫描计数达到本轮signed count时先把共享message写1，不查询最后候选但仍继续选择重置；最终重置使用参数1与EDX=`index*0x565`，普通返回后重读live当前目标、打开输入门并发布一基角色。对象、顺序表和最终重置均在首次真实访问typed-stop并保留前缀。五个静态caller均位于已关闭目标选择刷新，现统一组合本typed实现；message 7删除旧部分内联并恢复重置、发布、共享message 3、动画缓存与输入记录尾部。旧默认目标槽改为reserved且生产代码零调用。完整LST交叉扫描确认`0x0053BCEC`继续是跨启动、最终角色、输入、菜单与目标选择的唯一共享message owner，不建立同址runtime副本；`0x0053BFBC`收敛到final-actor pre-frame gate B唯一owner，global reset各清一次。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`130/422 = 125 platform_adapted + 5 assembly_exact + 292 pending_audit`，双跑SHA256为`45827505c66405dd7ed878f49107828225c07f42e53ed6e108865d5eeef3825f`。原版group-B对象、完成查询/选择重置共享副作用、五caller联合轨迹及EAX/ECX/EDX捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。

`audit_order=131`的`0x00465170`已关闭为`platform_adapted`。完整权威LST主体`0x00465170..0x004651CF`从proc到endp共45行、25条实际指令、0个call、2个跳转、2个局部标签、1个返回点且无外部chunk。函数以u32 `group-A count - 目标效果高word - 补充人数word`形成bound，queued角色减8形成零基目标；局部cursor每轮先加1，再按signed大于bound回绕1，循环期间不回写owner。目标表从第127项连续八dword候选表的第4项地址起读，正常索引1..4为`2/1/0/3`，u32负索引可回读前置`10/9/8/11`；已知八dword之外在首次真实读取typed-stop，不增加循环上限。匹配后依次发布cursor、候选加1、当前组B目标0和输入门1，返回EAX cursor、ECX一基目标、EDX bound。目标刷新两个静态caller通过共享属性回退路径直连，旧alternate槽改为reserved且生产代码零调用；typed-stop保留属性查询与角色记录写前缀并阻断message、输入记录及动画尾部。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。候选工作包为`131/422 = 126 platform_adapted + 5 assembly_exact + 291 pending_audit`，双跑SHA256为`792c78aa167cbfa3ad748bff933252f9ccba986015056ee9f4fa945f07a9b812`。该函数为确定性无callee叶函数，完整LST、共享常量表和固定状态测试覆盖全部可终止路径，不依赖动态oracle。

`audit_order=132`的`0x004651D0`已关闭为`platform_adapted`。完整权威LST主体`0x004651D0..0x00465474`从proc到endp共314行、210条实际指令、14个静态call、16个跳转、13个局部标签、1个返回点且无外部chunk。queued为0时以EAX 0直接返回并保留入口ECX/EDX；非零时先配置字体，按`queued-8`读取组A profile，kind为`0x38`且特殊门返回0时置权限4，再直连已关闭动作模式刷新。四固定行读取权限1..4和共享静态文字表第16..19项，以24像素步长、主/次颜色及style 4绘制，action 1..4在`x-1,y-1`以主色style `0x10`覆盖。三动态行复用启动reset中的token/code与权限6..8；只有可用性EAX精确为1且权限精确为1时用主色，否则保留查询ECX高16位、低word换次色并先清权限，action 6..8即使禁用仍执行主色选中覆盖。固定/动态普通及覆盖call的EAX/ECX/EDX形状逐分支保留；动态段重新装载原始Y与`6-0x00524419`陈旧EAX，三token全零时穿过循环进入最终字体恢复。启动阶段两项颜色更名为主/次文字颜色并保持唯一owner，组A reset/profile显式维护单一profile token/kind owner；九byte权限、三项动态workspace和21项静态文字表均复用既有owner。选择帧旧动作摘要槽改为reserved且零调用，message 1标签绘制后直连并传播子typed-stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`132/422 = 127 platform_adapted + 5 assembly_exact + 290 pending_audit`，连续双跑逐字节一致，SHA256为`db16f83587a0619840685c1fb13d17f653fd85e35900cc0141a92a909ca59ba6`。当前缺少原版组A profile、五类callee共享副作用、字体、文字surface与寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=133`的`0x00465480`已关闭为`platform_adapted`。完整权威LST主体`0x00465480..0x004655A3`从proc到endp共131行、91条实际指令、7个静态call、2个跳转、1个循环标签、1个返回标签、1个返回点且无外部chunk。函数以Y=`origin+32`、X从`origin+84`起每轮减42，依次直连偏移动作帧variant 2/1/0；随后按live action category绘制variant `category+4`、X=`origin+category*42`的当前分类框，不限制u32回绕。四框后字体样式依次为`0xF000/0xFFFE`，再完整清共享面板动作记录、置frame A为10、写动作`0x233B`/variant 0并更新；更新失败没有原分支，仍继续。矩形固定为`origin_x, origin_y+36, 190x152, RGB 0/4/4, mode 2`；矩形返回ECX只替换低word为面板资源，保留高16位后绘制`left=origin_x+6, top=origin_y+40, right=origin_x+186, bottom=origin_y+184, opacity 0, flags 0x80000008`九宫格。正常尾把live frame B按u32加2，signed大于7才把owner夹到7；EAX仍返回未夹值，ECX/EDX保留九宫格返回。面板记录复用主帧唯一owner，四框复用偏移动作帧唯一state；选择帧message 2直连，旧槽reserved且零调用，子stop阻断列表内容、auxiliary与cache A/B。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。工作包稳定为`133/422 = 128 platform_adapted + 5 assembly_exact + 289 pending_audit`，连续双跑逐字节一致，SHA256为`55d036dcfb077436289ad0e6d783a10b23c64afb1329af3ccdc8d4c97affb8f3`。当前缺少原版四次动作更新、字体、面板动作更新、矩形/九宫格framebuffer与寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=134`的`0x004655B0`已关闭为`platform_adapted`。完整权威LST主体`0x004655B0..0x004659BB`从proc到endp共465行、338条实际指令、21个静态call、16个跳转、16个局部/返回标签、1个返回点且无外部chunk。queued为0时返回EAX/ECX 0并保留入口EDX；非零时依次配置字体模式0、样式`0xFFFE`与宽度16，先清共享row-limit，再按组A对象执行列表初始化和刷新。iterator从滚动偏移加1开始；每轮查询value/可选资源，低word `0xFFFF`时刷新并提前结束，bit15选择负/普通解析且负路径按`&0x7FFF`清域。limit/value按i16 signed决定主/次色，资源返回低word非`0xFFFF`时直连资源`0x241C`、frame=`return-1`，名称与signed `%3d`数值按20像素行距绘制。selected row精确等于`row+1`时再次绘制名称，直连两处固定矩形，再以组A说明record的text index直连已关闭共享文字解析，按首个NUL长度居中绘制底部说明，最后才发布输入门和candidate iterator。循环只以row counter低wordunsigned `<7`继续，不进行第八次查询；正常尾再设`0xFFFE`并保留返回寄存器。当前分类、row-limit、颜色、门和candidate复用既有owner；启动状态增加说明record token/index唯一视图，MAPS/128字节buffer只接受外部owner span。选择帧旧列表内容槽reserved且零调用，message 2直连并传播资源、两矩形及共享文字stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。候选工作包为`134/422 = 129 platform_adapted + 5 assembly_exact + 288 pending_audit`，连续双跑SHA256为`6a94133212b611c8407ef7fe7938df0ee595c2a68b785d72f51bd9a694954106`。当前缺少原版组A对象、五类角色列表callee联合状态、字体/文字surface及寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=135`的`0x004659C0`已关闭为`platform_adapted`。完整权威LST主体`0x004659C0..0x00465E45`从proc到endp共503行、337条带机器码和真实助记符的实际指令、27个静态call、11个跳转、11个局部/返回标签、1个返回点且无外部chunk。queued为0时返回三寄存器全0；非零时以动作`0x2394`按variant `3/2/1/0`绘制四框，再按live category绘制variant `category+4`第五框。字体`0xF000/0xFFFE`后完整清共享面板记录，写动作`0x233B`/variant 0且更新失败继续；面板矩形为`origin, origin_y+36, 204x152`，矩形返回ECX保留高word并替换资源低word后直连`origin+6,+40`到`origin+200,+184`九宫格。随后配置字体模式0/样式`0xFFFE`/宽度16，清u16 row-limit并初始化组A列表。iterator从scroll加1开始，查询0时刷新并结束；只有flags bit15行计入display count，隐藏行只推进iterator且不增加modern cap，七行上限只比较display count低word。可见value按signed i16 `%2d`绘制于X+144，名称绘制于X+16；bit14选次色，否则主色，两条路径均保留callee颜色高word。selected row按display count+1精确匹配，在X+15/Y-1重绘名称并直连两矩形；第一矩形成功后先发布target iterator和输入门，再调用全宽矩形，随后以组A说明index直连共享文字并居中绘制底部说明。选择帧旧grid槽reserved且零调用，message 4直连并传播五框、面板、两矩形和共享文字stop。grid栈局部单独持有两组20-byte缓冲与flags/value；动作帧、面板记录、颜色、说明、target、MAPS和128-byte共享文字均复用既有owner。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。候选工作包为`135/422 = 130 platform_adapted + 5 assembly_exact + 287 pending_audit`，连续双跑SHA256为`e1f3e882fd7d52e46e393870c5cbb7aac205b769f7d01e25d67e3e4b1abc17f2`。当前缺少原版组A对象、三类列表callee联合状态、字体/文字surface、动作/矩形/九宫格framebuffer及寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=136`的`0x00465E50`已关闭为`platform_adapted`。完整权威LST主体`0x00465E50..0x00466181`从proc到endp共357行、238条带机器码和真实助记符的实际指令、20个静态call、5个跳转、5个局部/返回标签、1个返回点且无外部chunk。queued为0时返回三寄存器全0；非零时完整清共享面板动作记录，写动作`0x233B`/variant 0且更新失败继续。固定矩形为`origin,190x188,RGB 0/-8/-8,mode 1`；其返回EDX替换资源低word后绘制`origin+6,+8`到`origin+186,+24`第一九宫格，再以第一九宫格返回EDX替换同一低word后绘制`origin+6,+40`到`origin+186,+184`第二九宫格，两次各自保留陈旧高16位。随后从静态文字表直接复用固定标题，在`origin+80,+8`绘制`0xFFC0`文字，并配置字体模式0/样式`0xFFFE`/宽度16。函数清u16 row-limit，以固定selector 4初始化组A列表并刷新。iterator从scroll加1开始，查询没有flags筛选；查询0时再次刷新并结束。七行上限在查询成功后才按display count低wordunsigned `>=7`判断，因此第八次查询确实发生并保留callee/缓冲副作用，但不绘制。每行先用查询返回ECX高word+主色低word绘制名称，再按signed i16 `%2d`用格式化ECX高word+主色低word绘制数值。selected row精确等于row+1时重绘名称、配置样式并直连一处`194x24`矩形，矩形成功后才发布target iterator和输入门；本函数没有第二全宽矩形或共享说明。面板记录、row-limit、主色、门和target均复用既有owner，alternate state只保留原栈两组20-byte缓冲、display count与value。选择帧旧alternate槽reserved且零调用，message 27直连并传播矩形/双九宫格/选中矩形stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。候选工作包为`136/422 = 131 platform_adapted + 5 assembly_exact + 286 pending_audit`，连续双跑SHA256为`a4f355c35bf8e5786acb86f2c2dc611b8680091b7e0f31f1974752ee683aacee`。当前缺少原版组A对象、固定selector初始化/刷新/替代查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=137`的`0x00466190`已关闭为`platform_adapted`。完整权威LST主体`0x00466190..0x004664FB`从proc到endp共381行、260条带机器码和真实助记符的实际指令、20个静态call、10个跳转、7个局部/返回标签、1个返回点且无外部chunk。queued为0时返回EAX/ECX 0并保留入口EDX；非零时清共享面板记录，动作`0x233B`更新失败继续。固定`242x156`矩形返回EAX保留高word并替换资源低word后绘制顶部九宫格；第一九宫格返回EDX同样替换低word后绘制主体九宫格。标题复用静态文字表索引9，位置`origin+106,+8`、颜色`0xFFC0`，随后配置三项字体。首组查询固定mode0/page1，发布primary count低word到row-limit并以步长EDX刷新；次组总量查询后第二次刷新故意保留该callee返回EDX。十格固定两列五行，caller多推滚动参数保持未读。cell不大于首组时复用首查文字；处于次组时按page查询文字和signed i16组容量，先增加group index并精确等于容量时推进page；超过总量时只复制CP950“無\0”三字节并保留旧缓冲尾。selected cell在来源推进后直连`106x24`矩形，矩形成功才用返回EAX高word+主色低word绘制选中文字，发布输入门，并按page、group index等于1和首组零依次修正target；随后仍执行公共文字和字体，选中格共绘两次。十格后以一过尾cell 11作为EAX/EDX进入最终字体，再以secondary count替换ECX低word并按u16回绕加到row-limit。面板、row-limit、主色、门和target复用既有owner，mode state只保留单一20-byte缓冲和三个栈计数。选择帧旧mode槽reserved且零调用，message 30直连并传播矩形/双九宫格/选中矩形stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。候选工作包为`137/422 = 132 platform_adapted + 5 assembly_exact + 285 pending_audit`，连续双跑SHA256为`fc91d043d0c393bd047bdb6ef98d8f3a09423d39ef9fba592409cf359619749b`。当前缺少原版组A对象、首组/次组/总量callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=138`的`0x00466500`已关闭为`platform_adapted`。完整权威LST主体`0x00466500..0x004667AD`从proc到endp共293行、197条带机器码和真实助记符的实际指令、17个静态call、6个跳转、7个局部/返回标签、1个返回点且无外部chunk。queued为0时返回EAX 0并保留入口ECX/EDX；非零时清共享面板记录，动作`0x233B`更新失败继续。固定`190x172`矩形后，矩形返回EDX保留高word并替换资源低word绘制顶部九宫格；第一九宫格返回EDX同样替换低word绘制主体九宫格。标题复用静态文字表索引19，位置`origin+80,+8`、颜色`0xFFC0`，随后配置三项字体。标题后先清共享单byte再访问组A actor；初始化、刷新和查询保留三种不同EAX/EDX形状。source iterator从1开始，查询结果低word为`0xFFFF`时刷新并结束，为0时只推进iterator且不增加display count，其他值按有效行绘制；七行上限只比较display count低word，持续零结果没有现代扫描上限，第七个有效行后推进iterator但不执行第八次查询。20-byte查询文字物理区直接复用输入分派五dword选择工作区并按x86 little-endian双向装载。有效行固定X=`origin+16`，Y按display count每行22推进；普通颜色保留row-value高word。selected按显示行一基比较，先普通绘制，再用前一文字callee返回ECX高word重绘偏移行，最后直连`192x24`矩形；矩形成功后才配置样式、发布输入门和当前source iterator到candidate，再增加display count。面板、共享byte、主色、输入门、candidate和选择工作区均复用既有owner，narrow state只保存两个栈计数。选择帧旧narrow槽reserved且零调用，message 8直连并传播矩形/双九宫格/选中矩形stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过。源码零warning；app仅保留既有ALSA开发库CMake warning。候选工作包为`138/422 = 133 platform_adapted + 5 assembly_exact + 284 pending_audit`，连续双跑SHA256为`3172b8bdafa13d9629bf159f15dbaa79593207a4e866022804aa90f7652927d2`。当前缺少原版组A对象、窄列表初始化/刷新/查询callee联合状态、字体/文字surface、面板/矩形/九宫格framebuffer及寄存器联合捕获后端，动态差分为`blocked_runtime_oracle`。

`audit_order=139`的`0x004667B0`已关闭为`platform_adapted`。完整权威LST主体`0x004667B0..0x0046694E`共184行、119条实际指令、11个静态call、3个跳转、4个标签、1个返回点且无外部chunk。函数不预清面板动作记录，只覆盖动作与variant并在更新失败时继续；固定矩形后以矩形EAX高word绘制顶部九宫格，绘制“佈置護駕”标题，再以标题EDX高word绘制主体九宫格。随后按group-B行选择先绘制选框。护驾数量从target-effect高word零扩展，只有0跳过；循环无现代上限，每轮先设字体宽18，再从`group_a_count-count`起查询组A尾段名称并按22像素行距绘制。数量精确为1时补“無”，最终无条件恢复宽16。面板、组A数量、组B选择和target-effect均复用既有owner，不新增物理state。选择帧message 5直连，旧槽reserved且零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA warning。候选工作包为`139/422 = 134 platform_adapted + 5 assembly_exact + 283 pending_audit`，双跑SHA256为`936b60aaba238f15dca1ee80107256f3b471a5566ec9d24257d33c1c11e5ec0d`。动态差分因原版组A对象、名称/字体/文字/画面及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=140`的`0x00466950`已关闭为`platform_adapted`。完整权威LST主体`0x00466950..0x00466BFE`共317行、217条实际指令、18个静态call、12个跳转、7个标签、2个返回点且无外部chunk。入口按queued code读取五dword分组首项，值1、选择阻断1、published按i32不在`1..group-B count`时返回；第九对象可过count门但在首次真实名称call停止。名称byte长度保留`cdq/sub/sar`向零除2；mirror精确等于1才以`630-20*字符数-originX`布局名称，其他值使用originX。函数不预清面板记录，动作更新失败继续；矩形返回ECX高word与资源低word组合九宫格，随后字体20绘制名称再恢复16。指标低word至少10时以CP950“生命:”和signed `%d/%d`写20-byte局部缓冲，首次越界typed-stop；指标至少15且宽度非零时直连高3常量色垂直渐变。生命文字与渐变按mirror非零选择另一侧，保留mirror值2时名称不镜像而指标镜像的不对称。五dword分组、queued/published、组B count、阻断、mirror、面板与绘制状态全部复用既有owner，只由提示state持有共享渐变颜色槽。选择帧message 3 live输入门以固定`12/14`直连，旧槽reserved且零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA warning。候选工作包为`140/422 = 135 platform_adapted + 5 assembly_exact + 282 pending_audit`，双跑SHA256为`205da8f18e56d26ecd50c03f82cc2ea37965c57f4261200f7c9146f6f57dcff3`。动态差分因原版组B对象、名称/指标/文字/画面、动态栈地址及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=141`的`0x00466C00`已关闭为`platform_adapted`。完整权威LST主体`0x00466C00..0x00466F6D`共384行、247条实际指令、24个静态call、8个跳转、6个标签、1个返回点且无外部chunk。函数先以固定资源、横向重复4、纵向2绘制标题边框并显示“控制”，再以动态alternate limit绘制主体边框；两者颜色72并复用同一边框与渐变颜色state。字体reset后normal样式绘制“攻擊”，selected 1时selected样式重绘。随后三次主查询和两次特殊查询都在调用前清24-byte共享文字；selected group-B index按i16 sign-extension，对象越界在首次真实查询停止。失败查询只推进源索引；成功项压缩为连续选择索引并令Y加20。主项选中发布transition A与B=0；特殊项把文字重写为CP950“特殊”加一基序号，选中发布A=0与一基B。最后“釋放”索引为成功动态总数加2，并无条件恢复normal style。24-byte文字区由input-dispatch紧邻既有选择工作区的六dword唯一owner承接；其余alternate、组B索引、transition、边框和颜色槽均复用既有owner。选择帧message 7以live原点`+12/+8`及selected直连，旧槽reserved且零调用。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake warning。工作包正式推进到`141/422 = 136 platform_adapted + 5 assembly_exact + 281 pending_audit`，双跑SHA256为`ec03616724b2cb381d5b545c7d9504179429aa2653f420c06dd55669a3913666`。动态差分因原版组B对象、主/特殊查询、字体/文字、动态栈地址、边框画面及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=142`的`0x00466F70`已关闭为`platform_adapted`。完整权威LST主体`0x00466F70..0x004676BC`共911行、560条实际指令、40个静态call、47个跳转、43个局部标签、1个default标签、21个返回点且无外部chunk。入口先检查物理链门与第二对象门，再以message减96分派96–104和110–113十三项，105–109及域外值原样默认返回。96清三门；97按live组A数量读取显示坐标并sign-extend；98先置cache。99先清阻断/cleanup并置抑制，按live u32组B数量执行重置加完成查询，未全完成清message；再按live组A数量重置对象，读取双word控制。空控制转100，已完成角色转98/aux2；完整路径遍历组A三call链，重建18条28-byte记录与七dword优先记录，发布动作14、active/committed、50-dword表、published、cache与门，按56-byte profile解析道具并直连玩家道具数量，成功保存payload、aux1并u32递减special count。100依次直连胜利奖励与升级提示面板再走signed 150阈值；102/103同用signed 150阈值、103 debug bit3使用signed 30、104使用signed大于20，三处目标选择进入已直连。101在actor为FF时先直连角色升级属性提交，仍无actor才调用既有选角；101/110再按i8 actor与transition state查询/分配`2,0`或`4,0`，110在transition非零时直连角色成长对照面板，111固定直连成长标题框后再递增timer；112/113只在FF入口执行选角、sample、查询与`8,1`分配；112取得有效actor后直连成长完成标题框再递增timer，113有效actor仍进入其下一工作包边界，缺失时分别转113/102。message、数量、记录、workspace、标签、profile、sample、调试、角色与目标状态复用既有owner；message新state只承接物理链门、mode门和组B旁路门；当前道具token修正为胜利奖励十项payload数组第0项，全局重置清后两门和全部十项payload并保留链门。主帧HUD及第一后置阶段后直连本实现，旧第二后置槽reserved且生产零调用；子stop阻断第三后置及全部帧尾。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake warning。工作包正式推进到`142/422 = 137 platform_adapted + 5 assembly_exact + 280 pending_audit`，双跑SHA256为`ca25ce978b2a9e9ee07d8af88995b78fb29c1916f27fd3c75bc8481a68c1c72f`。动态差分因原版两组角色对象、18类未审callee、profile/音效/目标选择联合状态、动态链门及寄存器捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=143`的`0x00467710`已关闭为`platform_adapted`。完整权威LST主体`0x00467710..0x00467AB9`共418行、249条实际指令、22个静态call、18个跳转、15个标签、1个返回点且无外部chunk。函数不清共享动作记录，只发布胜利面板动作后直连矩形与双九宫格并绘制CP950“戰鬥勝利”；资源低word分别与矩形、标题文字返回EDX高word组合，第二面板底边使用live transition stage。奖励word位15已置位时只查询结算面板；否则依次渐隐音乐、停止全部sample、以live signed mix播放胜利音效。组B按live i32数量枚举对象并查询掉落，十个u16编号槽命中时只增加既有数量；未命中时以共享u16计数为槽，先直连玩家道具数量，再依次增加数量/计数并保存payload和编号，第十一个新槽在玩家道具副作用后首次数量访问停止。组A按live i32数量枚举，两个对象字段精确1或阻断查询非零时跳过；其余按共享标签访问56-byte世界角色资源，低于阈值时增加每人经验，再调用奖励、更新稀疏计数并执行准备/配置，组B数量至少3时计数再加一。最后先置奖励位15并清timer，再把低15位银币u32回绕加入世界剧情VM银币唯一owner；面板查询精确1时复用64-byte局部缓冲绘制“每人得到經驗值”“得到銀幣”“得到法寶經驗值”三行。十项payload修正上一工作包单标量误名；全局重置只清原写集合覆盖的payload、编号/数量前两项和三项奖励word。消息100先直连本实现再直连升级提示面板，旧槽reserved且零调用；本函数子stop阻断升级面板与caller，升级面板子stop保留胜利结算前缀并阻断caller准备门与timer。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。工作包正式推进到`143/422 = 138 platform_adapted + 5 assembly_exact + 279 pending_audit`，双跑SHA256为`87e1e9218cbddfd6f203b37d9b5d1d51db38b74b2b0e168faf0ab0cfc6a390ea`。动态差分因原版两组角色对象、六类未审callee、动作/画面/字体/音频、动态栈地址及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=144`的`0x00467AC0`已关闭为`platform_adapted`。完整权威LST主体`0x00467AC0..0x00467C41`共168行、99条实际指令、8个静态call、4个跳转、3个标签、1个返回点且无外部chunk。入口初始化64-byte局部文字；过渡角色不为FF时绘底板，为FF但过渡模式精确1时仍绘，其他模式跳过底板但继续公共查询。底板复用胜利奖励动作记录，只写动作`0x233B`和variant 0；固定矩形后第一九宫格资源保留矩形EDX高word，绘制CP950“升級”，第二九宫格资源改保留标题文字EDX高word，底边使用live transition stage。随后固定查询`212,244,3`，返回精确1才重读live actor；非FF actor按i8符号扩展访问十项标签，保留`label*7`与`label*16`算术，再从世界剧情VM四项角色资源的`+0x2C`读取独立等级byte。格式token实际是连续CP950`%s升第%d級`而非IDA误标的独立`%s`；名称token按标签16-byte缩放，格式成功后在`208,220`绘制。64-byte局部缓冲最多容纳63个非NUL byte；溢出在格式副作用后、文字绘制前停止。动作记录、过渡actor/mode/stage、标签与角色资源均复用既有owner，不新增物理state。消息100在胜利奖励后直连本实现，旧阶段槽继续reserved且零调用；子stop保留胜利结算与升级画面前缀并阻断caller全部门和timer。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅保留既有ALSA开发库CMake warning。工作包正式推进到`144/422 = 139 platform_adapted + 5 assembly_exact + 278 pending_audit`，双跑SHA256为`13da9421e983c01817de7fca4110e731a02667d26b612cf535ef6a1f0e10ce4f`。动态差分因原版动作/画面/字体、结算查询、角色名称/等级、动态栈地址、格式返回及寄存器捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=145`的`0x00467C50`已关闭为`platform_adapted`。完整权威LST主体`0x00467C50..0x00467EF2`共260行、153条实际指令、5个静态call、8个跳转、5个标签、1个返回点且无外部chunk。函数按live i32组A数量扫描，两个对象字段精确1则跳过；读取动作标签后只清EDX低word，以角色56-byte记录`+0x2C`独立等级byte替换CL并构造新等级。旧等级按u16不低于阈值时清过渡actor；低于阈值则查询升级需求，返回后重读live标签并以角色首dword对需求做i32 signed门。符合条件时清两份56-byte模板、完整复制当前56-byte角色记录到第三scratch，分别以旧/新等级和共享过渡模式生成模板。提交阶段直接替换等级byte与field20，按u16独立回绕应用三组limit差值并同步current，再应用`+0x10..+0x1E`八个u16差值；首dword、transient、field28和尾11 byte保持。随后发布当前actor，停止旧提示音并以live mix播放升级音，替换sample返回AL后首成功立即退出；所有正常路径最终置完成门，typed-stop不置。世界角色资源owner补齐`+0x2D..+0x37`尾11 byte并以`sizeof==0x38`锁定；同时修正升级面板把等级从误解释的`+0x1C`改为真实`+0x2C`。消息101在actor FF时直连本实现，成功actor直接进入原完成查询/转场，仍FF才选角；子stop阻断选角与全部后续写。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。工作包正式推进为`145/422 = 140 platform_adapted + 5 assembly_exact + 277 pending_audit`，双跑SHA256为`e1786fd3b3bb388fc1d3ddd994b5b8833a7215fb66dbbd022cff7d00275b29f8`。动态差分因原版组A对象、需求/双模板callee、三份scratch、角色记录加载、sample对象及寄存器捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=146`的`0x00467F00`已关闭为`platform_adapted`。完整权威LST主体`0x00467F00..0x00468927`共1007行、608条实际指令、48个静态call、33个跳转、14个标签、1个返回点且无外部chunk。actor FF时在64-byte局部缓冲初始化后立即早退；其他actor复用动作`0x233B`，绘制固定矩形、双九宫格与角色名称，再固定查询成长面板。查询精确1后从上一升级提交的56-byte快照显示七条CP950基线：三个limit按i16、四个派生属性按u16格式。成长阶段100清九项差值并直接显示当前值；阶段29按live actor/label以u16回绕计算三个primary和六个secondary差值，再递增到30；其他小于30的阶段只递增并在达到30前返回，30以上保持。当前对照每项重读live actor/label，前三项按signed增长、后四项按unsigned增长；增长项绘箭头和`current-remaining`。primary前三项与secondary显示四项各自串行递减，一帧每链至多推进一项并播放提示音；最后两项差值只计算和清理，不显示也不递减，保留原遗漏。消息110在transition存在时直连本实现，旧槽reserved且生产零调用；子stop直接传播，caller无后续写。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。工作包正式推进为`146/422 = 141 platform_adapted + 5 assembly_exact + 276 pending_audit`，双跑SHA256为`dc845eea6543cb0f8bd80ebb1d948d92b243705c463a763c24413f81aa3fcb92`。动态差分因原版动作/画面/字体、查询callee、角色名称与记录、动态栈、格式、sample及寄存器捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=147`的`0x00468930`已关闭为`platform_adapted`。完整权威LST主体`0x00468930..0x00468AC5`共173行、117条实际指令、10个静态call、2个跳转、1个标签、1个返回点且无外部chunk。入口初始化64-byte局部缓冲，仅transition mode精确1继续；随后按i8 actor访问标签，以`%s`格式CP950名称并调用长度，按有符号修正后除二。复用动作`0x233B`，矩形宽度为`floor(name_length/2)*20+8`，九宫格右边界为该值基准加250；高度与底边使用live transition stage。固定查询精确1后在第一行绘名称，清零完整64-byte缓冲，再以`[%s]`包裹共享24-byte标题。标题24 byte内缺NUL时在格式部分副作用后typed-stop；正常时第二次长度按原SAR/SHL链计算x=`250+floor(name_length/2)*8-floor(detail_length/4)*16`并绘第二行。共享标题加入角色升级逻辑owner，与相邻输入文字工作区保持独立；原全局重置会清该数组。消息111固定直连本实现，正常返回后u32递增timer；子stop阻断timer，旧槽reserved且生产零调用。补齐全局重置末尾六dword写对该typed owner的同步后，验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部重新通过；最终重跑无warning/error/failure，首轮app仅有既有ALSA开发库白名单提示。候选工作包为`147/422 = 142 platform_adapted + 5 assembly_exact + 275 pending_audit`，双跑SHA256为`2a0f1331d9bb1c1a3634dd29600722cd0ad26fe6c784e2ac2a0c3e4e74e60294`。动态差分因原版动作/画面/字体、查询callee、完整名称表与共享标题、动态栈、格式/长度及寄存器捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=148`的`0x00468AD0`已关闭为`platform_adapted`。完整权威LST主体`0x00468AD0..0x00468C7E`共187行、125条实际指令、11个静态call、3个跳转、2个标签、1个返回点且无外部chunk。入口先把live seed首byte写入64-byte局部缓冲并清其余63 byte，仅transition mode精确1继续。live stage为零时先以sample `0x160`和signed mix level播放，再按i8 actor访问十项动作标签；负一actor停止保留此前sample。名称格式、signed长度除二、动作`0x233B`、动态矩形/九宫格、固定查询、双行文字和24-byte共享标题复用相邻函数的业务核心，但独立variant锁定第一次格式、两次文字和第二次格式的不同EAX/ECX/EDX布局。第二行仍按`250+floor(name_length/2)*8-floor(detail_length/4)*16`居中。消息112在actor有效后直连本实现，正常返回才u32递增timer；子stop阻断timer，旧槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core `188/188`和Windows app `194/194`全部通过，源码构建零warning。候选工作包为`148/422 = 143 platform_adapted + 5 assembly_exact + 274 pending_audit`，双跑SHA256为`7c163bf631b27b6f3b63f35fcd8b395079cdcc0bc9fb729caf715e9b4b679325`。动态差分因原版seed全局、sample对象、动作/画面/字体、标题查询、完整名称与共享标题、动态栈、格式/长度及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=149`的`0x00468C80`已关闭为`platform_adapted`。完整权威LST主体`0x00468C80..0x00468CCE`共245行、144条实际指令、9个静态call、18个跳转、10个标签、1个返回点且无外部chunk。函数按live i32组A数量扫描，两个角色字段或完成查询精确1时跳过；动作标签映射四项成长profile，但道具链仍按物理角色索引读取。类型`0x1F`节点逐项加载共享定义scratch并释放临时说明，最后匹配项覆盖成长限制与高位置位道具码；奖励计数/限制按i32比较，低31位零值跳过。道具`0x1BB0`存在查询只有精确1才启用`0x0665/0x0669`黑名单。符合条件时按profile低u16派生编号，分配并清零`0xB0`节点、链接到共享世界道具链、加载160-byte定义，先置mode 1再复制24-byte标题，最后发布actor并清计数、重建限制/道具码；扫描不首成功早退，最后成功角色获胜。空分配在原memset访问点typed-stop；24-byte无NUL标题在写满24 byte后typed-stop，均保留此前副作用。消息112在actor FF时直连本实现，仍FF才转113；子stop阻断sample、完成查询、transition分配、完成标题与timer，旧选角槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`149/422 = 144 platform_adapted + 5 assembly_exact + 273 pending_audit`，双跑SHA256为`e80f04f0f6c9e85029d834b859d2aea359e5cb664e7982c2d54c08eebcc99f00`。动态差分因原版组A对象、完成查询、MON定义/说明、共享道具链、内部bit查询、分配器、全局scratch/profile/标题、动态堆地址、字符串复制返回及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=150`的`0x00468ED0`已关闭为`platform_adapted`。完整权威LST主体`0x00468ED0..0x00468FEC`共128行、84条实际指令、9个静态call、2个跳转、1个标签、1个返回点且无外部chunk。入口初始化64-byte局部文字，仅transition mode精确1继续；以CP950`法寶%s已完全成長!!`包装共享24-byte成长标题，缺NUL时在首次标题源越界停止，格式达到64 byte时保留完整写前缀后停止。第一次长度按signed向零除二，矩形宽为`half*17+44`、高为live stage加8；九宫格右边界为`half*17+232`、底边为stage加212，资源低word复用共享胜利面板记录并保留矩形返回EDX高word。第二次长度后固定查询`212,244,3`；返回精确1才把字体设为17，在`216,218`绘制单行文字，再恢复16。消息113在既有选角/sample/完成查询/分配链后直连本实现，正常返回才递增timer；子stop阻断timer，旧阶段113槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`150/422 = 145 platform_adapted + 5 assembly_exact + 272 pending_audit`，双跑SHA256为`32d5efc7ec0b5b33d2509027d994421177ebf17d42b093038a8377b453f7d2f8`。动态差分因原版共享标题/transition、framebuffer/边框、法宝提示查询、字体对象、动态栈、格式/长度及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=151`的`0x00468FF0`已关闭为`platform_adapted`。完整权威LST主体`0x00468FF0..0x0046907F`共77行、46条实际指令、5个静态call、6个跳转、4个标签、2个返回点且无外部chunk。函数以live i32组A数量扫描物理角色，每次跳过后重读数量；两项角色字段或完成查询精确1时跳过，成长结果callee只测试AX，低16位零时忽略非零高word。首个AX非零结果把完整32-bit编号传给定义加载，随后释放临时说明，先置mode 1，再把共享160-byte scratch标题复制到24-byte成长标题，最后发布物理actor并立即返回，不继续扫描。标题缺NUL时保留mode和完整24-byte复制前缀，在第25个目标byte停止并不发布actor。消息113在actor缺失时直连本实现；子stop阻断sample、完成查询、分配、无actor fallback、法宝完成提示与timer，旧选角槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`151/422 = 146 platform_adapted + 5 assembly_exact + 271 pending_audit`，双跑SHA256为`d85413d8c01df5e089d1bf183e11624b8e2a436ff867074bcdf59e07136b01ee`。动态差分因原版组A对象、完成查询、成长结果profile链及选择callee、MON定义/说明、共享scratch/标题/transition、字符串复制返回及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=152`的`0x00469080`已关闭为`platform_adapted`。完整权威LST主体`0x00469080..0x00469211`共182行、115条实际指令、10个静态call、3个跳转、2个标签、1个返回点且无外部chunk。函数建立64-byte局部缓冲，将live u16战利品数量用于`212+count*20`清单底边，以字体18、共享动作`0x233B`、矩形和双层九宫格绘制CP950“戰利品”；两次九宫格资源均只替换前一callee返回EAX低word并保留高16位。固定查询精确1且live数量非零时，按十项名称token/u16数量表用`%-12s X %2d`逐行格式化，在`210,212+index*20`绘制；每行后重读live数量。第十一项首次真实访问和64-byte格式后续NUL分别typed-stop，保留此前画面/文字前缀且不恢复字体。正常路径最终恢复字体16。消息102在非零数量的timer与signed 150目标选择链后直连本实现；子stop保留timer/目标选择前缀并阻断后续主帧，旧阶段102槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`152/422 = 147 platform_adapted + 5 assembly_exact + 270 pending_audit`，双跑SHA256为`2388ebe8469f93f60a05c2d65fbb53054820ffd64c7637f38986c9527eeeb978`。动态差分因原版seed、framebuffer/字体/边框、查询callee、名称指针/格式、动态栈、动作/画面/文字及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=153`的`0x00469220`已关闭为`platform_adapted`。完整权威LST主体`0x00469220..0x0046933B`共119行、68条实际指令、9个静态call、1个跳转、1个返回标签、1个返回点且无外部chunk。函数复用共享动作`0x233B`，以live stage构造矩形和双层九宫格；两次资源均只替换前一callee返回ECX低word并保留高16位。标题在`260,180`绘制CP950“戰鬥失敗”，固定查询`212,244,3`精确1时以字体17在`254,216`绘制“隊伍全滅!!”并恢复16；查询非1直接返回且不改字体。矩形或九宫格stop保留此前画面前缀并阻断后续查询/字体。消息103普通路径直连本实现，调试bit路径跳过；面板成功后才执行signed 150计时，子stop阻断timer、目标选择和后续主帧，旧阶段103槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`153/422 = 148 platform_adapted + 5 assembly_exact + 269 pending_audit`，双跑SHA256为`4bdf1012bf7cedd05117ec62ad7c7d8f4476743fb05bbfcdb34e6c620a4ecfa3`。动态差分因原版framebuffer/字体/边框、查询callee、动作/画面/文字及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=154`的`0x00469340`已关闭为`platform_adapted`。完整权威LST主体`0x00469340..0x004694D5`共178行、103条实际指令、10个静态call、2个跳转、2个标签、2个返回点且无外部chunk。函数初始化64-byte局部缓冲，复用共享动作`0x233B`并以live stage绘制矩形和双层九宫格；首层资源替换矩形返回EDX低word，次层资源替换首层返回ECX低word，两者均保留高16位。固定查询`212,252,3`非1时直接返回；精确1时重读live结果byte，精确1显示CP950“煉符成功”并用首项完整32-bit名称token格式化“得到符咒:%s”，其他值显示“煉符失敗”和“沒有得到東西”。64-byte格式缺NUL时保留成功标题与完整前缀，在第65个目标byte停止。消息98先置cache A再直连本实现；子stop阻断后续主帧，旧准备槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`154/422 = 149 platform_adapted + 5 assembly_exact + 268 pending_audit`，双跑SHA256为`a8a9154297ad68148d10ecbf6fc64ee312ac2b10261581696b4a55fc9fb7c6a6`。动态差分因原版seed、framebuffer/字体/边框、查询callee、名称指针/格式、动态栈、动作/画面/文字及寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=155`的`0x004694E0`已关闭为`assembly_exact`。完整权威LST主体`0x004694E0..0x00469542`共53行、31条实际指令、0个静态call、6个跳转、5个标签、1个返回点且无外部chunk。函数在共享控制pair高word非零时完整保留EAX/ECX/EDX与状态并短路；否则按4行×10列row-major扫描startup reset唯一40-word表。首个非零项先发布0..3行号，再重读完整u16值、只清命中半word并写pair高word，返回扁平索引、`0x00520000 | value`和行首token。四十项全零时返回EAX 10与双表尾token，并保留陈旧低word。动作分派case 13前四个角色行直接写同一表、后六行保留相邻tail；消息99直连本实现并继续消费行号/值，旧准备控制槽reserved且生产零调用。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194 全部通过。源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`155/422 = 149 platform_adapted + 6 assembly_exact + 267 pending_audit`，双跑SHA256为`7c9d24718fc9f4fc8b69161e8bbbbac4444290ae315173d2c2715531d349f769`。动态差分因原版共享表/pair与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=156`的`0x00469550`已关闭为`platform_adapted`。完整权威LST主体`0x00469550..0x00469619`共107行、73条实际指令、5个静态call、2个跳转、1个标签、2个返回点且无外部chunk。函数复用胜利结算唯一共享动作记录，写action `0x233B`与base variant零后更新动作；矩形参数保持`left,top,width,height,0,4,4,0`，内框按低32位回绕缩入4像素。九宫格资源从完整width重建高16位并只以live动作`field_4a`替换低word。显式文字X/Y都为零时改用`left+2,top+4`，否则保留调用者坐标；两路入口寄存器分别保留九宫格陈旧EDX或重建framebuffer/font/text链，最终返回文字callee完整EAX/ECX/EDX。HUD顶部姓名面板和footer提示均已直连本实现，footer保留signed向零除3及符号修正寄存器链；旧地址仅保留reserved常量且生产零调用。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`156/422 = 150 platform_adapted + 6 assembly_exact + 266 pending_audit`，双跑SHA256为`5bad96807c4944571a78f33f07f6c543c722dc48a546645d12de0060dccf8494`。动态差分因原版动作更新、framebuffer、矩形、九宫格、字体/文字与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=157`的`0x00469620`已关闭为`assembly_exact`。完整权威LST主体`0x00469620..0x00469648`共24行、14条实际指令、0个静态call、0个跳转、0个标签、1个返回点且无外部chunk。函数按低32位回绕计算`target - transition_stage - base_offset`，以x86 signed `idiv`向零取得商和余数，再把商回绕加到共享stage；商零时EAX/ECX返回1，否则返回0，EDX保留余数。除零与`INT_MIN/-1`在原`idiv`点typed-stop，保留numerator、入口stage和`cdq`符号扩展且不写stage。胜利摘要、升级提示、成长属性、两类成长标题、法宝完成提示、战利品清单、战败提示、炼符结果和调试状态面板十个caller均直连唯一stage owner；各旧“查询面板”及后置阶段端口槽改为reserved且生产零调用。同帧连续面板现观察前一caller的live stage写回，不再由测试伪造互相矛盾的任意返回。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。候选工作包为`157/422 = 150 platform_adapted + 7 assembly_exact + 265 pending_audit`，双跑SHA256为`52de3bfa93a4c4eeaceea18439d071df62783ee6891db4caa42e7e18af18447a`。动态差分因原版共享stage与十个caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=158`的`0x00469650`已关闭为`platform_adapted`。完整权威LST主体`0x00469650..0x004698D8`共283行、196条实际指令、18个静态call、7个跳转、6个标签、1个返回点且无外部chunk。调试位`0x20`关闭时只完成8-byte局部seed初始化并返回；开启时复用胜利奖励唯一动作记录，以live stage绘制动态矩形、第一九宫格和profile标题，再以标题返回ECX高word与live动作低word绘制第二动态框。随后直连已关闭stage推进，固定参数`122,302,3`，仅返回1时访问profile `+0x92`九个signed byte。九行按20像素间距绘制固定标签与3像素渐变；零、正、负和小于`-10`分别保留颜色、`lstrcpyA/wsprintfA`、两次float乘法与x87向零qword转换，深负值先完成普通负值副作用再执行覆盖颜色/格式/宽度。局部格式文字虽未传给文字callee仍保留写入；正常尾返回EAX 0、标签表尾EDX和最后渐变ECX。动态profile token和九byte快照加入目标选择runtime唯一owner，动作/颜色渐变/stage复用既有owner。主帧在对话后、双倒计时前直连本实现，旧post-dialog槽reserved且生产零调用，子stop阻断全部帧尾。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。候选工作包为`158/422 = 151 platform_adapted + 7 assembly_exact + 264 pending_audit`，双跑SHA256为`55df5c550e4865dbfbf26e5b7efdf189bb41c6779905bc5a4be200dffa2f0dac`。动态差分因原版profile、动作/画面/字体、动态栈和主帧寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=159`的`0x004698E0`已关闭为`platform_adapted`。完整权威LST主体`0x004698E0..0x0046995F`共67行、43条实际指令、2个call、3个跳转、3个标签、1个返回点且无外部chunk。函数固定分配并清零36-byte文字节点，按原顺序写两项参数、16-bit类型、文字token和`lstrlenA`长度，再OR flags；入口flags的bit6置位时覆盖第二参数为1及`+0x14=0xFFFFFFE0`。随后沿共享链头逐next扫描到尾并追加新节点，空链直接写头，正常返回EAX 0；零分配、文字不可访问和未知链token只在原首次真实访问点typed-stop并保留此前副作用。链头复用startup reset物理块首项，动态节点存储由startup state唯一承接且启动reset同步清空。40个静态caller中17处已关闭caller全部直连，原opaque槽改为reserved且保持旧枚举数值；23处未审caller留到所属工作包。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。候选工作包为`159/422 = 152 platform_adapted + 7 assembly_exact + 263 pending_audit`，双跑SHA256为`602c2a247eba0eda1d698b14ce4921d2bceb9307dc53f352c5440c18ce0a0e23`。动态差分因原版动态分配地址、真实CP950文字地址、共享链节点、40个caller联合寄存器及未审caller状态捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=160`的`0x00469960`已关闭为`platform_adapted`。完整权威LST主体`0x00469960..0x00469D10`共440行、306条实际指令、15个静态call、27个跳转、23个标签、1个返回点且无外部chunk。函数以两轮遍历处理第159项共享36-byte文字消息链：第一轮按bit31/30绘面板壳与固定渐变，按bit0/1确定文字X，只有i16计时大于0才依次执行bit4/5/6滑入、绘字并dec word；第二轮重读live链头，计时过期后按bit5向右、bit4向左、bit6向上顺序滑出，未达阈值保留，否则先原位摘链再释放并继续处理连续过期节点。bit4零X初始化640只发生在活动路径，bit5不初始化；bit6活动路径按CDQ/SUB/SAR向零逼近，过期路径只在冻结门为0时减Y但无条件倍增步长。冻结门复用主帧`render_abort_latch`并在每个原访问点live读取；链头、动态节点、动作记录、颜色渐变、framebuffer及画面接口全部复用既有唯一owner。主帧在消息阶段之后、packed-row之前直连，旧第三后置槽reserved且生产零调用，子stop阻断全部帧尾；新增文字和释放服务追加枚举尾部，旧值不平移。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。候选工作包为`160/422 = 153 platform_adapted + 7 assembly_exact + 262 pending_audit`，双跑SHA256为`468fe740f03f146000afab5d6089db5c75b6e6a8e861a2a9a0a683d6e7ed6f23`。动态差分因原版动态链节点、字体/文字surface、释放器、动作/画面、live冻结变化及主帧寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=161`的`0x00469D20`已关闭为`platform_adapted`。完整权威LST主体`0x00469D20..0x0046DF35`共7454行、4669条真实指令、256个call、285个跳转、328个标签、102个返回点且无外部chunk。入口按脚本i16分派`-1..83`共85项，`0/7/32/38`与范围外值保持默认不推进；typed实现逐项保留bit15跨帧状态、变长列表与文字、三类76-byte文字动作、180-byte链节点、24-byte效果节点、玩家道具双数量、x87向零转换、signed除法、陈旧寄存器和每条不对称cursor。`LegacyBattleScriptWorkspace`唯一承接`0x0053CCC8..0x0053CEB8`，其余138项可写状态复用startup、角色指标、输入、选择、消息和奖励owner；角色/组B顺序及攻击顺序插入直接组合既有typed实现，其余未审业务callee保留窄token端口。世界主帧唯一caller已删除固定返回1占位，SDL逐帧调用脚本分派并把0/1/2/3原样交给既有战斗退出恢复分支。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`161/422 = 154 platform_adapted + 7 assembly_exact + 261 pending_audit`，双跑SHA256为`1ab13a95a22a4175d6663b6a2c0e5c185632078aed386bcd51c1763d664373ce`。动态差分因原版共享状态、动态对象、CRT随机、69个callee、画面/音频/文件及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=162`的`0x0046E090`已关闭为`assembly_exact`。完整权威LST主体`0x0046E090..0x0046E096`共10行、2条实际指令、0个call、0个跳转、0个标签、1个返回点且无外部chunk；行为只以ECX this完整读取组A角色`+0x2B00` dword并原样返回EAX。全程序没有真实call，唯一DATA XREF位于脚本case58：前一条固定函数地址先装入EDX并测试，由于PE地址静态非零，`jnz`必然越过本函数地址加载和后续目标槽/攻击顺序块。typed侧继续复用既有角色结算跳过状态，不制造无用getter；case58回归确认组B路径仅推进4字节、零端口调用且攻击顺序不变。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过。工作包为`162/422 = 154 platform_adapted + 8 assembly_exact + 260 pending_audit`，双跑SHA256为`3a3d2dbd8e5b2f3f13402d674cd43e2dc965b85e18d23acc4048c031f4bdad47`；静态不可达且两指令完整审计，无需动态oracle。

`audit_order=163`的`0x0046E0A0`已关闭为`assembly_exact`。完整权威LST主体`0x0046E0A0..0x0046E0A6`共10行、2条实际指令、0个call、0个跳转、0个标签、1个返回点且无外部chunk；行为只以ECX this完整读取组A角色`+0x2B04` dword并原样返回EAX。全程序没有真实call，唯一DATA XREF位于脚本case58组B路径；固定地址装入EDX后测试，静态非零使分支必然跳过后续死块。由于地址加载本身可观察，typed case58以`compat::u32`保留该固定token到返回EDX，但不转换为主机指针或恢复调用。回归同时确认cursor推进4字节、零端口调用与攻击顺序不变。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`163/422 = 154 platform_adapted + 9 assembly_exact + 259 pending_audit`，双跑SHA256为`af7a8f6f695cd1a78feaa4bb3ad950c6615bcdeb3a40edc7069faecd3340072f`；静态不可达、两指令完整审计和caller寄存器回归共同关闭动态差分。

`audit_order=164`的`0x0046E0B0`已关闭为`platform_adapted`。完整权威LST主体`0x0046E0B0..0x0046E1D4`共148行、91条实际指令、9个call、4个跳转、3个返回点且无外部chunk。函数按`0x200+battle_id*4`读取FIGTALK数据偏移，再从`0x200+offset`分配、清零并固定读取`0x8000`-byte脚本窗口；文件已开门复用旧句柄，数据根缺失与零句柄有独立早退。typed `load_legacy_battle_script_window`复用`LegacyBattleAssets::script`唯一owner，以窗口offset承接cursor；持久Win32句柄适配为RAII文件，open/seek/read失败显式状态化，短读保留前缀和零尾。总资产加载器和SDL战斗入口保持FIGTALK→FFD→setup顺序，原启动caller证据已补齐且生产无旧地址/opaque端口。测试覆盖合成表项、6-byte短读、零尾、大小写路径、错误顺序及真实battle 98。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`164/422 = 155 platform_adapted + 9 assembly_exact + 258 pending_audit`，双跑SHA256为`b942b4ad9007de131e998f1dde7ccba88b8a987b47cc1379f478cde573a56605`。动态差分因原版持久句柄、无效句柄零比较、未检查文件调用和动态分配地址后端缺失而为`blocked_runtime_oracle`。

`audit_order=165`的`0x0046E1E0`已关闭为`platform_adapted`。完整权威LST主体`0x0046E1E0..0x0046E25B`共69行、40条实际指令、4个call、1个跳转、1个返回点且无外部chunk。函数释放旧脚本base后分配并清零`0x1000` bytes，把同一token发布为base/cursor，再从FIGTALK绝对位置`offset+0x200`固定读取一页，读取尝试后发布原offset，返回ReadFile BOOL；ECX恢复入口，EDX保留局部读取长度栈token。typed页面加载复用唯一`LegacyBattleAssets::script`，新增活动容量区分初始`0x8000`窗口与`0x1000`页，宿主尾部不可访问；动态分配适配为同一数组，文件调用经SDL命名服务。脚本分派10个LST callsite已回收，case19两条分支汇入一个typed调用点；成功前先发布新页cursor零，失败在服务边界停止。测试覆盖页面offset、短读、零尾、活动容量越界、case48成功/查询失败/服务失败及case61/65新页cursor。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`165/422 = 156 platform_adapted + 9 assembly_exact + 257 pending_audit`，双跑SHA256为`741d16a1a4336e12dd9c31424f3868563beb2d60dce4afdf3cdb4f03df08517c`。动态差分因原版动态地址、持久句柄、未检查文件调用和局部栈token后端缺失而为`blocked_runtime_oracle`。

`audit_order=166`的`0x0046E260`已关闭为`platform_adapted`。主块`0x0046E260..0x0046E285`与外部FUNCTION CHUNK `0x0046E390..0x0046E489`合计119行、53条实际指令、2个call、3个跳转、1个返回点。函数条件关闭FIGTALK句柄并清文件门，随后在chunk写帧门/脚本完成门1，按精确dword/word宽度清四项辅助值、value B/C、坐标、packed actor、等待低word、两项packed值、四word、list count、动态等待、page offset和辅助值，固定frame value写`0xFFFF`；未写的动态token、value A、对象/文字和完成状态保持。base非零才释放并清base/cursor，零base保留cursor。typed直接reset复用资产、workspace和shared唯一owner，固定数组以活动容量表示live分配。终止opcode与case1两个caller均回收旧端口；case1保存入口cursor，shutdown后仍写入口加4并传播2/3。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`166/422 = 157 platform_adapted + 9 assembly_exact + 256 pending_audit`，双跑SHA256为`81364312d136d0458c0e02d45666f9d87beb28d6f50b57a1f3cae320e2c531ff`。动态差分因原版持久句柄、关闭结果、动态地址和释放返回后端缺失而为`blocked_runtime_oracle`。

`audit_order=167`的`0x0046E290`已关闭为`platform_adapted`。完整权威LST主体`0x0046E290..0x0046E385`共105行、69条实际指令、5个call、0个跳转、1个返回点且无外部chunk。函数把四组i16 X/Y控制点符号扩展为float，先执行参数0的死局部曲线采样，再以`frame*0.05`和冻结三次B样条矩阵生成四项基值，按原乘加顺序求X/Y；两项结果分别经x87向零qword转换取低dword，返回最终Y。typed纯函数冻结矩阵位模式、float收窄点与非法转换零低dword；case39唯一caller直接按当前段四点采样，写原value与坐标owner，恢复输出owner token后调用角色坐标服务，再推进20帧/6段状态。旧枚举槽reserved且生产零调用。测试覆盖帧0/10/20的正负曲线锚点、Y返回值及case39第一帧直连。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning；app仅有白名单ALSA提示。工作包为`167/422 = 158 platform_adapted + 9 assembly_exact + 255 pending_audit`，双跑SHA256为`479882f5b376d12d09248f571e0ae3127d3e016f27cef80530fd3f18da50f24a`。动态差分因原版x87、局部栈、输出指针和caller寄存器后端缺失而为`blocked_runtime_oracle`。

`audit_order=168`的`0x0046E490`已关闭为`platform_adapted`。完整权威LST主体`0x0046E490..0x0046E4C4`共31行、19条实际指令、2个call、0个跳转、1个返回点且无外部chunk。thiscall先调用未审基础角色构造，再清对象`+0x2F26/+0x2F18`两个word，分配56-byte附属记录，先发布token后按14 dword清零，正常返回this、ECX零与分配EDX。零分配不提前拒绝：基础构造和两word/token副作用已完成，在首次记录写入点typed-stop并保留旧记录字节。typed元素状态唯一持有对象token、记录token/字节和两word，物理/动态地址保持u32 token。组A向量包装器证据同步更新：构造回调已关闭，但析构与MSVC异常回滚尚未审计，暂保留vector port而不伪造半套EH。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`168/422 = 159 platform_adapted + 9 assembly_exact + 254 pending_audit`，双跑SHA256为`ecc7299e3826e585f760568696ce83324239e2784feb5466b7dccafa10552141`。动态差分因原版基础构造、动态地址、全局对象和向量EH后端缺失而为`blocked_runtime_oracle`。

`audit_order=169`的`0x0046E4D0`已关闭为`platform_adapted`。主块`0x0046E4D0..0x0046E518`与SEH外部chunk `0x00498390..0x0049839D`合计62行、23条实际指令、2个call、2个跳转、1个返回点。正常路径建立SEH后按扩展清理→基础析构执行，返回基础EAX/ECX/EDX；扩展清理在unwind状态0抛出时，chunk重载this并仍调用基础析构，再继续传播原异常。typed析构复用元素唯一状态，两个内部析构保持窄端口；try/catch精确恢复基础析构保证，不吞异常。组A构造/析构回调均已关闭，两个vector包装器证据更新为只隔离MSVC对全局十对象数组的迭代与EH边界。测试覆盖正常附属记录释放、基础寄存器返回及扩展异常时基础析构后重抛。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`169/422 = 160 platform_adapted + 9 assembly_exact + 253 pending_audit`，双跑SHA256为`fa467eb8e05aaa1dd0a07bb27332df1a13b7fa8ad8ef9fddfaa08353e6bbb765`。动态差分因原版两级析构、全局对象、MSVC SEH和vector迭代器后端缺失而为`blocked_runtime_oracle`。

`audit_order=170`的`0x0046E520`已关闭为`platform_adapted`。完整权威LST主体`0x0046E520..0x0046E690`共179行、118条实际指令、0个call、12个跳转、2个返回点且无外部chunk。函数先检查状态word双bit、special/action完成门；通过后以u16 progress和全局i32阈值作signed比较。完成路径置action完成、清transition，按live frame-started/scene门清frame/post-action，固定清双cache并置update-ready，返回1。继续路径从首记录u16基值右移2/可选再右移1，参数1追加四分之一加1，按delay bits保留正负30%、multiplier百分比/二分和固定4点扣除，最后低16位回绕写progress、清完成并返回0。typed统一`LegacyBattleActorProgressState`，组A帧直接调用后按返回1继续AI；转场party/enemy两路更新startup角色状态，旧token/reserved槽生产零调用。测试覆盖入口门、完成尾、正负调整、固定扣除及两个caller。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`170/422 = 161 platform_adapted + 9 assembly_exact + 252 pending_audit`，双跑SHA256为`308250c723c812ae5b277abb1241c514893dc3c6a9c3a25b9836f7134527b894`。动态差分因原版角色对象、首记录、全局阈值和caller共享状态后端缺失而为`blocked_runtime_oracle`。

`audit_order=171`的`0x0046E6A0`已关闭为`platform_adapted`。完整权威LST主体`0x0046E6A0..0x0046E720`共46行、26条实际指令、0个call、0个跳转、1个返回点且无外部chunk。函数先按原顺序清`+0x2F10..+0x2F24`十一项u16和`+0x2F0C`一项u32，再依次以三个`rep stosd`清`+0x2BC8`起`0xBE`项、`+0x0AF0`起`0x4C`项和`+0x2B24`起`0x29`项；高低两段组成`+0x2B24..+0x2EBF`连续`0xE7`项，但保持先高、早期、再低的写序，且不触及`+0x2F0E/+0x2F26`。返回固定EAX零、ECX零、EDX原this。typed工作区挂入startup组A角色唯一记录并绑定实际actor token；唯一caller仍是待审的下一项，当前startup继续以窄端口隔离整个caller，不伪造其余行为。测试覆盖非零填充、精确三段/十二项清零、相邻保留、返回寄存器及startup owner。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`171/422 = 162 platform_adapted + 9 assembly_exact + 251 pending_audit`，双跑SHA256为`d879fea89c0e09f1ae20585691376351145d022eae2143669394d4c03e2c0aa0`。动态差分因原版组A对象、三段物理内存与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=172`的`0x0046E730`已关闭为`platform_adapted`。完整权威LST主体`0x0046E730..0x0046E841`共122行、80条实际指令、2个call、8个跳转、1个返回点且无外部chunk。函数先直连已关闭工作区零化器，再把32-byte placement复制到角色两处并发布尾dword，把56-byte源记录复制到角色基础记录后发布源/辅助token、文字byte和placement word；零placement word调用窄诊断port后继续。基础记录内部复制两项u16，源记录按`+0A,+04,+0C,+06,+0E,+08`顺序以signed i16仅上限夹到9999，角色副本保持夹值前内容；最后清特殊状态并按角色副本bit7置1。正常返回EAX基础记录token、ECX源记录token，EDX低word来自基础记录、高word来自placement或诊断callee。三类token在首次真实访问typed-stop并保留前缀。startup唯一caller已直连typed配置器，旧业务枚举槽reserved，新诊断服务追加尾部，typed-stop阻断后续mode查询。测试覆盖复制、signed夹值、诊断时序/参数/寄存器、特殊bit、三类typed-stop和startup caller回收。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`172/422 = 163 platform_adapted + 9 assembly_exact + 250 pending_audit`，双跑SHA256为`1e8eb49e2803ef138da7b3b4a23e16e85967aa751360dfbf5056e41f9b9d5ac0`。动态差分因原版组A对象、源/placement记录、窗口、诊断callee与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=173`的`0x0046E850`已关闭为`platform_adapted`。完整权威LST主体`0x0046E850..0x0046E860`共11行、4条实际指令、0个call、0个跳转、1个返回点且无外部chunk。函数把单个资源token依次完整写入组A角色`+0x2EC8/+0x2ECC`，返回EAX原token、ECX原this并完整保留入口陈旧EDX。typed资源pair挂入startup组A角色唯一记录；零actor token在首次写typed-stop且两项不变。startup初始队伍绑定循环在pending value callee后直连本实现并把其EDX传入，旧palette枚举槽reserved，正常后继续名字绑定。测试覆盖双写顺序、寄存器、首次写停止及两名caller直连。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`173/422 = 164 platform_adapted + 9 assembly_exact + 249 pending_audit`，双跑SHA256为`39296e047e86652a3e46b68c8bf4ce5496227c435f358a7f6574c6df0c4ea5d8`。动态差分因原版组A对象、共享资源表与caller前后callee寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=174`的`0x0046E870`已关闭为`platform_adapted`。完整权威LST主体`0x0046E870..0x0046E880`共11行、4条实际指令、0个call、0个跳转、1个返回点且无外部chunk。函数把单个完整dword依次写入组A角色`+0x2EC0/+0x2EC4`，返回EAX原值、ECX原this并完整保留入口EDX。typed数值pair挂入startup组A角色唯一记录；零actor token在首次写typed-stop且两项不变。startup初始队伍绑定循环按权威caller以source索引作为EDX、请求party value作为输入直连本实现，再把返回EDX传给已关闭资源pair；旧value枚举槽reserved。测试覆盖双写、寄存器、首次写停止、两名caller直连及相邻资源pair EDX纠正。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`174/422 = 165 platform_adapted + 9 assembly_exact + 248 pending_audit`，双跑SHA256为`87a1424ffba3d06e6b76fb4cbe99b5841dd4d41e8d34dc8468419fdd2ff441be`。动态差分因原版组A对象、party value表与caller相邻callee寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=175`的`0x0046E890`已关闭为`platform_adapted`。完整权威LST主体`0x0046E890..0x0046E9BD`共120行、102条实际指令、4个call、1个条件跳转、1个局部标签、1个返回点且无外部chunk。函数申请并清零0xA4召唤资料，按源记录角色号依次调用资料加载和动态文字释放，把同一32-byte源记录复制到角色两处并发布尾值/角色号；零角色号调用固定诊断后继续。资料六项word/byte、9-byte名字和角色别名按原顺序投影到角色基础记录，最终EAX返回资料token、EDX返回基础记录token，ECX组合资料指定dword高word与指定word。typed实现复用startup party唯一源/角色owner，四类首次访问stop保留此前调用和写入。action15首帧按共享召唤索引直连typed物化器，窗口token沿startup owner传播，旧地址生产零调用，成功后才发布phase。测试覆盖callee顺序、全部投影、非对齐名字、寄存器、诊断、四类stop和caller共享owner。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning。工作包为`175/422 = 166 platform_adapted + 9 assembly_exact + 247 pending_audit`，双跑SHA256为`1f0a60ebecb5d6c5a137cdf799c36d78f0f6372dc7c020c8a7a0374b277fda3d`。动态差分因原版组A对象、0xA4资料、mon.dat加载/动态文字释放、诊断窗口和action15寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=176`的`0x0046E9C0`已关闭为`platform_adapted`。完整权威LST主体`0x0046E9C0..0x0046EBA9`共196行、169条实际指令、4个call、1个条件跳转、1个局部标签、1个返回点且无外部chunk。函数分配并清零0xA4资料，按护援源角色号调用资料加载与动态文字释放，把32-byte源记录双份复制到角色并发布尾值/角色号，零角色号以NPC固定参数诊断后继续。资料主、次i16系数分别调整基础记录六项word/byte：`+0x0A`按signed基值，`+0x26/+0x28/+0x14/+0x16`与byte `+0x2C`按无符号基值，全部保持向零除10和低宽度回绕；随后投影资料byte、9-byte名字、镜像byte、别名token与`+0x60`word。正常返回EAX为基础记录token高word与资料word低word组合、ECX基础记录token、EDX资料token。typed实现复用startup party唯一角色、placement、配置源和基础记录owner；首角色live `+4`既可解析四份配置源，也可在stale首护援物化后切换为其基础记录。五类首次访问stop保留此前分配、清零、callee、复制与诊断副作用。startup随机与顺序两处caller统一直连，旧护援配置槽reserved且生产零调用，子stop阻断激活、mode、计数与使用标记。测试覆盖全部投影、signed/unsigned域、回绕、诊断、寄存器、五类stop、普通双caller、stale动态别名和零调整源前缀。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`176/422 = 167 platform_adapted + 9 assembly_exact + 246 pending_audit`，双跑SHA256为`3f337f7a288106bd5968dc273869d2e89f36673fb52b82ab134b68ae98df41e1`。动态差分因原版组A对象、0xA4资料、mon.dat加载/动态文字释放、动态调整源和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=177`的`0x0046EBB0`已关闭为`platform_adapted`。完整权威LST主体`0x0046EBB0..0x0046EE52`共265行、195条实际指令、2个call、18个跳转、10个局部标签、1个返回点且无外部chunk。函数入口以82个dword清角色两份内嵌0xA4资料；零参数表仍执行16轮空循环。非零表以槽0建立主资料、角色物品号和可选固定诊断，16槽共同按u16低位回绕累加六项word及两组条件word。槽7/8复制内嵌资料，非哨兵时覆盖物品号并调用待审资料应用callee；槽9/10跳过后半段；其余12槽按u8回绕累加九byte，槽0至6按独立gate累加三项角色尾值，非7至10槽的固定物品号写特殊latch。正常EAX为槽15记录token，EDX原this，ECX保留槽15 token高word、条件word高byte和最后属性byte的陈旧组合。typed实现复用世界物品状态64条角色sentinel唯一owner、角色基础记录和既有workspace；主资料与两份内嵌资料保留在角色唯一聚合状态，description只增加compat token元数据。整函数旧opaque槽reserved且生产零调用，唯一startup caller在物品排序后、数值/资源双写前直连；角色`+8`视图纠正为前一配置函数发布的辅助资料owner。三类首次访问stop保留此前清零、复制、诊断、callee和累加前缀，`sub_46F030`继续窄端口隔离。测试覆盖16槽分类、全部word/byte、低位回绕、两份内嵌资料、哨兵、特殊编号、固定诊断、空表、三类stop、最终寄存器、64根caller和缺失sentinel阻断。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`177/422 = 168 platform_adapted + 9 assembly_exact + 245 pending_audit`，双跑SHA256为`35eb20034b71ca82216aba763330245915c0afd684be1fff71f97d1c3fa20970`。动态差分因原版组A对象、64条动态角色物品根、真实资料/description指针、`sub_46F030`、诊断和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=178`的`0x0046EE60`已关闭为`platform_adapted`。完整权威LST主体`0x0046EE60..0x0046F02E`从proc到endp共194行，其中188个非标签物理行、126条实际指令、15个call、7个跳转、6个局部标签、1个返回点且无外部chunk。函数先检查live源记录byte `+0x25`的bit7，再依次处理角色累计u16 `+0x2F1E/+0x2F20/+0x2F22`：乘源记录i16 `+0x0A/+0x0C/+0x0E`并向零除100，低word为零时强制1，首通道再取负。每个活动通道严格执行效果参数、固定资源、signed幅值、六步偏移和固定1收尾五callee，后两通道保留乘积高word与效果低word的陈旧拼接；活动通道完成后清自己的临时word，零累计通道保留旧临时值。typed实现复用startup party唯一workspace、配置源/角色基础记录live token和新增的同角色三word临时owner。组A帧唯一caller在进度返回1后、AI标记读取前直连，旧整函数opaque调用生产零次；子stop保留进度函数已发布的完成前缀。测试覆盖bit7早退、三项零累计、signed百分比、低word截断、强制1、首通道取负、陈旧高word、三资源、0/6/12偏移、15-call序列、临时word生命周期、typed-stop、最终寄存器和caller直连。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`178/422 = 169 platform_adapted + 9 assembly_exact + 244 pending_audit`，双跑SHA256为`13a1a14a300298c4f9f06163ab02ad271eb544ea92b71d33987faaf521bdfc6f`。动态差分因原版组A角色对象、三项live累计、动态源记录、五个callee副作用和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=179`的`0x0046F030`已关闭为`platform_adapted`。完整权威LST主体`0x0046F030..0x0046F1CC`从proc到endp共225行，其中213个非标签物理行、146条实际指令、2个call、6个跳转、12个局部标签、9个返回点且无外部chunk；紧邻八项跳表也已逐项核对。资料类型50、53至57只在角色状态dword低byte OR六种固定mask。类型52按玩家物品数量低word右移1形成附加百分比，再把角色word `+0x26`增加固定10%和该附加值；类型51只处理九byte中的首个非零项，保留八位取负、乘10后再截低byte、signed百分比、负商转u16和再取负十分之一的异常低位链。typed实现复用属性汇总内嵌资料、角色基础记录和唯一状态owner；startup角色重置边界同步清该状态。上一属性汇总槽7/8直接调用，旧整函数opaque槽reserved且生产零调用，待审玩家物品数量查询保留为窄port。测试覆盖八类跳表、异常算术、九项零扫描、低位回绕、typed-stop、寄存器、两次caller、重置和窄查询；独立位级脚本共比对627,104组向量。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`179/422 = 170 platform_adapted + 9 assembly_exact + 243 pending_audit`，双跑SHA256为`c0013b8f0767ddb8f6d379c478c0eade88fd17dbe537000054268d6835ca7304`。动态差分因原版组A角色对象、动态内嵌资料、玩家物品链、数量查询callee和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=180`的`0x0046F1F0`已关闭为`platform_adapted`。完整权威LST主体`0x0046F1F0..0x0046F560`从proc到endp共410行，其中392个非标签物理行、226条实际指令、12个call、17个跳转、18个局部标签、2个返回点且无外部chunk；紧邻16项跳表也已逐项核对。入口懒加载内嵌资料物品id，只对完整32位类型21至36分派。类型21至30、32和33按原分支更新六类低byte状态、两个高byte状态、动作word、显示word、模式byte与激活byte，并选择性刷新共享进度乘数；类型31写四百分比加一派生值；类型34至36分别把乘数缩至约40%、20%、40%，再以基础记录三个signed word计算百分之一乘积加十分之一固定项。全部保留32位回绕、signed高位、算术右移、低word写回和公共数量5后置callee。typed实现复用唯一进度乘数和组A基础记录owner；新增owner仅保存缓存、效果状态、动作显示、模式激活与四个派生word。startup角色重置只同步权威证实的效果状态、动作word和首派生word三项清零，其余陈旧值不清。三个未审callee保持窄port；唯一caller属待审第187项，按规则留到其工作包。测试覆盖首次访问typed-stop、缓存、默认分支、16类型、全部状态写点、三类乘数、signed派生、基础记录stop、寄存器和重置陈旧值；独立位级脚本合计比对1,310,720组向量。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`180/422 = 171 platform_adapted + 9 assembly_exact + 242 pending_audit`，双跑SHA256为`6461117e3146c8594d8e1972c7b3d827868d07ab290a210c719225ed4029f10f`。动态差分因原版角色对象、动态内嵌资料、玩家物品链、三个待审callee和第187项caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=181`的`0x0046F5B0`已关闭为`platform_adapted`。完整权威LST主体`0x0046F5B0..0x0046F6D1`从proc到endp共128行，其中120个非标签物理行、87条实际指令、3个call、10个跳转、8个局部标签、1个返回点且无外部chunk。函数固定扫描组A角色两份内嵌资料：零id跳过，非零id从胜利奖励专用紧凑链根开始查找；命中未阻塞节点时先以u16回绕增加数量，再按unsigned最大值夹住，并以x87扩展精度分步计算百分比。整数改写不等价，理论420的特定输入原结果为419；零最大值保留indefinite。未命中时尾插20字节零节点，写id、数量、百分比，并保留根`+0x04`递增后可能影响第二资料扫描的别名行为。typed实现使用胜利与效果运行路径共享状态端口中的唯一紧凑链owner与startup角色已有两资料；断链、零分配和宿主分配均在对应前缀后stop。已关闭胜利奖励caller直接调用，旧调用及帧协调器槽reserved且生产零调用。测试覆盖全零、命中、阻塞、溢出、断链、尾插、零分母、根别名、caller正常和stop；内联x87联合核对5,229,047组。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过，源码零warning且app仅有既有ALSA提示。工作包为`181/422 = 172 platform_adapted + 9 assembly_exact + 241 pending_audit`，双跑SHA256为`d7c8ab24eb569ac1fe95f02e58c898ec9b573c2a56042166ed2a9d8a63a7ed6c`。动态差分因原版动态资料、紧凑链token拓扑、分配器、x87控制字和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=182`的`0x0046F6E0`已关闭为`platform_adapted`。完整权威LST主体`0x0046F6E0..0x0046F84B`从proc到endp共160行，其中151个非标签物理行、104条实际指令、3个call、15个跳转、9个局部标签、1个返回点且无外部chunk。函数固定扫描组A角色两份资料，依次要求非零id、kind 51、组B目标记录word在1至9及计算地址非零，才把返回latch置1并访问共享奖励链。命中未阻塞节点固定加12，保留u16回绕、unsigned夹值和x87百分比；未命中尾插20字节节点，固定数量12不夹最大值，并保留根`+0x04`递增影响第二资料的别名。typed实现显式保留入口actor差值EDX、目标token低word替换、地址计算和第二轮EDX重载。第181项紧凑链提升为胜利与效果port共享的单一虚拟owner；效果协调器显式借用frame context中的startup两资料和组B目标门word，三个typed分支覆盖原四个静态caller，旧完整函数调用生产零次。测试覆盖四重门、所有owner stop、命中、阻塞、夹值、尾插、断链、零分配、零最大值、根别名、三caller路径和协调器stop；内联x87联合核对458,751组。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`182/422 = 173 platform_adapted + 9 assembly_exact + 240 pending_audit`，双跑SHA256为`b840953fb5739e4319c08f7e2affa12b7ae9e00364597c72c557dd1601464816`。动态差分因原版动态资料、目标记录、共享链token、分配器、x87控制字和四处caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=183`的`0x0046F850`已关闭为`platform_adapted`。完整权威LST主体`0x0046F850..0x0046F8B4`从proc到endp共63行，其中57个非标签物理行、39条实际指令、0个call、8个跳转、6个局部标签、1个返回点且无外部chunk。函数固定扫描组A角色两份资料，从共享紧凑链根按显式token查找同id节点；阻塞或数量不足的重复节点不终止扫描，只有首个未阻塞且数量按unsigned不小于资料最大值的节点成功。成功后把数量向下夹到最大值、置阻塞word 1并返回item id；最大值零立即成功并清数量。首次成功后仍读取第二item id，第二id非零才在链访问前早退，保留对应资料游标与首次节点token。typed实现复用startup两资料和第181、182项共享链owner；已关闭成长结果caller直接调用，旧成长选择槽及frame coordinator转发槽reserved且生产零调用。测试覆盖两资料全零、链头stop、充分/不足/阻塞重复、断链、首成功后的第二资料、零最大值、第二资料成功、定义/描述/标题链、标题stop和message传播。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`183/422 = 174 platform_adapted + 9 assembly_exact + 239 pending_audit`，双跑SHA256为`2b1b964c11dbf21fe09f4abaddf3759f2efbb558028bdf5fdca90187dc9749e5`。动态差分因原版动态资料、共享链token、成长caller寄存器和后续定义加载联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=184`的`0x0046F8C0`已关闭为`platform_adapted`。完整权威LST主体`0x0046F8C0..0x0046FEEE`从proc到endp共668行，其中637个非标签物理行、386条实际指令、11个call、48个跳转、31个局部标签、5个返回点且无外部chunk。函数完整覆盖入口门与目标早退、首/次0x98记录准备、actor flag消费、运行时0x4000与0x8000、七项颜色初始化、slot清理、两类目标调用、活动slot准备、位置计算、完成word、motion绘制、资源typed-stop以及五记录最终清理。完成路径复用全局动作flag、startup actor进度和第180项物品效果flag/激活owner；新增owner只保存actor局部记录、slot、位置、motion和资源视图，胜利跳过数组通过frame coordinator只读span绑定。三处静态caller在已关闭行动调度器中收敛为两个typed调用位置，普通与alternate side均直连，旧完整函数调用生产零次。测试覆盖入口、早退、flag消费、颜色、slot、目标、渲染、motion资源stop、激活倒计时及caller两side、阻塞效果和framebuffer stop。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`184/422 = 175 platform_adapted + 9 assembly_exact + 238 pending_audit`，双跑SHA256为`769db821e21fdc38842ae86bb1cbbf882b1a3453689c411e6fec2c3c8abfa506`。动态差分因原版actor完整状态、目标、资源、九类callee和三处caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=185`的`0x0046FF00`已关闭为`platform_adapted`。完整权威LST主体`0x0046FF00..0x0046FFD0`从proc到endp共95行，其中92个非标签物理行、58条实际指令、1个call、8个跳转、3个局部标签、4个返回点且无外部chunk。函数依次检查两项跳过状态、物品效果、重复identity与内嵌状态bit；状态bit命中立即写profile mode 1但不清计数。否则以共享阈值加8比较完成计数，达到时确定选择并清计数、发布identity；未达到且计数小于8直接返回；计数至少8时固定随机10，只比较AX，AX大于5才选择。typed实现复用第184项actor/shared owner、第179项内嵌状态与第180项物品效果owner，随机保留完整寄存器窄port。唯一caller属待审第187项，按规则不提前拆整体边界。测试覆盖全部门、确定阈值、随机5/6、高word和寄存器。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`185/422 = 176 platform_adapted + 9 assembly_exact + 237 pending_audit`，双跑SHA256为`5bbcff8346c51a72f94294b4442da14e3e6d9b8edc606d85c076f0f4ab96c866`。动态差分因原版actor状态、共享随机计数、随机callee和第187项caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=186`的`0x0046FFE0`已关闭为`platform_adapted`。完整权威LST主体`0x0046FFE0..0x0046FFEC`共8行、3条实际指令、0个call、0个跳转、1个返回点且无外部chunk。函数无条件把actor next list index完整dword复制到current list index，EAX返回复制值，ECX/EDX保持；相等值仍写。两个字段复用第184项每actor动作owner，缺失时在首读取typed-stop。九个静态caller分布于六函数；待审第188至191项留到所属工作包，已关闭两个列表caller因未暴露actor物理owner登记边界缺口而不复制状态。测试覆盖typed-stop、完整32位复制、寄存器和相等写。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`186/422 = 177 platform_adapted + 9 assembly_exact + 236 pending_audit`，双跑SHA256为`ff1f2a148d947032f7b568f189054169c61541330ff2c814479697d9db0272b7`。动态差分因原版索引状态与九处caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=187`的`0x0046FFF0`已关闭为`platform_adapted`。完整权威LST主体`0x0046FFF0..0x00470172`共175行、103条实际指令、4个call、21个跳转、11个标签、2个返回点且无外部chunk。实现零动作早退、模式替换、16字节前置清零、live actor record派生、typed物品效果与profile模式、40字节资料缓冲双加载、内嵌bit5 flag、三类动作转场及非对称0/1返回。新增状态仅持有原物理短生命周期字段，其余复用既有唯一owner；startup reset清零新owner。组A帧两处旧整函数opaque调用均已改为typed直连，旧地址生产调用为零；待审资料加载保留窄port。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`187/422 = 178 platform_adapted + 9 assembly_exact + 235 pending_audit`，双跑SHA256为`e57b6e6a64cbc3b2353a7baa7d2f2537263a2e330aa4de26211644361f163c2d`。动态差分因原版actor、资料记录、callee与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=188`的`0x00470180`已关闭为`platform_adapted`。完整权威LST主体`0x00470180..0x004702D4`共191行、111条实际指令、3个call、25个跳转、16个标签、2个返回点且无外部chunk。实现三类category mask映射、两类type映射、typed索引提交、有序链表扫描、先增后比的occurrence语义、失败`0xFFFF`、资料加载、字符串复制、bit15/14/11顺序覆盖、type31最终强制1及陈旧profile index返回表读取。actor索引复用既有owner，新增唯一链表owner；待审资料加载为窄port。两个已关闭caller当前只有无地址语义行查询port且缺少节点物化owner，本包不复制第二份节点状态。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`188/422 = 179 platform_adapted + 9 assembly_exact + 234 pending_audit`，双跑SHA256为`c9defe880e71c231ce22f4b49c04fc9aadd1b35b9585cec25548647291e09f2c`。动态差分因原版链表、资料加载、返回表、字符串目标与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=189`的`0x004702E0`已关闭为`platform_adapted`。完整权威LST主体`0x004702E0..0x00470372`共94行、53条实际指令、1个call、17个跳转、8个标签、1个返回点且无外部chunk。复用第186/188项owner，实现相同category/type非对称筛选、全链扫描、调用者入口byte保留与逐匹配回绕递增；不清零、不提前停止、不加载资料，链尾EAX归零。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`189/422 = 180 platform_adapted + 9 assembly_exact + 233 pending_audit`，双跑SHA256为`bc99f85812f3e56476185df288b9e44dec03a7c3f3ac75936cd94862c8128449`。动态差分因原版链表、计数byte与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=190`的`0x00470380`已关闭为`platform_adapted`。完整权威LST主体`0x00470380..0x004705B7`共284行、164条实际指令、8个call、30个跳转、18个标签、4个返回点且无外部chunk。实现第N个27–30类节点筛选、bit15/14阈值发布、bit11资源链重建与资源选择、双索引提交、live signed容量比较、共享消息latch及消息/sample抑制顺序。主链、资源链、阈值和selected token扩展于第188项唯一owner；待审资源重建保持窄port。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`190/422 = 181 platform_adapted + 9 assembly_exact + 232 pending_audit`，双跑SHA256为`b470baa980d0fc1b5e3eb51745929f1e24f82750a59b551f46171a6174837bd7`。动态差分因原版两条链、live容量、消息/sample与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=191`的`0x004705C0`已关闭为`platform_adapted`。完整权威LST主体`0x004705C0..0x004707A9`共232行、144条实际指令、3个call、23个跳转、15个标签、3个返回点且无外部chunk。实现零occurrence无副作用早退、type mode flag、profile与16字节块清零、索引提交、第N节点筛选、资料加载、字符串复制、模式字段与三项派生word应用、copy latch及category mask返回规则。主链、final状态和物品效果均复用既有唯一owner；待审资料加载保持窄port。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`191/422 = 182 platform_adapted + 9 assembly_exact + 231 pending_audit`，双跑SHA256为`935fb2f925e84d3ec110a903f76519d1105ea2e46df0a9b71598c3cae630ab1c`。动态差分因原版链表、资料加载、字符串目标、模式字段与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=192`的`0x004707B0`已关闭为`platform_adapted`。完整权威LST主体`0x004707B0..0x0047081B`共54行、29条实际指令、3个call、1个跳转、1个标签、1个返回点且无外部chunk。实现局部记录构造、actor context解析、输出word、profile加载、profile buffer条件fallback写入及最终mode bit发布。profile buffer与mode byte复用既有唯一owner；三个待审callee保持窄port。typed-stop保留局部构造前缀。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`192/422 = 183 platform_adapted + 9 assembly_exact + 230 pending_audit`，双跑SHA256为`2d26ed039dc48da07c0929878f47fe1489c3539ee0fe943220454b5b5429cfbc`。动态差分因原版局部记录、三个callee、profile buffer与caller联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=193`的`0x00470820`已关闭为`platform_adapted`。完整权威LST主体`0x00470820..0x0047088C`共52行、29条实际指令、3个call、3个跳转、3个标签、2个返回点且无外部chunk。实现mode bit门控、primary容量16位回绕减法、signed负值夹零、selected resource双零参数释放、selected与required清零及统一刷新。组A帧唯一旧opaque caller已改为typed直连，startup reset清零链表owner；旧地址生产调用为零。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`193/422 = 184 platform_adapted + 9 assembly_exact + 229 pending_audit`，双跑SHA256为`36295238c0aedabbca297b2da0fdc67bbd65a102d8ec27e40eec44a434a4dcfb`。动态差分因原版actor记录、两个callee、selected resource与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=194`的`0x00470890`已关闭为`platform_adapted`。完整权威LST主体`0x00470890..0x004708BC`共23行、11条实际指令、0个call、2个跳转、1个标签、1个返回点且无外部chunk。实现secondary required零值早退、live容量16位回绕减法、signed负值夹零、字段清零及EAX/ECX/EDX寄存器结果。第193项两个caller均改为typed直连，旧刷新地址生产调用为零。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`194/422 = 185 platform_adapted + 9 assembly_exact + 228 pending_audit`，双跑SHA256为`3b36625fb2f7b531968a90e37018c19a31aa7e49dc8c3dc09bfcb200a9ecace6`。动态差分因原版actor记录、secondary required与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=195`的`0x004708C0`已关闭为`platform_adapted`。完整权威LST主体`0x004708C0..0x004708F7`共26行、14条实际指令、0个call、1个跳转、1个标签、1个返回点且无外部chunk。实现replacement读取、mode bit发布、completion latch、非零action kind复制及304字节工作区清零；复用final、item和workspace唯一owner。组A帧与目标选择刷新两个caller均改为typed直连，party span仅沿bindings传递引用，旧地址生产调用为零。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`195/422 = 186 platform_adapted + 9 assembly_exact + 227 pending_audit`，双跑SHA256为`bc01de2599a2a2d3436786582e0de1a79c26ab46281f05ba0b236f27b25e8ba6`。动态差分因原版actor四段状态、两个caller及寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=196`的`0x00470900`已关闭为`platform_adapted`。完整权威LST主体`0x00470900..0x0047090C`共8行、3条实际指令、0个call、0个跳转、0个标签、1个返回点且无外部chunk。实现next资源链头到current资源链头的完整dword提交，相等值与零值也写入；current/next扩展于第188项唯一链表owner。第190项bit11 caller已改为typed直连，待审caller留到所属工作包，旧地址生产调用为零。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`196/422 = 187 platform_adapted + 9 assembly_exact + 226 pending_audit`，双跑SHA256为`61edff9d9172acbf3fe675a6ca334657d402de59dffe303618b924e2f702b412`。动态差分因原版current/next链头与caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=197`的`0x00470910`已关闭为`platform_adapted`。完整权威LST主体`0x00470910..0x00470A05`共132行、86条实际指令、2个call、17个跳转、8个标签、2个返回点且无外部chunk。实现资源链头提交、破坏性前移、category/mode筛选、signed扫描派生、16位输出数量、bit15/14和字符串复制；保留计数后无条件比较导致的occurrence零陈旧成功。资源链与live容量复用唯一owner，权威LST无静态直接caller。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`197/422 = 188 platform_adapted + 9 assembly_exact + 225 pending_audit`，双跑SHA256为`c58f54c211943da1d68a40531cfd6f1749b5eac340567c397a7cdb2c13ff7191`。动态差分因原版资源链、live容量、字符串与寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=198`的`0x00470A10`已关闭为`platform_adapted`。完整权威LST主体`0x00470A10..0x00470ABB`共105行、60条实际指令、1个call、18个跳转、1个返回点且无外部chunk。实现六类category映射、保留输出初值、16位回绕计数、普通正值匹配和category 4额外bit独立递增。标准与alternate grid production链的计数、提交、查询均改为typed直连并复用party唯一owner；脚本化单测compat开关默认关闭。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`198/422 = 189 platform_adapted + 9 assembly_exact + 224 pending_audit`，双跑SHA256为`6c5586aaab691a5a1576cf43cdd9419129384afe85290c0d53509de99f714828`。动态差分因原版资源链、输出word和grid caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=199`的`0x00470AC0`已关闭为`platform_adapted`。完整权威LST主体`0x00470AC0..0x00470E15`共401行、244条实际指令、5个call、41个跳转、25个标签、4个返回点且无外部chunk。实现清零前缀、资源匹配、category 4双计数、三条profile加载路径、primary/secondary容量门、derived copy latch、runtime诊断、输出mode与数量递增/抑制。复用final、item、list、workspace、action和configuration唯一owner；第198项bit13重复byte owner已消除。目标选择动作提交动态category、message 27固定category4、message 30固定category5三处production caller均typed直连，脚本化compat默认关闭。相邻释放函数审计时按权威地址纠正了固定category两处先前误接的动作枚举槽。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`199/422 = 190 platform_adapted + 9 assembly_exact + 223 pending_audit`，双跑SHA256为`3e76f8dad71e3912b77eaca20b3db2c7f854c5ccab607ad9a4fb0a343a96b156`。动态差分因原版资源节点、profile加载、诊断、容量和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=200`的`0x00470E20`已关闭为`platform_adapted`。完整权威LST主体`0x00470E20..0x00470F65`共165行、100条实际指令、3个call、19个跳转、3个返回点且无外部chunk。实现哨兵链resource id定位、低byte门控递减、category bit7/27门、secondary/tertiary优先级、primary独立递减、selected清零、节点销毁、提交后循环次数回放重链，以及bit13返回抑制和AX半寄存器语义。复用list与workspace唯一owner。动作dispatch两处、目标选择三处和链表动作一处production caller均typed直连，脚本compat默认关闭。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`200/422 = 191 platform_adapted + 9 assembly_exact + 222 pending_audit`，双跑SHA256为`981dcf48b591865cb3d92a84f30a9eacc849fbcf677e7758c87e6a2de80ad2ad`。动态差分因原版资源节点、allocator和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=201`的`0x00470F70`已关闭为`platform_adapted`。完整权威LST主体`0x00470F70..0x00470FD5`共60行、36条实际指令、2个call、4个跳转、2个返回点且无外部chunk。实现bit13第N项扫描、occurrence零陈旧成功、破坏性current推进、名称复制和secondary/tertiary 16位回绕求和。复用resource list唯一owner。alternate grid production caller改为typed直连；20-byte caller缓冲区在原始复制边界typed-stop，脚本compat默认关闭。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`201/422 = 192 platform_adapted + 9 assembly_exact + 221 pending_audit`，双跑SHA256为`83d366d03421276e5ce1e0580e59cdce4a115028c0d4493d763a5d12f5afb40a`。动态差分因原版bit13资源链、名称缓冲区、数量word和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=202`的`0x00470FE0`已关闭为`platform_adapted`。完整权威LST主体`0x00470FE0..0x00471070`共87行、51条实际指令、2个call、9个跳转、7个局部标签、2个返回点且无外部chunk。实现mode零固定资源直达、mode非零bit27计数并排除固定id、occurrence零陈旧成功、selected token非对称写入、mode bits输出门、名称复制和secondary/tertiary 16位回绕求和。复用resource list唯一owner。mode grid production两处查询与两次已关闭链头提交改为typed直连，未审计secondary count保留窄port；20-byte caller缓冲区在原始复制边界typed-stop，脚本compat默认关闭。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`202/422 = 193 platform_adapted + 9 assembly_exact + 220 pending_audit`，双跑SHA256为`9c4944438acbe1b483f72b9df3a9383903af867abb8d37f9371d4b94c02604a5`。动态差分因原版mode资源链、selected token、caller局部和寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=203`的`0x00471080`已关闭为`platform_adapted`。完整权威LST主体`0x00471080..0x004710CD`共42行、25条实际指令、1个call、5个跳转、2个局部标签、1个返回点且无外部chunk。实现输出word先清零、破坏性资源链推进、bit27门、固定id排除、mode bits门，以及secondary/tertiary节点内和跨节点16位回绕累计。复用resource list唯一owner。mode grid production secondary-count caller改为typed直连，脚本compat默认关闭；随后的链头提交继续走typed路径。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`203/422 = 194 platform_adapted + 9 assembly_exact + 219 pending_audit`，双跑SHA256为`060968b2e3ee56d46b89501fbfa0bd0c0d15e1ff5d394634bd35b40b909b2f4d`。动态差分因原版mode资源链、caller输出word和寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=204`的`0x004710D0`已关闭为`platform_adapted`。完整权威LST主体`0x004710D0..0x0047126B`共171行、104条实际指令、7个call、3个跳转、3个局部标签、1个返回点且无外部chunk。实现group-B资源/坐标/解码窄callee序列、精确0x58字节演出记录、signed回绕坐标、资源高度分支、flags/mode bit、host surface重建，以及三段共236个尾部dword清零。每个group-A行动者新增唯一target-phase owner并复用既有particle emitter；隐藏this为group-A行动者、显式参数为group-B目标token。已关闭host-surface typed直连，其余未审callee保持窄port。action 6阶段零caller移除整函数opaque地址并直接调用typed实现。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`204/422 = 195 platform_adapted + 9 assembly_exact + 218 pending_audit`，双跑SHA256为`01134ec0bd9e21316e2cab6823607c18a4f1b8f2d2716028b6f0441ae1cec2e6`。动态差分因原版group-B资源对象、图像解码分配、坐标/属性callee和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=205`的`0x00471270`已关闭为`platform_adapted`。完整权威LST主体`0x00471270..0x004714AF`共239行、159条实际指令、7个call、7个跳转、6个局部标签、2个返回点且无外部chunk。实现16位tick前缀、particle emitter三值与active gate重写、已关闭image-particle frame typed直连、完成时资源释放和精确清零，以及remaining-batches非零时五档粒子槽调用。保留signed tick阈值，40以上每帧重复五档，回绕到负word后仅无条件第一档。target phase纠正为group-A行动者唯一owner并复用particle emitter；action 6公共推进点移除整函数opaque地址。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`205/422 = 196 platform_adapted + 9 assembly_exact + 217 pending_audit`，双跑SHA256为`874eb006550504b7b8ff044e090b29cbdd3fef4f47b5d871d133720ea842ea32`。动态差分因原版decoded buffer、粒子槽callee、CRT seed和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=206`的`0x004714B0`已关闭为`platform_adapted`。完整权威LST主体`0x004714B0..0x0047153D`共76行、51条实际指令、3个call、6个跳转、5个局部标签、5个返回点且无外部chunk。实现candidate零值不访问owner、固定首group-A actor记录level byte、32位差值，以及高于level、差值0、1..7、8..12、13以上五段返回规则。三个RNG分支固定bound100并保留35/70/90 inclusive阈值，确定分支不消费随机。group-A frame turn-resolution caller改为typed直连并纠正candidate零的旧opaque默认成功。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`206/422 = 197 platform_adapted + 9 assembly_exact + 216 pending_audit`，双跑SHA256为`e98195773f94491f0e042e58db5085c250a5e8682903fc68ab2fc9ea0e7a5e4b`。动态差分因原版first actor指针、secondary RNG状态和caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=207`的`0x00471540`已关闭为`platform_adapted`。完整权威LST主体`0x00471540..0x004717C8`共314行、191条实际指令、8个call、20个跳转、18个局部标签、5个返回点且无外部chunk。实现special-ready早退、模式零/一阈值二/六、signed倒计时递减与十五重置、152字节动作记录清零、模式一独占完成latch、动作记录更新、陈旧高半word帧键、bit0一次/两次翻转、sample声像陈旧寄存器、坐标正负十六偏移、帧源发布与最终软件绘制参数。每角色状态复用action-execution与progress唯一owner，共享帧源复用group-A action shared owner。两处group-A frame caller均typed直连并移除整函数地址。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`207/422 = 198 platform_adapted + 9 assembly_exact + 215 pending_audit`，双跑SHA256为`b3fa2ddef9b48fff1971a1aa58e912425f6f701c491dd14d4d70b520473c216c`。动态差分因原版角色动作记录、队列callee、帧记录、sample寄存器、坐标与软件绘制联合捕获后端缺失而为`blocked_runtime_oracle`。

`audit_order=208`的`0x004717D0`已关闭为`platform_adapted`。完整权威LST主体`0x004717D0..0x004717DD`共6行、4条实际指令、0个call、0个跳转、1个返回点且无外部chunk。实现角色`+0x2A86`低word bit13查询，ECX/EDX保持，空owner在原始word读取点typed-stop。字段归入既有每角色action-execution唯一owner。目标选择动作三十production caller改为typed直连，命中后发布动作十三、special gate和计数，不再调用旧opaque查询。验证：定向测试、AddressSanitizer、Linux core 188/188、Linux app 194/194全部通过且源码零warning。工作包为`208/422 = 199 platform_adapted + 9 assembly_exact + 214 pending_audit`，双跑SHA256为`9683f3206166bfd7a3823be063ba3df678a87b4e4e15a6d06b3a76e4fa304563`。动态差分因原版角色flags word与目标选择caller寄存器联合捕获后端缺失而为`blocked_runtime_oracle`。

下一项回收`audit_order=209`的`0x004717E0`战斗角色动作效果与渲染函数。

模块10只有在`422/422`均有实现映射、不可达证据或合规阻塞，完整战斗生命周期和I5通过后才能移交模块11。
