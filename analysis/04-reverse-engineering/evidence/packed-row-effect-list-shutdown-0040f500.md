# Packed-row 效果链关闭生命周期闭环

状态：`platform_adapted`、`assembly_exact`（原版有效链域）、`unit_tested`；原程序动态
差分仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst` 的 `0x0040F500..0x0040F539`。该函数从
`dword_4BAB9C` 的链首开始，逐节点释放两个内部行数组和节点本体，直到全局链首为空。

## 1. ABI、调用点与节点布局

物理 ABI 无参数。五个直接调用点为：

- `sub_40A570` 的 `0x0040A669`；
- `sub_40C130` 的 `0x0040C59E`；
- `sub_411D00` 的 `0x00411D01`；
- `sub_4251B0` 的 `0x00425206`；
- 剧情 VM opcode 88 的 `0x0042A727`。

五处调用后都直接执行下一条独立指令或调用，不读取 EAX，因此 IDA 伪码中的 `int`
返回值不是调用合同。opcode 88 随后只再调用 `sub_40F570`、写战斗请求并让出。

opcode 83 的 `0x0042A3D5` 以 `0x18` 字节建立节点；`sub_414E50` 的读取和
`sub_40F500` 的释放共同固定布局：

```text
+0x00  i16 base x
+0x02  i16 base y
+0x04  i16 row limit
+0x06  i16 row count
+0x08  u16 mode（高字节为效果阶段，低字节为脚本 id）
+0x0A  i16 color index
+0x0C  owned i16 row-offset array
+0x10  owned i16 row-length array
+0x14  next pointer
```

现代 `LegacyPackedRowEffect` 保留六个 16 位字段，以两个 `std::vector<i16>` 接管
`+0x0C/+0x10`，以 `std::list` 接管 `+0x14`。这是所有权表示替换，不改变正常域的
业务字段或链顺序。

## 2. 精确销毁控制流

入口先读取全局首；空首在 `0x0040F509` 直接跳到返回。非空时每轮严格执行：

1. 读取当前节点 `+0x14`，先写回 `dword_4BAB9C`；
2. 调用 `sub_4885A0(node->row_offsets)`；
3. 调用 `sub_4885A0(node->row_lengths)`；
4. 调用 `sub_4885A0(node)`；
5. 重新读取全局首，非空则继续。

因此数组释放顺序不可交换，节点也不能先于任一数组释放。原版对空数组指针仍调用
`free(NULL)`；现代 helper 对每个节点都记录两次释放调用，以空 vector 交换确保非零
容量真正归还。每轮先将头节点 `splice` 到临时独占链，使业务链首在任何释放前推进；
再释放两个数组并从临时链 `pop_front()`，对应原版的可观察所有权顺序。

## 3. 调用接线与平台边界

- 剧情 VM opcode 88 不再直接依赖容器 `clear()`，而是调用同一显式 helper；战斗号
  写回、四字节推进和跨帧让出顺序均未改变。
- 世界长期 owner 重建时复用 helper，避免 vector `clear()` 保留容量而偏离原始释放。
- SDL 的 `release_0040f500` 关闭槽绑定到 `LegacyWorldFrameEffectState::packed_rows`。
  owner 尚未建立时空绑定等价于原版空全局根；建立后显示重建与进程关闭均清理实际链。
- `sub_40A570` 对应的主过渡端口把 `release_0040f500` 转发到同一 SDL owner；其余尚未
  闭环的相邻释放槽仍保持独立，不借本函数扩大实现范围。
- 裸链损坏、悬空指针和重复所有权不复制到 C++；`std::list/vector` 只隔离这些原版无效
  内存域。

## 4. 双向收敛与测试

- LST→C++ 已覆盖入口空分支、头节点写回、`+0x0C` 先于 `+0x10`、两数组先于节点、
  逐头循环和最终空根；
- C++→LST 已反查 helper 的每个正常域副作用；释放统计和 RAII 是明确平台适配，不是
  新增游戏逻辑；
- UT 覆盖空链、三节点、两个数组同时存在、仅第一数组存在以及空数组也保留释放调用；
- opcode 88 的既有 UT 继续固定只清 packed-row 与角色头像两条链、负战斗号符号扩展
  和立即让出；
- 首轮 C++→LST 反查发现直接 `pop_front()` 会把链首推进延后到两个数组释放之后，已
  改为先 `splice` 摘头；调用者反查又发现主过渡端口仍为空，已接入真实 owner。修正后
  再次正向与反向逐块追溯没有剩余条件方向、字段偏移、释放次序、循环边界或调用者
  返回值差异；
- Linux `core` 184/184、Linux `app` 189/189、Windows LLVM `app` 189/189 CTest
  通过；两端应用均成功链接，Windows EXE 未启动。

核心实现为
`legacy_action_renderers.cpp::release_legacy_packed_row_effects`；剧情调用接线位于
`legacy_world_story_vm.cpp`，SDL 关闭接线位于 `main.cpp::SmokeShutdownPorts`。
