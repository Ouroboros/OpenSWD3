# 剧情 VM 对话控制 bit31 清除 handler：0x00427E72

状态：`assembly_exact`、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427E72..0x00427E95`

opcode：`7`

## 1. 完整指令合同

LST 是一条无 helper、无操作数的直线路径：

```text
00427E72  mov edx, dword_4A1360
00427E78  and edx, 7FFFFFFFh
00427E7E  mov dword_4A1360, edx
00427E84  add word ptr [ebp+0], 2
00427E89  add ebx, 2
00427E8C  mov [esp+var_50], ebx
00427E90  mov esi, 1
00427E95  jmp loc_42B0AE
```

因此有效行为只有：

1. `dword_4A1360 &= 0x7FFFFFFF`，只清 bit31；
2. context IP 精确推进 2 bytes；
3. 设置本地 continue 来源，在同一次 `sub_427920` 调用内重新 fetch。

它不读 `ip+2`，不调用 helper，不分配，不发出 audio service，也不单独 yield。现代 owner 是 `LegacyWorldStoryVmState::text_control_flags`；opcode7 已有的 clear/advance/continue 主体正确，独立重审发现唯一缺口是 common join 的 previous publication。

## 2. 公共 join 的本组行为

`0x0042B0AE..0x0042B0C8` 的顺序是：

```text
load local continue
OR with ESI
publish effective opcode to dword_4CF6D8
if zero -> audio/yield
else -> 0x00427B40 fetch
```

opcode7 把 `ESI=1`，所以本组必须先发布 `previous_opcode=7`，随后同帧继续；不能因“不 yield”省略 previous，也不能在继续前 service audio。现代 case 现在在 `continue` 前写 `state.previous_opcode`。

该证据只关闭 opcode7 对 common join 的这一条路径，不把 `story-vm-runtime-paths.tsv` 的全 handler `common_join` 行标为完成。

## 3. raw alias 与相邻合同

fetch 在一级分派前执行 `raw_word & 0x3FFF`，所以 `0007/4007/8007/C007` 都执行相同 bit31 clear。测试对四个 raw alias 后接显式未实现 opcode12，固定：

- 两条指令在同一次 step 中被 fetch；
- 返回时 IP 为 2，`previous_opcode=7`；
- bit31 清除、其余 31 bits 不变；
- opcode7 没有 audio callback。

另有两条组合证据：

- `7 -> default 194`：默认非法诊断观察 previous=7，之后按默认合同发布 194；
- `7 -> dialog 2`：同帧对话读取已清 bit31 并给 record 加 `0x20`，成功排队后才按对话合同把 `text_control_flags` reset 为 `0xFFFFFFFF`。

这两条分别证明 previous 写点与 bit31 的真实消费者时序。

## 4. 资产与受检边界

完整线性 TALK 目录含 opcode7 物理记录 2781 条：

```text
TALK1.DAT 840
TALK2.DAT 546
TALK3.DAT 623
TALK4.DAT 772
```

2781/2781 的 raw word 是 `0x0007`，`decoded_length=2`。real CTest 从 `TALK1.DAT@0x00004518` 回放一条原物理记录，再接受控 opcode12，固定真实两字节记录的 clear-and-continue 合同。四个高位 alias 属于完整 14-bit fetch 合同测试，不声称原资产命中过这些 alias。

本 handler 唯一前置边界是公共 fetch 必须能读当前 u16；该检查由 step 在进入 case 前完成。有效入口内无额外可能失败的读取或 callback。

## 5. 验证与停止线

synthetic、real、initial-session-real 三项定向 CTest 为 3/3。最终完整门禁为 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192；三个 build/test 进程均 lifecycle exit 0，且没有启动原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 3/146。下一行严格是：

```text
0x00427E9A
opcode 8
```

opcode8 虽与 opcode7 邻接，但读取 `ip+2` 并写不同 one-shot owner；必须重新独立审计，不能继承本组无操作数结论。
