# 剧情 VM 角色重定位与路径完成 `0x00428BA0`

## 结论

`sub_427920` 主分派 opcode 40 的唯一入口是 `0x00428BA0`。物理记录固定八字节：

```text
+0  u16 raw opcode
+2  u16 raw role selector
+4  u16 tile x
+6  u16 tile y
```

该handler不把`0xFFF0`替换成当前talk context GUID。raw selector原样传给共享resolver；`0xFFFE`仍由resolver解释为受控role index。

找到live role后，机器严格执行：

1. role lookup完成后才读取`+6` tile Y，再读取`+4` tile X；
2. 两个tile分量分别在16位宽度内左移四位，保留`u16`回绕；
3. 调用`sub_42DAF0(role_index, world_x, world_y, 1, -1, -1, -1)`；
4. 返回后立即调用`sub_42D920(role_index)`；
5. 清role完整32位flags的bit31；
6. 重读raw selector；仅当它等于talk context `source_guid`时，把role `+0x4C`与`+0x78`两个dword写成`0xFFFFFFFF`；
7. IP+8，发布previous opcode，并在同一次VM调用中继续。

现代typed layout中role `+0x4C/+0x78`分别映射`action.cached_base_variant`与`action.cached_variant_delta`。`schedule_legacy_world_story_path`对应`sub_42DAF0`，`complete_legacy_world_story_path`对应`sub_42D920`。不得复用`release_legacy_world_story_role_path`，因为该wrapper还会清`action.wait_remaining`，机器opcode40没有这项写入。

ordinary lookup miss不报错。机器读取原始tile Y/X与selector，调用`sub_40D460`只更新MAPS role-source的坐标；其余字段保持，flags masks为`OR 0`、`AND 0xFFFF`。随后同样IP+8、发布previous并继续。

## 唯一汇编边界

opcode-specific入口为`0x00428BA0..0x00428C9A`；下一独立handler opcode41位于`0x00428C9F`。

lookup及live helper顺序：

```text
00428BA0  mov dx,[ebx+2]
00428BA4  lea ecx,[var_role_index]
00428BA8  push ecx
00428BA9  push edx
00428BAA  call sub_40C0D0
00428BB2  test eax,eax
00428BB4  jz loc_428C45

00428BBA  mov eax,[window_base]
00428BBE  or esi,0FFFFFFFFh
00428BC1  push esi                  ; variant_delta = -1
00428BC2  push esi                  ; base_variant = -1
00428BC3  mov cx,[eax+6]            ; tile y first
00428BC7  mov dx,[eax+4]            ; then tile x
00428BCF  push esi                  ; action_id = -1
00428BD0  shl cx,4                  ; 16-bit wrap
00428BD4  push 1
00428BD6  push ecx                  ; world y
00428BD7  shl dx,4                  ; 16-bit wrap
00428BDB  push edx                  ; world x
00428BDC  push role_index
00428BDD  call sub_42DAF0
00428BE2  push role_index
00428BE7  call sub_42D920
```

两个helper的返回值在机器中不参与分支。调用返回后按role stride重载role，并执行caller尾：

```text
00428BEC  mov eax,[var_role_index]
...       role stride = index * 0xD8
00428C03  mov ebx,[role+10h]
00428C09  and ebx,7FFFFFFFh
00428C0F  mov [role+10h],ebx
00428C15  mov ebx,[window_base]
00428C19  mov dx,[ebx+2]            ; re-read raw selector
00428C1D  cmp dx,[context+24h]
00428C21  jnz loc_428C89
00428C23  mov [role+4Ch],0FFFFFFFFh
00428C29  mov [role+78h],0FFFFFFFFh
00428C2F  add word ptr [context_ip],8
00428C34  add window_ptr,8
00428C3B  mov esi,1
00428C40  jmp loc_42B0AE
```

missing path的11参数：

```text
00428C45  mov eax,[window_base]
00428C49  push 0FFFFh               ; logical_map_id preserve
00428C4E  push 0FFFFh               ; flags_and_mask
00428C53  push 0                     ; flags_or_mask
00428C55  mov dx,[eax+6]            ; tile y
00428C59  mov cx,[eax+4]            ; tile x
00428C5D  push 0FFFFh               ; path_data_id preserve
00428C62  push 0FFFFh               ; talk_script_id preserve
00428C67  push edx                   ; raw tile y
00428C68  mov dx,[eax+2]            ; raw selector
00428C6C  push ecx                   ; raw tile x
00428C6D  push 0FFFFh               ; variant_delta preserve
00428C72  push 0FFFFh               ; base_variant preserve
00428C77  push 0FFFFh               ; action_id preserve
00428C7C  push edx                   ; raw selector
00428C7D  call sub_40D460
00428C89  add word ptr [context_ip],8
00428C8E  add window_ptr,8
00428C95  mov esi,1
00428C9A  jmp loc_42B0AE
```

两个正常尾都到`loc_42B0AE`，随后`0x0042B0BD`把effective opcode写入`dword_4CF6D8`并继续抓取下一条。因此没有audio maintenance或yield。

## MAPS typed request

`sub_40D460`的11参数精确映射为：

```text
guid             = raw u16(+2)
action_id        = preserve
base_variant     = preserve
variant_delta    = preserve
tile_x           = raw u16(+4)
tile_y           = raw u16(+6)
talk_script_id   = preserve
path_data_id     = preserve
flags_or_mask    = 0
flags_and_mask   = 0xFFFF
logical_map_id   = preserve
```

MAPS fallback保存的是脚本tile单位，不执行live path的`<<4`。复用`LegacyMapsRolePatchRequest`与既有`patch_role_source`端口。

## Unsafe点与平台适配

原版共享resolver对`0xFFFE`直接返回受控role index，首次role unsafe access发生在helper内部。现代VM公共入口已有`controlled_role_index < roles.size()` typed session invariant；invalid owner在dispatch前返回`role_not_found`，opcode/count保持未dispatch状态。

机器直接访问脚本窗口；modern checked window在selector lookup后验证完整八字节，再按机器顺序读取`+6`与`+4`。截断会在原始operand unsafe点返回`operand_out_of_range`，不调用story-path helper、不发MAPS patch、不推进IP/previous。完整八字节记录不要求后续字节；副作用与previous发布先完成，下一次fetch才可失败。

原版两个story-path helper使用全局owner且忽略返回。modern缺少`story_paths` owner时在原始helper调用点返回`runtime_unavailable`；owner存在但路径节点/方向越界或其他typed helper内部checked失败时返回`role_path_failed`。此前helper已完成的写入不会回滚；caller的bit31/cache/IP/previous尾只在两个helper均完成后执行。这些仅隔离原版未定义域。

`complete_legacy_world_story_path`保留`sub_42D920`的type-2槽完成与普通路径恢复语义。测试用无saved path的type-2槽证明该槽被释放，同时`action.wait_remaining`保持不变，排除误用额外清wait的release wrapper。

以上typed owner、checked invalid-domain边界与MAPS port使本handler分类为`platform_adapted`。

## 真实资产审计

锁定inventory观察到：

```text
unique_physical_records = 222
entry_probe_instances   = 222
raw opcode               = 0x0028 (222/222)
decoded length           = 8 (222/222)
```

文件分布：

| 文件 | 记录数 |
| --- | ---: |
| `TALK1.DAT` | 58 |
| `TALK2.DAT` | 39 |
| `TALK3.DAT` | 55 |
| `TALK4.DAT` | 70 |

共有91种selector，范围`0x0001..0x752B`；`0xFFF0`与`0xFFFE`均为0条。tile X有85种，范围`0x0005..0x0092`；tile Y有79种，范围`0x0001..0x0078`。

真实回放使用`TALK1.DAT@0x0000464E`，八字节为：

```text
28 00 01 00 24 00 21 00
```

即selector 1、tile `(0x24,0x21)`。在没有live GUID 1的fixture中，它精确产生raw tile MAPS patch，然后IP+8并同调用抓取下一条。

四文件raw `0x0028`字节候选分别为`295/67/132/331`，合计825；`0x4028/0x8028/0xC028`均为0。只有inventory证明的222条作为指令记录，不把其他byte-word候选扩张为入口。

## 测试覆盖

synthetic与real测试覆盖：

- 四种raw opcode alias；
- ordinary miss的11字段MAPS request，raw tile坐标不左移且flags为`OR 0`/`AND 0xFFFF`；
- synthetic raw `0xFFF0` lookup miss时patch GUID保持`0xFFF0`，以及真实GUID `0xFFF0`可被literal lookup命中；
- `0xFFFE`受控owner越界由公共typed session boundary在dispatch前checked stop且不patch；
- live tile `0x1015/0x100F`分别以16位左移回绕到world `(336,240)`；
- `sub_42DAF0`后立即执行`sub_42D920`，无saved path的type-2槽被释放，bit31随后由caller清除；
- raw selector等于source GUID时两个完整u32 cache字段写`0xFFFFFFFF`；raw `0xFFFE`不等于source GUID时cache保持；
- `action.wait_remaining`保持，排除额外release wrapper；
- typed schedule runtime failure不推进IP/previous；
- found/missing lookup后的截断、`+4`存在但`+6`缺失，以及完整八字节窗口尾；
- IP+8、previous发布、无audio与same-call continuation；
- `TALK1.DAT@0x0000464E`真实记录回放。

## 双向收敛与分类

LST→C++：raw selector lookup、lookup后Y/X读取、两个16位shift、`sub_42DAF0`七参数、紧邻`sub_42D920`、bit31清除、raw-selector条件cache reset、11参数MAPS fallback、两个+8尾与common previous均一一映射。

C++→LST：新增行为只替换原先把两个helper合并近似的私有实现，复用已审计typed helper与MAPS端口；没有清`wait_remaining`、FFF0替换、audio、yield或额外业务callback。所有checked stop都位于原始脚本/owner/helper unsafe点。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
