# 战斗共享画面刷新 `0x0045AF90`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整LST范围

权威函数为`0x0045AF90..0x0045B0D7`，从proc到endp完整138行、9个静态call站点、2个`loc_`标签，无外部FUNCTION CHUNK。7个唯一callee。

十个静态caller中七个已关闭并回收：角色动作分派四处、对手动作分派一处、八槽效果帧一处、群体效果帧一处。三个尚未关闭的caller位于后续战斗函数，不提前修改。

## 2. 三word无变化早退

入口按顺序比较三组snapshot与当前word：

1. 先把snapshot 36写入EAX低word，保留caller EAX高word；不等立即刷新；
2. 相等时把snapshot 38写入ECX低word，保留caller ECX高word；不等立即刷新；
3. 再相等时把snapshot 3A写入EDX低word，保留caller EDX高word；
4. 三组均相等时不调用任何callee，返回当前完整EAX。

因比较在每次低word写后发生，较早不等路径不得提前覆盖后续ECX或EDX低word。typed状态显式保存入口和返回EAX/ECX/EDX。

## 3. 双surface循环

发生任一变化时固定执行两轮，factor依次为1和2。每轮顺序为：

1. 调用Miles serve；
2. 读取对应surface token并锁定，保存完整返回EAX；
3. 以surface token和锁定返回值解锁；
4. 捕获共享pitch；
5. 以固定640×480、零原点和零尾参数准备viewport；
6. 依次调用红、绿、蓝三项颜色处理。

两个surface token来自固定连续双槽；循环不以token值增加modern短路。

## 4. signed半值与调用参数

三项当前word都先按i16扩展，再执行算术右移1。负奇数向负无穷取整，例如全1仍为全1，负3得到负2。结果再按低32位乘当前factor。

颜色callee参数固定为：

```text
lock_token, 0x0003C000, signed_half * factor
```

红值经EAX发布，绿值经EDX发布，蓝值经ECX发布；每个typed请求同时携带调用点完整EAX/ECX/EDX，保留此前callee返回的陈旧高位与覆盖顺序。

## 5. 快照与最终surface

两轮全部完成后，函数按原顺序把当前38、36、3A分别写入ECX、EAX、EDX低word并更新三项snapshot。

随后：

1. ECX载入viewport token；
2. EAX载入最终surface token；
3. 发布refresh pending为1和active surface token；
4. 锁定viewport并保存完整EAX；
5. 以viewport token和锁定值解锁；
6. 返回最后解锁callee完整EAX。

一次实际刷新动态执行16次port call：双surface各7次，最终surface再2次。

## 6. caller回收

已关闭caller全部删除`0x0045AF90` token并直接组合统一typed刷新器。snapshot、surface、pitch和最终发布字段只存放在动作端口与效果端口共同虚继承的单一刷新状态基类；同一组合端口跨两类接口只存在一个物理typed存储，不在四类帧状态中复制全局：

- 角色动作分派与对手动作分派使用帧效果的signed三色值；
- 八槽效果帧与群体效果帧使用各自记录发布的三项共享word；
- 群体效果帧继续接收刷新器最终EAX/ECX/EDX，供后续陈旧寄存器链使用；
- 每个caller把内部port call数合并到原结果。

三个后续未关闭caller继续保留工作包中的导航边，不在本项伪造回收。

## 7. 测试与动态差分

定向测试覆盖：动作/效果端口组合后的同一物理刷新存储、三word完全相等零调用早退、入口高word保留、固定双surface与16次调用、640×480参数、正奇数半值、负1/负3算术右移、factor乘法、pitch捕获、snapshot更新、最终surface锁定参数、完整返回EAX，以及七处已关闭caller token消失。

当前缺少原版Miles serve、双surface、lock/unlock、viewport、三项颜色callee、framebuffer和寄存器联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
