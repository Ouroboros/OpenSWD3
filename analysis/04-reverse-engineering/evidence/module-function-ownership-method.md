# A2 函数模块归属方法与当前审计

状态：A2 机械全量归属与跨模块边界复核完成

## 1. 产物

- [`module-function-ownership.tsv`](../inventory/module-function-ownership.tsv)：每个已知函数入口的来源、模块候选、证据、完整汇编直调者/被调者和复核状态。
- [`module-cross-calls.tsv`](../inventory/module-cross-calls.tsv)：模块候选不同或跨外部边界的每个唯一 caller/callee 对及全部汇编调用点。
- [`module-boundary-functions.tsv`](../inventory/module-boundary-functions.tsv)：按实际被调入口汇总 caller 模块、调用点、ABI 提示和复核状态。
- [`build_module_function_ownership.py`](../../tools/build_module_function_ownership.py)：锁定输入哈希、已复核游戏边界摘要并重新生成三张表。

## 2. 覆盖口径

基表不仅包含 IDA 导出目录的 1,486 个地址，还包含完整汇编中已有 `PROC`、但导出目录漏掉的 24 个入口，共 1,510 行。额外入口包括 16 个软件 blitter 入口和 8 个菜单函数槽目标；它们本来就存在于汇编，不是新增研究范围。

当前覆盖断言：

| 项目 | 数量 |
|---|---:|
| IDA 导出目录 | 1,486 |
| 汇编额外 `PROC` | 24 |
| 总归属行 | 1,510 |
| `game` | 1,191 |
| `crt`/编译器代码 | 312 |
| DirectX/系统链接桩 | 7 |
| 无模块候选的 `unresolved` | 0 |
| 39 项关键 ABI 在表内 | 39 |
| 59 个顶层去重直接目标在表内 | 59 |

`unresolved = 0` 只表示每项都有机械候选，不表示 1,191 个游戏函数都已人工恢复语义。

## 3. 证据优先级

归属按以下顺序产生：

1. 39 项人工 ABI 合同和 59 个顶层直接目标使用既有逐汇编结论作为固定边界种子；两组去重后是 80 个函数。
2. `library function`、VC Debug CRT、异常/启动代码和 12 个纯静态初始化/析构登记桩进入 `external_crt`。
3. DirectDraw、DirectInput、IME 链接桩和相邻空桩进入 `external_third_party`；其 `follow_up_module` 指向实际消费边界。
4. 其余游戏函数先按旧链接地址中的连续实现簇给出模块候选。
5. 完整汇编直接调用图只在模糊项中投票，并按模块封顶；高扇出函数不能因消费者多而改变自身所有权。
6. `.sav`、`battle.ffd`、`figtalk.dat`、Miles/Bink、DirectInput、TSW/ACT/ANI 和 LMF 等 token 只作导航加权。该信息来自反编译文本时，表中明确写为 `decompile_token_navigation_only`，不能覆盖汇编。

这个顺序特意避免三类错误：把 `AIL_serve` 归给资源调用者、把 RNG 归给战斗调用者、把软件 blitter 归给菜单或战斗调用者。共享基础函数属于其实现模块，不属于最大消费者。

## 4. 汇编调用图边界

完整汇编中有 7,159 条直接调用指向展开的 `PROC`。其中 7,151 条位于可归属的函数主体或函数块；另外 8 条位于函数目录没有独立入口的静态初始化小段。生成器同时断言这两个数字，不把 8 条调用伪归给地址相邻函数。

`module-function-ownership.tsv` 的 caller/callee 单元使用：

```text
0x地址:静态调用次数;0x地址:静态调用次数
```

这里只记录能由完整汇编解析到目录入口的直接 `call`。虚表、菜单函数槽、跳转表和导入 API 不被冒充为普通直调；已知菜单函数槽仍由既有专项证据人工覆盖。

## 5. 当前候选分布

| 模块 | 函数候选 |
|---|---:|
| `runtime_platform` | 17 |
| `resource_io` | 65 |
| `input_time_rng` | 25 |
| `rendering` | 151 |
| `audio_video` | 75 |
| `asset_runtime` | 77 |
| `story_scene` | 23 |
| `world_map` | 112 |
| `special_modes` | 226 |
| `battle` | 404 |
| `persistence` | 16 |
| `external_crt` | 312 |
| `external_third_party` | 7 |

置信度为：405 个 `confirmed_boundary`、813 个 `medium`、292 个 `mechanical_boundary`。这里的 `medium` 大多表示“连续实现簇明确但函数内部语义未展开”；`mechanical_boundary` 是没有成为实际游戏调用合同的其余外部入口。两者都符合 A2 不逐函数展开语义的停止线。

## 6. 人工复核状态与停止线

已经完成：

- 39 项关键 ABI 的模块归属，沿用各 ABI 证据中的汇编参数、清栈、返回和调用者策略。
- 59 个顶层去重直接目标的模块归属，沿用 [`top-level-direct-call-coverage.md`](top-level-direct-call-coverage.md) 的逐汇编分类。
- 游戏、CRT/编译器和 DirectX/系统链接桩三类来源边界；1,510 项没有 `unresolved`。
- 1,644 个 `game_cross_module` caller/callee 对，汇总为 307 个唯一游戏被调入口，已按实现簇、完整函数体和调用环境复核。
- 404 个 `game_to_external` caller/callee 对，汇总为 26 个实际外部被调入口，已复核其游戏消费者合同；外部实现本身不进入游戏逻辑重写范围。

复核中纠正了地址簇和 token 导致的错误，包括：图像命令流编解码归 `rendering`，游戏内角色/物品模态对话框归 `special_modes`，压缩器内部阶段与文件探测归 `resource_io`，Fame 存档块编解码归 `persistence`，`0x0044FFC0..0x004515E0` 的战斗表现/状态簇归 `battle`。B1 逐函数复核又确认 `0x0040DCE0` 只是“第二参数精确等于一则设置、否则清除”的剧情位包装器，随三个剧情位访问器归 `story_scene`。这些都是按完整汇编函数体和直接调用环境作出的模块决定，伪码只用于定位。

生成器把 307 个游戏边界入口的地址、最终模块和 caller 模块集合规范化后锁定为 SHA-256 `8d5abed4ebfd4e0ca97bd65e79d97de3e160a1580edac85b5765304824cf7938`。数量或摘要变化会停止生成，不能把新边界自动继承为已复核。当前 `module-boundary-functions.tsv` 共 333 项，全部是 `priority_reviewed`。

A2 到此停止。813 个模块内部 `medium` 候选不在本阶段逐函数恢复业务语义；函数命名、opcode、地图字段、战斗算法和存档字段留到对应模块的“逆向—UT—实现—汇编复核”循环。
