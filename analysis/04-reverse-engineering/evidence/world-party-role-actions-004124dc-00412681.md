# 普通世界队伍角色动作循环（`0x004124DC..0x00412681`）

状态：`assembly_exact`、`unit_verified`、`sdl_runtime_integrated`；尚未
`original_diff_verified`

本文只以 `swd3.exe.lst` 的机器码和指令为行为真值。IDA 伪码只辅助阅读；循环边界、
先后顺序、符号扩展和空间链行为均以 LST 为准。

## 1. 循环边界与槽布局

`0x004124D7` 读取队伍数量，数量无符号 `<= 1` 时直接跳到 `0x0041268C`。否则索引从
1 开始，以 `0x21C` 为步长访问队伍对象槽，直到低 16 位索引不再小于队伍数量。索引
0 是当前主角，不进入本循环；原全局槽容量为 8，现代 owner 对超过容量的数量明确停止。

本段实际访问的槽字段为：

| 偏移 | 位宽 | 含义 |
|---|---:|---|
| `+0x00` | 16 | 角色索引；`0xFFFF` 跳过 |
| `+0x02` | 16 | 路径游标；无符号 `< 0x7FFF` 才移动 |
| `+0x16/+0x18` | 16×2 | X/Y 步长，符号扩展后只加一次 |
| `+0x1C+cursor` | 8 | 八方向编号 |

方向到 action variant 的表仍是 `0x00499574` 的 `[5,1,6,2,4,0,7,3]`；格偏移仍来自
`0x00499534/0x00499554` 的八方向 X/Y 表。

## 2. 与地图角色循环不同的精确行为

1. 只要槽内角色索引不是 `0xFFFF`，最终就会调用 action updater。路径游标
   `>= 0x7FFF` 只跳过移动，不跳过 action 更新。
2. 活动路径在 `0x00412531` 先读取 `slot[0x1C + cursor]`，随后才检查 action
   `wait_remaining`。等待非零时不索引方向表、不移动，但仍更新 action。
3. 等待为零时，两个有符号步长各加一次。这里没有地图角色循环的 action `+0x94` 和
   flags bit 26 两级翻倍。方向先写入 action variant。
4. 坐标未同时对齐 16 像素时，直接进入 action 更新；格指针、路径游标和表面占用保持。
5. 对齐时调用 `sub_411530(guid, flags & 3, 0, 0)`：从第 0 行开始按 GUID 解链，且
   **不重新插入**。随后依次清旧表面足迹、按方向移动格指针、标记新足迹、执行
   `(cursor + 1) | 0x8000`，最后按角色 flags bit 8 投影新格 flags。
6. `sub_4321E0` 失败只进入原字符串 `Act Err(picPaint:CompanyMove)` 的诊断槽，继续
   下一个队伍槽，不把失败升级为帧失败。

## 3. 状态所有权与接线

- `LegacyWorldRolePostMaterializationState` 在角色转入队伍时建立八个队伍槽和队伍数量。
  普通世界启动时把这份可变帧状态复制进 `LegacyWorldFrameCoordinatorState`，以后每帧
  由 `advance_legacy_world_party_role_actions` 唯一修改其游标。
- 角色数组、空间行链和表面格由世界会话借用；空间解链、表面足迹与格 flags 投影复用
  已验证的共享 owner。
- `run_legacy_world_frame` 在 72 槽地图角色路径之后、`sub_414570` 之前直接调用本
  owner；原 `company_role_actions_004124ef` 外部占位已经删除。

## 4. 验证边界

专用 UT 覆盖数量 `0/1` 跳过、八槽容量边界、空槽、停用游标、等待门前的路径字节读取、
等待时的非法方向不索引、单次有符号步长、未对齐保持、对齐后的空间只移除、表面迁移、
游标门位、格 flags 投影，以及 action 更新失败的非致命语义。coordinator 和真实首图
session 测试证明队伍数量/槽从角色物化状态进入实际普通世界帧。

Linux LLVM `core` 152/152、Windows LLVM `app` 156/156 CTest 通过。当前首图队伍
数量为 1，所以真实资产只覆盖汇编的直接跳过分支；多队员活动路径仍缺原程序动态状态
差分，不能标记 `original_diff_verified`。如需动态证据，只准备 Frida 工具并等待用户
运行，不由开发流程启动原版。
