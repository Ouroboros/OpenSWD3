# 剧情 VM 图片动作创建 `0x0042B1F1`

## 结论

`sub_427920` 的共享入口 `0x0042B1F1`同时承载主表opcode58与次表opcode153。两条指令布局和长度完全相同：

```text
+0 u16 raw opcode
+2 u16 screen_x
+4 u16 screen_y
+6 u16 action_id
+8 u16 base_variant
```

入口先分配并清零一个`0xA4`字节节点，再初始化节点`+8`处的`0x98`字节动作记录。四个操作数随后按`+2/+4/+6/+8`逐word读取和写入。全部完成后，opcode58把节点前插到主图片动作链，opcode153前插到次图片动作链。两条路径都推进10字节、发布normalized previous并跨帧让出。

## 分配、全清零与动作初始化顺序

入口第一项行为不是读取操作数，而是分配节点：

```text
0042B1F1  push 0A4h
0042B1F6  call sub_487C10
0042B1FB  mov  ebx,eax
0042B1FD  mov  ecx,29h
0042B202  xor  eax,eax
0042B204  mov  edi,ebx
0042B206  rep stosd
0042B208  lea  edi,[ebx+8]
0042B20B  push edi
0042B20C  call sub_40DC00
0042B215  add  esp,8
```

`0x29`个dword正好是`0xA4`字节。清零范围覆盖节点头、内嵌动作记录以及末尾next指针。`sub_40DC00`只覆盖动作记录中的八个初始化字段：

- `+0x1C/+0x20/+0x3C = 0xFFFFFFFF`；
- `+0x42/+0x44/+0x46/+0x48`的四个word清零；
- `+0x90`的dword清零。

因此其余字段继续保持前一步全零结果。modern使用一个尚未链接的临时`std::list<LegacyPictureActionNode>`节点执行同一次分配，并调用既有`initialize_legacy_action_record`恢复这八项初始化。

## 四个操作数是分阶段访问

初始化返回后，入口严格按顺序读取并立即写入：

```text
0042B218  mov ax,[current+2]
0042B21C  mov [ebx+0],ax
0042B223  xor eax,eax
0042B225  mov dx,[current+4]
0042B229  mov [ebx+2],dx
0042B231  xor edx,edx
0042B233  cmp word ptr [normalized_opcode],3Ah
0042B239  mov ax,[current+6]
0042B23D  mov [action+0],eax
0042B243  mov dx,[current+8]
0042B247  mov [action+8],edx
```

前两项保持`u16`宽度；后两项先零扩展为dword，分别写到内嵌动作记录的`action_id`与`base_variant`。入口没有先验证完整10字节记录，也没有在读取操作数前访问链表头。

modern据此在分配和初始化之后依次检查4、6、8、10字节边界。任何operand尾失败都不推进IP、不发布previous，也不能把未完成节点提前挂入目标链。由于原程序在进程故障时会遗留未链接堆块，而typed-stop必须返回调用者，modern在这类受检失败上由临时list析构释放节点；这是无效域的最小资源安全适配，不改变任何完整记录行为。

## 主链与次链归属

写完前两项坐标后，入口比较normalized opcode并保留CPU条件码；随后读取、写入`+6/+8`两项动作参数，期间的`mov`不改变条件码。第四项完成后才按先前比较结果分支并访问链头：

```text
0042B233  cmp word ptr [normalized_opcode],3Ah
0042B239  mov ax,[current+6]
0042B23D  mov [action+0],eax
0042B243  mov dx,[current+8]
0042B247  mov [action+8],edx
0042B24A  jnz 0042B260
```

modern同样在写完第二项后保存主/次目标判定，但把目标容器访问延迟到第四项完成之后。opcode58前插到`dword_4B7C70`：

```text
0042B24C  mov ecx,dword_4B7C70
0042B252  mov [ebx+0A0h],ecx
0042B258  mov dword_4B7C70,ebx
```

其他到达本共享入口的已锁定分派值只有opcode153，前插到`dword_4B8968`：

```text
0042B260  mov edx,dword_4B8968
0042B266  mov [ebx+0A0h],edx
0042B26C  mov dword_4B8968,ebx
```

原版节点末尾`+0xA0`保存旧链头，形成单向前插链。modern的`LegacyPictureActionLists::primary/secondary`使用标准list保存逻辑链序；临时节点通过`splice(destination.begin(), pending)`无二次分配地成为新表头。结构内保留的32位兼容指针继续为零，真实现代链路由容器维护。

## 分配与owner安全边界

原版没有检查`sub_487C10`的返回值；分配失败后会立即在`rep stosd`处写空指针。modern在同一分配边界把`std::bad_alloc/std::length_error`隔离为`picture_action_allocation_failed`，不读取操作数、不推进IP、不发布previous。

原版两个全局链头始终存在。modern的`picture_actions` owner是平台对象，因此只在四个操作数均已写入临时节点后检查。owner缺失返回`runtime_unavailable`，不链接节点、不推进IP、不发布previous；临时节点按上述安全适配释放。不得把owner检查前移到分配、初始化或operand读取之前。

## IP、previous 与精确尾

两条链路最终汇合：

```text
0042B276  add current,0Ah
0042B279  add word ptr [context_ip],0Ah
0042B27E  mov [current_local],current
0042B282  jmp 0042B0AE
```

handler没有把`ESI`设为1。共同join先把normalized opcode写入`dword_4CF6D8`，再因`ESI==0`返回。因此完整记录的合同是：

- opcode58或153只消费当前一条；
- IP推进10字节；
- previous发布为normalized `58/153`；
- 返回`yielded`，不在同调用获取下一条。

从`0x7FF6`开始的完整记录恰好结束于`0x8000`，可以完成初始化、链入、IP与previous更新后正常让出，不触发下一fetch。

## 真实资产

锁定目录共有84条物理记录、88个entry probe：

| opcode | 链归属 | TALK1 | TALK2 | TALK3 | TALK4 | 物理记录 | probes |
| ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 58 | 主链 | 15 | 24 | 11 | 23 | 73 | 77 |
| 153 | 次链 | 2 | 0 | 4 | 5 | 11 | 11 |

opcode58额外4个probe来自`TALK3.DAT`四条各被两条已锁定入口路径到达的物理记录。全部84条decoded记录均为低位raw `0x003A/0x0099`且长度10。四文件逐字节原始字样计数为：

| raw word | 原始出现 | decoded入口 |
| --- | ---: | ---: |
| `0x003A` | 286 | 73 |
| `0x403A/0x803A/0xC03A` | 0 | 0 |
| `0x0099` | 39 | 11 |
| `0x4099/0x8099/0xC099` | 0 | 0 |

原始低位字样中大量出现位置不是有效指令入口，不能用字样计数替代目录记录。高位alias只作synthetic dispatch覆盖。

代表性真实记录：

| opcode | 文件偏移 | 字节 | 结果 |
| ---: | ---: | --- | --- |
| 58 | `TALK1.DAT@0x0000549F` | `3A 00 52 00 58 01 2E 23 02 00` | 主链节点`(82,344,9006,2)` |
| 153 | `TALK1.DAT@0x0000468A` | `99 00 68 01 90 01 5A 23 00 00` | 次链节点`(360,400,9050,0)` |
| 153 | `TALK1.DAT@0x00004698` | `99 00 E0 01 90 01 5A 23 01 00` | 次链新表头`(480,400,9050,1)` |

第二条153在第一条之后执行，真实回放固定了次链前插顺序。

## 测试覆盖

- opcode58与153各四种raw alias，验证normalized分派和主/次链互不混写；
- 节点头保留字段与兼容next零值、动作三项`0xFFFFFFFF`哨兵及五项等待/模式清零；
- 四个`u16`参数，包括`0xFFFF` action id和`0x8000` base variant的零扩展；
- 已有节点前插顺序与未选中链不变；
- `0x7FFE/0x7FFC/0x7FFA/0x7FF8`四级operand尾，未完成节点不链接、IP/previous不变；
- 完整operand后的owner缺失边界；
- `0x7FF6`精确尾完成次链链接并在`0x8000`让出；
- 上述一条主链、连续两条次链TALK1真实记录回放。

## 双向收敛与分类

LST→C++：先分配、全节点清零、动作初始化、前两word写入、主/次条件保存、后两word写入、后置链头访问与前插、10字节推进、normalized previous与yield均有直接映射。

C++→LST：没有完整记录预验、提前owner检查、操作数未完成即链入、主次链合并、二次分配、signed参数扩展、同调用继续或漏发previous。标准list、typed allocation/owner状态和受检失败时释放未链接节点只隔离原版裸堆/全局指针无效域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
