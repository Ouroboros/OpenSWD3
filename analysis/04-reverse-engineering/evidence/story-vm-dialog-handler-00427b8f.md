# 剧情 VM 共享对话 handler：0x00427B8F

状态：`platform_adapted`、`assembly_exact`（有效内存域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`、`external_dependency_tested`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x00427B8F..0x00427E71`

共享 opcode：`1,2,3,4,5,6,89,90`

直接 helper：`sub_40C0D0`、`sub_487C10`、`sub_40AFF0`、`sub_40BB20`、`_AIL_serve@0`

## 1. 八个变体与混合字节布局

八个 opcode 共用同一入口，但 mode、固定头与奇偶副作用不同：

| opcode | mode | text 起点 | 固定字段 | 初始 record flag | 奇数副作用 |
| ---: | ---: | ---: | --- | ---: | --- |
| `1/2` | 0 | `ip+6` | `+2 selector, +4 action` | 0 | 仅 1 |
| `3/4` | 1 | `ip+14` | 再含 `+6 left,+8 top,+10 columns,+12 rows` | 0 | 仅 3 |
| `5/6` | 1 | `ip+14` | 同上 | `0x40` | 仅 5 |
| `89/90` | 2 | `ip+10` | 再含 `+6 columns,+8 rows` | 0 | 仅 89 |

text 以字节步进扫描到内存序 `25 51`（`%Q`）并消费这两个字节。首个 u16 同时是 selector 和传给 `sub_40AFF0` 的混合载荷前缀；不能把本组建模成普通 C 字符串或统一固定头。

fetch 仍先执行 `raw_word & 0x3FFF`，所以每个有效 opcode 的四个 raw alias 共享完整语义。测试另固定 opcode 90 的 `005A/405A/805A/C05A`。

## 2. selector 与 detached record

`0x00427B97..0x00427BD4`：

1. 读取 `u16(ip+2)`；
2. `0xFFF0` 改为当前 context `+0x24`，并把替换值写回脚本窗口；
3. `0xFFFD` 保留为 detached context；
4. 其他值调用 `sub_40C0D0` 取得 role index，但脚本中的普通 selector 保持原值；
5. 分配 0x4C bytes，逐 dword 清零。

现代实现用 detached `std::list<LegacyDialogMessage>` node 表示“已分配但尚未链入”的 0x4C record，成功尾部再 `splice` 到全局消息链；这样分配失败发生在第一处原 record dereference 之前，后续失败不会把半成品伪装成已排队消息。

selector lookup 失败时原程序仍会完成 record 分配、首次 audio service 和 `sub_40AFF0` 的 text prepare，随后在 caller 写 `role[index].interaction_gate` 时以 `-1` 索引触及危险点。现代边界在同一阶段返回 `role_not_found`，保留之前可见副作用，不提前短路。

## 3. caller flags、anchor 与首次 audio

caller 在进入 `sub_40AFF0` 前按原顺序建立：

- opcode 5/6 的 `record.flags |= 0x40`；
- `text_control_flags` bit 31/30/29/26 清零分别映射 record `0x20/0x400/0x80/0x02`；
- `next_text_aux_pending` 写 `lifetime_limit` 并置 `0x08`；
- `0x004A135C` 的 low word 不为 `0x8000` 时，以 camera + 两个 u16 写 record `anchor_left/top`；
- selector `0xFFFD` 先从 context `world_x/world_y` 写 anchor，再允许上述一次性 override 覆盖。

随后在 `0x00427D60` 直接 `_AIL_serve` 一次，才调用 text prepare/record setup。现代 `ports.service_audio()` 位于同一边界；缺 `%Q`、resolver 分配失败和 role lookup 失败都只保留这第一次 service。

`dialog_scale` 与 `dialog_character_delay_base` 分别拥有原 `dword_4A99F8` 和 `dword_4A9ECC`；启动默认 11/2，但不再把可配置全局硬编码成 record 常量。`sub_40E0B0` 不拥有或重置它们。

## 4. `sub_40B7F0` 窄合同与 mode0 三档

`sub_40AFF0` 把 mode 对应 text 到 `%Q` 的 bytes 复制到受检 owner，再调用 `sub_40B7F0`：

- `%T<decimal>.` 尝试从 `mon.dat` 装载记录并替换名字；解析/装载失败只走诊断并保留原 token，不终止 handler；
- 最终计数从 1 行开始；普通 byte 累计，每行超过 20 bytes 自动增行；
- `%N/%L/%K/%P` 消费 2 bytes、清当前行计数并增行；
- `%S/%C` 与内存序 `D%` 消费 3 bytes；
- `%B/%A` 消费 2 bytes；
- `%Q` 停止。

VM 通过 `prepare_dialog_text(source,destination)` 窄 port 请求完整 `%T/mon.dat` 外部转换。port 返回 false 时严格使用原 resolver-failure 路径并保留 source；SDL 当前没有 mon.dat typed owner，因此明确返回 false，不伪造一个名字。测试替身覆盖 success 返回 prepared bytes，证明 VM 使用转换后 bytes 计数和排队。后续 resource owner 可接真实 mon.dat，而无需改 opcode/IP/record 合同。

尺寸顺序：

1. `text_control_flags` bit 27 清零时，任意 mode 使用 `visible_bytes * scale`、`2 * scale`；
2. 否则 mode0：行数 `<=3 / ==4 / >4` 分别使用 `16x6 / 18x8 / 20x10` 再乘 scale；
3. mode1 从 `+10/+12` 取 columns/rows；
4. mode2 从 `+6/+8` 取 columns/rows。

原文本副本链 `0x004C99F8` 只有本 helper 和 opcode125 两个追加点，完整 LST 无业务读取者；现代不复制该无消费者缓存，不影响 record text owner。

## 5. record、几何与角色 gate

`sub_40AFF0` 继续按原 0x4C layout：

- frame action 按 `+4 action id` 在四项表查找，未命中诊断后回退 slot 0；
- frame action 先 update，speaker name 非空时再分配 caption、update caption action；
- role index、transition、delay/countdown、颜色 4/4、style 4、text/caption pointer token 按原偏移写入；
- character delay 为 `2 * dialog_character_delay_base`；
- `0x004CF73C` center latch 在初始 left 上减半 width。

mode1 保留 payload left/top。mode0/2 在 anchor 为零时按 role/context 建立中心点；role index 0 原分支跳过自动中心，这个看似异常的行为保留。随后：

- opcode104 对应 bit 28 清零时使用 `text_layout_first/second`；
- 否则普通 role 使用八项 facing offset，detached context 使用 `(0,-104)`；
- 第一轮 clamp 为 `left>=30, top>=40, right<576, bottom<456`；
- 非显式 layout、非 `0x80`、非 detached 且仍覆盖 role screen point 时，再加一次 facing offset，并按 `24/32/576/456` clamp。

`sub_40AFF0` 返回后 caller 先把 role/context gate 写 2。奇数 opcode `1/3/5/89` 再：

- `record.flags |= 0x10`；
- gate 改写为 1；
- `dword_4A9920` 对应 `flagged_dialog_counter` 加一。

偶数 `2/4/6/90` 保持 gate 2，不递增 counter。

## 6. 链入、IP、reset 与公共 join

caller 链入 record 后再次按 byte 扫到原脚本 `%Q`，把 context IP 写为 `%Q` 后一 byte。然后一次性 reset：

- anchor override 两个 word → `0x8000/0x8000`；
- text flags → `0xFFFFFFFF`；
- next aux value/pending → `60/false`；
- speaker buffer只清 byte 0，后续 stale bytes 保留；
- text layout pair → `0/0`；
- center latch → false。

跳到 common join 后先把当前 opcode 写入 previous owner，再因 continue flags 为零调用第二次 `_AIL_serve` 并返回 1/yield。现代同样在第二次 service 前写 `state.previous_opcode`；测试固定 dialog→default 的 previous 交接。

## 7. 受检失败顺序

- selector u16 不完整：在 record 分配/audio 前返回 `operand_out_of_range`；
- `%Q` 缺失：detached record 已分配，首次 audio 已发生，prepare/action/gate/queue/reset/第二次 audio 尚未发生；
- prepare 分配失败：首次 audio 与 prepare request 已发生，返回 `dialog_allocation_failed`；
- role 缺失：首次 audio 与 prepare 已发生，停在原 caller gate 的危险索引点；
- 以上失败都不推进 IP、不发布 previous、不 reset one-shot globals。

有效域内 callback 顺序由测试固定为：

```text
audio → prepare → frame action update → [caption update] → audio
```

## 8. 资产与验证

完整 TALK 静态目录包含本组 4392 条物理记录：

```text
1:294, 2:125, 3:3, 4:2187, 5:4, 6:559, 89:1103, 90:117
```

机械检查 4392/4392 均满足各自 6/14/10-byte 头且以 `%Q` 结束；长度为奇数的记录按 byte scan 合法保留。real CTest 另从原 `TALK1.DAT/TALK4.DAT` 各回放一条八变体物理记录；opcode1 样本含 `%T`，SDL/测试 false resolver 路径确认 token 保留。

synthetic、real、initial-session-real 三项定向 CTest 为 3/3。最终完整门禁为 Linux core 186/186、Linux app 192/192、Windows LLVM app 192/192；三个 build/test 进程均 lifecycle exit 0，且没有启动原版或 OpenSWD3 游戏 EXE。

## 9. 停止线

本组关闭后 workpack 为 2/146。下一行严格是：

```text
0x00427E72
opcode 7
```

不得把本组经过的 common join 证据扩展成其他 handler 已关闭；opcode7 必须重新独立审计其单条 bit 清除、IP/continue 和公共尾。
