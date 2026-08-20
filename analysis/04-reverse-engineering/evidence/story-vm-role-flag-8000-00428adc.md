# 剧情 VM 角色 `0x8000` 标记与 one-shot 清理 `0x00428ADC`

## 结论

`sub_427920` 主分派 opcode 39 的唯一入口是 `0x00428ADC`。物理记录固定四字节：

```text
+0  u16 raw opcode
+2  u16 role selector
```

raw selector `0xFFF0`仅在live-role lookup前替换为当前talk context的`source_guid`；`0xFFFE`由共享resolver解释为受控role index。

找到live role时严格按以下顺序执行：

1. 对完整32位`role.flags`执行`OR 0x00008000`；
2. 调用`sub_40AE20(role)`清除该role的surface occupancy；
3. surface调用返回后，才把role `+0x60`与`+0x7C`两个dword写成`0xFFFFFFFF`；
4. IP+4，发布previous opcode并在同一次VM调用中继续。

现代typed layout中`LegacyWorldRoleRecord::action`位于role `+0x40`，`one_shot_base_variant`与`one_shot_variant_delta`分别位于action `+0x20/+0x3C`，因此精确映射机器role `+0x60/+0x7C`。

ordinary lookup miss不报错。机器重读原始`+2`operand并调用`sub_40D460`，仅对MAPS role-source flags执行`OR 0x8000`、`AND 0xFFFF`，随后同样IP+4并继续。raw `0xFFF0`即使先被转换用于lookup，missing fallback的patch GUID仍是原始`0xFFF0`。

## 唯一汇编边界

opcode-specific入口为`0x00428ADC..0x00428B9F`；下一独立handler opcode40位于`0x00428BA0`。

lookup与live path：

```text
00428ADC  mov bx,[ebx+2]
00428AE0  cmp bx,0FFF0h
00428AE5  jnz loc_428AEF
00428AE7  mov edx,[esp+arg_0]
00428AEB  mov bx,[edx+24h]
00428AEF  lea eax,[esp+var_4C]
00428AF3  push eax
00428AF4  push ebx
00428AF5  call sub_40C0D0
00428AFD  test eax,eax
00428AFF  jz loc_428B54

00428B01  mov eax,[esp+var_4C]
...       role stride = index * 0xD8
00428B0E  mov edi,[role+10h]
00428B14  lea ecx,[role]
00428B1A  or edi,8000h
00428B20  push ecx
00428B21  mov [role+10h],edi
00428B27  call sub_40AE20
00428B2C  mov eax,[esp+var_4C]
...       reload role after call
00428B40  or ecx,0FFFFFFFFh
00428B43  mov [role+60h],ecx
00428B49  mov [role+7Ch],ecx
00428B4F  jmp loc_42D182
```

`OR`写回发生在`sub_40AE20`之前；两个one-shot字段写入发生在其返回之后。不得把三项写入合并到surface调用前，也不得像opcode38一样在surface后重新按GUID查找或扫描72个object槽。

missing path：

```text
00428B54  mov edx,[esp+var_50]
00428B58  push 0FFFFh          ; logical_map_id preserve
00428B5D  push 0FFFFh          ; flags_and_mask
00428B62  push 8000h           ; flags_or_mask
00428B67  mov ax,[edx+2]       ; re-read raw selector
00428B6B  push 0FFFFh          ; path_data_id preserve
...       six additional 0xFFFF preserve fields
00428B8E  push eax             ; raw selector
00428B8F  call sub_40D460
00428B94  reload window base
00428B98  add esp,2Ch
00428B9B  jmp loc_42D182
```

`sub_40C0D0`失败时EAX为零，随后`mov ax,[+2]`使传入GUID保持零扩展u16语义。

共享尾：

```text
0042D182  add ebx,4
0042D185  add word ptr [context_ip],4
0042D18E  mov esi,1
0042D193  jmp loc_42B0AE
0042B0BD  mov dword_4CF6D8,effective_opcode
0042B0C8  jmp loc_427B40
```

因此两个正常分支均固定+4、发布effective opcode并同调用继续，不调用audio maintenance，也不yield。

## MAPS typed request

`sub_40D460`的11参数精确映射为：

```text
guid             = raw u16(+2)
action_id        = preserve
base_variant     = preserve
variant_delta    = preserve
tile_x           = preserve
tile_y           = preserve
talk_script_id   = preserve
path_data_id     = preserve
flags_or_mask    = 0x8000
flags_and_mask   = 0xFFFF
logical_map_id   = preserve
```

复用`LegacyMapsRolePatchRequest`与既有`patch_role_source`端口，不增加第二套MAPS owner。

## Unsafe点与平台适配

原版共享resolver对`0xFFFE`直接返回受控role index，后续`[role+0x10]`才是首次role unsafe access。现代VM公共入口已有`controlled_role_index < roles.size()` typed session invariant，invalid owner在dispatch前返回`role_not_found`；handler内此前没有副作用，结果元数据保持未dispatch状态。

live path中flag write先于surface访问。若surface owner不存在，modern checked failure保留已写flag而不写两个one-shot字段。若footprint首cell有效、后续cell越界，已经完成的cell mask也保留，然后在原始surface unsafe点返回`role_surface_failed`；两个one-shot字段、IP与previous仍不发布。

ordinary selector miss才进入MAPS fallback。以上typed surface、MAPS port及checked invalid-domain边界使本handler分类为`platform_adapted`。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 553
entry_probe_instances   = 558
raw opcode               = 0x0027 (553/553)
decoded length           = 4 (553/553)
```

文件分布：

| 文件 | 记录数 |
| --- | ---: |
| `TALK1.DAT` | 120 |
| `TALK2.DAT` | 59 |
| `TALK3.DAT` | 142 |
| `TALK4.DAT` | 232 |

共有224种selector，范围`0x0001..0xEA3C`；`0xFFF0`与`0xFFFE`均为0条。真实回放使用`TALK1.DAT@0x000049F0`，四字节为`27 00 01 00`。在没有live GUID 1的fixture中，它精确产生flags `OR 0x8000`/`AND 0xFFFF` MAPS patch，然后IP+4并同调用抓取下一条。

四文件raw `0x0027`字节候选分别为`1355/546/728/1378`，合计4007；`0x4027/0x8027/0xC027`均为0。只有inventory证明的553条作为指令记录，不把其他byte-word候选扩张为入口。

## 测试覆盖

synthetic与real测试覆盖：

- 四种raw opcode alias；
- ordinary miss的11字段MAPS request，仅flags masks变化；
- synthetic raw `0xFFF0` lookup miss时patch GUID保持`0xFFF0`；
- `0xFFFE`受控owner越界由公共typed session boundary在dispatch前checked stop且不patch；
- live `0xFFF0`在surface owner缺失前已经完成完整32位flags OR，one-shot字段保持；
- footprint第二cell越界时首cell mask与flag保留，one-shot/IP/previous不发布；
- live success清surface后才把两个完整u32 one-shot字段写`0xFFFFFFFF`；
- selector截断时无副作用，完整四字节记录不要求后续字节；
- IP+4、previous发布、无audio与same-call continuation；
- `TALK1.DAT@0x000049F0`真实记录回放。

## 双向收敛与分类

LST→C++：FFF0 lookup替换、shared resolver、完整32位flag OR、surface clear、两个role dword写入、raw-selector MAPS fallback、共享+4尾与common previous均一一映射。

C++→LST：新增行为仅补回机器已有MAPS fallback与previous发布；字段offset由static assertions锁定。operand与surface failure保留机器访问顺序，受控owner越界沿用既有session boundary。没有新增object scan、audio、yield或业务callback。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
