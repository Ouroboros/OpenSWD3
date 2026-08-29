# 战斗角色资源释放 `0x00470E20`

状态：`platform_adapted`。完整LST、typed实现、六处production caller回收、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围

权威LST主体为`0x00470E20..0x00470F65`，proc至endp共165行、100条实际指令、3个call、19个跳转、3个返回点，没有外部`FUNCTION CHUNK`。callee为已关闭资源链头提交`0x00470900`两次，以及节点释放`0x004885A0`一次。

## 2. 定位与门控递减

入口先无条件把next资源链头提交到current。selected token为零时只清AX并保留已提交head的EAX高word。非零时从哨兵开始破坏性推进current，以candidate与selected的resource id word匹配；循环次数包含哨兵到candidate的每次推进。current或selected在原始首次解引用点无typed owner时停止。

匹配后current改为selected，actor workspace `+0x2F14`写入resource id。node `+0x48` bit10命中且低byte非零时，先把低byte减一并保留高byte；剩余非零直接跳过数量更新。剩余归零则继续普通路径。

普通路径读取node `+0x2C`。低byte bit7清零时允许更新；bit7置位但bit27同时置位时也允许更新；仅bit7置位且bit27清零时跳过。允许路径优先递减secondary word，secondary为零才递减tertiary word；primary word只要大于零就独立递减。三者均保持16位行为，随后无条件清selected token。

## 3. 销毁、重链与返回

secondary与tertiary均为零时，即使primary仍非零也销毁selected节点。函数先保存selected next，再释放节点，重新提交哨兵head，并按之前累计的循环次数回放到前驱，最后把前驱next改为保存值。删除后current停在前驱；未删除时current停在selected。

最终测试current node `+0x4D` byte bit5，也就是唯一word owner `capacity_gate_flags`的bit13。命中时只清AX；否则AX写actor workspace中的resource id。两条路径均保留EAX高word，并返回current token于ECX及分支形成的EDX陈旧值。

## 4. owner、caller与验证

链头、selected token、节点字段与资源vector继续由`LegacyBattleActorListQueryState`唯一持有；`+0x2F14`继续复用party workspace owner。节点释放映射为vector erase，且严格保留先取next、释放、重新提交、回放、重链的次序。

六处静态caller全部位于已关闭函数：动作dispatch两处、目标选择刷新三处、链表动作执行一处。production均直接使用startup party typed owner；既有脚本化port测试通过显式compat flag保留，默认关闭。第199项两处固定category caller同时按权威地址纠正到message 27与message 30，而不是动作枚举槽。

测试覆盖selected零的AX半寄存器行为、低byte门控先行递减、secondary优先、primary独立递减、bit13返回抑制、中间节点销毁、循环次数回放重链，以及动作dispatch、目标选择和链表动作caller集成。定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning。inventory连续双生成逐字节一致，稳定为`200/422 = 191 platform_adapted + 9 assembly_exact + 222 pending_audit`，SHA256为`981dcf48b591865cb3d92a84f30a9eacc849fbcf677e7758c87e6a2de80ad2ad`。动态差分因原版资源链节点、allocator释放和六处caller寄存器联合捕获后端缺失而登记为`blocked_runtime_oracle`。
