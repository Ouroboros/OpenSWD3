# 战斗角色回合完成查询 `0x00478690`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed:3/3`。

## 1. 完整LST边界与ABI

权威函数为`0x00478690..0x00478696`，共7字节、2条实际指令、0个call、0个条件分支和1个普通`retn`，没有外部`FUNCTION CHUNK`或中段入口：

```text
0x00478690  mov eax, [ecx+0x2AAC]
0x00478696  retn
```

入口为thiscall：ECX是actor token，`[ESP]`是真实caller返回地址。函数先从`actor+0x2AAC`读取完整32位回合完成latch到EAX，再由RET读取caller返回地址。正常返回时ESP增加4；ECX、EDX及全部flags保持进入值。

## 2. typed访问顺序与停止点

`query_legacy_battle_actor_turn_completion()`只模拟上述MOV与RET，不执行caller后续TEST，也不把非零值归一化为1。结果公开完整EAX、ECX、EDX、ESP、EIP、flags、字段token、字段读取、返回地址读取和栈读取计数。

两个独立typed-stop严格位于：

- `0x00478690`字段读取：缺少canonical view或字段不可读时保留入口EAX/ECX/EDX、ESP与flags，不读取返回地址；
- `0x00478696`返回地址读取：保留已装入EAX的完整latch、ECX/EDX、入口ESP与flags，不把RET视为已完成。

正常RET只记录真实caller返回地址一次，把ESP增加4并把EIP设为该地址。MOV与RET均不修改flags；即使latch为零，也不得合成TEST或XOR flags。

## 3. canonical owner

resolver按actor token直接借用既有唯一owner：

- Group-A token映射`LegacyBattleActionDispatchState::group_a_action_execution[10].turn_completion_latch`；
- Group-B token映射`LegacyBattleStartupState::group_b_lifecycle[8].action_execution.turn_completion_latch`。

无效token、缺失action owner、缺失startup或缺失Group-B lifecycle只产生空view并在真实MOV处停止。实现不新增平行actor数组、第二套latch或token槽。

## 4. 三处物理caller回收

权威LST中恰有三条`call sub_478690`，均已由frame caller直接组合typed leaf，生产路径不再通过generic port调用该地址。

### `0x00456DD6` / Group-A frame one-based目标

caller返回地址为`0x00456DDB`。one-based值经原算术形成Group-A actor token；CALL前保持`EAX=ordinal*0x3EF`、`EDX=ordinal*0xBCD`，flags来自最后一次`ordinal*0x3F0-ordinal`，ECX为所选Group-A actor。返回后原`test eax,eax`决定是否进入目标准备；leaf typed-stop发生在该TEST之前，保留已完成frame前缀并阻断当前目标分支与全部后缀。

### `0x00456EEB` / Group-A frame Group-B目标

caller返回地址为`0x00456EF0`。动作目标getter的AX经有符号扩展及地址算术形成Group-B token；有效索引域内CALL前保持`EAX=index*0x565`、EDX继承动作目标getter，flags来自最后一次`index*0x18-index`。owner直接取startup Group-B lifecycle。返回后原TEST控制动作开始；typed-stop保留动作目标查询残值，阻断execution/current-actor写入、selection处理及后缀。

### `0x0045784D` / Group-B frame Group-A扫描

caller返回地址为`0x00457852`。循环中的目标为Group-A token。前两个对象查询和固定AI门通过后，excluded getter返回EAX/EDX；紧随其后的`cmp eax,1`决定CALL进入flags，getter本身保持这些值。返回后原TEST控制source idle查询、clear control、prepare target及progress累加；typed-stop保留此前terminal、blocked和excluded调用，阻断当前项及剩余循环后缀。

## 5. 测试与动态差分

leaf测试覆盖Group-A/Group-B owner别名、无效/缺失owner、完整非零dword、零dword、字段读取停止、RET读取停止、真实返回地址、ESP、EAX/ECX/EDX及入口flags保持。Group-A/Group-B frame测试分别覆盖三处caller的字段停止、caller-specific寄存器与算术/CMP flags、真实owner域、返回EIP、generic地址零调用，以及各自post-call分支与suffix抑制。

最终注入式战斗定向`1/1`、Linux core `199/199`、AddressSanitizer/UBSan `199/199`、Linux app `205/205`及连续十轮完整core `10/10`均通过；所有正式stderr为空，源码warning、测试失败、sanitizer finding和runtime error扫描无命中。三个新C++文件全量clang-format、六个旧C++文件changed-range apply及`--dry-run --Werror`均通过。inventory由权威生成器连续双跑逐字节一致，关闭统计为`291/422 = 281 platform_adapted + 10 assembly_exact + 131 pending_audit`，SHA-256为`6f3b2d2ac6ed80eeae52310b85a946e408c4ce5846452bd2c0a82175d5283119`；TMP审计为`confirmed_entries=0`、`errors=[]`。未启动原版或OpenSWD3游戏程序。

原版动态差分登记为`blocked_runtime_oracle`：缺少完整Group-A/Group-B actor `+0x2AAC`字段、字段/返回地址异常内存页及三处caller EAX/ECX/EDX/ESP/flags与SEH联合捕获后端。该缺口不影响完整LST、静态访问顺序、typed停止或固定状态测试结论。
