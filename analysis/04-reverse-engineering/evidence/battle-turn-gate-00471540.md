# 战斗回合角色推进门 `0x00471540`

状态：`platform_adapted`。完整LST、typed实现、两处group-A frame caller回收以及current-coordinate与coordinate-publication两个叶调用均已关闭。

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

倒计时等于十五且参数为一时播放固定sample，再按post-action值选择播放返回ECX或EDX的陈旧高半word拼接sample低word，分别提交正十六或负十六声像，最后清sample word。声像callee返回后的`add esp,8`flags由Y局部地址反推真实栈恢复前后值，不由测试或端口注入；其他路径到达坐标查询的flags分别来自`cmp countdown,15`或`cmp argument,1`。

`0x004716FD`不再向port发送`0x00478600`。caller以`EAX=var_4`地址、`ECX=actor token`、EDX live residue直接组合current-coordinate typed leaf；`arg_0`和`var_4`两个完整栈局部仅被X/Y低word替换。group-A frame父级有startup时读取`startup.party[index]`的canonical记录；独立helper没有显式view时才回退到action-execution owner。六类查询typed-stop保存leaf寄存器、flags与X部分提交，并从`0x00471702`开始阻断比较、坐标调整、publication、frame source、绘制、倒计时和父级后缀。

查询成功后，参数不等于一时保持查询结果且publication flags来自`cmp argument,1`；参数等于一时按post-action值对完整X执行正十六或负十六调整，最终flags来自32-bit ADD或SUB。`0x0047172C`继续在canonical action-execution坐标owner上直接组合typed publication。入口固定承接完整`EAX=X`、`ECX=actor token`、`EDX=Y`、`ESI=actor token`、`EDI=0`。leaf先写X/Y低word，再复制八个source dword；任一写入或复制typed-stop保存精确leaf寄存器、flags与部分提交，并阻断frame token读取、共享source发布、绘制、countdown递减及外层turn/action后缀。

publication完成后才发布帧源token。X使用signed已发布角色坐标减signed水平偏移，Y使用signed已发布角色坐标减完整32位动作Y偏移；宽高取帧记录低word，flags和数据token原样提交。绘制后倒计时按32位回绕递减并返回零。

## 4. caller回收与验证

模式零caller继续控制回合候选累计与概率门；模式一caller继续标记当前角色bit、累计低byte计数并触发完成消息。测试覆盖special-ready早退、两档inclusive阈值、signed倒计时递减、152字节清零、模式一独占latch、动作更新零返回、查帧键陈旧高半word、双次bit0翻转、sample声像陈旧寄存器、current-coordinate正常与六类停止、栈局部高word、EAX/ECX/EDX、真实入口flags、X部分提交、startup party owner、publication无调整与正负十六、canonical绘制坐标、destination dword部分复制和全部后缀抑制。两处production caller不再调用整函数地址，raw `0x00478600`与`0x004785C0`均零调用。

原Workpack 207门禁为定向测试与独立AddressSanitizer通过、Linux core `188/188`、Linux app `194/194`。Workpack 287 REVIEW 3进一步回收本函数唯一坐标publication caller；Workpack 288 REVIEW 3再回收`0x004716FD` current-coordinate caller，使目标工作包累计达到`caller_reclaimed:21/21`，并通过定向`1/1`、Linux core `199/199`、ASan/UBSan `199/199`、Linux app `205/205`与连续十轮core。动态差分因原版角色动作记录、队列callee、帧记录、sample寄存器、异常栈与坐标页、寄存器/flags及软件绘制联合捕获后端缺失而登记为`blocked_runtime_oracle`。
