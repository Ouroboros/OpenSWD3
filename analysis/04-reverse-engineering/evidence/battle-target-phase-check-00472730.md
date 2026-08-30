# 战斗目标阶段等级与随机判定 `0x00472730`

状态：`platform_adapted`。完整LST、typed实现、caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

权威LST主体为`0x00472730..0x004728C3`，proc至endp共217行、165条实际指令、5个call、16个跳转、8个局部标签、9个返回点，没有外部`FUNCTION CHUNK`。唯一caller是动作dispatch的目标阶段启动路径。

函数先从group-B目标对象解析profile，再无条件调用`0x00484500`取得signed比较值与会覆盖原参数槽的signed基准值。随后要求profile flags的bit5清零、bit11置位，且profile `+0x52` unsigned word不大于21；这些门失败时不访问group-A嵌套profile。通过后读取当前group-A行动者嵌套profile `+0x2C`等级byte与目标profile `+0x54`等级word。

等级差分支严格保持原signed比较顺序。目标高12级及以上直接失败；行动者高10级及以上直接成功；目标高7至11级时比较`sampled_metric <= sampled_argument / 4`。行动者高5至9级时先比较signed三分之一，失败后消费一次`random(100)`并以80为inclusive阈值。目标高1至6级时随机inclusive阈值为`20 - 5 * delta`。行动者高0至4级时阈值为`20 + 10 * advantage`。所有signed除法均向零截断，确定性分支不消费随机。

实现把目标profile的flags、limit和level并入既有唯一group-B message profile owner，把group-A嵌套profile等级并入既有行动者执行owner。待审`0x00484500`保留为仅发布两个输出值的窄port，已知`0x00439070`继续复用既有随机port；不重新引入整个`0x00472730` opaque边界。typed-stop分别位于原目标profile解析点和通过profile门后的行动者嵌套profile访问点，保留此前值查询副作用。

测试覆盖目标profile缺失、profile门先于行动者访问、行动者profile typed-stop、行动者高十级确定成功、signed四分之一、行动者高五级inclusive 80、目标高二级随机拒绝、值查询与随机调用计数、production caller和旧地址零调用。定向测试与独立AddressSanitizer均通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`218/422 = 209 platform_adapted + 9 assembly_exact + 204 pending_audit`，SHA256为`3c41311dab8141c7779904c960c5b406c37a363944ce3edb2c748fd787b8b151`。动态差分因原版双方对象/profile、`0x00484500`双输出、随机状态与caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
