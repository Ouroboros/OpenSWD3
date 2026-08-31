# 战斗组B行动道具特殊选项加载 `0x00476860`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476860..0x004768CC`，从proc到endp共61行、41条带机器码且含助记符的实际指令、4个call、4个跳转、2个局部标签和3个返回点，没有外部`FUNCTION CHUNK`。四个call由两次待审定义加载器`0x00476DB0`和两次导入`lstrcpyA`组成。

函数是thiscall。入口`ECX`为组Bactor；两个栈参数依次为完整32位selector和文字目标，callee以`retn 8`弹出参数。两个静态caller分别位于已关闭的目标选择进入函数和控制面板逐帧函数；两处都只在完整`EAX == 1`时把该特殊选项计为可用，不是低`AX`比较。

## 2. selector与动态资源读取

函数只接受selector 0和1。入口先对完整`EAX`执行无效果的`sub eax,0`并在零时进入selector 0，非零时再`dec eax`；减一后仍非零的值进入共享失败返回并清零完整`EAX`。因此其他完整32位值均不读取actor或资源。

selector 0读取actor `+0x0C`的动态资源token，再读取资源`+0x72`的word；selector 1读取同一资源token，再读取资源`+0x76`的word。actor缺失只在所选分支首次actor访问点停止；资源token缺失只在所选word的首次真实访问点停止。所选word为零时不调用loader、不复制文字，并经共享失败返回清零完整`EAX`。

两个分支保留不同寄存器路径。selector 0由`EDX`承接资源token，入口selector为完整零，随后只用资源word覆盖`AX`；selector 1由`EAX`承接资源token，再只用资源word覆盖`AX`。因此selector 0传给loader的定义参数是所选word的零扩展值，而selector 1必须保留资源token高word。

## 3. 定义加载器与内嵌资料

两个分支都把actor `+0x10`作为164-byte内嵌行动资料目标，并把各自形成的完整`EAX`作为定义参数调用`0x00476DB0`。该callee仍属后续待审范围，本工作包复用第252项已建立的`LegacyBattleGroupBActionItemOptionPort::load_action_item_definition`窄边界，不重新引入整个函数opaque调用，也不增加frame coordinator调用枚举。

端口正常回复可发布完整164-byte定义到`action_composition.resource_definition`唯一owner。loader typed-stop保留其已经发布的定义字节和返回寄存器，并阻断文字复制。与第252项主选项函数不同，本函数在loader后不重读actor资源，不发布额外dword，也没有对应的输出参数。

## 4. 名称复制与返回寄存器

loader正常返回后，selector 0把文字目标载入`EAX`，selector 1把文字目标载入`ECX`；两者都以actor `+0x10`为源调用`lstrcpyA`。复制继续复用第252项窄端口，并按原导入函数的可观察访问逐字节建模：从定义首byte开始，源和目标每个byte都在访问前检查；复制包括首个NUL。任一侧在首次越界时立即`name_copy_typed_stop`，保留此前已经复制的前缀，不静默截断，也不补写额外终止符。

导入边界typed-stop保留loader及内嵌定义副作用。正常复制后原函数无条件覆盖完整`EAX=1`并返回；非法selector和零定义word覆盖完整`EAX=0`。`ECX`与`EDX`保留到各真实边界为止，测试分别锁定selector 0的资源`EDX`、selector 1的loader前陈旧`EDX`、两个分支的文字目标寄存器顺序和导入返回寄存器。

## 5. typed owner与两个caller回收

组Bactor、动态资源token/164-byte资源和内嵌定义继续借用`LegacyBattleStartupState::group_b_lifecycle`中的八槽唯一owner。共享文字目标继续使用既有选择文字workspace。定义加载和文字复制复用`LegacyBattleGroupBActionItemOptionPort`；没有建立平行资源、定义或文字owner。

目标选择进入函数的两次旧secondary opaque扫描已全部替换为typed直连，原枚举数值保留为`reserved_target_selection_scan_secondary_slot`且生产零调用。每次完整`EAX=1`才增加特殊选项可用计数；actor/resource停止映射到既有组Bactor停止，loader或复制停止映射到新增secondary选项停止并阻断余下扫描与阶段后缀。三次主选项扫描、两次secondary扫描、扫描顺序和最终transition清零保持不变。

控制面板逐帧函数的两次旧special opaque查询也已替换为同一typed函数，原槽保留为`reserved_query_special_option_slot`且生产零调用；frame coordinator对应数值同样保留为`reserved_control_panel_query_special_option_slot`。每次完整`EAX=1`才绘制格式化特殊行；actor/resource停止映射到既有组Bactor停止，loader或复制停止映射到新增special选项停止。此前标题、攻击行和主选项前缀保留，余下特殊行及面板后缀被阻断。

## 6. 验证与动态阻塞

纯函数测试覆盖非法selector、两个actor首次故障、两个资源首次故障、两个零定义路径、两类loader ABI、selector 1资源高word、loader payload/typed-stop、导入typed-stop、逐字节目标前缀、源定义无NUL和正常完整返回。

目标选择与控制面板集成测试覆盖唯一owner、两处动态定义word、两个完整`EAX`成功门、旧opaque槽生产零调用、定义/文字窄端口透传及typed-stop后缀阻断。战斗聚合测试、完整core AddressSanitizer`188/188`、Linux core`188/188`和Linux app`194/194`全部通过；最终日志无OpenSWD3源码warning、测试失败或sanitizer finding。新增文件全量及历史文件本包触碰行均通过clang-format Werror门。

当前缺少原版组Bactor、动态资源两处定义word、`0x00476DB0`真实定义加载副作用、内嵌定义名称、`lstrcpyA`逐字节访问、共享文字目标和两个caller寄存器的联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。该限制不影响完整静态闭环和Linux验证。
