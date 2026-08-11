# 倒计时绘制与初始化 `0x004308C0–0x00430B60`

状态：两个函数已按完整 LST 逐基本块复核并实现；正常资源路径为
`assembly_exact`、`world_runtime_integrated`，缺失或损坏动作帧由显式安全状态隔离。

## 证据范围

唯一行为真值是 `swd3.exe.lst`：

- 绘制函数：`sub_4308C0`，`0x004308C0–0x00430B52`；
- 初始化函数：`sub_430B60`，`0x00430B60–0x00430BDE`；
- 标志查询/设置：`sub_40DC50`、`sub_40DC80`；
- 动作更新/帧解析：`sub_4321E0`、`sub_4315D0`；
- 最终软件 blit：`sub_4170E0`。

绘制函数有三个直接调用点：

| 调用点 | 所属路径 | `(x, y, mode)` |
|---|---|---|
| `0x004126C7` | 世界地图 | `(400, 8, 0)` |
| `0x0045346C` | 战斗 | `(400, 8, ebx)` |
| `0x00453476` | 战斗 | `(10, 8, esi)` |

初始化函数有两个直接调用点：剧情 opcode 142 的 `0x0042C749` 和战斗的
`0x0045438F`。所有调用者都忽略函数返回寄存器。

## 可见性和计时源

`mode` 的分支不是普通二选一：

- `mode == 0`：标志 `0x10` 未设置便直接返回，使用 `dword_4CAD20`；
- `mode == 1`：标志 `0x4A` 未设置便直接返回，使用 `dword_4A9918`；
- 其他值：不查询活动标志，仍使用 `dword_4A9918`；
- 上述路径继续后都查询标志 `0x4C`，其已设置时不绘制。

选中的 32 位 tick 值按有符号数除以 30；汇编以 `0x88888889` 乘法和移位
实现该除法。商小于零时钳制为零。实现使用等价的 C++ 有符号除法，并先以
bit-cast 恢复原始有符号解释，没有把 `0xFFFFFFFF` 错当成很大的正倒计时。

## `M:SS` 动作帧序列

函数使用静态动作对象 `0x232C`，进入绘制段时把其 `+0x34` 字段清零。每个
显示片段依次写动作索引、调用 `sub_4321E0`，再用返回的两个 word 通过
`sub_4315D0` 取得帧。帧的 `+0x0C/+0x0E` 是无符号宽高，像素源交给
`sub_4170E0(x, y, width, height, 0, 0)`。

片段顺序为：

```text
seconds / 600                    仅非零时绘制
(seconds / 60) % 10              分钟个位
10                               冒号
(seconds % 60) / 10              秒十位
(seconds % 60) % 10              秒个位
```

因此 754 秒产生 `[1, 2, 10, 3, 4]`，65 秒产生 `[1, 10, 0, 5]`。每个
片段绘制后，后续 x 按该帧的无符号 16 位宽度推进；首位省略时保持原始 x。
最后一个片段之后原函数不再读取推进结果，现代接口同样不暴露该内部值。

## 初始化合同

`sub_430B60(minutes, seconds, primary_transition_value, mode)` 用 LEA 链计算：

```text
ticks = 30 * (seconds + 60 * minutes)
```

所有算术都是 x86 32 位回绕。`mode == 0` 时：

1. 清零 `dword_4C97E8`、`dword_4C97EC`；
2. 写入 primary tick `dword_4CAD20`；
3. 写入 `dword_4A93D4 = primary_transition_value`；
4. 按顺序设置标志 `0x10`、`0x12`。

`mode != 0` 时：

1. 清零 `dword_4BAB78`、`dword_4BAB7C`；
2. 写入 secondary tick `dword_4A9918`；
3. 不读取第三参数的业务值；
4. 设置标志 `0x4A`。

实现用无符号位运算显式保存 32 位回绕，避免 C++ 有符号溢出未定义行为。

## 现代安全边界

原函数假定动作更新和帧解析总会返回有效对象，并会直接解引用。现代 Provider
只在缺失帧、零宽高或 blitter 发现损坏源时返回独立错误状态。有效资源的动作
顺序、坐标推进、裁剪和像素路径不变，没有借机修复任何游戏逻辑。

## 普通世界原槽接线

`run_legacy_world_frame` 已在世界组合 `sub_412930` 返回后、条件地图标记
`sub_413FE0` 之前的 `0x004126C7` 调用现有 `draw_legacy_countdown`，固定请求仍为
`(400, 8, 0)`。它直接借用同一软件 framebuffer、raster clip、blit effect 与 row
jitter；标志查询转发到原 `sub_40DC50` 对应的 world service port。

`LegacyWorldFrameCoordinatorState` 持有 primary/secondary tick 和原静态 `0x98`
动作记录。动作记录只在可见性与 `0x4C` 抑制门全部通过后的首次 piece 请求时写入
`action_id=0x232C`、`variant_delta=0`；未激活路径不会因现代适配器提前修改它。随后每个
piece 依次写 `base_variant`，复用实际 action updater 与 TSW frame provider。原先的
`fixed_ui_004308c0` 外部占位已删除。

## 实现与验证

- `include/openswd3/rendering/legacy_countdown.hpp`；
- `src/rendering/legacy_countdown.cpp`；
- `tests/unit/rendering/legacy_countdown_test.cpp`。

UT 覆盖 primary/secondary 初始化、32 位回绕、标志设置顺序、两种标准显示
序列、负 tick 钳制、三个 mode 分支、`0x4C` 抑制，以及无效帧安全状态。

独立 countdown UT 之外，world coordinator UT 还固定了活动 `754` 秒的五 piece
顺序、`(400,8)` 首尾像素、静态动作最终字段、未激活零修改，以及第三个 piece 缺失时
在 `0x004308C0` 停止。真实初始世界则固定抵达该槽并命中未激活门。

当前验证结果：Linux LLVM `core` 153/153、Windows LLVM `app` 157/157 CTest
全部通过；验证过程没有启动原版或 OpenSWD3 EXE。
