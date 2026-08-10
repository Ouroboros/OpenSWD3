# B7 · 地图、世界、角色、碰撞与寻路

状态：执行中；单模块开始条件已满足

来源：`swd3.exe.lst` 完整汇编。汇编是唯一行为真值；伪码和符号只用于定位。

## 有限范围

完整地址集合以 `../inventory/module-function-ownership.tsv` 中 `module_candidate = world_map`
的过滤结果为准，共 114 个函数，分为三个连续工作组：

| 工作组 | 地址范围 | 数量 | 职责 |
|---|---:|---:|---|
| 控制、碰撞与角色辅助 | `0x00402030..0x00406960` | 37 | 玩家控制、格/角色命中、碰撞、路径辅助 |
| 世界运行与绘制协调 | `0x0040AD10..0x004151F0` | 69 | 地图私有资源、角色状态、世界更新、绘制和切换 |
| 地图装载与缓存 | `0x00425B50..0x00427300` | 8 | 世界清理、LMF/CM 装载、地图对象和交互 |

本模块消费 B2 已验证的 LMF 物理目录、CM 容器与资源数据库，消费 B3 输入/时钟/RNG、
B4 软件 framebuffer 和 B6 动作/TSW 运行时。剧情 VM、特殊模式、战斗数值和存档字段
解释不属于 B7；本模块只按汇编产生相应请求并由 app 在原顺序消费。

## 状态与接口

- 角色数组：`0x004BABA8`，256 条，每条 `0xD8`；索引 `0x004AB378` 指向受控角色。
- 地图会话拥有 LMF 展开数据、格状态、地图对象/事件、角色与四槽 CM 缓存借用关系。
- 角色 `+0x0C` 的旧 32 位单元指针在现代实现中改用受检索引/句柄；坐标换算和读取顺序
  保持不变。
- 输入、剧情、战斗、渲染和资产模块不取得地图会话所有权，只通过窄接口借用或提交请求。

## 当前纵向切片

实施顺序固定为：

1. `[x]` 建立精确 `0xD8` 角色记录前部与 `+0x40` 动作子记录组合。
2. `[x]` 实现 `0x00404FD0/0x004050B0` 角色格占用查询，作为碰撞的可验证底座。
3. `[x]` 用 B2 LMF API 建立一张真实地图会话和格表，再恢复 `0x00404610` 碰撞；
   LMF 子链和地图 22/24/500 已通过；`0x00426840..0x004272B8` 的 CM 命中、miss、
   淘汰、生成和完整读取也已闭环，Linux `core` 110/110、Windows LLVM `app`
   114/114 CTest 通过；`0x00404610` 九分支碰撞、原版异常足迹步长、事件/角色提前
   返回和受检 session grid 适配已经闭环；`0x004261CE..0x00426798` 的事件、两种
   地图角色、三组空间链，以及 `0x0040F2C1..0x0040F31B` 的受检格索引和 flags
   投影也已闭环。地图 22/24/500 的业务角色数为 49/29/1，Linux `core` 112/112、
   Windows LLVM `app` 116/116 CTest 通过。
4. `[~]` 接入 `0x00402F80` 输入/移动、`0x004120B0` 世界更新绘制和软件 framebuffer；
   四方向输入、八方向覆盖顺序、列表重复、移动边界、动作/速度状态和
   `0x004120F9..0x00412197` 坐标更新已实现；`0x0040BB50` 八方向占位、
   `0x00404510` 三带可通行扫描和 `0x004040B0` 单轴绕行/对角裁剪也已闭环。
   `0x00403AD7..0x00403DB6` 的两次碰撞回退、`0xD8` Talk 布局、地图事件副作用、
   角色相向和 `0x00411E20/0x00411F00` 朝向查表也已实现。`0x0040D9E0` 的 14 字节
   区域源、`0x0040E672` 的 16 字节阈值组、`0x0040DA60` 两次 RNG 选择，以及
   `0x00403F43..0x0040406F` 的全部门控和立即战斗切入已经闭环；当前 DVD MAPS 的
   11 个阈值组、115 条区域和全部候选列表通过。Linux `core` 119/119、Windows LLVM
   `app` 123/123 CTest 通过。`0x00412930` 的三条主体路径、清屏、局部 clip、service /
   control 短路、公共尾部和四条底图路径已经闭环；地图 24 的真实
   `LMF → CM → frame composition` RGB565 framebuffer 哈希为
   `0x947C15A53487BF9A`，Linux `core` 123/123、Windows LLVM `app` 127/127 CTest
   通过。`0x00413EA0/0x00413F00` 的 group 0 bit-29 扫描与固定透明绘制、
   `0x00413870/0x00413910` 的 group `2→0→1` 普通角色扫描、残影/主图/颜色叠加/
   覆盖层/粒子/标签，以及 `0x00413CA0` 距离音频已经闭环；普通角色 runtime adapter
   已接入真实 TSW 和软件 framebuffer，两个固定哈希分别为 `0xA6C3E08156F06060` 与
   `0xA4766C928B05DC88`。Linux `core` 128/128、Windows LLVM `app` 132/132 CTest
   通过。当前继续把空间对象结果接入 `0x00412930` 的实际 world runtime；最终呈现仍
   留在 `0x004120B0` 的原位置。

达到第 4 项即形成“真实地图→角色→输入→碰撞→画面”的首个闭环；不等待 114 个函数
全部内部命名后才实现。

## 验证与停止线

- 每个紧耦合小组先列正常、边界、无效状态、整数回绕和原始 BUG 向量，再写实现。
- 地图物理验证优先使用现有全量 LMF 回归，纵向集成固定地图 22、24、500 的状态与
  framebuffer 哈希。
- 需要原程序动态值时准备 Frida spawn 工具并等待用户执行；Codex 不启动原版。
- 地址范围全部有实现/转交/不可达映射且真实地图纵向闭环通过后，B7 才能收口。

当前单元证据见 [`role-spatial-query-00404fd0.md`](../evidence/role-spatial-query-00404fd0.md)、
[`lmf-world-map-session-00425be0.md`](../evidence/lmf-world-map-session-00425be0.md)、
[`cm-cache-runtime-00426840-004272b8.md`](../evidence/cm-cache-runtime-00426840-004272b8.md)、
[`movement-collision-00404610.md`](../evidence/movement-collision-00404610.md) 和
[`lmf-map-business-004261ce-00426798.md`](../evidence/lmf-map-business-004261ce-00426798.md)、
[`world-direction-adjustment-004040b0.md`](../evidence/world-direction-adjustment-004040b0.md)、
[`world-collision-talk-00403ad7.md`](../evidence/world-collision-talk-00403ad7.md)、
[`random-encounter-0040d9e0-0040db39.md`](../evidence/random-encounter-0040d9e0-0040db39.md) 和
[`world-frame-composition-004120b0-00413370.md`](../evidence/world-frame-composition-004120b0-00413370.md)、
[`world-spatial-roles-00413870-00413f00.md`](../evidence/world-spatial-roles-00413870-00413f00.md)。
