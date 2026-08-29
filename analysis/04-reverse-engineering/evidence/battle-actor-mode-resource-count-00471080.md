# 战斗角色模式资源数量累计 `0x00471080`

状态：`platform_adapted`。完整LST、typed实现、mode grid caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00471080..0x004710CD`，proc至endp共42行、25条实际指令、1个call、5个跳转、2个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。唯一callee为已关闭资源链头提交`0x00470900`。

## 2. 清零、筛选与累计

入口提交next资源链头后，先把caller输出word清零。循环每次从current节点读取next并破坏性写回current；next为空时结束，此时EAX为零。

只有同时满足三项条件的节点才参与累计：node `+0x2C` dword bit27置位、resource id `+0x04`不等于`0x0300`、node `+0x46` byte mask `0x05`非零。实现保留短路顺序，不对不合格节点读取数量。

合格节点先以16位加法计算secondary word `+0x08`与tertiary word `+0x0A`的小计，再以16位加法累加到输出word；负值按原位型参与，节点内和跨节点都允许回绕。扫描期间EDX固定为`0x0300`，ECX保留caller输出token。

## 3. owner与caller回收

资源链、节点字段和current head继续由`LegacyBattleActorListQueryState`唯一持有，旧地址只作为typed token。唯一caller是已关闭mode grid frame `0x00466190`。production secondary-count路径现在直接调用typed累计器并发布到既有state；脚本化port仅由单测显式开启，production默认关闭。随后的已关闭资源链头提交继续使用上一工作包已回收的typed路径。

## 4. 验证状态

测试覆盖固定id排除、bit27门、mode mask门、节点内与跨节点16位回绕、输出先清零、破坏性扫描到空链、EAX/ECX/EDX结果，以及mode grid production不再调用secondary-count port并仍生成相同primary/group布局。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`203/422 = 194 platform_adapted + 9 assembly_exact + 219 pending_audit`，SHA256为`060968b2e3ee56d46b89501fbfa0bd0c0d15e1ff5d394634bd35b40b909b2f4d`。动态差分因原版mode资源链、caller输出word和寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
