# 战斗角色顺序重建 `0x0045B190`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045B190..0x0045B27A`，从proc到endp完整126行、0个call站点、11个`loc_`标签，无外部FUNCTION CHUNK。

共有11个静态caller站点。九个已关闭站点位于战斗启动、画面转场两处、逐帧协调、角色动作分派、对手动作分派两处和最终角色步进两处；它们全部回收。其余两处位于尚未关闭的后续战斗函数，不提前修改。

## 2. total与零路径

入口先读取group A与group B完整u32数量并相加，结果按低32位回绕。总数为零时直接跳过选择，只执行18 dword选择mask清零。

非零路径把初始总数保存为固定外层迭代次数，并把角色顺序表起点保存为输出游标。后续选择不重新计算总数。

## 3. 初始候选扫描

每轮先从索引0扫描完整18槽metric表，候选必须同时满足：

- metric不为零；
- 对应mask不等于1。

metric为零时按短路顺序不读取mask。mask只有精确值1表示已选，其他非零值仍视为未选。

扫描使用signed `< 18`继续条件。若18槽都不满足，会在索引18处执行下一次真实metric读取；typed实现只在该读取点停止，不提前清mask。

找到候选后，函数再次读取该槽metric，保留重复读取顺序；候选索引作为同值比较的稳定优先项。

## 4. group B与group A比较

组B比较从候选后一槽开始，到group B数量为止。每槽先读取mask；mask不等于1时再读取metric并做signed严格小于比较。这里不重复初始扫描的非零门，因此后续零metric可以击败正候选，必须保留此非对称。

组A每轮重新读取group A数量一次，计算`count + 8`的u32边界。只有该结果unsigned大于8才从索引8扫描；每槽同样先读mask，再以signed严格小于比较metric。相等值不替换，保持最先出现的索引。

## 5. 发布与最终mask清零

每轮比较结束后按固定顺序：

1. 把选中索引对应mask写1；
2. 把选中索引写入角色顺序表当前dword；
3. 输出游标加4；
4. 固定剩余次数减1。

正常完成全部选择后，固定把18 dword mask全部清零。顺序表不在本函数入口或尾部清零；它由前一metric重建函数先清零并在此覆盖总数对应前缀。两张表与mask都位于同一共享typed角色metric状态中。

## 6. 返回与typed-stop

正常路径最终执行mask清零的`EAX=0`与`ECX=0`，返回0。总数零时EDX保留caller入口值；非零时EDX为最后一轮读取的group A数量。

访问顺序严格区分：

- 初始扫描先metric，非零后才mask；
- 两组比较先mask，未选后才metric；
- 发布先写mask，再写顺序表。

因此异常数量或预置mask只在首次真实metric读取、mask访问或顺序表store处typed-stop；此前选择、mask写和顺序表前缀保持不回滚，异常路径不执行最终mask清零。

## 7. caller回收与测试

九个已关闭caller均删除`0x0045B190` opaque边界并直接组合统一顺序重建：

- 战斗启动、逐帧协调、角色动作、对手动作和最终角色都紧接前一已关闭metric重建，复用同一端口物理状态；
- 画面转场两处在HUD后、后续surface阶段前组合，并通过转场端口复用战斗共享状态；
- 子函数typed-stop立即阻止caller后续阶段。

定向测试覆盖总数零与u32加法回绕、完整mask清零、signed最小值稳定顺序、组B再组A比较、后续零metric非对称、mask精确值1、索引18的metric读取、超大组B在mask先读处停止、异常路径不清mask、路径相关EDX、前项角色顺序表物理别名及九处caller回归。

当前缺少原版两组数量、18槽metric表、18槽mask、18槽角色顺序表、调用链共享状态和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
