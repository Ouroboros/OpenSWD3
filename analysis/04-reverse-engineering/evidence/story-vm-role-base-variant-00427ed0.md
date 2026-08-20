# 剧情 VM 角色 base variant handler：0x00427ED0

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427ED0..0x0042811A`

opcode：`10`

直接 helper：`sub_40C0D0`、`sub_42E740`、`sub_4321E0`、`sub_40D460`、`nullsub_1`

## 1. 编码与 selector

指令固定 6 bytes：

```text
+0 u16 opcode
+2 u16 role selector
+4 u16 base variant
```

selector `0xFFF0` 先替换为当前 context source guid，但不写回脚本窗口。随后 `sub_40C0D0` 解析 live role index。公共 fetch 的 `raw_word & 0x3FFF` 使 `000A/400A/800A/C00A` 共享完整语义。

## 2. live role 路径

role 命中时，LST 以 `4BABE8 + index*0xD8` 取得 role 内嵌 action 起点，并按顺序：

1. zero-extend `u16(ip+4)`，写 action `+0x08 base_variant`；
2. 把 action `+0x44 wait_remaining` 的 word 清零；
3. 调 `sub_42E740(ip+6, role_index)`；
4. 仅当该 helper 返回零时调用 `sub_4321E0(action)`；
5. 两个 helper 都返回零时只执行 `nullsub_1` 诊断；
6. 无论 helper 返回值，IP 都推进 6，common join 发布 previous=10 并同帧继续。

`sub_42E740` 只在下一条 raw opcode（不做 `&0x3FFF`）恰为 `10/11/45`，且下一条 selector 解析到同一 role 时返回一。因此连续同 role action 指令只在链尾 update；带高位的 `0x400B` 不匹配。已有链测试固定 raw 判定与 callback 次数。

现代 action 字段、wait 清零和 chain helper 已正确；独立重审发现 live path 只缺 common join 的 previous publication，现已补齐。action update 返回零只增加失败观测，不改变原继续合同；纯诊断不伪造业务 callback。

## 3. missing role fallback

role 未命中时原程序不返回错误，而是调用 `sub_40D460`：

```text
guid = resolved selector
base_variant = u16(ip+4)
flags_or_mask = 0x1000
其余字段 = 0xFFFF（保持）
```

`sub_40D460` 查找 MAPS role source，写 source `+6 base_variant` 并 OR source `+0x14 flags` 的 `0x1000`；source 不存在时仅诊断。现代通过已有 `patch_role_source(LegacyMapsRolePatchRequest)` typed port 保留该合同，然后同样 IP+6、previous=10、同帧继续。旧实现错误返回 `role_not_found`，现已修复。

SDL port 已接真实 `LegacyMapsWorldDatabase` patch owner；测试替身固定 request 的所有保持字段，未伪造 live role 或 patch 成功结果。

## 4. 受检边界与测试

固定载荷不足 6 bytes 时，现代返回 `operand_out_of_range`，且不修改 role、MAPS、IP、previous 或 action callback。selector `0xFFFE` 在原 resolver 中无条件映射 controlled index；若该 index 越界，原程序会在随后 live action 写入处危险访问而不会走 MAPS fallback。现代在该首个危险点前 checked `role_not_found`，不 patch、不推进、不发布 previous；该 unsafe-domain 回归由 opcode11 独立 REVIEW 发现并补齐。原有效路径没有其他可恢复错误；role/action helper 结果均按原控制流消费。

测试覆盖：

- 四个 raw alias 的 live role base variant、wait reset、action update、IP+6 与 previous=10；
- missing role 的 exact MAPS patch request 与继续行为；
- invalid controlled-role index 在危险 live 写入前 checked-stop 且不误 patch；
- 短载荷零副作用；
- raw next `11` 与 `0x400B` 的 coalescing 差异；
- real `TALK1.DAT@0x00004A24` 记录 `0A 00 01 00 00 00`。

## 5. 资产与验证

完整线性 TALK 目录含 opcode10 物理记录 1693 条：

```text
TALK1.DAT 679
TALK2.DAT 443
TALK3.DAT 337
TALK4.DAT 234
```

1693/1693 的 raw word 是 `0x000A`，`decoded_length=6`。synthetic、real、initial-session-real 三项定向 CTest 为 3/3；最终 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 通过，三个有效门禁进程均 exit 0，未启动原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 6/146。下一行严格是：

```text
0x00427FEB
opcode 11
```

opcode11 虽共享部分现代代码，但写不同 action 字段、设置 live role flags，并有不同 fallback 参数；必须重新独立审计。
