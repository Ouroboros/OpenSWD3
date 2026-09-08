# OpenSWD3 执行 GOAL

版本：v898

最后更新：2026-09-08

当前阶段：B · 按模块逆向、实现与验证

当前步骤：模块10 · 工作包289 REVIEW 2实施

## 0. 执行约定

阶段提交、推送、工作区保护和提交标题统一遵循[`AGENTS.md`](../AGENTS.md)第11节；阶段性进度与TG汇报统一遵循第12节。本文件不复制这些长期规则，只保存项目目标、阶段定义、完成条件和当前执行队列。

## 1. 目标

以 `swd3.exe.lst` 的完整反汇编为唯一行为真值，用 C++20、CMake 和 SDL3 实现可独立运行的 OpenSWD3，并继续读取原始游戏资产和存档。

初步还原要求 bug-for-bug 的 1:1 行为兼容。游戏逻辑 BUG 不修复；只有阻断启动或新系统兼容的问题允许在平台层做隔离且可验证的最小修正。

本文件是后续工作的唯一执行顺序与阶段判定依据。`../analysis/plan.md` 只保留既有调研历史；各证据文档和机器目录提供技术事实，不另行定义竞争性的执行流程。

## 2. 固定技术决定

- 语言：C++20。
- 构建：CMake 命令行，默认 Ninja Multi-Config；不依赖 Visual Studio IDE。
- 编译器：MSVC 或 LLVM，工程文件不硬编码本机编译器路径。
- 平台边界：SDL3；业务核心不直接依赖 DirectDraw、DirectInput 或其他旧 Windows 图形输入接口。
- 音视频解码：统一放入项目自有`openswd3_ffmpeg`动态库；五个FFmpeg n9.0最小LGPL静态归档只在该平台库边界内链接，主程序只依赖OpenSWD3的stream/video ABI，不直接散布FFmpeg API。运行目录不再分发拆分FFmpeg DLL/SO；二进制发行版必须同时提供精确源码、非FFmpeg目标文件和重链接脚本组成的LGPL合规包。
- 诊断基础设施：在继续扩大业务模块前建立线程安全日志系统；每条日志至少含毫秒时间、级别、源文件与行号、消息，正常落盘，并在日志初始化失败时回退到控制台/调试输出。日志只观测行为，不改变原逻辑时序和返回合同。
- 文本内核：剧情脚本仍按汇编以原始字节和字节偏移解析，文本载荷在边界按 EXE 同目录 `openswd3.toml` 的 `[scripts].encoding` 解码；`big5` 对应 CP950、`gbk` 对应 CP936，缺省为 `big5`，不自动猜测。解码后的内核公共接口统一使用 `char16_t`/UTF-16，不使用平台宽度不同的 `wchar_t`。
- 显示尺寸：游戏内分辨率固定为 `640×480`；`[window].width/height` 只记录 SDL3 宿主的普通窗口尺寸，`maximized` 记录最大化状态，内容保持等比缩放，正常退出时保存窗口布局并在下次启动恢复。
- 汇编优先级：完整 LST 中的机器码字节与指令是唯一汇编主证据，高于 IDA 伪码、符号名、字符串解释和主观推断；ASM 不提供 LST 缺失的行为信息，不作为范围判定前置输入。
- IDA 辅助入口：可用 `D:\Dev\Crack\IDA\idat.exe` 以 headless 模式打开 `swd3.idb`，辅助提取函数边界、交叉引用、类型和控制流；该输出只用于导航与复核，发生歧义时仍以完整 `swd3.exe.lst` 的机器码字节与指令为唯一行为真值。
- 文件位置：所有新增文档、源码、测试和生成结果统一放在 `OpenSWD3/` 下分类保存；执行 GOAL 位于 `goal/`，逆向分析材料位于 `analysis/`。

### 当前事实基线

- 进程入口、消息泵、单帧调度、世界/特殊模式/战斗分支和退出顶层流程已有汇编证据。
- 十个既有子系统已达到顶层 ABI 覆盖，39 项关键 ABI 合同已经人工复核；这不等于内部业务逻辑全部恢复。
- 公共解压、主要资源容器、16 位软件像素规则、输入和时间的静态规格已经形成；唯一 glyph-mask 基准已在正确的 Windows 11 台湾繁体中文、CP950 与经典 `mingliu.ttc` 环境取得，正式跨平台 atlas 已对 157 个三字号 mask 逐字节零差异；此前错误字体环境的输出已删除。
- 剧情 VM 198个显式opcode、146个handler、17条runtime path及全部special/default/window/common路径均已完成实现和P3验收。
- B7世界地图已有限收口，B8剧情VM已完成P1–P3验收，B9特殊模式227/227已关闭；当前执行B10战斗函数审计，存档业务字段由B11最终验收。

## 3. 执行方法

先恢复全局程序架构，再按依赖顺序逐模块闭环。不会先把所有业务细节无限调研完，也不会在模块边界未知时猜测实现。

阶段 A 只恢复足以稳定模块分工的总体框架。阶段 A 完成后立即进入首个模块；不等待剧情、地图、战斗和存档全部逆向完成。

每个模块先做一次接口级逆向，达到“单模块开始条件”后便建立该模块实现。随后以一个函数、一个 handler、一条格式规则或必须共同验证的紧密耦合小组为单元，重复以下循环：

```text
一个可独立验证的行为单元逆向
→ 不看 C++，按 LST 建立汇编语义与控制流记录
→ 更新该模块当前规格
→ 从汇编条件独立推导分支与边界测试向量
→ C++20 实现
→ 汇编到 C++、C++ 到汇编双向逐基本块追溯
→ 修正实现、测试和文档；有差异则从入口重新验证
→ 真实资产、存档或固定状态验证
→ 可用时与原程序差分
→ 零差异、零未决后并入模块
```

每完成一轮逆向后，必须重新完整读取`AGENTS.md`和`APPEND_SYSTEM.md`，再开始下一轮。

每次提交完成后，必须重新完整读取`AGENTS.md`和本文件`goal/execution-plan-pi.md`，再开始任何后续工作。

### REVIEW 与提交粒度

REVIEW单元是最小提交粒度。每个工作包开始修改前必须先确定它包含一个还是多个REVIEW单元：默认一个工作包对应一个REVIEW单元；只有存在多个能够独立实现、测试、审查和回退的生产行为切片时才允许预先拆分。小工作包不得为追求小提交而人为拆分；已经确定的划分不得在执行过程中为了临时提交而缩小，划分发生实质变化时必须重新REVIEW。

每个REVIEW单元必须同时包含实际生产代码路径、对应caller接入、对应测试、对应证据更新、影响范围要求的验证，以及完整staged/unstaged差异审查。禁止把只有目标本体、单个helper、接口声明、部分未验证实现、单独测试、单独文档或WIP/checkpoint作为独立REVIEW单元提交。

REVIEW通过后必须立即按`AGENTS.md`完成commit、push和TG，再重新完整读取`AGENTS.md`和本文件；不得继续叠加下一REVIEW单元。REVIEW通过后若代码、测试、文档、暂存内容或工作区相关差异发生变化，原REVIEW立即失效，必须重新执行。

一个工作包可以由一个或多个REVIEW单元组成，但当前`audit_order`未关闭前不得开始下一工作包。中间REVIEW只更新对应证据，inventory TSV继续保持`pending_audit`，主PLAN不得前移，也不得宣称工作包完成。最终REVIEW必须完成该工作包作用域内全部caller回收、inventory TSV、模块文档、主PLAN同步及完整发布门禁，随后才能把工作包标记为关闭。

每个工作包开始前，必须先把完整REVIEW划分写入本文件唯一的“当前WORKPACK REVIEW计划”节；未写入时不得开始生产代码修改。执行中只允许在该节原位更新当前REVIEW状态，不得追加历史计划。当前工作包关闭后，开始下一工作包前必须用下一工作包的完整计划替换该节全部内容；不得在本文件保留、累积或追加已经完成的WORKPACK REVIEW计划。

每个还原函数必须采用“汇编—C++ 双向收敛验证”，核对次数不设上限，以结论收敛而非
完成固定次数作为停止条件。验证前先锁定 LST 地址范围、ABI、结构偏移和相关全局状态；
在不参考现有 C++ 的情况下，按基本块记录地址、输入、条件跳转、数据读写、调用顺序、
副作用及全部返回和异常出口。实现后先从汇编逐块映射到 C++，再从每项 C++ 行为反查到
具体汇编地址或已批准的平台兼容例外。测试向量只能从汇编比较、跳转和数据宽度独立推导，
必须覆盖跳转两侧及相等、零、正负、哨兵、截断、符号扩展和回绕边界；不得用现有实现的
假设反向构造测试数据。发现任何差异时，必须同步修正实现、测试、规格和证据，并从函数
入口重新执行完整双向验证，不能只复查差异附近。

只有同时满足以下条件，函数或 handler 才能标记为 `assembly_exact`：全部汇编基本块均有
实现映射、不可达证据或兼容例外；全部 C++ 可观察行为均可反查到汇编；条件方向、数据
宽度、符号与零扩展、位运算、整数回绕、调用和重复调用顺序、状态写入、副作用及出口均
无未解释差异；汇编独立推导的分支测试和适用的真实资产验证通过；最后一轮完整正向与
反向追溯不再产生新差异或未决项。UT 通过、文档自洽或固定次数复核均不能单独证明收敛。

同一时间只允许一个阶段或一个模块处于执行状态。新发现先归入对应模块的待确认项，不能自动扩展成新阶段，也不能使已经满足停止线的总体架构调研重新无限展开。

验证门禁分层如下：

- 单个函数、handler 或紧密耦合小工作包闭环时，执行定向测试和 Linux `core`/`app` 门禁；
- Windows LLVM `app` 不随每个函数、handler 或小工作包重复编译；
- Windows LLVM `app` 只在大阶段或模块正式关闭边界统一执行；每个边界都必须取得独立完整
  门禁结果，前一阶段的通过结果不得替代后一阶段；
- 大阶段 Windows 门禁发现的问题统一收集、统一修复，再重跑该阶段 Windows 门禁直至通过；
  未取得对应 Windows 通过证据时不得宣告该大阶段完成；
- 未到大阶段边界的阶段性汇报只报告本轮实际执行的 Linux/定向验证，不得暗示 Windows 已运行。

阶段提交、推送、工作区保护和提交标题遵循[`AGENTS.md`](../AGENTS.md)第11节，不在本文件重复定义。

## 4. 阶段 A：原程序架构恢复

本阶段不创建正式重写工程，不实现游戏逻辑。

### A1 · 顶层执行路径与模块骨架

- `[x]` 覆盖进程入口、初始化、消息泵、单帧主循环、普通世界、特殊模式、战斗、保存/读取和退出。
- `[x]` 把每条顶层路径落到明确模块。
- `[x]` 记录帧内调用顺序以及互斥、等待和提前返回关系。

### A2 · 函数归属

- `[x]` 为现有函数目录中的每个函数机械分配模块候选或 `unresolved`，本阶段不逐函数恢复语义。
- `[x]` 区分游戏自有函数、编译器/CRT 代码和第三方库边界；后两类只恢复游戏实际依赖的调用合同，不冒充自有模块实现范围。
- `[x]` 只人工复核 39 个已确认 ABI 合同、全部顶层直接调用和跨模块边界调用。
- `[x]` `unresolved` 项必须记录调用者、被调者和以后负责处理的模块；当前 `unresolved = 0`。

### A3 · 状态所有权

- `[x]` 为已有所有权目录中的状态和会跨模块传递的候选结构记录创建、读取、写入及销毁方；不在本阶段恢复每个内部字段。
- `[x]` 找出剧情、动作、世界、特殊模式、战斗和存档的交叉写入。
- `[x]` 区分状态所有者与临时借用者，避免新工程复制成无所有权的全局变量集合。

### A4 · 依赖、生命周期与平台边界

- `[x]` 建立模块依赖方向和循环依赖清单。
- `[x]` 恢复初始化、每帧更新、场景切换、战斗切换、失焦恢复和退出销毁顺序。
- `[x]` 把 Win32、DirectDraw、DirectInput、Miles、Bink 和 GDI 字形来源与业务核心分开。

### A5 · 原程序模块到重写模块的映射

- `[x]` 确定模块职责、输入输出和允许的依赖方向。
- `[x]` 确定首轮实现顺序和每个模块的验证入口。
- `[x]` 固定正式源码/测试目录、CMake 入口和首个模块采用的测试框架，避免实现时临时决定工程骨架。
- `[x]` 只固定接口和所有权，不提前设计不受汇编证据支持的复杂类层次。
- `[x]` 冻结首版架构基线；满足阶段 A 完成条件后，下一项必须是首个模块的接口级逆向与工程建立，不得转去全量深挖其他业务模块。

### 阶段 A 产物

- `../analysis/04-reverse-engineering/program-architecture.md`
- `../analysis/04-reverse-engineering/inventory/module-function-ownership.tsv`
- `../analysis/04-reverse-engineering/inventory/module-state-ownership.tsv`
- `../analysis/04-reverse-engineering/inventory/module-dependencies.tsv`

### 阶段 A 完成条件

- 顶层执行路径全部进入明确模块。
- 函数目录每一项都有机械模块候选或有后续处理归属的 `unresolved` 状态；无需在本阶段人工理解全部函数。
- 39 个已确认 ABI 合同、全部顶层直接调用和跨模块调用已经人工归属。
- 已有所有权目录中的状态和会影响模块接口的共享状态都有所有者；无需在本阶段恢复模块私有字段。
- 可以说明各模块的初始化、帧内执行、切换和销毁顺序。
- 平台替换边界与 1:1 业务核心已经分开。
- 没有仍会改变模块划分、主要依赖方向或首个模块公共接口的未决问题。

满足以上条件立即结束架构阶段。函数业务命名、模块私有结构、具体 opcode、地图字段、战斗算法和存档字段都不得成为延长 A 的理由。

## 5. 阶段 B：按模块逆向、实现与验证

阶段 A 已完成，首轮模块顺序固定如下：

1. 兼容基础、SDL3 平台生命周期与顶层帧调度。
2. 文件、内存、资源容器与公共解压。
3. 输入、时间、等待与随机数。
4. 软件渲染、文字、画面效果与最终呈现。
5. 音频与视频。
6. TSW/ACT/ANI/SND 资产运行时与公共动作记录。
7. 地图、世界、角色、碰撞与寻路。
8. 剧情 VM、场景调度与异步 action。
9. 菜单、商店和其他特殊模式。
10. 战斗状态机、AI 与数值系统。
11. 存档、配置与持久化语义。

存档物理容器可在资源模块实现；字段语义随剧情、世界和战斗状态逐项闭环，最后由持久化模块统一验收。

阶段 A 可以依据汇编证据合并、拆分或调整以上候选，但只能形成一套最终顺序，不保留并行方案，也不得新增新的全局调研阶段。

每个模块只维护一个工作包，至少包含：范围与非范围、汇编/数据证据、接口和状态所有权、当前实现单元、测试与差分点、未决项。算法细节写入模块规格，不写回本执行计划。

### 单模块开始条件

- 已列出对应汇编函数、handler、全局状态、资源和主要调用点；内部 helper 可以在模块实施中继续补充。
- 职责、输入输出、依赖方向、生命周期和状态所有者明确。
- 会影响公共接口的整数宽度、错误行为、顺序和兼容例外已有汇编证据。
- 在写实现前先列出 UT 边界向量、适用的真实数据或固定状态样本，以及差分捕获点。

满足以上条件便开始该模块的工程与首个实现单元，不等待模块全部内部行为逆向完毕。

### 单模块闭环与移交条件

- 范围内每个函数、handler 和格式字段都有实现映射、不可达证据或明确阻塞。
- UT 覆盖正常、边界、等待、错误路径、整数回绕和已知原始 BUG。
- 真实资产、存档或固定状态样本验证通过。
- 范围内实现均已完成汇编—C++ 双向收敛验证并达到 `assembly_exact`，不存在未解释映射或仅靠 UT 支撑的结论。
- 可运行的原程序差分通过；缺少运行后端时标记为 `blocked_runtime_oracle`，允许继续下一个模块，但不能宣称最终 1:1 差分完成。
- 没有擅自修复游戏逻辑、改变随机调用顺序或改变帧内时序。

全部条件满足时状态为 `module_closed`。如果唯一缺口是已登记的原程序运行后端，状态为 `module_closed_pending_oracle`，可以继续下一个模块；存在其他规格、实现或测试缺口时不得移交。

## 6. 集成里程碑

- `I1`：程序启动，建立 SDL3 窗口、原始逻辑时钟和空 framebuffer 呈现。
- `I2`：读取原始资源，解压并按原像素规则产生确定 framebuffer。
- `I3`：输入驱动角色在一张真实地图中移动、碰撞和寻路。
- `I4`：剧情 VM 驱动地图、对话、动作和音频，等待/让出时序一致。
- `I5`：从世界进入战斗，完成一次完整战斗并按原返回路径恢复。
- `I6`：读取旧存档、运行、重新保存，并完成原程序读取兼容验证。

每个里程碑至少保存固定输入、关键状态快照和 framebuffer 哈希；涉及随机行为时同时保存种子及调用序列。

## 7. 验证状态

每项行为可以同时具有以下证据状态，禁止用笼统的“已完成”代替验证等级：

- `assembly_exact`：已完成汇编—C++ 双向逐基本块追溯、汇编独立分支测试与零未决收敛；固定次数复核或 UT 通过不足以取得此状态。
- `asset_verified`：已用真实资产或存档验证。
- `original_diff_verified`：已与原程序输出或状态差分。
- `platform_adapted`：存在已记录的平台兼容隔离。
- `unreachable_current_assets`：当前资产不可达，但原分支仍保留。
- `blocked_runtime_oracle`：缺少原程序运行/捕获环境。
- `hypothesis_only`：尚不能作为实现依据。

`hypothesis_only` 不能与 `assembly_exact` 同时成立，也不能作为兼容核心的实现依据。UT 通过不能自动升级为 `original_diff_verified`。

## 8. 全项目完成条件

- 原程序自有且影响可观察行为的函数、指令、状态机和数据格式全部有实现映射。
- 原始资产和现有存档可以使用，不依赖原 EXE。
- 启动、世界、剧情、特殊模式、战斗、音视频、保存和退出路径全部可运行。
- MSVC 与 LLVM 的规定构建配置均通过构建和测试。
- 原始 BUG、整数行为、随机顺序、帧内顺序、像素结果和存档语义按规格保留。
- 所有平台兼容例外都有原行为、失败原因、最小改动和验证记录。
- 全部模块与集成里程碑完成；剩余阻塞必须由用户明确决定是否接受，不能自动视为完成。

## 9. 计划维护限制

本文件只允许进行四类正文修改：

1. 更新步骤状态；
2. 记录实际阻塞；
3. 根据新汇编证据修正模块边界或顺序；
4. 修正已经被证据证明错误的完成条件。

新想法、函数细节和研究日志不得继续追加到本文件；它们进入模块规格、证据文档或待确认清单。未经用户确认，不新增阶段，不扩大完成条件。

每次正文修改必须递增页首版本号；纯状态更新也属于正文修改。文档不维护冗长变更日志。

## 10. 当前唯一执行队列

1. `[x]` A1：恢复顶层执行路径和首版模块骨架。
2. `[x]` A2：函数归属。
3. `[x]` A3：状态所有权。
4. `[x]` A4：依赖、生命周期和平台边界。
5. `[x]` A5：原程序模块到重写模块映射，并冻结首轮模块顺序。
6. `[x]` B1：兼容基础、SDL3平台生命周期与顶层帧调度已关闭，仅保留登记的原程序动态差分阻塞；见[`runtime-platform.md`](../analysis/04-reverse-engineering/modules/runtime-platform.md)。
7. `[x]` B2：文件、内存、资源容器与公共解压已关闭，仅保留登记的原程序动态差分阻塞；见[`resource-io.md`](../analysis/04-reverse-engineering/modules/resource-io.md)。
8. `[x]` 日志基础设施：线程安全日志、文件输出和失败回退已经完成；详细完成记录见历史归档。
9. `[x]` B3：输入、时间、等待与随机数已关闭，仅保留登记的原程序动态差分阻塞；见[`input-time-rng.md`](../analysis/04-reverse-engineering/modules/input-time-rng.md)。
10. `[x]` B4：软件渲染、文字、画面效果与最终呈现已有限收口，剩余跨模块接线归B10；见[`rendering.md`](../analysis/04-reverse-engineering/modules/rendering.md)。
11. `[x]` B5：音频与视频及FFmpeg n9.0平台后端已关闭，仅保留原版Miles/Bink动态差分阻塞；见[`audio-video.md`](../analysis/04-reverse-engineering/modules/audio-video.md)。
12. `[x]` B6：TSW/ACT/ANI/SND资产运行时与公共动作记录已关闭，仅保留登记的原程序动态差分阻塞；见[`asset-runtime.md`](../analysis/04-reverse-engineering/modules/asset-runtime.md)。
13. `[x]` B7：地图、世界、角色、碰撞与寻路已按模块移交条件有限收口；当前状态、阻塞和证据见[`world-map.md`](../analysis/04-reverse-engineering/modules/world-map.md)及相关inventory/evidence。
14. `[x]` B8：剧情VM、场景调度与异步action的P1–P3已经完成；[`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)不再覆盖当前队列。
15. `[x]` B9：菜单、商店和其他特殊模式的227/227工作项已经关闭；当前状态和阻塞见[`special-modes.md`](../analysis/04-reverse-engineering/modules/special-modes.md)。
16. `[>]` B10：战斗状态机、AI与数值系统进行中；完整队列见[`battle-function-workpack.tsv`](../analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv)。当前已关闭至`audit_order=288`；当前执行`audit_order=289 / 0x00478620`。
17. `[ ]` B11：存档、配置与持久化语义；等待B10满足移交条件后开始。

B7以后已经完成的详细执行记录已机械搬到[`execution-progress-history-pi.md`](execution-progress-history-pi.md)。该文件只保存历史，不定义当前执行顺序、状态或断点。

当前只执行B10，不并行展开B11。

当前执行`audit_order=289 / 0x00478620`战斗角色帧资源准备函数；下一项为`audit_order=290 / 0x00478670`。

### B10 当前WORKPACK REVIEW计划

本节始终只保存当前工作包计划。REVIEW完成状态在本节原位更新；工作包关闭后，本节全部内容由下一工作包计划整体替换，不追加历史。

当前工作包：`audit_order=289`、`0x00478620`。目标是完整实现actor动作记录复制、动作更新、帧资源查询与frame-token发布的typed函数，并回收三个caller函数中的五个物理callsite。

当前断点：完整LST已锁定`0x00478620..0x0047866C`共77字节、29条指令、2个call、1个条件分支与2个普通`retn`，没有外部chunk或中段入口。两个callee依次为已关闭`0x004321E0`动作更新与`0x004315D0`帧查询；五个物理caller为`0x004605D9`、`0x004607F6`、`0x00460A0A`、`0x004710DF`与`0x0048402F`，当前`caller_reclaimed:3/5`。权威摘录为`build/workpack289/478620-full.lst`与`build/workpack289/478620-callers-context.lst`。

#### REVIEW 1：typed帧资源准备与frame-input三处caller

状态：已完成。

- 新增独立typed帧资源准备API。actor `+0x02A0`源动作记录与`+0x0CB8`发布动作记录作为18槽物理动作数组的第0与第17槽，归入现有`LegacyBattleGroupAActionExecutionState`唯一owner；Group-A继续由action dispatch持有，Group-B继续复用lifecycle `action_execution`，不新增平行actor或动作记录数组。
- 精确实现thiscall入口及`push ebx/esi/edi`，在生产DF=0合同下按38次`source read -> destination write -> ESI/EDI加4 -> ECX减1`复制完整`0x98`字节。不得用无序结构赋值隐藏访问顺序；source或destination fault保留此前复制前缀、当前REP寄存器和入口flags，destination fault不提交或推进当前dword。
- 复制完成后先把目标地址压栈并调用typed动作更新，再执行`add esp,4`与`test eax,eax`；完整EAX为0时早退，不读取资源/帧word、不调用帧查询、不写`actor+0x254C`。非零时`mov ax,[dst+0x4C]`只替换updater EAX低word，随后`mov cx,[dst+0x4A]`只替换updater ECX低word，EDX完整保留updater残值；按resource低word、frame低word调用typed帧查询。帧查询返回token无论为零或非零都在`add esp,8`后写入canonical `turn_frame_token`。
- 结果公开EAX/ECX/EDX/EBX/ESI/EDI/ESP、flags、38项复制计数、动作更新与帧查询结果、两次字段读取、frame-token提交和早退。provider入口必须锁定`EAX=(updater_EAX&0xFFFF0000)|field_4C`、`ECX=(updater_ECX&0xFFFF0000)|field_4A`及完整updater EDX；成功返回保留provider EAX/ECX/EDX，flags来自真实`add esp,8`地址算术；零返回flags来自`test eax,eax`；正常两出口恢复入口EBX/ESI/EDI并由`retn`消费返回地址。
- typed-stop覆盖三次callee-saved栈保存、动作更新参数push与CALL返回地址push、38个source读取、38个destination写入、两个目标word读取、两次帧查询参数push与CALL返回地址push、frame-token写入、三次栈恢复及两个出口的RET返回地址读取。每个停止点保留当时ESP、已压栈内容、已提交记录、动作更新副作用、callee返回寄存器、flags与REP残值；禁止在调用动作更新前snapshot目标记录，禁止把帧查询失败改成额外早退。
- 回收frame input `0x0045FC60`中的`0x004605D9/0x004607F6/0x00460A0A`。三处在已关闭`0x004784A0` snapshot后直接再次组合本typed函数，分别保持Group-B命中测试与两条Group-A命中路径的actor token、入口寄存器、栈位形、资源对象首dword/宽高读取、mirror、八乘八像素扫描和选择发布顺序。后续surface adapter只解析typed返回的frame token，不再以actor token替代资源结果。
- leaf typed-stop保留动作记录复制/更新与frame-token部分提交，并阻断资源对象解析、像素查询、目标可用标记和全部当前/剩余actor后缀。leaf正常返回零时仍按caller真实首个资源对象访问登记typed-stop，不伪造成“无surface”成功；非零对象首dword为零只走原caller普通未命中分支。
- 新增leaf测试矩阵覆盖38组source/destination fault、callee-saved push/pop、两组参数push、两组CALL返回地址push、两个RET读取、动作更新零/非零、两个word fault、provider零/非零、frame-token fault、两个返回出口、updater EAX/ECX高字与EDX、provider残值、ESP、ADD/TEST flags及复制前缀。两个word fault分别验证尚未执行的低word覆盖不发生。扩展frame-input测试覆盖三个物理站点、二次updater/provider调用、返回token解析、资源对象零值、leaf fault后缀抑制及生产`0x00478620`零opaque调用，使`caller_reclaimed`达到`3/5`。
- 同步新目标证据与frame-input证据。验收要求changed-range格式、定向测试、Linux core、ASan/UBSan、Linux app、连续十轮core、TMP分类及完整staged/unstaged审计全部通过且stderr无源码warning；inventory row 289继续保持`pending_audit`。本REVIEW独立commit、push、TG，可单独回退而不撤销工作包288或改动后两处caller。

#### REVIEW 2：Group-A目标演出初始化caller

状态：实施中。

- 回收`0x004710D0:0x004710DF`。`start_legacy_battle_target_phase`以显式Group-B目标token解析canonical action-execution view并直接组合typed帧资源准备；leaf正常返回后才把EAX发布到Group-A source actor的`phase.resource_token`，再按原顺序执行已关闭基准坐标查询、`0x58`字节演出记录清零、资源对象访问、解码、宽高发布、属性查询、host surface与尾部清零。
- 保留caller入口EAX/EDX与EBX/ESI/EDI、四次parent栈保存、typed leaf两出口、返回ECX/EDX及leaf最终flags。后续`0x00478470`入口EAX仍为Y输出地址、ECX为目标actor、EDX为leaf返回残值，flags改由leaf真实TEST或`add esp,8`结果传递，不再取generic reply。
- leaf typed-stop必须保留目标actor的动作记录复制、动作更新和可能已提交的`turn_frame_token`，但阻断source actor `phase.resource_token`、基准坐标、演出记录清零和全部阶段后缀。leaf正常返回零仍先发布零token并执行坐标与记录清零，直到caller真实资源对象读取点停止；provider返回零不得在leaf内提前停止。
- 删除`kCallTargetPhaseResource`的生产调用；保留后续尚未关闭的`0x004019A0`解码与`0x0047CE70`属性窄port。扩展target-phase与action-dispatch测试，覆盖Group-B目标owner、source/target双token提交、零返回顺序、REP中段fault、frame-token fault、base-coordinate入口寄存器/flags、演出后缀抑制和raw地址零调用，使`caller_reclaimed`达到`4/5`。
- 同步目标阶段与新目标证据。验收要求changed-range格式、定向测试、Linux core、ASan/UBSan、Linux app、连续十轮core、TMP分类及完整staged/unstaged审计全部通过且stderr无源码warning；inventory row 289继续保持`pending_audit`。本REVIEW独立commit、push、TG，可单独回退到REVIEW 1的`3/5`状态。

#### REVIEW 3：Group-B目标演出caller与工作包关闭

状态：待执行。

- 回收`0x00484020:0x0048402F`及外层`0x00455D60:0x00456458` action 6生产路径。`0x00456458`已锁定arg0为Group-A目标索引、arg4为`0x005029D0 + index*0x2F34`的显式Group-A目标token；隐藏this为Group-B source actor，禁止把arg4误识别为Group-B actor。
- 复用REVIEW 2的target-phase typed初始化，但owner按`Group-B source index × Group-A target index`选择原`source+0x0E6C+index*0x58`物理演出槽。扩展现有`group_b_target_phases`为明确的每目标canonical槽并迁移其既有借用者，不增加第二套Group-B phase数组；source actor字段与mode byte继续来自同一lifecycle element，target帧准备与坐标查询来自显式Group-A action/startup owner。
- 保留`sub_484020`的parent栈位形与caller residue：typed leaf更新Group-A目标的`+0x0CB8/+0x254C`，正常返回后才把token写入Group-B source的目标phase `+0x255C`语义槽；随后按目标索引清零对应`0x58`记录并执行既有解码、属性、host-surface和尾部初始化。不得把Group-A目标记录、Group-B source phase和相邻目标槽合并。
- opponent action 6直接调用typed目标阶段初始化，删除`kCallPrepareTargetPhase=0x00484020`生产调用；`0x004841B0`完成阶段仍作为后续工作包的窄port。任一leaf或parent typed-stop保留当前目标actor与source phase已提交前缀，并阻断set-target-mode、clear-mode、attack-order移除、暗化、刷新、phase/input word和完成阶段调用。
- 扩展target-phase、opponent-dispatch与父级测试，覆盖至少两个Group-B source及两个Group-A目标索引、相邻phase隔离、显式Group-A token、source/target双提交、leaf复制中段fault、零token资源读取停止、parent后缀抑制、`0x00478620/0x00484020`生产零调用及`0x004841B0`保留调用，使`caller_reclaimed`达到`5/5`。`audit_order=419 / 0x00484020`仍保持自身`pending_audit`，不得由本工作包越权关闭。
- 完成新目标证据、三个caller证据、`modules/battle.md`、`analysis/tools/build_battle_workpack.py`关闭映射、inventory TSV与主PLAN同步。最终执行战斗定向测试、AddressSanitizer、Linux core、Linux app、changed-range格式、零源码warning、连续十次core、inventory双次稳定生成、TMP分类及完整staged/unstaged发布审计；row 289仅由权威生成器更新为`platform_adapted`。
- 原版动态差分若仍缺少完整Group-A/Group-B actor动作数组、异常栈与source/destination/resource内存页、动作更新和帧查询寄存器/flags、五处caller联合SEH捕获后端，则登记为`blocked_runtime_oracle`。本REVIEW独立commit、push、TG，可单独回退到REVIEW 2的`4/5`状态；通过后关闭row 289，重读规定文件，再切换工作包290。
