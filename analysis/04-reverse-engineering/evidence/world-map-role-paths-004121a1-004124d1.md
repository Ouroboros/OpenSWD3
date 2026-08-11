# 普通世界地图角色路径循环（`0x004121A1..0x004124D1`）

状态：`assembly_exact`、`unit_verified`、`sdl_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。IDA 伪码只辅助阅读，不能覆盖
下述顺序、位宽、符号扩展或跳转条件。

## 1. 固定遍历与对象槽布局

`0x00412116` 把循环计数设为 `0x48`，`0x0041219C` 令 EBP 指向首槽 `+0x10`，每轮
在 `0x004124C6` 增加 `0x21C`。因此该 owner 每帧固定扫描 72 个活动对象槽。

| 槽内偏移 | 位宽 | 当前含义 |
|---|---:|---|
| `+0x00` | 16 | 角色索引；`0xFFFF` 跳过 |
| `+0x02` | 16 | 路径游标；无符号 `>= 0x7FFF` 跳过 |
| `+0x04/+0x06` | 16+16 | 到达目标 X/Y，比较时零扩展 |
| `+0x10/+0x12/+0x14` | 16×3 | 到达时可选 action id/base variant/variant delta；`0xFFFF` 保留，否则符号扩展 |
| `+0x16/+0x18` | 16×2 | 本帧 X/Y 步长，符号扩展 |
| `+0x1B` | 8 | Talk 跳过低半字节与到达回调 bit 7 |
| `+0x1C+cursor` | 8 | 八方向编号或 `0xFF` |

方向的格偏移来自 `0x00499534/0x00499554`：

```text
X = [ 1, 0,-1,-1,-1, 0, 1, 1 ]
Y = [ 1, 1, 1, 0,-1,-1,-1, 0 ]
```

action variant 表 `0x00499574` 为 `[5,1,6,2,4,0,7,3]`。

## 2. 每槽精确顺序

1. 角色必须带 flags bit 15。当前 Talk source 等于角色 GUID 且槽 `+0x1B` 低半字节为
   1 时整槽跳过；flags bit 11 与角色 `+0x26 == 1` 也整槽跳过。
2. action `wait_remaining != 0` 时跳过路径移动和自动 Talk，但仍执行 action 更新以及
   选中角色的镜头重定位。
3. 读取两个有符号 16 位步长。action `+0x94 == 0` 时先翻倍，角色 flags bit 26 时再
   翻倍；全部加法保留 32 位回绕。方向不是 `0xFF` 时先写 action variant。
4. 坐标任一低四位非零时，不移动空间链、格指针、表面占用或路径游标，直接进入自动
   Talk、action 更新和镜头门。
5. 两轴对齐时，先按 GUID 从空间行链解下并按新 Y 重插。到达目标时再清等待、应用
   `0xBBFFFFFF`/bit 25 flags 变换和三个可选 action 字段。
6. 槽 bit 7 为零时，在原槽调用 `sub_42D920(role_index)`；其后清 action wait 与角色
   bit 31。该函数属于 `story_scene`，OpenSWD3 只保留显式跨模块 port；未接通时停止，
   不伪造剧情副作用。
7. GUID 1 到达时清四个玩家/镜头 transition 和固定 `0x200` 字节状态；runtime bit 1
   未置位时调用 `sub_40D0C0`。随后无条件清角色 bit 18。
8. 清旧表面足迹；方向不是 `0xFF` 时按方向表移动格索引；标记新足迹。之后重新读取
   槽内路径游标（因为 `sub_42D920` 可能改槽），执行 `(cursor + 1) | 0x8000`。角色
   flags bit 8 置位时，最后从新格投影 flags。
9. Talk source 为 `0xFFFF` 且角色 flags bit 13 置位时，用 `sub_40BB50` 的 `0x100`
   掩码探测八方向占用；非零便从角色 `+0x14/+0x20/+0x1E/+0x24/+0x10` 建立 Talk
   context。
10. 每个未被前置门跳过的角色都调用 action updater。失败只进入原 `Act Err` 诊断槽，
    不终止循环。角色索引等于当前选中角色且 runtime bit 1 未置位时，最后调用
    `sub_40D0C0` 重定位 640×480 镜头。

## 3. 实现所有权

- `LegacyWorldMapRolePathState` 拥有 72 个 `0x21C` 活动对象槽、Talk context 和 GUID 1
  到达时清零的 `0x200` 字节状态。
- 角色数组、空间行链、共享表面格、玩家 movement 和 camera 由普通世界 session 借用；
  空间重插与表面足迹复用既有共同 owner。
- `sub_40D0C0` 已提取为共享 `recenter_legacy_world_camera`，初始世界会话和逐帧路径循环
  使用同一份有符号比较、边缘钳制和 640×480 矩形逻辑。
- `run_legacy_world_frame` 在玩家/相机 transition 移动后直接执行本 owner；原
  `map_role_actions_004121a1` delegated stage 已删除。新 Talk source 随即同步到本帧
  composition state。

## 4. 验证边界

专用 UT 固定了 72 槽扫描、两个整槽跳过门、action wait 门、双倍步长、未对齐路径、
八方向 variant、空间链重插、表面清除/重标、到达 flags 与有符号 action 覆盖、
`sub_42D920` 后重读游标、GUID 1 清理与双镜头门、自动 Talk，以及 action 更新失败的
非致命行为。coordinator UT 同时证明该范围已在真实帧顺序中替代外部占位。

Linux LLVM `core` 151/151、Windows LLVM `app` 155/155 CTest 通过。真实初始
`MAPS + LMF + ACT + TSW` 会话会扫描当前为空的 72 槽并完成普通世界帧；活动路径的
原程序动态状态差分尚未执行，因此不能标记 `original_diff_verified`。如果后续需要该
动态证据，只准备 Frida 工具并等待用户运行，不由开发流程启动原版。
