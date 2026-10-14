# 剧情 VM 角色场景清除 `0x004289DE`

## 结论

`sub_427920` 主分派 opcode 38 的唯一入口是 `0x004289DE`。物理记录固定四字节：

```text
+0  u16 raw opcode
+2  u16 role selector
```

handler先把raw selector `0xFFF0`替换为当前talk context的`source_guid`用于live-role lookup；`0xFFFE`仍由共享resolver解释为受控role index。

找到live role时按下列顺序执行：

1. `role.flags &= 0x00007FFF`，清除bit15..31全部高位；
2. `sub_40AE20(role)`清除该role的surface occupancy；
3. 读取role GUID并再次调用`sub_40C020`，取得清flag后第一个同GUID且skip bit已清的role index；
4. 固定扫描72个`0x21C`字节object槽；槽首`u16`等于该重新查得index时，`sub_40DD40`把整个槽的135个dword全部写成`0xFFFFFFFF`；
5. IP+4，发布previous opcode并在同一次VM调用中继续。

ordinary lookup miss不报错：机器重读原始`+2`operand并调用`sub_40D460`，只对MAPS role-source flags执行`AND 0x7FFF`再`OR 0`，随后同样IP+4并继续。因而raw `0xFFF0`即使先被转换为context GUID进行lookup，missing fallback的patch GUID仍是原始`0xFFF0`。

## 唯一汇编边界

opcode-specific入口为`0x004289DE..0x00428ADB`；下一独立handler opcode39位于`0x00428ADC`。主要基本块：

```text
004289DE  mov bx,[ebx+2]
004289E2  cmp bx,0FFF0h
004289E7  jnz loc_4289F1
004289E9  mov ecx,[esp+arg_0]
004289ED  mov bx,[ecx+24h]
004289F1  lea edx,[esp+var_4C]
004289F5  push edx
004289F6  push ebx
004289F7  call sub_40C0D0
004289FF  test eax,eax
00428A01  jz loc_428A93
```

live path：

```text
00428A07  mov eax,[esp+var_4C]
...       role stride = index * 0xD8
00428A14  mov ecx,[role+10h]
00428A1A  and ecx,7FFFh
00428A20  mov [role+10h],ecx
00428A26  lea eax,[role]
00428A2C  push eax
00428A2D  call sub_40AE20
...       read word [role+24h] GUID
00428A45  call sub_40C020
00428A4D  mov esi,eax
00428A4F  xor eax,eax
00428A55  ; 72-slot loop
...       read zero-extended word [slot+0]
00428A6F  cmp edx,esi
00428A71  jnz loc_428A80
00428A73  push slot
00428A74  call sub_40DD40
00428A80  inc eax
00428A81  cmp eax,48h
00428A88  jb loc_428A55
00428A8E  jmp loc_42D182
```

`sub_40C020`发生在flag mask和surface clear之后，且返回的完整32位index而非最初`var_4C`被保存在`ESI`；槽首word先零扩展到32位再与`ESI`比较。不得把replacement index截成低16位，不得把loop简化为“清最初role index的object”，也不得只清第一个匹配槽。

missing path：

```text
00428A93  mov eax,[esp+var_50]  ; original window base
00428A97  push 0FFFFh           ; logical_map_id preserve
00428A9C  push 7FFFh            ; flags_and_mask
00428AA1  push 0                ; flags_or_mask
00428AA3  mov cx,[eax+2]        ; re-read raw selector
00428AA7  push 0FFFFh           ; path_data_id preserve
...       six additional 0xFFFF preserve fields
00428ACA  push ecx              ; raw selector
00428ACB  call sub_40D460
00428AD7  jmp loc_42D182
```

`loc_42D182`固定执行IP+4、`ESI=1`并进入common join。

## Resolver与unsafe点

`sub_40C0D0`对`0xFFFE`无条件返回受控role index；原版直到后续`[role+0x10]`才发生首次unsafe dereference。现代VM公共入口已有typed session invariant：`controlled_role_index >= roles.size()`在dispatch前返回`role_not_found`。该invalid-domain adaptation没有跳过任何handler内前置副作用，但结果元数据保持未dispatch状态（opcode与执行数均为0）；它不调用MAPS patch、不推进IP、不发布previous。valid controlled index仍由opcode38按机器顺序执行。

ordinary selector miss才进入MAPS fallback。live path中flag write先于surface occupancy访问；若modern runtime缺少surface owner或footprint越界，已完成的flag mask保留，随后在原始surface unsafe点返回checked failure。object槽由固定extent 72的typed span持有，每个槽完整保留原版`0x21C`字节，所以`sub_40DD40`可精确映射为整槽fill `0xFF`。

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
flags_or_mask    = 0x0000
flags_and_mask   = 0x7FFF
logical_map_id   = preserve
```

复用`LegacyMapsRolePatchRequest`与`patch_role_source`端口，不增加第二套MAPS owner。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 786
entry_probe_instances   = 790
raw opcode               = 0x0026 (786/786)
decoded length           = 4 (786/786)
```

文件分布：

| 文件 | 记录数 |
| --- | ---: |
| `TALK1.DAT` | 191 |
| `TALK2.DAT` | 113 |
| `TALK3.DAT` | 159 |
| `TALK4.DAT` | 323 |

共有396种selector；`0xFFF0`出现58次，`0xFFFE`未出现。真实回放使用`TALK1.DAT@0x00004656`：raw opcode `0x0026`、selector `0x0001`。在没有live GUID 1的fixture中，它精确产生flags `AND 0x7FFF`/`OR 0` MAPS patch，然后IP+4并同调用抓取下一条。

四个TALK文件的raw `0x0026`双字节出现数为`616/218/305/533`；只有inventory证明的786条被作为指令记录，不把其他byte-word候选扩张为入口。

## 测试覆盖

synthetic与real测试覆盖：

- 四种raw opcode alias；
- ordinary miss的11字段MAPS request，仅flags masks变化；
- raw `0xFFF0` lookup miss时patch GUID保持`0xFFF0`；
- `0xFFFE`受控owner越界由公共typed session boundary在dispatch前checked stop且不patch；
- live `0xFFF0`在surface owner缺失前已经完成`flags &= 0x7FFF`；
- live受控role清flag与surface occupancy；
- 清flag后同GUID first-match重新lookup，而非沿用初次index；
- 72槽首尾匹配都被完整fill `0xFF`，不匹配槽保持；
- replacement index=`0x10000`时不与object word `0x0000`发生低16位别名；
- selector截断时无任何副作用，完整四字节记录不要求后续字节；
- IP+4、previous发布与same-call continuation；
- `TALK1.DAT@0x00004656`真实记录回放。

## 双向收敛与分类

LST→C++：FFF0 lookup替换、shared resolver、全高位flag mask、surface clear、同GUID重查、object u16对replacement u32的完整宽度比较、72槽整槽清空、raw-selector MAPS fallback、共享+4尾与common previous均有一一映射。

C++→LST：新增端口只复用既有MAPS patch owner；operand与surface failure保留原始访问顺序。受控owner越界由既有VM session boundary提前隔离，且handler内没有此前副作用。没有新增audio、yield或action callback。

由于raw surface访问与MAPS patch被typed owner及checked failure替代，本handler归类为`platform_adapted`：

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
