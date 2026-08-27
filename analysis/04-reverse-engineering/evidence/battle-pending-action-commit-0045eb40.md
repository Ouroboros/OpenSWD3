# 战斗待执行动作提交 `0x0045EB40`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 范围与调用图

权威LST完整主体为`0x0045EB40..0x0045EC5A`，从`proc`到`endp`共133行、94条实际指令、7个call、7个跳转、7个局部标签，没有外部`FUNCTION CHUNK`。

函数无参数。唯一caller是已关闭逐帧画面协调器`0x00453200`；调用时点固定在角色帧顺序遍历和已关闭双方完成数协调之后、效果总协调之前。

七个静态call由四类callee组成：

- 两处`0x0047CF20`按角色对象执行待执行动作前置呈现；
- 两处已关闭`0x0045A980`查询角色ready；
- 两处`0x0047E5C0`提交角色对象的待执行动作；
- 一处`0x0045EFB0`从18条动作记录中移除已提交角色。

角色ready已直接组合；另外三类callee仍属后续工作包，保留传递对象token、actor code/index/group及完整EAX/ECX/EDX的语义窄端口。

## 2. 入口数量与固定顺序指针

入口只各读取一次live组B数量和组A数量，按低32位相加。结果按signed `<= 0`时立即返回：EAX为回绕总数，ECX为入口组A数量，EDX保持caller快照；不访问角色顺序也不调用callee。

signed正数时，以该入口总数建立固定循环次数，从18槽物理角色顺序首项开始每轮前移一个dword。callee可修改数量，但本次上界不重读；不增加现代循环上限。第19次真实顺序读取才typed-stop，保留前18轮全部副作用。

每轮首次顺序值以signed `< 8`选择组B，否则选择组A。该分组只决定本轮控制流，后续callee改写同一顺序槽不会重新选择组；但对象索引、ready索引、标记索引、提交对象及发布索引都在各自原指令点重新读取live顺序值。

## 3. 对象token与陈旧寄存器

两组对象地址都只作u32 token运算，不转宿主指针，也不作10/8槽预验：

```text
group B token = 0x00525508 + actor_code * 0x2B28
group A token = 0x005029D0 + (actor_code - 8) * 0x2F34
```

首个前置callee入口寄存器为：

- 组B：EAX=`code*0x565`，ECX=对象token，EDX=`code*0x159`；
- 组A：EAX=`(code-8)*0x3EF`，ECX=对象token，EDX=`(code-8)*0xBCD`。

callee后重新读取顺序值并直接组合角色ready。组A把前置callee返回EDX作为角色ready的caller快照；组B按已关闭callee自身规则重建EDX。ready只接受精确返回1，其他完整EAX归一为0，同时保留其内部callee的ECX/EDX供本caller后续陈旧路径使用。

第二个提交callee再次读取live顺序。组A入口EAX/EDX仍为`0x3EF/0xBCD`倍数；组B入口EAX为`0x159`倍数，EDX在ready成功时取ready后再次读取的live顺序值，ready不成功时保留ready内部callee的EDX。提交结果也只接受完整EAX精确等于1。

## 4. ready标记、发布与记录移除

ready精确为1时，先再次读取live顺序，再把唯一动作激活latch写1，最后把共享ready标记槽写`0xFFFFFFFF`。两组使用同一物理18槽：组B索引为code，组A索引为`code-8`。索引越界只在该store停止，且保留latch与前面全部callee副作用。

提交callee返回不等于1时直接进入下一顺序槽，不发布角色也不移除记录。精确返回1时，再次读取live顺序并按本轮初始分组归一索引：

1. 把唯一18槽actor publication对应项写为该归一索引；
2. 以同一归一索引调用待关闭记录移除callee。

publication越界只在原store停止，保留提交callee副作用。记录移除返回的完整EAX/ECX/EDX成为本轮及函数正常尾返回；若最后一轮未提交，则尾返回来自最后一个提交callee。

## 5. 单一typed owner与caller回收

双方数量和18槽角色顺序复用唯一actor metric state；动作激活latch也从组A帧副本回收到同一state。组A最终角色步进成功时清同一latch，全局重置按原写集合清零。

ready标记复用唯一战斗启动reset块；actor publication复用效果与启动共同使用的唯一18槽state。启动初始化把两者全部写`0xFFFFFFFF`，本函数不建立副本。

逐帧caller直接组合已关闭双方完成数协调，并读取其正常尾EDX作为零角色早退快照，随后直接组合typed待执行动作提交。旧第二followup opaque槽保留reserved枚举值但不再调用。子typed-stop保留角色帧与完成数协调副作用，阻断效果总协调、固定帧和全部后续绘制。

## 6. 验证与动态差分

定向测试覆盖signed零/负/回绕总数、两组对象token、首/次callee陈旧寄存器、ready与提交精确1门、ready成功/失败EDX差异、每个callee之间live顺序改写、共享ready/activation/publication owner、记录移除参数、ready与publication原store停点、第19次顺序读取，以及逐帧caller直连和typed-stop传播。

当前缺少原版两组角色对象、三类剩余callee、18槽顺序/ready/publication/动作记录、动态callee改写及寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
