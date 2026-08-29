# 战斗组A效果奖励资料合并 `0x0046F6E0`

状态：`platform_adapted`。完整LST、两资料四重门、typed实现、效果协调caller直连、共享紧凑链owner、定向测试、AddressSanitizer、Linux完整门、x87联合核对与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F6E0..0x0046F84B`，从proc到endp共160行，其中151个非标签物理行、104条实际指令、3个call、15个跳转、9个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，两个栈参数分别为固定奖励紧凑链token和目标记录指针；出口以`retn 8`弹栈。

入口在首次角色资料读取前先发布`0xFFFFFE58 - actor_token`到EDX，随后固定扫描角色两份0xA4内嵌资料。每轮从相对资料起点`+0x10`读取item id，从`+0x08`读取资料kind，从`+0x04`读取最大数量。

## 2. 四重资格门

每份资料必须依次满足四个条件才进入奖励链：item id非零、kind等于51、目标记录u16 `+0x10`在1至9、计算地址非零。目标记录只在前两项满足后首次解引用，typed owner缺失时保留目标token到EAX、actor高word与item id低word拼接到ECX，以及actor差值EDX。

地址计算按原顺序执行：目标word先零扩展，与保存的actor差值和资料游标相加后得到原目标word，再加actor token与固定偏移形成EDX。虽然中间项代数可约掉，typed实现不删除其寄存器发布。EDX为零时直接跳过，不访问奖励链。

任一资料通过全部四门后把返回latch置1。后续阻塞、命中、尾插或第二资料跳过均不会把该latch清回零。

## 3. 命中与尾插

奖励链与第181项共用同一物理typed owner。查找从根本身开始，按显式next token顺序扫描；断链在下一节点首次解引用处typed-stop。命中且节点`+0x0A`阻塞word非零时只保留latch，不改数量。

命中未阻塞时把节点u16数量固定加12，先回绕再与资料最大值作unsigned比较并按原条件夹值。百分比继续按x87扩展精度分步执行`quantity / maximum * 100.0f`并向零转qword，低word写节点百分比；零最大值保留integer-indefinite。

未命中时申请固定20字节，先发布尾next，再清新节点五个dword。新节点写item id、固定数量12和x87 `12 / maximum * 100`，不按最大值夹新数量；最后递增根`+0x04`低word，保留它可能让第二资料误命中根的别名行为。零分配token和宿主分配失败均在对应原前缀后隔离。

## 4. caller与共享owner回收

全程序四个静态caller都位于已关闭战斗效果协调器。typed实现中的公共分支把它们收敛为三个直接调用位置：当前组B角色命中组A目标、组B群体效果逐个处理组A、普通扫描逐个处理组A。每次都传入实际组A actor token、startup中该actor已有的两份内嵌资料、当前组B目标记录门word和目标token。

第181项紧凑链从胜利状态内部提升为`LegacyBattleGroupARewardProfileStatePort`虚拟共享owner；胜利奖励port与效果port在组合运行时只拥有同一实例。效果协调器不复制资料，只显式借用frame coordinator context中的startup owner。旧完整函数地址调用生产零次；分配callee继续保持固定20字节窄port。

## 5. typed-stop与寄存器

角色token或两资料owner缺失时在首次资料读取处停止，保留入口EAX/ECX及已经发布的actor差值EDX。目标记录、链头、next节点和分配失败各自只在原首次访问或故障点停止，并保留此前门计数、链遍历、尾链接和allocator副作用。

正常完成EAX只表示是否至少一份资料通过全部资格门。ECX/EDX保留最后一轮资格、阻塞、x87转换或循环重置的值；第二份资料开始前EDX无条件重载为actor差值，因此第一份百分比转换的高dword可被后续零资料覆盖。

## 6. 验证状态

单元测试覆盖入口actor差值、两资料全零、kind不匹配、目标owner缺失、0/10门跳过、链头typed-stop、根命中、固定12夹值、尾插不夹值、断链、零分配、零最大值、根别名影响第二资料、阻塞节点和显式next命中。效果协调回归覆盖正常直连共享owner、三种调用路径计数、旧opaque零调用及断链stop在actor发布前传播。

新x87实现与第181项已验证helper逐字一致；另以真实内联x87对固定12的全部65,535个非零最大值、六条全u16网格及零分母共458,751组联合核对，全部一致。

验证结果：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`182/422 = 173 platform_adapted + 9 assembly_exact + 240 pending_audit`，SHA256为`b840953fb5739e4319c08f7e2affa12b7ae9e00364597c72c557dd1601464816`。原版组A动态资料、组B目标记录、共享紧凑链token拓扑、分配器、x87控制字和四处caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。
