# 剧情 VM 已准备角色路径条件跳转 handler：0x004283AC

状态：`platform_adapted`、`assembly_exact`（有效内存/I/O 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x004283AC..0x00428443`

opcode：`17`，C++ 语义常量 `OP_17_JUMP_IF_ROLE_PATH_PREPARED`

直接 helper：`sub_40C0D0`、`sub_42E430`

## 1. 编码与 selector

逻辑指令长度为8 bytes：

```text
+0 u16 opcode
+2 u16 role_selector
+4 u32 target_data_offset
```

公共 fetch 的 `raw_word & 0x3FFF` 使 `0011/4011/8011/C011` 共用该入口。handler 把 `role_selector` 原样传给 `sub_40C0D0`，不做 `0xFFF0` source 替换；`0xFFFE` 仍由 helper 解析为 controlled role。

`sub_40C0D0` 先把 output 置0，再由 ordinary lookup 把 miss output 覆写为 `0xFFFFFFFF`。handler 忽略 bool return，使用该 output 扫描 slot；ordinary missing 与 `0xFFF0` missing 均不能匹配 16-bit slot role index，走 no-branch。现代 resolver 保留该 output；invalid controlled index 只在 VM 入口原危险访问点前 checked-stop，这是平台安全适配。

## 2. 独立 72-slot predicate

`0x004283CE..0x00428408` 从 active-object slot0 扫至 slot71，每槽步长 `0x21C`。branch predicate 必须同时满足：

1. slot `+0x00` 的 16-bit role index 等于 resolver output；
2. slot `+0x1B` low nibble 等于2；
3. 对应 role `+0x10` 的 `0x40000000` prepared-movement flag 非0。

前两项不满足时不读取 role flags。flag 为0时继续扫描；72槽均不满足时走顺序路径。此入口与 opcode16 的最后一项极性相反，但 closure 只依据本入口的 `test/jz` 控制流，不继承 opcode16 结论。

## 3. 两条控制流

### no-branch

未找到 predicate 时，`0x00428410..0x0042841A` 把指令指针和 context IP 都加8，随后直接进入公共 join。join 发布 `previous=17` 并 same-call fetch 下一 opcode；不直接 service audio，也不读取 `target_data_offset`。

因此窗口 offset `0x7FFC` 只有 opcode+selector 且角色未准备时，先推进到 `0x8004`，再由下一 fetch 返回 `instruction_out_of_range`；不能提前报 operand truncation。

### branch

predicate 命中后，`0x00428422` 才从当前指令 `+4` 读取 little-endian `u32` target，随后调用 `sub_42E430`：

1. service audio；
2. context data offset 写 target、IP 清零；
3. 当前 TALK 文件按 `target+0x200` seek；
4. 不预清共享窗口，读取最多 `0x8000` bytes；
5. `loc_42D18E` 设置 continuation，公共 join 发布 previous17 并 same-call 从新窗口 offset0 fetch。

现代复用 opcode15 已审计的 `load_same_file_story_window` typed owner。branch target 截断只在原 `u32` 读取点返回 `operand_out_of_range`；loader 非 ready 时保留 audio、offset 与 previous 后 checked-stop `load_failed`，并禁止从旧窗口继续执行。

## 4. 测试与真实资产

synthetic 测试独立覆盖：四 raw alias、prepared branch、unprepared no-branch、ordinary resolver miss `FFFFFFFF`、`FFF0` 不替换、slot type low nibble、invalid controlled-role 入口安全边界、branch loader failure，以及 offset `0x7FFC` 上 branch/no-branch 不同 target 读取时机。

`story-vm-talk-linear-records.tsv` 中 opcode17 共83条物理记录：`TALK1.DAT` 55条、`TALK2.DAT` 28条；全部 raw `0x0011`、decoded length 8、entry probe hit 1。共有30个 selector、77个 target，target data offset 范围 `0x7296..0x53FF8`，全部满足各自 TALK 文件的 `target+0x200` 物理范围。

真实回放使用：

```text
TALK2.DAT@0x000074A6
11 00 DA 00 96 72 00 00
selector = 0x00DA
target data offset = 0x00007296
```

构造 selector DA 的 type-2 slot 且 prepared flag 已置位，经真实 resource database 跳至 TALK2 data offset `0x7296`。目标首条 bytes 为 `6D 00 02 00 BD 00 DA 00`，即 opcode109；同一次 VM 调用先发布 previous17，再取到尚未恢复的 opcode109 并以 `unsupported_opcode` 停止，证明 branch 的同调用 continuation。

定向 synthetic、real-suite、initial-session-real-suite CTest 为3/3；完整 Linux core 186/186、Linux app 192/192 均以 exit 0 通过。三份 story VM C++ 文件精确 LSP 为0 error，生成器 Python `py_compile` 通过且两次重跑幂等。按执行计划 v254，小 handler 不运行 Windows；Windows LLVM app 只在剧情 VM P3 大阶段完成时统一编译、集中修复。未启动任何游戏 EXE。

关闭后 workpack 为 13/146。下一行严格是：

```text
0x0042845A
opcode 18
```

opcode18 尚未独立审计，常量保持无语义后缀 `OP_18`。
