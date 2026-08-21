# 剧情 VM 角色动作重复刷新 `0x004294C0`

## 结论

`sub_427920` 的 opcode54 独占入口 `0x004294C0`，物理长度固定为 6 字节：

```text
+0 u16 raw opcode
+2 u16 role selector
+4 s16 repeat count
```

handler 必定先对目标角色动作执行一次初始化刷新；只有 signed repeat count `> 0` 时，再执行指定次数的重复刷新。总刷新次数精确为：

```text
1 + max(repeat_count, 0)
```

初始刷新前清 `action+0x44` 和 `action+0x42`；每次重复刷新前只清 `action+0x44`，刷新后再清角色 `+0x98` 低字（即 action `field_58`）。刷新返回0只触发原版诊断，所有字段效果、后续循环、IP推进、previous发布和同调用继续均不取消。

## selector、lookup 与 operand 顺序

```text
004294C0  mov bx,[current+2]
004294C4  cmp bx,0FFF0h
004294C9  jnz 004294D3
004294CB  mov edx,[context]
004294CF  mov bx,[edx+24h]
004294D3  lea eax,lookup_index
004294D7  push eax
004294D8  push ebx
004294D9  call sub_40C0D0
004294DE  mov eax,lookup_index
004294E2  mov ecx,current
004294E8  movsx edx,word ptr [ecx+4]
```

`0xFFF0` 在handler内替换为talk context `+0x24` 的source GUID；`0xFFFE`不在handler内替换，由`sub_40C0D0`选择受控角色。普通selector使用同一helper的首个有效GUID匹配规则。

lookup发生在repeat word读取之前，且原版忽略helper返回值。普通missing selector会让output index成为`-1`，但仍先读取signed repeat，随后在第一次`action+0x44`写入点进入非法索引内存。modern先保留这两次operand/lookup顺序，repeat可读后才在首次动作写入处返回`role_not_found`；不伪造MAPS fallback。

## 初始刷新

lookup index按216字节role stride定位action：

```text
004294F9  mov word ptr [action+44h],0
00429500  mov word ptr [action+42h],0
00429507  lea eax,[action]
0042950D  push eax
0042950E  call sub_4321E0
```

字段映射：

- `action+0x44`：`wait_remaining`；
- `action+0x42`：`command_cursor`。

写入顺序固定为wait remaining先、command cursor后，再调用刷新。`sub_4321E0`可以回写动作状态；handler不会在初始调用后重清这些字段。

返回0时`0x0042951A..0x00429555`只收集动作ID、基变体、GUID和偏移进入`nullsub_1`诊断，随后无条件进入repeat判断。modern通过`action_update_failure_count`记录该诊断，不改变控制流。

## signed repeat 与循环顺序

```text
00429558  mov eax,repeat_count
0042955C  xor esi,esi
0042955E  cmp eax,edi        ; EDI=0
00429564  jle common +6 tail
```

因此`0/-1/INT16_MIN`都只执行初始刷新。正值循环每次严格执行：

```text
00429577  mov word ptr [action+44h],0
00429584  push action
00429585  call sub_4321E0
... optional diagnostic only ...
004295CF  reload role index
004295D3  inc iteration
004295DA  mov word ptr [role+98h],0
004295E6  cmp iteration,repeat_count
004295E8  jl 0042956E
```

`role+0x98`等于内嵌action的`field_58`。它在刷新返回之后才清零；所以第一次重复刷新能观察初始刷新对`field_58`的回写，第二次及以后观察前一轮的post-clear零值。最后一次刷新对`wait_remaining`、`command_cursor`等字段的回写保留，但最后一次`field_58`回写仍被post-clear覆盖。

测试callback快照固定repeat=2的三次调用入口状态：

```text
initial:  wait=0, command=0,      field_58=old
repeat1:  wait=0, command=write1, field_58=write1
repeat2:  wait=0, command=write2, field_58=0
```

非正repeat另验证初始刷新回写直接成为最终状态。

## 成功与边界

所有初始/重复刷新完成后跳到共享尾`0x00428E38`：推进6字节、设置ESI=1，并经共同出口发布normalized previous opcode54后同调用next fetch。

窗口边界保持原unsafe顺序：

- `0x7FFE`只有opcode：selector读取失败，无状态变化；
- `0x7FFC`有opcode+selector但无repeat：先完成lookup，再在repeat读取失败，无动作写入；
- 完整6字节记录从`0x7FFA`精确结束于`0x8000`：先完成全部刷新、post-clear、IP和previous，再由下一fetch返回`instruction_out_of_range`。

## 真实资产

`story-vm-talk-linear-records.tsv`锁定：

| 文件 | 物理记录 | entry probes |
| --- | ---: | ---: |
| `TALK1.DAT` | 99 | 99 |
| `TALK2.DAT` | 65 | 65 |
| `TALK3.DAT` | 80 | 82 |
| `TALK4.DAT` | 12 | 12 |
| 合计 | 256 | 258 |

全部记录raw为`0x0036`、长度6。两个TALK3物理入口各被两条静态路径探测，因此probe数比物理记录多2；不能把258误报为258条记录。

真实selector共10种：`1/3/4/6/16/920/602/640/2/32`；repeat范围`0..8`，没有负值资产。分布为：

```text
0:66, 1:65, 2:37, 3:53, 4:15, 5:9, 6:6, 7:4, 8:1
```

四文件原始低位字样`0x0036`共635次，但只有256个已解码入口可作指令证据；`0x4036/0x8036/0xC036`原始字样均为0，四种raw alias只作synthetic protocol覆盖。

代表性真实回放：

```text
TALK1.DAT@0x00005A6B
36 00 01 00 01 00
```

selector 1、repeat 1，总共执行初始+重复两次刷新，随后推进6字节并同调用继续。

## 测试覆盖

- 四种raw alias、normalized previous和same-call continuation；
- callback快照固定初始/两次重复刷新的三字段顺序与最后回写；
- repeat `INT16_MIN/-1/0`只刷新一次且保留初始回写；
- 三次刷新全部失败仍完成循环并累计三次诊断；
- `FFF0` source GUID翻译、`FFFE`受控角色选择；
- selector截断、lookup后repeat截断、missing角色首次unsafe写边界；
- `0x7FFA`精确尾；
- `TALK1.DAT@0x00005A6B`真实记录回放；
- 全资产256物理记录/258 probes、selector和repeat分布。

## 双向收敛与分类

LST→C++：selector翻译、lookup先于repeat、signed repeat、初始双清、初始刷新、循环单清/刷新/post-clear、诊断型失败、6字节推进、previous和same-call均有直接映射。

C++→LST：没有提前完整记录预验、missing角色MAPS patch、把repeat当u16、把repeat误当总次数、刷新失败提前停止、每轮误清command cursor、初始误清field_58、最后回写回滚或漏发previous。typed missing-role停止只隔离原版unchecked `-1`索引域，并保留此前读取顺序。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
