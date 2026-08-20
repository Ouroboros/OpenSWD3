# 剧情 VM 角色动作状态等待 handler：0x0042829C

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042829C..0x0042830B`

opcode：`14`，C++ 语义常量 `OP_14_WAIT_ROLE_ACTION_STATUS`

直接 helper：`sub_40C0D0`

## 1. 编码与 selector

指令固定 4 bytes：

```text
+0 u16 opcode
+2 u16 selector
```

公共 fetch 的 `raw_word & 0x3FFF` 使 `000E/400E/800E/C00E` 共用该 handler。原程序先把 selector `0xFFF0` 替换为 context source guid，再判断替换后的 selector 是否为 `0xFFFD`。因此 raw `0xFFF0` 且 source guid 为 `0xFFFD` 时，必须走 context-local status，不得再进入 role lookup。

selector `0xFFFD` 直接读取 context `+0x26`，现代字段为 `LegacyWorldTalkContext::field_26`。其余 selector 经 `sub_40C0D0` 解析，再读取 role `+0x26`，现代字段为 `LegacyWorldRoleRecord::interaction_gate`。

原 handler 不检查 resolver 返回值；ordinary missing 或越界 controlled index 会在紧随其后的 role `+0x26` 读取处危险访问。现代在该点 checked-stop 为 `role_not_found`，不推进 IP、不发布 previous、不 service audio。

## 2. wait/complete 与公共 join

两种 status owner 使用完全相同的判定：

- status 非零：IP 保持不变，本帧 yield，下一帧重试同一 opcode；
- status 为零：IP 增加4，但仍在本帧 yield，不 same-call fetch 下一指令。

两条路径都进入 `loc_42B0AE`。`sub_427920` 在 fetch 前把 `ESI` 清零，opcode14 不重写它，因此公共 join 固定执行：

```text
previous = 14
_AIL_serve()
return 1
```

原 `previous` 发布发生在 audio service 之前。现代在 valid wait/complete 路径按相同顺序写 `previous_opcode`、调用 `ports.service_audio()`、增加直接 audio 计数并返回 `yielded`。旧 C++ 仅返回 `yielded`，遗漏 previous 与 audio；本次已修复。

## 3. 宽度与危险点

selector 是唯一 operand。少于4 bytes 时现代在原 selector 读取危险点返回 `operand_out_of_range`，不推进 IP、不发布 previous、不 service audio。

FFFD 分支不需要 role lookup；普通 role、FFF0 role 与 FFFE controlled-role 都在 role `interaction_gate` 读取前完成 checked index 验证。除该安全边界外，valid-domain 分支、推进、等待和 yield 顺序与 LST 一致。

## 4. 测试与真实资产

synthetic 测试覆盖：四 raw alias、role status 非零到零的跨帧 retry、raw `0xFFF0`、raw `0xFFFD`、FFF0 替换后得到 FFFD、ordinary missing、invalid controlled 与窗口尾截断；同时固定 IP、previous、一次直接 audio service 和 yield。

`story-vm-talk-linear-records.tsv` 中 opcode14 共 2,894 条物理记录：

- `TALK1.DAT` 1,092 条；
- `TALK2.DAT` 651 条；
- `TALK3.DAT` 841 条；
- `TALK4.DAT` 310 条。

全 2,894 条均为 raw `0x000E`、decoded length 4；直接读取原始文件复核无坏记录。selector 共 392 种，其中 `0xFFF0` 43 条、`0x00F8` 10 条，资产未出现 FFFD/FFFE。单元测试回放 `TALK1.DAT@0x471F` 的真实 `0E 00 F8 00`，验证 status 非零等待、status 清零完成、previous、audio 与 yield。定向 synthetic、real-suite、initial-session-real-suite CTest 为 3/3；完整 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 均以 exit 0 通过，且未启动任何游戏 EXE。

关闭后 workpack 为 10/146。下一行严格是：

```text
0x00428310
opcode 15
```

opcode15 尚未独立审计，常量保持无语义后缀 `OP_15`。
