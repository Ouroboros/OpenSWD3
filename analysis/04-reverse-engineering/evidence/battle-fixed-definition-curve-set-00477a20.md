# 战斗定义驱动固定曲线设置 `0x00477A20`

状态：`platform_adapted`、`unit_tested`、`callers_reclaimed`。

## 1. 完整权威范围与调用图

唯一行为真值为`swd3.exe.lst`。完整主体为`0x00477A20..0x00477B30`，从proc到endp共127个物理行、82条实际指令、5个call、7个跳转、7个局部标签和2个返回点，没有外部`FUNCTION CHUNK`。

五个call依次为MON定义读取`0x00476DB0`、定义说明清理`0x00478220`、已有记录百分比截零`0x00489654`、20字节分配包装器`0x00487C10`和缺键记录百分比截零`0x00489654`。两个物理caller都在已关闭的共享角色、道具调试对话`0x0040F890`，callsite为新增命令路径`0x0040FC58`和直接修改路径`0x0040FED4`。

函数采用cdecl三参数ABI：根token、定义/记录键dword和待设置count dword。DI保存键低word，EBX保存根，ESI保存当前记录；完整键dword仍传给MON定义读取，count只在记录和百分比路径使用低word。

## 2. 初始记录选择与MON生命周期

入口先读根`word [root+4]`。该word为零时ESI保留根本身；非零时立即读`[root+0]`，把ESI切换到首个动态节点。此初始根count/link访问发生在MON读取之前。

随后以固定scratch `0x0053CF50`和完整键dword调用`0x00476DB0`。只要MON loader正常返回，包括文件打开失败或首tag不匹配导致的EAX零返回，原函数都继续调用`0x00478220`并进入链扫描；只有loader内部原访问点typed-stop才阻断后缀。清理读取scratch `+0xA0`：token非零时释放说明块并清零该dword，token为零时EAX变零。该清理不减少MON loader维护的说明累计字节数，保留原版跨调用陈旧计数行为。

定义scratch中的`word [0x0053CF50+0x44]`是后续maximum。函数不缓存另一份MON定义，不建立第二个文件会话或说明owner。

## 3. 扫描、命中锁位与已有记录更新

完成定义清理后，先比较DI和当前记录`word [esi+4]`。不匹配时严格循环读取当前记录`+0x00`到EAX；next为零才分配，非零则切换ESI并比较新记录`+0x04`。没有现代长度上限、环检测、排序或nil替代值。

根count为零时根本身参加首次键比较；根count非零时根充当链头，扫描从其`+0x00`首节点开始。该条件化根语义不得替换成统一哨兵或统一首记录模型。

已有记录命中后先读`word [record+0x0A]`。该锁word非零时立即返回EAX一，不写count、不读maximum、不执行x87转换；ECX/EDX保留MON清理后的残值。锁word为零时严格执行：

1. 只替换ECX低word为输入count，保留清理callee返回的ECX高word。
2. 先把输入count写到`word [record+6]`。
3. 完整读取scratch `+0x44`到EAX，以低word做无符号比较；`count >= maximum`时再次把maximum写入`+6`，因此相等也产生第二次写。
4. 清完整ECX，EAX只保留maximum低word，再把最终count读入CX。
5. x87计算`count / maximum * 100.0f`，经`0x00489654`以toward-zero转signed i64。
6. 只把返回AX写入`word [record+8]`，保留`+0x0A`锁word；最终强制EAX一，ECX为最终count零扩展，EDX为转换高dword。

maximum为零时不早退。inclusive夹限先把count写零，再形成`0/0` NaN；`fistp qword` invalid结果为integer indefinite `0x8000000000000000`，所以最终EAX仍被覆盖为一、ECX为零、EDX为`0x80000000`。

## 4. 缺键分配与写入顺序

终端next读取已把EAX置零，再以固定大小`0x14`调用`0x00487C10`。callee返回后立即清完整EDX，并按原顺序：

1. 把allocation token先写入终端前驱`+0x00`。
2. 清新节点`+0x00`。
3. 只替换allocator返回ECX低word为输入count，保留其高word。
4. 依次清新节点`+0x04`、`+0x08`、`+0x0C`、`+0x10`。
5. 从前驱`+0x00`重取新节点，写键低word到`+0x04`，写原count低word到`+0x06`。
6. 读取定义maximum，执行与已有记录相同的inclusive夹限、x87百分比和`+0x08`scale写。
7. 最后递增根`word [root+4]`，保留`0xFFFF→0`回绕，再返回EAX一。

allocator token、固定根`0x004B8A00`及动态20字节节点继续由唯一`LegacyBattleFixedObjectStatePort`持有，并复用相邻固定数量/曲线函数的`LegacyBattleFixedCountAllocationPort`。

## 5. 原访问点typed-stop

- 根`+0x04`count不可访问：在MON读取前停止，保留入口EAX/ECX/EDX。
- 非空根`+0x00`首link不可访问：在MON读取前停止。
- MON loader内部输出、流、说明分配访问停止：保留loader前缀，不调用`0x00478220`。
- 正常MON返回后当前token为零、未映射或`+0x04`键不可访问：先完成定义清理，再停在键读取点。
- 当前记录`+0x00`next、`+0x0A`锁、`+0x06`count或`+0x08`scale不可访问：保留全部已到达的扫描、写入、夹限和x87前缀。
- allocator返回零或不可映射token：前驱link已发布且EDX已清零，停在新节点`+0x00`。
- 分配记录清零不可访问：保留link、此前清零dword和`+0x00`之后替换count低word的allocator ECX高字。
- 分配后的键/count/scale访问不可用：五个dword清零及此前写入均保留，未到达的根count递增不执行。

typed-stop不伪造成功EAX一，也不执行未到达的页面刷新、编辑框清理或scratch释放。

## 6. Dialog caller回收

两个caller都先从道具记录`+0x2C`加载flags，与第二分类mask做`and`，再清结果bit15并比较mask。命中后把解析后的命令ID作为完整definition/record键，把附加值作为count，以根`0x004B8A00`直连本typed helper。callsite入口EAX为附加值、ECX为命令ID、EDX为masked flags，三者完整前缀均显式传入。

原`update_second_item_category`opaque端口及测试override已删除。新增与直接修改两条命令都复用同一MON port、同一固定对象owner和同一分配端口；helper typed-stop映射到独立`fixed_definition_curve_typed_stop`，保留此前库存与第一分类副作用，并阻断第三分类、低ID重复设置、页面刷新、编辑框清理和命令scratch释放。

## 7. 验证与动态差分

叶函数回归覆盖零count根命中、动态节点锁位、缺键分配与五dword清零、低word键/count截断、inclusive夹限、百分比截零、maximum零integer indefinite、说明分配/释放及累计字节残留、MON正常打开失败后继续、loader typed-stop，以及根、链、分配记录各原访问停止点。Dialog回归覆盖新增与直接修改两个物理caller、真实MON fixture、共享根`0x004B8A00`、第二分类typed-stop和与前后分类的严格顺序。

最终定向测试`2/2`、Linux core`194/194`、AddressSanitizer`194/194`、Linux app`200/200`、连续10轮完整core、changed-range clang-format和release审计全部通过；最终日志零OpenSWD3源码warning、测试失败、sanitizer finding或runtime error。生成器连续双跑逐字节一致，工作包为`272/422 = 262 platform_adapted + 10 assembly_exact + 150 pending_audit`，SHA256为`9065cdef0726c810d287228c25a22635f06eab4db80d96dd105b62db667e0c46`。验证期间未启动原版或OpenSWD3游戏程序。

当前缺少原版MON文件/说明堆、固定定义曲线链、allocator、x87 control/status word、Dialog记录/局部槽及两个callsite联合寄存器捕获后端，动态差分登记为`blocked_runtime_oracle`；这不阻止完整LST静态闭合、原位置typed-stop和Linux门禁。
