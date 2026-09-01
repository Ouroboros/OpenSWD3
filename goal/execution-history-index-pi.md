# OpenSWD3 执行历史索引

最后更新：2026-09-01

本文件用于按需追溯旧执行记录。正常工作先读[`execution-state-pi.md`](execution-state-pi.md)，不要把历史流水作为当前状态、工作顺序或行为真值。

## 1. 零损失冻结分片

重构前`execution-plan-pi.md` v853全文已拆为十五个互不重叠的连续分片。去掉每份文件新增的说明头后，按下表顺序拼接正文即可逐字节重建原文件：

- 原始大小：790597字节
- 原始行数：4337
- 原始SHA256：`38f84552d86422da0a8f7e11242d075aa198ed2cb431566a88cb684396290d84`

仓库不再额外保留一份79万字节完整副本，因此既保留全部有效信息、重复段落、旧状态、旧测试计数和历史“下一项”文字，也避免整档与切片双重重复。冻结分片不得覆盖当前inventory、evidence、代码、测试或[`execution-state-pi.md`](execution-state-pi.md)。

## 2. 按范围拆分的冻结历史

| 文档 | 原v853行范围 | 内容 |
|---|---:|---|
| [`history/execution-plan-v853-foundation-pi.md`](history/execution-plan-v853-foundation-pi.md) | 1–306 | 旧入口、规则、阶段、里程碑和完成条件 |
| [`history/execution-progress-baseline-pi.md`](history/execution-progress-baseline-pi.md) | 307–630 | A/B阶段清单与模块基线 |
| [`history/execution-progress-world-pi.md`](history/execution-progress-world-pi.md) | 631–1368 | 世界/B7及剧情前置流水 |
| [`history/execution-progress-story-vm-01-pi.md`](history/execution-progress-story-vm-01-pi.md) | 1369–1900 | 剧情VM P1/P2前段 |
| [`history/execution-progress-story-vm-02-pi.md`](history/execution-progress-story-vm-02-pi.md) | 1901–2450 | 剧情VM P2中段 |
| [`history/execution-progress-story-vm-03-pi.md`](history/execution-progress-story-vm-03-pi.md) | 2451–2985 | 剧情VM P2后段与P3验收 |
| [`history/execution-progress-media-transition-pi.md`](history/execution-progress-media-transition-pi.md) | 2986–3072 | FFmpeg、媒体修正和模块9过渡 |
| [`history/execution-progress-special-modes-01-pi.md`](history/execution-progress-special-modes-01-pi.md) | 3073–3650 | 模块9前段工作包 |
| [`history/execution-progress-special-modes-02-pi.md`](history/execution-progress-special-modes-02-pi.md) | 3651–3848 | 模块9后段工作包与关闭 |
| [`history/execution-progress-battle-early-01-pi.md`](history/execution-progress-battle-early-01-pi.md) | 3849–3900 | 媒体最终化及模块10早期工作包 |
| [`history/execution-progress-battle-early-02-pi.md`](history/execution-progress-battle-early-02-pi.md) | 3901–3971 | 模块10至`audit_order=87` |
| [`history/execution-progress-battle-088-118-pi.md`](history/execution-progress-battle-088-118-pi.md) | 3972–4033 | `audit_order=88..118` |
| [`history/execution-progress-battle-119-149-pi.md`](history/execution-progress-battle-119-149-pi.md) | 4034–4095 | `audit_order=119..149` |
| [`history/execution-progress-battle-150-209-pi.md`](history/execution-progress-battle-150-209-pi.md) | 4096–4215 | `audit_order=150..209` |
| [`history/execution-progress-battle-210-269-pi.md`](history/execution-progress-battle-210-269-pi.md) | 4216–4337 | `audit_order=210..269`及旧下一项 |

每份分片在说明头之后保留对应连续原文，不重写技术结论。只追溯单模块时读取对应分片；只有验证历史迁移完整性时才按顺序联合读取全部正文。

## 3. 当前技术资料索引

历史流水中的详细技术数据，当前应优先从以下去重后的权威资料读取：

| 主题 | 当前入口 |
|---|---|
| 架构、模块归属和依赖 | `analysis/04-reverse-engineering/program-architecture.md`及`inventory/module-*.tsv` |
| 世界/B7 | [`../analysis/04-reverse-engineering/modules/world-map.md`](../analysis/04-reverse-engineering/modules/world-map.md)及world-map inventories/evidence |
| 剧情VM P1–P3 | [`story-vm-closure-plan-pi.md`](story-vm-closure-plan-pi.md)、story VM inventories和opcode/handler evidence |
| FFmpeg n9.0 | [`../analysis/04-reverse-engineering/evidence/ffmpeg-media-backend-9.0.md`](../analysis/04-reverse-engineering/evidence/ffmpeg-media-backend-9.0.md)及`dependencies/ffmpeg/9.0/` |
| 模块9特殊模式 | [`../analysis/04-reverse-engineering/modules/special-modes.md`](../analysis/04-reverse-engineering/modules/special-modes.md)及special-modes inventory/evidence |
| 模块10战斗 | [`../analysis/04-reverse-engineering/modules/battle.md`](../analysis/04-reverse-engineering/modules/battle.md)、battle inventory及单函数evidence |
| 即时工作状态 | [`execution-state-pi.md`](execution-state-pi.md) |
| 稳定执行合同 | [`execution-rules-pi.md`](execution-rules-pi.md) |
| 阶段与完成条件 | [`execution-roadmap-pi.md`](execution-roadmap-pi.md) |

## 4. 使用与维护规则

- 历史文件只读冻结，不追加新工作包。
- 新函数语义写入evidence；模块摘要写入对应module；机械闭合状态写入inventory；即时指针覆盖写入state。
- 冻结历史与当前资料冲突时，优先级为LST、当前代码/测试、inventory、evidence、module、历史。
- 若必须修正历史迁移错误，不直接改冻结分片；在本索引记录来源行、错误和当前权威资料。
- 以后不再生成新的累积“本轮完成”日志文档；Git历史、提交、evidence和inventory共同承担可追溯性。
