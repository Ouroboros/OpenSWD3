# `0x004762F0` 战斗组B奖励道具随机选择

状态：`platform_adapted`、`blocked_runtime_oracle`

来源：`swd3.exe.lst`完整函数体、唯一caller `0x00467710`及已关闭secondary bounded RNG `0x00439070`。完整LST是唯一行为真值；IDA名称、反编译输出、旧证据和测试只用于导航。

## 1. 完整主体与边界

`0x004762F0..0x00476323`从proc到endp共27行、17条实际指令、1个call、2个条件跳转、1个局部标签和2个返回点。函数没有外部`FUNCTION CHUNK`。

唯一调用边为：

```text
0x0046782D  sub_467710 -> sub_4762F0
```

唯一callee为`0x00439070`。该callee已经由`legacy-secondary-rng-00438fa0.md`关闭：参数是u32上界；本函数固定传20；正常返回值位于`[0,20)`；每次尝试先丢弃一个raw值，再用第二个raw值执行拒绝采样。实现不得换用标准库分布或省略被丢弃的raw值。

## 2. 精确行为

入口`ECX`是组B actor。函数先保存`ESI`，再执行：

1. 从actor `+0x0C`读取动态164-byte资源指针。
2. 比较资源`+0x82`的u16奖励道具编号。
3. 编号为零时不调用RNG，仅执行`xor ax, ax`并返回。
4. 编号非零时调用`0x00439070(20)`一次。
5. RNG返回后重新从actor `+0x0C`读取资源指针。
6. 以无符号u16比较`AX`和资源`+0x84`阈值。
7. `AX >= threshold`时执行`xor ax, ax`并返回；只有`AX < threshold`时才重新读取资源`+0x82`到`AX`并返回道具编号。

因此相等必须拒绝。比较域只包含低16位；不得改成有符号比较，也不得把资源阈值夹到0..20。

寄存器合同保留以下细节：

- 编号为零的路径从未覆盖完整`EAX`高word，因此返回`resource_token & 0xFFFF0000`。
- RNG路径先由bounded RNG写入完整`EAX/EDX`；失败只清`AX`，成功只以资源道具编号覆盖`AX`。
- RNG路径在比较前把`ECX`改为重读的资源token；编号为零路径保留入口actor token。
- 编号为零路径保留入口`EDX`；RNG路径返回该次随机值对应的`EDX`。

已审RNG正常返回0..19，所以原版正常运行时RNG路径的`EAX`高word为零；typed UT仍保留对高word只由16位写修改的结构性检查。

## 3. 唯一caller

`0x00467710`胜利奖励分发在组B循环中：

- `EBX`从`0x00525508`开始，每轮增加`0x2B28`。
- 循环上界每轮都从共享组B数量重读，并以signed i32比较。
- 调用后把完整`EAX`复制到`EDI`，但分支和10槽去重扫描只消费`DI`低word。
- 道具编号为零时直接进入下一actor；非零时复用既有玩家道具数量、10槽编号、数量和token发布路径。
- caller在每轮末尾重新读取组B数量；现代八槽owner在第九个actor首次真实访问前以typed-stop终止。

caller后续没有消费该函数返回值的高word。尽管如此，独立typed函数仍保留原16位写对返回寄存器的影响。

## 4. C++映射与owner

实现位于：

```text
include/openswd3/battle/legacy_battle_group_b_reward_item_selection.hpp
src/battle/legacy_battle_group_b_reward_item_selection.cpp
```

映射如下：

- actor复用`LegacyBattleStartupState::group_b_lifecycle`八槽唯一owner。
- 动态资源token和164-byte内容复用`LegacyBattleActorGroupBElementState::resource_token/resource_bytes`，不建立平行奖励资料。
- 随机调用复用`LegacyBattleBoundedRandomPort`；frame coordinator把同一`LegacySecondaryRng` adapter传入胜利奖励，保持与同帧其他战斗逻辑共享游标。
- actor缺失只在原版`[actor+0x0C]`首次读取点停止。
- 初次resource token为零只在原版`[resource+0x82]`首次读取点停止；此前actor资源token读取和入口寄存器状态均保留。
- RNG返回后重读的resource token为零时，只在原版`[resource+0x84]`访问点停止；该次随机消费和`EAX/EDX`随机返回、`ECX`零token均保留。
- 胜利奖励的旧opaque query枚举槽保留为`reserved_query_group_b_item`稳定数值，但生产代码不再调用该槽。

胜利奖励结果保存每个有效组B槽的typed子结果。子函数typed-stop直接映射为caller的`group_b_reward_item_typed_stop`，并阻断道具归并、后续actor、组A奖励和金钱提交。

## 5. 静态与测试合同

专门UT覆盖：

- actor首次访问typed-stop且不消费RNG。
- 初次resource访问typed-stop且不消费RNG。
- RNG后resource重读typed-stop且保留该次随机返回寄存器。
- 道具编号零时不消费RNG，并只清`AX`。
- 固定上界20和每个非零编号恰好一次随机调用。
- 无符号低word比较、相等拒绝、成功重写`AX`及失败只清`AX`。
- `ECX/EDX`和高word线程。

胜利奖励caller UT覆盖重复道具归并、零道具跳过、10槽溢出前缀、live signed上界重读、typed-stop传播以及旧opaque query零调用。

最终验证：战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`、Linux app `194/194`全部通过。core和ASan日志均为零warning、零finding；app仅保留既有ALSA开发库环境提示。

## 6. 动态oracle缺口

原版组B actor、动态资源`+0x82/+0x84`、secondary RNG种子/游标、返回寄存器和唯一caller共享道具槽的联合捕获后端仍缺失，因此动态差分为`blocked_runtime_oracle`。所需最小回放记录为：

```text
actor_index
resource_token
resource_item_word_82
resource_threshold_word_84
secondary_rng_seed_or_pre_call_cursor
bounded_random_return
post_call_cursor
return_eax
return_ecx
return_edx
collected_item_ids[10]
collected_item_quantities[10]
```

该阻塞不影响完整静态闭环、typed owner收敛和Linux验证。
