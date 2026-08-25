# 模块10：战斗状态机、AI与数值系统

状态：`module_in_progress`

当前关闭进度：`13/422`。现有资产读取与建场代码只是此前恢复的有限切片，不提前计入完整函数关闭。

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
- `pending_audit`：`409`；
- `assembly_exact`：`3`；
- `platform_adapted`：`10`；
- 已关闭：`13`。

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

它们覆盖了`0x0046E0B0`、`0x0045F130`、`0x0045F1B0`与`0x00451B10`的部分有效资产路径或部分指令区间，但尚未证明所属函数的完整LST函数体、全部外部chunk、全部错误/循环/异常域、caller回收和完整战斗生命周期。因此这些历史切片继续不计数；当前`13/422`只来自完成全部关闭门的TSW命令流像素命中查询、战斗绘制矩形边界放置、主行偏移表、surface行偏移表、宿主surface设置、战斗绘制宿主与方向表初始化、战斗绘制附属缓冲释放、战斗绘制资源整体清理、线段光栅、方向向量光栅单步推进、方向图块双层surface扫描、图像粒子生成与图像粒子整帧协调，不得把其他测试通过或真实battle 98样本当作函数关闭。

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

`audit_order=7`的`0x00433F00`已关闭为`platform_adapted`。它对附属缓冲空token直接返回；非空时以入口snapshot调用释放端口，回调期间owner仍保留旧token，只有回调返回后才清零。唯一caller尚未现代实现，关闭时必须直接组合该typed入口。

`audit_order=10`的`0x004342E0`已关闭为`assembly_exact`。它按left/top/width/height处理负起点缩短尺寸，右/下达到surface边界时反向移动起点而不裁短，最后按原顺序发布四条绝对边并让EAX携带bottom。三个caller均未现代实现，因此当前没有opaque callback可回收；关闭它们时必须直接调用typed接口。

`audit_order=5`的`0x00433E20`已关闭为`platform_adapted`。它先释放旧主行表，按行数乘4的32位回绕值申请新表；申请失败只清指针并保留旧元数据，成功后才发布步长和行数。正行数逐项写回绕偏移；申请大小回绕导致的越界只在原写入点停止并保留此前前缀。两个caller均未现代实现，当前没有opaque callback可回收。

`audit_order=6`的`0x00433E90`已关闭为`platform_adapted`。它独立释放并重建surface行表，成功后发布被绘制矩形直接消费的surface宽高；控制流与主行表同构但owner严格分离。分配失败和乘法回绕写入点保留相同前缀规则，主行表不受影响。三个caller均未现代实现，当前没有opaque callback可回收。

`audit_order=8`的`0x00433F30`已关闭为`assembly_exact`。它在行表callee前预发布宿主surface宽高，以保存的入参snapshot直接调用surface行表与全surface矩形typed入口；分配失败仍继续发布矩形，原行表越界写typed-stop则不伪造后续副作用。两个callee已直接回收，五个上层调用点尚未现代实现。

`audit_order=11`的`0x00434350`已关闭为`assembly_exact`。它按起终点回绕差值确定轴向符号，以严格半误差阈值推进X/Y主轴坐标，并在更新后判定双轴终点。零长度线段仍先把Y加一，`INT_MIN`取反与误差累加均保留32位异常域；函数无callee，九个caller均忽略其返回值。

`audit_order=12`的`0x00434420`已关闭为`platform_adapted`。它按记录索引读取战斗对象内两张360项方向表，保留符号、严格半误差阈值、零向量Y加一、`INT_MIN`与坐标/误差回绕；越界索引只在原首次水平表读取点停止且不修改记录。该叶子无callee，唯一caller的两个调用点都忽略指针残值。

`audit_order=13`的`0x004344E0`已关闭为`platform_adapted`。它直接回收两处方向向量推进与单像素通道合成callee，按signed固定点商组织反向Y外层和正向X内层扫描；保留三项共享发布、镜像源字节公式、目标边界先行、双透明色、直接/合成写入、除零和源/行表/目标原访问点typed-stop。唯一caller不消费EAX残值。

`audit_order=15`的`0x00434DD0`已关闭为`platform_adapted`。它用固定CRT序列从源图像随机选择非透明像素，建立56字节粒子节点，保存并清除原2×2块，再额外链接空后继；保留bit6/bit7选择优先级、bit0镜像不减一、bit7沿用入口次数与旧Y局部的BUG、零尾发布、批次回绕，以及全部除法、源读、节点写和两次分配失败前缀。唯一caller `0x00434790`现已直接回收typed入口。

`audit_order=14`的`0x00434790`已关闭为`platform_adapted`。它只在首次调用以显式time seed CRT、发布三项共享值并扫描源图，随后直接组合已关闭粒子生成、线段推进与单像素颜色合成；剩余批次回放保留镜像检查X、源索引和实际写入X错位，粒子2×2绘制保留只跳第一透明色、只检查右像素及合成模式右上先合成后被原值覆盖。生命刷新、距离与目标矩形摘除、唯一/首/尾/中间四类双向链释放及其计数不对称均已闭环。三个上层caller都显式消费返回1作为阶段完成信号，尚待各自进入现代实现。

下一项关闭`0x00433DC0`；先恢复渲染模块拥有的14行跨模块尺寸getter `0x00437E90` typed接口，再直接组合已关闭surface行表、矩形和主行表入口，保持宽度有符号除二及调用顺序。

模块10只有在`422/422`均有实现映射、不可达证据或合规阻塞，完整战斗生命周期和I5通过后才能移交模块11。
