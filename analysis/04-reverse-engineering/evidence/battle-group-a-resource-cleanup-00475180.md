# 战斗group-A行动者双资源清理 `0x00475180`

状态：`platform_adapted`。本页以完整`swd3.exe.lst`机器码与指令为行为真值；反编译、命名和未关闭callee仅用于导航。

## 静态审计

权威主体为`0x00475180..0x004751B6`，proc至endp共31行、24条实际指令、2个call、2个跳转、2个局部标签、1个返回点，没有外部`FUNCTION CHUNK`。两个直接caller分别为runtime shutdown内的`0x0045EA42`和group-A元素析构内的`0x0046E4F5`；两个call都指向尚未审计的释放器`0x004885A0`。

函数先读取行动者`+0x2BC4` dword。非零时把该token同时作为栈参数和EAX调用释放器，callee返回后才把字段清零；零值不调用。随后无条件读取行动者`+0` dword并覆盖EAX，以相同规则可选释放并在callee成功后清零。顺序固定为`+0x2BC4`再`+0`，不能合并、倒序或先清字段。

若第一token非零而第二token为零，第二次字段读取把EAX覆盖为零，但ECX/EDX仍保留第一次callee返回；若两者都为零，最终EAX为零而ECX/EDX保持入口状态。第二token非零时最终EAX/ECX/EDX来自第二次callee。释放器抛出或失败时当前字段尚未清零，后续字段也不得提前处理。

## Typed实现与caller回收

新增`LegacyBattleGroupAResourceCleanupState`、窄`LegacyBattleGroupAResourceReleasePort`和`release_legacy_battle_group_a_resources`。唯一typed owner保存`+0` primary token与`+0x2BC4` secondary token；helper精确保留字段读取、零门、固定`0x004885A0` token、callee前寄存器、callee后清零和最终陈旧寄存器。typed owner或legacy行动者token缺失时在首个原版字段访问点停止，不调用释放器。

runtime shutdown固定十个group-A对象改为直接调用typed helper，并把每槽两个token owner并入既有startup party状态；无论token是否为空，十次cleanup仍全部执行。每个helper的EAX/EDX继续传给下一对象，ECX按原版由下一对象token覆盖；最后一个group-A结果继续传入尚未关闭的group-B析构循环。旧`release_group_a_object`整函数opaque槽收窄为只承载`0x004885A0`资源释放，枚举位置保持稳定。

group-A元素析构的正常与SEH展开路径也回收整函数opaque边界。正常路径执行typed双资源清理后再调用基础析构；primary资源释放后清除宿主description bytes。释放端口抛出时当前token保持、此前已完成的token清零仍保留，并由既有catch路径调用一次基础析构后原样重抛；资源typed-stop也先执行基础析构，再向外层传播停止状态。

## 测试与oracle

独立单元测试覆盖双非零顺序、字段offset、固定callee token、第一次callee陈旧寄存器传入第二次、secondary-only最终EAX归零、primary-only、双零零调用、typed owner缺失、零legacy token、第一释放异常与第二释放异常的部分副作用。runtime shutdown回归覆盖十槽双token共二十次窄释放、每槽secondary→primary顺序、全部清零、空token十次cleanup不省略、group-B八槽位置和尾寄存器；元素析构回归覆盖正常typed清理、description失效和SEH异常顺序。

动态差分登记为`blocked_runtime_oracle`：当前缺少原版十个group-A完整对象、两字段真实分配、`0x004885A0` allocator副作用、runtime shutdown循环寄存器、group-A元素SEH展开及基础析构联合捕获后端。该阻塞不影响完整LST静态闭环、typed实现和Linux验证。

定向battle测试为`1/1`，AddressSanitizer为`1/1`且无AddressSanitizer或LeakSanitizer finding，Linux core为`188/188`，Linux app为`194/194`，全部构建日志零warning。inventory连续双生成逐字节一致，稳定为`234/422 = 225 platform_adapted + 9 assembly_exact + 188 pending_audit`，SHA256为`d8423e666b9ead87b6f72550559d6404920f3c8f81f9644a18e25e0b5430a17e`。
