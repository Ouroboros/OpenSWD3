# 剧情 VM 角色位置 handler：0x0042811F

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`sdl_runtime_integrated`；当前 TALK 资产无物理记录，原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042811F..0x00428225`

opcode：`12`，C++ 语义常量 `OP_12_SET_ROLE_POSITION`

直接 helper：`sub_40C0D0`、`sub_42DAF0`、`nullsub_1`

## 1. 编码、selector 与宽度

指令固定 8 bytes：

```text
+0 u16 opcode
+2 u16 role selector
+4 u16 tile X
+6 u16 tile Y
```

selector `0xFFF0` 先替换为当前 context source guid，再交 `sub_40C0D0`。替换不写回窗口。公共 fetch 的 `raw_word & 0x3FFF` 使 `000C/400C/800C/C00C` 共享 handler。

两个坐标都在 16-bit 寄存器中左移 4 位，再作为 `u16` 传给 `sub_42DAF0`；高位溢出必须回绕。现代用 `static_cast<u16>(raw << 4)` 固定该行为，测试以 `0x1015/0x100F` 验证得到 `336/240`。

## 2. found role 路径

role 命中后，原程序重新读取脚本中的 raw selector，而不是替换后的 selector。仅当 `raw_selector == context.source_guid` 时，按顺序：

1. action `+0x0C cached_base_variant = 0xFFFFFFFF`；
2. action `+0x38 cached_variant_delta = 0xFFFFFFFF`；
3. role flags 清 `0x00080000`。

因此 raw `0xFFF0` 即使成功替换并解析到 source role，也通常不触发该 cache reset。synthetic 回归固定了这一区别。

随后调用：

```text
sub_42DAF0(
    role_index,
    (tile_x << 4) & 0xFFFF,
    (tile_y << 4) & 0xFFFF,
    0,
    -1,
    -1,
    -1
)
```

现代直接复用 `schedule_legacy_world_story_path` typed owner。request 的 flags 为0，三个 action 分量保持默认 `-1`；SDL world frame 注入真实 roles、72 slots、spatial index、surface、node pool、movement、camera、arrival bytes 和 render flags。测试覆盖正常 type-2 path schedule 与 selected-role 路径。

原程序忽略 helper 的整数返回；typed owner 的非-`completed` 状态表示原实现内部的无效 runtime、越界或失败危险点，现代在该点返回 `role_path_failed`，不伪造成功，也不提前推进 IP/previous/controlled bit。这是平台适配而非有效域语义变化。

## 3. missing role 与 controlled 副作用

ordinary selector 未找到 role 时，`sub_40C100` 把输出 index 写为 `0xFFFFFFFF`，原 handler 只执行纯诊断 `setpos`，不读坐标、不调用 `sub_42DAF0`，但仍消费 8 bytes。

无论 found 还是 ordinary missing，消费后都比较 resolver 输出 index 与 controlled role index；相等时对 `dword_4A9920` OR `0x8000`。现代映射为 `dialogs.close.flagged_dialog_counter |= 0x8000`。正常 missing 的 `0xFFFFFFFF` 不等于有效 controlled index；selected-role synthetic case 固定 bit15 写入。

`0xFFFE` 是 controlled-role selector。原 resolver 对它无条件返回 controlled index；index 越界会在 `sub_42DAF0` 内危险访问，绝不会走 ordinary missing 流。现代在 schedule 危险点前 checked `role_not_found`，不消费、不发布 previous、不置 bit15。

## 4. 推进、common join 与 unsafe 顺序

成功或 ordinary missing：

```text
IP += 8
previous = 12
ESI = 1
same-call continue
no direct audio service
```

边界检查按原危险点分阶段：

- 少于4 bytes：selector 不可读，零副作用返回 `operand_out_of_range`；
- found role 只有4..7 bytes：先保留原 cache/flag reset，再在坐标读取点 checked-stop；IP 与 previous 不推进；
- ordinary missing 只有4..7 bytes：原路径不读坐标，先推进到 `0x8004` 并发布 previous；现代在下一 fetch 的原危险点返回 `operand_out_of_range`，不错误回绕到窗口头。

## 5. 测试、资产与关闭结论

测试覆盖：四 raw alias、16-bit 坐标回绕、真实 story-path slot schedule、raw `0xFFF0` cache-reset 差异、controlled bit15、ordinary missing consume、invalid controlled checked-stop、runtime 缺失前的 reset 顺序、found/missing 两类短载荷，以及与 `OP_13` pending 停止点的同帧组合。

完整 `story-vm-talk-linear-records.tsv` 共 58,782 条物理记录，opcode12 命中 0 条。因此本组不声称 `real_asset_tested`；未观察不降低实现或静态关闭要求。synthetic、real-suite、initial-session-real-suite 三项定向 CTest 为 3/3（后两项同时证明既有真实资产无回归）。最终 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 均以 exit 0 通过；未启动任何原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 8/146。下一行严格是：

```text
0x0042822A
opcode 13
```

opcode13 语义尚未独立审计，常量暂保留为 `OP_13`；闭环后才改为带语义后缀的名称。
