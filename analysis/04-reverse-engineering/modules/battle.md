# 模块10：战斗状态机、AI与数值系统

状态：`module_started_scope_locked`

当前关闭进度：`0/422`。现有资产读取与建场代码只是此前恢复的有限切片，不提前计入完整函数关闭。

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
- `pending_audit`：`422`；
- 已关闭：`0`。

六个稳定导航分组为：

- `transferred_action_and_asset_helpers`：`15`项，范围`0x00433AA0..0x00434DD0`；
- `battle_record_leaves`：`2`项，范围`0x0044FFC0..0x0044FFE0`；
- `setup_frame_input_and_resolution`：`93`项，范围`0x00450270..0x0045FC60`；
- `script_dispatch_ai_and_targeting`：`77`项，范围`0x00460C40..0x0046FFF0`；
- `actor_actions_effects_and_rendering`：`194`项，范围`0x00470180..0x0047FC40`；
- `shared_battle_object_services`：`41`项，范围`0x004800F0..0x00484500`。

`audit_order`只是稳定的地址顺序，不证明函数语义，也不强制违背callee优先的实现顺序。每次关闭函数后必须立即回收已关闭callee在caller中的opaque边界。

生成器固定断言候选数、首尾地址、地址唯一性、六组计数、置信度计数、关闭状态集合和证据非空规则。连续两次生成的工作包必须逐字节一致。

## 3. 现有切片不提前计数

当前`src/battle/`只有两类历史切片：

- `legacy_battle_assets`：FIGTALK固定窗口和`battle.ffd`头、索引、记录读取；
- `legacy_battle_setup`：初始队伍筛选、固定阵型、镜像坐标和敌方记录布局。

它们覆盖了`0x0046E0B0`、`0x0045F130`、`0x0045F1B0`与`0x00451B10`的部分有效资产路径或部分指令区间，但尚未证明所属函数的完整LST函数体、全部外部chunk、全部错误/循环/异常域、caller回收和完整战斗生命周期。因此工作包保持`0/422`，不得把测试通过或真实battle 98样本当作函数关闭。

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
- `closure 0/422`；
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

当前阶段只完成机械范围与模块开始条件，不关闭任何战斗函数。范围提交后从`audit_order=1`开始完整审计`0x00433AA0`，并按callee优先原则处理其后续紧密耦合小组。

模块10只有在`422/422`均有实现映射、不可达证据或合规阻塞，完整战斗生命周期和I5通过后才能移交模块11。
