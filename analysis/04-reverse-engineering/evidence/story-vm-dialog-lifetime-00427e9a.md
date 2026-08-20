# 剧情 VM 对话 lifetime 暂存 handler：0x00427E9A

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427E9A..0x00427EBD`

opcode：`8`

## 1. 完整指令合同

LST 是一条无 helper 的直线路径：

```text
00427E9A  xor edx, edx
00427E9C  mov esi, 1
00427EA1  mov dx, [ebx+2]
00427EA5  mov dword_4CF738, esi
00427EAB  mov dword_4A1364, edx
00427EB1  add word ptr [ebp+0], 4
00427EB6  add ebx, 4
00427EB9  mov [esp+var_50], ebx
00427EBD  jmp loc_42B0AE
```

因此有效行为是：

1. 从 `ip+2` 读取无符号 u16；前置 `xor edx,edx` 明确要求零扩展；
2. `dword_4CF738 = 1`；
3. `dword_4A1364 = operand`；
4. context IP 精确推进 4 bytes；
5. common join 发布 `previous_opcode=8`，随后在同一次 VM 调用内继续 fetch。

它不调用 audio、不分配、不单独 yield。现代 owner 分别是 `next_text_aux_pending` 与 `next_text_aux_value`。旧 case 的读取、写入、推进和 continue 已正确；独立重审发现的唯一差异是漏掉 common join 的 previous publication，现已在 `continue` 前补齐。

## 2. 消费与 reset 顺序

共享对话 handler `0x00427B8F` 是这组 one-shot 状态的真实消费者：

- pending 为真时，把 value 写入 dialog record `lifetime_limit`；
- 同时给 record flags 增加 `0x08`；
- dialog 成功排队后才把 value/pending reset 为 `60/false`。

组合测试 `8 -> dialog 2` 固定同帧可见性、record 值和成功后的 reset。opcode8 本身不提前清状态，也不等待后续帧。

## 3. 边界、alias 与 previous

公共 fetch 先执行 `raw_word & 0x3FFF`，所以 `0008/4008/8008/C008` 共享完整语义。四个 alias 后接受控 opcode12 的测试固定：

- operand `0xFFFF` 以 65535 保存，不作符号扩展；
- 两条指令在同一次 step 中 fetch；
- 返回时 IP 为 4、`previous_opcode=8`；
- opcode8 不产生 audio callback。

当当前位置只能读取 opcode u16、不能读取 `ip+2` operand 时，现代边界返回 `operand_out_of_range`。该检查发生在 pending/value、IP、previous 和 audio 的任何可见修改之前，停在原 `mov dx,[ebx+2]` 危险点。

## 4. 资产验证

完整线性 TALK 目录含 opcode8 物理记录 2832 条：

```text
TALK1.DAT 856
TALK2.DAT 573
TALK3.DAT 631
TALK4.DAT 772
```

2832/2832 的 raw word 是 `0x0008`，`decoded_length=4`。real CTest 回放 `TALK1.DAT@0x0000451A` 的原字节 `08 00 FF FF`，固定真实 operand `0xFFFF` 的零扩展、one-shot 暂存、IP+4、previous=8 与同帧 continue。

## 5. 验证与停止线

synthetic、real、initial-session-real 三项定向 CTest 为 3/3。最终完整门禁为 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192；三个 build/test 进程均 lifecycle exit 0，且没有启动原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 4/146。下一行严格是：

```text
0x00427EC2
opcode 9
```

opcode9 修改 `text_control_flags` 的另一位并共用部分跳转目标，仍须独立审计其 mask、IP、previous 和 continue；不能从 opcode7 邻接形式直接推断。
