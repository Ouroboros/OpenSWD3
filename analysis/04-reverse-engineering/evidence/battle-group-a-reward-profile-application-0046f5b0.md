# 战斗组A角色奖励资料合并 `0x0046F5B0`

状态：`platform_adapted`。完整LST、两资料链表语义、typed实现、胜利奖励caller直连、定向测试、AddressSanitizer、Linux完整门、x87联合核对与inventory双生成均已关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x0046F5B0..0x0046F6D1`，从proc到endp共128行，其中120个非标签物理行、87条实际指令、3个call、10个跳转、8个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall，两个栈参数分别为胜利奖励紧凑链头token和32位数量值；主体只使用数量低word，出口以`retn 8`弹栈。

函数固定扫描角色两份0xA4内嵌资料。每份读取相对资料起点`+0x10`的物品id；零id直接跳过，不访问奖励链。相对资料起点`+0x04`的u16是该项最大数量。

## 2. 链表命中路径

奖励链根位于固定token，对应物理紧凑节点布局：下一节点token、`+0x04`的id或根计数字、`+0x06`数量、`+0x08`百分比、`+0x0A`阻塞word。每个非零资料先把结果latch置1，再从根本身开始顺序比较id并沿显式token找下一节点。

命中且阻塞word非零时不修改数量或百分比，但返回latch仍为1。命中且未阻塞时，先以u16回绕执行`quantity += delta`，再作unsigned比较；只有回绕结果不小于资料最大值才夹到最大值。因此溢出回绕到较小数时不会现代化补夹值。

百分比按`fild quantity; fidiv maximum; fmul 100.0f`在x87扩展精度中分步计算，再由已关闭转换callee临时改为向零并`fistp qword`。旧版typed实现用Linux上的分步`long double`求值；现用整数位运算模拟64位有效位除法及乘法的逐步舍入，避免Windows平台`long double`精度不同。不能直接改写为整数`quantity * 100 / maximum`，例如21除5乘100的原结果是419而非数学整数420。最大值为零时保留masked divide-by-zero及integer-indefinite，返回EAX低dword为0、EDX为`0x80000000`并把百分比低word写0。

## 3. 未命中尾插路径

未命中时继续沿token走到尾节点，调用分配callee申请固定20字节。原顺序先把返回token写入尾节点next，再从新token开始清五个dword；零token因此在分配调用和尾链接前缀之后故障。typed实现把该点隔离为allocation stop；宿主容器分配失败另有host stop，并保留已经发布的尾token。

新节点五个dword先全零，再写资料item id、数量低word和x87百分比；新建路径不按最大值夹数量。最后原指令无条件递增根节点`+0x04`低word。该word同时参加下一资料的根id比较，因此首个尾插可能让第二份资料误命中根并改写根数量；typed实现和测试显式保留这一可观察别名行为。

## 4. typed-stop与寄存器

角色token或两资料owner缺失时在首份资料id读取处停止，保留入口EAX/ECX/EDX。奖励链owner仅在遇到首个非零资料后才需要；缺失时EAX保留当前循环前缀，ECX只替换低word为item id，EDX保持此前值。

非零next token找不到typed节点时，在原下一节点首次解引用处停止，EAX保留断链token，ECX保留actor高word与item id低word拼接，EDX保持此前值。正常完成时EAX只表示两份资料中是否至少一项非零；ECX/EDX保留最后一次资料扫描、百分比转换或阻塞分支留下的值。

## 5. caller回收与owner

全程序唯一caller位于已关闭的战斗胜利奖励函数。它传入当前组A角色、固定奖励链token和奖励经验低word；原opaque调用之后只以EAX是否为1决定奖励gate。当前caller改为直接使用startup角色属性汇总中已有的两份内嵌资料和胜利、效果运行路径共享状态端口中的唯一紧凑链owner，直接发布typed返回寄存器与stop。

旧胜利奖励调用枚举槽及帧协调器转发槽保留为reserved，生产路径零调用。分配callee仍以固定20字节窄端口保留，不提前宣称底层内存管理器关闭。

## 6. 验证状态

单元测试覆盖两资料全零、首次资料与链头typed-stop、根命中、u16溢出后不夹、阻塞节点、显式next命中、断链、尾插、零分配token、零最大值indefinite、根id递增影响第二资料以及胜利caller正常与stop传播。独立程序以真实内联x87 `fild/fidiv/fmul/fisttp`对typed分步求值联合核对3,328,518个理论整数边界、1,900,529个全量行列网格及零分母，共5,229,047组，全部一致。

验证结果（原Workpack269阶段）：定向测试与独立AddressSanitizer均为`1/1`通过；Linux core为`188/188`，Linux app为`194/194`，源码零warning，app仅有既有ALSA提示。inventory连续双生成逐字节一致，稳定为`181/422 = 172 platform_adapted + 9 assembly_exact + 241 pending_audit`，SHA256为`d7c8ab24eb569ac1fe95f02e58c898ec9b573c2a56042166ed2a9d8a63a7ed6c`。原版组A两份动态资料、奖励紧凑链token拓扑、分配器副作用、x87控制字和胜利caller寄存器联合捕获后端缺失，动态差分登记为`blocked_runtime_oracle`。

## 7. Windows完整门禁触发的跨平台补修（不重定义Workpack316验收）

用户明确允许修复已关闭奖励范围的Windows精度问题。Windows全量core首次编译在共享MON测试夹具`opened_path`的`std::string = std::filesystem::path`处失败；将夹具成员改为与调用参数相同的`std::filesystem::path`后可编译。首次全量执行为199/200：本函数21/5乘100的已知x87边界期望419，旧版Windows`long double`实现未通过；`battle.actor_frame_316`同一构建的定向测试为1/1。该失败不由`sub_479850`改动引起，不得假装此前Windows全量门禁通过。

新百分比实现对16位非负输入先算数学整数及余数：非整数产品距边界至少`1/maximum`，大于本输入范围中两步x87舍入误差；整数产品再规范化`quantity/maximum`，逐位提取63个二进制小数位，按最近偶数舍入除法结果，并以精确余数与乘100后整数的下方半ULP比较。整数恰为2的幂时下方间距只有上方一半，单独采用较低阈值；等距舍入返回偶数整数，不做无证据的统一`quantity*100/maximum`截断。零分母仍沿原indefinite结果。既有原阶段5229047组x87联合核对对应的是**旧Linux实现**；不可把该数目移作本次新整数算法的已执行验证。本次新增Linux64位尾数参考网格及Windows门禁结果须另计。

修正后，Linux x86-64定向测试实际比较`67,880`组宿主80位`long double`分步结果与新整数实现（数量0..255、最大值1..255的`65,280`组，另有高位取样`2,600`组），`battle.legacy_battle_setup`为1/1；Windows同一测试及全量core为200/200、app为206/206。最终稳定源码按仓库包装器串行跑Linux core 200/200、ASan 200/200、app 206/206，以及设置已有Windows Python解释器后的`build.bat core --test` 200/200、`build.bat app --test` 206/206，五项均退出0；日志`build/tmp/runtime/wp316-action7-x87-final-five-gates.log`，SHA-256 `460940679c8db97aee357df9a6e0fd88b7ce17ed1fd7719e0dd06cd787db6ed3`。该网格以Linux扩展精度旧实现为受控参照，并非新算法67,880组原版动态回传；VM x87控制字及奖励现场未因此补齐。Workpack316依旧`pending_audit`、315/422。
