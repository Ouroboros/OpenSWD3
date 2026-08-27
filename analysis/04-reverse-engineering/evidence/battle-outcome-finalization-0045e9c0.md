# 战斗结果奖励整理 `0x0045E9C0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 范围与调用图

权威LST完整主体为`0x0045E9C0..0x0045EA26`，从`proc`到`endp`共53行、35条实际指令、2个call、4个跳转、4个局部标签，没有外部`FUNCTION CHUNK`。

两个call都指向已关闭玩家道具双数量步进`0x0045D180`。直接caller只有战斗结果判定`0x0045E580`两处：组A路径在暂停音频后调用，组B路径在全帧暗化精确返回1后调用。

## 2. 两项玩家奖励与陈旧EAX高word

函数入口只保存ESI，不初始化EAX。ESI从固定奖励区首地址开始，以2字节递增，到首地址加4为止，因此精确处理两个u16奖励ID。

每槽执行`mov ax,[esi]`：只替换EAX低word，完整高word保留。AX为0时跳过callee，当前完整EAX继续成为下一槽的陈旧高word；AX非零时以参数`(完整EAX, 0)`调用已关闭玩家道具双数量步进，callee完整返回token又成为下一槽高word来源。该高word会继续传入道具定义初始化callee，不能在typed边界截成单纯u16。

第一callee可改写第二奖励槽，因此第二次在实际循环读取点重新观察live奖励ID。玩家道具head、节点、两项u16数量、分配发布顺序与selector零路径直接复用唯一世界道具链状态。

## 3. 组B固定奖励动态循环

两项奖励处理后读取live组B数量到EAX，把ESI清0，并按signed `EAX <= 0`决定是否跳过循环。正值路径每轮固定调用玩家道具步进`(0x0300, 0)`，不使用角色索引作为item参数。

callee返回后按原顺序：

1. 重新读取live组B数量到EAX；
2. ESI按u32加1；
3. ESI与live EAX按signed `<`比较。

因此首个固定奖励的分配或初始化callee可放大或缩小后续迭代数。实现不缓存入口数量，不增加现代循环上限，也不把signed比较改成无符号比较。

## 4. 三项尾store与完整返回

玩家奖励和组B循环全部完成后，严格按下列顺序写入：

1. 清结果完成dword的高word；
2. 清组B动态数量完整dword；
3. 以一个完整dword语义清两项玩家奖励ID。

这些立即数store不改EAX。函数返回清零前最后一次读取的完整组B数量；零或signed负入口也按原位形返回，再把共享数量清零。

任何玩家道具callee typed-stop都发生在三项尾store之前：保留已完成奖励、玩家道具链副作用、奖励ID、完成word和组B数量，并阻断所有尾store。调用方继续按同样顺序停止，不执行frame mode更新、另一侧判定或后续逐帧阶段。

## 5. 单一typed owner与caller回收

两项奖励ID和结果完成双word由唯一`LegacyBattleOutcomeFinalizationStatePort`持有。全局重置只清原写集合覆盖的结果完成低/高word，不清玩家奖励ID；本函数只清结果完成高word和两项奖励ID。

组B数量复用`LegacyBattleActorMetricStatePort`唯一存储。战斗启动旧`value_53c4b0`重复副本已删除，启动初始化和全局重置都直接清同一metric数量。固定奖励继续复用唯一玩家道具链，不建立战斗私有inventory。

结果判定的两个`resolve_outcome`窄端口已删除并保留为reserved枚举数值槽：

- 组A路径把暂停音频callee的完整EAX作为本函数入口陈旧值；本函数停止时不发布frame active 2，也不进入组B侧；
- 组B路径把暗化返回1作为入口EAX；本函数停止时不清frame active；
- 正常组A路径会先把组B数量清0，随后结果判定对组B packed进度、数量和暗化门的动态重读会观察该零值，同帧仍可进入组B暗化。

## 6. 验证与动态差分

定向测试覆盖：零奖励/零数量、两项奖励、入口与前一callee陈旧EAX高word、第一callee后第二奖励live重读、固定`0x0300`奖励、signed负数量、callee放大动态数量、三项尾store顺序、玩家奖励/组B奖励分配停止、未知玩家链节点停止、启动数量owner回收、全局重置完成word别名、组A/组B两处caller和逐帧阻断传播。

当前缺少原版两项奖励轨迹、组B动态数量、玩家道具链、分配器/定义初始化副作用、音频返回寄存器、双侧结果状态和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
