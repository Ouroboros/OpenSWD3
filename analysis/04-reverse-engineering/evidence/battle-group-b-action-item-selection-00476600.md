# 战斗组B行动道具随机选择 `0x00476600`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整权威范围与ABI

权威LST主体为`0x00476600..0x00476776`，从proc到endp共178行、120条带机器码的实际指令、6个call、21个跳转、10个局部标签和3个返回点，没有外部`FUNCTION CHUNK`。六个call由四次已关闭第二套有界随机`0x00439070`和两次待审定义加载器`0x00476DB0`组成。

函数是thiscall。入口`ECX`为组B actor，两个栈参数分别按u16和u8读取；callee以`retn 8`弹出参数。唯一静态caller位于已关闭战斗消息99：组A角色资料查询返回值作为第一个参数，动作资料首byte作为第二个参数，控制pair高word选择组B actor。caller只比较返回`AX`是否等于零；返回高word不得被错误地纳入成功门。

## 2. 入口门、随机上限与选择值

函数先固定`EAX=1`，再测试actor `+0x26D0` word的bit 5。actor缺失只能在该首次访问点停止；bit 5清零时不消费随机并以`EAX=0`返回。

bit 5置位时，第一参数只读取低u16并选择首次随机上限：

- `0x0032..0x004F`使用2；
- `0x0050..0x0064`使用3；
- 其余值使用1。

`cmp cx, 0x28`后的`ja $+2`目标就是下一条指令，不改变控制流。首次随机返回先执行完整32位`inc`，再`and 0xFFFF`，最后完整32位`dec`。正常结果等价于低u16；低u16为`0xFFFF`时结果为`0xFFFFFFFF`。随后连续两次`dec`只接受选择0、1、2；其他值最终只清`AX`，保留当时`EAX`高word。

## 3. 三类动态资源与概率门

选择0、1、2分别读取动态164-byte资源的`+0x66`、`+0x6A`、`+0x6E` word。首次随机已经消费后才读取actor `+0x0C`资源token；token缺失在所选word的首次实际读取点停止。所选word为零时不消费第二次随机，返回时只清资源token的`AX`。

所选word非零后，把第二参数u8零扩展到`CX`或`DX`低word，并与动态资源`+0x54`按无符号u16比较：参数严格小于资源word时阈值为60，否则为90。第二次随机固定使用上限100，只有返回`AX`严格小于阈值才继续；相等失败。失败返回只清`AX`，保留随机返回的高word。

第二次随机之后必须重新读取actor资源token，并从新token对应的资源再次读取所选word。选择0/2把重读token放入`EDX`；选择1把重读token放入`EAX`并先形成actor内嵌定义目标token。token在随机期间变零时，只能在第二次所选word读取点停止，且不得回滚两次随机消费。

## 4. 待审定义加载器边界

`0x00476DB0`整体仍是后续待审定义加载器，本工作包只保留一个窄端口。两个物理参数始终为actor `+0x10`内嵌164-byte定义目标和重读的定义编号，但三类选择保留不同寄存器ABI：

- 选择0/2：`EAX`为第二次随机高word与定义编号低word的组合，`ECX=actor+0x10`，`EDX`为重读资源token；
- 选择1：`EAX`为重读资源token，`EDX=actor+0x10`，`ECX`只覆盖定义编号低word，高word保持第二次随机callee后的未知值。

端口正常返回可发布完整164-byte内嵌定义；发布发生在typed-stop判断前。loader typed-stop保留其字节与返回寄存器，阻断actor一次性bit清除和全部返回道具判定。

## 5. 加载后定义门与返回值

loader正常返回后，函数从内嵌定义`+0x20`读取完整dword到`EAX`，随后无条件清actor `+0x26D0`的bit 5。定义dword的bit 27清零时只清`AX`并返回，保留定义dword高word。bit 27置位时，以内嵌定义`+0x48` word覆盖`AX`并返回；该低word就是caller继续加入玩家道具链的编号。

清bit发生在定义dword读取之后、bit 27测试之前。概率失败、资源缺失、定义编号为零和loader typed-stop都不得提前清bit。

## 6. typed owner与caller回收

组B actor、动态资源token和164-byte资源继续复用`LegacyBattleStartupState::group_b_lifecycle`唯一owner；actor `+0x10`内嵌定义复用`action_composition.resource_definition`；`+0x26D0`继续由`action_execution.retreat_ready_flags`承接。两次有界随机复用胜利奖励绑定中的第二套随机owner，不建立新游标。

消息99删除旧整函数opaque调用，原枚举数值改为reserved并保持生产零调用。新窄定义加载调用追加到消息枚举和frame协调器枚举末尾。typed helper正常返回低`AX=0`时，caller仍写aux 2并保留完整高word；低`AX`非零时才直连玩家道具数量并递减特殊行动计数。actor、资源重读或loader typed-stop都保留消息99此前的角色重建、动作提交和资料查询前缀，同时阻断道具发布、aux写入和计数递减。

## 7. 验证与动态阻塞

纯函数测试覆盖入口actor门、bit 5旁路、全部首次随机上限区间、`0xFFFF`归一化回绕、三类选择、首次资源故障点、零定义编号、60/90阈值、严格小于、随机后资源token及定义编号重读、三类loader ABI、选择1未知`ECX`高word、loader字节发布/typed-stop、bit 27正反路径和仅清`AX`的返回高word。

消息99集成测试覆盖真实组B owner、两次随机、旧整函数槽零调用、新loader ABI、道具成功发布、loader typed-stop后缀阻断及完整`EAX`高word非零但低`AX`为零的失败分支。frame协调器测试覆盖新窄调用、参数、寄存器、定义payload和typed-stop透传。战斗聚合定向测试、完整core AddressSanitizer`188/188`、Linux core`188/188`和Linux app`194/194`全部通过，三份最终日志无OpenSWD3源码warning、测试失败或sanitizer finding。

当前缺少原版组B actor、动态资源三组定义word、第二套随机种子/游标、`0x00476DB0`真实定义加载副作用、内嵌定义和消息99调用方寄存器的联合捕获后端，因此`original_diff_verified`登记为`blocked_runtime_oracle`。该限制不影响完整静态闭环和Linux验证。
