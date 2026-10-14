# 剧情 VM 脚本时钟低位字节条件跳转 `0x00428934`

## 结论

`sub_427920` 主分派 opcode 35 的唯一入口是 `0x00428934`。物理记录固定八字节：

```text
+0  u16 raw opcode
+2  u8  value
+3  u8  padding（handler 不读取）
+4  u32 same-file target
```

handler 取 `dword_4ACDB0` 的低 16 位，与零扩展的 `u8(+2)` 作无符号比较：

```text
u32(value) <= (script_clock & 0xFFFF)
```

条件成立才读取 `u32(+4)` 并调用 `sub_42E430` 重载当前 TALK 文件；条件不成立不读取 padding 或 target，只按物理长度推进八字节。两条有效路径都发布 previous opcode，并在同一次 VM 调用中继续。

## 唯一汇编边界

handler 半开区间为 `0x00428934..0x0042896C`；`0x0042896C` 已是 opcode 36 的独立入口。完整 opcode-specific 本体：

```text
00428934  mov ecx,dword_4ACDB0
0042893A  xor eax,eax
0042893C  mov al,[ebx+2]
0042893F  and ecx,0FFFFh
00428945  cmp eax,ecx
00428947  mov [esp+var_48],eax
0042894B  ja  loc_4289A8
0042894D  mov edx,[ebx+4]
00428950  push edx
00428951  push offset unk_4B72A0
00428956  call sub_42E430
0042895B  mov ebx,[esp+var_50]
0042895F  add esp,8
00428962  mov esi,1
00428967  jmp loc_42B0AE
```

`xor eax,eax / mov al` 证明 value 是 `u8`。clock 先执行 `and 0xFFFF`，所以高 16 位必须忽略；不能直接把完整 `u32 script_clock` 与 value 比较。`cmp` 后的 `ja` 把严格大于送到 no-jump 尾，因此相等属于 taken。

`mov [esp+var_48],eax` 只保存函数局部工作值，之后不形成持久状态或业务 callback；现代实现无需为它增加 owner。

## 分阶段读取与物理 padding

机器在比较前只读取：

- opcode 的两字节；
- `+2` 的单个 value 字节；
- 持久 clock。

`+3` 从未被取数。只有 taken 才在 `0x0042894D` 读取 `+4..+7` target。因此现代边界必须分两阶段：

1. 初始只要求 `ip..ip+2` 三字节可用；
2. 条件成立后才要求 `ip+4..ip+7` 四字节可用。

不能预先要求完整八字节。若窗口只剩 opcode 与 value 三字节且条件不成立，机器仍进入 `loc_4289A8` 按八字节推进；padding 与 target 都不需要存在。若同一三字节输入条件成立，则在首次 target read 处失败，且不推进、不发布 previous opcode、不调用 loader。

## taken 与 no-jump 时序

### Taken

`sub_42E430` 使用当前 TALK 文件与 `u32(+4)` 重载窗口。已审计 typed port `load_same_file_story_window` 保留：

1. 直接 service audio；
2. 发布目标 TALK offset 与 IP=0；
3. 从当前文件读取目标窗口；
4. 返回后发布 opcode35为 previous opcode；
5. 同一次 VM 调用从新窗口继续。

现代文件 owner 可报告 seek/read failure。该 checked I/O boundary 在已发生的 audio、目标offset/IP发布与窗口加载尝试之后返回 `load_failed`，previous opcode仍按调用返回后的原顺序发布。

### Not taken

`loc_4289A8` 固定执行：

```text
004289A8  add ebx,8
004289AB  add word ptr [ebp+0],8
004289B0  mov [esp+var_50],ebx
004289B4  mov esi,1
004289B9  jmp loc_42B0AE
```

因此 no-jump 路径 IP+8，不直接 service audio，并同调用继续。`loc_42B0AE` 统一发布 effective opcode 的低16位到 `dword_4CF6D8`。

## 状态 owner

`dword_4ACDB0` 已映射到 `LegacyWorldStoryVmState::script_clock`，由普通世界主帧的21帧分频推进、opcode34设置、后续opcode35–37和PATH runtime共享。opcode35只读该 owner，不写 clock、frame divider或snapshot。

## 真实资产审计

锁定的线性记录与entry-probe inventory均未观察到opcode35：

```text
unique_physical_records = 0
entry_probe_instances   = 0
coverage                = not_seen_in_linear_prefix_probe
```

直接字节扫描会命中大量非入口序列，不能冒充真实记录：

| raw word | TALK1 | TALK2 | TALK3 | TALK4 | 合计 |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0x0023` | 106 | 29 | 138 | 116 | 389 |
| `0x4023` | 318 | 150 | 129 | 156 | 753 |
| `0x8023` | 1 | 0 | 0 | 0 | 1 |
| `0xC023` | 1 | 0 | 0 | 1 | 2 |

共1145处原始双字节候选均没有被锁定CFG证明为opcode35入口。因此本组使用 `asset_absence_verified`，不伪造real replay。

## 测试覆盖

synthetic测试覆盖：

- 四种raw alias；
- clock高16位非零但低16位为1，value=2必须no-jump；
- value与clock低16位相等时taken；
- taken同文件重载、direct audio、IP=0、previous与同调用续行；
- no-jump在窗口只剩三字节时不要求padding/target，仍IP+8；
- 相同三字节输入在taken条件下于target read处失败；
- value字节本身缺失时在比较前失败；
- checked loader failure的audio、target offset/IP和previous发布顺序；
- handler不修改script clock。

## 双向收敛与分类

LST→C++：`mov al`、clock低16 mask、`ja`方向、branch-only target读取、`sub_42E430`、no-jump +8、`ESI=1`和common previous join均有一一映射。

C++→LST：初始三字节与taken target的两次bounds check分别位于原始首次value/target读取；其余比较、加载、推进和继续均可反查到上述地址。没有读取`+3`，没有新增clock写入、诊断或yield。

由于taken路径使用typed资源加载/audio端口，并以checked I/O failure替代原始不安全资源边界，本handler归类为`platform_adapted`：

```text
assembly_exact;unit_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated
```
