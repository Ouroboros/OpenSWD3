# 剧情 VM 全局整数共享 handler `0x0042B074`

## 结论

`sub_427920` 的主分派 opcode `29..33` 共用入口 `0x0042B074`，操作持久的 64 项 `dword_4ACBD0` 整数区：

- `29`：赋值；
- `30`：32 位回绕加法；
- `31`：32 位回绕减法，结果符号位置位时把选中项写零；
- `32`：选中项按无符号 32 位大于等于阈值时执行同文件跳转；
- `33`：选中项按无符号 32 位小于等于阈值时执行同文件跳转。

五条正常路径都经过共享尾：若 `dword_4ACBD0[0]` 的符号位置位，就把第零项写零。主分派的 `29..31` 固定六字节，`32..33` 固定十字节；正常路径发布 previous opcode 并在同一次 VM 调用中继续。

本证据只关闭主入口 `0x0042B074` 的 `29..33`。二级分派 `181..185` 从独立入口 `0x0042B070` 进入相同后半段，但值宽度、target位置和物理长度不同，不能继承本条关闭结论；该入口现已在 [`story-vm-wide-global-integers-0042b070.md`](story-vm-wide-global-integers-0042b070.md) 中独立闭环。

## 唯一汇编边界

主分派表把五个有效 opcode 都送到 `0x0042B074`：

```text
29 -> 0x0042B074
30 -> 0x0042B074
31 -> 0x0042B074
32 -> 0x0042B074
33 -> 0x0042B074
```

入口首先执行：

```text
0042B074  movsx edi, word ptr [ebx+2]
0042B078  cmp   [esp+...var_3C], 21h
0042B07D  ja    short loc_42B087
0042B07F  movsx ecx, word ptr [ebx+4]
0042B084  jmp   short loc_42B08B
```

因此主分派的 index 与 value/threshold 都是 `s16`。`0x0042B087` 的 `u32 [ebx+4]` 只属于二级入口 `181..185`；其独立宽值合同见对应evidence。

随后唯一门控为：

```text
0042B08B  cmp edi, 40h
0042B08E  jl  loc_42B0CD
```

这是有符号 `jl`，所以机器域不是简单的 `0..63`：

- `0..63`：访问 64 项整数区；
- 负 index：同样通过门控，在数组基址之前读写；
- `index >= 64`：走纯诊断路径，不访问数组。

高 index 诊断只进入 `nullsub_1`。该路径不经过共享数值尾，IP 不推进，`ESI` 保持零；common join 仍发布 effective opcode，随后调用 `_AIL_serve` 并跨帧让出。因此同一条坏指令会在后续帧重试。这里不能改成推进、终止或同帧忙循环。

## 五个操作

### opcode 29：赋值

`0x0042B0E4..0x0042B0EF` 将符号扩展后的 `s16` value 的 32 位 bit pattern 写入 `dword_4ACBD0[index]`。例如 `FFFF` 写成 `0xFFFFFFFF`，不是 `65535`。

### opcode 30：回绕加法

`0x0042B0F4..0x0042B0FC` 直接执行 32 位 `add`。没有溢出检查，也没有 PATH VM 数值指令所用的 `1000` 上限。story VM 的 `990 + 50` 必须得到 `1040`。

### opcode 31：减法后按符号位归零

`0x0042B0FD..0x0042B133` 先执行 32 位回绕减法，再用 `test eax,eax / jns` 判断结果符号位；符号位置位时把选中项写零。这不是按借位饱和，也不是有符号高精度运算。减去负 `s16` 仍按其符号扩展后的 32 位 bit pattern 运算。

### opcode 32：无符号大于等于跳转

`0x0042B13D..0x0042B171` 先读取 `u32 [ebx+6]` target，再比较选中项与符号扩展阈值。机器使用 `jb` 进入不跳转尾，所以 taken 条件为：

```text
u32(variable) >= u32(sign_extend_s16(threshold))
```

### opcode 33：无符号小于等于跳转

`0x0042B172..0x0042B18B` 共用同一 target，机器使用 `ja` 进入不跳转尾，所以 taken 条件为：

```text
u32(variable) <= u32(sign_extend_s16(threshold))
```

两个条件 handler 都在比较之前读取 target；即使条件最终不成立，`+6..+9` 仍是必读字节。只有 `index >= 64` 的早期诊断路径会在读取 target 之前退出。

## 共享尾、跳转与时序

主分派 `29..31` 的顺序尾把物理长度设为六字节；`32..33` 的顺序尾把长度设为十字节。taken 分支调用 `sub_42E430` 重新装入当前 TALK 文件的目标窗口，并把顺序增量设为零。

无论赋值、加减、条件不成立还是条件跳转返回，正常路径都在 `0x0042B1D9..0x0042B1EC` 检查 `dword_4ACBD0[0]` 的符号位并按需清零，然后设置 `ESI=1`。common join 发布 previous opcode 后立即抓取下一条，不跨帧让出。

现代 `load_same_file_story_window` 是已审计 `sub_42E430` 的 typed port：taken 时先服务 audio、发布新 TALK offset/IP=0、读取窗口，再同调用继续。若现代文件 owner 返回 checked load failure，仍按原调用返回后的机器顺序执行第零项共享 clamp 并发布 previous opcode，然后返回 `load_failed`。

## 负 index 平台隔离

机器对负 index 的行为是数组前越界访问，无法作为可移植 typed owner 的有效地址语义。现代实现保留所有有效域行为，并在机器首次越界读写的位置返回 `script_variable_index_out_of_range`：

- `29..31`：已读取 index/value，尚未写数组时停止；
- `32..33`：仍先读取 target，再在首次读取数组元素前停止。

这不是把机器门控改成无符号比较：`index >=64` 仍严格保留原机“不推进、发布 previous opcode、audio service、yield/retry”的独立行为。负 index checked boundary 仅覆盖原始未定义内存访问；真实资产审计证明当前四个 TALK 文件没有负 index。

因此本 handler 分类为 `platform_adapted`，而不是把负 index 伪报为 assembly-safe。

## 真实资产审计

直接读取四个 `TALK*.DAT` 并按锁定线性记录表核对，主分派 `29..33` 共 44 条：

| opcode | 条数 | 物理长度 | value/threshold 范围 |
| ---: | ---: | ---: | --- |
| `29` | 9 | 6 | `0..7` |
| `30` | 22 | 6 | `1..500` |
| `31` | 0 | 6 | 资产缺席 |
| `32` | 8 | 10 | `1..20` |
| `33` | 5 | 10 | `0..16` |

文件分布为 TALK1/TALK2/TALK3/TALK4=`38/6/0/0`。全部 44 条 raw word 都等于 effective opcode，没有高位 alias。观察到的 index 只有 `0,2,41,50,62`，范围 `0..62`；负 index 与 `index>=64` 均为零命中。

代表性真实记录：

```text
TALK1.DAT@0x00007FAF: 30, index 0, value 50
TALK1.DAT@0x0000FF97: 33, index 62, threshold 2, target 0x0000FDB3
TALK1.DAT@0x0000FFA1: 32, index 62, threshold 4, target 0x0000FDB3
TALK1.DAT@0x0000FFAB: 29, index 62, value 4
TALK1.DAT@0x0000FFB1: raw 0xFFFF terminator
```

将 variable 62 初始化为 3 时，后一段真实链按 `33 not-taken -> 32 not-taken -> 29 set 4 -> terminator` 执行；没有同文件重载，previous opcode 在终止前为 29。

## 现代 owner 与验证覆盖

`LegacyWorldStoryVmState::script_variables[64]` 已是 `dword_4ACBD0` 的持久 typed owner，并与 world PATH runtime 共享；初始化只把 element 0 写为 100，符合既有 `sub_40E0B0` 边界。本次没有增加第二份状态或临时 shadow。

synthetic 测试覆盖：

- 五个 opcode 的四种 raw alias；
- `s16` value 符号扩展；
- set/add/sub 的 32 位回绕；
- opcode 30 无 `1000` 上限；
- opcode 31 结果符号位 clamp 与减负数；
- 32/33 的无符号 taken/not-taken；
- taken reload、同调用继续、audio service 与 checked load failure 顺序；
- 所有正常路径的 element 0 共享 clamp；
- `index>=64` 不读取 target、不 clamp element 0、不推进、发布 previous opcode并yield；
- value/target 截断的阶段读取；
- 负 index 在原始首次越界点隔离。

real-asset 测试覆盖 TALK1 的 opcode 30 记录，以及上述 `33 -> 32 -> 29 -> 0xFFFF` 连续真实链。opcode 31 由全资产零命中证明 `asset_absence_verified`，但仍有完整 synthetic 语义覆盖。

## 关闭判定

主入口 `0x0042B074` 的五个 opcode 已完成独立 LST→C++→LST 收敛：有效域、宽度、回绕、无符号比较、target 读取时机、共享 clamp、IP、previous opcode、same-call/yield、真实资产与负 index 平台隔离均有显式证据和测试。

关闭标签：

```text
assembly_exact;unit_tested;real_asset_tested;asset_absence_verified;platform_adapted;sdl_runtime_integrated
```

后续状态：二级入口 `0x0042B070` 的 opcode `181..185` 已按其 `u32` value、`+8` target、8/12字节长度独立审计和关闭；其计数与证明不复用本证据。
