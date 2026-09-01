# 战斗固定键曲线计数设置 `0x00477920`

状态：`platform_adapted`、`unit_tested`、`callers_reclaimed`。

## 1. 完整权威范围与调用图

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477920..0x004779EF`，从proc到endp共97个物理行、67条实际指令、3个call、5个跳转、5个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。

三个call由20字节分配包装器`0x00487C10`一次和x87截零转换helper`0x00489654`两次组成；已有记录与缺键路径各调用转换helper一次。两个物理caller都在已关闭的共享角色、道具调试对话`0x0040F890`，callsite为新增命令路径`0x0040FC2B`和直接修改路径`0x0040FEA7`。

函数采用cdecl四参数ABI：根token、键word、最大值dword和待设置count dword。后三项只在比较、记录和浮点计算中使用低word；入口保存EBX/ESI/EDI，DI保存键，EBX保存根，ESI保存当前记录。

## 2. 根匹配、扫描与已有记录

固定根不是纯哨兵。函数先比较`word [root+4]`；不匹配时严格从当前记录`+0x00`读取next到EAX，next非零则切换ESI并比较新记录`+0x04`，next为零才分配。没有现代长度上限、环检测、排序或nil替代值。

已有根或动态节点命中后按原顺序执行：

1. 只替换ECX低word为输入count，保留入口ECX高word；EAX完整加载maximum参数。
2. 先按原始count写`word [record+6]`。
3. 以无符号word比较count和maximum；`count >= maximum`时再次写maximum，因此inclusive相等也产生第二次写。
4. 清完整ECX，EAX只保留maximum低word，再把最终记录count读入CX。
5. x87计算`count / maximum * 100.0f`，经`0x00489654`toward-zero转signed i64。
6. 只把返回AX写入`word [record+8]`，保留`+0x0A`高word；正常返回EDX:EAX为转换结果，ECX为最终count零扩展。

输入count不递增，不使用固定数量20限制；它与maximum都保持word截断和无符号比较。

## 3. 缺键分配与副作用顺序

缺键时终端next已把EAX置零，再以固定大小20调用`0x00487C10`。callee返回后采用EAX token和ECX陈旧值，完整清零EDX。原顺序为：

1. 把token先写入前驱`+0x00`。
2. 清新节点`+0x00`。
3. 只替换allocator返回ECX低word为输入count，保留其高word。
4. 依次清`+0x04`、`+0x08`、`+0x0C`、`+0x10`；`+0x04`之后才平衡allocator参数栈，不改变通用寄存器。
5. 从前驱link重取新节点，完整加载maximum到EAX并先完成unsigned比较。
6. 先写键word，再写原count word；`count >= maximum`时再次把count写为maximum。
7. 清ECX、截EAX并计算百分比，写scale低word。
8. 最后word递增根`+0x04`，保留`0xFFFF→0`回绕，再返回。

allocator token、固定根`0x004ACBA8`及动态20字节节点继续由唯一`LegacyBattleFixedObjectStatePort`持有，并复用`LegacyBattleFixedCountAllocationPort`，没有建立第二条物理链。

## 4. x87特殊值与转换

两个路径都只保留一个函数自有x87值：`fild count`后以maximum做`fidiv`，再直接乘单精度`100.0f`，转换helper弹出该值。实现使用`long double`保留80-bit中间精度，并在原转换点显式toward-zero。

maximum为零时不早退。由于unsigned `count >= 0`恒成立，记录先按原顺序写输入count再写零，随后形成`0/0` NaN；`fistp qword` invalid结果保留为integer indefinite `0x8000000000000000`，因此EAX/AX为零，EDX为`0x80000000`。

## 5. 原访问点typed-stop

- 根或next记录`+0x04`键不可访问：保留此前扫描EAX和当时ECX/EDX。
- 已有记录`+0x06`首写不可访问：EAX已完整加载maximum，ECX只替换了count低word，记录尚未修改。
- 已有记录`+0x08`scale写不可访问：原count写、inclusive夹限和x87转换均已完成，返回寄存器保留转换EDX:EAX与最终count ECX。
- allocator返回零或不可映射token：前驱link已发布，EDX已清零，停在新节点`+0x00`。
- 分配记录清零不可访问：保留link、此前清零前缀及`+0x00`之后替换count低word的allocator ECX高字。
- 分配记录键/count写不可访问：五个dword已经清零，EAX已加载maximum，ECX保留allocator高字和输入count低word；未执行百分比与根递增。

停止点不伪造正常返回，也不执行未到达的scale、根word和后缀。

## 6. Dialog caller回收

Dialog第一分类条件先以记录flags和mask完成`and`及bit15清除；命中后用记录`+0x50`word替换EDX低word，同时保留masked flags高word。typed组合把记录word作为键、解析后的命令ID作为maximum、附加值作为count，并把原callsite的EAX/ECX/EDX完整前缀传给helper。

原`update_first_item_category`opaque端口已从接口、实现与测试fixture删除。新增与直接修改两条命令都直接组合typed helper；curve stop保留此前库存修改，阻断第二、第三分类、页面刷新、编辑框清理和scratch释放，并以独立`fixed_curve_typed_stop`区别后续固定数量链故障。第二分类`0x00477A20`仍为后续待审边界，不提前修改。

## 7. 验证与动态差分

叶函数UT覆盖已有根、动态节点二次命中、先写后inclusive夹限、缺键五dword清零、根word回绕、百分比截零、scale高word、maximum零integer indefinite、已有count/scale访问stop及分配清零时allocator ECX高字。Dialog回归覆盖新增与直接修改两个物理caller、真实共享owner上的curve/count双链、完整masked EDX高字、curve stop与后续fixed-count stop的不同前缀。

最终Linux core`194/194`、AddressSanitizer`194/194`与Linux app`200/200`全部通过，battle/special_modes两项聚合测试随完整core连续复跑10轮均通过；最终日志无OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。`99471b16`代码范围通过changed-range clang-format与diff检查，inventory连续双生成逐字节稳定；验证期间未启动原版或OpenSWD3游戏程序。

当前缺少原版固定曲线链、allocator堆、x87 control/status word、Dialog记录/局部槽及两个callsite联合寄存器捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合、原位置typed-stop和Linux门禁。
