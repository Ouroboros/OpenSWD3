# 剧情 VM 角色 variant delta handler：0x00427FEB

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427FEB..0x0042811A`

opcode：`11`

直接 helper：`sub_40C0D0`、`sub_42E740`、`sub_4321E0`、`sub_40D460`、`nullsub_1`

## 1. 编码与 selector

指令固定 6 bytes：

```text
+0 u16 opcode
+2 u16 role selector
+4 u16 variant delta
```

selector `0xFFF0` 先替换为当前 context source guid，但不写回脚本窗口。随后 `sub_40C0D0` 解析 live role index。公共 fetch 的 `raw_word & 0x3FFF` 使 `000B/400B/800B/C00B` 共享完整 handler 语义。

## 2. live role 路径

role 命中时，LST 以 `4BABE8 + index*0xD8` 取得 role 内嵌 action 起点，并按顺序：

1. zero-extend `u16(ip+4)`，写 action `+0x34 variant_delta`；
2. 把 action `+0x44 wait_remaining` 的 word 清零；
3. 调 `sub_42E740(ip+6, role_index)`；
4. 仅当该 helper 返回零时调用 `sub_4321E0(action)`；
5. 两 helper 都返回零时只执行 `nullsub_1` 诊断；
6. 对 role `+0x10 flags` 执行 `or ch,0x10`，即保留其他位并 OR `0x00001000`；
7. IP 推进 6，common join 发布 previous=11 并同帧继续。

flag 写发生在可选 action update 之后，现代保持该顺序。action update 返回零只增加失败观测，不改变原继续合同；纯诊断不伪造业务 callback。

`sub_42E740` 重新独立核对为：只接受下一条 raw opcode 恰为 `10/11/45`，读取其原始 selector，直接调用 `sub_40C0D0`，且仅在解析 index 与当前 role 相等时返回一。它不 mask raw opcode，也不把下一条 `0xFFF0` 替换为 source guid。现代 helper 的 4-byte 边界、raw 判定和 resolver/index 比较一致。`11→45` 同 role 测试固定了 update 只发生在链尾。

## 3. missing role fallback

ordinary selector 未找到 live role 时，原程序不返回 VM 错误，而调用 `sub_40D460`。11 个 cdecl 参数逐项还原为：

```text
guid = resolved selector
variant_delta = u16(ip+4)
flags_or_mask = 0x1000
其余字段 = 0xFFFF（保持）
```

该 helper 查 MAPS role source，写 source `+8 variant_delta` 并 OR source `+0x14 flags` 的 `0x1000`；source 不存在时仅诊断。现代通过已有 `patch_role_source(LegacyMapsRolePatchRequest)` typed port 保留合同，随后同样 IP+6、previous=11、同帧继续。旧实现错误返回 `role_not_found`，现已修复。

SDL port 已接真实 `LegacyMapsWorldDatabase` patch owner；测试替身固定 request 保持字段，未伪造 live role 或 patch 成功结果。

## 4. unsafe selector 与受检边界

`0xFFFE` 是 controlled-role selector。原 `sub_40C0D0` 对它无条件返回 controlled index 与 success；若 index 越界，程序会在随后 action 写入处危险访问，绝不会走 MAPS fallback。现代 resolver 额外做 bounds check，因此必须区分 ordinary missing role：越界 controlled index 在首个 live 写入前返回 checked `role_not_found`，不 patch MAPS、不推进 IP、不发布 previous，也不调用 action update。该平台适配同时回归 opcode10 的共享分支。

固定载荷不足 6 bytes 时返回 `operand_out_of_range`，且不修改 role、MAPS、IP、previous 或 callback。原路径在首次业务写入前只有读取与 lookup；现代提前边界检查没有吞掉可见副作用。

## 5. 测试、资产与验证

测试覆盖：四个 raw alias 的 live mutation/wait/flags/update/IP/previous；ordinary missing-role exact patch；invalid controlled-role checked-stop；短载荷；`11→45` same-role coalescing；以及 real `TALK1.DAT@0x00004A2E` 的 `0B 00 01 00 00 00`。

完整线性 TALK 目录含 opcode11 物理记录 1234 条：

```text
TALK1.DAT 463
TALK2.DAT 282
TALK3.DAT 289
TALK4.DAT 200
```

1234/1234 的 raw word 是 `0x000B`，`decoded_length=6`。synthetic、real、initial-session-real 三项定向 CTest 为 3/3；最终 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 通过，三个门禁进程均 exit 0，未启动原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 7/146。下一行严格是：

```text
0x0042811F
opcode 12
```

opcode12 必须从完整 LST 独立审计，不继承本 handler 或旧 C++ 的完成状态。
