# 世界角色空间链插入闭环

状态：`platform_adapted`、`assembly_exact`（有效角色、组、行和链域）、`unit_tested`；
原程序动态差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x00411490..0x00411527`。该函数把一个角色插入
指定空间组、指定逻辑 Y 行的单向链；链内排序不是普通全序，必须保留汇编的逐分支条件。

## 1. ABI、物理布局与调用域

物理 ABI 为 `sub_411490(role*, group)`。角色记录步长为 `0xD8`，本函数只访问：

| 偏移 | C++ 字段 | 用途 |
|---:|---|---|
| `+0x00` | `spatial_next_link_32` | 同行下一角色 |
| `+0x08` | `world_y` | 行号及链分支比较 |
| `+0x24` | `guid` | 16 位链分支比较 |

第二参数先索引 `dword_4A9A04[group]`，再以 `+0x50 + row * 4` 定位行首；`0x50`
恰好是 20 个四字节前缀行。现代 `LegacyRoleSpatialIndex` 因此保留三个组和 20 行前缀，
裸角色指针及 next 指针改为一基 `u32` 索引，零仍表示空链。

全部九个直接调用点均已逐条反查：

- `sub_40C130:0x0040CAC7`；
- `sub_411530:0x004115D3/0x0041160D`；
- `sub_425BE0:0x004263DA/0x00426770`；
- `sub_427920:0x00429950/0x00429A0E`；
- `sub_448360:0x00448499/0x004484EB`。

九处都从同一角色 `+0x10` 读取 flags，再以 `and 3` 形成 group；没有调用者消费返回
时的 EAX。尽管当前调用域恒为 `role.flags & 3`，C++ 仍显式保留独立 group 参数，不能
把物理 ABI 事实隐含进 helper。现代调用者逐处显式传入该值，布尔返回只承载平台边界
诊断。

## 2. 行号和空链

`world_y` 按有符号 32 位解释。汇编的 `cdq; and edx, 0x0F; add eax, edx; sar eax, 4`
等价于有符号整数除以 16 并向零截断；因此 `-1` 属于逻辑行零，而不是行 `-1`。

行首为空时，汇编依次把行首写为待插角色，并把其 next 写零。现代实现保留相同最终
状态。组、前缀行或索引越界只在现代边界返回失败；原程序会越界解引用，不能把该隔离
误记为游戏逻辑。

## 3. 非空链的精确分支

所有 Y 比较都是无符号 32 位比较，GUID 比较是无符号 16 位比较。

### 单节点

当前首节点的 next 为零时，只要下列任一条件成立，就把待插角色放到首节点之前：

```text
current.world_y > inserted.world_y
or current.guid < inserted.guid
```

否则追加到当前节点之后，并把待插角色 next 写零。

### 多节点首部

首节点已有 next 时，仅当下列两个条件同时成立，才把待插角色放到行首：

```text
head.world_y <= inserted.world_y
and inserted.guid >= head.guid
```

### 多节点遍历

其余情况从 `(current, next)` 开始遍历。只有下列三个条件同时成立才插到两者之间：

```text
current.world_y <= inserted.world_y
and current.guid >= inserted.guid
and next.guid <= inserted.guid
```

条件不成立就令 `current = next`；到链尾后追加并把待插角色 next 写零。这里故意不比较
`next.world_y`，也不把三项条件重写成常规排序器；这是汇编的真实行为。

## 4. 平台隔离与收敛验证

原函数信任组号、行指针和整条裸指针链。现代实现只增加以下宿主安全边界：拒绝角色零
哨兵、越界角色、组、行首或 next 索引；链本身若形成有效索引环，仍像原函数一样不会
自行终止。失败边界不会预先清空待插角色 next，正常域写入次序和最终链完全保持。

收敛检查从两个方向重复执行：

- LST→C++：逐基本块核对行除法、组寻址、空链、单节点两条路径、多节点首插、三条件
  中插、推进与尾插；
- C++→LST：逐个反查每个正常域比较宽度、比较方向、next/head 写入和返回路径；显式
  group、索引/span 及边界失败均只属于平台适配；
- 九个调用点再次核对后，group 来源均收敛为同一角色的 `flags & 3`，无返回值消费者；
- 独立 UT 固定负数向零除法、显式 group、空链、单节点 Y/GUID 首插、单节点追加、
  多节点首插、中插、尾插和现代越界隔离。

最后一轮正向与反向复核没有剩余控制流、字段宽度、比较方向、写入或有效域状态差异。
Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest 全部
通过，两端应用成功链接且未启动任何 EXE。实现位于
`legacy_world_map_business.cpp::insert_legacy_role_spatially`。
