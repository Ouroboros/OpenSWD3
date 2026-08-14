# 世界物品链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效根表域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F410..0x0040F4F8`。该函数按固定顺序销毁
玩家库存链、四条队伍物品哨兵链和 64 条角色物品哨兵链；每个节点都先释放 `+0xAC`
说明字符串，再释放节点本体。

## 1. ABI、节点与调用者

物理 ABI 无参数。唯一直接调用点 `sub_4251B0` 的 `0x004252AD` 在调用后立即执行
`sub_433010`，不读取 EAX，因此 IDA 标注的 `_DWORD **sub_40F410()` 不构成返回合同。
这是进程级总关闭路径，不是地图切换路径。

既有 `ItemNode` 证据固定节点为 `0xB0` 字节：

```text
+0x00  next pointer
+0x04  u16 item id
+0x06  u16 shop selected count
+0x08  u16 quantity A
+0x0A  u16 quantity B
+0x0C  0xA0 bytes of inline definition data
+0xAC  owned description pointer
```

现代 `LegacyWorldItemNode` 保留四个 16 位业务字段和 `0xA0` 字节定义快照，以
`std::vector<u8>` 接管说明字符串，以 `std::list` 接管 `next`。列表必须逐次
`pop_front()`；直接依赖容器整体析构不能证明原版的头到尾释放顺序。

## 2. 三段销毁控制流

### 玩家库存：`0x0040F411..0x0040F440`

`dword_4A9940` 是可为空的普通链首，不含哨兵。空首直接跳到下一段；非空时每轮先把
全局首写为 `node->next`，然后依次 `free(node->description)`、`free(node)`，再重新读取
全局首。现代实现对应 `player_inventory.front()` 的说明释放和 `pop_front()`。

### 四条必需队伍链：`0x0040F442..0x0040F498`

EDI 从 `0x004A9490` 每次加 4，直到 `0x004A94A0`，精确覆盖四个根。函数直接执行
`eax = *slot; esi = *eax`，没有空根检查；因此原版有效调用域要求四个哨兵全部存在。
每条链先从 `sentinel->next` 逐个释放普通节点，再释放哨兵的说明和哨兵本体，最后把根槽
写零。

`LegacyWorldItemListState` 在建立后按 `sub_44D5D0` 的已证实默认值创建四个哨兵：
`item_id = 0xFFDC`、`quantity_a = 1`，定义名称开头为 CP950 `B5 4C 00`（“無”）。若
现代状态缺少任一必需根，helper 在任何释放前返回其索引；这是对原版空指针崩溃域的
事务式平台隔离，不改变正常域。

### 64 条可空角色链：`0x0040F49A..0x0040F4F4`

EDI 从 `0x004C8AD0` 每次加 4，直到 `0x004C8BD0`，即 `0x100 / 4 = 64` 个根。与前段
不同，每个根先判空；空根只推进槽位，非空根按相同顺序销毁普通节点、哨兵说明、哨兵，
再把槽写零。初始化函数 `sub_40E0B0` 的 `0x0040E970..0x0040E9D0` 也遍历同一边界，
逐槽销毁旧值并调用 `sub_44D5D0` 重建，独立确认 64 项边界及哨兵语义。

三段的先后次序不可交换：玩家库存必须先于四条队伍链，四条队伍链必须先于 64 条角色
链。所有链内均先改写头链接，再释放当前节点；现代有所有权容器没有可观察的悬空链接，
但仍按头到尾顺序释放载荷和节点。

## 3. 平台适配与调用接线

- 裸 `next`、分配地址和根指针改由 `std::list`、`std::optional` 与 RAII 表达；不复制
  循环链、悬空指针、重复释放等损坏域。
- 每次说明释放都使用空 vector 交换，确保容量真正归还；只调用 `clear()` 不等价于
  原版 `free`。即使说明为空，原版仍调用 `free(NULL)`，结果中的
  `description_release_calls` 因而对每个普通节点和哨兵都计数。
- 四个必需根的损坏状态在写入前拒绝；64 个角色根继续保留原版逐槽可空语义。
- SDL 总关闭端口原有 `release_0040f410` 槽现调用该 helper；正常关闭不再依赖最终 C++
  析构偶然完成未登记的物品根清理。

## 4. 双向收敛与测试

- LST→C++ 已覆盖玩家空/非空分支、四次无条件哨兵解引用、64 次逐槽判空、三段范围与
  次序、每节点说明先于本体、哨兵说明先于本体、根槽最终置空及无返回合同；
- C++→LST 已反查所有正常域副作用；RAII、受检必需根和释放统计是明确平台隔离；
- UT 固定构造后的 `0xFFDC/quantity_a=1/「無」` 哨兵，覆盖玩家两节点、四条队伍链、首尾角色
  槽、角色空槽、空说明也执行释放调用，以及缺少第三个必需根时事务式无修改；
- 最后一轮正向与反向逐块追溯没有剩余条件方向、范围、数据宽度、释放次序或根写回差异；
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  通过；两端应用均成功链接，Windows EXE 未启动。

核心实现为
`legacy_world_item_lifecycle.cpp::release_legacy_world_item_lists`，SDL 调用接线位于
`main.cpp::SmokeShutdownPorts::perform_shutdown_operation`。
