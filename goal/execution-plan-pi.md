# OpenSWD3 原生重实现执行计划（Pi / v913）

> 本文件是当前唯一执行入口。
> 当前工作包：`audit_order=299 / 0x00478770 / sub_478770`。
> Workpacks 1–298已关闭；模块10仍未完成，禁止提前进入模块11或最终验收。

## 1. 最终目标

完整执行战斗函数workpack，直至：

1. `analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv`达到`422/422`；
2. 每项均具有实现映射、不可达证明或合规阻塞说明；
3. 战斗生命周期、跨模块集成和I5验收通过；
4. B11及项目最终验收完成；
5. 所有要求均由当前仓库、测试、日志、提交和远端状态证明后，才可结束Goal。

Workpack 299完成不等于Goal完成。关闭后必须把本文件整体前移到Workpack 300。

## 2. 当前基线

- 分支：`main`。
- 已关闭基线：Workpacks 1–298。
- 当前inventory：`298/422 = 288 platform_adapted + 10 assembly_exact + 124 pending_audit`。
- 当前inventory SHA-256：`8bf8a407bbf1781c98d2096019ff31a38a4fe7c17e1de2b00695457b94793b76`。
- 当前目标：`audit_order=299 / 0x00478770 / sub_478770`。
- 当前目标唯一直接caller：`sub_478B60`内`0x00478CC8 -> 0x00478CCD`。
- `sub_478B60`自身为`audit_order=315`，当前仍是`pending_audit`。
- 不启动原版或OpenSWD3游戏程序；只执行静态取证、构建和测试。

## 3. 权威机器边界

完整LST将目标锁定为`0x00478770..0x0047877A`共11字节：

```text
00478770  and dword ptr [ecx+26B8h],7FFFFFFFh
0047877A  retn
```

边界事实：

- 1条读改写AND和1条普通RET；
- 无callee、无条件分支、无局部标签、无外部chunk、无栈参数；
- ECX是actor token；
- 唯一直接caller在`sub_478B60`：
  - `0x00478CC0 mov ecx,ebp`
  - `0x00478CC2 mov [ebp+2AC4h],edi`
  - `0x00478CC8 call sub_478770`
  - `0x00478CCD pop edi`
- caller在调用前已通过`actor+0x26B8` bit31门、`sub_47BA80`返回零门，并把`actor+0x2AC4`写为EDI；返回后直接执行四次POP与`retn 4`。

## 4. 必须恢复的精确语义

### 4.1 字段读改写

- 从`actor+0x26B8`读取完整dword；
- 与`0x7FFFFFFF`执行32位AND；
- 把结果写回同一dword；
- 仅清bit31，低31位逐位保持；
- 即使bit31原本为零，也仍执行真实读改写访问，不得变成提前返回。

### 4.2 寄存器、flags与栈

- EAX、ECX、EDX保持入口值；
- AND成功后的flags来自32位逻辑结果：CF/OF清零，ZF/SF/PF按结果，AF未定义；
- 普通RET最后读取`[ESP]`返回地址并使ESP增加4；
- RET本身不改flags。

### 4.3 fault与部分提交

- 读改写必须区分字段读不可达和字段写不可达；
- 同一AND指令写失败时不提交字段，也不伪造AND完成后的flags；
- RET读取失败发生在字段写入和AND flags已经提交之后；
- 不增加actor预验、nil继续、范围夹限、短对象容错或替代返回。

## 5. canonical owner要求

先从完整`sub_478B60`、字段xref和现有startup/action/lifecycle状态中确定`actor+0x26B8`的唯一owner：

1. 优先扩展已经承载同一物理actor的canonical owner；
2. Group-A与Group-B必须沿既有actor token解析规则找到同一物理字段语义；
3. 禁止新增与startup/action/lifecycle平行的actor数组、影子状态或第二套token槽；
4. 如果现有待审opaque reply已发布该字段效果，必须改为同步同一canonical owner，不得同时保留相互分叉的状态。

字段在语义核定前使用中性命名；证据确认bit31含义后再采用业务名。

## 6. 唯一caller合同

`sub_478B60`为Workpack 315，当前不得借Workpack 299提前恢复其完整大函数。Workpack 299必须：

1. 记录`0x00478CC8 -> 0x00478CCD`唯一物理CALL身份；
2. 证明当前生产路径没有对`0x00478770`的generic opaque直调；
3. 在Workpack 315合同中登记：该caller必须在写`actor+0x2AC4`之后直接组合本typed leaf，保留入口EAX/EDX、AND flags、RET fault，以及返回后四次POP和`retn 4`的后缀阻断；
4. 若已关闭Group-A/Group-B frame通过`sub_478B60`窄reply消费该调用效果，则reply只投影真实执行信息，由外层在同一canonical owner上组合typed leaf；不得伪造未执行分支；
5. 不得为了“回收caller”复制或部分现代化`sub_478B60`剩余逻辑。

## 7. 实施清单

1. 重新完整读取`AGENTS.md`、memory、本PLAN、inventory、相邻证据、目标LST、唯一caller上下文和相关源码测试。
2. 取证目标完整边界、字段全xref、`sub_478B60`控制流前后缀及现有actor owner。
3. 新增独立typed leaf接口与实现，保留RMW、flags、RET、寄存器、ESP/EIP和typed-stop。
4. 接入唯一canonical owner；只在第6节条件满足时增加待审caller reply投影。
5. 新增叶函数和必要的caller合同测试；不得移动reserved地址或ordinal。
6. 新文件全量clang-format；旧文件仅格式化changed ranges；运行`git diff --check`。
7. 运行定向构建与真实CTest测试名；Ninja Multi-Config使用`-C Debug`。
8. 仅用`analysis/tools/build_battle_workpack.py`把`0x00478770`登记为核定状态并生成inventory；连续双跑验证逐字节稳定。
9. 新增证据文档并更新`analysis/04-reverse-engineering/modules/battle.md`。
10. 执行正式门、发布审计、精确暂存、commit、push和TG。
11. 提交后重新完整读取`AGENTS.md`和本PLAN，再整体前移到Workpack 300。

## 8. 测试要求

叶函数测试至少覆盖：

- `0xFFFFFFFF -> 0x7FFFFFFF`；
- `0x80000000 -> 0`；
- bit31原本为零时值保持但RMW访问仍发生；
- PF/ZF/SF/CF/OF及AF未定义；
- EAX/ECX/EDX保持；
- 字段读typed-stop零提交；
- 字段写typed-stop零提交并保留入口flags；
- RET typed-stop保留字段写入和AND flags，但ESP不推进；
- 成功RET返回真实caller地址并使ESP增加4；
- Group-A/Group-B canonical owner解析与非法token停止；
- 生产`0x00478770` generic raw调用为零；
- `0x00478CCD`caller身份和Workpack 315延期合同被静态锁定。

## 9. 正式门禁

所有项目命令使用：

```bash
export TMPDIR="$PWD/build/tmp/runtime"
export TMP="$PWD/build/tmp/runtime"
export TEMP="$PWD/build/tmp/runtime"
export OPENSWD3_BUILD_JOBS=16
export OPENSWD3_TEST_JOBS=16
```

必须完成：

1. 定向构建与目标测试；
2. `./build-asan.sh --test`；
3. `./build.sh core --test`；
4. `./build.sh app --test`；
5. 连续10轮`./build.sh core --test`；
6. 日志扫描：零OpenSWD3源码warning、测试失败、sanitizer finding和runtime error；
7. TMP审计；
8. inventory连续双生成与SHA-256稳定；
9. unstaged与staged release audit；
10. 新文件staged mode为`100644`；
11. `goal/HANDOFF.md`和仓库根`compile_commands.json`不存在；
12. 父级`compile_commands.json` symlink指向`OpenSWD3/build/linux-core/compile_commands.json`。

直接CMake/CTest只能作定向诊断，不能替代正式仓库脚本门禁。源码或测试语义变化后必须重跑受影响的正式门。

## 10. 文档、提交与通知

- 证据文档必须包含边界、指令语义、owner、RMW/flags/fault、caller合同、测试和动态差分状态。
- `analysis/04-reverse-engineering/modules/battle.md`追加Workpack 299关闭记录。
- PLAN状态变化时递增版本；关闭后整体替换为Workpack 300计划。
- 一个workpack只做一个REVIEW和一个commit，不拆分leaf、owner、测试或文档。
- 提交必须使用`$commit` Skill；主Agent精确暂存，只提交本工作包文件，不提交`build/`。
- push成功后发送固定五段TG，段间使用真实空行，只写高层中文。
- 模块10验证段固定为：`验证：定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205 全部通过。`

## 11. 动态差分与阻塞

当前缺少原版完整Group-A/Group-B actor、`actor+0x26B8`异常内存页、RET异常栈页及唯一caller联合寄存器、flags与SEH捕获后端。静态LST、typed fault测试和现代路径可继续闭环；原版动态差分按证据登记`blocked_runtime_oracle`，不得伪造`original_diff_verified`。

## 12. 完成条件

Workpack 299只在以下全部成立时关闭：

- 11字节完整机器语义和canonical owner已恢复；
- 唯一caller合同有静态证据且不存在generic target调用；
- 测试、格式、inventory、正式门、审计、提交、push与TG全部通过；
- PLAN已前移到Workpack 300。

Goal只有在Workpacks 299–422、B11和最终项目验收全部完成并经逐项审计后才能调用`goal_complete`。
