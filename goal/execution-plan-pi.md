# OpenSWD3 原生重实现执行计划（Pi / v915）

> 本文件是当前唯一执行入口。
> 当前工作包：`audit_order=301 / 0x004787C0 / sub_4787C0`。
> Workpacks 1–300已关闭；模块10仍未完成，禁止提前进入模块11或最终验收。

## 1. 最终目标

完整执行战斗函数workpack，直至：

1. `analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv`达到`422/422`；
2. 每项均具有实现映射、不可达证明或合规阻塞说明；
3. 战斗生命周期、跨模块集成和I5验收通过；
4. B11及项目最终验收完成；
5. 所有要求均由当前仓库、测试、日志、提交和远端状态证明后，才可结束Goal。

Workpack 301完成不等于Goal完成。关闭后必须把本文件整体前移到Workpack 302。

## 2. 当前基线

- 分支：`main`。
- 已关闭基线：Workpacks 1–300。
- 当前inventory：`300/422 = 290 platform_adapted + 10 assembly_exact + 122 pending_audit`。
- 当前inventory SHA-256：`6b2a0955844e203c1cdec332519159fed4225d54e4b44f2408253fdd2f241306`。
- 当前目标：`audit_order=301 / 0x004787C0 / sub_4787C0`。
- 导航列列出唯一caller函数`0x00453200`和一处物理CALL；必须以完整机器码核验入口寄存器、flags、返回地址和后缀。
- `actor+0x26B8`必须复用Workpacks 299–300建立的同一中性canonical owner。
- 不启动原版或OpenSWD3游戏程序；只执行静态取证、构建和测试。

## 3. 权威机器边界

完整LST将目标锁定为`0x004787C0..0x004787C9`共10字节：

```text
004787C0  mov eax,[ecx+26B8h]
004787C6  shr eax,1Fh
004787C9  retn
```

边界事实：

- 3条实际指令；
- 0个callee；
- 0个分支；
- 1个普通RET；
- 无显式栈参数、外部chunk或中段入口。

## 4. 必须恢复的机器语义

1. 从`actor+0x26B8`读取完整dword到EAX，不得先做布尔化字段访问或省略真实读取。
2. 对EAX执行`SHR 31`，完整返回值严格为原bit31的零或一。
3. ECX保持actor token，EDX保持入口值。
4. SHR计数为31：CF取原bit30；ZF、SF、PF按零或一结果；OF对计数大于1未定义；AF未定义。
5. 字段读取故障停在`0x004787C0`并保持入口寄存器、flags与ESP。
6. 普通RET从`[ESP]`读取返回地址并令ESP增加4；RET故障停在`0x004787C9`，保留SHR后的EAX与flags但不推进ESP。

## 5. Canonical owner要求

1. `actor+0x26B8`直接复用`LegacyBattleGroupAActionExecutionState::field_26b8`及Group-B lifecycle同型action-execution owner；
2. Group-A继续从action/startup唯一状态解析，Group-B继续从startup lifecycle唯一状态解析；
3. 查询与Workpack 299高位清除、Workpack 300高位设置必须观察同一物理backing；
4. 禁止新增布尔缓存、平行actor数组、第二套`field_26b8`或token槽；
5. generic setter `0x00478A70/0x00478B20`在自身回收前仍须把已知写效果同步到同一canonical owner。

## 6. caller闭包要求

唯一物理CALL位于`sub_453200`：

```text
00453409  call sub_4787C0
0045340E  test eax,eax
00453410  jnz  loc_453434
```

必须：

1. 取证caller完整函数及外部chunk，核验`0x00453409 -> 0x0045340E`身份；
2. 恢复以`dword_53BD54`计算Group-A actor token的完整EAX/ECX线程、入口EDX和最后一次`SUB`产生的flags；
3. 在原调用点直接组合typed leaf，不保留`0x004787C0` generic opaque生产调用；
4. 正常返回后以完整EAX执行`TEST EAX,EAX`并保持原`JNZ`后缀；
5. leaf typed-stop保留CALL前副作用，并抑制`TEST`、分支和其余frame后缀；
6. 物理CALL身份、返回地址和trace不得因共享helper折叠；reserved地址和ordinal保留但生产零调用。

## 7. 实施清单

1. 重新完整读取`AGENTS.md`、memory、本PLAN、inventory、相邻证据、目标LST、caller完整上下文和相关源码测试。
2. 取证10字节完整边界、`+0x26B8`全部相关xref、唯一CALL及caller前后缀。
3. 新增或复用独立typed query接口，保留字段访问、SHR flags、寄存器、ESP/EIP、RET与typed-stop。
4. 复用Workpacks 299–300的canonical owner，不新增平行状态。
5. 在`sub_453200`现代实现原位置直接组合typed leaf并恢复TEST/JNZ后缀。
6. 新增叶函数、owner共享、故障、flags与caller后缀测试。
7. 新文件全量clang-format；旧文件只格式化changed ranges；运行`git diff --check`。
8. 运行定向构建与真实CTest测试名；Ninja Multi-Config使用`-C Debug`。
9. 仅用`analysis/tools/build_battle_workpack.py`关闭`0x004787C0`并生成inventory；连续双跑验证逐字节稳定。
10. 新增证据文档并更新`analysis/04-reverse-engineering/modules/battle.md`。
11. 执行正式门、发布审计、精确暂存、commit、push和TG。
12. 提交后重新完整读取`AGENTS.md`和本PLAN；下一工作包继续执行，不得结束Goal。

## 8. 测试要求

叶函数和caller测试至少覆盖：

- 原bit31清除返回0，原bit31置位返回1；
- bit30分别为零/一时CF结果；
- SHR后的PF/ZF/SF、CF，以及AF/OF definedness；
- EAX完整覆盖、ECX/EDX保持、ESP/EIP；
- 字段读取与RET独立typed-stop；
- RET fault保留字段读取和SHR结果；
- Group-A/Group-B canonical owner与Workpacks 299–300写端共享；
- 唯一物理CALL的入口寄存器、flags、返回地址与post-call TEST/JNZ；
- caller typed-stop后缀阻断；
- 生产`0x004787C0` generic raw调用为零。

## 9. 正式门禁

所有项目命令使用：

```bash
export TMPDIR="$PWD/build/tmp/runtime"
export TMP="$TMPDIR"
export TEMP="$TMPDIR"
export OPENSWD3_BUILD_JOBS=16
export OPENSWD3_TEST_JOBS=16
```

必须全部通过：

1. 定向构建；
2. 真实CTest名`battle.legacy_battle_setup`，Ninja Multi-Config带`-C Debug`；
3. `./build-asan.sh --test`；
4. `./build.sh core --test`；
5. `./build.sh app --test`；
6. 连续10轮`./build.sh core --test`；
7. 全部正式日志零OpenSWD3源码warning、测试失败、sanitizer finding和runtime error；
8. `git diff --check`；
9. inventory连续双生成字节一致；
10. TMP审计`confirmed_entries=0`；
11. `goal/HANDOFF.md`和仓库根`compile_commands.json`不存在；
12. 父级`compile_commands.json` symlink指向`OpenSWD3/build/linux-core/compile_commands.json`。

直接CMake/CTest只能作定向诊断，不能替代正式仓库脚本门禁。源码或测试语义变化后必须重跑受影响的正式门。

## 10. 文档、提交与通知

- 证据文档必须包含边界、MOV/SHR语义、owner、flags/fault、唯一caller合同、测试和动态差分状态。
- `analysis/04-reverse-engineering/modules/battle.md`追加Workpack 301关闭记录。
- PLAN状态变化时递增版本；关闭后整体替换为Workpack 302计划。
- 一个workpack只做一个REVIEW和一个commit。
- 提交必须使用`$commit` Skill；主Agent精确暂存，只提交本工作包文件，不提交`build/`。
- push成功后发送固定五段TG，段间使用真实空行，只写高层中文。
- 模块10验证段固定为：`验证：定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205 全部通过。`

## 11. 动态差分与阻塞

当前缺少原版完整Group-A actor、`actor+0x26B8`异常内存页、RET异常栈页及唯一caller联合寄存器、flags与SEH捕获后端。静态LST、typed fault测试和现代路径可继续闭环；原版动态差分按证据登记`blocked_runtime_oracle`，不得伪造`original_diff_verified`。

## 12. 完成条件

Workpack 301只在以下全部成立时关闭：

- 10字节完整机器语义与`+0x26B8`canonical owner已恢复；
- 唯一物理CALL已直接组合，且不存在generic target调用；
- 测试、格式、inventory、正式门、审计、提交、push与TG全部通过；
- PLAN已前移到Workpack 302。

Goal只有在Workpacks 301–422、B11和最终项目验收全部完成并经逐项审计后才能调用`goal_complete`。
