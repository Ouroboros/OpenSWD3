# 战斗组B行动道具选项加载 `0x00476780`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476780..0x0047685D`，从proc到endp共110行、80条带机器码且含助记符的实际指令、6个call、6个跳转、3个局部标签和4个返回点，没有外部`FUNCTION CHUNK`。六个call由三次待审定义加载器`0x00476DB0`和三次导入`lstrcpyA`组成。

函数是thiscall。入口`ECX`为组Bactor；三个栈参数依次为完整32位selector、文字目标和输出dword目标，callee以`retn 0x0C`弹出参数。两个静态caller分别位于已关闭的目标选择进入函数和控制面板逐帧函数；两处都只在完整`EAX == 1`时把该主选项计为可用，不是低`AX`比较。

## 2. selector与首次资源读取

函数只接受selector 0、1、2。入口对selector依次执行减零、两次`dec`和条件跳转；其他完整32位值进入共享失败返回并清零完整`EAX`。

三个有效selector都先读取actor `+0x0C`的动态资源token，再分别读取资源`+0x66`、`+0x6A`或`+0x6E`的word。actor缺失只在首次actor访问点停止；资源token缺失只在所选word的首次真实访问点停止。所选word为零时不调用loader、不复制文字，并经共享失败返回清零完整`EAX`。

selector 0和1只用所选word覆盖`AX`，因此loader定义参数的高word沿用入口selector的高word；有效selector使该高word为零。selector 2先把资源token载入完整`EAX`，再只用资源`+0x6E`覆盖`AX`，故传给loader的定义参数必须保留资源token高word。

## 3. 定义加载器与内嵌资料

三个分支都把actor `+0x10`作为164-byte内嵌行动资料目标，并把各自形成的完整`EAX`作为定义参数调用`0x00476DB0`。该callee仍属后续待审范围，本工作包复用`LegacyBattleGroupBActionItemSelectionPort::load_action_item_definition`窄边界，不重新引入整个函数opaque调用。

端口正常回复可发布完整164-byte定义到`action_composition.resource_definition`唯一owner。loader typed-stop保留其已经发布的字节和返回寄存器，并阻断资源重读、输出写入和文字复制。三个分支的目标token与定义参数均由测试逐项锁定，特别覆盖selector 2的资源高word保留。

## 4. loader后资源重读与固定输出

loader返回后，函数必须重新读取actor `+0x0C`资源token，不能复用loader前快照。三个selector随后都读取重读资源的`+0x66`word，并把该word零扩展为dword写入第三参数；即使selector 1或2最初加载的是`+0x6A`或`+0x6E`定义，发布值仍固定来自当前资源`+0x66`。

三个分支保留不同寄存器顺序：

- selector 0以`EDX`承接重读资源、`ECX`承接输出目标，并清`EAX`后只写`AX`；
- selector 1以`EAX`承接重读资源、`EDX`承接输出目标，并清`ECX`后只写`CX`；
- selector 2以`ECX`承接重读资源、`EAX`承接输出目标，并清`EDX`后只写`DX`。

资源在loader期间失效时，只能在这次真实`+0x66`读取点typed-stop，并保留loader调用及此前内嵌定义副作用。输出目标缺失只在原始dword写入点停止，不能提前阻断资源重读。

## 5. 名称复制与返回寄存器

输出写入完成后，三个分支都以第二参数为`lstrcpyA`目标、actor `+0x10`为源复制加载后的名称。复制按原导入函数的可观察逐字节访问建模：从定义首byte开始，源和目标每个byte都在访问前检查；复制包括首个NUL。任一侧在首次越界时立即`name_copy_typed_stop`，保留此前已经复制的前缀，不静默截断，也不补写额外终止符。

导入边界的typed-stop同样保留此前资源重读与输出dword。正常复制后，原函数无条件覆盖完整`EAX=1`并返回；非法selector、零定义word和其他正常失败路径覆盖完整`EAX=0`。测试覆盖导入返回寄存器、逐字节前缀、源定义无NUL、目标不足和成功NUL复制。

## 6. typed owner与两个caller回收

组Bactor、动态资源token/164-byte资源和内嵌定义继续借用`LegacyBattleStartupState::group_b_lifecycle`中的八槽唯一owner。共享文字目标继续使用既有选择文字workspace；定义加载和文字复制通过frame coordinator追加的两个窄调用统一适配，没有建立平行资源或文字owner。

目标选择进入函数的三次旧主选项opaque扫描已全部替换为typed直连，原枚举数值保留为`reserved_target_selection_scan_primary_slot`且生产零调用。每次完整`EAX=1`才增加主选项可用计数；actor/resource停止映射到既有组Bactor停止，loader、重读、输出或复制停止映射到新增主选项停止并阻断余下扫描与阶段后缀。

控制面板逐帧函数的三次旧主选项opaque查询也已替换为同一typed函数，原槽保留为`reserved_query_primary_option_slot`且生产零调用。三行扫描顺序、特殊选项旧路径、选中行发布和后续边框/文字流程不变；typed-stop保留此前面板前缀并阻断余下主选项和绘制后缀。frame coordinator、消息阶段和选择帧均从startup唯一owner注入同一组Bactor span。

## 7. 验证与动态阻塞

纯函数测试覆盖非法selector、actor/resource首次故障、三个零定义路径、三类loader ABI、selector 2高word、loader期间资源变化、loader payload/typed-stop、资源重读typed-stop、固定`+0x66`输出、输出写入typed-stop、导入typed-stop、逐字节复制前缀、源无NUL越界和正常完整返回。

目标选择、控制面板、输入分发、选择帧和frame coordinator集成测试覆盖唯一owner注入、三个主选项完整`EAX`门、旧opaque槽零调用、定义/文字窄端口透传及typed-stop后缀阻断。战斗聚合定向测试、完整core AddressSanitizer`188/188`、Linux core`188/188`和Linux app`194/194`全部通过；最终日志无OpenSWD3源码warning、测试失败或sanitizer finding。新增文件全量及历史文件本包触碰行均通过clang-format Werror门。

当前缺少原版组Bactor、动态资源三处定义word、`0x00476DB0`真实定义加载副作用、内嵌定义名称、`lstrcpyA`逐字节访问、共享输出目标和两个caller寄存器的联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。该限制不影响完整静态闭环和Linux验证。
