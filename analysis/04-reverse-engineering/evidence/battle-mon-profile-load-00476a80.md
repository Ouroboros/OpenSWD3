# 战斗MON.DAT资料读取 `0x00476A80`

状态：`platform_adapted`、`unit_tested`、`real_asset_verified`、`caller_reclaimed`。

## 1. 完整LST范围

权威主体为`0x00476A80..0x00476DA6`，从`proc`到`endp`共398行、222条实际指令、12个call、31个跳转、29个局部标签和3个返回点。没有外部`FUNCTION CHUNK`。

函数接收40字节输出资料和16位资料编号。它共享两个原版全局：MON.DAT惰性文件句柄与临时1024字节读取块。`0x00476DB0`也使用同一会话；后续已关闭的`0x004776A0`在战斗全局重置尾部统一关闭共享句柄并清会话门。

原版还通过字符串复制与拼接建立数据目录下的MON.DAT路径，再调用文件open/seek/read、1024字节分配和释放。modern把路径拼接收敛到SDL数据目录适配层，并把其余操作收敛为`LegacyBattleMonDatabasePort`的五种低层请求，没有保留整函数opaque调用。

## 2. 惰性文件会话与二级目录

首次调用在共享句柄尚未打开时，以只读、共享读、open-existing合同打开`MON.DAT`。打开失败保留句柄哨兵`0xFFFFFFFF`并返回EAX零；成功句柄缓存到唯一会话owner。后续调用复用同一句柄，不再次open。

每次成功进入读取路径后固定执行三次seek和三次read：

- seek到文件`+0x204`，读取4字节目录root；
- seek到`0x200 + root + (profile_id & 0xFFFF) * 4`，读取4字节资料相对偏移；
- seek到`0x200 + relative`，分配1024字节并读取固定1024字节资料流。

原版不校验目录root、资料编号、相对偏移、文件长度或实际读取字节数。modern不添加这些业务防护。两个4字节局部缓冲在read前保留旧值；底层短读只覆盖实际返回字节，因此未覆盖高字节继续参与寻址，保持原版陈旧栈字节语义。

1024字节分配返回零时，原版在下一次流访问前已经完成三次seek、两次目录read和一次分配。modern只在该真实访问点形成`stream_zero_typed_stop`，保留此前会话、寄存器线程和调用次数，不伪造第三次read或释放。

## 3. 首tag与解析跳表

第三次read后，函数先无条件读取流首个u16。首tag不为零时立即释放临时块并返回EAX零；输出资料保持原内容。首tag为零时，从流起点进入循环，按u16 tag推进。

完整tag行为如下：

- tag 0：读取连续两个u32到输出`+0x0C/+0x10`，流推进8字节；
- tag 1：只消费tag，不消费payload；
- tag 2：读取首字节到输出`+0x24`，但流推进2字节；
- tag 3：输出`+0x04`按u32或入`0x00000001`；
- tag 4：读取u16到输出`+0x16`；
- tag 5：终止解析；
- tag 6：读取u16到`+0x18`，再把`+0x04`或入`0x00000080`；
- tag 7：读取u16到`+0x18`，不设置该flag；
- tag 8：读取u16到`+0x14`；
- tag 9：`+0x04`或入`0x00000002`；
- tag 10：读取u16到`+0x1E`，再把输出`+0x1F`高位设为1；
- tag 11：`+0x04`或入`0x00000004`；
- tag 12：读取u32到`+0x08`；
- tag 13：读取u16到`+0x1C`；
- tag 14：先把`+0x04`或入`0x00000008`，再读取u16到`+0x1A`；
- tag 15、16、17：分别把`+0x04`或入`0x10/0x20/0x40`；
- tag 18、19、20、21：分别把`+0x04`或入`0x100/0x200/0x400/0x800`；
- tag 22：把`+0x04`或入`0x1000`，读取u16到`+0x20`和下一字节到`+0x24`，总payload推进4字节；
- tag 23：读取u16到`+0x22`；
- tag 24、25：分别把`+0x04`或入`0x2000/0x4000`；
- tag大于25：只消费tag，不消费payload。

函数不清零40字节输出，也不初始化未被tag写入的字段。是否先清零由caller决定。所有flag均读取输出当前u32后执行OR，因此复用scratch且不清零的caller会保留陈旧位。

解析正常遇到tag 5后释放临时块并返回EAX一。流或输出访问越界只在原版实际读写点形成typed-stop；此前字段写入、flag、游标、共享句柄和调用次数全部保留。由于原版会在该点故障，modern不越过故障点释放临时块。

## 4. 寄存器与资源生命周期

低层port请求和reply显式携带EAX、ECX与EDX。文件open失败、首tag非零、分配零、每个tag正常解析、访问typed-stop和最终释放均有独立终端寄存器断言。

正常路径的EAX在释放前发布输出token，释放正常返回后强制返回1；ECX与EDX保留释放callee reply。首tag非零路径也先释放，再强制EAX为0。分配零路径保留原版常量ECX `0x100`和分配reply的EDX。解析typed-stop返回原版故障点前的输出token、流地址与线程EDX，不执行清理后缀。

临时1024字节块只在首tag非零或正常tag 5路径释放一次。open失败和分配零没有块可释放；访问typed-stop不提前释放。文件句柄属于跨调用唯一会话owner，当前读取函数不关闭它；战斗全局重置的数据库关闭函数按双哨兵规则统一关闭。

## 5. 已关闭caller回收

十四个已关闭caller已删除当前整函数的opaque边界并typed直连：十个战斗caller与四个特殊模式caller。尚未审计的`0x00480220`拥有八个调用站点，本轮不修改。

战斗caller覆盖组B动作配置、重配置、组合、profile模式、profile选择、actor列表查询与应用、actor资料准备、资源选择以及组A最终处理。顶层startup、action、script和input port通过虚继承共享同一个MON会话和40字节scratch；旧预制profile回调、profile buffer接口及整函数callee枚举槽均已删除或保留为不可调用reserved值。

四个特殊模式caller保持各自原始输出初始化差异：

- 装备提交先清共享40字节scratch，加载后读取`+0x04 bit0`；
- 装备绘制复用同一scratch且不清零，加载后读取`+0x10`低字；
- 游戏菜单提交先清scratch，加载后读取`+0x04 bit0`；
- guardian属性应用分配40字节临时资料，加载后读取`+0x18 bit15`，并在完成后释放该临时owner。

这些caller只读取LST证明的字段，不把40字节资料现代化为第二套结构，也不增加无合同来源的默认值。失败时保留原版已发生的清零、分配、资料写入、计数和选择前缀，并阻断故障点之后的副作用。

## 6. 真实MON.DAT验证

只读验证使用仓库外原始资产`/mnt/e/Game/swd3/MON.DAT`，文件大小163731字节，目录root为`0x1AEC`。没有启动原版或OpenSWD3游戏程序。

稳定样本profile 0的目录相对偏移为`0x2244`，文件偏移为`0x2444`，tag序列为`0,4,8,13,5`。profile 21的相对偏移为`0x2430`，文件偏移为`0x2630`，tag序列为`0,4,3,8,5`。测试逐字段验证两条资料的输出投影与最终流游标。

同一port连续加载profile 0和21只open一次，共执行6次seek、6次read、2次1024字节分配和2次释放，证明惰性会话与每次读取生命周期都按原合同工作。

## 7. 验证与动态差分

纯函数测试覆盖open失败、句柄缓存、短DWORD陈旧高字节、分配零、首tag非零、tag 0至25、unknown tag、tag 1、tag 5、全部字段和flag投影、寄存器线程、访问typed-stop与释放次数。caller回归覆盖十四个已关闭caller的资料编号、scratch清零差异、字段读取、失败前缀和共享会话。

战斗聚合定向测试、完整core AddressSanitizer `188/188`、Linux core `188/188`和Linux app `194/194`全部通过；最终日志零OpenSWD3源码warning、测试失败和sanitizer finding。ASan曾因战斗聚合测试单函数帧逼近默认8MiB而暴露栈耗尽；一个原本独立的测试块已改为聚合二进制的独立入口，没有禁用sanitizer或缩减断言。

当前缺少原版动态文件句柄、分配token、短读陈旧栈内容、全部caller及callee寄存器联合捕获后端，`original_diff_verified`登记为`blocked_runtime_oracle`。该阻塞不影响完整LST静态闭环、typed访问隔离、真实资产只读验证和跨平台构建验证。
