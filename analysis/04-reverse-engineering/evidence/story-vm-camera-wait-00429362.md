# 剧情 VM 镜头移动等待 `0x00429362`

## 结论

`sub_427920` 的 opcode51 独占入口 `0x00429362`，物理长度固定为 2 字节且没有 operand：

```text
+0  u16 raw opcode
```

handler 按固定短路顺序读取镜头移动的四个有符号 dword 状态：

1. X 剩余位移 `dword_4A99F0`；
2. Y 剩余位移 `dword_4A98C0`；
3. X 每帧步长 `dword_4A9480`；
4. Y 每帧步长 `dword_4A93D8`。

任一值非零就不推进指令位置，经共同出口service audio一次并让出；四值全零才推进 2 字节并在同一次调用继续且不service audio。两路都会经过共同出口，把归一化后的有效 opcode 51 发布到 `dword_4CF6D8`。没有额外helper、operand 读取或镜头状态写入。

## LST 完整路径

每次进入分发表前，`0x00427B59 xor esi,esi` 把本条的同调用继续标志清零。opcode51 入口随后按 X 剩余、Y 剩余、X 步长、Y 步长顺序逐项短路：

```text
00429362  mov eax,[dword_4A99F0]
00429367  test eax,eax
00429369  jnz 0042B0AE
0042936F  mov eax,[dword_4A98C0]
00429374  test eax,eax
00429376  jnz 0042B0AE
0042937C  mov eax,[dword_4A9480]
00429381  test eax,eax
00429383  jnz 0042B0AE
00429389  mov eax,[dword_4A93D8]
0042938E  test eax,eax
00429390  jnz 0042B0AE
```

任一非零分支直接到共同出口；没有修改 instruction pointer、四个镜头字段或 `ESI`，所以保留 `ESI=0` 并结束本次解释器调用。

全零路径才执行：

```text
00429396  add word ptr [context_ip],2
0042939B  add current,2
0042939E  save current
004293A2  mov esi,1
004293A7  jmp 0042B0AE
```

共同出口对等待与完成两路都执行：

```text
0042B0AE  mov eax,[effective_opcode]
0042B0B6  and eax,0FFFFh
0042B0BB  or continuation_state,esi
0042B0BD  mov [dword_4CF6D8],eax
0042B0C2  jz return_or_yield
0042B0C8  jmp next_fetch
```

函数入口`0x00427955 xor esi,esi`，首轮fetch的`0x00427B32`把零保存到`var_28`。因此 previous opcode 发布不是完成路径专属，也不能在等待返回前省略。全零路径设置 `ESI=1` 后同调用取下一条且不service audio；等待路径保持`ESI=0`，`var_28 | ESI`为零，从`0x0042B0C2`进入`0x0042D4D7 _AIL_serve`一次后返回。

## C++ 差异与修正

首次审计修正了两路漏发`previous_opcode`，但误把等待路径记录为无audio，C++也漏掉共同出口的`_AIL_serve`。opcode191审计重新追踪`var_28`来源与`0x0042B0C2`零分支后完成独立修复：

- 任一字段非零：先发布有效 opcode51，service audio一次，再返回 `yielded`；
- 四字段全零：先推进 2 字节，再发布有效 opcode51，并同调用继续，不service audio；
- raw 高两位别名只影响分派前原始字，不影响发布的有效 opcode；
- `camera_pan` owner 缺失是 typed 平台边界，在第一次状态读取前返回 `runtime_unavailable`，不伪造 previous 发布。

四个字段使用普通进程状态 owner，无法在安全 C++ 中复刻单个 dword 地址失效；有效 owner 域的短路顺序和结果保持不变。

## 窗口边界

opcode51 只需要已取到的 2 字节 opcode：

- 在 `0x7FFE` 且任一字段非零时，保留 IP `0x7FFE`，发布 previous、service audio一次后让出；
- 在 `0x7FFE` 且四字段全零时，先推进到 `0x8000` 并发布 previous，再由下一次 fetch 返回 `instruction_out_of_range`。

没有额外 operand 边界，也不能把完整尾记录提前判为失败。

## 真实资产

`story-vm-talk-linear-records.tsv` 锁定：

| 文件 | 物理记录 | entry probes |
| --- | ---: | ---: |
| `TALK1.DAT` | 35 | 35 |
| `TALK2.DAT` | 25 | 25 |
| `TALK3.DAT` | 9 | 9 |
| `TALK4.DAT` | 39 | 39 |
| 合计 | 108 | 108 |

全部记录 raw 为 `0x0033`，长度均为 2。原始四个 TALK 文件中的低位字样 `0x0033` 共出现 277 次，但只有上述 108 个已解码入口可作为指令证据；`0x4033/0x8033/0xC033` 原始字样均为 0，不能伪造资产别名记录。

代表性真实回放使用 `TALK1.DAT@0x000046C2: 33 00`。该记录紧随同文件 `0x000046B8` 的绝对镜头移动初始化：非零移动状态时原地发布 previous、service audio一次并让出，状态归零后推进 2 字节、发布 previous并同调用继续且不再service audio。

## 测试覆盖

- 四种 raw 高位别名；
- 四个镜头字段分别单独非零，确认值不被修改、IP 不推进并让出；
- 四字段全零时推进 2 字节、发布归一化 previous并同调用继续；
- 等待和完成两路都固定 previous 发布；等待路恰好一次audio，完成路无audio；
- `camera_pan` owner 缺失的首次访问边界；
- `0x7FFE` 等待尾与完成尾；
- `TALK1.DAT@0x000046C2` 真实记录的等待→完成两阶段回放。

## 双向收敛与分类

LST→C++：四字段读取集合与顺序、任一非零等待、全零推进 2、`ESI`/`var_28`续取判定、等待路`_AIL_serve`、两路 previous 发布和无额外副作用均有对应。

C++→LST：没有只检查位移余量、清零步长、主动推进镜头、循环阻塞、完成路额外音频、operand 读取或等待路径漏发previous/audio。新增 owner 检查只隔离原版不可安全表达的全局地址失效域。

修复后Story VM synthetic/real/initial-session 3/3、Linux core 186/186与app 192/192完整门全部通过。未启动原版或OpenSWD3游戏EXE。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
