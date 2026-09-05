# 战斗回合角色推进门 `0x00471540`

历史状态：`platform_adapted`。工作包282正在修正记录重叠与寄存器差异；旧完整门不代表本轮已放行，见第5节。

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

倒计时等于十五且参数为一时，先把turn记录`+0x58`（actor `+0x04C0`）WORD写为`0x2F`，再播放固定sample。播放后`0x004716BF`重新读取完整post-action值到EAX：精确等于一时，`0x004716CC`读取sample WORD并只覆盖CX，声像为负十六；否则`0x004716D8`只覆盖DX，声像为正十六。两路保留播放返回的对应高WORD及另一完整寄存器；声像调用后才清sample WORD。坐标查询完成后，模式一按post-action值对X做正十六或负十六偏移，再发布坐标。

绘制前发布帧源token，X使用signed角色坐标减signed水平偏移，Y使用signed角色坐标减完整32位动作Y偏移；宽高取帧记录低word，flags和数据token原样提交。绘制后倒计时按32位回绕递减并返回零。

## 4. caller回收与验证

模式零caller继续控制回合候选累计与概率门；模式一caller继续标记当前角色bit、累计低byte计数并触发完成消息。测试覆盖special-ready早退、两档inclusive阈值、signed倒计时递减、152字节清零、模式一独占latch、动作更新零返回、查帧键陈旧高半word、双次bit0翻转、sample声像陈旧寄存器、坐标正负偏移、绘制参数、实际帧解引用typed-stop，以及两处production caller不再调用整函数地址。

历史工作包207的定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`207/422 = 198 platform_adapted + 9 assembly_exact + 215 pending_audit`，SHA256为`b3fa2ddef9b48fff1971a1aa58e912425f6f701c491dd14d4d70b520473c216c`。动态差分因原版角色动作记录、队列callee、帧记录、sample寄存器、坐标与软件绘制联合捕获后端缺失而登记为`blocked_runtime_oracle`。

## 5. 工作包282修正中

删除独立`turn_sample_word`，敌我演出及召唤清理统一访问`turn_action_record.field_58`。组A此前错误地在post-action为一时取EDX、否则取ECX，且未把WORD写入对应调用寄存器，也未为声像调用重载EAX；现按上述LST修正。

新增测试覆盖完整mode `0/1/0x00010001`、播放callee改写sample WORD与mode、两个陈旧高WORD、声像调用EAX/ECX/EDX、声像后清理及相邻`field_5a`不变。组B相反声像方向及召唤帧故障前的WORD清理亦纳入定向回归。

`proc_3b78`的core24与ASan15定向`battle.legacy_battle_setup`均`1/1`通过（2.59/4.27秒），无匹配编译/sanitizer诊断，`git diff --check`通过；尚未执行本轮全量或独立审查放行。

### 保存ECX的栈槽与WORD输出

入口`push ecx`保存`var_4`。无坐标查询的所有正常出口现恢复入口ECX，包括队列返回、记录清理和动作更新返回零；不能保留callee ECX或以`rep stosd`耗尽值替代最终`pop ecx`。

`0x004716F1/0x004716F5`分别取得Y的`var_4`地址与X的`arg_0`地址。callee `0x0047860B/0x00478619`的`66`前缀明确只写WORD，故查询后：

```text
X stack DWORD = (argument & 0xFFFF0000) | X WORD
Y stack DWORD = (entry ECX & 0xFFFF0000) | Y WORD
```

原参数精确等于一时才对X执行完整DWORD的正/负十六调整；保留越过低WORD的进位/借位。发布callee收到这两个完整栈DWORD，入口EAX为X、EDX为Y、ECX为actor。正常绘制返回后，`0x0047179A pop ecx`恢复Y栈DWORD；frame/shared typed-stop保留已到达寄存器，不执行这个pop。

请求新增原始X/Y栈地址捕获，结果中两个stack word只观察局部字节，不增加持续所有者。零地址表示未捕获，不是原始地址证据。`0x00478600`仍按未审窄边界隔离，本轮没有提前关闭其inventory。

同时修正镜像宽度读取时EDX低WORD、两处frame stop与source发布stop的寄存器，以及blitter入口EAX=render Y、ECX=render X、EDX=signed target X offset。

### 两处组A帧调用

`0x00457398`的DWORD读取跨越turn WORD与`+0x0053BF1E`相邻WORD，模式零入口EAX现从turn值与目标选择runtime的相邻字段组合，不补零；EDX来自真实末次terminal查询回复。缺少该相邻所有者时，在pending门写入前报告`turn_control_typed_stop`。

模式一在`0x0045759B/0x004575A0`分别得到EAX=`turn & 0x7FFF`、EDX=`1 << actor_index`。两处都以actor token传入ECX，各自传递独立栈地址捕获。已覆盖正常返回与子typed-stop传播，不执行故障后结算后缀。

### 本轮验证范围

测试覆盖三个完整参数、镜像两侧、低WORD边界和DWORD回绕、正常返回/早期frame故障/后期frame故障/shared故障、无查询正常出口、相邻全局字段缺失及两个root调用。最终向量使thiscall入口ECX与actor token一致。

core25/ASan16定向通过后，调整向量一致性并重跑core26/ASan17，均`1/1`通过（2.92/4.76秒），无匹配编译/sanitizer诊断，`git diff --check`通过。此为栈槽及调用寄存器修正证据，不是工作包282全量放行；`+0x2B08`等跨状态所有权和原始动态捕获仍待收敛。
