# 剧情 VM 对话控制 bit30 清除 handler：0x00427EC2

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427EC2..0x00427ECE`，随后进入共享尾 `0x00427E7E..0x00427E95`

opcode：`9`

## 1. 完整指令合同

```text
00427EC2  mov edx, dword_4A1360
00427EC8  and edx, 0BFFFFFFFh
00427ECE  jmp loc_427E7E
00427E7E  mov dword_4A1360, edx
00427E84  add IP, 2
00427E90  mov esi, 1
00427E95  jmp loc_42B0AE
```

因此本组只清 `dword_4A1360` 的 bit30，保留其余 31 bits；IP 精确推进 2 bytes。common join 随后发布 `previous_opcode=9` 并在同一次 VM 调用内继续 fetch。opcode9 无操作数、无 helper、无 audio、不单独 yield。

现代 owner 是 `text_control_flags`。旧 case 的 mask、推进和 continue 正确；独立重审发现的唯一差异是遗漏 common join 的 previous publication，现已补齐。

## 2. 消费、alias 与组合顺序

共享对话 handler 在 bit30 已清时给 dialog record flags 增加 `0x400`，成功排队后再把 `text_control_flags` reset 为 `0xFFFFFFFF`。组合测试 `9 -> dialog 2` 固定了同帧可见性、record flag 与 reset 顺序。

fetch 先执行 `raw_word & 0x3FFF`，所以 `0009/4009/8009/C009` 共享完整语义。四 alias 后接受控 opcode12 的测试固定：

- 只清 bit30，bit31 和其余位不变；
- IP 为 2、`previous_opcode=9`；
- opcode9 不产生 audio callback。

`9 -> default 194` 另固定默认非法诊断先观察 previous=9，再按默认合同发布 194。

## 3. 资产验证

完整线性 TALK 目录含 opcode9 物理记录 1537 条：

```text
TALK1.DAT 315
TALK2.DAT 314
TALK3.DAT 138
TALK4.DAT 770
```

1537/1537 的 raw word 是 `0x0009`，`decoded_length=2`。real CTest 回放 `TALK1.DAT@0x0000451E` 的原两字节记录，固定 bit30 clear、IP+2、previous=9 与同帧 continue。

本 handler 唯一前置边界是公共 fetch 能读取当前 opcode u16；有效入口内没有额外可能失败的读取或 callback。

## 4. 验证与停止线

synthetic、real、initial-session-real 三项定向 CTest 为 3/3。最终完整门禁为 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192；三个 build/test 进程均 lifecycle exit 0，且没有启动原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 5/146。下一行严格是：

```text
0x00427ED0
opcode 10
```

opcode10 涉及 role selector、role 查找、action update 与错误分支，必须从完整 LST 独立恢复，不能继承本组无操作数合同。
