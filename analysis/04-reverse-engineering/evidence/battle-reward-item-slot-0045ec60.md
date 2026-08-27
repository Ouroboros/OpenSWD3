# 战斗奖励道具槽写入 `0x0045EC60`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`。

## 1. 权威范围与ABI

权威LST完整主体为`0x0045EC60..0x0045EC71`，从`proc`到`endp`共13行、4条实际指令、0个call、0个跳转、0个局部标签，没有外部`FUNCTION CHUNK`。

函数是两参数cdecl叶函数。caller按32位栈槽依次传入u32索引和低16位有效值，caller负责回收8字节。机器体严格只有：

1. ECX读取完整u32索引；
2. AX读取第二栈槽低16位，保留入口EAX高16位；
3. 向`0x004FF2EA + index*2`写一个word；
4. 直接返回。

正常尾EAX为入口高16位与写入值低16位拼接，ECX为索引，EDX完整保持入口值。

## 2. 三word物理别名

LST数据目录显示`0x004FF2EA`只显式声明一个word，紧邻`0x004FF2EC`是结果奖励整理读取的dword。唯一caller`0x00462740`的三个callsite只传索引1、1、2，因此实际业务写入为：

```text
index 0 -> 0x004FF2EA，奖励槽前缀word
index 1 -> 0x004FF2EC，第一项玩家奖励ID
index 2 -> 0x004FF2EE，第二项玩家奖励ID
```

三word统一映射到结果奖励整理的唯一typed state：保留一个前缀word和两项奖励ID，不建立平行数组。已关闭`0x0045E9C0`会live读取索引1/2写入的两项ID，发放奖励后只清两项ID；全局重置按原写集合保留前缀和奖励ID。

## 3. 回绕与typed-stop

目标地址使用u32低32位乘2再加固定基址；例如索引全1得到`0x004FF2E8`。不作现代索引预验。

当前typed owner只覆盖物理索引0、1、2。其他索引在唯一原始word store处typed-stop；停止前已经完成ECX索引读取、AX低字覆盖和u32目标地址计算，不修改三wordowner。索引3会命中相邻`0x004FF2F0`全局，不能被误当作奖励数组扩容。

## 4. caller与验证

导航调用图的三个静态callsite全部位于尚未关闭`0x00462740`，因此本工作包不提前回收caller；后续关闭该函数时必须复用结果奖励唯一state并直接组合本typed writer。

定向测试覆盖索引0/1/2三项物理地址与写入别名、入口EAX高字、第二参数高字不读取、ECX索引、EDX保持、索引3首个相邻全局停止、全1索引地址回绕、不修改owner，以及写入奖励被已关闭结果奖励整理直接消费并按原尾store清零。

该叶函数合法三word域由完整LST、固定状态与逐字节owner验证；原版相邻全局内存、三个caller输入轨迹及寄存器联合捕获后端缺失，越界域只能在真实store隔离，`original_diff_verified`为`blocked_runtime_oracle`。
