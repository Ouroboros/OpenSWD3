# 暂停层、动作精灵、行特效与限时消息

状态：`assembly_exact`

本单元只以完整 `swd3.exe.lst` 为行为真值，覆盖五个在同一帧绘制簇内、但借用
不同 owner 状态的协调函数。IDA 伪码和旧错误字符串只辅助命名，不覆盖指令顺序。

| 地址 | 行为 | OpenSWD3 映射 |
|---|---|---|
| `0x00411FA0` | 暂停面板、文字及立即提交 | `draw_legacy_pause_overlay` |
| `0x00414B60` | 世界坐标动作精灵更新、绘制与退休 | `update_draw_legacy_moving_action_sprites` |
| `0x00414CE0` | 角色头像动作、缓动/弹道与退休 | `update_draw_legacy_role_head_sprites` |
| `0x00414E50` | 五模式 packed-row 特效队列 | `update_draw_legacy_packed_row_effects` |
| `0x004153D0` | 右对齐限时消息队列 | `update_and_draw_legacy_timed_messages` |

动作记录仍由后续 `asset_runtime/story_scene` 生产和拥有；B4 使用中性定宽字段视图及
端口，不复制尚未恢复的旧全局对象布局。现代 `std::list` 只替代旧节点的分配与链接，
遍历顺序、当前/前驱推进和删除时机仍由以下汇编合同决定。

## 1. `0x00411FA0` 暂停层

原函数复制 `0x004A0360` 的 23 字节缓冲，22 个非 NUL byte 是固定 CP950 文本：

```text
B9 43 C0 B8 BC C8 B0 B1 20 20 AB F6 46 38 C4 7E C4 F2 B9 43 C0 B8 00
```

布局不按 Unicode 字符数，而按 `lstrlenA` 的 byte 数计算：

```text
half = trunc(22 / 2) = 11
x = 320 - 11 * half = 199
width = 22 * half = 242
y = 229
height = 22
```

随后严格执行三步：

1. `sub_43BAB0(x,229,width,22,0,0,0,0)` 绘制效果面板；
2. `sub_4239D0(25,23,17)` 产生 packed 颜色，以 20×20 renderer、`flags=4`
   在同一 `(x,229)` 绘制原始 byte string；
3. 在 `0x00412046` 对 game framebuffer 做 full-surface、immediate present，返回值
   继续返回给本来就忽略它的调用者。

面板或现代 provider 报错不能跳过后续文字和 present；这与原调用者忽略各绘制返回值
一致。现代静态 byte array 替代一次性 0x40-byte 临时分配，不改变可观察布局和调用序列。

## 2. `0x00414B60` 世界坐标动作精灵

每个节点先把 `float position_x/position_y` 通过 `sub_489654` 转成整数。该 helper 临时把
x87 rounding control 设为向零截断、`fistp qword`，调用者使用低 32 位；之后减去相机
`x/y` 得到屏幕坐标。动作更新 `sub_4321E0` 即使失败也只写空日志，仍继续后续流程。

可见性是四个严格比较，等号在外：

```text
-72 < screen_x < 712
-72 < screen_y < 552
```

可见时用更新后的 `resource_id/frame_index` 取帧，传入资源描述中的 palette、节点
`flags` 和 byte opacity，在 `(screen_x-offset_x, screen_y-offset_y)` 绘制。更新对不可见
节点同样执行。

节点 `+0x44 != 0` 时本帧不移动也不退休。为零时先执行两个单精度位置加法并写回，
再向零截断新位置；只有新 `x/y` 同时严格落在各自目标的 `±32` 内才立即 unlink/free。
恰好等于四条边界不删除。绘制使用移动前坐标，退休判断使用移动后坐标。

## 3. `0x00414CE0` 角色头像动作

每个节点无条件按以下顺序执行：动作更新、取帧、以 palette=null 直接源绘制、再更新
横坐标。绘制点是两个有符号 word：

```text
x = signed(current_x) - offset_x
y = signed(current_y) - offset_y
```

横向更新由 `(velocity_bits & 0x7fff)` 选择，因而 `0x0000` 和 `0x8000` 都走缓动：

```text
step = trunc_toward_zero(2 * (target_x - current_x) / 3)
current_x = low16(current_x + step)
if -1 <= step <= 1: current_x = target_x
```

缓动分支从不做屏幕外删除。其他速度走 16 位回绕的弹道分支：先
`current_x += velocity`，再 `velocity *= 3`；更新后的有符号 `current_x <= -120` 或
`>= 760` 时才立即删除。边界包含等号，且节点始终先绘制一次再可能删除。

## 4. `0x00414E50` packed-row 特效

节点保存有符号 `base_x/base_y/limit/row_count/color_index`、低 byte 未解释标记、高
byte mode，以及两个逐行 word 数组。动态模式从 `row_count-1` 逆序到零；简单模式从
零正序。所有 word 加减、乘三和 `limit-offset` 都保留低 16 位，再按有符号 word 比较。

| 高 byte | 每行更新和绘制 | 全部完成后 |
|---:|---|---|
| `0x08` | `(base_x,base_y+i)` 绘制固定 `limit` | 保留 |
| `0x80` | `length += 48 + 2*rng(6)`，`>=limit` 夹到 limit | mode 改为 `0x08` |
| `0x40` | `offset += -48 - 2*rng(24)`，`<=0` 夹到 0；`length=limit-offset` | mode 改为 `0x08` |
| `0x20` | `offset += 48 + 2*rng(48)`，`>=limit` 夹到 limit；`length<=1` 强制 2 | 释放两数组和节点 |
| `0x10` | `length += -48 - 2*rng(48)`，`<=2` 夹到 2 | 释放两数组和节点 |

mode 转为 `0x08` 时保留原低 byte，而且不会在同一次调用补跑简单模式：原指令先检查
`0x08`，才依次检查 `0x80/0x40/0x20/0x10`。随机源固定为已还原的
`LegacySecondaryRng`，不能换成 CRT RNG。

每行最终调用 `sub_417DE0(destination,color_pattern,length)`。该底层 packed 像素循环仍是
下一组真实缺口，因此当前协调器通过 `LegacyPackedRowDrawPorts` 保留精确坐标、pattern、
有符号长度和调用顺序；没有把它误报成已经恢复的像素公式。

## 5. `0x004153D0` 限时消息

函数先把 12×12 renderer 的背景设为 `0xFFFE`、secondary color 设为零。队列节点物理
文本区从 `+0x04` 到 `+0x83`，固定 0x80 byte；下一指针位于 `+0x84`。每次调用的 y
从 8 开始，每访问一个节点都加 24，无论该条是否实际绘制。

每个节点先查询 `sub_40DC50(0x0E)`。结果为零才用 `lstrlenA` 计算并绘制：

```text
x = 640 - 11 * byte_length
y = 8 + 24 * queue_index
flags = 0x10
foreground = word at 0x0049E0D4
```

随后 lifespan 做 32 位回绕减一；结果恰好为零时立即 unlink/free，否则保留。初值为零
会变成 `-1` 而不是被删除。现代固定数组缺少 NUL 时只做安全隔离并报告
`missing_terminator`；正常 byte 长度、输入查询次数、右对齐和寿命顺序不变。

## 6. 验证

独立 UT 固定并通过：

- 暂停文本 23 个原始 byte、`199/229/242/22` 布局、RGB555 `0x66F1`、面板→文字→
  immediate present 顺序；
- 世界精灵的四条严格可见边界、不可见仍更新、移动前绘制、移动后目标窗口删除；
- 头像 direct-source 绘制、`0x8000` 特殊缓动、`[-1,1]` snap、16 位乘三和包含等号的
  屏幕外删除；
- 五个 row mode 的正/逆行序、每行 RNG 次数、夹值、低 byte 保留、转简单与删除；
- 消息 control 14 抑制、0x80-byte NUL 边界、每节点 y 前进、寿命减一后删除和文字状态。

Linux Clang `core` 为 60/60 CTest，Windows LLVM `app` 为 62/62 CTest。原程序
framebuffer 动态差分仍沿用 B4 已登记的 `blocked_runtime_oracle`，本轮没有启动原版程序。
