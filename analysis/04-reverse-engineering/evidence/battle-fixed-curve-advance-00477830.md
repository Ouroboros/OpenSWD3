# 战斗固定键曲线计数推进 `0x00477830`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 当前语义接口

次数递增直接接收共享状态、WORD键、WORD最大值与WORD乘数。
结果返回实际次数、百分比、signed i64计算值，以及命中路径和失败位置。
删除入口/返回寄存器、模拟x87栈阶段、读写与转换计数、父级查询计数。
效果调用方直接使用角色的三个WORD字段，并把计算值低WORD写入共享动作数值。
下文寄存器和ABI说明保留为原指令证据，不再作为本函数的C++调用合同。

## 1. 完整权威范围与调用图

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477830..0x0047791C`，从proc到endp共103个物理行、72条实际指令、5个call、4个跳转、4个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。唯一caller是已关闭的`0x00474FC0`，物理callsite为`0x00474FF3`。

五个call由20字节分配包装器`0x00487C10`一次和x87截零转换helper`0x00489654`四次组成；两条主路径各调用转换helper两次。`0x00489654`完整主体先保存x87 control word，把舍入控制位改为toward-zero，以`fistp qword`转换ST(0)，再恢复原control word并返回EDX:EAX；它没有外部chunk。

函数采用cdecl四参数ABI：根token、键、最大值和乘数。后三项都只使用低word。入口保存EBX/ESI/EDI，把键放入BX，以EDI保存根、ESI保存当前记录；正常返回恢复三个非易失寄存器。

## 2. 固定根、扫描和已有记录路径

固定根不是纯哨兵，函数先比较`word [root+4]`；不匹配时严格从当前记录`+0x00`读取next到EAX，next非零则切换ESI并比较新记录`+0x04`键，next为零才进入分配路径。没有现代长度上限、环检测、排序或nil替代值。

已有根或动态节点命中后按原顺序执行：

1. 对`word [record+6]`执行word递增，保留`0xFFFF→0`回绕。
2. 以递增后的无符号count与最大值低word比较。
3. `count >= maximum`时再次写`word [record+6] = maximum`；不是只在大于时夹限。
4. 清完整ECX，再把最终count写入CX；EAX只保留maximum低word。
5. 以x87计算`count / maximum`。
6. 复制比值、乘单精度常量`100.0f`，经`0x00489654`截零为signed i64，并把返回AX写`word [record+8]`；`+0x0A`高word保持原值。
7. 原比值乘零扩展后的乘数低word，再经同一helper截零；最终EDX:EAX是该signed i64结果，ECX是最终count零扩展。

因此最大值不是数量20上限，而是本次曲线分母和已有计数夹值。递增回绕发生在比较前，不能现代化为宽整数或饱和加法。

## 3. 缺键分配、x87交错与返回

缺键时以固定大小20调用`0x00487C10`。callee返回后只采用EAX token；EDX被最大值低word覆盖，ECX完整清零。原函数先把token写入前驱`+0x00`，再以零按以下精确顺序初始化新节点：

1. 清`+0x00`。
2. 把maximum作为signed dword压入x87。
3. 清`+0x04`和`+0x08`。
4. 用单精度常量`1.0f`执行反向除法，得到`1 / maximum`。
5. 清`+0x0C`和`+0x10`。
6. 从前驱link重取新节点token，写`word +0x04 = key`、`word +0x06 = 1`。
7. 把比值乘`100.0f`并截零，写AX到`word +0x08`。
8. 把乘数低word放入EAX后，以word回绕递增根`+0x04`。
9. 原比值乘乘数并截零，返回EDX:EAX；ECX保持零。

实现保留先链接、清零与x87除法交错、字段写入、根word递增和第二次转换的顺序。
根和动态节点仍使用同一共享状态；节点改由实际容器分配和持有。
分配Port、寄存器请求/回复和转发已删除，固定状态Port尚待后续迁移。
当前分配与测试合同见[实际分配记录](battle-fixed-chain-owned-allocation.md)。
本文allocator寄存器及短区回复描述保留为原指令和历史夹具分析。

## 4. x87特殊值和转换合同

正常输入使用`long double`表达原x87中间值；当前Linux ABI提供80-bit扩展精度，并在两个原转换点显式toward-zero。测试以`1/3 * 100`锁定结果为33，而不是四舍五入34；caller回归另以`32769/65535 * 65535`锁定较大分数路径。

maximum为零时不增加防护：已有记录先把count夹为零，形成`0/0` NaN；缺键形成`1/0`无穷。`fistp qword`的invalid结果按x87 integer indefinite保留为`0x8000000000000000`，因此AX和最终EAX为零，EDX为`0x80000000`。该行为不能用现代零值早退掩盖。

## 5. 原访问点typed-stop

- 根或next记录`+0x04`键不可访问：保留此前扫描得到的EAX token及入口ECX/EDX。
- 已有记录`+0x06`递增不可访问：键已经命中，但count、比例、scale和后缀均未发生。
- 已有记录`+0x08`scale写不可访问：计数递增/夹限和第一次x87转换已经完成，返回寄存器保留第一次转换的EDX:EAX及ECX count。
- allocator返回零或不可映射token：前驱link已先发布，随后停在新节点`+0x00`。
- 分配记录清零不可访问：保留前驱link和此前完成的dword清零前缀；`+0x04/+0x08`故障时x87 ST(0)阶段为已加载maximum，`+0x0C/+0x10`故障时为已完成`1/maximum`的ratio。

当前结果保留失败记录与偏移，并通过实际共享数据体现已经完成的写入。旧结果中的模拟x87栈阶段已删除；浮点最大值加载、除法、百分比转换和乘数转换仍在原位置。失败时不伪造正常返回，不执行未到达的键、count、scale、根word或第二次转换后缀。

## 6. caller回收

`0x00474FC0`在原`0x00474FF3`位置直接传入行动者的曲线键、最大值和乘数。行动者motion和共享motion仍先清零；成功后把实际signed i64计算值低WORD写入共享motion，再继续方向、mode-one skip、目标刷新、效果计算、累计和发布。不再从曲线结果重建EAX/ECX/EDX。

fixed-curve typed-stop保留两次motion清零和helper内部前缀，阻断共享motion发布、方向后缀、目标callee和最终effect-application latch。动作4与特殊动作400的typed组合继续向上发布独立`fixed_curve_typed_stop`，不把故障混同为shared缺失。生产`include/src`除typed closure注释外不再保留`0x00477830`地址调用边界。

## 本次语义迁移复核

完整LST终点为`47791C`，已修正文首旧终点。
共享截零函数现在直接返回signed i64；NaN、无穷和范围外结果保留
`INT64_MIN`，不会提前返回成功零值。
本批仅同步另外两个曲线设置函数对同一截零函数的调用。
[普通曲线设置](battle-fixed-curve-set-00477920.md)与
[定义曲线设置](battle-fixed-definition-curve-set-00477a20.md)随后已完成语义迁移。
固定状态Port及MON内部剩余协议仍待处理。

调用方在写入共享动作数值后，不再使用曲线返回寄存器作为业务输入。
后续跳过判断在`47CD60`覆盖EAX，按自身参数与状态判断；其DX比较使用
`47CE33`新加载的字节。效果计算在`481013`直接从参数覆盖EDX。
父效果函数其他通用调用仍待迁移，不代表整个效果链已经清理完毕。

测试检查`1/3`截零、较大分数、次数回绕、百分比高WORD保留，
以及`0/0`和缺键`1/0`的实际64位结果。
失败测试检查数量不可写时原记录保留、百分比不可写时数量前缀保留，
以及调用方两个动作数值确已清零而后续发布未发生。

本批core和AddressSanitizer分别构建并执行固定链、setup、角色帧和菜单，
八项定向测试均各通过1/1；删除剩余旧编号断言后角色帧已复测。
SDL应用构建通过，未启动游戏。
日志位于`build/tmp/runtime/fixed-curve-advance-semantic-`前缀下：
`core-chain.log`、`core-setup.log`、`core-actor-final.log`、`core-menu.log`、
`asan-chain.log`、`asan-setup.log`、`asan-actor-final.log`、`asan-menu.log`
及`sdl.log`。源码、测试和文档差异已逐项复核。
生产及测试扫描已无本接口旧request、查询计数和callee编号调用断言；
地址只留在源码的原函数出处注释中。

## 7. 历史验证与动态差分

以下完整门禁及短区分配夹具属于旧工作包记录，不是本次重新执行的验证。

叶函数UT覆盖已有根命中、动态节点、递增后inclusive夹限、word回绕、缺键分配、五字清零、`1/3`双截零、scale高word保留、maximum零x87 indefinite、已有count/scale访问stop，以及分配`+0x08`前maximum阶段和`+0x0C`前ratio阶段。caller测试覆盖fixed owner真实曲线输出、mode-one skip、signed motion累计、9999夹限、负一抑制、caller count访问stop及全部旧地址零调用。

验证：battle聚合定向通过并连续实际执行10次；Linux core`194/194`、AddressSanitizer`194/194`、Linux app`200/200`及changed-range clang-format Werror全部通过。最终日志零源码warning、测试失败、sanitizer finding或runtime error；验证期间未启动原版或OpenSWD3游戏程序。inventory连续双生成逐字节一致，稳定为`269/422 = 259 platform_adapted + 10 assembly_exact + 153 pending_audit`，SHA256为`95514d8b21ac60676fb2395a4deffbff78803f691dfce0d7661cf44c06a0a9e5`。

当前缺少原版固定曲线链、allocator堆、x87 control/status word、行动者曲线字段以及`0x00474FF3`联合寄存器捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合、原位置typed-stop和Linux门禁。
