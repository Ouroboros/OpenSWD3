# 战斗过渡阶段推进 `0x00469620`

状态：`assembly_exact`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围

权威LST主体为`0x00469620..0x00469648`，从proc到endp共24行、14条带机器码和真实助记符的实际指令、0个静态call、0个跳转、0个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。十个静态caller中，胜利奖励、升级面板、成长面板、两类成长标题、法宝完成提示、战利品清单、战败提示和炼符结果九个caller均已关闭；最后一个caller属于下一未审函数，留在其所属工作包。

## 2. signed算术与共享写回

函数依次读取`target`、共享`transition_stage`和`base_offset`，按低32位回绕计算：

```text
numerator = target - transition_stage - base_offset
quotient  = signed(numerator) / signed(divisor)
remainder = signed(numerator) % signed(divisor)
transition_stage += quotient
```

`cdq/idiv`保持x86 signed向零除法；共享加法仍按低32位回绕。商为零时EAX与ECX都返回1，否则都返回0；EDX保留`idiv`余数。除数为零或`INT_MIN / -1`时在原`idiv`点typed-stop：共享stage尚未写回，EAX保留numerator，ECX保留入口共享stage，EDX保留`cdq`符号扩展。

## 3. caller回收

九个已关闭caller都直接复用`LegacyBattleTargetSelectionRuntimeState::transition_stage`唯一owner，并在原位置用各自固定实参推进：胜利摘要使用`212,284,2`；升级提示与法宝完成提示、战败提示使用`212,244,3`；成长属性面板使用`112,268,2`；两类成长标题使用`180,236,3`；炼符结果使用`212,252,3`；战利品清单的target继续取live `212 + count*20`、base为212、除数3。所有旧“查询面板”端口槽保留枚举数值并改为reserved，生产代码零调用；frame coordinator仅保留reserved映射。

caller继续在返回EAX精确等于1时进入文字/明细路径。因共享stage只有一份，同一帧连续面板必须观察前一caller写回后的live值；测试不再伪造同一stage updater同时返回多个任意结果。最后一个未审caller不提前回收。

## 4. 验证

定向测试覆盖正商、负商、向零截断、非零余数、商零返回、双减法与共享加法回绕、除零及`INT_MIN / -1`停点寄存器和写回时机，并覆盖九个已关闭caller的固定目标、动态战利品目标、共享stage连续消费、reserved槽零调用及后续文字门。定向测试、AddressSanitizer、Linux core `188/188`和Linux app `194/194`全部通过，源码构建零warning；app仅有既有ALSA开发库CMake提示。

原版共享stage与十个caller寄存器联合捕获后端尚不可用，`original_diff_verified`为`blocked_runtime_oracle`。
