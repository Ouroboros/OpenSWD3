# 战斗组B行动配置 `0x00475720`

状态：`platform_adapted`。完整LST、typed状态、两个caller、定向/ASan/Linux验证和inventory双生成收敛后关闭。

## 1. 完整权威范围与ABI

权威LST主体为`0x00475720..0x00475813`，从proc到endp共108行、70条实际指令、3个call、4个跳转、4个局部标签和1个返回点，没有外部`FUNCTION CHUNK`。函数是thiscall并由callee以`retn 8`弹出两个参数：`arg_0`是32-byte行动记录，`arg_4`是资源定义参数。两个直接caller为startup `0x00451FE9`和对手动作15 `0x00455F07`。

入口先保存actor与source，再令EAX为actor`+0x0D50`、ECX为8，执行第一次`rep movsd`；随后从actor`+0x0D50`再复制8个dword到actor`+0x0D70`。因此source缺失只能在第一次dword读取点停止，actor缺失只能在第一次目标写入点停止；此前prologue、EAX、ECX、EDX和参数snapshot必须保留。记录精确布局为32 bytes：`+0x14` action、`+0x16`保留word、`+0x18/+0x1A`坐标、`+0x1C` runtime dword。

## 2. 资源加载与调整

两次复制后先把actor`+0x26B4`清零，再调用`0x00476DB0`。调用边界为：

- 栈参数依次形成资源token与定义参数；
- EAX保持定义参数；
- ECX为actor`+0x0C`资源token；
- EDX保持source记录token。

callee返回后重新读取actor`+0x0C`，首次真实资源访问是`resource+0x20`。资源token为零时严格停在该byte读取点：两份记录复制、`+0x26B4=0`和资源加载callee副作用已发生，后续资源字段、profile与释放均不执行。

资源`+0x20` bit5置位时，按原word宽度依次令`+0x5A += 6`、`+0x56 += 10`；两项都按u16回绕，不夹值。bit5未置位时两项保持原值。

## 3. 行动字段与profile ABI

资源可用时按固定顺序：

1. source`+0x1C`完整dword写actor`+0x2AA0`；
2. 资源`+0x64`按i16符号扩展为i32，写回资源`+0x4C`；
3. 资源`+0x90`低byte写actor`+0x2A93`；
4. 资源`+0x50`低word写actor`+0x2A0C`；若该word为零，则以source`+0x14`低word替代；
5. 资源`+0x60`低word覆盖EDX低16位，保留source runtime高16位；
6. actor`+0x0D90`送入EAX与第一个栈参数，调用`0x00476A80`。

profile调用的完整寄存器合同为：EAX=`actor+0x0D90`，ECX=`sign_extend(resource[+0x64])`的高16位与最终action低16位组合，EDX=source runtime高16位与资源`+0x60`低16位组合。modern端口允许callee发布精确40-byte profile buffer；typed-stop保留此前全部资源写和寄存器snapshot。

profile返回后以资源token覆盖ECX并调用`0x00478220`；EAX和EDX继承profile callee。释放callee typed-stop在该调用边界停止，不执行两个action特例。

## 4. 两个action特例与终端寄存器

释放callee正常返回后依次比较最终action：

- action `0x001C`：EDX覆盖为资源token，EAX写`0x0000A028`，actor`+0x26B4`与资源`+0x4C`同步写该值；ECX保持释放callee返回值；
- action `0x002E`：ECX覆盖为资源token，EAX写`0x0001D4C0`，actor`+0x26B4`与资源`+0x4C`同步写该值；EDX保持释放callee返回值；
- 其他action：EAX/ECX/EDX完整继承`0x00478220`。

两个比较按原顺序独立执行，但同一u16 action不可能同时命中。函数不返回逻辑成功值，不现代化为布尔合同。

## 5. typed owner与两个caller回收

固定全局记录表`0x005213A0 + index*0x20`不再在startup与动作分派各保留副本；它由惰性堆分配的八槽`LegacyBattleActorGroupBElementState::action_record`唯一持有。相同槽同时唯一持有actor token、资源token、164-byte资源块、两份配置记录、40-byte profile和配置字段。全局reset只清行动记录与配置状态，不误释放静态actor资源owner；SDL坐标桥也读写同一记录。

startup按LST先reset actor、清41 dword scratch，再只写记录`+0x14/+0x18/+0x1A/+0x1C`，保留未知20-byte前缀与`+0x16`。mirror严格等于一时先调用mode callee，再以u16执行`640-X`，定义参数只用`mov cx`覆盖低word并保留callee ECX高word。随后直接调用typed实现；旧整函数opaque枚举槽保留为reserved且零调用。

对手动作15按当前group-B count选择相同八槽，保留记录前缀，写special action、固定X、220/350的Y和runtime零；scratch更新后只覆盖EAX低word为special action，完整EAX作为定义参数。typed配置完成后才执行pop mode、group-B count递增与三个stage；任一资源/profile/释放typed-stop都阻断该wave完成尾。第九项仍在首次组B对象访问点停止，保留前八项完整副作用。

三个内部callee尚未审计，分别以资源加载、profile加载和资源文本释放三个窄typed端口隔离；不重新引入整个`0x00475720` opaque调用。

## 6. 验证与动态阻塞

纯函数回归覆盖source与资源首次访问stop、两次32-byte复制、未知前缀、u16加法回绕、i16符号扩展、资源action与source fallback、三callee完整ABI、profile/释放typed-stop、普通终端寄存器及action `0x001C/0x002E`非对称终端。startup回归覆盖共享记录、mirror低word、定义参数陈旧高word、三callee直连、旧opaque零调用与资源加载stop；对手动作15回归覆盖共享前缀、双wave坐标、callee EAX高word、profile stop、旧opaque零调用和第九项停止。定向CTest `1/1`、AddressSanitizer/UndefinedBehaviorSanitizer定向CTest `1/1`、Linux core `188/188`和Linux app `194/194`全部通过；四份构建日志与四份测试日志均无编译warning、sanitizer finding、runtime error或失败项。

原版八个组B完整对象、记录表、动态资源字节、profile buffer、三个callee副作用、startup与动作15的scratch及两个caller寄存器缺少联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。
