# `0x00476330` 战斗组B状态行动随机判定

状态：`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe.lst`完整函数体、唯一caller `0x004576A0`及已关闭secondary bounded RNG `0x00439070`。完整LST是唯一行为真值；IDA名称、反编译输出、旧证据和测试只用于导航。

## 1. 完整主体与边界

`0x00476330..0x004763D0`从proc到endp共77行、52条实际指令、4个call、7个条件跳转、3个局部标签和4个返回点。函数没有外部`FUNCTION CHUNK`。

唯一调用边为：

```text
0x00457B01  sub_4576A0 -> sub_476330
```

四个callsite都调用`0x00439070`。该callee已经由`legacy-secondary-rng-00438fa0.md`关闭：参数是u32上界；正常返回在`[0,bound)`；每次尝试先丢弃一个raw值，再用第二个raw值执行拒绝采样。实现不得换用标准库分布或省略其内部raw消费。

## 2. 入口门与首次随机消费

入口`ECX`是组B actor。栈参数虽然由caller以完整`EDX`压入，但函数只在后面以`and eax, 0xFF`消费低byte。

函数首先读取actor `+0x26D1`并测试bit 3，即既有`actor+0x26D0`低word的bit 11：

- bit 11为一：直接返回零，不调用RNG，也不访问动态资源；
- bit 11为零：固定调用一次`0x00439070(12)`；该返回值不参与任何判定，只推进secondary RNG并暂时占据`EAX/EDX`。

首次RNG返回后才从actor `+0x0C`读取动态164-byte资源token，再读取资源`+0x91`的u8机会值。机会值为零时直接返回零，且不会调用第二次RNG。

函数没有任何actor、资源或共享状态写入。

## 3. 差值与三段判定

资源机会值非零时，函数把资源`+0x54`读取为零扩展u16基值。随后计算：

```text
signed_delta = signed_i32((argument_low_byte - resource_word_54) mod 2^32)
```

所有`jle/jl`都按signed i32解释该减法结果。由实际输入宽度可得到三个随机判定带和一个直接失败带：

1. `signed_delta > 10`：调用`random_bounded(10)`，仅当随机返回低word `< 8`时返回一；相等八失败。
2. `5 < signed_delta <= 10`：调用`random_bounded(10)`，仅当随机返回低word `< 5`时返回一；相等五失败。
3. `signed_delta < 5`：不再调用RNG，直接返回零。
4. `signed_delta == 5`：调用`random_bounded(10)`，然后重新读取actor `+0x0C`资源token与新资源`+0x91`机会值；仅当随机返回低word `<`重读机会值时返回一，相等失败。

`delta==5`路径在第二次RNG前还有`test cl, cl / jbe`。由于该路径只能从此前已证明非零的同一个`CL`到达，且中间没有call或`CL`写入，该分支静态上不会失败；实现保留其非零前置事实，不发明额外资源访问。

正常每次调用最多消费两次bounded RNG：首个上界固定12且结果废弃；只有上述三个判定带之一再消费一次上界10。资源机会值大于10时，`delta==5`路径在正常bounded返回域0..9内恒成功；不得把机会值夹到十。

## 4. 返回寄存器合同

所有正常决策只以完整`EAX=0/1`返回。其余寄存器仍保留下列汇编细节：

- actor bit 11早退：`ECX`仍为actor token，`EDX`仍为caller压入的完整参数寄存器值。
- 首次资源访问前故障：`EAX`已是零resource token，`EDX`仍是bound 12随机返回；RNG后的`ECX`不可由现有窄随机port恢复。
- 首次资源机会为零：`mov cl, [resource+0x91]`只覆盖未知RNG后`ECX`的低byte；`EDX`仍是首个随机返回。
- `delta<5`直接失败：`EDX`是零扩展资源`+0x54`基值；`ECX`低byte是首次资源机会，其高24位仍未知。
- 上、中判定带：第二次RNG覆盖完整`EDX`；最终布尔规范化只覆盖`EAX`；RNG后的`ECX`未知。
- `delta==5`：第二次RNG后把`ECX`改为重读resource token；`movzx dx, byte ptr [ecx+0x91]`只覆盖`DX`，因此`EDX`高word保留第二次随机返回的高word，低word是重读机会值。
- `delta==5`重读token为零时，停止点位于第二次资源`+0x91`访问，保留第二次随机返回的`EAX/EDX`和零`ECX`。

现有bounded RNG正常返回值很小，但typed UT仍用非零高word验证16位寄存器写结构，避免实现意外清除高word。

## 5. 唯一caller与旧边界回收

`0x004576A0`在`0x00457AE0..0x00457B01`执行：

1. 从共享profile索引计算`index * 14`。
2. 只把`dword_4AB7BC[index*14]`写入`DL`，保留`EDX`高24位陈旧值。
3. 以当前组B actor为`ECX`调用本函数。
4. 返回非零时跳到`0x00457E23`，依次发布selection零、action mode `0x11`和status mode `2`，随后进入公共行动阶段。
5. 返回零时继续读取该actor的packed status word，执行负标志、`0x4000/0x2000`模式、特殊行动、目标选择及profile分支。

C++ caller保留profile十四字节步长和低byte覆盖结构，把完整陈旧`EDX`作为typed请求输入。旧`0x00476330` opaque调用已删除；caller直接调用typed函数并复用`LegacyBattleStartupState::group_b_lifecycle`八槽actor owner及同一`LegacyBattleBoundedRandomPort`游标。typed-stop阻断true/false两侧全部caller后缀。

## 6. C++映射与测试合同

实现位于：

```text
include/openswd3/battle/legacy_battle_group_b_status_action.hpp
src/battle/legacy_battle_group_b_status_action.cpp
```

唯一owner映射为：

- actor gate复用`LegacyBattleGroupAActionExecutionState::retreat_ready_flags`的`actor+0x26D0`低word。
- 动态资源token和164-byte内容复用`LegacyBattleActorGroupBElementState::resource_token/resource_bytes`。
- 随机调用复用frame coordinator已持有的`LegacyBattleBoundedRandomPort`，不建立独立随机流。
- actor缺失只在原版入口`[actor+0x26D1]`访问点停止。
- 首次resource token为零只在首个`[resource+0x91]`访问点停止，保留bound 12随机消费。
- `delta==5`路径重读token为零只在第二个`[resource+0x91]`访问点停止，保留两次随机消费和第二次随机返回寄存器。

专门UT覆盖入口actor stop、bit 11早退、首次resource stop、机会零、signed负差、三个判定带、固定bounds `12/10`、阈值相等失败、低word比较、动态重读机会、重读resource stop及寄存器高word线程。frame caller UT覆盖true后缀、actor/resource stop传播、profile低byte输入、共享RNG消费和旧opaque地址零调用。

最终验证：战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`、Linux app `194/194`全部通过；core和ASan零warning、零finding，app仅保留既有ALSA开发库环境提示。

## 7. 动态oracle缺口

原版组B actor、动态资源`+0x54/+0x91`、secondary RNG种子/游标、返回寄存器与唯一caller状态后缀的联合捕获后端仍缺失，因此动态差分为`blocked_runtime_oracle`。所需最小回放记录为：

```text
actor_index
actor_flags_word_26d0
profile_index
profile_argument_full_edx
resource_token_before_first_rng
resource_word_54
resource_chance_byte_91_before_decision
secondary_rng_seed_or_pre_call_cursor
bounded_random_bounds[]
bounded_random_returns[]
resource_token_after_second_rng
resource_chance_byte_91_after_second_rng
post_call_cursor
return_eax
return_ecx
return_edx
caller_selection
caller_action_mode
caller_status_mode
```

该阻塞不影响完整静态闭环、typed owner收敛和Linux验证。
