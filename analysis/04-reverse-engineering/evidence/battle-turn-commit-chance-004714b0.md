# 战斗回合提交概率门 `0x004714B0`

状态：`platform_adapted`。完整LST、typed实现、group-A frame caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x004714B0..0x0047153D`，proc至endp共76行、51条实际指令、3个call、6个跳转、5个局部标签、5个返回点，没有外部`FUNCTION CHUNK`。三个call均为已关闭secondary RNG有界接口`0x00439070`，固定bound为100。

## 2. 输入、owner与零值短路

函数只读取参数低word。candidate为零时立即返回0，不解引用first group-A actor记录，也不消费RNG。非零时从固定group-A基址对象的首指针读取记录`+0x2C`低byte；typed实现复用startup party零号角色的唯一configuration actor-record owner，token为空时在原始二级解引用点停止。

actor level按unsigned byte，candidate按unsigned word。差值使用32位`level - candidate`。caller传入的是此前遍历group-B对象得到的最大值；物理this始终固定第一group-A actor，不随当前frame actor index变化。

## 3. 分段概率

分支严格如下：

- candidate大于level：调用一次RNG，返回`roll <= 35`；
- 差值等于0：直接返回0；
- 差值1..7：调用一次RNG，返回`roll <= 70`；
- 差值8..12：调用一次RNG，返回`roll <= 90`；
- 差值大于等于13：直接返回1。

三个随机阈值均包含端点，不改成百分比浮点或标准分布。确定性分支不额外消费随机。

## 4. caller回收与验证

唯一caller为group-A frame `0x00456680`的turn-resolution分支。production现在直接调用typed概率门；成功继续原publish-turn-result与negative-turn链，失败执行原全group-A失败发布、消息、sample和门清零。原整函数callee token与测试注入均已删除。candidate为零的既有测试因此纠正为权威LST的确定失败路径，不再依赖opaque默认返回1。

测试覆盖零值不访问owner、candidate高于level的35/36边界、相等直接失败、差值1的70端点、差值8的90端点、差值13直接成功、随机消费次数，以及成功/失败caller集成和整函数port移除。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`206/422 = 197 platform_adapted + 9 assembly_exact + 216 pending_audit`，SHA256为`e98195773f94491f0e042e58db5085c250a5e8682903fc68ab2fc9ea0e7a5e4b`。动态差分因原版first actor指针、secondary RNG状态和caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
