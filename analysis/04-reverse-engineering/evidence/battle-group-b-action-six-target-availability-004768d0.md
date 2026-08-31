# 战斗组B行动六目标可用性 `0x004768D0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围与ABI

权威LST主体为`0x004768D0..0x004768F0`，从proc到endp共24行、15条带机器码且含助记符的实际指令、0个call、2个跳转、2个局部标签和3个返回点，没有外部`FUNCTION CHUNK`。

函数是无栈参数thiscall。入口`ECX`为组Bactor；正常路径和两个早退都以普通`retn`返回。两个静态caller分别位于已关闭的战斗帧鼠标输入与目标解析函数和战斗选择帧函数；两处都对完整`EAX`作零/非零测试。

## 2. 动态资源、状态位与阈值

函数先读取actor `+0x0C`的动态资源token，再读取资源`+0x20`的完整dword。若该dword低byte的bit5已置位，立即清完整`EAX`并返回零。bit5未置位时继续测试原dword高byte的bit3，即完整dword的bit11；该位未置位同样清完整`EAX`并返回零。

两项位门都通过后，函数只把`AX`改为常量21，再与同一资源`+0x52`的word作unsigned比较。`cmp`后的`SBB EAX,EAX; INC EAX`把结果规范为完整32位零或一：资源word无符号小于等于21返回一，大于21返回零。资源dword的高word虽在`mov AX,21`时暂时保留，但正常比较后被`SBB/INC`完整覆盖，不能泄露到正常返回。

## 3. 寄存器与typed-stop

actor缺失只在首次`[ECX+0x0C]`访问点停止，保留入口`EAX/ECX/EDX`。资源token缺失只在首次资源`+0x20`访问点停止；此时`ECX`已经是零资源token，`EAX/EDX`仍为入口值。

正常读取资源后，`ECX`保持资源token，`EDX`全程不改。bit5或bit11早退都返回完整`EAX=0`；阈值路径返回完整`EAX=0/1`。现代资源owner是固定164-byte完整记录，故同一非零资源token内的`+0x20`和`+0x52`均在owner范围；不添加无汇编来源的额外空值门。

## 4. 帧输入caller回收

战斗帧鼠标输入与目标解析函数在组B像素命中、目标发布和模式1配置完成后，仅当真实行动种类dword等于6时查询本函数。既有实现错误使用了另一个选择索引字段作为门；本工作包按caller LST改回行动种类。

原`query_group_b_mode`整函数opaque调用已删除，枚举数值保留为`reserved_query_group_b_action_six_target_availability_slot`，frame coordinator对应值也改为reserved，生产零调用。typed函数直接借用startup组B生命周期owner；返回零才清此前像素扫描置一的目标可用标记，返回一保留该值。actor/resource typed-stop保留已发布目标、像素命中、配置与可用标记前缀，并阻断本帧成功返回后缀。

## 5. 选择帧caller补全

选择帧在当前组B目标完成后轮转target cursor，发布新目标，再调用原`0x00478400`完成查询。该完成查询返回非一时，原LST仍在行动种类等于6时先把目标可用标记置一，再调用本函数；返回零将标记清零，返回一保留一。

既有typed选择帧此前遗漏了这次调用，而不是为它保留独立opaque枚举。本工作包在真实完成查询之后补入typed直连；`query_group_b_completion`的三个现有callsite均仍对应原`0x00478400`，不错误回收或改名。调用前`EAX`按原地址计算保持为actor索引乘`0x565`，`EDX`保持为actor索引乘`0x159`，`ECX`为actor token。typed-stop保留target cursor、新目标、published actor和可用值一，并阻断当前目标标记、提示框及选择帧余下后缀。

## 6. owner、验证与动态阻塞

actor、资源token和164-byte资源继续复用`LegacyBattleStartupState::group_b_lifecycle`八槽唯一owner；目标可用标记继续复用帧输入state既有owner，不建立平行状态或新端口。

纯函数测试覆盖actor/resource首次故障、bit5优先早退、bit11缺失早退、阈值0、21、22和`0xFFFF`、资源高word覆盖及完整返回寄存器。两个caller集成测试覆盖真实行动种类门、错误选择索引不触发、旧frame-input opaque槽零调用、像素/目标前缀、选择帧三次真实完成查询不变、阈值双结果及typed-stop后缀阻断。

战斗聚合测试、完整core AddressSanitizer`188/188`、Linux core`188/188`和Linux app`194/194`全部通过；最终日志无OpenSWD3源码warning、测试失败或sanitizer finding。新增文件全量及历史文件本包触碰行均通过clang-format Werror门。

当前缺少原版组Bactor、动态资源状态dword、阈值word、两个caller目标发布前缀与EAX/ECX/EDX联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。该限制不影响完整静态闭环和Linux验证。
