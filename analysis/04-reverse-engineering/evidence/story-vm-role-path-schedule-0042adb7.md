# 剧情 VM 角色路径批量调度共享 handler：0x0042ADB7

状态：`platform_adapted`、`assembly_exact`（有效内存/owner 域）、`unit_tested`、`real_asset_tested`、`sdl_runtime_integrated`；原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据：`swd3.exe_export_for_ai/swd3.exe.lst`

入口：`0x0042ADB7..0x0042B06E`

共享 opcode：

- `20`：C++ 语义常量 `OP_20_SCHEDULE_ROLE_PATHS`；每 record 6 bytes；
- `169`：C++ 语义常量 `OP_169_SCHEDULE_ROLE_PATHS_WITH_ACTIONS`；每 record 12 bytes。

直接 helper：`sub_40C0D0`、`sub_42DAF0`、`sub_42E280`；`nullsub_1` 只承接旧诊断。

## 1. count/phase word 与两种 record

两种 opcode 共用 `+2 u16 count_word`：

```text
bits 0..13  record count
bit 14      phase marker：0=schedule，1=wait-ready
bit 15      转发给 sub_42DAF0 的 flag
```

payload 从 `+4` 开始：

```text
opcode20 record, 6 bytes:
+0 u16 role_selector
+2 u16 tile_x
+4 u16 tile_y

opcode169 record, 12 bytes:
+0 u16 role_selector
+2 u16 tile_x
+4 u16 tile_y
+6 i16 action_id
+8 i16 base_variant
+10 i16 variant_delta
```

逻辑长度分别为 `4 + count*6` 与 `4 + count*12`。公共 fetch 的 `raw_word & 0x3FFF` 使每个 opcode 的四个 raw alias进入同一共享入口；handler 用 effective opcode 区分 stride。

## 2. schedule phase（bit14 clear）

handler 按 record 顺序调用 `sub_40C0D0`。resolver miss 只走旧诊断并跳过该 record；不会读取该 record 的坐标/action tail，也不要求 path runtime。resolver 成功后，原顺序是：

1. 若 record selector 等于 context source GUID，清 role `+0x10` 的 `0x00080000`，并把 role `+0x4C/+0x78` 两个 cached action variant dword 置 `0xFFFFFFFF`；
2. role `+0x88` wait override 为0时写 `0x8001`；
3. 读取 `tile_y`、`tile_x`；任一为 `0xFFFF` 时使用 controlled/selected role 对应世界坐标 `>>4`，再统一 `<<4` 传给 helper；
4. opcode20 向 `sub_42DAF0` 传三个 `-1` action 参数；opcode169 从 record `+6/+8/+10` 传三个 16-bit action 参数；
5. count word bit15 原样作为 helper flag；
6. 若 resolved role 是 selected role，设置 dialog close counter bit15。

`sub_42DAF0` 机器返回值不参与 handler 控制流。现代 `schedule_legacy_world_story_path` typed owner 在有效域保留其副作用；owner 缺失或 typed failure只在原 helper调用点 checked-stop，不回滚此前 source-role/wait-override副作用，也不伪造 phase成功。

全部 record处理后，handler只把 operand bit14置1，保留bit15与low14 count；IP不推进，公共 join发布 previous effective opcode并 yield。count0同样先置bit14并 yield。

旧C++只实现 opcode20，并存在三类差异：把 `+0x4C/+0x78` 误写到 one-shot字段、把 `+0x88` 误映射到 interaction gate，以及在任何 record副作用前预检整个 payload/runtime。本轮均按LST顺序修复，并补齐 opcode169。

## 3. wait-ready phase（bit14 set）

handler再次按相同 stride扫描，但每条只读取 `role_selector`：

- role `+0x10` bit `0x02000000` 已置：该 record计为 ready，不调用 helper；
- 否则调用 `sub_42E280(role_index)`，仅返回2计为 ready；返回0/1均保持未完成；
- resolver bool仍被忽略；ordinary miss output在随后的 role flags读取处形成原危险访问，现代只在该点 checked-stop `role_not_found`。

ready count不足时，IP与 operand均不变，公共 join发布 previous并 yield。全部 ready时：

1. operand 与 `0x3FFF`，同时清bit14和bit15；
2. IP推进 `4 + count*stride`，按16-bit context IP自然回绕；
3. 公共 join发布 previous并在同一次VM调用取下一opcode。

wait phase不读取坐标或action tail。因此窗口尾只要各 record selector仍可读，就可完成ready判断并推进到窗口外，下一fetch才返回 `instruction_out_of_range`；不得用统一完整指令预检提前拒绝。

## 4. 测试与真实资产

synthetic测试独立覆盖：20/169各四raw alias；6/12-byte stride；source-role cached字段与wait override精确映射；20的三个`-1` action参数；169的三个record action参数；count0双阶段；bit15转发与完成时清除；`0xFFFF` selected-role坐标；selected-role dialog bit；initial missing selector跳过record tail；valid selector在坐标截断前的副作用；169 action tail截断；wait phase selector-only窗口尾；wait phase resolver miss；not-ready yield；owner缺失时的副作用顺序。真实sample两种opcode均完成schedule→ready→同调用继续。

全资产静态反查：

- opcode20：423条物理指令、515个records；TALK1/2/3/4分别213/110/53/47条；全部raw `0x0014`、长度严格满足`4+6*count`；17个X与17个Y使用`0xFFFF` fallback；资产中bit14/bit15均未预置；
- opcode169：184条物理指令、331个records；TALK1/2/3/4分别18/4/52/110条；全部raw `0x00A9`、长度严格满足`4+12*count`；资产中bit14未预置，3条预置bit15；action_id有308个`0xFFFF`，base_variant有4个`0xFFFF`，variant_delta无`0xFFFF`；
- 两类合计607条物理指令、846个records，长度公式零差异。

真实回放样本：

```text
TALK1.DAT@0x000049F6
14 00 01 00 01 00 22 00 1E 00
opcode20, count1, selector1, tile(34,30)

TALK1.DAT@0x0002A56D
A9 00 01 00 01 00 17 00 25 00 FF FF 00 00 07 00
opcode169, count1, selector1, tile(23,37), actions(-1,0,7)
```

定向 story-path owner、synthetic、real-suite、initial-session-real-suite CTest为4/4；完整Linux core 186/186、Linux app 192/192均以exit 0通过。生成器Python `py_compile`通过且两次重跑幂等；两套真实CMake均已编译本轮全部C++改动。按执行计划v258，小handler不运行Windows；Windows LLVM app只在剧情VM P3大阶段完成时统一编译、集中修复。未启动任何游戏EXE。

关闭后workpack为16/146。下一行严格是共享handler：

```text
0x00428533
opcode 21,22
```

opcode21/22尚未独立审计；现有C++与导航语义不得继承完成状态，两个变体必须同组闭环。
