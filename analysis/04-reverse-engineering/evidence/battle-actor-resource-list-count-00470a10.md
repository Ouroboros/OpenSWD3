# 战斗角色资源链计数 `0x00470A10`

状态：`platform_adapted`。完整LST、typed实现、双caller生产链回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470A10..0x00470ABB`，proc至endp共105行、60条实际指令、1个call、18个跳转、1个返回点，没有外部`FUNCTION CHUNK`。内部资源链头提交`0x00470900`已关闭。

## 2. 行为

category 0至5映射为0x10、0x0C、0x1001、0x0800、0x2000、0x08000000，其余值原样作为掩码。入口不清输出word，而是保留调用者初值；每次递增均按16位回绕。

提交链头后逐节点破坏性推进。category、mode bits 0/2和`signed word0A - unsigned word06 + signed word08`正值全部命中时递增一次。仅category 4映射的0x2000模式在每个节点筛选完成后额外检查node byte `+0x4D` bit5，命中再独立递增一次；因此同一节点可增加两次，不满足普通筛选的节点也可由额外bit增加一次。

## 3. caller与owner

资源节点与current/next链头复用第188项唯一owner。两个静态caller位于已关闭标准grid `0x004659C0`和alternate grid `0x00465E50`。production selection frame现在传递startup party span，初始化计数、链头提交和行查询三段均typed直连。现有脚本化grid单测显式开启compat port开关，production默认关闭且始终走typed路径。

## 4. 验证状态

测试覆盖六类映射中的category 4、保留初值、16位回绕、普通正值匹配、额外bit独立匹配、同节点双增量、破坏性链尽以及grid生产bindings。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`198/422 = 189 platform_adapted + 9 assembly_exact + 224 pending_audit`，SHA256为`6c5586aaab691a5a1576cf43cdd9419129384afe85290c0d53509de99f714828`。动态差分因原版资源链、输出word和两个grid caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
