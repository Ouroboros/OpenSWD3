# 战斗角色空闲状态查询 `0x004786A0`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:7/7`。

## 1. 完整LST边界与ABI

权威函数为`0x004786A0..0x004786A6`，共7字节、2条实际指令、0个call、0个条件分支和1个普通`retn`，没有外部`FUNCTION CHUNK`或中段入口：

```text
0x004786A0  mov eax, [ecx+0x2AB4]
0x004786A6  retn
```

入口为thiscall：ECX是actor token，`[ESP]`是真实caller返回地址。函数先从`actor+0x2AB4`读取完整32位状态latch到EAX，再由RET读取caller返回地址。正常返回时ESP增加4；ECX、EDX及全部flags保持进入值。

该leaf不解释字段值的业务含义，也不把非零值归一化为1。七处caller分别用后续TEST或`cmp eax,1`解释完整返回值。

## 2. typed访问顺序与停止点

`query_legacy_battle_actor_idle_state()`只模拟上述MOV与RET，不执行caller后续TEST/CMP。结果公开完整EAX、ECX、EDX、ESP、EIP、flags、字段token、字段读取、返回地址读取和栈读取计数。

两个独立typed-stop严格位于：

- `0x004786A0`字段读取：缺少canonical view或字段不可读时保留入口EAX/ECX/EDX、ESP与flags，不读取返回地址；
- `0x004786A6`返回地址读取：保留已装入EAX的完整latch、ECX/EDX、入口ESP与flags，不把RET视为已完成。

正常RET只记录真实caller返回地址一次，把ESP增加4并把EIP设为该地址。MOV与RET均不修改flags；即使latch为零，也不得合成TEST或XOR flags。

## 3. 字段状态机与canonical owner

`actor+0x2AB4`在权威LST中的已知直接访问为：

- `0x004786A0`完整dword读取；
- `0x0047886F`reset路径写0；
- `0x00478A75`动作选择发布路径写1；
- `0x00478D7A`直接比较；
- `0x0047D592`直接清零。

因此字段属于既有actor action-execution生命周期，而不是独立查询缓存。实现把它命名为`idle_state_latch`并直接加入同一canonical记录：

- Group-A token映射`LegacyBattleActionDispatchState::group_a_action_execution[10].idle_state_latch`；
- Group-B token映射`LegacyBattleStartupState::group_b_lifecycle[8].action_execution.idle_state_latch`。

无效token、缺失action owner、缺失startup或缺失Group-B lifecycle只产生空view并在真实MOV处停止。实现不新增平行actor数组、第二套latch或token槽。

## 4. 七处物理caller回收

权威LST中恰有七条`call sub_4786A0`，均已由frame caller直接组合typed leaf，生产路径不再通过generic port调用该地址。

### `0x00456917` / Group-A actor启动门

caller返回地址为`0x0045691C`。前一queue-completion callee提供EAX/EDX，紧随其后的`test eax,eax`决定是否到达本leaf；CALL前ECX为当前Group-A actor，EAX/EDX及TEST flags保持。返回后`cmp eax,1`只允许完整值1进入actor-available查询和启动后缀。typed-stop发生在CMP之前，抑制availability、frame-start、攻击顺序登记及后续frame路径。

### `0x00456AB9` / Group-A peer扫描

caller返回地址为`0x00456ABE`。peer状态callee提供EAX/EDX，`cmp eax,1`的非等分支到达本leaf；CALL前ECX恢复当前Group-A actor，EAX/EDX和该CMP flags保持。返回后TEST仅在完整值0时清control并累加progress。typed-stop阻断当前项提交、iteration计数和剩余扫描。

### `0x00456C48` / Group-A直接空闲门

caller返回地址为`0x00456C4D`。CALL前EAX为前一状态TEST的零值，ECX为当前Group-A actor，EDX保持此前残值，flags保持该TEST结果。返回后TEST只在完整值0时进入clear-presentation、AI/目标选择和最终处理。typed-stop抑制整个后缀。

### `0x00456DFB` / Group-A selected-target门

caller返回地址为`0x00456E00`。前一已关闭回合完成查询返回0且active-effect目标不等于`selected+7`时到达本leaf；CALL前EAX为`selected+7`，EDX保持one-based Group-A地址算术残值，flags来自active-effect目标与该值的CMP。返回后TEST只在完整值0时进入clear-control、clear-presentation、delay、selection和最终处理。typed-stop保留回合完成前缀并抑制该后缀。

### `0x00457858` / Group-B frame的Group-A目标扫描

caller返回地址为`0x0045785D`。前一已关闭回合完成查询返回0时到达本leaf；CALL前ECX为当前Group-B source actor，EAX为完成latch零值，EDX保持excluded查询残值，flags来自完成latch的TEST。返回后TEST只在完整值0时清目标control、准备目标并累加phase。typed-stop保留terminal/blocked/excluded/完成查询前缀并阻断当前项与剩余循环。

### `0x00457ABD` / Group-B selection阶段

caller返回地址为`0x00457AC2`。选择初始化与随机扫描汇合后，ECX恢复当前Group-B source actor；EAX、EDX与flags继承紧前选择路径。typed request显式携带这组三项caller入口状态。返回后TEST只在完整值0时进入packed-status、profile和动作模式处理。typed-stop抑制全部status/profile后缀。

### `0x00457E3F` / Group-B action-decision阶段

caller返回地址为`0x00457E44`。各动作决定路径汇合后，ECX为当前Group-B source actor；EAX、EDX与flags继承紧前status/mode路径，typed request不改写这些入口值。返回后`cmp eax,1`只允许完整值1进入已关闭opponent action dispatcher及完成/cleanup路径。typed-stop抑制dispatcher和公共完成后缀。

## 5. 测试与动态差分

leaf测试覆盖Group-A/Group-B owner别名、无效/缺失owner、完整非零dword、零dword、字段读取停止、RET读取停止、真实返回地址、ESP、EAX/ECX/EDX及入口flags保持。Group-A/Group-B frame测试覆盖七处caller的字段停止、caller-specific寄存器与TEST/CMP flags、真实owner域、四个Group-A及三个Group-B返回地址、完整零/1/其他非零值分支、后缀抑制和generic地址零调用。

验证：定向测试、AddressSanitizer、Linux core 199/199、Linux app 205/205 全部通过。连续10轮完整core均为199/199；新增文件全量及既有文件changed-range clang-format Werror、inventory双生成、零源码warning、TMP分类和完整发布审计均通过。工作包为`292/422 = 282 platform_adapted + 10 assembly_exact + 130 pending_audit`，inventory SHA-256为`9eacf35fec012c1c797eed0020ee0b2f6b46525ceda67c2e84f5a8d5112c0373`。未启动原版或OpenSWD3游戏程序。

原版动态差分登记为`blocked_runtime_oracle`：缺少完整Group-A/Group-B actor `+0x2AB4`字段、字段/返回地址异常内存页及七处caller EAX/ECX/EDX/ESP/flags与SEH联合捕获后端。该缺口不影响完整LST、静态访问顺序、typed停止或固定状态测试结论。
