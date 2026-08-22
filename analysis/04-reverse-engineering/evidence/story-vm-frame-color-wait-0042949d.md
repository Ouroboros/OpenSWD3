# 剧情 VM 画面三通道渐变等待 `0x0042949D`

## 结论

`sub_427920` 的 opcode53 独占入口 `0x0042949D`，物理长度固定为 2 字节且没有 operand。它把画面三通道渐变状态的 countdown 当作 signed dword：

- countdown `> 0`：IP 不变，在当前 opcode 原地让出；
- countdown `<= 0`：IP 推进 2 字节，在同一次调用继续取下一条；
- 两路都经过原版共同出口，发布归一化 previous opcode53；
- handler 不修改 countdown、current、target 或 step；正值等待路经common join调用audio一次，非正完成路same-call无audio。

既有 C++ 的 signed 条件、等待和推进已经正确，但两路都绕过了共同出口的 previous 发布，且 case 仍是裸数字。修正只补齐这两个真实差异。

## LST 控制流

```text
0042949D  mov  eax,dword_4A9934
004294A2  test eax,eax
004294A4  jg   0042B0AE

004294AA  add  word ptr [context_ip],2
004294AF  add  current,2
004294B2  save current
004294B6  mov  esi,1
004294BB  jmp  0042B0AE
```

`test eax,eax` 后使用 signed `JG`，不是 unsigned 比较，也不是仅检测非零。因此 `1..INT32_MAX` 等待，`0/-1/INT32_MIN` 完成。

## 让出标志与共同出口

函数入口 `0x00427955 xor esi,esi` 把 ESI 置零；首次 dispatch 前 `0x00427B32` 把这个零保存到 `var_28`。每次 same-call next fetch 在 `0x00427B59 xor esi,esi` 再次清零。

因此 countdown 正值直接跳往共同出口时，`var_28 | esi == 0`，共同出口发布 previous 后转到解释器返回路径；modern 对应 `yielded`。完成路径显式设置 `ESI=1`，共同出口发布 previous 后回到 next fetch；modern 对应推进后 `continue`。

共同出口：

```text
0042B0AE  mov eax,normalized opcode local
0042B0B2  mov ecx,var_28
0042B0B6  and eax,0FFFFh
0042B0BB  or  ecx,esi
0042B0BD  mov dword_4CF6D8,eax
0042B0C2  jz  return/yield path
0042B0C8  jmp next fetch
```

因此 waiting 与 completed 都必须发布 normalized opcode53；raw `0x4035/0x8035/0xC035` 也不能泄漏高位到 previous。

## typed owner 与状态不变式

原版在入口第一条业务指令直接读取 `dword_4A9934`。modern 将其映射为 `LegacyFrameColorTransitionState::countdown`；`frame_color == nullptr` 在这次首次状态读取处返回 `runtime_unavailable`，不推进 IP、不发布 previous。

有效 owner 下只有 countdown 读取。等待与完成均不递减 countdown，也不覆写current/target/step；逐帧颜色推进由独立的 `sub_4146F0` 负责，opcode53 只观察完成条件。

## 窗口边界

opcode53 只有两字节 opcode word，因此 `0x7FFE` 是合法精确尾：

- countdown 正值：保持 IP `0x7FFE`，发布previous并yield；
- countdown非正：先推进到`0x8000`并发布previous，再由下一fetch返回`instruction_out_of_range`。

完成路径的状态与previous效果不能因为下一fetch失败而回滚。

## 真实资产

`story-vm-talk-linear-records.tsv` 锁定：

| 文件 | 物理记录 | entry probes |
| --- | ---: | ---: |
| `TALK1.DAT` | 732 | 732 |
| `TALK2.DAT` | 24 | 24 |
| `TALK3.DAT` | 174 | 174 |
| `TALK4.DAT` | 430 | 430 |
| 合计 | 1360 | 1360 |

全部记录 raw 为 `0x0035`、长度 2。四文件原始低位字样 `0x0035` 共 2114 次，但只有 1360 个已解码入口可作为指令证据。

高位字样只存在原始字节候选：`0x4035` 1 次、`0x8035` 0 次、`0xC035` 6 次，均不是已解码入口；高位 alias 只作synthetic protocol覆盖，不能伪造真实资产。

代表性真实相邻序列：

```text
TALK1.DAT@0x000043B8
34 00 E2 FF E2 FF E2 FF 00 00 00 00 00 00 06 00
35 00
```

第一条opcode52把三通道从`-30`渐变到`0`、duration设为6并同调用进入opcode53；opcode53在countdown为6时停在自身并让出。countdown变为0后再次执行，推进2字节并同调用续取。

## 测试覆盖

- 四种raw高位alias和normalized previous；
- countdown `1`、`INT32_MAX`等待；
- countdown `0`、`-1`、`INT32_MIN`完成；
- 等待/完成均不修改全部颜色状态；等待audio一次，完成same-call无audio；
- owner缺失停在首次countdown访问；
- `0x7FFE`等待尾和完成尾；
- `TALK1.DAT@0x000043B8`真实opcode52→53等待/完成序列。

## 双向收敛与分类

LST→C++：signed `JG`、正值不推进、非正值推进2、ESI wait/continue、共同出口previous和same-call next fetch均有直接映射。

C++→LST：没有自行递减countdown、改写颜色、把负值当等待、漏发previous、提前推进等待路径、漏掉等待路common audio或增加完成路audio。typed owner检查只隔离原版不可安全表达的空地址域。

```text
assembly_exact;unit_tested;real_asset_tested;platform_adapted;sdl_runtime_integrated
```
