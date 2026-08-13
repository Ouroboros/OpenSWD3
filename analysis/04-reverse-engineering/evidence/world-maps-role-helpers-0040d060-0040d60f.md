# 世界 MAPS 角色 helper 闭环

状态：`assembly_exact`（`0x0040D0C0/0x0040D160`）、`platform_adapted`
（`0x0040D060/0x0040D200/0x0040D3C0/0x0040D460/0x0040D560`）。

## 证据和复核方法

唯一行为依据是 `swd3.exe.lst`。本轮没有继承既有模块文档的完成状态，而是先从 LST
独立恢复每个入口的字段宽度、比较条件、循环终点和返回值，再对 C++ 做双向逐基本块
追溯。第一轮发现两项旧实现缺口：`sub_40D060` 对 `role+0x2C` 只写低 word，而旧 C++
覆盖了整个 dword；`sub_40D560` 没有独立共享 owner。修正后重新从两个方向核对，未留
未解释基本块。

## 独立函数合同

| 地址 | 精确合同 | 现代 owner |
|---:|---|---|
| `0x0040D060` | 在六字节、GUID 零终止的默认值目录中查找；命中后只写 `role+0x2C` 低 word，并把第二个 word 复制到 `role+0x30` 两半；返回 1，否则返回 0 | `apply_legacy_maps_role_defaults` |
| `0x0040D0C0` | 以受控角色为中心重算全局视口；X 前导量为 `0x130`，Y 为 `0xF0` | `recenter_legacy_world_camera` |
| `0x0040D160` | 根据显式 X/Y 写入调用者矩形；X 前导量为 `0x140`，Y 为 `0xF0`，返回输出地址 | `calculate_legacy_world_camera_rect` |
| `0x0040D200` | 从索引一开始复制角色，执行三项跳过门、bit 7 source patch、PATH type 8 的 72 槽坐标覆盖，并把普通角色完整写回 MAPS | `preload_legacy_world_roles_before_load` |
| `0x0040D3C0` | 按 GUID 写回动作三字段、先截断再右移的坐标、Talk、Path、Path 游标和 flags | `synchronize_legacy_maps_role_source_record` |
| `0x0040D460` | `0xFFFF` 逐字段保留；Path ID 被写入时把 Path 游标清零；flags 固定先 AND 后 OR；命中返回 1，缺失返回 0 | `patch_legacy_maps_role_source_record` |
| `0x0040D560` | 按 GUID 把 MAPS 源字段载入现有角色，Path 游标固定清零；返回零扩展逻辑地图号，缺失返回 `0xFFFFFFFF` 且不改目标 | `load_legacy_maps_role_source_record` |

两个相机函数不能合并：同一角色 X 坐标下，`sub_40D0C0` 的 left 比 `sub_40D160`
大 16 像素。两者都先用有符号条件钳制左/上，再以有符号 `jl` 检查右/下是否严格小于
地图像素边界；等于边界会进入尾端钳制。地图小于视口时的无符号减法回绕没有被改写。

## 平台隔离

三个写回入口保留正常目录内全部可见行为，但现代 owner 不复制原程序的越界内存行为：

- `sub_40D060/sub_40D560` 的原始终止符扫描可越过损坏目录；现代 helper 只接受已由
  MAPS decoder 验证并拥有的记录；
- `sub_40D200` 的原全局角色数为零会使索引循环回绕；现代 span 在空表上直接结束；
- `sub_40D3C0` 只有 service bit 2 开启时才把 `0xFFFF` 首 word 当作未命中终点，关闭时
  会继续越过目录；现代有界数据库把未命中作为显式状态；
- `sub_40D460` 的源记录或可变 payload 越界在现代边界返回受检状态，不执行越界写。

这些隔离不改变已解码有效 MAPS 目录中的分支、字段宽度或写入顺序，因此相关函数登记为
`platform_adapted`，不能宣称逐机器故障一致。

## 验证

汇编独立 UT 固定：默认值低 word 保留、两个相机 X 偏移、源物化成功和缺失不修改、
完整写回各字段截断、选择性 patch 的 sentinel 与 mask 顺序，以及 `sub_40D200` 的跳过、
bit 7、PATH type 8、坐标回绕和 72 槽边界。当前真实 MAPS/PATH 回归继续固定 1,371 条
角色源和 136 条初始 PATH 记录。
