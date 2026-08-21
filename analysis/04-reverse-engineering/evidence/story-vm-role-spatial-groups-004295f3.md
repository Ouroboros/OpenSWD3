# 剧情 VM 角色空间分组切换 `0x004295F3`

## 结论

`sub_427920` 的共享入口 `0x004295F3`承载opcode55、56、57。三条指令长度均为4字节：

```text
+0 u16 raw opcode
+2 u16 role selector
```

它们先保存角色flags低两位旧分组，再清低两位，并分别写入新分组：

| opcode | 新分组 |
| ---: | ---: |
| 55 | 1 |
| 56 | 0 |
| 57 | 2 |

随后以旧分组解链，并按已经写入flags的新分组重插空间行链。正常完成后推进4字节、发布normalized previous，并跨帧让出；不在同调用继续下一条。

## selector 与原unchecked失败点

```text
004295F3  mov bx,[current+2]
004295F7  cmp bx,0FFF0h
004295FC  jnz 00429606
004295FE  mov edx,[context]
00429602  mov bx,[edx+24h]
00429606  lea eax,lookup_index
0042960A  push eax
0042960B  push ebx
0042960C  call sub_40C0D0
```

`0xFFF0`由handler替换为talk context `+0x24`的source GUID；`0xFFFE`保留给lookup helper选择受控角色。普通selector使用helper的首个有效GUID匹配规则。

原版不检查lookup返回。missing selector产生index `-1`，随后在`0x00429625`第一次读取角色flags时进入数组前内存。modern在同一首次角色访问边界返回`role_not_found`；不会修改任一角色flags、IP或previous，也不伪造MAPS fallback。

## 先保存旧分组，再写新分组

角色stride为`0xD8`，flags位于角色`+0x10`：

```text
00429625  mov ecx,[role+10h]
0042962B  mov edx,ecx
0042962D  and ecx,0FFFFFFFCh
00429630  and edx,3
00429633  sub edi,37h
00429636  mov saved_old_group,edx
0042963A  mov [role+10h],ecx
```

`EDI`是normalized opcode。入口先执行一次完整dword低两位清除写，再按`opcode-55`选择：

```text
opcode55: cleared | 1
opcode56: cleared | 0
opcode57: cleared | 2
```

高30位完全保留。空间helper接收的是清除前保存的旧分组，而角色结构在调用前已经持有新分组。这一顺序使helper先从旧分组解链，再由`sub_411490(role, role.flags & 3)`按新分组重插。

## logical Y 起始行与helper参数

写完新flags后，入口读取角色`world_y`与u16 GUID：

```text
00429656  mov ecx,[role+08h]
0042965C  push 0
0042965E  shr ecx,4
00429661  dec ecx
00429662  push ecx
00429663  push saved_old_group
00429666  mov dx,[role+24h]
0042966D  push edx
0042966E  call sub_411530
```

物理调用参数为：

```text
(guid_u32, old_group_u32, first_row_i32, mode=0)
```

`first_row_bits = (world_y_u32 >> 4) - 1`使用logical shift与32位回绕，再按原helper的signed比较解释。它不是对signed Y做算术右移，也不是向零除16。测试以`world_y=0xFFFFFFFF`固定这一旧行为：角色能按既有插入规则位于第0行，但handler的logical起始行成为巨大正值，`sub_411530`不扫描链，flags仍切换而角色继续留在旧链。

末参数0要求解链后重插；现代`relocate_legacy_role_spatially_by_guid(..., reinsert=true)`对应这一合同。helper在旧链找不到GUID时只调用原版诊断并返回0，caller完全忽略返回，因此handler仍推进、发布previous并让出，flags保持新分组而链不变。

## 平台安全边界

有效空间链上，modern必须完成旧组解链、新组重插，并保留原链排序。以下只隔离原版裸指针损坏域：

- `spatial_index` owner缺失：flags已经写为新分组，随后在原helper调用点返回`runtime_unavailable`；
- 旧分组3、行首越界或损坏链：flags写入保留，返回`role_spatial_relocation_failed`；
- 已完成旧组解链但新组重插失败：不回滚解链、`spatial_next_link_32`清零和新flags，返回`role_spatial_relocation_failed`。

这些typed-stop路径不推进IP、不发布previous。`LegacyRoleSpatialRelocationStatus::role_not_found`不是损坏边界，必须按原caller继续。

## IP、previous 与精确尾

三个分支最终都跳到：

```text
0042C7E6  mov ebx,current
0042C7EA  add ebx,4
0042C7ED  add word ptr [context_ip],4
0042C7F2  mov current,ebx
0042C7F6  jmp 0042B0AE
```

入口没有把ESI设为1。共同join先把normalized opcode写入`dword_4CF6D8`，随后因`var_28 | ESI == 0`走返回路径。因此：

- 成功路径推进4字节；
- 发布normalized previous 55/56/57；
- 返回`yielded`；
- 从`0x7FFC`开始的完整记录可把IP推进到`0x8000`，完成flags与空间链迁移后正常让出，不进行下一fetch。

## 真实资产

锁定目录只有4条物理记录、4个entry probe，全部位于`TALK4.DAT`，raw均为低位形式且长度4：

| opcode | 文件偏移 | selector | 字节 |
| ---: | ---: | ---: | --- |
| 55 | `0x000121DA` | 322 | `37 00 42 01` |
| 56 | `0x00012B12` | 322 | `38 00 42 01` |
| 57 | `0x00005084` | 701 | `39 00 BD 02` |
| 57 | `0x0000526B` | 702 | `39 00 BE 02` |

四文件逐字节原始字样计数：

| raw word | 次数 | decoded入口 |
| --- | ---: | ---: |
| `0x0037` | 97 | 1 |
| `0x4037` | 0 | 0 |
| `0x8037` | 0 | 0 |
| `0xC037` | 4 | 0 |
| `0x0038` | 287 | 1 |
| `0x4038/0x8038/0xC038` | 0 | 0 |
| `0x0039` | 124 | 2 |
| `0x4039` | 1 | 0 |
| `0x8039/0xC039` | 0 | 0 |

因此高位alias只作synthetic protocol覆盖，不能把5个原始高位字样伪报为资产指令。

## 测试覆盖

- 三个opcode×四种raw alias，分别把旧分组0/2/1迁移到新分组1/0/2；
- 高30位保留、旧组行首清除、新组行首重插、normalized previous与跨帧yield；
- `FFF0` source GUID与`FFFE`受控角色；
- 有效角色不在旧空间链时只诊断并继续；
- `world_y=0xFFFFFFFF`的logical shift旧行为；
- selector截断、missing role首次flags访问边界；
- owner缺失、旧分组3、解链后重插失败及不回滚；
- `0x7FFC`精确尾推进到`0x8000`后让出；
- `TALK4.DAT`四条真实记录逐条回放。

## 双向收敛与分类

LST→C++：selector翻译、旧分组保存、先清后写的新分组三分支、logical Y起始行、u16 GUID、旧组解链/新组重插、ignored not-found、4字节推进、previous和yield均有直接映射。

C++→LST：没有完整记录外的operand预读、MAPS fallback、把新分组误传为旧组、signed Y右移、helper not-found提前失败、同调用继续、漏发previous或重插失败回滚。typed owner/损坏链状态只隔离原版无效裸指针域并保留此前flags/解链效果。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
