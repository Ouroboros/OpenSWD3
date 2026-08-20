# 剧情 VM 脚本时钟相对快照条件跳转 `0x0042896C`

## 结论

`sub_427920` 主分派 opcode 36 的唯一入口是 `0x0042896C`。物理记录固定八字节：

```text
+0  u16 raw opcode
+2  u16 delta
+4  u32 same-file target
```

handler 以完整32位 `dword_4BA42C` snapshot 加零扩展 `u16 delta`，按u32回绕得到threshold，再与完整32位 `dword_4ACDB0` script clock作无符号严格比较：

```text
script_clock > u32(script_clock_origin + u16(delta))
```

严格大于时才读取target并调用`sub_42E430`。与普通same-file jump不同，helper返回后仍进入统一`+8`尾：taken从新窗口IP 8继续，而不是IP 0；not-taken则从当前窗口IP+8继续。两条路径都发布previous opcode并在同一次VM调用中继续。

## 唯一汇编边界

opcode-specific入口从`0x0042896C`开始；下一独立handler opcode37位于`0x004289BE`。完整本体与共享尾：

```text
0042896C  mov ecx,dword_4BA42C
00428972  xor eax,eax
00428974  mov ax,[ebx+2]
00428978  mov [esp+var_48],8
00428980  add ecx,eax
00428982  mov eax,dword_4ACDB0
00428987  cmp eax,ecx
00428989  jbe loc_4289A8
0042898B  mov edx,[ebx+4]
0042898E  push edx
0042898F  push offset unk_4B72A0
00428994  call sub_42E430
00428999  add esp,8
0042899C  mov [esp+var_48],0
004289A4  mov ebx,[esp+var_50]
004289A8  add ebx,8
004289AB  add word ptr [ebp+0],8
004289B0  mov [esp+var_50],ebx
004289B4  mov esi,1
004289B9  jmp loc_42B0AE
```

`xor eax,eax / mov ax`证明delta为零扩展`u16`。`add ecx,eax`没有溢出检查，必须保留32位回绕。`cmp eax,ecx / jbe`把clock小于或等于threshold都送到no-jump尾，所以taken是无符号严格`>`，不是`>=`或有符号关系。

`var_48`是函数局部工作值；本路径中写8与写0都不形成持久owner或callback。它不能作为改变硬编码`loc_4289A8`的理由。

## Taken后仍加八字节

`sub_42E430`本体明确执行：

1. `_AIL_serve`；
2. `[context+0x14]=target`；
3. `[context+0x20]=0`；
4. seek到`target+0x200`；
5. 把`0x8000`字节读入固定TALK窗口。

helper返回后opcode36没有直接去common join，而是落入`0x004289A4..0x004289B9`。固定TALK窗口地址不变，caller重新取得窗口基址，再把指针和`[context+0x20]`都加8。因此最终状态是：

```text
talk_data_offset = target
instruction_offset = 8
```

同一次解释器调用从新窗口offset 8抓取下一条。这是机器明确行为，不能照抄opcode35或其他taken branch的IP 0合同。

现代typed loader若返回checked seek/read failure，原机器caller在helper返回后仍会执行+8和common previous发布。因此现代failure顺序固定为：audio → target offset/IP0 → load attempt → caller IP+8 → previous opcode36 → `load_failed`返回。

## 分阶段读取

机器比较前只读取opcode与完整`u16(+2)` delta。`u32(+4)` target仅在严格taken后读取：

- not-taken在窗口只剩四字节时仍按物理长度推进8；
- taken在同样输入上于首次target read处失败，不推进、不发布previous、不调用loader；
- delta本身截断时在比较前失败。

不得预先把八字节整体校验为一个事务。

## 状态owner

```text
LegacyWorldStoryVmState::script_clock        -> dword_4ACDB0
LegacyWorldStoryVmState::script_clock_origin -> dword_4BA42C
```

opcode36只读两者。它不修改clock、origin或21帧divider。origin由后续opcode37及PATH runtime共享，clock由主帧推进、opcode34及PATH runtime共享。

## 真实资产审计

锁定的线性记录与entry-probe inventory均未观察到opcode36：

```text
unique_physical_records = 0
entry_probe_instances   = 0
coverage                = not_seen_in_linear_prefix_probe
```

直接字节扫描仅得到非入口候选：

| raw word | TALK1 | TALK2 | TALK3 | TALK4 | 合计 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0x0024` | 99 | 22 | 32 | 222 | 375 |
| `0x4024` | 0 | 0 | 0 | 0 | 0 |
| `0x8024` | 0 | 0 | 1 | 0 | 1 |
| `0xC024` | 0 | 0 | 0 | 0 | 0 |

共376处双字节候选都没有被锁定CFG证明为opcode36入口。本组使用`asset_absence_verified`，不伪造real replay。

## 测试覆盖

synthetic测试覆盖：

- 四种raw alias；
- `0xFFFFFFF0 + 0x20 -> 0x10`的u32回绕；
- clock等于threshold时no-jump，clock=`threshold+1`时taken；
- 完整32位clock参与比较，不截成低16位；
- target仅taken读取，delta与target各自的窗口尾截断；
- taken loader后从新窗口offset 8继续，offset0放置不同默认字节以隔离错误实现；
- taken direct audio、目标offset、previous与same-call合同；
- checked load failure后仍执行caller +8并发布previous；
- clock与origin均不被handler修改。

## 双向收敛与分类

LST→C++：delta零扩展、u32回绕、完整clock、无符号严格比较、branch-only target、`sub_42E430`、taken/not-taken共享+8、`ESI=1`和common previous join均有一一映射。

C++→LST：两次bounds check分别位于原始delta/target首次读取；typed loader后的+8和previous在成功/失败路径都保留。没有新增状态写入、诊断或yield。

由于taken使用typed资源加载/audio端口并增加checked I/O failure，本handler归类为`platform_adapted`：

```text
assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated
```
