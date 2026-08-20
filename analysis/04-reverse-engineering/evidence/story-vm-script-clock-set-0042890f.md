# 剧情 VM 有界脚本时钟设置 `0x0042890F`

## 结论

`sub_427920` 主分派 opcode 34 的唯一入口是 `0x0042890F`。指令固定四字节：

```text
+0  u16 raw opcode
+2  u16 requested clock
```

handler 把 `+2` 零扩展为 `u32` 并先写入 `dword_4ACDB0`；值无符号小于等于 1000 时保留，大于 1000 时再把同一全局写零。随后固定推进四字节，发布 previous opcode，并在同一次 VM 调用中继续。

现代已有 owner `LegacyWorldStoryVmState::script_clock` 对应 `dword_4ACDB0`。本指令不修改 `script_clock_frame_counter`（`dword_4A94A0`）或 `script_clock_origin`（`dword_4BA42C`），也不需要新 port。

## 唯一汇编边界

主分派表只把 effective opcode 34 送到 `0x0042890F`。handler 半开区间严格为 `0x0042890F..0x00428934`；`0x00428934` 已是 opcode 35 的独立入口。

完整本体：

```text
0042890F  xor eax,eax
00428911  mov ax,[ebx+2]
00428915  cmp eax,3E8h
0042891A  mov dword_4ACDB0,eax
0042891F  jbe loc_42D182
00428925  mov dword_4ACDB0,0
0042892F  jmp loc_42D182
```

`xor eax,eax` 后只写 `ax`，证明参数是零扩展 `u16`，不是 `s16`。比较使用 `jbe`，所以边界是无符号 `<=1000`：

- `0` 保留为 `0`；
- `1000` 保留为 `1000`；
- `1001` 写入后再变为 `0`；
- `65535` 写入后再变为 `0`。

机器顺序是先写请求值，再在超界支路覆写零，不是将输入 clamp 到 1000。两次写之间没有 helper、callback、诊断或其他可观察出口；现代实现仍保留这两个语句的顺序。

## 共享顺序尾

两条本体出口都到 `0x0042D182`：

```text
0042D182  add ebx,4
0042D185  add word ptr [ebp+0],4
0042D18A  mov [esp+var_50],ebx
0042D18E  mov esi,1
0042D193  jmp loc_42B0AE
```

`loc_42B0AE` 把 effective opcode 的低 16 位写入 `dword_4CF6D8`。因为 `ESI=1`，`ecx|esi` 非零，控制流跳回 `0x00427B40` 抓取下一条，而不是进入 `_AIL_serve`/yield。现代合同因此是：

- IP `+4`；
- previous opcode=`34`；
- 不直接 service audio；
- 同一次 `step_legacy_world_story_vm` 调用继续执行下一条。

窗口尾缺少 `+2..+3` 时，现代 checked boundary 在机器首次 operand read 处返回 `operand_out_of_range`，不写 clock、不推进、不发布 previous opcode。

## 持久状态 owner

`dword_4ACDB0` 不是仅供 opcode 34–37 使用的临时计数器。普通世界主帧门控每次调用剧情 VM 前执行：

1. `dword_4A94A0` 加一；
2. 超过 20 时把它写零；
3. 此时 `dword_4ACDB0` 加一；
4. `dword_4ACDB0 > 1000` 时写零。

现代 `advance_legacy_world_script_clock` 已将这三个全局分别映射为：

```text
script_clock_frame_counter -> dword_4A94A0
script_clock               -> dword_4ACDB0
script_clock_origin        -> dword_4BA42C
```

SDL world frame 已在每次 story step 之前调用该推进函数。opcode 34 直接写同一 `script_clock` owner，因此后续 opcode 35–37、PATH runtime 和后续帧都观察同一状态；新增第二份 VM-local counter 会造成错误分叉，未采用。

## 真实资产审计

锁定的线性记录与 entry-probe inventory 对 opcode 34 均为零：

```text
unique_physical_records = 0
entry_probe_instances   = 0
coverage                = not_seen_in_linear_prefix_probe
```

直接扫描四个 TALK 文件的原始字节会找到 501 处 `0x0022` 双字节序列：TALK1/2/3/4=`106/53/53/289`；这些都不在已证明的指令入口，不能冒充真实 opcode 34 记录。高位 alias `0x4022/0x8022/0xC022` 的原始字节出现数均为零。

因此本 handler 使用 `asset_absence_verified`，不伪造 real-asset replay。资产缺席不影响实现：完整机器本体和共享尾均可静态确定。

## 测试覆盖

synthetic 测试覆盖：

- raw `0x0022/0x4022/0x8022/0xC022` 四种 alias；
- 输入 `0`、边界 `1000`、首个超界值 `1001`、最大 `u16 65535`；
- `>1000` 写零而不是 clamp 到 1000；
- clock 写入不改变 frame divider 或 snapshot；
- IP+4、previous publication、同调用继续、无直接 audio service；
- 窗口尾只剩 opcode 时，在写入前返回 operand failure。

## 双向收敛

LST→C++：

- `xor eax/eax + mov ax` → `read_u16` 零扩展；
- 先写全局 → 先写 `state.script_clock`；
- `cmp 3E8h / jbe` → `>1000` 时覆写零；
- `loc_42D182` → IP+4；
- `ESI=1 / loc_42B0AE` → previous publication并同调用继续。

C++→LST：实现中的 operand check 是原始首次越界读取的 checked platform boundary；其余每项写入、比较、推进和继续均有上述机器地址。没有 helper、省略的业务副作用、诊断、yield 或额外状态写入。

关闭标签：

```text
assembly_exact;unit_tested;asset_absence_verified;sdl_runtime_integrated
```
