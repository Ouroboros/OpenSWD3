# OpenSWD3 原生重实现执行计划（Pi / v914）

> 本文件是当前唯一执行入口。
> 当前工作包：`audit_order=300 / 0x00478780 / sub_478780`。
> Workpacks 1–299已关闭；模块10仍未完成，禁止提前进入模块11或最终验收。

## 1. 最终目标

完整执行战斗函数workpack，直至：

1. `analysis/04-reverse-engineering/inventory/battle-function-workpack.tsv`达到`422/422`；
2. 每项均具有实现映射、不可达证明或合规阻塞说明；
3. 战斗生命周期、跨模块集成和I5验收通过；
4. B11及项目最终验收完成；
5. 所有要求均由当前仓库、测试、日志、提交和远端状态证明后，才可结束Goal。

Workpack 300完成不等于Goal完成。关闭后必须把本文件整体前移到Workpack 301。

## 2. 当前基线

- 分支：`main`。
- 已关闭基线：Workpacks 1–299。
- 当前inventory：`299/422 = 289 platform_adapted + 10 assembly_exact + 123 pending_audit`。
- 当前inventory SHA-256：`3e37262d7ed737667d380c92eece59d05b4bd4d68b4fa941099029f7d2543de0`。
- 当前目标：`audit_order=300 / 0x00478780 / sub_478780`。
- 导航列列出21个caller函数、35处物理CALL；必须以机器码逐条复核，不得把共享helper当成物理CALL合并依据。
- `sub_478B60`中的一处caller属于`audit_order=315`待审父边界，必须继续使用窄reply延期合同。
- 不启动原版或OpenSWD3游戏程序；只执行静态取证、构建和测试。

## 3. 权威机器边界

完整LST将目标锁定为`0x00478780..0x004787B4`共53字节：

```text
00478780  test dword ptr [ecx+26C0h],02000000h
0047878A  jnz  short locret_4787B4
0047878C  mov  eax,[ecx+26B8h]
00478792  test eax,80000000h
00478797  jz   short loc_4787A9
00478799  xor  edx,edx
0047879B  mov  [ecx+2A78h],dx
004787A2  mov  [ecx+542h],dx
004787A9  or   eax,80000000h
004787AE  mov  [ecx+26B8h],eax
004787B4  retn
```

边界事实：

- 11条实际指令；
- 0个callee；
- 2个条件分支；
- 1个普通RET；
- 无外部chunk或中段入口。

## 4. 必须恢复的机器语义

1. 先对`actor+0x26C0`完整dword执行`TEST 0x02000000`；bit25置位时直接RET，不得读取或写入其余字段。
2. gate未置位时读取`actor+0x26B8`完整dword，再对bit31执行TEST。
3. bit31原本置位时先`XOR EDX,EDX`，按顺序把零写入`actor+0x2A78` word与`actor+0x0542` word；任一写fault保留此前提交。
4. 无论bit31原值如何，随后都在EAX中OR `0x80000000`并把完整dword写回`actor+0x26B8`；不得按bit31已置位而省略最终写入。
5. gate早退保留入口EAX/EDX，flags来自首个TEST；正常路径EAX返回写回值，ECX保持actor token。
6. bit31原置位路径的EDX被XOR清零；bit31原清除路径保持入口EDX。
7. 正常字段写回后flags来自OR：CF/OF清零，ZF/SF/PF按结果，AF未定义；RET不改flags。
8. 普通RET读取`[ESP]`并令ESP增加4。

## 5. 字段与canonical owner要求

Workpack 300必须先核定全部字段xref和现有重叠typed view：

1. `actor+0x26B8`复用Workpack 299新增的中性`field_26b8`唯一owner；
2. `actor+0x2A78`复用既有`summon_completion_word`；
3. `actor+0x0542`对应slot 4 `special_target_action_record.command_cursor`，必须复用十八槽物理动作记录owner；
4. `actor+0x26C0`当前存在dword `delay_mode`与低byte `effect_direction_flags`重叠语义，必须收敛到单一可别名的canonical backing，不得再新增分叉dword或byte影子状态；
5. Group-A继续使用action owner，Group-B继续使用startup lifecycle中的同型action-execution owner；
6. 禁止新增平行actor数组、第二套字段缓存或token槽。

若取证推翻上述导航推定，以完整LST、静态xref和现有物理布局为准并同步修正文档。

## 6. caller闭包要求

1. 逐条提取并核验35处机器码CALL的调用地址、返回地址、入口寄存器、flags来源、条件可达性和后缀；
2. 已关闭现代caller必须直接组合typed leaf，不得保留`0x00478780` generic opaque生产调用；
3. 共享C++ helper只能消除样板，不能折叠不同物理CALL身份；
4. 待审caller不得被复制或部分现代化；只能登记精确延期合同；
5. `sub_478B60`窄reply只能投影该CALL真实执行信息，由Group-A/Group-B frame在同一owner上组合typed leaf，不得伪造未执行分支；
6. caller typed-stop必须保留当前CALL之前的副作用，并抑制当前CALL之后的真实后缀；
7. retired/reserved地址和ordinal保持，但生产路径对已关闭目标零调用。

## 7. 实施清单

1. 重新完整读取`AGENTS.md`、memory、本PLAN、inventory、相邻证据、目标LST、全部caller上下文和相关源码测试。
2. 取证53字节完整边界、四字段全xref、35处CALL及所有待审父边界。
3. 新增独立typed leaf接口与实现，保留访问顺序、部分提交、寄存器、flags、ESP/EIP、RET和typed-stop。
4. 收敛`+0x26C0`重叠别名并复用其余三项canonical owner。
5. 回收全部已关闭caller；为待审caller登记窄reply或延期合同。
6. 新增叶函数、owner别名、故障矩阵、35处物理身份及必要caller后缀测试。
7. 新文件全量clang-format；旧文件仅格式化changed ranges；运行`git diff --check`。
8. 运行定向构建与真实CTest测试名；Ninja Multi-Config使用`-C Debug`。
9. 仅用`analysis/tools/build_battle_workpack.py`关闭`0x00478780`并生成inventory；连续双跑验证逐字节稳定。
10. 新增证据文档并更新`analysis/04-reverse-engineering/modules/battle.md`。
11. 执行正式门、发布审计、精确暂存、commit、push和TG。
12. 提交后重新完整读取`AGENTS.md`和本PLAN；下一工作包继续执行，不得结束Goal。

## 8. 测试要求

叶函数和caller测试至少覆盖：

- `+0x26C0` bit25早退及其TEST flags；
- `+0x26B8` bit31原清除与原置位两条路径；
- bit31已置位时两个word按顺序清零；
- bit31已清除时不访问两个word但仍写回`+0x26B8`；
- gate、`+0x26B8`读取、两个word写、最终dword写和RET的独立typed-stop；
- 两个word第二项fault保留第一项写入；
- 最终dword写fault保留EAX/OR flags及此前word写入，但不提交内存写回；
- EAX/ECX/EDX、ESP/EIP、PF/ZF/SF/CF/OF和AF definedness；
- Group-A/Group-B canonical owner及`+0x26C0`byte/dword别名；
- 35处物理CALL返回地址与已关闭caller后缀；
- 生产`0x00478780` generic raw调用为零；
- Workpack 315及其余待审caller延期合同被静态锁定。

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

- 证据文档必须包含边界、指令语义、owner/重叠别名、访问顺序、flags/fault、35处caller合同、测试和动态差分状态。
- `analysis/04-reverse-engineering/modules/battle.md`追加Workpack 300关闭记录。
- PLAN状态变化时递增版本；关闭后整体替换为Workpack 301计划。
- 一个workpack只做一个REVIEW和一个commit，不拆分leaf、owner、caller、测试或文档。
- 提交必须使用`$commit` Skill；主Agent精确暂存，只提交本工作包文件，不提交`build/`。
- push成功后发送固定五段TG，段间使用真实空行，只写高层中文。
- 模块10验证段固定为：`验证：定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205 全部通过。`

## 11. 动态差分与阻塞

当前缺少原版完整Group-A/Group-B actor、四字段异常内存页、RET异常栈页及35处caller联合寄存器、flags与SEH捕获后端。静态LST、typed fault测试和现代路径可继续闭环；原版动态差分按证据登记`blocked_runtime_oracle`，不得伪造`original_diff_verified`。

## 12. 完成条件

Workpack 300只在以下全部成立时关闭：

- 53字节完整机器语义、四字段canonical owner与重叠别名已恢复；
- 35处物理CALL均有直接组合或精确延期合同，且不存在generic target调用；
- 测试、格式、inventory、正式门、审计、提交、push与TG全部通过；
- PLAN已前移到Workpack 301。

Goal只有在Workpacks 300–422、B11和最终项目验收全部完成并经逐项审计后才能调用`goal_complete`。
