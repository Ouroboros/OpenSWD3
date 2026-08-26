# 战斗双对象数值转场 `0x0045D690`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围与调用图

权威函数为`0x0045D690..0x0045D801`，从proc到endp完整191行、141条实际指令、21个静态call、6个分支与5个局部标签，无外部FUNCTION CHUNK。

静态call分布为：对象kind查询1处、动作标识发布5处、数值发布5处、mode发布5处、三参数提交3处，以及mode 2与mode 4双输出查询各1处。

六个直接caller均已关闭：玩家动作分派2处、对手动作分派1处、效果总协调器3处。caller均不按本函数普通返回值分支。

## 2. ABI、入口snapshot与零值早退

两个参数都是32位对象token，分别作为primary与secondary对象。函数入口先把共享primary完整dword保存到ESI，再把两个局部dword清零。对象kind查询和全部后续callee即使改写共享primary，也不改变该入口ESI snapshot。

共享primary为零时不调用任何callee，直接保留入口EAX、ECX、EDX返回。modern接口显式接收三项入口寄存器，不能伪造固定零返回。

对象token不转换为主机指针；真实对象访问只发生在端口callee中，因此本函数自身没有新增数组或指针typed-stop。

## 3. low-word kind分派

primary对象kind查询后只比较AX：

- low word等于1进入单对象负值路径；
- low word等于2进入辅助奖励路径；
- low word等于4进入打包高word路径；
- 其他值立即返回查询callee的完整EAX、ECX、EDX。

因此高word任意的`0xABCD0001/2/4`仍分别进入对应路径；非识别low word不得清共享值。

每次端口callee都可以发布共享primary、secondary或打包奖励高word副作用，typed实现按调用点立即应用。只有本函数原尾部固定store才覆盖这些副作用。

## 4. kind 1单对象负值路径

kind 1严格按顺序对primary对象：

1. 发布动作标识`0x246F`；
2. 对入口ESI执行32位二补数取负并发布为数值；
3. 发布mode 1；
4. 以`(negated, 0, 0)`提交。

共享primary本身不由本函数尾部清零或改写；若前述callee改写它，副作用保持。最终完整EAX、ECX、EDX来自三参数提交callee。

## 5. kind 2辅助奖励路径

先对secondary对象调用双输出查询。两个局部入口均为零；只消费第二输出的低16位并按i16符号扩展为完整EAX，第一输出保持真实写入语义但随后不读。

以32位回绕计算`delta = candidate - entry_snapshot`，再把delta按i32作signed `jg`：

- delta大于0：保持入口ESI，不调用secondary数值发布；
- delta小于等于0：ESI替换为符号扩展candidate，并先对secondary发布该值。

随后无条件：secondary发布动作标识`0x235E`与mode 1；primary发布动作标识`0x2367`、ESI数值与mode 1；primary以`(0, ESI, 0)`提交。

提交返回后才对ESI作32位取负，把低word写共享secondary，随后把共享primary清零。两个store不改变提交callee的完整尾寄存器。

## 6. kind 4打包高word路径

候选查询、i16符号扩展、32位回绕delta和signed选择与kind 2完全相同。后半段差异为：

- primary动作标识改为`0x2366`；
- 三参数提交为`(0, 0, ESI)`；
- 提交后先把共享primary清零，再把取负ESI的低word仅写入打包奖励高word，低word保持入口值；
- 不写共享secondary。

零除、夹值、绝对值、范围保护和现代数值上限均不属于该函数。

## 7. 单一typed物理状态

三项物理全局继续只保留一份typed存储：

- primary完整dword由玩家/对手动作累计值、效果协调器主反馈值和本函数共用`LegacyBattlePairTransitionStatePort`；
- secondary word由单体/群体效果辅助奖励、效果协调器次反馈值和本函数共用同一port；
- 打包奖励高word继续复用既有`LegacyBattleEffectShiftStatePort`，本函数只替换高word。

全局重置原固定写集合只清primary完整dword；secondary与打包奖励高word不在该重置写集合中，必须保留入口值。动作、效果和协调器中的旧副本已删除。

## 8. caller回收与测试

玩家动作分派两处、对手动作分派一处和效果总协调器三处都删除旧callee token并直接组合typed函数。caller继续忽略普通返回寄存器，只累加内部端口调用并保留共享状态副作用；共享primary为零时，typed子函数仍计一次caller组合但内部零调用。

定向测试覆盖入口零值与陈旧寄存器、未知low word和callee共享副作用、kind 1入口snapshot、kind 2正负delta、未写局部输出、kind 2尾store、kind 4参数顺序、仅高word替换、提交尾寄存器，以及六处caller直连和全局重置单一别名。

当前缺少原版对象kind、双输出查询、动作/数值/mode发布、三参数提交、三项共享全局与寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
