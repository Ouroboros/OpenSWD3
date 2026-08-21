# 剧情 VM 角色 action wait override `0x00428DB8`

## 结论

`sub_427920` 主分派 opcode 44 的唯一入口是 `0x00428DB8`。物理记录固定为 6 字节：

```text
+0  u16 raw opcode
+2  u16 role selector
+4  u16 wait override
```

handler把`+2`读为完整u16；raw selector `0xFFF0`先替换为当前TALK context `+0x24`的source GUID，然后调用`sub_40C0D0`。该helper还保留另一个独立特殊值：`0xFFFE`直接返回受控角色index，ordinary selector则按u16 GUID查找第一个bit28清的角色。

lookup返回值被机器完全忽略。随后handler才读取`u16(+4)`，把它写到所选角色内嵌action `+0x48`的16位`wait_override`，再把action `+0x44`的16位`wait_remaining`写零，并调用一次`sub_4321E0(action)`。refresh返回零只触发`Act Err(Talk:ChangSpd)`的`nullsub_1`诊断；写入不回滚，仍推进6字节、发布previous并在同一次VM调用继续。

## 唯一汇编边界

opcode-specific入口为`0x00428DB8..0x00428E4D`；下一handler opcode45从`0x00428E52`开始。

selector、lookup和staged value read：

```text
00428DB8  mov bx,[current+2]
00428DBC  cmp bx,0FFF0h
00428DC1  jnz loc_428DCB
00428DC3  mov eax,[talk_context]
00428DC7  mov bx,[eax+24h]           ; context source GUID
00428DCB  lea ecx,[var_4C]
00428DCF  push ecx
00428DD0  push ebx
00428DD1  call sub_40C0D0
00428DD6  mov eax,[var_4C]           ; return value ignored
00428DDA  lea eax,[eax+eax*2]
00428DDD  lea edx,[eax+eax*8]
00428DE0  mov eax,[current]
00428DE4  mov cx,[eax+4]             ; value read after lookup
00428DE8  lea esi,[action_base+edx*8]
```

两次16位写与refresh：

```text
00428DEF  push esi
00428DF0  mov word ptr [esi+48h],cx
00428DF4  mov word ptr [esi+44h],0
00428DFA  call sub_4321E0
00428E02  test eax,eax
00428E04  jnz loc_428E38
...       nullsub_1("Act Err(Talk:ChangSpd)", action fields)
00428E38  mov ebx,[current]
00428E3C  add word ptr [talk_context+0],6
00428E41  add ebx,6
00428E44  mov esi,1
00428E49  mov [current],ebx
00428E4D  jmp loc_42B0AE
```

`LegacyActionRecord`的layout锁定`wait_remaining`位于`+0x44`、`wait_override`位于`+0x48`，两者均为u16。不能把任一写扩成u32，也不能交换写入或在refresh失败时恢复旧值。

## 角色 selector helper

独立复核`sub_40C0D0`及其调用链：

```text
0040C0D0  mov output,0
0040C0D8  cmp selector_low16,0FFFEh
0040C0E5  mov ecx,dword_4AB378
0040C0EB  mov [output],ecx           ; FFFE -> controlled index
0040C0ED  mov eax,1
...
0040C0F5  call sub_40C100            ; ordinary selector
```

`sub_40C100`总会把`sub_40C020`结果写入output；miss写`0xFFFFFFFF`并返回0。`sub_40C020`从index 0起扫描`dword_49E0C4`个0xD8-byte role：只比较role `+0x24`的u16 GUID，并跳过flags bit28置位的匹配项，返回第一个bit28清的匹配index；无匹配返回`0xFFFFFFFF`。

所以两个特殊值不可合并：

- `0xFFF0`由opcode44自身替换为context source GUID，脚本operand不被自修改；
- `0xFFFE`原样进入helper，直接选择受控index，即使另一个角色的GUID恰好为`0xFFFE`也不会按GUID扫描。

modern复用已经按`0x0040C020/0x0040C0D0`锁定的`resolve_role_index`，不新增不同的查找规则。

## Unsafe点与平台适配

原版ordinary miss把output写成`0xFFFFFFFF`，handler仍计算`action_base - 0xD8`并继续。精确顺序是：

1. 读取selector；
2. 执行lookup并得到missing index；
3. 读取`u16(+4)` value；
4. 第一次写`action[-1]+0x48`时进入越界内存。

modern保留该顺序：先检查/读取selector并lookup，再检查/读取value，最后在首次unsafe action访问点返回`role_not_found`。因此missing selector同时缺value时优先返回`operand_out_of_range`，而不是提前返回`role_not_found`。没有MAPS fallback，也不推进IP、发布previous或调用action update。

原版裸窗口和角色数组始终存在。modern在`+2`或`+4`读取点分别报告`operand_out_of_range`；public VM session在opcode fetch前验证受控角色owner，因此无效controlled index保持opcode/count/IP/previous为原值。`sub_4321E0`通过typed action port接入，零返回只记录诊断。以上均隔离原始越界/裸owner域，有效域行为精确，故分类为`platform_adapted`。

完整6-byte记录可恰好结束于0x8000-byte窗口：位于`0x7FFA`时先执行两次写、一次refresh、IP推进和previous发布；随后下一次fetch才返回`instruction_out_of_range`。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 8
entry_probe_instances   = 8
raw opcode              = 0x002C (8/8)
physical length         = 6 (8/8)
encoding class          = fixed_raw_operands (8/8)
```

文件分布：TALK1/2/3/4=`5/2/0/1`。八条记录为：

| 文件/offset | selector | value |
| --- | ---: | ---: |
| `TALK1.DAT@0x00041D04` | `0x027F` | `0x0000` |
| `TALK1.DAT@0x00041D1C` | `0x0143` | `0x0001` |
| `TALK1.DAT@0x00043ECA` | `0x0143` | `0x0000` |
| `TALK1.DAT@0x00044604` | `0x0143` | `0x0000` |
| `TALK1.DAT@0x00044626` | `0x0019` | `0x0002` |
| `TALK2.DAT@0x00031D62` | `0x0316` | `0x0000` |
| `TALK2.DAT@0x00031FEC` | `0x0316` | `0x0000` |
| `TALK4.DAT@0x00022029` | `0x0027` | `0x0000` |

资产未观察到`0xFFF0`或`0xFFFE` selector；synthetic测试仍覆盖两者。真实回放使用首条：

```text
TALK1.DAT@0x00041D04: 2C 00 7F 02 00 00
```

四文件raw `0x002C`候选为TALK1/2/3/4=`65/20/12/80`，合计177；`0x402C/0x802C/0xC02C`均为0。只有inventory证明的8条作为指令记录。

## 测试覆盖

synthetic与real测试覆盖：

- 四种raw opcode alias；
- ordinary GUID、bit28 skip与首个clear match；
- `0xFFF0` context GUID替换且不自修改operand；
- `0xFFFE`直接选择受控index，即使另一个角色GUID也是`0xFFFE`；
- `wait_override`完整u16写、`wait_remaining`清零、相邻`wait_default`保持、一次action update与无audio；
- action update返回零后保持两次写，记录failure并same-call继续；
- ordinary miss在完整value读取后的checked action边界；
- selector word截断，以及missing lookup后value word截断的优先顺序；
- invalid controlled-session前置边界；
- `0x7FFA`精确窗口尾的side effect、IP、previous及下一fetch失败；
- `TALK1.DAT@0x00041D04`真实六字节记录回放。

首轮编译只暴露测试缺少role-lookup header的直接include，生产代码已编译；补include后第二轮3/3通过，REVIEW补齐两项证明性断言后的第三轮3/3通过且stderr空。

## 双向收敛与分类

LST→C++：FFF0替换、FFFE helper特殊值、u16 GUID/bit28/首匹配、忽略lookup返回、staged value read、两个u16 action写、一次refresh、零返回纯诊断、六字节推进、previous与same-call continuation均一一映射。

C++→LST：没有新增MAPS patch、fallback角色、u32字段写、operand自修改、audio、rollback、yield或额外callback。checked窗口、action[-1]及session边界明确隔离原版unsafe域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
