# MAPS 世界装载与初始会话

状态：`assembly_exact`（本文覆盖的载入、目录、角色物化后附加状态、动作刷新和相机顺序）、
`asset_verified`（当前游戏数据的新游戏路径）、`original_diff_verified`（GUID 248/249
初始角色绘制状态）；其余原程序动态差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一行为依据是 `swd3.exe.lst` 的完整机器指令。当前资产锁定为：

- `MAPS.DAT` SHA-256：`a6faee71c0cb41c1be94f29152deed12ed19eb3e1842fb0f10e13e00132a20ba`；
- `huge.lmf` SHA-256：`1fde9e3757a914a4235faa491733f06f236aa720ef8402522404636875833fc7`。

本文收敛 `sub_40F160` 建立 MAPS 载荷和 `sub_40C130` 建立地图会话的主链。
`0x0040CAD3..0x0040CCBC` 的角色物化后附加状态已经由
`maps-role-post-materialization-0040cad3-0040ccbc.md` 独立锁定。它调用的 `sub_40D610`
共享角色转移 owner 也已覆盖非整格坐标、表面占用清理和空间链重排；剧情解释器仍未
接线，因此不能用本文宣称任意地图切换或 opcode 65 完整。

## MAPS 物理载入

`0x0040F166..0x0040F1B5` 的顺序是：

1. 释放旧 `dword_4C9A10`；
2. 取得文件大小并减 `0x200`；
3. 按余下大小分配新缓冲；
4. 从文件起点偏移 `0x200` 定位；
5. 把余下字节原样读入 `dword_4C9A10`。

因此运行时指针指向的是去掉物理前缀后的载荷，MAPS 头内偏移全部相对此指针，不能把
`0x200` 再加到任何目录偏移上。现代 `LegacyResourceDatabases::reload_maps_payload()`
拥有同一段可变载荷；文件过短、定位/读取失败和分配失败在所有权边界返回受检状态。

## 载荷头与目录

主链实际使用的头字段为：

| 载荷偏移 | 内容 | 汇编消费者 |
|---:|---|---|
| `+0x04` | 22 字节 MAPS 角色目录 | `sub_40E0B0` 在 `0x0040E73A..0x0040E74C` 发布为 `dword_4ACE40` |
| `+0x0C` | 14 字节地图描述目录 | `0x0040C27B..0x0040C2B3` |
| `+0x10` | 七个 word 的初始装载记录 | `0x0040F1C9..0x0040F26A` |
| `+0x54` | 6 字节角色默认值目录 | `sub_40D060` |
| `+0x64` | 18 字节角色/地图状态门控目录 | `0x0040CB52..0x0040CBE3` |

地图描述目录以首 word `0xFFFF` 终止。每条 14 字节记录的 `+0x00` 是逻辑地图号，
`+0x02` 是传给 `sub_425BE0` 的 LMF 归档地图号；两者不能合并成一个 map id。

角色目录同样以记录首 word `0xFFFF` 终止，每条为 11 个 word：

| 偏移 | 载入角色字段 |
|---:|---|
| `+0x00` | 逻辑地图号 |
| `+0x02` | GUID |
| `+0x04/+0x06/+0x08` | action id、base variant、variant delta |
| `+0x0A/+0x0C` | tile X/Y，物化时左移四位 |
| `+0x0E/+0x10` | Talk id、Path id |
| `+0x12` | 有符号 Path word index |
| `+0x14` | flags |

默认值目录以 GUID 零终止。`sub_40D060` 逐条步进六字节；GUID 命中后把 `+2` 写入
角色 `+0x2C` 的低 word，并把 `+4` 同时复制到角色 `+0x30` 的高、低 word。

## 初始记录与调用参数

`0x0040F1CF..0x0040F1E8` 从头 `+0x10` 定位记录，把记录 `+0x0C` 的 GUID 发布到
`dword_4B7BC4`。当 `sub_40F160` 参数等于一时，`0x0040F242..0x0040F26A` 以逆序
压栈调用 `sub_40C130`：

```text
logical_map_id = word +0x00
tile_x         = word +0x02
tile_y         = word +0x04
action_id      = word +0x06
base_variant   = word +0x08
variant_delta  = word +0x0A
selected_guid  = word +0x0C（通过全局传递）
load_flags     = 0
```

当前游戏数据记录为 `(81, 13, 28, 1, 0, 3, 1)`。逻辑地图 81 的描述记录把它映射到
LMF 归档地图 81，其余五个 word 为 `(16, 4, 8, 0, 10)`。

## `sub_40C130` 的关键顺序

关键顺序不能按模块方便重新排列：

1. `0x0040C27B..0x0040C36F` 查找逻辑地图描述并发布描述字段；
2. `0x0040C6BD` 调用 `sub_425BE0`，先完成 LMF/CM/地图业务状态；
3. `0x0040C8AE..0x0040C914` 按 22 字节记录遍历角色，并跳过此前已经出现的 GUID；
4. GUID `0/10000/10001` 在 `0x0040C914..0x0040C931` 被迁到目标逻辑地图；
5. 选中 GUID 在 `0x0040C936..0x0040C9A7` 改写地图、动作三字段、坐标，清零
   Talk/Path 三字段并把 flags 写成 `0xD100`；
6. 只物化目标逻辑地图的角色；
7. `0x0040CA17` 与 `0x0040CA2C` 使用无符号 `ja`，所以坐标等于宽/高仍被接受；只有
   严格大于才走错误分支，把源记录 X/Y 同时改为 `width-1/height-1`，并跳过该角色；
8. `0x0040CA34..0x0040CABA` 复制角色基础字段、调用 `sub_40D060`；
9. `0x0040CAC7` 调用 `sub_411490` 插入角色空间链；
10. `0x0040CAD3..0x0040CCBC` 在每条 MAPS 角色空间插入后执行 GUID 1 action 覆盖、
    状态门控、队伍转移和最多四条 flags bit 9 记录；
11. 全部角色完成后，`0x0040CD61..0x0040CD78` 从角色一开始逐条调用
    `sub_40F280`；该函数先在 `0x0040F289` 清零 action `mode_flags`，再于
    `0x0040F291` 调用 action updater，因此 `IV` 等命令产生的最终模式位必须保留；
    动作载入失败只记录诊断，不中止循环；
12. `sub_40F280` 随后清除/投影地图 flags，并绑定角色所在地图格；
13. `0x0040CD7A` 最后调用 `sub_40D0C0` 生成选中角色相机。

OpenSWD3 在 `LegacyWorldMapSession` 的 LMF 业务状态完成与最终格绑定之间增加了一个
受控回调槽。`LegacyWorldRuntimeSession` 在该槽内改写 MAPS、追加角色、建立空间链并
在调用 ACT updater 前执行同一 action mode 清零，随后复用只处理地图 flags 与格索引的
统一格绑定器，保持上述第 2、3–10、11 项的逐指令相对顺序。

载入 flags 的 bit 0 会在目标 LMF 载入前调用 `sub_40D200`，把旧运行时角色状态同步回
MAPS。现代入口在请求提供旧世界上下文时执行已恢复的 `0x0040D200..0x0040D552` 单元；
缺少上下文仍返回 `preload_coordinate_stage_required`，同步失败则返回
`preload_role_synchronization_failed`，两种情况都不能静默跳过后报告成功。

## 相机

`sub_40D0C0` 使用选中角色像素坐标：

```text
left = x - 0x130
top  = y - 0x0F0
right  = left + 0x280
bottom = top  + 0x1E0
```

左、上为负时钳到零；右、下达到或超过地图像素边界时，把对应轴移到最后一个
`640×480` 视口。比较是 `jl`，等于边界也进入尾端钳制。

## 当前游戏数据验证

真实 MAPS 载荷为 162,417 字节，解出 345 条地图描述和 1,371 条角色源记录。新游戏
改写前地图 81 有 9 条角色；GUID `1/10000/10001` 迁入后为 12 条。当前十二条记录的
flags bit 7 与 bit 9 均为零，因此已恢复的状态门控和四记录分支在当前首图不触发。

真实集成测试使用 `MAPS.DAT + huge.lmf + 六个 ACT 包` 建立地图 81 会话：12 条 MAPS
角色全部物化，GUID 1 位于 `(13×16, 28×16)`，动作更新失败数为零；队伍计数为一且
索引零指向 GUID 1，门控扫描和附加记录数均为零。全部角色在动作更新之后完成格绑定。

原版 `0x00413910` attach 采集进一步固定了地图 81 的两个静止角色：GUID 248 为
`world=(416,416)`、`action=623/variant 4`、`offset=(27,48)`、`TSW=188/4`、
`mode_flags=1`、`destination=(309,240)`；GUID 249 为 `world=(320,432)`、
`action=623/variant 5`、`offset=(5,46)`、`TSW=188/8`、`mode_flags=0`、
`destination=(235,258)`。真实集成回归同时固定初始化与首帧后的这两个 mode 值，防止把
`0x0040F289` 的清零错误移到 updater 之后。
