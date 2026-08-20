# 剧情 VM 完整闭环追加 PLAN

状态：执行中，P0/P1 已完成，当前步骤 P2；当前 handler `0x0042845A`（opcode 18）

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
- 当前 C++ 接入 61 个显式 opcode。
- 当前资产静态控制流观察到 143 个 opcode，其中仍有 83 个尚未实现。
- 另有 55 个 opcode 未在当前资产静态控制流中观察到；未观察不等于不可达或可以删除。
- `0..124` 已有人工汇编语义；`125..193` 目前只有分派、长度和保守 CFG，尚不能直接翻译为 C++。
- 当前已实现的 61 个 opcode 不继承完成状态，必须随所属 handler 组重新审计和验证。

## 3. 固定决策

剧情 VM 的完整范围一次锁定为 `sub_427920` 的全部 198 个显式 opcode、默认非法分支、
表外特殊值、窗口切换和公共解释循环。停止按单条剧情运行轨迹“遇到一个补一个”。

实现和验证单位是 146 个唯一汇编 handler 入口及其共享 opcode 组，不是孤立 opcode，
也不是机械编号段。每处理一个入口，必须把该入口下已经实现和尚未实现的全部 opcode
变体一起逆向、实现和验证。

“完整范围一次锁定”不等于连续写完 198 个 opcode 后统一测试，也不等于先把全部语义
研究完再开始编码。执行方式固定为按 handler 组边逆向、边实现、边验证，当前组完全
收敛后再进入下一组。

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
行全部从 `pending_audit` 开始，25 个共享入口、50 个现代 case label、125 行旧语义、
143/55 资产观察及候选端口仅作导航；`story-vm-runtime-paths.tsv` 另锁定 17 条默认、特殊
值、窗口、公共 join/yield 与返回路径。P2 前十三行已独立关闭：默认非法与共享对话两组、
opcode7/9 的 bit31/bit30 clear、opcode8 lifetime、opcode10/11 action、opcode12 position、
opcode13 role step、opcode14 action wait、opcode15 same-file jump、opcode16 unprepared role-path
jump 与 opcode17 prepared role-path jump。当前为 13/146；下一行只审计 `0x0042845A` 的
opcode18。

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
