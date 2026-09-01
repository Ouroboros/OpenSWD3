# OpenSWD3 执行路线图

最后更新：2026-09-01

本文件只在阶段、模块或全项目完成判定时读取。当前工作包见[`execution-state-pi.md`](execution-state-pi.md)，稳定执行规则见[`execution-rules-pi.md`](execution-rules-pi.md)。

## 1. 总目标

以`swd3.exe.lst`完整反汇编为唯一原程序行为真值，用C++20、CMake和SDL3实现可独立运行的OpenSWD3，并继续读取原始游戏资产和存档。初步还原保持bug-for-bug可观察兼容；游戏逻辑BUG不修复，仅允许平台层可验证的最小隔离。

## 2. 阶段A：原程序架构恢复

状态：已完成。

已完成范围：

- A1：顶层入口、初始化、消息泵、单帧、普通世界、特殊模式、战斗、保存/读取和退出路径进入明确模块。
- A2：函数归属、CRT/第三方边界、39个关键ABI及跨模块调用归属。
- A3：共享状态owner、跨模块读写和销毁方。
- A4：模块依赖、生命周期、场景/战斗切换、失焦恢复及平台边界。
- A5：正式模块顺序、源码/测试/CMake骨架和首版架构基线。

权威产物：

- [`../analysis/04-reverse-engineering/program-architecture.md`](../analysis/04-reverse-engineering/program-architecture.md)
- [`../analysis/04-reverse-engineering/inventory/module-function-ownership.tsv`](../analysis/04-reverse-engineering/inventory/module-function-ownership.tsv)
- [`../analysis/04-reverse-engineering/inventory/module-state-ownership.tsv`](../analysis/04-reverse-engineering/inventory/module-state-ownership.tsv)
- [`../analysis/04-reverse-engineering/inventory/module-dependencies.tsv`](../analysis/04-reverse-engineering/inventory/module-dependencies.tsv)

阶段A不得因函数命名、模块私有结构、具体opcode、地图字段、战斗算法或存档字段重新开启。

## 3. 阶段B：按模块逆向、实现与验证

唯一模块顺序：

| 顺序 | 模块 | 当前状态 | 日常入口 |
|---:|---|---|---|
| 1 | 兼容基础、SDL3生命周期与顶层帧调度 | 已关闭；保留已登记动态oracle | `analysis/04-reverse-engineering/`对应架构与证据 |
| 2 | 文件、内存、资源容器与公共解压 | 已关闭；保留已登记动态oracle | [`../analysis/04-reverse-engineering/modules/asset-runtime.md`](../analysis/04-reverse-engineering/modules/asset-runtime.md) |
| 3 | 输入、时间、等待与随机数 | 已关闭；保留已登记动态oracle | 对应evidence与inventory |
| 4 | 软件渲染、文字、画面效果与最终呈现 | 已关闭；保留framebuffer/text等动态oracle | [`../analysis/04-reverse-engineering/modules/rendering.md`](../analysis/04-reverse-engineering/modules/rendering.md) |
| 5 | 音频与视频 | 已关闭；FFmpeg n9.0合规边界已落地，Miles/Bink差分保留 | [`../analysis/04-reverse-engineering/modules/audio-video.md`](../analysis/04-reverse-engineering/modules/audio-video.md) |
| 6 | TSW/ACT/ANI/SND资产运行时与公共动作记录 | 已关闭；保留已登记动态oracle | [`../analysis/04-reverse-engineering/modules/asset-runtime.md`](../analysis/04-reverse-engineering/modules/asset-runtime.md) |
| 7 | 地图、世界、角色、碰撞与寻路 | 已关闭；B7工作包完成，保留动态oracle | [`../analysis/04-reverse-engineering/modules/world-map.md`](../analysis/04-reverse-engineering/modules/world-map.md) |
| 8 | 剧情VM、场景调度与异步action | P1–P3完成；追加计划停止覆盖 | [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)及story VM inventories |
| 9 | 菜单、商店和其他特殊模式 | `227/227`关闭；保留动态oracle | [`../analysis/04-reverse-engineering/modules/special-modes.md`](../analysis/04-reverse-engineering/modules/special-modes.md) |
| 10 | 战斗状态机、AI与数值系统 | 执行中；第270项最终收尾 | [`../analysis/04-reverse-engineering/modules/battle.md`](../analysis/04-reverse-engineering/modules/battle.md)与battle inventory |
| 11 | 存档、配置与持久化语义 | 未开始正式模块闭合 | 物理容器可复用模块2，字段语义待最终统一验收 |

存档物理容器可先在资源模块实现；字段语义随剧情、世界和战斗逐项闭合，最后由模块11统一验收。不得保留并行模块顺序或新增无证据的全局调研阶段。

## 4. 模块实施边界

每个模块只维护一个机械工作包inventory和一个模块摘要；函数算法、调用方、typed-stop与验证细节进入单项evidence。开始条件、工作包闭环方法和关闭条件统一见[`execution-rules-pi.md`](execution-rules-pi.md)，不得在本路线图复制逐项流水。

模块只有在范围内每项均有实现映射、不可达证据或合规阻塞，全部规定门禁通过，且不存在非oracle规格/实现/测试缺口时才能移交。唯一缺口为已登记原程序运行后端时可标记`module_closed_pending_oracle`。

## 5. 集成里程碑

- `I1`：程序启动，建立SDL3窗口、原始逻辑时钟和空framebuffer呈现。
- `I2`：读取原始资源，解压并按原像素规则产生确定framebuffer。
- `I3`：输入驱动角色在一张真实地图中移动、碰撞和寻路。
- `I4`：剧情VM驱动地图、对话、动作和音频，等待/让出时序一致。
- `I5`：从世界进入战斗，完成一次完整战斗并按原返回路径恢复。
- `I6`：读取旧存档、运行、重新保存，并完成原程序读取兼容验证。

每个里程碑至少保存固定输入、关键状态快照和framebuffer哈希；涉及随机行为时同时保存种子及调用序列。模块完成不能自动替代集成里程碑证据。

## 6. 大阶段门禁

- 单函数、handler或紧密小工作包：定向测试、Linux core、AddressSanitizer、Linux app及release审计。
- Windows LLVM app只在大阶段或正式模块关闭边界统一执行；前一阶段结果不得替代后一阶段。
- Windows门禁发现的问题集中修复并重跑直至通过；无对应证据不得宣告大阶段完成。
- 剧情VM P0、P1、P2、P3各自是独立大阶段；现均已完成并停止覆盖当前模块队列。

实际命令、仓库卫生和汇报格式见[`execution-rules-pi.md`](execution-rules-pi.md)。

## 7. 全项目完成条件

只有同时满足以下条件才能完成本GOAL：

- 原程序自有且影响可观察行为的函数、指令、状态机和数据格式全部有实现映射。
- 原始资产和现有存档可用，不依赖原EXE。
- 启动、世界、剧情、特殊模式、战斗、音视频、保存和退出路径全部可运行。
- MSVC与LLVM的规定构建配置均通过构建和测试。
- 原始BUG、整数行为、随机顺序、帧内顺序、像素结果和存档语义按规格保留。
- 所有平台兼容例外都有原行为、失败原因、最小改动和验证记录。
- 全部模块与I1–I6完成。
- 剩余阻塞由用户明确决定是否接受；不得自动把`blocked_runtime_oracle`视为最终完成。

当前模块10和模块11未完成，因此本GOAL尚未达到完成条件。
