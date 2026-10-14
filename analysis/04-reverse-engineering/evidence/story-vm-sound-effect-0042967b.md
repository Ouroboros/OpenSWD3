# 剧情 VM 音效请求 `0x0042967B`

## 结论

`sub_427920` 的`0x0042967B`只承载主表opcode59，物理长度4字节：

```text
+0 u16 raw opcode
+2 u16 sound_id
```

handler读取当前全局样本音量等级与u16声音编号，调用`sub_485610`提交一次单次、居中音效请求。调用返回值不参与剧情控制流。随后推进4字节、发布normalized previous并跨帧让出，不在同调用继续获取下一条。

## handler读取与调用顺序

完整入口只有一条直线路径：

```text
0042967B  mov eax,dword_4AB784
00429680  mov cx,[current+2]
00429684  push eax
00429685  push ecx
00429686  call sub_485610
0042968B  add esp,8
0042968E  jmp 0042C7E6
```

`dword_4AB784`是当前样本混音等级，初始化默认值为6。全局等级在operand读取前快照；`mov cx`只覆盖ECX低16位，但`sub_485610`随后显式`AND 0xFFFF`，所以上半位不参与声音编号。

原版没有完整记录预验；`+2`缺失会在已经读取全局等级后发生裸内存访问。modern在VM窗口边界上返回`operand_out_of_range`。平台端口把当前世界音效等级与播放请求封装在一次无异常调用中；等级读取没有回调或状态写入，单线程有效域与原顺序一致。

## `sub_485610`参数变换

wrapper范围`0x00485610..0x00485645`：

```text
00485610  mov ecx,[level]
00485614  mov eax,2E8BA2E9h
00485619  shl ecx,7
0048561C  imul ecx
00485624  sar edx,1
0048562A  shr eax,1Fh
0048562D  add edx,eax
00485631  and ecx,0FFFFh
```

乘法高半与修正等价于先按32位回绕执行`level << 7`，再作signed、向零截断的`/ 11`。该结果作为volume传入`sub_485CE0`，随后由`sub_486260`夹到`0..127`。声音编号零扩展后的低16位原样传入。

调用`sub_485CE0`的六个显式参数固定为：

```text
(existing_buffer=0, sound_id=u16, volume=scaled_level,
 pan=0, loop_count=1, named_file_auxiliary=0)
```

因此剧情opcode59请求单次播放、居中声像，不自行等待播放结束。

## 资源编号与返回值

`sub_485CE0`先检查音频管理器状态，再拒绝`sound_id==0`。非零编号进入`sub_486490`；该helper以`id << 4`索引16字节目录槽，并读取`base + id*16 - 16`，固定编号为一基。

原版目录访问没有在此处证明上界检查；异常资源、无可用sample handle或Miles配置失败均沿内部失败出口返回0，成功启动后也在`0x00485E84`清EAX并返回0。opcode59完全忽略返回值，故任何音频后端失败都不能阻止IP推进、previous发布或yield。

modern的`audio_video::play_legacy_sample`恢复低16位编号、`(level << 7) / 11`、pan0和loop1；`LegacySampleManager`对0、超槽编号、缺失资源及后端失败安全返回0。超槽范围检查与SDL样本后端是原版裸目录/Miles边界的最小平台适配，VM仍不根据结果分支。

SDL `StoryVmPorts`在每一世界帧以当前`spatial_audio.mix_level`构造，并调用这一已审计audio_video wrapper，不在VM中复制另一份音量状态。

## IP、previous 与精确尾

handler跳到共享4字节推进尾：

```text
0042C7E6  mov ebx,[current_local]
0042C7EA  add ebx,4
0042C7ED  add word ptr [context_ip],4
0042C7F2  mov [current_local],ebx
0042C7F6  jmp 0042B0AE
```

入口没有设置`ESI=1`。共同join把normalized opcode写入`dword_4CF6D8`，并因`ESI==0`返回。因此：

- 成功提交或音频内部失败都推进4字节；
- previous发布为normalized opcode59；
- common join调用`_AIL_serve`恰好一次；
- 返回`yielded`，同调用不继续下一条；
- 从`0x7FFC`开始的完整记录可在`0x8000`完成请求、IP与previous更新后正常让出。

这也修正了既有组合测试中的旧假设：opcode15/16/17/23/25/26在同调用继续到59后，最终previous必须是59，而不是前一条继续型指令。

## 真实资产

锁定目录共有740条物理记录、740个entry probe，无重复probe：

| 文件 | 物理记录 | probes |
| --- | ---: | ---: |
| `TALK1.DAT` | 224 | 224 |
| `TALK2.DAT` | 155 | 155 |
| `TALK3.DAT` | 279 | 279 |
| `TALK4.DAT` | 82 | 82 |

全部decoded记录均为raw `0x003B`且长度4。声音编号共有93种，范围`1..656`，未观察到0；最常见编号193有383条。四文件逐字节原始字样计数为：

| raw word | 原始出现 | decoded入口 |
| --- | ---: | ---: |
| `0x003B` | 1121 | 740 |
| `0x403B/0x803B/0xC03B` | 0 | 0 |

原始低位字样中的381处不是已锁定指令入口，不能用字样计数替代目录记录。高位alias仅作synthetic dispatch覆盖。

代表性真实回放：

| 文件偏移 | 字节 | 声音编号 |
| --- | --- | ---: |
| `TALK2.DAT@0x00003370` | `3B 00 01 00` | 1 |
| `TALK3.DAT@0x00015D89` | `3B 00 90 02` | 656 |

## 测试覆盖

- 四种raw alias，分别提交`0/1/0x1234/0xFFFF`，验证u16宽度与normalized opcode；
- 单次端口请求、IP推进4、previous发布59、`executed_instruction_count=1`与yield；
- `0x7FFE`缺失operand：无请求、IP/previous不变；
- `0x7FFC`完整精确尾：请求后IP到`0x8000`并让出；
- opcode15/16/17/23/25/26同调用继续到59后的previous组合回归；
- TALK2最小观察编号1与TALK3最大观察编号656真实记录回放；
- 既有audio_video wrapper测试验证低16位编号、等级5缩放为58、等级11缩放为128后夹到127、pan居中与loop1。

## 双向收敛与分类

LST→C++：u16声音编号、当前世界音效等级、精确缩放wrapper、返回忽略、共享+4尾、normalized previous与yield均有映射；真实平台端口接到已审计样本管理器。

C++→LST：没有同调用继续、音效完成等待、返回值分支、编号符号扩展、漏掉或重复common audio service、或漏发previous。SDL后端、资源范围检查与受检窗口边界只隔离原版Miles/裸内存无效域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
