# 战斗MON.DAT定义读取 `0x00476DB0`

状态：`platform_adapted`、`unit_tested`、`real_asset_verified`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476DB0..0x0047720F`，从`proc`到`endp`共586个物理行、335条实际指令、15个call、52个跳转、42个局部标签和3个返回点。没有外部`FUNCTION CHUNK`；下一过程为`0x00477290`。

函数接收固定`0xA4`字节输出定义区和32位逻辑definition参数。它与`0x00476A80`共享MON.DAT惰性文件句柄、1024字节临时流合同和唯一typed port，不建立第二套文件状态。modern把Win32路径、文件、堆块大小查询、分配和释放隔离为`LegacyBattleMonDatabasePort`的追加低层请求；已有枚举值不移动，退役上层调用只保留reserved槽或兼容别名。

## 2. 旧说明释放、输出清零与目录读取

入口先读取输出`+0xA0`的旧说明token。token非零时严格依次执行块大小查询、从共享说明字节计数减去查询值、释放旧块、清零`+0xA0..+0xA3`并丢弃对应typed owner。减法保持32位回绕，不新增下溢保护。随后函数无条件只清零输出前`0xA4`字节；更长caller对象的后缀不得被触碰。

文件会话尚未打开时，以只读、共享读、open-existing合同打开`MON.DAT`；失败保留句柄哨兵并返回EAX零。成功后每次固定执行三次seek和三次read：

- 绝对seek到`+0x204`并读取4字节目录probe；该值只保留读取副作用，不参与后续寻址；
- 原版`0x00476E9F`先把逻辑definition参数与`0xFFFF`，再从当前文件位置相对seek `low16_id * 4 - 4`并读取4字节相对偏移；
- 绝对seek到`0x200 + relative`，分配1024字节并读取固定1024字节定义流。

两个目录DWORD都在read前装入caller提供的陈旧值；短读仅覆盖实际返回字节，未覆盖高字节继续参与寻址。目录probe、相对偏移、文件长度和实际读取量均不做现代化校验。1024字节分配返回零时，typed-stop位于原版首次流访问之前，保留此前旧说明释放、输出清零、seek/read和寄存器副作用，不伪造第三次read或释放。

第三次read后先无条件读取流首u16。首tag不是1000时释放临时流并返回EAX零；输出保持已经清零的前缀。首tag为1000时从流首重新进入解析循环。

## 3. tag跳表与字段投影

循环先用`mov ax,[cursor]`取tag并推进2字节；tag 5直接结束，因此保留EAX高16位。其余tag执行`and eax,0xFFFF`后进入跳表。完整已实现分支为：

- tag 1：从流复制`0x4D`字节到输出`+0x50`，逐字节保持首次越界故障点；
- tag 6..22：各读一个u16，依次写输出`+0x40,+0x24,+0x26,+0x2C,+0x32,+0x46,+0x42,+0x44,+0x50,+0x28,+0x2A,+0x2E,+0x30,+0x34,+0x36,+0x38,+0x48`；每个分支按LST分别只改AX、CX或DX；
- tag 25：跳过2字节后读取u32到输出`+0x20`并令EDX为该值，总payload推进6字节；
- tag 26、27、100：分别读取u16到`+0x3A,+0x3C,+0x3E`；
- tag 28、29：分别读取u8到`+0x9B,+0x9C`；
- tag 30：扫描动态说明并维护输出`+0xA0`token；
- tag 1000：扫描名称并复制到输出起点；
- tag 2000：复制9字节到`+0x92..+0x9A`，再写`+0x9B,+0x9C,+0x54,+0x52`，总payload推进15字节；当逻辑definition低16位为`0x0126`时保留原版参数收窄副作用；
- tag 0、2、3、4、23、24及所有其他值：只消费tag，不消费payload。

名称和说明都最多从当前游标检查`0xFF`个起点上的`$$`。名称找到终止符时复制正文、不补NUL并推进`length+2`；未找到时原版保留EAX=`0xFF`并以`eax+cursor+2`推进`0x101`。说明未找到时则只按循环后的指针推进`0xFF`，不分配也不改输出token。两条畸形路径不同，不能合并。

## 4. 动态说明与寄存器线程

tag 30找到终止符后以`length+1`请求说明块，把返回token先写入输出`+0xA0`，再到原版首次memset目标访问点判断零token。零token typed-stop保留已写零token、EAX零、EDX为分配长度；ECX保持原版`rep stosd`/`rep stosb`入口状态：长度小于4时为余数字节数，因此一字节空说明固定为ECX一。

成功时typed owner建立`length+1`字节缓冲，末字节为零；共享说明字节计数按u32累加，EAX为新总计数、ECX为块token、EDX为累加前计数。正常tag 5路径释放1024字节临时流并强制返回EAX一；首tag失败路径释放后强制EAX零。解析输出或流访问typed-stop不提前释放临时流，不越过原版故障点。

## 5. caller回收与待审隔离

权威LST共有45个直接调用站点，分布于30个过程。随`0x00477A20`工作包回收后，27个已关闭过程共38个站点已完成回收：

- 战斗caller：`0x004539B0`、`0x00455D60`、`0x0045D180`、`0x00468C80`、`0x00468FF0`、`0x0046E890`、`0x0046E9C0`、`0x004707B0`、`0x00475720`、`0x00475820`、`0x00476160`、`0x00476600`、`0x00476780`、`0x00476860`、`0x00477400`、`0x00477A20`；
- 先前模块已关闭caller：`0x00402F80`、`0x004070A0`、`0x0040B7F0`、`0x0043C0D0`、`0x0043C9C0`、`0x0043CEF0`、`0x0043D050`、`0x0043D530`、`0x0043F1E0`、`0x0044D0F0`、`0x0044D2D0`。

适用的战斗、物品数量、growth、group A/B、message/startup/frame和特殊模式路径全部只调用低层MON typed loader。旧whole-function虚函数和生产兼容桥已删除；测试流转换只存在于`tests/support/legacy_battle_mon_database_fixture.hpp`。`0x00402F80`对应原版隐藏调试物品遍历仍维持其既有、已记录的SDL debug-only平台跳过，不伪造不存在的旧物品链。

`0x00477400`现于LEVEL记录要求新增队伍物品时，以新节点 `+0x0C` 和物品ID直连本loader；MON typed-stop保留已经链接并清零的新节点，正常返回后才写过渡模式与成长标题。`0x00477A20`现以命令ID读取MON定义，并在定义说明清理后更新固定根`0x004B8A00`；新增与直接修改两个Dialog站点均已直连loader及共享MON port。其余三个尚未审计过程的7个站点继续隔离：`0x00477B40`、`0x00477BD0`、`0x00480220`；reserved枚举槽不构成提前回收。

连续物理定义必须重建完整`0xA4`镜像。物品节点、growth actor和growth item result因此在调用内组合固定字节与`+0xA0`说明token，再分别回写；special-mode数量记录使用调用内说明owner并保留记录中的释放token。共享actor/profile scratch和动态说明各只有一个typed owner。

## 6. 真实MON.DAT验证

只读样本为仓库外`/mnt/e/Game/swd3/MON.DAT`，大小163731字节；没有启动原版或OpenSWD3游戏程序。

稳定definition 1的目录槽为`0x208`，相对偏移`0x716E`，文件偏移`0x736E`，tag序列为`1000,1,25,6,26,30,5`，最终游标109；其名称为4字节CP950数据，说明为空字符串并实际执行一字节说明分配。definition `0x126`的目录槽为`0x69C`，相对偏移`0xF020`，文件偏移`0xF220`，tag序列为`1000,2000,14,25,6,26,30,5`，最终游标93。

同一port和同一输出owner连续加载两条记录只open一次，共6次seek、6次read、2次1024字节分配和2次释放；第二次加载先查询并释放第一条的一字节说明，再分配39字节新说明。测试同时验证低16位目录索引、相对seek、名称、最终token、说明NUL和共享说明计数。

## 7. 验证与动态差分

独立definition测试覆盖open失败、共享句柄、短目录read陈旧字节、低16位索引、1024字节分配零、首tag失败、tag 1、6..22、25..30、100、1000、2000、default、全部字段偏移、寄存器线程、说明旧块释放、空说明一字节分配、两种无终止符推进、输出/流访问typed-stop和三个返回路径。caller回归现覆盖36个已关闭站点对应的生产投影、失败前缀、说明生命周期和reserved槽稳定性；`0x00477400`另有真实LEVEL物品1501到真实MON名称的联合回归。

最终Linux core为`189/189`，完整AddressSanitizer为`189/189`，Linux app为`195/195`；日志均为零OpenSWD3源码warning、零测试失败和零sanitizer finding。触碰行clang-format门禁与`git diff --check`通过。

当前缺少原版文件句柄、相对seek返回值、堆token、陈旧短读、说明块计数、全部caller及callee寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整LST静态闭环、typed故障隔离、真实资产只读验证和Linux构建验证。
