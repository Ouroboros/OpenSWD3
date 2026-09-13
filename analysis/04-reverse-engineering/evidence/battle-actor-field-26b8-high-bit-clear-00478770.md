# 战斗角色`+0x26B8`高位清除（0x00478770）

## 1. 范围与边界

权威LST把`sub_478770`锁定为`0x00478770..0x0047877A`共11字节、2条实际指令、0个callee、0个分支和1个普通`retn`，没有外部chunk、中段入口或隐藏异常尾：

```text
00478770  and dword ptr [ecx+26B8h],7FFFFFFFh
0047877A  retn
```

反汇编机器码为`81 A1 B8 26 00 00 FF FF FF 7F C3`。入口ECX是角色对象token；函数不读取显式参数。

## 2. 精确机器语义

`AND r/m32, imm32`必须按真实32位读改写执行：

1. 读取`actor+0x26B8`完整dword；
2. 与`0x7FFFFFFF`执行AND，只清bit31并保留低31位；
3. 把结果写回同一dword；
4. 从`[ESP]`读取返回地址，普通RET令ESP增加4。

bit31入口已为零时仍执行完整字段读和字段写，不允许按最终值未变化而提前返回。EAX、ECX和EDX保持入口值。

AND成功后提交逻辑flags：CF和OF清零；ZF、SF、PF按32位结果；AF未定义。MOV式字段读写和RET都不再改变flags。

## 3. 部分提交与故障边界

`clear_legacy_battle_actor_field_26b8_high_bit()`按真实顺序建模三个停止点：

1. `field_read_typed_stop`：停在`0x00478770`，字段未读未写，入口寄存器、ESP和flags全部保持；
2. `field_write_typed_stop`：仍停在`0x00478770`，保留一次字段读取，但不提交字段结果，也不提交未完成AND的flags；
3. `return_address_read_typed_stop`：停在`0x0047877A`，字段与AND flags已经提交，但不读取返回地址、不推进ESP。

正常返回记录字段read/write、RET栈token与返回地址，返回EIP为调用者返回地址。该模型不把读改写折叠为高层条件赋值，也不在写fault后伪造逻辑flags。

## 4. 字段生命周期与canonical owner

对完整LST静态扫描只发现7次`actor+0x26B8`访问：

- `0x00478770`：当前函数清bit31；
- `0x0047878C`和`0x004787AE`：`sub_478780`读取并以OR设置bit31；
- `0x004787C0`：`sub_4787C0`按signed右移查询bit31；
- `0x00478CA5`：`sub_478B60`以TEST检查bit31；
- `0x0047D51E`：`sub_47D350`基础初始化写零；
- `0x0047E6CF`：`sub_47E650`以signed比较决定是否跳过补充画面绘制。

这些静态事实证明字段至少包含一个跨初始化、置位、查询、清位和绘制门使用的bit31；低31位在当前函数中必须原样保留，但现有证据不足以给它们赋予更窄的业务含义。因此C++保留中性名`field_26b8`，并放入既有`LegacyBattleGroupAActionExecutionState`：

- Group-A：`LegacyBattleActionDispatchState::group_a_action_execution[index].field_26b8`；
- Group-B：`LegacyBattleStartupState::group_b_lifecycle[index].action_execution.field_26b8`。

两组角色继续复用startup/action/lifecycle canonical owner，没有新增平行actor数组、第二份字段缓存或token槽。

## 5. 唯一物理caller

机器码全局扫描只有一条`call sub_478770`：

```text
00478B6C  mov  ebx,1
...
00478CA5  test dword ptr [ebp+26B8h],80000000h
00478CAF  jz   short loc_478CD4
00478CB1  mov  ecx,ebp
00478CB3  call sub_47BA80
00478CB8  cmp  eax,ebx
00478CBA  jnz  loc_479845
00478CC0  mov  ecx,ebp
00478CC2  mov  [ebp+2AC4h],edi
00478CC8  call sub_478770
00478CCD  pop  edi
00478CCE  pop  esi
00478CCF  pop  ebp
00478CD0  pop  ebx
00478CD1  retn 4
```

真实调用地址为`0x00478CC8`，返回地址为`0x00478CCD`。EBX从函数入口起固定为1；只有`sub_47BA80`返回EAX=1时`cmp eax,ebx`才落入CALL，因此leaf入口EAX固定为1。随后两个MOV不改flags，CALL入口flags来自`cmp 1,1`：CF=0、PF=1、AF=0、ZF=1、SF=0、OF=0。ECX为角色token；EDX保持`sub_47BA80`及后续CMP/MOV未修改的返回残值。返回后不再读取leaf返回值或flags，只执行四次POP和`retn 4`。

caller所属`sub_478B60 / 0x00478B60`是`audit_order=315`，当前仍为`pending_audit`。Workpack 299不复制或现代化该完整LST共1403行、31个CALL的待审大函数。既有Group-A/Group-B frame继续通过`0x00478B60`窄port执行未审父边界；其reply只在原路径真实执行`0x00478CC8`时发布`executed`和入口EDX。外层随即以固定EAX=1、`CMP 1,1` flags和返回地址直接组合typed leaf。

当前窄reply组合发生在待审父边界返回到现代frame之后；leaf typed-stop会立即抑制Group-A/Group-B frame剩余后缀，但Workpack 299不伪装已经局部恢复父函数内部epilogue。Workpack 315必须把typed leaf直接放在`actor+0x2AC4`写入之后，使任一leaf停止真实阻断`0x00478CCD`开始的四次POP与`retn 4`。未执行分支不发布pending call，不伪造`sub_478770`副作用。

## 6. 双向追溯与验证范围

LST到C++追溯覆盖11字节完整边界、32位字段RMW、bit31清除、低31位保持、AND flags、普通RET、EAX/ECX/EDX保持、唯一caller及其固定返回地址。C++到LST反向追溯覆盖字段read/write与RET三个停止点、entry flags部分提交、Group-A/Group-B canonical owner、窄reply的真实执行门、现代frame后缀抑制、Workpack 315父epilogue延期合同及生产generic调用归零。

测试覆盖bit31置位、仅bit31置位和bit31已清三类输入，确认已清输入仍发生一次read和一次write；同时覆盖owner别名、寄存器、ESP/EIP、字段与栈token、AND flags、字段读fault、字段写fault、RET fault、唯一物理caller身份、Group-A正常组合和Group-B RET停止后的frame后缀抑制。

## 7. 动态差分状态

当前缺少原版完整Group-A/Group-B actor、`actor+0x26B8`异常内存页、RET异常栈页，以及唯一caller的联合寄存器、flags与SEH捕获后端，原版动态差分登记为`blocked_runtime_oracle`。该阻塞不改变11字节leaf和唯一静态caller的LST闭环。
