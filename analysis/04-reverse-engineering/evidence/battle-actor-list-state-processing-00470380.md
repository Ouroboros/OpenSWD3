# 战斗角色链表状态处理 `0x00470380`

状态：`platform_adapted`。完整LST、typed实现、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470380..0x004705B7`，proc至endp共284行、164条实际指令、8个call、30个跳转、18个局部标签、4个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭索引提交两次、待审资源链重建一次、已关闭消息发布三次和已关闭sample播放两次。

## 2. 主链与字段发布

category选择器映射仍为0到`0x10`、1到`0x0C`、2到`0x1001`、其他保持。occurrence为零时在首次索引提交前返回0。否则提交索引，从主链按next顺序筛选mode byte与`0x05`、category flags与mask以及type 27至30；匹配计数先增后比，第N项未找到返回0。

命中节点value word的bit15清零selected resource并把低15位写primary required；bit14清零selected resource并把低14位写secondary required。两项可同时发布，均不先清除另一阈值。

## 3. bit11资源分支

bit11命中时以低11位覆盖primary required，调用窄资源链重建，然后从resource head按next扫描resource id。未找到，或找到但两项signed quantity均不大于零时，仅在共享message token为空时发布缺资源消息与sample，最终返回0。任一quantity大于零时发布selected resource token并返回1。此分支不执行第二次索引提交。

## 4. 普通容量分支

bit11未命中时再次调用typed索引提交。primary required非零时，将live actor signed primary capacity与零扩展required做signed比较；容量不小于required立即返回1，否则按共享message latch发布主容量不足消息与sample。随后对secondary required和live signed secondary capacity执行同样流程。两required都为零或均不足时返回0；首条消息若发布了token，会抑制同调用后续消息。

## 5. owner、stop与caller

主链、资源链、阈值与selected token均扩展在第188项唯一owner中，actor索引继续复用第186项。缺失任一owner或节点时在首次访问处typed-stop并保留此前提交、字段写和消息副作用。待审资源重建保留窄port；消息与sample使用语义窄边界。唯一caller位于已关闭目标选择刷新函数，现有边界尚未暴露链表物化owner，本包不复制第二份状态；生产源码无旧地址调用。

## 6. 验证状态

测试覆盖零occurrence早退、双索引提交、相等容量成功、bit11资源选择、缺资源消息/sample及共享message token发布。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`190/422 = 181 platform_adapted + 9 assembly_exact + 232 pending_audit`，SHA256为`b470baa980d0fc1b5e3eb51745929f1e24f82750a59b551f46171a6174837bd7`。动态差分因原版两条链、live容量、消息/sample与caller联合捕获后端缺失而登记为`blocked_runtime_oracle`。
