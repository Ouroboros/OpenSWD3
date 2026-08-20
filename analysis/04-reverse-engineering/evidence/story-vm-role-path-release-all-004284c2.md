# 剧情 VM 全角色路径释放 handler：0x004284C2

状态：`platform_adapted`、`assembly_exact`（有效内存/owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x004284C2..0x00428532`，最终公共消费块 `0x00427E84..0x00427E95`

opcode：`19`，C++ 语义常量 `OP_19_RELEASE_ROLE_PATHS`

直接 helper：`sub_42D920`

## 1. 编码与 role-count 边界

逻辑指令只有2-byte opcode，无 operand。公共 fetch 的 `raw_word & 0x3FFF` 使 `0013/4013/8013/C013` 共用该入口。

handler 从 `dword_49E0C4` 读取当前 role count，循环 index 从1开始，条件是严格小于 count。因此：

- count 为0或1时不进入循环；
- role index0 永远跳过，即使其 bit31 置位也不修改；
- 现代 `roles` 是 map business 当前 role vector 的 span，`roles.size()` 对应该 count，不使用固定256容量。

## 2. 每角色处理

对 `[1, count)` 中每个 role，先读取 role record base `+0x10` bit31：

- bit31 为0：直接进入下一 index，不调用 helper，不清 action wait；
- bit31 为1：调用 `sub_42D920(role_index)`，机器返回值完全忽略；随后无条件清 role `+0x10` bit31，并把 role record base `+0x84` 的 `u16` 清零，对应 `LegacyActionRecord::wait_remaining`。

`sub_42D920` 的独立合同已在 opcode18 evidence 中闭环：bit31 clear immediate-return；bit31 set 时扫描72个 `0x21C`-byte active slots，仅匹配相同 role index 且 type low nibble大于1；无槽返回0；匹配槽则清空无 chained path 的 slot，或恢复 saved chained path。opcode19 不根据0/1返回值等待或重试：无槽返回0后仍清 role state并继续下一 role。

现代抽取共享 `release_legacy_world_story_role_path` typed owner，让 opcode18保留 return-dependent 单次重试，而 opcode19只在 bit31 set 时调用并忽略 legacy return。matching slot 真正需要 owner 时，owner 缺失或 chained-path typed failure在原 helper危险点 checked-stop；此时不得伪造当前 role 清理，也不得继续处理后续 role。

## 3. 最终消费与公共 join

循环结束或 count≤1后都跳到 `loc_427E84`：

1. context IP 与指令指针加2；
2. 设置 continuation；
3. 公共 join 发布 previous19；
4. 同一次 VM 调用取下一 opcode。

窗口 offset `0x7FFE` 的 opcode19 因自身2 bytes完整，会先执行循环、推进到 `0x8000`并发布 previous19，随后下一 fetch 才返回 `instruction_out_of_range`。

## 4. 测试与真实资产

synthetic 测试独立覆盖：四 raw alias；role0跳过；bit31-set 无槽返回0仍清理；bit31-set type2匹配槽完成；bit31-clear wait保持；type1不匹配；首个需owner角色失败时按 index顺序停止且后续角色不动；仅一个 role 的 count-one边界；以及窗口 `0x7FFE` 先消费再越界。opcode18/19共享 helper 的 bit31-clear direct owner测试继续通过。

`story-vm-talk-linear-records.tsv` 中 opcode19 只有1条物理记录：

```text
TALK2.DAT@0x00010C93
13 00 1C 00
payload offset = 0x00010A93
next opcode = 28
```

该记录 raw `0x0013`、decoded length 2、entry probe hit 1。真实回放让所有非零 role 的 bit31保持clear，证明 opcode19不清其 wait，随后推进2、发布 previous19，并在同一次 VM 调用取到尚未恢复的 opcode28后以 `unsupported_opcode` 停止。

定向 story-path owner、synthetic、real-suite、initial-session-real-suite CTest 为4/4；完整 Linux core 186/186、Linux app 192/192 均以 exit 0 通过。生成器 Python `py_compile` 通过且两次重跑幂等；两套真实 CMake 均已编译本轮全部 C++ 改动。按执行计划 v257，小 handler不运行 Windows；Windows LLVM app只在剧情 VM P3大阶段完成时统一编译、集中修复。未启动任何游戏 EXE。

关闭后 workpack 为15/146。下一行严格是共享 handler：

```text
0x0042ADB7
opcode 20,169
```

该共享入口尚未独立审计；opcode20常量保持中性 `OP_20`，opcode169在审计前不得获得语义后缀，也不得只审计其中一个变体。
