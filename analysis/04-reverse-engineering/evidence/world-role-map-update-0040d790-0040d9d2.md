# 世界角色移出队伍与 MAPS 更新

状态：有效输入行为为 `assembly_exact`，固定全局数组和损坏状态隔离为
`platform_adapted`；另有 `implemented`、`unit_tested`、`asset_verified`、
`blocked_runtime_oracle`。

唯一行为真值：`swd3.exe.lst` 的 `0x0040D790..0x0040D9D2`。IDA 名称和伪码只用于
导航。本函数由剧情 opcode 66 的 `0x00429B14..0x00429B5D` 调用，但本文只关闭共享的
world-map owner；16 字节指令解析、IP 前进和让出合同转交剧情 VM 工作包。

## 1. 七个参数

调用点把指令 `+2..+14` 的七个 word 全部零扩展后依次传入：

```text
role selector, Path id, Talk id, action id,
base variant, variant delta, flags
```

角色选择先经过 `sub_40C0D0`；`0xFFFE` 使用受控角色索引，其他值按 GUID 查找首个
flags bit 28 清零的运行时角色。

## 2. 运行时角色不存在

`0x0040D98A..0x0040D9B9` 扫描 22 字节、地图号 `0xFFFF` 终止的 MAPS 角色目录。
命中输入 selector 对应的 GUID 后只执行：

```text
source.flags &= 0xFF7F
```

七参数中的另外六项完全不写入。目录中也不存在该 GUID 时只调用诊断槽并返回。现代
owner 在已解码的有界目录中执行同一修改，并将缺失记录作为显式状态暴露。

## 3. 运行时角色存在

原函数固定扫描完整八项物理队伍索引数组，不读取逻辑队伍人数。即使某项位于
`party_count` 之后，只要残留索引相等仍按命中处理；OpenSWD3 明确保留该原始行为。
八项均不匹配时只诊断并返回，不修改角色或 MAPS。

命中槽后，只有对象槽 `u16(+0x02) < 0x7FFF` 且角色 X/Y 至少一轴非整格才执行移动中
协调：

1. `sub_40AE20` 先用旧角色状态清表面占用；
2. cursor 与 `0x7FFF` 后读取 `slot + 0x1C + cursor` 的方向；
3. 方向不是 `0xFF` 时，先按 X 表、再按 Y 表每次相加四像素步长直到低四位归零；
4. 无论方向是否为 `0xFF`，都调用 `sub_411530(guid, flags & 3,
   (world_y >> 4) - 1, 0)`，解除角色旧空间链并按新坐标重插。

加法方向表为：

```text
X = [ 4,  0, -4, -4, -4,  0,  4,  4]
Y = [ 4,  4,  4,  0, -4, -4, -4,  0]
```

`sub_411530` 的返回值不被调用者观察。空间链找不到角色时原程序诊断后仍继续全部后续
副作用；现代 owner 保留后续顺序，同时在结果中暴露失败状态。

## 4. 字段与物理数组副作用

随后严格执行：

1. 用 `sub_40DD40` 把命中的 `0x21C` 队伍对象槽全部写成 `0xFF`；
2. 运行时角色写 action/base/variant、Talk、Path、Path cursor 零和完整 32 位 flags；
3. `sub_40D460` 对 MAPS 只写 Talk、Path、Path cursor 零，并执行
   `flags = (flags & 0xFF7F) | 0`；动作三字段仍传 `0xFFFF`，所以不跟随运行时角色；
4. `sub_40AEC0` 用新 flags 重标当前表面占用；
5. 队伍索引从命中槽开始固定搬移 `0x1C - index*4` 字节，队伍对象固定搬移
   `0xEC4 - index*0x21C` 字节；两者都一直复制到物理槽七，尾槽保留重复值；
6. 逻辑队伍人数减一。

MAPS patch 的返回值也不被原调用者观察；现代 owner 即使报告 patch 失败，仍执行其后的
表面重标、两个物理数组搬移和人数递减，防止安全边界改变有效后续时序。

## 5. 平台隔离与验证收敛

现代边界显式隔离受控索引越界、零或大于八的逻辑人数、方向表越界、零步长导致的原版
无限循环、表面越界、损坏空间链和 MAPS 目录缺失。这些状态不属于当前合法资产，故本项
登记为 `platform_adapted`。

实现后进行了三轮反向核对：

1. 第一轮纠正 MAPS 不同步动作三字段，以及两组 memcpy 固定复制到物理尾槽；
2. 第二轮回查 `sub_411530`，纠正末参数零必须解除后重插而非只移除；
3. 第三轮纠正两个 callee 返回值均不阻断后续副作用。

定向 UT 覆盖三条主路径、八槽外逻辑残留命中、`0xFF` 方向、正负步长、零扩展高位参数、
空间链 miss、MAPS patch miss、非法方向和不能收敛坐标。真实 `MAPS.DAT` 回归从实际
角色记录派生 bit 7 向量，确认运行时缺失分支只清该 bit，其他字段逐项不变。

- Linux `core`：181/181 CTest 通过；
- Linux `app`：186/186 CTest 通过；
- Windows LLVM `app`：186/186 CTest 通过；
- 真实 `MAPS.DAT` 回归包含在上述三套测试中；
- 原程序动态差分：`blocked_runtime_oracle`；
- 未启动原版或重写版 EXE。
