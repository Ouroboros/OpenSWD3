# 战斗组A成长奖励选择 `0x0046F850`

状态：`platform_adapted`。完整LST、两资料链扫描、typed实现、成长结果caller直连、共享owner、定向测试、AddressSanitizer、Linux完整门与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F850..0x0046F8B4`，从proc到endp共63行，其中57个非标签物理行、39条实际指令、0个call、8个跳转、6个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，唯一栈参数为奖励紧凑链token，出口以`retn 4`弹栈。

入口无条件把EAX、选择latch和资料索引清零，并把EDX发布为actor token加首资料item字段偏移；因此首个角色资料owner缺失时不保留入口EAX/EDX。

## 2. 两资料与链扫描

函数固定扫描角色两份0xA4内嵌资料，每份只读取相对资料起点`+0x10`的item id和`+0x04`的最大数量。零item id不访问紧凑链，循环尾仍把EDX资料游标前进0xA4。

非零item id从共享紧凑链根本身开始按显式next token顺序扫描。每个同id节点依次检查`+0x0A`阻塞word和`+0x06`数量：阻塞非零或数量小于资料最大值时不能选择，但必须继续沿链寻找后续同id重复节点。只有首个未阻塞且数量按unsigned不小于最大值的节点成功。

成功后把节点数量低word写成资料最大值，即使原数量更大也向下夹值；随后把阻塞word写1，EAX返回资料item id。最大值为零时任意未阻塞同id节点都立即成功，数量写零。

## 3. 首次成功后的第二资料行为

成功latch不在当前资料尾立即返回。函数仍把资料游标推进到第二份并读取第二item id；若第二id非零，才在访问链前因latch为1提前返回。若第二id为零，则完成第二轮循环尾并返回。因此两种路径的最终EDX分别停在第二item字段或越过两份资料后的游标，ECX继续保留首次成功节点token。

如果两份资料均未成功，EAX保持零。链走到尾时ECX为零；两资料全零时ECX始终保持actor token。

## 4. typed-stop与共享owner

actor token或两资料owner缺失时在首份item id读取处停止，返回EAX 0、ECX actor token和首资料游标EDX。遇到非零资料后才访问链owner；链头缺失时ECX保留传入链token。非零next token找不到typed节点时，在下一节点首次解引用处停止，EAX保持0、ECX保留断链token、EDX保留当前资料游标。

紧凑链继续复用第181、182项提升出的`LegacyBattleGroupARewardProfileStatePort`唯一owner，角色资料继续借用startup party属性汇总已有两份记录，不复制第二套状态。

## 5. caller回收

全程序唯一caller位于已关闭成长物品结果选择器。原流程在每个未跳过且未完成的组A角色上调用本函数，AX非零时加载物品定义、释放描述、复制标题并发布角色索引。

当前caller直接传入该actor的startup两资料和共享紧凑链，完整发布typed返回寄存器与stop。旧成长选择调用枚举槽和frame coordinator转发槽保留为reserved，生产路径零调用；后续定义加载、描述释放和标题复制的端口顺序保持不变，只移除旧完整函数那一次port call。

## 6. 验证状态

单元测试覆盖入口寄存器前缀、两资料全零、链头typed-stop、根充分命中、数量不足、阻塞重复后选择下一重复、断链、首资料成功后第二id非零早退、零最大值和第二资料成功。成长结果回归覆盖空资料跳过、直接选择后的定义/描述/标题四调用链、标题typed-stop、共享节点阻塞写回、旧opaque零调用和链断裂向message phase传播。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`183/422 = 174 platform_adapted + 9 assembly_exact + 239 pending_audit`，SHA256为`2b1b964c11dbf21fe09f4abaddf3759f2efbb558028bdf5fdca90187dc9749e5`。原版组A动态资料、共享紧凑链token拓扑、成长caller寄存器和后续物品定义加载联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
