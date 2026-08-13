# 世界对话选择热点链 helper 闭环

状态：`platform_adapted`、`assembly_exact`（有效链行为）、`unit_tested`；原程序动态差分
仍为 `blocked_runtime_oracle`。

唯一行为依据是 `swd3.exe.lst`。本轮先不看既有 C++，独立恢复
`0x0040DB40..0x0040DBFF`，再从 LST 到 C++、从 C++ 回到 LST 反复逐分支追溯，直到
没有未解释写入、比较或返回值。

## `sub_40DB40`

`0x0040DB40..0x0040DB54` 从 `[0x004C8BEC]` 开始，每次沿节点 `+0x04` 读取 next，
并把 EAX 加一，直到空指针。因此返回值就是选择热点链节点数；空链返回零。

现代 owner 用保持顺序的 `std::span<LegacyWorldInteractionHotspot>` 表示同一组节点，
`count_legacy_world_choice_hotspots` 返回 span 数量。损坏链的环路和 32 位计数回绕不会被
现代容器复刻，因此本项登记为 `platform_adapted`。

## `sub_40DB60`

`0x0040DB60..0x0040DBB2` 读取鼠标 X/Y `[0x004A9924/0x004A9928]`，并按节点布局：

```text
+0x04 next
+0x08 left  (u16)
+0x0A top   (u16)
+0x0C right (u16)
+0x0E bottom(u16)
```

依次执行无符号严格开区间测试：`x > left && x < right && y > top && y < bottom`。
命中时输出节点并返回零基序号；完全未命中时输出空指针并返回节点总数。现代
`find_legacy_world_choice_hotspot` 保留首个命中、边界排除、命中索引和 miss 返回值，
只把原始裸链表换成受检 span。

## `sub_40DBC0`

`0x0040DBC0..0x0040DBFF` 在每次释放当前节点前先保存 `+0x04` next，并在释放后立即把
`[0x004C8BEC]` 发布为 next。链结束后将 `[0x004C8BE8..0x004C8BF8]` 五个 dword
全部清零。

对所有读写点复核后确认，这五个 dword 是一个五双字哨兵节点：`+0x04` 才是链头，
其余四项没有独立消费者。它们不是对话控制状态，不能为了“对应四项清零”而误清选择
接受、关闭或输入状态。现代 owner 把节点嵌入每条 `LegacyDialogMessage::choices`，没有
裸 sentinel；`clear_legacy_dialog_choice_chain` 清空所有消息的热点 vector，RAII 同时
完成节点释放和五双字 sentinel 消失。`step_world_interaction` 在原点击接受路径调用该
owner；世界切换、总销毁和战斗 owner 的调用点则分别由会话 reset、进程资源销毁和战斗
模块负责，不在 world-map 中伪造跨模块状态。

## 双向追溯与测试

- LST → C++：覆盖空链、逐节点计数、四条严格边界、首命中、完整 miss、释放 next
  顺序和 sentinel 清零的现代所有权对应；
- C++ → LST：两个查询 helper 的每条判断和返回都有上述指令依据；清链只清热点容器，
  没有增加对无关对话状态的写入；
- UT 固定两节点 count、空链、第二节点命中、四条边界排除、miss 返回 terminal count、
  多消息热点释放和无关对话状态保留；
- `coordinate_legacy_world_interaction` 组合 UT 继续固定 UI 点击绝对优先、选择序号写入、
  左键四 dword 清零和当帧早退。

合法链行为已零未决收敛。剩余差异只有受检容器/RAII 与原裸链表故障边界，以及尚未执行
的原程序动态差分。
