# 战斗逐帧输入与指令分派 `0x0045F2A0`

状态：`platform_adapted`、`unit_tested`、`fixed_state_tested`、`caller_reclaimed`。

## 1. 完整范围

权威LST完整主体为`0x0045F2A0..0x0045FC5F`，从proc到endp共1176行、868条实际指令、54个call、129个跳转、71个局部标签，没有外部`FUNCTION CHUNK`。

函数无栈参数，入口先把菜单动作dword清零；渲染中止dword完整等于1时立即返回入口EAX。其余路径保留EBX=1、EBP=0和路径相关EAX/ECX/EDX，普通返回不被唯一caller用作成功门。

54个callsite完整归类为8次已关闭DIK byte查询、40次战斗动作/选择callee、2次已关闭热点计数、1次已关闭热点链清理、2次固定Sleep和1次已关闭样本播放。17类尚未关闭的战斗callee由单一typed输入端口表达；热点计数/清理直接操作唯一vector owner，DIK直接读取256字节typed快照。

## 2. 八个直接指令键

只有共享消息值signed小于2、live active actor非零且对话消息链为空时，才按DIK 2到9固定顺序查询。按键2/3/4/5和7/8/9先把零消息置1，再直连已关闭动作模式刷新，并在普通返回后实时读取启动reset中`0x00524414..0x0052441B`对应permission byte；不再保留刷新前快照。byte不精确等于1时立即保留刷新返回寄存器结束，刷新typed-stop则阻断后续动作、gate、selection与目标选择发布。

permission为1时写动作kind 6、pre-frame gate A 1和selection `1/2/3/4/6/7/8`，调用selection commit后立即返回。按键6是原始特殊切换：selection不等于5时才发布kind/gate/selection5并commit，随后无条件把消息、gate清零且selection重置1；原本已为5时不调用任何战斗callee。

## 3. 输入记录与重复节拍

后半直接复用`LegacyInputNormalizationState::records`的20条精确`0x10`记录，读取的固定索引为0、1、2、3、4、5、6、7、8、9、12、14、15、17、18。rapid multiplicity是`+0x00`，held sample count是`+0x0C`。

多数动作接受held等于1，或held按i32 signed大于等于15且`idiv 3`余数等于1；基础确认改用除数6。左右列表、上下模式和最终动作另有不带15门的固定`idiv 3`。所有除法都保留quotient/remainder寄存器与held=1跳过装载除数时的陈旧ECX，不把signed比较改成unsigned。

记录9和14/15/12会按原时点把record0的rapid写1并复制live held。基础确认随后只检查record0 rapid，但节拍使用入口record1 held或本帧合成后留下的陈旧ESI，不重新读取record0 held。

输入span过短时只在下一次真实固定记录访问typed-stop，保留此前菜单清零、DIK调用、记录合成和共享写。

## 4. 撤退准备与警告

记录18节拍命中后依次检查阻断word、动作块门、角色重定向门、消息99/100和对话链。随后先以u32回绕计算`active actor-8`，再按组A stride计算对象token并查询状态；组B数量减排除低byte精确等于1且目标word非全1时也直接返回。

active actor非零时再次查询对象。查询零或battle mode bit`0x200`置位时固定Sleep 20ms，以五参数直连共享文字消息入链，再以样本`0x8C`和live signed混音等级播放，并返回样本完整寄存器；文字typed-stop阻断样本及后续输入分支。

允许撤退时固定Sleep 50ms，先写消息17；原地址`0x0053AF58[(active-8)*4]`折叠到唯一共享数组的物理`opponent_workspace[active+2]`并写17，再以`active-8`组Atoken调用角色配置；随后动态重读active并依次发布辅助门、secondary、消息0、动作word低word0、published `active-7`、阻断word bit`0x4000`、active0、pre-frame gate A 0、双frame gate 1和尾部`0/0/4`。workspace索引按u32回绕且只在原store typed-stop，消息17已经可见并且不调用角色配置。

## 5. 确认、鼠标与热点链

记录17命中时按原顺序清pre-frame gate B、置terminal与action execution active；原地址`0x0053AF38[active*4]`同样折叠到唯一共享数组的`opponent_workspace[active+2]`并写1，成功后置gate A、selection1和消息3并立即返回。

记录14把prompt计数清零、置gate B并合成record0。记录15先以option替换EAX低word再置gate B；selected option word非全1时按i16符号扩展并直连typed动作提交，普通返回后才按消息1补一次selection commit并把word恢复全1；队列或角色typed-stop保留option与gate并阻断两项尾操作。否则清prompt计数，按interaction mode 1到4分别调用四类callee且每次动态重读record15 held；鼠标Y位于两个unsigned开区间且held精确1时发布Y，独立mouse gate为1且held signed正时把菜单动作写5。

基础确认处理signed status bit0、按gate B和热点门选择是否直接清唯一热点vector及对话sentinel；record15 rapid为0时清gate B、把prompt计数写300、置gate A并清interaction mode。battle mode只清低byte bit`0x20`，最后commit selection。

记录3/5在热点链非空时分别按低word bit15回绕后退和按完整u32上界前进；计数直接使用已关闭热点计数语义。记录3在反向角色轮转普通返回后直连菜单上下文后退，typed-stop保留轮转并阻断后续记录；记录5先直连菜单上下文前进，普通返回后才轮转角色动作，typed-stop阻断轮转。记录4/6/7/8和最终record0按原顺序调用模式、确认与方向callee，最终成功后只清两个尾dword。

## 6. caller回收与共享owner

唯一caller是逐帧画面协调器。旧第二前置opaque stage已改为reserved槽；第一个`0x0045FC60`前置stage返回的ECX/EDX直接作为本函数入口snapshot。本函数普通返回无论0或非零都继续已关闭角色预处理，并把完整返回ECX/EDX传给该callee；输入记录或workspace typed-stop则保留第一前置stage与本函数前缀，阻断角色预处理、metric、surface和全部后续帧。

316生产入口逐步接线时，从LST `0x0045F2A0`先读渲染中止latch、`0x0045F2B2`无条件清菜单动作`dword_53BD9C`及`0x0045F2B8`精确等于1的短路门提取共用前缀。SDL复用既有协调器`render_abort_latch`和输入状态`menu_action`；latch等于1则在上层`0x00453243→sub_45D490`前typed-stop，其余值在读取真实SDL消息、排队角色及共享对话链后按signed消息小于2、角色非零、链为空的三个门分流：满足时按LST依序查询同一SDL键盘快照的DIK2..9，首个按下的键分别在`0x0045F2FE/0x0045F351/0x0045F3A3/0x0045F3FE/0x0045F451/0x0045F4AD/0x0045F4FF/0x0045F556`变更门前typed-stop；全未按下及未进入键盘分支则借用SDL归一化输入的记录1以及唯一输入状态的`input_gate`，按`0x0045F5A3..C4`三个原版比较执行`dword_53BDA4 |= 1`，继而按`0x0045F5C4..E0`读取记录9：rapid非零且held有符号正时把记录0 rapid写1、held写原值，在`0x0045F5E0`记录2首读前typed-stop（该版实际SDL回调见下）。随后新增记录2共享前缀：`0x0045F5E0..0x0045F629`的rapid／held读取、signed阈值与除3余1门、signed消息大于1早退、动作前选项及action kind写入；继续路径在`0x0045F629`记录18首读前、早退路径在`0x0045FC5B`前、动作路径在`0x0045F61D→sub_462320`前typed-stop。LST `0x0045F602 33 ED`明确清BP，所以held=16时`0x0045F610`写入的选项word是0，不是先前实现错误的3；定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过，尚无新SDL回调运行。继续路径的记录18前缀`0x0045F629..0x0045F672`也共用：rapid为零或signed重复门不成立时在`0x0045F806`记录17首读前停止；活跃门先检查已绑定的retreat word／action gate，需早退时在`0x0045FC5B`前停止，否则在未绑定SDL owner的`0x0045F672`debug gate首读前停止，不假装已执行该读取；core顺序补齐`0x0045F67E`加载消息EAX，消息99/100早退的EAX已由新定向反例验证。记录18零rapid及非触发路径再按LST依序只读记录17、14、15、12、1、4、6、3、5、7、8、0的rapid；遇非零即在其后续首个非默认物理指令前typed-stop，12项全零也只停在`0x0045FC5B`RET前，不伪造input-dispatch正常返回。每个非零位置及全零路径的定向core 1/1、Linux core／ASan各200/200及Linux／Windows app各206/206通过；随后用户第二次实际回调日志`build/vm/wp316-sdl-frame-user-run-v2/openswd3-2026-10-03_19-14-31-66208.log`显示record2/18 rapid及held均0、`idle_records_inspected=12`，首个物理停点为`0x0045FC5B`输入分派RET前，脚本status=32、opcode=1、offset=37后进程正常停止。源与副本SHA256同为`a14973592137527791bd614ee17621be6ddf9080e74de5764df3bef1d92cc7ac`；这不证明正常RET、下游预帧或角色帧执行。记录9新前缀定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过；记录1新前缀定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过。键盘查询前缀该版定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过；不能虚构普通全帧返回。此前`LegacyBattleTargetSelectionRuntimeState::candidate_gate_a`重复标注原地址`0x0053BD9C`，现已删去第二份C++状态，目标选择刷新、选择面板及全局重置共用输入状态的`menu_action`；同址修正的定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过。另按LST `0x0045F5BE`与`0x00462A5B`，将旧目标刷新独立`selection_aux_gate`并入输入状态`input_latch`，全局重置不清该dword；该合并的定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过。尚无跨模块实际帧调用差分。该版定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，`clang-format --dry-run --Werror`和`git diff --check`通过；新增有符号消息三门分流的该版定向core 1/1、完整Linux core／ASan各200/200及app206/206通过，格式与diff检查通过。独立Windows开场战斗实际日志`build/vm/wp316-sdl-frame-user-run/openswd3-2026-10-03_18-15-50-20916.log`记录frame回调顺序执行到本函数记录2首读`0x0045F5E0`前，`keyboard_queries=0`、`record_zero_written=0`、共享menu/latch/记录0前后均为0，随后脚本以typed-stop结束；该运行不覆盖非默认输入或角色帧。

实现直接复用启动permission、最终角色、动作workspace、角色数量、共享消息/terminal、调试mode、context prompt、输入记录、键盘、对话消息和热点链owner。动作刷新新增的live相邻查找复用action kind、published actor、目标/列表选择owner，并只为`0x004A7554`保留一个独立typed值；`0x004A75C8`映射复用启动状态单一十dword物理视图。其余尚未命名输入全局由单一`LegacyBattleInputDispatchStatePort`持有；全局重置只同步LST原写集合，未被reset触碰的输入值保持入口值。

## 7. 验证与动态差分

定向测试覆盖渲染中止、八键顺序与刷新后live permission、动作刷新普通/typed-stop传播、按键6特殊复位、记录合成、signed重复节拍、陈旧ESI、撤退20/50ms双路径、警告与signed混音、workspace停点、记录17立即返回、selected option、鼠标开区间、热点清理与左右回绕、mode bit局部清除、输入span停点、全局重置别名及逐帧caller阻断。

## 8. `0x00478330`撤退角色写入

工作包278关闭`0x0045F765`唯一物理call。撤退确认路径在发布角色code后，以`(actor_code-8)*0xBCD`形成EDX并把完整dword `1`写入对应组A角色`+0x2AE4`；typed实现复用最终角色状态中的单一availability owner。写停止时EAX已为1，ECX为角色token，EDX为原乘法结果，随后的消息、选择门和动作状态提交全部被阻断。

当前缺少原版20条输入记录与DIK联合轨迹、其余战斗callee共享副作用、两组角色对象、对话/热点链、Sleep墙钟、样本后端、全部输入全局及EAX/ECX/EDX联合捕获后端，`original_diff_verified`为`blocked_runtime_oracle`。
