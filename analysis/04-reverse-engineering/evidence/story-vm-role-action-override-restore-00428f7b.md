# 剧情 VM 角色 action override 恢复组 `0x00428F7B`

## 结论

`sub_427920` 主分派 opcodes 46–49 共享唯一入口 `0x00428F7B`。四条物理记录均固定为 4 字节：

```text
+0  u16 raw opcode
+2  u16 role selector
```

四条都原样把selector交给`sub_40C0D0`：本handler不把`0xFFF0`替换为context source GUID，因此它是ordinary字面GUID；`0xFFFE`仍由helper直接选择受控角色index。ordinary selector按u16 GUID查找第一个bit28清的角色。

lookup返回值被机器忽略；随后按输出index直接形成action指针。live role路径按effective opcode分支：

- opcode46：无条件把pending action id/base variant/variant delta复制回三个active字段，再调用`sub_40DC00`重置pending与等待状态；
- opcode47：仅在pending base variant不等于`0xFFFFFFFF`时把完整u32值复制回active base variant，并清pending；
- opcode48：仅在pending variant delta不等于`0xFFFFFFFF`时把完整u32值复制回active variant delta，并清pending；
- opcode49：仅在u16 wait override不等于`0xFFFF`时写入`0xFFFF`，不修改相邻wait字段。

无论47–49是否实际写字段，四条最终都恰好调用一次`sub_4321E0(action)`。refresh零返回只诊断，不回滚或阻止4字节推进、previous发布和same-call continuation。

## 唯一汇编边界

共享入口及四分支为`0x00428F7B..0x00429061`；下一共享handler从`0x00429066`开始。

selector lookup与branch选择：

```text
00428F7B  mov dx,[current+2]
00428F7F  lea ecx,[role_index]
00428F83  push ecx
00428F84  push edx
00428F85  call sub_40C0D0
00428F8A  mov eax,[role_index]       ; helper return ignored
00428F91  lea eax,[eax+eax*2]
00428F94  lea eax,[eax+eax*8]
00428F97  lea esi,[action_base+eax*8]
00428F9E  mov eax,[effective_opcode]
00428FA2  cmp ax,002Eh               ; opcode46
00428FC4  cmp ax,002Fh               ; opcode47
00428FDE  cmp ax,0030h               ; opcode48
00428FF8  cmp ax,0031h               ; opcode49
```

### opcode46

```text
00428FA8  mov ecx,[action+1Ch]
00428FAB  mov edx,[action+20h]
00428FAE  mov eax,[action+3Ch]
00428FB2  mov [action+00h],ecx
00428FB4  mov [action+08h],edx
00428FB7  mov [action+34h],eax
00428FBA  call sub_40DC00(action)
```

三个copy均无sentinel判断；pending值为`0xFFFFFFFF`时也会覆盖active字段。`sub_40DC00`随后精确执行：

```text
0040DC07  action+1C = FFFFFFFF
0040DC0A  action+20 = FFFFFFFF
0040DC0D  action+3C = FFFFFFFF
0040DC12  action+48 word = 0         ; wait_override
0040DC16  action+46 word = 0         ; wait_default
0040DC1A  action+44 word = 0         ; wait_remaining
0040DC1E  action+42 word = 0         ; command_cursor
0040DC22  action+90 dword = 0        ; external_mode
```

modern既有`initialize_legacy_action_record`与`0x0040DC00..0x0040DC28`逐字段一致；opcode46在三个copy之后调用它，不新增更宽或更多字段清理。

### opcodes47–49

```text
00428FCA  mov eax,[action+20h]
00428FCD  cmp eax,FFFFFFFFh
00428FD2  mov [action+08h],eax
00428FD5  mov [action+20h],FFFFFFFFh

00428FE4  mov eax,[action+3Ch]
00428FE7  cmp eax,FFFFFFFFh
00428FEC  mov [action+34h],eax
00428FEF  mov [action+3Ch],FFFFFFFFh

00428FFE  mov eax,0000FFFFh
00429003  cmp word ptr [action+48h],ax
00429009  mov word ptr [action+48h],ax
```

47/48的比较、active写和pending清均为完整u32；49的比较和写严格为u16。

## 公共 refresh 与顺序尾

四分支都汇入：

```text
0042900D  push action
0042900E  call sub_4321E0
00429016  test eax,eax
00429018  jnz loc_42904C
...       nullsub_1("Act Err(Talk:RestoreOpSet)", action fields)
0042904C  mov ebx,[current]
00429050  add word ptr [talk_context+0],4
00429055  add ebx,4
00429058  mov esi,1
0042905D  mov [current],ebx
00429061  jmp loc_42B0AE             ; common previous publication
```

因此“pending不存在”或“wait override已经是FFFF”只跳过条件写，不跳过refresh。refresh返回零只增加diagnostic，全部已完成字段效果保留，仍推进并继续。

## Unsafe点与平台适配

`sub_40C0D0` ordinary miss把输出index写为`0xFFFFFFFF`并返回零；handler不检查返回值，而是形成`action_base - 0xD8`。四条第一次unsafe访问分别是：

- 46读取`action[-1]+0x1C/+0x20/+0x3C`；
- 47读取`action[-1]+0x20`；
- 48读取`action[-1]+0x3C`；
- 49读取`action[-1]+0x48`的u16。

modern保留selector读取与lookup顺序，在第一次branch-specific action访问点以`role_not_found`隔离原版越界域；不patch MAPS、不refresh、不推进IP或发布previous。public VM session在opcode fetch前检查受控owner，因此无效controlled index保持opcode/count/IP/previous不变。

selector word不完整时在`+2`读取点返回`operand_out_of_range`。完整4-byte live记录可位于窗口`0x7FFC`：四条均先完成branch效果、一次refresh、IP推进和previous发布，下一fetch才返回`instruction_out_of_range`。四条都没有audio、yield或额外MAPS副作用。

## 真实资产缺失证明

锁定`story-vm-talk-linear-records.tsv`对四条均为：

```text
unique_physical_records = 0
entry_probe_instances   = 0
asset status            = asset_absence_verified
```

四文件raw低位字候选仅是任意byte-word出现，不能证明指令入口：

| opcode | TALK1 | TALK2 | TALK3 | TALK4 | 合计 |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 46 / `0x002E` | 38 | 14 | 20 | 41 | 113 |
| 47 / `0x002F` | 64 | 18 | 19 | 33 | 134 |
| 48 / `0x0030` | 44 | 15 | 18 | 28 | 105 |
| 49 / `0x0031` | 79 | 14 | 18 | 27 | 138 |

高位alias也只有零散byte-word候选：`0xC02E` 2次、`0x402F` 1次、`0xC030` 2次、`0x8031` 2次，其余alias为0。没有任何候选被线性记录目录或entry probe证明为opcode46–49，所以不伪造real replay或`real_asset_tested`标记。

## 测试覆盖

synthetic测试覆盖：

- 四opcode各自四种raw alias、4字节推进、normalized previous与same-call continuation；
- opcode46三个完整u32无条件copy，以及`sub_40DC00`精确pending/wait/cursor/external字段重置和邻接字段保持；
- opcode46 pending均为`FFFFFFFF`时仍覆盖三个active字段；
- opcode47/48完整u32条件copy和pending清零；opcode49精确u16写且相邻wait字段保持；
- 47–49 sentinel已满足时仍恰好refresh一次；
- `0xFFF0`字面GUID、`0xFFFE`受控index、bit28 skip与首个clear match；
- 四opcode各自missing role、selector截断、invalid controlled-session与`0x7FFC`精确窗口尾；
- 四opcode refresh零返回保持branch效果、记录failure并继续；
- 无MAPS patch、无audio。

## 双向收敛与分类

LST→C++：共享lookup、四个effective分支、所有u32/u16宽度、opcode46无条件copy与初始化、47–49条件写、共同一次refresh、纯诊断失败、4字节推进、previous及same-call continuation均一一映射。

C++→LST：没有新增FFF0替换、MAPS fallback、sentinel默认值保护、字段扩宽、额外初始化、refresh跳过、audio或yield。checked role/session/window边界只隔离原版unsafe域。

```text
assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated
```
