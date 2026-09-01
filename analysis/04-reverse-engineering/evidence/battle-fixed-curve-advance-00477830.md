# 战斗固定键曲线计数推进 `0x00477830`

状态：`platform_adapted`、`unit_tested`、`caller_reclaimed`。

## 1. 完整权威范围与调用图

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477830..0x0047791A`，从proc到endp共103个物理行、72条实际指令、5个call、4个跳转、4个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。唯一caller是已关闭的`0x00474FC0`，物理callsite为`0x00474FF3`。

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

实现保留先链接、清零与x87除法交错、字段写入、根word递增和第二次转换的顺序。allocator token、固定根与全部动态20字节节点继续只由`LegacyBattleFixedObjectStatePort`持有，复用既有`LegacyBattleFixedCountAllocationPort`，没有建立第二条曲线链。

## 4. x87特殊值和转换合同

正常输入使用`long double`表达原x87中间值；当前Linux ABI提供80-bit扩展精度，并在两个原转换点显式toward-zero。测试以`1/3 * 100`锁定结果为33，而不是四舍五入34；caller回归另以`32769/65535 * 65535`锁定较大分数路径。

maximum为零时不增加防护：已有记录先把count夹为零，形成`0/0` NaN；缺键形成`1/0`无穷。`fistp qword`的invalid结果按x87 integer indefinite保留为`0x8000000000000000`，因此AX和最终EAX为零，EDX为`0x80000000`。该行为不能用现代零值早退掩盖。

## 5. 原访问点typed-stop

- 根或next记录`+0x04`键不可访问：保留此前扫描得到的EAX token及入口ECX/EDX。
- 已有记录`+0x06`递增不可访问：键已经命中，但count、比例、scale和后缀均未发生。
- 已有记录`+0x08`scale写不可访问：计数递增/夹限和第一次x87转换已经完成，返回寄存器保留第一次转换的EDX:EAX及ECX count。
- allocator返回零或不可映射token：前驱link已先发布，随后停在新节点`+0x00`。
- 分配记录清零不可访问：保留前驱link和此前完成的dword清零前缀；`+0x04/+0x08`故障时x87 ST(0)阶段为已加载maximum，`+0x0C/+0x10`故障时为已完成`1/maximum`的ratio。

结果以`LegacyBattleFixedCurveX87StackState`显式记录本函数在typed-stop时留下的`empty/maximum/ratio`阶段；`empty`只表示没有本函数自有的待处理值，不断言caller入口物理x87栈为空。正常第二次转换完成后恢复empty。停止点不伪造正常返回，不执行未到达的键、count、scale、根word或第二次转换后缀。

## 6. caller回收

`0x00474FC0`已在原`0x00474FF3`位置删除整函数opaque调用，直接把行动者曲线乘数、最大值和键的低word连同入口三寄存器交给typed helper。行动者motion和共享motion仍先清零；成功后只把helper返回AX写共享motion，再继续方向、mode-one skip、目标刷新、效果计算、累计和发布。

fixed-curve typed-stop保留两次motion清零和helper内部前缀，阻断共享motion发布、方向后缀、目标callee和最终effect-application latch。动作4与特殊动作400的typed组合继续向上发布独立`fixed_curve_typed_stop`，不把故障混同为shared缺失。生产`include/src`除typed closure注释外不再保留`0x00477830`地址调用边界。

## 7. 验证与动态差分

叶函数UT覆盖已有根命中、动态节点、递增后inclusive夹限、word回绕、缺键分配、五字清零、`1/3`双截零、scale高word保留、maximum零x87 indefinite、已有count/scale访问stop，以及分配`+0x08`前maximum阶段和`+0x0C`前ratio阶段。caller测试覆盖fixed owner真实曲线输出、mode-one skip、signed motion累计、9999夹限、负一抑制、caller count访问stop及全部旧地址零调用。

验证：battle聚合定向通过并连续实际执行10次；Linux core`194/194`、AddressSanitizer`194/194`、Linux app`200/200`及changed-range clang-format Werror全部通过。最终日志零源码warning、测试失败、sanitizer finding或runtime error；验证期间未启动原版或OpenSWD3游戏程序。inventory连续双生成逐字节一致，稳定为`269/422 = 259 platform_adapted + 10 assembly_exact + 153 pending_audit`，SHA256为`95514d8b21ac60676fb2395a4deffbff78803f691dfce0d7661cf44c06a0a9e5`。

当前缺少原版固定曲线链、allocator堆、x87 control/status word、行动者曲线字段以及`0x00474FF3`联合寄存器捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合、原位置typed-stop和Linux门禁。
