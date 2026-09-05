# 战斗回合角色推进门 `0x00471540`

状态：`platform_adapted`。完整LST、typed实现、两处group-A frame caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00471540..0x004717C8`，proc至endp共314行、191条实际指令、8个call、20个跳转、18个局部标签、5个返回点，没有外部`FUNCTION CHUNK`。8个callee依次是队列完成查询、动作记录更新、帧记录查询、sample播放、sample声像设置、坐标查询、坐标发布与软件绘制边界。

唯一caller为group-A frame `0x00456680`，有两处调用：`0x004573BA`传入模式零，`0x004575C5`传入模式一。两处原整函数地址均已删除并改为typed直连。

## 2. owner与分支顺序

角色物理状态继续复用唯一`LegacyBattleGroupAActionExecutionState`与`LegacyBattleActorProgressState`。动作记录、倒计时、完成latch、动作编号、special mode、角色坐标、显示偏移、渲染flags和帧token均落在既有每角色owner；共享帧源token复用group-A action shared owner，没有建立平行状态。

函数先精确检查`special_ready == 1`，命中时只清完成latch并返回一。否则按参数写入模式零阈值二或模式一阈值六，再查询队列完成状态：精确返回一且signed倒计时高于阈值时只递减并返回零；小于等于阈值时把倒计时重置为十五并返回一。

队列未完成且signed倒计时小于等于阈值时，只清零角色内嵌的152字节动作记录、重置倒计时，并仅在参数精确等于一时置完成latch。模式零保留旧latch，不现代化为统一布尔赋值。

## 3. 动作、音频、坐标与绘制链

继续路径先置完成latch，把角色profile value写入动作编号、固定base variant四十二，并按special mode重建动作记录external mode。动作更新返回零时，保留此前全部副作用并按原函数返回一。

更新成功后以EAX和EDX的陈旧高半word分别拼接动作记录帧键，查询帧记录。渲染flags先翻转bit0；角色post-action值为一时再翻转一次，并以`frame width - draw offset`的16位回绕结果替换水平偏移。帧owner为空时只在原始第一次帧解引用位置typed-stop，保留此前写入与窄callee副作用。

倒计时等于十五且参数为一时播放固定sample，再按post-action值选择播放返回ECX或EDX的陈旧高半word拼接sample低word，分别提交正十六或负十六声像，最后清sample word。坐标查询完成后，模式一按post-action值对X做正十六或负十六偏移，再发布坐标。

绘制前发布帧源token，X使用signed角色坐标减signed水平偏移，Y使用signed角色坐标减完整32位动作Y偏移；宽高取帧记录低word，flags和数据token原样提交。绘制后倒计时按32位回绕递减并返回零。

## 4. caller回收与验证

模式零caller继续控制回合候选累计与概率门；模式一caller继续标记当前角色bit、累计低byte计数并触发完成消息。测试覆盖special-ready早退、两档inclusive阈值、signed倒计时递减、152字节清零、模式一独占latch、动作更新零返回、查帧键陈旧高半word、双次bit0翻转、sample声像陈旧寄存器、坐标正负偏移、绘制参数、实际帧解引用typed-stop，以及两处production caller不再调用整函数地址。

定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`207/422 = 198 platform_adapted + 9 assembly_exact + 215 pending_audit`，SHA256为`b3fa2ddef9b48fff1971a1aa58e912425f6f701c491dd14d4d70b520473c216c`。动态差分因原版角色动作记录、队列callee、帧记录、sample寄存器、坐标与软件绘制联合捕获后端缺失而登记为`blocked_runtime_oracle`。
