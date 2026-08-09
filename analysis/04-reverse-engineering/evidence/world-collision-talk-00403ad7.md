# 世界碰撞回退与 Talk 构造：0x00403AD7..0x00403DB6

状态：完整 LST 逐基本块实现；`assembly_exact`，原程序动态差分仍为
`blocked_runtime_oracle`

来源：`swd3.exe.lst`，SHA-256
`701732b5481ba34876b62ca97535c9463f65ec3feb2ed745c03772dd4bc3ad8b`。
汇编指令和数据字节是唯一行为真值。

## 1. 两次碰撞查询

`0x00403AD7` 先把角色命中输出初始化为 `0xFFFFFFFF`，事件返回初始化为零：

1. 修正后的 `dx/dy` 非零时调用一次 `0x00404610`。
2. 只有第一次返回值为零时，才用修正前保存的原始 `dx/dy` 再调用一次。
3. 第二次调用会覆盖第一次的事件返回和角色索引。

条件只检查第一次的 `EAX`，不检查角色命中输出。因此第一次即使命中角色，只要
`0x00404610` 按原合同返回零，仍会继续尝试原始方向，第一次角色索引也可能被第二次
覆盖。实现和 UT 明确保留了这个不常规顺序。

查询结束后，若角色索引不是 `0xFFFFFFFF`，代码忽略碰撞返回值，改读该角色
`+0x1E` 的 16 位 Talk id。最终事件值为零才直接进入后续移动状态。

## 2. 地图事件路径

没有命中角色时，非零事件值经 `0x0040DC30` 在地图事件链按节点 `+0x04` 查找。
事件记录的直接消费字段为：

```text
+0x04  碰撞事件键
+0x08  低 16 位 Talk id
+0x0C  高 16 位内部 bit 编号
```

查找后先以 `event+0x0C >> 16` 调用 `0x0040DC50`。返回值精确等于一时，修正后的
两个移动分量清零。这个副作用发生在 Talk 空闲检查之前；已有 Talk 时仍会停下移动。

原版在查找失败时只输出诊断，随后仍执行 `mov eax,[edi+0Ch]`，会解引用空指针。
现代接口以 `missing_map_event` 暴露这条原始异常路径，避免在 C++ 中制造未定义行为；
正常资产路径不改变。

Talk 空闲时写入：

```text
context+0x04 = player.world_x - direction_x[player.direction] * 16
context+0x08 = player.world_y - direction_y[player.direction] * 16
context+0x10 = 0
context+0x14 = 0
context+0x1E = low16(event+0x08)
context+0x20 = 0
context+0x24 = 0xFFFD
```

两张八项 dword 表来自 `.rdata:0x00499380/0x004993A0`：

```text
X = { 4, 0, -4, -4, -4, 0, 4, 4 }
Y = { 4, 4,  4,  0, -4,-4,-4, 0 }
```

原版没有方向下标保护；现代接口把大于七的值作为
`invalid_player_direction`，不猜测表后内存。

## 3. 距离、角度与八方向

角色路径调用 `0x0040E030`，后者用受控角色和目标角色的
`world + action.HW * 8` 中心点依次调用：

- `0x00411E20`：32 位回绕平方和、x87 平方根截断距离、`dy*8192/distance`，再在
  `0x0049F250` 的 91 项整数正弦表中按五度步长选最接近项；相同差值选择后一个角度。
- `0x00411F00`：以 `angle*16/360` 选 16 个扇区，映射表为
  `{3,7,7,0,0,4,4,2,2,6,6,1,1,5,5,3}`。

平方和的高位变成负数时，原 x87 `fsqrt`/`fistp` 产生整数 indefinite；返回低 dword
为零。实现用确定的整数路径复现该结果，不依赖不同平台的 NaN 转整数行为。

## 4. 角色相向与原始刷新不对称

命中角色 flags 含 `0x00000800` 时：

1. 保存目标 action 的 `base_variant/variant_delta` 到两个 one-shot 字段。
2. 把目标 `base_variant` 清零、`variant_delta` 写为朝向、`wait_remaining` 清零。
3. 以目标 action 调用第一次 `0x004321E0`。

随后玩家无条件写：

```text
base_variant = 0
variant_delta = (facing & ~1) + ((facing - 1) & 1)
wait_remaining = 0
```

这里有一条必须保留的不对称：`0x00403CFF` 在写完玩家字段后压栈的仍是早先保存在
`ESI` 中的目标 action 指针。因此第二次 `0x004321E0` 仍刷新目标，不刷新玩家。
目标 flags 不含 `0x800` 时会跳过第一次刷新，但这次第二刷新仍存在。两个刷新返回零
都只进入诊断，不撤销已经写入的状态，也不阻止 Talk 构造。

角色 Talk 最后只复制：

```text
context+0x10 = target.flags
context+0x14 = target.talk_data_offset
context+0x1E = target.talk_script_id
context+0x20 = target.talk_initial_offset
context+0x24 = target.guid
```

碰撞角色路径不写 `context+0x04/+0x08`，其中旧值原样保留；最后清零一次性交互状态
`0x004A99F4`。

## 5. 实现与验证

实现文件：

- `include/openswd3/world_map/legacy_world_facing.hpp`
- `src/world_map/legacy_world_facing.cpp`
- `include/openswd3/world_map/legacy_world_collision_talk.hpp`
- `src/world_map/legacy_world_collision_talk.cpp`

UT 覆盖零/四轴/四对角/非等边朝向、平方和符号回绕、第一次角色命中仍回退、第二次
覆盖、非零事件短路、地图事件 bit 在 Talk 门前生效、八项位置表、精确 `==1`、角色
字段复制、双次目标 action 刷新、无 `0x800` 分支，以及所有现代受检失败状态。
91 项整数正弦表另与原 EXE `0x0049F250` 数据逐项核对零差异。Linux Clang `core`
117/117、Windows LLVM `app` 121/121 CTest 通过。

本单元没有启动原版或 OpenSWD3 EXE。动态差分需要时按项目规则准备 Frida spawn
工具并等待用户执行。
