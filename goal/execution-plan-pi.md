# OpenSWD3 执行 GOAL

版本：v854

最后更新：2026-09-01

当前阶段：B · 按模块逆向、实现与验证

当前步骤：模块10 · 完成`audit_order=270`（`0x00477920`）最终门禁与收尾

## 1. 文档职责

本文件只作为执行入口和索引，不承载函数语义、逐工作包完成流水、测试明细、SHA256清单或历史复盘。

执行时先读取当前状态，再按任务类型加载规则、路线图和对应模块资料。不得把证据文档、inventory、模块规格或历史流水重新复制回本文件。

## 2. 最小读取顺序

1. 每次恢复工作先读[`execution-state-pi.md`](execution-state-pi.md)，确认唯一当前步骤、已通过门禁、剩余动作和阻塞。
2. 新会话首次执行、规则不明确或准备提交/发布时读[`execution-rules-pi.md`](execution-rules-pi.md)。
3. 只读取当前模块的模块文档、inventory和当前工作包证据；模块10入口见下方索引。
4. 只有切换模块、阶段或判断全项目完成条件时才读[`execution-roadmap-pi.md`](execution-roadmap-pi.md)。
5. 只有追溯旧决定、历史验证或迁移来源时才读[`execution-history-index-pi.md`](execution-history-index-pi.md)及相关冻结分片；正常执行不得联合加载全部历史分片。

## 3. 权威顺序

1. [`../../swd3.exe_export_for_ai/swd3.exe.lst`](../../swd3.exe_export_for_ai/swd3.exe.lst)：原程序行为唯一真值，完整主体和全部外部`FUNCTION CHUNK`都必须审计。
2. 当前工作树、构建和测试输出：OpenSWD3实现与验证现状。
3. `../analysis/04-reverse-engineering/inventory/`：机械范围、顺序和闭合状态。
4. `../analysis/04-reverse-engineering/evidence/`：单函数、格式和调用方的技术结论。
5. `../analysis/04-reverse-engineering/modules/`：模块职责、集成边界和阶段摘要。
6. `goal/history/`：仅作历史追溯，不得覆盖以上当前事实。

发生冲突时按以上顺序处理，并把修正写回真正负责该事实的文档，不在本入口保留竞争性结论。

## 4. 当前执行入口

- 唯一当前状态：[`execution-state-pi.md`](execution-state-pi.md)
- 当前模块：[`../analysis/04-reverse-engineering/modules/battle.md`](../analysis/04-reverse-engineering/modules/battle.md)
- 当前inventory：[`../analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv`](../analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv)
- 当前工作包证据：[`../analysis/04-reverse-engineering/evidence/battle-fixed-curve-set-00477920.md`](../analysis/04-reverse-engineering/evidence/battle-fixed-curve-set-00477920.md)
- inventory生成器：[`../analysis/tools/build_battle_workpack.py`](../analysis/tools/build_battle_workpack.py)

当前工作包完成后，下一候选由inventory的`audit_order`唯一确定；不得从冻结历史中的旧“下一项”文字取值。

## 5. 计划文档索引

| 文档 | 何时读取 | 承载内容 |
|---|---|---|
| [`execution-state-pi.md`](execution-state-pi.md) | 每次恢复工作 | 唯一当前步骤、即时门禁、下一动作、当前阻塞 |
| [`execution-rules-pi.md`](execution-rules-pi.md) | 新会话、规则疑义、提交/发布前 | 行为真值、兼容原则、工作包方法、验证、提交、TG与仓库卫生 |
| [`execution-roadmap-pi.md`](execution-roadmap-pi.md) | 阶段或模块切换 | 阶段、模块顺序、里程碑、验证状态与全项目完成条件 |
| [`execution-history-index-pi.md`](execution-history-index-pi.md) | 历史追溯 | 冻结旧计划、拆分历史流水及现行模块资料索引 |
| [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md) | 仅追溯剧情VM P1–P3 | 已完成的剧情VM追加计划，不再覆盖当前队列 |

`execution-plan.md`和`story-vm-closure-plan.md`是非Pi历史变体，不是本GOAL的当前执行入口。

## 6. 维护规则

本文件只允许修改：

1. 版本、日期、当前阶段和当前步骤指针；
2. 文档层级或索引路径；
3. 因仓库结构变化而必须修正的权威顺序；
4. 已被证据证明错误的入口说明。

逐工作包语义进入`evidence/`；机械状态进入inventory；模块进度进入对应`modules/*.md`；即时执行状态覆盖写入`execution-state-pi.md`；稳定规则进入`execution-rules-pi.md`；阶段与完成条件进入`execution-roadmap-pi.md`；历史只写入冻结历史文件。

禁止在本文件追加“本轮完成”、逐地址摘要、测试日志、commit/push/TG流水、workpack哈希或大段阻塞列表。当前状态必须覆盖更新，不建立累积日志。
