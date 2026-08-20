# 剧情 VM 角色路径条件跳转 handler：0x00428318

状态：`platform_adapted`、`assembly_exact`（有效内存/I/O 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00428318..0x004283AB`

opcode：`16`，C++ 语义常量 `OP_16_JUMP_IF_ROLE_PATH_UNPREPARED`

直接 helper：`sub_40C0D0`、`sub_42E430`

## 1. 编码与 selector

逻辑指令长度为8 bytes：

```text
+0 u16 opcode
+2 u16 role_selector
+4 u32 target_data_offset
```

公共 fetch 的 `raw_word & 0x3FFF` 使 `0010/4010/8010/C010` 共用该入口。handler 把 `role_selector` 原样传给 `sub_40C0D0`，不做 `0xFFF0` source 替换；`0xFFFE` 仍由 helper 解析为 controlled role。

`sub_40C0D0` 先把 output 置0，再由 ordinary lookup 把 miss output 覆写为 `0xFFFFFFFF`。handler 忽略 bool return，使用该 output 扫描 slot；因此 ordinary missing/FFF0 missing 无法与 16-bit slot role index 匹配，走 no-branch，而不是退化到 role0。

现代 `resolve_role_index` 保留 underlying output，即使返回 false 仍继续按 output 扫描。invalid controlled index 受 VM 入口已有 checked boundary 约束，在 dispatch 前返回 `role_not_found`；这是平台安全适配，不伪造 handler 成功。

## 2. 72-slot predicate

原循环从 active-object slot0 扫到 slot71，每个 slot 步长 `0x21C`。branch predicate 必须同时满足：

1. slot `+0x00` 的 16-bit role index 等于 resolver output；
2. slot `+0x1B` low nibble 等于2；
3. 对应 role `+0x10` 的 `0x40000000` prepared-movement flag 为0。

前两项不满足时不会读取 role flags。若 role index 非法但存在匹配 slot，原程序会在 flags 读取处危险访问；现代只在该点 checked-stop。prepared flag 非零时继续扫描剩余 slot；72个 slot 全部不满足时走 no-branch。

## 3. 两条控制流

### no-branch

未找到 predicate 时，原程序不读取 `target_data_offset`，直接把指令指针和 context IP 都加8，设置 `ESI=1` 进入公共 join。join 发布 `previous=16` 后 same-call fetch 下一 opcode；无直接 audio service。

这一区分影响窗口尾：在 offset `0x7FFC` 只有 opcode+selector 的情况下，no-branch 仍先推进到 `0x8004`，随后下一 fetch 才越界。现代对应返回 `instruction_out_of_range`，保留 IP `0x8004` 与 previous16；不得提前以 operand truncation 拒绝。

### branch

找到 predicate 后才从当前指令 `+4` 读取完整 little-endian `u32` target，并经 `sub_42E430`：

1. service audio；
2. context data offset 写 target、IP 清零；
3. 当前 TALK 文件内按物理 `target+0x200` seek；
4. 不预清共享窗口，读取最多 `0x8000` bytes；
5. caller 发布 previous16 并 same-call 从新窗口 offset0 fetch。

现代复用 opcode15 已审计的 `load_same_file_story_window` typed owner。branch target 截断在原 `u32` 读取点返回 `operand_out_of_range`；loader 非 ready 时保留 audio、offset 与 previous 后 checked-stop `load_failed`，并禁止读取旧窗口。

## 4. 测试与真实资产

synthetic 测试覆盖：四 raw alias、predicate branch、无 slot no-branch、prepared flag no-branch、ordinary miss output `FFFFFFFF`、FFF0 不替换、VM controlled-index 安全边界、branch loader failure，以及 offset `0x7FFC` 上 branch/no-branch 不同危险点顺序。

`story-vm-talk-linear-records.tsv` 中 opcode16 共3条物理记录，全部位于 `TALK2.DAT`：

```text
0x0000749E  10 00 BD 00 AE 72 00 00
0x0000F963  10 00 E4 00 87 F7 00 00
0x0000F979  10 00 E6 00 87 F7 00 00
```

三条均为 raw `0x0010`、decoded length 8、entry probe hit 1；三个 selector 为 `00BD/00E4/00E6`，两个 target `0x72AE/0xF787` 均在 `TALK2.DAT` 有效范围内。

真实回放使用 `TALK2.DAT@0xF963`，构造 selector E4 的 type-2 slot 与 prepared flag clear，经真实 resource database 跳到 data offset `0xF787`。目标首条为 opcode67、duration `0x012C`；同一次 VM 调用执行 opcode16 后启动 wait 并把 operand 自修改为 `0x812C`。定向 synthetic、real-suite、initial-session-real-suite CTest 为 3/3；完整 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 均以 exit 0 通过，且未启动任何游戏 EXE。

关闭后 workpack 为 12/146。下一行严格是：

```text
0x004283AC
opcode 17
```

opcode17 尚未独立审计，常量保持无语义后缀 `OP_17`。
