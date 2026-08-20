# 剧情 VM 角色步进 handler：0x0042822A

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`sdl_runtime_integrated`；当前 TALK 资产无物理记录，原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042822A..0x00428297`

opcode：`13`，C++ 语义常量 `OP_13_STEP_ROLE`

直接 helper：`sub_40C0D0`、`sub_42E280`、`nullsub_1`

## 1. 编码与 selector

指令固定 4 bytes：

```text
+0 u16 opcode
+2 u16 role selector
```

公共 fetch 的 `raw_word & 0x3FFF` 使 `000D/400D/800D/C00D` 共用该 handler。selector `0xFFF0` 先替换为 context source guid，再交 `sub_40C0D0`；替换不写回脚本窗口。

ordinary selector 未找到 role 时，原程序只调用纯诊断 `Talk(Step)`，随后仍消费指令并 yield。`0xFFFE` 是 controlled-role selector；原 resolver 返回 controlled index 后会立即读取 role flags。现代对越界 controlled index 在该危险读取点 checked-stop，不错误归入 ordinary missing。

## 2. role gate 与 `sub_42E280`

role 命中后先测试 flags `0x02000000`：

- bit25 已置位：不调用 helper，直接进入公共 4-byte join；
- bit25 清零：调用 `sub_42E280(role_index)`，但 handler 完全忽略返回值。

`query_legacy_world_story_path` 是 `sub_42E280` 的 typed owner，保留三种历史返回：

- `0`：未找到 role 对应的 type-2 slot；
- `1`：找到下一方向，写 cursor frame gate/stall；无碰撞时打开 gate、写 4-pixel X/Y step，并对 role flags OR `0x40000000`；
- `2`：路径到达，清 wait/arrival flags，并按 selected-role 规则复位 movement/arrival bytes、必要时重置 camera。

opcode13 对三种返回都执行相同的 consume/yield；helper 的内部副作用必须保留。typed owner 的 runtime、slot、cursor、direction、surface 或 selected-role 状态失败在原程序对应危险点 checked-stop，现代映射为 `runtime_unavailable` 或 `role_path_failed`，不伪造成功。

## 3. 公共 join、previous 与 audio

handler 的所有正常分支跳到 `loc_42C7E6`，再执行固定长度 join：

```text
IP += 4
previous = 13
_AIL_serve()
return 1
```

`sub_427920` 每次 fetch 前把 `ESI` 清零；opcode13 不重写它。`loc_42B0AE` 先发布 `dword_4CF6D8 = 13`，随后检测 `var_28 | ESI == 0`，因此必定走 `loc_42D4D7` 的 `_AIL_serve` 并 yield，不会 same-call 继续。现代按同一顺序推进 IP、发布 `previous_opcode`、调用 `ports.service_audio()`，然后返回 `yielded`。

ordinary missing 与 bit25-set 路径不需要 story-path runtime，但仍执行一次直接 audio service。found/bit25-clear 路径仅在 helper 成功后推进和 service；checked failure 不提前发布 previous 或调用 audio。

## 4. 边界与测试

4-byte selector 是唯一 operand。少于4 bytes 时现代在原 selector 读取危险点返回 `operand_out_of_range`，不推进 IP、不发布 previous、不 service audio。

synthetic 测试覆盖：四 raw alias、真实 type-2 slot 的 return1 step、副作用后 return2 arrival、无 slot return0、raw `0xFFF0`、bit25 skip、ordinary missing、invalid controlled、runtime 缺失与窗口尾截断；同时固定 IP+4、previous=13、一次直接 audio service 和 yield。

完整 `story-vm-talk-linear-records.tsv` 共 58,782 条物理记录，opcode13 命中 0 条，因此本组不声称 `real_asset_tested`。story VM synthetic、real-suite、initial-session-real-suite 定向 CTest 为 3/3；最终 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192 均以 exit 0 通过，未启动任何原版或 OpenSWD3 游戏 EXE。

关闭后 workpack 为 9/146。下一行严格是：

```text
0x0042829C
opcode 14
```

opcode14 尚未独立审计，production 常量保持无语义后缀 `OP_14`。
