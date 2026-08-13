# 世界空间角色绘制与距离音频（`0x00413870..0x00413FDD`）

状态：`assembly_exact`、`asset_verified`、`original_diff_verified`（GUID 248/249
角色位置、动作帧与水平模式）；完整 framebuffer/audio/jitter 差分尚未完成。

本文只以 `swd3.exe.lst` 的机器码与指令为行为真值。它固定普通世界画面组合中两次
空间角色扫描、普通角色绘制和距离音频的调用顺序；IDA 伪码与符号名只用于定位。

## 1. 两条空间扫描

`0x00412930` 的 normal 路径先在 service `0x0B` 为零时调用 `0x00413EA0`，随后无条件
调用 `0x00413870`。两者不能合并：扫描组、纵向范围、筛选条件和绘制方式都不同。

### `0x00413EA0` 的 bit 29 角色

- 只读取 `0x004A9A04`，即空间组 0；
- 起始逻辑行为 `trunc_toward_zero(camera_y / 16) - 5`；
- 物理循环计数从 `-10` 到 `<30`，最多四十行；
- 负行跳过，`row >= map_height` 立即结束整个扫描；
- 每行保持 `role+0x00` 链序，只为 flags bit 29 非零的角色调用 `0x00413F00`。

`0x00413F00` 要求 `(flags & 0x8400) == 0x8000`，并使用严格水平范围
`-320 < role_x-camera_x < 960`。动作 id 为零时资源号是 `0xFFFF`，否则取 action
`+0x4A`；它不先更新 action。最终绘制参数固定为：

```text
x       = role_x + role[+0x28] - action[+0x10] - camera_x
y       = role_y + role[+0x2A] - camera_y + 8
flags   = (action[+0x18] & 0x80000017) | 0x16
opacity = 4
```

### `0x00413870` 的普通角色

外层三次迭代通过 `0x0041387F..0x0041389A` 选择空间组，真实顺序固定为：

```text
iteration 0 -> 0x004A9A0C -> group 2
iteration 1 -> 0x004A9A04 -> group 0
iteration 2 -> 0x004A9A08 -> group 1
```

每组起始行为 `trunc_toward_zero(camera_y / 16) - 20`，随后固定扫描七十个行槽。
`0x004138BC..0x004138C6` 的上界是无符号比较 `row < map_height + 20`：

- 负行转换为大无符号数后跳过；
- `0..map_height+19` 都会读取行头，因此地图底部二十行 padding 是可访问范围；
- 超出上界只跳过当前行，不会提前结束剩余七十次循环。

每个链节点严格先调用 `0x00413910`，再检查 `word role[+0x2C]`；低 16 位非零才调用
`0x00413CA0`，最后读取 `role[+0x00]` 后继。重写保留这个 draw→audio→next 顺序，且
用一基 `u32` 索引隔离宿主指针宽度；损坏索引或环只在旧程序将发生无效解引用的位置
返回受检错误，不改变有效链顺序。

## 2. `0x00413910` 普通角色绘制

入口同样要求 `(flags & 0x8400) == 0x8000`，但水平范围包含两个端点：
`-320 <= role_x-camera_x <= 960`。非零 action id 先以 service `0x0B` 门控；通过后，
`role+0x98` 的一次性音效按角色世界坐标提交并清零，再按资源号与 action `+0x4C`
取得 TSW 帧。

可见绘制顺序为：

1. flags bit 8 非零且 `frame_counter & 7 < 4` 时调用 `0x004145F0` 绘制变形残影；
2. 以 action flags、byte `+0x8A` opacity 和 TSW `+0x04` auxiliary 绘制主图；
3. bit 8 的三项全局颜色与 flags bits `20..23` 的表值相加，任一非零时用
   `(action_flags & 0x80000013) | 0x10` 再绘一次；
4. `role+0x3C` 非零时解析另一 action，按其 TSW/flags 绘制覆盖层；
5. 把 blitter 全局 phase 的低字节写回 action `+0x89`；
6. flags bit 9 非零且 service `0x48` 为零时提交角色粒子；
7. talk target 为 `0xFFFF` 且 `role+0x38` 非零时，按原始 byte 字符串长度、颜色表和
   style 4 绘制标签。

普通主图坐标是：

```text
x = role_x + role[+0x28] - action[+0x10] - camera_x
y = role_y + role[+0x2A] - action[+0x14] - camera_y
```

覆盖层 Y 还会同时减去覆盖 action 与主 action 的两个 Y offset，再加常量 28。
`0x00413B5B` 虽暂时把 camera Y 装入 EDI，但 `0x00413B72/0x00413B8D` 两条路径都会
在粒子调用前恢复 EDI 为角色世界 Y；不存在分支相关的错误 Y 参数。

## 3. 残影继承上一笔 jitter 的原始顺序

残影调用位于 `0x004139D3..0x00413A13`，而当前角色的 jitter group/phase 直到
`0x00413A1B..0x00413A31` 才从 action `+0x88/+0x89` 写入
`0x004CD724/0x004CD758`。因此：

- 残影使用进入本角色前由上一笔 blit 留下的全局 group/phase；
- 残影即使推进全局 phase，随后也会被当前角色 `+0x89` 覆盖；
- 主图、颜色叠加和覆盖层共享当前角色加载后的状态；
- 函数末尾只把最终 phase 的低字节写回当前 action。

重写以同一个 `LegacyRleRowJitterState` 贯穿空间链和实际 blitter。UT 固定了“前一角色
主图结束于 group 2/phase 12，后一角色残影先看到该值，随后主图切换到自身 group 3 /
phase 20”的顺序，避免把 jitter 错误降格成单角色局部变量。

## 4. `0x00413CA0` 距离音频

监听者来自受控角色索引。距离先按 32 位回绕计算 `dx*dx + dy*dy`，再走 x87
`fild/fsqrt` 与原转换 helper；负的回绕平方和保持原来的 indefinite 低双字结果零。

`role+0x30` 高低 word 是原周期调度器：高 word 不是 `0xFFFF` 时先递减低 word；
递减后非零便写回且本帧不能启动，等于零则允许启动。距离大于 512 或 flags bit 15
为零时，若 bit 24 表示循环音仍活动，就停止音效并清 bit 24。距离恰好 512 仍进入更新。

有效路径按 GUID 查找第一个 flags bit 28 为零的角色索引。允许启动且 bit 24 尚未置位
时，把高 word 重载到低 word；高 word 为 `0xFFFF` 的无限调度会得到 `0xFFFFFFFF`、
置 bit 24，并以固定 `(volume=0, pan=0, loop=1)` 启动样本。随后：

```text
stored_distance = trunc_toward_zero(distance * 128 / 512)
volume          = trunc_toward_zero((128 - stored_distance) * mix / 11)
pan             = trunc_toward_zero(((role_x-listener_x) << 6) / 512)
```

垂直数组保留汇编只取 Y 低 word、左移六位后再符号扩展的回绕行为。GUID 未命中、受控
索引无效或输出数组不足只在旧程序会越界写的位置形成现代隔离状态。

## 5. 实现与验证

- `draw_legacy_world_flagged_roles` / `draw_legacy_world_flagged_role` 映射
  `0x00413EA0/0x00413F00`；
- `draw_legacy_world_role` 映射 `0x00413910` 及其 `0x004145F0` 残影调用合同；
- `update_legacy_world_spatial_audio` 映射 `0x00413CA0`；
- `draw_legacy_world_roles` 映射 `0x00413870` 的三组、七十行和 draw→audio 顺序；
- `LegacyWorldRoleRenderRuntimePorts` 把 TSW runtime、当前 clip、效果状态、共享 jitter 和
  RGB565 framebuffer 接到普通角色路径；service、音频、覆盖 action、粒子和标签继续由
  窄外部端口提供，不引入平台依赖。
- `compose_legacy_world_runtime_frame` 在 `0x00412930` 的原 stage 槽实际调用上述两条
  空间路径；相机、talk target、角色数组、clip、framebuffer 与 jitter 不再由测试各自
  拼接。

合成测试覆盖严格/包含裁剪边界、三个空间组顺序、负行与底部 padding、链环、调度低字
回绕、512 距离边界、无限循环、被忽略的 blitter 失败及所有现代隔离门。当前真实 TSW
资源的固定 RGB565 framebuffer FNV-1a64 为：

- bit 29 角色：`0xA6C3E08156F06060`；
- 普通角色：`0xA4766C928B05DC88`。
- 底图叠加 bit-29 与普通角色的整帧纵向切片：`0xA6144A91E57939F9`。

原版 `0x00413910` 动态采集固定 GUID 248 使用 `TSW 188/4`、目标 `(309,240)` 和
`mode_flags=1`，GUID 249 使用 `TSW 188/8`、目标 `(235,258)` 和 `mode_flags=0`。
宽 63 的真实 188/4 帧在 bit 0 下由 `0x004185C0` 水平反转；这解释了蓝衣角色有效像素
相对正向帧向右镜像 18 像素，而红衣角色保持正向。真实数据回归覆盖这两个模式位从地图
初始化到首帧的保留。

原程序完整逐帧 framebuffer、音频调用和 jitter 状态差分仍登记为
`blocked_runtime_oracle`；如需验证只
准备 Frida spawn 工具并等待用户执行，不由开发流程自行启动原版。
