# MAPS 角色物化后附加状态

状态：`assembly_exact`（`0x0040CAD3..0x0040CCBC` 在 `sub_40C130` 调用点的全部可达行为）；真实游戏数据静态与集成验证完成；原程序动态差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一行为依据是 `swd3.exe.lst` 的机器码字节与指令。本单元从 MAPS 角色完成基础字段复制、`sub_40D060` 默认值补充和 `sub_411490` 空间链插入后开始，覆盖：

- GUID 1 的两级动作覆盖；
- MAPS 载荷头 `+0x64` 指向的 18 字节角色/地图状态门控表；
- 门控允许时调用 `sub_40D610` 所产生的可达角色、对象槽、MAPS 和队伍改写；
- flags bit 9 向四条固定 16 字节记录的投影。

`sub_40D610` 还有供剧情 opcode 65 等运行时调用者使用的非整格坐标路径。该路径会调用 `sub_40AE20` 清格、按方向表对齐坐标并通过 `sub_411530` 重排空间链。它不在本调用点可达：`0x0040CA72..0x0040CA87` 刚把 MAPS 的 word tile X/Y 左移四位写入角色，至 `0x0040CC28` 前没有任何坐标修改。因此本单元没有把该通用运行时路径伪报为已完成；外部破坏整格不变量时返回显式状态。

## GUID 1 动作覆盖

`0x0040CAD3..0x0040CB48` 只处理 GUID 1，顺序不可交换：

1. 旧逻辑地图为 22、目标地图不是 22 且 `dword_4C8BE0` 非零时，把该全局值写到 `role.action.action_id`；
2. 地图描述 `field_0c` 的 bit 15 非零时，无条件再写 action `0x60`；
3. 目标地图为 6、8 或 200 时扫描 `dword_4A9940` 链；存在 `node+4 == 0x0192` 的节点则把 action 改成 `0x5F`。

所以地图描述高位分支优先于旧地图 22 的覆盖，`0x5F` 又优先于 `0x60`。动作刷新 `sub_40F280` 在全部 MAPS 角色物化后才执行，能看到这里的最终 action。

## `+0x64` 门控目录

MAPS 载荷 `+0x64` 是相对载荷首地址的 u32 偏移。目录每条 18 字节：

```text
+0x00  u16 role GUID
+0x02  u16 map state 0
...
+0x10  u16 map state 7
```

下一条首 word 为 `0xFFFF` 时终止。比较使用地图描述 `field_0c & 0x7FFF`。

原寄存器 `EDX` 从一开始。扫描每条记录时：

```text
GUID 不匹配：decision 不变
GUID 匹配：  decision &= ~1
八个 state 任一匹配：decision |= 2
```

扫描不会在 GUID 或 state 命中后提前结束。因此最终调用 `sub_40D610` 的条件是：目录中没有该 GUID，或至少一条同 GUID 记录包含当前 map state。存在同 GUID 记录但八个 state 全不匹配时，角色保留在地图中，不执行转移。

`0x0040CBE5..0x0040CC17` 还扫描常量字节对表 `0x0049FBE8`，把 GUID 对应 byte 写入局部变量。该局部值在本函数后续没有读取，因而本单元不为它虚构现代状态或副作用。

## `sub_40D610` 的本调用点效果

`sub_40C130` 在角色遍历前把八个队伍角色索引写成 `0xFFFFFFFF`、把八个 `0x21C` 队伍对象槽写成 `0xFF`，并把队伍计数设为一。选中 GUID 物化时写入队伍索引零。

门控允许后，`sub_40D610` 的可达顺序是：

1. 若 `role.path_data_id != 0`，扫描固定 72 个、每槽 `0x21C` 字节的活动对象槽，以槽首 u16 匹配角色索引；
2. 命中时，由于本调用点坐标必然整格，不进入坐标对齐分支；调用 `sub_40D460` 把同 GUID MAPS 源 flags 先保留后 OR `0x0080`，再把命中对象槽完整写成 `0xFF`；
3. 把角色索引追加到当前队伍计数位置，并把对应队伍对象槽完整写成 `0xFF`；
4. 把 `role.talk_script_id` 清零，清 flags bit 14，设置 flags bit 7，递增队伍计数；
5. 返回调用者后设置 flags bit 15；目标地图为 22 或描述 `field_0c` bit 15 非零时立即清除此位。

原数组均没有容量检查。现代边界保留八个队伍槽和 72 个活动对象槽的物理数量，但在越界前返回 `party_capacity_exceeded`。Path 角色需要真实活动对象槽才能判定是否命中；缺少该状态时返回 `active_object_slots_required`，不静默当作 72 个空槽。

## flags bit 9 固定记录

`0x0040CC57..0x0040CCBC` 独立检查最终 role flags 的 `0x0200`。写指针从 `0x004CACE4` 开始，以 `0x004CAD24` 为上界，物理上只有四条 16 字节记录。每次写入：

```text
+0x00 low16(role.world_x)
+0x02 low16(role.world_y)
+0x04 0
+0x06 0
+0x08 role.guid
+0x0A 0
+0x0C..+0x0F 保持原字节
```

第五条及以后只调用诊断空桩并继续角色循环；写指针不再推进。现代记录保持精确 16 字节布局，只修改前十二字节，并单独累计溢出次数。

## 实现与验证合同

独立 owner `legacy_world_role_post_materialization` 在空间链插入后、ACT updater 前执行。固定 UT 覆盖：

- 旧地图 22、描述 bit 15 和故事状态 `0x0192` 的覆盖优先级；
- 无 GUID、同 GUID 无 state、同 GUID 有 state 三种门控结果；
- `sub_40D610` 的 Talk/flags/队伍追加、目标地图 22 的 bit 15 清除；
- 72 槽缺失、对象槽命中后完整清空、MAPS flags 回写；
- 四条 bit 9 记录、第五条溢出及 `+0x0C..+0x0F` 保留；
- `+0x64` 字段、目录偏移、截断记录、缺少终止符和八槽容量边界。

当前游戏数据应用初始记录后，地图 81 物化 12 条 MAPS 角色；flags bit 7 和 bit 9 数量均为零。真实集成固定：队伍计数为一、队伍索引零等于 GUID 1 的运行时角色索引、门控扫描数为零、附加记录数为零、GUID 1 action 仍为一。真实 `+0x64` 目录另以一个不存在的合成 GUID 验证完整扫描至 `0xFFFF`。

## 验证结果

- Linux LLVM `core`：146/146 CTest 通过；
- Windows LLVM `app`：150/150 CTest 通过；
- 未启动 OpenSWD3，也未启动原程序。
