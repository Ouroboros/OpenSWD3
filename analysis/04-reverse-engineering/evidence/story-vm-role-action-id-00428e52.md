# 剧情 VM 角色 action id 切换 `0x00428E52`

## 结论

`sub_427920` 主分派 opcode 45 的唯一入口是 `0x00428E52`。物理记录固定为 6 字节：

```text
+0  u16 raw opcode
+2  u16 role selector
+4  u16 requested action id
```

handler先读取selector；raw `0xFFF0`只在本条handler内替换为当前TALK context `+0x24`的source GUID，脚本operand不被改写。随后调用`sub_40C0D0`：`0xFFFE`直接选择受控角色index，ordinary selector按u16 GUID查找第一个bit28清的角色。

角色存在时，`u16(+4)`先零扩展并完整写入action `+0x00`的u32 `action_id`。然后`sub_42E740`强制读取尚未消费的下一条raw opcode：只有精确raw `0x000A/0x000B/0x002D`才继续读取下一selector并解析角色；若解析到同一role index，本条跳过`sub_4321E0`，否则立即刷新action。refresh零返回只触发诊断，不回滚写入。最后对角色完整u32 flags OR `0x00001000`，推进6字节、发布previous并在同一次VM调用继续。

角色不存在时不访问live action，也不执行lookahead或refresh，而是调用`sub_40D460`，只把MAPS源记录的action id改为`u16(+4)`并对flags OR `0x1000`；其他字段保持。patch helper失败只诊断，caller仍推进并继续。

## 唯一汇编边界

opcode-specific入口为`0x00428E52..0x00428F76`；顺序尾复用`0x00428E38..0x00428E4D`，下一handler opcode46–49从`0x00428F7B`开始。

selector、lookup与found/missing分流：

```text
00428E52  mov bx,[current+2]
00428E56  cmp bx,0FFF0h
00428E5D  mov ecx,[talk_context]
00428E61  mov bx,[ecx+24h]           ; FFF0 -> context source GUID
00428E65  lea edx,[var_4C]
00428E69  push edx
00428E6A  push ebx
00428E6B  call sub_40C0D0
00428E73  cmp eax,1
00428E76  jnz loc_428F37             ; missing -> MAPS fallback
```

found path的u32写、lookahead、refresh与flags：

```text
00428E7C  mov eax,[current]
00428E80  cmp word ptr [eax+4],0     ; zero only emits diagnostic
...
00428EA2  mov eax,[role_index]
00428EA6  xor edx,edx
00428EAE  mov eax,[current]
00428EB2  mov dx,[eax+4]             ; zero-extend full u16
00428EB6  lea esi,[action_base+index*D8h]
00428EBD  mov [esi],edx              ; full u32 action_id write
00428EBF  mov edx,[current]
00428EC3  add edx,6                  ; next instruction, current IP unchanged
00428ECA  push role_index
00428ECB  push edx
00428ECC  call sub_42E740
00428ED4  test eax,eax
00428ED6  jnz loc_428F17             ; same role -> skip refresh
00428ED8  push esi
00428ED9  call sub_4321E0
00428EE1  test eax,eax               ; zero only emits diagnostic
...
00428F17  mov eax,[role_index]
00428F21  mov ecx,[role_flags]
00428F28  or ch,10h                  ; full u32 flags |= 0x1000
00428F2B  mov [role_flags],ecx
00428F32  jmp loc_428E38
```

共享六字节顺序尾：

```text
00428E38  mov ebx,[current]
00428E3C  add word ptr [talk_context+0],6
00428E41  add ebx,6
00428E44  mov esi,1
00428E49  mov [current],ebx
00428E4D  jmp loc_42B0AE             ; common previous publication
```

`action_id`写是零扩展后的完整u32，不是只覆盖低16位。`or ch,10h`等价于对完整角色flags OR `0x00001000`，保留其他31位。

## `sub_42E740` lookahead合同

helper精确边界为`0x0042E740..0x0042E783`：

```text
0042E740  mov ecx,[next_instruction]
0042E744  mov ax,[ecx]               ; found path无条件读取next raw opcode
0042E747  cmp ax,000Ah
0042E74D  cmp ax,000Bh
0042E753  cmp ax,002Dh
0042E757  jnz return_false
0042E759  mov cx,[ecx+2]             ; 仅三个精确raw opcode读取selector
0042E762  push ecx
0042E763  call sub_40C0D0
0042E76B  test eax,eax
0042E76D  jz return_false
0042E76F  mov edx,[resolved_index]
0042E773  mov eax,[current_role_index]
0042E777  cmp edx,eax
0042E77B  mov eax,1                  ; same resolved role
0042E781  xor eax,eax                ; refresh required
```

因此它不是按分派后的effective opcode判断：`0x400A/0x800B/0xC02D`均不合并。它也不执行opcode45自身的`0xFFF0`替换；next selector `0xFFF0`作为ordinary GUID进入`sub_40C0D0`。`0xFFFE`仍由helper直接解析为受控index。

found path即使当前6-byte记录恰好完整结束，也必须继续读取next raw opcode。若next raw opcode是10/11/45，还必须读取其selector；这两个读取都发生在refresh、flags、当前IP推进和previous发布之前。modern用三态checked helper保留该unsafe顺序：缺next opcode或缺recognized-next selector返回`operand_out_of_range`，并保留已完成的action_id写，但不执行后续效果。opcode10/11共享同一个原版helper，modern调用点也使用相同三态合同。

## Missing MAPS fallback

missing分支在lookup失败后才读取`u16(+4)`：

```text
00428F37  mov edx,[current]
00428F3B  push FFFFh                 ; logical map id preserve
00428F40  push FFFFh                 ; flags AND preserve
00428F45  push 1000h                 ; flags OR
00428F4A  mov ax,[edx+4]
00428F4E  push FFFFh                 ; remaining fields preserve
...
00428F6C  push eax                   ; action id
00428F6D  push ebx                   ; translated/current selector
00428F6E  call sub_40D460
00428F73  add esp,2Ch
00428F76  jmp loc_428E38
```

`sub_40D460`扫描MAPS源记录直到首word `0xFFFF`，按记录`+2`的u16 GUID匹配。该调用只写记录`+4` action id，并对记录`+0x14` flags OR `0x1000`；其余参数均为`0xFFFF`保持值。helper返回值被caller忽略。modern通过`LegacyMapsRolePatchRequest`表达同一字段组合，不为missing live role虚构角色或action refresh。

## Unsafe点与平台适配

有效域顺序分为两条：

1. found：selector读取→lookup→action id读取→u32 action写→next opcode读取→必要时next selector/lookup→可选refresh→flags OR→IP/previous；
2. missing：selector读取→lookup失败→action id读取→MAPS patch→IP/previous。

modern按原始访问点隔离裸窗口：

- `+2`不完整时在selector访问点返回`operand_out_of_range`；
- found或missing的`+4`不完整时都发生在lookup之后，且不写action、不patch MAPS；
- found的完整记录位于`0x7FFA`时，action id先写入，随后next-opcode访问失败；不refresh、不置flags、不推进IP、不发布previous；
- next raw opcode位于`0x7FFE`且为10/11/45时，selector访问失败，保留action id写但没有后续效果；
- next raw opcode不是10/11/45时只需要该2-byte opcode，不额外读取selector；
- missing完整记录位于`0x7FFA`时不执行lookahead，先patch、推进和发布previous，下一次fetch才返回`instruction_out_of_range`；
- public VM session在opcode fetch前验证受控角色owner，无效owner保持opcode/count/IP/previous不变。

以上checked边界只隔离原版裸内存与owner域；有效domain的字段宽度、回调次数、lookahead顺序和same-call continuation不变，故分类为`platform_adapted`。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 65
entry_probe_instances   = 68
raw opcode              = 0x002D (65/65)
physical length         = 6 (65/65)
encoding class          = fixed_raw_operands (65/65)
```

文件分布：TALK1/2/3/4=`47/1/14/3`。selector共11种：`0x0003` 40条、`0x0001` 13条、`0x0010` 4条，其余8种各1条；未观察到`0xFFF0`或`0xFFFE`。action id均非零，其中`0x0001` 46条。

真实lookahead中，紧邻下一raw opcode为10共9条（7条同selector、2条不同），为11共2条（均同selector），为45共3条（均不同selector）。因此资产实际覆盖9次same-role refresh合并，也覆盖5次“recognized opcode但不同角色”立即refresh。

真实回放使用首条：

```text
TALK1.DAT@0x000051C9: 2D 00 F8 00 23 02
```

即selector `0x00F8`、action id `0x0223`。四文件raw `0x002D`字节候选为TALK1/2/3/4=`134/22/50/112`，合计318；`0x402D/0x802D/0xC02D`均为0。只有inventory证明的65条作为指令记录。

## 测试覆盖

synthetic与real测试覆盖：

- 四种current raw opcode alias；
- ordinary GUID、bit28 skip与首个clear match；
- current `0xFFF0`替换且operand不自修改；current/next `0xFFFE`受控角色选择；
- action id从u16零扩展为完整u32、零值仍写入、flags完整OR `0x1000`与无audio；
- exact raw same-role 45→10/11/45合并、next alias不合并、next `0xFFF0`不替换；
- refresh失败只记录diagnostic，仍置flags、推进、发布previous并继续；
- missing role的精确MAPS action/flags patch；
- selector截断、found/missing在lookup后的action-id截断；
- found `0x7FFA`精确尾的action写后next-opcode失败；recognized next opcode的selector截断；unrecognized next opcode无需selector；
- missing `0x7FFA`精确尾先patch、推进和发布previous；
- invalid controlled-session前置边界；
- `TALK1.DAT@0x000051C9`真实六字节记录回放。

最终剧情 VM定向目标及三条CTest路径3/3通过且stderr为空；Linux core 186/186、Linux app 192/192均通过，app只保留既有ALSA开发库能力warning。Windows门禁依计划留到P3，本阶段未运行；未启动游戏EXE。

## 双向收敛与分类

LST→C++：current FFF0替换、FFFE helper选择、u16 GUID/bit28/首匹配、lookup后staged action-id读取、u16→u32零扩展写、精确raw lookahead、同role合并、refresh零返回纯诊断、flags OR、missing MAPS patch、六字节推进、previous与same-call continuation均一一映射。

C++→LST：没有新增next alias归一化、next FFF0替换、fallback live role、额外refresh、audio、rollback或yield。checked窗口与session边界明确隔离原版unsafe域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
