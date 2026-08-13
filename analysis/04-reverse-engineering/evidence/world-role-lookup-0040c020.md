# 世界角色 GUID 与 selector 查询：`0x0040C020..0x0040C125`

状态：四个函数已从完整 LST 独立重建并完成双向收敛；`0x0040C020/0x0040C0D0/`
`0x0040C100` 为 `assembly_exact`，`0x0040C060` 的无效索引分支为
`platform_adapted`；原程序动态差分仍为 `blocked_runtime_oracle`

来源：`swd3.exe.lst` 完整汇编。汇编是唯一行为真值。

## 1. GUID 到索引

`sub_40C020` 从索引零开始，以固定 `0xD8` 步长扫描角色表。只有 GUID 的 16 位比较相等
且角色 flags bit 28 为零时才返回当前索引；bit 28 非零的同 GUID 记录继续向后扫描。
没有命中或角色数为零时返回 `0xFFFFFFFF`。

`sub_40C100` 无条件把上述返回值写入输出。值不是 `0xFFFFFFFF` 时返回一，否则返回零；
失败时输出因此是 `0xFFFFFFFF`，不是函数开头的临时零值。

## 2. selector 到索引

`sub_40C0D0` 先把输出写成零。selector 低 16 位等于 `0xFFFE` 时，把当前受控角色索引
写入输出并直接返回一，不检查该索引是否小于角色数。其他 selector 原样交给
`sub_40C100`，所以普通 GUID 查找失败会把输出再次覆盖成 `0xFFFFFFFF`。

调用者若忽略返回值，随后会以 `-1` 构造角色地址；这是原程序的异常行为，不得误写为
“失败时操作角色零”。现代 owner 保留 helper 的输出与返回合同，在真正消费角色数组前
用显式状态隔离宿主越界。

## 3. 索引到 GUID

`sub_40C060` 的有效路径直接返回 `roles[index].guid` 的零扩展值。无效索引路径先从同一
越界记录读取 GUID 格式化诊断、显示 `MessageBoxA`，随后再次读取该越界 GUID 并返回。
该分支依赖角色数组外内存且结果不确定；现代 `legacy_world_role_guid_at` 返回
`invalid_role_index`，不伪造 GUID，也不复现 UI 阻塞和越界读取，因此本函数整体标记为
`platform_adapted`。

## 4. 实现与验证

公共 owner 为 `legacy_world_role_lookup.cpp`，PATH、剧情 VM 和空间音频共用同一实现。
定向 UT 覆盖索引零、首个命中、bit 28 跳过、缺失 GUID 的 `0xFFFFFFFF` 输出、`0xFFFE`
不校验索引、普通 selector 委派、有效 index-to-GUID 及无效索引隔离。收敛过程中纠正了
PATH 文档和测试中“查找失败保留索引零”的旧误解；相关调用点现在只在数组消费边界返回
明确失败。最后一轮从四段汇编到实现、再从每项实现行为回到指令地址均未产生新差异。
