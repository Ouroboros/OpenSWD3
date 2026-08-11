# MAPS 角色切换前同步

状态：`assembly_exact`（`0x0040D200..0x0040D552`）；真实游戏数据静态验证完成；原程序动态差分仍为 `blocked_runtime_oracle`。

## 证据边界

唯一行为依据是 `swd3.exe.lst` 的机器码字节与指令。本单元覆盖：

- `sub_40D200`：切换地图或写存档前，把运行时角色状态同步回可变 MAPS 角色目录；
- `sub_40D3C0`：以 GUID 查找 MAPS 角色并写回完整可持久字段；
- `sub_40D460`：以 `0xFFFF` 为保留值，选择性修改一条 MAPS 角色记录。

`sub_40D200` 在 `sub_40C130` 中只由 `load_flags bit 0` 触发。调用发生在目标地图号已经发布、旧地图状态尚未销毁、目标 LMF 尚未载入的位置。另一个调用点在存档序列化之前。因此它不是目标地图坐标初始化器，而是旧运行时状态的持久化同步阶段。

## `sub_40D460` 的选择性修改

函数按 22 字节步长扫描 MAPS 角色目录，以 `record+0x02` 的 GUID 匹配第一个记录。找不到时返回零；找到时返回一。

十一个参数对应以下写入：

```text
guid
action_id       -> record +0x04
base_variant    -> record +0x06
variant_delta   -> record +0x08
tile_x          -> record +0x0A
tile_y          -> record +0x0C
talk_script_id  -> record +0x0E
path_data_id    -> record +0x10，并把 record +0x12 清零
flags_or_mask   -> record +0x14 |= value
flags_and_mask  -> record +0x14 &= value
logical_map_id  -> record +0x00
```

每个参数只比较低 16 位。值为 `0xFFFF` 时保留旧字段。flags 的执行顺序固定为先 AND、后 OR；两者不能交换。只有 `path_data_id` 实际写入时才清 path word index。

## `sub_40D3C0` 的完整写回

函数同样按 GUID 命中第一条 MAPS 角色记录，然后依次写入：

```text
record +0x04 <- low16(role.action.action_id)
record +0x06 <- low16(role.action.base_variant)
record +0x08 <- low16(role.action.variant_delta)
record +0x0A <- low16(role.world_x) >> 4
record +0x0C <- low16(role.world_y) >> 4
record +0x0E <- role.talk_script_id
record +0x10 <- role.path_data_id
record +0x12 <- low16(role.path_word_index)
record +0x14 <- low16(role.flags)
```

坐标转换先以 16 位 `mov` 截断，再执行 16 位逻辑右移。它不等价于先对完整 32 位坐标右移再截断。例如 `world_x=0xFFFFFFF0` 会写出 tile `0x0FFF`，不是 `0xFFFF`。这是原始可见行为，必须保留。

原函数查询全局 bit 2 后才把首 word `0xFFFF` 当作失败终止。当前游戏数据在有界角色目录内工作；现代实现以已验证的解码目录代替越界扫描，GUID 缺失保持不修改并记录计数。

## `sub_40D200` 的角色循环

函数从角色索引一开始，逐条复制完整 `0xD8` 字节角色到栈上；角色零永远不处理。以下任一条件成立时跳过该角色：

```text
role.flags & 0x18000000 != 0
role.guid == 0xFFFF
role_index == controlled_role_index
```

所有修改只作用于栈副本或 MAPS 记录，不反写运行时角色数组。

### flags bit 7 分支

`role.flags` 低字节 bit 7 非零时调用 `sub_40D460`：

```text
action 三字段 <- 当前运行时角色
tile X/Y       <- sub_40D200 的两个调用参数
Talk/Path      <- 当前运行时角色
flags          <- 先 AND 0，再 OR 当前 role.flags 低 16 位
logical map    <- 当前目标地图号
```

该分支随后直接处理下一角色，不执行普通完整写回。

### 普通分支与 PATH type 8

普通角色的 `path_data_id` 为零时直接完整写回。非零时，函数以整文件映射的 PATH.DAT 执行原地址公式：

```text
relative = u32(path_file + 0x200 + path_data_id * 4)
command  = u16(path_file + 0x200 + relative + path_word_index * 2)
```

Path 非零时，角色副本的 32 位 `world_x/world_y` 会先分别与 `0xFFFFFFF0`，然后再读取当前命令。只有命令严格等于八时才继续执行：

1. 扫描固定 72 个、步长 `0x21C` 的对象槽，以槽首 word 匹配角色索引；
2. 命中时用槽 `+0x04/+0x06` 的两个零扩展 word 覆盖角色副本坐标；
3. 坐标严格大于旧地图 `width/height << 4` 时只诊断，不钳制、不撤销；
4. 无论 72 槽是否命中，都对 32 位 `path_word_index` 加一并回绕；
5. 通过 `sub_40D3C0` 完整写回。

PATH 命令不是八时，已完成的坐标对齐仍用于完整写回，但 path word index 不推进。

## 实现与验证合同

OpenSWD3 把 `sub_40D460` 建成可复用的 MAPS 角色 patch API，把其余行为放入独立的切换前同步单元。`LegacyWorldRuntimeSessionRequest` 只有在 `load_flags bit 0` 非零时要求旧世界上下文；缺少上下文仍返回显式状态，不能静默跳过。

固定 UT 覆盖：

- 十一个 patch 参数的 `0xFFFF` 保留规则、Path index 清零、flags 先 AND 后 OR；
- 完整写回的 action、Talk、Path、flags 和“先截断再右移”坐标；
- 三个跳过条件与角色零不处理；
- bit 7 分支的目标地图、目标坐标和 flags 精确赋值；
- PATH type 8 的对齐、72 槽覆盖、未命中仍推进和 32 位回绕；
- 非 type 8 不推进；
- PATH 目录/命令越界和不足 72 槽的受检边界。

当前 MAPS.DAT 有 1,371 条角色记录，其中 136 条初始 Path 非零，初始 path word index 全为零；这些位置在当前 PATH.DAT 中读到的命令均为五。当前源记录没有 flags bit 7。真实数据回归因此固定“全部 136 条不错误进入 type 8 分支”，但不取代合成向量对 type 8 和特殊角色分支的验证。

现代上下文只投影对象槽首 word 与 `+0x04/+0x06` 坐标字段；物理扫描仍固定为 72 槽、
每槽步长 `0x21C`，没有把投影结构误当作原始槽布局。

## 验证结果

- Linux LLVM `core`：144/144 CTest 通过；
- Windows LLVM `app`：148/148 CTest 通过；
- Windows 阶段只编译和运行测试，没有启动 OpenSWD3，也没有启动原程序。
